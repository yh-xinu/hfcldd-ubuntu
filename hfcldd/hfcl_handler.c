/*
 * hfcl_handler.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char intr_rcsid[] = "$Id: hfcl_handler.c,v 1.52.2.11.2.26.2.5.2.1.6.22.4.15.2.21.2.4.2.1.2.2.2.6.2.1 2016/02/19 03:05:29 mhayashi Exp $";

#include "hfcldd.h"
#include "hfcl_tbol.h"
#include "hfcl_strategy.h"
#include "hfcl_stra_trace.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_ioctl.h"
#include "hfcl_top.h"
#include "hfcl_detect.h"
#include "hfcl_handler.h"
#include "hfcl_handler_trace.h"
#include "hfcl_mlpf.h"

#include "hfcldd_fx.h"
#include "hfcl_strategy_fx.h"
#include "hfcl_stra_trace_fx.h"
#include "hfcl_timer_recovery_fx.h"
#include "hfcl_ioctl_fx.h"
#include "hfcl_top_fx.h"
#include "hfcl_detect_fx.h"
#include "hfcl_handler_fx.h"
#include "hfcl_mlpf_fx.h"
#include "hfcl_hand_timer_trace_fx.h"

static uchar   logdata[16];

#if _HFC_DEBUG_HAND_06
extern int hfc_debug_ioerr_code;
#endif

/*
 * Function:	htc_intr
 *
 * Purpose:     Interrupt handler for the host bus adapter.
 *  			This routine examines each factor of INT_A register
 *   			on PCI memory space and handles the particular factor. 
 *
 *  			Interrupt factors are following:
 *					EXG_MCK                     (INT_A_00)
 *					PCI SERR                    (INT_A_01)
 *					PCI PERR                    (INT_A_02)
 *					MailBox_Response            (INT_A_21)
 *					MailBox_Intreq              (INT_A_20)
 *					XRB Response                (INT_A_22)
 *					MPINT3(MCK Recovery)        (INT_A_23)
 *
 * Arguments:   
 *  irq        - IRQ number
 *  dev_id     - Pointer to adap_info
 *  regs       - (Snapshot of CPU context before CPU interruption)
 *
 * Returns:    - IRQ_HANDLED
 *
 * Notes:       
 */


irqreturn_t
//hfc_intr(int irq, void *dev_id, struct pt_regs *regs)
hfc_intr(int irq, void *dev_id )
{
	const uint hwerr_int[4] = {
								0x00000000, /* PKTYPE is none */
								0xf0000000, /* 1 HFC_PKTYPE_FPP */
								0xf0000000, /* 2 HFC_PKTYPE_FIVE */
								0x80000000  /* 3 HFC_PKTYPE_FIVE_EX */
							};

	struct adap_info		*ap;
	int 					int_a_reg = 0;							/* FCLNX-0274 */
	int 					int_a_rst = 0;
	int 					xrb_in_no;
	int 					xrb_out_no;
	uint					save_xrb_outp;
	uint					save_xrb_inp;
	uint					work_save_xrb_outp;
	uint					work_save_xrb_inp;
	uint					int_status_reg0, int_status_reg1, err_detail;
	ulong					flags = 0;											/* FCLNX-0274 */
	ulong					io_flags = 0;										/* FCLNX-0274 */
	uint					hyp_support=0, hyp_status;	/* FCLNX-GPL-428 */
	
	/* Check dev_id parameter */
	ap = (struct adap_info *)dev_id;

	if (!ap) {
		HFC_DBGPRT("hfc_intr: NULL adap_ptr \n");

		return IRQ_NONE;

	}

	spin_lock_irqsave(HOST_LOCK, io_flags);

	HFC_ADAPLOCK_IRQSAVE(flags);
	
	if( test_bit(HFC_CTLRST_PROCESS, (ulong *)&ap->mp_adap_info->status ) ) /* Do not read registers when CTLRST is going */
	{
		hfc_hand2_trace(
			HFC_TRC_HANDLER, 0x82, ap, NULL, NULL,
			0, (uint64_t)int_a_reg, 0);

		goto intr_exit;
	}
	
	/* FCLNX-GPL-266 start *//* FCLNX-GPL-550 */
	/* Do not read registers when CHKSTP has happen */
//	if( test_bit(HFC_CHK_STOP, (ulong *)&ap->status ) )
//	{
/*		hfc_hand2_trace( */
/*			HFC_TRC_HANDLER, 0x83, ap, NULL, NULL, */
/*			0, (uint64_t)int_a_reg, 0); */

//		goto intr_exit;
//	}
	/* FCLNX-GPL-266 end *//* FCLNX-GPL-550 */
	
	/* This part is added for the interface of hfc_initialize	*/
	int_a_reg = hfc_identify_int_factor(ap, irq, ap->msi_flag);

	if( int_a_reg == 0 ) /* Check whether this int. is for handler or not */
	{
/*		hfc_hand2_trace(	FCLNX-FCLNX_GPL-32	*/
/*			HFC_TRC_HANDLER, 0x81, ap, NULL, NULL,	*/
/*			0, (uint64_t)int_a_reg, 0);		*/
		
		goto intr_exit;
	}


	hfc_hand2_trace( HFC_TRC_HANDLER,0x00, ap,NULL, NULL, 0,(uint64_t)int_a_reg,0);
	
	/*  FPP, FIVE: Exg. Mck or PCI Error (SERR, PERR, SPERR) occured */ /* 0xf0000000 */
	/*  FIVE-EX: Exg. Mck occured */ /* 0x80000000 */
	if( int_a_reg & hwerr_int[ap->pkg.type] )
	{
		HFC_DBGPRT("hfcldd : hfcl_intr - HFC_HWERR_INT occured\n");
		/* Check whether PCI bus error occured or not */
		int_status_reg0 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4);
		int_status_reg1 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS1, (char)0x4);
		err_detail = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4);

		/* Reset PCI space to clear HW_ERROR_INT			 */
		int_a_rst = hfc_inta_mask[ap->pkg.type];
		hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);
		
		if ( HFC_MMODE_CHECK_SHADOW(ap) ){	/* FCLNX-GPL-428 */
			if((test_bit(HFC_ISOL, (ulong *)&ap->status)) ||
			(test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status))){
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,
				0xb1,(uchar*)&hyp_status,4);	/* FCLNX-GPL-423 */
				goto intr_hwerr;
			}
			hyp_support = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 4);
			if ( hyp_support & HFC_HG_LPAR_ISOLATION_SUPPORT ) { /* read and write(clear) HYPER Interrupt Detail */
				hfc_write_hg_reg(ap, HFC_IOHGSPC_HYPINTDETAIL, 0x4, HFC_HG_HYPINTDET_FMCK);
			}
		}	/* FCLNX-GPL-428 */
		
		hfc_hwerr_int(ap,int_a_reg,int_status_reg0,int_status_reg1,err_detail);
		
		goto intr_hwerr;
	}
		                                                                            /* @MLPF */
	/* for MLPF */
	if ( HFC_MMODE_CHECK_SHARED (ap) ){
		if( hfc_mlpf_intr(ap, int_a_reg))
			goto intr_hwerr;
	}
	
	/* for FIVE-EX only */
	if( ap->pkg.type == HFC_PKTYPE_FIVE_EX ){
		if( int_a_reg & HFC_PCIE_SRAM_CE ){
			hfc_issue_int_a_rst(ap, HFC_PCIE_SRAM_CE, int_a_reg);
			hfc_abend(ap, HFC_ABEND_SRAM_CE);
			goto intr_hwerr;
		}
	}
	
	/* Mailbox response received */
	if( int_a_reg & HFC_MBRSP_INT )
	{
		HFC_DBGPRT("hfcldd : hfcl_intr - HFC_MBRSP_INT (Mailbox Response) occured\n");

		int_a_rst = int_a_reg & HFC_MBRSP_INT;
		hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);

		hfc_mb_resp(ap);
		
	} else if( int_a_reg & HFC_MBINT_REQ )
    {
		HFC_DBGPRT("hfcldd : hfcl_intr - HFC_MBINT_REQ  (Mailbox Int. Request) occured\n");

		/* Mailbox int. request received */	
		int_a_rst = int_a_reg & HFC_MBINT_REQ;
		hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);
		
		hfc_mb_intreq(ap);

	}
	else if( int_a_reg & HFC_XRB_RSP )
	{
		
		if( HFC_MMODE_CHECK_SHARED(ap) )                                /* FCLNX-0x0382 */
		{
			/* Reset INT_A */
			int_a_rst = int_a_reg & HFC_XRB_RSP ;
			hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);
		}
		
		/* XRB report received */	
		work_save_xrb_inp = ap->fw_init_p->xrb_inp;
		work_save_xrb_outp = ap->fw_init_p->xrb_outp;
		
		HFC_4B_TO_4L(save_xrb_inp, work_save_xrb_inp);
		HFC_4B_TO_4L(save_xrb_outp, work_save_xrb_outp);

		
		if( !HFC_MMODE_CHECK_SHARED(ap) )                               /* FCLNX-0x382 */
		{
			
			/* Reset INT_A */
			int_a_rst = int_a_reg & HFC_XRB_RSP ;
			hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);
		}
		
		xrb_in_no =((save_xrb_inp & 0x00ff0000)>>16)*HFC_XRB_PER_PAGE ;
		xrb_in_no += (save_xrb_inp & 0x0000ffff);
		xrb_out_no =((save_xrb_outp & 0x00ff0000)>>16)*HFC_XRB_PER_PAGE ;
		xrb_out_no += (save_xrb_outp & 0x0000ffff);

		if( xrb_in_no == xrb_out_no )
		{

		} else 
		{
			hfc_xrb_resp(ap,xrb_out_no,xrb_in_no);
		}
	} else
	{ 
		/* No other interrupt factors happen because int mask is disabled */

		HFC_DBGPRT("hfcldd : hfcl_intr - other (error case) \n");

		int_a_rst = int_a_reg ;
		hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);

		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,
			0x6a,(uchar*)&int_a_reg,4);

	}
	
	if( ap->next_dstart_cnt )
	{
		/* Starts unconditionally */
		hfc_issue_intl_start(ap,ap->next_dstart_top);
	} 

//	if(hfc_manage_info.hfcplus_enable){					/* FCLNX-0506 */
//		hfc_manage_info.npubp->hfc_check_errcount(ap);	/* FCLNX-0506 */
//	}
	hfc_check_errcount(ap);	/* FCLNX-GPL-349 */

intr_hwerr:												/* FCLNX-GPL-279 */
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info )
	{
		if ( ap->retry_hfcp_top )
			hfc_manage_info.npubp->hfc_retry_strategy(ap);

		if ( hfc_manage_info.wait_reset_mp )						/* FCLNX-0429 */
		{
			hfc_manage_info.npubp->hfc_check_dev_reset_complete();	/* FCLNX-0429 */
			hfc_manage_info.npubp->hfc_check_bus_reset_complete();	/* FCLNX-0429 */
		}
	}

	hfc_hand2_trace(HFC_TRC_HANDLER,0x10, ap,NULL, NULL, 0, (uint64_t)int_a_reg, 0);

intr_exit:												/* FCLNX-GPL-279 */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	spin_unlock_irqrestore(HOST_LOCK, io_flags);
			
	return IRQ_HANDLED;


}


/*
 * Function:    hfc_mb_resp
 *
 * Purpose:     This routine deals with Mailbox interrupt 
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from the other device.
 *
 * Notes:       
 */
void hfc_mb_resp(struct adap_info *ap)
{

	HFC_ENTRY("hfc_mb_resp");

	if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
	{
		HFC_DBGPRT("hfc_mb_resp @ HFC_MB_PROL\n");
		hfc_wake_up(&ap->mb_event, &(ap->mb_event_wait));
		ap->mb_prol_wake_up_time = (uint)jiffies; /* FCLNX-GPL-243 */
	}
	else
	{
		
		HFC_DBGPRT("hfc_mb_resp @ otherwise %x\n", ap->mb->mb_resp.type.respcmd.cmd.sub_cmd);

		switch( ap->mb->mb_resp.type.respcmd.cmd.sub_cmd){
			case HFC_MBSCMD_LOGIN:
				hfc_login_resp(ap);
				break;
			
			case  HFC_MBSCMD_PDISC:
				hfc_pdisc_resp(ap);
				break;
			
			case HFC_MBSCMD_LINKINIT:
				hfc_link_resp(ap);
				break;
			
			case HFC_MBSCMD_NMSRV:
				hfc_gidft_resp(ap);
				break;
				
			case HFC_MBSCMD_GIDPN:
				hfc_gidpn_resp(ap);
				break;
 			
			case  HFC_MBSCMD_GPNID:
				hfc_gpnid_resp(ap);
				break;
			
			case HFC_MBSCMD_MIHLOG:
				hfc_mihlog_resp(ap);
				break;
			
		}
	}

	HFC_EXIT("hfc_mb_resp (*) ");

}


/*
 * Function:    hfc_login_resp
 *
 * Purpose:     This routine deals with LOGIN response
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */ 
void hfc_login_resp(struct adap_info *ap)
{
	struct mailbox              *mbox = NULL ;
	struct target_info          *target = NULL ;
	int                         mb_resp_status = 0 ;
	uint                        rc_passthrouh = 0xffffffff;
	uint64_t                    ww_name = 0 ; /* FCLNX-GPL-069 */
	uint64_t                    node_name ;
	uint						save_xrb_outp, save_xrb_inp ;				/* FCWIN-0152 */
	uint						work_save_xrb_outp, work_save_xrb_inp ;		/* FCWIN-0152 */
	uint 						xrb_in_no,xrb_no,xrb_cnt;		/* FCWIN-0152 */
	struct hfc_pkt              *hfcp_wk = NULL;	/* FCLNX-0579 */
	int                         hash = 0;           /* FCLNX-0579 */
	int                         scsito = 0;         /* FCLNX_GPL-0338 */
	
	HFC_ENTRY("hfc_login_resp");
	
	mbox = ap->mb;
	ap->mb_status = 0;														/* FCLNX-0037 */
	
	/*-- Search target info by Pseq # */
	HFC_DBGPRT(" hfcldd%d : hfc_login_resp search :target info pseq = %d \n", ap->dev_minor, mbox->mb_init.pseq_no);

	target = hfc_pseq_target_info(ap, mbox->mb_init.pseq_no);
	
	if( target == NULL )
	{/* Target_info is not found */
		HFC_DBGPRT(" hfcldd : hfc_login_resp : target info was not founded \n");
		hfc_hand2_trace( HFC_TRC_LGINRSP, 0x11, ap, NULL, NULL, 0, 0, 0 );
		
		return;
	}/* Target_info is not found */
	else
	{/*  When target_info is found */
		HFC_DBGPRT("hfc_login_resp : target info was founded \n");
		rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);			/* FCWIN-0170 */
		
		hfc_hand2_trace( HFC_TRC_LGINRSP, 0x12, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
			unlock_mailbox(ap);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				if ( test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags)	/* When other HBAs exists in Fabric */
				 || ( !( (ap->mb_results == 0x02FF1030)||(ap->mb_results == 0x02FF1050) ) ) ) { 	/* FCLNX-364 */
					hfc_mb_errlog(ap, target, HFC_TRC_LGINRSP, rc_passthrouh);	/* FCWIN-0155  */
				}

			}
			
			if(ap->rt_err_enable){	/* FCLNX-GPL-349 */
				if(hfc_manage_info.hfcldd_mp_mod){
					if(hfc_check_cmnd_timeout(ap,target)){			/* FCLNX-GPL-430 */
						hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);  /* FCLNX-0506 */
					}
				}
				else{
					if(hfc_check_cmnd_timeout(ap,target)){ /* FCLNX-GPL-431 */
						hfc_watched_errcount_i(ap, NULL, HFC_RT_ERR);  /* FCLNX-GPL-349 */
					}
				}
			}

			if( test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status) ){	/* FCLNX-GPL-576 */
				clear_bit(HFC_WAIT_CANCEL, (ulong *)&target->status);
				hfc_hand2_trace( HFC_TRC_LGINRSP, 0x15, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );
				hfc_abend(ap, HFC_ABEND_PIC_ERROR);
				return;
			}															/* FCLNX-GPL-576 */

			mb_resp_status = SCS_NO_DEV_RESP;			/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			/* mb_resp_status clear */
			HFC_DBGPRT("hfc_login_resp : clear mb responce status \n");
			mb_resp_status = 0 ;
			ap->link_dead_cnt = 0 ;

			if (hfc_read_val(mbox->mb_resp.type.drvioctl1.login.dependent_code) == 0x8000) /* FCWIN-0182 STR*/

			{	 /* Response when F/W internal reset is requested  */
				mb_resp_status = SCS_CANCEL_RESP;
				break;					
			}														/* FCWIN-0182 END*/


			HFC_8B_TO_8L(ww_name, mbox->mb_resp.type.drvioctl1.login.port_name);
			HFC_8B_TO_8L(node_name, mbox->mb_resp.type.drvioctl1.login.node_name);

			/* Skip WWN check during the process of target generation */	
			if ( !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) )  
				break;

			if( (target->ww_name != ww_name) || (target->node_name != node_name) )
			{ /* WWN is mismatched */
				mb_resp_status = SCS_LOGIN_WWCHG;
				if( target->ww_name != ww_name )
				{
					memcpy(logdata,(uchar*)&target->ww_name,8);
					memcpy(&logdata[8],(uchar*)&ww_name,8);
				}
				else
				{
					memcpy(logdata,(uchar*)&target->node_name,8);
					memcpy(&logdata[8],(uchar*)&node_name,8);
				}
				hfc_errlog(ap,target,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_EVNT3,0x0b,logdata,16) ;	/* FCLNX-GPL-0130 */
			}
			break;
		default :
			break;
		}
	}
	
	ap->mb_retry_cnt = 0;
	
	/* After completing LOGIN process, XRB response before LOGIN completion */
	/* is deleted by F/W */									/* FCWIN-0152 STR */
	/* However, discard XRB here in case XRB response has already stored */
	                                            
	HFC_DBGPRT("hfc_login_resp : discard xrb response before login has finished  \n");
	work_save_xrb_inp = ap->fw_init_p->xrb_inp;
	work_save_xrb_outp = ap->fw_init_p->xrb_outp;
	HFC_4B_TO_4L(save_xrb_inp, work_save_xrb_inp);
	HFC_4B_TO_4L(save_xrb_outp, work_save_xrb_outp);
	xrb_in_no  =  ((save_xrb_inp & 0x00ff0000)>>16)*HFC_XRB_PER_PAGE ;
	xrb_in_no  += (save_xrb_inp & 0x0000ffff) ;
	xrb_no     =  ((save_xrb_outp & 0x00ff0000)>>16)*HFC_XRB_PER_PAGE ;
	xrb_no     += (save_xrb_outp & 0x0000ffff) ;

	/* Calculate the number of xrb */
	if ( xrb_no < xrb_in_no )
		xrb_cnt = xrb_in_no - xrb_no ;
	else
	{
		xrb_cnt = ap->xrb_max;
		xrb_cnt -= xrb_no ;
		xrb_cnt += xrb_in_no ;
	}

	if( test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) )
	{
		while ( xrb_cnt > 0 )
		{
			if( ap->xrb[xrb_no].flag & HFC_XRB_VALID ) {
				if ( ap->xrb[xrb_no].drv_work.target_id
									== target->target_id ) {
					ap->xrb[xrb_no].skip |= HFC_XRB_SKIP; /* FCLNX-008 */
				}
			}
			xrb_no++;
			xrb_cnt--;
			if( xrb_no >= ap->xrb_max )
				xrb_no = 0 ;
		}																	 /* FCWIN-0152 END */
	}

	scsito = 0;																/* FCLNX_GPL-0338 ST */
	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		if (target->we_que_top[hash] != NULL)
		{	/* hfcp exists in queue */
			hfcp_wk = target->we_que_top[hash];
			
			while ( hfcp_wk != NULL ) {
				if (test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp_wk->cmd_flags)) {
					scsito = 1;
					break;
				}
				hfcp_wk = hfcp_wk->cmd_forw ;
			}
		}
		if (scsito) break;
	}																		/* FCLNX_GPL-0338 END */

	switch(mb_resp_status)
	{
	case SCS_LOGIN_WWCHG :
	/* WWN is mismatched */
		HFC_DBGPRT("hfc_login_resp : unmatch www name \n");
		clear_bit(HFC_WWN_VALID, (ulong *)&target->flags);

#ifdef SYSFS_SUPPORT /* FCLNX-GPL-206 start */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status ) ) {
			set_bit( HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status );
			atomic_set(&ap->rport_event_wait, 1);		/* FCLNX-GPL-259,565 */
			wake_up_interruptible(&ap->rport_event);					/* FCLNX-GPL-259 */
		}
