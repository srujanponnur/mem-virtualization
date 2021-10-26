#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) { // private heap is still pending

    STATWORD ps;
    disable(ps);

    if (npages > 256 || npages < 1 || bs_id < 0 || bs_id > 7) {
        return SYSERR;
    }

    if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED) {  
        return bsm_tab[bs_id].bs_npages;
    }

    else if (bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_pid == currpid) {
        restore(ps);
        return bsm_tab[bs_id].bs_npages;
    }
    else {  //this is when the backing store index is not of that's process
        if (bsm_tab[bs_id].bs_private_heap) {
            restore(ps);
            return SYSERR;
        }
        else {
            restore(ps);
            return bsm_tab[bs_id].bs_npages;
        }
    }
    restore(ps);
    return SYSERR;
}


