/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev, frame_index;
	unsigned int pdbr;
	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}

	for (frame_index = 0; frame_index < NFRAMES; frame_index++) {
		if (frm_tab[frame_index].fr_status == FRM_MAPPED && frm_tab[frame_index].fr_pid == pid) {
			if (frm_tab[frame_index].fr_type == FR_PAGE) {
				//kprintf("clearing frame Index: %d\n", frame_index);
				remove_from_list(frame_index);
				frm_tab[frame_index].fr_status = FRM_UNMAPPED;
				//free_frm(frame_index);
			}
		}
	}

	kprintf("The pid of the process is :%d\n", pid);
	int store;
	for (store = 0; store < 8; store++) {
		if (bsm_tab[store].bs_status == BSM_MAPPED && bsm_tab[store].bs_pid == pid) {
			bsm_tab[store].bs_pid = BADPID; // -1
			bsm_tab[store].bs_status = BSM_UNMAPPED;
			bsm_tab[store].bs_vpno = -1;
			bsm_tab[store].bs_npages = 0;
			bsm_tab[store].bs_sem = -1;
			bsm_tab[store].bs_private_heap = 0;
		}
	}

	//int store = proctab[pid].store;
	//release_bs(store); //clearing the store ID


	pdbr = proctab[pid].pdbr;
	frame_index = (pdbr / NBPG) - FRAME0;
	//kprintf("Clearing the page directory of the pid: %d at frame index: %d", pid, frame_index);
	frm_tab[frame_index].fr_status = FRM_UNMAPPED;
	frm_tab[frame_index].fr_pid = BADPID;
	frm_tab[frame_index].fr_vpno = -1;
	frm_tab[frame_index].fr_refcnt = 0;
	frm_tab[frame_index].fr_type = -1;
	frm_tab[frame_index].fr_dirty = 0;
	frm_tab[frame_index].fr_age = 0;

	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}

	restore(ps);
	return(OK);
}