#endif
#endif /* FCLNX-GPL-206 end */

	case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
	case SCS_CANCEL_RESP :
		/* LOGIN failed */
		HFC_DBGPRT("hfc_login_resp : login has failed \n");
		
		if((target->we_que_cnt != 0 )&&(mb_resp_status == (SCS_NO_DEV_RESP))){	/* FCLNX-GPL-350 */
			set_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
			clear_bit(HFC_WAIT_LOGIN, (ulong *)&target->status);
			clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);				
			HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY);							/* FCLNX-GPL-389 */
			if( hfc_issue_relogin(ap, target) ){								/* FCLNX-GPL-389 */
				hfc_enque_login_req(ap, target);									
			}																	/* FCLNX-GPL-389 */
			hfc_hand2_trace( HFC_TRC_LGINRSP, 0x13, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );	/* FCLNX-GPL-389 */
			return;																
		}																		/* FCLNX-GPL-350 */

		if( test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) )
		{
			hfc_notify_tout(ap, target);	/* FCLNX-GPL-573 output SCSI time-out log. */
			if (test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status)) {										/* FCLNX-0095 */
				if ((scsito)&&( hfc_manage_info.hfcldd_mp_mod )) {											/* FCLNX_GPL-0338 *//* FCLNX-GPL-347*/
					hfc_cancel_scsi_cmd(ap, target, 0, NULL, mb_resp_status, 								/* FCLNX_GPL-0338 */
										HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);						/* FCLNX_GPL-0338 */
				}																							/* FCLNX_GPL-0338 */
				else {																						/* FCLNX_GPL-0338 */
					hfc_cancel_weque(ap, target, 0, NULL, mb_resp_status, HFC_CSCSI_ERROR, HFC_FLASH_TARGET);	/* FCLNX-0429 */
				}
			}
			else {																							/* FCLNX-0095 */
				hfc_cancel_scsi_cmd(ap, target, 0, NULL, mb_resp_status, 
										HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
			}
		}

		clear_bit(HFC_WAIT_LOGIN, (ulong *)&target->status);
		clear_bit(HFC_WAIT_CANCEL, (ulong *)&target->status);		/* FCLNX-GPL-038 */
		clear_bit(HFC_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);
				
		break;
	case 0 :
		/* LOGIN succeeded */
		HFC_DBGPRT("hfc_login_resp : login succeeded \n");

		hfc_notify_tout(ap, target);	/* FCLNX-GPL-573 output SCSI time-out log. */
		if ((scsito)&&( hfc_manage_info.hfcldd_mp_mod ) ){													/* FCLNX_GPL-0338 *//* FCLNX-GPL-347*/
			hfc_cancel_scsi_cmd(ap, target, 0, NULL, mb_resp_status, 										/* FCLNX_GPL-0338 */
									HFC_CSCSI_INHALT, TRUE, TRUE, HFC_FLASH_TARGET);						/* FCLNX_GPL-0338 */
		}																									/* FCLNX_GPL-0338 */
		else {																								/* FCLNX_GPL-0338 */
			hfc_cancel_weque(ap, target, 0, NULL, mb_resp_status, HFC_CSCSI_INHALT, HFC_FLASH_TARGET);
		}

		if(target->link_recovered){
			HFC_INFPRT("hfcldd%d : Target port( WWPN : %llx ) links up.\n", ap->dev_minor, (unsigned long long)ww_name);		/* FCLNX-GPL-334 */
			target->link_recovered = 0;
		}
		
		/* Cancel Link Up waiting timer between SW and device */
		clear_bit(HFC_SCN_WLINKUP, (ulong *)&target->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
//			clear_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
		hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, TRUE);
		clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
		clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
		
		clear_bit(HFC_WAIT_LOGIN, (ulong *)&target->status);
		clear_bit(HFC_WAIT_CANCEL, (ulong *)&target->status);		/* FCLNX-GPL-038 */
		clear_bit(HFC_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);
		set_bit(HFC_WWN_VALID, (ulong *)&target->flags);
		

		/* FCLNX-GPL-261 */
		/* Size of transmission frame used by new statistical information I/O calculation */
		if (ap->fw_init_p->func2 & HFC_FWF_STATCCA) {
			HFC_2B_TO_2L(target->send_frame_size, mbox->mb_resp.type.drvioctl1.login.send_frame_size);
		} else {
			target->send_frame_size = 2048;
		}

		break;

	default :
		break;
	}
	
	if ( test_bit(HFC_WAIT_BUSRSP, (ulong *)&ap->status) )
	{
		HFC_DBGPRT("hfc_login_resp : complete bus reset in this target \n");
		clear_bit(HFC_WAIT_BUS_RESET, (ulong *)&target->status);
	}

	/* Is mailbox locked in the wwnvwrify subroutine? */
	if (!mb_resp_status || !test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status))		/* FCLNX-0095 */
	{
		HFC_DBGPRT("hfc_login_resp : call wwnverify login \n");
		hfc_hand2_trace( HFC_TRC_LGINRSP, 0x14, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );			/* FCLNX-GPL-372 *//* FCLNX-GPL-377 */
		hfc_wwnverify_login(ap, target, mb_resp_status, ww_name);
	}
	
	HFC_DBGPRT("hfc_login_resp : call unlock mailbox \n");
	unlock_mailbox(ap);											/* FCWIN-0084 */
	
	HFC_EXIT("hfc_login_resp");
	
	return;
}


/*
 * Function:    hfc_pdisc_resp
 *
 * Purpose:     This routine deals with PDISC response
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_pdisc_resp(struct adap_info *ap)
{
	struct mailbox      *mbox = NULL ;
	struct target_info  *target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint64_t            ww_name ;
	uint64_t            node_name ;
	
	HFC_ENTRY("hfc_pdisc_resp");
	
	mbox = ap->mb;
	ap->mb_status = 0;														/* FCLNX-0037 */
		
    /* Search target info by Pseq # */
	target = hfc_pseq_target_info(ap, mbox->mb_init.pseq_no);
	
	if( target == NULL )
	{/* Target_info is not found */
		hfc_hand2_trace( HFC_TRC_PDISCRSP, 0x11, ap, NULL, NULL, 0, 0, 0 );
		return;
	}
	else
	{/* When target_info is found */
		rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);			/* FCWIN-0170 */
		
		hfc_hand2_trace( HFC_TRC_PDISCRSP, 0x12, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
			unlock_mailbox(ap);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_mb_errlog(ap, target, HFC_TRC_PDISCRSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			ap->link_dead_cnt = 0 ;
			HFC_8B_TO_8L(ww_name, mbox->mb_resp.type.drvioctl1.pdisc.port_name);
			HFC_8B_TO_8L(node_name, mbox->mb_resp.type.drvioctl1.pdisc.node_name);
			if( (target->ww_name != ww_name) || (target->node_name != node_name) )
			{	/* WWN is mismatched */
				mb_resp_status = SCS_PDISC_WWCHG ;
				if( target->ww_name != ww_name )
				{
					memcpy(logdata,(uchar*)&target->ww_name,8);
					memcpy(&logdata[8],(uchar*)&ww_name,8);
				}
				else
				{
					memcpy(logdata,(uchar*)&target->node_name,8);
					memcpy(&logdata[8],(uchar*)&node_name,8);
				}
				hfc_errlog(ap,target,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_EVNT3,0x0f,logdata,16);	/* FCLNX-GPL-0130 */
			}
			else
			{
				mb_resp_status = 0 ;
			}
			break;
		default :
			break;
		}
	}
	
	ap->mb_retry_cnt = 0;
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
		if(test_bit(HFC_WPDISC_LOGO_RESP, (ulong *)&target->status) )
		{
			set_bit(HFC_WWN_VALID, (ulong *)&target->flags);
		}
		clear_bit(HFC_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
		clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
		break;

	case SCS_PDISC_WWCHG :
		clear_bit(HFC_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
		clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
		break;

	case 0 :
		clear_bit(HFC_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_WPDISC_LOGO_RESP, (ulong *)&target->status);
		break;

	default :
		break;
	}
	
	unlock_mailbox(ap);

	HFC_EXIT("hfc_pdisc_resp");
}


/*
 * Function:    hfc_link_resp
 *
 * Purpose:     This routine deals with Link initialize response
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_link_resp(struct adap_info    *ap)
{
	uint                 rc_passthrouh = 0xffffffff;
	uint                 mb_resp_status = 0;
	struct mailbox       *mbox = ap->mb;
	uchar                config_chk_flg = 0;
	
	HFC_ENTRY("hfc_link_resp");
	
	ap->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);		/* FCWIN-0170 */
	
																	/* @MLPF STR */
	if ( HFC_MMODE_CHECK_MLPF(ap) )
	{
		if ( rc_passthrouh == HFC_MBPASS_SUCCESS )
		{
			if ( hfc_mlpf_config_check(ap) ){
				config_chk_flg = 1;
				rc_passthrouh = HFC_MB_FATAL;
			}
		}
		hfc_mlpf_cca_setup(ap);		/* FCLNX-GPL-494 */
	}
	if ( HFC_MMODE_CHECK_SHADOW(ap) ) { 
//		uint hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
		if ( test_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol) ) {	/* FCLNX-GPL-427 */
			hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
			clear_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol);
		}
	}
																	/* @MLPF END */
	
	memset((void *)logdata, 0, 16);
	logdata[0] = mbox->mb_resp.type.respcmd.cmd.command ;
	logdata[1] = mbox->mb_resp.type.respcmd.cmd.sub_cmd ;
	logdata[4] = mbox->mb_resp.xcc ;
	logdata[5] = mbox->mb_resp.esw ;
	logdata[6] = mbox->mb_resp.ssn ;
	logdata[7] = mbox->mb_resp.son ;
	memcpy(&logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_mailbox(ap);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			if ( ( ap->mb_results & 0x00ffffff ) == 0xE01001 ){
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_OPTERR0, 0x9c,logdata,16) ;
				ap->isol_detail = HFC_ISOLATE_SFPNOTSUPPORT;					/* FCLNX-0514 */
				set_bit(HFC_ISOL, (ulong *)&ap->status);
			}
			else if ( ( ap->mb_results & 0x00ffffff ) == 0xE11002 ){
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_ERR5, 0x9d,logdata,16) ;
				ap->isol_detail = HFC_ISOLATE_SFPFAIL;		/* FCLNX-0514 */
				set_bit(HFC_ISOL, (ulong *)&ap->status);
			}
			else if ( ( ap->mb_results & 0x00ffffff ) == 0xE11003 ){									/* FCLNX-0273 */
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_ERR5, 0x9e,logdata,16) ;	/* FCLNX-0273 */
				ap->isol_detail = HFC_ISOLATE_SFPFAIL;          /* FCLNX-0514 */
				set_bit(HFC_ISOL, (ulong *)&ap->status);
			}
			else if ( ( ap->mb_results & 0x00ffffff ) == 0xE11004 ){
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_ERR5, 0x9f,logdata,16) ;
				ap->isol_detail = HFC_ISOLATE_SFPDOWN;						/* FCLNX-0514 */
				set_bit(HFC_ISOL, (ulong *)&ap->status);
			}
			else if (!(ap->mb_results == 0x02FF1004))
			{
				HFC_DBGPRT("link_resp, error  %d\n",rc_passthrouh);
				hfc_mb_errlog(ap, NULL, HFC_TRC_LINKRSP, rc_passthrouh);    /* FCWIN-154  */
			}
		}
		
		HFC_DBGPRT("link_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;								/* FCWIN-0151 */
		clear_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status);			/* FCWIN-0082 */
		clear_bit(HFC_ONLINE, (ulong *)&ap->status);
		set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
		
		hfc_wwnverify_linkup_timeout(ap, NULL, 0);						/* FCLNX-GPL-314 */
																	/* @MLPF STR */
		if ( ( HFC_MMODE_CHECK_SHADOW(ap) ) && ( config_chk_flg == 0 ) ){
			hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_LINKDOWN, HFC_ENABLE_LPAR_STATE);
			switch(ap->isol_detail){	/* FCLNX-GPL-489 */
				case HFC_ISOLATE_SFPFAIL:
					hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_SFP_FAIL, HFC_ENABLE_LPAR_STATE);
					break;
				case HFC_ISOLATE_SFPNOTSUPPORT:
					hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_SFP_NOTSUPT, HFC_ENABLE_LPAR_STATE);
					break;
				case HFC_ISOLATE_SFPDOWN:
					hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_SFP_DOWN, HFC_ENABLE_LPAR_STATE);
					break;
			}							/* FCLNX-GPL-489 */
		}
																	/* @MLPF END */
		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("link_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		clear_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status);
		set_bit(HFC_ONLINE, (ulong *)&ap->status);
		break;
	default :
		break;
	}
	
	
	/* F/W init table information is stored in adap_info */

	HFC_DBGPRT("hfcldd : hfcl_intr - hfc_link_resp copy iocinfo adap_info = %lx\n",(ulong)ap);

	hfc_copy_iocinfo(ap);									/* FCWIN-0082 */
	ap->used_nmsrv = 0;
	

	HFC_DBGPRT("hfcldd : hfcl_intr - call wwnverify_linkup adap_info = %lx\n",(ulong)ap);

																	/* @MLPF STR */
	/* Shadow LPAR should not create target device object */
	/* So, Shadow LPAR skips the routine of device creation */
	if ( !HFC_MMODE_CHECK_SHADOW(ap) ) {                            /* FCLNX-0351 */
		hfc_wwnverify_linkup(ap, NULL, mb_resp_status, 0);
	}
	else {
		if(ap->initialize != 0){
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
	}                                                               /* FCLNX-0351 */
																	/* @MLPF END */
	unlock_mailbox(ap);															/* FCWIN-0084 */

	HFC_EXIT("hfc_link_resp");
		
	return;
}


