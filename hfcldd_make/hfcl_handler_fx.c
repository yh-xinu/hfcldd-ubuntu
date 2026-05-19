/*
 * hfcl_handler_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char intr_fx_rcsid[] = "$Id: hfcl_handler_fx.c,v 1.1.2.83.2.17.2.2.2.19.2.1 2016/02/19 03:05:29 mhayashi Exp $";

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
#include "hfcl_npiv_fx.h"

//static uchar   logdata[16];

#if _HFC_DEBUG_HAND_06
extern int hfc_fx_debug_ioerr_code;
#endif

/*
 * Function:	hfc_fx_intr_xrb
 *
 * Purpose:		Interrupt handler for Mailbox Response.
 *				This routine examines each factor of INT_VECTOR register
 *				on PCI memory space and handles xrb response interrupts.
 *
 * Arguments:
 *  irq        - IRQ number
 *  intr_entry - Pointer to the instance of hfc_intr_entry structure
 *				 which corresponds to the interruption.
 *
 * Returns:    - IRQ_HANDLED
 *
 * Notes:
 */

irqreturn_t
hfc_fx_intr_xrb(int irq, void *intr_entry)
{
	struct port_info		*pp;
	struct port_info		*vpp;
	struct region_info		*rp;
	struct core_info		*core;
	
	int						i;
	int						rtn = 0;
	uchar					core_no;
	uchar					portlock;
	ulong					flags = 0;
	uint64_t				time = 0;
	uint					cpuno = 0;
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} mpint;

	struct hfc_pkt_fx		*wait_iodone_hfcp=NULL;

	/* Check if *intr_entry is not NULL */
	if (!intr_entry)
	{
		return IRQ_NONE;
	}

	/* Check if *pp is not NULL */
	if ( !( ((struct hfc_intr_entry *)intr_entry)->pp ) )
	{
		return IRQ_NONE;
	}
	/* Initialize pp */
	pp = (struct port_info *)((struct hfc_intr_entry *)intr_entry)->pp;
	
	/* get timestamp and processor id */
	if (pp->pm_control == HFC_FX_PM_ON) {
		cpuno = smp_processor_id();
		rdtscll(time);
	}
	
	/* Check if core <= 4 */
	if ( ((struct hfc_intr_entry *)intr_entry)->core > 4 )
	{
		return IRQ_NONE;
	}
	/* Initialize core */
	core_no = ((struct hfc_intr_entry *)intr_entry)->core;
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if (HFC_FX_VPORT_EXIST(pp)) {
		/* vport exists in physical port */
		mpint.l = (uint)hfc_fx_read_reg_core(pp, core_no, HFC_IOSPACE_MPINTAC,
											 0x04, HFC_FX_CORE_OFFSET10);
	}
	else {
		mpint.l = 0;
	}
	if ( mpint.c[0] == 0x80) {
		/* physical port */
		mpint.c[0] = 0x00;
	}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
		mpint.c[0] = pp->rid;
	}
	
	rp   = pp->region_arg[mpint.c[0]];
	if ( !rp )
	{
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);	/* FCLNX-GPL-FX-437 */
		return IRQ_NONE;
	}
	
	core = rp->core_arg[core_no];
	if ( !core )
	{
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);	/* FCLNX-GPL-FX-437 */
		return IRQ_NONE;
	}
	
	HFC_CORELOCK(core);
	
	if ((!core->tskmgm_cmd_num) && (!core->preserve_cmd_num)){	/* FCLNX-FX-031 */
		/* task management command none in core */
		HFC_PORTUNLOCK(pp);
		portlock = 0;
	}
	else {
		/* task management command exists in core and master core */
		HFC_COREUNLOCK(core);
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
			HFC_PORTUNLOCK(pp);
			HFC_SYSLOCK(pp);
			HFC_PORTLOCK(pp);
		}
		HFC_ALLCORELOCK(rp);
		portlock = 1;
		
		if((!core->tskmgm_cmd_num) && (!core->preserve_cmd_num)){
			HFC_ALLCOREUNLOCK(rp);
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				HFC_SYSUNLOCK(pp);
			}
			HFC_CORELOCK(core);
			HFC_PORTUNLOCK(pp);
			portlock = 0;
		}
		hfc_fx_hand2_trace(
			HFC_FX_TRC_HANDLER, 0x20, pp, rp, core, NULL, NULL,
			0, 0, 0);
	}

	hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, ((struct hfc_intr_entry *)intr_entry)->shr_info );
	
	if ( HFC_FX_MMODE_CHECK_SHARED (pp) ){
		vpp = pp->vport_ptr[0].vport_arg;
	}else{
		vpp = pp->vport_ptr[mpint.c[0]].vport_arg;
	}
	if (vpp == NULL) {
		HFC_DBGPRT("hfcldd%d : hfc_fx_intr_xrb - vpp = NULL rid=%d",pp->dev_minor,mpint.c[0]);
	}
	else if (HFC_PP_FX_STATUS_DETAIL2_TEST(HFC_PD_WAIT_CLOSE, vpp)) {
		HFC_DBGPRT("hfcldd%d : hfc_fx_intr_xrb - HFC_PD_WAIT_CLOSE rid=%d",pp->dev_minor,mpint.c[0]);
	}
	else {
		rtn = hfc_fx_xrb_resp(vpp, rp, core, time, cpuno, &wait_iodone_hfcp);
		/* rtn	byte0:	0x00		*/
		/*		byte1:	icc_count	*/
		/*		byte2:	abend_code	*/
		/*		byte3:	resp_code	*/
	}
	
	if (!rtn) {
		/* all io commands normal end */
		if (core->next_dstart_cnt) {
			hfc_fx_issue_intl_start(core->next_dstart_top->pp, rp, core, core->next_dstart_top);
		}
	}
	else {
		/* io command or task management command abnormal end */
		if (!portlock) {
			HFC_COREUNLOCK(core);
			if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
				HFC_SYSLOCK(pp);
			}
			HFC_PORTLOCK(pp);
			HFC_ALLCORELOCK(rp);
			portlock = 1;
		}
		
		if (rtn & (HFC_XRBRESP_IOCMD_LINK_ERR | HFC_XRBRESP_TMCMD_LINK_ERR)) { /* FCLNX-GPL-FX-280, 287 */
			if (core->icc_err != NULL) {
				if (core->icc_err->err_no != 0) {
					hfc_fx_errlog(
						core->icc_err->icc_pp,
						core,
						core->icc_err->icc_pp->target_arg[core->icc_err->pseq],
						&core->icc_err->hfcp,
						HFC_ERRLOG_TYPE_XRB,
						core->icc_err->err_id,
						core->icc_err->err_no,
						core->icc_err->logdata,
						16);
					
					if (core->icc_err->first_icc != 1) {
						/* not icc after linkdown */
						if (pp->if_err_limit) {
							/* rtn byte1 is icc count */
							for (i=0; i<((rtn >> 16) & 0x000000ff); i++) {
								if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-324 Start */
									hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_IF_ERR);
								}else{
									hfc_fx_watched_errcount_i(pp, NULL, HFC_IF_ERR);
								}	/* FCLNX-GPL-FX-324 End */
							}
						}
						if (HFC_FX_VPORT_EXIST(pp)) {	/* FCLNX-GPL-FX-297, 300 */
							if (pp->c_err) {
								if (pp->region_arg[0] == NULL) {
									goto intr_xrb_exit;
								}
								HFC_ALLCOREUNLOCK(rp);
								
								/* all core_lock for rid0 */
								HFC_ALLCORELOCK(pp->region_arg[0]);
								if (hfc_fx_check_errcount(pp)) {
									/* isolate done */
									rp = pp->region_arg[0];
									goto intr_xrb_exit;
								}
								HFC_ALLCOREUNLOCK(pp->region_arg[0]);
								
								HFC_ALLCORELOCK(rp);
							}
						}
						else {
							if (hfc_fx_check_errcount(pp)) {
								/* isolate done */
								goto intr_xrb_exit;
							}
						}	/* FCLNX-GPL-FX-297, 300 */
					}else{	/* FCLNX-GPL-FX-429 */
						if(!test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)
						&& !test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
							set_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);
							hfc_fx_w_stop( pp, core, HFC_FX_LINKUP_TMR );
							hfc_fx_w_start( pp, core, HFC_FX_LINKUP_TMR, pp->linkup_tmo );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
							if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
								set_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
								hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, TRUE);	
								hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, FALSE);
							}
#endif	/* FCLNX-GPL-FX-424 */
						}
					}	/* FCLNX-GPL-FX-429 */
				}
			}
		}
		
		if (rtn & HFC_XRBRESP_ABEND) {
			/* rtn byte2 is abend code */
			hfc_fx_hand2_trace(
				HFC_FX_TRC_HANDLER, 0x21, pp, rp, core, NULL, NULL,
				0, (uint64_t)mpint.c[0], 0);
			hfc_fx_abend(pp, core, (rtn >> 8));
			goto intr_xrb_exit;
		}
		
		if (rtn & (HFC_XRBRESP_TMCMD_NORMAL | HFC_XRBRESP_TMCMD_ERR)) {
			for (i=0; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				
				if (vpp->rid != core->rp->rid)
					continue;
				
				if((atomic_read(&vpp->check_mbreq)) && !test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&vpp->status_detail2 ) ) 
					start_fx_next_mailbox(vpp, NULL);
			}
		}
		
		if (core->next_dstart_cnt) {
			hfc_fx_issue_intl_start(core->next_dstart_top->pp, rp, core, core->next_dstart_top);
		}
	}
	
intr_xrb_exit:
	if (!portlock) {
		HFC_COREUNLOCK_IRQRESTORE(core, flags);
	}
	else {
		if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {	/* FCLNX-GPL-FX-250 */
			if(wait_iodone_hfcp != NULL){	/* FCLNX-GPL-FX-261 */
//				HFC_DBGPRT("hfcldd%d : hfc_fx_intr_xrb - call hfc_fx_mp_intr_xrb. rtn=%08x",pp->dev_minor,rtn);
				
//				HFC_DBGPRT("hfc_fx_intr_xrb hfc_pkt_fx dump\n");
//				structdump( 0xcc, (uchar *)(wait_iodone_hfcp), sizeof(struct hfc_pkt_fx) );
				
				hfc_manage_info.npubp->hfc_fx_mp_intr_xrb(wait_iodone_hfcp);
			}
			if ( hfc_manage_info.wait_reset_mp_fx )						/* FCLNX-0429 *//* FCLNX-GPL-FX-261 */
			{
				hfc_manage_info.npubp->hfc_fx_check_dev_reset_complete();	/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_fx_check_bus_reset_complete();	/* FCLNX-0429 */
			}/* FCLNX-GPL-FX-261 */
		}
		hfc_fx_hand2_trace(
			HFC_FX_TRC_HANDLER, 0x30, pp, rp, core, NULL, NULL,
			0, 0, 0);
		
		HFC_ALLCOREUNLOCK(rp);
		if (!hfc_manage_info.hfcldd_mp_mod) {
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		}else{
			HFC_PORTUNLOCK(pp);
			HFC_SYSUNLOCK_IRQRESTORE(pp, flags);
		}
	}
	
	return IRQ_HANDLED;
}

/*
 * Function:	hfc_fx_intr_share
 *
 * Purpose:		Interrupt handler for interrupts except Mailbox Response.
 *				This routine examines each factor of INT_VECTOR register
 *				on PCI memory space and handles the particular factor.
 *
 *				Interrupt factors are following:
 *					Hardware                    (0x04)
 *					Hyper(hvm)                  (0x01)
 *					Mailbox                     (0x80)
 *					Mailbox Response            (0x40)
 *					XRB Response                (0x20)
 *
 * Arguments:
 *  irq        - IRQ number
 *  intr_entry - Pointer to the instance of hfc_intr_entry structure
 *				 which corresponds to the interruption.
 *
 * Returns:    - IRQ_HANDLED
 *
 * Notes:
 */

irqreturn_t
hfc_fx_intr_share(int irq, void *intr_entry)
{
	struct port_info		*pp;
	struct port_info		*vpp;
	struct region_info		*rp = NULL;
	struct core_info		*core = NULL;
	struct target_info_fx	*target;
	
	int 					int_vector_reg = 0, int_vector_reg1 = 0;
	uint					int_status_reg0, int_status_reg1, err_detail;
	ulong					flags = 0;
	int						i,j,k;
	int						rtn = 0;
	uchar					int_rid_of_core[4]; /* Interruption RID of each core */
	uchar					portlock;
	uchar					trace = 0;
	uchar					abend_code = 0, f_mck_type = 0, hw_mck=0;
	uint64_t				time = 0;
	uint					cpuno = 0;
	uint					int_a_reg=0,hyp_status=0;
	uint					vector,entry_num;
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} int_vector;
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} mpint;
	uint					core_status2=0, addr = 0;
	uchar					xrb_factor_num = 0;

	struct hfc_pkt_fx		*wait_iodone_hfcp=NULL;
	
	/* Check if *intr_entry is not NULL */
	if (!intr_entry)
	{
		HFC_DBGPRT("hfc_fx_intr_share: NULL *intr_entry\n");
		return IRQ_NONE;
	}

	/* Check if *pp is not NULL */
	if ( !( (struct port_info *) ((struct hfc_intr_entry *)intr_entry)->pp ) )
	{
		HFC_DBGPRT("hfc_fx_intr_share: NULL *pp\n");
		return IRQ_NONE;
	}
	
	/* Initialize pp */
	pp = (struct port_info *)((struct hfc_intr_entry *)intr_entry)->pp;
	
	/* get timestamp and processor id */
	if (pp->pm_control == HFC_FX_PM_ON) {
		cpuno = smp_processor_id();
		rdtscll(time);
	}
	
	vector = ((struct hfc_intr_entry *)intr_entry)->vector;
	entry_num = ((struct hfc_intr_entry *)intr_entry)->entry_num;
	
	/* Read INT_VECTOR */
	if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
		int_vector.l = hfc_fx_read_reg_rss_core(pp, 0, entry_num, (uint)HFC_IOSPACE_RSS_INT_VECTOR,
						(char)0x4, HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
	}
	else {
		int_vector.l = (uint)hfc_fx_read_reg(pp, (uint)HFC_IOSPACE_INT_VECTOR, (char)0x4);
	}
	
	if (!hfc_manage_info.hfcldd_mp_mod) {
		HFC_PORTLOCK_IRQSAVE(pp,flags);
	}else{
		HFC_SYSLOCK_IRQSAVE(pp, flags);
		HFC_PORTLOCK(pp);
	}
	
	if( int_vector.l == 0 ) /* Check whether this int. is for handler or not */
	{
		if(HFC_FX_MMODE_CHECK_SHADOW(pp)){	/* FCLNX-GPL-FX-385 *//* FCLNX-GPL-FX-408,461 */
			HFC_DBGPRT("hfcldd : hfc_fx_intr_share 3 - Check dstart queue and mbreq");
			if ( test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) && test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) ) {
				HFC_DBGPRT("hfcldd : hfc_fx_intr_share 4 - Check dstart queue and mbreq");
				clear_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);
				hfc_fx_mck_recovery_five_fx(pp, HFC_ABEND_MCK_RESUME); /* resume MCK recovery */
			}
		}
		/* int factor dummy reset */
		if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
			hfc_fx_write_reg_rss_core(pp, 0, entry_num, HFC_IOSPACE_RSS_INT_VECTOR_RST,
								0x4, 0x02000000, HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
			pp->dummy_int_rst_cnt++;
		}
		goto intr_share_exit;
	}

	if( int_vector.l & 0xdfdfdfdf )
	{
		hfc_fx_hand2_trace(
			HFC_FX_TRC_HANDLER, 0x00, pp, NULL, NULL, NULL, NULL,
			(uint64_t)int_vector.l, 0, 0);
	}

	/* Mask int_vector with shr_info */
	int_vector.l = int_vector.l & ((struct hfc_intr_entry *)intr_entry)->shr_info ;

	/*  FIVE-FX: Exg. Mck, PCI Error (SERR, PERR, SPERR) or SRAM 1bit Error occured */
	if( int_vector.l & 0x04040404 )
	{
		/* check vport link_reset pending */
		for (i=1; i<=pp->max_vport_count; i++) {
			vpp = pp->vport_ptr[i].vport_arg;
			if (vpp == NULL)
				continue;
			
			if (test_bit( HFC_PD_LINK_RESET, (ulong *)&vpp->status_detail2)) {
				HFC_DBGPRT("hfc_fx_intr_share: vport link_reset pending\n");
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2);
				break;
			}
		}
		
		/* HFC_HWERR_INT */
		for(i=0; i<4; i+=4/pp->core_num)
		{
			if(int_vector.c[3-i] & 0x04)
			{
				if( pp->region_arg[pp->rid] != NULL ){
					if( pp->region_arg[pp->rid]->core_arg[i] != NULL ){
						core = pp->region_arg[pp->rid]->core_arg[i];
					}
					else{
						continue;
					}
				}
				
				/* Read Int_a_reg to recognize the factor */
				int_a_reg = hfc_fx_read_reg_core(pp, i, (uint)HFC_IOSPACE_INTA,
						(char)0x4, HFC_FX_CORE_OFFSET10);
				
				if( int_a_reg & HFC_FX_HWERR_INT_A )
				{
					if( !hfc_fx_check_cs_disable(pp, core) ){
						hw_mck = 1;
						if(hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i], 0x4 ) & HFC_HWERR_MPCHK){
							HFC_DBGPRT("hfcldd : hfcl_intr_share - intr type is HFC_HWERR_INT(MPCK), core#%d status:%08x %08x %08x %08x\n", i,
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i], 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0x4, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0x8, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0xc, 0x4 )
								);
							if( abend_code == 0 ){
								HFC_DBGPRT("hfcldd : hfcl_intr_share - MPCK i = %d core# = %d\n",i, core->core_no);
								abend_code = HFC_ABEND_MPCHK ;
								pp->mck_core_no = core->core_no;
								if( !test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 ) ){
									core->mck_err_cnt++;
								}
							}
						
						}
						else{
							core_status2 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0x8, 0x4 );
							HFC_DBGPRT("hfcldd : hfcl_intr_share - intr type is HFC_HWERR_INT(HWMCK), core#%d status:%08x %08x %08x %08x\n", i,
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i], 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0x4, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0x8, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[i]+0xc, 0x4 )
								);
							if( test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ) ){
								addr = 0x326 + 0x80*i;
								f_mck_type = hfc_fx_read_reg_ext(pp, ( uint )addr, ( char )0x1 );
							
								if( abend_code == 0 ){
									if( f_mck_type ){
										abend_code = HFC_ABEND_T3 ;
										pp->mck_core_no = core->core_no;
										HFC_DBGPRT("hfcldd : hfcl_intr_share - intr type is HFC_HWERR_INT(F-MCK), core#%d\n",i);
										if( !test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 ) ){
											core->mck_err_cnt++;
										}
									}
								}
							}
							else if( !( core_status2 & HFC_FX_HBROADMCK )){
								if( !test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 ) ){	/* FCLNX-GPL-FX-430 */
									core->mck_err_cnt++;
								}	/* FCLNX-GPL-FX-430 */
								if( abend_code == 0 ){
									abend_code = HFC_ABEND_MCK_INT ;
									pp->mck_core_no = core->core_no;
									HFC_DBGPRT("hfcldd : hfcl_intr_share - intr type is HFC_HWERR_INT(HWMCK), core#%d\n",i);
								}
							}
						}
					}
				
					HFC_DBGPRT("hfcldd : hfcl_intr_share - MPCK_Code=%08x, Force_MCK_Code=%02x\n",
						(uint)hfc_fx_read_reg_ext(pp, (uint) (0x300 + 0x80 * pp->region_arg[0]->core_arg[i]->pcore_no + 0x4), 0x4),
						(uint)hfc_fx_read_reg_ext(pp, (uint) (0x300 + 0x80 * pp->region_arg[0]->core_arg[i]->pcore_no + 0x26), 0x1) );
				}
			}
		}
		
		/* HW MCK Case */
		if( hw_mck == 1 ){
			if( !pp->core_deg_mode ){	/* FCLNX-GPL-FX-051 */
				if( abend_code == 0 ){
					HFC_DBGPRT("hfcldd : hfcl_intr_share - intr type is HFC_HWERR_INT(HWMCK), core#\n");
					abend_code = HFC_ABEND_MCK_INT ;
					if( !test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 ) ){
						for(i=0; i<4; i+=4/pp->core_num)
						{
							if( pp->region_arg[pp->rid] != NULL ){
								if( pp->region_arg[pp->rid]->core_arg[i] != NULL ){
									pp->region_arg[pp->rid]->core_arg[i]->mck_err_cnt++;
								}
								else{
									continue;
								}
							}
						}
					}
				}
			}
			else{
				if( !test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 ) ){
					pp->mck_err_cnt++;
				}
			}							/* FCLNX-GPL-FX-051 */
		
			for (i=0; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				
				clear_bit( HFC_PS_WAIT_MCKINT, (ulong *)&vpp->status );
			}
		
			/* Check whether PCI bus error occured or not */
			int_status_reg0 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[0], 0x4 );
			int_status_reg1 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[0]+0x4, 0x4 );
			err_detail = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4);
			
			HFC_DBGPRT("hfcldd : hfcl_intr_share - int_status_reg0=%08x, int_status_reg1=%08x, abend_code = %02x\n",
				int_status_reg0,int_status_reg1,abend_code);

			/* Reset all interrupt factor */
			hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INTA_RST,( char )0x4, 0xffffffff );
			hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INT_1_RST,( char )0x4, 0xffffffff );
			hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INT_2_RST,( char )0x4, 0xffffffff );
			hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INT_3_RST,( char )0x4, 0xffffffff );
			
			if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-458 */
				hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
				if((hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOL)||
				(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOLRCV)||
				(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_ISOL)){
					HFC_DBGPRT("hfclddd : READ HYPERSTATUS - 3\n");
					hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,
						0x6a,(uchar*)&hyp_status, 4);
					goto intr_hwerr;
				}
			}
			
			hfc_fx_hwerr_int(pp,int_vector.l,int_status_reg0,int_status_reg1,err_detail, abend_code);
			
			goto intr_hwerr;
		}
		else{
			HFC_DBGPRT("hfcldd : hfcl_intr_share - Not HW_MCK. abend_code = %02x\n", abend_code);
			/* check SRAM_CE or not */
			for(i=0; i<4; i+=4/pp->core_num)
			{
				if(int_vector.c[3-i] & 0x04)
				{
					if( pp->region_arg[pp->rid] != NULL ){
						if( pp->region_arg[pp->rid]->core_arg[i] != NULL ){
							core = pp->region_arg[pp->rid]->core_arg[i];
						}
						else{
							continue;
						}
					}
				
					/* Read Int_a_reg to recognize the factor */
					int_a_reg = hfc_fx_read_reg_core(pp, i, (uint)HFC_IOSPACE_INTA,
						(char)0x4, HFC_FX_CORE_OFFSET10);
					
					if( int_a_reg & HFC_FX_PCIE_SRAM_CE )
					{
						abend_code = HFC_ABEND_SRAM_CE;
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
							(char)0x1, 0x08, HFC_FX_CORE_OFFSET10);
					}
				}
			}
			if( abend_code == HFC_ABEND_SRAM_CE ){
				/* Check whether PCI bus error occured or not */
				int_status_reg0 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[0], 0x4 );
				int_status_reg1 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[0]+0x4, 0x4 );
				err_detail = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4);
			
				HFC_DBGPRT("hfcldd : hfcl_intr_share - abend_code = HFC_ABEND_SRAM_CE\n");
				hfc_fx_hwerr_int(pp,int_vector.l,int_status_reg0,int_status_reg1,err_detail, abend_code);
			
				goto intr_hwerr;
			}
		}
	}
	else if( int_vector.l & 0x01010101 )
	{
		/* for MLPF */
		for(i=0; i<4; i+=4/pp->core_num)
		{
			int_rid_of_core[i] = 0xff;
			if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
				int_rid_of_core[i] = 0x00;
			}else{
				mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
													 0x04, HFC_FX_CORE_OFFSET10);
				int_rid_of_core[i] = pp->rid;
			}
			
			if(int_vector.c[3-i] & 0x01)
			{
				if( int_rid_of_core[i] != 0xff ){
					rp = pp->region_arg[int_rid_of_core[i]];
					if(rp == NULL){
						HFC_DBGPRT("hfcldd%d : hfcl_intr_share mlpf_int - rp = NULL i=%d int_rid_of_core[i]=%d\n",pp->dev_minor,i,int_rid_of_core[i]);
						hfc_fx_hand2_trace(
							HFC_FX_TRC_HANDLER,0x15, pp, NULL, NULL, NULL, NULL,
							(uint64_t)int_vector.l, 0, 0);
						continue;
					}
					core = rp->core_arg[i];
					if(core == NULL){
						HFC_DBGPRT("hfcldd%d : hfcl_intr_share mlpf_int - core = NULL i=%d int_rid_of_core[i]=%d\n",pp->dev_minor,i,int_rid_of_core[i]);
						hfc_fx_hand2_trace(
							HFC_FX_TRC_HANDLER,0x16, pp, NULL, NULL, NULL, NULL,
							(uint64_t)int_vector.l, 0, 0);
						continue;
					}
					if ( HFC_FX_MMODE_CHECK_SHARED (pp) ){
						if( hfc_fx_mlpf_intr(pp, core, int_vector.l) ){	/* FCLNX-GPL-FX-386 */
							goto intr_mlpf;
						}
					}
				}
			}
		}
		/* HVM int factor reset */
		hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, (int_vector.l & 0x01010101) );
	}
	else if( int_vector.l & 0x40404040 )
	{
		/* Mailbox response received */
		HFC_DBGPRT("hfcldd : hfcl_intr_share - Mailbox response received, int_vector.l=0x%08x\n", int_vector.l);
		for(i=0; i<4; i+=4/pp->core_num)
		{
			int_rid_of_core[i] = 0xff;
			if( int_vector.c[3-i] & 0x40 )
			{
				/* Read RID */
				if (HFC_FX_VPORT_EXIST(pp)) {
					/* vport exists in physical port */
					mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
														 0x04, HFC_FX_CORE_OFFSET10);
				}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
					mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
														 0x04, HFC_FX_CORE_OFFSET10);
				}else {
					mpint.l = 0;
				}
				if ( mpint.c[1] == 0x80) {
					/* physical port */
					int_rid_of_core[i] = 0x00;
				}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
					int_rid_of_core[i] = pp->rid;
				}
				else {
					/* virtual port */
					int_rid_of_core[i] = mpint.c[1];
				}
				HFC_DBGPRT("hfcldd : hfcl_intr_share - Mailbox response received, rid=%d\n", mpint.c[1]);
			}
		}

		/* Mailbox int factor reset */
		if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
			hfc_fx_write_reg_rss_core(pp, 0, entry_num, HFC_IOSPACE_RSS_INT_VECTOR_RST,
								0x4, (int_vector.l & 0x40404040), HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
		}
		else {
			hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, (int_vector.l & 0x40404040) );
		}

		/* Dummy read */
		if ( ((struct hfc_intr_entry *)intr_entry)->mode == HFC_INT_TYPE_INTX )
		{
			int_vector_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INT_VECTOR, (char)0x4);
		}

		for(i=0; i<4; i+=4/pp->core_num)
		{
			if( int_rid_of_core[i] != 0xff )
			{
				rp = pp->region_arg[int_rid_of_core[i]];
				if(rp == NULL){
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - rp = NULL i=%d int_rid_of_core[i]=%d\n",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x02, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
					continue;
				}
				core = rp->core_arg[i];
				if(core == NULL){
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - core = NULL i=%d int_rid_of_core[i]=%d\n",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x03, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
					continue;
				}
				HFC_ALLCORELOCK(rp);
				vpp = rp->mb_pp;
				if(vpp == NULL){
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - vpp = NULL i=%d int_rid_of_core[i]=%d\n",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x04, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
				}
				else {
					hfc_fx_mb_resp(vpp, rp->core_arg[i], rp);
				}
				HFC_ALLCOREUNLOCK(rp);
			}
		}
	}

	else if( int_vector.l & 0x80808080 )
	{
		/* Mailbox Interrupt Request */
		HFC_DBGPRT("hfcldd : hfcl_intr_share - Mailbox Interrupt Request, int_vector.l=0x%08x\n", int_vector.l);
		for(i=0; i<4; i+=4/pp->core_num)
		{
			int_rid_of_core[i] = 0xff;
			if( int_vector.c[3-i] & 0x80 )
			{
				/* Read RID */
				if (HFC_FX_VPORT_EXIST(pp)) {
					/* vport exists in physical port */
					mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
														 0x04, HFC_FX_CORE_OFFSET10);
				}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
					mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
														 0x04, HFC_FX_CORE_OFFSET10);
				}else {
					mpint.l = 0;
				}
				if ( mpint.c[2] == 0x80) {
					/* physical port */
					int_rid_of_core[i] = 0x00;
				}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
					int_rid_of_core[i] = pp->rid;
				}
				else {
					/* virtual port */
					int_rid_of_core[i] = mpint.c[2];
				}
				HFC_DBGPRT("hfcldd : hfcl_intr_share - Mailbox Interrupt Request, rid=%d\n", mpint.c[2]);
			}
		}

		/* Mailbox int factor reset */
		if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
			hfc_fx_write_reg_rss_core(pp, 0, entry_num, HFC_IOSPACE_RSS_INT_VECTOR_RST,
								0x4, (int_vector.l & 0x80808080), HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
		}
		else {
			hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, (int_vector.l & 0x80808080) );
		}

		/* Dummy read */
		if ( ((struct hfc_intr_entry *)intr_entry)->mode == HFC_INT_TYPE_INTX )
		{
			int_vector_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INT_VECTOR, (char)0x4);
		}
		
		for(i=0; i<4; i+=4/pp->core_num)
		{
			if( int_rid_of_core[i] != 0xff )
			{
				rp = pp->region_arg[int_rid_of_core[i]];
				if(rp == NULL){
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - rp = NULL i=%d int_rid_of_core[i]=%d",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x05, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
					continue;
				}
				core = rp->core_arg[i];
				if(core == NULL){
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - core = NULL i=%d int_rid_of_core[i]=%d",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x06, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
					continue;
				}
				
				HFC_ALLCORELOCK(rp);
				if (HFC_FX_EXT_VPORT_EXIST(rp)) {
					vpp = hfc_fx_get_port_info_by_portid(pp->vport_ptr[int_rid_of_core[i]].vport_arg, rp, core);
				}
				else if ( HFC_FX_MMODE_CHECK_SHARED (pp) ){	/* FCLNX-GPL-FX-381 */
					vpp = pp->vport_ptr[0].vport_arg;
				}	/* FCLNX-GPL-FX-381 */
				else {
					vpp = pp->vport_ptr[int_rid_of_core[i]].vport_arg;
				}
				if (vpp == NULL) {
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - vpp = NULL i=%d int_rid_of_core[i]=%d",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x07, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
				}
				else if (HFC_PP_FX_STATUS_DETAIL2_TEST(HFC_PD_WAIT_CLOSE, vpp)) {
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - HFC_PD_WAIT_CLOSE i=%d int_rid_of_core[i]=%d",pp->dev_minor,i,int_rid_of_core[i]);
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER,0x08, pp, NULL, NULL, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
				}
				else {
					hfc_fx_mb_intreq(vpp, rp->core_arg[i], rp);
				}
				HFC_ALLCOREUNLOCK(rp);
			}
		}
	}
	
	else if( int_vector.l & 0x20202020 )
	{
		/* XRB Interrupt Response */
		for(i=0; i<4; i+=4/pp->core_num)
		{
			int_rid_of_core[i] = 0xff;
			if( int_vector.c[3-i] & 0x20 )
			{
				/* Read RID */
				if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
					mpint.c[0] = (uchar)entry_num;
				}
				else if (HFC_FX_VPORT_EXIST(pp)) {
					/* vport exists in physical port */
					mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
														 0x04, HFC_FX_CORE_OFFSET10);
				}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
					mpint.l = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_MPINTAC,
														 0x04, HFC_FX_CORE_OFFSET10);
				}else {
					mpint.l = 0;
				}
				if ( mpint.c[0] == 0x80) {
					/* physical port */
					int_rid_of_core[i] = 0x00;
				}else if(HFC_FX_MMODE_CHECK_MLPF(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
					int_rid_of_core[i] = pp->rid;
				}
				else {
					/* virtual port */
					int_rid_of_core[i] = mpint.c[0];
				}
				xrb_factor_num++;
			}
		}
		
		/* XRB int factor reset */
		if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
			hfc_fx_write_reg_rss_core(pp, 0, entry_num, HFC_IOSPACE_RSS_INT_VECTOR_RST,
								0x4, (int_vector.l & 0x20202020), HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
		}
		else {
			hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, (int_vector.l & 0x20202020) );
		}

		/* Dummy read */
		if ( ((struct hfc_intr_entry *)intr_entry)->mode == HFC_INT_TYPE_INTX )
		{
			int_vector_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INT_VECTOR, (char)0x4);
		}
		
		portlock = 1;
		
		for(i=0; i<4; i+=4/pp->core_num)
		{
			if( int_rid_of_core[i] != 0xff )
			{
				rp = pp->region_arg[int_rid_of_core[i]];
				core = rp->core_arg[i];
				
				HFC_CORELOCK(core);
				
				if ((!core->tskmgm_cmd_num) && (!core->preserve_cmd_num)){
					/* task management command none in core or not master core */
					HFC_PORTUNLOCK(pp);
					if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
						HFC_SYSUNLOCK(pp);
					}
					portlock = 0;
				}
				else {
					/* task management command exists in core and master core */
					HFC_COREUNLOCK(core);
					HFC_ALLCORELOCK(rp);
					portlock = 1;
					trace = 1;
					
					hfc_fx_hand2_trace(
						HFC_FX_TRC_HANDLER, 0x01, pp, rp, core, NULL, NULL,
						(uint64_t)int_vector.l, 0, 0);
				}
				
				rtn = 0;
				if ( HFC_FX_MMODE_CHECK_SHARED (pp) ){
					vpp = pp->vport_ptr[0].vport_arg;
				}else{
					vpp = pp->vport_ptr[int_rid_of_core[i]].vport_arg;
				}
				if (vpp == NULL) {
					trace = 1;
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - vpp = NULL i=%d int_rid_of_core[i]=%d",pp->dev_minor,i,int_rid_of_core[i]);
				}
				else if (HFC_PP_FX_STATUS_DETAIL2_TEST(HFC_PD_WAIT_CLOSE, vpp)) {
					trace = 1;
					HFC_DBGPRT("hfcldd%d : hfcl_intr_share - HFC_PD_WAIT_CLOSE i=%d int_rid_of_core[i]=%d",pp->dev_minor,i,int_rid_of_core[i]);
				}
				else {
					rtn = hfc_fx_xrb_resp(vpp, rp, core, time, cpuno, &wait_iodone_hfcp);
					/* rtn	byte0:	0x00		*/
					/*		byte1:	icc_count	*/
					/*		byte2:	abend_code	*/
					/*		byte3:	resp_code	*/
				}
				
				if (!rtn) {
					/* all io commands normal end */
					if (core->next_dstart_cnt) {
						hfc_fx_issue_intl_start(core->next_dstart_top->pp, rp, core, core->next_dstart_top);
					}
					
					if (!portlock) {
						if (xrb_factor_num == 1) {
							HFC_COREUNLOCK_IRQRESTORE(core,flags);
							goto intr_share_xrb_exit;
						}
						else {
							HFC_COREUNLOCK(core);
							if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
								HFC_SYSLOCK(pp);
							}
							HFC_PORTLOCK(pp);
							portlock = 1;
						}
					}
					else {
						HFC_ALLCOREUNLOCK(rp);
					}
				}
				else {
					/* io command or task management command abnormal end */
					trace = 1;
					
					if (!portlock) {
						HFC_COREUNLOCK(core);
						if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
							HFC_SYSLOCK(pp);
						}
						HFC_PORTLOCK(pp);
						HFC_ALLCORELOCK(rp);
						portlock = 1;
					}
					
					if (rtn & (HFC_XRBRESP_IOCMD_LINK_ERR | HFC_XRBRESP_TMCMD_LINK_ERR)) { /* FCLNX-GPL-FX-280, 287 */
						if (core->icc_err != NULL) {
							if (core->icc_err->err_no != 0) {
								hfc_fx_errlog(
									core->icc_err->icc_pp,
									core,
									core->icc_err->icc_pp->target_arg[core->icc_err->pseq],
									&core->icc_err->hfcp,
									HFC_ERRLOG_TYPE_XRB,
									core->icc_err->err_id,
									core->icc_err->err_no,
									core->icc_err->logdata,
									16);
								
								if (core->icc_err->first_icc != 1) {
									/* not icc after linkdown */
									if (pp->if_err_limit) {
										/* rtn byte1 is icc count */
										for (j=0; j<((rtn >> 16) & 0x000000ff); j++) {
											if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-324 Start */
												hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_IF_ERR);
											}else{
												hfc_fx_watched_errcount_i(pp, NULL, HFC_IF_ERR);
											}	/* FCLNX-GPL-FX-324 End */
										}
									}
									if (HFC_FX_VPORT_EXIST(pp)) {	/* FCLNX-GPL-FX-297, 300 */
										if (pp->c_err) {
											if (pp->region_arg[0] == NULL) {
												HFC_ALLCOREUNLOCK(rp);
												goto intr_share_xrb_exit;
											}
											HFC_ALLCOREUNLOCK(rp);
											
											/* all core_lock for rid0 */
											HFC_ALLCORELOCK(pp->region_arg[0]);
											if (hfc_fx_check_errcount(pp)) {
												/* isolate done */
												HFC_ALLCOREUNLOCK(pp->region_arg[0]);
												goto intr_share_xrb_exit;
											}
											HFC_ALLCOREUNLOCK(pp->region_arg[0]);
											
											HFC_ALLCORELOCK(rp);
										}
									}
									else {
										if (hfc_fx_check_errcount(pp)) {
											/* isolate done */
											HFC_ALLCOREUNLOCK(rp);
											goto intr_share_xrb_exit;
										}
									}	/* FCLNX-GPL-FX-297, 300 */
								}else{	/* FCLNX-GPL-FX-429 */
									if(!test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)
									&& !test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
										set_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);
										hfc_fx_w_stop( pp, core, HFC_FX_LINKUP_TMR );
										hfc_fx_w_start( pp, core, HFC_FX_LINKUP_TMR, pp->linkup_tmo );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
										if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
											set_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
											hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, TRUE);	
											hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, FALSE);
										}
#endif	/* FCLNX-GPL-FX-424 */
									}
								}	/* FCLNX-GPL-FX-429 */
							}
						}
					}
					
					if (rtn & HFC_XRBRESP_ABEND) {
						/* rtn byte2 is abend code */
						hfc_fx_hand2_trace(
							HFC_FX_TRC_HANDLER, 0x09, pp, rp, core, NULL, NULL,
							(uint64_t)int_vector.l, (uint64_t)int_rid_of_core[i], 0);
						hfc_fx_abend(pp, core, (rtn >> 8));
						HFC_ALLCOREUNLOCK(rp);
						goto intr_share_xrb_exit;
					}
					
					if (rtn & (HFC_XRBRESP_TMCMD_NORMAL | HFC_XRBRESP_TMCMD_ERR)) {
						if((atomic_read(&vpp->check_mbreq)) && !test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&vpp->status_detail2 )) {
							start_fx_next_mailbox(vpp, NULL);
						}
					}
					
					if (core->next_dstart_cnt) {
						hfc_fx_issue_intl_start(core->next_dstart_top->pp, rp, core, core->next_dstart_top);
					}
					
					HFC_ALLCOREUNLOCK(rp);
				}
			}
		}
		
intr_share_xrb_exit:
		if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
			if(wait_iodone_hfcp != NULL){	/* FCLNX-GPL-FX-250 */
				hfc_manage_info.npubp->hfc_fx_mp_intr_xrb(wait_iodone_hfcp);
			}
			if ( hfc_manage_info.wait_reset_mp_fx )						/* FCLNX-0429 */
			{
				hfc_manage_info.npubp->hfc_fx_check_dev_reset_complete();	/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_fx_check_bus_reset_complete();	/* FCLNX-0429 */
			}
		}
		if (trace) {
			hfc_fx_hand2_trace(
				HFC_FX_TRC_HANDLER, 0x11, pp, rp, core, NULL, NULL,
				(uint64_t)int_vector.l, 0, 0);
		}
		if (portlock) {
			if (!hfc_manage_info.hfcldd_mp_mod) {
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			}else{
				HFC_PORTUNLOCK(pp);
				HFC_SYSUNLOCK_IRQRESTORE(pp, flags);
			}
		}
		
		return IRQ_HANDLED;
	}
	else
	{
		/* No other interrupt factors happen because int mask is disabled */
		/* Reset INT_VECTOR */
		int_vector_reg = int_vector.l ;
		/* hfc_fx_issue_int_a_rst(pp, int_vector_reg, int_vector.l); */
		int_vector_reg &= 0xE5E5E5E5;
		if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
			hfc_fx_write_reg_rss_core(pp, 0, entry_num, HFC_IOSPACE_RSS_INT_VECTOR_RST,
								0x4, int_vector_reg, HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
		}
		else {
			hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, int_vector_reg );
		}
		if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){	/* FCLNX-GPL-FX-385 *//* FCLNX-GPL-FX-408 */
			rp = pp->region_arg[0];
			HFC_4L_TO_4B(int_vector_reg1, int_vector.l);
			HFC_ALLCORELOCK(rp);
			hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,0x6a,(uchar*)&int_vector_reg1, 4);
			HFC_ALLCOREUNLOCK(rp);
		}else if(HFC_FX_MMODE_CHECK_SHADOW(pp)){	/* FCLNX-GPL-FX-385 *//* FCLNX-GPL-FX-408,461 */
			HFC_DBGPRT("hfcldd : hfc_fx_intr_share 1 - Check dstart queue and mbreq");
			if ( test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) && test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) ) {
				HFC_DBGPRT("hfcldd : hfc_fx_intr_share 2 - Check dstart queue and mbreq");
				clear_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);
				hfc_fx_mck_recovery_five_fx(pp, HFC_ABEND_MCK_RESUME); /* resume MCK recovery */
			}
		}	/* FCLNX-GPL-FX-385 *//* FCLNX-GPL-FX-408,461 */
	}
	
intr_mlpf:

	/* Check dstart queue and mbreq */
	HFC_DBGPRT("hfcldd : hfc_fx_intr_share - Check dstart queue and mbreq");
	for (i=0; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		rp = pp->region_arg[vpp->rid];
		if (rp == NULL)
			continue;
		
		HFC_ALLCORELOCK(rp);
		if((atomic_read(&vpp->check_mbreq)) && !test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&vpp->status_detail2 ) ) 
			start_fx_next_mailbox(vpp, NULL);
		
		for(j=0; j<4; j+=4/vpp->core_num) {
			core = rp->core_arg[j];
			if (!core)	break;
			
			if (core->next_dstart_cnt) {
				for (k=0; k<MAX_TARGET_PROBE; k++) {
					target = vpp->target_arg[k];
					if(target != NULL) {
						hfc_fx_issue_intl_start(vpp, rp, core, target);
					}
				}
			}
		}
		HFC_ALLCOREUNLOCK(rp);
	}
	
	/* Check errcount */
	HFC_DBGPRT("hfcldd : hfcl_intr_share - Check errcount");
	if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){	/* FCLNX-GPL-FX-385 */
		rp = pp->region_arg[0];
	}else{
		rp = pp->region_arg[pp->rid];
	}	/* FCLNX-GPL-FX-385 */
	HFC_ALLCORELOCK(rp);
	hfc_fx_check_errcount(pp);
	HFC_ALLCOREUNLOCK(rp);
	
intr_hwerr:

	hfc_fx_hand2_trace(
		HFC_FX_TRC_HANDLER,0x10, pp, NULL, NULL, NULL, NULL,
		(uint64_t)int_vector.l, 0, 0);

intr_share_exit:
	if (!hfc_manage_info.hfcldd_mp_mod) {
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	}else{
		HFC_PORTUNLOCK(pp);
		HFC_SYSUNLOCK_IRQRESTORE(pp, flags);
	}

	return IRQ_HANDLED;

}


/*
 * Function:    hfc_fx_mb_resp
 *
 * Purpose:     This routine deals with Mailbox interrupt 
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from the other device.
 *
 * Notes:       
 */
void hfc_fx_mb_resp(struct port_info *pp, struct core_info *core, struct region_info *rp)
{
	uint	mb_code=0, payload_cmd=0;
	uint64_t	mb_resp=0;
	struct mailbox_fx	*mbox=NULL;

	HFC_ENTRY("hfc_fx_mb_resp");
//	HFC_DBGPRT("*hfcldd : entry %s()\n)","hfc_fx_mb_resp");

	if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) )
	{
		hfc_fx_hand2_trace(
			HFC_FX_TRC_MBRESP, 0x01, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
	
		HFC_DBGPRT("hfc_fx_mb_resp @ HFC_MB_PROL\n");
		hfc_fx_wake_up(&pp->mb_event, &(pp->mb_event_wait));
		pp->mb_prol_wake_up_time = (uint)jiffies; /* FCLNX-GPL-243 */
	}
	else
	{
		mbox = core->mb;
		if( mbox == NULL ) return;
		
		HFC_4B_TO_4L(mb_code, mbox->mb_resp.mb_code);

		HFC_DBGPRT("hfc_fx_mb_resp @ core# = %d otherwise %08x\n", core->core_no, mb_code);
		
		mb_resp |= (uint64_t)mb_code;
		if((mb_code & 0xffff0000) == HFC_MBCMD_SNDRCV){
			payload_cmd = (uchar) hfc_fx_read_val( core->payload->send_payload.data0[0]);
			mb_resp |= (uint64_t)payload_cmd;
		}
		hfc_fx_hand2_trace(
			HFC_FX_TRC_MBRESP, 0x00, pp, core->rp, core, NULL, NULL,
			(uint64_t)mb_resp, 0, 0);
			
		switch( (mb_code & 0xffff0000) ){
			case HFC_MBCMD_CORESTART:
				if(test_bit(HFC_PD_WAIT_CORE_START, (ulong *)&pp->status_detail1))
					hfc_fx_core_start_resp(pp, core);
				break;
			
			case HFC_MBCMD_OFFLINEMB:
				hfc_fx_offline_mb_resp(pp, core);
				break;
			
			case HFC_MBCMD_SHADOWUP:
				hfc_fx_shadow_up_resp(pp, core);
				break;
			
			case HFC_MBCMD_ADDPORTID:
				if(test_bit(HFC_PD_WAIT_ADD_PORTID, (ulong *)&pp->status_detail1))
					hfc_fx_add_port_id_resp(pp, core);
				break;
				
			case HFC_MBCMD_DELPORTID:
				hfc_fx_delete_port_id_resp(pp, core);
				break;
 			
			case HFC_MBCMD_MIHLOG:
				hfc_fx_mihlog_resp(pp, core);
				break;
			
			case HFC_MBCMD_DIAG:
				hfc_fx_diag_resp(pp, core);
				break;
				
			case HFC_MBCMD_LDCHTRC:
				hfc_fx_load_ch_trace_log_resp(pp, core);
				break;
			
			case HFC_MBCMD_LINKINIT:
				hfc_fx_link_resp(pp, core);
				break;
			
			case HFC_MBCMD_FLOGI:
				hfc_fx_flogi_resp(pp, core);
				break;
			
			case HFC_MBCMD_PLOGI:
				hfc_fx_plogi_resp(pp, core);
				break;
				
			case HFC_MBCMD_PDISC:
				hfc_fx_pdisc_resp(pp, core);
				break;
 			
			case HFC_MBCMD_CSCSI_WAIT_DMA:	/* FCLNX-GPL-FX-112 */
				hfc_fx_cancel_scsi_resp(pp, core);
				break;
			
			case HFC_MBCMD_SNDRCV:
				hfc_fx_frmsndrcv_resp(pp, core);
				break;
			
		}
	}

	HFC_EXIT("hfc_fx_mb_resp (*) ");
//	HFC_DBGPRT("*hfcldd : exit  %s()\n","hfc_fx_mb_resp");

}


/*
 * Function:    hfc_fx_add_port_id_resp
 *
 * Purpose:     This routine deals with Add Port_ID response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_add_port_id_resp(struct port_info *pp, struct core_info *core )
{
	uint				rc_passthrouh = 0xffffffff;
	uint				mb_resp_status = 0;
	struct core_info	*core_wk;
	struct mailbox_fx	*mbox = core->mb;
	uint				i;
	uchar				failed_master_core = 0, failed_slave_core = 0;
	uchar				wait_mb_core = 0, tmp_linkdown_occurred = 0;
	
	HFC_ENTRY("hfc_fx_add_port_id_resp");
	
	core->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);		/* FCWIN-0170 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_ADD_PORTIDRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			)&&
			(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */
				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_ADD_PORTIDRSP, rc_passthrouh);
		}
		
		if(pp->linkdown_occurred == 1) tmp_linkdown_occurred = 1;	/* FCLNX-GPL-FX-174 */
		HFC_DBGPRT("add_port_id_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;

		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("add_port_id_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		
		break;
	default :
		break;
	}
	
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	/* check if whether all the cores finish add port_id or not */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		core_wk = pp->region_arg[pp->rid]->core_arg[i];
		if ( core_wk == NULL
			 || core_wk == core
			 || hfc_fx_check_cs_disable(pp, core_wk) ) /* FCLNX-GPL-FX-438 */
			continue;
		if ( test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core_wk->status) ){
			wait_mb_core = 1;
			break;
		}
		if (core->mb_results != HFC_MBPASS_SUCCESS) {
		    if (core->core_no == pp->master_core_no) {
				failed_master_core = 1;
			} else {
				failed_slave_core = 1;
			}
		}
	}
	
	/* other cores are executing mailbox */
	if (wait_mb_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_ADD_PORTIDRSP, 0x01, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		return;
	}
	
	/* all the cores finished add port_id */
	clear_bit(HFC_PD_WAIT_ADD_PORTID, (ulong *)&pp->status_detail1);

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174  */
	if( tmp_linkdown_occurred == 1 ){	/* FCLNX-GPL-FX-058,174  */
		unlock_fx_mailbox(pp);
		return;
	}	/* FCLNX-GPL-FX-058 */

	/* master_core failed. ChangeState(LinkDown) */
	if (failed_master_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_ADD_PORTIDRSP, 0x02, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		
		hfc_fx_initialize_failed(pp, core, NULL);
		
		return;	
	}
	else if (failed_slave_core) {
		/* only slave core failed. the core is isolated. */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_ADD_PORTIDRSP, 0x03, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		
		hfc_fx_initialize_failed(pp, core, NULL);
		
		return;	
	}
	
	/* Issue SCR with master core */
	hfc_fx_hand2_trace(
		HFC_FX_TRC_ADD_PORTIDRSP, 0x10, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );

	set_bit(HFC_PD_NEED_SCR, (ulong *)&pp->status_detail1);
	atomic_set(&pp->check_mbreq, 1);
	
	/* Start SCR Delay Timer */
//		hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//			pp->mb_timer[ HFC_MBTIME_SCR ].delay ,TRUE);
//		hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//			pp->mb_timer[ HFC_MBTIME_SCR ].delay ,FALSE);
	unlock_fx_mailbox(pp);
	
	return;
}


/*
 * Function:    hfc_fx_delete_port_id_resp
 *
 * Purpose:     This routine deals with Delete Port_ID response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_delete_port_id_resp(struct port_info *pp, struct core_info *core )
{
	uint                 rc_passthrouh = 0xffffffff;
	uint                 mb_resp_status = 0, i=0, wait_mb_core=0;
	struct core_info	*core_wk;
	struct mailbox_fx	 *mbox = core->mb;
	
	HFC_ENTRY("hfc_fx_delete_port_id_resp");
	
	core->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);		/* FCWIN-0170 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_DEL_PORTIDRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_DEL_PORTIDRSP, rc_passthrouh);
		}
		
		HFC_DBGPRT("del_port_id_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;								/* FCWIN-0151 */
//		clear_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1);			/* FCWIN-0082 */

		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("del_port_id_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
//		clear_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1);
//		set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
		break;
	default :
		break;
	}
	
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	/* check if whether all the cores finish add port_id or not */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		core_wk = pp->region_arg[pp->rid]->core_arg[i];
		if ( core_wk == NULL
			 || core_wk == core
			 || hfc_fx_check_cs_disable(pp, core_wk) ) /* FCLNX-GPL-FX-438 */
			continue;
		if ( test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core_wk->status) ){
			wait_mb_core = 1;
			break;
		}
	}

	
	/* other cores are executing mailbox */
	if (wait_mb_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_DEL_PORTIDRSP, 0x01, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		return;
	}
	
	/* all the cores finished add port_id */
	clear_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1);

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	hfc_fx_copy_master_to_slave( pp, core );							/* FCLNX-GPL-FX-18 */
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		hfc_fx_change_portstat_linkdown(pp, core);
	}
	else if (!HFC_PP_FX_STATUS_DETAIL2_TEST(HFC_PD_WAIT_CLOSE, pp)) {
		hfc_fx_change_portstat_linkdown(pp, core);
	}
	hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);						/* FCLNX-GPL-314 */
	
	if(pp->initialize != 0){
		hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_DEL_PORTIDRSP, 0x10, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);															/* FCWIN-0084 */

	HFC_EXIT("hfc_fx_del_port_id_resp");
		
	return;
}


/*
 * Function:    hfc_fx_mihlog_resp
 *
 * Purpose:     This routine deals with MIH_LOG response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_mihlog_resp(struct port_info *pp, struct core_info *core )
{
	uint                rc_passthrouh = 0xffffffff;
	uint                mb_resp_status;
	struct target_info_fx  *target=NULL;
	struct mailbox_fx	*mbox;
	uint				lun	=0;
	uint				TargetId	=0;								 /* FCWIN-0117 */
	struct hfc_pkt_fx	*hfcp_to = NULL;							 /* FCWIN-0153 */
	struct hfc_pkt_fx	*find_hfcp = NULL;
	struct hfc_pkt_fx	*hfcp_wk;
//	struct hfc_pkt_fx	*hfcp_top_p;
//	struct hfc_pkt_fx	*hfcp_end_p;
	struct hfc_pkt_fx	*hfcp_ad;
	struct dev_info_fx	*dev= NULL;									/* FCLNX-GPL-0343 *//* FCLNX-GPL-FX-014 */

	HFC_ENTRY("hfc_fx_mihlog_resp");

	mbox = core->mb;
	core->mb_status = 0;												/* FCLNX-0037 */

	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);	/* FCWIN-0170 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_MIHLGRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0);
	
	/* Check hfc_pkt_fx address */							/* FCLNX-GPL-0135 */
	hfcp_wk = (struct hfc_pkt_fx *)(ulong)mbox->mb_init.type.mih_log.driver_used_area;
	
	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_MIHLGRSP, rc_passthrouh);       /* FCWIN-0154 */
		}
		
		mb_resp_status = SCS_NO_DEV_RESP;								/* FCWIN-0151 */
																	/* FCWIN-0153 STR*/
		if( hfcp_wk != NULL ){
			TargetId = (uint)hfcp_wk->target_id;
			lun = (uint)hfcp_wk->lun_id;
		}
		break;

	case HFC_MBPASS_SUCCESS :
		/* Get SOFTLOG data successfully */
		mb_resp_status = 0 ;

		if( mbox->mb_resp.type.mih_log.mih_sbc != 0 ) /* No need for endian conversion */ 
		{	
			if( hfcp_wk != NULL ){
				hfc_fx_hand2_trace(
					HFC_FX_TRC_MIHLGRSP, 0x02, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0);
				hfcp_ad = hfcp_wk;
				hfcp_to = (struct hfc_pkt_fx *)hfcp_ad;
				TargetId = (uint)hfcp_wk->target_id;
				lun = (uint)hfcp_wk->lun_id;
				
				/* Does the target SCSI initiation for this MIH-LOG exist in SCSI response waiting queue? */
				if ((target = hfc_fx_hash_target_info(pp, TargetId)) != NULL) {
					find_hfcp = target->core_queue[core->core_no].we_que_top[lun % HASH_T_NUM];
				
					while( find_hfcp != NULL)
					{
						if ( find_hfcp == hfcp_to )
							break ;

						find_hfcp = find_hfcp->cmd_forw;
					}
				}
				
				if (find_hfcp) {
					/* Target SCSI initiation exists in SCSI response waiting queue */
					hfcp_to->tout_slog_ssn = mbox->mb_resp.type.mih_log.mih_slog ;
					HFC_2B_TO_2L(hfcp_to->tout_slog_sbc, mbox->mb_resp.type.mih_log.mih_sbc);
					set_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp_to->cmd_flags);
					HFC_DBGPRT( "hfc_fx_mihlog_resp - find hfcp\n" );
				}
				else {
					HFC_DBGPRT( "hfc_fx_mihlog_resp - not find hfcp\n" );
				}
			}else{																/* FCWIN-0153 END*/
				hfc_fx_hand2_trace(
					HFC_FX_TRC_MIHLGRSP, 0x03, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0);
				/* Get LINK INCIDENT LOG data successfully */
				memset((void *)core->logdata, 0, 16);
				if(pp->linknego_tmo_boot == 1){		/* FCLNX-GPL-FX-139 Start */
					hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKINCLOG, ERRID_HFCP_ERR6, 0x8b, core->logdata, 16 );
				}else{
					hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKINCLOG, ERRID_HFCP_EVNT3, 0x8c, core->logdata, 16 );
				}		/* FCLNX-GPL-FX-139 Start */
			}
		}
		break;

	default :
		break;
	}
	
	/* FCLNX-GPL-FX-014 Start */
	hfcp_wk = (struct hfc_pkt_fx *)(ulong)mbox->mb_init.type.mih_log.driver_used_area;
	if(hfcp_wk != NULL){
		dev = hfcp_wk->dev;
		if(((pp->abort_t_restrain)&&(pp->tgtrst_restrain))
		||((test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp_wk->cmd_flags))&&(test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 )))){
			HFC_DBGPRT( "hfc_fx_mihlog_resp - Issue Link Reset pp->status_detail2 = %08x.\n", pp->status_detail2 );
			set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
			hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
		}else if((target != NULL)&&(dev != NULL)&&(test_bit(HFC_DC_WAIT_MIHLOG, (ulong *)&dev->dev_core_stat.core[core->core_no]))){	/* FCLNX-GPL-FX-178 *//* FCLNX-GPL-FX-282, 288 */
			clear_bit(HFC_DC_WAIT_MIHLOG, (ulong *)&dev->dev_core_stat.core[core->core_no]);
			if ( (dev->dev_core_stat.all == 0)&&(test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat) )){	/* FCLNX-GPL-FX-178 */
				if((!test_bit(HFC_DS_FAIL_ABORT, (ulong *)&dev->lustat))&&(!test_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat))){
					// Issue AbortTaskSet and C_SCSI(wait stop DMA)
					HFC_DBGPRT( "hfc_fx_mihlog_resp - hfc_fx_issue_devrst_cscsi\n" );
					if (HFC_FX_MQ_VALID(pp)) {
						hfc_fx_issue_devrst_cscsi(
							pp->pport, pp->pport->target_arg[target->pseq], dev, (((0x00000001 << CFLAG_ABORT) | (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA))));
					}
					else {
						hfc_fx_issue_devrst_cscsi(
							pp, target, dev, (((0x00000001 << CFLAG_ABORT) | (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA))));
					}
				}
			}
		}else if((target != NULL)&&(test_bit(HFC_TC_WAIT_MIHLOG, (ulong *)&target->tgt_core_stat.core[core->core_no]))){	/* FCLNX-GPL-FX-178 */
			clear_bit(HFC_TC_WAIT_MIHLOG, (ulong *)&target->tgt_core_stat.core[core->core_no]);
			if (HFC_FX_MQ_VALID(pp)) {
				if (pp->pport->target_arg[target->pseq]->tgt_core_stat.all == 0){
					if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&pp->pport->target_arg[target->pseq]->status)){
						HFC_DBGPRT( "hfc_fx_mihlog_resp - hfc_fx_issue_tgtrst_cscsi\n" );
						hfc_fx_issue_tgtrst_cscsi(pp->pport, pp->pport->target_arg[target->pseq], 
							pp->pport->target_arg[target->pseq]->dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA));	/* FCLNX-GPL-FX-112 */
					}
				}
			}
			else {
				if (target->tgt_core_stat.all == 0){
					if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status)){
						HFC_DBGPRT( "hfc_fx_mihlog_resp - hfc_fx_issue_tgtrst_cscsi\n" );
						hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA));	/* FCLNX-GPL-FX-112 */
					}
				}
			}
		}else if(test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp_wk->cmd_flags)){
			if(test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp_wk->cmd_flags )){	/* FCLNX-GPL-FX-320 Start */
				HFC_DBGPRT( "hfc_fx_mihlog_resp - Issue Link Reset.\n" );
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			}else{
				HFC_DBGPRT( "hfc_fx_mihlog_resp - Output Error log.\n" );
				hfc_fx_errlog( pp, core, hfcp_wk->target, hfcp_wk, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, 0x29, hfcp_wk->core->logdata, 16 ); /* FCLNX-GPL-FX-091 */
				clear_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp_wk->cmd_flags);
				hfc_fx_deque_we_que(pp, hfcp_wk->target, hfcp_wk);
				clear_bit(HFC_TC_WAIT_TGTRST, (ulong *)&hfcp_wk->target->tgt_core_stat.core[core->core_no]);
			}		/* FCLNX-GPL-FX-320 End */
		}else if(test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp_wk->cmd_flags)){		/* FCLNX-GPL-FX-320 Start */
			HFC_DBGPRT( "hfc_fx_mihlog_resp - Output Error log.\n" );
			hfc_fx_errlog( pp, core, hfcp_wk->target, hfcp_wk, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, 0x26, hfcp_wk->core->logdata, 16 ); /* FCLNX-GPL-FX-091 */
			clear_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp_wk->cmd_flags);
			hfc_fx_deque_we_que(pp, hfcp_wk->target, hfcp_wk);
			clear_bit(HFC_DC_WAIT_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);
		}		/* FCLNX-GPL-FX-320 End */
	}
	/* FCLNX-GPL-FX-014 End */
	
	clear_bit(HFC_PD_WAIT_MIHLOG, (ulong *)&pp->status_detail2);	/* FCLNX-0506 */
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);

	unlock_fx_mailbox(pp);

	
	HFC_EXIT("hfc_fx_mihlog_resp");
	
	return;
}


/*
 * Function:    hfc_fx_load_ch_trace_log_resp
 *
 * Purpose:     This routine deals with Load_CH_Trace_Log response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_load_ch_trace_log_resp(struct port_info *pp, struct core_info *core )
{
	uint                 rc_passthrouh = 0xffffffff;
	uint                 mb_resp_status = 0;
	struct mailbox_fx	 *mbox = core->mb;
	
	HFC_ENTRY("hfc_fx_delete_port_id_resp");
	
	core->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);		/* FCWIN-0170 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LDCH_TRCLOGRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0);
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_LDCH_TRCLOGRSP, rc_passthrouh);
		}
		
		HFC_DBGPRT("load_ch_trace_log_resp, fatal %d\n",rc_passthrouh);

		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("load_ch_trace_log_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		clear_bit(HFC_PD_WAIT_LOAD_CH_TRACE, (ulong *)&pp->status_detail2);
//		set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
		break;
	default :
		break;
	}
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	unlock_fx_mailbox(pp);															/* FCWIN-0084 */

	HFC_EXIT("hfc_fx_load_ch_trace_log_resp");
		
	return;
}


/*
 * Function:    hfc_fx_cancel_scsi_resp
 *
 * Purpose:     This routine deals with Cancel SCSI response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_cancel_scsi_resp(struct port_info *pp, struct core_info *core )
{
	uint                rc_passthrouh = 0xffffffff;
	struct region_info	*rp=NULL;
	struct target_info_fx  *target=NULL;
	struct mailbox_fx	*mbox;
	struct core_info 	*core_wk=NULL;
	uint				i=0, wait_mb_core=0, did=0;					/* FCLNX-GPL-FX-112 */
	uint				mb_code=0;					/* FCWIN-0117 *//* FCLNX-GPL-FX-112 */
	short lun_id=0;
//	uint64_t	hfcp_ad;
	struct dev_info_fx		*dev= NULL;									/* FCLNX-GPL-0343 */

	HFC_ENTRY("hfc_fx_cancel_scsi_resp");

//	HFC_DBGPRT("hfc_fx_cancel_scsi_resp");

	mbox = core->mb;
	core->mb_status = 0;												/* FCLNX-0037 */
	rp = pp->region_arg[pp->rid];

	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_CSCSIRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0);
	
	HFC_4B_TO_4L(mb_code, mbox->mb_resp.mb_code);
	
	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_CSCSIRSP, rc_passthrouh);       /* FCWIN-0154 */
		}
		
																	/* FCWIN-0153 STR*/
		break;

	case HFC_MBPASS_SUCCESS :
		break;

	default :
		break;
	}
	
	HFC_DBGPRT("clear_bit HFC_CS_WAIT_MAILBOX core_no%d", core->core_no);
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	if (rc_passthrouh != HFC_MBPASS_SUCCESS) {
		/* Issue Link Reset */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CSCSIRSP, 0x02, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - Issue Link Reset.\n");
		set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
		return;
	}
	
	/* check if core_start of all cores finished or not. */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		core_wk = pp->region_arg[pp->rid]->core_arg[i];
		if ( core_wk == NULL
			 || core_wk == core
			 || hfc_fx_check_cs_disable(pp, core_wk) )	/* FCLNX-GPL-FX-438 */
			continue;
		if ( test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core_wk->status) ){
			wait_mb_core = 1;
			break;
		}
	}

	
	/* core_start of other cores have not finished yet. */
	if (wait_mb_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CSCSIRSP, 0x03, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		HFC_DBGPRT("core_start of other cores have not finished yet.core_no=%x",core->core_no);
		return;
	}
	
	did = ( (uint) hfc_fx_read_val( mbox -> mb_init.type.cscsi.fcph_hdr.d_id[0] ) << 16 ) +
		  ( (uint) hfc_fx_read_val( mbox -> mb_init.type.cscsi.fcph_hdr.d_id[1] ) << 8 ) +
			(uint) hfc_fx_read_val( mbox -> mb_init.type.cscsi.fcph_hdr.d_id[2] );
			
	HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - did = %08x.\n",did);
			
	for(i=0; i<MAX_TARGET_PROBE; i++)										/* FC-GW */
	{
		target = pp->target_arg[i];
		if ((target != NULL)&&(target->scsi_id == did)){
			break;
		}
	}
	
	if(target != NULL){
		HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - find taget.\n");
		if(mbox -> mb_init.type.cscsi.cnexus == HFC_CANCEL_ITLNEXUS){
			HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - cancel nexus = I-T-L Nexus.\n");
			HFC_2B_TO_2L(lun_id,  mbox -> mb_init.type.cscsi.fcp_lun.lun)
			dev = (struct dev_info_fx *)hfc_fx_search_dev_info( target, lun_id );			/* FCLNX-GPL-0343 */
			if(dev != NULL){
				HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - find devt.\n");
				hfc_fx_hand2_trace(
					HFC_FX_TRC_CSCSIRSP, 0x20, pp, core->rp, core, target, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				clear_bit(HFC_DS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core_wk = rp->core_arg[i]) == NULL)
						continue;
					/* SoftLo Errlog output */
					hfc_fx_notify_tout(pp, core_wk, target, dev->lun, HFC_FLASH_DEV);	/* FCLNX-GPL-596 */
					hfc_fx_cancel_weque(pp, core_wk, target, dev->lun, NULL, SCS_CMD_ABORTED, 0, HFC_FLASH_DEV);
				}
				if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
					hfc_fx_mq_cancel_scsi_cmd(pp, target, dev->lun, NULL, SCS_CMD_ABORTED, 0,
						FALSE, FALSE, FALSE, TRUE, FALSE, HFC_FLASH_DEV);
				}
			}
		}
		else if(mbox -> mb_init.type.cscsi.cnexus == HFC_CANCEL_ITNEXUS){
			HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - cancel nexus = I-T Nexus.\n");
			hfc_fx_hand2_trace(
				HFC_FX_TRC_CSCSIRSP, 0x22, pp, core->rp, core, target, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			clear_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
			hfc_fx_cancel_xrb(pp, target, HFC_FLASH_TARGET);	/* FCLNX-GPL-FX-228 */
			if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {	/* FCLNX-GPL-FX-274, 285 */
				hfc_fx_mq_cancel_xrb(pp, target, HFC_FLASH_TARGET);
			}
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = rp->core_arg[i]) == NULL)
					continue;
				/* SoftLo Errlog output */
				hfc_fx_watchdog_enter(pp, core_wk, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);	/* FCLNX-GPL-FX-190 */
				clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);							/* FCLNX-GPL-FX-190 */
				hfc_fx_notify_tout(pp, core_wk, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */
				hfc_fx_cancel_weque(pp, core_wk, target, 0, NULL, SCS_CMD_RESET, HFC_CSCSI_ERROR, HFC_FLASH_TARGET);	/* FCLNX-GPL-FX-112 */
				dev = target->dev;
				while(dev != NULL) {			/* FCLNX-GPL-FX-190 Start */
					dev->lustat = 0x00;
					dev->dev_core_stat.all = 0;
					hfc_fx_mp_watchdog_enter(pp, core_wk, target, NULL, dev, dev->lun, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);
					dev = dev->next;
				}								/* FCLNX-GPL-FX-190 End */
			}
			if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
				hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_CMD_ABORTED, HFC_CSCSI_ERROR,
					FALSE, FALSE, FALSE, TRUE, FALSE, HFC_FLASH_TARGET);
			}
		}
	}
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	unlock_fx_mailbox(pp);
	
	HFC_EXIT("hfc_fx_cancel_scsi_resp");
	
	return;
}


/*
 * Function:    hfc_fx_core_start_resp
 *
 * Purpose:     This routine deals with core_start response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_core_start_resp(struct port_info *pp, struct core_info *core )
{
	uint                rc_passthrouh = 0xffffffff;
	uint                mb_resp_status = 0;
	struct mailbox_fx	*mbox = core->mb;
//	uchar               config_chk_flg = 0;
	uchar				available_pcore = 0;
	uchar				wait_mb_core = 0;
	int					i;
	
	HFC_ENTRY("hfc_fx_core_start_resp");
//	HFC_DBGPRT("*hfcldd : entry %s()\n)","hfc_fx_core_start_resp");
	
	if(core == NULL){
		HFC_DBGPRT("*hfcldd : hfc_fx_core_start_resp - core == NULL");
		return;
	}
	
	core->mb_status = 0;
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_CORESTARTRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0);
	
	if(mbox == NULL){
		HFC_DBGPRT("*hfcldd : hfc_fx_core_start_resp - mbox == NULL");
		return;
	}
	
	if(core->logdata == NULL){
		HFC_DBGPRT("*hfcldd : hfc_fx_core_start_resp - core->logdata == NULL");
		return;
	}
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
//			HFC_DBGPRT("core_start_resp, HFC_MBPASS_WAIT_RETRY %d \n",rc_passthrouh);
			return;
		case HFC_MBPASS_TIMEDOUT :
//			HFC_DBGPRT("core_start_resp, HFC_MBPASS_TIMEDOUT %d \n",rc_passthrouh);
			unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_CORESTARTRSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;
//			HFC_DBGPRT("core_start_resp, SCS_NO_DEV_RESP %d \n",rc_passthrouh);
			break;

		case HFC_MBPASS_SUCCESS :
//			HFC_DBGPRT("core_start_resp, success %d \n",rc_passthrouh);
			mb_resp_status = 0 ;
			break;
		default :
//			HFC_DBGPRT("core_start_resp, default %d \n",rc_passthrouh);
			break;
	}
	
//	HFC_DBGPRT("clear_bit HFC_CS_WAIT_MAILBOX core_no%d", core->core_no);
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	if (mb_resp_status != 0) {
		/* Forced-Checkstop to the core and Logout */
		HFC_DBGPRT("Forced-Checkstop to the core and Logout");
		hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
	}
	
	/* check if core_start of all cores finished or not. */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */
		if (test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status) ) {
			wait_mb_core = 1;
			break;
		}
		if (core->mb_results == HFC_MBPASS_SUCCESS) {
			available_pcore |= (0x80 >> core->pcore_no);
		}
	}
	
	/* core_start of other cores have not finished yet. */
	if (wait_mb_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CORESTARTRSP, 0x01, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		HFC_DBGPRT("core_start of other cores have not finished yet.core_no=%x",core->core_no);
		return;
	}
	
	clear_bit(HFC_PD_WAIT_CORE_START, (ulong *)&pp->status_detail1);
	
	/* Core_start of all cores have finished. */
	unlock_fx_mailbox(pp);

	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR); // STOP mailbox retry timer per port

	if (available_pcore == 0) {
		/* core_start of all cores failed. -> checkstop. */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CORESTARTRSP, 0x02, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
//		hfc_fx_hw_chk_stop(pp); 	/* TBD */
		set_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status);
		set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->pport->status_detail2);
		hfc_fx_change_vport_isol_state(pp);
		HFC_DBGPRT("core_start of all cores failed. -> checkstop.");
		
		if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) ){		/* FCNLX-GPL-FX-126 */
			HFC_DBGPRT("hfc_fx_core_start_resp @ HFC_MB_PROL\n");
			hfc_fx_wake_up(&pp->mb_event, &(pp->mb_event_wait));
		}														/* FCNLX-GPL-FX-126 */
		return;	
	}
	
	if (pp->available_pcore != available_pcore) {
		/* core_start of some cores failed. */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CORESTARTRSP, 0x03, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		
		/* Updates init_table of master and available cores. */
		/* Determine master core */
		hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);

		/* Set fw_init_tbl */
		hfc_fx_set_fw_init_tbl(pp);
		
		set_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1);	/* FCLNX-GPL-FX-414 */
		
		/* check the results of all core_start */
		if (hfc_fx_all_core_start(pp) != 0) {
			hfc_fx_hand2_trace(
				HFC_FX_TRC_CORESTARTRSP, 0x04, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			/* all core_start failed. */
//			hfc_fx_hw_chk_stop(pp);		/* TBD */
		}
		HFC_DBGPRT("core_start of some cores failed. available_pcore=0x%02x",available_pcore);	/* FCLNX-GPL-FX-414 */
		if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) ){		/* FCNLX-GPL-FX-126 */
			HFC_DBGPRT("hfc_fx_core_start_resp @ HFC_MB_PROL\n");
			hfc_fx_wake_up(&pp->mb_event, &(pp->mb_event_wait));
		}														/* FCNLX-GPL-FX-126 */
		return;
	} else {
		/* all cores are available */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CORESTARTRSP, 0x05, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );

		set_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1);
		if (!( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
		|| (pp->after_isolrec)
		|| (pp->mck_linkup))
			atomic_set(&pp->check_mbreq, 1);	/* FCLXN-GPL-FX-462 */
		if(pp->initialize != 0){
			if (!( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
			|| (!pp->after_isolrec))
				hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);	/* FCLXN-GPL-FX-462 */
			HFC_DBGPRT("core_start - wakeup");
		}
		else{
			HFC_DBGPRT("core_start - atomic_set check_mbreq");
		}
	}
	
	if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) ){			/* FCNLX-GPL-FX-126 */
		HFC_DBGPRT("hfc_fx_core_start_resp @ HFC_MB_PROL\n");
		clear_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1);
		hfc_fx_wake_up(&pp->mb_event, &(pp->mb_event_wait));
	}															/* FCNLX-GPL-FX-126 */
	
	HFC_EXIT("hfc_fx_core_start_resp");
//	HFC_DBGPRT("*hfcldd : exit  %s()\n)","hfc_fx_core_start_resp");

	return;
}

/*
 * Function:    hfc_fx_offline_mb_resp
 *
 * Purpose:     This routine deals with Offline_mb response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_offline_mb_resp(struct port_info *pp, struct core_info *core )
{
	uint                 rc_passthrouh = 0xffffffff;
	uint                 mb_resp_status = 0;
	struct core_info     *core_wk;
	struct mailbox_fx	 *mbox = core->mb;
	int i=0,wait_mb_core=0;
	
	HFC_ENTRY("hfc_fx_offline_mb_resp");
	
	core->mb_status = 0;
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_OFFLINEMBRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_OFFLINEMBRSP, rc_passthrouh);
		}
		
		HFC_DBGPRT("offline_mb_resp, fatal %d\n",rc_passthrouh);
		clear_bit(HFC_PD_WAIT_OFFLINE_MB, (ulong *)&pp->status_detail2);

		clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);	/* FCLNX-GPL-FX-062 */

		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("offline_mb_resp, success %d \n",rc_passthrouh);
		clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
		mb_resp_status = 0 ;
		break;
	default :
		break;
	}
	
	/* check if whether all the cores finish offline_mb or not */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		core_wk = pp->region_arg[pp->rid]->core_arg[i];
		if ( core_wk == NULL
			 || core_wk == core
			 || hfc_fx_check_cs_disable(pp, core_wk) )	/* FCLNX-GPL-FX-438 */
			continue;
		if ( test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core_wk->status) ){
			wait_mb_core = 1;
			break;
		}
	}
	
	/* other cores are executing mailbox */
	if (wait_mb_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_OFFLINEMBRSP, 0x01, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		return;
	}

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	/* all the cores finished to issue offline_mb */
	clear_bit(HFC_PD_WAIT_OFFLINE_MB, (ulong *)&pp->status_detail2);
	clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	if(pp->initialize != 0){
		hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
	}
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_offline_mb_resp");
		
	return;
}

/* FCLNX-GPL-FX-126 */
/*
 * Function:    hfc_fx_diag_resp
 *
 * Purpose:     This routine deals with Daig Mailbox response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_diag_resp(struct port_info *pp, struct core_info *core )
{
	uint				rc_passthrouh = 0xffffffff;
	uint				mb_resp_status = 0;
	struct core_info	*core_wk;
	struct mailbox_fx	*mbox = core->mb;
	uint				i;
	uchar				failed_master_core = 0, failed_slave_core = 0;
	uchar				wait_mb_core = 0, tmp_linkdown_occurred = 0;
	
	HFC_ENTRY("hfc_fx_diag_resp");
	
	core->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);		/* FCWIN-0170 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_DIAGRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			)&&
			(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */
				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_DIAGRSP, rc_passthrouh);
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
		}
		
		if(pp->linkdown_occurred == 1) tmp_linkdown_occurred = 1;	/* FCLNX-GPL-FX-174 */
		HFC_DBGPRT("add_port_id_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;

		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("add_port_id_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		
		break;
	default :
		break;
	}
	
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);

	/* Copy result information to internal DIAG area */
	pp->diag->uni.post.core_result[core->core_no] = mbox->mb_resp.fsb;
	/* Set error code */
	pp->diag->uni.post.core_err_code[core->core_no][0] = mbox->mb_resp.err_code[0];
	pp->diag->uni.post.core_err_code[core->core_no][1] = mbox->mb_resp.err_code[1];
	pp->diag->uni.post.core_err_code[core->core_no][2] = mbox->mb_resp.err_code[2];
	
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	/* check if whether all the cores finish diag or not */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		core_wk = pp->region_arg[pp->rid]->core_arg[i];
		if ( core_wk == NULL
			 || core_wk == core
			 || hfc_fx_check_cs_disable(pp, core_wk) )	/* FCLNX-GPL-FX-438 */
			continue;
		if ( test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core_wk->status) ){
			wait_mb_core = 1;
			break;
		}
		if (core->mb_results != HFC_MBPASS_SUCCESS) {
		    if (core->core_no == pp->master_core_no) {
				failed_master_core = 1;
			} else {
				failed_slave_core = 1;
			}
		}
	}
	
	/* other cores are executing mailbox */
	if (wait_mb_core) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_DIAGRSP, 0x01, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		return;
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	
	/* all the cores finished add port_id */

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	/* Issue SCR with master core */
	hfc_fx_hand2_trace(
		HFC_FX_TRC_DIAGRSP, 0x10, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	HFC_DBGPRT("hfc_fx_diag_resp @ HFC_MB_PROL\n");
	if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) ){
		hfc_fx_wake_up(&pp->mb_event, &(pp->mb_event_wait));
	}
	pp->mb_prol_wake_up_time = (uint)jiffies; /* FCLNX-GPL-243 */
	
	return;
}
/* FCLNX-GPL-FX-126 */


/*
 * Function:    hfc_fx_link_resp
 *
 * Purpose:     This routine deals with Link initialize response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_link_resp(struct port_info *pp, struct core_info *core )
{
	uint				rc_passthrouh = 0xffffffff;
	uint				mb_resp_status = 0, i, count, status=0;	/* FCLNX-GPL-FX-407 */
	struct mailbox_fx	*mbox = core->mb;
	uchar				config_chk_flg = 0;
	uchar				flag, loop_direct=1, domain=0;	/* FCLNX-GPL-FX-386 */
	
	HFC_ENTRY("hfc_fx_link_resp");
	
	HFC_DBGPRT("hfcldd%d: hfc_fx_link_resp start \n", pp->dev_minor);
	
	core->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);		/* FCWIN-0170 */
																	/* @MLPF STR */
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LINKRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) )
	{
		hfc_fx_mlpf_cca_setup(pp);		/* FCLNX-GPL-494 */
	}
																	/* @MLPF END */
	if(rc_passthrouh == HFC_MBPASS_SUCCESS){
		rc_passthrouh = hfc_fx_check_linkresp_param(pp, core);
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LINKRSP, 0x01, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
	}
	
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			if ( ( core->mb_results & 0x00ffffff ) == 0xE01001 ){
				hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_OPTERR0, 0x9c,core->logdata,16) ;
				set_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status);
				set_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)&pp->pport->status_detail2);
			}
			else if ( ( core->mb_results & 0x00ffffff ) == 0xE11002 ){
				hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_ERR2, 0x9d,core->logdata,16) ;	/* FCLNX-GPL-FX-194 */
				set_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status);
				set_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->pport->status_detail2);
			}
			else if ( ( core->mb_results & 0x00ffffff ) == 0xE11003 ){
				hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_ERR2, 0x9e,core->logdata,16) ;	/* FCLNX-GPL-FX-194 */
				set_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status);
				set_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->pport->status_detail2);
			}
			else if ( ( core->mb_results & 0x00ffffff ) == 0xE11004 ){
				hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_ERR2, 0x9f,core->logdata,16) ;	/* FCLNX-GPL-FX-194 */
				set_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status);
				set_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)&pp->pport->status_detail2);
			}
			else if (!(core->mb_results == 0x02FF1004))
			{
				if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {	/* FCLNX-GPL-FX-219 */
					if (!(core->mb_results == 0x02ff1011)) {
						HFC_DBGPRT("link_resp, error  %d\n",rc_passthrouh);
						hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_LINKRSP, rc_passthrouh);
					}
				}
				else {
					HFC_DBGPRT("link_resp, error  %d\n",rc_passthrouh);
					hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_LINKRSP, rc_passthrouh);
				}
			}	/* FCLNX-GPL-FX-246 */
			
			hfc_fx_change_vport_isol_state(pp);
		}
		
//		HFC_DBGPRT("link_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;
		
		
		if ( ( HFC_FX_MMODE_CHECK_SHADOW(pp) ) && ( config_chk_flg == 0 ) ){
			if(test_bit(HFC_PS_ISOL, (ulong *)&pp->status)){	/* FCLNX-GPL-489 */
				if(test_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->status_detail2)){
					hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_SFP_FAIL, HFC_ENABLE_LPAR_STATE);
				}else if(test_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)&pp->status_detail2)){
					hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_SFP_NOTSUPT, HFC_ENABLE_LPAR_STATE);
				}else if(test_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)&pp->status_detail2)){
					hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_SFP_DOWN, HFC_ENABLE_LPAR_STATE);
				}
			}							/* FCLNX-GPL-489 */
		}
																	/* @MLPF END */
		break;

	case HFC_MBPASS_SUCCESS :
	
//		HFC_DBGPRT("link_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		set_bit(HFC_PS_CONNECTED, (ulong *)&pp->status);
		
		if( HFC_FX_MMODE_CHECK_SHARED(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) ) ) 
			core->fw_init_p->fw_iocinfo.connect_type = hfc_fx_read_val( core->mb->mb_resp.type.linkini.connect_type );
		else
			core->fw_init_p->fw_iocinfo.connect_type = hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type );
		
		core->fw_init_p->fw_iocinfo.trans_rate = hfc_fx_read_val( core->mb->mb_resp.type.linkini.trans_rate );
		
		core->fw_init_p->fw_iocinfo.configure_flag = hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_config_flag );
		flag = core->fw_init_p->fw_iocinfo.configure_flag;
		
//		HFC_DBGPRT("hfcldd%d: hfc_fx_link_resp connect_type = %d trans_rate = %d conf_flag = %02x\n", 
//			pp->dev_minor, core->fw_init_p->fw_iocinfo.connect_type, core->fw_init_p->fw_iocinfo.trans_rate,
//			flag);

		switch( core->fw_init_p->fw_iocinfo.connect_type ){
			case HFC_FX_AL :
			case HFC_FX_MULTI_ALPA:
				core->fw_init_p->fw_iocinfo.alpa_count = hfc_fx_read_val( core->mb->mb_resp.type.linkini.alpa_count );
				if( flag & HFC_FX_POSMAP_VALID ){
					count = (uint) hfc_fx_read_val(core->mb->mb_resp.type.linkini.position_map[0]);
//					core->fw_init_p->fw_iocinfo.assign_alpa = hfc_fx_read_val( core->mb->mb_resp.type.linkini.assign_alpa );
					if( count > 0){
						core->fw_init_p->pos_map[0] = core->mb->mb_resp.type.linkini.position_map[0];
						for (i=1;i<=count; i++) {
							core->fw_init_p->pos_map[i] = core->mb->mb_resp.type.linkini.position_map[i];
							if( core->fw_init_p->pos_map[i] == 0x00 ){
								loop_direct = 0;
							}
						}
					}
				}
				if( flag & HFC_FX_ALPA_VALID ){
					core->fw_init_p->fw_iocinfo.assign_alpa = hfc_fx_read_val(core->mb->mb_resp.type.linkini.assign_alpa );
					pp->scsi_id = (uint64_t)hfc_fx_read_val(core->mb->mb_resp.type.linkini.assign_alpa );

					core->fw_init_p->fw_iocinfo.self_port_id[0] = 0;
					core->fw_init_p->fw_iocinfo.self_port_id[1] = 0;
					core->fw_init_p->fw_iocinfo.self_port_id[2] = (uchar)hfc_fx_read_val(core->mb->mb_resp.type.linkini.assign_alpa );
					core->fw_init_p->fw_iocinfo.configure_flag |= HFC_FX_PID_VALID;
				}
				if( core->fw_init_p->fw_iocinfo.connect_type == HFC_MULTI_ALPA ){
					count = core->mb->mb_resp.type.linkini.acquired_alpa[0];
					if( count > 0){
						core->fw_init_p->active_alpa[0] = core->mb->mb_resp.type.linkini.acquired_alpa[0];
						for (i=1;i<=count; i++) {
							core->fw_init_p->active_alpa[i] = core->mb->mb_resp.type.linkini.acquired_alpa[i];
						}
					}
				}
				
				if( loop_direct == 1 ){	/* Loop Direct connection with I/O */
					pp->switch_exist = 0;
				}
				else{					/* Loop connection with Switch, AL_PA of switch : 0x00 */
					pp->switch_exist = HFC_SWITCH_EXIST;
				}
				break;
			

			default:
				loop_direct = 0;
				pp->switch_exist = HFC_SWITCH_EXIST;
				break;

		}
		
		break;
	default :
		break;
	}
	
	
	/* F/W init table information is stored in port_info */

//	HFC_DBGPRT("hfcldd : hfcl_intr - hfc_fx_link_resp copy iocinfo port_info = %lx\n",(ulong)pp);

	hfc_fx_copy_iocinfo(pp, core);
	if (mb_resp_status == 0) {
		if( loop_direct == 1 )
		{
			/* Loop Direct connection with I/O */
			HFC_DBGPRT("hfcldd%d hfc_fx_link_resp Loop Direct connection\n",pp->dev_minor);
			hfc_fx_copy_master_to_slave( pp, core );
			hfc_fx_change_portstat_linkup(pp, core);
			hfc_fx_hand2_trace(
				HFC_FX_TRC_LINKRSP, 0x11, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			
			if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
				if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
					if(pp->initialize != 0){
						hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
					}
				}
				else {
					hfc_fx_wwnverify_linkup(pp, NULL, core, mb_resp_status, 0);
				}
			}
			else {
				if(pp->initialize != 0){
					hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
				}
			}
		}
		else{
			if( pp->connect_type == HFC_FX_F_PORT ){
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LINKRSP, 0x12, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				HFC_DBGPRT("hfcldd : hfcl_intr - hfc_fx_link_resp F_PORT\n");
				
				if( (HFC_FX_MMODE_CHECK_SHARED(pp)) && (!HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){	/* FCLNX-GPL-FX-386 Start */
					pp->scsi_id = 0;
					domain = (uchar)pp->rid + 0x80;
					pp->scsi_id |= (uint64_t)(domain << 16);
					
					/* Set p2p_tgt_port_id of Init_Table */
					core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] = 0x01;
					core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] = 0;
					core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] = 0;
					
					/* Set self_port_id of Init_Table */
					core->fw_init_p->fw_iocinfo.self_port_id[0] = domain;
					core->fw_init_p->fw_iocinfo.self_port_id[1] = 0;
					core->fw_init_p->fw_iocinfo.self_port_id[2] = 0;
					
					hfc_fx_copy_master_to_slave( pp, core );
					hfc_fx_change_portstat_linkup(pp, core);
					hfc_fx_wwnverify_linkup(pp, NULL, core, mb_resp_status, 0);
				}	/* FCLNX-GPL-FX-386 End */
				
				if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {	/* FCLNX-GPL-FX-247,272 */
					atomic_set(&pp->check_mbreq, 0);
					hfc_fx_change_portstat_linkup(pp, core);
					if(pp->initialize != 0){
						hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
					}
					HFC_DBGPRT("hfcldd : hfcl_intr - hfc_fx_link_resp F_PORT for MQ\n");
				}	/* FCLNX-GPL-FX-247,272 */
			}
			else if( pp->connect_type == HFC_FX_MULTI_ALPA ){	/* FCLNX-GPL-FX-171 Start */
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LINKRSP, 0x15, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD1, NULL, 0) ;
				
				if(HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-407 */
					status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);	/* FCLNX-GPL-FX-407 */
					status &= ~HFC_HG_LPRSTATUS_LINKDOWN;	/* FCLNX-GPL-FX-407 */
					status |= (HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_FCSW | HFC_HG_LPRDETAIL_MULTIALPA);	/* FCLNX-GPL-FX-407 */
					if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
						status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
					hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );	/* FCLNX-GPL-FX-407 */
				}	/* FCLNX-GPL-FX-407 */
			
				hfc_fx_initialize_failed(pp, core, NULL);
				
				hfc_fx_copy_master_to_slave( pp, core );
				hfc_fx_change_portstat_linkdown(pp, core);
		
				hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);

				if(pp->initialize != 0){
					hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
				}
			}	/* FCLNX-GPL-FX-171 End */
			else{
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LINKRSP, 0x13, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				HFC_DBGPRT("hfcldd%d hfc_fx_link_resp HFC_NEED_FLOGI\n",pp->dev_minor);
				if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
					atomic_set(&pp->check_mbreq, 0);
					hfc_fx_change_portstat_linkup(pp, core);
					if(pp->initialize != 0){
						hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
					}
				}
				else {
					set_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp -> status_detail1 );
					atomic_set(&pp->check_mbreq, 1);
				}
			}
		}
	}
	else {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LINKRSP, 0x14, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		hfc_fx_initialize_failed(pp, core, NULL);
		
		hfc_fx_copy_master_to_slave( pp, core );
		hfc_fx_change_portstat_linkdown(pp, core);
		
		hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);

		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
//	HFC_DBGPRT("hfcldd : hfc_fx_link_resp port_info = %lx\n",(ulong)pp);

	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_link_resp");
		
	return;
}

int hfc_fx_check_linkresp_param(struct port_info *pp, struct core_info *core){
	
	uchar retry_linkini=FALSE;
	
	if(hfc_fx_read_val( core->mb->mb_resp.type.linkini.position_map[0] ) & HFC_FX_FABRIC_VALID){
		pp->switch_exist = HFC_SWITCH_EXIST;
	}else{
		pp->switch_exist = 0;
	}
	
	switch(hfc_fx_read_val( core->mb->mb_resp.type.linkini.trans_rate )){
		case	HFC_FX_TRUNKN	: case	HFC_FX_1GBPS	:  
		case	HFC_FX_10GBPS	: case	HFC_FX_40GBPS	:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - trans_rate0 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.trans_rate ));
			break;
		case	HFC_FX_4GBPS	: case	HFC_FX_8GBPS	: case	HFC_FX_16GBPS	: case	HFC_FX_2GBPS	:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - trans_rate1 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.trans_rate ));
			break;
		default:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - trans_rate2 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.trans_rate ));
			hfc_fx_abend( pp, core, HFC_ABEND_MB_RSPERR );
			return HFC_MB_FATAL;
	}
	
	switch(hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type )){
		case	HFC_FX_CTUNKN	:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - con_type0 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type ));
			break;
		case	HFC_FX_PT2PT	:
		case	HFC_FX_SWITCH	:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - con_type1 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type ));
			break;
		case	HFC_FX_AL		:
		case	HFC_FX_MULTI_ALPA	:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - con_type2 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type ));
			if((!hfc_fx_read_val( core->mb->mb_resp.type.linkini.alpa_count ))
			||(!hfc_fx_read_val( core->mb->mb_resp.type.linkini.position_map[0] ))){
				retry_linkini = TRUE;
			}
			break;
		case	HFC_FX_F_PORT	:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - con_type3 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type ));
			break;
		default:
			HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - con_type4 = %llx\n", pp->dev_minor, hfc_fx_read_val( core->mb->mb_resp.type.linkini.link_con_type ));
			hfc_fx_abend( pp, core, HFC_ABEND_MB_RSPERR );
			return HFC_MB_FATAL;
	}
	
	if( retry_linkini != FALSE ){
		if((pp->mb_timer[core->mb_retry_tid].retry & HFC_FX_MBRTY_POLICY_CNT) ){
			/* retry if it is less than retry count. */
			if (core->mb_retry_cnt == 0) {			/* retry out	*/
				HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - HFC_MBPASS_RETRY_OVER2\n", pp->dev_minor);
				return HFC_MBPASS_RETRY_OVER ;	/* error end */
			}
		} else {
			/* retry if it is less than retry continuation time. */
			if ( !test_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1) ) {
				HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - HFC_MBPASS_RETRY_OVER1\n", pp->dev_minor);
				/* retry out since the end of a retry continuation timer is carried out */
				return HFC_MBPASS_RETRY_OVER ;	/* error end */
			}
		}
		core->mb_retry_cnt--;
		core->mb_callback = HFC_MB_INTL;
		hfc_fx_w_stop(pp, core, HFC_FX_MB_RETRY_DELAY_TMR);
		hfc_fx_w_start(pp, core, HFC_FX_MB_RETRY_DELAY_TMR, 1);
		set_bit(HFC_CS_MB_RETRY_DELAY, (ulong *)&core->status);	/* FCLNX-GPL-FX-161 */
		HFC_DBGPRT("hfcldd%d: hfc_fx_check_linkresp_param - HFC_MBPASS_WAIT_RETRY\n", pp->dev_minor);
		return HFC_MBPASS_WAIT_RETRY ;	/* error end */
	}
	return HFC_MBPASS_SUCCESS;

}


/*
 * Function:    hfc_fx_flogi_resp
 *
 * Purpose:     This routine deals with FLOGI response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_flogi_resp(struct port_info *pp, struct core_info *core )
{
	uint				rc_passthrouh = 0xffffffff;
	uint				mb_resp_status = 0;
	struct mailbox_fx	*mbox = core->mb;
//	uchar				flogi_config_flag=0;
	uint64_t			ww_name, node_name;
	uint				port_id, need_plogi_tgt_exist=0, tmp_linkdown_occurred = 0;	/* FCLNX-GPL-FX-118 */
	struct target_info_fx	*target=NULL;
	
	HFC_ENTRY("hfc_fx_flogi_resp");

	core->mb_status = 0;												/* FCLNX-0037 */
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);		/* FCWIN-0170 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FLOGIRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_resp rc_passthrouh = %08x\n", pp->dev_minor, rc_passthrouh);
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		if((pp->switch_exist == 0)&&(pp->connect_type == HFC_FX_PT2PT)){
			if((pp->target_arg[0] != NULL)&&(test_bit(HFC_TS_NEED_PLOGI, (ulong *)&pp->target_arg[0]->status))){
				need_plogi_tgt_exist=1;
			}
			if(((test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)) && (!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)))
			||(test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)&&(test_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1)))
			||(test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)&&(need_plogi_tgt_exist == 1))){	/* FCLNX-GPL-FX-047 *//* FCLNX-GPL-FX-118 */
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				clear_bit(HFC_PD_WAIT_FLOGI, (ulong *)&pp->status_detail1);
				pp->flogi_retry_change = 0;				/* FCLNX-GPL-FX-179 */
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR); /* FCLNX-GPL-FX-132 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				unlock_fx_mailbox(pp);
				hfc_fx_hand2_trace(
					HFC_FX_TRC_FLOGIRSP, 0x16, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				return;		/* FCLNX-GPL-FX-047 */
			}
		}
		/* FCLNX-GPL-069 */
		if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
			(pp->linkdown_occurred == 0) &&
			(pp->flogi_retry_change == 0 ) ){	/* FCLNX-GPL-FX-174 *//* FCLNX-GPL-FX-179 */
				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_FLOGIRSP, rc_passthrouh);
		}
		else if(( pp->flogi_retry_change == 1)&&(pp->linkdown_occurred == 0)){	/* FCLNX-GPL-FX-189 */
			pp->flogi_retry_change = 0;				/* FCLNX-GPL-FX-179 */
			hfc_fx_hand2_trace(
				HFC_FX_TRC_FLOGIRSP, 0x17, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
		}
		else{		/* FCLNX-GPL-FX-058 */
			
			pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
			clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
			clear_bit(HFC_PD_WAIT_FLOGI, (ulong *)&pp->status_detail1);
			pp->flogi_retry_change = 0;				/* FCLNX-GPL-FX-179 */
			hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR); 
			unlock_fx_mailbox(pp);
			hfc_fx_hand2_trace(
				HFC_FX_TRC_FLOGIRSP, 0x18, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			return;
		}		/* FCLNX-GPL-FX-058 */
		
		
		HFC_DBGPRT("flogi_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;
		
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("flogi_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		break;
	default :
		break;
	}
	
	if(pp->linkdown_occurred == 1) tmp_linkdown_occurred = 1;	/* FCLNX-GPL-FX-174 */
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	clear_bit(HFC_PD_WAIT_FLOGI, (ulong *)&pp->status_detail1);
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR); 
	
	if (mb_resp_status != 0) {	
	
		hfc_fx_initialize_failed(pp, core, NULL);
		
		hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);						/* FCLNX-GPL-314 */
		
		hfc_fx_copy_master_to_slave( pp, core );
		hfc_fx_change_portstat_linkdown(pp, core);
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}
	else{

		pp->flogi_max_frame_size = ( ushort )hfc_fx_read_val( mbox->mb_resp.type.flogi.flogi_max_frame_size ) ;
		pp->flogi_config_flag = (uchar)hfc_fx_read_val( mbox->mb_resp.type.flogi.flogi_config_flag ) ;
		pp->flogi_rsp_param = (uchar)hfc_fx_read_val( mbox->mb_resp.type.flogi.flogi_rsp_param ) ;
		
		core->fw_init_p->fw_iocinfo.fabric_param = 
			(pp->flogi_rsp_param & core->mb->mb_init.type.flogi.flogi_param);

		pp -> fabric_s_id = ( (uint) hfc_fx_read_val( mbox->mb_resp.type.flogi.recv_d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_resp.type.flogi.recv_d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_resp.type.flogi.recv_d_id[2] );
		pp -> fabric_d_id = ( (uint) hfc_fx_read_val( mbox->mb_resp.type.flogi.recv_s_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_resp.type.flogi.recv_s_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_resp.type.flogi.recv_s_id[2] );

		ww_name = hfc_fx_read_val( mbox->mb_resp.type.flogi.target_wwpn ) ;
		node_name = hfc_fx_read_val( mbox->mb_resp.type.flogi.target_wwnn ) ;
		
		HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_resp flogi_max_frame_size = %04x flogi_config_flag = %02x flogi_rsp_param = %02x\n", 
			pp->dev_minor, pp->flogi_max_frame_size, pp->flogi_config_flag,
			pp->flogi_rsp_param);
			
		HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_resp fabric_d_id = %llx fabric_s_id = %llx\n", 
			pp->dev_minor, pp -> fabric_d_id, pp -> fabric_s_id);
			
		HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_resp ww_name = %llx node_name = %llx\n", 
			pp->dev_minor, ww_name,
			node_name);
		
		if( pp->flogi_config_flag & HFC_FL_PID_VALID ){
			pp->scsi_id=0;
			pp->scsi_id = ( (uint64_t) hfc_fx_read_val( mbox->mb_resp.type.flogi.assign_portid[0] ) << 16 ) +
							( (uint64_t) hfc_fx_read_val( mbox->mb_resp.type.flogi.assign_portid[1] ) << 8 ) +
							  (uint64_t) hfc_fx_read_val( mbox->mb_resp.type.flogi.assign_portid[2] );
							  
			core->fw_init_p->fw_iocinfo.self_port_id[0] = mbox->mb_resp.type.flogi.assign_portid[0];
			core->fw_init_p->fw_iocinfo.self_port_id[1] = mbox->mb_resp.type.flogi.assign_portid[1];
			core->fw_init_p->fw_iocinfo.self_port_id[2] = mbox->mb_resp.type.flogi.assign_portid[2];
			core->fw_init_p->fw_iocinfo.configure_flag |= HFC_FX_PID_VALID;
		}

		if( pp->flogi_config_flag & HFC_FL_P2P_PID_VALID ){
			HFC_DBGPRT("hfcldd%d HFC_FL_P2P_PID_VALID\n",pp->dev_minor);
			
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] 
				= (uchar)hfc_fx_read_val( mbox->mb_resp.type.flogi.p2p_tgt_port_id[0] );
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] 
				= (uchar)hfc_fx_read_val( mbox->mb_resp.type.flogi.p2p_tgt_port_id[1] );
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] 
				= (uchar)hfc_fx_read_val( mbox->mb_resp.type.flogi.p2p_tgt_port_id[2] );
		}
		
		if( pp->flogi_config_flag & HFC_FL_FABRIC_EXIST ){
			HFC_DBGPRT("hfcldd%d HFC_FL_FABRIC_EXIST\n",pp->dev_minor);
			
			pp->switch_exist = HFC_SWITCH_EXIST;
			if(pp ->connect_type != HFC_FX_AL){
				pp ->connect_type = HFC_FX_SWITCH;
				core->fw_init_p->fw_iocinfo.connect_type = HFC_FX_SWITCH;	/* FCLNX-GPL-FX-171 */
			}
			
			core->fw_init_p->fw_iocinfo.configure_flag |= HFC_FX_FABRIC_VALID;
			core->fw_init_p->fw_iocinfo.configure_flag &= ~HFC_FX_P2P_PID_VALID;
			
			if((tmp_linkdown_occurred == 1)&&(HFC_FX_VIRTUAL_PORT(pp))){	/* FCLNX-GPL-FX-174 */
				set_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1);
				atomic_set(&pp->check_mbreq, 1);
			}
			else if( core-> mb -> mb_init.type.flogi.flogi_param & FLOGI_PARAM_VF ){
				hfc_fx_hand2_trace(
					HFC_FX_TRC_FLOGIRSP, 0x11, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
//				if( !(core-> mb -> mb_init.type.flogi.flogi_param & FLOGI_PARAM_SECURITY) ){
//					set_bit(HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1);
//					atomic_set(&pp->check_mbreq, 1);
//				}
			}
			else{
				/* Start Add PortID Delay Timer */
				hfc_fx_hand2_trace(
					HFC_FX_TRC_FLOGIRSP, 0x12, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				set_bit(HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1);
				atomic_set(&pp->check_mbreq, 1);
//				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//								pp->mb_timer[ HFC_MBTIME_ADD_PORTID ].delay ,TRUE);
//				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//								pp->mb_timer[ HFC_MBTIME_ADD_PORTID ].delay ,FALSE);
			}
		}
		else{
			HFC_DBGPRT("hfcldd%d else\n",pp->dev_minor);
			
			pp->switch_exist = 0;
			pp->connect_type = HFC_FX_PT2PT;
			
			set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
			
			if( core->fw_init_p->fw_iocinfo.connect_type == HFC_FX_AL ){
				HFC_DBGPRT("hfcldd%d HFC_NEED_CHANGE_STATE\n",pp->dev_minor);
				hfc_fx_change_portstat_linkup(pp, core);
				hfc_fx_hand2_trace(
					HFC_FX_TRC_FLOGIRSP, 0x13, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
					hfc_fx_wwnverify_linkup(pp, NULL, core, mb_resp_status, 0);
				}
				else {
					if(pp->initialize != 0){
						hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
					}
				}
			}
			else{	/* Connection Type : Point to Point	*/
//				pp->connect_type = HFC_FX_PT2PT;
				
				if( pp->ww_name > ww_name ){	/*  FCLNX-GPL-FX-026 */
					if (pp->target_arg[0] == NULL) { /* FCLNX-GPL-FX-237 */
						pp->scsi_id = HFC_PTOP_INIT_PORTID;	/* FCLNX-GPL-FX-066 */
						
						port_id = HFC_PTOP_TGT_PORTID;		/* FCLNX-GPL-FX-066 */

						/* Create target_info_fx */
						target = hfc_fx_add_target_info_fx(pp, port_id);
					}
					else {
						target = pp->target_arg[0];
						pp->scsi_id = HFC_PTOP_INIT_PORTID;	/* FCLNX-GPL-FX-066 */
						
						target->scsi_id = HFC_PTOP_TGT_PORTID;		/* FCLNX-GPL-FX-066 */
					}
					hfc_fx_hand2_trace(
						HFC_FX_TRC_FLOGIRSP, 0x14, pp, core->rp, core, target, NULL,
						(uint64_t)rc_passthrouh, 0, 0 );
					HFC_DBGPRT("hfcldd%d HFC_FL_FABRIC_EXIST\n",pp->dev_minor);
					
					if (target != NULL) {	/* FCLNX-GPL-FX-446 >>> */
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
						atomic_set(&pp->check_mbreq, 1);
					}						/* FCLNX-GPL-FX-446 <<< */
				}
				else{	/* Waiting to receive PLOGI from I/O Device */
					hfc_fx_hand2_trace(
						HFC_FX_TRC_FLOGIRSP, 0x15, pp, core->rp, core, NULL, NULL,
						(uint64_t)rc_passthrouh, 0, 0 );
					HFC_DBGPRT("hfcldd%d HFC_WAIT_RECEIVE_PLOGI\n",pp->dev_minor);
					set_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1);
				}
			}
		}
		
		clear_bit(HFC_PD_WAIT_FLOGI, (ulong *)&pp->status_detail1);
		pp->flogi_retry_change = 0;				/* FCLNX-GPL-FX-179 */

	}
	
	if (HFC_FX_NPIV_ENABLE(pp)) {
		hfc_fx_npiv_config_check(pp, core);
	}
	
	unlock_fx_mailbox(pp);
	
	HFC_EXIT("hfc_fx_flogi_resp");
		
	return;
}


/*
 * Function:    hfc_fx_plogi_resp
 *
 * Purpose:     This routine deals with PLOGI response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */ 
void hfc_fx_plogi_resp(struct port_info *pp, struct core_info *core )
{
	
	HFC_ENTRY("hfc_fx_plogi_resp");
	
	HFC_DBGPRT("hfc_fx_plogi_resp\n");
	
	
	if( test_bit(HFC_PD_WAIT_PLOGI_N, (ulong *)&pp->status_detail1) ){
		hfc_fx_plogi_fabric_resp(pp, core);
	}
	else{
		hfc_fx_plogi_target_resp(pp, core);
	}

	HFC_DBGPRT("hfc_fx_plogi_resp : call unlock mailbox \n");
	
	HFC_EXIT("hfc_fx_plogi_resp");
	
	return;
}


/*
 * Function:    hfc_fx_plogi_fabric_resp
 *
 * Purpose:     This routine deals with PLOGI response to Name Server
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */ 
void hfc_fx_plogi_fabric_resp(struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx			*mbox = NULL ;
	int                         mb_resp_status = 0 ;
	uint                        rc_passthrouh = 0xffffffff;
//	uint						scsi_id=0;
	
	HFC_ENTRY("hfc_fx_plogi_fabric_resp");
	
	HFC_DBGPRT("hfc_fx_plogi_fabric_resp\n");
	
	mbox = core->mb;
	core->mb_status = 0;
	
//	HFC_DBGPRT("hfcldd%d: hfc_fx_plogi_fabric_resp scsi_id = %08x ww_name = %llx node_name = %llx\n", 
//			pp->dev_minor, scsi_id, ww_name,
//			node_name);
	
	/* PLOGI to Name_Server */
	HFC_DBGPRT(" hfcldd : hfc_fx_plogi_resp : receive responce from Name_Server \n");
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_PLOGIRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );

	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_PLOGIRSP, rc_passthrouh);
			}else{	/* FCLNX-GPL-FX-058 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_PD_WAIT_PLOGI_N, (ulong *)&pp->status_detail1);
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			mb_resp_status = SCS_NO_DEV_RESP;			/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			/* mb_resp_status clear */
			HFC_DBGPRT("hfc_fx_plogi_fabric_resp : clear mb responce status \n");
			mb_resp_status = 0 ;
			break;

		default :
			break;
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	clear_bit(HFC_PD_WAIT_PLOGI_N, (ulong *)&pp->status_detail1);
	
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	if( mb_resp_status != 0 )
	{
		HFC_DBGPRT("hfc_fx_login_fabric_resp : login has failed \n");
		/* PLOGI to Name Server failed */
		hfc_fx_initialize_failed(pp, core, NULL);
	}
	else{
		/* PLOGI succeeded */
		HFC_DBGPRT("hfc_fx_login_resp : login succeeded \n");
		pp->fabric_ww_name = hfc_fx_read_val( mbox->mb_resp.type.plogi.target_wwpn );
		pp->fabric_node_name = hfc_fx_read_val( mbox->mb_resp.type.plogi.target_wwnn );
		pp->link_dead_cnt = 0 ;

		if (pp->rft_id_skip == HFC_FX_RFT_ID_SKIP_DISABLE) {
			HFC_DBGPRT("hfc_fx_plogi_fabric_resp(start RFT_ID delay timer)\n");

			hfc_fx_hand2_trace(
				HFC_FX_TRC_PLOGIRSP, 0x11, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			/* Start RFT_ID Delay Timer */
			set_bit(HFC_PD_NEED_RFTID, (ulong *)&pp->status_detail1);
			atomic_set(&pp->check_mbreq, 1);
//			hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//				pp->mb_timer[ HFC_MBTIME_RFT_ID ].delay ,TRUE);
//			hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//				pp->mb_timer[ HFC_MBTIME_RFT_ID ].delay ,FALSE);
		}
		else {
			hfc_fx_hand2_trace(
				HFC_FX_TRC_PLOGIRSP, 0x12, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			HFC_DBGPRT("hfc_fx_plogi_fabric_resp(start RFF_ID delay timer)\n");

			/* Start RFF_ID Delay Timer */
			set_bit(HFC_PD_NEED_RFFID, (ulong *)&pp->status_detail1);
			atomic_set(&pp->check_mbreq, 1);
//			hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//				pp->mb_timer[ HFC_MBTIME_RFF_ID ].delay ,TRUE);
//			hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//				pp->mb_timer[ HFC_MBTIME_RFF_ID ].delay ,FALSE);
		}
	}

	unlock_fx_mailbox(pp);
	
	HFC_EXIT("hfc_fx_plogi_fabric_resp");
	
	return;
}


/*
 * Function:    hfc_fx_plogi_target_resp
 *
 * Purpose:     This routine deals with PLOGI response to Target
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */ 
void hfc_fx_plogi_target_resp(struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx			*mbox = NULL ;
	struct target_info_fx			*target = NULL ;
	int                         mb_resp_status = 0 ;
	uint                        rc_passthrouh = 0xffffffff;
	uint64_t                    ww_name = 0, ww_name1=0 ;
	uint64_t                    node_name = 0, node_name1=0 ;
//	uint						save_xrb_outp, save_xrb_inp ;
//	uint						work_save_xrb_outp, work_save_xrb_inp ;
//	uint 						xrb_in_no,xrb_no,xrb_cnt;
//	struct hfc_pkt_fx			*hfcp_wk = NULL;
//	int                         hash = 0;
//	int                         scsito = 0;
	uint						scsi_id=0,Tdid=0,Rsid=0,Tsid=0,Rdid=0;
	
	HFC_ENTRY("hfc_fx_plogi_target_resp");
	
	HFC_DBGPRT("hfc_fx_plogi_target_resp\n");
	
	mbox = core->mb;
	core->mb_status = 0;
	
	/*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.plogi.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.plogi.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.plogi.fcph_hdr.d_id[2] );
							
	ww_name = hfc_fx_read_val( mbox->mb_resp.type.plogi.target_wwpn );
	node_name = hfc_fx_read_val( mbox->mb_resp.type.plogi.target_wwnn );
	
	/* PLOGI to Target */
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	if( target == NULL )
	{/* Target_info is not found */
		HFC_DBGPRT(" hfcldd : hfc_fx_plogi_resp : target info was not founded \n");
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LGINRSP, 0x11, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		
		return;
	}/* Target_info is not found */
	
	/* FCLNX-GPL-FX-446 >>> */
	if (!(test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status))) {
		hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBRSP, 0);

		clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
		clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

		/* Stop timer and delete ID */
		hfc_fx_watchdog_enter( pp , core, NULL, NULL, 0, HFC_FX_MB_RSP_TMR, 0, TRUE);
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);

		unlock_fx_mailbox(pp);

		return;
	}
	/* FCLNX-GPL-FX-446 <<< */
	
	/*  When target_info_fx is found */
	HFC_DBGPRT("hfc_fx_login_resp : target info was founded \n");
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LGINRSP, 0x00, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */

				if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags)	/* When other HBAs exists in Fabric */
				 || ( !( (core->mb_results == 0x02FF1030)||(core->mb_results == 0x02FF1050) ) ) ) {
					hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_LGINRSP, rc_passthrouh);
				}

			}else{	/* FCLNX-GPL-FX-058 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status);
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			
			mb_resp_status = SCS_NO_DEV_RESP;
			break;
		case HFC_MBPASS_SUCCESS :
			/* mb_resp_status clear */
			HFC_DBGPRT("hfc_fx_plogi_target_resp : clear mb responce status \n");
			mb_resp_status = 0 ;
			pp->link_dead_cnt = 0 ;

			/* Skip WWN check during the process of target generation */	
			if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) )  
				break;

			if( (target->ww_name != ww_name) || (target->node_name != node_name) )
			{ /* WWN is mismatched */
				mb_resp_status = SCS_LOGIN_WWCHG;
				if( target->ww_name != ww_name )
				{
					HFC_8L_TO_8B(ww_name1, ww_name);
					HFC_8L_TO_8B(node_name1, node_name);
					memcpy(&core->logdata[0],(uchar*)&ww_name1,8);
					memcpy(&core->logdata[8],(uchar*)&node_name1,8);
				}
				else
				{
					HFC_8L_TO_8B(ww_name1, ww_name);
					HFC_8L_TO_8B(node_name1, node_name);
					memcpy(&core->logdata[0],(uchar*)&ww_name1,8);
					memcpy(&core->logdata[8],(uchar*)&node_name1,8);
				}
				hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_EVNT3,0x0b,core->logdata,16) ;
			}
			break;
		default :
			break;
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	clear_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status);
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	switch(mb_resp_status)
	{
	case SCS_LOGIN_WWCHG :
	/* WWN is mismatched */
		HFC_DBGPRT("hfc_fx_login_target_resp : unmatch www name \n");
		clear_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
		if (HFC_FX_MQ_VALID(pp))
			hfc_fx_mq_change_target_info(pp, target);

	case SCS_NO_DEV_RESP :
	case SCS_CANCEL_RESP :
		/* LOGIN failed */
		HFC_DBGPRT("hfc_fx_login_target_resp : login has failed \n");
		
		if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status)){	/* FCLNX-GPL-FX-112 Start */
			HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY);
			hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
			if(pp->rt_err_enable){
				HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - Isolate HBA Port.\n");
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
					hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);
				}
				else{
					hfc_fx_watched_errcount_i(pp, NULL, HFC_RT_ERR);
				}
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LGINRSP, 0x16, pp, core->rp, core, target, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
			}else{
				HFC_DBGPRT("hfcldd : hfc_fx_cancel_scsi_resp - Issue Link Reset.\n");
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);					/* FCLNX-GPL-FX-112 End */
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LGINRSP, 0x13, pp, core->rp, core, target, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
			}
			return;																
		}else{
			if((!(pp->flogi_config_flag & HFC_FL_FABRIC_EXIST) ) && (pp->connect_type == HFC_FX_PT2PT)){
				HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY);
				hfc_fx_initialize_failed(pp, core, target);
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LGINRSP, 0x12, pp, core->rp, core, target, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				return;																
			}
		}
		
		break;
	case 0 :
		/* PLOGI succeeded */
		HFC_DBGPRT("hfc_fx_login_target_resp : login succeeded \n");

		if(target->link_recovered){
			if (HFC_FX_PHYSICAL_PORT(pp)) {
				HFC_INFPRT("hfcldd%d : Target port( WWPN : %llx ) links up.\n", 
					pp->dev_minor, (unsigned long long)ww_name);		/* FCLNX-GPL-334 */
			}
			else {
				HFC_INFPRT("hfcldd%d : Target port( WWPN : %llx ) links up (PORT_ID:0x%06llx) (0x%02x:0x%02x).\n", 
					pp->pport->dev_minor, (unsigned long long)ww_name, pp->scsi_id, pp->sub_rid, pp->rid);
			}
			target->link_recovered = 0;
		}
		
		if( test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) ) {	/* FCLNX-GPL-FX-066 */
			if(( pp->connect_type == HFC_FX_PT2PT )&&(!(pp->flogi_config_flag & HFC_FL_FABRIC_EXIST))){
				Tdid = scsi_id;
				Tsid = ( (uint) hfc_fx_read_val( mbox->mb_init.type.plogi.fcph_hdr.s_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.plogi.fcph_hdr.s_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.plogi.fcph_hdr.s_id[2] );
				Rdid = ( (uint) hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_d_id[2] );
				Rsid = ( (uint) hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_s_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_s_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_s_id[2] );
				HFC_DBGPRT("Rdid = %08x, Rsid = %08x, Tdid = %08x, Tsid = %08x\n", Rdid, Rsid, Tdid, Tsid);
				
				if((Rdid != Tsid)||(Rsid != Tdid)){
					HFC_DBGPRT("hfc_fx_receive_plogi_intreq - Link Failed\n");
					HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY);
					hfc_fx_initialize_failed(pp, core, target);
					hfc_fx_hand2_trace(
					HFC_FX_TRC_LGINRSP, 0x12, pp, core->rp, core, target, NULL,
						(uint64_t)rc_passthrouh, 0, 0 );
					return;
				}
				
				/* FCLNX-GPL-FX-039 Start */
				/* Set p2p_tgt_port_id of Init_Table */
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] = (uchar)hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_s_id[0] );
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] = (uchar)hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_s_id[1] );
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] = (uchar)hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_s_id[2] );
				
				/* Set self_port_id of Init_Table */
				core->fw_init_p->fw_iocinfo.self_port_id[0] = (uchar)hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_d_id[0] );
				core->fw_init_p->fw_iocinfo.self_port_id[1] = (uchar)hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_d_id[1] );
				core->fw_init_p->fw_iocinfo.self_port_id[2] = (uchar)hfc_fx_read_val( mbox->mb_resp.type.plogi.recv_d_id[2] );
		
				/* Set configure_flag of Init_Table  */
				core->fw_init_p->fw_iocinfo.configure_flag = ( HFC_FL_PID_VALID | HFC_FL_P2P_PID_VALID );
				/* FCLNX-GPL-FX-039 End */
			
				clear_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp -> status_detail1 );	/* FCLNX-GPL-FX-032 */
				HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq pp->scsi_id=%llx.\n",pp->dev_minor, pp->scsi_id);	/* FCLNX-GPL-FX-032 */
				hfc_fx_copy_master_to_slave( pp, core );	/* FCLNX-GPL-FX-005 */
				hfc_fx_change_portstat_linkup(pp, core);	/* FCLNX-GPL-FX-005 */
			}
		}	/* FCLNX-GPL-FX-066 */
		
		/* Cancel Link Up waiting timer between SW and device */
		clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
		hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
//			clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
		clear_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status);
		
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);

		/* Size of transmission frame used by new statistical information I/O calculation */
		target->send_frame_size = (ushort)hfc_fx_read_val( mbox->mb_resp.type.plogi.plogi_max_frame_size );
		
		HFC_DBGPRT("hfc_fx_login_target_resp : plogi_max_frame_size=%04x\n", target->send_frame_size);
		
		if( target->send_frame_size <= 0x800 ){
			target->mfsize = target->send_frame_size;
		}
		else{
			target->mfsize = 0x800;
		}
		
		target->fc_class = hfc_fx_read_val( mbox->mb_resp.type.plogi.class );

		if ((pp->connect_type == HFC_FX_SWITCH ) 						/* FCWIN-0082STR*/
		||  ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
			if( pp->flogi_max_frame_size != 0 ){
				if( pp->flogi_max_frame_size < target->mfsize ){
					target->mfsize = pp->flogi_max_frame_size;
				}
			}
		}
		HFC_DBGPRT("hfc_fx_login_target_resp : target->mfsize=%04x, pp->flogi_max_frame_size=%04x, target->send_frame_size=%04x \n", 
			target->mfsize, pp->flogi_max_frame_size, target->send_frame_size);
		
		break;

	default :
		break;
	}

	HFC_DBGPRT(" hfcldd%d : hfc_fx_plogi_target_resp search :target info scsi_id = %08x \n", pp->dev_minor, scsi_id);
	
//	if ( test_bit(HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2) )
//	{
//		HFC_DBGPRT("hfc_fx_plogi_target_resp : complete bus reset in this target \n");
//		clear_bit(HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2);
//	}

	/* Is mailbox locked in the wwnvwrify subroutine? */
	if (!mb_resp_status || !test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
	{
		HFC_DBGPRT("hfc_fx_plogi_target_resp : call wwnverify login \n");
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LGINRSP, 0x14, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		hfc_fx_wwnverify_plogi(pp, target, core, mb_resp_status, ww_name);
	}
	
	HFC_DBGPRT("hfc_fx_plogi_target_resp : call unlock mailbox \n");
	unlock_fx_mailbox(pp);
	
	HFC_EXIT("hfc_fx_plogi_target_resp");
	
	return;
}


/*
 * Function:    hfc_fx_pdisc_resp
 *
 * Purpose:     This routine deals with PDISC response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_pdisc_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint64_t            ww_name=0, ww_name1=0 ;
	uint64_t            node_name=0, node_name1=0 ;
	uint				scsi_id=0;
	
	HFC_ENTRY("hfc_fx_pdisc_resp");
	
	mbox = core->mb;
	core->mb_status = 0;
		
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.pdisc.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.pdisc.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.pdisc.fcph_hdr.d_id[2] );
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	
	if( target == NULL )
	{/* Target_info is not found */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PDISCRSP, 0x11, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return;
	}
	else
	{/* When target_info_fx is found */
		rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);			/* FCWIN-0170 */
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PDISCRSP, 0x00, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_PDISCRSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			pp->link_dead_cnt = 0 ;
			HFC_8B_TO_8L(ww_name, mbox->mb_resp.type.pdisc.target_wwpn);
			HFC_8B_TO_8L(node_name, mbox->mb_resp.type.pdisc.target_wwnn);
			if( (target->ww_name != ww_name) || (target->node_name != node_name) )
			{	/* WWN is mismatched */
				mb_resp_status = SCS_PDISC_WWCHG ;
				if( target->ww_name != ww_name )
				{
					HFC_8L_TO_8B(ww_name1, ww_name);
					HFC_8L_TO_8B(node_name1, node_name);
					memcpy(&core->logdata[0],(uchar*)&ww_name1,8);
					memcpy(&core->logdata[8],(uchar*)&node_name1,8);
				}
				else
				{
					HFC_8L_TO_8B(ww_name1, ww_name);
					HFC_8L_TO_8B(node_name1, node_name);
					memcpy(&core->logdata[0],(uchar*)&ww_name1,8);
					memcpy(&core->logdata[8],(uchar*)&node_name1,8);
				}
				hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBRESP,ERRID_HFCP_EVNT3,0x0f,core->logdata,16);	/* FCLNX-GPL-0130 */
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
	
	core->mb_retry_cnt = 0;
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
		if(test_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status) )
		{
			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
		}
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
		atomic_set(&pp->check_mbreq, 1);
		break;

	case SCS_PDISC_WWCHG :
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
		atomic_set(&pp->check_mbreq, 1);
		break;

	case 0 :
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		break;

	default :
		break;
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_PDISCRSP, 0x10, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_pdisc_resp");
}


/*
 * Function:    hfc_fx_shadow_up_resp
 *
 * Purpose:     This routine deals with Shadow_up response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_shadow_up_resp(struct port_info *pp, struct core_info *core )
{
	uint				rc_passthrouh = 0xffffffff;
	uint				mb_resp_status = 0;
	struct mailbox_fx	*mbox = core->mb;
	uint				i;
	uchar				failed_master_core = 0, failed_slave_core = 0;
	uchar				wait_mb_core = 0;
	
	HFC_ENTRY("hfc_fx_shadow_up_resp");
	
	core->mb_status = 0;
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_SHADOWUPRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	memset((void *)core->logdata, 0, 16);
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code, 4) ;
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);

	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_SHADOWUPRSP, rc_passthrouh);
		}
		
		HFC_DBGPRT("shadow_up_resp, fatal %d\n",rc_passthrouh);
		mb_resp_status = SCS_NO_DEV_RESP;

		break;

	case HFC_MBPASS_SUCCESS :
		HFC_DBGPRT("shadow_up_resp, success %d \n",rc_passthrouh);
		mb_resp_status = 0 ;
		
		break;
	default :
		break;
	}
	
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	
	if( mb_resp_status == 0 ){
		/* check if whether all the cores finish add port_id or not */
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if(hfc_fx_check_cs_disable(pp, core))
				continue;	/* FCLNX-GPL-FX-438 */
			if ( test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status) ){
				wait_mb_core = 1;
				break;
			}
			if (core->mb_results != HFC_MBPASS_SUCCESS) {
			    if (core->core_no == pp->master_core_no) {
					failed_master_core = 1;
				} else {
					failed_slave_core = 1;
				}
			}
		}
	
		/* other cores are executing mailbox */
		if (wait_mb_core) {
			hfc_fx_hand2_trace(
				HFC_FX_TRC_SHADOWUPRSP, 0x01, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
			return;
		}
	
		/* all the cores finished add port_id */
		clear_bit(HFC_PD_WAIT_SHADOW_UP, (ulong *)&pp->status_detail2);

		/* STOP mailbox retry timer per port */
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);

		/* master_core failed. ChangeState(LinkDown) */
		if (failed_master_core) {
			hfc_fx_hand2_trace(
				HFC_FX_TRC_SHADOWUPRSP, 0x02, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
		
			hfc_fx_initialize_failed(pp, core, NULL);
		 
			return;	
		}
		else if (failed_slave_core) {
			/* only slave core failed. the core is isolated. */
			hfc_fx_hand2_trace(
				HFC_FX_TRC_SHADOWUPRSP, 0x03, pp, core->rp, core, NULL, NULL,
				(uint64_t)rc_passthrouh, 0, 0 );
	
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				if(hfc_fx_check_cs_disable(pp, core))
					continue;	/* FCLNX-GPL-FX-438 */
				if (core->mb_results != HFC_MBPASS_SUCCESS) {
					/* Execute core FORCE-CHKSTOP command and logout */
			    	hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
				}
			}
			/* Determine master core */
			hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);
			
			/* Set fw_init_tbl */
			hfc_fx_set_fw_init_tbl(pp);
			
			/* check the results of all core_start */
			if (hfc_fx_all_core_start(pp) != 0) {
				hfc_fx_hand2_trace(
					HFC_FX_TRC_CORESTARTRSP, 0x04, pp, core->rp, core, NULL, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				/* all core_start failed. */
//				hfc_fx_hw_chk_stop(pp);		/* TBD */
			}
		}
	
		/* Issue SCR with master core */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_SHADOWUPRSP, 0x04, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );

//		atomic_set(&pp->check_mbreq, 1);
		
	}
	else{
		
	}

	unlock_fx_mailbox(pp);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_SHADOWUPRSP, 0x10, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	HFC_EXIT("hfc_fx_shadow_up_resp");
		
	return;
}


/*
 * Function:    hfc_fx_frmsndrcv_resp
 *
 * Purpose:     This routine deals with FRMSNDRCV response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_frmsndrcv_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	ushort				payload_type=0;
	uchar				payload_cmd=0;
	struct payload_fx	*pyload = NULL;


	HFC_ENTRY("hfc_fx_frmsndrcv_resp");
	
	HFC_DBGPRT("hfc_fx_frmsndrcv_resp\n");
	
	mbox = core->mb;
	core->mb_status = 0;
	pyload = core->payload;
	
	payload_cmd = (uchar) hfc_fx_read_val( pyload->send_payload.data0[0]);
	
	HFC_DBGPRT("hfc_fx_frmsndrcv_resp payload_cmd %02x\n",payload_cmd);


	switch ( payload_cmd )
	{
		case HFC_PRLI_REQDATA0 :
			hfc_fx_prli_resp( pp, core );
			break;
		case HFC_PRLO_REQDATA0 :
			hfc_fx_prlo_resp( pp, core );
			break;
		case HFC_SCR_REQDATA0 :
			hfc_fx_scr_resp( pp, core );
			break;
		case HFC_LOGO_REQDATA0 :
			hfc_fx_logo_resp( pp, core );
			break;
		case HFC_GXX_REQDATA0 :
			HFC_DBGPRT("hfc_fx_frmsndrcv_resp HFC_GXX_REQDATA0\n");
			payload_type = ( (ushort)hfc_fx_read_val( pyload->send_payload.type.gxx.data1[0] ) << 8 ) +
					(ushort)hfc_fx_read_val( pyload->send_payload.type.gxx.data1[1] );
			HFC_DBGPRT("hfc_fx_frmsndrcv_resp payload_type %04x\n",payload_type);
			switch ( payload_type )
			{
				case HFC_GCSID_REQDATA8 :
					hfc_fx_gcs_id_resp( pp, core );
					break;
				case HFC_GIDPN_REQDATA8 :
					hfc_fx_gid_pn_resp( pp, core );
					break;
				case HFC_GPNID_REQDATA8 :
					hfc_fx_gpn_id_resp( pp, core );
					break;
				case HFC_GIDFT_REQDATA8 :
					hfc_fx_gid_ft_resp( pp, core );
					break;
				case HFC_RFTID_REQDATA8 :
					hfc_fx_rft_id_resp( pp, core );
					break;
				case HFC_RFFID_REQDATA8 :
					hfc_fx_rff_id_resp( pp, core );
					break;
				case HFC_GPNFT_REQDATA8 :
					hfc_fx_gpn_ft_resp( pp, core );
					break;
			}
		default :
			break;
	}

	HFC_EXIT("hfc_fx_frmsndrcv_resp");
}


/*
 * Function:    hfc_fx_prli_resp
 *
 * Purpose:     This routine deals with PLGI response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_prli_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	uchar				payload_resp =0;
	uchar				reason_code =0, service_param=0,i=0;	/* FCLNX-GPL-FX-112 */
//	uchar				reason_code =0, code_explanation=0, service_param=0;
//	ushort				payload_length=0;
	uint64_t 			ww_name=0;
	ushort				prli_req=0, prli_resp=0, prli_parm=0;
	struct payload_fx	*pyload=NULL;
	struct core_info	*core_wk=NULL;							/* FCLNX-GPL-FX-112 */
	struct dev_info_fx	*dev=NULL;
	
	HFC_ENTRY("hfc_fx_prli_resp");
	mbox = core->mb;
	pyload = core->payload;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	
	if( target == NULL )
	{/* Target_info is not found */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PRLIRSP, 0x11, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return;
	}
	else
	{/* When target_info_fx is found */
		/* FCLNX-GPL-FX-446 >>> */
		if (!(test_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status))) {
			hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBRSP, 0);

			clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
			clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

			/* Stop timer and delete ID */
			hfc_fx_watchdog_enter( pp , core, NULL, NULL, 0, HFC_FX_MB_RSP_TMR, 0, TRUE);
			hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);

			unlock_fx_mailbox(pp);

			return;
		}
		/* FCLNX-GPL-FX-446 <<< */
		
		rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PRLIRSP, 0x00, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */

				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_PRLIRSP, rc_passthrouh);
			}else{	/* FCLNX-GPL-FX-058 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				clear_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status);
				clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			service_param = (uchar)hfc_fx_read_val( pyload->receive_payload.type.prli.data1[6]) ;
			if( !(service_param & 0x20) ){	/* Image Pair not Established */
				reason_code = (uchar)hfc_fx_read_val( pyload->receive_payload.type.prli.data1[5]) ;
				mb_resp_status =  SCS_NO_DEV_RESP;
			}
			break;
		default :
			break;
		}
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	
//	HFC_DBGPRT("hfc_fx_prli_resp mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_prli_resp mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :
		if(test_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status) )
		{
			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
		}
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status);
		if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status)){
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				
				hfc_fx_notify_tout(pp, core_wk, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */
				hfc_fx_cancel_scsi_cmd(pp, core_wk, target, 0, NULL, mb_resp_status,
					 HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);
			}
			if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
				hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, mb_resp_status, HFC_CSCSI_ERROR,
					TRUE, TRUE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
			}
			hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
			clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
		}																	/* FCLNX-GPL-FX-112 End */
		break;

	case 0 :
		clear_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
		if (HFC_FX_MQ_VALID(pp))
			hfc_fx_mq_change_target_info(pp, target);
		
		if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status)){	/* FCLNX-GPL-FX-112 Start */
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				HFC_DBGPRT("hfcldd : hfc_fx_prli_target_resp - 11 core_no=%d \n",core_wk->core_no);
				/* SoftLo Errlog output */
				hfc_fx_notify_tout(pp, core_wk, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */
				hfc_fx_cancel_weque(pp, core_wk, target, 0, NULL, mb_resp_status, HFC_CSCSI_ERROR, HFC_FLASH_TARGET);
			}
			if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
				hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, mb_resp_status, HFC_CSCSI_ERROR,
					FALSE, FALSE, FALSE, TRUE, FALSE, HFC_FLASH_TARGET);
			}
			hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
			clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
			dev = target->dev;
			while( dev != NULL){
				dev->lustat = 0x00;
				dev->dev_core_stat.all = 0;	/* FCLNX-GPL-FX-014 *//* FCLNX-GPL-FX-073 */
				dev = dev->next;
			}
		}																	/* FCLNX-GPL-FX-112 End */
		
		prli_req = (ushort)hfc_fx_read_val(pyload->send_payload.type.prli.prli_param_req);
		prli_resp = (ushort)hfc_fx_read_val(pyload->receive_payload.type.prli.prli_param_resp);
		
		HFC_DBGPRT("hfc_fx_prli_target_resp : prli_req = %04x prli_resp = %04x\n",prli_req, prli_resp);
		
		prli_parm = (prli_req)&(prli_resp);
		prli_parm &= 0x0180;
		prli_parm |= (prli_resp & 0x0030);
		prli_parm |= 0x0002;
		target->prli_parm = prli_parm;
		
		HFC_DBGPRT("hfc_fx_prli_target_resp : target->prli_parm = %04x \n",target->prli_parm);
		HFC_DBGPRT("TARGET PORT FOUND\n");
		
		/* FCLNX-GPL-FX-446 : Reset LOGIN sequence counter >>> */
		target->login_seq_retry_cnt = pp->login_seq_retry_cnt;
		/* FCLNX-GPL-FX-446 <<< */

		break;

	default :
		break;
	}
	
	/* Is mailbox locked in the wwnvwrify subroutine? */
	if (!mb_resp_status || !test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
	{
		HFC_DBGPRT("hfc_fx_prli_target_resp : call wwnverify login \n");
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PRLIRSP, 0x14, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		hfc_fx_wwnverify_prli(pp, target, core, mb_resp_status, ww_name);
	}
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_prli_resp");
}


/*
 * Function:    hfc_fx_prlo_resp
 *
 * Purpose:     This routine deals with PRLO response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_prlo_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	uchar				payload_resp = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	
	HFC_ENTRY("hfc_fx_prlo_resp");
	
	mbox = core->mb;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	
	if( target == NULL )
	{/* Target_info is not found */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PRLORSP, 0x11, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return;
	}
	else
	{/* When target_info_fx is found */
		rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PRLORSP, 0x00, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_PRLORSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			break;
		default :
			break;
		}
	}
	
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
		if(test_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status) )
		{
			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
		}
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
		atomic_set(&pp->check_mbreq, 1);
		break;

	case 0 :
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		break;

	default :
		break;
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_PRLORSP, 0x10, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_prlo_resp");
}


/*
 * Function:    hfc_fx_scr_resp
 *
 * Purpose:     This routine deals with SCR response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_scr_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
//	uchar				payload_resp = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	struct payload_fx	*pyload = NULL;
	
	HFC_ENTRY("hfc_fx_scr_resp");
	
	HFC_DBGPRT("hfc_fx_scr_resp\n");
	
	mbox = core->mb;
	pyload = core->payload;
	
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_SCRRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_SCRRSP, rc_passthrouh);
			}else{	/* FCLNX-GPL-FX-058 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				clear_bit(HFC_PD_WAIT_SCR, (ulong *)&pp->status_detail1);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			HFC_DBGPRT("flogi_resp, fatal %d\n",rc_passthrouh);
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			HFC_DBGPRT("hfc_fx_scr_resp HFC_MBPASS_SUCCESS\n");
			mb_resp_status = 0 ;
			break;
		default :
			break;
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
	clear_bit(HFC_PD_WAIT_SCR, (ulong *)&pp->status_detail1);
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	if( mb_resp_status != 0 ){
		hfc_fx_hand2_trace(
			HFC_FX_TRC_SCRRSP, 0x11, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		/* Issue change State */
		hfc_fx_initialize_failed(pp, core, NULL);
	}
	else{
		/* Issue PLOGI to Name Server with Master core */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_SCRRSP, 0x12, pp, core->rp, core, NULL, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		set_bit(HFC_PD_NEED_PLOGI_N, (ulong *)&pp->status_detail1);
		atomic_set(&pp->check_mbreq, 1);
		
		/* Start PLOGI(NameServer) Delay Timer */
//		hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//			pp->mb_timer[ HFC_MBTIME_PLOGI ].delay ,TRUE);
//		hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR,
//			pp->mb_timer[ HFC_MBTIME_PLOGI ].delay ,FALSE);
	}
	
	unlock_fx_mailbox(pp);
	
	HFC_EXIT("hfc_fx_scr_resp");
}


/*
 * Function:    hfc_fx_logo_resp
 *
 * Purpose:     This routine deals with LOGO response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_logo_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	struct core_info	*core_wk = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	uchar				payload_resp = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	uchar				i=0;
	
	HFC_ENTRY("hfc_fx_logo_resp");
	
	mbox = core->mb;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	
	/* FCLNX-GPL-FX-446 >>> */
	if( (target != NULL) && (scsi_id != 0x00fffffe) ){
		if (!(test_bit(HFC_TS_WAIT_LOGO_TGT, (ulong *)&target->status))) {
			hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBRSP, 0);

			clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
			clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

			/* Stop timer and delete ID */
			hfc_fx_watchdog_enter( pp , core, NULL, NULL, 0, HFC_FX_MB_RSP_TMR, 0, TRUE);
			hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);

			unlock_fx_mailbox(pp);

			return;
		}
	}
	/* FCLNX-GPL-FX-446 <<< */
	
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LOGORSP, 0x00, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	switch (rc_passthrouh)
	{
	case HFC_MBPASS_WAIT_RETRY :
		return;
	case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
		return;
	case HFC_MBPASS_RETRY_OVER :
	case HFC_MBPASS_RETRY_FAIL :
	case HFC_MBPASS_ERROR :
	case HFC_MB_FATAL :
		/* FCLNX-GPL-069 */
		if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
			(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
			(rc_passthrouh == HFC_MBPASS_ERROR)			){

			hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_LOGORSP, rc_passthrouh);
		}
		mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
		break;
	case HFC_MBPASS_SUCCESS :
		mb_resp_status = 0 ;
		break;
	default :
		break;
	}
	
	core->mb_retry_cnt = 0;
	if(test_bit(HFC_PD_WAIT_LOGO_FCSW, (ulong *)&pp->status_detail1)){
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LOGORSP, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );													/* FCLNX-GPL-FX-18 */
		clear_bit(HFC_PD_WAIT_LOGO_FCSW, (ulong *)&pp->status_detail1);
	}
	else if((target != NULL)&&(test_bit(HFC_TS_WAIT_LOGO_TGT, (ulong *)&target->status))){	/* FCLNX-GPL-FX-18 */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LOGORSP, 0x21, pp, core->rp, core, target, NULL,
			0, 0, 0 );													/* FCLNX-GPL-FX-18 */
		clear_bit(HFC_TS_WAIT_LOGO_TGT, (ulong *)&target->status);
	}
	
	if((!( pp->flogi_config_flag & HFC_FL_FABRIC_EXIST )) && (pp->connect_type == HFC_FX_PT2PT)){
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	if(target != NULL){/* FCLNX-GPL-FX-18 */
		switch(mb_resp_status)
		{
		/* FCLNX-GPL-FX-289, 291 */
		case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
		case 0 :
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				
				hfc_fx_notify_tout(pp, core_wk, target, 0, HFC_FLASH_TARGET);
				hfc_fx_cancel_scsi_cmd(pp, core_wk, target, 0, NULL, mb_resp_status,
					 HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);
			}
			if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
				hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, mb_resp_status, HFC_CSCSI_ERROR,
					TRUE, TRUE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
			}
			
			hfc_fx_clear_target_info_fx( pp, target, TRUE );	/* FCLNX-GPL-FX-112 */
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
			
			break;
		/* FCLNX-GPL-FX-289, 291 */

		default :
			break;
		}
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LOGORSP, 0x10, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_logo_resp");
}


/*
 * Function:    hfc_fx_gcs_id_resp
 *
 * Purpose:     This routine deals with GCS_ID response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_gcs_id_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	ushort				resp_code = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
//	uint64_t			class_of_service = 0 ;
	
	HFC_ENTRY("hfc_fx_gcs_id_resp");
	
	mbox = core->mb;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	
	if( target == NULL )
	{/* Target_info is not found */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_GCSIDRSP, 0x11, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return;
	}
	else
	{/* When target_info_fx is found */
		rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_GCSIDRSP, 0x00, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_GCSIDRSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			break;
		default :
			break;
		}
	}
	
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
		if(test_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status) )
		{
			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
		}
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
		atomic_set(&pp->check_mbreq, 1);
		break;

	case 0 :
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		break;

	default :
		break;
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_GCSIDRSP, 0x10, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_gcs_id_resp");
}


/*
 * Function:    hfc_fx_gid_pn_resp
 *
 * Purpose:     This routine deals with GID_PN response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_gid_pn_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	ushort				resp_code = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	uint				port_id=0;
	
	HFC_ENTRY("hfc_fx_gid_pn_resp");
	
	mbox = core->mb;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);
	
	if( target == NULL )
	{/* Target_info is not found */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_GIDPNRSP, 0x11, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return;
	}
	else
	{/* When target_info_fx is found */
		rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_GIDPNRSP, 0x12, pp, core->rp, core, target, NULL,
			(uint64_t)rc_passthrouh, 0, 0 );
		switch (rc_passthrouh)
		{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){
				
				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_GIDPNRSP, rc_passthrouh);  
			}
			
			HFC_DBGPRT("gidpn_resp() - responce fail.");
			mb_resp_status = SCS_NO_DEV_RESP;	
//			clear_bit(HFC_T_WAIT_GIDPN, (ulong *)&target->status);
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
			
			/* ABEND -> Was the device deleted?						*/
			/* Start Link Up waiting Timer between SW and device 	*/
			if (!test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status)) {
				set_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
				atomic_set(&pp->check_mbreq, 1);
				
				clear_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
				hfc_fx_enque_plogi_req(pp, target);	
				
				set_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
				hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);
				hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, FALSE);
				
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
					set_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
			}
			break;
		case HFC_MBPASS_SUCCESS :
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
			
			port_id = ((uint)hfc_fx_read_val( core -> payload->receive_payload.type.gxx.sub_type.gid_pn.port_id[0] ) << 16) +
				((uint)hfc_fx_read_val( core -> payload->receive_payload.type.gxx.sub_type.gid_pn.port_id[1] ) << 8) +
				 (uint)hfc_fx_read_val( core -> payload->receive_payload.type.gxx.sub_type.gid_pn.port_id[2] );

			mb_resp_status = 0 ;

			if ((test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
			 || (!test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags))) {

				clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
				set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
				if (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
					target->link_recovered = 1;
				hfc_fx_enque_plogi_req(pp, target);
			}

			/* Cancel link Up waiting timer between SW and device */
			if(pp->ld_err_limit_s)	/* FCLNX-GPL-349 */
			{
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
					if (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
						hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, target, HFC_OCCURED_FAILURE, HFC_TGT_LDS_ERR);
				} else {
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
					if( test_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags) )
#else
					if (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
#endif
						hfc_fx_watched_errcount_i(pp, target, HFC_TGT_LDS_ERR);	
				}
			}

			clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
			hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
//				clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif

			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);


			if ( target -> scsi_id != port_id ) { /* Target SCSI ID is mismatched with cuerrent ID */
				HFC_DBGPRT("gidpn_resp() - scsi_id change (0x%llx -> 0x%llx)",
					(unsigned long long)target->scsi_id, (unsigned long long)port_id);

				target -> scsi_id = port_id;	/* Switch SCSI ID */

				/* Initiate LOGIN (Clear target state of SCSI response wait) */
				set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
				clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
				hfc_fx_enque_plogi_req(pp, target);
			}
			else {
				if (test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status)) {
					set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
					atomic_set(&pp->check_mbreq, 1);
					clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
					hfc_fx_enque_plogi_req(pp, target);
				}
				else if (test_bit(HFC_TS_SCN_RESP, (ulong *)&target->status) ||
						 test_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status) ){
					set_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
					hfc_fx_enque_pdisc_req(pp, target);
				}
			}

			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
						
			break;
		default :
			break;
		}
	}
	
	core->mb_retry_cnt = 0;

	HFC_DBGPRT("hfc_fx_gid_pn_resp mailbox dump\n");
	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
	HFC_DBGPRT("hfc_fx_gid_pn_resp mailbox payload dump\n");
	structdump( 0xee, (uchar *)core->payload, 4096 );

	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :												/* FCWIN-0151 */
		if(test_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status) )
		{
			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
		}
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
		atomic_set(&pp->check_mbreq, 1);
		clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-038 */	/* FCLNX-GPL-FX-014 */
		break;

	case 0 :
		clear_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
		break;

	default :
		break;
	}
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_gid_pn_resp");
}

/*
 * Function:    hfc_fx_gpn_id_resp
 *
 * Purpose:     This routine deals with GPN_ID response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_gpn_id_resp(struct port_info *pp, struct core_info *core)
{
#if 0
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
	ushort				resp_code = 0 ;
	uchar				reason_code = 0, code_explanation = 0 ;
	ushort				payload_length=0;
	
	HFC_ENTRY("hfc_fx_gpn_id_resp");
	
	mbox = core->mb;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
	hfc_fx_hand2_trace(
		HFC_FX_TRC_GPNIDRSP, 0x12, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_GPNIDRSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			break;
		default :
			break;
	}
	
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
//	HFC_DBGPRT("hfc_fx_gpn_id_resp mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_gpn_id_resp mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );

//	clear_bit(HFC_PD_WAIT_GPNID, (ulong *)&pp->status);
	hfc_fx_wwnverify_gpnid(pp, NULL, core, mb_resp_status);

	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_gpn_id_resp");
#endif
}

/*
 * Function:    hfc_fx_gid_ft_resp
 *
 * Purpose:     This routine deals with GID_FT response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_gid_ft_resp(struct port_info *pp, struct core_info *core)
{
#if 0
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
	ushort				resp_code = 0 ;
	uchar				reason_code = 0, code_explanation = 0 ;
	ushort				payload_length=0;
	struct payload_fx	*pyload = NULL;
	
	HFC_ENTRY("hfc_fx_gid_ft_resp");
	
	HFC_DBGPRT("hfc_fx_gid_ft_resp\n");
	
	mbox = core->mb;
	pyload = core->payload;
	
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_GIDFTRSP, 0x12, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );

	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(	(rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			){

				hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_GIDFTRSP, rc_passthrouh);
			}
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			pp->used_nmsrv = 0;
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			pp->used_nmsrv = 1;
			break;
		default :
			break;
	}
	
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
//	HFC_DBGPRT("hfc_fx_gid_ft_resp mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_gid_ft_resp mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );
	
	clear_bit(HFC_PD_WAIT_GIDFT, (ulong *)&pp->status_detail2);
	hfc_fx_wwnverify_gidft(pp, NULL, core, mb_resp_status);
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_gid_ft_resp");
#endif
}

/*
 * Function:    hfc_fx_rft_id_resp
 *
 * Purpose:     This routine deals with RFT_ID response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_rft_id_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	ushort				resp_code = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	struct payload_fx	*pyload = NULL;
	
	HFC_ENTRY("hfc_fx_rft_id_resp");
	
	HFC_DBGPRT("hfc_fx_rft_id_resp\n");
	
	mbox = core->mb;
	pyload = core->payload;
	
    /*-- Get scsi_id of Fabric # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );

	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
	hfc_fx_hand2_trace(
		HFC_FX_TRC_RFTIDRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_RFTIDRSP, rc_passthrouh);
			}else{	/* FCLNX-GPL-FX-058 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				clear_bit(HFC_PD_WAIT_RFTID, (ulong *)&pp->status_detail1);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			break;
		default :
			break;
	}
	
//	HFC_DBGPRT("hfc_fx_rft_id_resp mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_rft_id_resp mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :
	case 0 :
		clear_bit(HFC_PD_WAIT_RFTID, (ulong *)&pp->status_detail1);
		set_bit(HFC_PD_NEED_RFFID, (ulong *)&pp->status_detail1);
		atomic_set(&pp->check_mbreq, 1);
		break;

	default :
		break;
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_RFTIDRSP, 0x10, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_rft_id_resp");
}


/*
 * Function:    hfc_fx_rff_id_resp
 *
 * Purpose:     This routine deals with RFF_ID response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_rff_id_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
	uint				scsi_id=0;
//	ushort				resp_code = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	struct payload_fx	*pyload = NULL;
	
	HFC_ENTRY("hfc_fx_rff_id_resp");

	HFC_DBGPRT("hfc_fx_rff_id_resp\n");
	
	mbox = core->mb;
	pyload = core->payload;
	
    /*-- Search target info by scsi_id # */
	scsi_id = ( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[0] ) << 16 ) +
							( (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[1] ) << 8 ) +
							  (uint) hfc_fx_read_val( mbox->mb_init.type.frmsndrcv.fcph_hdr.d_id[2] );
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
		
	hfc_fx_hand2_trace(
		HFC_FX_TRC_RFFIDRSP, 0x00, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			return;
		case HFC_MBPASS_TIMEDOUT :
		unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */

				hfc_fx_mb_errlog(pp, NULL, core, HFC_FX_TRC_RFFIDRSP, rc_passthrouh);
			}else{	/* FCLNX-GPL-FX-058 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				clear_bit(HFC_PD_WAIT_RFFID, (ulong *)&pp->status_detail1);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			break;
		case HFC_MBPASS_SUCCESS :
			mb_resp_status = 0 ;
			break;
		default :
			break;
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
//	HFC_DBGPRT("hfc_fx_rff_id_resp mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_rff_id_resp mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );
	
	switch(mb_resp_status)
	{
	case SCS_NO_DEV_RESP :
	case 0 :
		clear_bit(HFC_PD_WAIT_RFFID, (ulong *)&pp->status_detail1);
		hfc_fx_copy_master_to_slave( pp, core );
		hfc_fx_change_portstat_linkup(pp, core );
		break;

	default :
		break;
	}
	
	/* Shadow LPAR should not create target device object */
	/* So, Shadow LPAR skips the routine of device creation */
	if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
		hfc_fx_wwnverify_linkup(pp, NULL, core, mb_resp_status, 0);
	}
	else {
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_RFFIDRSP, 0x10, pp, core->rp, core, NULL, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_rff_id_resp");
}


/*
 * Function:    hfc_fx_gpn_ft_resp
 *
 * Purpose:     This routine deals with GPN_FT response
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  TRUE       - Initiation completed successufully.
 *  not zero   - Interrupt from other device.
 *
 * Notes:       
 */
void hfc_fx_gpn_ft_resp(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx 	*target = NULL ;
	int                 mb_resp_status = 0 ;
	uint                rc_passthrouh = 0xffffffff;
//	uint				scsi_id=0;
//	ushort				resp_code = 0 ;
//	uchar				reason_code = 0, code_explanation = 0 ;
//	ushort				payload_length=0;
	struct payload_fx	*pyload = NULL;
	
	HFC_ENTRY("hfc_fx_gpn_ft_resp");
	
	HFC_DBGPRT("hfc_fx_gpn_ft_resp\n");
	
	mbox = core->mb;
	pyload = core->payload;
	
	rc_passthrouh = hfc_fx_mb_passthrough_rsp(pp, core, HFC_MB_INTL);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_GPNFTRSP, 0x00, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );

	switch (rc_passthrouh)
	{
		case HFC_MBPASS_WAIT_RETRY :
			HFC_DBGPRT("hfc_fx_gpn_ft_resp, HFC_MBPASS_WAIT_RETRY %d \n",rc_passthrouh);
			return;
		case HFC_MBPASS_TIMEDOUT :
			HFC_DBGPRT("hfc_fx_gpn_ft_resp, HFC_MBPASS_TIMEDOUT %d \n",rc_passthrouh);
			unlock_fx_mailbox(pp);
			return;
		case HFC_MBPASS_RETRY_OVER :
		case HFC_MBPASS_RETRY_FAIL :
		case HFC_MBPASS_ERROR :
		case HFC_MBPASS_RETRY_OVER_GPNFT :	/* FCLNX-GPL-FX-139 */
		case HFC_MB_FATAL :
			/* FCLNX-GPL-069 */
			if(((rc_passthrouh == HFC_MBPASS_RETRY_OVER)	||
				(rc_passthrouh == HFC_MBPASS_RETRY_FAIL)	||
				(rc_passthrouh == HFC_MBPASS_ERROR)			) &&
				(pp->linkdown_occurred == 0) ){	/* FCLNX-GPL-FX-174 */
					hfc_fx_mb_errlog(pp, target, core, HFC_FX_TRC_GPNFTRSP, rc_passthrouh);
			}else if(rc_passthrouh != HFC_MBPASS_RETRY_OVER_GPNFT){	/* FCLNX-GPL-FX-058 *//* FCLNX-GPL-FX-139 */
				pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
				clear_bit(HFC_PD_WAIT_GPNFT, (ulong *)&pp->status_detail2);
				hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
				unlock_fx_mailbox(pp);
				return;
			}	/* FCLNX-GPL-FX-058 */
			HFC_DBGPRT("hfc_fx_gpn_ft_resp, HFC_MB_FATAL %d \n",rc_passthrouh);
			mb_resp_status = SCS_NO_DEV_RESP;					/* FCWIN-0151 */
			pp->used_nmsrv = 0;
			break;
		case HFC_MBPASS_SUCCESS :
			HFC_DBGPRT("hfc_fx_gpn_ft_resp, HFC_MBPASS_SUCCESS %d \n",rc_passthrouh);
			mb_resp_status = 0 ;
			pp->used_nmsrv = 1;
			break;
		default :
			break;
	}
	
	pp->linkdown_occurred = 0;	/* FCLNX-GPL-FX-174 */
	core->mb_retry_cnt = 0;
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	
//	HFC_DBGPRT("hfc_fx_gid_ft_resp mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_gid_ft_resp mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );
	
	clear_bit(HFC_PD_WAIT_GPNFT, (ulong *)&pp->status_detail2);
	hfc_fx_wwnverify_gpnft(pp, NULL, core, mb_resp_status);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_GPNFTRSP, 0x10, pp, core->rp, core, target, NULL,
		(uint64_t)rc_passthrouh, 0, 0 );
	
	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_gpn_ft_resp");
}


/*
 * Function:    hfc_fx_mb_intreq
 *
 * Purpose:     
 *	This routine deals with mailbox interrupt 
 *	interrupt factors are followings;
 *	1) Correctable_error was received
 *	2) Online Update was received
 *	3) Receive_Frame was received
 *	4) PLOGI was received
 *	5) Link Up was received
 *	6) Link Down was received
 *	7) FLOGI was received
 *	8) PDISC was received
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mb_intreq(struct port_info *pp, struct core_info *core, struct region_info *rp)
{
	struct mailbox_fx		*mbox ;
	struct port_info		*vpp;
	uint					mb_resp ;
	uchar					mb_int0=0, mb_int1=0;
	ushort					mb_code;
	int						i;
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_MBINT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_mb_intreq");
	
	HFC_DBGPRT("hfcldd%d hfc_fx_mb_intreq start\n",pp->dev_minor);
	
	/* Setup mailbox pointer */
	mbox = core->mb;
	mb_int0 = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	mb_int1 = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	mb_code = (ushort)(mb_int0 << 8) + (ushort)(mb_int1);
	
	hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBINT, 0);	/* FCLNX-GPL-FX-139 */
	
	HFC_DBGPRT("hfcldd%d hfc_fx_mb_intreq mb_code = %04x\n",pp->dev_minor, mb_code);
	
	if( mb_int0 == 0xa0 )
	{
		mb_resp = hfc_fx_frame_a_data(mbox, pp->pkg.type, (ushort)mb_code, (uchar)pp->rid);
		
		switch( mb_code )
		{
			case HFC_MBINTREQ_DCE :		hfc_fx_correctabled_error_intreq(pp, core);		break;
			case HFC_MBINTREQ_COU :		hfc_fx_comp_online_update_intreq(pp, core);		break;
		}

		/*-- Response of mailbox --*/
//		hfc_fx_write_reg_ext(pp,( uint )hfc_framea_of_core[core->core_no],( char )0x4, ( int )mb_resp );
		hfc_fx_write_reg_core(pp, core->core_no, (uint)HFC_IOSPACE_FRAMEA,
							(char)0x4, (int)mb_resp, HFC_FX_CORE_OFFSET40);
	}
	else if( mb_int0 == 0xb0  )
	{
		mb_resp = hfc_fx_frame_a_data(mbox, pp->pkg.type, (ushort)mb_code, (uchar)pp->rid);
		
		HFC_DBGPRT("hfcldd%d hfc_fx_mb_intreq mb_resp = %04x\n",pp->dev_minor, mb_resp);

		switch( mb_code )
		{
			case HFC_MBINTREQ_LINKUP :
				if (HFC_FX_EXT_VPORT_EXIST(pp->region_arg[pp->rid])) {
					for (i=0; i<=pp->pport->max_vport_count; i++) {
						vpp = pp->pport->vport_ptr[i].vport_arg;
						if (vpp == NULL)
							continue;
						
						if (vpp->rid != pp->rid)
							continue;
						
						hfc_fx_linkup_intreq(vpp, core);
					}
				}
				else if (HFC_FX_MQ_VALID(pp)) {
					if (HFC_FX_PHYSICAL_PORT(pp)) {
						for (i=0; i<=pp->max_vport_count; i++) {
							vpp = pp->pport->vport_ptr[i].vport_arg;
							if (vpp == NULL)
								continue;
							if (!test_bit(HFC_PS_ENABLE, (ulong *)&vpp->status))
								continue;
							if (test_bit(HFC_PD_NEED_CORE_START, (ulong *)&vpp->status_detail1))
								continue;
							
							hfc_fx_linkup_intreq(vpp, vpp->region_arg[vpp->rid]->core_arg[core->core_no]);	/* FCLNX-GPL-FX-206 */
							if (HFC_FX_PHYSICAL_PORT(vpp)) {	/* FCLNX-GPL-FX-246,272 */
								if ((vpp->connect_type == HFC_FX_AL)||(vpp->connect_type == HFC_FX_MULTI_ALPA)) {
									HFC_DBGPRT("hfcldd hfc_fx_mb_intreq HFC_FX_AL or HFC_FX_MULTI_ALPA\n");
									break;
								}
							}	/* FCLNX-GPL-FX-246,272 */
						}
					}
				}
				else {
					hfc_fx_linkup_intreq(pp, core);
				}
				break;
			case HFC_MBINTREQ_LINKDOWN :
				if (HFC_FX_EXT_VPORT_EXIST(pp->region_arg[pp->rid])) {
					for (i=0; i<=pp->pport->max_vport_count; i++) {
						vpp = pp->pport->vport_ptr[i].vport_arg;
						if (vpp == NULL)
							continue;
						
						if (vpp->rid != pp->rid)
							continue;
						
						hfc_fx_linkdown_intreq(vpp, core, 0);
					}
				}
				else if (HFC_FX_MQ_VALID(pp)) {
					if (HFC_FX_PHYSICAL_PORT(pp)) {
						for (i=0; i<=pp->max_vport_count; i++) {
							vpp = pp->pport->vport_ptr[i].vport_arg;
							if (vpp == NULL)
								continue;
							if (!test_bit(HFC_PS_ENABLE, (ulong *)&vpp->status))
								continue;
							if (test_bit(HFC_PD_NEED_CORE_START, (ulong *)&vpp->status_detail1))
								continue;
							
							hfc_fx_linkdown_intreq(vpp, vpp->region_arg[vpp->rid]->core_arg[core->core_no], 0);	/* FCLNX-GPL-FX-206 */
						}
					}
					else {
						hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKDOWN );
					}
				}
				else {
					hfc_fx_linkdown_intreq(pp, core, 0);
				}
				break;
			case HFC_MBINTREQ_RCVFRM :		hfc_fx_receive_frame_intreq(pp,core, mbox);	break;
			case HFC_MBINTREQ_RCVFLOGI :	hfc_fx_receive_flogi_intreq(pp,core, mbox);	break;
			case HFC_MBINTREQ_RCVPLOGI :	hfc_fx_receive_plogi_intreq(pp,core, mbox);	break;
			case HFC_MBINTREQ_RCVPDISC :	hfc_fx_receive_pdisc_intreq(pp,core, mbox);	break;
		}
		
		/*-- Response of mailbox --*/		
//		hfc_fx_write_reg_ext( pp, ( uint )hfc_framea_of_core[core->core_no], ( char )0x4, ( int )mb_resp );
		hfc_fx_write_reg_core(pp, core->core_no, (uint)HFC_IOSPACE_FRAMEA,
							(char)0x4, (int)mb_resp, HFC_FX_CORE_OFFSET40);
	}
	else /* Err Case */
	{
		hfc_fx_hand2_trace(
			HFC_FX_TRC_MBINT, 0x80, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		
		if( test_bit(HFC_LPTEST_ALRDY_INTREQ, (ulong *)&pp->io_status) )
		{
			clear_bit(HFC_LPTEST_ALRDY_INTREQ, (ulong *)&pp->io_status);
		} 
		else 
		{
			/*-- rsp_code=0xff --*/
			mb_resp = hfc_fx_frame_a_data(mbox, pp->pkg.type, (ushort)mb_code, (uchar)pp->rid);
			
			memset(core->logdata, 0, 16 );
//			memcpy(&core->logdata[0], &(core->mb->mb_intreq.type.resvx[16]), 16);

			hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINT,
						ERRID_HFCP_EVNT3, 0x1c, core->logdata, 16 );
			
			/*-- Response of mailbox --*/
//			hfc_fx_write_reg_ext( pp, ( uint )hfc_framea_of_core[core->core_no], 0x4, ( int )mb_resp );
			hfc_fx_write_reg_core(pp, core->core_no, (uint)HFC_IOSPACE_FRAMEA,
								  (char)0x4, (int)mb_resp, HFC_FX_CORE_OFFSET40);

			hfc_fx_hand2_trace(
				HFC_FX_TRC_MBINT, 0x81, pp, core->rp, core, NULL, NULL,
				0, 0, 0);
		}
	}

//	if( !test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ) ) {
//		start_fx_next_mailbox( pp, NULL);
//	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_MBINT, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_EXIT("hfc_fx_mb_intreq");
	
	HFC_DBGPRT("hfcldd%d hfc_fx_mb_intreq exit\n",pp->dev_minor);

	return;
}


/*
 * Function:    hfc_fx_correctabled_error_intreq
 *
 * Purpose:     Logging and Counter management
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_correctabled_error_intreq(struct port_info *pp, struct core_info *core)
{
	/* Constants */
	const struct mailbox_fx *mbox = core->mb;
	/* Values */
	struct hfc_fw_err1bit_fx *ptr = NULL;
	uint	far=0, i, wk;
	uint64_t time = 0;
	uchar	fsynd=0;

	/* Set Log Data */ /* FCLNX-GPL-116 */
	memset(core->logdata, 0, 16 );

	/* Check HBA Package Type */
	if (pp->max_core_ce_cnt != 0) { /* Check the parameter */
		if (core->core_ce_cnt < HFC_FX_FW_1BIT_LOG_ENTRY) {
			/*** Get Log Data ***/
			ptr = (struct hfc_fw_err1bit_fx *) &pp->ce_fw_log.core_fw_err1bit[core->core_no][core->core_ce_cnt];
			/* Get now time */
			time = get_jiffies_64(); /* get jiffies_64 */
			HFC_8L_TO_8B(ptr->time_stamp, time);
			/* Get Addr(Defect has hpppend in this addr.) */
			far = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.far[0] ) << 16 ) +
						( (uint) hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.far[1] ) << 8 ) +
						  (uint) hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.far[2] );
			ptr->core_fw_far = far; 
			fsynd = hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.fsynd );
			far |= (uint)(fsynd << 24);
			
			hfc_fx_write_val( ptr->core_fw_far, far );
			
			/* Count up */
			core->core_ce_cnt++;
			hfc_fx_write_val( pp->ce_fw_log.core_ce_cnt[core->pcore_no], core->core_ce_cnt );
			
			HFC_DBGPRT("hfcldd%d hfc_fx_correctabled_error_intreq core#%d ce_cnt = %d time = %llx far = %08x fsynd = %02x\n", pp->dev_minor,
			  core->core_no, core->core_ce_cnt, time, far, fsynd);
			
			if ( core->core_ce_cnt == pp->max_core_ce_cnt ) {
				/* Count over */ /* log out only 1 time */
				core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
				core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
				
				core->logdata[4] = fsynd;
				core->logdata[5] = hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.far[0] );
				core->logdata[6] = hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.far[1] );
				core->logdata[7] = hfc_fx_read_val( mbox->mb_intreq.type.detect_correctable_error.far[2] );
				
				/*** Set SRAM CE Log ***/
				/* Set ErrNo */
				for(i=0; i<3; i++){
					wk = 0xfffe1100+i;
					hfc_fx_write_val( pp->ce_log[i].err_num, wk );
				}
				hfc_fx_write_val( pp->ce_fw_log.err_num, 0xfffe1103 );
				
				hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_SRAMCE,
					ERRID_HFCP_ERR2, 0xa4, core->logdata, 16 );
			}
		}
	}

	return;
}


/* Function:    hfc_fx_comp_online_update_intreq
 *
 * Purpose:     Process F/W online update completion interruption
 *
 * Arguments:
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:	None (void)
 *
 * Notes:
 */
void hfc_fx_comp_online_update_intreq(struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mbox = NULL ;
	uchar                   hfc_buf[4];	/* FCLNX-GPL-FX-456 */

	mbox = core->mb;
	memset(core->logdata, 0, 16 );
	core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	
	memcpy(&core->logdata[4],(char *)&mbox->mb_intreq.type.complete_online_update.sysrev, 4) ;

	hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
		ERRID_HFCP_EVNT4, 0xA7, core->logdata, 16 );
	
	if(HFC_FX_MMODE_CHECK_SHADOW(pp)){	/* FCLNX-GPL-FX-456 */
		if(!hfc_fx_read_flash(pp, 0, 4, hfc_buf) ){
			hfc_fx_mlpf_set_mmio_hg(pp, hfc_buf, HFC_IOHGSPC_SYSREV0, HFC_IOHGSPC_SYSREV_LEN);
		}
	}	/* FCLNX-GPL-FX-456 */

}


/*
 * Function:    hfc_fx_linkdown_intreq
 *
 * Purpose:     This routine deals with Linkdown interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *  mig        - 1:Execute LPAR Migration process
 *             - 0:No execute LPAR Migration process
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_fx_linkdown_intreq(
	struct port_info *pp,
	struct core_info *core,
	uchar	mig)
{
	struct target_info_fx		*target;
	struct mailbox_fx		*mbox;
	uint                    lp=0, i=0, status=0;	/* FCLNX-GPL-FX-407 */
	ushort                  detail;
	struct dev_info_fx			*dev=NULL;
	uchar isol_detail = 0, keep_mblock=0;	/* FCLNX-GPL-FX-161 */
	struct core_info	*wk_core=NULL;

	HFC_ENTRY("hfc_fx_linkdown_intreq");
	
	HFC_DBGPRT("hfcldd%d hfc_fx_linkdown_intreq start\n",pp->dev_minor);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LINKDOWN_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	
	mbox = core->mb;
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {	/* FCLNX-GPL-206 */
		mbox = pp->pport->region_arg[0]->core_arg[core->core_no]->mb;
	}
	
																	/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )	/* FCLNX-GPL-FX-407 */
	{
		status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
		status &= ~HFC_HG_LPRSTATUS_UNSHARABLE;
		status |= HFC_HG_LPRSTATUS_LINKDOWN;
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
			status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );
	}	/* FCLNX-GPL-FX-407 */
																	/* @MLPF END */
	/* Update the adaptor state */
	set_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);
	if( (HFC_FX_MMODE_CHECK_SHARED(pp)) && (!HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){	/* FCLNX-GPL-FX-386 Start */
		set_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 );
	}
	
	clear_bit(HFC_PS_CONNECTED, (ulong *)&pp->status);
	pp->linkdown_occurred = 1 ;	/* FCLNX-GPL-FX-174 */
	
	/* Stops LOGIN_DELAY_TMR	*/
	hfc_fx_watchdog_enter(pp, core, 0, NULL, 0, HFC_FX_LOGIN_DELAY_TMR, 0, TRUE);		/* FCLNX-GPL-038 */
	clear_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 );						/* FCLNX-GPL-038 */
	
	/* STOP mailbox retry timer per port */
	hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
	if(test_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1)){
		clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {	/* FCLNX-GPL-FX-161 Start */
			if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if(hfc_fx_check_cs_disable(pp, wk_core))
				continue;	/* FCLNX-GPL-FX-438 */
			if(test_bit(HFC_CS_MB_RETRY_DELAY, (ulong *)&wk_core->status)){
				hfc_fx_w_stop(pp, wk_core, HFC_FX_MB_RETRY_DELAY_TMR);
				clear_bit(HFC_CS_MB_RETRY_DELAY, (ulong *)&wk_core->status);
				clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&wk_core->status);
			}else if(test_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&wk_core->status)){
				/* A Mailbox is issued. */
				keep_mblock = 1;
			}
		}
		if(keep_mblock == 0){
			HFC_CLEAR_PD_WAIT(pp);
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				target = pp->target_arg[i];
				if(target == NULL) break;
				HFC_CLEAR_TS_WAIT(target);
			}
			unlock_fx_mailbox(pp);
		}
	}	/* FCLNX-GPL-FX-161 End */
	
	/* Calcel all waiting queue which is reserved under the device of target adapter */
	if(!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){	/* FCLNX-GPL-FX-067 */
		if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){	/* FCLNX-GPL-FX-058 */
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)					/* FCWIN-0083 */
			{
				target = hfc_fx_hash_target_info(pp, lp);
				
				if(target != NULL)
				{
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
							continue;
						hfc_fx_cancel_xob(pp, wk_core, target,0,NULL,HFC_FLASH_TARGET) ;	/* FCLNX-GPL-FX-058 */
						hfc_fx_cancel_weque(pp,wk_core,target,0,NULL,SCS_INTR_LINKDOWN,HFC_CSCSI_ERROR,HFC_FLASH_TARGET) ;	/* FCLNX-GPL-FX-058 */
					}
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_DELAY_TMR, 0, TRUE);	/* FCWIN-0082 */
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE); /* FCWIN-0146 */
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE); /* FCLNX-GPL-FX-190 */
					clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);						/* FCLNX-GPL-FX-190 */
					dev = target->dev;															/* FCLNX-GPL-038	*/
					if(dev != NULL) {
						/* stop LUN Reset Delay Timer */
//						hfc_manage_info.npubp->hfc_fx_all_clear_dev_info_fx(pp, dev);					/* FCLNX-GPL-0343	*/
						hfc_fx_all_clear_dev_info_fx(pp, dev);										/* FCLNX-GPL-0343	*/
					}																			/* FCLNX-GPL-038	*/
					target->status = HFC_NON_STATUS;
				}
			}
		}
	
		if (!HFC_FX_MQ_VIRTUAL_PORT(pp)) {
			hfc_fx_w_stop( pp, core, HFC_FX_LINKUP_TMR );
			hfc_fx_w_start( pp, core, HFC_FX_LINKUP_TMR, pp->linkup_tmo );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
				set_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, TRUE);	
				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, FALSE);
			}
#endif	/* FCLNX-GPL-FX-424 */
		}
	}
	
	HFC_DETAIL_CLEAR_LINKDOWN(pp);
	hfc_fx_copy_master_to_slave( pp, core );	/* FCLNX-GPL-FX-18 */
	if (!HFC_FX_MQ_VIRTUAL_PORT(pp)) {
		hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKDOWN );
	}
	memset(core->logdata,0,16);
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LINKDOWN_INT, 0x11, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
//	memcpy(&core->logdata[0], &(core->mb->mb_intreq.type.resvx[16]), 16) ;

	detail = hfc_fx_read_val( mbox->mb_intreq.type.linkdown.down_detail );
	
	HFC_DBGPRT("hfcldd%d hfc_fx_linkdown_intreq detail = %04x\n",pp->dev_minor, detail);
	
	if(!mig){															/* FCLNX-GPL-489 */
		if ( detail == HFC_SFP_NOT_SUPPORT ){
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_OPTERR0, 0x9c,core->logdata,16);
			set_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)&pp->status_detail2);
			isol_detail = 1;
		}
		else if ( detail == HFC_SFP_RECV_ERR ){
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR2, 0x9d,core->logdata,16);	/* FCLNX-GPL-FX-194 */
			set_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->status_detail2);
			isol_detail = 1;
		}
		else if ( detail == HFC_SFP_TRAN_ERR ){
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR2, 0x9e,core->logdata,16);	/* FCLNX-GPL-FX-194 */
			set_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->status_detail2);
			isol_detail = 1;
		}
		else if ( detail == HFC_SFP_DOWN ){
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR2, 0x9f,core->logdata,16);	/* FCLNX-GPL-FX-194 */
			set_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)&pp->status_detail2);
			isol_detail = 1;
		}
		else if ( detail == HFC_MCK_PCHK ){									/* FCLNX_GPL-0109 */
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERR9, 0xad,core->logdata,16);
		}
		else if( !test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) ){
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_ERRB, 0x14,core->logdata,16);
		}
		else
			HFC_DBGPRT("hfcldd%d : FC Port Status is Link-down. \n", pp->dev_minor);
		
		hfc_fx_change_vport_isol_state(pp);
	}
	if (( isol_detail ) || ( detail == HFC_MCK_PCHK )|| (mig) ) {			/* FCLNX_GPL-0109 *//* FCLNX-GPL-489 */
		if ( isol_detail!=0 ) {
			set_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status);
			hfc_fx_change_vport_isol_state(pp);
		}
		
		if (!HFC_FX_MQ_VIRTUAL_PORT(pp)) {
			hfc_fx_w_stop( pp, core, HFC_FX_LINKUP_TMR );
			hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_MCK_TMR );								/* FCLNX_GPL-0109 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
				hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_CNT_TMR );	
#endif	/* FCLNX-GPL-FX-424 */
		}
		
		clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
		if (( detail == HFC_MCK_PCHK )||( !test_bit(HFC_PS_ISOL, (ulong *)&pp->status )) ){									/* FCLNX_GPL-0109 *//* FCLNX-GPL-489 */
			set_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status );
		}
		
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++){
			target = hfc_fx_hash_target_info(pp, lp);
			if( target != NULL ){
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					hfc_fx_cancel_scsi_cmd(pp, core, target, 0, NULL, SCS_INTR_LINKDOWN,
						 HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);	/* FCLNX-0553 */
				}
				target->status = HFC_NON_STATUS;
			}
		}
		
		hfc_fx_copy_master_to_slave( pp, core );	/* FCLNX-GPL-FX-18 */
		hfc_fx_change_portstat_linkdown(pp, pp->region_arg[pp->rid]->core_arg[pp->master_core_no]);
		hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);
		if (!HFC_FX_MQ_VIRTUAL_PORT(pp)) {
			hfc_fx_w_stop( pp, core, HFC_FX_LINKUP_TMR );					/* FCLNX-GPL-FX-067 */
			hfc_fx_w_start( pp, core, HFC_FX_LINKUP_TMR, pp->linkup_tmo );	/* FCLNX-GPL-FX-067 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
				set_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, TRUE);
				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_WLINKUP_CNT_TMR, 0, FALSE);
			}
#endif	/* FCLNX-GPL-FX-424 */
		}
		
		if( (test_bit( HFC_PD_WAIT_LINK_INI, (ulong *)&pp->status_detail1)) || (pp->initialize == 1) )	/* FCLNX-0514 */
		{					
			HFC_DBGPRT(KERN_ERR "hfcldd%d : hfc_fx_watchdog() lock fail. \n", pp->dev_minor);
//			unlock_fx_mailbox( pp ) ;
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
//			clear_bit( HFC_PD_WAIT_LINK_INI, (ulong *)&pp->status_detail1);
		}
		
		if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )
		{
			if(test_bit(HFC_PS_ISOL, (ulong *)&pp->status)){	/* FCLNX-GPL-489 */
				if(test_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->status_detail2)){
					hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_SFP_FAIL, HFC_ENABLE_LPAR_STATE);
				}else if(test_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)&pp->status_detail2)){
					hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_SFP_NOTSUPT, HFC_ENABLE_LPAR_STATE);
				}else if(test_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)&pp->status_detail2)){
					hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_SFP_DOWN, HFC_ENABLE_LPAR_STATE);
				}
			}							/* FCLNX-GPL-489 */
		}
	}

	HFC_EXIT("hfc_fx_linkdown_intreq");
	
	HFC_DBGPRT("hfcldd%d hfc_fx_linkdown_intreq exit\n",pp->dev_minor);
	
	return ;
}


/*
 * Function:    hfc_fx_linkup_intreq
 *
 * Purpose:     This routine deals with Linkup interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_fx_linkup_intreq(
	struct port_info        *pp,
	struct core_info		*core)
{
//	struct mp_adap_info     *mpap;
	struct mailbox_fx       *mbox;
	uint					i, count, status=0;	/* FCLNX-GPL-FX-407 */
	uchar					flag, loop_direct=1, domain=0;	/* FCLNX-GPL-FX-436 */
	struct core_info	*wk_core=NULL;	/* FCLNX-GPL-FX^067 */
	
	HFC_ENTRY("hfc_fx_linkup_intreq");	
	
	HFC_DBGPRT("hfcldd%d hfc_fx_linkup_intreq start\n",pp->dev_minor);

//	mpap = pp->mp_adap_info;
	mbox = core->mb;
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {	/* FCLNX-GPL-206 */
		mbox = pp->pport->region_arg[0]->core_arg[core->core_no]->mb;
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LINKUP_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) )
		hfc_fx_mlpf_cca_setup(pp);		/* FCLNX-GPL-494 */
	
	/* Store F/W init table information to port_info */
	hfc_fx_copy_iocinfo(pp, core);									/* FCWIN-0082 */

	hfc_fx_cancel_xrb(pp, NULL, HFC_FLASH_ADAP);	/* FCLNX-GPL-FX-228,272 */
	
	if(pp->ld_err_limit_s){	/* FCLNX-GPL-349 */
		/* FCLNX-GPL-FX-424 */
		if((test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)||test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)||test_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1))	/* FCLNX-0506 */
		&&(!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)))	/* FCLNX-GPL-FX-161 */
		{
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_LDS_ERR);	/* FCLNX-GPL-349 */
			}
			else{
				hfc_fx_watched_errcount_i(pp, NULL, HFC_LDS_ERR);	/* FCLNX-GPL-349 */
			}
		}
	}

	if((test_bit(HFC_PS_ISOL, (ulong *)&pp->status))	/* FCLNX-GPL-FX-143 */
	&& (pp->status_detail2 & HFC_PD2_SFP_ERROR_ANY)){	/* FCLNX-GPL-FX-067 *//* FCLNX-GPL-FX-143 */
		HFC_DETAIL_CLEAR_ISOLREC(pp);
		clear_bit(HFC_PS_ISOL, (ulong *)&pp->status);	/* FCLNX-GPL-FX-143 */
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			wk_core->mck_err_cnt = 0;
		}
	}	/* FCLNX-GPL-FX-067 */
	
	set_bit(HFC_PS_CONNECTED, (ulong *)&pp->status);
	clear_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);	/* FCLNX-GPL-FX-005 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
#endif	/* FCLNX-GPL-FX-424 */
	if(!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
		set_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1);
	}
	
	core->fw_init_p->fw_iocinfo.connect_type = hfc_fx_read_val( mbox->mb_intreq.type.linkup.link_con_type );
	core->fw_init_p->fw_iocinfo.trans_rate = hfc_fx_read_val( mbox->mb_intreq.type.linkup.trans_rate );
	
	core->fw_init_p->fw_iocinfo.configure_flag = hfc_fx_read_val( mbox->mb_intreq.type.linkup.link_config_flag );
	flag = core->fw_init_p->fw_iocinfo.configure_flag;
	
	/* FCLNX-GPL-FX-005 */
	if( !test_bit(HFC_DIAG_END , (ulong *)&pp->status) )
	{
		if( !test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) ){
			memset(core->logdata,0,16);
			hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2,0x1b,core->logdata,16) ;
		}
		else{
			HFC_DBGPRT("hfcldd%d : FC Port Status is Link-up. \n", pp->dev_minor);
		}
	}
	else
	{
		clear_bit(HFC_DIAG_END , (ulong *)&pp->status);
	}
	
	set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);
	/* FCLNX-GPL-FX-005 */
	
	switch( core->fw_init_p->fw_iocinfo.connect_type ){
		case HFC_FX_AL :
		case HFC_FX_MULTI_ALPA:
			core->fw_init_p->fw_iocinfo.alpa_count = hfc_fx_read_val( mbox->mb_intreq.type.linkup.alpa_count );
			if( flag & HFC_FX_POSMAP_VALID ){
				count = (uint) hfc_fx_read_val(mbox->mb_intreq.type.linkup.position_map[0]);
				core->fw_init_p->fw_iocinfo.assign_alpa = hfc_fx_read_val( mbox->mb_intreq.type.linkup.assign_alpa );
				if( count > 0){
					core->fw_init_p->pos_map[0] = mbox->mb_intreq.type.linkup.position_map[0];
					for (i=1;i<=count; i++) {
						core->fw_init_p->pos_map[i] = mbox->mb_intreq.type.linkup.position_map[i];
						if( core->fw_init_p->pos_map[i] == 0x00 ){
							loop_direct = 0;
						}
					}
				}
			}
			if( flag & HFC_FX_ALPA_VALID ){
				core->fw_init_p->fw_iocinfo.assign_alpa = hfc_fx_read_val(core->mb->mb_intreq.type.linkup.assign_alpa );
				pp->scsi_id = (uint64_t)hfc_fx_read_val(core->mb->mb_intreq.type.linkup.assign_alpa );

				core->fw_init_p->fw_iocinfo.self_port_id[0] = 0;
				core->fw_init_p->fw_iocinfo.self_port_id[1] = 0;
				core->fw_init_p->fw_iocinfo.self_port_id[2] = (uchar)hfc_fx_read_val(core->mb->mb_intreq.type.linkup.assign_alpa );
				core->fw_init_p->fw_iocinfo.configure_flag |= HFC_FX_PID_VALID;	/* FCLNX-GPL-FX-399 */
			}
			if( core->fw_init_p->fw_iocinfo.connect_type == HFC_MULTI_ALPA ){
				count = core->mb->mb_intreq.type.linkup.acquired_alpa[0];	/* FCLNX-GPL-FX-417 */
				if( count > 0){
					core->fw_init_p->active_alpa[0] = core->mb->mb_intreq.type.linkup.acquired_alpa[0];	/* FCLNX-GPL-FX-417 */
					for (i=1;i<=count; i++) {
						core->fw_init_p->active_alpa[i] = core->mb->mb_intreq.type.linkup.acquired_alpa[i];	/* FCLNX-GPL-FX-417 */
					}
				}
			}

			if( loop_direct == 1 ){	/* Loop Direct connection with I/O */
				pp->switch_exist = 0;
			}
			else{					/* Loop connection with Switch, AL_PA of switch : 0x00 */
				pp->switch_exist = HFC_SWITCH_EXIST;
			}
			break;

		default:
			loop_direct = 0;
			pp->switch_exist = HFC_SWITCH_EXIST;
			break;
	}

	/* F/W init table information is stored in port_info */

	HFC_DBGPRT("hfcldd : hfc_fx_linkup_intreq copy iocinfo port_info = %lx\n",(ulong)pp);

	hfc_fx_copy_iocinfo(pp, core);
	
	if( (HFC_FX_MMODE_CHECK_SHARED(pp)) && (!HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){	/* FCLNX-GPL-FX-386,436 Start */
		if( test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 )
		&& ((hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
		||  (!hfc_fx_mlpf_check_normal_hypsts(pp)))){
			goto mlpf_guest_skip;
		}
	}	/* FCLNX-GPL-FX-386 End */
	
	if( loop_direct == 1 )
	{
		/* Loop Direct connection with I/O */
		hfc_fx_copy_master_to_slave( pp, core );
		hfc_fx_change_portstat_linkup(pp, core);
		if ( HFC_FX_MMODE_CHECK_SHARED(pp) && ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){
			if(pp->initialize != 0){
				hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
			}
		}
		else {
			if ((!HFC_FX_MQ_VIRTUAL_PORT(pp))&&( !HFC_FX_MMODE_CHECK_SHADOW(pp) )) {
				hfc_fx_wwnverify_linkup(pp, NULL, core, 0, 0);
			}
		}
	}
	else{
		if( pp->connect_type == HFC_FX_F_PORT ){
			HFC_DBGPRT("hfcldd : hfcl_intr - hfc_fx_link_resp F_PORT\n");
			if( (HFC_FX_MMODE_CHECK_SHARED(pp)) && (!HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){	/* FCLNX-GPL-FX-436 Start */
				HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_linkend_int_glpar - Complete Link Initialize for F_PORT.\n",pp->dev_minor);
				pp->scsi_id = 0;
				domain = (uchar)pp->rid + 0x80;
				pp->scsi_id |= (uint64_t)(domain << 16);
				
				core->fw_init_p->fw_iocinfo.configure_flag = ( HFC_FL_PID_VALID | HFC_FL_P2P_PID_VALID );       /* FCLNX-GPL-398 */
				
				/* Set p2p_tgt_port_id of Init_Table */
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] = 0x01;
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] = 0;
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] = 0;
				
				/* Set self_port_id of Init_Table */
				core->fw_init_p->fw_iocinfo.self_port_id[0] = domain;
				core->fw_init_p->fw_iocinfo.self_port_id[1] = 0;
				core->fw_init_p->fw_iocinfo.self_port_id[2] = 0;
				
				hfc_fx_copy_master_to_slave( pp, core );
				hfc_fx_change_portstat_linkup(pp, core);
				hfc_fx_wwnverify_linkup(pp, NULL, core, 0, 0);
			}	/* FCLNX-GPL-FX-436 End */
		}
		else if( pp->connect_type == HFC_FX_MULTI_ALPA ){	/* FCLNX-GPL-FX-171 Start */
			hfc_fx_hand2_trace(
				HFC_FX_TRC_LINKUP_INT, 0x02, pp, core->rp, core, NULL, NULL,
				0, 0, 0);
			
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD1, NULL, 0) ;
			
			if(HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-407 */
				status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);	/* FCLNX-GPL-FX-407 */
				status &= ~HFC_HG_LPRSTATUS_LINKDOWN;	/* FCLNX-GPL-FX-407 */
				status |= (HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_FCSW | HFC_HG_LPRDETAIL_MULTIALPA);	/* FCLNX-GPL-FX-407 */
				if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
					status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
				hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );	/* FCLNX-GPL-FX-407 */
			}	/* FCLNX-GPL-FX-407 */
			
			hfc_fx_initialize_failed(pp, core, NULL);
			
			hfc_fx_copy_master_to_slave( pp, core );
			hfc_fx_change_portstat_linkdown(pp, core);
			
			hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);

			if(pp->initialize != 0){
				hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
			}
			else {
				if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
					hfc_fx_wwnverify_linkup(pp, NULL, core, 0, 0);
				}
			}
		}	/* FCLNX-GPL-FX-171 End */
		else{
			if (!HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				set_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp -> status_detail1 );
				atomic_set(&pp->check_mbreq, 1);
			}
		}
	}
	
mlpf_guest_skip:

//	unlock_fx_mailbox(pp);

	HFC_EXIT("hfc_fx_linkup_intreq");	
	
	return ;
}


/*
 * Function:    hfc_fx_receive_flogi_intreq
 *
 * Purpose:     This routine deals with Receive FLOGI interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_fx_receive_flogi_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	uchar                   find; /* target detection flag	*/
	struct target_info_fx      *target = NULL;
	uint                    lp;
	ushort					payload_length;
	uchar					accept=1;
	uchar					d_id[3], s_id[3], fc_class, domain=0;
	uint					port_id;
	uchar					reject_flag=0, initialize_flag=0;	/* FCLNX-GPL-FX-138 */
	uchar					Reason=0, Reason_ex=0;
	/*==== FC-PH Header ======================================================*/
	uint					 Rdid					= 0;
	uint					 Rsid					= 0;
	/*==== receiveFLOGI Specific Area ========================================*/
//	uchar					 class_valid			= 0;
	uchar					 flogi_configure_flag	= 0;
	ushort					 flogi_max_frame_size	= 0;
	uchar					 flogi_param			= 0;
	uchar					 flogi_rsp_param		= 0;
	uint64_t				 ww_name				= 0;

	hfc_fx_hand2_trace(
		HFC_FX_TRC_FLOGI_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_flogi_intreq");	
	
	payload_length = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.payload_length );
	if( payload_length != HFC_INTREQ_PRLI_LENGTH ) accept = 0;
	
	/*-- Search target info by scsi_id # */
	d_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[0] );
	d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[1] );
	d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[2] );
	s_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[0] );
	s_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[1] );
	s_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[2] );
	
	pp->flogi_ww_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.target_wwpn );
	pp->flogi_node_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.target_wwnn );
	fc_class = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fc_class );
	
	if (pp->fcsp_flag) {
		flogi_param |= FLOGI_PARAM_SECURITY;
	} else {
		flogi_param &= ~FLOGI_PARAM_SECURITY;
	}
	if (pp->vfab_enable) {
		flogi_param |= FLOGI_PARAM_VF;
	} else {
		flogi_param &= ~FLOGI_PARAM_VF;
	}

	/* Setup reject responce to receive_flogi control area */
	hfc_fx_write_val( mbox -> mb_intresp.mb_code, HFC_MBINTRSP_RCVFLOGI );
		
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.cs_ctl, 0);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[0], d_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[1], d_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[2], d_id[2]);
	
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.rctl, HFC_RCVFRMRSP_ELS);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[0], s_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[1], s_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[2], s_id[2]);

	hfc_fx_write_val( mbox -> mb_intresp.frame_ctl, 0);
	hfc_fx_write_val( mbox -> mb_intresp.fc_class, fc_class);
	hfc_fx_write_val( mbox -> mb_intresp.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mbox -> mb_intresp.self_wwnn, (pp -> node_name));
	hfc_fx_write_val( mbox -> mb_intresp.param, pp -> flogi_param);
	
	flogi_max_frame_size = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.flogi_max_frame_size );
	flogi_configure_flag = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.flogi_conf_flag );
	flogi_rsp_param = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.flogi_rsp_param );

//	pp -> fabric_s_id = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[0] ) << 16 ) +
	Rdid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[0] ) << 16 ) +
		( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[1] ) << 8 ) +
			(uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[2] );
	Rdid &= HFC_MBX_PID_PORTID;
//	pp -> fabric_d_id = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[0] ) << 16 ) +
	Rsid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[0] ) << 16 ) +
		( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[1] ) << 8 ) +
			(uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.s_id[2] );
	Rsid &= HFC_MBX_PID_PORTID;
	
	/*==== Check Class Specify(Supported Class2/Class3) ======================*/ 
	if ((fc_class != HFC_FC_CLASS2) && (fc_class != HFC_FC_CLASS3)) {
		/*---- Reject, cause Service Parameter Error(Request no-reply) -------*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_SPERR;
	}
	/*==== Check Distnation Port ID ==========================================*/
//	if (Rdid != pp->scsi_id) {
//		/*---- Reject, cause FC-PH Header Error(Request no-reply) ------------*/
//		reject_flag = 1;
//		Reason		= HFC_MBX_LS_RC_LOGERR;
//		Reason_ex	= HFC_MBX_LS_RE_NOADD;
//	}
	/*==== Check FLOGI Max Frame Size ========================================*/
	if (  (flogi_max_frame_size < 0x0080)
		||(flogi_max_frame_size & 0x0003)
		||(flogi_max_frame_size > 0x0800))
	{
		/*----Unuse Selection of Send Frame Size(Set to Illegal Value"0") ----*/
		flogi_max_frame_size = 0x0000;
	}
	/*==== Valid Check for WWPN ==============================================*/
	if ((!pp->flogi_ww_name) || (pp->flogi_ww_name == (uint64_t)-1)) {
		/*---- Reject, cause Invalid N_Port_Name/F_Port Name(Request no-reply)*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVPNAME;
	}
	/*==== Valid Check for WWNN ==============================================*/
	if ((!pp->flogi_node_name) || (pp->flogi_node_name == (uint64_t)-1)) {
		/*---- Reject, cause Invalid Node_Name/Fablic_Name(Request no-reply)--*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVNNAME;
	}
	
	/*==========================================================================
	 *    Set Request Data to PORT/CORE Resouce
	 *========================================================================*/
	/*==== Save FLOGI Data on Diside Frame Accept ============================*/
	pp->flogi_max_frame_size	= flogi_max_frame_size;
	pp->flogi_config_flag		= flogi_configure_flag;
	pp->flogi_rsp_param			= flogi_rsp_param;
	pp->flogi_param				= flogi_param;
	pp->fabric_s_id				= (uint)Rdid;
	pp->fabric_d_id				= (uint)Rsid;

	if((test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status))&&( pp->connect_type != HFC_FX_F_PORT )){	/* FCLNX-GPL-FX-038 */
		initialize_flag=1;	/* FCLNX-GPL-FX-138 */
		/* Accept responce send case */
		if( !reject_flag ){
			/* Setup Payload */
			for(lp=0; lp<=7; lp++){
				hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x02);
		}
		/* Reject responce send case */
		else{
			/* Setup Payload */
			for(lp=0; lp<=7; lp++){
				hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x01);
			/* Set Reason Code : Logical Error */
			hfc_fx_write_val( mbox -> mb_intresp.payload[5], Reason);
			hfc_fx_write_val( mbox -> mb_intresp.payload[6], Reason_ex);
		}

		if( pp->flogi_config_flag & HFC_FL_PID_VALID ){
			pp->scsi_id=0;
			pp->scsi_id = ( (uint64_t) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.assign_portid[0] ) << 16 ) +
						( (uint64_t) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.assign_portid[1] ) << 8 ) +
						  (uint64_t) hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.assign_portid[2] );
		}

		if( pp->flogi_config_flag & HFC_FL_P2P_PID_VALID ){
			HFC_DBGPRT("hfcldd%d HFC_FL_P2P_PID_VALID\n",pp->dev_minor);
			HFC_DBGPRT("hfcldd%d HFC_FL_P2P_PID_VALID\n",pp->dev_minor);
			
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] 
				= (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.p2p_tgt_port_id[0] );
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] 
				= (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.p2p_tgt_port_id[1] );
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] 
				= (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.p2p_tgt_port_id[2] );
		}
		
		if( pp->flogi_config_flag & HFC_FL_FABRIC_EXIST ){
			HFC_DBGPRT("hfcldd%d HFC_FL_FABRIC_EXIST\n",pp->dev_minor);
			
			pp->switch_exist = HFC_SWITCH_EXIST;
			if(pp ->connect_type != HFC_FX_AL)
				pp ->connect_type = HFC_FX_SWITCH;
		}
		else{
			HFC_DBGPRT("hfcldd%d else\n",pp->dev_minor);
			
			pp->switch_exist = 0;
			
			clear_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp -> status_detail1 );	/* FCLNX-GPL-FX-032,118 */
			if(test_bit( HFC_PD_WAIT_FLOGI, (ulong *)&pp -> status_detail1 )){	/* FCLNX-GPL-FX-047 */
				clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);/* FCLNX-GPL-FX-047 */
			}/* FCLNX-GPL-FX-047 */	/* FCLNX-GPL-FX-032,118 */
			
			if( core->fw_init_p->fw_iocinfo.connect_type == HFC_FX_AL ){
				HFC_DBGPRT("hfcldd%d HFC_NEED_CHANGE_STATE\n",pp->dev_minor);
				hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKUP );
			}
			else{	/* Connection Type : Point to Point	*/
				pp->connect_type = HFC_FX_PT2PT;	/* FCLNX-GPL-FX-038 */
				
				set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);	/* FCLNX-GPL-FX-038 */
				
				if( pp->ww_name > pp->flogi_ww_name ){	/* FCLNX-GPL-FX-026 */
					if (pp->target_arg[0] == NULL) { /* FCLNX-GPL-FX-237 */
						pp->scsi_id = HFC_PTOP_INIT_PORTID;	/* FCLNX-GPL-FX-066 */
						
						port_id = HFC_PTOP_TGT_PORTID;		/* FCLNX-GPL-FX-066 */
						
						/* Create target_info */
						target = hfc_fx_add_target_info_fx(pp, port_id);
					}
					else {
						target = pp->target_arg[0];
						pp->scsi_id = HFC_PTOP_INIT_PORTID;	/* FCLNX-GPL-FX-066 */
						
						target->scsi_id = HFC_PTOP_TGT_PORTID;		/* FCLNX-GPL-FX-066 */
					}
					HFC_DBGPRT("hfcldd%d HFC_FL_FABRIC_EXIST\n",pp->dev_minor);
					
					if (target != NULL) {	/* FCLNX-GPL-FX-446 >>> */
						HFC_CLEAR_TS_LOGINOUT(target);	/* FCLNX-GPL-FX-446 */
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
						hfc_fx_enque_plogi_req(pp,target);
					}						/* FCLNX-GPL-FX-446 <<< */
				}
				else{	/* Waiting to receive PLOGI from I/O Device */
					HFC_DBGPRT("hfcldd%d HFC_WAIT_RECEIVE_PLOGI\n",pp->dev_minor);
					set_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1);
//					set_bit(HFC_COMP_RECEIVE_FLOGI, (ulong *)&pp->status1);
				}
			}
		}
	}
	else if((test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status))&&( pp->connect_type == HFC_FX_F_PORT )){	/* F_PORT case */	/* FCLNX-GPL-FX-038 */
		initialize_flag=1;	/* FCLNX-GPL-FX-138 */
		/* Accept responce send case */
		if( !reject_flag ){
			/* Setup Payload */
			for(lp=0; lp<=7; lp++){
				hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x02);
		}
		/* Reject responce send case */
		else{
			/* Setup Payload */
			for(lp=0; lp<=7; lp++){
				hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x01);
			/* Set Reason Code : Logical Error */
			hfc_fx_write_val( mbox -> mb_intresp.payload[5], Reason);
			hfc_fx_write_val( mbox -> mb_intresp.payload[6], Reason_ex);
		}
			
		HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_intreq flogi_max_frame_size = %04x flogi_config_flag = %02x flogi_rsp_param = %02x\n", 
				pp->dev_minor, pp->flogi_max_frame_size, pp->flogi_config_flag,
				pp->flogi_rsp_param);
			
		HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_intreq fabric_d_id = %08x fabric_s_id = %08x\n", 
				pp->dev_minor, (uint)pp -> fabric_d_id, (uint)pp -> fabric_s_id);
			
		HFC_DBGPRT("hfcldd%d: hfc_fx_flogi_intreq ww_name = %llx node_name = %llx\n", 
				pp->dev_minor, pp->flogi_ww_name,
				pp->flogi_node_name);
			
		pp->scsi_id = 0;
		domain = (uchar)pp->rid + 0x80;
		if( HFC_FX_MMODE_CHECK_SHARED(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) ) ) {
			domain |= 0x01;
			pp->scsi_id |= ( (uint64_t) domain << 16 );
		}
		else{
			pp->scsi_id |= ( (uint64_t) domain << 16 );
		}
			
		/* Set p2p_tgt_port_id of Init_Table */
		core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] = 0x01;
		core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] = 0;
		core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] = 0;
		
		/* Set self_port_id of Init_Table */
		core->fw_init_p->fw_iocinfo.self_port_id[0] = domain;
		core->fw_init_p->fw_iocinfo.self_port_id[1] = 0;
		core->fw_init_p->fw_iocinfo.self_port_id[2] = 0;
		
		/* Set configure_flag of Init_Table  */
		core->fw_init_p->fw_iocinfo.configure_flag = ( HFC_FL_PID_VALID | HFC_FL_P2P_PID_VALID );
			
		hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[0], domain);
		hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[1], 0);
		hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[2], 0);
	
		hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[0], 0x01);
		hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[1], 0);
		hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[2], 0);

		/* F_PORT Connection with I/O */
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_flogi_intreq F_PORT connection\n",pp->dev_minor);
		hfc_fx_copy_master_to_slave( pp, core );
		hfc_fx_change_portstat_linkup(pp, core);
				
		if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
			hfc_fx_wwnverify_linkup(pp, NULL, core, 0, 0);
		}
		else {
			if(pp->initialize != 0){
				hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
			}
		}
	}
	else{	/* FCLNX-GPL-FX-038 Start */
		if( !reject_flag ){
			/* Setup Payload */
			for(lp=0; lp<=7; lp++){
				hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x02);
		}
		/* Reject responce send case */
		else{
			/* Setup Payload */
			for(lp=0; lp<=7; lp++){
				hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x01);
			/* Set Reason Code : Logical Error */
			hfc_fx_write_val( mbox -> mb_intresp.payload[5], Reason);
			hfc_fx_write_val( mbox -> mb_intresp.payload[6], Reason_ex);
		}
	}	/* FCLNX-GPL-FX-038 End */

																	/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
	find = FALSE ;
	
	if ((pp->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PLOGI_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return;
	}
	
	memset(core->logdata,0,16);
	core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	
	HFC_8L_TO_8B(ww_name, pp->flogi_ww_name);
	memcpy(&core->logdata[8],(uchar*)&ww_name,8);

	if( initialize_flag == 0 ){	/* FCLNX-GPL-FX-086 *//* FCLNX-GPL-FX-138 */
		hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0xdc,core->logdata,16) ;
	}
	
																	/* @MLPF STR */
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
																	/* @MLPF END */
	
	HFC_EXIT("hfc_fx_receive_flogi_intreq");	
	return ;
}


/*
 * Function:    hfc_fx_receive_plogi_intreq
 *
 * Purpose:     This routine deals with Receive PLOGI interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *	mbox	   - pointer to mailbox_fx
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_fx_receive_plogi_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	struct target_info_fx      *target = NULL;
	uint                    lp;
	ushort					payload_length, plogi_max_frame_size=0;
	uchar					d_id[3], s_id[3];
	uchar					reject_flag=2, initialize_flag=0;	/* FCLNX-GPL-FX-034 */
	struct fw_init_tbl_fx	*tbl					= NULL;
	uint				     tid					= 0;
	/*==== Common Flag Variables =============================================*/
	uchar					 frame_ctl				= 0;
	uchar					 fc_class				= 0;
	/*==== FC-PH Header ======================================================*/
	uint					 Rdid					= 0;
	uint					 Rsid					= 0;
	/*==== receivePLOGI Specific Area ========================================*/
	uchar					 class_valid			= 0;
	uchar					 plogi_param			= 0;
	uchar					 plogi_rsp_param		= 0;
	uint64_t				 ww_name				= 0;
	uint64_t				 ww_name1				= 0;
	uint64_t				 node_name				= 0;
	/*==== Replay Sequence ===================================================*/
	uchar					 Reason					= HFC_MBX_LS_RC_LOGERR;
	uchar					 Reason_ex				= HFC_MBX_LS_RE_NOADD;
	uchar					 find					= FALSE;

	hfc_fx_hand2_trace(
		HFC_FX_TRC_PLOGI_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_receive_plogi_intreq");	
	
	payload_length = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.payload_length );
	
	
	/*==== Common Flag Variables =============================================*/
	frame_ctl = (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.frame_ctl);
	fc_class = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fc_class );
	
	/*==== receivePLOGI Specific Area ========================================*/
	class_valid	 = (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.class);
	plogi_max_frame_size = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.plogi_max_frame_size );
	plogi_rsp_param	= (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.plogi_param);	
	ww_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.target_wwpn );
	node_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.target_wwnn );
	
	/*-- Search target info by scsi_id # */
	d_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[0] );
	d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[1] );
	d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[2] );
	s_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.s_id[0] );
	s_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.s_id[1] );
	s_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.s_id[2] );
	
	/*-- Search target info by scsi_id # */
	Rdid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[2] );
	HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq Rdid = %08x pp->scsi_id = %08x\n",pp->dev_minor,
			Rdid, (uint)pp->scsi_id);
	
	Rdid &= HFC_MBX_PID_PORTID;
	Rsid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.s_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.s_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.s_id[2] );
	Rsid &= HFC_MBX_PID_PORTID;
	
	/*==========================================================================
	 *    Setup Interrupt Response    
	 *========================================================================*/
	/*==== PLOGI PARAM =======================================================*/
	if (pp->fcsp_flag) {
		plogi_param |=  PLOGI_PARAM_SECURITY;
	} else {
		plogi_param &= ~PLOGI_PARAM_SECURITY;
	}
	/*==========================================================================
	 *     Check Interrupt Request Filed(Driver Must Check)
	 *========================================================================*/
	/*==== Check Class Specify(Supported Class2/Class3) ======================*/ 
	if ((fc_class != HFC_FC_CLASS2) && (fc_class != HFC_FC_CLASS3)) {
		/*---- Reject, cause Service Parameter Error(Request no-reply) -------*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 0\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_SPERR;
	}
	/*==== Check Distnation Port ID ==========================================*/
	if (((!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status))||( pp->connect_type != HFC_FX_PT2PT ))&& (Rdid != pp->scsi_id)) {	/* FCLNX-GPL-FX-032 */
		/*---- Reject, cause FC-PH Header Error(Request no-reply) ------------*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 1\n",pp->dev_minor);
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case Rdid = %08x pp->scsi_id = %08x\n",pp->dev_minor,
			Rdid, (uint)pp->scsi_id);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_NOADD;
	}
	/*==== Valid Check for Class3 Service Parameter ==========================*/
	if (!(class_valid & HFC_PLOGI_CLASS3)) {
		/*---- Reject, cause Service Parameter Error(Request no-reply) -------*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 2\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_SPERR;
	}
	/*==== Valid Check for Class2 Service Parameter ==========================*/
	if (!(class_valid & HFC_PLOGI_CLASS2)) {
		/*---- Set Specify Class3 on Frame Send ------------------------------*/
		fc_class = HFC_FC_CLASS3;
	}
	/*==== Check PLOGI Max Frame Size ========================================*/
	if (  (plogi_max_frame_size < 0x0080)
		||(plogi_max_frame_size & 0x0003)
		||(plogi_max_frame_size > 0x0800))
	{
		/*----Unuse Selection of Send Frame Size(Set to Illegal Value"0") ----*/
		plogi_max_frame_size = 0x0000;
	}
	/*==== Valid Check for WWPN ==============================================*/
	if ((!ww_name) || (ww_name == (uint64_t)-1)) {
		/*---- Reject, cause Invalid N_Port_Name/F_Port Name(Request no-reply)*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 3\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVPNAME;
	}
	/*==== Valid Check for WWNN ==============================================*/
	if ((!node_name) || (node_name == (uint64_t)-1)) {
		/*---- Reject, cause Invalid Node_Name/Fablic_Name(Request no-reply)--*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 4\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVPNAME;
	}
	/*==========================================================================
	 *     Check Driver Sequence and FC Configuration
	 *========================================================================*/
	/*==== Check Driver Sequence =============================================*/
	if((test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status))
	&&( pp->connect_type == HFC_FX_PT2PT )
	&&( !test_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1))){ /* FCLNX-GPL-FX-047 */
		/*---- Reject, cause Sequence Error(Request Reply) -------------------*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 5\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_NOADD;
	}	/* FCLNX-GPL-FX-047 */
	/*==== Check FC Configuration ============================================*/
	tbl = core->fw_init_p;
	if ((tbl->fw_iocinfo.connect_type == HFC_PT2PT)&&(pp->ww_name > ww_name)) {	/* FCLNX-GPL-FX-032 */
		/*---- Reject, cause Invalid N_Port_Name/F_Port Name(Request no-reply)*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 6\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVPNAME;
	}
	if (pp->fabric_d_id == Rsid) {
		/*---- Reject, cause Request from FC-SW(Request no-reply) ------------*/
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 7\n",pp->dev_minor);
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_NOADD;
	}
	/*==========================================================================
	 *    Accept Sequence to Request Specify Target
	 *========================================================================*/
	for (tid=0; tid < MAX_TARGET_PROBE; tid++) {
		target = pp->target_arg[pp->tid_map[tid]];	/* FCLNX-GPL-FX-474 */
		/*======================================================================
		 *     Check target resouce(Throught Unspecify Target)
		 *====================================================================*/
		if(!target)								{ continue; }
		/* FCLNX-GPL-FX-474 >>> */
		if (!test_bit(HFC_TF_DEVFLG_VALID,	(ulong *)&target->flags)  &&
			!test_bit(HFC_TS_NEED_PLOGI,	(ulong *)&target->status) &&
			!test_bit(HFC_TS_NEED_PRLI,		(ulong *)&target->status) &&
			!test_bit(HFC_TS_NEED_LOGO_TGT,	(ulong *)&target->status) &&
			!test_bit(HFC_TS_WAIT_PLOGI,	(ulong *)&target->status) &&
			!test_bit(HFC_TS_WAIT_PRLI,		(ulong *)&target->status) &&
			!test_bit(HFC_TS_WAIT_LOGO_TGT,	(ulong *)&target->status))
			continue;
		/* FCLNX-GPL-FX-474 <<< */
		
		if(target->ww_name != ww_name)				{ continue; }
		/*======================================================================
		 *    Set Need Cancel SCSI to Target Status
		 *====================================================================*/
		if(!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){	/* FCLNX-GPL-FX-117 */
			set_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
			atomic_set(&pp->check_mbreq, 1);
		}    /* FCLNX-GPL-FX-117 */
		/*======================================================================
		 *    Clear Target Status
		 *====================================================================*/
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
		/*======================================================================
		 *     Save Request Data to Target Resouce
		 *====================================================================*/
		target->plogi_param				 = plogi_param & plogi_rsp_param;
		target->fc_class_mask			 = class_valid;
		target->fc_class				 = fc_class;
		target->max_frame_size	 = plogi_max_frame_size;
		/*======================================================================
		 *    Reset Send Frame Size
		 *====================================================================*/
		/*---- base Set HW Max Frame Size ------------------------------------*/
		target->send_frame_size			 = 0x800;
		/*---- Compare with PLOGI Max Frame Size -----------------------------*/
		if (  (target->max_frame_size)
			&&(target->max_frame_size < target->send_frame_size))
		{
			target->send_frame_size = target->max_frame_size;
		}
		/*---- Compare with FLOGI Max Frame Size -----------------------------*/
		if (  (pp->flogi_max_frame_size)
			&&(pp->flogi_max_frame_size < target->send_frame_size))
		{
			target->send_frame_size = pp->flogi_max_frame_size;
		}
		target->mfsize = target->send_frame_size;
		reject_flag = 0;
		find = TRUE;
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq target found.\n",pp->dev_minor);
		break;
	}
	if(( reject_flag == 2 )&&( pp->connect_type == HFC_FX_PT2PT )){	/* FCLNX-GPL-FX-034 Start *//* FCLNX-GPL-FX-117 */
		if( pp->ww_name < ww_name ){	/*  FCLNX-GPL-FX-026 */
			if (pp->target_arg[0] == NULL) { /* FCLNX-GPL-FX-237 */
				/* Create target_info_fx */
				target = hfc_fx_add_target_info_fx(pp, Rsid);	
			}
			else {
				target = pp->target_arg[0];
			}
			
			HFC_DBGPRT("hfcldd%d HFC_NEED_CHANGE_STATE\n",pp->dev_minor);

			if( target != NULL ) {
				/*======================================================================
				 *     Save Request Data to Target Resouce
				 *====================================================================*/
				target->plogi_param				 = plogi_param & plogi_rsp_param;
				target->fc_class_mask			 = class_valid;
				target->fc_class				 = fc_class;
				target->max_frame_size	 = plogi_max_frame_size;
				HFC_DBGPRT("wwnverify_linkup() - Connect PtoP. Set WWNN/WWPN");
				target->ww_name = ww_name;
				target->node_name = node_name;
				/*======================================================================
				 *    Reset Send Frame Size
				 *====================================================================*/
				/*---- base Set HW Max Frame Size ------------------------------------*/
				target->send_frame_size			 = 0x800;
				/*---- Compare with PLOGI Max Frame Size -----------------------------*/
				if (  (target->max_frame_size)
					&&(target->max_frame_size < target->send_frame_size))
				{
					target->send_frame_size = target->max_frame_size;
				}
				/*---- Compare with FLOGI Max Frame Size -----------------------------*/
				if (  (pp->flogi_max_frame_size)
					&&(pp->flogi_max_frame_size < target->send_frame_size))
				{
					target->send_frame_size = pp->flogi_max_frame_size;
				}
				target->mfsize = target->send_frame_size;
				pp->mailbox_pseq = target->pseq;
				HFC_CLEAR_TS_LOGINOUT(target);	/* FCLNX-GPL-FX-446 */
				set_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
				hfc_fx_enque_prli_req(pp,target);
				reject_flag = 0;
				find = TRUE;
				HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq target found.\n",pp->dev_minor);
				hfc_fx_hand2_trace(
					HFC_FX_TRC_PLOGI_INT, 0x14, pp, core->rp, core, target, NULL,
					0, 0, 0 );
				HFC_DBGPRT("hfcldd%d PtoP\n",pp->dev_minor);
			}
		}
	}	/* FCLNX-GPL-FX-034 End */
	/*==========================================================================
	 *    Set Driver Status to PORT Resouce
	 *========================================================================*/
	
	if( test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)
	&& test_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1)) {
		clear_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1);
		initialize_flag=1;	/* FCLNX-GPL-FX-034 */
		if( pp->connect_type == HFC_FX_PT2PT ){
			pp->scsi_id = Rdid;							/* FCLNX-GPL-FX-032 */
			
			/* FCLNX-GPL-FX-039 Start */
			/* Set p2p_tgt_port_id of Init_Table */
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] = s_id[0];
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] = s_id[1];
			core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] = s_id[2];
			
			/* Set self_port_id of Init_Table */
			core->fw_init_p->fw_iocinfo.self_port_id[0] = d_id[0];
			core->fw_init_p->fw_iocinfo.self_port_id[1] = d_id[1];
			core->fw_init_p->fw_iocinfo.self_port_id[2] = d_id[2];
		
			/* Set configure_flag of Init_Table  */
			core->fw_init_p->fw_iocinfo.configure_flag = ( HFC_FL_PID_VALID | HFC_FL_P2P_PID_VALID );
			/* FCLNX-GPL-FX-039 End */
			
			clear_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp -> status_detail1 );	/* FCLNX-GPL-FX-032 */
			if(test_bit( HFC_PD_WAIT_FLOGI, (ulong *)&pp -> status_detail1 )){	/* FCLNX-GPL-FX-047 */
				clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);/* FCLNX-GPL-FX-047 */
			}/* FCLNX-GPL-FX-047 */
			HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq pp->scsi_id=%llx.\n",pp->dev_minor, pp->scsi_id);	/* FCLNX-GPL-FX-032 */
			hfc_fx_copy_master_to_slave( pp, core );	/* FCLNX-GPL-FX-005 */
			hfc_fx_change_portstat_linkup(pp, core);	/* FCLNX-GPL-FX-005 */
			
			if(target != NULL){	/* FCLNX-GPL-FX-117 Start */
				HFC_CLEAR_TS_LOGINOUT(target);	/* FCLNX-GPL-FX-446 */
				set_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
			}	/* FCLNX-GPL-FX-117 End */
		}
	}
	else{
		if( reject_flag == 0 ){
			if( pp->connect_type == HFC_FX_PT2PT ){	/* FCLNX-GPL-FX-117 Start */
				if(target != NULL){
					HFC_CLEAR_TS_LOGINOUT(target);	/* FCLNX-GPL-FX-446 */
					set_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
					atomic_set(&pp->check_mbreq, 1);
				}
			}else{
				if(target != NULL){
					/* FCLNX-GPL-FX-446 >>> */
					HFC_CLEAR_TS_LOGINOUT(target);

					if (target->login_seq_retry_cnt > 0) {
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
//						target->login_seq_retry_cnt--;	/* FCLNX-GPL-FX-476 */
					}
					else
						set_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status);
					/* FCLNX-GPL-FX-446 <<< */
				}
			}	/* FCLNX-GPL-FX-117 End */
			atomic_set(&pp->check_mbreq, 1);
			if ((pp->connect_type == HFC_FX_SWITCH ) 
				||  ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00))) {
					HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq NEED GPN-FT\n",pp->dev_minor);
					set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
			}
		}
	}
	
	/* Setup reject responce to receive_plogi control area */
	hfc_fx_write_val( mbox -> mb_intresp.mb_code, HFC_MBINTRSP_RCVPLOGI );
		
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.cs_ctl, 0);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[0], d_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[1], d_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[2], d_id[2]);
	
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.rctl, HFC_RCVFRMRSP_ELS);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[0], s_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[1], s_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[2], s_id[2]);

	hfc_fx_write_val( mbox -> mb_intresp.frame_ctl, frame_ctl);
	hfc_fx_write_val( mbox -> mb_intresp.fc_class, fc_class);
	hfc_fx_write_val( mbox -> mb_intresp.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mbox -> mb_intresp.self_wwnn, (pp -> node_name));
	hfc_fx_write_val( mbox -> mb_intresp.param, pp -> plogi_param);
	
	/* Accept responce send case */
	if( !reject_flag ){
		/* Setup Payload */
		for(lp=0; lp<=7; lp++){
			HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq Accept case 1\n",pp->dev_minor);
			hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
		}
		hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x02);
	}
	/* Reject responce send case */
	else{
		HFC_DBGPRT("hfcldd%d hfc_fx_receive_plogi_intreq reject case 10\n",pp->dev_minor);
		/* Setup Payload */
		for(lp=0; lp<=7; lp++){
			hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
		}
		hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x01);
		/* Set Reason Code : Logical Error */
		hfc_fx_write_val( mbox -> mb_intresp.payload[5], Reason);
		hfc_fx_write_val( mbox -> mb_intresp.payload[6], Reason_ex);
	}
																	/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		goto mlpf_shadow_skip;	/* FCLNX-GPL-FX-477 */
	}
																	/* @MLPF END */
	
	if ((pp->limit_log != HFC_LOGMODE_EXT) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On *//* FCLNX-GPL-FX-207 */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PLOGI_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return;
	}

	memset(core->logdata,0,16);
	core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	
	core->logdata[4] = class_valid;
	core->logdata[5] = plogi_max_frame_size;
	core->logdata[6] = plogi_rsp_param;	
	
	HFC_8L_TO_8B(ww_name1, ww_name);
	memcpy(&core->logdata[8],(uchar*)&ww_name1,8);
	
	if(initialize_flag == 0){	/* FCLNX-GPL-FX-034 */
		hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x16,core->logdata,16) ;
	}
	
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
	
	HFC_EXIT("hfc_fx_receive_plogi_intreq");	
	return ;
}


/*
 * Function:    hfc_fx_receive_pdisc_intreq
 *
 * Purpose:     This routine deals with Receive PDISC interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *	mbox	   - pointer to mailbox_fx
 *
 * Returns:    - 
 *
 * Notes:       
 */
void hfc_fx_receive_pdisc_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	uint                    lp;
	ushort					payload_length;
	uchar					d_id[3], s_id[3];
	uchar					reject_flag=0;
	/*==== Common Flag Variables =============================================*/
	uchar					 frame_ctl				= 0;
	uchar					 fc_class				= 0;
	/*==== FC-PH Header ======================================================*/
	uint					 Rdid					= 0;
	uint					 Rsid					= 0;
	/*==== receivePDISC Specific Area ========================================*/
	uchar					 class_valid			= 0;
	ushort					 plogi_max_frame_size	= 0;
	//uchar					 plogi_param			= 0;	/* FCLNX-GPL-FX-446 */
	uchar					 plogi_rsp_param		= 0;
	uint64_t				 ww_name				= 0;
	uint64_t				 ww_name1				= 0;
	uint64_t				 node_name				= 0;
	/*==== Replay Sequence ===================================================*/
	uchar					 Reason					= 0;
	uchar					 Reason_ex				= 0;
	/*==== Find Target =======================================================*/
	struct target_info_fx	*target					= NULL;
	uint				     tid					= 0;
	uchar					 find					= FALSE;
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_PDISC_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_receive_pdisc_intreq");	
	
	/*-- Search target info by scsi_id # */
	Rdid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[2] );
	Rdid &= HFC_MBX_PID_PORTID;
	Rsid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.s_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.s_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.s_id[2] );
	Rsid &= HFC_MBX_PID_PORTID;
	
	/*==== Common Flag Variables =============================================*/
	frame_ctl = (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.frame_ctl);
	fc_class  = (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fc_class);
	
	payload_length = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.payload_length );
	
	/*==== receivePDISC Specific Area ========================================*/
	class_valid = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.class );
	plogi_max_frame_size = (ushort)hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.plogi_max_frame_size);
	plogi_rsp_param		 = (uchar)hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.plogi_param);	
	
	/*-- Search target info by scsi_id # */
	d_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[0] );
	d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[1] );
	d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[2] );
	s_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.s_id[0] );
	s_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.s_id[1] );
	s_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.s_id[2] );
	
	ww_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.target_wwpn );
	node_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.target_wwnn );
	
	/*==== Check Class Specify(Supported Class2/Class3) ======================*/ 
	if ((fc_class != HFC_FC_CLASS2) && (fc_class != HFC_FC_CLASS3)) {
		/*---- Reject, cause Service Parameter Error(Request no-reply) -------*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_SPERR;
	}
	/*==== Check Distnation Port ID ==========================================*/
	if (Rdid != pp->scsi_id) {
		/*---- Reject, cause FC-PH Header Error(Request no-reply) ------------*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_NOADD;
	}
	/*==== Valid Check for Class3 Service Parameter ==========================*/
	if (!(class_valid & HFC_PLOGI_CLASS3)) {
		/*---- Reject, cause Service Parameter Error(Request no-reply) -------*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_SPERR;
	}
	/*==== Valid Check for Class2 Service Parameter ==========================*/
	if (!(class_valid & HFC_PLOGI_CLASS2)) {
		/*---- Set Specify Class3 on Frame Send ------------------------------*/
		fc_class = HFC_FC_CLASS3;
	}
	/*==== Check PLOGI Max Frame Size ========================================*/
	if (  (plogi_max_frame_size < 0x0080)
		||(plogi_max_frame_size & 0x0003)
		||(plogi_max_frame_size > 0x0800))
	{
		/*----Unuse Selection of Send Frame Size(Set to Illegal Value"0") ----*/
		plogi_max_frame_size = 0x0000;
	}
	/*==== Valid Check for WWPN ==============================================*/
	if ((!ww_name) || (ww_name == (uint64_t)-1)) {
		/*---- Reject, cause Invalid N_Port_Name/F_Port Name(Request no-reply)*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVPNAME;
	}
	/*==== Valid Check for WWNN ==============================================*/
	if ((!node_name) || (node_name == (uint64_t)-1)) {
		/*---- Reject, cause Invalid Node_Name/Fablic_Name(Request no-reply)--*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_INVNNAME;
	}
	/*==========================================================================
	 *    Accept Sequence to Request Specify Target
	 *========================================================================*/
	for (tid=0; tid < MAX_TARGET_PROBE; tid++) {
		target = hfc_fx_hash_target_info(pp, tid);
		/*======================================================================
		 *     Check target resouce(Throught Unspecify Target)
		 *====================================================================*/
		if (!target)								{ continue; }
		if (!test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ) { continue; }
		if (target->ww_name != ww_name)				{ continue; }

#if 0	/* FCLNX-GPL-FX-446 >>> */
		/*======================================================================
		 *    Clear Target Status
		 *====================================================================*/
		clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
		clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
		clear_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
		/*======================================================================
		 *     Save Request Data to Target Resouce
		 *====================================================================*/
		target->plogi_param				 = plogi_param & plogi_rsp_param;
		target->fc_class_mask			 = class_valid;
		target->fc_class				 = fc_class;
		target->max_frame_size	 = plogi_max_frame_size;
		/*======================================================================
		 *    Reset Send Frame Size
		 *====================================================================*/
		/*---- base Set HW Max Frame Size ------------------------------------*/
		target->send_frame_size			 = 0x800;
		/*---- Compare with PLOGI Max Frame Size -----------------------------*/
		if (  (target->max_frame_size)
			&&(target->max_frame_size < target->send_frame_size))
		{
			target->send_frame_size = target->max_frame_size;
		}
		/*---- Compare with FLOGI Max Frame Size -----------------------------*/
		if (  (pp->flogi_max_frame_size)
			&&(pp->flogi_max_frame_size < target->send_frame_size))
		{
			target->send_frame_size = pp->flogi_max_frame_size;
		}
		target->mfsize = target->send_frame_size;
#endif	/* FCLNX-GPL-FX-446 <<< */

		find = TRUE;
	}
	
	/* Setup reject responce to receive_plogi control area */
	hfc_fx_write_val( mbox -> mb_intresp.mb_code, HFC_MBINTRSP_RCVPDISC );
		
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.cs_ctl, 0);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[0], d_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[1], d_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[2], d_id[2]);
	
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.rctl, HFC_RCVFRMRSP_ELS);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[0], s_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[1], s_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[2], s_id[2]);

	hfc_fx_write_val( mbox -> mb_intresp.frame_ctl, frame_ctl);
	hfc_fx_write_val( mbox -> mb_intresp.fc_class, fc_class);
	hfc_fx_write_val( mbox -> mb_intresp.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mbox -> mb_intresp.self_wwnn, (pp -> node_name));
	hfc_fx_write_val( mbox -> mb_intresp.param, pp -> plogi_param);
	
	/* Accept responce send case */
	if( !reject_flag ){
		/* Setup Payload */
		for(lp=0; lp<=7; lp++){
			hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
		}
		hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x02);
	}
	/* Reject responce send case */
	else{
		/* Setup Payload */
		for(lp=0; lp<=7; lp++){
			hfc_fx_write_val( mbox -> mb_intresp.payload[lp], 0x00);
		}
		hfc_fx_write_val( mbox -> mb_intresp.payload[0], 0x01);
		/* Set Reason Code : Logical Error */
		hfc_fx_write_val( mbox -> mb_intresp.payload[5], Reason);
		hfc_fx_write_val( mbox -> mb_intresp.payload[6], Reason_ex);
	}

#if 0	/* FCLNX-GPL-FX-446 >>> */
	if(target != NULL){
		if( find == TRUE )
		{
			clear_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
			atomic_set(&pp->check_mbreq, 1);
		}
	}
#endif	/* FCLNX-GPL-FX-446 <<< */

	if ((pp->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PDISC_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return;
	}
																	/* @MLPF STR */
//mlpf_shadow_skip:
																	/* @MLPF END */
	memset(core->logdata,0,16);
	core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	
	core->logdata[4] = class_valid;
	core->logdata[5] = plogi_max_frame_size;
	core->logdata[6] = plogi_rsp_param;	
	
	HFC_8L_TO_8B(ww_name1, ww_name);
	memcpy(&core->logdata[8],(uchar*)&ww_name1,8);
	
	hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0xdc, core->logdata,16) ;
	
	HFC_EXIT("hfc_fx_receive_pdisc_intreq");	
	return ;
}


/*
 * Function:    hfc_fx_receive_frame_intreq
 *
 * Purpose:     This routine deals with Recieve_Frame interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *  mbox       - pointer to mailbox_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_receive_frame_intreq(
	struct port_info	*pp,
	struct core_info	*core,
	struct mailbox_fx	*mbox)
{
	uchar					els_command=0, rctl=0;
	uchar					d_id[3], s_id[3];
//	struct rcvfrm_payload_fx	*rcvfrm_pyload = NULL;
	uint					lp;
	/*==== Common Flag Variables =============================================*/
	uchar					frame_ctl		= 0;
	uchar					fc_class		= 0;
	ushort					payload_len		= 0;
	/*==== FC-PH Header ======================================================*/
	uint					Rdid			= 0;
//	uint					Rsid			= 0;
	/*==== Replay Sequence ===================================================*/
	uchar					Reason			= 0;
	uchar					Reason_ex		= 0;
//	uchar					rsp				= FALSE;
	uchar					reject_flag		= 0;
	ushort					res_payload_len	= 0;
	struct rcvfrm_payload_fx	*rcvfrm_pyload = NULL;
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_RFRAME_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_receive_frame_intreq");
	
	/* Read ELS_Command code from Receive Frame Payload */
	els_command = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.els_command );
	rctl = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.rctl );
	
	/*-- Search target info by scsi_id # */
	d_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[0] );
	d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[1] );
	d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[2] );

	s_id[0] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[0] );
	s_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[1] );
	s_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[2] );
	
	Rdid = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[0] ) << 16 ) +
		( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[1] ) << 8 ) +
		(uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[2] );
	Rdid &= HFC_MBX_PID_PORTID;

	/*==== Common Flag Variables =============================================*/
	frame_ctl	 = (uchar )hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.frame_ctl);
	fc_class	 = (uchar )hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fc_class );
	payload_len	 = (ushort)hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.payload_length);
	
	/*==== Mailbox Command(ReceiveFrame) =====================================*/
	hfc_fx_write_val( mbox -> mb_intresp.mb_code, HFC_MBINTRSP_RCVFRM );
		
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.cs_ctl, 0);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[0], d_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[1], d_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.s_id[2], d_id[2]);
	
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.rctl, HFC_RCVFRMRSP_ELS);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[0], s_id[0]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[1], s_id[1]);
	hfc_fx_write_val( mbox -> mb_intresp.fcph_hdr.d_id[2], s_id[2]);

	hfc_fx_write_val( mbox -> mb_intresp.frame_ctl, frame_ctl);
	hfc_fx_write_val( mbox -> mb_intresp.fc_class, fc_class);
	hfc_fx_write_val( mbox -> mb_intresp.self_wwpn, (core -> phys_rcvfrm_payload));
	hfc_fx_write_val( mbox -> mb_intresp.self_wwnn, 0);
	hfc_fx_write_val( mbox -> mb_intresp.param, 0);
	
	if ((fc_class != HFC_FC_CLASS2) && (fc_class != HFC_FC_CLASS3)) {
		/*---- Reject, cause Service Parameter Error(Request no-reply) -------*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_SPERR;
		/* Set the length of Reject Response */
		res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
	}
	/*==== Check Distnation Port ID ==========================================*/
	if (Rdid != pp->scsi_id) {
		/*---- Reject, cause FC-PH Header Error(Request no-reply) ------------*/
		reject_flag = 1;
		Reason		= HFC_MBX_LS_RC_LOGERR;
		Reason_ex	= HFC_MBX_LS_RE_NOADD;
		/* Set the length of Reject Response */
		res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
	}
	
	/* Set RCVFRM Payload Area */
	rcvfrm_pyload = core->rcvfrm_payload;
	for(lp=0; lp<HFC_PRLI_RLENGTH; lp++){
		hfc_fx_write_val( rcvfrm_pyload -> data0[lp], 0x00);
	}
	
	if( !reject_flag ){
		if(rctl == HFC_FRMSNDRCV_ELS){
			switch( els_command ){
			case HFC_INTREQ_ELS_PRLI :
				reject_flag = hfc_fx_prli_intreq(pp, core, mbox);
				if (!reject_flag) {
					res_payload_len = (ushort)HFC_PRLI_RLENGTH;
				} else {
					res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
				}
				break;
			case HFC_INTREQ_ELS_PRLO :
				reject_flag = hfc_fx_prlo_intreq(pp, core, mbox);
				if (!reject_flag) {
					res_payload_len = (ushort)HFC_PRLO_RLENGTH;
				} else {
					res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
				}
				break;
			case HFC_INTREQ_ELS_RSCN :
				reject_flag = hfc_fx_rscn_intreq(pp, core, mbox);
				if (!reject_flag) {
					res_payload_len = (ushort)HFC_LS_ACC_LENGTH;
				} else {
					res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
				}
				break;
			case HFC_INTREQ_ELS_LOGO :
				reject_flag = hfc_fx_logo_intreq(pp, core, mbox);
				if (!reject_flag) {
					res_payload_len = (ushort)HFC_LS_ACC_LENGTH;
				} else {
					res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
				}
				break;
//			case HFC_INTREQ_ELS_EVFP :		hfc_fx_evfp_intreq(pp, core, mbox);		break;
//			case HFC_INTREQ_ELS_AUTH_ELS :	hfc_fx_auth_els_intreq(pp, core, mbox);	break;
			default :
				reject_flag = 1;
				Reason		= HFC_MBX_LS_RC_CMDNSP;
				Reason_ex	= HFC_MBX_LS_RE_NOADD;
				/* Set the length of Reject Response */
				res_payload_len = (ushort)HFC_LS_RJT_LENGTH;
				break;
			}
			if( !reject_flag ){
				/* Accept responce send case */
				/* Setup Payload */
				hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x02);
			}
			else{
				/* Reject responce send case */
				/* Setup Payload */
				hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x01);
				/* Set Reason Code : Logical Error */
				hfc_fx_write_val( rcvfrm_pyload -> data0[5], Reason);
				hfc_fx_write_val( rcvfrm_pyload -> data0[6], Reason_ex);
			}
			hfc_fx_write_val( mbox -> mb_intresp.payload_length, (ushort)res_payload_len);
		}
		
	}
	else{
		/* Reject responce send case */
		/* Setup Payload */
		hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x01);
		/* Set Reason Code : Logical Error */
		hfc_fx_write_val( rcvfrm_pyload -> data0[5], Reason);
		hfc_fx_write_val( rcvfrm_pyload -> data0[6], Reason_ex);
		hfc_fx_write_val( mbox -> mb_intresp.payload_length, (ushort)res_payload_len);
	}
	
	if( ( pp->limit_log == HFC_LOGMODE_EXT ) && ( reject_flag ) ){	/* FCLNX-GPL-FX-052 */
		memset(core->logdata,0,16);
		core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
		core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
		core->logdata[2] = els_command;
		core->logdata[3] = rctl;
		
		core->logdata[7] = Reason;

		hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINT,
			ERRID_HFCP_EVNT2, 0xdc, core->logdata,16) ;
	}																/* FCLNX-GPL-FX-052 */
	
	HFC_EXIT("hfc_fx_receive_frame_intreq");

	return ;
}


/*
 * Function:    hfc_fx_prli_intreq
 *
 * Purpose:     This routine deals with PRLI interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *  mbox       - pointer to mailbox_fx
 *
 * Returns:     
 *
 * Notes:       
 */
uchar hfc_fx_prli_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	uchar                   find; /* target detection flag	*/
	struct target_info_fx      *target = NULL;
	uint                    lp=0;
	ushort					payload_length;
	uchar					accept=1;
	uint					prli_param_req=0, prli_param_resp=0;
	struct rcvfrm_payload_fx	*rcvfrm_pyload = NULL;
	uchar 					param_resp=0, rtn=1;
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_PRLI_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_prli_intreq");
	
	payload_length = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.payload_length );
	if( payload_length != HFC_INTREQ_PRLI_LENGTH ) accept = 0;
	
	prli_param_req = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.prli.prli_param_req );
	
	prli_param_resp = (prli_param_req & 0x00000180);
	/* Initiator mode */
	prli_param_resp |= 0x00000022;
	
	/* Reject Responce */
	hfc_fx_write_val( mbox -> mb_intresp.payload_length, 0x08);
	
	/* Set RCVFRM Payload Area */
	rcvfrm_pyload = core->rcvfrm_payload;
	for(lp=0; lp<0x08;lp++){
		hfc_fx_write_val( rcvfrm_pyload -> data0[lp], 0);
	}
	hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x01);
	/* Set Reason Code : Logical Error */
	hfc_fx_write_val( rcvfrm_pyload -> data0[5], HFC_REASON_CODE_LOGICAL_ERROR);
	/* Reason Code Explanation */
	hfc_fx_write_val( rcvfrm_pyload -> data0[6], 0x00);
	
	param_resp = (uchar)(prli_param_resp >> 24);
	hfc_fx_write_val( rcvfrm_pyload -> data0[10], param_resp);
	
	param_resp = (uchar)(prli_param_resp >> 16);
	hfc_fx_write_val( rcvfrm_pyload -> data0[11], param_resp);
	
	param_resp = (uchar)(prli_param_resp >> 8);
	hfc_fx_write_val( rcvfrm_pyload -> data0[12], param_resp);
	
	param_resp = (uchar)prli_param_resp;
	hfc_fx_write_val( rcvfrm_pyload -> data0[13], param_resp);

																	/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
	find = FALSE ;
	
	/* Search target by comparing WWN of receiving PRLI with WWN of target under */
	/* the designated adapter */
	target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);	/* FCLNX-GPL-FX-034 */
	
	if (target != NULL)
	{
		find = TRUE ;
	}
	
	if(target != NULL){ /* FCLNX-GPL-069 */
		if( (find == TRUE) && (!test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status)) )
		{	/* A target exists with unfinished LOGIN status */
//			hfc_fx_cancel_scsi_cmd(pp, core, target, 0, NULL, SCS_INTR_PLOGI,
//								 HFC_CSCSI_INHALT, TRUE, FALSE, HFC_FLASH_TARGET);	/* FCLNX-0429 */
			
			clear_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
			clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);				/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
			set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);	/* FCLNX-GPL-FX-034 */
			if (HFC_FX_MQ_VALID(pp))
				hfc_fx_mq_change_target_info(pp, target);
		
			target->prli_parm = prli_param_resp;	/* FCLNX-GPL-FX-034 */
		}
	}
	
	if ((pp->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PLOGI_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return rtn;
	}
																	/* @MLPF STR */
																	/* @MLPF END */
	if(!rtn){																/* @MLPF END */
		memset(core->logdata,0,16);
		core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
		core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
		
		core->logdata[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.els_command );
		core->logdata[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.rctl );
		
		hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0xdc,core->logdata,16) ;
	}
	
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
	
	HFC_EXIT("hfc_fx_prli_intreq");	
	return rtn ;
}


/*
 * Function:    hfc_fx_prlo_intreq
 *
 * Purpose:     This routine deals with PRLO interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *  mbox       - pointer to mailbox_fx
 *
 * Returns:     
 *
 * Notes:       
 */
uchar hfc_fx_prlo_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	uchar                   find; /* target detection flag	*/
	struct target_info_fx      *target = NULL;
	uint                  	lp=0,s_id=0;	/* FCLNX-GPL-FX-117 */
	ushort					payload_length;
	uchar					accept=1, rtn=1;
	struct rcvfrm_payload_fx	*rcvfrm_pyload = NULL;
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_PRLO_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_prlo_intreq");	
	
	payload_length = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.payload_length );
	if( payload_length != HFC_INTREQ_PRLO_LENGTH ) accept = 0;
	
	/* Accept Responce */
	hfc_fx_write_val( mbox -> mb_intresp.payload_length, 0x14);
	
	/* Set RCVFRM Payload Area */
	rcvfrm_pyload = core->rcvfrm_payload;
	for(lp=0; lp<0x14;lp++){
		hfc_fx_write_val( rcvfrm_pyload -> data0[lp], 0);
	}
	hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x02);
	hfc_fx_write_val( rcvfrm_pyload -> data0[4], 0x08);

																	/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		goto mlpf_shadow_skip;
	}
																	/* @MLPF END */
		/*-- Search target info by scsi_id # */
	s_id = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[2] );
	
	find = FALSE ;
	
	/* Search target by comparing WWN of receiving PRLI with WWN of target under */
	/* the designated adapter */
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)					/* FCWIN-0082 */
	{
		target = hfc_fx_hash_target_info(pp, lp);
		
		if (target != NULL)
		{
			if ((target->scsi_id & 0x00ffffff) == (s_id & 0x00ffffff)) /* FCLNX-GPL-FX-117 Start */
			{
				find = TRUE ;
				break ;
			} /* FCLNX-GPL-FX-117 End */
		}
	}
	
	if(target != NULL){ /* FCLNX-GPL-069 */
		if( (find == TRUE) 
		&& (!test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status)) 
		&& (!test_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status)))
		{	/* A target exists with unfinished LOGIN status */
			clear_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
			set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);					/* FCWIN-0146 */
			atomic_set(&pp->check_mbreq, 1);
			clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);				/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
		}
	}
	else { /* FCLNX-GPL-FX-117 Start */
		if ( (pp->connect_type == HFC_FX_SWITCH )
		|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00)))
		{
			set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
			atomic_set(&pp->check_mbreq, 1);
		}
	} /* FCLNX-GPL-FX-117 End */
	
	if ((pp->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_PLOGI_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return rtn;
	}
																	/* @MLPF STR */
																	/* @MLPF END */
	if(!rtn){																/* @MLPF END */
		memset(core->logdata,0,16);
		core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
		core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
		
		core->logdata[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.els_command );
		core->logdata[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.rctl );
		hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0xdc,core->logdata,16) ;
	}
	
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
	
	HFC_EXIT("hfc_fx_prlo_intreq");	
	return rtn;
}


/*
 * Function:    hfc_fx_rscn_intreq
 *
 * Purpose:     This routine deals with RSCN interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *  mbox       - pointer to mailbox_fx
 *
 * Returns:     
 *
 * Notes:       
 */
uchar hfc_fx_rscn_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	struct target_info_fx      *target = NULL;
	ushort                  scn_cnt = 0;		/* FCLNX-0637 */
	ushort                  scn_tcnt;
	ushort                  scn_no  = 0;		/* FCLNX-0637 */
	uint                    lp;
	int                     p_mask;
	uint                    port_id;
	uint                    mb_port_id;
	char                    find = FALSE;
	uchar					rtn=0;
	struct rcvfrm_payload_fx	*rcvfrm_pyload = NULL;
	
	HFC_ENTRY("hfc_fx_rscn_intreq");
	hfc_fx_hand2_trace(
		HFC_FX_TRC_RSCN_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
		
	/* Accept Response */
	hfc_fx_write_val( mbox -> mb_intresp.payload_length, HFC_LS_ACC_LENGTH);
	
	/* Set RCVFRM Payload Area */
	rcvfrm_pyload = core->rcvfrm_payload;
	hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x02);
	hfc_fx_write_val( rcvfrm_pyload -> data0[1], 0);
	hfc_fx_write_val( rcvfrm_pyload -> data0[2], 0);
	hfc_fx_write_val( rcvfrm_pyload -> data0[3], 0);

	/* FCLNX-GPL-FX-086 */
	if( pp->connect_type == HFC_FX_F_PORT ){
		hfc_fx_hand2_trace(
			HFC_FX_TRC_RSCN_INT, 0x21, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return rtn;
	}
	/* FCLNX-GPL-FX-086 */
																	/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		goto mlpf_shadow_skip;
	}																/* @MLPF END */

	/*-- Count up the number of targets with the RSCN notification */
	scn_cnt =  hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.rscn.scn_cnt);
	scn_cnt = (scn_cnt - 4)/4 ;
	scn_tcnt = scn_cnt;
	scn_no = 0 ;
	
	while( scn_no < scn_cnt ) /* Repeat to check all RSCN notification */
	{
		if (pp->filter_target == HFC_FX_MB_LOGIN_FILTER_ON) {
			mb_port_id = (uint)hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.rscn.portid_page[scn_no]);
			if ((pp->scsi_id & 0x00ffff00) == (mb_port_id & 0x00ffff00)) { /* domain,area */
				/* AccessGateway switch same physical port */
				scn_tcnt--;
				scn_no++ ;
				continue; 
			}
		}
		
		find = FALSE ;
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)			 /* FCWIN-0083 */
		{	/* Repeat to check all targets */
			target = hfc_fx_hash_target_info(pp, lp);
			
			if (target != NULL)
			{/* When target exists */
				port_id = (uint)target->scsi_id ;
				mb_port_id = (uint)hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.rscn.portid_page[scn_no]);
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
					if( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) )	/* FCLNX-0274 */ /* FCLNX-GPL-038 */
					{
						set_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
						
//						if ( (pp->connect_type == HFC_FX_SWITCH) 	/* FCWIN-0082STR*/
//							|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
//							set_bit(HFC_T_NEED_GIDPN, (ulong *)&target->status);
//							atomic_set(&pp->check_mbreq, 1);
//							pp -> next_gidpn = TRUE;
//						}
//						else {
//							set_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
//							atomic_set(&pp->check_mbreq, 1);
//							hfc_fx_enque_pdisc_req(pp, target);
//						}										/* FCWIN-0082END*/
						find = TRUE ;
					}
				}
				else if( (port_id & p_mask) == (mb_port_id & p_mask) )
				{
					if( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) )	/* FCLNX-0274 */ /* FCLNX-GPL-038 */
					{
						set_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);

//						if ( (pp->connect_type == HFC_FX_SWITCH) 	/* FCWIN-0082STR*/
//							|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
//							set_bit(HFC_T_NEED_GIDPN, (ulong *)&target->status);
//							atomic_set(&pp->check_mbreq, 1);
//							pp -> next_gidpn = TRUE;
//						}
//						else {
//							set_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
//							atomic_set(&pp->check_mbreq, 1);
//							hfc_fx_enque_pdisc_req(pp, target);
//						}										/* FCWIN-0082END*/

						find = TRUE ;
					}
				}
			}
		}
		scn_no++ ;
	}
	
	if (scn_tcnt == 0) {
		return rtn;
	}
	
	hfc_fx_wwnverify_scn(pp, target, 0);	/* FCLNX-GPL-503 */
	
	if ((pp->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) { /* FCLNX-GPL-491 Limit Log Mode On */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_RSCN_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return rtn;
	}
	
	memset(core->logdata,0,16);
	core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	
	core->logdata[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.els_command );
	core->logdata[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.rctl );

	if(!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
		hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x18,core->logdata,16) ;
	}

mlpf_shadow_skip:	/* FCLNX-GPL-FX-407 */

	HFC_EXIT("hfc_fx_rscn_intreq");
	return rtn;
}


/*
 * Function:    hfc_fx_logo_intreq
 *
 * Purpose:     This routine deals with LOGO interrupt request
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *  mbox       - pointer to mailbox_fx
 *
 * Returns:    -
 *
 * Notes:       
 */
uchar hfc_fx_logo_intreq(
	struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox)
{
	uchar                   find,find2; /* Object target detection flag	FCWIN-0082*/
	struct target_info_fx      *target = NULL;
	uint                    lp=0;
	uint                    n_port_id=0;
	uint					d_id, s_id;
	uint64_t				n_port_name;
	uchar					rtn=0;
	struct rcvfrm_payload_fx	*rcvfrm_pyload = NULL;
	
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LOGO_INT, 0x00, pp, core->rp, core, NULL, NULL,
		0, 0, 0);
	HFC_ENTRY("hfc_fx_logo_intreq");
	
	/* Set RCVFRM Payload Area */
	rcvfrm_pyload = core->rcvfrm_payload;
	hfc_fx_write_val( rcvfrm_pyload -> data0[0], 0x02);
	
	/* Accept Response */
	hfc_fx_write_val( mbox -> mb_intresp.payload_length, HFC_LS_ACC_LENGTH);

	/*-- Search target info by scsi_id # */
	d_id = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[2] );
	s_id = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.s_id[2] );

	n_port_name = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.logo.n_port_name );
	n_port_id   = ( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.logo.n_port_id[0] ) << 16 ) +
				( (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.logo.n_port_id[1] ) << 8 ) +
				  (uint) hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.logo.n_port_id[2] );	/* FCLNX-GPL-FX-252,272 */
	
	HFC_DBGPRT("hfcldd%d hfc_fx_logo_intreq n_port_name = %llx, n_port_id=%08x\n",
			pp->dev_minor, (unsigned long long)n_port_name, (uint)n_port_id);
	

																	/* @MLPF STR */
	if (HFC_FX_MMODE_CHECK_SHADOW(pp) )
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
		target = pp->target_arg[pp->tid_map[lp]];	/* FCLNX-GPL-FX-474 */
		
		if (target != NULL)
		{
			/* FCLNX-GPL-FX-474 >>> */
			if (!test_bit(HFC_TF_DEVFLG_VALID,	(ulong *)&target->flags)  &&
				!test_bit(HFC_TS_NEED_PLOGI,	(ulong *)&target->status) &&
				!test_bit(HFC_TS_NEED_PRLI,		(ulong *)&target->status) &&
				!test_bit(HFC_TS_NEED_LOGO_TGT,	(ulong *)&target->status) &&
				!test_bit(HFC_TS_WAIT_PLOGI,	(ulong *)&target->status) &&
				!test_bit(HFC_TS_WAIT_PRLI,		(ulong *)&target->status) &&
				!test_bit(HFC_TS_WAIT_LOGO_TGT,	(ulong *)&target->status))
				continue;
			/* FCLNX-GPL-FX-474 <<< */
			
			if (target->ww_name == n_port_name) { 
				if ((target->scsi_id & 0x00ffffff) == (n_port_id & 0x00ffffff)) /* FCWIN-0082 */
				{
					find = TRUE ;
					break ;
				}
				else {
					if ( (pp->connect_type == HFC_FX_SWITCH )
						|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00)))/* FCWIN-0185 */
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
		if( (find == TRUE) && !test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status) )
		{	/* A target exists with unfinished LOGIN status */
//			hfc_fx_cancel_scsi_cmd(pp, core, target, 0, NULL, SCS_INTR_LOGO, 
//								HFC_CSCSI_INHALT, TRUE, FALSE, HFC_FLASH_TARGET);	/* FCLNX-0429 */
			
			clear_bit(HFC_TS_NEED_PDISC, (ulong *)&target->status);
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
			
			/* FCLNX-GPL-FX-446 >>> */
			HFC_CLEAR_TS_LOGINOUT(target);

			if (target->login_seq_retry_cnt > 0) {
				set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
//				target->login_seq_retry_cnt--;	/* FCLNX-GPL-FX-476 */
			}
			else
				set_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status);
			/* FCLNX-GPL-FX-446 <<< */
			
			atomic_set(&pp->check_mbreq, 1);
			/*======================================================================
		 	*    Set Need Cancel SCSI to Target Status
		 	*====================================================================*/
			set_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
			
			if ( find2 == TRUE ) {								/* FCWIN-0082 */
				set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
				if (HFC_FX_MQ_VALID(pp))
					hfc_fx_mq_change_target_info(pp, target);
				
				if( test_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status) )
				{
					set_bit(HFC_TS_WPDISC_LOGO_RESP, (ulong *)&target->status);
				}
				
				hfc_fx_enque_plogi_req(pp, target);
			}
			else {												/* FCWIN-0082STR*/
				/* FC-SW and WWN of the target is identical to the existing target, */
				/* but scsi_id is mismatch between the two 							*/
				/*	-> Issue GPN_FT to renew port number 							*/
				if (!test_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2) ) {
					set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);		/* Reserve GPN_FT */
				}
//				set_bit(HFC_T_NEED_GIDPN, (ulong *)&target->status);
//				pp -> next_gidpn = TRUE;
//				hfc_fx_issue_gidpn( pp, target );
			}													/* FCWIN-0082END*/
		}
	}
//	set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);		/* Reserve GID_FT */
	atomic_set(&pp->check_mbreq, 1);
	
	/* FCLNX-GPL-503 */
	if ((pp->limit_log == HFC_ENABLE_LIMITLOG) && (find == FALSE)) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LOGO_INT, 0x20, pp, core->rp, core, NULL, NULL,
			0, 0, 0);
		return rtn;
	}
	
	memset(core->logdata,0,16);
	core->logdata[0] = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	core->logdata[1] = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	
	core->logdata[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.subtype.els_command );
	core->logdata[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.rctl );

	hfc_fx_errlog(pp,core,target,NULL,HFC_ERRLOG_TYPE_MBINT,ERRID_HFCP_EVNT2, 0x17,core->logdata,16) ;
	
																	/* @MLPF STR */
mlpf_shadow_skip:	/* FCLNX-GPL-FX-477 */
																	/* @MLPF END */
	
	HFC_EXIT("hfc_fx_logo_intreq");
	return rtn;
}


#define HFC_CMD_RESULT_PRE_CHK	0x00ff00ff

/*
 * Function:    hfc_fx_xrb_resp
 *
 * Purpose:     This routine deals with XRB Response interrupt
 *
 * Arguments:
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *  rp         - pointer to region_info
 *
 * Returns:    -
 *
 * Notes:
 */
int hfc_fx_xrb_resp(
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	uint64_t				time,
	uint					cpuno,
	struct hfc_pkt_fx		*(*wait_iodone_hfcp))
{
	struct port_info		*save_pp = pp;
	struct target_info_fx	*target=NULL;
	struct hfc_pkt_fx		*hfcp=NULL, *hfcp_end=NULL;	/* FCLNX-GPL-FX-267 */
	struct hfc_pkt_fx		*target_hfcp;
	struct xrb_fx			xrb;
	
	uint					xrb_cnt;            /* Transferred XRB counts */
	uint					xrb_no ;            /* Current XRB number, max value is pp->xob_max */
	uint					xrb_no_org ;
	uint					targetid;
	uint					lunid;
	uint					free_xrb_area;
	uchar					hfcp_find;
	uchar					link_err = 0;
	uchar					tskmgm_cmd = 0;
	uchar					tskmgm_cnt = 0;
	int						rtn = 0;

	xrb_cnt = 0 ;
	xrb_no  = core->drv_next_xrb ;
	xrb_no_org = xrb_no ;

	/* Check xrbs while xrb valid bit is enabled */
	while(core->xrb[core->drv_next_xrb].xcrbchk.valid & HFC_XRB_VALID)
	{
		/* Set xrb_no */
		xrb_no = core->drv_next_xrb ;
		memcpy(&xrb, &core->xrb[xrb_no], sizeof(xrb));

		hfcp = (struct hfc_pkt_fx *)(ulong)xrb.xcrb.hfc_pkt;
		pp = hfcp->pp;
		
		if (pp == NULL) {
			/* Dequeue XRB */
			core->xrb[core->drv_next_xrb].xcrbchk.valid &= ~HFC_XRB_VALID ;     /* Reset xrb valid flag */
			xrb_cnt++ ;
			core->drv_next_xrb++ ;
			if( core->drv_next_xrb >= save_pp->xrb_max)
				core->drv_next_xrb = 0 ;	/* FCLNX-GPL-FX-332 */

			continue ;
		}
		
		if (HFC_PP_FX_STATUS_DETAIL2_TEST(HFC_PD_WAIT_CLOSE, pp)) {
			/* Dequeue XRB */
			core->xrb[core->drv_next_xrb].xcrbchk.valid &= ~HFC_XRB_VALID ;     /* Reset xrb valid flag */
			xrb_cnt++ ;
			core->drv_next_xrb++ ;
			if( core->drv_next_xrb >= pp->xrb_max)
				core->drv_next_xrb = 0 ;
			
			hfc_fx_hand2_trace(
				HFC_FX_TRC_XRBRSP, 0x24, pp, rp, core, NULL, NULL,
				(uint64_t)xrb_cnt, (uint64_t)xrb_no, 0);

			continue ;
		}

		/* Check rid */
		if ((HFC_FX_MMODE_CHECK_SHARED(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp))) ||
			 HFC_FX_VPORT_EXIST(pp))
		{
			if ((hfcp != NULL) && ( hfcp->rid != rp->rid ))
			{
				hfc_fx_hand2_trace(
					HFC_FX_TRC_XRBRSP, 0x24, pp, rp, core, NULL, NULL,
					(uint64_t)xrb_cnt, (uint64_t)xrb_no, 0);
				
				pp = save_pp;
				return (((HFC_ABEND_RID_INVALID << 8) | HFC_XRBRESP_ABEND) & 0x0000ffff);
			}
		}

		/* Check if this XRB is valid: XRB_Type bit5-7 != 001 */
		if( (xrb.xcrb.type & 0x07 ) != HFC_XRB_I )
		{
			hfc_fx_hand2_trace(
				HFC_FX_TRC_XRBRSP, 0x11, pp, rp, core, NULL, NULL,
				(uint64_t)xrb_cnt, (uint64_t)xrb_no, 0);
			
			pp = save_pp;
			return (((HFC_ABEND_XRB_INVALID << 8) | HFC_XRBRESP_ABEND) & 0x0000ffff);
		}

		/* Check SKIP bit */
		if(xrb.xcrb.skip & HFC_XRB_SKIP)
		{	/* If xrb valid flag is zero */
		
			/* Dequeue XRB */
			HFC_DBGPRT("hfcldd%d : hfc_fx_xrb_resp - HFC_XRB_SKIP bit is enabled. xrb_n=%02x",pp->dev_minor, core->drv_next_xrb);
			/* Dequeue XRB */
			core->xrb[core->drv_next_xrb].xcrbchk.valid &= ~HFC_XRB_VALID ;     /* Reset xrb valid flag */
			core->xrb[core->drv_next_xrb].xcrb.skip &= ~HFC_XRB_SKIP;
			xrb_cnt++ ;
			core->drv_next_xrb++ ;
			if( core->drv_next_xrb >= pp->xrb_max)
				core->drv_next_xrb = 0 ;

		
			continue ;
		}
		
		/* Check valid bit in hfc_pkt_fx */
		if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_VALID ,hfcp))
		{
			/* Dequeue XRB */
			core->xrb[core->drv_next_xrb].xcrbchk.valid &= ~HFC_XRB_VALID ;     /* Reset xrb valid flag */
			xrb_cnt++ ;
			core->drv_next_xrb++ ;
			if( core->drv_next_xrb >= pp->xrb_max)
				core->drv_next_xrb = 0 ;

			continue ;
		}

		/* Get target_id, lun_id */
		targetid = ((struct hfc_pkt_fx *)(ulong)xrb.xcrb.hfc_pkt)->target_id;
		lunid = ((struct hfc_pkt_fx *)(ulong)xrb.xcrb.hfc_pkt)->lun_id;
		target = hfc_fx_hash_target_info(pp, targetid);

		if(target == NULL)
		{
			hfc_fx_hand2_trace(
				HFC_FX_TRC_XRBRSP, 0x12, pp, rp, core, NULL, hfcp,
				(uint64_t)xrb_cnt, (uint64_t)xrb_no, 0);
			target_hfcp = NULL;
		}
		else
		{
			target_hfcp = target->core_queue[core->core_no].we_que_top[lunid % HASH_T_NUM];
		}
		hfcp_find= FALSE;

		while( target_hfcp != NULL)
		{
			/* Check whether saved hfc_pkt_fx at initialization is in wait_end_que */
			if( target_hfcp == hfcp )
			{
				hfcp_find = TRUE ;
				break ;
			}
			target_hfcp = target_hfcp->cmd_forw;
		}

		if(hfcp_find == FALSE )
		{	/* Device and target cancellation has already processed */

			/* Dequeue XRB */
			core->xrb[core->drv_next_xrb].xcrbchk.valid &= ~HFC_XRB_VALID ;     /* Reset xrb valid flag */
			xrb_cnt++ ;
			core->drv_next_xrb++ ;
			if( core->drv_next_xrb >= pp->xrb_max)
				core->drv_next_xrb = 0 ;
			
			hfc_fx_hand2_trace(
				HFC_FX_TRC_XRBRSP, 0x22, pp, rp, core, target, hfcp,
				(uint64_t)xrb_cnt, (uint64_t)xrb_no, 0);
			target_hfcp = NULL;
			
			continue ;
		}
		
		/* set timestamp and processor id */
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				pp->pm_pkt_pool[hfcp->pm_pkt_no].cpuno_xrb_int = cpuno;
				pp->pm_pkt_pool[hfcp->pm_pkt_no].tsc_xrb_int = time;
			}
		}
		
		if ( target != NULL)
		{
			if ( hfcp->cmd_flags & CFLAG_RESET_ANY ) {	/* FCLNX-GPL-FX-014 */
				tskmgm_cnt++;
				tskmgm_cmd = 1;
			}
			else {
				tskmgm_cmd = 0;
			}
			
			if ((xrb.xcrb.fsb & ((~HFC_FSB_RETRY)&(~HFC_FSB_IL)) )==0) {
				/* link ok */
				if (tskmgm_cmd) {
					/* Check task management command */
					if (hfc_fx_task_mgm_chk(pp, hfcp, &xrb)) {
						rtn |= HFC_XRBRESP_TMCMD_ERR;
						if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
							if(*wait_iodone_hfcp == NULL){
								*wait_iodone_hfcp = hfcp;
								((struct hfc_pkt_fx*)(*wait_iodone_hfcp))->cmd_prev = NULL;	/* FCLNX-GPL-FX-267 */
								((struct hfc_pkt_fx*)(*wait_iodone_hfcp))->cmd_forw = NULL;	/* FCLNX-GPL-FX-267 */
							}else{
								/* FCLNX-GPL-FX-267 Start */
								hfcp_end = *wait_iodone_hfcp;
								while(hfcp_end->cmd_forw){
									hfcp_end = hfcp_end->cmd_forw;
								}
								hfcp_end->cmd_forw = hfcp;
								hfcp->cmd_prev = hfcp_end;
								hfcp->cmd_forw = NULL;
								/* FCLNX-GPL-FX-267 End */
							}
						}
					}
				}
				else {
					/* Check SCSI command */
					if(hfc_fx_scsi_chk(pp, target, hfcp, &xrb)){	/* FCLNX-GPL-FX-332 */
						if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
							rtn |= HFC_XRBRESP_INTERNAL_RETRY;
							HFC_DBGPRT("hfcldd%d : hfc_fx_xrb_resp - hfcp->cmd_pkt->result=%08x",pp->dev_minor,hfcp->cmd_pkt->result);
							if(*wait_iodone_hfcp==NULL){
								*wait_iodone_hfcp = hfcp;
								((struct hfc_pkt_fx*)(*wait_iodone_hfcp))->cmd_prev = NULL;	/* FCLNX-GPL-FX-267 */
								((struct hfc_pkt_fx*)(*wait_iodone_hfcp))->cmd_forw = NULL;	/* FCLNX-GPL-FX-267 */
							}else{
								/* FCLNX-GPL-FX-267 Start */
								hfcp_end = *wait_iodone_hfcp;
								while(hfcp_end->cmd_forw){
									hfcp_end = hfcp_end->cmd_forw;
								}
								hfcp_end->cmd_forw = hfcp;
								hfcp->cmd_prev = hfcp_end;
								hfcp->cmd_forw = NULL;
								/* FCLNX-GPL-FX-267 End */
							}
						}
					}
				}
			}
			else {
				/* link error */
				hfc_fx_link_chk(pp, target, hfcp, &xrb);
				link_err++;
				if (tskmgm_cmd) {
					rtn |= HFC_XRBRESP_TMCMD_LINK_ERR;
				}
				else {
					rtn |= HFC_XRBRESP_IOCMD_LINK_ERR;
				}
				if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
					if(*wait_iodone_hfcp == NULL){
						*wait_iodone_hfcp = hfcp;
						((struct hfc_pkt_fx*)(*wait_iodone_hfcp))->cmd_prev = NULL;	/* FCLNX-GPL-FX-267 */
						((struct hfc_pkt_fx*)(*wait_iodone_hfcp))->cmd_forw = NULL;	/* FCLNX-GPL-FX-267 */
					}else{
						/* FCLNX-GPL-FX-267 Start */
						hfcp_end = *wait_iodone_hfcp;
						while(hfcp_end->cmd_forw){
							hfcp_end = hfcp_end->cmd_forw;
						}
						hfcp_end->cmd_forw = hfcp;
						hfcp->cmd_prev = hfcp_end;
						hfcp->cmd_forw = NULL;
						/* FCLNX-GPL-FX-267 End */
					}
				}
			}
		}

		/* Dequeue XRB */
		core->xrb[core->drv_next_xrb].xcrbchk.valid &= ~HFC_XRB_VALID ;     /* Reset xrb valid flag */
		xrb_cnt++ ;
		core->drv_next_xrb++ ;
		if( core->drv_next_xrb >= pp->xrb_max)
			core->drv_next_xrb = 0 ;
	}

	if(xrb_cnt){	/* FCLNX-GPL-FX-447 */
		/* Calcurate free_xrb_area */
		if (!HFC_FX_VIRTUAL_PORT(pp)) {
			/* physical port */
			free_xrb_area  = 0x300;
			free_xrb_area += 0x80 * core->core_no;
			free_xrb_area += 0x2;
		}
		else {
			/* virtual port */
			free_xrb_area  = 0x1000;
			free_xrb_area += 0x80 * pp->rid;
			free_xrb_area += 0x20 * core->core_no;
			free_xrb_area += 0x2;
		}
	
		/* Free XRB */
		hfc_fx_write_reg_ext(pp, (uint)free_xrb_area, (char)0x02, (ushort)xrb_no);
		
		core->xrb_resp_cnt++;
		if (xrb_cnt > core->max_cmd_num_int) {
			core->max_cmd_num_int = xrb_cnt;
		}
	}	/* FCLNX-GPL-FX-447 */
	
	if (!rtn && tskmgm_cnt) {
		rtn = HFC_XRBRESP_TMCMD_NORMAL;
	}
	
	rtn |= 0x00ff0000 & (link_err << 16 );
	
	if (!(pp->debug_func & HFC_FX_DEBUG_IOTRACE_OFF)) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_XRBRSP, 0x10, pp, rp, core, target, hfcp,
			(uint64_t)xrb_cnt, (uint64_t)xrb_no, 0);
	}
	
	/* get processor_id and timestamp */
	if (hfcp != NULL) {
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				pp->pm_pkt_pool[hfcp->pm_pkt_no].xrb_cmd_cnt = xrb_cnt;
				pp->pm_pkt_pool[hfcp->pm_pkt_no].cpuno_iodone = smp_processor_id();
				rdtscll(pp->pm_pkt_pool[hfcp->pm_pkt_no].tsc_iodone);
			}
		}
	}
	
	pp = save_pp;
	return (rtn);
}


/*
 * Function:    hfc_fx_link_chk
 *
 * Purpose:     This routine check XCRB
 *
 * Arguments:   
 *  rp         - pointer to region_info
 *  core       - pointer to core_info
 *  hfcp       - pointer to hfc_pkt_fx
 *
 * Returns:     
 *  HFC_LINK_CHK_OK  - XCRB check is finished successufully.
 *  HFC_LINK_CHK_ERR - XCRB check is finished with errors.
 *  HFC_LINK_CHK_CCC - ccc has occured.
 *
 * Notes:       
 */
uint hfc_fx_link_chk(
	struct port_info		*pp,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct xrb_fx			*xrb)
{
	struct scsi_cmnd		*Scmd;
	struct dev_info_fx		*dev=NULL;
	struct core_info		*core=NULL, *core_wk=NULL;
	
	uint					xrb_no ;
	uint					func_rc = 0, i=0, j=0 ;
	uint					result;
	uint					cmd_flags;
	
	core = hfcp->core;
	xrb_no = core->drv_next_xrb;
	Scmd = hfcp->cmd_pkt;
	
	if (pp->if_err_limit) {
//		TBD
//		hfc_fx_watched_errcount_i(pp, NULL, HFC_IF_ERR);
	}
	
	if( xrb->xcrb.fsb & HFC_FSB_CDC )
	{	/* FSB = CDC */
		result = DID_ERROR;
		func_rc = HFC_LINK_CHK_ERROR;
		hfcp->adap_status = SCS_IO_CDC;
	}
	else if( xrb->xcrb.fsb & HFC_FSB_CCC )
	{	/* FSB = CCC */
		result = DID_ERROR;
		func_rc = HFC_LINK_CHK_CCC;
		hfcp->adap_status = SCS_IO_CCC;
	}
	else if( xrb->xcrb.fsb & HFC_FSB_ICC )
	{	/* FSB = ICC */
		switch( xrb->xcrb.err_code[0] )
		{
		case HFC_ICC_TIMEOUT :
			result = DID_ERROR;
			func_rc = HFC_LINK_CHK_ERROR;
			hfcp->adap_status = SCS_IO_ICC_TIMEOUT;
			break ;
		case HFC_ICC_NO_RESP :
			result = DID_ERROR;
			func_rc = HFC_LINK_CHK_ERROR;
			hfcp->adap_status = SCS_IO_ICC_NO_RESP;
			break ;
		default :
			result = DID_ERROR;
			func_rc = HFC_LINK_CHK_ERROR;
			hfcp->adap_status = SCS_IO_ICC_LINK_CHK;
		}
	}
	else
	{	/* FSB = PC */
		result = DID_ERROR;
		func_rc = HFC_LINK_CHK_ERROR;
	}
	
	
	if(	test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags) ||
		test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) 	||
		test_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&hfcp->cmd_flags) ||
		test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags) )	/* FCLNX-GPL-FX-014 */
	{	/* Response of target reset */
		/* Dequeue hfc_pkt_fx from wait_end_que */
		hfc_fx_deque_we_que(pp, ((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target, hfcp);
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LINK_CHK, 0x20, pp, hfcp->rp, core, target, hfcp,
			0, 0, 0);
		
		memset(core->icc_err, 0, sizeof(struct icc_errlog));
		HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
		memcpy(core->icc_err->logdata,(uchar*)&cmd_flags,4);
		core->icc_err->logdata[5] = xrb->xcrb.esw ;
		core->icc_err->logdata[6] = xrb->xcrb.softlog;
		memcpy(&core->icc_err->logdata[8],(uchar *)&xrb->xcrb.sbc,2);
		core->icc_err->logdata[10] = xrb->resp_iu1.fcp_status2 ;
		core->icc_err->logdata[11] = xrb->resp_iu1.scsi_status ;
		memcpy(&core->icc_err->logdata[12],(uchar *)&xrb->xcrb.fsb,4);
		memcpy(&core->icc_err->hfcp, hfcp, sizeof(struct hfc_pkt_fx));
		core->icc_err->pseq = target->pseq;
		core->icc_err->icc_pp = pp;
		
		if( xrb->xcrb.fsb & HFC_FSB_PC ) {
//			hfc_fx_errlog(pp,core,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR9,0xA8,core->logdata,16) ;
			core->icc_err->err_no = 0xA8;
			core->icc_err->err_id = ERRID_HFCP_ERR9;
		}
		else {
//			hfc_fx_errlog(pp,core,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR6,0x20,core->logdata,16) ;
			core->icc_err->err_no = 0x20;
			core->icc_err->err_id = ERRID_HFCP_ERR6;
		}
		if ((core->icc_err->logdata[13] == 0xd2) &&
			(core->icc_err->logdata[14] == 0x00)) {
			if (core->icc_err->logdata[15] == 0x00) {
				/* first icc after linkdown */
				core->icc_err->first_icc = 1;
			}
			else if (core->icc_err->logdata[15] == 0x01) {
				/* skip errlog for icc */
				core->icc_err->err_no = 0;
				core->skip_icc_cnt++;
			}
		}
		
		/* FCLNX-GPL-FX-014 Start */
		if (test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_TC_WAIT_TGTRST, (ulong *)&target->tgt_core_stat.core[ core->core_no ]);
		}
		if (test_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_TC_WAIT_CSCSI_TGT_WAIT_DMA, (ulong *)&target->tgt_core_stat.core[ core->core_no ]);
			if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status)){							/* FCLNX-GPL-FX-112 */
				hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);		/* FCLNX-GPL-FX-112 */
			}																							/* FCLNX-GPL-FX-112 */
		}
		if (test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_TC_WAIT_CSCSI_TGT_WITHOUT_DMA, (ulong *)&target->tgt_core_stat.core[ core->core_no ]);
		}
		/* FCLNX-GPL-FX-014 End */
		
		set_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags);	/* FCLNX-GPL-FX-014 */
		clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-GPL-FX-085 */
		clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-GPL-FX-085 */
		
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if(hfc_fx_check_cs_disable(pp, core_wk))
				continue;	/* FCLNX-GPL-FX-438 */
			
			if(test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 )){	/* FCLNX-GPL-FX-085 */
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core_wk, HFC_ABEND_LINK_RESET);
				break;
			}	/* FCLNX-GPL-FX-085 */
			if(hfc_fx_check_cmnd_timeout(pp,core_wk,target,NULL)){	/* FCLNX-GPL-FX-014 */
				HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 8 core_no=%d \n",core_wk->core_no);	/* FCLNX-GPL-FX-014 */
				if(pp->rt_err_enable){
					HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 9 core_no=%d \n",core_wk->core_no);	/* FCLNX-GPL-FX-014 */
					if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
						hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);
						break;
					}
					else{
						hfc_fx_watched_errcount_i(pp, NULL, HFC_RT_ERR);
						break;
					}
				}
				else{
				/* Issue link reset */
					set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
					hfc_fx_abend(pp, core_wk, HFC_ABEND_LINK_RESET);
					break;
				}
			}
		}
		
		if (target->tgt_core_stat.all == 0){	/* FCLNX-GPL-FX-014 */
			HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 10 \n");
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
		}	/* FCLNX-GPL-FX-014 */
		
		hfc_fx_set_cmnd_res(pp, core, Scmd, hfcp, result);
		if (!hfc_manage_info.hfcldd_mp_mod) {
			hfc_fx_iodone(pp, core, Scmd, hfcp);
		}
		return func_rc;
	}
	
	if (test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags) )	/* FCLNX-GPL-FX-014 */
	{	/* Response of Abort Task Set or LUN Reset --*/
		HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 11 \n");
	
		/* Dequeue hfc_pkt_fx from wait_end_que */
		hfc_fx_deque_we_que(pp, ((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target, hfcp);
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_LINK_CHK, 0x21, pp, hfcp->rp, core, target, hfcp,
			0, 0, 0);
		
		HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
		memset(core->icc_err, 0, sizeof(struct icc_errlog));
		HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
		memcpy(core->icc_err->logdata,(uchar*)&cmd_flags,4);
		core->icc_err->logdata[5] = xrb->xcrb.esw ;
		core->icc_err->logdata[6] = xrb->xcrb.softlog;
		memcpy(&core->icc_err->logdata[8],(uchar *)&xrb->xcrb.sbc,2);
		core->icc_err->logdata[10] = xrb->resp_iu1.fcp_status2 ;
		core->icc_err->logdata[11] = xrb->resp_iu1.scsi_status ;
		memcpy(&core->icc_err->logdata[12],(uchar *)&xrb->xcrb.fsb,4);
		memcpy(&core->icc_err->hfcp, hfcp, sizeof(struct hfc_pkt_fx));
		core->icc_err->pseq = target->pseq;
		core->icc_err->icc_pp = pp;
		
		if( xrb->xcrb.fsb & HFC_FSB_PC ) {
//			hfc_fx_errlog(pp,core,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR9,0xA9,core->logdata,16) ;
			core->icc_err->err_no = 0xA9;
			core->icc_err->err_id = ERRID_HFCP_ERR9;
		}
		else {
//			hfc_fx_errlog(pp,core,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_ERR6,0x21,core->logdata,16) ;
			core->icc_err->err_no = 0x21;
			core->icc_err->err_id = ERRID_HFCP_ERR6;
		}
		if ((core->icc_err->logdata[13] == 0xd2) &&
			(core->icc_err->logdata[14] == 0x00)) {
			if (core->icc_err->logdata[15] == 0x00) {
				/* first icc after linkdown */
				core->icc_err->first_icc = 1;
			}
			else if (core->icc_err->logdata[15] == 0x01) {
				/* skip errlog for icc */
				core->icc_err->err_no = 0;
				core->skip_icc_cnt++;
			}
		}
		
		dev = (struct dev_info_fx *)hfc_fx_search_dev_info( ((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target, hfcp->lun_id );
		if( dev == NULL ){
			hfc_fx_hand2_trace(
				HFC_FX_TRC_LINK_CHK, 0x23, pp, hfcp->rp, core, target, hfcp,
				0, 0, 0);
			return func_rc;
		}
		
		/* FCLNX-GPL-FX-014 Start */
		if ( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags) 
		  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags)){
			clear_bit(HFC_DC_WAIT_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);
		}
		if (test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&hfcp->cmd_flags)){
			clear_bit(HFC_DC_WAIT_CSCSI_LU_WAIT_DMA, (ulong *)&dev->dev_core_stat.core[core->core_no]);
		}
		if (test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_DC_WAIT_CSCSI_LU_WITHOUT_DMA, (ulong *)&dev->dev_core_stat.core[core->core_no]);
		}
		/* FCLNX-GPL-FX-014 End */
		
		if (test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) ) {
			set_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
			clear_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
		}
		else {
			set_bit(HFC_DS_FAIL_ABORT, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
			clear_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
		}
		
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if(hfc_fx_check_cmnd_timeout(pp,core_wk,target,&hfcp->lun_id)){	/* FCLNX-GPL-FX-014 */
				hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);	/* FCLNX-GPL-FX-190 */
				HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 17 core_no=%d \n",core_wk->core_no);
				if(pp->rt_err_enable){
					HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 18 \n");
					if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
						hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);
						break;
					}
					else{
						hfc_fx_watched_errcount_i(pp, NULL, HFC_RT_ERR);
						break;
					}
				}
				else{
					if(pp->tgtrst_restrain){
						/* Issue target reset */
						set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
						hfc_fx_abend(pp, core_wk, HFC_ABEND_LINK_RESET);
						break;
					}else{	/* FCLNX-GPL-FX-014 */
						if( (test_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status ) ) ||
						(test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status ) ) )
							break ;
						
						/* Cancel wait queue of SCSI command and Task Management *//* FCLNX-GPL-FX-092 */
						for (j=0 ; j< MAX_CORE_PROBE_FX ; j += (MAX_CORE_PROBE_FX/pp->core_num)) {
							if ((core_wk = pp->region_arg[pp->rid]->core_arg[j]) == NULL)
								continue;
							hfc_fx_cancel_scsi_cmd(pp,core_wk,target,0,NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
							FALSE,TRUE, HFC_FLASH_TARGET );
						}/* FCLNX-GPL-FX-092 */
						if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
							hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
								TRUE, FALSE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
						}
						
						/* FCLNX-GPL-FX-112 Start */
						set_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
						
						target->target_reset_cmnd = hfcp->cmd_pkt;
						
			 			if(hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))){
							clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
							break;
						}
						/* FCLNX-GPL-FX-112 End */
						
						if((pp->hba_isolation == HFC_ISOL_START)&&(pp->total_tgtrst_to)){
							hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
 							hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, FALSE);
						}
						break;
					}	/* FCLNX-GPL-FX-014 */
				}
			}
		}
		
		if (dev->dev_core_stat.all == 0){	/* FCLNX-GPL-FX-014 */
			if (test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) ) {
				clear_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
				HFC_DBGPRT("hfcldd : hfc_fx_link_chk - 20 \n");
			}
			else{
				clear_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
			}
		}	/* FCLNX-GPL-FX-014 */
		
		hfc_fx_set_cmnd_res(pp, core, Scmd, hfcp, result);
		if (!hfc_manage_info.hfcldd_mp_mod) {
			hfc_fx_iodone(pp, core, Scmd, hfcp);
		}
		return func_rc;
	}

	hfc_fx_deque_we_que(pp, ((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target, hfcp);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_LINK_CHK, 0x22, pp, hfcp->rp, core, target, hfcp, 
		0, 0, 0);
	
	memset(core->icc_err, 0, sizeof(struct icc_errlog));
	HFC_4L_TO_4B(cmd_flags, hfcp->cmd_flags);
	memcpy(core->icc_err->logdata,(uchar*)&cmd_flags,4);
	core->icc_err->logdata[5] = xrb->xcrb.esw ;
	core->icc_err->logdata[6] = xrb->xcrb.softlog;
	memcpy(&core->icc_err->logdata[8],(uchar *)&xrb->xcrb.sbc,2);
	core->icc_err->logdata[10] = xrb->resp_iu1.fcp_status2 ;
	core->icc_err->logdata[11] = xrb->resp_iu1.scsi_status ;
	memcpy(&core->icc_err->logdata[12],(uchar *)&xrb->xcrb.fsb,4);
	memcpy(&core->icc_err->hfcp, hfcp, sizeof(struct hfc_pkt_fx));
	core->icc_err->pseq = target->pseq;
	core->icc_err->icc_pp = pp;
	
	if( xrb->xcrb.fsb & HFC_FSB_PC ) {
		core->icc_err->err_no = 0xAA;
		core->icc_err->err_id = ERRID_HFCP_ERR9;
	}
	else {
		core->icc_err->err_no = 0x22;
		core->icc_err->err_id = ERRID_HFCP_ERR6;
	}
	if ((core->icc_err->logdata[13] == 0xd2) &&
		(core->icc_err->logdata[14] == 0x00)) {
		if (core->icc_err->logdata[15] == 0x00) {
			/* first icc after linkdown */
			core->icc_err->first_icc = 1;
		}
		else if (core->icc_err->logdata[15] == 0x01) {
			/* skip errlog for icc */
			core->icc_err->err_no = 0;
			core->skip_icc_cnt++;
		}
	}
	
	hfc_fx_set_cmnd_res(pp, core, Scmd, hfcp, result);
	if (!hfc_manage_info.hfcldd_mp_mod) {
		hfc_fx_iodone(pp, core, Scmd, hfcp);
	}

	return func_rc ;
}


/*
 * Function:    
 *
 * Purpose:     This routine check XCRB for the regular SCSI initiation
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  target     - pointer to target_info_fx
 *  hfcp       - pointer to hfc_pkt_fx
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_scsi_chk(
	struct port_info		*pp,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct xrb_fx			*xrb)
{	/* FCLNX-GPL-FX-332 */
	struct scsi_cmnd		*Scmd;
	struct core_info		*core;
	
	uint					resid = 0;
	uint					work_resid = 0;
	int 					xrb_no = 0 ;
	uchar					scsi_status = 0 ;
	uint					sns_offset = 0 ;
	uint					work_sns_offset = 0;
	uint					sns_length = 0 ;
	uint					work_sns_length = 0;
	ulong					data_len = 0;
	ulong					frame_num = 0;
	ulong					frame_unit = 0;
	ulong					word_num = 0;
	uint					resp_length = 0 ;
	uint					work_resp_length = 0;
	uint					result;

	core = hfcp->core;
	xrb_no = core->drv_next_xrb ;
	Scmd = hfcp->cmd_pkt;
	
	if ((HFC_FX_VIRTUAL_PORT(pp) && (!HFC_FX_MQ_VIRTUAL_PORT(pp)))
	|| ( !(HFC_FX_MMODE_CHECK_BASIC(pp)) && (pp->hg_cca_p != NULL) )){	/* FCLNX-GPL-FX-433 */
		/* New method I/O statistical information calcurated with Driver */
		data_len  = hfcp->data_size;
		frame_num = 0;
		word_num  = 0;
		if (data_len != 0) {
			if (xrb->resp_iu1.fcp_status2 & HFC_XRB_RESID_UNDER) {		/* underrun */
				work_resid = xrb->resp_iu1.resid;
				HFC_4B_TO_4L(resid, work_resid);
				data_len -= resid;
			}
			/* size of a frame */
			if ( Scmd->sc_data_direction == SCSI_DATA_READ ) {	/* Read */
				frame_unit = 2048;
			} 
			else if ( Scmd->sc_data_direction == SCSI_DATA_WRITE ) {	/* Write */
				frame_unit = (target->send_frame_size == 0) ? 2048 : target->send_frame_size;
			}
			/* From data length to the number of frames */
			frame_num = data_len / frame_unit;
			frame_num += (data_len % frame_unit) == 0 ? 0 : 1 ;
			/* the number of words */
			word_num  = data_len / 4;
		}
		
		if ( Scmd->sc_data_direction == SCSI_DATA_WRITE ) {		/* Write */
			pp->tx_frames[core->core_no] += (frame_num + 1);	/* FCP_DATA + FCP_CMND */
			pp->rx_frames[core->core_no] += 2;					/* FCP_XFER_RDY + FCP_RSP */
			
			pp->tx_words[core->core_no] += (word_num + 8);		/* FCP_DATA + FCP_CMND(8) */
			pp->rx_words[core->core_no] += 9;					/* FCP_XFER_RDY(3) + FCP_RSP(6) */
		}
		else {
			pp->rx_frames[core->core_no] += (frame_num + 1);	/* FCP_DATA + FCP_RSP */
			pp->tx_frames[core->core_no] += 1;					/* FCP_CMND */	
			
			pp->rx_words[core->core_no] += (word_num + 6);		/* FCP_DATA + FCP_RSP */
			pp->tx_words[core->core_no] += 8;					/* FCP_CMND */
		}
	}

	/* FCLNX-GPL-494 set statistics for Virtage *//* FCLNX-GPL-FX-433 */
	if ( !(HFC_FX_MMODE_CHECK_BASIC(pp)) && (pp->hg_cca_p != NULL) ) {
		pp->hg_cca_p[core->core_no].tx_frame = pp->tx_frames[core->core_no] ; 
		pp->hg_cca_p[core->core_no].tx_word  = pp->tx_words[core->core_no] ;
		pp->hg_cca_p[core->core_no].rx_frame = pp->rx_frames[core->core_no] ;
		pp->hg_cca_p[core->core_no].rx_word  = pp->rx_words[core->core_no] ;
	}

	scsi_status = xrb->resp_iu1.scsi_status ;
	Scmd->result |= (int)scsi_status;

	if( scsi_status != HFC_SCSISTAT_GOOD )
	{	/* SCSI STATUS is abnormal */
		if( (scsi_status == HFC_SCSISTAT_CHECK_CONDITION)||
			(scsi_status == HFC_SCSISTAT_COMMAND_TERMINATED))
		{
			/* CHECK_CONDITION/CMD_TERM occurred */
			if( (xrb->resp_iu1.fcp_status2 & HFC_XRB_SNSLEN_VALID) &&
				(Scmd->sense_buffer != NULL) )
				
			{	/* Auto Sense is effective */
				sns_offset = 0 ;
				if( xrb->resp_iu1.fcp_status2& HFC_XRB_RSPLEN_VALID )
				{	/* Response Length is effective */
					
					/* Get the start position of sense data */
					work_sns_offset = xrb->resp_iu1.resp_len;
					HFC_4B_TO_4L(sns_offset, work_sns_offset);
				}
				
				/* Set up the length of sense data */
				work_sns_length = xrb->resp_iu1.sns_len;
				HFC_4B_TO_4L(sns_length, work_sns_length);
				
				if (!test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags )) {
					if( SCSI_SENSE_BUFFERSIZE < sns_length )
						sns_length = SCSI_SENSE_BUFFERSIZE;
				}
				else {
					if( HFC_SCSI_SENSE_BUFFERSIZE < sns_length )
						sns_length = HFC_SCSI_SENSE_BUFFERSIZE;
				}
				
				if( (sns_length+sns_offset) > sizeof(xrb->fcp_info) )
					sns_length = sizeof(xrb->fcp_info) - sns_offset ;
				
				/* Copy sense data */
				memcpy(
					Scmd->sense_buffer,
					&xrb->fcp_info[sns_offset],
					sns_length);
			}
			
			hfcp->adap_status = SCS_CHECK_CONDITION;
		}
		else
		{
			hfcp->adap_status = SCS_SCSI_CHECK;
		}
	}
	else
	{
		hfcp->adap_status = SCS_NORMAL_END;
	}

	result = DID_OK;

	if( xrb->resp_iu1.fcp_status2 & HFC_XRB_RSPLEN_VALID)
	{
		/*-- Resp Length Valid --*/
		work_resp_length = xrb->resp_iu1.resp_len;
		HFC_4B_TO_4L(resp_length, work_resp_length);
		
		if (resp_length>3 && xrb->fcp_info[3] )
		{
			/* RSP_CODE byte3 != 0 */
			result = DID_ERROR;
			hfcp->adap_status = SCS_RSPCODE_FAILURE;
		}
	}

	if ( xrb->resp_iu1.fcp_status2 & HFC_XRB_RESID_OVER )
	{	/* Data overruns */
		result = DID_ERROR;
		hfcp->adap_status = SCS_DATA_OVERRUN;
	}
	else {
		resid = 0;

			if (xrb->resp_iu1.fcp_status2 & HFC_XRB_RESID_UNDER )
			{	/* Data underruns */
				work_resid = xrb->resp_iu1.resid;
				HFC_4B_TO_4L(resid, work_resid);
			}

		if (resid) {
			/* kernel 5.x+: sdb.resid removed; use scsi_cmnd->resid_len */
			Scmd->resid_len = resid;
			hfcp->adap_status = SCS_DATA_UNDERRUN;
			
			if (!scsi_status) {
				if ((unsigned)(Scmd->sdb.length - resid) < Scmd->underflow)
				{
					result = DID_ERROR;
				}
			}
		}
	}
	
	/*-- Dequeue hfc_pkt_fx from wait_end_que --*/
	hfc_fx_deque_we_que(pp, ((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target, hfcp);
	
	if (hfcp->adap_status != SCS_NORMAL_END) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_SCSI_CHK, 0x11, pp, hfcp->rp, core, target, hfcp,
			0, 0, 0);
	}

	hfc_fx_set_cmnd_res(pp, core, Scmd, hfcp, result);
	if (!hfc_manage_info.hfcldd_mp_mod) {
		hfc_fx_iodone(pp, core, Scmd, hfcp);
	}else{
		if( !(Scmd->result & HFC_CMD_RESULT_PRE_CHK) ){
			hfc_fx_iodone(pp, core, Scmd, hfcp);
		}
		else{
			return(1);	/* FCLNX-GPL-FX-332 */
		}
	}

	return(0) ;	/* FCLNX-GPL-FX-332 */
}


/*
 * Function:    hfc_fx_task_mgm_chk
 *
 * Purpose:     This routine check XCRB for the ABORT/RESET
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  hfcp       - pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_task_mgm_chk(
	struct port_info		*pp,
	struct hfc_pkt_fx		*hfcp,
	struct xrb_fx			*xrb)
{
	struct target_info_fx	*target;
	struct scsi_cmnd		*Scmd;
	struct dev_info_fx		*dev, *wk_dev;
	struct core_info		*core=NULL, *core_wk=NULL;
	
	uchar					resp_code = 0, lun_reset_end=0;	/* FCLNX-GPL-FX-255,272 */
	uchar					rc_error = 0;
	uchar					need_next_reset = 0;	/* FCLNX-GPL-FX-014 */
	int 					xrb_no = 0, rtn=0;
	uint					result=0, i=0, j=0;

	HFC_DBGPRT("hfcldd : hfc_fx_task_mgm_chk - start, rid=%d\n", pp->rid);

	core = hfcp->core;
	xrb_no = hfcp->core->drv_next_xrb;
	Scmd = hfcp->cmd_pkt;
	dev = hfcp->dev;
	target = (struct target_info_fx *)hfc_fx_hash_target_info(
		pp,
		((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target_id );

	if( (xrb->resp_iu1.fcp_status2 & HFC_XRB_RSPLEN_VALID) && 
		!(xrb->resp_iu1.fcp_status2 & ~HFC_XRB_RSPLEN_VALID) )
	{
		resp_code = xrb->fcp_info[3] ;
		if ( resp_code != HFC_RSP_SUCCESSFUL )
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
		test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) ||
		test_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&hfcp->cmd_flags) ||
		test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags) )	/* FCLNX-GPL-FX-014 */
	{/* Response of target reset */
		/* FCLNX-GPL-FX-014 Start */
		
		if (test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_TC_WAIT_TGTRST, (ulong *)&target->tgt_core_stat.core[ core->core_no ]);
		}
		if (test_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_TC_WAIT_CSCSI_TGT_WAIT_DMA, (ulong *)&target->tgt_core_stat.core[ core->core_no ]);
			if(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status)){	/* FCLNX-GPL-FX-112 */
				need_next_reset = 2;											/* FCLNX-GPL-FX-112 */
			}																	/* FCLNX-GPL-FX-112 */
		}
		if (test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_TC_WAIT_CSCSI_TGT_WITHOUT_DMA, (ulong *)&target->tgt_core_stat.core[ core->core_no ]);
			need_next_reset = 1;
		}
		/* FCLNX-GPL-FX-014 End */
		
		/* Dequeue hfc_pkt_fx from wait_end_que	--*/
		hfc_fx_deque_we_que(pp, target, hfcp);

		/* After SCSI T.O Reset(taget reset)error check */
		if (rc_error == 0)
		{	/* Target Reset succeeded */
			/* Cancellation of XOB and response waiting queue */
			
			hfcp->adap_status = SCS_NORMAL_END;
			
			if (!test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
				Scmd->result |= DID_OK << 16;
		}
		else
		{
			hfcp->adap_status = SCS_RESET_FAILED;
			set_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-GPL-FX-085 */
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-GPL-FX-085 */

			if (!test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
				Scmd->result |= DID_ERROR << 16;
				
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				if(hfc_fx_check_cs_disable(pp, core_wk))
					continue;	/* FCLNX-GPL-FX-438 */
				
				if(test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 )){	/* FCLNX-GPL-FX-085 */
					set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
					hfc_fx_abend(pp, core_wk, HFC_ABEND_LINK_RESET);
					break;
				}	/* FCLNX-GPL-FX-085 */
				if ( hfc_fx_check_cmnd_timeout(pp,core_wk,target,NULL))	/* FCLNX-GPL-FX-014 */
				{	/* T.O. cmd exist */
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);	/* FCLNX-GPL-FX-112 */
					if(pp->rt_err_enable){
						if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
							hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);
							break;
						}
						else{
							hfc_fx_watched_errcount_i(pp, NULL, HFC_RT_ERR);
							break;
						}
					}
					else{
						set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
						hfc_fx_abend(pp, core_wk, HFC_ABEND_LINK_RESET);
						break;
					}
				}
			}
		}
		
		/* FCLNX-GPL-FX-014 Start */
		if (target->tgt_core_stat.all == 0){
			if(!test_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags)){
				if (need_next_reset) {
					// Issue TargetReset and C_SCSI(wait stop DMA)
					if(need_next_reset == 1){									/* FCLNX-GPL-FX-112 */
						hfc_fx_issue_tgtrst_cscsi(
						   pp, target, dev, ((0x00000001 << CFLAG_TARGET_RESET) | (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA)));
					}else{
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status );	/* FCLNX-GPL-FX-112 */
						atomic_set(&pp->check_mbreq, 1);						/* FCLNX-GPL-FX-112 */
					}
				}else { /* Target Reset succeeded */
					clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);
					/* Cancellation of XOB and response waiting queue */
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
							continue;
						/* SoftLo Errlog output */
						hfc_fx_watchdog_enter(pp, core_wk, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);	/* FCLNX-GPL-FX-190 */
						clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);							/* FCLNX-GPL-FX-190 */
						hfc_fx_notify_tout(pp, core_wk, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */
						hfc_fx_cancel_weque(pp, core_wk, target, 0, NULL, SCS_CMD_RESET, HFC_CSCSI_ERROR, HFC_FLASH_TARGET);	/* FCLNX-GPL-FX-112 */
						wk_dev = target->dev;
						while(wk_dev != NULL) {			/* FCLNX-GPL-FX-190 Start */
							wk_dev->lustat = 0x00;
							wk_dev->dev_core_stat.all = 0;
							hfc_fx_mp_watchdog_enter(pp, core_wk, target, hfcp, wk_dev, wk_dev->lun, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);
							wk_dev = wk_dev->next;
						}								/* FCLNX-GPL-FX-190 End */
					}
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
						hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_CMD_RESET, HFC_CSCSI_ERROR,
							FALSE, FALSE, FALSE, TRUE, FALSE, HFC_FLASH_TARGET);
					}
				}
			}
		}
		/* FCLNX-GPL-FX-014 End */
		
		/* Start SCSI Delay Timer  */
		if( pp->scsi_reset_delay ){	/* FCLNX-GPL-FX-112 */
			hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_DELAY_TMR, 0, TRUE);
			rtn = hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_DELAY_TMR, 0, FALSE);
			if( !rtn ) set_bit(HFC_TS_SCSI_DELAY, (ulong *)&target->status);
		}
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_MGM_CHK, 0x20, pp, hfcp->rp, core, target, hfcp,
			0, 0, 0 );
		
		hfc_fx_set_cmnd_res(pp, core, Scmd, hfcp, result);
		if (!hfc_manage_info.hfcldd_mp_mod) {
			hfc_fx_iodone(pp, core, Scmd, hfcp);
		}else{
			if( !rc_error ){
				hfc_fx_iodone(pp, core, Scmd, hfcp);
			}
		}
		
		return (rc_error);
	}

	if ( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&hfcp->cmd_flags)
	  || test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags) )	/* FCLNX-GPL-FX-014 */
	{	/* Response of Abort Task Set or LUN Reset --*/
	
		/* FCLNX-GPL-FX^014 Start */
		dev = (struct dev_info_fx *)hfc_fx_search_dev_info( ((struct hfc_pkt_fx *)(ulong)xrb->xcrb.hfc_pkt)->target, hfcp->lun_id );
		
		if( dev == NULL ){
			hfc_fx_hand2_trace(
				HFC_FX_TRC_MGM_CHK, 0x32, pp, hfcp->rp, core, target, hfcp,
				0, 0, 0 );
			return(rc_error);
		}
		
		if ( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags) 
		  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags)){
			clear_bit(HFC_DC_WAIT_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);
		}
		if (test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&hfcp->cmd_flags)){
			clear_bit(HFC_DC_WAIT_CSCSI_LU_WAIT_DMA, (ulong *)&dev->dev_core_stat.core[core->core_no]);
		}
		if (test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&hfcp->cmd_flags)) {
			clear_bit(HFC_DC_WAIT_CSCSI_LU_WITHOUT_DMA, (ulong *)&dev->dev_core_stat.core[core->core_no]);
			need_next_reset = 1;
		}
		/* FCLNX-GPL-FX^014 End */
		
		/* Dequeue hfc_pkt_fx from wait_end_que */
		hfc_fx_deque_we_que(pp, target, hfcp);

		if( rc_error == 0 )
		{
			hfcp->adap_status = SCS_NORMAL_END;
			Scmd->result |= DID_OK << 16;
		}
		else
		{
			if(test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat)
			|| test_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat)){	/* FCLNX-GPL-FX-255,272 */
				set_bit(HFC_DS_FAIL_ABORT, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
				clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
				clear_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
			}else{
				set_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
				clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
				clear_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
			}
			
			hfcp->adap_status = SCS_ABORT_FAILED;
			Scmd->result |= DID_ABORT << 16;
			
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				if(hfc_fx_check_cmnd_timeout(pp,core_wk,target,&hfcp->lun_id)){	/* FCLNX-GPL-FX-014 */
					hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);	/* FCLNX-GPL-FX-190 */
					if(pp->rt_err_enable){
						if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
							hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);
							break;
						}
						else{
							hfc_fx_watched_errcount_i(pp, NULL, HFC_RT_ERR);
							break;
						}
					}
					else{
						if(pp->tgtrst_restrain){
							set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
							hfc_fx_abend(pp, core_wk, HFC_ABEND_LINK_RESET);
							break;
						}else{	/* FCLNX-GPL-FX-014 */
							/* Cancel wait queue of SCSI command and Task Management *//* FCLNX-GPL-FX-092 */
							for (j=0 ; j< MAX_CORE_PROBE_FX ; j += (MAX_CORE_PROBE_FX/pp->core_num)) {
								if ((core_wk = pp->region_arg[pp->rid]->core_arg[j]) == NULL)
									continue;
								hfc_fx_cancel_scsi_cmd(pp,core_wk,target,0,NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
								FALSE,TRUE, HFC_FLASH_TARGET );
							}/* FCLNX-GPL-FX-092 */
							if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
								hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
									TRUE, FALSE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
							}
							
							/* FCLNX-GPL-FX-112 Start */
							set_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
							
							target->target_reset_cmnd = hfcp->cmd_pkt;
							
			 				if(hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))){
								clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
								break;
							}
							/* FCLNX-GPL-FX-112 End */
			 				
							if((pp->hba_isolation == HFC_ISOL_START)&&(pp->total_tgtrst_to)){
								hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
		 						hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, FALSE);
							}
							break;
						}	/* FCLNX-GPL-FX-014 */
					}
				}
			}
		}
		
		/* FCLNX-GPL-FX-014 Start */
		if (dev->dev_core_stat.all == 0){
			if(test_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat)){
				clear_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
				if(!test_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat))
					lun_reset_end=1;	/* FCLNX-GPL-FX-255,272 */
			}else if(test_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat)){
				clear_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
			}
			if((!test_bit(HFC_DS_FAIL_ABORT, (ulong *)&dev->lustat))&&(!test_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat))){
				if (need_next_reset) {
					// Issue AbortTaskSet and C_SCSI(wait stop DMA)
					if(test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat)){
						hfc_fx_issue_devrst_cscsi(
					             pp, target, dev, (((0x00000001 << CFLAG_LUN_RESET) | (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA))));
					}else if(test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat)){
						hfc_fx_issue_devrst_cscsi(
					             pp, target, dev, (((0x00000001 << CFLAG_ABORT) | (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA))));
					}
				} else { // AbortTaskSet, Cancel_scsi Success
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
							continue;
						/* SoftLo Errlog output */
						hfc_fx_mp_watchdog_enter(pp, core_wk, target, hfcp, dev, dev->lun, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);	/* FCLNX-GPL-FX-190 */
						hfc_fx_notify_tout(pp, core_wk, target, dev->lun, HFC_FLASH_DEV);	/* FCLNX-GPL-596 */
						hfc_fx_cancel_weque(pp, core_wk, target, dev->lun, NULL, SCS_CMD_ABORTED, 0, HFC_FLASH_DEV);
					}
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
						hfc_fx_mq_cancel_scsi_cmd(pp, target, dev->lun, NULL, SCS_CMD_ABORTED, 0,
							FALSE, FALSE, FALSE, TRUE, FALSE, HFC_FLASH_DEV);
					}
				}
			}
		}
		/* FCLNX-GPL-FX-014 End */
		
		/* Start LUN Reset Delay Timer  */
		if( lun_reset_end ){	/* FCLNX-GPL-FX-255,272 */
			if( dev != NULL ){
				if( pp->lun_reset_delay ){
					hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_DELAY_TMR_DEV, 0, TRUE);
					rtn = hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_DELAY_TMR_DEV, 0, FALSE);
					if( !rtn ) hfc_fx_set_dev_info_fx(dev);
				}
			}
		}
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_MGM_CHK, 0x31, pp, hfcp->rp, core, target, hfcp,
			0, 0, 0 );
		
		hfc_fx_set_cmnd_res(pp, core, Scmd, hfcp, result);
		if (!hfc_manage_info.hfcldd_mp_mod) {
			hfc_fx_iodone(pp, core, Scmd, hfcp);
		}else{
			if( !rc_error ){
				hfc_fx_iodone(pp, core, Scmd, hfcp);
			}
		}
		
		return (rc_error);
	}
	
	hfc_fx_errlog(pp,core,target,hfcp,HFC_ERRLOG_TYPE_XRB,ERRID_HFCP_EVNT3,0x80,core->logdata,16) ;

	return (rc_error);
}	


/*
 * Function:    hfc_fx_notify_tout
 *
 * Purpose:     This routine get a soft log data and organize it.
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  target     - pointer to target_info_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_notify_tout(
	struct port_info			*pp,
	struct core_info			*core,
	struct target_info_fx		*target,
	uint						lun,
	uchar						type)
{
	struct hfc_pkt_fx			*hfcp;
	uint						hash;
	uchar						err_no;	/* FCLNX-GPL-FX-091 */
	struct dev_info_fx			*dev=NULL;	/* FCLNX-GPL-FX-112 */

	/* This subroutine does not need to dequeue and iodone time-out SCSI process */
	/*  (These termination process owes to higher level reset) */
	
	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		if (target->core_queue[core->core_no].we_que_top[hash] != NULL)
		{	/* hfcp exists in queue */
			hfcp = target->core_queue[core->core_no].we_que_top[hash];
			
			while( hfcp != NULL )
			{
				if ( ( type == HFC_FLASH_TARGET )
				|| ( ( type == HFC_FLASH_DEV )&&( hfcp->lun_id == lun ) ) ){/* FCLNX-GPL-596 */
				
					if ( test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags) )
					{	/* If pkt has MIHLOG */
						memset( hfcp->core->logdata, 0, 16 );
						memcpy( hfcp->core->logdata, (uchar*)&hfcp->core->xob[hfcp->cmd_xob].fcp_cmd.fcp_cntl, 4 );
						
						/* FCLNX-GPL-FX-091 Start */
						if(!( hfcp->cmd_flags & CFLAG_RESET_ANY )){
							err_no = 0x24;
						}
						else if(test_bit(CFLAG_ABORT,(ulong *)&hfcp->cmd_flags)||test_bit(CFLAG_LUN_RESET,(ulong *)&hfcp->cmd_flags)){
							err_no = 0x26;
						}
						else if(test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)||test_bit(CFLAG_BUS_RESET,(ulong *)&hfcp->cmd_flags)){
							err_no = 0x29;
						}else{
							hfcp = hfcp->cmd_forw ;
							continue;
						}/* FCLNX-GPL-FX-091 End */
						
						if( test_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags) )
						{
							HFC_DBGPRT("hfc_fx_notify_tout - soft_log valid rid=%d\n", pp->rid);
							hfc_fx_errlog( pp, core, hfcp->target, hfcp, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, err_no, hfcp->core->logdata, 16 ); /* FCLNX-GPL-FX-091 */
							clear_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags);
						}
						else {
							HFC_DBGPRT("hfc_fx_notify_tout - soft_log invalid rid=%d\n", pp->rid);
							hfc_fx_errlog( pp, core, hfcp->target, hfcp, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRA, err_no, hfcp->core->logdata, 16 ); /* FCLNX-GPL-FX-091 */
						}
						
						if (test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags ) && (!( hfcp->cmd_flags & CFLAG_RESET_ANY ))) /* FCLNX-GPL-FX-091 */
						{
							hfc_manage_info.npubp->hfc_fx_make_fcinfo(pp, hfcp, 0x24, 
									hfcp->adap_status, hfcp->core->logdata, sizeof(hfcp->core->logdata));
						}
					}
				}
				hfcp = hfcp->cmd_forw ;
			}
		}
	}
	/* FCLNX-GPL-FX-112 Start */
	dev = target->dev;
	while( dev != NULL){
		if ( ( type == HFC_FLASH_TARGET )
		|| ( ( type == HFC_FLASH_DEV )&&( dev->lun == lun ) ) ){
			clear_bit(HFC_DS_FAIL_ABORT, (ulong *)&dev->lustat);
		}
		dev = dev->next;
	}
	/* FCLNX-GPL-FX-112 End */
}


/*
 * Function:    hfc_fx_deque_we_que
 *
 * Purpose:     This routine deque a hfc_pkt_fx entry from the wait_end_queue
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  target     - pointer to target_info_fx
 *  hfcp       - pointer to hfc_pkt_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_deque_we_que(
	struct port_info			*pp,
	struct target_info_fx		*target,
	struct hfc_pkt_fx			*hfcp)
{
	int							tblno;
	struct hfc_pkt_fx			*hfcp_wk;
	struct core_info			*core;
	uchar						target_id;
	
	core = hfcp->core;
	tblno = hfcp->lun_id % HASH_T_NUM;
	target_id = hfcp->target_id;
	
	if(target->core_queue[core->core_no].we_que_top[tblno] != NULL)
	{	/* One or more targets exist in we_que */
		if( target->core_queue[core->core_no].we_que_top[tblno] == target->core_queue[core->core_no].we_que_end[tblno] )
		{	/* Depth of this queue is only one */
			target->core_queue[core->core_no].we_que_top[tblno] = NULL ;
			target->core_queue[core->core_no].we_que_end[tblno] = NULL ;
		}
		else
		{	/* Depth of this queue is two or more */
			if( hfcp == target->core_queue[core->core_no].we_que_top[tblno])
			{	/* Dequeue from the top of the queue */
				target->core_queue[core->core_no].we_que_top[tblno] = hfcp->cmd_forw;
				hfcp->cmd_forw->cmd_prev  = NULL;
			}
			else if(hfcp == target->core_queue[core->core_no].we_que_end[tblno])
			{	/* Dequeue from the end of the cue  --*/
				target->core_queue[core->core_no].we_que_end[tblno] = hfcp->cmd_prev;
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
		
		core->we_que_sizecnt -= hfcp->data_size;
		core->we_que_cnt_all--;
		target->core_queue[core->core_no].we_que_cnt--;
		
		if( hfcp->cmd_flags & CFLAG_RESET_ANY){	/* FCLNX-GPL-FX-172 */
			if (core->tskmgm_cmd_num != 0) {
				core->tskmgm_cmd_num--;
			}
		}	/* FCLNX-GPL-FX-172 */
		
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() && hfcp->dev ){
			hfc_manage_info.npubp->hfc_fx_queue_count(hfcp, 1, 1);	/* we dequeue */
			if((hfcp->cmd_pkt != NULL) 
			&& (((uchar)hfcp->cmd_pkt->cmnd[0] == 0x5E) || ((uchar)hfcp->cmd_pkt->cmnd[0] == 0x5F))){
				HFC_DBGPRT("hfc_fx_deque_we_que : core->ppreserve_cmd_num = %d\n",core->preserve_cmd_num);
				core->preserve_cmd_num--;
			}
		}
	}
}


/*
 * Function:    hfc_fx_mb_errlog
 *
 * Purpose:     Get error log of mail box
 *
 * Arguments:   
 *  pp            - Pointer to port_info
 *  target        - Pointer to target_info_fx
 *	core		  - Pointer to core_info
 *  trc_id        - ID of caller
 *  rc_passthoruh - Hfc_mb_pass_throuh return value
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mb_errlog(
	struct port_info        *pp,
	struct target_info_fx      *target,
	struct core_info		*core,
	uchar                   trc_id,
	uint                    rc_passthrouh)
{
	uint                    err_num=0, mb_code=0;
	uint                    err_id,event_level=0;
	uchar                   type=0;
	struct mailbox_fx       *mbox = core->mb;
	int						disable_log = 0; /* FCWIN-GPL-503 */

	switch(trc_id)
	{
	case HFC_FX_TRC_LGINRSP : case HFC_FX_TRC_PRLIRSP : case HFC_FX_TRC_CSCSIRSP : /* response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :	/* FCLNX-GPL-FX-153 */
			if ( ( core->mb_results & 0x00ffffff ) == 0x040403 ){
			    err_num = 0x0e;     type = HFC_ERRLOG_TYPE_MBRESP;
			}else{
			    err_num = 0x0c;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
			}
			if ((pp->limit_log == HFC_ENABLE_LIMITLOG)&&(trc_id!=HFC_FX_TRC_CSCSIRSP)){ /* FCLNX-GPL-503 */
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LGINRSP, 0x30, pp, core->rp, core, target, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				disable_log = 1;
			}
			break;	/* FCLNX-GPL-FX-153 */
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x0d;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x0e;     type = HFC_ERRLOG_TYPE_MBRESP;  
			if ((pp->limit_log == HFC_ENABLE_LIMITLOG)&&(trc_id!=HFC_FX_TRC_CSCSIRSP)){ /* FCLNX-GPL-503 */
				hfc_fx_hand2_trace(
					HFC_FX_TRC_LGINRSP, 0x30, pp, core->rp, core, target, NULL,
					(uint64_t)rc_passthrouh, 0, 0 );
				disable_log = 1;
			}
			break;
		}
		break;
	case HFC_FX_TRC_PDISCRSP :         /* PDISC response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x10;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x11;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x12;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_FX_TRC_GIDFTRSP : case  HFC_FX_TRC_GCSIDRSP : case HFC_FX_TRC_GPNFTRSP	:  /* GIDFT response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x7b;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x7c;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x7d;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_FX_TRC_GIDPNRSP :         /* GIDPN response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x81;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x82;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x83;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
	case HFC_FX_TRC_GPNIDRSP :         /* GPNID response processing    */
		switch(rc_passthrouh)
		{
		case HFC_MBPASS_RETRY_OVER :    err_num = 0x84;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		case HFC_MBPASS_RETRY_FAIL :    err_num = 0x85;     type = HFC_ERRLOG_TYPE_NONE;    break;
		case HFC_MBPASS_ERROR      :    err_num = 0x86;     type = HFC_ERRLOG_TYPE_MBRESP;  break;
		}
		break;
		
	case HFC_FX_TRC_DEL_PORTIDRSP	: case HFC_FX_TRC_LOGORSP		 : case HFC_FX_TRC_OFFLINEMBRSP	:
	case HFC_FX_TRC_PRLORSP			: case HFC_FX_TRC_RFTIDRSP		 : case HFC_FX_TRC_RFFIDRSP		:
	case HFC_FX_TRC_SHADOWUPRSP		: case HFC_FX_TRC_LDCH_TRCLOGRSP :
		                                err_num = 0xDB;     type = HFC_ERRLOG_TYPE_MBRESP;	
										event_level=1;
		break;
		
	case HFC_FX_TRC_MIHLGRSP :         /* MIHLOG response processing    */
		                                err_num = 0x7e;     type = HFC_ERRLOG_TYPE_MBRESP;
		break;
		
	case HFC_FX_TRC_LINKRSP 		: case HFC_FX_TRC_FLOGIRSP		: case HFC_FX_TRC_ADD_PORTIDRSP	:
	case HFC_FX_TRC_PLOGIRSP		: case HFC_FX_TRC_SCRRSP		: case HFC_FX_TRC_CORESTARTRSP	:/* Link Initialize Response */
		 
		if(test_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1)|| pp->mck_linkup == HFC_LINKUP_MCK){
										err_num = 0x1a;
										event_level=1;
		}
		else{
										err_num = 0x8a;
		}
		if(rc_passthrouh == HFC_MBPASS_RETRY_FAIL){
															type = HFC_ERRLOG_TYPE_NONE;
		}
		else{/* HFC_MBPASS_RETRY_OVER or HFC_MBPASS_ERROR */
															type = HFC_ERRLOG_TYPE_MBRESP;
		}
		break;
	}
	
	if( (rc_passthrouh == HFC_MBPASS_ERROR) && (mbox->mb_resp.fsb & HFC_FSB_PC) ) {
		err_id = ERRID_HFCP_ERR9;
		err_num = 0xAB;				/* FCLNX_GPL-0114 */
	}
	else if(event_level == 1){
		err_id = ERRID_HFCP_EVNT2;
	}
	else {
		err_id = ERRID_HFCP_ERR6;
	}
	
	memset((void *)core->logdata, 0, 16);
	
	memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code,4);
	
	HFC_4B_TO_4L(mb_code, mbox->mb_resp.mb_code);
	if((mb_code & 0xffff0000) == HFC_MBCMD_SNDRCV){
		memcpy(&core->logdata[2],(char *)&core->payload->send_payload.data0[0],2);
	}
	core->logdata[5] = mbox->mb_resp.esw ;
	core->logdata[6] = mbox->mb_resp.ssn ;
	core->logdata[7] = rc_passthrouh ;
	memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
	core->logdata[12] = mbox->mb_resp.fsb ;
	memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);
	
	if(( err_num != 0)&&(!disable_log)) { /* FCLNX-GPL-503 */
		hfc_fx_errlog(pp, core, target, NULL, type, err_id, err_num, core->logdata, 16);
	}

	return;
}


void hfc_fx_timeout_by_mb_delay(struct port_info *pp)
{
	struct core_info *core = NULL;
	
	if ((pp == NULL) || (pp->region_arg[pp->rid] == NULL) || 
	    (pp->region_arg[pp->rid]->core_arg[pp->master_core_no] == NULL))
    {
		HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - invalid parameter() NG.\n");
		return;
	}
	core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
	
	if( test_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1) ){
		/* core_start mailbox for all the cores */
		if (hfc_fx_all_core_start(pp) != 0) {
			HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - all_core_start() NG.\n");
			/* all the cores failed. Execute Checkstop. */
//			hfc_fx_hw_chk_stop(pp);			/* TBD */

//			/* Set only Hyper inter. */		/* TBD */
//			if (HFC_FX_MMODE_CHECK_SHARED(pp)) {
//				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
//					if(pp->region_arg[pp->rid] != NULL){
//						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
//							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
//							  ( char )0x4, (HFC_MLPF_REC_END | HFC_MLPF_ERR_INT), HFC_FX_CORE_OFFSET10);
//						}
//					}
//				}
//			}
		}
	}
	else if( test_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1) ){
//		if (hfc_fx_issue_linkini(pp, core) != 0) {	/* TBD */
		if (hfc_fx_issue_linkini(pp) != 0) {		/* TBD */
			HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - issue_linkini() NG.\n");

			hfc_fx_initialize_failed(pp, core, NULL);
			return;
		}
	}
	else if( test_bit(HFC_PD_NEED_FLOGI, (ulong *)&pp->status_detail1) ){
		if (hfc_fx_issue_flogi(pp) != 0) {
			HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - issue_flogi() NG.\n");

			hfc_fx_initialize_failed(pp, core, NULL);
			return;
		}
	}
	else if( test_bit(HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1) ){
		/* add port_id mailbox for all the cores */
//		if (hfc_fx_all_add_portid(pp) != 0) {		/* TBD */
//			HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - hfc_fx_all_add_portid() NG.\n");
//		
//			hfc_fx_initialize_failed(pp, core, NULL);
//			return;
//		}
	}
	else if( test_bit(HFC_PD_NEED_SCR, (ulong *)&pp->status_detail1) ){
		if (hfc_fx_issue_frmsndrcv(pp, NULL, 0, HFC_SNDRCV_SCR) != 0) {
			HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - SNDRCV_SCR() NG.\n");
		
			hfc_fx_initialize_failed(pp, core, NULL);
			return;
		}
	}
	else if( test_bit(HFC_PD_NEED_PLOGI_N, (ulong *)&pp->status_detail1) ){
		if (hfc_fx_issue_plogi(pp, NULL) != 0) {
			HFC_DBGPRT("hfc_fx_timeout_by_mb_delay - hfc_fx_issue_plogi(NameServer) NG.\n");
		
			hfc_fx_initialize_failed(pp, core, NULL);
			return;
		}
	}
}

void hfc_fx_timeout_by_mb_intvl(struct port_info *pp, struct core_info *core)
{
	core->mb_resp	= 0;
	core->mb_results = 0;

	/* retry mailbox */			/* TBD */
//	if ( hfc_fx_mb_passthrough(pp, core,
//					core->mb_retry_tid,
//					core->mb_retry_tout,
//					core->mb_retry_cnt,
//					core->mb_callback) !=0 ) 
//	{
//		core->mb_results = 0x80000000;
//		core->mb_resp = HFC_MBR_RETRYFAIL_FX;  
//	}
}


/*
 * Function:    hfc_fx_timeout_by_mailbox
 *
 * Purpose:     Timeout operation of mail box
 *
 * Arguments:   
 *  pp            - Pointer to port_info
 *	core		  - Pointer to core_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_timeout_by_mailbox(
	struct port_info *pp,
	struct core_info *core)
{
//	uint	logdata[4];
	
	core->mb_resp = HFC_MBR_TIMEOUT;  
	memset((void *)core->logdata, 0, 16);
	core->logdata[0] = core->mb->mb_init.mb_code ;
	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT,
	              ERRID_HFCP_EVNT4, 0x2a, core->logdata, 16) ;
	hfc_fx_hand2_trace(
		HFC_FX_TRC_WDOG, 0xff, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );

	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status) ){
		hfc_fx_abend(pp, core, HFC_ABEND_MB_TOUT);
	}
	
	return ;
}


/*
 * Function:    hfc_fx_issue_intl_start
 *
 * Purpose:     Execute hfc_fx_start() 
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core       - pointer to core_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_issue_intl_start(struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target)
{
	struct target_info_fx	*target_wk;
	uchar					start_flag = FALSE ;
	
	HFC_ENTRY("hfc_fx_issue_intl_start");
	
	if (target->core_queue[core->core_no].wx_que_cnt > 0)
	{
		start_flag = TRUE ;
	}
	
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
		/* use physical target for sstatus check */
		target_wk = pp->pport->target_arg[target->pseq];
	}
	else {
		target_wk = target;
	}

	if( start_flag == TRUE)
	{
		if((test_bit(HFC_PS_ONLINE, (ulong *)&pp->status))
			&& !(test_bit( HFC_PS_ISOL, (ulong *)&pp->status ))
			&& !(pp->status & HFC_PS_BLOCKED_SCSI)
			&& !(pp->pport->status & HFC_PS_BLOCKED_SCSI_PPORT)
			&& !( test_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2)))	/* FCLNX-GPL-FX-224 */
//			&& !(target_wk->status & HFC_TS_BLOCKED_SCSI))	/* FCLNX-GPL-FX-181 */
		{
			/* The status of adaptor, target and device can execute SCSI command */
			hfc_fx_start(pp, rp, core, target) ;
			
			if (target->core_queue[core->core_no].next_dstart_flag) {
				/* Enqueue this target to the end of the queue */
				if( (core->next_dstart_cnt > 1) && (core->next_dstart_end != target) )
				{
					hfc_fx_deque_next_dstart(pp, rp, core, target);
					hfc_fx_enque_next_dstart(pp, rp, core, target);
				}
			}
			return;
		}
	}
	
	
	if( ( target->core_queue[core->core_no].wx_que_cnt > 0 ) &&
		!test_bit(HFC_TS_SCSI_DELAY, (ulong *)&target_wk->status) )
	{
		/* Enqueue this target to the end of the queue */
		if( (core->next_dstart_cnt > 1) && (core->next_dstart_end != target) )
		{
			hfc_fx_deque_next_dstart(pp, rp, core, target);
		}
		
		hfc_fx_enque_next_dstart(pp, rp, core, target);
	}
	else if( target->core_queue[core->core_no].wx_que_cnt == 0 )
	{
		hfc_fx_deque_next_dstart(pp, rp, core, target);
	}
	
	HFC_EXIT("hfc_fx_issue_intl_start");
}

/*
 * Function:    hfc_fx_frame_a_data
 *
 * Purpose:     Create "Frame_A Data"
 *
 * Arguments:   
 *  mbox       - pointer to mailbox
 *  type       - HBA Package Type
 *  frame_type - Frame Type of mailbox intreq
 *	rid		   - Region ID
 *
 * Returns:     Frame_A Data
 *
 * Notes:       
 */
uint hfc_fx_frame_a_data(struct mailbox_fx *mbox, uchar type, ushort frame_type, uchar rid){
	/* Values */
	uint frame_a = 0x00000000;

	/* Check HBA Package Type */
	if( type == HFC_PKTYPE_FIVE_FX ){
		switch(frame_type){
			case HFC_MBINTREQ_DCE :
				frame_a = 0x2000a010;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_COU :
				frame_a = 0x2000a020;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_LINKUP :
				frame_a = 0x2000b080;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_LINKDOWN :
				frame_a = 0x2000b088;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_RCVFRM :
				frame_a = 0x2800b010;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_RCVFLOGI :
				frame_a = 0x2800b006;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_RCVPLOGI :
				frame_a = 0x2800b004;
				frame_a |= (uint)(rid << 16);
				break;
			case HFC_MBINTREQ_RCVPDISC :
				frame_a = 0x2800b005;
				frame_a |= (uint)(rid << 16);
				break;
			default:
				break;
		}
	}

	return frame_a;
}

/*
 * Function:    hfc_fx_identify_int_factor
 *
 * Purpose:     Get "INT_A reg" 
 *
 * Arguments:
 *  pp         - pointer to port_info
 *  vector     - vector#
 *  type       - INT type ( INTx, MSI, MSI-X )
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_fx_identify_int_factor(struct port_info *pp, int vector, int type)
{
#if 0
	/* Values */
	uint int_a_reg = 0x00000000;

	switch(type){
		/******* INTx, MSI ******************************/
		case HFC_INT_TYPE_INTX:
		case HFC_INT_TYPE_MSI:
			/* Read INT_A register */
			int_a_reg = (uint)hfc_fx_read_reg(pp, (uint)HFC_IOSPACE_INTA, (char)0x4);
			break;

		/*******  MSI-X *********************************/
		case HFC_INT_TYPE_MSIX:
			/* Check XRB */
			if(pp->entries[HFC_MSIX_XRB].vector == vector){
				int_a_reg = HFC_XRB_RSP;
				break;
			}
			
			/* Read INT_A register */
			int_a_reg = (uint)hfc_fx_read_reg(pp, (uint)HFC_IOSPACE_INTA, (char)0x4);
			break;

		/******* Unkonwn type **************************/
		default:
			/* Nop */
			break;
	}
	return int_a_reg;
#endif
	return 0;
}

struct port_info *hfc_fx_get_port_info_by_portid(struct port_info *pp, struct region_info *rp, struct core_info *core)
{
	struct port_info	*wkpp;
	struct mailbox_fx	*mbox ;
	uchar				mb_int0=0, mb_int1=0;
	ushort				mb_code;
	uchar				d_id[4] = {0,0,0,0};
	uchar				buf[4]  = {0,0,0,0};
	int					rcv_did = 0, hit = 0;
	int					i;
	
	HFC_ENTRY("hfc_fx_get_port_info_by_portid");
	HFC_DBGPRT("hfc_fx_get_port_info_by_portid - rid=%d, sub_rid=%d", pp->rid, pp->sub_rid);
	
	mbox = core->mb;
	mb_int0 = hfc_fx_read_val( mbox->mb_intreq.mb_code[0] );
	mb_int1 = hfc_fx_read_val( mbox->mb_intreq.mb_code[1] );
	mb_code = (ushort)(mb_int0 << 8) + (ushort)(mb_int1);
	HFC_DBGPRT("hfc_fx_get_port_info_by_portid - mb_int0=0x%02x, mb_int1=0x%02x, mb_code=0x%04x\n",
					mb_int0, mb_int1, mb_code);
	
	if (mb_int0 == 0xb0) {
		switch (mb_code)
		{
			case HFC_MBINTREQ_RCVFRM :
				d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[0] );
				d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[1] );
				d_id[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvfrm.fcph_hdr.d_id[2] );
				rcv_did = 1;
				HFC_DBGPRT("hfc_fx_get_port_info_by_portid - HFC_MBINTREQ_RCVFRM\n");
				break;
			case HFC_MBINTREQ_RCVFLOGI :
				d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[0] );
				d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[1] );
				d_id[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvflogi.fcph_hdr.d_id[2] );
				rcv_did = 1;
				HFC_DBGPRT("hfc_fx_get_port_info_by_portid - HFC_MBINTREQ_RCVFLOGI\n");
				break;
			case HFC_MBINTREQ_RCVPLOGI :
				d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[0] );
				d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[1] );
				d_id[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fcph_hdr.d_id[2] );
				rcv_did = 1;
				HFC_DBGPRT("hfc_fx_get_port_info_by_portid - HFC_MBINTREQ_RCVPLOGI\n");
				break;
			case HFC_MBINTREQ_RCVPDISC :
				d_id[1] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[0] );
				d_id[2] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[1] );
				d_id[3] = hfc_fx_read_val( mbox->mb_intreq.type.rcvpdisc.fcph_hdr.d_id[2] );
				rcv_did = 1;
				HFC_DBGPRT("hfc_fx_get_port_info_by_portid - HFC_MBINTREQ_RCVPDISC\n");
				break;
			default:
				return (pp);
		}
	}
	HFC_DBGPRT("hfc_fx_get_port_info_by_portid - d_id[1]=0x%02x, d_id[2]=0x%02x, d_id[3]=0x%02x\n",
					d_id[1], d_id[2], d_id[3]);
	
	wkpp = pp->vport_ptr[0].vport_arg;
	
	if (rcv_did) {
		for (i=0; i<=pp->max_vport_count; i++) {
			wkpp = pp->vport_ptr[i].vport_arg;
			if (!wkpp)
				continue;
			
			HFC_4L_TO_4B(buf, wkpp->scsi_id);
			HFC_DBGPRT("hfc_fx_get_port_info_by_portid - buf=0x%02x%02x%02x%02x\n",
					buf[0], buf[1], buf[2], buf[3]);
			if (memcmp(&buf[1], &d_id[1], 3)) {
				continue;
			}
			else {
				hit = 1;
				break;
			}
		}
		if (!hit) {
			wkpp = NULL;
		}
	}
	
	if (wkpp != NULL) {
		HFC_DBGPRT("hfc_fx_get_port_info_by_portid - rid=%d, sub_rid=%d", wkpp->rid, wkpp->sub_rid);
	}
	
	HFC_EXIT("hfc_fx_get_port_info_by_portid");
	
	return (wkpp);
}

