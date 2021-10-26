/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	int store;
	if ((hsize < 1 || hsize > 256) || get_bsm(&store) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	else {
		int pid = create(procaddr, ssize, priority, name, nargs, args);
		if (pid == SYSERR) {
			restore(ps);
			return SYSERR;
		}
		int ret = bsm_map(pid, 4096, store, hsize);
		bsm_tab[store].bs_private_heap = 1;  //setting this backing store index as a private heap 
		if (ret == SYSERR) {
			restore(ps);
			return SYSERR;
		}
                kprintf("The store value: %d and the starting address is: %d\n", store, BACKING_STORE_BASE + store * BACKING_STORE_UNIT_SIZE);
		proctab[pid].store = store;
		proctab[pid].vhpno = 4096;                  
		proctab[pid].vhpnpages = hsize;
		proctab[pid].vmemlist->mnext = BACKING_STORE_BASE + store * BACKING_STORE_UNIT_SIZE;
		proctab[pid].vmemlist->mnext->mlen = hsize * NBPG;
		restore(ps);
		return pid;
	};
}


/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