/*
 * Function:    hfc_gidft_resp
 *
 * Purpose:     This routine deals with GID_FT response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_gidft_resp(struct adap_info *ap)
{
	uint               rc_passthrouh = 0xffffffff;
	uint               mb_resp_status = 0 ;

	HFC_ENTRY("hfc_gidft_resp");

	ap->mb_status = 0;														/* FCLNX-0037 */

	rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);	/* FCWIN-0170 */
	
	hfc_hand2_trace( HFC_TRC_GIDFTRSP, 0x00, ap, NULL, NULL, (uint64_t)rc_passthrouh, 0, 0 );
	
	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_mailbox(ap);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){
			
			if( !(ap->mb_results == 0x02040409) )					/* FCWIN-276 */
			hfc_mb_errlog(ap, NULL, HFC_TRC_GIDFTRSP, rc_passthrouh);       /* FCWIN-154 */
		}
		ap->used_nmsrv = 0;
		mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-151 */
		break;
	case HFC_MBPASS_SUCCESS :
		ap->used_nmsrv = 1;
		mb_resp_status = 0 ;
		break;
	default :
		break;
	}

	clear_bit(HFC_WAIT_NMSRV, (ulong *)&ap->status);
	hfc_wwnverify_gidft(ap, NULL, mb_resp_status);
	unlock_mailbox(ap);										/* FCWIN-0084 */
	
	HFC_EXIT("hfc_gidft_resp");
	
	return;
}


/*
 * Function:    hfc_gidpn_resp
 *
 * Purpose:     This routine deals with GID_PN response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_gidpn_resp(										  /* FCWIN-0082 */
	struct adap_info    *ap)
{
	struct mailbox              *mbox = NULL ;
	struct target_info          *target = NULL ;
	int                         mb_resp_status = 0 ;
	uint                        rc_passthrouh = 0xffffffff; 
	uint64_t                    mb_scsi_id;

	HFC_ENTRY("hfc_gidpn_resp");
	
	mbox = ap->mb;
	target = hfc_pseq_target_info(ap, mbox->mb_init.pseq_no);
	
	if( target == NULL ) 
	{
		hfc_hand2_trace( HFC_TRC_GIDPNRSP, 0x11, ap, target, NULL, 0, 0, 0 );
	}
	else
	{
		rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);	/* FCWIN-0170 */
		
		hfc_hand2_trace( HFC_TRC_GIDPNRSP, 0x12, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );
		
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
			unlock_mailbox(ap);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){
				
				hfc_mb_errlog(ap, target, HFC_TRC_GIDPNRSP, rc_passthrouh);             /* FCWIN-0154 */
			}
			
			HFC_DBGPRT("gidpn_resp() - responce fail.");
			mb_resp_status = SCS_NO_DEV_RESP;										/* FCWIN-0151 */
			clear_bit(HFC_WAIT_GIDPN, (ulong *)&target->status);
			clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
			
			/* ABEND -> Was the device deleted?						*/
			/* Start Link Up waiting Timer between SW and device 	*/
			if (!test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status)) {
				set_bit(HFC_NEED_CANCEL, (ulong *)&target->status);					/* FCLNX-0095 STR */ 	/* FCLNX-GPL-038 */
				clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);				/* FCLNX-GPL-038 */
				hfc_enque_login_req(ap, target);									/* FCLNX-0095 END */
				
				set_bit(HFC_SCN_WLINKUP, (ulong *)&target->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
					set_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
				hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, FALSE);
			}
			break;

		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			clear_bit(HFC_WAIT_GIDPN, (ulong *)&target->status);
			clear_bit(HFC_SCN_RESP, (ulong *)&target->status);

			if ((test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status))
			 || (!test_bit(HFC_WWN_VALID, (ulong *)&target->flags))) {				/* FCLNX-0095 */

				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);				/* FCLNX-0095 */
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);					/* FCLNX-0095 */
				if (test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status))
					target->link_recovered = 1;											/* FCLNX-GPL-334 */
				hfc_enque_login_req(ap, target);
			}

			/* Cancel link Up waiting timer between SW and device */
//			if((hfc_manage_info.hfcplus_enable)&&(ap->lds_errcnt_info!=NULL))	/* FCLNX-GPL-327 */
			if(ap->ld_err_limit_s)	/* FCLNX-GPL-349 */
			{
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ){ /* FCLNX-GPL-FX-472 */
					if ( test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status) )
						hfc_manage_info.npubp->hfc_watched_errcount(ap, target, HFC_OCCURED_FAILURE, HFC_TGT_LDS_ERR);	/* FCLNX-0506 */
				} else {
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
					if ( test_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags) )
#else
					if ( test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status) )
#endif
						hfc_watched_errcount_i(ap, target, HFC_TGT_LDS_ERR);	/* FCLNX-GPL-349 */
				}
			}																	/* FCLNX-GPL-327 */

			clear_bit(HFC_SCN_WLINKUP, (ulong *)&target->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
//				clear_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, TRUE);

			mb_scsi_id = hfc_read_val( ap->mb->mb_resp.type.drvioctl1.gid_pn.port_id );
			mb_scsi_id &= 0x00ffffff;

			set_bit(HFC_WWN_VALID, (ulong *)&target->flags);

			if ( target -> scsi_id != mb_scsi_id ) { /* Target SCSI ID is mismatched with cuerrent ID */
				HFC_DBGPRT("gidpn_resp() - scsi_id change (0x%llx -> 0x%llx)",
							(unsigned long long)target->scsi_id, (unsigned long long)mb_scsi_id);

				target -> scsi_id = mb_scsi_id;	/* Switch SCSI ID */

				/* Initiate LOGIN (Clear target state of SCSI response wait) */
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
				hfc_enque_login_req(ap, target);
			}
			else {
				if (test_bit(HFC_NEED_LOGIN, (ulong *)&target->status)) {
					set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
					clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
					hfc_enque_login_req(ap, target);
				}
				else if (test_bit(HFC_SCN_RESP, (ulong *)&target->status) ||
						 test_bit(HFC_NEED_PDISC, (ulong *)&target->status) ){
					set_bit(HFC_NEED_PDISC, (ulong *)&target->status);				/* FCWIN-0146 */
					hfc_enque_pdisc_req(ap, target);
				}
			}

			clear_bit(HFC_SCN_RESP, (ulong *)&target->status);						/* FCWIN-0146 */
			break;

		default :
			break;
		}
	}
	
	unlock_mailbox(ap);
	
	HFC_EXIT("hfc_gidpn_resp");
	
	return;
}


/*
 * Function:    hfc_gpnid_resp
 *
 * Purpose:     This routine deals with GPN_ID response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_gpnid_resp(
	struct adap_info    *ap)
{
	uint              mb_resp_status = 0 ;
	uint              rc_passthrouh = 0xffffffff;
	

	HFC_ENTRY("hfc_gpnid_resp");

	rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);		/* FCWIN-0170 */
	
	hfc_hand2_trace( HFC_TRC_GPNIDRSP, 0x00, ap, NULL, NULL, (uint64_t)rc_passthrouh, 0, 0 );
	
	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
			unlock_mailbox(ap);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_mb_errlog(ap, NULL, HFC_TRC_GPNIDRSP, rc_passthrouh);   /*  FCWIN-154 */
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			break;
		default :
			break;
	}
	
	clear_bit(HFC_WAIT_GPNID, (ulong *)&ap->status);
	hfc_wwnverify_gpnid(ap, NULL, mb_resp_status);
	unlock_mailbox(ap);										/* FCWIN-0084 */
	
	HFC_EXIT("hfc_gpnid_resp");

	return;
}


/*
 * Function:    hfc_mihlog_resp
 *
 * Purpose:     This routine deals with MIH_LOG response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_mihlog_resp(struct adap_info    *ap)
{
	uint                rc_passthrouh = 0xffffffff;
	uint                mb_resp_status;
	struct target_info  *target;
	struct mailbox		*mbox;
	uint				lun			=0;
	/*struct lun_ext	  *lubuf;									    FCWIN-0117 */
	uint				TargetId	=0;								 /* FCWIN-0117 */
	struct hfc_pkt		*hfcp_to;									 /* FCWIN-0153 */
	struct hfc_pkt		*find_hfcp = NULL;
	struct hfc_pkt		*hfcp_wk;
	struct hfc_pkt		*hfcp_top_p;
	struct hfc_pkt		*hfcp_end_p;
	int					i;
	uchar				hfcp_find;
	uchar				abend_code ;
	uchar				seq_no;
	uint64_t	hfcp_ad;
	struct dev_info		*dev= NULL;									/* FCLNX-GPL-0343 */

	HFC_ENTRY("hfc_mihlog_resp");

	mbox = ap->mb;
	ap->mb_status = 0;													/* FCLNX-0037 */

	rc_passthrouh = hfc_mb_passthrough_rsp(ap, HFC_MB_INTL);			/* FCWIN-0170 */
	
	hfc_hand2_trace( HFC_TRC_MIHLGRSP, 0x00, ap, NULL, NULL, (uint64_t)rc_passthrouh, 0, 0);
	clear_bit(HFC_WAIT_MIHLOG, (ulong *)&ap->status);	/* FCLNX-0506 */
	
	/* Check hfc_pkt address */							/* FCLNX-GPL-0135 */
	hfcp_wk = (struct hfc_pkt *)(ulong)ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.hfc_pkt;
	hfcp_find= FALSE;

	if (hfc_manage_info.hfcldd_mp_mod) {				/* FCLNX-GPL-0244 */
		if (hfcp_wk != NULL) {
			if (test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp_wk->cmd_flags )) {
				hfcp_find = TRUE ;
			}
		}
	}													/* FCLNX-GPL-0244 */
	
	if (hfcp_find == FALSE) {							/* FCLNX-GPL-0244 */
		for (i=0;i<HFC_PKT_POOL_NUM;i++) {
			hfcp_top_p = ap->pkt_pool[i];
			hfcp_end_p = hfcp_top_p + HFC_PKT_POOL_SIZE;	/* FCLNX-GPL-FX-151 */
			if( (hfcp_wk >= hfcp_top_p) && (hfcp_wk < hfcp_end_p) ) {
				hfcp_find = TRUE ;
				break;
			}
		}
	}
	
	if (hfcp_find == FALSE) {
		hfc_hand2_trace( HFC_TRC_MIHLGRSP, 0x10, ap, NULL, NULL, (uint64_t)rc_passthrouh, 0, 0);
		
		abend_code = HFC_ABEND_HFCPKT_CHK ;
		seq_no = 0x02;
		memset(logdata,0,16);
		memcpy(&logdata[0],(uchar *)&seq_no,1);
		memcpy(&logdata[8],(uchar *)&hfcp_wk,8);
		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xBE,logdata,16) ;
		unlock_mailbox(ap);
		hfc_abend(ap,abend_code) ;
		return;
	}													/* FCLNX-GPL-0135 */
	
	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_mailbox(ap);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			hfc_mb_errlog(ap, NULL, HFC_TRC_MIHLGRSP, rc_passthrouh);       /* FCWIN-0154 */
		}
		
		mb_resp_status = SCS_NO_DEV_RESP;								/* FCWIN-0151 */
																	/* FCWIN-0153 STR*/
		hfcp_to = (struct hfc_pkt *)(ulong)ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.hfc_pkt;
		TargetId = ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.target_id;
		lun = ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.lun_id; 
		break;

	case HFC_MBPASS_SUCCESS :
		/* Get MIH-LOG data successfully */
		mb_resp_status = 0 ;

		hfcp_ad = (uint64_t)ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.hfc_pkt;
		hfcp_to = (struct hfc_pkt *)(ulong)hfcp_ad;
		TargetId = ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.target_id;
		lun = ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.lun_id;

		if( ap->mb->mb_resp.type.drvlogb0.mih_log.sbc != 0 ) /* No need for endian conversion */ 
		{	
			/* Does the target SCSI initiation for this MIH-LOG exist in SCSI response waiting queue? */
			if ((target = hfc_hash_target_info(ap, TargetId)) != NULL) {
				find_hfcp = target->we_que_top[lun % HASH_T_NUM];
				
				while( find_hfcp != NULL)
				{
					if ( find_hfcp == hfcp_to )
						break ;

					find_hfcp = find_hfcp->cmd_forw;
				}
			}
			
			if (find_hfcp) {
				/* Target SCSI initiation exists in SCSI response waiting queue */
				hfcp_to->tout_slog_ssn = ap->mb->mb_resp.type.drvlogb0.mih_log.ssn ;
				hfcp_to->tout_slog_son = ap->mb->mb_resp.type.drvlogb0.mih_log.son ;
				HFC_2B_TO_2L(hfcp_to->tout_slog_sbc, ap->mb->mb_resp.type.drvlogb0.mih_log.sbc);
				set_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp_to->cmd_flags);
			}
		}																/* FCWIN-0153 END*/
		break;

	default :
		break;
	}
	
	unlock_mailbox(ap);

	/*  After acquiring MIHLOG, initiate the reset with highest priority among  */
	/* following resets										*//* FCWIN-0153 STR */
	/* (1) Abort Task Set	(HFC_NEED_ABORT)			*/
	/* (2) Target Reset		(HFC_NEED_TARGET_RESET)		*/
	/* (3) LOGIN			(HFC_NEED_LOGIN)			*/


	if ((target = hfc_hash_target_info(ap, TargetId)) != NULL)
	{
		/* Turn off the control flag in MIHLOG */
		clear_bit(HFC_DEFER_TARGET_RESET, (ulong *)&target->target_reset);				/* FCLNX-GPL-036 */

		dev = (struct dev_info *)hfc_search_dev_info( target, find_hfcp );			/* FCLNX-GPL-0343 */
		if( dev == NULL ) return;

		dev->lustat &= ~HFC_DEFER_ABORT;						/* FCLNX-GPL-0343 */

//		target->lustat[lun] &= ~HFC_DEFER_ABORT;

		if ( (test_bit(HFC_NEED_LOGIN, (ulong *)&target->status))||(test_bit(HFC_NEED_CANCEL, (ulong *)&target->status)) )		/* FCWIN-0186 *//* FCLNX-GPL-038 */
		{
			hfc_issue_relogin(ap, target);
		}													/* FCWIN-0186 */
		else if (test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset) )		/* FCLNX-GPL-036 */
		{
			if( find_hfcp != NULL)
			{
				hfc_issue_task_mgm(ap, target, find_hfcp, lun, HFC_ISSUE_TARGET_RESET);
			}
		}
		else if (dev->lustat & HFC_NEED_ABORT)					/* FCLNX-GPL-0343 */
		{
			if( find_hfcp == NULL)
			{
				find_hfcp = target->we_que_top[lun % HASH_T_NUM];

				while( find_hfcp != NULL)
				{
					if ( (find_hfcp->target_id == TargetId)
					  && (find_hfcp->lun_id    == lun) )
					{
						break ;
					}

					find_hfcp = find_hfcp->cmd_forw;
				}
			}
			
			if ( find_hfcp != NULL ) {
				hfc_issue_task_mgm(ap, target, find_hfcp, lun, HFC_ISSUE_ABORT);
			}
			else {
				dev->lustat &= ~HFC_NEED_ABORT;					/* FCLNX-GPL-0343 */
			}
		}
	}																	/* FCWIN-0153 END */
	
	HFC_EXIT("hfc_mihlog_resp");
	
	return;
}


 
/*
 * Function:    hfc_mb_intreq
 *
 * Purpose:     
 *	This routine deals with mailbox interrupt 
 * interrupt factors are followings;
 *	1) SCN was received
 *	2) RSCN was received
 *	3) LOGO was received
 *	4) PLOGI was received
 *	5) Link Up was received
 *	6) Link Down was received
 *	8) SCSI Error log report was reported
 *		- adapter fatel hardware failure
 *		- unrecoverable adapter cmd failure
 *		- SCSI bus reset detected
 *		- maximum buffer usage detected
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mb_intreq(struct adap_info *ap)
{
	struct mailbox          *mbox ;
	uint                    mb_resp ;
	
	hfc_hand2_trace(HFC_TRC_MBINT, 0x00 ,ap ,NULL, NULL, 0, 0, 0);
	HFC_ENTRY("hfc_mb_intreq");
	
	/* Setup mailbox pointer */
	mbox = ap->mb;
	
	if( mbox->mb_intreq.type.fwintreq0.int_code == HFC_MBINT0 )
	{
		mb_resp = hfc_frame_a_data(mbox, ap->pkg.type, (uchar)0x00);
		
		switch(mbox->mb_intreq.type.fwintreq0.sub_int_code)
		{
			case HFC_MBINT_LINKDOWN :	hfc_linkdown_intreq(ap, 0);		break;/* FCLNX-GPL-489 */
			case HFC_MBINT_LINKUP :		hfc_linkup_intreq(ap);			break;
			case HFC_MBINT_PLOGI :		hfc_plogi_intreq(ap,mbox);		break;
			case HFC_MBINT_LOGO :		hfc_logo_intreq(ap,mbox);		break;
			case HFC_MBINT_SCN :		hfc_scn_intreq(ap,mbox);		break;
			case HFC_MBINT_RSCN :		hfc_rscn_intreq(ap,mbox);		break;
		}

		/* FCWIN-0105 */
		hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA,( char )0x4, ( int )mb_resp );
	}
	else if( mbox->mb_intreq.type.fwintreq0.int_code == HFC_MBINT2 ) /* FCAIX-0267 qJ*/
	{
		mb_resp = hfc_frame_a_data(mbox, ap->pkg.type, (uchar)0x00);
		
		/* SRAM 1bit Correctable Err (It's announced by FW) */
		if ( mbox->mb_intreq.type.fwintreq0.sub_int_code == HFC_MBINT_REPO_ASYN_EVENT )
		{   /* 1bit Failure */
			hfc_core_ce_event(ap);
		}
		
		/* Online update */
		if( mbox->mb_intreq.type.fwintreq0.sub_int_code == HFC_MBINT_ONLINEUP )
		{
			hfc_fw_online_update_complete(ap); /* FCLNX-GPL-112 */

		}
		
		/*-- Response of mailbox --*/
		hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA,( char )0x4, ( int )mb_resp );
	}
	else /* Err Case */
	{
		hfc_hand2_trace( HFC_TRC_MBINT, 0x80, ap, NULL, NULL, 0, 0, 0 );
		
		if( test_bit(HFC_LPTEST_ALRDY_INTREQ, (ulong *)&ap->io_status) )
		{
			clear_bit(HFC_LPTEST_ALRDY_INTREQ, (ulong *)&ap->io_status);
		} 
		else 
		{
			/*-- rsp_code=0xff --*/
			mb_resp = hfc_frame_a_data(mbox, ap->pkg.type, (uchar)0xff);
			
			memset(logdata, 0, 16 );
			memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16); /* FCLNX-GPL-163 */

			hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINT,
						ERRID_HFCP_EVNT3, 0x1c, logdata, 16 );
			
			/*-- Response of mailbox --*/										/* FCWIN-0105 */
			hfc_write_reg( ap, ( uint )HFC_IOSPACE_FRAMEA, 0x4, ( int )mb_resp );
			hfc_hand2_trace( HFC_TRC_MBINT, 0x81, ap, NULL, NULL, 0, 0, 0);
		}
	}

	if( !test_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status ) ) {
		start_next_mailbox( ap );										/* FCWIN-0297 */
	}
	
	hfc_hand2_trace( HFC_TRC_MBINT, 0x10, ap, NULL, NULL, 0, 0, 0);
	HFC_EXIT("hfc_mb_intreq");

	return;
}


