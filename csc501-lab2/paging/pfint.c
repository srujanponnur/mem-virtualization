/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{

	STATWORD ps;
	disable(ps);

	unsigned long fault_addr, pdb_val, vp_num;
	int store, page_index, ret_val, pd_index, pt_index, free_frame_index;


	fault_addr = read_cr2(); //read the faulted address

	pdb_val = proctab[currpid].pdbr;
	ret_val = bsm_lookup(currpid, faul_addr, &store, &page_index);

	if (ret_val == SYSERR) {
		kprintf("Illegal Address, killing the process");
		kill(currpid);
	}
	//kprintf("The store value is: %d and The page index is: %d", store, page_index);
	virt_addr_t* temp_addr;
	temp_addr = (virt_addr_t*)&faul_addr;

	pd_index = temp_addr->pd_offset;
	pt_index = temp_addr->pt_offset;
	page_index = temp_addr->pg_offset;

	pd_t* pde = pdb_val + (sizeof(pd_t) * pd_index);  //pth entry of the page directory
	if (!pde->pd_pres) {
		ret_val = get_frm(&free_frame_index);
		if (ret_val == SYSERR) {
			restore(ps);
			return SYSERR;
		}

		frm_tab[free_frame_index].fr_status = FRM_MAPPED;  // adding a frame entry
		frm_tab[free_frame_index].fr_pid = currpid;
		frm_tab[free_frame_index].fr_dirty = 0;
		frm_tab[free_frame_index].fr_type = FR_TBL;
		frm_tab[free_frame_index].fr_pid = currpid;
		frm_tab[free_frame_index].fr_vpno = 0;

		pt_t *pte = (pt_t*)((FRAME0 + free_frame_index) * NBPG);
		int table_index;
		for (table_index = 0; table_index < 1024; table_index++) { // need to place this as common helper function
			pte->pt_pres = 0;
			pte->pt_write = 0;
			pte->pt_user = 0;
			pte->pt_pwt = 0;
			pte->pt_pcd = 0;
			pte->pt_acc = 0;
			pte->pt_dirty = 0;
			pte->pt_mbz = 0;
			pte->pt_global = 0;
			pte->pt_avail = 0;
			pte->pt_base = 0;
			pte++;
		}
		pde->pd_pres = 1;
		pde->pd_write = 1; // set this always to one
		pde->pd_user = 0;
		pde->pd_pwt = 0;
		pde->pd_pcd = 0;
		pde->pd_acc = 0;
		pde->pd_mbz = 0;
		pde->pd_fmb = 0;
		pde->pd_global = 0;
		pde->pd_avail = 0;
		pde->pd_base = FRAME0 + free_frame_index;

	}

	pt_t* pte = (pt_t *)((pde->pd_base * NBPG) + (pt_index * sizeof(pt_t)));

	if (!pte->pt_pres) {

		ret_val = get_frm(&free_frame_index); 
		if (ret_val == SYSERR) {
			restore(ps);
			return SYSERR;
		}

		frm_tab[free_frame_index].fr_status = FRM_MAPPED;  // adding a frame entry
		frm_tab[free_frame_index].fr_pid = currpid;
		frm_tab[free_frame_index].fr_dirty = 0;
		frm_tab[free_frame_index].fr_type = FR_PAGE;
		int table_frame_index = pde->pd_base - FRAME0;
		//kprintf(" Table frame Index is: %d ", table_frame_index);
		frm_tab[table_frame_index].fr_refcnt++; // update reference count of the page table
		frm_tab[free_frame_index].fr_vpno = 0;

		char* start_address = (char*)((FRAME0 + free_frame_index) * NBPG); // starting address of the frame
		read_bs(start_address, store, page_index); // writing from store to the free frame 

		pte->pt_pres = 1;
		pte->pt_write = 1;
		pte->pt_base = FRAME0 + free_frame_index; //pointing page table entry to point to free frame

		// --------------------------------  need  to update few page replacement related datastructures  ---------------------------------

	}

	restore(ps);
	return OK;
}


