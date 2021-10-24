/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);
	int storeIndex;
	for (storeIndex = 0; storeIndex < 8; storeIndex++) {
		bsm_tab[storeIndex].bs_pid = BADPID; // -1
		bsm_tab[storeIndex].bs_status = BSM_UNMAPPED;
		bsm_tab[storeIndex].bs_vpno = -1;
		bsm_tab[storeIndex].bs_npages = 0;
		bsm_tab[storeIndex].bs_sem = -1;
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	disable(ps);
	int storeIndex;

	if (*avail == NULL) {
		restore(ps);
		return SYSERR;
	}

	for (storeIndex = 0; storeIndex < 8; storeIndex++) {
		if (bsm_tab[storeIndex] == BSM_UNMAPPED) {
			*avail = storeIndex;
			break;
		}
	}
	restore(ps);
	return OK;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
	disable(ps);
	bsm_tab[storeIndex].bs_pid = BADPID; // -1
	bsm_tab[storeIndex].bs_status = BSM_UNMAPPED;
	bsm_tab[storeIndex].bs_vpno = -1;
	bsm_tab[storeIndex].bs_npages = 0;
	bsm_tab[storeIndex].bs_sem = -1;
	restore(ps);
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD ps;
	disable(ps);
	int storeIndex, flag = 1;
	for (storeIndex = 0; storeIndex < 8; storeIndex++) {
		if (bsm_tab[storeIndex].bs_pid == pid) {
			flag = 0;
			int pageOffset = vaddr / NBPG;  // pageOffset with in a backing store
			int pageMax = bsm_tab[storeIndex].bs_vpno + bsm_tab[storeIndex].bs_npages;
			if (pageOffset < bsm_tab[storeIndex].bs_vpno || pageOffset > pageMax) {
				restore(ps);
				return SYSERR;
			}
			else {
				*store = storeIndex;
				*pageth = pageOffset - bsm_tab[storeIndex].bs_vpno;
				restore(ps);
				return OK;
			}
		}
	}
	if (flag) {
		restore(ps);
		return SYSERR;
	}
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages) //private heap is still pending
{
	STATWORD ps;
	disable(ps);
	bsm_tab[source].bs_pid = pid;
	bsm_tab[source].bs_status = BSM_MAPPED;
	bsm_tab[source].bs_npages = npages;
	bsm_tab[source].bs_vpno = vpno;
	bsm_tab[source].bs_sem = 1;
	restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
	disable ps;
	int storeIndex;
	if (isbadpid(pid)) {
		restore ps;
		return SYSERR;
	}
	storeIndex = proctab[pid].store;
	bsm_tab[storeIndex].bs_pid = BADPID;
	bsm_tab[storeIndex].bs_status = BSM_UNMAPPED;
	bsm_tab[storeIndex].bs_npages = 0;
	bsm_tab[storeIndex].bs_vpno = -1;
	bsm_tab[storeIndex].bs_sem = -1;
}