/*
 * Function:    hfc_linkdown_intreq
 *
 * Purpose:     This routine deals with Linkdown interrupt response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  mig        - 1:Execute LPAR Migration process
 *             - 0:No execute LPAR Migration process
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_linkdown_intreq(
	struct adap_info *ap,
	uchar	mig)						/* FCLNX-GPL-489 */
{
	struct target_info     *target;
	struct mailbox         *mbox;
	uint                    lp;
	ushort                  detail;
	struct dev_info			*dev=NULL;									/* FCLNX-GPL-038	*/

	HFC_ENTRY("hfc_linkdown_intreq");
	
	hfc_hand2_trace( HFC_TRC_LINKDOWN_INT, 0x00, ap, NULL, NULL, 0, 0, 0);
	
	mbox = ap->mb;
	
																	/* @MLPF STR */
	if ( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_LINKDOWN, HFC_ENABLE_LPAR_STATE);    /* FCLNX-0361 */
		hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_UNSHARABLE, HFC_DISABLE_LPAR_STATE);    /* FCLNX-0396 */
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
	/* Update the adaptor state */
	set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		set_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
#endif	/* FCLNX-GPL-FX-424 */

	/* Stops LOGIN_DELAY_TMR	*/
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LOGIN_DELAY_TMR, 0, TRUE);		/* FCLNX-GPL-038 */
	clear_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status );						/* FCLNX-GPL-038 */
	
	/* Calcel all waiting queue which is reserved under the device of target adapter */
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)					/* FCWIN-0083 */
	{
		target = hfc_hash_target_info(ap, lp);
		
		if(target != NULL)
		{
			hfc_cancel_xob(ap, target, 0, NULL, HFC_FLASH_TARGET);
			hfc_cancel_weque(ap, target, 0, NULL, SCS_INTR_LINKDOWN, HFC_CSCSI_ERROR, HFC_FLASH_TARGET); /* FCLNX-0429 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_DELAY_TMR, 0, TRUE);	/* FCWIN-0082 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, TRUE); /* FCWIN-0146 */
			dev = target->dev;															/* FCLNX-GPL-038	*/
			if(dev != NULL) {
				/* stop LUN Reset Delay Timer */
//				hfc_manage_info.npubp->hfc_all_clear_dev_info(ap, dev);					/* FCLNX-GPL-0343	*/
				hfc_all_clear_dev_info(ap, dev);										/* FCLNX-GPL-0343	*/
			}																			/* FCLNX-GPL-038	*/
			target->status = HFC_NON_STATUS;
		}
	}
	
	hfc_w_stop( ap, HFC_LINKUP_TMR );
	hfc_w_start( ap, HFC_LINKUP_TMR );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		hfc_w_stop( ap, HFC_WLINKUP_CNT_TMR );
		hfc_w_start( ap, HFC_WLINKUP_CNT_TMR );
	}
#endif	/* FCLNX-GPL-FX-424 */
																	/* @MLPF STR */
mlpf_shadow_skip:
																	/* @MLPF END */
	memset(logdata,0,16);
	hfc_hand2_trace(HFC_TRC_LINKDOWN_INT, 0x11, ap, NULL, NULL, 0, 0, 0);
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16) ;

	HFC_2B_TO_2L(detail, mbox->mb_intreq.type.fwintreq0.detail);
	if(!mig){															/* FCLNX-GPL-489 */
		if ( detail == HFC_SFP_NOT_SUPPORT ){
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_OPTERR0, 0x9c,logdata,16);
			ap->isol_detail = HFC_ISOLATE_SFPNOTSUPPORT;					/* FCLNX-0514 */
		}
		else if ( detail == HFC_SFP_RECV_ERR ){
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR5, 0x9d,logdata,16);
			ap->isol_detail = HFC_ISOLATE_SFPFAIL;							/* FCLNX-0514 */
		}
		else if ( detail == HFC_SFP_TRAN_ERR ){
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR5, 0x9e,logdata,16);
			ap->isol_detail = HFC_ISOLATE_SFPFAIL;							/* FCLNX-0514 */
		}
		else if ( detail == HFC_SFP_DOWN ){
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR5, 0x9f,logdata,16);
			ap->isol_detail = HFC_ISOLATE_SFPDOWN;							/* FCLNX-0514 */
		}
		else if ( detail == HFC_MCK_PCHK ){									/* FCLNX_GPL-0109 */
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR9, 0xad,logdata,16);	/* FCLNX_GPL-0114 */
		}
		else
		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERRB, 0x14,logdata,16);
	}
	if (( ap->isol_detail ) || ( detail == HFC_MCK_PCHK )|| (mig) ) {			/* FCLNX_GPL-0109 *//* FCLNX-GPL-489 */
		if ( ap->isol_detail!=0 ) {
			set_bit(HFC_ISOL, (ulong *)&ap->status);
		}
		
		hfc_w_stop( ap, HFC_LINKUP_TMR );
		hfc_w_stop( ap, HFC_LINKUP2_TMR );								/* FCLNX_GPL-0109 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
			hfc_w_stop( ap, HFC_WLINKUP_CNT_TMR );
#endif	/* FCLNX-GPL-FX-424 */
		
		clear_bit(HFC_ONLINE, (ulong *)&ap->status);
		if (( detail == HFC_MCK_PCHK )||( ap->isol_detail==0 )){									/* FCLNX_GPL-0109 *//* FCLNX-GPL-489 */
			set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status );
		}
		
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++){
			target = hfc_hash_target_info(ap, lp);
			if( target != NULL ){
				hfc_cancel_scsi_cmd(ap, target, 0, NULL, SCS_INTR_LINKDOWN,
						 HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);	/* FCLNX-0553 */
				target->status = HFC_NON_STATUS;
			}
		}
		hfc_wwnverify_linkup_timeout(ap, NULL, 0);
		
		
		if( (test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status)) || (ap->initialize == 1) )	/* FCLNX-0514 */
		{					
			HFC_DBGPRT(KERN_ERR "hfcldd%d : hfc_watchdog() lock fail. \n", ap->dev_minor);
			unlock_mailbox( ap ) ;
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
			clear_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status);
		}
		
		if ( HFC_MMODE_CHECK_SHADOW(ap) )
		{
			switch(ap->isol_detail){	/* FCLNX-GPL-489 */
				case HFC_ISOLATE_SFPFAIL:
					hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_SFP_FAIL, HFC_ENABLE_LPAR_STATE);
					break;
				case HFC_ISOLATE_SFPNOTSUPPORT:
					hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_SFP_NOTSUPT, HFC_ENABLE_LPAR_STATE);
					break;
				case HFC_ISOLATE_SFPDOWN:
					hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_SFP_DOWN, HFC_ENABLE_LPAR_STATE);
					break;
			}							/* FCLNX-GPL-489 */
		}
	}

	HFC_EXIT("hfc_linkdown_intreq");
	
	return ;
}


/*
 * Function:    hfc_linkup_intreq
 *
 * Purpose:     This routine deals with Linkup interrupt response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_linkup_intreq(
	struct adap_info        *ap)
{
	struct mp_adap_info     *mpap;
	struct mailbox          *mbox;
	
	HFC_ENTRY("hfc_linkup_intreq");	

	mpap = ap->mp_adap_info;
	mbox = ap->mb;
	hfc_hand2_trace( HFC_TRC_LINKUP_INT, 0x00, ap, NULL, NULL, 0, 0, 0);
	
	/* @MLPF STR */
	if (HFC_MMODE_CHECK_SHADOW(ap) )
	{
		if ( hfc_mlpf_config_check(ap) )
		{	/* FCLNX-GPL-186 */
			if(ap->initialize != 0)
			{
				hfc_wake_up(&ap->init_event,&ap->int_a_poll);
			}
			goto mlpf_shadow_skip;
		}	/* FCLNX-GPL-186 */
	}
																	/* @MLPF END */
	if ( HFC_MMODE_CHECK_MLPF(ap) )
		hfc_mlpf_cca_setup(ap);		/* FCLNX-GPL-494 */
	
	/* Store F/W init table information to adap_info */
	hfc_copy_iocinfo(ap);									/* FCWIN-0082 */

//	if((hfc_manage_info.hfcplus_enable)&&(ap->lds_errcnt_info!=NULL))
	if(ap->ld_err_limit_s){	/* FCLNX-GPL-349 */
		if(test_bit(HFC_ONLINE, (ulong *)&ap->status) || test_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status))	/* FCLNX-0506 *//* FCLNX-GPL-FX-424 */
		{
			if(ap->mck_linkup == HFC_LINKUP_NOMCK){	/* FCLNX-595 */
				if(hfc_manage_info.hfcldd_mp_mod){
					hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_LDS_ERR);	/* FCLNX-GPL-349 */
				}
				else{
					hfc_watched_errcount_i(ap, NULL, HFC_LDS_ERR);	/* FCLNX-GPL-349 */
				}
			}
		}
	}
	ap->mck_linkup = HFC_LINKUP_NOMCK;			/* FCLNX-595 */

	if( !test_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status) )
	{
		set_bit(HFC_ONLINE, (ulong *)&ap->status);
	}
	
	clear_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
		hfc_w_stop( ap, HFC_WLINKUP_CNT_TMR );
	}
#endif	/* FCLNX-GPL-FX-424 */

	hfc_w_stop( ap, HFC_LINKUP_TMR );
	hfc_w_stop( ap, HFC_LINKUP2_TMR );						/* FCLNX-0241 */
	
	if ( !HFC_MMODE_CHECK_SHADOW(ap) ) {                    /* FCWIN-0353 */
		hfc_wwnverify_linkup(ap, NULL, 0, 0);
	}
	else	/* FCLNX-GPL-186 */
	{	/* for shadow driver */
		if(ap->initialize != 0)
		{
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
	}		/* FCLNX-GPL-186 */
	
	if( !test_bit(HFC_DIAG_END , (ulong *)&ap->status) )
	{
		memset(logdata,0,16);
		memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16) ;
		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT1,0x15,logdata,16) ;
	}
	else
	{
		clear_bit(HFC_DIAG_END , (ulong *)&ap->status);
	}
	
																	/* @MLPF STR */
mlpf_shadow_skip:
																	/* @MLPF END */
	HFC_EXIT("hfc_linkup_intreq");	

	return ;
}


/*
 * Function:    hfc_plogi_intreq
 *
 * Purpose:     This routine deals with PLOGI interrupt response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  mbox       - pointer to mailbox
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_plogi_intreq(
	struct adap_info        *ap,
	struct mailbox          *mbox)
{
	uchar                   find; /* target detection flag	*/
	struct target_info      *target = NULL;
	uint                    lp;
	uint64_t                ww_name;
	uint64_t                node_name;
	
	hfc_hand2_trace( HFC_TRC_PLOGI_INT, 0x00, ap, NULL, NULL, 0, 0, 0);
	HFC_ENTRY("hfc_plogi_intreq");	
	
																	/* @MLPF STR */
	if ( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
	find = FALSE ;
	
	/* Search target by comparing WWN of receiving PLOGI with WWN of target under */
	/* the designated adapter */
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)					/* FCWIN-0082 */
	{
		target = hfc_hash_target_info(ap, lp);
		
		if (target != NULL)
		{
			HFC_8B_TO_8L(ww_name, mbox->mb_intreq.type.fwintreq0.un.plogi.port_name);
			HFC_8B_TO_8L(node_name, mbox->mb_intreq.type.fwintreq0.un.plogi.node_name);
			if( (target->ww_name == ww_name) && (target->node_name == node_name) )
			{
				find = TRUE ;
				break ;
			}
		}
	}
	
	if(target != NULL){ /* FCLNX-GPL-069 */
		if( (find == TRUE) && (!test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status)) )
		{	/* A target exists with unfinished LOGIN status */
			hfc_cancel_scsi_cmd(ap, target, 0, NULL, SCS_INTR_PLOGI,
								 HFC_CSCSI_INHALT, TRUE, FALSE, HFC_FLASH_TARGET);	/* FCLNX-0429 */
			
			clear_bit(HFC_NEED_PDISC, (ulong *)&target->status);
			clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
			clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);
			set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);					/* FCWIN-0146 */
			clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);				/* FCLNX-GPL-038 */
			
			if ((ap->connect_type == HFC_SWITCH ) 						/* FCWIN-0082STR*/
			||  ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
				set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);				/* FCWIN-0146 */
				ap -> next_gidpn = TRUE;
				hfc_issue_gidpn( ap, target );	/* Issue GID_PN */
			}
			else { 														/* FCWIN-0082END*/
				hfc_enque_login_req(ap, target);
			}
		}
	}
	if (find == FALSE) { /* FCLNX-GPL-491 Limit Log Mode On */	/* FCLNX-GPL-FX-479 */
		hfc_hand2_trace( HFC_TRC_PLOGI_INT, 0x20, ap, NULL, NULL, 0, 0, 0);
		return;
	}
	
	memset(logdata,0,16);
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16) ;
	hfc_errlog(ap,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x16,logdata,16) ;
	
																	/* @MLPF STR */
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
																	/* @MLPF END */
	
	HFC_EXIT("hfc_plogi_intreq");	
	return ;
}


/*
 * Function:    hfc_logo_intreq
 *
 * Purpose:     This routine deals with LOGO interrupt response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  mbox       - pointer to mailbox
 *
 * Returns:    -
 *
 * Notes:       
 */
void hfc_logo_intreq(
	struct adap_info        *ap,
	struct mailbox          *mbox)
{
	uchar                   find,find2; /* Object target detection flag	FCWIN-0082*/
	struct target_info      *target = NULL;
	uint                    lp;
	uint64_t                ww_name;
	uint                    mb_port_id;
	
	
	hfc_hand2_trace( HFC_TRC_LOGO_INT, 0x00, ap, NULL, NULL, 0, 0, 0);
	HFC_ENTRY("hfc_logo_intreq");	
	
																	/* @MLPF STR */
	if (HFC_MMODE_CHECK_SHADOW(ap) )
	{
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
	find = FALSE ;
	find2 = TRUE ;
	

	/* Search target by comparing WWN and port_id of receiving LOGO with WWN of */
	/* target under the designated adapter */

	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)					/* FCWIN-0083 */
	{
		target = hfc_hash_target_info(ap, lp);
		
		if (target != NULL)
		{
			HFC_8B_TO_8L(ww_name, mbox->mb_intreq.type.fwintreq0.un.logo.port_name);
			HFC_4B_TO_4L(mb_port_id, mbox->mb_intreq.type.fwintreq0.un.logo.n_port_id);
			if (target->ww_name == ww_name) { 
				if ((target->scsi_id & 0x00ffffff) == (mb_port_id & 0x00ffffff)) /* FCWIN-0082 */
				{
					find = TRUE ;
					break ;
				}
				else {
					if ( (ap->connect_type == HFC_SWITCH )
						|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00)))/* FCWIN-0185 */
					{
						find = TRUE ;
						find2 = FALSE ;
						break ;
					}
				}
			}
		}
	}
	
	if(target != NULL){ /* FCLNX-GPL-069 */
		if( (find == TRUE) && !test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )
		{	/* A target exists with unfinished LOGIN status */
			hfc_cancel_scsi_cmd(ap, target, 0, NULL, SCS_INTR_LOGO, 
								HFC_CSCSI_INHALT, TRUE, FALSE, HFC_FLASH_TARGET);	/* FCLNX-0429 */
		
			clear_bit(HFC_NEED_PDISC, (ulong *)&target->status);
			clear_bit(HFC_SCN_RESP, (ulong *)&target->status);
			clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);
			set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);					/* FCWIN-0146 */
			clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);				/* FCLNX-GPL-038 */
			
			if ( find2 == TRUE ) {								/* FCWIN-0082 */
				set_bit(HFC_WWN_VALID, (ulong *)&target->flags);
				
				if( test_bit(HFC_WAIT_PDISC, (ulong *)&target->status) )
				{
					set_bit(HFC_WPDISC_LOGO_RESP, (ulong *)&target->status);
				}
				
				hfc_enque_login_req(ap, target);
			}
			else {												/* FCWIN-0082STR*/
				/* FC-SW and WWN of the target is identical to the existing target, */
				/* but scsi_id is mismatch between the two 							*/
				/*	-> Issue GID_PN to renew port number 							*/
				set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);
				ap -> next_gidpn = TRUE;
				hfc_issue_gidpn( ap, target );
			}													/* FCWIN-0082END*/
		}
	}
	/* FCLNX-GPL-503 */
	if ((ap->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) {
		hfc_hand2_trace( HFC_TRC_LOGO_INT, 0x20, ap, NULL, NULL, 0, 0, 0);
		return;
	}
	
	memset(logdata,0,16);
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16) ;
	hfc_errlog(ap,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x17,logdata,16) ;
	
																	/* @MLPF STR */
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
																	/* @MLPF END */
	
	HFC_EXIT("hfc_logo_intreq");
	return;
}


