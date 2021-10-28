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
	int frameIndex;
	for (frameIndex = 0; frameIndex < NFRAMES; frameIndex++) {
	        if (frm_tab[frameIndex].fr_status == FRM_UNMAPPED) {
			*avail = frameIndex;
			return OK;
		}
	}
    return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

  kprintf("To be implemented!\n");
  return OK;
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
			freemem(temp, sizeof(list_node));
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
