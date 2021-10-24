#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
   if (bs_id < 0 || bs_id > 7) {
		return SYSERR
   }
   bsm_tab[bs_id].bs_pid = BADPID;
   bsm_tab[bs_id].bs_status = BSM_UNMAPPED;
   bsm_tab[bs_id].bs_npages = 0;
   bsm_tab[bs_id].bs_vpno = -1;
   bsm_tab[bs_id].bs_sem = -1;
   return OK;
}