/*
 * Function:    hfc_scn_intreq
 *
 * Purpose:     This routine deals with SCN interrupt response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  mbox       - pointer to mailbox
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_scn_intreq(
	struct adap_info        *ap,
	struct mailbox          *mbox)
{
	struct target_info      *target = NULL;
	ushort                  scn_cnt = 0;		/* FCLNX-0637 */
	ushort                  scn_no  = 0;		/* FCLNX-0637 */
	uint                    lp;
	uint                    port_id;
	uint                    mb_port_id;
	char                    find;

	HFC_ENTRY("hfc_scn_intreq");

	hfc_hand2_trace( HFC_TRC_SCN_INT, 0x00, ap, NULL, NULL, 0, 0, 0);
	
																	/* @MLPF STR */
	if ( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
	
	/* Count up the number of targets with the SCN notification */
	HFC_2B_TO_2L(scn_cnt, mbox->mb_intreq.type.fwintreq0.un.scn.pay_len);
	scn_cnt = (scn_cnt - 4)/4 ;
	scn_no = 0 ;
	
	while( scn_no < scn_cnt )
	{
		find = FALSE ;
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)				/* FCWIN-0083 */
		{
			target = hfc_hash_target_info(ap, lp);
			
			if (target != NULL)
			{
				port_id = (uint)target->scsi_id ;
				HFC_4B_TO_4L(mb_port_id, mbox->mb_intreq.type.fwintreq0.un.scn.n_port_id[scn_no]);
				
				if( (port_id & 0x00ffffff) == (mb_port_id & 0x00ffffff) )
				{
					if( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) )	/* FCLNX-0274 */ /* FCLNX-GPL-038 */
					{
						set_bit(HFC_SCN_RESP, (ulong *)&target->status);
						if ( (ap->connect_type == HFC_SWITCH) 	 /* FCWIN-0082STR*/
							|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
							set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);
							ap -> next_gidpn = TRUE;
						}
						else {
							set_bit(HFC_NEED_PDISC, (ulong *)&target->status);
							hfc_enque_pdisc_req(ap, target);
						}										/* FCWIN-0082END*/

						find = TRUE ;
						break ;
					}
				}
			}
		}
		scn_no++ ;
	}
	
	hfc_wwnverify_scn(ap ,target, 0);
	
	memset(logdata,0,16);
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16) ;
	hfc_errlog(ap,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x18,logdata,16) ;
	
																	/* @MLPF STR */
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
																	/* @MLPF END */
	
	HFC_EXIT("hfc_scn_intreq");
	
	return;
}


/*
 * Function:    hfc_rscn_intreq
 *
 * Purpose:     This routine deals with RSCN interrupt response
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  mbox       - pointer to mailbox
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_rscn_intreq(
	struct adap_info        *ap,
	struct mailbox          *mbox)
{
	struct target_info      *target = NULL;
	ushort                  scn_cnt = 0;		/* FCLNX-0637 */
	ushort                  scn_no  = 0;		/* FCLNX-0637 */
	uint                    lp;
	int                     p_mask;
	uint                    port_id;
	uint                    mb_port_id;
	char                    find = FALSE;
	
	HFC_ENTRY("hfc_rscn_intreq");	
	hfc_hand2_trace( HFC_TRC_RSCN_INT, 0x00, ap, NULL, NULL, 0, 0, 0);

																	/* @MLPF STR */
	if ( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		goto mlpf_shadow_skip;
	}																/* @MLPF END */

	/*-- Count up the number of targets with the RSCN notification */
	HFC_2B_TO_2L(scn_cnt, mbox->mb_intreq.type.fwintreq0.un.rscn.pay_len);	/* FCLNX-0612 */
	scn_cnt = (scn_cnt - 4)/4 ;
	scn_no = 0 ;
	
	while( scn_no < scn_cnt ) /* Repeat to check all RSCN notification */
	{
		find = FALSE ;
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)			 /* FCWIN-0083 */
		{	/* Repeat to check all targets */
			target = hfc_hash_target_info(ap, lp);
			
			if (target != NULL)
			{/* When target exists */
				port_id = (uint)target->scsi_id ;
				HFC_4B_TO_4L(mb_port_id, mbox->mb_intreq.type.fwintreq0.un.rscn.n_port_id[scn_no]);
				switch(mb_port_id & 0x03000000 )
				{
					case 0x00000000 :		/* port address   */
						p_mask = 0x00ffffff ;
						break ;
					case 0x01000000 :		/* Area address   */
						p_mask = 0x00ffff00 ;
						break ;
					case 0x02000000 :		/* Domain address */
						p_mask = 0x00ff0000 ;
						break ;
					default :				/* fablic address */
						p_mask = 0 ;
				}
				
				if( p_mask == 0 )
				{
					if( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) )	/* FCLNX-0274 */ /* FCLNX-GPL-038 */
					{
						set_bit(HFC_SCN_RESP, (ulong *)&target->status);
						
						if ( (ap->connect_type == HFC_SWITCH) 	/* FCWIN-0082STR*/
							|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
							set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);
							ap -> next_gidpn = TRUE;
						}
						else {
							set_bit(HFC_NEED_PDISC, (ulong *)&target->status);
							hfc_enque_pdisc_req(ap, target);
						}										/* FCWIN-0082END*/
						find = TRUE ;
					}
				}
				else if( (port_id & p_mask) == (mb_port_id & p_mask) )
				{
					if( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) )	/* FCLNX-0274 */ /* FCLNX-GPL-038 */
					{
						set_bit(HFC_SCN_RESP, (ulong *)&target->status);

						if ( (ap->connect_type == HFC_SWITCH) 	/* FCWIN-0082STR*/
							|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
							set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);
							ap -> next_gidpn = TRUE;
						}
						else {
							set_bit(HFC_NEED_PDISC, (ulong *)&target->status);
							hfc_enque_pdisc_req(ap, target);
						}										/* FCWIN-0082END*/

						find = TRUE ;
					}
				}
			}
		}
		scn_no++ ;
	}
	
	hfc_wwnverify_scn(ap, target, 0);	/* FCLNX-GPL-503 */
	
	if ((ap->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On */
		hfc_hand2_trace( HFC_TRC_RSCN_INT, 0x20, ap, NULL, NULL, 0, 0, 0);
		return;
	}
	
	memset(logdata,0,16);
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16) ;
	hfc_errlog(ap,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x18,logdata,16) ;
	
																	/* @MLPF STR */
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
																	/* @MLPF END */
	
	HFC_EXIT("hfc_rscn_intreq");
	return;
}


/*
 * Function:    hfc_xrb_resp
 *
 * Purpose:     This routine deals with XRB interrupt 
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  xrb_out_no - outpointer to XRB 
 *  xrb_in_no  - inpointer XRB
 *
 * Returns:    -
 *
 * Notes:       
 */
void hfc_xrb_resp(
	struct adap_info        *ap,
	uint                    xrb_out_no,
	uint                    xrb_in_no)
{
	struct target_info      *target=NULL;
	struct hfc_pkt          *hfcp=NULL;
	struct hfc_pkt          *target_hfcp;
	struct hfc_pkt          *hfcp_wk;
	struct hfc_pkt          *hfcp_top_p;
	struct hfc_pkt          *hfcp_end_p;
	uint                    xrb_cnt;
	uint                    xrb_cnt_org ;
	uint                    xrb_no ;
	uint                    xrb_out_page ;
	uint                    xrb_out_entry ;
	uint                    xrb_outp;
	uint                    targetid;
	uint                    lunid;
//	uchar                   rid;                /* FCLNX-0371 */
	uint                    fw_xrb_outp;
	uchar                   hfcp_find;
	uchar                   abend_code ;
	uchar                   abend_code2 = 0;    /* FCLNX-0371 */
	uchar                   seq_no;
	int                     link_rc ;
	int                     i ;
	
		
/* Caliculate the number of xrb */
	if( xrb_out_no < xrb_in_no )
		xrb_cnt = xrb_in_no - xrb_out_no ;
	else
	{
		xrb_cnt = ap->xrb_max;         /*--    FCWIN-0071      --*/
		xrb_cnt -= xrb_out_no ;
		xrb_cnt += xrb_in_no ;
	}
	xrb_cnt_org = xrb_cnt ;
	

	xrb_no = xrb_out_no ;
	/* Setup Xrb_outp */
	ap->xrb_no = (ushort)xrb_no ;
	
	abend_code = 0 ;
	
	/* Check all xrbs */
	while( xrb_cnt > 0 )
	{
#if 0 /* FCLNX-GPL-549 */
		/* FCLNX-GPL-167 */
		if( HFC_MMODE_CHECK_SHARED(ap)  && !(HFC_MMODE_CHECK_SHADOW(ap) ) ) /* FCLNX-0371 */
		{
			rid = ap->xrb[xrb_no].drv_work.rid;
			if ( rid != ap->rid )
			{
				abend_code2 = HFC_ABEND_RID_INVALID;
				
				memset(logdata,0,16);                                               /* FCLNX-0387 */
				memcpy(logdata, (uchar*)&rid, 1);
				memcpy(&logdata[4], (uchar*)&ap->rid, 4) ;
				memcpy(&logdata[8], (uchar*)&xrb_no,4);
				
				hfc_errlog(
					ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4,
					0xb2, logdata, 16) ;                                            /* FCLNX-0387 */

				hfc_hand2_trace(
					HFC_TRC_XRBRSP, 0x24, ap, NULL, NULL, 
					(uint64_t)xrb_cnt, 0, 0);
				
				xrb_cnt-- ;
				xrb_no++ ;
				if( xrb_no >= ap->xrb_max)      /*--    FCWIN-0071      --*/
					xrb_no = 0 ;
				ap->xrb_no = (ushort)xrb_no ;
				
				/* Calculate xrb_outp from xrb_no and set it to ap->fw_init_p.xrb_outp */
				xrb_out_page = xrb_no / HFC_XRB_PER_PAGE ;
				xrb_out_entry = xrb_no % HFC_XRB_PER_PAGE ;
				xrb_outp = ((xrb_out_page << 16) & 0x00ff0000) | (xrb_out_entry & 0x0000ffff) ; /* FCWIN-156 STR*/
				HFC_4B_TO_4L(fw_xrb_outp, xrb_outp);
				ap->fw_init_p->xrb_outp = fw_xrb_outp;                                          /* FCWIN-156 END*/

				continue ;
			}
		}                                                                                   /* FCLNX-0371 */
#endif /* FCLNX-GPL-549 */                                                                                  /* FCLNX-0371 */
		
		if( !(ap->xrb[xrb_no].flag & HFC_XRB_VALID)
		 || (ap->xrb[xrb_no].skip & HFC_XRB_SKIP) )							/* FCWIN-0152 */ /* FCLNX-008 */
		{	/* If xrb valid flag is zero */
			if ( !(ap->xrb[xrb_no].skip & HFC_XRB_SKIP) )					/* FCWIN-0152 */ /* FCLNX-008 */
			{
				abend_code = HFC_ABEND_XRB_INVALID ;
				memcpy(&logdata[0],(uchar *)&xrb_no,4);
				memcpy(&logdata[4],(uchar *)&xrb_in_no,4);
				memcpy(&logdata[8],(uchar *)&xrb_cnt,4);
				memcpy(&logdata[12],(uchar *)&xrb_cnt_org,4);
				hfcp = (struct hfc_pkt *)(ulong)ap->xrb[xrb_no].drv_work.hfc_pkt;
				hfc_errlog(ap,NULL,hfcp,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,0x1d,logdata,16) ;
			}																/* FCWIN-0152 */
			else
			{
				ap->xrb[xrb_no].flag &= ~HFC_XRB_VALID;
				ap->xrb[xrb_no].skip &= ~HFC_XRB_SKIP; /* FCLNX-008 */
			}

			hfc_hand2_trace(
				HFC_TRC_XRBRSP, 0x11, ap, NULL, NULL, 
				(uint64_t)xrb_cnt, 0, 0);
			
#if _HFC_DEBUG_HAND_00
			if ( !(ap->xrb[xrb_no].skip & HFC_XRB_SKIP) )					/* FCWIN-0152 */ /* FCLNX-008 */
			{
				ap->link_dead_cnt = HFC_LINK_DEAD_CNT + 1 ; 
				hfc_abend(ap,abend_code) ;
				return ;
			}																/* FCWIN-0152 */
#endif
			xrb_cnt-- ;
			xrb_no++ ;
			if( xrb_no >= ap->xrb_max)      /*--    FCWIN-0071      --*/
				xrb_no = 0 ;
			ap->xrb_no = (ushort)xrb_no ;
			
			/* Calculate xrb_outp from xrb_no and set it to ap->fw_init_p.xrb_outp */
			xrb_out_page = xrb_no / HFC_XRB_PER_PAGE ;
			xrb_out_entry = xrb_no % HFC_XRB_PER_PAGE ;
			xrb_outp = ((xrb_out_page << 16) & 0x00ff0000) | (xrb_out_entry & 0x0000ffff) ; /* FCWIN-156 STR*/
			HFC_4B_TO_4L(fw_xrb_outp, xrb_outp);
			ap->fw_init_p->xrb_outp = fw_xrb_outp;                                          /* FCWIN-156 END*/

			continue ;
		}
		
		/* Check hfc_pkt address */						/* FCLNX-GPL-0135 */
		hfcp_wk = (struct hfc_pkt *)(ulong)ap->xrb[xrb_no].drv_work.hfc_pkt;
		hfcp_find= FALSE;
		
		if (hfc_manage_info.hfcldd_mp_mod) {
			if (hfcp_wk != NULL) {						/* FCLNX-GPL-0244 */
				if (test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp_wk->cmd_flags )) {
					hfcp_find = TRUE ;
				}
			}
		}												/* FCLNX-GPL-0244 */
		
		if (hfcp_find == FALSE) {						/* FCLNX-GPL-0244 */
			for (i=0;i<HFC_PKT_POOL_NUM;i++) {
				hfcp_top_p = ap->pkt_pool[i];
				hfcp_end_p = hfcp_top_p + HFC_PKT_POOL_SIZE;	/* FCLNX-GPL-FX-124 */
				if( (hfcp_wk >= hfcp_top_p) && (hfcp_wk < hfcp_end_p) ) {
					hfcp_find = TRUE ;
					break;
				}
			}
		}
		
		if (hfcp_find == FALSE) {
			hfc_hand2_trace(HFC_TRC_XRBRSP, 0x23, ap, target,NULL, (uint64_t)xrb_cnt, 0, 0);
			
			abend_code = HFC_ABEND_HFCPKT_CHK ;
			seq_no = 0x01;
			memset(logdata,0,16);
			memcpy(&logdata[0],(uchar *)&seq_no,1);
			memcpy(&logdata[8],(uchar *)&hfcp_wk,8);
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xBE,logdata,16) ;
			break ;
		}												/* FCLNX-GPL-0135 */
		
		/*-- Reset xrb valid flag */
		ap->xrb[xrb_no].flag &= ~HFC_XRB_VALID ;
		
		/* Get hfc_pkt, target_id, lun */
		hfcp = (struct hfc_pkt *)(ulong)ap->xrb[xrb_no].drv_work.hfc_pkt;
		
		if(!test_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags) )
		{

			HFC_DBGPRT(" hfcldd : xrb_resp CFLAG INVALID %x \n", hfcp->cmd_flags);

			xrb_cnt-- ;
			xrb_no++ ;
			if( xrb_no >= ap->xrb_max)      /*--    FCWIN-0071      --*/
				xrb_no = 0 ;
			ap->xrb_no = (ushort)xrb_no ;
			
			/* Calculate xrb_outp from xrb_no and set it to ap->fw_init_p.xrb_outp */
			xrb_out_page = xrb_no / HFC_XRB_PER_PAGE ;
			xrb_out_entry = xrb_no % HFC_XRB_PER_PAGE ;
			xrb_outp = ((xrb_out_page << 16) & 0x00ff0000) | (xrb_out_entry & 0x0000ffff) ; /* FCWIN-156 STR*/
			HFC_4B_TO_4L(fw_xrb_outp, xrb_outp);
			ap->fw_init_p->xrb_outp = fw_xrb_outp;                                          /* FCWIN-156 END*/

			continue ;
		}

		targetid = ap->xrb[xrb_no].drv_work.target_id;
		lunid = ap->xrb[xrb_no].drv_work.lun_id;
		
		target = hfc_hash_target_info(ap, targetid);
		
		if(target == NULL)
		{
			HFC_DBGPRT(" hfcldd : xrb_resp search target NULL (target_id = %d). \n", targetid);

			hfc_hand2_trace(
				HFC_TRC_XRBRSP, 0x12, ap, target,NULL, 
				(uint64_t)xrb_cnt, 0, 0);
			target_hfcp = NULL;
		}
		else
		{
			target_hfcp = target->we_que_top[lunid % HASH_T_NUM];
		}
		hfcp_find= FALSE;
		
		while( target_hfcp != NULL)
		{
			/* Check whether saved hfc_pkt at initialization is in wait_end_que */
			if( target_hfcp == hfcp )
			{
				hfcp_find = TRUE ;
				break ;
			}
			target_hfcp = target_hfcp->cmd_forw;
		}
		
		if(hfcp_find == FALSE )
		{
			
			HFC_DBGPRT(" hfcldd : xrb_resp hfc_pkt is invalid \n");
			
			/* Device and target cancellation has already processed */
			xrb_cnt-- ;
			xrb_no++ ;
			if( xrb_no >= ap->xrb_max )     /*--    FCWIN-0071      --*/
				xrb_no = 0 ;
			ap->xrb_no = (ushort)xrb_no ;
			
			/* Calculate xrb_outp from xrb_no and set it to ap->fw_init_p.xrb_outp */	
			xrb_out_page = xrb_no / HFC_XRB_PER_PAGE ;
			xrb_out_entry = xrb_no % HFC_XRB_PER_PAGE ;
			xrb_outp = ((xrb_out_page << 16) & 0x00ff0000) | (xrb_out_entry & 0x0000ffff);
			HFC_4B_TO_4L(fw_xrb_outp, xrb_outp);
			ap->fw_init_p->xrb_outp = fw_xrb_outp;
			
			hfc_hand2_trace(
				HFC_TRC_XRBRSP, 0x22, ap, target, hfcp, 
				(uint64_t)xrb_cnt, 0, 0);
			
			continue ;
		}
		
		if ( target != NULL)
		{
			if( (link_rc = hfc_link_chk(ap, target, hfcp)) == 0 )
			{
				ap->link_dead_cnt = 0 ;
				if( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)		||
					test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags)	||	/* FCLNX-0429 */
					test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)	||
					test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags)	)
				{	/* Check task management command */
					HFC_DBGPRT(" hfcldd : xrb_resp call hfc_task_mgm_chk \n");
					hfc_task_mgm_chk(ap, hfcp) ;
				}
				else
				{	/* Check SCSI command */
					hfc_scsi_chk(ap, target, hfcp);
				}
			}
			else {
				if( link_rc == -1 ){
					HFC_DBGPRT(" hfcldd : xrb_resp link_rc = -1; abend\n");
					abend_code = HFC_ABEND_CCC ;
				}
			}
		}
		
		/*-- Renew xrb outp --*/
		xrb_cnt-- ;
		xrb_no++ ;
		if( xrb_no >= ap->xrb_max )     /*--    FCWIN-0071      --*/
			xrb_no = 0 ;
		ap->xrb_no = (ushort)xrb_no ;
		
		/* Calculate xrb_outp from xrb_no and set it to ap->fw_init_p.xrb_outp */
		xrb_out_page = xrb_no / HFC_XRB_PER_PAGE ;
		xrb_out_entry = xrb_no % HFC_XRB_PER_PAGE ;
		xrb_outp = ((xrb_out_page << 16) & 0x00ff0000) | (xrb_out_entry & 0x0000ffff);
		HFC_4B_TO_4L(fw_xrb_outp, xrb_outp);
		ap->fw_init_p->xrb_outp = fw_xrb_outp;


	}   
	
	if( HFC_MMODE_CHECK_SHARED(ap) && !(HFC_MMODE_CHECK_SHADOW(ap) ) )          /* FCLNX-0371 */
	{
		if ( abend_code2 == HFC_ABEND_RID_INVALID ) /* RID INVALID is stronger than CCC or XRB invalid */
		{
			abend_code = abend_code2;
		}
	}                                                                           /* FCLNX-0371 */
	
	if( abend_code != 0 ){	
		HFC_DBGPRT(" hfcldd : xrb_resp call hfc_abend\n");
		hfc_abend(ap,abend_code) ;
	}

	hfc_hand2_trace(HFC_TRC_XRBRSP, 0x10, ap, target, hfcp, (uint64_t)xrb_cnt, 0, 0);

	return;
}


