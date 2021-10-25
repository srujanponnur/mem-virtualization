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
		if (frm_tab[frameIndex].fr_type == FRM_UNMAPPED) {
			return frameIndex;
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


//void init_pd(int pid) {
//
//	int free_frame_no;
//	get_frm(&free_frame_no);
//	int base_add = (FRAME0 + free_frame_no) * NBPG;
//	frm_tab[free_frame_no]
//	frm_tab[free_frame_no].fr_status = FRM_MAPPED;
//	frm_tab[free_frame_no].fr_pid = pid;
//	frm_tab[free_frame_no].fr_type = FR_TBL;
//	return 0;
//}
//
//void alloc_pd(int pid) {
//
//}