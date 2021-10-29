/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	STATWORD ps;
	disable(ps);
	int frameIndex;
	init_list(); //initializing list for page replacment.
	for (frameIndex = 0; frameIndex < NFRAMES; frameIndex++) {
		frm_tab[frameIndex].fr_status = FRM_UNMAPPED;
		frm_tab[frameIndex].fr_pid = BADPID;
		frm_tab[frameIndex].fr_vpno = -1;
		frm_tab[frameIndex].fr_refcnt = 0;
		frm_tab[frameIndex].fr_type = -1;
		frm_tab[frameIndex].fr_dirty = 0;
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	STATWORD ps;
	disable(ps);
	int frameIndex, evt_frame;
	for (frameIndex = 0; frameIndex < 12; frameIndex++) {
	        if (frm_tab[frameIndex].fr_status == FRM_UNMAPPED) {
			*avail = frameIndex;
			restore(ps);
			return OK;
		}
	}
	// currently no frame is available have to pick a frame
	evt_frame = pick_frame(); // have to free this frame before returning
	if (evt_frame == -1) {
		kprintf("Unable to find the free frame");
		restore(ps);
		return SYSERR;
	}
	free_frm(evt_frame);
	restore(ps);
	return evt_frame;

}


unsigned int init_pd(int pid) {

	int free_frame_no;
	unsigned int pd_base;
	get_frm(&free_frame_no); // Getting next free frame for pid's Page Directory 
       pd_base = ((FRAME0 + free_frame_no) * NBPG);
	frm_tab[free_frame_no].fr_type = FR_DIR;
	frm_tab[free_frame_no].fr_pid = pid;
	frm_tab[free_frame_no].fr_status = FRM_MAPPED;
	frm_tab[free_frame_no].fr_dirty = 0;
	frm_tab[free_frame_no].fr_vpno = 0;
	frm_tab[free_frame_no].fr_refcnt = 4;
	return pd_base;
}

void alloc_pd(unsigned int pd_base) {

	int dir_index;
	pd_t* pde = (pd_t *) pd_base;

	for (dir_index = 0 ; dir_index < 1024; dir_index++) {
		pde->pd_pres = 0;
		pde->pd_write = 1; // set this always to one
		pde->pd_user = 0;
		pde->pd_pwt = 0;
		pde->pd_pcd = 0;
		pde->pd_acc = 0;
		pde->pd_mbz = 0;
		pde->pd_fmb = 0;
		pde->pd_global = 0;
		pde->pd_avail = 0;
		pde->pd_base = 0;
		if (dir_index >= 0 && dir_index < GLOBALPAGES) {
			pde->pd_pres = 1; // only when being used
			pde->pd_base = FRAME0 + dir_index;
		}
		pde++;
	}
	return;
}

void init_list() {
	list_node* head = (list_node *)getmem(sizeof(list_node));
	head->frame_index = -1;
	head->next = head;
	head->prev = head;
	size = 0;
}

void insert_into_list(int frame_index) {
	list_node* last_node = head;
	while (last_node->next != head) {
		last_node = last_node->next;
	}
	list_node* new_node = (list_node *)getmem(sizeof(list_node));
	new_node->prev = last_node;
	new_node->next = last_node->next;
	new_node->frame_index = frame_index;
	last_node->next->prev = new_node; // changing the head's prev to the new value
	last_node->next = new_node;
	size++; // increasing the size of the list
}

void remove_from_list(int frame_index) {
	if (frame_index == -1) {
		return; //cant remove the head
	}
	list_node * temp = head->next;
	while (temp != head) {
		if (temp->frame_index == frame_index) {
			list_node * prev_node = temp->prev;
			list_node* next_node = temp->next;
			prev_node->next = next_node;
			next_node->prev = prev_node;
			freemem((struct mblock* )temp, sizeof(list_node));
			size--;
		}
		else {
			temp = temp->next;
		}
	}
}


void display_list() {
	STATWORD ps;
	disable(ps);
	list_node* temp = head->next;
	while (temp != head) {
		kprintf("Frame Index: %d \n", temp->frame_index);
		temp = temp->next;
	}
	restore(ps);
	return;
}


int pick_frame() {
	int frame_index;
	int policy = grpolicy(); // we need page replacement;
	kprintf("\nInside pick_frame\n");
	if (policy == SC) {
		frame_index = get_frame_using_sc();
		kprintf("The index selected for eviction is :%d", frame_index);
		return frame_index;
	}
	else if (policy == AGING) {
		return get_frame_using_aging();
	}
	return -1;
}