/*
 * Function:    hfc_link_chk
 *
 * Purpose:     This routine check XCRB
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  target     - pointer to target_info
 *  hfcp       - pointer to hfc_pkt
 *
 * Returns:     
 *  HFC_LINK_CHK_OK  - XCRB check is finished successufully.
 *  HFC_LINK_CHK_ERR - XCRB check is finished with errors.
 *  HFC_LINK_CHK_CCC - ccc has occured.
 *
 * Notes:       
 */
uint hfc_link_chk(
	struct adap_info		*ap,
	struct target_info		*target,
	struct hfc_pkt			*hfcp)
{
	uint					xrb_no ;
	uint					func_rc = 0 ;
	struct scsi_cmnd		*Scmd;
	uint					result;
	uint					cmd_flags;
	struct dev_info			*dev=NULL;
	
	xrb_no = ap->xrb_no;
	Scmd = hfcp->cmd_pkt;
	
	if((ap->xrb[xrb_no].xcc == HFC_XCC_END) && !(ap->xrb[xrb_no].fsb & HFC_FSB_MASK) )
	{	/* Start process completes without error */
		/*  pc/icc/ccc/cdc zero */
		
		return HFC_LINK_CHK_OK ;
	}
	
	
	HFC_DBGPRT(" hfcldd : hfc_link_chk - error\n");

//	if(hfc_manage_info.hfcplus_enable){
	if(ap->if_err_limit){                              /* FCLNX-0506 */
		if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-GPL-349 */
			hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_IF_ERR);  /* FCLNX-0506 */
		}
		else{
			hfc_watched_errcount_i(ap, NULL, HFC_IF_ERR);  /* FCLNX-GPL-349 */
		}
	}

	/* Error has occured */
	if (!test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )		/* FCWIN-0153 */
	{
		result = DID_ERROR;
		Scmd->result |= ( result << 16);
		hfcp->adap_status = SCS_NO_CMND;
	}
	if( ap->xrb[xrb_no].xcc != HFC_XCC_END )
	{	/* XCB != 0x80 */
		result = DID_ERROR;
		Scmd->result |= ( result << 16);
		func_rc = HFC_LINK_CHK_CCC ;
		hfcp->adap_status = SCS_TRANSPORT_DEAD;				/* FCWIN-0151 */
	}

	if( ap->xrb[xrb_no].xcc == HFC_XCC_END )
	{	/* FSB = pc /icc/ccc/cdc */
		if( ap->xrb[xrb_no].fsb & HFC_FSB_CDC )
		{	/* FSB = CDC */
			result = DID_ERROR;
			Scmd->result |= ( result << 16);
			func_rc = HFC_LINK_CHK_ERROR;
			hfcp->adap_status = SCS_IO_CDC;		/* FCWIN-0534 */		
		}
		else if( ap->xrb[xrb_no].fsb & HFC_FSB_CCC )
		{	/* FSB = CCC */
			result = DID_ERROR;
			Scmd->result |= ( result << 16);
			func_rc = HFC_LINK_CHK_CCC;
			hfcp->adap_status = SCS_IO_CCC;		/* FCWIN-0151 */
		}
		else if( ap->xrb[xrb_no].fsb & HFC_FSB_ICC )
		{	/* FSB = ICC */
#if _HFC_DEBUG_HAND_01
			switch( ap->xrb[xrb_no].err_code )
#else
			switch( ap->xrb[xrb_no].err_code[0] )
#endif
			{
			case HFC_ICC_TIMEOUT :
				result = DID_ERROR;
				Scmd->result |= ( result << 16);
				func_rc = HFC_LINK_CHK_ERROR;
				hfcp->adap_status = SCS_IO_ICC_TIMEOUT;
				break ;
			case HFC_ICC_NO_RESP :
				result = DID_ERROR;
				Scmd->result |= ( result << 16);
				func_rc = HFC_LINK_CHK_ERROR;
				hfcp->adap_status = SCS_IO_ICC_NO_RESP;	/* FCWIN-0151 */
				break ;
			default :
				result = DID_ERROR;
				Scmd->result |= ( result << 16);
				func_rc = HFC_LINK_CHK_ERROR;
				hfcp->adap_status = SCS_IO_ICC_LINK_CHK;/* FCWIN-0151 */
			}
		}
		else
		{	/* FSB = PC */
			result = DID_ERROR;
			Scmd->result |= ( result << 16);
			func_rc = HFC_LINK_CHK_ERROR;
			hfcp->adap_status = SCS_ADAP_FAILURE;		/* FCWIN-0151 */
			
		}
#if _HFC_DEBUG_HAND_02
		hfc_abend(ap,0xfc); 		/*	 icc,ccc,cdc */
		return func_rc ;	
#endif
	}
	
	if(	test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags) ||
		test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) 		)
	{	/* Response of target reset */
		
		/* Output soft log */
//		hfc_notify_tout(ap, target);								/* FCLNX-GPL-0152 */
		
		/* Dequeue hfc_pkt from wait_end_que */
		hfc_deque_we_que(ap, target, hfcp);							/* FCWIN-0153 END */
		
		hfc_hand2_trace(
			HFC_TRC_LINK_CHK, 0x20, ap, target, hfcp, 
			0, 0, 0);
		
		HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
		memcpy(logdata,(uchar*)&cmd_flags,4);
		logdata[4] = ap->xrb[xrb_no].xcc ;
		logdata[5] = ap->xrb[xrb_no].esw ;
		logdata[6] = ap->xrb[xrb_no].ssn ;
		logdata[7] = ap->xrb[xrb_no].son ;
		memcpy(&logdata[8],(uchar *)&ap->xrb[xrb_no].sbc,2);
		logdata[10] = ap->xrb[xrb_no].resp_iu.fcp_status2 ;
		logdata[11] = ap->xrb[xrb_no].resp_iu.scsi_status ;
		logdata[12] = ap->xrb[xrb_no].fsb ;
#if !(_HFC_DEBUG_HAND_03)
		memcpy(&logdata[13],ap->xrb[xrb_no].err_code,3);
#endif
		if( ap->xrb[xrb_no].fsb & HFC_FSB_PC ) {
			hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR9,0xA8,logdata,16) ;	/* FCLNX_GPL-0114 */
		}
		else {
			hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR6,0x20,logdata,16) ;
		}
		
		clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);

		if (!test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )			/* FCLNX-0500 */
		{
			if( hfc_issue_relogin(ap,target) )
			{
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);		/* FCLNX-GPL-038 */
				hfc_enque_login_req(ap, target);							/* FCLNX-GPL-038 */
				
				hfc_hand2_trace(HFC_TRC_LINK_CHK, 0x25, ap, target, hfcp, 0, 0, 0);
			}
		}																	/* FCLNX-0500 */
		
		hfc_iodone(ap, Scmd, hfcp);											/* FCLNX-0429 */
		return func_rc;
	}
	
	if (test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)
	 || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) )				/* FCLNX-0429 */
	{/* Response of ABORT TASK SET */
	
		/* Soft log output */
//		hfc_notify_tout(ap, target);										/* FCLNX-GPL-0152 */
		
		/* Dequeue hfc_pkt from wait_end_que */
		hfc_deque_we_que(ap, target, hfcp);
		
		hfc_hand2_trace(HFC_TRC_LINK_CHK, 0x21, ap, target, hfcp, 0, 0, 0);
		
		HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
		memcpy(logdata,(uchar*)&cmd_flags,4);
		logdata[4] = ap->xrb[xrb_no].xcc ;
		logdata[5] = ap->xrb[xrb_no].esw ;
		logdata[6] = ap->xrb[xrb_no].ssn ;
		logdata[7] = ap->xrb[xrb_no].son ;
		memcpy(&logdata[8],(uchar *)&ap->xrb[xrb_no].sbc,2);
		logdata[10] = ap->xrb[xrb_no].resp_iu.fcp_status2 ;
		logdata[11] = ap->xrb[xrb_no].resp_iu.scsi_status ;
		logdata[12] = ap->xrb[xrb_no].fsb ;
#if !(_HFC_DEBUG_HAND_04)
		memcpy(&logdata[13],ap->xrb[xrb_no].err_code,3);
#endif
		if( ap->xrb[xrb_no].fsb & HFC_FSB_PC ) {
			hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR9,0xA9,logdata,16) ;	/* FCLNX_GPL-0114 */
		}
		else {
			hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR6,0x21,logdata,16) ;	/* FCLNX_GPL-0114 */
		}

		dev = (struct dev_info *)hfc_search_dev_info( target, hfcp );		/* FCLNX-GPL-0343 */
		if( dev == NULL ){
			hfc_hand2_trace(
				HFC_TRC_LINK_CHK, 0x23, ap, target, hfcp, 
					0, 0, 0);
			return func_rc;
		}
		if (test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) ) {		/* FCLNX-GPL-0343 */
			dev->lustat &= ~HFC_WAIT_LUN_RESET;
		}
		else {
			dev->lustat &= ~HFC_WAIT_ABORT;
		}																	/* FCLNX-GPL-0343 */
		
		if (!test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )			/* FCLNX-GPL-0152 */
		{
			if( hfc_issue_relogin(ap,target) )
			{
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
				hfc_enque_login_req(ap, target);
				
				hfc_hand2_trace(HFC_TRC_LINK_CHK, 0x25, ap, target, hfcp, 0, 0, 0);
			}
		}																	/* FCLNX-GPL-0152 */
		
		hfc_iodone(ap, Scmd, hfcp);											/* FCLNX-0429 */
		
		return func_rc;
	}
	
	
	hfc_deque_we_que(ap, target, hfcp);
	
	hfc_hand2_trace(
		HFC_TRC_LINK_CHK, 0x22, ap, target, hfcp, 
		0, 0, 0);
	
	HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
	memcpy(logdata,(uchar*)&cmd_flags,4);
	logdata[4] = ap->xrb[xrb_no].xcc ;
	logdata[5] = ap->xrb[xrb_no].esw ;
	logdata[6] = ap->xrb[xrb_no].ssn ;
	logdata[7] = ap->xrb[xrb_no].son ;
	memcpy(&logdata[8],(uchar *)&ap->xrb[xrb_no].sbc,2);
	logdata[10] = ap->xrb[xrb_no].resp_iu.fcp_status2 ;
	logdata[11] = ap->xrb[xrb_no].resp_iu.scsi_status ;
	logdata[12] = ap->xrb[xrb_no].fsb ;
#if !(_HFC_DEBUG_HAND_05)
	memcpy(&logdata[13],ap->xrb[xrb_no].err_code,3);
#endif
	if( ap->xrb[xrb_no].fsb & HFC_FSB_PC ) {
		hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR9,0xAA,logdata,16) ;	/* FCLNX_GPL-0114 */
	}
	else {
		hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR6,0x22,logdata,16) ;
	}
	
	if (hfcp != NULL)														/* FCLNX-0429 */
	{
		if (test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags ))		/* FCLNX-0436 */
		{
			hfc_manage_info.npubp->hfc_make_fcinfo(ap, hfcp, 0x22, 
					hfcp->adap_status, logdata, ((sizeof(logdata) < 32) ? sizeof(logdata) : 32) );
		}
	}
	
	hfc_iodone(ap, Scmd, hfcp);												/* FCLNX-0429 */
	
	return func_rc ;
}


