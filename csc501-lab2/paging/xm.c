/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  if (source < 0 || source > 7 || npages > 256 || npages < 1) {
	  restore(ps);
	  return SYSERR;
  }
  int ret_val = bsm_map(currpid, virtpage, source, npages); // bs_private_heap is set to zero
  restore(ps);
  return ret_val;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);
  int ret_val = bsm_unmap(currpid, virtpage, -1);
  return ret_val;
}