/*-------------------------------------------------------------------------
 * free_frm - free a frame
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
	STATWORD ps;
	disable(ps);
	int access_bit, frame_index, pid, status, type, ret_val, pageth, store, table_index;
	unsigned int pdbr, address;
	virt_addr_t* virtual_address;
	pd_t* pde;
	pt_t* pte;
	kprintf("Inside free frame\n");
	status = frm_tab[i].fr_status;
	pid = frm_tab[i].fr_pid;
	pdbr = proctab[pid].pdbr;
	type = frm_tab[i].fr_type;
	address = frm_tab[frame_index].fr_vpno * NBPG;
	virtual_address = (virt_addr_t*)&address;
	pde = (pd_t*)(pdbr + (sizeof(pd_t) * virtual_address->pd_offset));
	pte = (pt_t*)((pde->pd_base * NBPG) + (sizeof(pt_t) * virtual_address->pt_offset));
	if (status) {
		if (type == FR_PAGE) {
			ret_val = bsm_lookup(pid, address, &store, &pageth);
			if (ret_val == SYSERR) {
				kprintf("Faulted Address: %d\n",address);
				return SYSERR;
			}
			else {
				
				table_index = pde->pd_base - FRAME0;
				kprintf("The table frame index of the evicted page is", table_index);
				if (frm_tab[table_index].fr_type == FR_TBL) {
					if (frm_tab[table_index].fr_refcnt > 0) {
						frm_tab[table_index].fr_refcnt--;
						if (frm_tab[table_index].fr_refcnt == 0) { // remove the frame of the corresponding page directory entry
							frm_tab[table_index].fr_status = FRM_UNMAPPED; //removing the entry of i from frm_tab
							frm_tab[table_index].fr_pid = BADPID;
							frm_tab[table_index].fr_vpno = -1;
							frm_tab[table_index].fr_refcnt = 0;
							frm_tab[table_index].fr_type = -1;
							frm_tab[table_index].fr_dirty = 0;
							pde->pd_pres = 0;
						}
					}
				}
				char* wr_strt_addr = (char*)((i + FRAME0) * NBPG);
				write_bs(wr_strt_addr, store, pageth);
				frm_tab[i].fr_status = FRM_UNMAPPED; //removing the entry of i from frm_tab
				frm_tab[i].fr_pid = BADPID;
				frm_tab[i].fr_vpno = -1;
				frm_tab[i].fr_refcnt = 0;
				frm_tab[i].fr_type = -1;
				frm_tab[i].fr_dirty = 0;
				pte->pt_pres = 0; // setting the page table entry bit to 0
				write_cr3(pdbr); // resetting the TLB cache by resetting the page directory 
			}
		}
	}
	restore(ps);
	return OK;
}

int  get_frame_using_sc() {
	int access_bit, frame_index,pid, picked_frame = -1, tries = 2;
	unsigned int pdbr, address;
	virt_addr_t* virtual_address;
	pd_t* pde;
	pt_t* pte;
	kprintf("\nInside SC Eviction\n");
	while (tries > 0) {
		list_node* temp = head->next;
		while (temp != head) {

			frame_index = temp->frame_index;
			kprintf("The current frame index is: %d\n", frame_index);
			pid = frm_tab[frame_index].fr_pid;
			address = frm_tab[frame_index].fr_vpno * NBPG;
			virtual_address = (virt_addr_t*)&address;
			pdbr = proctab[pid].pdbr;
			pde = (pd_t*)(pdbr + (sizeof(pd_t) * virtual_address->pd_offset));
			pte = (pt_t*)((pde->pd_base * NBPG) + (sizeof(pt_t) * virtual_address->pt_offset));

			if (pte->pt_acc) {
				kprintf("The access bit for the frame: %d is set and being cleared", frame_index);
				pte->pt_acc = 0;
			}
			else {
				// we will be choosing this frame.
				picked_frame = temp->frame_index;
				kprintf("\nReaching here, choosing the frame: %d for eviction\n", picked_frame);
				remove_from_list(temp->frame_index);
				return picked_frame;
			}
			temp = temp->next;
		}
		tries--;
	}
	return -1;
}

int  get_frame_using_aging() {

	return -1;

}