/*
 * Function:    
 *
 * Purpose:     This routine check XCRB for the regular SCSI initiation
 *
 * Arguments:   
 *  ap         - pointer to adap_indo
 *  target     - pointer to target_info
 *  hfcp       - pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_scsi_chk(
	struct adap_info		*ap,
	struct target_info		*target,
	struct hfc_pkt			*hfcp)
{
	struct scsi_cmnd		*Scmd;
	uint					resid = 0;
	uint					work_resid = 0;
	int 					xrb_no = 0 ;
	uchar					scsi_status = 0 ;
	uint					sns_offset = 0 ;
	uint					work_sns_offset = 0;
	uint					sns_length = 0 ;
	uint					work_sns_length = 0;
	ulong					data_len = 0;     /* FCLNX-GPL-261 */
	ulong					frame_num = 0;    /* FCLNX-GPL-261 */
	ulong					frame_unit = 0;   /* FCLNX-GPL-261 */
	ulong					word_num = 0;     /* FCLNX-GPL-261 */
	uint					resp_length = 0 ;		/* FCLNX-GPL-0312, 0321 */
	uint					work_resp_length = 0;	/* FCLNX-GPL-0312, 0321 */

	xrb_no = ap->xrb_no ;
	Scmd = hfcp->cmd_pkt;

	/* FCLNX-GPL-261 */
	/* New method I/O statistical information calcurated with Driver */
	data_len  = hfcp->data_size;
	frame_num = 0;
	word_num  = 0;
	if (data_len != 0) {
		if (ap->xrb[xrb_no].resp_iu.fcp_status2 & HFC_XRB_RESID_UNDER) { /* underrun */
			work_resid = ap->xrb[xrb_no].resp_iu.resid;
			HFC_4B_TO_4L(resid, work_resid);
			data_len -= resid;
		}
		/* size of a frame */
		if ( Scmd->sc_data_direction == SCSI_DATA_READ ) {	/* Read */
			frame_unit = 2048;
		} 
		else if ( Scmd->sc_data_direction == SCSI_DATA_WRITE ) {	/* Writed */
			frame_unit = (target->send_frame_size == 0) ? 2048 : target->send_frame_size;
		}
		/* From data length to the number of frames */
		frame_num = data_len / frame_unit;
		frame_num += (data_len % frame_unit) == 0 ? 0 : 1 ;
		/* the number of words */
		word_num  = data_len / 4;
	}
	if ( Scmd->sc_data_direction == SCSI_DATA_WRITE ) { /* Write */
		ap->tx_frames += (frame_num + 1);	/* FCP_DATA + FCP_CMND */
		ap->rx_frames += 2;					/* FCP_XFER_RDY + FCP_RSP */

		ap->tx_words += (word_num + 8);		/* FCP_DATA + FCP_CMND(8) */
		ap->rx_words += 9;					/* FCP_XFER_RDY(3) + FCP_RSP(6) */
	}
	else { 		/* Read *//* FCLNX-GPL-496 */
		ap->rx_frames += (frame_num + 1);	/* FCP_DATA + FCP_RSP */
		ap->tx_frames += 1;					/* FCP_CMND */

		ap->rx_words += (word_num + 6);		/* FCP_DATA + FCP_RSP */
		ap->tx_words += 8;					/* FCP_CMND */
	}
	/* New method I/O statistical information calcurated with Driver */
	/* FCLNX-GPL-261 */

	/* FCLNX-GPL-494 set statistics for Virtage */
	if ( !(HFC_MMODE_CHECK_BASIC(ap)) && (ap->hg_cca_p != NULL) ) {
		ap->hg_cca_p->tx_frame = ap->tx_frames; 
		ap->hg_cca_p->tx_word  = ap->tx_words;
		ap->hg_cca_p->rx_frame = ap->rx_frames;
		ap->hg_cca_p->rx_word  = ap->rx_words;
	}

	scsi_status = ap->xrb[xrb_no].resp_iu.scsi_status ;
	Scmd->result |= (int)scsi_status;

	if( scsi_status != HFC_SCSISTAT_GOOD )
	{	/* SCSI STATUS is abnormal */
		HFC_DBGPRT(" hfcldd : hfc_scsi_chk - scsi_status = 0x%02x\n", scsi_status);

		if( (scsi_status == HFC_SCSISTAT_CHECK_CONDITION)||
			(scsi_status == HFC_SCSISTAT_COMMAND_TERMINATED))
		{
			/* CHECK_CONDITION/CMD_TERM occurred */
			if( (ap->xrb[xrb_no].resp_iu.fcp_status2 & HFC_XRB_SNSLEN_VALID) &&
//				(sizeof(Scmd->sense_buffer) > 0) &&		/* FCLNX-GPL-396 */
				(Scmd->sense_buffer != NULL) )
				
			{	/* Auto Sense is effective */
				sns_offset = 0 ;
				if( ap->xrb[xrb_no].resp_iu.fcp_status2& HFC_XRB_RSPLEN_VALID )
				{	/* Response Length is effective */
					
					/* Get the start position of sense data */
					work_sns_offset = ap->xrb[xrb_no].resp_iu.resp_len;
					HFC_4B_TO_4L(sns_offset, work_sns_offset);
				}
				
				/* Set up the length of sense data */
				work_sns_length = ap->xrb[xrb_no].resp_iu.sns_len;
				HFC_4B_TO_4L(sns_length, work_sns_length);
				
				if (!test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags )) {	/* FCLNX-GPL-463 */
					if( SCSI_SENSE_BUFFERSIZE < sns_length ) 	/* FCLNX-GPL-396 */
						sns_length = SCSI_SENSE_BUFFERSIZE;  	/* FCLNX-GPL-396 */
				}
				else {
					if( HFC_SCSI_SENSE_BUFFERSIZE < sns_length )
						sns_length = HFC_SCSI_SENSE_BUFFERSIZE;
				}
				
				if( (sns_length+sns_offset) > sizeof(ap->xrb[xrb_no].resp_iu.resp_info) )
					sns_length = sizeof(ap->xrb[xrb_no].resp_iu.resp_info) - sns_offset ;
				
				/* Copy sense data */
				memcpy(
					Scmd->sense_buffer, 							/*	???? */
					&ap->xrb[xrb_no].resp_iu.resp_info[sns_offset],
					sns_length);
			}
			
			hfcp->adap_status = SCS_CHECK_CONDITION;
//			Scmd->result |= DID_OK << 16;
		}
//		else if (scsi_status == HFC_SCSISTAT_BUSY)					/* FCLNX-0594 */
//		{
//			Scmd->result |= DID_BUS_BUSY << 16;						/* FCLNX-0594 */
//		}
		else
		{
			hfcp->adap_status = SCS_SCSI_CHECK;						/* FCLNX-0594 */
		}
	}
	else
	{
		hfcp->adap_status = SCS_NORMAL_END; 					/*--	FCWIN-0122		--*/
	}

	Scmd->result |= DID_OK << 16;									/* FCLNX-0594 */

#if _HFC_DEBUG_HAND_06
	/**********************************************/
	/* PseudoCheck Condition injection processing */
	/**********************************************/
	if (CMND_TARGET(Scmd) == 0 && CMND_LUN(Scmd) == 1) {
		int diskerr_code = hfc_debug_ioerr_code;

		if ((diskerr_code >= 100) && ((uchar)Scmd->cmnd[0] == 0x2A)) {
			static int diskerr_count = 0;
			int diskerr_issue        = 10000;
			int diskerr_max          = 100;
			
			if (diskerr_count == 0)
				printk(KERN_ERR "hfcldd : Wait I/O Error (diskerr_code=%d, err_max=%d).\n",diskerr_code, diskerr_max);
			
			if ((diskerr_count % 1000) == 0)
				printk(KERN_ERR "diskerr_count = %d /%d.\n",diskerr_count,diskerr_issue);
			
			if (diskerr_count < diskerr_issue)
			{
				diskerr_count++;
			}
			else if (diskerr_count > (diskerr_issue + diskerr_max)) {
				diskerr_count = 0;
			}
			else {
				if (Scmd->result & 0x00ff00ff) {
					printk(KERN_ERR "Unexpected I/O Error. (cmnd->result=0x%08x).\n", Scmd->result);
				}
				else {
					Scmd->result &= 0xFF00FF00;
					Scmd->result |= DID_OK << 16;
					Scmd->result |= (CHECK_CONDITION << 1);
					Scmd->sense_buffer[0] = 0xf0;			/* Error Code      */

					switch (diskerr_code) {
						case 100 : Scmd->sense_buffer[2] = 0x00;	break;	/* NO_SENSE        */
						case 101 : Scmd->sense_buffer[2] = 0x01;	break;	/* RECOVERED_ERROR */
						case 102 : Scmd->sense_buffer[2] = 0x02;	break;	/* NOT_READY       */
						case 103 : Scmd->sense_buffer[2] = 0x03;	break;	/* MEDIUM_ERROR    */
						case 104 : Scmd->sense_buffer[2] = 0x04;	break;	/* HARDWARE_ERROR  */
						case 105 : Scmd->sense_buffer[2] = 0x05;	break;	/* ILLEGAL_REQUEST */
						case 106 : Scmd->sense_buffer[2] = 0x06;	break;	/* UNIT_ATTENTION  */
						case 107 : Scmd->sense_buffer[2] = 0x07;	break;	/* DATA_PROTECT    */
						case 108 : Scmd->sense_buffer[2] = 0x08;	break;	/* BLANK_CHECK     */
						case 110 : Scmd->sense_buffer[2] = 0x0a;	break;	/* COPY_ABORTED    */
						case 111 : Scmd->sense_buffer[2] = 0x0b;	break;	/* ABORTED_COMMAND */
						case 113 : Scmd->sense_buffer[2] = 0x0d;	break;	/* VOLUME_OVERFLOW */
						case 114 : Scmd->sense_buffer[2] = 0x0e;	break;	/* MISCOMPARE      */
						default  : printk(KERN_ERR "Illigal _HFC_DEBUG_HAND_06( value=%d? ).\n",diskerr_code);
					}

				}
				
				diskerr_count++;
			}
		}
	}
#endif

	if( ap->xrb[xrb_no].resp_iu.fcp_status2 & HFC_XRB_RSPLEN_VALID) 	/* FCLNX-GPL-0312, 0321 */
	{
		/*-- Resp Length Valid --*/
		work_resp_length = ap->xrb[xrb_no].resp_iu.resp_len;
		HFC_4B_TO_4L(resp_length, work_resp_length);
		
		if (resp_length>3 && ap->xrb[xrb_no].resp_iu.resp_info[3] )
		{
			/* RSP_CODE byte3 != 0 */
			HFC_DBGPRT(" hfcldd : hfc_scsi_chk - RSP_CODE byte3 != 0 -> DID_ERROR.\n");
			
			Scmd->result &= 0xff00ffff;
			Scmd->result |= DID_ERROR << 16;
			hfcp->adap_status = SCS_RSPCODE_FAILURE;
		}
	}																	/* FCLNX-GPL-0312, 0321 */

	if ( ap->xrb[xrb_no].resp_iu.fcp_status2 & HFC_XRB_RESID_OVER )
	{	/* Data overruns */
		HFC_DBGPRT(" hfcldd : hfc_scsi_chk - data overrun.\n");

		Scmd->result &= 0xff00ffff;
		Scmd->result |= DID_ERROR << 16;
		hfcp->adap_status = SCS_DATA_OVERRUN;					/*--	FCWIN-0122		--*/
	}
	else {															/* FCLNX-0257 */
		resid = 0;
		
#if 0		/* FCLNX-GPL-0449 */
		if ( ((uchar)Scmd->cmnd[0] == 0xA0)
		  && (hfc_manage_info.hfcldd_mp_mod)
		  && (hfc_manage_info.lg_target_info)
		  && (hfcp->dev != NULL) ) {
			uint xrf_len;

			xrf_len = hfc_manage_info.npubp->hfc_convert_rptluns(hfcp, Scmd, NULL);/* FCLNX-0611 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
			resid = Scmd->sdb.length - xrf_len;
#else
			resid = Scmd->request_bufflen - xrf_len;
#endif
		}
		else {
			if ( (hfc_manage_info.hfcldd_mp_mod)						/* FCLNX-0611 */
			  && (hfc_manage_info.lg_target_info) 						/* FCLNX-0611 */
			  && ((uchar)Scmd->cmnd[0] == 0xA0) 						/* FCLNX-0611 */
			  && (hfcp->dev == NULL)									/* FCLNX-0611 */
			  && (Scmd->scsi_done == (void *) hfc_ioctl_iodone ) ) {	/* FCLNX-0611 */
				hfc_manage_info.npubp->hfc_check_luconfig(hfcp, Scmd);	/* FCLNX-0611 */
			}
#endif		/* FCLNX-GPL-0449 */

			if (ap->xrb[xrb_no].resp_iu.fcp_status2 & HFC_XRB_RESID_UNDER )
			{	/* Data underruns */
				HFC_DBGPRT(" hfcldd : hfc_scsi_chk - data underrun.\n");
				
				work_resid = ap->xrb[xrb_no].resp_iu.resid;
				HFC_4B_TO_4L(resid, work_resid);
			}
//		}

		if (resid) {												/* FCLNX-0257 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
			Scmd->sdb.resid = resid;
#else
			Scmd->resid = resid;
#endif
			hfcp->adap_status = SCS_DATA_UNDERRUN;					/*--	FCWIN-0122		--*/
			
			if (!scsi_status) {										/* FCLNX-0429 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
				if ((unsigned)(Scmd->sdb.length - resid) < Scmd->underflow)
#else
				if ((unsigned)(Scmd->request_bufflen - resid) < Scmd->underflow)
#endif
				{
					HFC_DBGPRT(" hfcldd : hfc_scsi_chk - data underrun -> DID_ERROR.\n");
					
					Scmd->result &= 0xff00ffff;
					Scmd->result |= DID_ERROR << 16;
				}
			}
		}
	}
	
	/*-- Dequeue hfc_pkt from wait_end_que --*/
	hfc_deque_we_que(ap, target, hfcp);

	hfc_hand2_trace(
			HFC_TRC_SCSI_CHK, 0x10, ap, target, hfcp, 
			0, 0, 0);
	
	hfc_iodone(ap, Scmd, hfcp);										/* FCLNX-0429 */
	
	return ;
}


/*
 * Function:    hfc_task_mgm_chk
 *
 * Purpose:     This routine check XCRB for the ABORT/RESET
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  hfcp       - pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_task_mgm_chk(
	struct adap_info		*ap,
	struct hfc_pkt			*hfcp)
{
	struct target_info		*target;
	uchar					resp_code = 0;
	uchar					rc_error = 0;
	struct scsi_cmnd		*Scmd;
	struct dev_info			*dev;
	
	int 					xrb_no = 0, rtn=0;

	xrb_no = ap->xrb_no;
	Scmd = hfcp->cmd_pkt;
	dev = hfcp->dev;
	target = (struct target_info *)hfc_hash_target_info(ap,
							 ap->xrb[xrb_no].drv_work.target_id);		/* FCWIN-0153 */

	if( (ap->xrb[xrb_no].resp_iu.fcp_status2 & HFC_XRB_RSPLEN_VALID) && 
		!(ap->xrb[xrb_no].resp_iu.fcp_status2 & ~HFC_XRB_RSPLEN_VALID) )
	{
		resp_code = ap->xrb[xrb_no].resp_iu.resp_info[3] ;
		if( resp_code != HFC_RSP_SUCCESSFUL )
		{
			switch (resp_code)
			{
			case HFC_RSP_TSKMGM_UNSUPPORED:
				rc_error = HFC_MB_FATAL;
				break;
			case HFC_RSP_TSKMGM_FAILED:
				rc_error = HFC_MB_FATAL;
				break ;
			default:
				rc_error = HFC_MB_FATAL;
			}
		}
	}
	

	if( test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags) ||
		test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
	{/* Response of target reset */

		/* SoftLog Errlog output */
		hfc_notify_tout(ap, target);

		/* Dequeue hfc_pkt from wait_end_que	--*/
		hfc_deque_we_que(ap, target, hfcp);

		if (rc_error == 0)
		{	/* Target Reset succeeded */
			/* Cancellation of XOB and response waiting queue */
			hfc_cancel_scsi_cmd(
				ap, target, 0, NULL, SCS_CMD_RESET, HFC_CSCSI_ERROR,		/* FCLNX-0429 */
				TRUE, TRUE,HFC_FLASH_TARGET);
				
			hfcp->adap_status = SCS_NORMAL_END;
			
			if (!test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
				Scmd->result |= DID_OK << 16;
			
			clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);		/* FCLNX-0500 */
		}
		else
		{
			hfcp->adap_status = SCS_RESET_FAILED;

			if (!test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
				Scmd->result |= DID_ERROR << 16;
				
			clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-0500 */
			
			if (!test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )
			{
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);			/* FCLNX-GPL-038	*/
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);		/* FCLNX-GPL-038	*/
				if( hfc_issue_relogin(ap,target) )
				{
					set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
					clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038	*/
					hfc_enque_login_req(ap, target);
					
					hfc_hand2_trace(HFC_TRC_MGM_CHK, 0x25, ap, target, hfcp, 0, 0, 0);
				}
			}																/* FCLNX-0500 */
		}
		
		/* Start SCSI Delay Timer  */										/* FCLNX-GPL-038	*/
		if( ap->scsi_reset_delay ){
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_DELAY_TMR, 0, TRUE);
			rtn = hfc_watchdog_enter(ap, target, NULL, 0, HFC_DELAY_TMR, 0, FALSE);
			if( !rtn ) set_bit(HFC_SCSI_DELAY, (ulong *)&target->status);
		}																	/* FCLNX-GPL-038	*/
		
		hfc_iodone(ap, Scmd, hfcp);											/* FCLNX-0429 */

		hfc_hand2_trace( HFC_TRC_MGM_CHK, 0x20, ap, target, hfcp, 0, 0, 0 );
		return;
	}

	if ( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) )			/* FCLNX-0429 */
	{	/* Response of Abort Task Set or LUN Reset --*/
		
		/* SoftLo Errlog output */
		hfc_notify_tout(ap, target);

		/* Dequeue hfc_pkt from wait_end_que */
		hfc_deque_we_que(ap, target, hfcp);

		if( rc_error == 0 )
		{
			/* Cancel XOB and response wait queue */
			hfc_cancel_scsi_cmd(
				ap, target, hfcp->lun_id, NULL, SCS_CMD_ABORTED,		/* FCLNX-0553 */
				HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_DEV);			/* FCLNX-0553 */
			
			hfcp->adap_status = SCS_NORMAL_END;
			Scmd->result |= DID_OK << 16;
		}
		else
		{
			hfcp->adap_status = SCS_ABORT_FAILED;
			Scmd->result |= DID_ABORT << 16;
//			if((hfc_manage_info.hfcplus_enable)&&(ap->rt_err_enable)){						/* FCWIN-0506 */
			if (!test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )		/* FCLNX-GPL-359 */
			{
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
				if( hfc_issue_relogin(ap,target) )
				{
					set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
					clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
					hfc_enque_login_req(ap, target);
					
					hfc_hand2_trace(HFC_TRC_MGM_CHK, 0x26, ap, target, hfcp, 0, 0, 0);
				}
			}																/* FCLNX-GPL-359 */
			if(ap->rt_err_enable){	/* FCLNX-GPL-349 */
				if(hfc_manage_info.hfcldd_mp_mod){
					if(hfc_check_cmnd_timeout(ap,target)){			/* FCLNX-GPL-430 */
						hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);  /* FCLNX-0506 */
					}
				}
				else{
					hfc_watched_errcount_i(ap, NULL, HFC_RT_ERR);  /* FCLNX-GPL-349 */
				}
			}
		}

//		dev = hfc_search_dev_info( target, hfcp );							/* FCLNX-GPL-0343 */
		/* Start LUN Reset Delay Timer  */									/* FCLNX-GPL-038	*/
		if( test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) ){
//			dev = hfc_manage_info.npubp->hfc_search_dev_info( target, hfcp );		/* FCLNX-GPL-047	*//* FCLNX-GPL-0343 */
			if( dev != NULL ){
				if( ap->lun_reset_delay ){
//					hfc_manage_info.npubp->hfc_mp_watchdog_enter(ap, target, hfcp, dev, hfcp->lun_id, HFC_DELAY_TMR_DEV, 0, TRUE);
					hfc_mp_watchdog_enter(ap, target, hfcp, dev, hfcp->lun_id, HFC_DELAY_TMR_DEV, 0, TRUE);				/* FCLNX-GPL-0343 */
//					rtn = hfc_manage_info.npubp->hfc_mp_watchdog_enter(ap, target, hfcp, dev, hfcp->lun_id, HFC_DELAY_TMR_DEV, 0, FALSE);
					rtn = hfc_mp_watchdog_enter(ap, target, hfcp, dev, hfcp->lun_id, HFC_DELAY_TMR_DEV, 0, FALSE);		/* FCLNX-GPL-0343 */
//					if( !rtn ) hfc_manage_info.npubp->hfc_set_dev_info(dev);											/* FCLNX-GPL-0343 */
					if( !rtn ) hfc_set_dev_info(dev);																	/* FCLNX-GPL-0343 */
				}
			}																/* FCLNX-GPL-047	*/
		}																	/* FCLNX-GPL-038	*/
		
//		dev = (struct dev_info *)hfc_search_dev_info( target, hfcp );			/* FCLNX-GPL-0343 */
		if( dev == NULL ){
			hfc_hand2_trace( HFC_TRC_MGM_CHK, 0x32, ap, target, hfcp, 0, 0, 0 );
			return;
		}
		if (test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) ) {		/* FCLNX-GPL-0343 */
			dev->lustat &= ~HFC_WAIT_LUN_RESET;
		}
		else {
			dev->lustat &= ~HFC_WAIT_ABORT;
		}																	/* FCLNX-GPL-0343 */
		hfc_hand2_trace( HFC_TRC_MGM_CHK, 0x31, ap, target, hfcp, 0, 0, 0 );
		hfc_iodone(ap, Scmd, hfcp);										/* FCLNX-0429 */
		
		return;
	}																	/* FCWIN-0153 END */
	
	hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_EVNT3,0x80,logdata,16) ;
	
	return;
}	


/*
 * Function:    hfc_notify_tout
 *
 * Purpose:     This routine get a soft log data and organize it.
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  target     - pointer to target_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_notify_tout(										  /* FCWIN-0153 */
	struct adap_info			*ap,
	struct target_info			*target)
{
	struct hfc_pkt				*hfcp;
	uint						hash;

	/* This subroutine does not need to dequeue and iodone time-out SCSI process */
	/*  (These termination process owes to higher level reset) */
	
	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		if (target->we_que_top[hash] != NULL)
		{	/* hfcp exists in queue */
			hfcp = target->we_que_top[hash];
			
			while( hfcp != NULL )
			{
				if ( test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags) )
				{	/* If pkt has MIHLOG */
					memset( logdata, 0, 16 );
					memcpy( logdata, (uchar*)&ap->xob[ap->xob_no].fcp_cmd.fcp_cntl, 4 );
					if( test_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags) )
					{
						hfc_errlog( ap, target, hfcp, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, 0x24, logdata, 16 );
						clear_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags);
					}
					else {
						hfc_errlog( ap, target, hfcp, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRA, 0x24, logdata, 16 );
					}
					
					if (test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags ))	/* FCLNX-0429 */
					{
						hfc_manage_info.npubp->hfc_make_fcinfo(ap, hfcp, 0x24, 
								hfcp->adap_status, logdata, sizeof(logdata));
					}
				}
				hfcp = hfcp->cmd_forw ;
			}
		}
	}

}


/*
 * Function:    hfc_deque_we_que
 *
 * Purpose:     This routine deque a hfc_pkt entry from the wait_end_queue
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  target     - pointer to target_info
 *  hfcp       - pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_deque_we_que(
	struct adap_info            *ap,
	struct target_info          *target,
	struct hfc_pkt              *hfcp)
{
	int                         tblno;
	struct hfc_pkt              *hfcp_wk;

	tblno = hfcp->lun_id % HASH_T_NUM;
	
	if(target->we_que_top[tblno] != NULL)
	{	/* One or more targets exist in we_que */
		if( target->we_que_top[tblno] == target->we_que_end[tblno] )
		{	/* Depth of this queue is only one */
			target->we_que_top[tblno] = NULL ;
			target->we_que_end[tblno] = NULL ;
		}
		else
		{	/* Depth of this queue is two or more */
			if( hfcp == target->we_que_top[tblno])
			{	/* Dequeue from the top of the queue */
				target->we_que_top[tblno] = hfcp->cmd_forw;
				hfcp->cmd_forw->cmd_prev  = NULL;
			}
			else if(hfcp == target->we_que_end[tblno] )
			{	/* Dequeue from the end of the cue  --*/
				target->we_que_end[tblno] = hfcp->cmd_prev;
				hfcp->cmd_prev->cmd_forw = NULL;
			}
			else
			{	/* Dequeue from the middle of the queue */
				hfcp_wk = hfcp->cmd_forw;
				hfcp_wk->cmd_prev = hfcp->cmd_prev;
				hfcp_wk = hfcp->cmd_prev;
				hfcp_wk->cmd_forw = hfcp->cmd_forw;
			}
		}

		target -> we_que_cnt--;							/* FCWIN-0086 */

		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info && hfcp->dev)
			hfc_manage_info.npubp->hfc_queue_count(hfcp, 1, 1);	/* we dequeue */
	}

																/* FCWIN-0153 */

	return ;
}


/*
 * Function:    hfc_mb_errlog
 *
 * Purpose:     Get error log of mail box
 *
 * Arguments:   
 *  ap            - Pointer to adap_info
 *  target        - Pointer to target_info
 *  trc_id        - ID of caller
 *  rc_passthoruh - Hfc_mb_pass_throuh return value
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mb_errlog(
	struct adap_info        *ap,
	struct target_info      *target,
	uchar                   trc_id,
	uint                    rc_passthrouh)
{
	uint                    err_num=0;
	uint                    err_id;
	uchar                   type=0;
	struct mailbox          *mbox = ap->mb;
	int						disable_log = 0; /* FCWIN-GPL-503 */
	
	switch(trc_id)
	{
	case HFC_TRC_LGINRSP :          /* LOGIN response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x0c;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x0d;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x0e;     type = HFC_ERRLOG_TYPE_MBRESP;  
			if (ap->limit_log == HFC_ENABLE_LIMITLOG) { /* FCLNX-GPL-503 */
				hfc_hand2_trace( HFC_TRC_LGINRSP, 0x30, ap, target, NULL, (uint64_t)rc_passthrouh, 0, 0 );
				disable_log = 1;
			}
			break;
		}
		break;
	case HFC_TRC_PDISCRSP :         /* PDISC response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x10;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x11;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x12;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_TRC_GIDFTRSP :         /* GIDFT response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x7b;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x7c;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x7d;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_TRC_GIDPNRSP :         /* GIDPN response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x81;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x82;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x83;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_TRC_GPNIDRSP :         /* GPNID response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x84;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x85;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x86;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_TRC_MIHLGRSP :         /* MIHLOG response processing    */
		                                err_num = 0x7e;     type = HFC_ERRLOG_TYPE_MBRESP;
		break;

	case HFC_TRC_LINKRSP : /* Link Initialize Response */
		switch(rc_passthrouh)
		{
			case HFC_MBPASS_RETRY_OVER : err_num = 0x88; type = HFC_ERRLOG_TYPE_MBRESP; break;
			case HFC_MBPASS_RETRY_FAIL : err_num = 0x89; type = HFC_ERRLOG_TYPE_NONE;   break;
			case HFC_MBPASS_ERROR      : err_num = 0x8A; type = HFC_ERRLOG_TYPE_MBRESP; break;
		}
		break;			
	}
	
	if( (rc_passthrouh == HFC_MBPASS_ERROR) && (mbox->mb_resp.fsb & HFC_FSB_PC) ) {
		err_id = ERRID_HFCP_ERR9;
		err_num = 0xAB;				/* FCLNX_GPL-0114 */
	}
	else {
		err_id = ERRID_HFCP_ERR6;
	}
	
	memset((void *)logdata, 0, 16);
	logdata[0] = mbox->mb_resp.type.respcmd.cmd.command ;
	logdata[1] = mbox->mb_resp.type.respcmd.cmd.sub_cmd ;
	logdata[4] = mbox->mb_resp.xcc ;
	logdata[5] = mbox->mb_resp.esw ;
	logdata[6] = mbox->mb_resp.ssn ;
	logdata[7] = mbox->mb_resp.son ;
	memcpy(&logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&logdata[13],(char *)&mbox->mb_resp.err_code,3);
	
	if(( err_num != 0)&&(!disable_log)) { /* FCLNX-GPL-503 */
		hfc_errlog(ap, target, NULL, type, err_id, err_num, logdata, 16);
	}
	
	return;
}


/*
 * Function:    hfc_issue_intl_start
 *
 * Purpose:     Execute hfc_start() 
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  target     - pointer to target_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_issue_intl_start(struct adap_info *ap,struct target_info *target)
{
	uchar	start_flag = FALSE ;
	
	struct hfc_pkt *hfcp=NULL;
	
	if( ap->xob_wait_exec_cnt > 0 )
	{
		start_flag = TRUE ;
	}
	else if( target->wx_que_cnt > 0 )
	{
		if(  test_bit(HFC_ONLINE, (ulong *)&ap->status) &&
			(target->status == HFC_NON_STATUS) && !(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status)) )
		{
				/* The status of adaptor, target and device can execute SCSI command */
				start_flag = TRUE ;
		}
	}
	if( start_flag == TRUE)
	{
		hfcp = target->wx_que_top;

		if( (test_bit(HFC_ONLINE, (ulong *)&ap->status)) 
				&& !( test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status ))
	  			&& !( test_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status ))
	  			&& !( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ))
	  			&& !( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ))
	  			&& !( test_bit( HFC_ISOL, (ulong *)&ap->status ))	/* FCLNX-GPL-572 */
	  			&& ( !HFC_MMODE_CHECK_SHARED(ap) || !test_bit( HFC_WAIT_T3, (ulong *)&ap->status ))	/* FCLNX-GPL-0320 */
	  			&&  (target->status == HFC_NON_STATUS) )
		{
			/* The status of adaptor, target and device can execute SCSI command */
			hfc_start(ap,target,hfcp) ;
			
			if (target->next_dstart_flag) {										/* FCLNX-GPL-480 */
				/* Enqueue this target to the end of the queue */
				if( (ap->next_dstart_cnt > 1) && (ap->next_dstart_end != target) )
				{
					hfc_deque_next_dstart(ap,target);
					hfc_enque_next_dstart(ap,target);
				}
			}
			return;																/* FCLNX-GPL-480 */
		}
	}

//	hfc_watchdog_enter(ap, target, NULL, 0,HFC_WEXEC_TMR, 0, TRUE);				/* FCLNX-GPL-453 */
	if( ( (target->wx_que_cnt > 0)||(ap->xob_wait_exec_cnt > 0)) &&
		!test_bit(HFC_SCSI_DELAY, (ulong *)&target->status) )
	{
		/* Enqueue this target to the end of the queue */
		if( (ap->next_dstart_cnt > 1) && (ap->next_dstart_end != target) )
		{
			hfc_deque_next_dstart(ap,target);
//			hfc_enque_next_dstart(ap,target);									/* FCLNX-GPL-453 */
		}
		
		hfc_enque_next_dstart(ap,target);										/* FCLNX-GPL-453 */
//		hfc_watchdog_enter(ap, target, NULL, 0,									/* FCLNX-GPL-453 */
//							 HFC_WEXEC_TMR, 0, FALSE);							/* FCLNX-GPL-453 */
	}
	else if( target->wx_que_cnt == 0 )
	{
		hfc_deque_next_dstart(ap,target);
	}
}

/*
 * Function:    hfc_frame_a_data
 *
 * Purpose:     Create "Frame_A Data"
 *
 * Arguments:   
 *  mbox       - pointer to mailbox
 *  type       - HBA Package Type
 *  resp_code  - RspCode for Frame_A
 *
 * Returns:     Frame_A Data
 *
 * Notes:       
 */
uint hfc_frame_a_data(struct mailbox *mbox, uchar type, uchar resp_code){
	/* Values */
	uint frame_a = 0x00000000;
	
	/* Check HBA Package Type */
	switch(type){
		case HFC_PKTYPE_FPP:
		case HFC_PKTYPE_FIVE:
			frame_a |= (uint)((mbox->mb_intreq.type.fwintreq0.int_code << 16) & 0x00ff0000);
			frame_a |= (uint)((mbox->mb_intreq.type.fwintreq0.sub_int_code << 8) & 0x0000ff00);
			frame_a |= (uint)HFC_FRAMEA_INT_RSP;
			frame_a |= (uint)resp_code;
			break;
		case HFC_PKTYPE_FIVE_EX:
			frame_a |= (uint)((mbox->mb_intreq.type.fwintreq0.int_code << 8) & 0x0000ff00);
			frame_a |= (uint)((mbox->mb_intreq.type.fwintreq0.sub_int_code) & 0x000000ff);
			frame_a |= (uint)HFC_FRAMEA_INT_RSP;
			break;
		default:
			break;
	}
	
	return frame_a;
}

/*
 * Function:    hfc_core_ce_event
 *
 * Purpose:     FPP,FIVE: Execute core 1bit err(CE) logging process
 *              FIVE-EX:  Logging and Counter management
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_core_ce_event(struct adap_info *ap)
{
	/* Constants */
	const struct mailbox *mbox = ap->mb;
	/* Values */
	hfc_err1bit_t *ptr = NULL; /* FCLNX-GPL-116 */
	uint far;
	uint64_t time = 0;

	/* Set Log Data */ /* FCLNX-GPL-116 */
	memset(logdata, 0, 16 );
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16); /* FCLNX-GPL-163 */

	/* Check HBA Package Type */
	switch(ap->pkg.type){
		case HFC_PKTYPE_FPP:
		case HFC_PKTYPE_FIVE:
			/* Get Log */
			hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINT,
				ERRID_HFCP_EVNT4, 0xa0, logdata, 16 );
				
			break;
		case HFC_PKTYPE_FIVE_EX:

			/* FCLNX-GPL-116 */
			if (ap->max_core_ce_cnt != 0) { /* Check the parameter */
				if (ap->core_ce_cnt < HFC_1BIT_LOG_ENTRY) {
					/*** Get Log Data ***/
					ptr = (hfc_err1bit_t *) &ap->ce_log.core_ce_data[ap->core_ce_cnt][0];
					/* Get now time */
					time = get_jiffies_64(); /* get jiffies_64 */
					HFC_8L_TO_8B(ptr->time_stamp, time);
					/* Get Addr(Defect has happend in this addr.) */
					memcpy(&far, (uint *)&(mbox->mb_intreq.type.fwintreq0.un.ce), 4);
					ptr->far = far; /* FCLNX-GPL-127 */
					/* Count up */
					ap->core_ce_cnt++;
					/* set "All" srem ce log */
					hfc_set_sram_ce_log(ap); 
					
					if ( ap->core_ce_cnt == ap->max_core_ce_cnt ) {
						/* Count over */ /* log out only 1 time */
						hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_SRAMCE,
							ERRID_HFCP_ERR2, 0xa4, logdata, 16 );
					}
				}
			}

			break;
		default:
			break;
	}
	return;
}

/*
 * Function:    hfc_identify_int_factor
 *
 * Purpose:     Get "INT_A reg" 
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *  vector     - vector#
 *  type       - INT type ( INTx, MSI, MSI-X )
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_identify_int_factor(struct adap_info *ap, int vector, int type)
{
	/* Values */
	uint int_a_reg = 0x00000000;
	
	switch(type){
		/******* INTx, MSI ******************************/
		case HFC_INT_TYPE_INTX:
		case HFC_INT_TYPE_MSI:
			/* Read INT_A register */
			int_a_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
			break;

		/*******  MSI-X *********************************/
		case HFC_INT_TYPE_MSIX:
			/* Check XRB */
			if(ap->entries[HFC_MSIX_XRB].vector == vector){
				int_a_reg = HFC_XRB_RSP;
				break;
			}
			
			/* Read INT_A register */
			int_a_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
			break;

		/******* Unkonwn type **************************/
		default:
			/* Nop */
			break;
	}
	
	return int_a_reg;
}


/* FCLNX-GPL-112 */
/* Function:    hfc_online_update_complete
 *
 * Purpose:     Process F/W online update completion interruption
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:	None (void)
 *
 * Notes:
 */
void hfc_fw_online_update_complete(struct adap_info *ap)
{
	struct mailbox	*mbox = NULL ;
	uchar                   sys_buf[4];	/* FCLNX-GPL-FX-456 */
	
	mbox = ap->mb;

	memset(logdata, 0, 16 );
	memcpy(&logdata[0], &(ap->mb->mb_intreq.type.resvx[16]), 16); /* FCLNX-GPL-163 */

	hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
		ERRID_HFCP_EVNT4, 0xA7, logdata, 16 );
	
	if(HFC_MMODE_CHECK_SHADOW(ap)){	/* FCLNX-GPL-FX-456 */
		if( !hfc_read_flash(ap, 0, 4, sys_buf) ){
			hfc_mlpf_set_mmio_hg(ap, sys_buf, HFC_IOHGSPC_SYSREV0, HFC_IOHGSPC_SYSREV_LEN);
		}
	}	/* FCLNX-GPL-FX-456 */

}


