/*
 * hfcl_timer_recovery_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char timer_fx_rcsid[] = "$Id: hfcl_timer_recovery_fx.c,v 1.1.2.67.2.17.2.7.2.12 2015/07/29 08:04:08 toyo Exp $";

/*--------------------------------------------------------------------------*/
/*    hfc_fx_handler()                                                         */
/*        +--hfc_fx_hwerr_int()                                                */
/*        +--hfc_fx_mb_resp()                                                  */
/*        |      +--hfc_fx_issue_relogin()                                     */
/*        |      +--hfc_fx_cancel_scsi_cmd()                                   */
/*        |      +--hfc_fx_issue_pdisc()                                       */
/*        +--hfc_fx_mb_intreq()                                                */
/*        |      +--hfc_fx_cancel_scsi_cmd()                                   */
/*        +--hfc_fx_xrb_resp()                                                 */
/*        |      +--hfc_fx_abend()                                             */
/*        |      +--hfc_fx_link_chk()                                          */
/*        |      +--hfc_fx_task_mgm_chk()                                      */
/*        |      |      +--hfc_fx_deque_we_que()                               */
/*        |      |      +--hfc_fx_iov_update()                                 */
/*        |      |      +--hfc_fx_dma_unmap()                                  */
/*        |      |      +--hfc_fx_start()                                      */
/*        |      |      +--hfc_fx_cancel_weque()                               */
/*        |      +--hfc_fx_scsi_chk()                                          */
/*        |             +--hfc_fx_enque_wr_que()                               */
/*        |             +--hfc_fx_deque_we_que()                               */
/*        |             +--hfc_fx_iov_update()                                 */
/*        |             +--hfc_fx_dma_unmap()                                  */
/*        |                                                                 */
/*--------------------------------------------------------------------------*/


#include "hfcldd.h"
#include "hfcl_tbol.h"
#include "hfcl_strategy.h"
#include "hfcl_handler.h"
#include "hfcl_stra_trace.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_top.h"
#include "hfcl_hand_timer_trace.h"
#include "hfcl_detect.h"
#include "hfcl_ioctl.h"
#include "hfcl_mlpf.h"
#include "hfcldd_conf.h"

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

#ifndef _HFC_NO_RASLOG
#include "hraslog.h"
#endif

#if defined(__x86_64)  /* FCLNX-0398 */ /* FCLNX-0629 */
#include <linux/nmi.h>
#endif			/* FCLNX-0398 */ /* FCLNX-GPL-564 */

extern const struct errlog_t errlog_info[];
extern const struct hraslog_t hraslog_info[];

/* FCLNX-GPL-547 start */
enum log_files_fx {
	HFC_SYSLOG  = 0,
	HFC_DRV_LOG = 1,
	HFC_ALL_LOG = 2  /* syslog and driver_log */
};

enum log_parts_fx {
	HFC_LOG_HEAD = 0,
	HFC_LOG_BODY = 1, /* Example: "hfcldd0: HFC_ERRB FC Adapter Link Down (ErrNo:0x15)"  */
	HFC_LOG_DUMP = 2
};

enum log_prefixs_fx {
	HFC_LOG_ERR = 0,
	HFC_LOG_WRN = 1,
	HFC_LOG_INF = 2
};

const uchar
alarm_tbl_fx[ERRID_HFCP_TBL_END] = {
	FALSE , /*  0: reserved           */
	TRUE  , /*  1: ERRID_HFCP_ERR1    */
	TRUE  , /*  2: ERRID_HFCP_ERR2    */
	TRUE  , /*  3: ERRID_HFCP_ERR3    */
	TRUE  , /*  4: ERRID_HFCP_ERR4    */
	TRUE  , /*  5: ERRID_HFCP_ERR5    */
	TRUE  , /*  6: ERRID_HFCP_ERR6    */
	TRUE  , /*  7: ERRID_HFCP_ERR7    */
	TRUE  , /*  8: ERRID_HFCP_ERR8    */
	TRUE  , /*  9: ERRID_HFCP_ERR9    */
	TRUE  , /* 10: ERRID_HFCP_ERRA    */
	TRUE  , /* 11: ERRID_HFCP_ERRB    */
	FALSE , /* 12: ERRID_HFCP_ERRC    */
	TRUE  , /* 13: ERRID_HFCP_ERRD    */
	TRUE  , /* 14: ERRID_HFCP_ERRE    */
	TRUE  , /* 15: ERRID_HFCP_ERRF    */
	FALSE , /* 16: ERRID_HFCP_EVNT1   */
	FALSE , /* 17: ERRID_HFCP_EVNT2   */
	FALSE , /* 18: ERRID_HFCP_EVNT3   */
	FALSE , /* 19: ERRID_HFCP_EVNT4   */
	TRUE    /* 20: ERRID_HFCP_OPTERR0 */
};              /* 21: ERRID_HFCP_TBL_END */

const enum log_prefixs_fx
log_prefix_tbl_fx[ERRID_HFCP_TBL_END] = {
	HFC_LOG_INF , /*  0: reserved           */
	HFC_LOG_ERR , /*  1: ERRID_HFCP_ERR1    */
	HFC_LOG_ERR , /*  2: ERRID_HFCP_ERR2    */
	HFC_LOG_ERR , /*  3: ERRID_HFCP_ERR3    */
	HFC_LOG_WRN , /*  4: ERRID_HFCP_ERR4    */
	HFC_LOG_ERR , /*  5: ERRID_HFCP_ERR5    */
	HFC_LOG_WRN , /*  6: ERRID_HFCP_ERR6    */
	HFC_LOG_WRN , /*  7: ERRID_HFCP_ERR7    */
	HFC_LOG_WRN , /*  8: ERRID_HFCP_ERR8    */
	HFC_LOG_WRN , /*  9: ERRID_HFCP_ERR9    */
	HFC_LOG_WRN , /* 10: ERRID_HFCP_ERRA    */
	HFC_LOG_WRN , /* 11: ERRID_HFCP_ERRB    */
	HFC_LOG_WRN , /* 12: ERRID_HFCP_ERRC    */
	HFC_LOG_ERR , /* 13: ERRID_HFCP_ERRD    */
	HFC_LOG_ERR , /* 14: ERRID_HFCP_ERRE    */
	HFC_LOG_ERR , /* 15: ERRID_HFCP_ERRF    */
	HFC_LOG_WRN , /* 16: ERRID_HFCP_EVNT1   */
	HFC_LOG_WRN , /* 17: ERRID_HFCP_EVNT2   */
	HFC_LOG_WRN , /* 18: ERRID_HFCP_EVNT3   */
	HFC_LOG_WRN , /* 19: ERRID_HFCP_EVNT4   */
	HFC_LOG_INF   /* 20: ERRID_HFCP_OPTERR0 */
};                    /* 21: ERRID_HFCP_TBL_END */

#define HFC_FX_MAX_LOG_FILE_SELECTS 6
#define HFC_FX_MAX_LOG_PARTS        3

/* for Alarm Log */
const enum log_files_fx
log_file_tbl_alm_fx[HFC_FX_MAX_LOG_FILE_SELECTS][HFC_FX_MAX_LOG_PARTS] = {
	/* Log-Head   , Log-Body    , Log-Dump    */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=0 */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=1 */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=2 */
	{ HFC_ALL_LOG , HFC_ALL_LOG , HFC_DRV_LOG }, /* hfc_log_file=3 */
	{ HFC_DRV_LOG , HFC_DRV_LOG , HFC_DRV_LOG }, /* hfc_log_file=4 */
	{ HFC_ALL_LOG , HFC_ALL_LOG , HFC_DRV_LOG }  /* hfc_log_file=5 */
};

/* for Non-Alarm Log */
const enum log_files_fx
log_file_tbl_non_fx[HFC_FX_MAX_LOG_FILE_SELECTS][HFC_FX_MAX_LOG_PARTS] = {
	/* Log-Head   , Log-Body    , Log-Dump    */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=0 */
	{ HFC_ALL_LOG , HFC_ALL_LOG , HFC_DRV_LOG }, /* hfc_log_file=1 */
	{ HFC_DRV_LOG , HFC_DRV_LOG , HFC_DRV_LOG }, /* hfc_log_file=2 */
	{ HFC_DRV_LOG , HFC_DRV_LOG , HFC_DRV_LOG }, /* hfc_log_file=3 */
	{ HFC_DRV_LOG , HFC_DRV_LOG , HFC_DRV_LOG }, /* hfc_log_file=4 */
	{ HFC_ALL_LOG , HFC_ALL_LOG , HFC_DRV_LOG }  /* hfc_log_file=5 */
};
/* FCLNX-GPL-547 end */

#define HFC_MFC			512
#define HFC_MODELNAME	528
#define	HFC_FWVERSION	560
#define	HFC_DRVVERSION	576
#define	HFC_PARTSNUM	592
#define	HFC_LOCATION	608
#define	HFC_LOGSEQNUM	624
#define	HFC_IRQNUM		628
#define	HFC_EC			632
#define	HFC_ADAPWWPN	640
#define	HFC_DEVICEID	656
#define HFC_WARNING		660
#define HFC_SFP_INFO	760								/* FCLNX-GPL-311 */

#define HFC_COMMON_MCK		0x00000020
#define HFC_USER_MCK		0x00000080
#define HFC_DUMP_FRZ_CLR	0x00400000

				  
//static uchar	logdata[16] ;			/* FCLNX-GPL-391 */

/*
 * Function:    hfc_fx_hwerr_int
 *
 * Purpose:     Process Hardware error
 *
 * Arguments:   
 *  pp          - 
 *  int_a_reg   - 
 *  status_reg0 - 
 *  status_reg1 - 
 *  detail_reg  - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_hwerr_int(struct port_info *pp,
					uint int_a_reg,
					uint status_reg0,
					uint status_reg1,
					uint detail_reg,
					uchar abend_code)
{
//	uchar		  abend_code=0 ;
	uint64_t      status_reg;

	status_reg = ((uint64_t)status_reg0 << 32) & 0xffffffff00000000ULL;
	status_reg |= (uint64_t)status_reg1 & 0x00000000ffffffffULL;
	
	/* Align detail_reg with FPP format if package type is FIVE */
	if( pp->pkg.type != HFC_PKTYPE_FPP)                                 /* FCWIN-0240 */
		detail_reg <<= 8;

	HFC_DBGPRT(" hfcldd%d : hfc_fx_hwerr_int \n",pp->dev_minor);

	hfc_fx_hand2_trace(
		HFC_FX_TRC_HWERR, 0x00, pp, NULL, NULL, NULL, NULL,
		(uint64_t)int_a_reg, status_reg, (uint64_t)detail_reg);

#if 0
	if( int_a_reg & HFC_FX_HWERR_INT ) {
		
		clear_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ); /* FCLNX-GPL-137 */
		
		if( status_reg0 & HFC_FX_HWERR_T3  )	/* FCLNX-0269 */ /* FCLNX-GPL-61 */
		{
			abend_code = HFC_ABEND_T3 ;
		}
		else if( status_reg0 & HFC_FX_HWERR_MPCHK )
		{
			abend_code = HFC_ABEND_MPCHK ;
		}
		else
		{
			abend_code = HFC_ABEND_MCK_INT ;
		}
	}

	if( int_a_reg & HFC_HWERR_PCI )
	{
		if( status_reg0 & HFC_HWERR_SERR )
			abend_code = HFC_ABEND_SERR ;
		if( status_reg0 & HFC_HWERR_PERR )
			abend_code = HFC_ABEND_PERR ;
		if( status_reg0 & HFC_HWERR_SPERR )
			abend_code = HFC_ABEND_SPERR ;
	}
#endif

	hfc_fx_abend(pp, NULL, abend_code);

	HFC_DBGPRT(" hfcldd : hfc_fx_hwerr_int - end\n");
	hfc_fx_hand2_trace(
		HFC_FX_TRC_HWERR, 0x10, pp, NULL, NULL, NULL, NULL,
		(uint64_t)int_a_reg, status_reg, (uint64_t)detail_reg);
}


/*
 * Function:    hfc_fx_errlog
 *
 * Purpose:     Collect error log data 
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *  target     - Pointer to target_info_fx 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *  type       - Error log type
 *  err_id     - Error name
 *  err_num    - error_no (Error log)
 *  data       - Output data format
 *  data_len   - Output data length
 *
 * Returns:     
 *
 * context:     Interruption level/process level
 *
 * Notes:       
 */
void hfc_fx_errlog( struct port_info *pp,
			struct core_info		*core,
			struct target_info_fx	*target,
			struct hfc_pkt_fx		*hfcp,
			uchar					type,
			uint					err_id,
			uint					err_num,
			uchar					*data,
			ushort					data_len )
{
	struct hfc_err_rec		*err_rec;
	hfc_fx_errfmt1_t		*err1;				/* errlog format1 area pointer */
	hfc_fx_errfmt2_t		*err2;				/* errlog format2 area pointer */
	hfc_fx_errfmt3_t		*err3;				/* errlog format3 area pointer *//* FCLNX-GPL-FX-139 */
	hfc_fx_errfmt4_t		*err4;				/* errlog format4 area pointer *//* FCLNX-GPL-FX-139 */
	struct dev_info_fx		*dev=NULL;
	struct scsi_device		*sdev=NULL;
	struct request_queue	*rq=NULL;
	struct region_info		*rp=NULL;
	
	uchar	ssn = 0;					/* slog page No 	*/
	uchar	son = 0;					/* slog offset No	*/
	ushort	sbc = 0;					/* slog length		*/
	ushort	sbc_wk = 0;	
	uint	slog_in_page=0;
	uint	slog_adr;
	uint	errlog_size=0;
	uint	trc_num;
	int 	errlog_info_p=0;
	uint	i;
	ushort	slog_len;
	uint	slog_num;
	uint	wk_slog_num;
	char	wkbuf[VPD_PN_LEN+13];  /* +13=NULL */
	char	ras_error_id1[9], ras_error_id2[9];
	char	resource_name[16] ;
	uint	raslog=2;
	uint	slog_area_size = 0 ;
	uint	wk_err_num ;
	uint	wk_tmo_sec = 0;
	char	sfp_type_name[HFC_SFP_TYPE_NAME_LEN+1];
	char	sfp_serial_no[HFC_SFP_SERIAL_NO_LEN+1];
	char	sfp_date_code[HFC_SFP_DATE_CODE_LEN+1];
	uchar	skip_errlog = 0;
	
	if( pp != NULL ) {
		HFC_DBGPRT(" hfcldd%d : hfc_fx_errlog - start\n",pp->dev_minor);
		rp = pp->region_arg[pp->rid];
		if (rp == NULL) {
			/* region_info null*/
			skip_errlog = 1;
		}
		else {
			if (core == NULL) {
				core = rp->core_arg[pp->master_core_no];
				if (core == NULL) {
					/* core_info null*/
					skip_errlog = 1;
				}
			}
		}
	}
	else {
		/* port_info null*/
		skip_errlog = 1;
	}

	errlog_info_p = 0;
	while( errlog_info[errlog_info_p].type != 0xFFFFFFFF ){
		if( errlog_info[errlog_info_p].type == err_id ){
			break;
		}
		errlog_info_p++;
	}

	memset(ras_error_id1,0,sizeof(ras_error_id1));
	sprintf(&ras_error_id1[strlen(ras_error_id1)], "KALB%c%c%02X",
		hraslog_info[errlog_info_p].alart1[0], hraslog_info[errlog_info_p].alart1[1], err_num);

	memset(ras_error_id2,0,sizeof(ras_error_id2));
	sprintf(&ras_error_id2[strlen(ras_error_id2)], "KALB%c%c%02X",
		hraslog_info[errlog_info_p].alart2[0], hraslog_info[errlog_info_p].alart2[1], err_num);

	memset(resource_name,0,sizeof(resource_name));
	if( pp != NULL){
		sprintf(&resource_name[strlen(resource_name)], "hfcldd%d", pp->pport->dev_minor);
	}
	else if( pp == NULL){
		sprintf(&resource_name[strlen(resource_name)], "hfcldd");
	}
	
	if(	( skip_errlog )              ||
		 (pp->pci_cfginf == NULL )   ||
		 (pp->mem_base_addr == 0) 		){
		
		HFC_ERRPRT_S(err_id, hfc_manage_info.log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd : %s (ErrNo:0x%02x) \n",errlog_info[errlog_info_p].errmsg, err_num); 
		if( ( data != NULL ) && ( data_len != 0 ) ){
			if( data_len > 16 )
				data_len = 16;
			HFC_ERRPRT_S(err_id, hfc_manage_info.log_file, HFC_LOG_DUMP, /* FCLNX-GPL-547 */
				"      [%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x]\n",
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
				data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
		}
		return;
	}
	
	if( pp->raslog_install ){
		if (HFC_FX_VIRTUAL_PORT(pp)) {
			if (hfc_fx_skip_vport_errlog(pp, err_num)) {
				/* skip errlog */
				return;
			}
		}
	}
	
	err_rec = &pp->err_rec;
	err1 = (hfc_fx_errfmt1_t *)err_rec->log_area ;
	err3 = (hfc_fx_errfmt3_t *)err_rec->log_area ;	/* FCLNX-GPL-FX-139 */
	err4 = (hfc_fx_errfmt4_t *)err_rec->log_area ;	/* FCLNX-GPL-FX-139 */
	HFC_BZERO( (char*)err_rec, sizeof( struct hfc_err_rec ) );
	errlog_size = sizeof( struct hfc_err_rec );
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() && (hfcp != NULL) ) {
		if (hfc_manage_info.npubp->hfc_fx_errlog_mp(hfcp)) return;
	}
	
	pp->raslog_cnt = 0;
	if( pp->raslog_install ){
		HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd%d: Firmware version %06x, Driver version %s, device %02x:%02x.%02x IRQ %d\n",
			pp->pport->dev_minor,
			hfc_fx_get_sysrev(core),
			hfc_manage_info.package_ver,
			pp->pci_cfginf->bus->number,
			PCI_SLOT(pp->pci_cfginf->devfn),
			PCI_FUNC(pp->pci_cfginf->devfn),
			pp->pci_cfginf->irq);
		HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd%d: Adapter wwpn : %llx \n", pp->pport->dev_minor,(unsigned long long)pp->ww_name);
	}
	
	memset(wkbuf,0,sizeof(wkbuf));
	memcpy(wkbuf, ((struct hfc_vpd_five_fx *)(pp->vpd_buf))->pn_value, VPD_PN_LEN);
	if( pp->raslog_install )
		HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd%d: Parts number : %11s\n", pp->pport->dev_minor, wkbuf);
	
	/* Output SFP Information */
	if( pp->raslog_install ){
		if(!(core->fw_init_p->sfp_info.sfp_status & HFC_FX_SFP_INSTALL)){
			HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
				"hfcldd%d: SFP Information : N/A\n", pp->pport->dev_minor);
		}else {
			memset(sfp_type_name,0,sizeof(sfp_type_name));
			memcpy(sfp_type_name, core->fw_init_p->sfp_info.sfp_type_name, sizeof(core->fw_init_p->sfp_info.sfp_type_name)); 
			hfc_fx_delete_space(sfp_type_name);
			
			memset(sfp_serial_no,0,sizeof(sfp_serial_no));
			memcpy(sfp_serial_no, core->fw_init_p->sfp_info.sfp_serial_no, sizeof(core->fw_init_p->sfp_info.sfp_serial_no)); 
			hfc_fx_delete_space(sfp_serial_no);

			memset(sfp_date_code,0,sizeof(sfp_date_code));
			memcpy(sfp_date_code, core->fw_init_p->sfp_info.sfp_date_code, sizeof(core->fw_init_p->sfp_info.sfp_date_code)); 
			hfc_fx_delete_space(sfp_date_code);

			if(!(core->fw_init_p->sfp_info.sfp_status & HFC_FX_SFP_VALID)){
				HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
					"hfcldd%d: SFP Information : incorrect data(%s, %s, %s)\n",
					pp->pport->dev_minor,
					sfp_type_name,
					sfp_serial_no,
					sfp_date_code);
			}else{
				HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
					"hfcldd%d: SFP Information : %s, %s, %s\n",
					pp->pport->dev_minor,
					sfp_type_name,
					sfp_serial_no,
					sfp_date_code);
			}
		}
	}

	if((type != HFC_ERRLOG_TYPE_LINKUPTOUT)&&(type != HFC_ERRLOG_TYPE_LINKINCLOG)){	/* FCLNX-GPL-FX-139 */
/*----------------------------------*/
/*--- 1st errlog (log format 1)  ---*/
/*----------------------------------*/
/* error detail infomation 1 */
		/* error number */
		HFC_4L_TO_4B(err1->err_detail_1.err_num, err_num);
		
		/* current sysrev */
		if( core->fw_init_p != NULL ) {
			err1->err_detail_1.curr_sysrev = core->fw_init_p->fls_hdr.sys_rev;
		}
		
		/* original wwpn */
		HFC_8L_TO_8B(err1->err_detail_1.org_ww_name, pp->org_ww_name);
		
		/* error dependent data */
		if( ( data != NULL ) && ( data_len != 0 ) ){
			if( data_len > 16 )
				data_len = 16;
			memcpy(&(err1->err_detail_1.drv_log[0]), data, data_len) ;
		}
		
		if( ( ( type == HFC_ERRLOG_TYPE_MCK ) ||
			 (type == HFC_ERRLOG_TYPE_CHKSTP) ||
			 (type == HFC_ERRLOG_TYPE_IMLLOG) ||
			 ((type == HFC_ERRLOG_TYPE_MBINT) && (err_num == 0x14)) ||
			 ((type == HFC_ERRLOG_TYPE_NONE) && (err_num == 0x2e)) ||
			 (type == HFC_ERRLOG_TYPE_LINKINCLOG))) {	/* FCLNX-GPL-FX-391,406 */
			/* Shadow LPAR driver saves driver log for Guest LPAR driver.*/
			if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
				HFC_MEMCPY(pp->mlpf_drv_log, err_rec, 64 );
			}
		}
		
/* Common error infomation */
/* --- FW_INIT_TABLE,XOB,XRB offset +20--7f -----*/
		hfc_fx_error_common(pp, core, err1, hfcp) ;
		
/* --- Common Driver Infomation ---------*/
/* port infomation  offset +80--af */
		HFC_4L_TO_4B(err1->err_pp.status, pp->status);
		HFC_4L_TO_4B(err1->err_pp.status_detail1, pp->status_detail1);
		HFC_4L_TO_4B(err1->err_pp.status_detail2, pp->status_detail2);
		HFC_4L_TO_4B(err1->err_pp.issue_mailbox, pp->issue_mailbox);
		err1->err_pp.rid				= pp->rid ;
		err1->err_pp.vport_id			= (uchar)pp->vport_id ;
		err1->err_pp.npiv_mode			= (uchar)pp->npiv_mode ;
		if( HFC_FX_MMODE_CHECK_SHARED(pp)  && !(HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){
			err1->err_pp.rid_hg = (uchar)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_RID, 0x4);
		}
		err1->err_pp.mlpf_mode			= (uchar)pp->mlpf_mode ;
		err1->err_pp.dev_major			= (uchar)pp->dev_major ;
		err1->err_pp.dev_minor			= (uchar)pp->dev_minor ;
		err1->err_pp.host_no			= (uchar)pp->host_no ;
		err1->err_pp.instance			= (uchar)pp->instance ;
		err1->err_pp.master_core_no		= (uchar)pp->master_core_no ;
		err1->err_pp.available_pcore	= (uchar)pp->available_pcore ;
		err1->err_pp.core_num			= (uchar)pp->core_num ;
		err1->err_pp.target_cnt			= (uchar)pp->target_cnt ;
		err1->err_pp.open_status		= (uchar)pp->open_status ;
		err1->err_pp.frame_ctl			= (uchar)pp->frame_ctl ;
		err1->err_pp.fc_class			= (uchar)pp->fc_class ;
		err1->err_pp.flogi_param		= (uchar)pp->flogi_param ;
		err1->err_pp.plogi_param		= (uchar)pp->plogi_param ;
		err1->err_pp.switch_exist		= (uchar)pp->switch_exist ;
		err1->err_pp.mailbox_pseq		= (uchar)pp->mailbox_pseq ;
		HFC_2L_TO_2B(err1->err_pp.seq_no, pp->seq_no);
//		HFC_2L_TO_2B(err1->err_pp.pkt_no, pp->pkt_no);
		HFC_2L_TO_2B(err1->err_pp.trc_num, pp->trc_num);
		
/* core infomation  offset offset +b0--df */
		HFC_4L_TO_4B(err1->err_core.status, core->status);
		err1->err_core.core_no			= core->core_no;
		err1->err_core.pcore_no			= core->pcore_no;
		if( rp != NULL ){
			err1->err_core.rid				= rp->rid;
		}
		err1->err_core.mb_status		= core->mb_status;
		HFC_4L_TO_4B(err1->err_core.mb_resp, core->mb_resp);
		HFC_4L_TO_4B(err1->err_core.mb_results, core->mb_results);
		err1->err_core.mb_retry_cnt		= core->mb_retry_cnt;
		err1->err_core.mb_retry_tid		= core->mb_retry_tid;
		err1->err_core.drv_next_xob		= (uchar)core->drv_next_xob;
		err1->err_core.drv_next_xrb		= (uchar)core->drv_next_xrb;
		HFC_2L_TO_2B(err1->err_core.wx_que_cnt_all, core->wx_que_cnt_all);
		HFC_2L_TO_2B(err1->err_core.we_que_cnt_all, core->we_que_cnt_all);
		HFC_4L_TO_4B(err1->err_core.we_que_sizecnt, core->we_que_sizecnt);
		err1->err_core.next_dstart_cnt	= (uchar)core->next_dstart_cnt;
		err1->err_core.frame_inp		= (uchar)core->frame_inp;
		err1->err_core.mck_err_cnt		= (uchar)core->mck_err_cnt;
		HFC_4L_TO_4B(err1->err_core.scsi_exec_cnt, core->scsi_exec_cnt);
		HFC_4L_TO_4B(err1->err_core.scsi_end_cnt, core->scsi_end_cnt);
		HFC_2L_TO_2B(err1->err_core.iov_no, core->iov_no);
		HFC_2L_TO_2B(err1->err_core.trc_num, core->trc_num);
		HFC_2L_TO_2B(err1->err_core.scsi_err_cnt, core->scsi_err_cnt);
		HFC_2L_TO_2B(err1->err_core.mb_retry_tout, core->mb_retry_tout);
	
/* target infomation  offset +e0--ff */
		if( (target == NULL) && (pp->target_arg[0] != NULL) )
			target = pp->target_arg[0] ;
		if( target != NULL )
		{
			HFC_4L_TO_4B(err1->err_target.status, target->status);
			err1->err_target.flags		= target->flags ;
			err1->err_target.target_id	= target->target_id ;
			err1->err_target.fc_class	= target->fc_class ;
			err1->err_target.pseq		= target->pseq ;
			err1->err_target.wx_que_cnt[0] = (uchar)target->core_queue[0].wx_que_cnt;
			err1->err_target.wx_que_cnt[1] = (uchar)target->core_queue[1].wx_que_cnt;
			err1->err_target.wx_que_cnt[2] = (uchar)target->core_queue[2].wx_que_cnt;
			err1->err_target.wx_que_cnt[3] = (uchar)target->core_queue[3].wx_que_cnt;
			HFC_8L_TO_8B(err1->err_target.scsi_id, target->scsi_id);
			HFC_8L_TO_8B(err1->err_target.ww_name, target->ww_name);
		}
		
/* device infomation  offset +100--11f */
		if(type != HFC_ERRLOG_TYPE_TGT_NOTFOUND){	/* FCLNX-GPL-FX-139 */
			if( hfcp != NULL ){
				dev = hfcp->dev;
			}
			else if (target != NULL) {
				if (target->dev != NULL) {
					dev = target->dev;
				}
			}
			
			if( dev != NULL ){
				err1->err_dev.flags			= (uchar)dev->flags;
				err1->err_dev.lustat		= (uchar)dev->lustat;
				HFC_2L_TO_2B(err1->err_dev.lun[0], dev->lun);	/* FCLNX-GPL-FX-255 */
				err1->err_dev.curr_core		= (uchar)dev->curr_core;
				err1->err_dev.curr_cmd_type	= (uchar)dev->curr_cmd_type;
				err1->err_dev.target_id		= (uchar)dev->target_id;
				
				/* for HFC-PCM */
				err1->err_dev.status		= (uchar)dev->status;
				err1->err_dev.owner_ctl		= (uchar)dev->owner_ctl;
				err1->err_dev.priority 		= (uchar)dev->priority;
				err1->err_dev.group_id		= (uchar)dev->group_id;
				err1->err_dev.path_id 		= (uchar)dev->path_id;
				HFC_4L_TO_4B(err1->err_dev.ioerror, dev->ioerror);
				HFC_4L_TO_4B(err1->err_dev.iocount, dev->iocount);
				HFC_4L_TO_4B(err1->err_dev.id_size, dev->id_size);
				HFC_4L_TO_4B(err1->err_dev.id_size, dev->io_status);
			}
		}else{	/* FCLNX-GPL-FX-139 Start */
			for(i=0;i<32;i++){
				err4->err_unfnd_tgtlist[i] = pp->unfnd_tgtlist[i];
			}
		}		/* FCLNX-GPL-FX-139 End */
	}else{	/* FCLNX-GPL-FX-139 */
/*----------------------------------*/
/*--- 1st errlog (log format 3)  ---*/
/*----------------------------------*/
/* error detail infomation 1 */
		/* error number */
		HFC_4L_TO_4B(err3->err_detail_1.err_num, err_num);
		
		/* current sysrev */
		if( core->fw_init_p != NULL ) {
			err3->err_detail_1.curr_sysrev = core->fw_init_p->fls_hdr.sys_rev;
		}
		
		/* original wwpn */
		HFC_8L_TO_8B(err3->err_detail_1.org_ww_name, pp->org_ww_name);
		
		/* error dependent data */
		if( ( data != NULL ) && ( data_len != 0 ) ){
			if( data_len > 16 )
				data_len = 16;
			memcpy(&(err3->err_detail_1.drv_log[0]), data, data_len) ;
		}
		
		if( ( ( type == HFC_ERRLOG_TYPE_MCK ) ||
			 (type == HFC_ERRLOG_TYPE_CHKSTP) ||
			 (type == HFC_ERRLOG_TYPE_IMLLOG) ||
			 ((type == HFC_ERRLOG_TYPE_MBINT) && (err_num == 0x14)) ||
			 ((type == HFC_ERRLOG_TYPE_NONE) && (err_num == 0x2e)) ||
			 (type == HFC_ERRLOG_TYPE_LINKINCLOG))) {	/* FCLNX-GPL-FX-391,406 */
			/* Shadow LPAR driver saves driver log for Guest LPAR driver.*/
			if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
				HFC_MEMCPY(pp->mlpf_drv_log, err_rec, 64 );
			}
		}
		
/* Common error infomation */
/* --- FW_INIT_TABLE,XOB,XRB offset +20--7f -----*/
		hfc_fx_error_common(pp, core, err1, hfcp) ;
	
/* --- Common Driver Infomation ---------*/
/* port infomation  offset +80--af */
		HFC_4L_TO_4B(err3->err_pp.status, pp->status);
		HFC_4L_TO_4B(err3->err_pp.status_detail1, pp->status_detail1);
		HFC_4L_TO_4B(err3->err_pp.status_detail2, pp->status_detail2);
		HFC_4L_TO_4B(err3->err_pp.issue_mailbox, pp->issue_mailbox);
		err3->err_pp.rid				= pp->rid ;
		err3->err_pp.vport_id			= (uchar)pp->vport_id ;
		err3->err_pp.npiv_mode			= (uchar)pp->npiv_mode ;
		if( HFC_FX_MMODE_CHECK_SHARED(pp)  && !(HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){
			err3->err_pp.rid_hg = (uchar)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_RID, 0x4);
		}
		err3->err_pp.mlpf_mode			= (uchar)pp->mlpf_mode ;
		err3->err_pp.dev_major			= (uchar)pp->dev_major ;
		err3->err_pp.dev_minor			= (uchar)pp->dev_minor ;
		err3->err_pp.host_no			= (uchar)pp->host_no ;
		err3->err_pp.instance			= (uchar)pp->instance ;
		err3->err_pp.master_core_no		= (uchar)pp->master_core_no ;
		err3->err_pp.available_pcore	= (uchar)pp->available_pcore ;
		err3->err_pp.core_num			= (uchar)pp->core_num ;
		err3->err_pp.target_cnt			= (uchar)pp->target_cnt ;
		err3->err_pp.open_status		= (uchar)pp->open_status ;
		err3->err_pp.frame_ctl			= (uchar)pp->frame_ctl ;
		err3->err_pp.fc_class			= (uchar)pp->fc_class ;
		err3->err_pp.flogi_param		= (uchar)pp->flogi_param ;
		err3->err_pp.plogi_param		= (uchar)pp->plogi_param ;
		err3->err_pp.switch_exist		= (uchar)pp->switch_exist ;
		err3->err_pp.mailbox_pseq		= (uchar)pp->mailbox_pseq ;
		HFC_2L_TO_2B(err3->err_pp.seq_no, pp->seq_no);
//		HFC_2L_TO_2B(err3->err_pp.pkt_no, pp->pkt_no);
		HFC_2L_TO_2B(err3->err_pp.trc_num, pp->trc_num);
	}	/* FCLNX-GPL-FX-139 */
	
/* error detail infomation 2  offset +120--17f */
	if( (type == HFC_ERRLOG_TYPE_MCK) || (type == HFC_ERRLOG_TYPE_CHKSTP) ) {
		/* TBD */
	}
    else if( type == HFC_ERRLOG_TYPE_MBINIT ){
    	/* mb_initiate_type */
    	if ((core->mb != NULL) && (core->payload != NULL) ) {
			memcpy(&(err1->err_detail_2.uni.mbinit.mb_initiate[0]), (uchar *)&(core->mb->mb_init.mb_code), 20) ;
			memcpy(&(err1->err_detail_2.uni.mbinit.mb_initiate[20]), (uchar *)&(core->mb->mb_init.mb_code)+0x24, 44) ;
			memcpy(&(err1->err_detail_2.uni.mbinit.send_payload[0]), (uchar *)&(core->payload->send_payload.data0[0]), 32) ;
		}
	}
	else if( type == HFC_ERRLOG_TYPE_MBRESP ){
		/* mb_response_type */
		if ((core->mb != NULL) && (core->payload != NULL) ) {
			memcpy(&(err1->err_detail_2.uni.mbresp.mb_initiate[0]), (uchar *)&(core->mb->mb_init.mb_code), 16) ;
			memcpy(&(err1->err_detail_2.uni.mbresp.mb_response[0]), (uchar *)&(core->mb->mb_resp.flag)+0x10, 48) ;
			memcpy(&(err1->err_detail_2.uni.mbresp.send_payload[0]), (uchar *)&(core->payload->send_payload.data0[0]), 16) ;
			memcpy(&(err1->err_detail_2.uni.mbresp.receive_payload[0]), (uchar *)&(core->payload->receive_payload.type.prli.data1[0]), 16) ;
		}
	}
	else if( type == HFC_ERRLOG_TYPE_MBINT ){
		/* mb_int_type */
		if ((core->mb != NULL) && (core->rcvfrm_payload != NULL) ) {
			memcpy(&(err1->err_detail_2.uni.mbint.mb_intreq[0]), (uchar *)&(core->mb->mb_intreq), 64) ;
			memcpy(&(err1->err_detail_2.uni.mbint.rcvfrm_payload[0]), (uchar *)&(core->rcvfrm_payload->data0[0]), 32) ;
		}
	}
	else if(( type == HFC_ERRLOG_TYPE_LINKUPTOUT )||( type == HFC_ERRLOG_TYPE_LINKINCLOG )){	/* FCLNX-GPL-FX-139 */
		trc_num = pp->current_mbtrc_no;
	}	/* FCLNX-GPL-FX-139 */
	else if(( hfcp != NULL )&&(type != HFC_ERRLOG_TYPE_LINKUPTOUT)&&(type != HFC_ERRLOG_TYPE_LINKINCLOG)){
		/* scsi type */
		if( hfcp->cmd_pkt != NULL ){
			memcpy(&(err1->err_detail_2.uni.scsi.cmnd[0]), hfcp->cmd_pkt->cmnd, 16) ;
			/* kernel 5.15+: scsi_cmnd->serial_number removed; use 0 for trace */
			err1->err_detail_2.uni.scsi.serial_number = 0;	/* 0 is endian-neutral */
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.retries, hfcp->cmd_pkt->retries);
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.allowed, hfcp->cmd_pkt->allowed);
			sdev = hfcp->cmd_pkt->device;
			if( sdev != NULL ){
				rq = sdev->request_queue;
				if( rq != NULL ){
					wk_tmo_sec = (rq->rq_timeout/HZ);
				}
			}
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.rq_tmo_sec, wk_tmo_sec);
			
			wk_tmo_sec = 0;
			/* kernel 5.16+: cmnd->request removed; use scsi_cmd_to_rq() */
			{
				struct request *_rq = scsi_cmd_to_rq(hfcp->cmd_pkt);
				if (_rq != NULL)
					wk_tmo_sec = (_rq->timeout/HZ);
			}
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.req_tmo_sec, wk_tmo_sec);
			
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.result, hfcp->cmd_pkt->result);
			err1->err_detail_2.uni.scsi.cmd_len = hfcp->cmd_pkt->cmd_len;
			/* kernel 5.4+: cmnd->tag removed; use scsi_cmd_to_rq()->tag */
			err1->err_detail_2.uni.scsi.tag =
				(uchar)scsi_cmd_to_rq(hfcp->cmd_pkt)->tag;
		}
		
		err1->err_detail_2.uni.scsi.core_no = hfcp->core_no;
		HFC_2L_TO_2B(err1->err_detail_2.uni.scsi.lun_id[0], hfcp->lun_id);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.cmd_flags, hfcp->cmd_flags);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.adap_status, hfcp->adap_status);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.seg_cnt, hfcp->seg_cnt);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.data_size, hfcp->data_size);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.iov_no, hfcp->iov_no);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.iov_cnt, hfcp->iov_cnt);
		err1->err_detail_2.uni.scsi.cmd_xob = hfcp->cmd_xob;
		err1->err_detail_2.uni.scsi.target_id = hfcp->target_id;
		err1->err_detail_2.uni.scsi.group_id = hfcp->group_id;
		err1->err_detail_2.uni.scsi.seg_info_count = core->xob[hfcp->cmd_xob].seg_cnt;
		err1->err_detail_2.uni.scsi.seg_info[0].xob_segno = core->xob[hfcp->cmd_xob].seg_info_xob[0].xob_segno;
		err1->err_detail_2.uni.scsi.seg_info[1].xob_segno = core->xob[hfcp->cmd_xob].seg_info_xob[1].xob_segno;
		err1->err_detail_2.uni.scsi.seg_info[2].xob_segno = core->xob[hfcp->cmd_xob].seg_info_xob[2].xob_segno;
		err1->err_detail_2.uni.scsi.seg_info[3].xob_segno = core->xob[hfcp->cmd_xob].seg_info_xob[3].xob_segno;
	}
	
	if((type != HFC_ERRLOG_TYPE_LINKUPTOUT)&&(type != HFC_ERRLOG_TYPE_LINKINCLOG)){	/* FCLNX-GPL-FX-139 */
/* driver core trace offset +180--27f */
		trc_num = core->trc_num ;
		if( trc_num == 0 )
			trc_num = HFC_MAX_TRCCNT - 1 ;
		else
			trc_num-- ;
		for(i=0 ; i<7 ; i++)
		{
			memcpy(err1->err_coretrc1[i], (char*)&(core->trc_ptr[trc_num].trc_id),32) ;
			if( trc_num == 0 )
				trc_num = HFC_MAX_TRCCNT - 1 ;
			else
				trc_num-- ;
		}
		for(i=0 ; i<16 ; i++)
		{
			err1->err_coretrc2[i][0] = core->trc_ptr[trc_num].trc_id ;
			err1->err_coretrc2[i][1] = core->trc_ptr[trc_num].trc_data[0] ;
			if( trc_num == 0 )
				trc_num = HFC_MAX_TRCCNT - 1 ;
			else
				trc_num-- ;
		}
	
/* driver port trace  offset +280--3ff */
		trc_num = pp->trc_num ;
		if( trc_num == 0 )
			trc_num = HFC_MAX_TRCCNT - 1 ;
		else
			trc_num-- ;
		for(i=0 ; i<11 ; i++)
		{
			memcpy(err1->err_ddtrc1[i], (char*)&(pp->trc_ptr[trc_num].trc_id),32) ;
			if( trc_num == 0 )
				trc_num = HFC_MAX_TRCCNT - 1 ;
			else
				trc_num-- ;
		}
		for(i=0 ; i<16 ; i++)
		{
			err1->err_ddtrc2[i][0] = pp->trc_ptr[trc_num].trc_id ;
			err1->err_ddtrc2[i][1] = pp->trc_ptr[trc_num].trc_data[0] ;
			if( trc_num == 0 )
				trc_num = HFC_MAX_TRCCNT - 1 ;
			else
				trc_num-- ;
		}
	}else{	/* FCLNX-GPL-FX-139 */
		trc_num = pp->current_mbtrc_no ;
		if( trc_num == 0 )
			trc_num = HFC_FX_MAX_MB_TRACE - 1 ;
		else
			trc_num-- ;
		for(i=0 ; i<53 ; i++)
		{
			memcpy(err3->err_mb_trc[i], (char*)&(pp->mb_trace[trc_num].flag),16) ;
			if( trc_num == 0 )
				trc_num = HFC_FX_MAX_MB_TRACE - 1 ;
			else
				trc_num-- ;
		}
	}	/* FCLNX-GPL-FX-139 */
	
/* logout format1 */
	if( !pp->raslog_install ){
		raslog = hfc_fx_raslog( pp, core, err_rec, (char *)&ras_error_id1,
					 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
	}
	if( ( pp->raslog_install ) || (raslog != 0) ) {
		hfc_fx_errsave( pp, errlog_info_p, err_num, err_rec, errlog_size, 0 );
	}
	
/*---------------------------------------*/
/*--- 2nd errlog (log format 2 or 3)  ---*/
/*---------------------------------------*/
	err2 = (hfc_fx_errfmt2_t *)err_rec->log_area ;
	HFC_BZERO( (char*)err_rec, sizeof( struct hfc_err_rec ) );
	errlog_info_p = 17;
	
/* HW_LOG ... MCK,IML,CHK-STP.... log format 3 */
	if(  ((type == HFC_ERRLOG_TYPE_MCK) ||
		 (type == HFC_ERRLOG_TYPE_CHKSTP) ||
		 ((type == HFC_ERRLOG_TYPE_NONE) && (err_num == 0x2e)) ||
		 (type == HFC_ERRLOG_TYPE_IMLLOG) ) && (pp->hw_log != NULL) )
	{	/* FCLNX-GPL-FX-406 */
		if(( type == HFC_ERRLOG_TYPE_MCK ) 
		|| ((type == HFC_ERRLOG_TYPE_NONE) && (err_num == 0x2e))){	/* FCLNX-GPL-FX-406 */
			wk_err_num = 0xfffe0000 ;
		}
		if( type == HFC_ERRLOG_TYPE_CHKSTP ) {
			wk_err_num = 0xfffe0100 ;
		}
		
		if(type != HFC_ERRLOG_TYPE_NONE){	/* FCLNX-GPL-FX-406 */
			for(i=0 ; i<pp->max_hwlog_cnt ; i++) {
				memcpy(err_rec->log_area,(uchar *)&pp->hw_log[i*256],1024);
				if( !pp->raslog_install ){
					raslog = hfc_fx_raslog( pp, core, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
				}
				if( ( pp->raslog_install ) || (raslog != 0) ) {
					hfc_fx_errsave( pp, errlog_info_p, err_num, err_rec, errlog_size, 0 );
				}
			}
		}	/* FCLNX-GPL-FX-406 */
		
		if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
			hfc_fx_mlpf_errlog_slpar(pp, core, ssn, son, type);	/* FCLNX-GPL-FX-391 */
		}
		return ;
	}
	if ( type == HFC_ERRLOG_TYPE_SRAMCE ) {
		for(i=0;i<3;i++){
			memcpy(err_rec->log_area,(uchar *)&pp->ce_log[i],1024);
			if( !pp->raslog_install ){
				raslog = hfc_fx_raslog( pp, core, err_rec, (char *)&ras_error_id2,
						 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( pp->raslog_install ) || (raslog != 0) ) {
				hfc_fx_errsave( pp, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
		}
		memcpy(err_rec->log_area,(uchar *)&pp->ce_fw_log,1024);
		if( !pp->raslog_install ){
			raslog = hfc_fx_raslog( pp, core, err_rec, (char *)&ras_error_id2,
					 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
		}
		if( ( pp->raslog_install ) || (raslog != 0) ) {
			hfc_fx_errsave( pp, errlog_info_p, err_num, err_rec, errlog_size, 0 );
		}
	}
	
/* Soft Log ... ICC,etc... log format 2 */
	wk_err_num = 0;
	if( type == HFC_ERRLOG_TYPE_XRB )
	{
		if( core->icc_err->logdata[5] & HFC_ESW_MEINT_REPO )
		{
			ssn = core->icc_err->logdata[6] / 4;						/* Soft_Log_Area Start Number	*/
			son = core->icc_err->logdata[6] % 4;						/* Soft_Log_Area offset Number	*/
			HFC_2B_TO_2L(sbc, core->icc_err->logdata[8]);				/* Soft_Log Byte Count			*/
			wk_err_num = 0xfffffffe ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_MBRESP )
	{
		if( core->mb->mb_resp.esw & HFC_ESW_MEINT_REPO )
		{
			ssn = core->mb->mb_resp.ssn / 4;
			son = core->mb->mb_resp.ssn % 4;
			HFC_2B_TO_2L(sbc, core->mb->mb_resp.sbc);
			wk_err_num = 0xfffffffe ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_MBINT )
	{
		if( core->mb->mb_intreq.esw & HFC_ESW_MEINT_REPO )
		{
			ssn = core->mb->mb_intreq.softlog / 4;
			son = core->mb->mb_intreq.softlog % 4;
			HFC_2B_TO_2L(sbc, core->mb->mb_intreq.sbc);
			wk_err_num = 0xfffffffd ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_TOUTLOG )
	{
		if( hfcp != NULL )
		{
			ssn = hfcp->tout_slog_ssn / 4;
			son = hfcp->tout_slog_ssn % 4;
			sbc = hfcp->tout_slog_sbc ;
			wk_err_num = 0xfffffffe ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_LINKINCLOG )
	{
		ssn = core->mb->mb_resp.type.mih_log.mih_slog / 4;
		son = core->mb->mb_resp.type.mih_log.mih_slog % 4;
		HFC_2B_TO_2L(sbc, core->mb->mb_resp.type.mih_log.mih_sbc);
		wk_err_num = 0xfffffffe ;
	}
	if( sbc != 0 )
	{
		/* Slog_Entry length = zero ? */
		slog_len = HFC_SLOG_LEN;
		wk_slog_num = core->fw_init_p->slog_num;
		HFC_4B_TO_4L(slog_num, wk_slog_num);
		slog_num = slog_num / 4;
		if( slog_len == 0 )
			return ;
		slog_in_page = HFC_PAGE_SIZE / (uint)slog_len ;
		if( (son >= slog_in_page) || (ssn >= slog_num) ) 
			return ;
		slog_area_size = slog_num * HFC_PAGE_SIZE ;
		if( sbc > slog_area_size )
			return ;

		slog_adr = (HFC_PAGE_SIZE * ssn)+(slog_len * son) ;
		if( slog_adr >= slog_area_size ) 
			return ;
		sbc_wk = sbc ;
		if( ( pp->pkg.type == HFC_PKTYPE_FPP ) || ( pp->pkg.type == HFC_PKTYPE_FIVE ) ) {
			if( sbc_wk > 960 )
				sbc_wk = 960 ;
			if( wk_err_num )
				HFC_4L_TO_4B(err2->err_num, wk_err_num);
			memcpy(&err2->err_detail[60],(uchar *)(core->slog_addr[ssn]+(slog_len * son)),sbc_wk) ;
		}
		else {
			if( sbc_wk > 1024 )
				sbc_wk = 1024 ;
			memcpy((uchar *)&err2->err_num, (uchar *)(core->slog_addr[ssn]+(slog_len * son)), sbc_wk) ;
		}
		if( !pp->raslog_install ){
			raslog = hfc_fx_raslog( pp, core, err_rec, (char *)&ras_error_id2,
						 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
		}
		if( ( pp->raslog_install ) || (raslog != 0) ) {
			hfc_fx_errsave( pp, errlog_info_p, err_num, err_rec, errlog_size, 0 );
		}
		if(( type == HFC_ERRLOG_TYPE_MBINT )||(type == HFC_ERRLOG_TYPE_LINKINCLOG)){	/* FCLNX-GPL-FX-391 Start */
			if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
				hfc_fx_mlpf_errlog_slpar(pp, core, ssn, son, type);
			}
		}	/* FCLNX-GPL-FX-391 End */

/*--- format 4 ------*/
		if( sbc > 1024 )
		{
			sbc -= 1024 ;
			while( sbc != 0 )
			{
				son++ ;
				if( son == slog_in_page )
				{
					son = 0 ;
					ssn++ ;
					if( ssn == slog_num )
						ssn = 0 ;
				}
				slog_adr = (HFC_PAGE_SIZE * ssn)+(slog_len * son) ;
				if( slog_adr >= slog_area_size ) 
					return ;
				sbc_wk = sbc ;
				if( sbc_wk > 1024 )
				{
					sbc_wk = 1024 ;
					sbc -= 1024 ;
				}
				else sbc = 0 ;
				memcpy((uchar *)&err2->err_num, (uchar *)(core->slog_addr[ssn]+(slog_len * son)), sbc_wk) ;	/* FCLNX_GPL-0121 */
				if( !pp->raslog_install ){
					raslog = hfc_fx_raslog( pp, core, err_rec, (char *)&ras_error_id2,
								 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
				}
				if( ( pp->raslog_install ) || (raslog != 0) ) {
					hfc_fx_errsave( pp, errlog_info_p, err_num, err_rec, errlog_size, 0 );
				}
			}
		}
	}
	
	return ;
}

void hfc_fx_error_common(struct port_info *pp,
				struct core_info		*core,
				hfc_fx_errfmt1_t 		*err1,
				struct hfc_pkt_fx 		*hfcp)
{
	uint	num_work = 0 ;

	if( pp == NULL ) return ;
	if( err1 == NULL ) return ;
	if( core == NULL ) return ;
	if( core->fw_init_p == NULL ) return ;
	if( core->xob == NULL ) return ;
	if( core->xrb == NULL ) return ;
	
/*-- FW_INIT_TABLE  offset +20--2f --*/
	err1->err_init_tbl.connect_type		= core->fw_init_p->fw_iocinfo.connect_type ;
	err1->err_init_tbl.trans_rate		= core->fw_init_p->fw_iocinfo.trans_rate ;
	err1->err_init_tbl.configure_flag	= core->fw_init_p->fw_iocinfo.configure_flag ;
	err1->err_init_tbl.fabric_param		= core->fw_init_p->fw_iocinfo.fabric_param ;
	err1->err_init_tbl.alpa_count		= core->fw_init_p->fw_iocinfo.alpa_count ;
	memcpy(&(err1->err_init_tbl.p2p_tgt_port_id[0]), 
			(char*)&(core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0]), 3) ;
	err1->err_init_tbl.assign_alpa		= core->fw_init_p->fw_iocinfo.assign_alpa ;
	err1->err_init_tbl.vf_id			= core->fw_init_p->fw_iocinfo.vf_id ;
	memcpy(&(err1->err_init_tbl.self_port_id[0]), 
			(char*)&(core->fw_init_p->fw_iocinfo.self_port_id[0]), 3) ;
	
/*-- Xob  offset +30--5f --*/
	num_work = core->drv_next_xob ;
	if( num_work == 0 ) {
		num_work = pp->xob_max -1 ;
	}
	else {
		num_work-- ;
	}
	if( hfcp != NULL ) {
		num_work = hfcp->cmd_xob ;
	}
	
	err1->err_xob.flag		= core->xob[num_work].flag ;
	err1->err_xob.skip		= core->xob[num_work].skip ;
	err1->err_xob.seg_cnt	= core->xob[num_work].seg_cnt ;
	err1->err_xob.queue_no	= core->xob[num_work].queue_no ;
	memcpy(&(err1->err_xob.trans_s_id[0]),
			(uchar *)&(core->xob[num_work].trans_s_id[0]),3);
	err1->err_xob.drv_work	= core->xob[num_work].drv_work ;
	memcpy(&(err1->err_xob.fcp_cmd[0]),
			(uchar *)&(core->xob[num_work].fcp_cmd),32);
	
/*-- Xrb  offset +60--7f --*/
	num_work = core->drv_next_xrb ;
	if( num_work == 0 ) {
		num_work = pp->xrb_max -1 ;
	}
	else {
		num_work-- ;
	}
	
	err1->err_xrb.retry_delay	= core->xrb[num_work].resp_iu1.retry_delay ;
	err1->err_xrb.fcp_status2	= core->xrb[num_work].resp_iu1.fcp_status2 ;
	err1->err_xrb.scsi_status	= core->xrb[num_work].resp_iu1.scsi_status ;
	err1->err_xrb.resid			= core->xrb[num_work].resp_iu1.resid ;
	memcpy(&(err1->err_xrb.fcp_info[0]), 
			(uchar *)&core->xrb[num_work].fcp_info[0], 4) ;
	err1->err_xrb.esw			= core->xrb[num_work].xcrb.esw ;
	err1->err_xrb.softlog		= core->xrb[num_work].xcrb.softlog ;
	err1->err_xrb.sbc			= core->xrb[num_work].xcrb.sbc ;
	err1->err_xrb.fsb			= core->xrb[num_work].xcrb.fsb ;
	memcpy(&(err1->err_xrb.err_code[0]), 
			(uchar *)&core->xrb[num_work].xcrb.err_code[0], 4) ;
	err1->err_xrb.valid			= core->xrb[num_work].xcrbchk.valid ;
	memcpy(&(err1->err_xrb.trans_s_id[0]), 
			(uchar *)&core->xrb[num_work].xcrb.trans_s_id[0], 3) ;
	err1->err_xrb.drv_work		= core->xrb[num_work].xcrb.hfc_pkt ;
	
	return ;
}

/*
 * Function:    hfc_fx_errsave
 *
 * Purpose:     Output error log data 
 *
 * Arguments:   
 *  pp            - Pointer to port_info 
 *  errlog_info_p - Error name table number
 *  err_num       - Number of pages
 *  err_rec       - Pointer to hfc_err_rec 
 *  cnt           - Output data length
 *  offset        - Address offset value
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_errsave( struct port_info *pp,
				  int errlog_info_p,	
				  uint err_num,		
				  struct hfc_err_rec *err_rec,
				  uint cnt,
				  uint offset)
{
	uint *err_info;
	int lp;
	int lp2, lp3;
	int int_cnt;
	uint amari;
//	char log_wk[470] ;							/* FCLNX_GPL-0151 */
	char *name;		
	char c_instance[9];	
	uint wlog0, wlog1, wlog2, wlog3;
	uint log0, log1, log2, log3;
	uint err_id = errlog_info[errlog_info_p].type; /* FCLNX-GPL-547 */
	enum log_prefixs_fx log_prefix_fx = HFC_LOG_INF; /* FCLNX-GPL-547 */


	HFC_DBGPRT(" hfcldd : hfc_fx_errsave - start\n");
	
	/* FCLNX-GPL-547 start */
	/* Table size check */
	if( err_id < ERRID_HFCP_TBL_END )
	{	/* Table size is correct. */
		log_prefix_fx = log_prefix_tbl_fx[err_id];
	}
	/* FCLNX-GPL-547 end */
	
	if( pp != NULL ){
		if( cnt > sizeof( struct hfc_err_rec ) ){	
			cnt = sizeof( struct hfc_err_rec );
		}

		/* Convert output data bytes in word-unit */
		int_cnt = cnt / 16;
		amari   = cnt % 16;
		if( amari > 12 ){
			int_cnt++;
			amari = 0;
		}

//		err_info = (uint *)&err_rec->head.err_num;
		err_info = (uint *)&err_rec->log_area;
		
		name = "hfcldd";	
		
		c_instance[0] = 0 ;
		sprintf( c_instance, "%x", pp->pport->dev_minor );	
		HFC_BZERO( pp->log_wk, sizeof( pp->log_wk ) );	/* FCLNX-GPL-147 */

		HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_BODY, "%s%d: %s (ErrNo:0x%02x)\n", /* FCLNX-GPL-547 */
			 name, pp->pport->dev_minor, errlog_info[errlog_info_p].errmsg, err_num );	

		lp2 = 0 ;
		lp3 = 0;

		for( lp = 0; lp < ( int_cnt * 4 ); ){
#if defined(__x86_64)  /* FCLNX-0629 */
			touch_nmi_watchdog();		/* FCLNX-0398 */
#endif	
			wlog0 = (uint)err_info[lp];
			wlog1 = (uint)err_info[lp+1];
			wlog2 = (uint)err_info[lp+2];
			wlog3 = (uint)err_info[lp+3];
			HFC_4L_TO_4B(log0, wlog0);
			HFC_4L_TO_4B(log1, wlog1);
			HFC_4L_TO_4B(log2, wlog2);
			HFC_4L_TO_4B(log3, wlog3);
			sprintf( (char *)&pp->log_wk[lp2],
				 "0x%04x:[ %08x %08x %08x %08x ]\n",
				 ( lp * 4 +  offset ),
				 log0, log1, log2, log3);

			lp += 4;
			lp2 = strlen( pp->log_wk );
			lp3++;
			if(lp3 == 10 )  /* Ten line output: 470 bytes */
			{
				/* FCLNX-GPL-547 start */
				switch( log_prefix_fx ){
					case HFC_LOG_ERR:
						HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "%s", pp->log_wk );
						break;
					case HFC_LOG_WRN:
						HFC_WRNPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "%s", pp->log_wk );
						break;
					case HFC_LOG_INF:
					default :
						HFC_INFPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "%s", pp->log_wk );
						break;
				}
				/* FCLNX-GPL-547 end */
				lp2 = 0;
				lp3 = 0;
				HFC_BZERO( pp->log_wk, sizeof( pp->log_wk ) );	
			}
		}	

		switch( amari ){
			case 1 :
			case 2 :
			case 3 :
			case 4 :
				wlog0 = (uint)err_info[lp];
				HFC_4L_TO_4B(log0, wlog0);
				sprintf( (char *)&pp->log_wk[lp2],	
					 "0x%04x:[ %08x ] \n",
					 ( lp * 4 + offset ), wlog0 );
				lp++;
				lp2 = strlen( pp->log_wk );
				break;
			case 5 :
			case 6 :
			case 7 :
			case 8 :
				wlog0 = (uint)err_info[lp];
				wlog1 = (uint)err_info[lp+1];
				HFC_4L_TO_4B(log0, wlog0);
				HFC_4L_TO_4B(log1, wlog1);
				sprintf( (char *)&pp->log_wk[lp2],	
					 "0x%04x:[ %08x %08x ] \n",
					 ( lp * 4 + offset ),
					 wlog0, wlog1);
				lp += 2;
				lp2 = strlen( pp->log_wk );
				break;
			case 9 :
			case 10 :
			case 11 :
			case 12 :
				wlog0 = (uint)err_info[lp];
				wlog1 = (uint)err_info[lp+1];
				wlog2 = (uint)err_info[lp+2];
				HFC_4L_TO_4B(log0, wlog0);
				HFC_4L_TO_4B(log1, wlog1);
				HFC_4L_TO_4B(log2, wlog2);
				sprintf( (char *)&pp->log_wk[lp2],	
					 "0x%04x:[ %08x %08x %08x ] \n",
					 ( lp * 4 + offset ),
					 wlog0, wlog1, wlog2);
				lp += 3;
				lp2 = strlen( pp->log_wk );	
				break;
			default :
				break;
		}
		/* FCLNX-GPL-547 start */
		switch( log_prefix_fx ){
			case HFC_LOG_ERR:
				HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "%s", pp->log_wk);
				HFC_ERRPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "\n");
				break;
			case HFC_LOG_WRN:
				HFC_WRNPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "%s", pp->log_wk);
				HFC_WRNPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "\n");
				break;
			case HFC_LOG_INF:
			default :
				HFC_INFPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "%s", pp->log_wk);
				HFC_INFPRT_S(err_id, pp->log_file, HFC_LOG_DUMP, "\n");
				break;
		}
		/* FCLNX-GPL-547 end */
	}
	return;
}


/*
 * Function:    hfc_fx_raslog
 *
 * Purpose:     Output error log data to raslog files
 *
 * Arguments:   
 *  pp            - Pointer to port_info 
 *  err_rec       - Pointer to hfc_err_rec 
 *  ras_error_id  - Pointer to hraslog_errid
 *  uchar         - Pointer to resource_name
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_fx_raslog( struct port_info *pp,
				  struct core_info *core,
				  struct hfc_err_rec *err_rec,
				  char 	*ras_error_id1,
				  char 	*ras_error_id2,
				  char	 *resource_name,
				  struct hfc_pkt_fx		*hfcp,
				  struct scsi_cmnd		*cmnd,
				  uchar	 type)
{
	uint raslog=2;
#ifndef _HFC_NO_RASLOG
	char	sfp_type_name[HFC_SFP_TYPE_NAME_LEN+1];
	char	sfp_serial_no[HFC_SFP_SERIAL_NO_LEN+1];
	char	sfp_date_code[HFC_SFP_DATE_CODE_LEN+1];
	uint raslog_retry;
	struct hfc_vpd_five_fx		*vpdex_info=NULL;
#endif
	HFC_DBGPRT(" hfcldd : hfc_fx_raslog - start\n");

	memset(pp->detail_data, 0, sizeof(pp->detail_data));
	if( pp == NULL ){
#ifndef _HFC_NO_RASLOG
		memcpy( pp->detail_data, &err_rec->log_area[0], 64);
		raslog = _hraslog((char *)ras_error_id1, (char *)resource_name, 64, (char *)pp->detail_data );
#endif
		return(raslog);
	}

#ifndef _HFC_NO_RASLOG
	memcpy( pp->detail_data, &err_rec->log_area[0], 512);
	sprintf( &pp->detail_data[HFC_MFC], "%s", "Hitachi");
	sprintf( &pp->detail_data[HFC_MODELNAME], "%s", pp->model_name);
	
	vpdex_info = (struct hfc_vpd_five_fx *)pp->vpd_buf;
	sprintf( &pp->detail_data[HFC_FWVERSION], "%06x", hfc_fx_get_sysrev(core));
	sprintf( &pp->detail_data[HFC_DRVVERSION], "%s", vpdex_info->driver_ver);
	sprintf( &pp->detail_data[HFC_PARTSNUM], "%s", vpdex_info->pn_value);
	sprintf( &pp->detail_data[HFC_EC], "%c", vpdex_info->ec_level);

	sprintf( &pp->detail_data[HFC_LOCATION], "%02x:%02x.%02x", 
			pp->pci_cfginf->bus->number,PCI_SLOT(pp->pci_cfginf->devfn),PCI_FUNC(pp->pci_cfginf->devfn));
	sprintf( &pp->detail_data[HFC_LOGSEQNUM], "%d", pp->raslog_cnt);
	sprintf( &pp->detail_data[HFC_IRQNUM], "%d", pp->pci_cfginf->irq);
	sprintf( &pp->detail_data[HFC_ADAPWWPN], "%llx", (unsigned long long)pp->ww_name);
	sprintf( &pp->detail_data[HFC_DEVICEID], "%04x", (short)pp->pkg.device_id);

	/* Output SFP Information */
	if(!(core->fw_init_p->sfp_info.sfp_status & HFC_SFP_INSTALL)){
			sprintf( &pp->detail_data[HFC_SFP_INFO], "N/A");
	}else {
		memset(sfp_type_name,0,sizeof(sfp_type_name));
		memcpy(sfp_type_name, core->fw_init_p->sfp_info.sfp_type_name, sizeof(core->fw_init_p->sfp_info.sfp_type_name)); 
		hfc_fx_delete_space(sfp_type_name);
			
		memset(sfp_serial_no,0,sizeof(sfp_serial_no));
		memcpy(sfp_serial_no, core->fw_init_p->sfp_info.sfp_serial_no, sizeof(core->fw_init_p->sfp_info.sfp_serial_no)); 
		hfc_fx_delete_space(sfp_serial_no);

		memset(sfp_date_code,0,sizeof(sfp_date_code));
		memcpy(sfp_date_code, core->fw_init_p->sfp_info.sfp_date_code, sizeof(core->fw_init_p->sfp_info.sfp_date_code)); 
		hfc_fx_delete_space(sfp_date_code);

		if(!(core->fw_init_p->sfp_info.sfp_status & HFC_FX_SFP_VALID)){
			sprintf( &pp->detail_data[HFC_SFP_INFO], "incorrect data(%s, %s, %s)",
					sfp_type_name,
					sfp_serial_no,
					sfp_date_code);
		}else{
			sprintf( &pp->detail_data[HFC_SFP_INFO], "%s, %s, %s",
					sfp_type_name,
					sfp_serial_no,
					sfp_date_code);
		}
	}
#endif

#ifndef _HFC_NO_RASLOG
	for(raslog_retry=0; raslog_retry < HFC_RASLOG_RETRY; raslog_retry++){
		raslog = _hraslog((char *)ras_error_id1, (char *)resource_name, HFC_RASLOG_LEN, (char *)pp->detail_data ); /* FCLNX-GPL-311 */
		if( raslog == 0 ){
			pp->raslog_cnt++; 
			break;

		}
	}
	if( raslog == 1 ){
		HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBD)\n", pp->pport->dev_minor); 
	}
	else if( raslog == 2 ){
		HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBC)\n", pp->pport->dev_minor); 
	}
	else if( raslog == 0 ){
		memcpy( pp->detail_data, &err_rec->log_area[512], 512);
		sprintf( &pp->detail_data[HFC_LOGSEQNUM], "%d", pp->raslog_cnt);
		for(raslog_retry=0; raslog_retry < HFC_RASLOG_RETRY; raslog_retry++){
			raslog = _hraslog((char *)ras_error_id2, (char *)resource_name, HFC_RASLOG_LEN, (char *)pp->detail_data ); /* FCLNX-GPL-311 */
			if( raslog == 0 ){
				pp->raslog_cnt++; 
				break;
			}
		}
		if( raslog == 1 ){
			HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBD)\n", pp->pport->dev_minor); 
		}
		else if( raslog == 2 ){
			HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBC)\n", pp->pport->dev_minor); 
		}
	}
#endif
	return(raslog);
}



/*
 * Function:    hfc_five_fx_logout
 *
 * Purpose:     HW logout for FIVE-FX
 *
 * Arguments:   
 *  pp          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
 /* FCLNX-GPL-FX-098 */

const Type_mem_fx	ffx_mck_log[] = {
	/*==========================================================================
	 * System log(HW Log Format#1 - #4)
	 *========================================================================*/
	/*==== HW Log Format#1 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(FSYND(0)))	,	0x000C),				/* 02 CORE0_FAR/CSAR                  */
	HFCFX_HL_PCI(0x80,		TOP						,	0x0010),				/* 03 LSIREV                          */
	HFCFX_HL_PCI(0x80,		PORT(STATUSH)			,	0x0010),				/* 04 PORT_STATUS                     */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(STATUSH(0)))	,	0x0010),				/* 05 CORE0_STATUS                    */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(STATUSH(1)))	,	0x0010),				/* 06 CORE2_STATUS                    */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(STATUSH(2)))	,	0x0010),				/* 07 CORE3_STATUS					  */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(STATUSH(3)))	,	0x0010),				/* 08 CORE4_STATUS                    */
	HFCFX_HL_PCI(0x80,		ECID					,	0x0010),				/* 09 ECID                            */
	HFCFX_HL_PCI(0x80,		BOOTFAR					,	0x0010),				/* 10 BOOTFAR                         */
	HFCFX_HL_PCI(0x80,		SCAN(TLRBSY(0))			,	0x0010),				/* 11 TLRBSY                          */
	HFCFX_HL_PCI(0x80,		SCAN(RLRBSY(0))			,	0x0040),				/* 12 RLRBSY                          */
	HFCFX_HL_SYS_COREINFO(0x00),												/* 13 System Log CORE#0 INFO          */
	/*==== HW Log Format#2 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(FSYND(1)))	,	0x000C),				/* 02 CORE1_FAR/CSAR                  */
	HFCFX_HL_CFG(			HEAD			,	0x00A0),						/* 03 PCI Config                      */
	HFCFX_HL_TIMESTAMP(							0x0008),						/* 04 Time Stamp                      */
	HFCFX_HL_RESERVE(							0x0028),						/* 05 Reserved Location               */
	HFCFX_HL_SYS_COREINFO(0x01),												/* 06 System Log CORE#1 INFO          */
	/*==== HW Log Format#3 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(FSYND(2)))	,	0x000C),				/* 02 CORE2_FAR/CSAR                  */
	HFCFX_HL_CFG(			CAP(AER)				,	0x0040),				/* 03 PCIe AER Capability             */
	HFCFX_HL_CFG(			CAP(SEC)				,	0x0010),				/* 04 PCIe SEC-EXT Capability         */
	HFCFX_HL_PCI(0x80,		SCAN(INT(0))			,	0x0010),				/* 05 CORE0_INT                       */
	HFCFX_HL_PCI(0x80,		SCAN(INT(1))			,	0x0010),				/* 06 CORE1_INT                       */
	HFCFX_HL_PCI(0x80,		SCAN(INT(2))			,	0x0010),				/* 07 CORE2_INT                       */
	HFCFX_HL_PCI(0x80,		SCAN(INT(3))			,	0x0010),				/* 08 CORE3_INT                       */
	HFCFX_HL_WS(0x00,		CCAI(0)					,	0x0008),				/* 09 CORE0 CCA ULP#0                 */
	HFCFX_HL_WSULP(0x00,	OF_CCAI					,	0x0008),				/* 10 CORE0 CCA Current ULP   [OFFSET]*/
	HFCFX_HL_WS(0x01,		CCAI(0)					,	0x0008),				/* 11 CORE1 CCA ULP#0                 */
	HFCFX_HL_WSULP(0x01,	OF_CCAI					,	0x0008),				/* 12 CORE1 CCA Current ULP   [OFFSET]*/
	HFCFX_HL_WS(0x02,		CCAI(0)					,	0x0008),				/* 13 CORE2 CCA ULP#0                 */
	HFCFX_HL_WSULP(0x02,	OF_CCAI					,	0x0008),				/* 14 CORE2 CCA Current ULP   [OFFSET]*/
	HFCFX_HL_WS(0x03,		CCAI(0)					,	0x0008),				/* 15 CORE3 CCA ULP#0                 */
	HFCFX_HL_WSULP(0x03,	OF_CCAI					,	0x0008),				/* 16 CORE3 CCA Current ULP   [OFFSET]*/
	HFCFX_HL_SYS_COREINFO(0x02),												/* 17 System Log CORE#2 INFO          */
	/*==== HW Log Format#4 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_PCI(0x80,		SCAN(CORE(FSYND(3)))	,	0x000C),				/* 02 CORE3_FAR/CSAR                  */
	HFCFX_HL_CHK(			SCAN(CKCMNTU)			,	0x00D0),				/* 03 Checker            [INIT:CMN-TU]*/
	HFCFX_HL_SYS_COREINFO(0x03),												/* 04 System Log CORE#3 INFO          */
	/*==========================================================================
	 * Extend log(HW Log Format#5 - #93)
	 *========================================================================*/
	/*==== HW Log Format#5 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_RESERVE(									0x000C),				/* 02 Reserved Location               */
	HFCFX_HL_FLASH(			TOP						,	0x00F0),				/* 03 Flash Header(0x0000 - 0x00FE)   */
	HFCFX_HL_PCI(0x80,		SCAN(CKCMNTU)			,	0x01A0),				/* 04 Common(and PORT#0) Checher      */
	HFCFX_HL_PCIPT(			397						,	0x0160),				/* 05 PCIe Pakect Trace(#397 - #418)  */
	/*==== HW Log Format#6 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_RESERVE(									0x000C),				/* 02 Reserved Location               */
	HFCFX_HL_PCIPT(			419						,	0x03F0),				/* 03 PCIe Pakect Trace(#419 - #481)  */
	/*==== HW Log Format#7 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_RESERVE(									0x000C),				/* 02 Reserved Location               */
	HFCFX_HL_PCIPT(			482						,	0x01F0),				/* 03 PCIe Pakect Trace(#482 - #512)  */
	HFCFX_HL_RUT(			418						,	0x0200),				/* 04 RU Trace(#418 - #449)           */
	/*==== HW Log Format#8 ===================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_RESERVE(									0x000C),				/* 02 Reserved Location               */
	HFCFX_HL_RUT(			450						,	0x03F0),				/* 03 RU Trace(#450 - #512)           */
	/*==== HW Log Format#9 - #12 =============================================*/
	HFCFX_HL_SCAN_RSC(0x00),							/*==== #09 CORE#0 SCAN CORE Resouce ==========================*/
	HFCFX_HL_SCAN_RSC(0x01),							/*==== #10 CORE#1 SCAN CORE Resouce ==========================*/
	HFCFX_HL_SCAN_RSC(0x02),							/*==== #11 CORE#2 SCAN CORE Resouce ==========================*/
	HFCFX_HL_SCAN_RSC(0x03),							/*==== #12 CORE#3 SCAN CORE Resouce ==========================*/
	/*==== HW Log Format#13 - #20 ============================================*/
	HFCFX_HL_FW_BRANCH_TRC(0x00),						/*==== #13,14 CORE#0 FW Bramch Trace =========================*/
	HFCFX_HL_FW_BRANCH_TRC(0x01),						/*==== #15,16 CORE#1 FW Bramch Trace =========================*/
	HFCFX_HL_FW_BRANCH_TRC(0x02),						/*==== #17,18 CORE#2 FW Bramch Trace =========================*/
	HFCFX_HL_FW_BRANCH_TRC(0x03),						/*==== #19,20 CORE#3 FW Bramch Trace =========================*/
	/*==== HW Log Format#21 - #24 ============================================*/
	HFCFX_HL_BCR_INFO(0x00),							/*==== #21 CORE#0 BCR Info ===================================*/
	HFCFX_HL_BCR_INFO(0x01),							/*==== #22 CORE#1 BCR Info ===================================*/
	HFCFX_HL_BCR_INFO(0x02),							/*==== #23 CORE#2 BCR Info ===================================*/
	HFCFX_HL_BCR_INFO(0x03),							/*==== #24 CORE#3 BCR Info ===================================*/
	/*==== HW Log Format#25 - #92 ============================================*/
	HFCFX_HL_FW_INFO(0x00),								/*==== #25-41 CORE#0 FW WS Info and Trace ====================*/
	HFCFX_HL_FW_INFO(0x01),								/*==== #42-58 CORE#1 FW WS Info and Trace ====================*/
	HFCFX_HL_FW_INFO(0x02),								/*==== #59-75 CORE#2 FW WS Info and Trace ====================*/
	HFCFX_HL_FW_INFO(0x03),								/*==== #76-92 CORE#3 FW WS Info and Trace ====================*/
	/*==== HW Log Format#93 ==================================================*/
	HFCFX_HL_ERRID,																/* 01 (HW) ErrorID                    */
	HFCFX_HL_RESERVE(									0x000C),				/* 02 Reserved Location               */
	HFCFX_HL_CCR(			TOP(0),						0x0040),				/* 03 CORE#0 CCR Info                 */
	HFCFX_HL_CCR(			TOP(1),						0x0040),				/* 04 CORE#1 CCR Info                 */
	HFCFX_HL_CCR(			TOP(2),						0x0040),				/* 05 CORE#2 CCR Info                 */
	HFCFX_HL_CCR(			TOP(3),						0x0040),				/* 06 CORE#3 CCR Info                 */
	HFCFX_HL_RESERVE(									0x02F0),				/* 07 Reserved Location               */
	/*==== HW Log Format END MARK ============================================*/
	HFCFX_HL_ENDMARK
};


const Type_mem_fx ffx_iml_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00     , 4   , FXLOG_AC }, /********* #1 ***************/
	{ TYPE_PCI    , 0xC24    , 12  , FXLOG_AC }, /* CORE0_FAR/CSAR           */
	{ TYPE_PCI    , 0x0      , 16  , FXLOG_AC }, /* LSIREV                   */
	{ TYPE_PCI    , 0x10     , 16  , FXLOG_AC }, /* PORT_STATUS              */
	{ TYPE_PCI    , 0xC10    , 16  , FXLOG_AC }, /* CORE0_STATUS             */
	{ TYPE_PCI    , 0xD10    , 16  , FXLOG_AC }, /* CORE1_STATUS             */
	{ TYPE_PCI    , 0xE10    , 16  , FXLOG_AC }, /* CORE2_STATUS             */
	{ TYPE_PCI    , 0xF10    , 16  , FXLOG_AC }, /* CORE3_STATUS             */
	{ TYPE_PCI    , 0x8A0    , 16  , FXLOG_AC }, /* ECID                     */
	{ TYPE_PCI    , 0x840    , 16  , FXLOG_AC }, /* BOOTFAR                  */
	{ TYPE_PCI    , 0x4B90   , 16  , FXLOG_AC }, /* TLRBSY                   */
	{ TYPE_PCI    , 0x4D80   , 64  , FXLOG_AC }, /* RLRBSY                   */
	{ TYPE_PCI    , 0x4040   , 64  , FXLOG_C0 }, /* CORE0_GR0-F              */
	{ TYPE_PCI    , 0x4990   , 64  , FXLOG_C0 }, /* CORE0_RTBR               */
	{ TYPE_TRC1   , 0x0      , 128 , FXLOG_C0 }, /* FW branch trace          */
	{ TYPE_TRC4   , 0x0      , 112 , FXLOG_C0 }, /* WS_LEVEL_WK              */
	{ TYPE_0      , 0x100    , 28  , FXLOG_C0 }, /* WS_RID info ULP#0        */
	{ TYPE_0      , 0x158    , 4   , FXLOG_C0 }, /* WS_RID info ULP#0        */
	{ TYPE_TRC5   , 0x80     , 28  , FXLOG_C0 }, /* WS_RID info current ULP# */
	{ TYPE_TRC5   , 0xD8     , 4   , FXLOG_C0 }, /* WS_RID info current ULP# */
	{ TYPE_0      , 0x3000   , 16  , FXLOG_C0 }, /* WS_Common                */
	{ TYPE_0      , 0x3060   , 16  , FXLOG_C0 }, /* WS_Common                */
	{ TYPE_0      , 0x3070   , 16  , FXLOG_C0 }, /* WS_Common                */
	{ TYPE_0      , 0x31D0   , 16  , FXLOG_C0 }, /* WS_Common                */
	{ TYPE_EVT0   , FX_ETRC  , 192 , FXLOG_C0 }, /* LS Event trace(1019-1024)*/
	{ TYPE_FRM0   , FX_FTRC  , 112 , FXLOG_C0 }, /* LS Frame trace(1018-1024)*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #2 ***************/
	{ TYPE_PCI    , 0xD24    , 12  , FXLOG_AC }, /* CORE1_FAR/CSAR           */
	{ TYPE_CFG0   , 0x0      , 160 , FXLOG_AC }, /* PCIe CFG                 */
	{ TYPE_RSV    , 0x0      , 48  , FXLOG_AC }, /* Reserved                 */
	{ TYPE_PCI    , 0x5040   , 64  , FXLOG_C1 }, /* CORE1_GR0-F              */
	{ TYPE_PCI    , 0x5990   , 64  , FXLOG_C1 }, /* CORE1_RTBR               */
	{ TYPE_TRC1   , 0x0      , 128 , FXLOG_C1 }, /* FW branch trace          */
	{ TYPE_TRC4   , 0x0      , 112 , FXLOG_C1 }, /* WS_LEVEL_WK              */
	{ TYPE_0      , 0x100    , 28  , FXLOG_C1 }, /* WS_RID info ULP#0        */
	{ TYPE_0      , 0x158    , 4   , FXLOG_C1 }, /* WS_RID info ULP#0        */
	{ TYPE_TRC5   , 0x80     , 28  , FXLOG_C1 }, /* WS_RID info current ULP# */
	{ TYPE_TRC5   , 0xD8     , 4   , FXLOG_C1 }, /* WS_RID info current ULP# */
	{ TYPE_0      , 0x3000   , 16  , FXLOG_C1 }, /* WS_Common                */
	{ TYPE_0      , 0x3060   , 16  , FXLOG_C1 }, /* WS_Common                */
	{ TYPE_0      , 0x3070   , 16  , FXLOG_C1 }, /* WS_Common                */
	{ TYPE_0      , 0x31D0   , 16  , FXLOG_C1 }, /* WS_Common                */
	{ TYPE_EVT0   , FX_ETRC  , 192 , FXLOG_C1 }, /* LS Event trace(1019-1024)*/
	{ TYPE_FRM0   , FX_FTRC  , 112 , FXLOG_C1 }, /* LS Frame trace(1018-1024)*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #3 ***************/
	{ TYPE_PCI    , 0xE24    , 12  , FXLOG_AC }, /* CORE2_FAR/CSAR           */
	{ TYPE_CFG0   , 0x100    , 64  , FXLOG_AC }, /* PCIe CFG_AER             */
	{ TYPE_CFG0   , 0x254    , 16  , FXLOG_AC }, /* PCIe CFG_SEC-EXT         */
	{ TYPE_PCI    , 0x40A0   , 16  , FXLOG_AC }, /* CORE0_INT                */
	{ TYPE_PCI    , 0x50A0   , 16  , FXLOG_AC }, /* CORE1_INT                */
	{ TYPE_PCI    , 0x60A0   , 16  , FXLOG_AC }, /* CORE2_INT                */
	{ TYPE_PCI    , 0x70A0   , 16  , FXLOG_AC }, /* CORE3_INT                */
	{ TYPE_0      , 0x0      , 8   , FXLOG_C0 }, /* CORE0_WS_CCA_ULP#0       */
	{ TYPE_TRC6   , 0x0      , 8   , FXLOG_C0 }, /* CORE0_WS_CCA_CURRENT_ULP */
	{ TYPE_0      , 0x0      , 8   , FXLOG_C1 }, /* CORE1_WS_CCA_ULP#0       */
	{ TYPE_TRC6   , 0x0      , 8   , FXLOG_C1 }, /* CORE1_WS_CCA_CURRENT_ULP */
	{ TYPE_0      , 0x0      , 8   , FXLOG_C2 }, /* CORE2_WS_CCA_ULP#0       */
	{ TYPE_TRC6   , 0x0      , 8   , FXLOG_C2 }, /* CORE2_WS_CCA_CURRENT_ULP */
	{ TYPE_0      , 0x0      , 8   , FXLOG_C3 }, /* CORE3_WS_CCA_ULP#0       */
	{ TYPE_TRC6   , 0x0      , 8   , FXLOG_C3 }, /* CORE3_WS_CCA_CURRENT_ULP */
	{ TYPE_PCI    , 0x6040   , 64  , FXLOG_C2 }, /* CORE2_GR0-F              */
	{ TYPE_PCI    , 0x6990   , 64  , FXLOG_C2 }, /* CORE2_RTBR               */
	{ TYPE_TRC1   , 0x0      , 128 , FXLOG_C2 }, /* FW branch trace          */
	{ TYPE_TRC4   , 0x0      , 112 , FXLOG_C2 }, /* WS_LEVEL_WK              */
	{ TYPE_0      , 0x100    , 28  , FXLOG_C2 }, /* WS_RID info ULP#0        */
	{ TYPE_0      , 0x158    , 4   , FXLOG_C2 }, /* WS_RID info ULP#0        */
	{ TYPE_TRC5   , 0x80     , 28  , FXLOG_C2 }, /* WS_RID info current ULP# */
	{ TYPE_TRC5   , 0xD8     , 4   , FXLOG_C2 }, /* WS_RID info current ULP# */
	{ TYPE_0      , 0x3000   , 16  , FXLOG_C2 }, /* WS_Common                */
	{ TYPE_0      , 0x3060   , 16  , FXLOG_C2 }, /* WS_Common                */
	{ TYPE_0      , 0x3070   , 16  , FXLOG_C2 }, /* WS_Common                */
	{ TYPE_0      , 0x31D0   , 16  , FXLOG_C2 }, /* WS_Common                */
	{ TYPE_EVT0   , FX_ETRC  , 192 , FXLOG_C2 }, /* LS Event trace(1019-1024)*/
	{ TYPE_FRM0   , FX_FTRC  , 112 , FXLOG_C2 }, /* LS Frame trace(1018-1024)*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #4 ***************/
	{ TYPE_PCI    , 0xF24    , 12  , FXLOG_AC }, /* CORE3_FAR/CSAR           */
	{ TYPE_PCI    , 0x4370   , 160 , FXLOG_AC }, /* CHECKER                  */
	{ TYPE_RSV    , 0x0      , 48  , FXLOG_AC }, /* CHECKER MCK_Code         */
	{ TYPE_PCI    , 0x7040   , 64  , FXLOG_C3 }, /* CORE3_GR0-F              */
	{ TYPE_PCI    , 0x7990   , 64  , FXLOG_C3 }, /* CORE3_RTBR               */
	{ TYPE_TRC1   , 0x0      , 128 , FXLOG_C3 }, /* FW branch trace          */
	{ TYPE_TRC4   , 0x0      , 112 , FXLOG_C3 }, /* WS_LEVEL_WK              */
	{ TYPE_0      , 0x100    , 28  , FXLOG_C3 }, /* WS_RID info ULP#0        */
	{ TYPE_0      , 0x158    , 4   , FXLOG_C3 }, /* WS_RID info ULP#0        */
	{ TYPE_TRC5   , 0x80     , 28  , FXLOG_C3 }, /* WS_RID info current ULP# */
	{ TYPE_TRC5   , 0xD8     , 4   , FXLOG_C3 }, /* WS_RID info current ULP# */
	{ TYPE_0      , 0x3000   , 16  , FXLOG_C3 }, /* WS_Common                */
	{ TYPE_0      , 0x3060   , 16  , FXLOG_C3 }, /* WS_Common                */
	{ TYPE_0      , 0x3070   , 16  , FXLOG_C3 }, /* WS_Common                */
	{ TYPE_0      , 0x31D0   , 16  , FXLOG_C3 }, /* WS_Common                */
	{ TYPE_EVT0   , FX_ETRC  , 192 , FXLOG_C3 }, /* LS Event trace(1019-1024)*/
	{ TYPE_FRM0   , FX_FTRC  , 112 , FXLOG_C3 }, /* LS Frame trace(1018-1024)*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #5 ***************/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_2      , 0x0      , 240 , FXLOG_AC }, /* Flash Header             */
	{ TYPE_PCI    , 0x4300   , 416 , FXLOG_AC }, /* Common checker           */
	{ TYPE_TRC2   , 0x0      , 352 , FXLOG_AC }, /* PCIe packet trace #13-34 */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #6 ***************/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC2   , 0x0      , 1008, FXLOG_AC }, /* PCIe packet trace #35-97 */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #7 ***************/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC2   , 0x0      , 496 , FXLOG_AC }, /* PCIe packet trace #98-128*/
	{ TYPE_TRC3   , 0x0      , 512 , FXLOG_AC }, /* RU trace #34-65          */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #8 ***************/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC3   , 0x0      , 1008, FXLOG_AC }, /* RU trace #66-128         */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #9 (core0) *******/
	{ TYPE_PCI    , 0x110    , 4   , FXLOG_AC }, /* PTYP                     */
	{ TYPE_PCI    , 0x4AB4   , 4   , FXLOG_C0 }, /* ULP                      */
	{ TYPE_PCI    , 0x4CFC   , 4   , FXLOG_C0 }, /* TOD                      */
	{ TYPE_PCI    , 0x4F00   , 240 , FXLOG_C0 }, /* Core checker             */
	{ TYPE_0      , 0x0      , 512 , FXLOG_C0 }, /* GRS                      */
	{ TYPE_PCI    , 0x4090   , 16  , FXLOG_C0 }, /* LBR0-3                   */	
	{ TYPE_PCI    , 0x4D00   , 32  , FXLOG_C0 }, /* OTHER_BR                 */
	{ TYPE_PCI    , 0x4510   , 8   , FXLOG_C0 }, /* FTBBSY                   */
	{ TYPE_PCI    , 0x4520   , 8   , FXLOG_C0 }, /* STBBSY                   */
	{ TYPE_PCI    , 0x4B00   , 4   , FXLOG_C0 }, /* RLRA                     */
	{ TYPE_PCI    , 0x4B04   , 4   , FXLOG_C0 }, /* TLRA                     */
	{ TYPE_PCI    , 0x4500   , 4   , FXLOG_C0 }, /* BSTRA                    */
	{ TYPE_PCI    , 0x4560   , 4   , FXLOG_C0 }, /* RID                      */
	{ TYPE_RSV    , 0x0      , 48  , FXLOG_AC }, /* RSV                      */	
	{ TYPE_PCI    , 0x41C0   , 4   , FXLOG_C0 }, /* ULP#0 FRiP/FRoP          */
	{ TYPE_TRC7   , 0x1004   , 124 , FXLOG_C0 }, /* ULP#1 - 1F FRiP/FRoP     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #10(core1) *******/
	{ TYPE_PCI    , 0x110    , 4   , FXLOG_AC }, /* PTYP                     */
	{ TYPE_PCI    , 0x4AB4   , 4   , FXLOG_C1 }, /* ULP                      */
	{ TYPE_PCI    , 0x4CFC   , 4   , FXLOG_C1 }, /* TOD                      */
	{ TYPE_PCI    , 0x4F00   , 240 , FXLOG_C1 }, /* Core checker             */
	{ TYPE_0      , 0x0      , 512 , FXLOG_C1 }, /* GRS                      */
	{ TYPE_PCI    , 0x4090   , 16  , FXLOG_C1 }, /* LBR0-3                   */	
	{ TYPE_PCI    , 0x4D00   , 32  , FXLOG_C1 }, /* OTHER_BR                 */
	{ TYPE_PCI    , 0x4510   , 8   , FXLOG_C1 }, /* FTBBSY                   */
	{ TYPE_PCI    , 0x4520   , 8   , FXLOG_C1 }, /* STBBSY                   */
	{ TYPE_PCI    , 0x4B00   , 4   , FXLOG_C1 }, /* RLRA                     */
	{ TYPE_PCI    , 0x4B04   , 4   , FXLOG_C1 }, /* TLRA                     */
	{ TYPE_PCI    , 0x4500   , 4   , FXLOG_C1 }, /* BSTRA                    */
	{ TYPE_PCI    , 0x4560   , 4   , FXLOG_C1 }, /* RID                      */
	{ TYPE_RSV    , 0x0      , 48  , FXLOG_AC }, /* RSV                      */	
	{ TYPE_PCI    , 0x41C0   , 4   , FXLOG_C1 }, /* ULP#0 FRiP/FRoP          */
	{ TYPE_TRC7   , 0x1004   , 124 , FXLOG_C1 }, /* ULP#1 - 1F FRiP/FRoP     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #11(core2) *******/
	{ TYPE_PCI    , 0x110    , 4   , FXLOG_AC }, /* PTYP                     */
	{ TYPE_PCI    , 0x4AB4   , 4   , FXLOG_C2 }, /* ULP                      */
	{ TYPE_PCI    , 0x4CFC   , 4   , FXLOG_C2 }, /* TOD                      */
	{ TYPE_PCI    , 0x4F00   , 240 , FXLOG_C2 }, /* Core checker             */
	{ TYPE_0      , 0x0      , 512 , FXLOG_C2 }, /* GRS                      */
	{ TYPE_PCI    , 0x4090   , 16  , FXLOG_C2 }, /* LBR0-3                   */	
	{ TYPE_PCI    , 0x4D00   , 32  , FXLOG_C2 }, /* OTHER_BR                 */
	{ TYPE_PCI    , 0x4510   , 8   , FXLOG_C2 }, /* FTBBSY                   */
	{ TYPE_PCI    , 0x4520   , 8   , FXLOG_C2 }, /* STBBSY                   */
	{ TYPE_PCI    , 0x4B00   , 4   , FXLOG_C2 }, /* RLRA                     */
	{ TYPE_PCI    , 0x4B04   , 4   , FXLOG_C2 }, /* TLRA                     */
	{ TYPE_PCI    , 0x4500   , 4   , FXLOG_C2 }, /* BSTRA                    */
	{ TYPE_PCI    , 0x4560   , 4   , FXLOG_C2 }, /* RID                      */
	{ TYPE_RSV    , 0x0      , 48  , FXLOG_AC }, /* RSV                      */	
	{ TYPE_PCI    , 0x41C0   , 4   , FXLOG_C2 }, /* ULP#0 FRiP/FRoP          */
	{ TYPE_TRC7   , 0x1004   , 124 , FXLOG_C2 }, /* ULP#1 - 1F FRiP/FRoP     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #12(core3) *******/
	{ TYPE_PCI    , 0x110    , 4   , FXLOG_AC }, /* PTYP                     */
	{ TYPE_PCI    , 0x4AB4   , 4   , FXLOG_C3 }, /* ULP                      */
	{ TYPE_PCI    , 0x4CFC   , 4   , FXLOG_C3 }, /* TOD                      */
	{ TYPE_PCI    , 0x4F00   , 240 , FXLOG_C3 }, /* Core checker             */
	{ TYPE_0      , 0x0      , 512 , FXLOG_C3 }, /* GRS                      */
	{ TYPE_PCI    , 0x4090   , 16  , FXLOG_C3 }, /* LBR0-3                   */	
	{ TYPE_PCI    , 0x4D00   , 32  , FXLOG_C3 }, /* OTHER_BR                 */
	{ TYPE_PCI    , 0x4510   , 8   , FXLOG_C3 }, /* FTBBSY                   */
	{ TYPE_PCI    , 0x4520   , 8   , FXLOG_C3 }, /* STBBSY                   */
	{ TYPE_PCI    , 0x4B00   , 4   , FXLOG_C3 }, /* RLRA                     */
	{ TYPE_PCI    , 0x4B04   , 4   , FXLOG_C3 }, /* TLRA                     */
	{ TYPE_PCI    , 0x4500   , 4   , FXLOG_C3 }, /* BSTRA                    */
	{ TYPE_PCI    , 0x4560   , 4   , FXLOG_C3 }, /* RID                      */
	{ TYPE_RSV    , 0x0      , 48  , FXLOG_AC }, /* RSV                      */	
	{ TYPE_PCI    , 0x41C0   , 4   , FXLOG_C3 }, /* ULP#0 FRiP/FRoP          */
	{ TYPE_TRC7   , 0x1004   , 124 , FXLOG_C3 }, /* ULP#1 - 1F FRiP/FRoP     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #13(core0) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C0 }, /* FW branch trace #515-769 */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #14(core0) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C0 }, /* FW branch trace #770-1024*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #15(core1) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C1 }, /* FW branch trace #515-769 */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #16(core1) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C1 }, /* FW branch trace #770-1024*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #17(core2) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C2 }, /* FW branch trace #515-769 */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #18(core2) *******/
	{ TYPE_TRC0   , 0x0      , 1020, FXLOG_C2 }, /* FW branch trace #770-1024*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #19(core3) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C3 }, /* FW branch trace #515-769 */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #20(core3) *******/
	{ TYPE_TRC1   , 0x0      , 1020, FXLOG_C3 }, /* FW branch trace #770-1024*/
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #21(core0) *******/
	{ TYPE_0      , 0x4      , 1020, FXLOG_C0 }, /* BCR                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #22(core1) *******/
	{ TYPE_0      , 0x4      , 1020, FXLOG_C1 }, /* BCR                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #23(core2) *******/
	{ TYPE_0      , 0x4      , 1020, FXLOG_C2 }, /* BCR                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #24(core3) *******/
	{ TYPE_0      , 0x4      , 1020, FXLOG_C3 }, /* BCR                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #25(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x080    , 128 , FXLOG_C0 }, /* RID area info ULP#00     */
	{ TYPE_0      , 0x180    , 128 , FXLOG_C0 }, /* RID area info ULP#01     */
	{ TYPE_0      , 0x280    , 128 , FXLOG_C0 }, /* RID area info ULP#02     */
	{ TYPE_0      , 0x380    , 128 , FXLOG_C0 }, /* RID area info ULP#03     */
	{ TYPE_0      , 0x480    , 128 , FXLOG_C0 }, /* RID area info ULP#04     */
	{ TYPE_0      , 0x580    , 128 , FXLOG_C0 }, /* RID area info ULP#05     */
	{ TYPE_0      , 0x680    , 128 , FXLOG_C0 }, /* RID area info ULP#06     */
	{ TYPE_0      , 0x780    , 112 , FXLOG_C0 }, /* RID area info ULP#07     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #26(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x7F0    , 16  , FXLOG_C0 }, /* RID area info ULP#07     */
	{ TYPE_0      , 0x880    , 128 , FXLOG_C0 }, /* RID area info ULP#08     */
	{ TYPE_0      , 0x980    , 128 , FXLOG_C0 }, /* RID area info ULP#09     */
	{ TYPE_0      , 0xA80    , 128 , FXLOG_C0 }, /* RID area info ULP#0A     */
	{ TYPE_0      , 0xB80    , 128 , FXLOG_C0 }, /* RID area info ULP#0B     */
	{ TYPE_0      , 0xC80    , 128 , FXLOG_C0 }, /* RID area info ULP#0C     */
	{ TYPE_0      , 0xD80    , 128 , FXLOG_C0 }, /* RID area info ULP#0D     */
	{ TYPE_0      , 0xE80    , 128 , FXLOG_C0 }, /* RID area info ULP#0E     */
	{ TYPE_0      , 0xF80    , 96  , FXLOG_C0 }, /* RID area info ULP#0F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #27(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0xFE0    , 32  , FXLOG_C0 }, /* RID area info ULP#0F     */
	{ TYPE_0      , 0x1080   , 128 , FXLOG_C0 }, /* RID area info ULP#10     */
	{ TYPE_0      , 0x1180   , 128 , FXLOG_C0 }, /* RID area info ULP#11     */
	{ TYPE_0      , 0x1280   , 128 , FXLOG_C0 }, /* RID area info ULP#12     */
	{ TYPE_0      , 0x1380   , 128 , FXLOG_C0 }, /* RID area info ULP#13     */
	{ TYPE_0      , 0x1480   , 128 , FXLOG_C0 }, /* RID area info ULP#14     */
	{ TYPE_0      , 0x1580   , 128 , FXLOG_C0 }, /* RID area info ULP#15     */
	{ TYPE_0      , 0x1680   , 128 , FXLOG_C0 }, /* RID area info ULP#16     */
	{ TYPE_0      , 0x1780   , 80  , FXLOG_C0 }, /* RID area info ULP#17     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #28(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x17D0   , 48  , FXLOG_C0 }, /* RID area info ULP#17     */
	{ TYPE_0      , 0x1880   , 128 , FXLOG_C0 }, /* RID area info ULP#18     */
	{ TYPE_0      , 0x1980   , 128 , FXLOG_C0 }, /* RID area info ULP#19     */
	{ TYPE_0      , 0x1A80   , 128 , FXLOG_C0 }, /* RID area info ULP#1A     */
	{ TYPE_0      , 0x1B80   , 128 , FXLOG_C0 }, /* RID area info ULP#1B     */
	{ TYPE_0      , 0x1C80   , 128 , FXLOG_C0 }, /* RID area info ULP#1C     */
	{ TYPE_0      , 0x1D80   , 128 , FXLOG_C0 }, /* RID area info ULP#1D     */
	{ TYPE_0      , 0x1E80   , 128 , FXLOG_C0 }, /* RID area info ULP#1E     */
	{ TYPE_0      , 0x1F80   , 64  , FXLOG_C0 }, /* RID area info ULP#1F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #29(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x1FC0   , 64  , FXLOG_C0 }, /* RID area info ULP#1F     */
	{ TYPE_0      , 0x2000   , 256 , FXLOG_C0 }, /* LEVEL#0_WORK             */
	{ TYPE_0      , 0x2100   , 256 , FXLOG_C0 }, /* LEVEL#1A_WORK            */
	{ TYPE_0      , 0x2200   , 256 , FXLOG_C0 }, /* LEVEL#2A_WORK            */
	{ TYPE_0      , 0x2300   , 176 , FXLOG_C0 }, /* LEVEL#3_WORK             */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #30(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23B0   , 80  , FXLOG_C0 }, /* LEVEL#3_WORK             */
	{ TYPE_0      , 0x2400   , 256 , FXLOG_C0 }, /* LEVEL#1B_WORK            */
	{ TYPE_0      , 0x2500   , 256 , FXLOG_C0 }, /* LEVEL#2B_WORK            */
	{ TYPE_0      , 0x2600   , 256 , FXLOG_C0 }, /* LEVEL#1C_WORK            */
	{ TYPE_0      , 0x2700   , 160 , FXLOG_C0 }, /* LEVEL#2C_WORK            */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #31(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23A0   , 96  , FXLOG_C0 }, /* LEVEL#2C_WORK            */
	{ TYPE_0      , 0x3000   , 912 , FXLOG_C0 }, /* WS_Common                */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #32(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x3390   , 112 , FXLOG_C0 }, /* WS_Common                */
	{ TYPE_TRCA   , 0x0      , 512 , FXLOG_C0 }, /* FRAME_A                  */
	{ TYPE_0      , 0x23400  , 384 , FXLOG_C0 }, /* SCSI Cancel (driver)     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #33(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23580  , 640 , FXLOG_C0 }, /* SCSI Cancel (driver)     */
	{ TYPE_0      , 0x23480  , 368 , FXLOG_C0 }, /* SCSI Cancel (async)      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #34(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x235F0  , 656 , FXLOG_C0 }, /* SCSI Cancel (async)      */
	{ TYPE_EVT0   , FX_ETRC  , 352 , FXLOG_C0 }, /* Event trace #897-907     */	
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #35(core0) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C0 }, /* Event trace #908-938     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #36(core0) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C0 }, /* Event trace #939-969     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #37(core0) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C0 }, /* Event trace #970-1000    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #38(core0) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 768 , FXLOG_C0 }, /* Event trace #1001-1024   */
	{ TYPE_FRM0   , FX_FTRC  , 224 , FXLOG_C0 }, /* Frame trace (14event)    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #39(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 1008, FXLOG_C0 }, /* Frame trace (63event)    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #40(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 816 , FXLOG_C0 }, /* Frame trace (51event)    */
	{ TYPE_TRC9   , 0x2B000  , 192 , FXLOG_C0 }, /* LPAR trace ULP#0         */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #41(core0) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC9   , 0x2B0C0  , 320 , FXLOG_C0 }, /* LPAR trace ULP#0         */
	{ TYPE_TRC9   , 0x0      , 512 , FXLOG_C0 }, /* LPAR trace CURRENT_ULP   */
	{ TYPE_RSV    , 0x0      , 176 , FXLOG_AC }, /* RSV                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #42(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x080    , 128 , FXLOG_C1 }, /* RID area info ULP#00     */
	{ TYPE_0      , 0x180    , 128 , FXLOG_C1 }, /* RID area info ULP#01     */
	{ TYPE_0      , 0x280    , 128 , FXLOG_C1 }, /* RID area info ULP#02     */
	{ TYPE_0      , 0x380    , 128 , FXLOG_C1 }, /* RID area info ULP#03     */
	{ TYPE_0      , 0x480    , 128 , FXLOG_C1 }, /* RID area info ULP#04     */
	{ TYPE_0      , 0x580    , 128 , FXLOG_C1 }, /* RID area info ULP#05     */
	{ TYPE_0      , 0x680    , 128 , FXLOG_C1 }, /* RID area info ULP#06     */
	{ TYPE_0      , 0x780    , 112 , FXLOG_C1 }, /* RID area info ULP#07     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #43(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x7F0    , 16  , FXLOG_C1 }, /* RID area info ULP#07     */
	{ TYPE_0      , 0x880    , 128 , FXLOG_C1 }, /* RID area info ULP#08     */
	{ TYPE_0      , 0x980    , 128 , FXLOG_C1 }, /* RID area info ULP#09     */
	{ TYPE_0      , 0xA80    , 128 , FXLOG_C1 }, /* RID area info ULP#0A     */
	{ TYPE_0      , 0xB80    , 128 , FXLOG_C1 }, /* RID area info ULP#0B     */
	{ TYPE_0      , 0xC80    , 128 , FXLOG_C1 }, /* RID area info ULP#0C     */
	{ TYPE_0      , 0xD80    , 128 , FXLOG_C1 }, /* RID area info ULP#0D     */
	{ TYPE_0      , 0xE80    , 128 , FXLOG_C1 }, /* RID area info ULP#0E     */
	{ TYPE_0      , 0xF80    , 96  , FXLOG_C1 }, /* RID area info ULP#0F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #44(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0xFE0    , 32  , FXLOG_C1 }, /* RID area info ULP#0F     */
	{ TYPE_0      , 0x1080   , 128 , FXLOG_C1 }, /* RID area info ULP#10     */
	{ TYPE_0      , 0x1180   , 128 , FXLOG_C1 }, /* RID area info ULP#11     */
	{ TYPE_0      , 0x1280   , 128 , FXLOG_C1 }, /* RID area info ULP#12     */
	{ TYPE_0      , 0x1380   , 128 , FXLOG_C1 }, /* RID area info ULP#13     */
	{ TYPE_0      , 0x1480   , 128 , FXLOG_C1 }, /* RID area info ULP#14     */
	{ TYPE_0      , 0x1580   , 128 , FXLOG_C1 }, /* RID area info ULP#15     */
	{ TYPE_0      , 0x1680   , 128 , FXLOG_C1 }, /* RID area info ULP#16     */
	{ TYPE_0      , 0x1780   , 80  , FXLOG_C1 }, /* RID area info ULP#17     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #45(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x17D0   , 48  , FXLOG_C1 }, /* RID area info ULP#17     */
	{ TYPE_0      , 0x1880   , 128 , FXLOG_C1 }, /* RID area info ULP#18     */
	{ TYPE_0      , 0x1980   , 128 , FXLOG_C1 }, /* RID area info ULP#19     */
	{ TYPE_0      , 0x1A80   , 128 , FXLOG_C1 }, /* RID area info ULP#1A     */
	{ TYPE_0      , 0x1B80   , 128 , FXLOG_C1 }, /* RID area info ULP#1B     */
	{ TYPE_0      , 0x1C80   , 128 , FXLOG_C1 }, /* RID area info ULP#1C     */
	{ TYPE_0      , 0x1D80   , 128 , FXLOG_C1 }, /* RID area info ULP#1D     */
	{ TYPE_0      , 0x1E80   , 128 , FXLOG_C1 }, /* RID area info ULP#1E     */
	{ TYPE_0      , 0x1F80   , 64  , FXLOG_C1 }, /* RID area info ULP#1F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #46(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x1FC0   , 64  , FXLOG_C1 }, /* RID area info ULP#1F     */
	{ TYPE_0      , 0x2000   , 256 , FXLOG_C1 }, /* LEVEL#0_WORK             */
	{ TYPE_0      , 0x2100   , 256 , FXLOG_C1 }, /* LEVEL#1A_WORK            */
	{ TYPE_0      , 0x2200   , 256 , FXLOG_C1 }, /* LEVEL#2A_WORK            */
	{ TYPE_0      , 0x2300   , 176 , FXLOG_C1 }, /* LEVEL#3_WORK             */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #47(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23B0   , 80  , FXLOG_C1 }, /* LEVEL#3_WORK             */
	{ TYPE_0      , 0x2400   , 256 , FXLOG_C1 }, /* LEVEL#1B_WORK            */
	{ TYPE_0      , 0x2500   , 256 , FXLOG_C1 }, /* LEVEL#2B_WORK            */
	{ TYPE_0      , 0x2600   , 256 , FXLOG_C1 }, /* LEVEL#1C_WORK            */
	{ TYPE_0      , 0x2700   , 160 , FXLOG_C1 }, /* LEVEL#2C_WORK            */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #48(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23A0   , 96  , FXLOG_C1 }, /* LEVEL#2C_WORK            */
	{ TYPE_0      , 0x3000   , 912 , FXLOG_C1 }, /* WS_Common                */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #49(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x3390   , 112 , FXLOG_C1 }, /* WS_Common                */
	{ TYPE_TRCA   , 0x0      , 512 , FXLOG_C1 }, /* FRAME_A                  */
	{ TYPE_0      , 0x29000  , 384 , FXLOG_C1 }, /* SCSI Cancel (driver)     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #50(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x29180  , 640 , FXLOG_C1 }, /* SCSI Cancel (driver)     */
	{ TYPE_0      , 0x29400  , 368 , FXLOG_C1 }, /* SCSI Cancel (async)      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #51(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x29570  , 656 , FXLOG_C1 }, /* SCSI Cancel (async)      */
	{ TYPE_EVT0   , FX_ETRC  , 352 , FXLOG_C1 }, /* Event trace #897-907     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #52(core1) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C1 }, /* Event trace #908-938     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #53(core1) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C1 }, /* Event trace #939-969     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #54(core1) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C1 }, /* Event trace #970-1000    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #55(core1) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 768 , FXLOG_C1 }, /* Event trace #1001-1024   */
	{ TYPE_FRM0   , FX_FTRC  , 224 , FXLOG_C1 }, /* Frame trace (14event)    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #56(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 1008, FXLOG_C1 }, /* Frame trace (63event)    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #57(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 816 , FXLOG_C1 }, /* Frame trace (51event)    */
	{ TYPE_TRC9   , 0x2B000  , 192 , FXLOG_C1 }, /* LPAR trace ULP#0         */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #58(core1) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC9   , 0x2B0C0  , 320 , FXLOG_C1 }, /* LPAR trace ULP#0         */
	{ TYPE_TRC9   , 0x0      , 512 , FXLOG_C1 }, /* LPAR trace CURRENT_ULP   */
	{ TYPE_RSV    , 0x0      , 176 , FXLOG_AC }, /* RSV                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #59(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x080    , 128 , FXLOG_C2 }, /* RID area info ULP#00     */
	{ TYPE_0      , 0x180    , 128 , FXLOG_C2 }, /* RID area info ULP#01     */
	{ TYPE_0      , 0x280    , 128 , FXLOG_C2 }, /* RID area info ULP#02     */
	{ TYPE_0      , 0x380    , 128 , FXLOG_C2 }, /* RID area info ULP#03     */
	{ TYPE_0      , 0x480    , 128 , FXLOG_C2 }, /* RID area info ULP#04     */
	{ TYPE_0      , 0x580    , 128 , FXLOG_C2 }, /* RID area info ULP#05     */
	{ TYPE_0      , 0x680    , 128 , FXLOG_C2 }, /* RID area info ULP#06     */
	{ TYPE_0      , 0x780    , 112 , FXLOG_C2 }, /* RID area info ULP#07     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #60(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x7F0    , 16  , FXLOG_C2 }, /* RID area info ULP#07     */
	{ TYPE_0      , 0x880    , 128 , FXLOG_C2 }, /* RID area info ULP#08     */
	{ TYPE_0      , 0x980    , 128 , FXLOG_C2 }, /* RID area info ULP#09     */
	{ TYPE_0      , 0xA80    , 128 , FXLOG_C2 }, /* RID area info ULP#0A     */
	{ TYPE_0      , 0xB80    , 128 , FXLOG_C2 }, /* RID area info ULP#0B     */
	{ TYPE_0      , 0xC80    , 128 , FXLOG_C2 }, /* RID area info ULP#0C     */
	{ TYPE_0      , 0xD80    , 128 , FXLOG_C2 }, /* RID area info ULP#0D     */
	{ TYPE_0      , 0xE80    , 128 , FXLOG_C2 }, /* RID area info ULP#0E     */
	{ TYPE_0      , 0xF80    , 96  , FXLOG_C2 }, /* RID area info ULP#0F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #61(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0xFE0    , 32  , FXLOG_C2 }, /* RID area info ULP#0F     */
	{ TYPE_0      , 0x1080   , 128 , FXLOG_C2 }, /* RID area info ULP#10     */
	{ TYPE_0      , 0x1180   , 128 , FXLOG_C2 }, /* RID area info ULP#11     */
	{ TYPE_0      , 0x1280   , 128 , FXLOG_C2 }, /* RID area info ULP#12     */
	{ TYPE_0      , 0x1380   , 128 , FXLOG_C2 }, /* RID area info ULP#13     */
	{ TYPE_0      , 0x1480   , 128 , FXLOG_C2 }, /* RID area info ULP#14     */
	{ TYPE_0      , 0x1580   , 128 , FXLOG_C2 }, /* RID area info ULP#15     */
	{ TYPE_0      , 0x1680   , 128 , FXLOG_C2 }, /* RID area info ULP#16     */
	{ TYPE_0      , 0x1780   , 80  , FXLOG_C2 }, /* RID area info ULP#17     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #62(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x17D0   , 48  , FXLOG_C2 }, /* RID area info ULP#17     */
	{ TYPE_0      , 0x1880   , 128 , FXLOG_C2 }, /* RID area info ULP#18     */
	{ TYPE_0      , 0x1980   , 128 , FXLOG_C2 }, /* RID area info ULP#19     */
	{ TYPE_0      , 0x1A80   , 128 , FXLOG_C2 }, /* RID area info ULP#1A     */
	{ TYPE_0      , 0x1B80   , 128 , FXLOG_C2 }, /* RID area info ULP#1B     */
	{ TYPE_0      , 0x1C80   , 128 , FXLOG_C2 }, /* RID area info ULP#1C     */
	{ TYPE_0      , 0x1D80   , 128 , FXLOG_C2 }, /* RID area info ULP#1D     */
	{ TYPE_0      , 0x1E80   , 128 , FXLOG_C2 }, /* RID area info ULP#1E     */
	{ TYPE_0      , 0x1F80   , 64  , FXLOG_C2 }, /* RID area info ULP#1F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #63(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x1FC0   , 64  , FXLOG_C2 }, /* RID area info ULP#1F     */
	{ TYPE_0      , 0x2000   , 256 , FXLOG_C2 }, /* LEVEL#0_WORK             */
	{ TYPE_0      , 0x2100   , 256 , FXLOG_C2 }, /* LEVEL#1A_WORK            */
	{ TYPE_0      , 0x2200   , 256 , FXLOG_C2 }, /* LEVEL#2A_WORK            */
	{ TYPE_0      , 0x2300   , 176 , FXLOG_C2 }, /* LEVEL#3_WORK             */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #64(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23B0   , 80  , FXLOG_C2 }, /* LEVEL#3_WORK             */
	{ TYPE_0      , 0x2400   , 256 , FXLOG_C2 }, /* LEVEL#1B_WORK            */
	{ TYPE_0      , 0x2500   , 256 , FXLOG_C2 }, /* LEVEL#2B_WORK            */
	{ TYPE_0      , 0x2600   , 256 , FXLOG_C2 }, /* LEVEL#1C_WORK            */
	{ TYPE_0      , 0x2700   , 160 , FXLOG_C2 }, /* LEVEL#2C_WORK            */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #65(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23A0   , 96  , FXLOG_C2 }, /* LEVEL#2C_WORK            */
	{ TYPE_0      , 0x3000   , 912 , FXLOG_C2 }, /* WS_Common                */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #66(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x3390   , 112 , FXLOG_C2 }, /* WS_Common                */
	{ TYPE_TRCA   , 0x0      , 512 , FXLOG_C2 }, /* FRAME_A                  */
	{ TYPE_0      , 0x29000  , 384 , FXLOG_C2 }, /* SCSI Cancel (driver)     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #67(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x29180  , 640 , FXLOG_C2 }, /* SCSI Cancel (driver)     */
	{ TYPE_0      , 0x29400  , 368 , FXLOG_C2 }, /* SCSI Cancel (async)      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #68(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x29570  , 656 , FXLOG_C2 }, /* SCSI Cancel (async)      */
	{ TYPE_EVT0   , FX_ETRC  , 352 , FXLOG_C2 }, /* Event trace #897-907     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #69(core2) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C2 }, /* Event trace #908-938     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #70(core2) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C2 }, /* Event trace #939-969     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #71(core2) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C2 }, /* Event trace #970-1000    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #72(core2) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 768 , FXLOG_C2 }, /* Event trace #1001-1024   */
	{ TYPE_FRM0   , FX_FTRC  , 224 , FXLOG_C2 }, /* Frame trace (14event)    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #73(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 1008, FXLOG_C2 }, /* Frame trace (63event)    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #74(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 816 , FXLOG_C2 }, /* Frame trace (51event)    */
	{ TYPE_TRC9   , 0x2B000  , 192 , FXLOG_C2 }, /* LPAR trace ULP#0         */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #75(core2) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC9   , 0x2B0C0  , 320 , FXLOG_C2 }, /* LPAR trace ULP#0         */
	{ TYPE_TRC9   , 0x0      , 512 , FXLOG_C2 }, /* LPAR trace CURRENT_ULP   */
	{ TYPE_RSV    , 0x0      , 176 , FXLOG_AC }, /* RSV                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #76(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x080    , 128 , FXLOG_C3 }, /* RID area info ULP#00     */
	{ TYPE_0      , 0x180    , 128 , FXLOG_C3 }, /* RID area info ULP#01     */
	{ TYPE_0      , 0x280    , 128 , FXLOG_C3 }, /* RID area info ULP#02     */
	{ TYPE_0      , 0x380    , 128 , FXLOG_C3 }, /* RID area info ULP#03     */
	{ TYPE_0      , 0x480    , 128 , FXLOG_C3 }, /* RID area info ULP#04     */
	{ TYPE_0      , 0x580    , 128 , FXLOG_C3 }, /* RID area info ULP#05     */
	{ TYPE_0      , 0x680    , 128 , FXLOG_C3 }, /* RID area info ULP#06     */
	{ TYPE_0      , 0x780    , 112 , FXLOG_C3 }, /* RID area info ULP#07     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #77(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x7F0    , 16  , FXLOG_C3 }, /* RID area info ULP#07     */
	{ TYPE_0      , 0x880    , 128 , FXLOG_C3 }, /* RID area info ULP#08     */
	{ TYPE_0      , 0x980    , 128 , FXLOG_C3 }, /* RID area info ULP#09     */
	{ TYPE_0      , 0xA80    , 128 , FXLOG_C3 }, /* RID area info ULP#0A     */
	{ TYPE_0      , 0xB80    , 128 , FXLOG_C3 }, /* RID area info ULP#0B     */
	{ TYPE_0      , 0xC80    , 128 , FXLOG_C3 }, /* RID area info ULP#0C     */
	{ TYPE_0      , 0xD80    , 128 , FXLOG_C3 }, /* RID area info ULP#0D     */
	{ TYPE_0      , 0xE80    , 128 , FXLOG_C3 }, /* RID area info ULP#0E     */
	{ TYPE_0      , 0xF80    , 96  , FXLOG_C3 }, /* RID area info ULP#0F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #78(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0xFE0    , 32  , FXLOG_C3 }, /* RID area info ULP#0F     */
	{ TYPE_0      , 0x1080   , 128 , FXLOG_C3 }, /* RID area info ULP#10     */
	{ TYPE_0      , 0x1180   , 128 , FXLOG_C3 }, /* RID area info ULP#11     */
	{ TYPE_0      , 0x1280   , 128 , FXLOG_C3 }, /* RID area info ULP#12     */
	{ TYPE_0      , 0x1380   , 128 , FXLOG_C3 }, /* RID area info ULP#13     */
	{ TYPE_0      , 0x1480   , 128 , FXLOG_C3 }, /* RID area info ULP#14     */
	{ TYPE_0      , 0x1580   , 128 , FXLOG_C3 }, /* RID area info ULP#15     */
	{ TYPE_0      , 0x1680   , 128 , FXLOG_C3 }, /* RID area info ULP#16     */
	{ TYPE_0      , 0x1780   , 80  , FXLOG_C3 }, /* RID area info ULP#17     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #79(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x17D0   , 48  , FXLOG_C3 }, /* RID area info ULP#17     */
	{ TYPE_0      , 0x1880   , 128 , FXLOG_C3 }, /* RID area info ULP#18     */
	{ TYPE_0      , 0x1980   , 128 , FXLOG_C3 }, /* RID area info ULP#19     */
	{ TYPE_0      , 0x1A80   , 128 , FXLOG_C3 }, /* RID area info ULP#1A     */
	{ TYPE_0      , 0x1B80   , 128 , FXLOG_C3 }, /* RID area info ULP#1B     */
	{ TYPE_0      , 0x1C80   , 128 , FXLOG_C3 }, /* RID area info ULP#1C     */
	{ TYPE_0      , 0x1D80   , 128 , FXLOG_C3 }, /* RID area info ULP#1D     */
	{ TYPE_0      , 0x1E80   , 128 , FXLOG_C3 }, /* RID area info ULP#1E     */
	{ TYPE_0      , 0x1F80   , 64  , FXLOG_C3 }, /* RID area info ULP#1F     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #80(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x1FC0   , 64  , FXLOG_C3 }, /* RID area info ULP#1F     */
	{ TYPE_0      , 0x2000   , 256 , FXLOG_C3 }, /* LEVEL#0_WORK             */
	{ TYPE_0      , 0x2100   , 256 , FXLOG_C3 }, /* LEVEL#1A_WORK            */
	{ TYPE_0      , 0x2200   , 256 , FXLOG_C3 }, /* LEVEL#2A_WORK            */
	{ TYPE_0      , 0x2300   , 176 , FXLOG_C3 }, /* LEVEL#3_WORK             */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #81(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23B0   , 80  , FXLOG_C3 }, /* LEVEL#3_WORK             */
	{ TYPE_0      , 0x2400   , 256 , FXLOG_C3 }, /* LEVEL#1B_WORK            */
	{ TYPE_0      , 0x2500   , 256 , FXLOG_C3 }, /* LEVEL#2B_WORK            */
	{ TYPE_0      , 0x2600   , 256 , FXLOG_C3 }, /* LEVEL#1C_WORK            */
	{ TYPE_0      , 0x2700   , 160 , FXLOG_C3 }, /* LEVEL#2C_WORK            */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #82(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x23A0   , 96  , FXLOG_C3 }, /* LEVEL#2C_WORK            */
	{ TYPE_0      , 0x3000   , 912 , FXLOG_C3 }, /* WS_Common                */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #83(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x3390   , 112 , FXLOG_C3 }, /* WS_Common                */
	{ TYPE_TRCA   , 0x0      , 512 , FXLOG_C3 }, /* FRAME_A                  */
	{ TYPE_0      , 0x29000  , 384 , FXLOG_C3 }, /* SCSI Cancel (driver)     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #84(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x29180  , 640 , FXLOG_C3 }, /* SCSI Cancel (driver)     */
	{ TYPE_0      , 0x29400  , 368 , FXLOG_C3 }, /* SCSI Cancel (async)      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #85(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_0      , 0x29570  , 656 , FXLOG_C3 }, /* SCSI Cancel (async)      */
	{ TYPE_EVT0   , FX_ETRC  , 352 , FXLOG_C3 }, /* Event trace #897-907     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #86(core3) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C3 }, /* Event trace #908-938     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #87(core3) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C3 }, /* Event trace #939-969     */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #88(core3) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 992 , FXLOG_C3 }, /* Event trace #970-1000    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #89(core3) *******/
	{ TYPE_RSV    , 0x0      , 28  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_ETRC  , 768 , FXLOG_C3 }, /* Event trace #1001-1024   */
	{ TYPE_FRM0   , FX_FTRC  , 224 , FXLOG_C3 }, /* Frame trace - 14event    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #90(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 1008, FXLOG_C3 }, /* Frame trace - 63event    */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #91(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC0   , FX_FTRC  , 816 , FXLOG_C3 }, /* Frame trace - 51event    */
	{ TYPE_TRC9   , 0x2B000  , 192 , FXLOG_C3 }, /* LPAR trace ULP#0         */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #92(core3) *******/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_TRC9   , 0x2B0C0  , 320 , FXLOG_C3 }, /* LPAR trace ULP#0         */
	{ TYPE_TRC9   , 0x0      , 512 , FXLOG_C3 }, /* LPAR trace CURRENT_ULP   */
	{ TYPE_RSV    , 0x0      , 176 , FXLOG_AC }, /* RSV                      */
	{ TYPE_ERRID  , 0x0      , 4   , FXLOG_AC }, /********* #93 **************/
	{ TYPE_RSV    , 0x0      , 12  , FXLOG_AC }, /* RSV                      */
	{ TYPE_2      , 0x000    , 64  , FXLOG_AC }, /* CCR_CORE0                */
	{ TYPE_2      , 0x400    , 64  , FXLOG_AC }, /* CCR_CORE1                */
	{ TYPE_2      , 0x800    , 64  , FXLOG_AC }, /* CCR_CORE2                */
	{ TYPE_2      , 0xC00    , 64  , FXLOG_AC }, /* CCR_CORE3                */
	{ TYPE_RSV    , 0x0      , 752 , FXLOG_AC }, /* RSV                      */
	{ 0           , 0        , 0   , 0        }
};

/*============================================================================*/
/* NAME        : hfc_fx_save_hwlog_five_fx()
 * Aargument   : pp        ---- Address for struct port_info
 *             : core      ---- Address for struct core_info
 *             : hfc_errno ---- HFC Local Defined Error No
 *             : mode      ---- Logging Mode
 * Return      : NONE
 * Description : Save HW Log for FIVE-FX
 */
/*============================================================================*/
void hfc_fx_save_hwlog_five_fx(struct port_info *pp, struct core_info *core, uint hfc_errno, uchar mode)
{
	Type_mem_fx		*logmap		= NULL;
	uchar			 cnum		= 0;
	uint			 size_lmt	= 0;
	uint			 mck_code	= 0;
	uint			 errorid	= 0;
	uint			 unit_id	= 0;
	uint			 statusH	= 0;
	uint			 statusL	= 0;
	uint			 edetail	= 0;
	uint			 set		= 0;
	uint			 get		= 0;
	uint			*buf		= NULL;
	uint			 data		= 0;
	uint			 ram_adr	= 0;
	uint			 reg_adr	= 0;
	uint			 chkaddr1	= 0;
	uint			 chkaddr2	= 0;
	uint			 chksize1	= 0;
	uint			 chksize2	= 0;
	uint			 CurAddr	= 0;
	uint			 EntWord	= 0;
	uchar			 value		= 0;	/* FCLNX-GPL-FX-141 */
	uchar			 Shift		= 0;
	uchar			 type		= 0;
	uchar			 pcore		= 0;
	uint			 indirect_access = 0, get_flag = 1, i=0;
	uint64_t		 time		= 0;
	
	/*==========================================================================
	 *    Argument Check
	 *========================================================================*/
	 logmap  = (Type_mem_fx *) ffx_mck_log ;
	 
	if (!pp) {
		/**** Debug Print *****************************************************/
		HFC_DBGPRT("hfcldd%d hfc_fx_save_hwlog_five_fx SAVE_LOG Argument Check ---- Port_Info NULL\n", 
			pp->dev_minor);
		/**** Debug Print *****************************************************/
		return ;
	}
	/*==========================================================================
	 *    Check Format Define
	 *========================================================================*/
	if (!logmap) {
		/**** Debug Print *****************************************************/
		HFC_DBGPRT("hfcldd%d hfc_fx_save_hwlog_five_fx SAVE_LOG Format   Check ---- Format None\n", 
			pp->dev_minor);
		/**** Debug Print *****************************************************/
		return ;
	}
	/*==========================================================================
	 *    Check PORT Resouce
	 *========================================================================*/
	if (!(pp->hw_log)) {
		/**** Debug Print *****************************************************/
		HFC_DBGPRT("hfcldd%d hfc_fx_save_hwlog_five_fx SAVE_LOG Resouce  Check ---- Log Area None\n", 
			pp->dev_minor);
		/**** Debug Print *****************************************************/
		return ;
	}
	/*==========================================================================
	 *    Setup ErrorID and Log Size
	 *========================================================================*/
	switch (mode) {
	case HFC_ERRLOG_TYPE_IMLLOG:
	case HFC_ERRLOG_TYPE_IMLLOG_DIAG:
		errorid		= HFCFX_HL_ERRID_IMLFAIL;
		size_lmt	= HFCFX_HL_SIZE_IMLFAIL;
		break;
	case HFC_ERRLOG_TYPE_MCK:
		errorid		= HFCFX_HL_ERRID_MCK;
		size_lmt	= HFCFX_HL_SIZE_MCK;
		break;
	case HFC_ERRLOG_TYPE_CHKSTP:
		errorid		= HFCFX_HL_ERRID_CHKSTOP;
		size_lmt	= HFCFX_HL_SIZE_CHKSTOP;
#if 0
		if ( test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status) ) {
			chklog	= (HWLOG_DETAIL *)pp->hw_log;
			if (chklog->ErrorID != HFC_ERRLOG_TYPE_MCK) { break; }
			/*---- Re-Write ErrorID (Non-Save HW Log) -----------------------*/
			for (chk = 0; chk < (HFC_FX_HWLOG_SIZE/HFCFX_HL_SIZE_UNITB); chk++) {
				chklog->ErrorID = (uint)(errorid | chk);
				chklog++;
			}
			return;
		}
#endif
		break;
	default:
		errorid		= HFCFX_HL_ERRID_MCK;
		size_lmt	= HFCFX_HL_SIZE_MCK;
		break;
	}
	if (HFC_FX_HWLOG_SIZE < size_lmt) { size_lmt = HFC_FX_HWLOG_SIZE; }
	/**** Debug Print *********************************************************/
	HFC_DBGPRT("hfcldd%d SAVE_LOG Start Error ID ERROR:%08x SIZE:%08x jiffies = %08x\n",
				pp->dev_minor,
				errorid,
				size_lmt,
				(uint)jiffies);
	/**** Debug Print *********************************************************/
	
	while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
	{
		/* 1ms wait */
		mdelay(1); 

		if( i == 100 ){ /* 100msec */
			HFC_DBGPRT("hfcldd%d SAVE_LOG Does not Get enable indirect access flag.\n",pp->dev_minor);
			get_flag = 0;
			break;
		}
		i++;
	}
	
	if( get_flag == 1 ){
		HFC_DBGPRT("hfcldd%d SAVE_LOG Enable indirect access flag.\n",pp->dev_minor);
	
		/* Enable indirect access flag */
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x08);

		/*** flag check ***/ 
		i=0;
		while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
		{
			/* 1ms wait */
			mdelay(1); 

			if( i == 100 ){ /* 100msec */
				/* Clear Flag */
				hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00);	/* FCLNX-GPL-FX-162 */
				HFC_DBGPRT("hfcldd%d SAVE_LOG Clear enable indirect access flag.\n",pp->dev_minor);
				get_flag = 0;
				break;
			}
			i++;
		}
	}
	
	if( get_flag == 1 ){
		HFC_DBGPRT("hfcldd%d SAVE_LOG Get enable indirect access flag.\n",pp->dev_minor);
		indirect_access = 1;
	}

	/*==========================================================================
	 *    Setup MCK_CODE and Checker Type 
	 *========================================================================*/
	statusH = HFCFX_MMIO_R4(pp, PORT(STATUSH));
	statusL = HFCFX_MMIO_R4(pp, PORT(STATUSL));
	edetail = HFCFX_MMIO_R4(pp, PORT(EDETAIL));
	if (    (edetail & HFCFX_MMIO_EDTL_HMCKST1)
		&&(!(edetail & HFCFX_MMIO_EDTL_HBROADMCK)))
	{
		if (statusH & HFCFX_MMIO_STTH_COMMONMCK) {
			if (edetail & HFCFX_MMIO_EDTL_HPUMCKST) {
				/*==== Common Block "PU" MCK =================================*/
				mck_code	= HFCFX_HL_MCKCD_CMNPU;
				chkaddr1	= HFCFX_MMIO(SCAN(CKCMNPU));
				chksize1	= HFCFX_HL_SIZE_CKCMNPU;
			} else {
				/*==== Common Block Other MCK ================================*/
				mck_code = HFCFX_HL_MCKCD_CMN;
				chkaddr1	= HFCFX_MMIO(SCAN(CKCMNTU));
				chkaddr2	= HFCFX_MMIO(SCAN(CKCMNZU));
				chksize1	= HFCFX_HL_SIZE_CKCMNTU;
				chksize2	= HFCFX_HL_SIZE_CKCMNZU;
			}
		} else {
			/*==== PORT Block MCK ============================================*/
			mck_code	= HFCFX_HL_MCKCD_PORT(pp->port_no);
			chkaddr1	= HFCFX_MMIO(SCAN(CKPORT(pp->port_no)));
			chksize1	= HFCFX_HL_SIZE_CKPORT;
		}
	} else {
		/*==== CORE Block MCK ================================================*/
		for (HFC_LOOP_CORE(cnum, (struct port_info *)pp)) {
			statusH = HFCFX_MMIO_R4(pp, CORE(STATUSH(cnum)));
			statusL = HFCFX_MMIO_R4(pp, CORE(STATUSL(cnum)));
			edetail = HFCFX_MMIO_R4(pp, CORE(EDETAIL(cnum)));
			core = HFC_GET_CORE_ADDR(pp, cnum);
			if (!core)									{ continue; }
			if ( hfc_fx_check_cs_disable(pp, core ) )	{ continue; }
			if (statusH & HFCFX_MMIO_STTH_HCHKSTOP)		{ continue; }
			if (edetail & HFCFX_MMIO_EDTL_HBROADMCK)	{ continue; }
			if (edetail & HFCFX_MMIO_EDTL_HMCKST0) {
				break;
			}
		}
		if (core) {
			mck_code = HFCFX_HL_MCKCD_CORE(core->pcore_no);
			chkaddr1 = HFCFX_MMIO(SCAN(CKCORE(core->pcore_no)));
			chksize1 = HFCFX_HL_SIZE_CKCORE;
		} else {
			mck_code = HFCFX_HL_MCKCD_CORE(0);
			chkaddr1 = HFCFX_MMIO(SCAN(CKCORE(0)));
			chksize1 = HFCFX_HL_SIZE_CKCORE;
		}
	}
	/**** Debug Print *********************************************************/
//	HFC_DBGPRT("hfcldd%d hfc_fx_save_hwlog_five_fx SAVE_LOG Setup MCK CODE MCKCODE:%08x CHK1[ADDR:%08x SIZE:%08x] CHK2[ADDR:%08x SIZE:%08x]\n",
//				pp->dev_minor,
//				mck_code,
//				chkaddr1,
//				chksize1,
//				chkaddr2,
//				chksize2);
	/**** Debug Print *********************************************************/
	/*==========================================================================
	 *    Save H/W Log
	 *========================================================================*/
	buf		= pp->hw_log;
	set		= 0;
	get		= 0;
	while (logmap->type) {
		if (!(set < size_lmt)) {
			/**** Debug Print *************************************************/
			HFC_DBGPRT("hfcldd%d SAVE_LOG Check Log Size OFFSET:%08x LMT:%08x\n",
						pp->dev_minor,
						set,
						size_lmt);
			/**** Debug Print *************************************************/
			break;
		}
		/*======================================================================
		 *    Check Core Installed
		 *====================================================================*/
		if (logmap->func_core & FXLOG_FUNC(pp->pkg.port, pp->port_no)) {
			type  = logmap->type;
			pcore = FXLOG_PCORE(logmap->func_core);
		} else {
			type = HLOG_UICORE;
		}
		/**** Debug Print *****************************************************/
//		HFC_DBGPRT("hfcldd%d SAVE_LOG get Log Format TYP:%02x ADDR:%08x SIZE:%08x CORE:%08x\n",
//					pp->dev_minor,
//					logmap->type,
//					logmap->reg_adr,
//					logmap->size,
//					logmap->func_core);
		/**** Debug Print *****************************************************/
		switch (type) {
		case HLOG_ERRID:
			/*==== Error ID ==================================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				data	= (uint)(errorid | unit_id);
				HFC_4L_TO_4B(buf[set], data);
			}
			unit_id++;
			break;
		case HLOG_RSV:
			/*==== Reserved Location =========================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				data	= HFCFX_HL_DATA_RESEVED;
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_UICORE:
			/*==== Unget Field(Uninstalled Core Resouce) =====================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				data	= HFCFX_HL_DATA_CORES;
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_CFG:
			/*==== PCI Config Space ==========================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				reg_adr	= logmap->reg_adr + get;
				data	= HFCFX_PCI_R4(pp, OFF(reg_adr));
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_PCI:
			/*==== PCI Memory ================================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				reg_adr	= logmap->reg_adr + get;
				data	= HFCFX_MMIO_R4(pp, OFF(reg_adr));
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_CHK:
			/*==== Checker Area(PCI Memory) ==================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				if ((chkaddr1) && (get < chksize1)) {
					reg_adr	= chkaddr1 + get;
					data	= HFCFX_MMIO_R4(pp, OFF(reg_adr));
				}
				else if ((chkaddr2) && (get < (chksize1 + chksize2))) {
					reg_adr	= chkaddr2 + (get - chksize1);
					data	= HFCFX_MMIO_R4(pp, OFF(reg_adr));
				}
				else {
					reg_adr	= 0;
					data	= HFCFX_HL_DATA_RESEVED;
				}
				HFC_4L_TO_4B(buf[set], data);
			}
			HFC_4L_TO_4B(buf[(set - 1)], mck_code);
			break;
		case HLOG_WS:
			/*==== RAM Indirect Access WS ====================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(WS(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_WSULP:
			/*==== RAM Indirect Access WS(ULP# Shift) ========================*/
			Shift = HFCFX_MMIO_R1(pp, SCAN(ULP(pcore)));
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(WS(ULP(OFF(logmap->reg_adr+get), Shift)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_WSLEVEL:
			/*==== RAM Indirect Access WS(LEVEL# Shift) ======================*/
			/* FCLNX-GPL-FX-141 */
			value = HFCFX_MMIO_R1(pp, SCAN(CORE(ELV(pcore))));
			if (value) {
				Shift = (uchar)hfc_getBitPow8(value);
			} else {
				Shift = (uchar)hfc_getBitPow8(HFCFX_MMIO_ELV_LEVEL3);
			}
			/* FCLNX-GPL-FX-141 */
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(WS(LEV(OFF(logmap->reg_adr+get), Shift)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_WSCMN:
			/*==== RAM Indirect Access WS(Common Area Shift) =================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(WS(CMN(OFF(logmap->reg_adr + get))));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_GRS:
			/*==== RAM Indirect Access GRS ===================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(GRS(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_LS:
			/*==== RAM Indirect Access LS ====================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(LS(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_LSULPF:
			Shift = HFCFX_MMIO_R1(pp, SCAN(ULP(pcore)));
			/*==== RAM Indirect Access LS (ULP# Shift for FRAME) =============*/
			for (get = 0; get < logmap->size; get += 4) {
				ram_adr	= HFCFX_RADR(LS(ULPF(OFF(logmap->reg_adr+get), Shift)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				set++;
			}
			break;
		case HLOG_LSULPL:
			Shift = HFCFX_MMIO_R1(pp, SCAN(ULP(pcore)));
			/*==== RAM Indirect Access LS (ULP# Shift for LPAR) ==============*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(LS(ULPL(OFF(logmap->reg_adr+get), Shift)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_CCR:
			/*==== RAM Indirect Access CCR ===================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(CCR(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_2, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_ZCTR:
			/*==== RAM Indirect Access ZCTR ==================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(ZCTR(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_1, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_ZKTR:
			/*==== RAM Indirect Access ZKTR ==================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(ZKTR(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_3, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_BCR:
			/*==== RAM Indirect Access BCR ===================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(BCR(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_FLASH:
			/*==== RAM Indirect Access FLASH =================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(FLS(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_2, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_IDREG:
			/*==== RAM Indirect Access IDREG =================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(ID(OFF(logmap->reg_adr + get)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_2, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		case HLOG_FRMTR:
			/*==== FRAME Trace(on LS Area) ===================================*/
			EntWord	= logmap->reg_adr;
			CurAddr	= (uint)HFCFX_MMIO_R2(pp, SCAN(FRIPFROP(pcore)));
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(FRMTR(LA(HFCFX_RAMI(FRMTR(CR(CurAddr))),
					                          EntWord)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				EntWord++;
			}
			break;
		case HLOG_FWEVTR:
			/*==== FW Event Trace(on LS Area) ================================*/
			EntWord	= logmap->reg_adr;
			ram_adr	= HFCFX_RADR(WS(CMN(ETRCCA)));			
			CurAddr	= (uint)hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
			if (  (CurAddr == HFCFX_HL_DATA_RAMIE1)
				||(CurAddr == HFCFX_HL_DATA_RAMIE2))
			{
				ram_adr	= HFCFX_HL_DATA_UNDEF;
				data	= CurAddr;
				for (HFC_LOOP_HLOG(logmap, get, set)) {
					HFC_4L_TO_4B(buf[set], data);
				}
				break;
			}
			CurAddr &= 0x00FFFFFF;
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(FWEVTR(LA(HFCFX_RAMI(FWEVTR(CR(CurAddr))),
					                           EntWord)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				EntWord++;
			}
			break;
		case HLOG_FWFRTR:
			/*==== FW Frame Trace(on LS Area) ================================*/
			EntWord	= logmap->reg_adr;
			ram_adr	= HFCFX_RADR(WS(CMN(FTRCCA)));			
			CurAddr	= (uint)hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
			if (  (CurAddr == HFCFX_HL_DATA_RAMIE1)
				||(CurAddr == HFCFX_HL_DATA_RAMIE2))
			{
				ram_adr	= HFCFX_HL_DATA_UNDEF;
				data	= CurAddr;
				for (HFC_LOOP_HLOG(logmap, get, set)) {
					HFC_4L_TO_4B(buf[set], data);
				}
				break;
			}
			CurAddr &= 0x00FFFFFF;
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(FWFRTR(LA(HFCFX_RAMI(FWFRTR(CR(CurAddr))),
					                           EntWord)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_0, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				EntWord++;
			}
			break;
		case HLOG_FWBRTR:
			/*==== FW Branch Trace(on ZCTR Area) =============================*/
			EntWord	= logmap->reg_adr;
			CurAddr	= (uint)HFCFX_MMIO_R2(pp, SCAN(CORE(TRCCA(pcore))));
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(FWBRTR(LA(HFCFX_RAMI(FWBRTR(CR(CurAddr))),
					                           EntWord)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_1, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				EntWord++;
			}
			break;
		case HLOG_PCIPTR:
			/*==== PCIe Packet Trace(on ZKTR) ================================*/
			EntWord	= logmap->reg_adr;
			CurAddr	= (uint)HFCFX_MMIO_R2(pp, CMMN(TRCCA));					/* FCLNX-GPL-FX-141 */
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr	= HFCFX_RADR(PCIPTR(LA(HFCFX_RAMI(PCIPTR(CR(CurAddr))),
					                           EntWord)));
				data	= hfc_fx_read_ramid(pp, ram_adr, TYPE_3, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				EntWord++;
			}
			break;
		case HLOG_RUTR:
			/*==== RU Trace(on ZKTR) =========================================*/
			EntWord	= logmap->reg_adr;
			CurAddr	= (uint)HFCFX_MMIO_R2(pp, CMMN(RUTRCCA));				/* FCLNX-GPL-FX-141 */
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				ram_adr = HFCFX_RADR(RUTR(LA(HFCFX_RAMI(RUTR(CR(CurAddr))),
					                         EntWord)));
				data = hfc_fx_read_ramid(pp, ram_adr, TYPE_3, pcore, indirect_access);
				HFC_4L_TO_4B(buf[set], data);
				EntWord++;
			}
			break;
		case HLOG_TIMESTAMP:
			/*==== Time Stamp ==================================================*/
			for (HFC_LOOP_HLOG(logmap, get, set)) {
				if(get==0){
					time = (uint64_t)jiffies;
					data = (uint)((time >> 32)  & 0x00000000ffffffff);
				}else{
					data = (uint)(time & 0x00000000ffffffff);
				}
				HFC_4L_TO_4B(buf[set], data);
			}
			break;
		default:
			break;
		}
		logmap++;
	}
	
	/**** Finalize process of indirect access ****//* FCLNX-GPL-FX-162 */
	if( indirect_access == 1) {
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,  0x01, 0x00);	
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,  0x04, 0x80000000);	
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00); /* Clear indirect access flag */
	}
	
	/**** Debug Print *********************************************************/
	HFC_DBGPRT("hfcldd%d SAVE_LOG:End of Log     jiffies = %08x\n", pp->dev_minor, (uint)jiffies);
	/**** Debug Print *********************************************************/
	return ;
}
/*==== End of Function "hfc_fx_save_hwlog_five_fx()" =========================*/


/*
 * Function:    hfc_fx_save_pcie_sram_log
 *
 * Purpose:     FIVE-EX SRAM 1bit CE Log out
 *
 * Arguments:   
 *  pp         - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_save_pcie_sram_log(struct port_info *pp) 
{
	struct hfc_err1bit_fx 	*ptr = NULL;
	uint				indirect_access=0, reg_adr, read_pos, log_offset=0;
	uint				i, wk4=0, get_flag = 1;

	struct ram_adr_t {
		uint size;
		uint ram_adr;
	} ram_adr_tbl[] = {
		{224, 0x0d000000}, /* SRAM ECC Err Status Register0 */
		{48,  0x0d000100}, /* SRAM ECC Err Status Register1 */
		{48,  0x0d000140}, /* SRAM ECC Err Status Register2 */
		{160, 0x0d000200}, /* SRAM ECC Err Status Register3 */
		{   0,          0}  /* stopper     */
	};

	if (pp->pcie_sram_ce_cnt >= HFC_FX_1BIT_LOG_ENTRY)
		return;

	switch( pp->pcie_sram_ce_cnt ){
		case 0 :
			ptr = (struct hfc_err1bit_fx *) &pp->ce_log[0].pcie_sram_data[0];
			break;
		case 1 :
			ptr = (struct hfc_err1bit_fx *) &pp->ce_log[0].pcie_sram_data[1];
			break;
		case 2 :
			ptr = (struct hfc_err1bit_fx *) &pp->ce_log[1].pcie_sram_data[0];
			break;
		case 3 :
			ptr = (struct hfc_err1bit_fx *) &pp->ce_log[1].pcie_sram_data[1];
			break;
		case 4 :
			ptr = (struct hfc_err1bit_fx *) &pp->ce_log[2].pcie_sram_data[0];
			break;
		case 5 :
			ptr = (struct hfc_err1bit_fx *) &pp->ce_log[2].pcie_sram_data[1];
			break;
	}
	
	/* Set indirect access flag */
//	HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_sram_log\n",pp->dev_minor);
	i=0;
	while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
	{
		/* 1ms wait */
		mdelay(1); 

		if( i == 100 ){ /* 100msec */
			HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_sram_log Does not Get enable indirect access flag.\n",pp->dev_minor);
			get_flag = 0;
			break;
		}
		i++;
	}
	
	if( get_flag == 1 ){
		HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_sram_log Enable indirect access flag.\n",pp->dev_minor);
	
		/* Enable indirect access flag */
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x08);

		/*** flag check ***/ 
		i=0;
		while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
		{
			/* 1ms wait */
			mdelay(1); 

			if( i == 100 ){ /* 100msec */
				/* Clear Flag */
				hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00);	/* FCLNX-GPL-FX-162 */
				HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_sram_log Clear enable indirect access flag.\n",pp->dev_minor);
				get_flag = 0;
				break;
			}
			i++;
		}
	}
	
	if( get_flag == 1 ){
		HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_sram_log Get enable indirect access flag.\n",pp->dev_minor);
		indirect_access = 1;
	}

#if 0
	if( !(hfc_fx_read_reg(pp, HFC_IOSPACE_RAMADR, (char)0x1) & 0x80) ) {
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x08);		
		if( hfc_fx_read_reg(pp, HFC_IOSPACE_RAMADR, (char)0x1) & 0x80 ) {
			/* Clear Flag */
			hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00);		/* FCLNX-GPL-FX-162 */
		}else{
			/* Indirect Access Admitted */
			indirect_access = 1;
		}
	}
#endif
	
	if( indirect_access == 1 ){
		/* Clear Upstream Bridge Status Register */
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK, (char)0x1, 0x20);
		log_offset = 0;
		
		for(i=0; i<4; i++){
//			HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_log i = %d size = %08x\n",pp->dev_minor, i, (uint)ram_adr_tbl[i].size);
			for(reg_adr=0 ; reg_adr<ram_adr_tbl[i].size ; reg_adr+=4) {
				read_pos = ram_adr_tbl[i].ram_adr+reg_adr;
//				HFC_DBGPRT("hfcldd%d hfc_fx_save_pcie_log read_pos = %08x\n",pp->dev_minor, read_pos);
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR, (char)0x4, (uint)read_pos);
				wk4 = (uint)hfc_fx_read_reg(pp, HFC_IOSPACE_INDAREA, (char)0x4 );
				HFC_4L_TO_4B(ptr->pcie_sram_data[log_offset], wk4);
				log_offset++ ;
			}
		}
		
		/**** Finalize process of indirect access ****/
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,  0x01, 0x00);	
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,  0x04, 0x80000000);
		/* Clear indirect access flag */	
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00); 
	}

	return;
}

/*
 * Function:    hfc_fx_abend
 *
 * Purpose:     Abend processing
 *
 * Arguments:   
 *  pp         - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_abend(
	struct port_info        *pp,
	struct core_info		*core,
	uchar                   type)
{
	struct port_info		*vpp;
	struct region_info		*rp;
	uint					dump_reg[4] ;
	uint					err_no=0 ;
	uint					adap_status=0,i, addr=0;	/* FCLNX-GPL-FX-077 */
	uchar					wk=0;						/* FCLNX-GPL-FX-077 */
	uchar					wk_type;					/* FCLNX-GPL-FX-334 */

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//	struct target_info_fx  	*target = NULL;
//	int 					rsp;
//	uint 					lp;
//	struct dev_info_fx	 	*dev=NULL;
#endif

	HFC_DBGPRT(" hfcldd%d : hfc_fx_abend - start type = %x \n", pp->dev_minor, type);

	hfc_fx_hand2_trace(
		HFC_FX_TRC_ABEND, type, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);
	switch( type )
	{
		case HFC_ABEND_XRB_INVALID:
		case HFC_ABEND_CCC:
		case HFC_ABEND_MB_RSPERR:
			/* (1) In the mailbox response, xcc=83 or fsb=CCC    */
			/* (2) In the SCSI response, xcc=83 or fsb=CCC       */
			/* (3) In the SCSI response, xrb Valid=0             */
			/*==> Process <==                                    */
			/* Count over (pp->link_dead_cnt >HFC_LINK_DEAD_CNT) */
			/*   -> Execute force check stop                     */
			/* Otherwise                                         */
			/*   -> Return (Counter Up)                          */

			pp->link_dead_cnt++ ;
			if( pp->link_dead_cnt > HFC_LINK_DEAD_CNT )
			{
				HFC_FX_ISSUE_FMCK(pp, core, type);									/* FCLNX-0279 */
			}
			break ;
		case HFC_ABEND_SERR:
		case HFC_ABEND_PERR:
		case HFC_ABEND_SPERR:
			/* (1) PCI ERROR INT                                 */
			/* Issue Force Machine Check                         */
			
			/* Get Log for MLPF */
			if( HFC_FX_MMODE_CHECK_SHARED(pp) && !( HFC_FX_MMODE_CHECK_SHADOW(pp) ) ) /* @MLPF */
				hfc_fx_mlpf_pci_error(pp, type);
			
			/* Get Log */
			switch(type){
				case HFC_ABEND_SERR:
					adap_status = SCS_PCI_BUS_SERR;
					err_no = 0x00000032;
					break;
				case HFC_ABEND_PERR:
					adap_status = SCS_PCI_BUS_PERR;
					err_no = 0x00000033;
					break;
				case HFC_ABEND_SPERR:
					adap_status = SCS_PCI_BUS_SPERR;
					err_no = 0x00000034;
					break;
			}
			
			dump_reg[0] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INTA, (char)0x4) ;
			dump_reg[1] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
			dump_reg[2] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
			dump_reg[3] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
			
/*			hfc_fx_logout(pp,err_no,HFC_ERRLOG_TYPE_MCK); */ /* FCLNX-GPL-111 */
			HFC_MEMCPY(pp->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
			hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_ERRD,err_no,pp->logdata,16) ;	/* FCLNX-GPL-391 */

 			/* For hsdldd */
			if (hfc_manage_info.npubp->hfc_fx_mp_hsd_enable)						/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_fx_make_fcinfo(pp, NULL, err_no, adap_status, NULL, 0);
			
			/* Issue Force-MCK */
			HFC_FX_ISSUE_FMCK(pp, core, type);										/* FCLNX-0279 */
			
			break ;
			
		case HFC_ABEND_MCK_INT:
		case HFC_ABEND_MPCHK:
		case HFC_ABEND_T3:
		case HFC_ABEND_T3_NO_MCKINT:
			/* (1) MCK INT										*/
			/*==> Processing <==								*/
			/* Machine check recovery and increment counter 	*/
			/* Cancel pending SCSI commands under this adapter	*/
			
			if (HFC_FX_VIRTUAL_PORT(pp))
				break;
			
			wk_type = type;		/* FCLNX-GPL-FX-311,334 */
			
			/* Link Reset */
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {	/* FCLNX-GPL-FX-077 */
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						addr = 0x326 + 0x80*i;
						wk = hfc_fx_read_reg_ext(pp, ( uint )addr, ( char )0x1 );
						if(wk  == HFC_ABEND_LINK_RESET){
							type = HFC_ABEND_LINK_RESET;
							break;
						}
					}
				}
			}																			/* FCLNX-GPL-FX-077 */
			
			/* Stop optical transmission */
			if(!(HFC_FX_MMODE_CHECK_SHARED(pp))){
				hfc_fx_write_reg(pp,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
			}
			else{
				hfc_fx_mlpf_set_fcif(pp, 0x80808080);
			}
			
			/* Send TSEQ */
			hfc_fx_reset_start(pp, HFC_TSEQ);
			
			rp = pp->region_arg[0];
			if (wk_type != HFC_ABEND_T3_NO_MCKINT) {	/* FCLNX-GPL-FX-311,334 */
				HFC_ALLCORELOCK(rp);
			}

			/* Cancel pending SCSI commands, and stop timers. */
			hfc_fx_mck_prepare(pp, type);

			if (wk_type != HFC_ABEND_T3_NO_MCKINT) {	/* FCLNX-GPL-FX-311,334 */
				HFC_ALLCOREUNLOCK(rp);
			}
			
			for (i=1; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				rp = pp->region_arg[vpp->rid];
				if (rp == NULL)
					continue;
				
				if ((wk_type == HFC_ABEND_T3_NO_MCKINT) && (vpp->rid == 0)) {	/* FCLNX-GPL-FX-231 *//* FCLNX-GPL-FX-311,334 */
					/* already core_lock */
					HFC_DBGPRT(" hfcldd%d : hfc_fx_abend - already core_lock(rid=0) \n", pp->dev_minor);
				}
				else {
					HFC_ALLCORELOCK(rp);
				}	/* FCLNX-GPL-FX-231 */
				
				/* Cancel pending SCSI commands, and stop timers. */
				hfc_fx_mck_prepare(vpp, type);
				if ((wk_type == HFC_ABEND_T3_NO_MCKINT) && (vpp->rid == 0)) {	/* FCLNX-GPL-FX-231 *//* FCLNX-GPL-FX-311,334 */
					/* no need core_unlock */
					HFC_DBGPRT(" hfcldd%d : hfc_fx_abend - no need core_unlock(rid=0) \n", pp->dev_minor);
				}
				else {
					HFC_ALLCOREUNLOCK(rp);
				}	/* FCLNX-GPL-FX-231 */
			}
			
			/* Close INT_A mask */
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						if( HFC_FX_MMODE_CHECK_SHARED(pp) ){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  ( char )0x4, (int)HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
						}else{
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  ( char )0x4, 0x00000000, HFC_FX_CORE_OFFSET10);
						}
					}
				}
			}
			
			/* Machine check recovery and increment counter */
			if( hfc_fx_pcibus_chk(pp) != 0 )
			{	/* "PCI BUS ERR" has hpppen. */
				HFC_FX_ISSUE_CSTP_PCIERR(pp);
			}
			else
			{
				hfc_fx_mck_recovery(pp, type);
			}
			
			if (hfc_manage_info.npubp->hfc_fx_mp_hsd_enable)
				hfc_manage_info.npubp->hfc_fx_make_fcinfo(pp, NULL, err_no, SCS_MCK, NULL, 0);
			
			break ;
			
		case HFC_ABEND_MB_TOUT:
		case HFC_ABEND_TOUTCHK_XOB:
		case HFC_ABEND_RID_INVALID:
		case HFC_ABEND_HFCPKT_CHK:											/* FCLNX-GPL-0135 */
		case HFC_ABEND_PIC_ERROR:											/* FCLNX-GPL-0576 */
		case HFC_ABEND_LINK_RESET:
			/* (1) Mail Box timeout */
			/*==> Process <==                                    */
			/*   -> Execute force check stop                     */
			if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
			{	/* "PCI BUS ERR" has hpppen. */
				HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
			}
			else
			{
				HFC_FX_ISSUE_FMCK(pp, core, type);							/* FCLNX-0279 */
			}
			break ;
			
		case HFC_ABEND_SRAM_CE:
			hfc_fx_pcie_sram_ce_recovery(pp);
			break;
		default:
			HFC_FX_ISSUE_FMCK(pp, core, type);										/* FCLNX-0279 */
			break ;
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_ABEND, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);

	return;
}


/*
 * Function:    hfc_fx_pcibus_chk
 *
 * Purpose:     Check the status of PCI bus
 *
 * Arguments:   
 *  pp         -
 *
 * Returns:     
 *  0          - OK
 *  -1         - NG
 *
 * Notes:       
 */
int hfc_fx_pcibus_chk(struct port_info *pp)
{

	uint			read_reg = 0 ;

	HFC_DBGPRT(" hfcldd : hfc_fx_pcibus_chk - start\n");

//	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) )	/* FCLNX-GPL-550 */
//		return(-1);

	read_reg = hfc_fx_read_reg_ext( pp,(uint)0, (char)0x4) ;
	
	if( read_reg == 0xffffffff )
	{
		hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_EVNT4,
													0x5c,NULL,0) ;
		return(-1) ;
	}

	return(0) ;
}


/*
 * Function:    hfc_fx_mck_recovery_five_fx
 *
 * Purpose:     Machine check recovery(for Five-FX)
 *
 * Arguments:   
 *  pp         - Pointer to dpp_info
 *  type       - error type
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mck_recovery_five_fx(                                                         /* FCWIN-0220 */
	struct port_info            *pp,
	uchar                        type)
{
	struct core_info		*core=NULL;
	struct port_info		*vpp;
	uint                    dump_reg[4], wk4 ;
	uint                    status0;
	uint                    err_no, i;
	int						rtn=1, normal_core=0;
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_MCKREC, 0x20, pp, NULL, NULL, NULL, NULL,
		0,(uint64_t)type,0);
	
	pp->mck_type = type;
	
	for (i=0; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		set_bit( HFC_PS_MCK_RECOVERY, (ulong *)&vpp->status );
	}
	
	hfc_fx_w_stop(pp, core, HFC_FX_MCKINT_TMR) ;


	if( test_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 ) )
	{
		/*--- DIAG Mode ---*/
		set_bit(HFC_MCK_HW_INIT, (ulong *)&pp->mck_result );
		set_bit(HFC_PS_ISOL, (ulong *)&pp->status );
		clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
		clear_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );				/* FCLNX-GPL-0517 */
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		
		/* Set the pointer of core_info that occured MCK	*/
		if( pp->region_arg[pp->rid] != NULL ){
			if( pp->region_arg[pp->rid]->core_arg[pp->mck_core_no] != NULL ){
				core = pp->region_arg[pp->rid]->core_arg[pp->mck_core_no];
			}
		}
		
		hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);					/* FCLNX-0296 */
		clear_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
		
		/* Log out */ /* FCLNX-GPL-111 */
		dump_reg[0] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INTA, (char)0x4) ;
		dump_reg[1] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
		dump_reg[2] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
		dump_reg[3] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
		
		HFC_MEMCPY(pp->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
		hfc_fx_save_hwlog_five_fx(pp, core, 0x36, HFC_ERRLOG_TYPE_IMLLOG);
		hfc_fx_errlog(
			pp,NULL,NULL,NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF,0x36,pp->logdata,16) ;	/* FCLNX-GPL-391 */

		return ;
	}

	if( test_bit( HFC_PS_DIAG, (ulong *)&pp->status ) )
	{
		/*--- DIAG Mode ---*/
		set_bit(HFC_MCK_HW_INIT, (ulong *)&pp->mck_result );
		set_bit(HFC_PS_ISOL, (ulong *)&pp->status );
		clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
		clear_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );				/* FCLNX-GPL-0517 */
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		
		/* Set the pointer of core_info that occured MCK	*/
		if( pp->region_arg[pp->rid] != NULL ){
			if( pp->region_arg[pp->rid]->core_arg[pp->mck_core_no] != NULL ){
				core = pp->region_arg[pp->rid]->core_arg[pp->mck_core_no];
			}
		}
		
		/* Wake up proccess */
		hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
		
		/* Log out */ /* FCLNX-GPL-111 */
		dump_reg[0] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INTA, (char)0x4) ;
		dump_reg[1] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
		dump_reg[2] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
		dump_reg[3] = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
		
		HFC_MEMCPY(pp->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
		hfc_fx_save_hwlog_five_fx(pp, core, 0x36, HFC_ERRLOG_TYPE_IMLLOG);
		hfc_fx_errlog(
			pp,NULL,NULL,NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF,0x36,pp->logdata,16) ;	/* FCLNX-GPL-391 */

		return ;
	}

	if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2))
	{
		set_bit(HFC_MCK_HWCHKSTOP, (ulong *)&pp->mck_result );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		clear_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );				/* FCLNX-GPL-0517 */
		
		/* Wake up proccess */
		for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
			core = pp->region_arg[pp->rid]->core_arg[i];
			if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) )
			{
				hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
			}
		}
		
		if(test_bit(HFC_PS_DIAG, (ulong *)&pp->status )) 
		{
			hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
		}
		
		if( (test_bit( HFC_PD_WAIT_LINK_INI, (ulong *)&pp->status_detail1)) || (pp->initialize == 1) )
		{
			unlock_fx_mailbox( pp ) ; 
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
			clear_bit( HFC_PD_WAIT_LINK_INI, (ulong *)&pp->status_detail1);		
		}
		
		return ;
	}
	
	/* FCLNX-GPL-FX-146 */
	/*==========================================================================
	 * Suspend MCK logout and recovery until flash update command completion 
	 *=========================================================================*/
	if( !HFC_FX_MMODE_CHECK_SHARED(pp)){
		if ( test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) ) {
			hfc_fx_hand2_trace( HFC_FX_TRC_MCKREC, 0x30, pp, NULL, NULL, NULL, NULL,
				0,(uint64_t)type,0);
			
			memset((void *)pp->logdata, 0, 16);
			
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT4, (uint)0xf2, pp->logdata, 16) ;
			return;
		}
	}else{
		if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_HYPSTATUS_LOCKED_RAMACC, HFC_CHECK_HYPER_STATE)){
			hfc_fx_hand2_trace( HFC_FX_TRC_MCKREC, 0x31, pp, NULL, NULL, NULL, NULL,
				0,(uint64_t)type,0);
			
			set_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);		/* FCLNX-GPL-FX-414 */
			
			memset((void *)pp->logdata, 0, 16);
			
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT4, (uint)0xf2, pp->logdata, 16) ;
			return;
		}
	}
	
	wk4 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[pp->mck_core_no], 0x4 ) ;
	HFC_4L_TO_4B(dump_reg[0],wk4);
	wk4 = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[pp->mck_core_no]+0x8, 0x4 ) ;
	HFC_4L_TO_4B(dump_reg[1],wk4);
	wk4 = (uint) hfc_fx_read_reg_core(pp, pp->mck_core_no, HFC_IOSPACE_CA_MPCHK_CODE,
			0x04, HFC_FX_CORE_OFFSET80);
	HFC_4L_TO_4B(dump_reg[2],wk4);	
	wk4 = (uint)hfc_fx_read_reg_ext(pp, (uint) (0x326 + 0x80 * pp->mck_core_no ), 0x1);
	HFC_4L_TO_4B(dump_reg[3],wk4);
	
	if( type == HFC_ABEND_MPCHK )
		err_no = 0x0000002c ;
	else if( ( type == HFC_ABEND_T3 ) || ( type == HFC_ABEND_T3_NO_MCKINT ) )
		err_no = 0x0000002d ;
	else if( type == HFC_ABEND_LINK_RESET )
		err_no = 0x0000002e ;
	else
		err_no = 0x0000002b ;
	
	/* Set the pointer of core_info that occured MCK	*/
	if( pp->region_arg[pp->rid] != NULL ){
		if( pp->region_arg[pp->rid]->core_arg[pp->mck_core_no] != NULL ){
			core = pp->region_arg[pp->rid]->core_arg[pp->mck_core_no];
		}
	}
	
	HFC_MEMCPY(pp->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
	
	if( pp->mck_progress == HFC_MCK_PROGRESS )
	{
		/* MCK occured during MCK, or MCK recovery is in progress sub port. */
		HFC_FX_ISSUE_CSTP( pp , HFC_ABEND_FCSTP ) ;				/* FCLNX-0276 *//* @MLPF *//* FCLNX-GPL-316 */
		pp->mck_progress = 0;									/* FCLNX-GPL-0517 */
		return;
	}
	
	status0 = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;

	if( (pp->max_mck_cnt) && (pp->mck_err_cnt >= pp->max_mck_cnt) )	/* FCLNX-GPL-057 */
	{
		/* MCK occured eight times or more */

		HFC_FX_ISSUE_CSTP( pp, HFC_ABEND_FCSTP ) ;					/* FCLNX-0276 *//* FCLNX-GPL-316 */
		set_bit(HFC_MCK_CNT_OVER, (ulong *)&pp->mck_result );
		
		/* FCLNX-GPL-209 */
		
		return;
	}
	
	/* Get HW ERR Log *//* FCLNX-GPL-FX-089 */
	hfc_fx_save_hwlog_five_fx(pp, core, err_no, HFC_ERRLOG_TYPE_MCK);

	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if(pp->region_arg[pp->rid] != NULL){
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
	
			if( !hfc_fx_check_cs_disable(pp, core) ){
				if ((pp->max_mck_cnt) && (core->mck_err_cnt >= pp->max_mck_cnt)) {
					HFC_DBGPRT("hfcldd : hfc_fx_mck_recovery_fx - core check_stop core# = %d\n",core->core_no);
					// Core FORCE-CHKSTOP command & Logout
					rtn = hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
				}
				else {
					normal_core++;
				}
			}
			
			/* Enable interrupt factors */
			hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
				(char)0x4, ( int )0xffffffff, HFC_FX_CORE_OFFSET10);
		}
	}

	if( normal_core ){
		/* Get Dump Log */
		if( type == HFC_ABEND_MCK_INT )
		{
			hfc_fx_errlog(
				pp, core, NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR2,
				err_no,pp->logdata,16) ;	/* FCLNX-GPL-391 */
		}
		else if( type == HFC_ABEND_LINK_RESET){
			hfc_fx_errlog(
				pp, core ,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT2,
				err_no,pp->logdata,16) ;	/* FCLNX-GPL-391 *//* FCLNX-GPL-FX-089 *//* FCLNX-GPL-FX-153 */
		}
		else{
			hfc_fx_errlog(
				pp, core, NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR4,
				err_no,pp->logdata,16) ;	/* FCLNX-GPL-391 */
		}
	}
	else{
		/* All cores are isolated */
		HFC_FX_ISSUE_CSTP( pp, HFC_ABEND_FCSTP ) ;
		return;
	}
	
	/* Clear Registers for FIVE-FX */
	if(pp->pkg.type == HFC_PKTYPE_FIVE_FX){ /* Check pkg.type */
		/* Clear Config Register Sticky bit */
		hfc_fx_clear_sticky_bit(pp);
	}
	
	/* Set ERPTYP internal loop state */
	hfc_fx_write_reg(pp, HFC_IOSPACE_PTYP0, 0x1, 0x04);
	
	/* Reset UTL Register */								/* FCLNX-GPL-FX-079 */
	hfc_fx_reset_start(pp, HFC_UTL_REG_CLEAR);				/* FCLNX-GPL-FX-079 */
	
	/* Recovery Reset (CTLRES) */
	hfc_fx_reset_start(pp, HFC_CTLRST);
	
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_CTLRST_DELAY_TMR, 0, TRUE);	/* FCLNX-0276 */
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_CTLRST_DELAY_TMR, 1, FALSE);	/* FCLNX-0276 */

	return;
}


/*
 * Function:    hfc_fx_mck_port_recovery
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - 
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_mck_port_recovery(struct port_info	*pp)
{

	struct core_info	*core=NULL;
	uint i=0;

	HFC_DBGPRT(" hfcldd : hfc_fx_mck_port_recovery - start\n");

    if( ( test_bit(HFC_MCK_SUCCESS,   (ulong *)&pp->mck_result) ) ||
        ( test_bit(HFC_PS_ISOL, (ulong *)&pp->status  )   ) )
	{
		/* Mck process succeeded */
//		hfc_fx_reset_start(pp, HFC_FW_START);                      /* FCWIN-0237 */
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		//@MLPF
		
		if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
			if(pp->mck_on_sleep) {
				hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);				/* FCLNX-0296 */
			}
		}

		/* Link Up Timer Set */
		hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_MCK_TMR );						/* FCLNX-0241 */
		hfc_fx_w_start( pp, core, HFC_FX_WLINKUP_MCK_TMR, pp->linkup_tmo );					/* FCLNX-0241 */
		set_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );	/* FCLNX-GPL-FX-005 */
	}
	else if( test_bit(HFC_MCK_HW_INIT, (ulong *)&pp->mck_result) )
	{
		/* HW initialization is in progress */
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
			if(pp->mck_on_sleep) {
				hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
	}
	else if( test_bit(HFC_MCK_HWCHKSTOP, (ulong *)&pp->mck_result) )
	{
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
			if(pp->mck_on_sleep) {
				hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Set interruption mask */
		clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );
	}
	else if( test_bit(HFC_MCK_CNT_OVER, (ulong *)&pp->mck_result) )
	{
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
			if(pp->mck_on_sleep) {
				hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Set interruption mask */
		clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );
	}
	else if( test_bit(HFC_MCK_END, (ulong *)&pp->mck_result)||test_bit(HFC_MCK_EXEC, (ulong *)&pp->mck_result) )
	{
		hfc_fx_reset_port_info(pp);
		/* INITTBL ADR(0x310)  */
		/* ALPA(0x319)         */
		hfc_fx_reset_start(pp, HFC_SET_INIADR);
//		hfc_fx_reset_start(pp, HFC_FW_START);                      /* FCWIN-0237 */
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		//@MLPF
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					if( HFC_FX_MMODE_CHECK_SHARED(pp) )
					{
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
						  ( char )0x4, hfc_inta_mask_mlpf[pp->pkg.type], HFC_FX_CORE_OFFSET10);
					}
					else{
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
						  ( char )0x4, hfc_inta_mask[pp->pkg.type], HFC_FX_CORE_OFFSET10);
					}
				}
			}
		}

		/* Link Up Timer Set */
		hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_MCK_TMR );						/* FCLNX-0241 */
		hfc_fx_w_start( pp, core, HFC_FX_WLINKUP_MCK_TMR, pp->linkup_tmo );					/* FCLNX-0241 */
		set_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );				/* FCLNX-GPL-FX-005 */
	}
	
	unlock_fx_mailbox(pp);

	return TRUE;
}


/*
 * Function:    hfc_fx_chk_stop
 *
 * Purpose:     Check stop processing
 *
 * Arguments:   
 *  pp         - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_chk_stop(struct port_info *pp)
{
	struct core_info			*core=NULL;
	struct target_info_fx		*target=NULL; /* FCLNX-GPL-327 */
	struct port_info			*vpp;
	struct region_info			*rp;
	
	int							i,j;
	uchar						rid;
	uint						wk4, logdata1;

	HFC_DBGPRT(" hfcldd : hfc_fx_chk_stop - start\n");
	HFC_DBGPRT(" hfcldd%d : hfc_fx_chk_stop \n",pp->pport->dev_minor);
	
	rid = pp->rid;
	
	for (j=0; j<=pp->pport->max_vport_count; j++) {
		vpp = pp->pport->vport_ptr[j].vport_arg;
		if (vpp == NULL)
			continue;
		
		rp = pp->pport->region_arg[vpp->rid];
		if (rp == NULL)
			continue;
		
		if (rid != vpp->rid) {
			HFC_ALLCORELOCK(rp);
		}
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CHKSTP, 0x20, vpp, NULL, NULL, NULL, NULL,
			0,0,0);
		
		set_bit(HFC_PS_ISOL, (ulong *)&vpp->status ) ;
		
		if (hfc_manage_info.npubp->hfc_fx_mp_hsd_enable)					/* FCLNX-0429 */
			hfc_manage_info.npubp->hfc_fx_make_fcinfo(vpp, NULL, 0x31, SCS_MCK, NULL, 0);
		
		/* pkg.type is FIVE-FX */
		/* Inhibit interruption in all ports */	
		if(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()){	/*FCLNX-GPL-349*/
			hfc_fx_w_stop(vpp, core, HFC_FX_LDLERR_TMR);	/*FCLNX-0506*/
			hfc_fx_w_stop(vpp, core, HFC_FX_LDSERR_TMR);	/*FCLNX-0506*/
			hfc_fx_w_stop(vpp, core, HFC_FX_IFERR_TMR);		/*FCLNX-0506*/
			hfc_fx_w_stop(vpp, core, HFC_FX_TOERR_TMR);		/*FCLNX-0506*/
			for (i=0;i<vpp->max_target;i++){	/* FCLNX-GPL-327 */
				target = vpp -> target_arg[i];
				if(target != NULL){
					hfc_fx_watchdog_enter( vpp, core, target, NULL, 0, HFC_FX_TGT_LDLERR_TMR, 0,1);
					hfc_fx_watchdog_enter( vpp, core, target, NULL, 0, HFC_FX_TGT_LDSERR_TMR, 0,1);
				}
			}								/* FCLNX-GPL-327 */
		}
		
		if (HFC_FX_PHYSICAL_PORT(vpp)) {
			/*** Log out ***/
			memset(vpp->logdata, 0, 16);			/* FCLNX-GPL-391 */
			vpp->logdata[0]=vpp->c_err;				/* FCLNX-GPL-391 */
			vpp->logdata[1]=vpp->status_detail2;	/* FCLNX-GPL-391 */
			
			wk4 = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
			HFC_4B_TO_4L(logdata1, wk4);
			memcpy(&vpp->logdata[4],(char *)&logdata1, 4) ;
			wk4 = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
			HFC_4B_TO_4L(logdata1, wk4);
			memcpy(&vpp->logdata[8],(char *)&logdata1, 4) ;
			
			if(!test_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&vpp->status_detail2)
			&& !test_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&vpp->status_detail2)){	/* FCLNX-GPL-FX-322 */
				set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&vpp->status_detail2);		/* FCLNX-707 */
				if( pp->imllog_logout != 1 ){		/* FCLNX-GPL-FX-141 */
					hfc_fx_save_hwlog_five_fx(pp, NULL, 0x31, HFC_ERRLOG_TYPE_CHKSTP);
					hfc_fx_errlog(vpp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_CHKSTP,ERRID_HFCP_ERR1,0x31,vpp->logdata,16) ;	/* FCLNX-GPL-391 */
				}
				else{
					pp->imllog_logout = 0;			/* FCLNX-GPL-FX-141 */
				}
			}
			
			vpp->mck_err_cnt = 0;								/* FCLNX-GPL-FX-229,272 */
			
			/* Stop optical transmission */
			if(!(HFC_FX_MMODE_CHECK_SHARED(vpp))){
				hfc_fx_write_reg(vpp,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
			}
			else{
				hfc_fx_mlpf_set_fcif(vpp, 0x80808080);	/* FCLNX-GPL-399 */
			}
			
			/* Send TSEQ */
			hfc_fx_reset_start(vpp, HFC_TSEQ);
		}
		
		if (test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->pport->status_detail2)) {
			set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&vpp->status_detail2);		/* FCLNX-707 */
		}
		
		/* Cancel pending SCSI commands, and stop timers. */
		hfc_fx_mck_prepare(vpp, 0x00); /* FCLNX-GPL-209 */
		
		if (vpp->rport_lu_scan == 1) {
			hfc_fx_wwnverify_linkup_timeout(vpp, NULL, 0);	/* FCLNX-GPL-565 */
		}
		
		clear_bit(HFC_PS_MCK_RECOVERY, (ulong *)&vpp->status);
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&vpp->status_detail2 );
		
		if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&vpp->status_detail1)){
			if(vpp->mck_on_sleep) { /* FCLNX-GPL-209 */
				hfc_fx_wake_up(&vpp->mck_event, &vpp->mck_event_wait);		/* FCLNX-0296 */
			}
		}
		
		if (HFC_FX_PHYSICAL_PORT(vpp)) {
			/* FCLNX-GPL-FX-322 Start */
			if (test_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&vpp->status_detail2 )){
				hfc_fx_errlog(
						vpp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0x8E, vpp->logdata, 16);	/* FCLNX-GPL-391 */
				
				HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
						vpp->pci_cfginf->bus->number,
						PCI_SLOT(vpp->pci_cfginf->devfn),
						PCI_FUNC(vpp->pci_cfginf->devfn));
			}
			else if(test_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&vpp->status_detail2)){
				hfc_fx_errlog(
						vpp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0x8F, pp->logdata, 16);	/* FCLNX-GPL-391 */
				
				HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by error \n",
						vpp->pci_cfginf->bus->number,
						PCI_SLOT(vpp->pci_cfginf->devfn),
						PCI_FUNC(vpp->pci_cfginf->devfn));
			}
			/* FCLNX-GPL-FX-322 End */
			/* Ctrl Reset */
			if( !(vpp->debug_func & HFC_DEBUG_CTLRST) ){ /* FCLNX-GPL-209 */
				/* FCLNX-GPL-554 BS500 TX_Disable disconnect */
				hfc_fx_reset_start(vpp, HFC_CTLRST);
				udelay(1000); /* FCLNX-GPL-081 */
			}
			
			/* Turn LED (Yellow and Green) off */
			if(!(HFC_FX_MMODE_CHECK_SHARED(vpp))){
				hfc_fx_write_reg(vpp, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
			}
			else{
				hfc_fx_mlpf_set_led(vpp, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
			}
			
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					if ((core = pp->region_arg[pp->rid]->core_arg[i]) != NULL){
						if ( HFC_FX_MMODE_CHECK_SHARED(pp) ){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  ( char )0x4, (int)HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
						}else{
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  ( char )0x4, (int)(0x00000000), HFC_FX_CORE_OFFSET10);
						}
						
						if( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
							HFC_DBGPRT("hfcldd%d HFC_FX_MMODE_CHECK_SHADOW\n",pp->dev_minor);
							if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&vpp->status_detail2)){
								HFC_DBGPRT("hfcldd%d HFC_FX_MMODE_CHECK_SHADOW1\n",pp->dev_minor);
								if(!hfc_fx_check_cs_disable(pp, core)){	/* FCLNX-GPL-FX-461 */
									if((i + MAX_CORE_PROBE_FX/pp->core_num) < MAX_CORE_PROBE_FX)
										hfc_fx_write_hg_reg_core(pp, i, HFC_IOHGSPC_CMD_REG0, 4, HFC_FX_MLPF_CORE_CSTPEND, HFC_FX_CORE_OFFSET40);     /* FCLNX-0388 *//* FCLNX-GPL-FX-405 */
									else
										hfc_fx_write_hg_reg_core(pp, i, HFC_IOHGSPC_CMD_REG0, 4, HFC_FX_MLPF_PORT_CSTPEND, HFC_FX_CORE_OFFSET40);     /* FCLNX-0388 *//* FCLNX-GPL-FX-405 */
								}	/* FCLNX-GPL-FX-461 */
								// Core FORCE-CHKSTOP command
								hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_CORE0_CMD1,
								  (char)0x1, 0x01, HFC_FX_CORE_OFFSET100);
								
								set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
							}
						}
						else if ( HFC_FX_MMODE_CHECK_BASIC(pp) ){
							HFC_DBGPRT("hfcldd%d HFC_FX_MMODE_CHECK_BASIC",pp->dev_minor);
							// Core FORCE-CHKSTOP command
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_CORE0_CMD1,
							  (char)0x1, 0x01, HFC_FX_CORE_OFFSET100);
							
							set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
						}
						
						core->mck_err_cnt = 0;	/* FCLNX-GPL-FX-105 */
#if 0	/* FCLNX-GPL-FX-105 */
						if( test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&vpp->status_detail2) ){/* FCLNX-GPL-FX-060 */
							core->mck_err_cnt = 0;
						}	/* FCLNX-GPL-FX-060 */
#endif	/* FCLNX-GPL-FX-105 */

					}
				}
			}
		}
		
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/vpp->core_num){
			if(vpp->region_arg[vpp->rid] != NULL){
				if ((core = vpp->region_arg[vpp->rid]->core_arg[i]) != NULL){
					if (test_bit(HFC_CS_CHK_STOP, (ulong *)&pp->pport->region_arg[0]->core_arg[i]->status)) {
						set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
					}
				}
			}
		}
		
		clear_bit(HFC_PS_ONLINE, (ulong *)&vpp->status );
		clear_bit(HFC_PS_CONNECTED, (ulong *)&vpp->status );	/* FCLNX-GPL-FX-005 */
		clear_bit( HFC_PS_WAIT_LINKUP, (ulong *)&vpp->status );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
			clear_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
#endif	/* FCLNX-GPL-FX-424 */
		
		if( HFC_FX_MMODE_CHECK_SHADOW(vpp) )
		{
			if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&vpp->status_detail2)){
				HFC_ERRPRT("hfcldd%d : shared FC driver enable status off by HW error.\n",vpp->dev_minor);
				hfc_fx_mlpf_change_state_port(vpp,HFC_HG_LPRSTATUS_UNSHARABLE,HFC_ENABLE_LPAR_STATE);		/* FCLNX-GPL-FX-446 */
				hfc_fx_mlpf_change_state_port(vpp, HFC_HG_HYPSTATUS_ENABLE, HFC_DISABLE_HYPER_STATE );   /* FCLNX-0385 */
			}
		}
		
		/*** Wake up sleep event  ***/
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/vpp->core_num){
			if(vpp->region_arg[vpp->rid] != NULL){
				if ((core = vpp->region_arg[vpp->rid]->core_arg[i]) != NULL){
					if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) ){
						hfc_fx_wake_up(&vpp->mb_event, &vpp->mb_event_wait);
					}
				}
			}
		}
		
		if( (test_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&vpp->status)) ||
		 	(vpp->initialize == 1) )
		{
			unlock_fx_mailbox( vpp ) ; 
			hfc_fx_wake_up(&vpp->init_event, &vpp->int_a_poll);
			clear_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&vpp->status);
		}
		
		clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&vpp->status);	/* FCLNX-GPL-FX-005 */
		
		hfc_fx_hand2_trace(
			HFC_FX_TRC_CHKSTP, 0x21, vpp, NULL, NULL, NULL, NULL,
			0,0,0);
		
		if (rid != vpp->rid) {
			HFC_ALLCOREUNLOCK(rp);
		}
	}
	
	return;
}

/*
 * Function:    hfc_fx_logout
 *
 * Purpose:     LOG OUT processing
 *
 * Arguments:   
 *  pp         -
 *  err_no     -
 *  mode       -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_logout(
	struct port_info        *pp,
	uint                    err_no,
	uchar                   mode)
{
	switch(pp->pkg.type){
		case HFC_PKTYPE_FPP:
			break;
		
		case HFC_PKTYPE_FIVE:
			break;
		
		case HFC_PKTYPE_FIVE_EX:
			break;
		
		case HFC_PKTYPE_FIVE_FX:
			hfc_fx_save_hwlog_five_fx(pp, NULL, err_no, mode);
			break;
		
		default:
			/* NOP */
			break;
	}
}


/*
 * Function:    hfc_fx_watchdog
 *
 * Purpose:     Watchdog timer processing
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_watchdog(struct wtimer_fx *w_timer)
{
	struct port_info		*pp=NULL, *wk_pp=NULL;
	struct port_info		*vpp;
	struct core_info		*core=NULL, *core_wk=NULL;	
	struct target_info_fx	*target=NULL, *target_wk=NULL ;
	struct hfc_pkt_fx		*hfcp=NULL , *issue_hfcp_top=NULL;
	struct hfc_pkt_fx		*wx_hfcp=NULL, *wk_hfcp=NULL;
	struct dev_info_fx		*dev=NULL;
	struct region_info		*rp;
	uchar	to_cmnd_exist = 0 ;	/* FCLNX-GPL-FX-076,406 */
	
	unsigned long	flags   = 0;
	int		lp=0,i=0, j=0;
	ushort	mb_tid=0;	/* FCLNX-GPL-FX-014 */
	int		lun=0, issue_mihlog=0;
	int 	rc=0 ;
	uint	hyp_status = 0, mb_code=0, int_a_rst=0;
	int		pkt_cnt, shortage;
	uint 					int_vector_reg = 0;
	
	HFC_DBGPRT(" hfcldd : hfc_fx_watchdog - start id=%x\n",w_timer->timer_id);
	
	if (w_timer->pp == NULL) return;
	
	if (HFC_FX_MQ_VALID(w_timer->pp) && (w_timer->timer_id == HFC_FX_SCSI_CMD_TMR)) {
		if ((pp = w_timer->pp->pport) == NULL) return;
		if (w_timer->core == NULL) return;
		if ((core = pp->region_arg[0]->core_arg[w_timer->core->core_no]) == NULL) return;
		if (w_timer->target == NULL) return;
		if ((target = pp->target_arg[w_timer->target->pseq]) == NULL) return;
	}
	else {
		pp = w_timer->pp ;
		core = w_timer->core ;
		target = w_timer->target;
	}
	if(pp == NULL) return;
	
	hfcp = w_timer->hfcpk;
	if(hfcp != NULL)
	{
		lun = hfcp->lun_id;
	}
	dev = w_timer->dev;
	
	rp = pp->region_arg[pp->rid];
	if(rp == NULL) return;
	
    HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
    
    if(( w_timer->timer_id == HFC_FX_MB_RSP_TMR)
    || ( w_timer->timer_id == HFC_FX_SCSI_CMD_TMR)
    || ( w_timer->timer_id == HFC_FX_ABORT_TMR)
    || ( w_timer->timer_id == HFC_FX_TARGET_RST_TMR)
    || ( w_timer->timer_id == HFC_FX_CANCEL_SCSI_TMR)
    || ( w_timer->timer_id == HFC_FX_LINKUP_TMR)
    || ( w_timer->timer_id == HFC_FX_MCKINT_TMR)
    || ( w_timer->timer_id == HFC_FX_SCN_LINKUP_TMR)
    || (!HFC_FX_MMODE_CHECK_SHARED(pp))){
	    int_vector_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INT_VECTOR, (char)0x4);
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_WDOG, 0x00, pp, rp, core, target, hfcp,
		(uint64_t)w_timer->wdog_time, (uint64_t)w_timer->timer_id, (uint64_t)int_vector_reg);	/* FCLNX-GPL-FX-061 */
					
	w_timer->timer_flag &= ~HFC_TIMER_VALID;
//	mpap = pp->mp_adap_info;
	
	switch(w_timer->timer_id)
	{
		case HFC_FX_MB_DELAY_TMR:
			clear_bit(HFC_PD_MB_DELAY, (ulong *)&pp->status_detail1 );
			issue_fx_next_mailbox(pp, hfcp);
			break;
		case HFC_FX_MB_RETRY_DELAY_TMR:
			if(core == NULL) break;
			clear_bit(HFC_CS_MB_RETRY_DELAY, (ulong *)&core->status);	/* FCLNX-GPL-FX-161 */
			hfc_fx_mb_passthrough(pp, core, core->mb_retry_tid, core->mb_retry_tout, core->mb_retry_cnt,core->mb_callback);
			break;
		case HFC_FX_MB_RETRY_TMR :
			clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
			break;
		case HFC_FX_MB_RSP_TMR:			/* MailBox Time-Out			*/
			if(core == NULL) break;
			if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status ) )
			{
				core->mb_status = 0 ;				/* FCLNX-0402 */
				core->mb_resp = HFC_MBR_TIMEOUT;
				hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
				break ;
			}
			core->mb_status = 0 ;
			core->mb_resp = HFC_MBR_TIMEOUT;

			memset(core->logdata,0,16);					/* FCLNX-GPL-391 */
			memcpy(&core->logdata[0],(char *)&core->mb->mb_init.mb_code, 4) ;
			HFC_4B_TO_4L(mb_code, core->mb->mb_init.mb_code);
			if((mb_code & 0xffff0000) == HFC_MBCMD_SNDRCV){
				memcpy(&core->logdata[4],(char *)&core->payload->send_payload.data0[0],2);
			}
			if( !test_bit( HFC_PS_DIAG, (ulong *)&pp->status ) )	/* FCLNX-GPL-FX-233,272 */
			{
				hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_MBINIT,ERRID_HFCP_EVNT4,0x2a,core->logdata,16) ;	/* FCLNX-GPL-391 */
			}														/* FCLNX-GPL-FX-233,272 */
            hfc_fx_abend(pp, core, HFC_ABEND_MB_TOUT);

			break ;

		
		case HFC_FX_DELAY_TMR:			/* DELAY timer after executing TARGET RESET		*//* FCLNX-GPL-FX-014 */
			if(target == NULL) break;	/* FCLNX-GPL-038 */ /* FCLNX-GPL-189 */
			clear_bit( HFC_TS_SCSI_DELAY, (ulong *)&target->status );

			if( (test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status) || test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)) 
			&&  (test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) )	/* FCLNX-GPL-FX-005 */
			{
				/* If adapter is waiting LINK_UP, I/O start can not be executed. */
				break;																/* FCLNX-GPL-038 */
			}
			else if( !(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) )
			{
				/* 
				 * If adapter is not online, target should have been already canceled.
				 * Break this process 
				 */
				break ;																/* FCLNX-GPL-038 */
			}
			else if( !(test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status) || test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) )
			  && (test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) )/* FCLNX-GPL-FX-005 */
			{
				/* Restart waiting request if adapter is ONLINE */
				if(!(test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status) )  
				&& test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status)  )			/* FCLNX-GPL-038 */
				{
					/* Initiated LOGIN but failed with mailbox busy */
					break;
				}
				else if( test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status) )  	/* FCLNX-GPL-038 */
				{
					/* Initiated CANCEL SCSI but failed with mailbox busy */
					break;
				}																	/* FCLNX-GPL-038 */
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					if(hfc_fx_check_cs_disable(pp, core_wk))
						continue;	/* FCLNX-GPL-FX-438 */
					if(  target->core_queue[core_wk->core_no].wx_que_cnt > 0 ){					/*-- FCLNX-019 STR--*/
						hfc_fx_issue_intl_start(pp, pp->region_arg[pp->rid], core_wk, target) ;
					}												/*-- FCLNX-019 END--*/
				}
			}
			break ;
			
		case HFC_FX_SCSI_CMD_TMR:		/* SCSI Command(Normal) Time-Out	*/
			if( target == NULL) break; /* FCLNX-GPL-189 */
			if( hfcp == NULL ) break;
			if( dev == NULL ) break;		/* FCLNX-GPL-0343 */
			if( core == NULL) break;
			
			if (HFC_FX_MQ_VALID(pp)) {
				if ((vpp = pp->vport_ptr[hfcp->rid].vport_arg) == NULL) break;
				if ((target_wk = vpp->target_arg[target->pseq]) == NULL) break;
				if (pp->region_arg[hfcp->rid] == NULL) break;
				if ((core_wk = pp->region_arg[hfcp->rid]->core_arg[core->core_no]) == NULL) break;
			}
			else {
				target_wk = target;
				core_wk = core;
				vpp = pp;
			}
			
			if ((target_wk->core_queue[core_wk->core_no].we_que_cnt == 0) &&
				(target_wk->core_queue[core_wk->core_no].wx_que_cnt == 0))
				break;
			
			if(pp->to_err_limit){                              /* FCLNX-GPL-349 */
				if(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()){
					hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_TO_ERR);  /* FCLNX-0506 */
				}
				else{
					hfc_fx_watched_errcount_i(pp, NULL, HFC_TO_ERR);  /* FCLNX-GPL-349 */
				}
			}
			
			set_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );
			pp->scsi_timeout_fail++;
			
			if(hfc_fx_pcibus_chk(pp) != 0)
			{	/* "PCI BUS ERR" has happen. */
				HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
			}
			else{
				wx_hfcp = target_wk->core_queue[core_wk->core_no].wx_que_top;
				
				while( wx_hfcp != NULL )
				{
					if( wx_hfcp == hfcp )
						break;
					
					wx_hfcp = wx_hfcp->cmd_forw;
				}
				
				if( wx_hfcp != NULL )
				{
					hfc_fx_deque_wx_que(core_wk, hfcp);
					hfc_fx_iodone(vpp, core, wx_hfcp->cmd_pkt, wx_hfcp);
					break ;
				}
				
				if( hfc_fx_toutchk_xob(vpp, target_wk, hfcp, lun, HFC_ISSUE_ABORT) )
				{
					/* 
					 * Timed-out xob remain in queue so do not initiate
					 * MIH-LOG and Abort-Task-Set 
					 */
					 break;
				}
				if((!test_bit(HFC_MAILBOX_BUSY, (ulong *)&vpp->region_arg[vpp->rid]->mb_lock ))
				 && (!test_bit( HFC_PD_MB_DELAY, (ulong *)&vpp -> status_detail1 ))
				 && !( dev->lustat & HFC_DS_ISSUE_ANY_RESET )									/* FCLNX-GPL-FX-177 */
				 && !( target->status & HFC_TS_ISSUE_ANY_RESET )								/* FCLNX-GPL-FX-177 */
				 && (decide_fx_next_mailbox(vpp, &mb_tid) == 0)){	/* FCLNX-GPL-FX-014 Start */
					set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&vpp->status_detail2);					/* FCLNX-0506 */
					hfc_fx_issue_mihlog(vpp, hfcp);
					issue_mihlog=1;
					set_bit(HFC_DC_WAIT_MIHLOG, (ulong *)&dev->dev_core_stat.core[core->core_no]);
				}
				
				if(pp->c_err!=0) break;	/* FCLNX-GPL-FX-321 */
				
				/* check pkt_pool */
				pkt_cnt = 0;
				shortage = 0;
				for (i=0; i<=pp->max_vport_count; i++) {
					vpp = pp->pport->vport_ptr[i].vport_arg;
					if (vpp == NULL)
						continue;
					if (vpp->region_arg[i] == NULL)
						continue;
					
					for(j=0;j<MAX_CORE_PROBE_FX;j+=MAX_CORE_PROBE_FX/pp->core_num){
						if (vpp->region_arg[i]->core_arg[j]) {
							pkt_cnt += vpp->region_arg[i]->core_arg[j]->pkt_cnt;
						}
					}
				}
				if ((pp->pport->pkt_num - pkt_cnt) < 4*pp->pport->core_num) {
					/* pkt_pool shortage */
					shortage = 1;
				}
				HFC_DBGPRT(" hfcldd : hfc_fx_watchdog - HFC_FX_SCSI_CMD_TMR pkt_num=%d, pkt_cnt=%d, shortage=%d\n",
								pp->pport->pkt_num, pkt_cnt, shortage);
				
				if((pp->abort_t_restrain)&&(pp->tgtrst_restrain)&&(!issue_mihlog)){
					set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
					hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
					break;
				}else if ((pp->abort_t_restrain) || shortage){
					if( target->status & HFC_TS_ISSUE_ANY_RESET)				/* FCLNX-GPL-FX-177 */
						break ;
					/* Cancel wait queue of SCSI command and Task Management *//* FCLNX-GPL-FX-092 */
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
							continue;
						hfc_fx_cancel_scsi_cmd(pp,core_wk,target,0,NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
						FALSE,TRUE, HFC_FLASH_TARGET );
					}/* FCLNX-GPL-FX-092 */
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
						hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
							TRUE, FALSE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
					}
					
					set_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);	/* FCLNX-GPL-FX-112 */
					target->target_reset_cmnd = hfcp->cmd_pkt;
					if(issue_mihlog){
						clear_bit(HFC_DC_WAIT_MIHLOG, (ulong *)&dev->dev_core_stat.core[core->core_no]); /* FCLNX-GPL-FX-283 */
						set_bit(HFC_TC_WAIT_MIHLOG, (ulong *)&target_wk->tgt_core_stat.core[core->core_no]);
					}else{	/* FCLNX-GPL-FX-255,272 Start */
						if(hfc_fx_issue_tgtrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))){
							clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
							break;
						}
					}	/* FCLNX-GPL-FX-255,272 End */
					
					if((pp->hba_isolation == HFC_ISOL_START)&&(pp->total_tgtrst_to)){
						hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
 						hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, FALSE);
					}
				}else{
					if( ( dev->lustat & HFC_DS_ISSUE_ANY_RESET )
					||  ( target->status & HFC_TS_ISSUE_ANY_RESET ) )				/* FCLNX-GPL-FX-177,202 */
						break ;
					/* Cancel wait queue of SCSI command and Task Management */	/* FCLXN-GPL-FX-092 */
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
						hfc_fx_cancel_scsi_cmd(pp,core_wk,target,dev->lun,NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
						FALSE,TRUE, HFC_FLASH_DEV );
					}	/* FCLXN-GPL-FX-092 */
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
						hfc_fx_mq_cancel_scsi_cmd(pp, target, dev->lun, NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
							TRUE, FALSE, FALSE, FALSE, TRUE, HFC_FLASH_DEV);
					}
					
					set_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
					dev->abtcmd_core_no = core->core_no;
					dev->abort_cmnd = hfcp->cmd_pkt;
					if(issue_mihlog) set_bit(HFC_DC_WAIT_MIHLOG, (ulong *)&dev->dev_core_stat.core[core->core_no]);
					
					if((hfc_fx_issue_devrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_CSCSI_LU_WITHOUT_DMA)))
					&&(!test_bit(HFC_DC_WAIT_MIHLOG, (ulong *)&dev->dev_core_stat.core[core->core_no]))){
						hfc_fx_issue_devrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_ABORT));
					}
					if((pp->hba_isolation == HFC_ISOL_START)&&(pp->total_abort_to)){
						hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);
						hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_TOTAL_ABORT_TMR, 0, FALSE);
					}
				}	/* FCLNX-GPL-FX-014 End */
			}
			break;
			
		case HFC_FX_ABORT_TMR:		/* Abort Task Set/Clear ACA Time-Out*/
			if(target == NULL) break; /* FCLNX-GPL-189 */
			if( target->core_queue[core->core_no].we_que_cnt == 0 )
				break ;
			if( hfcp == NULL ) break;
			if( dev == NULL ) break;		/* FCLNX-GPL-0343 */
			
			hfc_fx_mp_watchdog_enter(pp, core, target, hfcp, dev, hfcp->lun_id, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);	/* FCLNX-GPL-FX-190 */
			
			if(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()){	/* FCLNX-GPL-430 */
				if(pp->rt_err_enable){                              /* FCLNX-0506 */
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if(pp->region_arg[pp->rid] != NULL){
							if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
								continue;
							if(hfc_fx_check_cmnd_timeout(pp,core_wk,target,NULL)){			/* FCLNX-GPL-430 *//* FCLNX-GPL-FX-014 */
								hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);  /* FCLNX-0506 */
								break;
							}
						}
					}
				}
				else{
					if(pp->to_errcnt_info!=NULL)              /* FCLNX-0506 */
						 hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_TO_ERR);
				}
			}
			else if(pp->rt_err_enable){                              /* FCLNX-0506 */
				hfc_fx_watched_errcount_i(pp, NULL, HFC_RT_ERR);
			}
									
			hfcp->adap_status |= SCS_CMD_TIMEOUT;

			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){									/* FCLNX-GPL-FX-112 Start */
				core_wk = pp->region_arg[pp->rid]->core_arg[i];	/* FCLNX-GPL-FX-076 */
				if((core_wk != NULL)&&(hfc_fx_check_cmnd_timeout(pp,core_wk,target,(ushort*)&dev->lun))){	/* FCLNX-GPL-FX-014 */
					to_cmnd_exist = 1 ;	/* FCLNX-GPL-FX-076 */
					break;
				}	/* FCLNX-GPL-FX-076 */
			}																								/* FCLNX-GPL-FX-112 End */
			
			set_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );	/* FCLNX-GPL-FX-091 *//* FCLNX-GPL-FX-112 */
			
			if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ) {	/* FCLNX-GPL-0343 */
				clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
				clear_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
				set_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-076 */
			}
			else {
				clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
				clear_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
				set_bit(HFC_DS_FAIL_ABORT, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-076 */
			}													/* FCLNX-GPL-0343 */

			if( hfc_fx_toutchk_xob(pp, target, hfcp, lun, HFC_ISSUE_TARGET_RESET) )
			{
					/*
					 * Timed-out xob remain in queue so do not initiate
					 * MIH-LOG and Abort-Task-Set 
					 */
				break ;
			}

			if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
			{	/* "PCI BUS ERR" has hpppen. */
				HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
			}
			else{
				if((!test_bit(HFC_MAILBOX_BUSY, (ulong *)&pp->region_arg[pp->rid]->mb_lock ))	/* FCLNX-GPL-FX-014 */
				 && (!test_bit( HFC_PD_MB_DELAY, (ulong *)&pp -> status_detail1 ))
				 && !( target->status & HFC_TS_ISSUE_ANY_RESET )								/* FCLNX-GPL-FX-177 */
				 && (decide_fx_next_mailbox(pp, &mb_tid) == 0)
				 && (!to_cmnd_exist)){															/* FCLNX-GPL-FX-112 */
					set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);					/* FCLNX-0506 */
					hfc_fx_issue_mihlog(pp, hfcp);
					issue_mihlog=1;
				}
				
				if(pp->c_err!=0) break; /* FCLNX-0597 */	/* FCLNX-GPL-FX-321 */
				
				if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ) {	/* FCLNX-GPL-FX-321 */
					if((!to_cmnd_exist)&&(!test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags ))){	/* FCLNX-GPL-FX-085 *//* FCLNX-GPL-FX-320 */
						if(!issue_mihlog){
							hfc_fx_timeout_by_reset(pp, target, hfcp);
							hfc_fx_deque_we_que(pp, target, hfcp);
							clear_bit(HFC_DC_WAIT_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);
						}
						break;	/* FCLNX-GPL-FX-076 */
					}	/* FCLNX-GPL-FX-085 */	/* FCLNX-GPL-FX-321 */
				}
				
				if(!pp->tgtrst_restrain) {
					if( target->status & HFC_TS_ISSUE_ANY_RESET )	/* FCLNX-GPL-FX-112,177 */
						break ;
					/* Cancel wait queue of SCSI command and Task Management *//* FCLNX-GPL-FX-092 */
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
							continue;
						hfc_fx_cancel_scsi_cmd(pp,core_wk,target,0,NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
						FALSE,TRUE, HFC_FLASH_TARGET );
					}/* FCLNX-GPL-FX-092 */
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
						hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
							TRUE, FALSE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
					}
					
					set_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);	/* FCLNX-GPL-FX-112 */
					target->target_reset_cmnd = hfcp->cmd_pkt;
					if(issue_mihlog){
						set_bit(HFC_TC_WAIT_MIHLOG, (ulong *)&target->tgt_core_stat.core[core->core_no]);
					}else{
						if(hfc_fx_issue_tgtrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))){
							clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
							break;
						}
					}
					
					if((pp->hba_isolation == HFC_ISOL_START)&&(pp->total_tgtrst_to)){
						hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
 						hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, FALSE);
					}
				}else{
					set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
					hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
				}
			}
			
			break ;

		case HFC_FX_TARGET_RST_TMR:	/* Target Reset Time-Out			*/
			if(target == NULL) break;	/* FCLNX-GPL-FX-076 */
			if(core == NULL) break;
			if( target->core_queue[core->core_no].we_que_cnt == 0 )
				break ;
			if( hfcp == NULL ) break;	/* FCLNX-GPL-FX-076 */
			
			/* FCLNX-0506 */
			set_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags);	/* FCLNX-GPL-FX-085 */
			clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-GPL-FX-085 */
			clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status);	/* FCLNX-GPL-FX-085 */
			
			if(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()){	/* FCLNX-GPL-430 */
				if(pp->rt_err_enable){                              /* FCLNX-0506 */
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if(pp->region_arg[pp->rid] != NULL){
							if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
								continue;
							if(hfc_fx_check_cmnd_timeout(pp,core_wk,target,NULL)){			/* FCLNX-GPL-430 *//* FCLNX-GPL-FX-014 */
								hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);  /* FCLNX-0506 */
								break;
							}
						}
					}
				}
				else{
					if(pp->to_errcnt_info!=NULL)              /* FCLNX-0506 */
						 hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_TO_ERR);
				}
			}
			else if(pp->to_err_limit){	/* FCLNX-GPL-349 */
				hfc_fx_watched_errcount_i(pp, NULL, HFC_TO_ERR);  /* FCLNX-GPL-349 */
			}
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				core_wk = pp->region_arg[pp->rid]->core_arg[i];
				if((core_wk != NULL)&&(hfc_fx_check_cmnd_timeout(pp,core_wk,target,NULL))){
					to_cmnd_exist = 1 ;	/* FCLNX-GPL-FX-085 */
					break;
				}
			}
			set_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );	/* FCLNX-GPL-FX-091 */
			
			if((!test_bit(HFC_MAILBOX_BUSY, (ulong *)&pp->region_arg[pp->rid]->mb_lock ))	/* FCLNX-GPL-FX-014 */
			 && (!test_bit( HFC_PD_MB_DELAY, (ulong *)&pp -> status_detail1 ))
			 && (decide_fx_next_mailbox(pp, &mb_tid) == 0)
			 && (!to_cmnd_exist)){															/* FCLNX-GPL-FX-112 */
				HFC_DBGPRT( "hfc_fx_watchdog - Issue MIHLOG.\n" );
				set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);					/* FCLNX-0506 */
				hfc_fx_issue_mihlog(pp, hfcp);
				issue_mihlog=1;
			}
			
			if(pp->c_err!=0) break; /* FCLNX-GPL-FX-321 */
			
			if(((to_cmnd_exist)||(test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 ))
			  ||(test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags )))
			  &&(!issue_mihlog)){	/* FCLNX-GPL-FX-014,085 *//* FCLNX-GPL-FX-320 */
				HFC_DBGPRT( "hfc_fx_watchdog - Issue Link Reset.\n" );
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			}else if((!to_cmnd_exist)&&(!issue_mihlog)){	/* FCLNX-GPL-FX-085 */
				HFC_DBGPRT( "hfc_fx_watchdog - Target Reset Time-Out Proc End.\n" );
				hfc_fx_timeout_by_reset(pp, target, hfcp);
				hfc_fx_deque_we_que(pp, target, hfcp);
				clear_bit(HFC_TC_WAIT_TGTRST, (ulong *)&target->tgt_core_stat.core[core->core_no]);
			}	/* FCLNX-GPL-FX-085 */
			
			break ;
			
		case HFC_FX_CANCEL_SCSI_TMR:	/* FCLNX-GPL-FX-014 */
			set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
			hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			break;	/* FCLNX-GPL-FX-014 */
#if 0
		case HFC_FX_ELS_TMR:			/* ELS Time_Out						*/
#endif
		case HFC_FX_LINKINIT_TMR:		/* Link Initialize Time-Out			*/
		case HFC_FX_LINKUP_TMR :		/* LINK UP waiting timer */
		case HFC_FX_WLINKUP_MCK_TMR :	/* LINK UP waiting timer after MCK recovers */ /* FCLNX-241 */
			
			if (HFC_FX_MQ_VALID(pp)) {
				if (HFC_FX_MQ_VIRTUAL_PORT(pp))
					break;
				
				hfc_fx_wdog_linkup_tmr(pp, core, w_timer->timer_id);
				
				for (i=1; i<=pp->max_vport_count; i++) {
					vpp = pp->pport->vport_ptr[i].vport_arg;
					if (vpp == NULL)
						continue;
					if (!test_bit(HFC_PS_ENABLE, (ulong *)&vpp->status))
						continue;
					
					hfc_fx_wdog_linkup_tmr(vpp, core, w_timer->timer_id);
				}
			}
			else {
				hfc_fx_wdog_linkup_tmr(pp, core, w_timer->timer_id);
			}
			break ;

		case HFC_FX_WLINKUP_CNT_TMR :	/* FCLNX-GPL-FX-424 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
				if(!test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status)){
					if(pp->c_err==0){	
						set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
						atomic_set(&pp->check_mbreq, 1);
					}
					pp->linknego_tmo_boot=0;
				}else{
					if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-436 Start */
						memset((void *)pp->logdata, 0, 16);
						pp->logdata[7] = HFC_LINKUPTMR_TO ;
						hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
					}else if( (test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2))
							  &&  (!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
							  &&  (hfc_fx_mlpf_check_normal_hypsts(pp)) ){
						set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
						set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
						hfc_fx_mlpf_linkend_int_glpar(pp);
						return;
					}else{
						memset((void *)pp->logdata, 0, 16);
						pp->logdata[7] = HFC_LINKUPTMR_TO ;
						hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
					}	/* FCLNX-GPL-FX-436 End */
				}
				clear_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
			}
#endif
			break;

			/* FCLNX-GPL-FX-014 Start*/
		case HFC_FX_TOTAL_ABORT_TMR :
			if(target == NULL) break;
			if( dev == NULL ) break;
			if( hfcp == NULL ) break;
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){									/* FCLNX-GPL-FX-112,190 Start */
				core_wk = pp->region_arg[pp->rid]->core_arg[i];	/* FCLNX-GPL-FX-076 */
				if((core_wk != NULL)&&(hfc_fx_check_cmnd_timeout(pp,core_wk,target,(ushort*)&dev->lun))){	/* FCLNX-GPL-FX-014 */
					to_cmnd_exist = 1 ;	/* FCLNX-GPL-FX-076 */
					break;
				}	/* FCLNX-GPL-FX-076 */
			}
			if(to_cmnd_exist == 0)break;
																											/* FCLNX-GPL-FX-112,190 End */
			
			if(pp->tgtrst_restrain){
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			}else{
				
				/* Cancel wait queue of SCSI command and Task Management *//* FCLNX-GPL-FX-092 */
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					hfc_fx_cancel_scsi_cmd(pp,core_wk,target,0,NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
					FALSE,TRUE, HFC_FLASH_TARGET );
				}/* FCLNX-GPL-FX-092 */
				if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
					hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
						TRUE, FALSE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
				}
				
				set_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);	/* FCLNX-GPL-FX-172 Start */
				target->target_reset_cmnd = hfcp->cmd_pkt;
				
				if(hfc_fx_issue_tgtrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))){
					clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);
					break;
				}
				
				if((pp->hba_isolation == HFC_ISOL_START)&&(pp->total_tgtrst_to)){
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, TRUE);
 					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, FALSE);
				}	/* FCLNX-GPL-FX-172 End */
			}
			break;
			
		case HFC_FX_TOTAL_TGTRST_TMR :
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){	/* FCLNX-GPL-FX-190 Start */
				core_wk = pp->region_arg[pp->rid]->core_arg[i];
				if((core_wk != NULL)&&(hfc_fx_check_cmnd_timeout(pp,core_wk,target,NULL))){
					to_cmnd_exist = 1 ;	/* FCLNX-GPL-FX-085 */
					break;
				}
			}
			if(to_cmnd_exist == 0)break;									/* FCLNX-GPL-FX-190 End */
			
			set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
			hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			break;
			/* FCLNX-GPL-FX-014 End*/
			
		case HFC_FX_CTLRST_DELAY_TMR :											/* FCLNX-0279 */
			/* for FIVE-FX */
			HFC_DBGPRT(" hfcldd%d : hfc_fx_watchdog - HFC_CTLRST_DELAY_TMR\n",pp->dev_minor);
			
			if (HFC_FX_VIRTUAL_PORT(pp))
				break;
			
			/* Reset Receive Control Flag */								/* FCLNX-GPL-FX-079 */
			hfc_fx_reset_start(pp, HFC_RECEIVE_CTL_FLAG_CLEAR);				/* FCLNX-GPL-FX-079 */
			
			hfc_fx_reset_start(pp, HFC_WSCA_CLEAR);
			
			/* Set Isolate core into CCA */									/* FCLNX-GPL-FX-079 */
			hfc_fx_reset_start(pp, HFC_SET_ISOLATE_CORE);					/* FCLNX-GPL-FX-079 */
			
			for (i=0; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				
				hfc_fx_reset_port_info(vpp) ;
				set_bit(HFC_MCK_SUCCESS, (ulong *)&vpp->mck_result );
			}
			
			hfc_fx_w_stop(pp, core, HFC_FX_REBOOT_DELAY_TMR) ;
			hfc_fx_w_start(pp, core, HFC_FX_REBOOT_DELAY_TMR, 0) ;
			
			/* Adapter recovery process from isolation */
			if ( test_bit ( HFC_PD_ISOLATE_RECOVERY,   (ulong *)&pp->status_detail2) )
			{
			 	/** Clear Interruption register (This process is different from machine check recovery */
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
							  (char)0x4, 0xffffffff, HFC_FX_CORE_OFFSET10);
						}
					}
				}
			}
			
			hfc_fx_reset_start(pp, HFC_REBOOT);
			
			break;

		case HFC_FX_REBOOT_DELAY_TMR :

			/* Check HW status and POST result (retry count is zero) */
			/* FIVE-FX */
			/* Check POST result (1 time retry) */
			
			HFC_DBGPRT(" hfcldd%d : hfc_fx_watchdog - HFC_FX_REBOOT_DELAY_TMR\n",pp->dev_minor);
			
			if (HFC_FX_VIRTUAL_PORT(pp))
				break;
			
			hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1,(char)0xa0);
			
			if(pp->pkg.type == HFC_PKTYPE_FIVE_FX)
			{
				rc = hfc_fx_config_hw_set_five_fx(pp,HFC_CONFIG_HW_CHECK_RETRY_RECV);	/* FCLNX-GPL-FX-215 */
			}
			
			if( rc )
			{
				/* H/W Initialize failed. */
				HFC_FX_ISSUE_CSTP( pp, HFC_ABEND_FCSTP) ; /* FCLNX-0279 *//* @MLPF *//* FCLNX-GPL-316 */
			}
			else
			{
				HFC_DBGPRT(" hfcldd%d : hfc_fx_watchdog - hfc_fx_config_hw_set_five_fx normal end\n",pp->dev_minor);
				
				/* H/W Initialize succeeded. */
				
				for (j=0; j<=pp->max_vport_count; j++) {
					vpp = pp->vport_ptr[j].vport_arg;
					if (vpp == NULL)
						continue;
					
					if (HFC_FX_MIN_PORT_IN_REGION(vpp)) {
						/* INITTBL ADR(0x310)  */
						hfc_fx_reset_start(vpp, HFC_SET_INIADR);
					}
					
					hfc_fx_mck_port_recovery(vpp);
					
					clear_bit(HFC_PS_CONNECTED, (ulong *)&vpp->status);
					clear_bit(HFC_MCK_SUCCESS, (ulong *)&vpp->mck_result );
					
					vpp->mck_linkup = HFC_LINKUP_MCK; /* FCLNX-0595 */
					
					if ( test_bit (HFC_PS_ISOL, (ulong *)&vpp->status) ||
						 test_bit (HFC_PD_ISOLATE_RECOVERY, (ulong *)&vpp->status_detail2) ) {
						
						clear_bit( HFC_PS_ISOL, (ulong *)&vpp->status );
						HFC_DETAIL_CLEAR_ISOLREC(vpp);
						vpp->c_err = 0x00;
						clear_bit( HFC_PD_ISOLATE_RECOVERY, (ulong *)&vpp->status_detail2 );
						
						/* FCLNX-GPL-219 */
						vpp->mck_type = 0;
						vpp->mck_result = 0;
						
						if (HFC_FX_PHYSICAL_PORT(vpp)) {
							hfc_fx_errlog(
								pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
								ERRID_HFCP_EVNT2, 0xD3, NULL, 0);
							
							HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is recovered \n",
								pp->pci_cfginf->bus->number,
								PCI_SLOT(pp->pci_cfginf->devfn),
								PCI_FUNC(pp->pci_cfginf->devfn));
						}
					}
					
					/* Determine master core */
					hfc_fx_determine_master_core(vpp, vpp->region_arg[vpp->rid]);
					
					if (HFC_FX_MIN_PORT_IN_REGION(vpp)) {
						/* Set fw_init_tbl */
						hfc_fx_set_fw_init_tbl(vpp);
					}
					
					if (HFC_FX_VIRTUAL_PORT(vpp) && !HFC_FX_VPORT_ENABLE(vpp)) {
						/* skip core start during vport disable */
						clear_bit( HFC_PD_NEED_CORE_START, (ulong *)&vpp->status_detail1 );
					}
					else if (HFC_FX_MIN_PORT_IN_REGION(vpp) &&
							(!test_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2))) {	/* FCLNX-GPL-FX-248,272 */
						set_bit( HFC_PD_NEED_CORE_START, (ulong *)&vpp->status_detail1 );
						atomic_set(&vpp->check_mbreq, 1);
					}
					
					if (HFC_FX_PHYSICAL_PORT(vpp)) {
						/* Enable interrupt for all the core */
						for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
							if(pp->region_arg[pp->rid] != NULL){
								if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
									continue;
								if(hfc_fx_check_cs_disable(pp, core))
									continue;	/* FCLNX-GPL-FX-438 */
								
								HFC_DBGPRT( " hfcldd : HFC_FX_REBOOT_DELAY_TMR - enable interrupt\n");
								
								hfc_fx_write_reg_ext(pp, ( uint )(0x326 + 0x80*i), (char)0x1, 0x00);	/* FCLNX-GPL-FX-405,407 */
								
								if (HFC_FX_MMODE_CHECK_SHARED(pp)){
									HFC_DBGPRT("hfcldd%d HFC_FX_MMODE_CHECK_SHARED",pp->dev_minor);
									hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
									  (char)0x4, ( int )hfc_inta_mask_mlpf[pp->pkg.type], HFC_FX_CORE_OFFSET10);
								}
								else{
									hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
									  (char)0x4, ( int )hfc_inta_mask[pp->pkg.type], HFC_FX_CORE_OFFSET10);
								}
							}
						}
					}
					for(i=0; i<4; i+=4/vpp->core_num) {	/* FCLNX-GPL-FX-277 */
						if ((core = vpp->region_arg[vpp->rid]->core_arg[i]) == NULL)
							continue;
						if(hfc_fx_check_cs_disable(pp, core))
							continue;	/* FCLNX-GPL-FX-438 */

						if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) )
						{
							hfc_fx_wake_up(&vpp->mb_event, &vpp->mb_event_wait);
							break;
						}	/* FCLNX-GPL-FX-277 */
					}
				}
			}
			
			break ;

		case HFC_FX_MCKINT_TMR :															/* FCLNX-0275 */
			
			int_a_rst = hfc_inta_mask[pp->pkg.type];
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
						  (char)0x4, int_a_rst, HFC_FX_CORE_OFFSET10);
					}
				}
			}
			
			for (i=0; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				
				clear_bit( HFC_PS_WAIT_MCKINT, (ulong *)&vpp->status );
			}
			
			if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
				hfc_fx_mlpf_issue_ffcstp(pp, HFC_ABEND_FCSTP);
			else
				hfc_fx_abend(pp, core, HFC_ABEND_T3_NO_MCKINT);
			
			break;																		/* FCLNX-0275 */


// @MLPF
		case HFC_FX_MLPF_FMCK_TMR :
			clear_bit(HFC_PD_MLPF_WAIT_FMCK, (ulong *)&pp->status_detail2 );
			
			if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
				hfc_fx_issue_forced_mck(pp, core, HFC_ABEND_T3);
			
			break;
		
		case HFC_FX_MLPF_FCSTP_TMR :
			clear_bit(HFC_PD_MLPF_WAIT_FCSTP, (ulong *)&pp->status_detail2 );
			if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
				hfc_fx_chk_stop(pp);
			
			break;

		case HFC_FX_RESTART_TMR :						/* FCLNX-0500 */
			hfc_fx_timeout_by_restart(pp, target, dev, hfcp);
			break;

		case HFC_FX_WEXEC_TMR:
			if (core == NULL) break;
			
			if (core->next_dstart_cnt != 0) {
				hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0,HFC_FX_WEXEC_TMR, 0, FALSE);
			}
			
			for (i=0; i<=pp->pport->max_vport_count; i++) {
				vpp = pp->pport->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				
				if (vpp->rid != core->rp->rid)
					continue;
				
				for (lp=0; lp<MAX_TARGET_PROBE; lp++) {
					target = vpp->target_arg[lp];
					if (target != NULL) {
						hfc_fx_issue_intl_start(vpp, core->rp, core, target);
					}
				}
			}
			
			break ;
			
		case HFC_FX_LOGIN_DELAY_TMR:		/* Delayed Time-Out when LOGIN process completes	*/ 	/* FCLNX-0243 */
			clear_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 );
			
			if (pp->initialize){											/* FCLNX-0270 */
				if( pp->no_target == 1){									/* FCLNX-GPL-570 */
					pp->no_target = 0;
					hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
				}															/* FCLNX-GPL-570 */
			}
			else {															/* FCLNX-0270 */
				if( !(test_bit(HFC_PS_ONLINE, (ulong *)&pp -> status ) ) )
					break;
				
				if ( (pp->connect_type == HFC_FX_SWITCH )
					|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00)) ) {
					if (!test_bit(HFC_PD_WAIT_GPNFT, (ulong *)&pp->status_detail2) ) {
						set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
						atomic_set(&pp->check_mbreq, 1);
					}
				}
			}																/* FCLNX-0270 */
			break;
			
		case HFC_FX_SCN_LINKUP_TMR :
			hfc_fx_timeout_by_scnlinkup(pp, target);
			break;

		case HFC_FX_DIAG_DELAY_TMR:			/* Delayed Time-Out when diag process completes	*/
			if( test_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 ) )
			{
				hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);										/* FCLNX-0296 */
				break ;
			}
			break ;
			
			/* FCLNX-GPL-FX- xxx */
		case HFC_FX_LDLERR_TMR:	/* Long Link Down Error count timer time-out*/
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_WATCHERR_TMR_TO, HFC_LDL_ERR);		/* FCLNX-0506 */
			break;		/* FCLNX-0506 */
		case HFC_FX_LDSERR_TMR:	/* Short Link Down Error count timer time-out*/
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_WATCHERR_TMR_TO, HFC_LDS_ERR);		/* FCLNX-0506 */
			break;		/* FCLNX-0506 */
		case HFC_FX_IFERR_TMR:	/* Interface Error count timer time-out*/
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_WATCHERR_TMR_TO, HFC_IF_ERR);			/* FCLNX-0506 */
                        break;		/* FCLNX-0506 */
		case HFC_FX_TOERR_TMR:	/* SCSI T.O Error count timer time-out*/
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_WATCHERR_TMR_TO, HFC_TO_ERR);			/* FCLNX-0506 */
                        break;		/* FCLNX-0506 */
		case HFC_FX_TGT_LDLERR_TMR:	/* Long Link Down Error count timer time-out between FCSW and Disk Array */
			if(target == NULL) break;
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, target, HFC_WATCHERR_TMR_TO, HFC_TGT_LDL_ERR);		/* FCLNX-GPL-327 */
			break;		/* FCLNX-GPL-327 */
		case HFC_FX_TGT_LDSERR_TMR:	/* Short Link Down Error count timer time-out between FCSW and Disk Array */
			if(target == NULL) break;
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, target, HFC_WATCHERR_TMR_TO, HFC_TGT_LDS_ERR);		/* FCLNX-GPL-327 */
			break;		/* FCLNX-GPL-327 */
			/* FCLNX-GPL-FX- xxx */
			
		case HFC_FX_DELAY_TMR_DEV:			/* DELAY timer after executing LUN RESET	*//* FCLNX-GPL-FX-014 Start */
			if(target == NULL) break;	/* FCLNX-GPL-189 */	/* FCLNX-GPL-038 */
			if(dev == NULL) break;		/* FCLNX-GPL-189 */	/* FCLNX-GPL-038 */
			hfc_fx_clear_dev_info_fx( dev );							/* FCLNX-GPL-0343 */

			if( test_bit( HFC_TS_SCSI_DELAY, (ulong *)&target->status ) )
				break;
			if( (test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status) || test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) )
			 && (test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) ) /* FCLNX-GPL-FX-005 */
			{
				/* If adapter is waiting LINK_UP, I/O start can not be executed. */
				break;
			}
			else if( !(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) )
			{
				/* 
				 * If adapter is not online, target should have been already canceled.
				 * Break this process 
				 */
				break ;
			}
			else if( !(test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status) )  && (test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) )
			{
				/* Restart waiting request if adapter is ONLINE */
				if(!(test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status) )  
				&& test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status)  )			/* FCLNX-GPL-038 */
				{
					break;
				}
				else if( test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status) )  	/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
				{
					break;
				}																	/* FCLNX-GPL-038 */
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					if(hfc_fx_check_cs_disable(pp, core_wk))
						continue;	/* FCLNX-GPL-FX-438 */
					if( target->core_queue[core_wk->core_no].wx_que_cnt > 0 ){					/*-- FCLNX-019 STR--*/
						hfc_fx_issue_intl_start(pp, pp->region_arg[pp->rid], core_wk, target);
					}
				}												/*-- FCLNX-019 END--*/
			}
			break ;/* FCLNX-GPL-FX-014 End */
			
#if 0			
		case HFC_FX_INT_CHECK_TMR: /* FCLNX-GPL-306 */
			HFC_DBGPRT( "hfcldd : hfc_fx_watchdog - HFC_FX_INT_CHECK_TMR is Time-out.");
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			/* Check interrupts */
			hfc_fx_intr(0, (void *)pp);
			HFC_PORTLOCK_IRQSAVE(pp,flags);

			if(pp->int_check == TRUE)
			{	/* timer restart */
				HFC_DBGPRT( "hfcldd : hfc_fx_watchdog - Restore HFC_FX_INT_CHECK_TMR.");
				hfc_fx_w_stop(pp, core, HFC_FX_INT_CHECK_TMR);
				hfc_fx_w_start(pp, core, HFC_FX_INT_CHECK_TMR, 1);
			}
			else
			{	/* timer stop */
				HFC_DBGPRT( "hfcldd : hfc_fx_watchdog - Stop HFC_FX_INT_CHECK_TMR.");
				hfc_fx_w_stop(pp, core, HFC_FX_INT_CHECK_TMR);
			}
			break;
#endif
		case HFC_FX_MLPF_ISOLEND_TMR:
			if ( (HFC_FX_MMODE_CHECK_SHARED(pp)) && (!HFC_FX_MMODE_CHECK_SHADOW(pp))) {
				hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
				hfc_fx_mlpf_isol_end_glpar(pp, hyp_status);
			}
			break;

		case HFC_FX_PATH_RETRY_TMR:
			HFC_ALLCOREUNLOCK(rp);
			HFC_PORTUNLOCK(pp);
			if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {
				for(i=0; i<MAX_ADAP_CNT; i++) /* FCLNX-GPL-177 */
				{
					wk_pp = hfc_manage_info.port_info_arg[i];
					if(wk_pp == NULL) continue;
					HFC_PORTLOCK(wk_pp);
					HFC_ALLCORELOCK(wk_pp->region_arg[wk_pp->rid]);
					hfc_manage_info.npubp->hfc_fx_deque_retry_hfcp(wk_pp, &issue_hfcp_top);
					HFC_ALLCOREUNLOCK(wk_pp->region_arg[wk_pp->rid]);
					HFC_PORTUNLOCK(wk_pp);
				}
			}
			if(issue_hfcp_top != NULL){
				HFC_DBGPRT(" hfcldd%d : hfc_fx_watchdog - call hfc_fx_retry_strategy.\n",pp->dev_minor);
				hfc_manage_info.npubp->hfc_fx_retry_strategy(issue_hfcp_top);
			}
			HFC_PORTLOCK(pp);
			HFC_ALLCORELOCK(rp);
			break;
			
	}	/*-- end of switch --*/
	
	if (HFC_FX_VPORT_EXIST(pp)) {	/* FCLNX-GPL-FX-297, 300 */
		if (pp->c_err) {
			if (pp->region_arg[0] != NULL) {
				HFC_ALLCOREUNLOCK(rp);
				/* all core_lock for rid0 */
				HFC_ALLCORELOCK(pp->region_arg[0]);
				hfc_fx_check_errcount(pp->pport);
				/* isolate done */
				HFC_ALLCOREUNLOCK(pp->region_arg[0]);
				
				HFC_ALLCORELOCK(rp);
			}
		}
	}
	else {
		hfc_fx_check_errcount(pp->pport);	/* FCLNX-GPL-349 */
	}	/* FCLNX-GPL-FX-297, 300 */

	if((atomic_read(&pp->check_mbreq))&&!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, hfcp);

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
	{
		if ( pp->retry_hfcp_top ){
			wk_hfcp = pp->retry_hfcp_top;
			hfc_fx_mp_watchdog_enter(pp, wk_hfcp->core, wk_hfcp->target, wk_hfcp, wk_hfcp->dev, wk_hfcp->lun_id, HFC_FX_PATH_RETRY_TMR, 0, TRUE);
			hfc_fx_mp_watchdog_enter(pp, wk_hfcp->core, wk_hfcp->target, wk_hfcp, wk_hfcp->dev, wk_hfcp->lun_id, HFC_FX_PATH_RETRY_TMR, 0, FALSE);
		}
	
		if ( hfc_manage_info.wait_reset_mp_fx )								/* FCLNX-0429 *//* FCLNX-GPL-FX-261 */
		{
			hfc_manage_info.npubp->hfc_fx_check_dev_reset_complete();			/* FCLNX-0429 */
			hfc_manage_info.npubp->hfc_fx_check_bus_reset_complete();			/* FCLNX-0429 */
		}
	}

	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	return;
	
}


/*
 * Function:    hfc_fx_timeout_by_reset
 *
 * Purpose:     Responce Time out of Target Reset in Bus Reset
 *
 * Arguments:   
 *  pp         - Pointer to dpp_info 
 *  target     - Pointer to arget_info 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_timeout_by_reset(												/* FCLNX-0500 */
	struct port_info		*pp,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp)
{
	int	   lun=0;
	
	if(hfcp != NULL)		/* FCLNX-0608 */
	{
		lun = hfcp->lun_id;
	}						/* FCLNX-0608 */
	
	if (test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)||test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags)){	/* FCLNX-GPL-FX-085 */
		memset( pp->logdata, 0, 16 );			/* FCLNX-0608 *//* FCLNX-GPL-391 */
		memcpy( pp->logdata, (uchar*)&hfcp->core->xob[hfcp->cmd_xob].fcp_cmd.fcp_cntl, 4 );	/* FCLNX-GPL-391 */
		hfc_fx_errlog( pp, NULL, target, hfcp, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, 0x26, pp->logdata, 16 );	/* FCLNX-GPL-391 */
	}else{
		memset( pp->logdata, 0, 16 );			/* FCLNX-0608 *//* FCLNX-GPL-391 */
		memcpy( pp->logdata, (uchar*)&hfcp->core->xob[hfcp->cmd_xob].fcp_cmd.fcp_cntl, 4 );	/* FCLNX-GPL-391 */
		hfc_fx_errlog( pp, NULL, target, hfcp, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRA, 0x29, pp->logdata, 16 );	/* FCLNX-GPL-391 */
	}	/* FCLNX-GPL-FX-085 */

#if 0
	clear_bit( HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status );

	if( hfc_fx_toutchk_xob(pp, target, hfcp, lun, HFC_ISSUE_TARGET_RESET) )
	{
		/*
		 * Timed-out xob remain in queue so do not initiate
		 * MIH-LOG and Abort-Task-Set
		 */
		return ;
	}

	if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has hpppen. */
		HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
	}
	else /* FCLNX-0608 */
	{	
		if(pp->c_err!=0) return;
		/* Failed to initiate mailbox request       */
		/* Execute Abort without collecting MIH-LOG */
		if( test_bit(HFC_TS_NEED_CANCEL_SCSI_WAITT_DMA, (ulong *)&target->status ) ){
			set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status );
			atomic_set(&pp->check_mbreq, 1);
			clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status );	/* FCLNX-GPL-038 */
		}
		if( test_bit(HFC_TS_NEED_LOGIN, (ulong *)&target->status ) )
		{
			if( hfc_fx_issue_relogin(pp,target) )
			{
				set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status );
				atomic_set(&pp->check_mbreq, 1);
				clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status );	/* FCLNX-GPL-038 */
				hfc_fx_enque_login_req(pp, target);
			}
			return ;
		}
		if(test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status ))  return ;                                            
		hfc_fx_issue_task_mgm(pp, rp->core_arg[pp->master_core_no], target, hfcp, dev, HFC_ISSUE_LOGIN) ;
		/* If initiation succeeds, set HFC_WAIT_HALT in hfc_fx_isuue_task_mgm() */
		if(pp->c_err!=0) return; 
	}			/* FCLNX-0608 */
#endif
	return;
}

/*
 * Function:    hfc_fx_occurred_mck
 *
 * Purpose:      occured Machin Check
 *
 * Arguments:
 *  pp         - Pointer to port_info structure
 *  event          - cause of MCK
 *
 * Returns:
 *
 * Notes:
 */
void hfc_fx_occurred_mck(struct port_info *pp, uchar point)
{        /* FCLNX-0533 */
#if 0
	HFC_DBGPRT("hfc_fx_mck_point() : Machine check occurred  hfcldd%d (",pp->instance);
	switch(point){
		case HFC_BEFORE_POSTCHK:
			HFC_DBGPRT(KERN_ERR "  before Post Check.  ");
			break;
		case HFC_AFTER_LINKINITIALIZE:
			HFC_DBGPRT(KERN_ERR "  after Link Initilaize. ");
			break;
		case HFC_AFTER_SCSI_HOST_RESCAN:
			HFC_DBGPRT(KERN_ERR "  after scsi host rescan. ");
			break;
		default:
			break;
		}
        HFC_DBGPRT(KERN_ERR ")\n");
//      hfc_fx_issue_forced_mck(pp, core, HFC_ABEND_T3);

        hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1, 0x08 );
#endif
        return;
}       /* FCLNX-0533 */

/*
 * Function:    hfc_fx_reset_start
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  type       - 1:ctlrst , 2:reboot , 3:f_start, 4:fw_start
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_reset_start(struct port_info *pp,uchar type)
{
	uint			wk1=0, wk2=0, wk3=0 ;
	uint			length ;  
	unsigned long long	wkp;
	int				dma_size, i, ulp, rss;
	uint			wk_reg=0; 		/* FCLNX-GPL-220 */
	uint			init_addr;
	struct core_info	*core=NULL;
	uchar			isolate_core=0;		/* FCLNX-GPL-FX-079 */
	uint			indirect_access=0, get_flag=1;	/* FCLNX-GPL-FX-079 */
	uint 			status_low	= 0;	/* FCLNX-GPL-FX-197 */

	HFC_DBGPRT(" hfcldd : hfc_fx_reset_start - start\n");

	switch( type )
	{
		case HFC_CTLRST : /* FCLNX-GPL-220 start */
			/* FIVE-FX and others */
			HFC_DBGPRT(" hfcldd : hfc_fx_reset_start - HFC_CTLRST\n");
			hfc_fx_write_reg(pp, HFC_IOSPACE_CMDRES, 0x1, 0x42); /* CTLRES *//* FCLNX-GPL-FX-221 */
			/* Dummy read *//* FCLNX-GPL-FX-221 */
			wk_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ZERO, (char)0x4);
			break ; 
			
		case HFC_REBOOT :
			HFC_DBGPRT(" hfcldd : hfc_fx_reset_start - HFC_REBOOT\n");
			/* Not execute FW reboot command when flash parity error occured. *//* FCLNX-GPL-FX-197 */
			status_low = HFCFX_MMIO_R4(pp, PORT(STATUSL));	
			if (status_low & (HFCFX_MMIO_STTL_BOOTERR | HFCFX_MMIO_STTL_FMEMBOOTEE)) {	
				return;
			}	/* FCLNX-GPL-FX-197 */
			hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDBOOT,( char )0x1,(char)0x20);
			break ;

		case HFC_F_START :
			hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1,(char)0x80);
			break ;

		case HFC_TSEQ :
			HFC_DBGPRT(" hfcldd : hfc_fx_reset_start - TSEQ\n");
			hfc_fx_write_reg_ext(pp, 0x0144, (char)0x1, 0x05);
			hfc_fx_write_reg_ext(pp, 0x003c, (char)0x1, 0x08);
			break ;
		
		case HFC_SET_INIADR :
			if (HFC_FX_VIRTUAL_PORT(pp)) {
				/* Set INIT_ADDR in Ext CCA */
				init_addr  = 0x1000;
				init_addr += 0x80 * pp->rid;
				
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if (pp->region_arg[pp->rid] != NULL) {
						hfc_fx_write_reg_ext(pp, (uint)init_addr + i*0x20 + 0x10, (char)0x04,
							((pp->region_arg[pp->rid]->core_arg[i]->padr_init) >> 32));
						hfc_fx_write_reg_ext(pp, (uint)init_addr + i*0x20 + 0x14, (char)0x04,
							(pp->region_arg[pp->rid]->core_arg[i]->padr_init));
					}
				}
			}
			else {
				dma_size = sizeof(dma_addr_t);
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							core = pp->region_arg[pp->rid]->core_arg[i];
							wkp = core->padr_init;
							if(dma_size == 8){
								wkp >>=32;
							}
							else{
								wkp = 0;
							}
							hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_CA_INIT_ADDR0,
								0x4, wkp, HFC_FX_CORE_OFFSET80);
							hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_CA_INIT_ADDR1,
								0x4, core->padr_init, HFC_FX_CORE_OFFSET80);
						}
					}
				}
			}
			break ;
		case HFC_SET_WS80 :
			hfc_fx_write_reg(pp, ( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x80);		
			break ;
		case HFC_SET_WS40 :
			hfc_fx_write_reg(pp, ( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x40);		
			break ;
		case HFC_SET_WS04 :    
			hfc_fx_write_reg(pp, ( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x04);		
			break ;
		case HFC_INI_RESET :
			hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDRES,( char )0x1,(char)0x04);
			/* 1ms dummy wait after iniRes */
			wk1 = 0;
			udelay(1000); /* 1ms wait */
			break ;
		case HFC_WSCA_CLEAR :
			/* clear 128 bytes from PCI address 0x300/0x380/0x400/0x480 */
			length = 128;
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				wk2 = 0x300+0x80*i;
				for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )                    
				{
					hfc_fx_write_reg_ext(pp,( uint )wk1,( char )0x4,0);
				}
			}
			
			/* clear 1024 bytes from PCI 0x1000/0x1400/0x1800/0x1c00 */
			length = 0x20;
			for(ulp=0; ulp<MAX_REGION_PROBE; ulp+=1){		/* FCLNX-GPL-FX-078 */
				wk3 = 0x1000+0x80*ulp;
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					wk2 = wk3 + 0x20*i;
					for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )                    
					{
						hfc_fx_write_reg_ext(pp,( uint )wk1,( char )0x4,0);
					}
				}
			}												/* FCLNX-GPL-FX-078 */
			break;
		
		case HFC_SET_ISOLATE_CORE :							/* FCLNX-GPL-FX-079 */
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						core = pp->region_arg[pp->rid]->core_arg[i];
						if( hfc_fx_check_cs_disable(pp, core) ){	/* FCLNX-GPL-FX-438 */
							isolate_core |= (0x80 >> core->pcore_no);
						}
					}
				}
			}
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
//				HFC_DBGPRT("hfcldd%d hfc_fx_reset_start HFC_SET_ISOLATE_CORE core#%d isolate_core = %02x\n",
//					pp->dev_minor, i, (uchar)isolate_core);
				wk2 = 0x35c+0x80*i;
				hfc_fx_write_reg_ext(pp,( uint )wk2,( char )0x1,isolate_core);
			}
			break;											/* FCLNX-GPL-FX-079 */
		
		case HFC_UTL_REG_CLEAR :							/* FCLNX-GPL-FX-079 */
			/* Set indirect access flag */
//			HFC_DBGPRT("hfcldd%d hfc_fx_reset_start HFC_UTL_REG_CLEAR\n",pp->dev_minor);
			i=0;
			while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
			{
				/* 1ms wait */
				mdelay(1); 

				if( i == 100 ){ /* 100msec */
					HFC_DBGPRT("hfcldd%d hfc_fx_reset_start Does not Get enable indirect access flag.\n",pp->dev_minor);
					get_flag = 0;
					break;
				}
				i++;
			}
	
			if( get_flag == 1 ){
				HFC_DBGPRT("hfcldd%d hfc_fx_reset_start Enable indirect access flag.\n",pp->dev_minor);
	
				/* Enable indirect access flag */
				hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x08);

				/*** flag check ***/ 
				i=0;
				while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
				{
					/* 1ms wait */
					mdelay(1); 

					if( i == 100 ){ /* 100msec */
						/* Clear Flag */
						hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00);	/* FCLNX-GPL-FX-162 */
						HFC_DBGPRT("hfcldd%d hfc_fx_reset_start Clear enable indirect access flag.\n",pp->dev_minor);
						get_flag = 0;
						break;
					}
					i++;
				}
			}
			
			if( get_flag == 1 ){
				HFC_DBGPRT("hfcldd%d hfc_fx_reset_start Get enable indirect access flag.\n",pp->dev_minor);
				indirect_access = 1;
			}
			
			if( indirect_access == 1 ){
				/* Clear Upstream Bridge Status Register */
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK, (char)0x1, 0x20);
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR, (char)0x4, 0x0d8000bc);	/* FCLNX-GPL-FX-162 */
				hfc_fx_write_reg(pp, HFC_IOSPACE_INDAREA,(char)0x4, 0xffffffff);
				
				/* Clear UTL PCI Express Port Status Register */
//				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK, (char)0x1, 0x20);
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR, (char)0x4, 0x0d8000a4);	/* FCLNX-GPL-FX-162 */
				hfc_fx_write_reg(pp, HFC_IOSPACE_INDAREA,(char)0x4, 0xffffffff);
				
				/* Clear UTL PCI Express Port Interrupt Enable Register *//* FCLNX-GPL-FX-145 */
//				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK, (char)0x1, 0x20);
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR, (char)0x4, 0x0d8000ac);	/* FCLNX-GPL-FX-162 */
				hfc_fx_write_reg(pp, HFC_IOSPACE_INDAREA,(char)0x4, 0x00100000);
				
				/* Clear indirect access flag */
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,  0x01, 0x00);	
				hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,  0x04, 0x80000000);	
				hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00); 
			}
			break;											/* FCLNX-GPL-FX-079 */
		
		case HFC_RECEIVE_CTL_FLAG_CLEAR :					/* FCLNX-GPL-FX-079 */
			switch( pp->core_num ){
				case 1:		/* 4 ports card */
					if( pp->port_no == 0 ){
						wk2 = 0x4dd0;
					}
					else if( pp->port_no == 1 ){
						wk2 = 0x4df0;
					}
					else if( pp->port_no == 2 ){
						wk2 = 0x4fc0;
					}
					else if( pp->port_no == 3 ){
						wk2 = 0x4fd0;
					}
					length = 16;
					for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )                    
					{
//						HFC_DBGPRT("hfcldd%d hfc_fx_reset_start HFC_RECEIVE_CTL_FLAG_CLEAR wk1 = %08x\n",pp->dev_minor, wk1);
						hfc_fx_write_reg_ext(pp,( uint )wk1,( char )0x4,0);
					}
					break;
				case 2:		/* 2 ports card */
					if( pp->port_no == 0 ){
						wk2 = 0x4dd0;
					}
					else if( pp->port_no == 1 ){
						wk2 = 0x4fc0;
					}
					length = 16;
					for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )                    
					{
//						HFC_DBGPRT("hfcldd%d hfc_fx_reset_start HFC_RECEIVE_CTL_FLAG_CLEAR wk1 = %08x\n",pp->dev_minor, wk1);
						hfc_fx_write_reg_ext(pp,( uint )wk1,( char )0x4,0);
					}
					break;
				case 4:		/* 1 port card */
					wk2 = 0x4dd0;
					length = 16;
					for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )                    
					{
//						HFC_DBGPRT("hfcldd%d hfc_fx_reset_start HFC_RECEIVE_CTL_FLAG_CLEAR wk1 = %08x\n",pp->dev_minor, wk1);
						hfc_fx_write_reg_ext(pp,( uint )wk1,( char )0x4,0);
					}
					break;
			}
			break;											/* FCLNX-GPL-FX-079 */

		case HFC_SET_MLPF_MODE :                                            /* FCLNX-0379 */
			if( HFC_FX_MMODE_CHECK_SHARED (pp) ){
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_ext(pp,( uint )(0x325+0x80*i),( char )0x1,(uchar)0x80);	/* FCLNX-GPL-FX-380 */
						}
					}
				}
			}
			break;
			
		case HFC_GR_CLEAR : /* FCLNX-GPL-220 */
			/* Set ER'PTYP */ /* FCLNX-GPL-231 */
			if( !(HFC_FX_MMODE_CHECK_SHARED (pp)) ){	/* FCLNX-GPL-399 */
				hfc_fx_write_reg_ext(pp, 0x110, (char)0x02, 0x0404);
			}else{
				hfc_fx_write_reg_ext(pp, 0x110, (char)0x01, 0x04);
				hfc_fx_write_reg_ext(pp, 0x111, (char)0x01, 0x04);
			}										/* FCLNX-GPL-399 */
			
			/* Fill all standing bits, from 0x1040 to 0x107f (64byte) */
			for( wk_reg = 0x1040 ; wk_reg < 0x1080 ; wk_reg+=0x4 )
			{
				hfc_fx_write_reg_ext(pp, (uint)wk_reg, 0x4, (uint)0xffffffff); /* FCLNX-GPL-254 */
			}
			break;
		case HFC_RESET_ALL_INT :/* Reset all interrupt factor *//* FCLNX-GPL-FX-262,272 */
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						for (rss=0 ; rss< HFC_FX_MSIX_NVEC ; rss++) {
							hfc_fx_write_reg_rss_core(pp, i, rss, HFC_IOSPACE_RSS_INTA_RST,
								0x4, 0xffffffff, HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
						}
					}
				}
			}
			
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
						  (char)0x4, 0xffffffff, HFC_FX_CORE_OFFSET10);
					}
				}
			}
			break;
	}

}	


/*
 * Function:    hfc_fx_issue_forced_mck
 *
 * Purpose:     Issue forced MCK and initiate timer of all ports for supervision.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *  = 0        - Forced MCK initiation succeeded 
 *  !=0        - Forced MCK initiation failed 
 *
 * Notes:       Lock mpap->lock<HFC_MLOCK_MCKTMR_ENTRY to initiate timer. 
 */
int hfc_fx_issue_forced_mck(struct port_info *pp, struct core_info *core, uchar type)					/* FCLNX-0279 */
{
	struct port_info		*vpp=NULL;
	uint					wk_reg=0, mb_code=0;
	uint					addr = 0, core_no=0;
	uint					i;
	
	HFC_DBGPRT(" hfcldd : hfc_fx_issue_forced_mck - start\n");
	
	if( core != NULL ){
		core_no = core->core_no;
	}
	
	if( !test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->pport->status )
	 && !test_bit( HFC_PS_ISOL, (ulong *)&pp->pport->status ) )
	{
		if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->pport->status) ){
			/* skip issue forced mck */
			HFC_DBGPRT(" hfc_fx_issue_forced_mck() - skip issue forced mck\n");
		}																														/* FCLNX-GPL-034 */
		else {
			for (i=0; i<=pp->pport->max_vport_count; i++) {
				vpp = pp->pport->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				
				set_bit( HFC_PS_WAIT_MCKINT, (ulong *)&vpp->status );
			}
			
			hfc_fx_w_stop(pp->pport, pp->pport->region_arg[0]->core_arg[core_no], HFC_FX_MCKINT_TMR) ;
			hfc_fx_w_start(pp->pport, pp->pport->region_arg[0]->core_arg[core_no], HFC_FX_MCKINT_TMR, HFC_FX_MCKINT_TO) ;
			
			if (core != NULL) {
				HFC_4B_TO_4L(mb_code, core->mb->mb_init.mb_code);
				wk_reg = (uint)(mb_code >> 16);
				
				addr = 0x32c+0x80*core_no;
				hfc_fx_write_reg_ext(pp->pport,( uint )addr,( char )0x2, (ushort)wk_reg);
			}
			
			wk_reg = 0;
			wk_reg |= type & 0xff;
			
			addr = 0x326+0x80*core_no;
			hfc_fx_write_reg_ext(pp->pport,( uint )addr,( char )0x1,(uchar)wk_reg);

			hfc_fx_write_reg(pp->pport,( uint )HFC_IOSPACE_CMDCTL,( char )0x1, 0x08 );
		}
	}

	return (1);
}																			/* FCLNX-0279 */


/*
 * Function:    hfc_fx_reset_port_info
 *
 * Purpose:     Initiate port_info
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_reset_port_info(
	struct port_info        *pp)
{
	struct core_info	*core=NULL;
	uint                lp, i;
//	struct port_info	*ap=pp;

	HFC_DBGPRT(" hfcldd : hfc_fx_reset_port_info - start\n");
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core = pp->region_arg[pp->rid]->core_arg[i];
		if( core != NULL ){
			HFC_DBGPRT(" hfcldd : hfc_fx_reset_port_info - core=%d\n",core->core_no);
			for(lp=0 ; lp<pp->xob_max ; lp++)       /*--    FCWIN-0071      --*/
			{
				core->xob[lp].flag = 0 ;
				core->xob[lp].skip = 0 ;
			}
	
			for(lp=0 ; lp<pp->xrb_max ; lp++)       /*--    FCWIN-0071      --*/
			{
				core->xrb[lp].xcrbchk.valid = 0 ;
				core->xrb[lp].xcrb.skip = 0 ;				/*--    FCLNX-GPL-038   --*/
			}
	
			core->mb_retry_cnt = 0 ;
//			core->xob_no = 0 ;
//			core->xrb_no = 0 ;
			core->iov_no = 0 ;
//			pp->iov_map_cnt = 0 ;					/*--	LINUX-049		--*/
//			core->xob_exec_cnt = 0 ;
//			core->xob_wait_exec_cnt = 0 ;
			core->drv_next_xob = 0;
			core->drv_next_xrb = 0;
	
			for(lp=0 ; lp<MAX_FX_FRAME_CNT ; lp++){
				core->frame_start_xob[lp].start = 0;
				core->frame_start_xob[lp].num = 0;
				core->frame_start_xob[lp].pkt_no = 0;
//				core->xob_outp_end[lp] = 0 ;
			}
			core->frame_inp = 0 ;
			core->post_err_cnt = 0;
//			core->frame_chkp = 0 ;
		}
	}
	
	pp->next_tstart = NULL ;
//	pp -> plogi_target = NULL ;
//	pp -> prli_target = NULL;
	pp->next_gidpn = FALSE ;                /*--    FCLNX-GPL-038      --*/
	pp->mck_core_no = 0;
	pp->flogi_retry_change = 0;				/* FCLNX-GPL-FX-179 */
	
	/* clear port_status_detail1 */
	clear_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_CORE_START, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_LINK_INI, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_FLOGI, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_FLOGI, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_ADD_PORTID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_SCR, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_SCR, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_PLOGI_N, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_PLOGI_N, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_RFTID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_RFTID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_RFFID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_RFFID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_LOGO_FCSW, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1 );
	clear_bit(HFC_PD_MB_DELAY, (ulong *)&pp->status_detail1 );
	
	/* clear port_status_detail2 */
	clear_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_GPNFT, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_NEED_OFFLINE_MB, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_OFFLINE_MB, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_MIHLOG, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_NEED_LOAD_CH_TRACE, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_LOAD_CH_TRACE, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_NEED_DIAG, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_DIAG, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);	/* FCLNX-GPL-FX-146 */

	return;
}


/*
 * Function:    hfc_fx_timeout_by_scnlinkup
 *
 * Purpose:     Process when HFC_FX_LINKUP_TMR is timed out.
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *  target     - Pointer to target_info_fx 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_timeout_by_scnlinkup(
	struct port_info		*pp,
	struct target_info_fx	*target)
{
	uint	i;
	struct core_info	*core=NULL;

	HFC_DBGPRT( "hfc_fx_timeout_by_scnlinkup() - timeout.");

//	if((hfc_manage_info.hfcplus_enable)&&(pp->ldl_errcnt_info!=NULL)){	/* FCLNX-GPL-327 */
	/* FCLNX-GPL-FX-318 */
	if(pp->ldl_errcnt_info!=NULL){										/* FCLNX-GPL-349 */
		hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, target, HFC_OCCURED_FAILURE, HFC_TGT_LDL_ERR);
	}																	/* FCLNX-GPL-327 */
	/* FCLNX-GPL-FX-318 */

	target->link_recovered = 1;											/* FCLNX-GPL-334 */

#if !( defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
	clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target -> status );
#else
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target -> status );
#endif

	/* Discards XRB when SCN_WLINKUP timer timed out 	*/

	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		hfc_fx_cancel_scsi_cmd(pp, core, target, 0, NULL, SCS_SCN_LINKDOWN, 		/* FCLNX-GPL-038 */
			HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);			/* FCLNX-GPL-038 */
	}
	if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
		hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_SCN_LINKDOWN, HFC_CSCSI_ERROR,
			TRUE, TRUE, FALSE, FALSE, TRUE, HFC_FLASH_TARGET);
	}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target -> status );
#endif

	/* Invalidates this target device					*/

	hfc_fx_clear_target_info_fx( pp, target, TRUE );				/* FCLNX-GPL-038 */
	if (HFC_FX_MQ_VALID(pp))	/* FCLNX-GPL-FX-270, 275 */
		hfc_fx_mq_change_target_info(pp, target);
}


/*
 * Function:    hfc_fx_timeout_by_restart
 *
 * Purpose:     The task management is reactivated
 *
 * Arguments:   
 *  pp         - Pointer to port_info structure
 *  target     - 
 *  hfcp       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_timeout_by_restart(								/* FCLNX-0500 */
	struct port_info        *pp,
	struct target_info_fx 	*target,
	struct dev_info_fx 		*dev,
	struct hfc_pkt_fx		*hfcp)
{
	
	if(target != NULL)
	{
		if( test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status ) ){		/* FCLNX-GPL-036 */
			/* Target Reset is requested.  */
			hfc_fx_issue_task_mgm(pp, pp->region_arg[pp->rid]->core_arg[pp->master_core_no], target, hfcp, dev, HFC_ISSUE_TARGET_RESET);		/* FCLNX-GPL-036 *//* FCLNX-GPL-328 */
			return;
		}
		if((dev != NULL)&&(test_bit( HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat ))){
			/* LUN Reset is requested.  */
			hfc_fx_issue_task_mgm(pp, pp->region_arg[pp->rid]->core_arg[pp->master_core_no], target, hfcp, dev, HFC_ISSUE_LUN_RESET);		/* FCLNX-GPL-036 *//* FCLNX-GPL-328 */
		}
	}
}


/*
 * Function:    hfc_fx_reset_watchdog
 *
 * Purpose:     Release all executing timers
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_reset_watchdog(struct port_info *pp)
{

	uint                            lp;
	struct target_info_fx           *target;
	struct dev_info_fx				*dev=NULL;				/* FCLNX-GPL-038	*/
	struct core_info				*core=NULL;

	HFC_DBGPRT("hfcldd%d : hfc_fx_reset_watchdog\n",pp->dev_minor);

	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_REBOOT_DELAY_TMR, 0, TRUE);
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_LINKUP_TMR, 0, TRUE);
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_LOGIN_DELAY_TMR, 0, TRUE);				/* FCLNX-GPL-038 */
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_MLPF_ISOLEND_TMR, 0, TRUE);
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_MB_DELAY_TMR, 0, TRUE);

	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)			/* FCWIN-0083 */
	{
		target = hfc_fx_hash_target_valid(pp, lp);	/* FCLNX-GPL-FX-112 */
		if (target != NULL)
		{
			hfc_fx_watchdog_enter(pp, NULL, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);
			hfc_fx_watchdog_enter(pp, NULL, target, NULL, 0, HFC_FX_DELAY_TMR, 0, TRUE);
//			hfc_fx_watchdog_enter(pp, NULL, target, NULL, 0, HFC_FX_WEXEC_TMR, 0, TRUE);
			hfc_fx_watchdog_enter(pp, NULL, target, NULL, 0, HFC_FX_RESTART_TMR, 0, TRUE);		/* FCLNX-GPL-328 */
			dev = target->dev;										/* FCLNX-GPL-038	*/
			if(dev != NULL) {
				/* stop LUN Reset Delay Timer */
//				hfc_manage_info.npubp->hfc_fx_all_clear_dev_info_fx( pp, dev );			/* FCLNX-GPL-0343 */
				hfc_fx_all_clear_dev_info_fx( pp, dev );									/* FCLNX-GPL-0343 */
			}														/* FCLNX-GPL-038	*/
		}
	}
	
	for(lp=0; lp<MAX_CORE_PROBE_FX; lp+=MAX_CORE_PROBE_FX/pp->core_num)
	{
		core = pp->region_arg[pp->rid]->core_arg[lp];
		if( core != NULL ){
			hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_WEXEC_TMR, 0, TRUE);
			hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0, HFC_FX_MB_RSP_TMR, 0, TRUE);
		}
	}

}


/*
 * Function:    hfc_fx_toutchk_xob 
 *
 * Purpose:     Search timed-out requests remain in xob.
 *              If so, force FW to forced MCK status 
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  hfcp       - Pointer to hfc_pkt_fx
 *  lun        - lun number
 *  mode       - 
 *
 * Returns:     
 *  0          - No timed-out requests remain in xobs
 *  -1         - One or more timed-out requests remain in xobs.
 *
 * Notes:       
 */
int hfc_fx_toutchk_xob(
	struct port_info		*pp,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	uint					lun,			/* FCLNX-GPL-0343 */
	uchar					mode)
{
	struct core_info		*core;
	int i;
	
	HFC_ENTRY("hfc_fx_toutchk_xob");
	
	if (hfcp == NULL)
		return(0);
	
	if (hfcp->core == NULL)
		return(0);
	
	core = hfcp->core;
	
	if ( (mode == HFC_ISSUE_ABORT)
	  && (hfcp->lun_id != lun) ) {
		return(0);
	}
	
	for(i=0; i < pp->xob_max; i++)
	{
		if( hfcp == (struct hfc_pkt_fx *)(ulong)core->xob[i].drv_work ) {
			if( core->xob[i].flag & HFC_XOB_VALID ){
				if( !(core->xob[i].skip & HFC_XOB_SKIP) ){
					/* Timed-out requests remains in xobs */
					HFC_DBGPRT("hfcldd%d : hfc_fx_toutchk_xob timeout pkt results in xob", pp->dev_minor);
					
					memset(core->logdata,0,16);
					core->logdata[0] = i;
					core->logdata[1] = core->core_no;
					HFC_2L_TO_2B(core->logdata[2], hfcp->pkt_no);
					HFC_4L_TO_4B(core->logdata[4], hfcp->cmd_flags);
					core->logdata[8] = lun;
					core->logdata[9] = hfcp->target_id;
					core->logdata[10] = mode;
					hfc_fx_errlog(
						pp, core, NULL, hfcp, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4,
						0x8d, core->logdata, 16) ;
					hfc_fx_abend(pp, core, HFC_ABEND_TOUTCHK_XOB);
					return(-1) ;
				}
			}
		}
	}
	
	HFC_EXIT("hfc_fx_toutchk_xob");
	
	return(0);
}

int hfc_fx_force_linkdown( struct port_info *pp, uchar proc)				/* FCLNX-GPL-147 *//* FCLNX-GPL-FX-043 */
{
	uint					i;									/* FCLNX-GPL-428 */
	struct port_info		*vpp;

	HFC_ENTRY("hfc_fx_force_linkdown");                 //FCLNX-0488
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FORCE_ISOL, 0x00, pp, NULL, NULL, NULL, NULL,
		proc, 0, 0);

	if( test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ) ) {		/* FCLNX-GPL-352 */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_FORCE_ISOL, 0x04, pp, NULL, NULL, NULL, NULL,
			0, 0, 0);
		return EINVAL;
	}
	
	if ( test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ) {
		hfc_fx_hand2_trace(
			HFC_FX_TRC_FORCE_ISOL, 0x02, pp, NULL, NULL, NULL, NULL,
			0, 0, 0);
		return EINVAL;
	}
	
	if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has hpppen. */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_FORCE_ISOL, 0x09, pp, NULL, NULL, NULL, NULL,
			0, 0, 0);	/* FCLNX-0707 */
		HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
		return EINVAL;
	}
	
	if (proc == TRUE) {
		if (test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) {
			if (test_bit(HFC_PD_ISOLATE_PORT_C, (ulong *)(ulong *)&pp->status_detail2)){
				hfc_fx_hand2_trace(
					HFC_FX_TRC_FORCE_ISOL, 0x05, pp, NULL, NULL, NULL, NULL,
					0, 0, 0);
				return 0;
			}
		}
	}
	else {
		if (HFC_FX_MMODE_CHECK_SHARED(pp)) {	/* FCLNX-GPL-421 */
			if (test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) {
				hfc_fx_mlpf_set_led(pp, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 *//* FCLNX-GPL-435 */
				if (test_bit(HFC_PD_ISOLATE_PORT_C, (ulong *)(ulong *)&pp->status_detail2)){
					hfc_fx_errlog(
						pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0x8E, pp->logdata, 16);

					HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
						pp->pci_cfginf->bus->number,
						PCI_SLOT(pp->pci_cfginf->devfn),
						PCI_FUNC(pp->pci_cfginf->devfn));
				}
				return EINVAL;
			}
		}								/* FCLNX-GPL-421 */
			
		/* This port has already isolated, but the adapter has not isolated yet */
		if (test_bit ( HFC_PS_ISOL, (ulong *)&pp->status)) {
			hfc_fx_hand2_trace(
				HFC_FX_TRC_FORCE_ISOL, 0x08, pp, NULL, NULL, NULL, NULL,
				0, 0, 0);
			return EINVAL;
		}
	}

	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				if ( HFC_FX_MMODE_CHECK_SHARED(pp) ){
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  (char)0x4, (int)HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
				}else{
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  (char)0x4, (int)(0x00000000), HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}
	
	for (i=0; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		hfc_fx_reset_all_timer(vpp);
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FORCE_ISOL, 0x20, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);
	
	/* Issue forced machine check request */
	hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1, 0x08 );
	
	/* Set HFC_HWWISOL to mp_adap_info */
	set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
	hfc_fx_change_vport_isol_state(pp);
	
	if (proc == TRUE){
		set_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 );
	}else{
		set_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&pp->status_detail2 );
	}
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FORCE_ISOL, 0x50, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);
	
	hfc_fx_chk_stop(pp);
	
	for (i=0; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		vpp->status = 0;
		
		set_bit(HFC_PS_ENABLE,  (ulong *)&vpp->status);	/* FCLNX-GPL-572 */
		set_bit(HFC_PS_ISOL, (ulong *)&vpp->status);	/* FCLNX-GPL-572 */
	}
	
	hfc_fx_change_vport_isol_state(pp);
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FORCE_ISOL, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);

	HFC_EXIT("hfc_fx_force_linkdown");                  //FCLNX-0488
	return 0;                                      //FCLNX-0488
}				/* FCLNX-GPL-147 */


/*
 * Function:    hfc_fx_force_linkdown_recovery
 *
 * Purpose:     Recovery for isolated Aappter port
 *
 * Arguments:
 *  pp         -
 *  err_no     -
 *  mode       -
 *
 * Returns:
 *
 * Notes:
 */
int hfc_fx_force_linkdown_recovery(struct port_info *pp){				/* FCLNX-GPL-147 */

	uint				i,j;
	struct core_info	*core=NULL;
	struct port_info	*vpp;
	uint				status_low = 0;		/* FCLNX-GPL-FX-197 */

	HFC_ENTRY("hfc_fx_force_linkdown_recovery");					/*FCLNX-0506*/
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FORCE_ISOL_REC, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);
	
	if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has hpppen. */
		hfc_fx_hand2_trace(
			HFC_FX_TRC_FORCE_ISOL_REC, 0x05, pp, NULL, NULL, NULL, NULL,
			0, 0, 0);
		HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
		return EINVAL;
	}

	/* This adapter is not an isolated state */
	if (!test_bit ( HFC_PS_ISOL, (ulong *)&pp->status)){
		hfc_fx_hand2_trace(
			HFC_FX_TRC_FORCE_ISOL_REC, 0x03, pp, NULL, NULL, NULL, NULL,
			0, 0, 0);
		return EINVAL;
	}

	/* Not execute isol recovery when flash parity error occured. */
	status_low = HFCFX_MMIO_R4(pp, PORT(STATUSL));	/* FCLNX-GPL-FX-197 */
	if (status_low & (HFCFX_MMIO_STTL_BOOTERR | HFCFX_MMIO_STTL_FMEMBOOTEE)) {	
		return EINVAL;
	}	/* FCLNX-GPL-FX-197 */

	/* Adapter isolation recovery process is in progress */
	clear_bit ( HFC_PS_ISOL, (ulong *)&pp->status);
	
	for (j=0; j<=pp->max_vport_count; j++) {
		vpp = pp->vport_ptr[j].vport_arg;
		if (vpp == NULL)
			continue;
		
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/vpp->core_num){	/* FCLNX-GPL-FX-060 */
			if(vpp->region_arg[vpp->rid] != NULL){
				if ((core = vpp->region_arg[vpp->rid]->core_arg[i]) != NULL){
					clear_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
					if( HFC_FX_MMODE_CHECK_SHADOW(vpp) ){	/* FCLNX-GPL-FX-405 */
						if(test_bit(HFC_CS_CHK_STOP, (ulong *)&core->status))
							hfc_fx_write_hg_reg_core(pp, i, (uint)HFC_IOHGSPC_CMD_REG0,
							  (char)0x4, ( int )HFC_MLPF_CMD_CSTPCLEAR, HFC_FX_CORE_OFFSET40);
					}	/* FCLNX-GPL-FX-405 */
					if (HFC_FX_PHYSICAL_PORT(vpp)) {
						// Reset CHKSTOP command
						hfc_fx_write_reg_core(vpp, i, (uint)HFC_IOSPACE_CORE0_CMD2,
							 (char)0x1, 0x01, HFC_FX_CORE_OFFSET100);
						if( HFC_FX_MMODE_CHECK_SHADOW(vpp) ){
							HFC_DBGPRT("hfcldd%d hfc_fx_force_linkdown_recovery - HFC_FX_MMODE_CHECK_SHADOW1\n",pp->dev_minor);
							hfc_fx_write_hg_reg_core(vpp, i, (uint)HFC_IOHGSPC_CMD_REG0,
							  (char)0x4, ( int )HFC_MLPF_CMD_CSTPCLEAR, HFC_FX_CORE_OFFSET40);
						}
					}
				}
			}
		}	/* FCLNX-GPL-FX-060 */
	}
	
	HFC_DETAIL_CLEAR_ISOLREC(pp);
	pp->c_err = 0x00;
	set_bit ( HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2);
	hfc_fx_change_vport_isol_state(pp);
	
	/* FCLNX-GPL-FX-090 Start */
	/* Clear Registers for FIVE-FX */
	if(pp->pkg.type == HFC_PKTYPE_FIVE_FX){ /* Check pkg.type */
		/* Clear Config Register Sticky bit */
		hfc_fx_clear_sticky_bit(pp);
	}
	
	/* Set ERPTYP internal loop state */
	hfc_fx_write_reg(pp, HFC_IOSPACE_PTYP0, 0x1, 0x04);
	
	/* Reset UTL Register */								/* FCLNX-GPL-FX-079 */
	hfc_fx_reset_start(pp, HFC_UTL_REG_CLEAR);				/* FCLNX-GPL-FX-079 */
	
	/* Recovery Reset (CTLRES) */
	hfc_fx_reset_start(pp, HFC_CTLRST);
	/* FCLNX-GPL-FX-090 End */
	
	/* Issue control reset */
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL,0, HFC_FX_CTLRST_DELAY_TMR, 0, TRUE);   /* FCLNX-0276 */
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_CTLRST_DELAY_TMR, 1, FALSE);  /* FCLNX-0276 */
	
	hfc_fx_hand2_trace(
		HFC_FX_TRC_FORCE_ISOL_REC, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);

	HFC_EXIT("hfc_fx_force_linkdown_recovery");	/*FCLNX-0506*/
	return (0);
}				/* FCLNX-GPL-147 */


/*
 * Function:    hfc_fx_hand2_trace
 *
 * Purpose:     Collection of trace
 *
 * Arguments:   
 *  id         - 
 *  sub_id     - 
 *  pp         - 
 *  target     - 
 *  hfcp       - 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_hand2_trace(
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	uint64_t				etc1,
	uint64_t				etc2,
	uint64_t				etc3)
{
	uchar					trc_wk[128] ;
	struct err_fx_trc1		*trc1 ;
	struct err_fx_trc2		*trc2 ;
	struct err_fx_trc3		*trc3 ;
	struct err_fx_trc4		*trc4 ;
	struct err_fx_trc5		*trc5 ;
	struct err_fx_trc6		*trc6 ;
	struct err_fx_trc7		*trc7 ;
	struct err_fx_trc8		*trc8 ;
	struct scsi_cmnd    	*cmnd = NULL;
	struct scsi_device		*sdev=NULL;
	struct request_queue	*rq=NULL;
	uint					etc1_wk=0, etc2_wk=0, etc3_wk=0, wdog_time;
	ushort					etc2_uwk, timer_id;
	uchar					*ptr;
	uchar					buf[4];
	uchar					port_trace=0;
	
	uint					int_a_reg=0;

	memset(trc_wk,0,128) ;
	etc1_wk = (uint)etc1;
	etc2_wk = (uint)etc2;
	etc2_uwk = (ushort)etc2;
	if(hfcp != NULL ) cmnd = hfcp->cmd_pkt;

/*----------------------------------------------*/
/*                  TRACE3                      */
/* etc1 = xrb_cnt                               */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	if( id == HFC_FX_TRC_XRBRSP )
	{/*-- trace format 3 --*/
		trc3 = (struct err_fx_trc3 *)trc_wk ;
		
		trc3->id = id ;
		trc3->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc3->seq_no, pp->seq_no);
			trc3->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc3->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc3->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc3->a_scsi_id[0], &buf[1], 3);
		}
		if (rp != NULL)
		{
			trc3->r_rid = rp->rid;
		}
		if (core != NULL)
		{
			trc3->c_core_no = core->core_no;
			HFC_2L_TO_2B(trc3->c_drv_next_xob, core->drv_next_xob);
			HFC_2L_TO_2B(trc3->c_drv_next_xrb, core->drv_next_xrb);
			HFC_8L_TO_8B(trc3->c_scsi_exec_cnt, core->scsi_exec_cnt);
			HFC_8L_TO_8B(trc3->c_scsi_end_cnt, core->scsi_end_cnt);
			
			if (hfcp != NULL) {
				HFC_LP_TO_BP(trc3->hfcp, hfcp);
			}
			
			ptr = (char *)&core->xrb[etc2].xcrb;
			HFC_MEMCPY(trc3->xrb, ptr, 28);
			ptr = (char *)&core->xrb[etc2].resp_iu1.retry_delay;
			HFC_MEMCPY(&trc3->xrb[28], ptr, 8);
			ptr = (char *)&core->xrb[etc2].fcp_info[8];
			HFC_MEMCPY(&trc3->xrb[36], ptr, 28);
		}
		
		HFC_2L_TO_2B(trc3->xrb_cnt, etc1);
	}/*-- trace format 3 --*/
	
/*----------------------------------------------*/
/*                  TRACE4                      */
/* etc1 = cmnd                                  */
/* etc2 = hfc_pkt_fx                            */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	else if((id == HFC_FX_TRC_SCSI_CHK)	||
			(id == HFC_FX_TRC_MGM_CHK)	||
			(id == HFC_FX_TRC_LINK_CHK)	||
			(id == HFC_FX_TRC_DEQ_WE))
	{/*-- trace format 4 --*/
		trc4 = (struct err_fx_trc4 *)trc_wk ;
		
		trc4->id = id ;
		trc4->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc4->seq_no, pp->seq_no);
			trc4->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc4->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc4->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc4->a_scsi_id[0], &buf[1], 3);
		}
		if (rp != NULL)
		{
			trc4->r_rid = rp->rid;
		}
		if (core != NULL)
		{
			trc4->c_core_no = core->core_no;
			
			if (target != NULL) {
				HFC_2L_TO_2B(trc4->ct_wx_que_cnt, target->core_queue[core->core_no].wx_que_cnt);
				HFC_2L_TO_2B(trc4->ct_we_que_cnt, target->core_queue[core->core_no].we_que_cnt);
			}
			
			HFC_2L_TO_2B(trc4->c_drv_next_xob, core->drv_next_xob);
			HFC_2L_TO_2B(trc4->c_drv_next_xrb, core->drv_next_xrb);
			HFC_8L_TO_8B(trc4->c_scsi_exec_cnt, core->scsi_exec_cnt);
			HFC_8L_TO_8B(trc4->c_scsi_end_cnt, core->scsi_end_cnt);
			
			if (hfcp != NULL) {
				HFC_LP_TO_BP(trc4->hfcp, hfcp);
			}
		}
		if (target != NULL) {
			trc4->t_flag = (uchar)target->flags;
			HFC_4L_TO_4B(trc4->t_status, target->status);
			trc4->t_id = (uchar)target->target_id;
			HFC_4L_TO_4B(buf, target->scsi_id);
			HFC_MEMCPY(&trc4->t_scsi_id[0], &buf[1], 3);
		}
		if( hfcp != NULL)
		{
			trc4->h_cmd_flags = hfcp->cmd_flags;
			HFC_2L_TO_2B(trc4->h_adap_status, hfcp->adap_status);
			HFC_4L_TO_4B(trc4->h_iov_no, hfcp->iov_no);
			HFC_4L_TO_4B(trc4->h_iov_cnt, hfcp->iov_cnt);
			trc4->h_cmd_xob = hfcp->cmd_xob;
		}
		if( cmnd != NULL )
		{
			trc4->cmnd0 = (uchar)cmnd->cmnd[0];
			HFC_4L_TO_4B(trc4->retries, cmnd->retries);
			HFC_4L_TO_4B(trc4->allowed, cmnd->allowed);
			HFC_MEMCPY(&trc4->cmnd[0], cmnd->cmnd, 16);
			/* kernel 5.x+: sdb.resid → resid_len; serial_number removed */
			HFC_4L_TO_4B(trc4->resid, cmnd->resid_len);
			trc4->serial_number = 0;	/* 0 is endian-neutral */
			HFC_4L_TO_4B(trc4->result, cmnd->result);
		}
	}/*-- trace format 4 --*/
	
/*----------------------------------------------*/
/*                  TRACE1                      */
/* etc1 = int_a_reg                             */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	else if( id == HFC_FX_TRC_HANDLER )
	{/*-- trace format 1 --*/
		trc1 = (struct err_fx_trc1 *)trc_wk ;
		
		trc1->id = id ;
		trc1->sub_id = sub_id ;
		if( pp != NULL )
		{
			HFC_2L_TO_2B(trc1->seq_no, pp->seq_no);
			trc1->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc1->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc1->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc1->a_scsi_id[0], &buf[1], 3);
		}
		if( etc1 != 0)
			HFC_4L_TO_4B(trc1->int_a_status, etc1_wk);
		
		trc1->rid = (uint)etc2_wk;
		
		port_trace = 1;
	}/*-- trace format 1 --*/
	
/*----------------------------------------------*/
/*                  TRACE2                      */
/* etc1 = NULL                                  */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	else if((id == HFC_FX_TRC_MBRESP)			||
			(id == HFC_FX_TRC_LINKRSP)			||
			(id == HFC_FX_TRC_PDISCRSP)			||
			(id == HFC_FX_TRC_GIDFTRSP)			||
			(id == HFC_FX_TRC_MIHLGRSP)			||
			(id == HFC_FX_TRC_GPNIDRSP)			||
			(id == HFC_FX_TRC_LGINRSP)			||
			(id == HFC_FX_TRC_GIDPNRSP)			||
			(id == HFC_FX_TRC_ADD_PORTIDRSP)	||
			(id == HFC_FX_TRC_DEL_PORTIDRSP)	||
			(id == HFC_FX_TRC_LDCH_TRCLOGRSP)	||
			(id == HFC_FX_TRC_CSCSIRSP)			||
			(id == HFC_FX_TRC_CORESTARTRSP)		||
			(id == HFC_FX_TRC_FLOGIRSP)			||
			(id == HFC_FX_TRC_PLOGIRSP)			||
			(id == HFC_FX_TRC_PRLIRSP)			||
			(id == HFC_FX_TRC_PRLORSP)			||
			(id == HFC_FX_TRC_SCRRSP)			||
			(id == HFC_FX_TRC_GCSIDRSP)			||
			(id == HFC_FX_TRC_RFTIDRSP)			||
			(id == HFC_FX_TRC_RFFIDRSP)			||
			(id == HFC_FX_TRC_GPNFTRSP)			||
			(id == HFC_FX_TRC_LOGORSP)			)
	{/*-- trace format 2 --*/
		trc2 = (struct err_fx_trc2 *)trc_wk ;
		
		trc2->id = id ;
		trc2->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc2->seq_no, pp->seq_no);
			trc2->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc2->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc2->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc2->a_scsi_id[0], &buf[1], 3);
		}
		if (rp != NULL)
		{
			trc2->r_rid = rp->rid;
		}
		if (core != NULL)
		{
			trc2->c_core_no = core->core_no;
			trc2->mb_status = (uchar)core->mb_status;
			trc2->mb_retry_cnt = core->mb_retry_cnt;
			ptr = (uchar *)&core->mb->mb_resp.flag;
			HFC_MEMCPY(trc2->mb_resp, ptr, 64);
		}
		if (target != NULL) {
			trc2->t_flag = (uchar)target->flags;
			HFC_4L_TO_4B(trc2->t_status, target->status);
			trc2->t_id = (uchar)target->target_id;
			HFC_4L_TO_4B(buf, target->scsi_id);
			HFC_MEMCPY(&trc2->t_scsi_id[0], &buf[1], 3);
			HFC_8L_TO_8B(trc2->t_ww_name, target->ww_name);
			HFC_8L_TO_8B(trc2->t_node_name, target->node_name);
		}
		if( etc1 != 0)
			HFC_4L_TO_4B(trc2->passthrough_rsp, etc1_wk);
		
		port_trace = 1;
	}/*-- trace format 2 --*/
	
	else if((id == HFC_FX_TRC_MBINT)		||
			(id == HFC_FX_TRC_LINKDOWN_INT)	||
			(id == HFC_FX_TRC_LINKUP_INT)	||
			(id == HFC_FX_TRC_PLOGI_INT)	||
			(id == HFC_FX_TRC_LOGO_INT)		||
			(id == HFC_FX_TRC_SCN_INT)		||
			(id == HFC_FX_TRC_RSCN_INT)		||
			(id == HFC_FX_TRC_FLOGI_INT)	||
			(id == HFC_FX_TRC_PDISC_INT)	||
			(id == HFC_FX_TRC_RFRAME_INT)	||
			(id == HFC_FX_TRC_PRLI_INT)		||
			(id == HFC_FX_TRC_PRLO_INT)		)
	{/*-- trace format 2 --*/
		trc2 = (struct err_fx_trc2 *)trc_wk ;
		
		trc2->id = id ;
		trc2->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc2->seq_no, pp->seq_no);
			trc2->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc2->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc2->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc2->a_scsi_id[0], &buf[1], 3);
		}
		if (rp != NULL)
		{
			trc2->r_rid = rp->rid;
		}
		if (core != NULL)
		{
			trc2->c_core_no = core->core_no;
			trc2->mb_status = (uchar)core->mb_status;
			trc2->mb_retry_cnt = core->mb_retry_cnt;
			ptr = (uchar *)&core->mb->mb_intreq.mb_code;
			HFC_MEMCPY(trc2->mb_resp, ptr, 64);
		}
		if (target != NULL) {
			trc2->t_flag = (uchar)target->flags;
			HFC_4L_TO_4B(trc2->t_status, target->status);
			trc2->t_id = (uchar)target->device_flags;
			HFC_4L_TO_4B(buf, target->scsi_id);
			HFC_MEMCPY(&trc2->t_scsi_id[0], &buf[1], 3);
			HFC_8L_TO_8B(trc2->t_ww_name, target->ww_name);
			HFC_8L_TO_8B(trc2->t_node_name, target->node_name);
		}
		
		port_trace = 1;
	}/*-- trace format 2 --*/

/*----------------------------------------------*/
/*                  TRACE5                      */
/* etc1 = cmnd                                  */
/* etc2 = timer_id                              */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	else if( id == HFC_FX_TRC_WDOG )
	{/*-- trace format 5 --*/
		trc5 = (struct err_fx_trc5 *)trc_wk ;
		
		trc5->id = id ;
		trc5->sub_id = sub_id ;
		
		timer_id = (ushort)etc2;				/* FCLNX-GPL-FX-061 */
		HFC_2L_TO_2B(trc5->timer_id, timer_id);	/* FCLNX-GPL-FX-061 */
		wdog_time = (uint)etc1;
		HFC_4L_TO_4B(trc5->wdog_timeout, wdog_time);
		
		int_a_reg = (uint)etc3;
		HFC_4L_TO_4B(trc5->int_a_reg, int_a_reg);
		
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc5->seq_no, pp->seq_no);
			trc5->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc5->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc5->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc5->a_scsi_id[0], &buf[1], 3);
		}
		if (rp != NULL)
		{
			trc5->r_rid = rp->rid;
		}
		if (core != NULL)
		{
			trc5->c_core_no = core->core_no;
			trc5->c_status = (uchar)core->status;
			
			if (target != NULL) {
				HFC_2L_TO_2B(trc5->ct_wx_que_cnt, target->core_queue[core->core_no].wx_que_cnt);
				HFC_2L_TO_2B(trc5->ct_we_que_cnt, target->core_queue[core->core_no].we_que_cnt);
			}
		}
		if (target != NULL) {
			trc5->t_flag = (uchar)target->flags;
			trc5->t_pseq = (uchar)target->pseq;
			HFC_4L_TO_4B(trc5->t_status, target->status);
			trc5->t_id = (uchar)target->target_id;
			HFC_4L_TO_4B(buf, target->scsi_id);
			HFC_MEMCPY(&trc5->t_scsi_id[0], &buf[1], 3);
		}
		if( hfcp != NULL)
		{
			trc5->h_cmd_flags = hfcp->cmd_flags;
			HFC_2L_TO_2B(trc5->h_adap_status, hfcp->adap_status);
			HFC_4L_TO_4B(trc5->h_iov_no, hfcp->iov_no);
			HFC_4L_TO_4B(trc5->h_iov_cnt, hfcp->iov_cnt);
			trc5->h_cmd_xob = hfcp->cmd_xob;
			trc5->h_rid = hfcp->rid;
		}
		if( cmnd != NULL )
		{
			HFC_4L_TO_4B(trc5->retries, cmnd->retries);
			HFC_4L_TO_4B(trc5->allowed, cmnd->allowed);
			HFC_MEMCPY(&trc5->cmnd[0], cmnd->cmnd, 16);
			/* kernel 5.x+: sdb.resid → resid_len; serial_number removed */
			HFC_4L_TO_4B(trc5->resid, cmnd->resid_len);
			trc5->serial_number = 0;	/* 0 is endian-neutral */
			HFC_4L_TO_4B(trc5->result, cmnd->result);
			
			sdev = cmnd->device;
			if( sdev != NULL ){
				rq = sdev->request_queue;
				if( rq != NULL ){
					trc5->timeout = (rq->rq_timeout/HZ);
				}
			}
		}
		
		port_trace = 1;
	}/*-- trace format 5 --*/
	
/*----------------------------------------------*/
/*                  TRACE6                      */
/* etc1 = int_a_reg                             */
/* etc2 = status_reg                            */
/* etc3 = detail_reg                            */
/*----------------------------------------------*/

	else if((id == HFC_FX_TRC_CHKSTP)	||
			(id == HFC_FX_TRC_ABEND)	||
			(id == HFC_FX_TRC_MCKREC)	||
			(id == HFC_FX_TRC_HWERR)	)
	{/*-- trace format 6 --*/
		trc6 = (struct err_fx_trc6 *)trc_wk ;
		
		trc6->id = id ;
		trc6->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc6->seq_no, pp->seq_no);
			trc6->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc6->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc6->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc6->a_scsi_id[0], &buf, 4);
			HFC_4L_TO_4B(trc6->mck_err_cnt, pp->mck_err_cnt);
			HFC_4L_TO_4B(trc6->link_dead_cnt, pp->link_dead_cnt);
		}
		if (core != NULL)
		{
			trc6->c_core_no = core->core_no;
		}
		if( etc1 != 0)
			trc6->int_a_status = (uint)etc1;
		if( etc2 != 0)
			trc6->status_reg = etc2 ;
		if( etc3 != 0)
			trc6->detail_reg = (uint)etc3;
		
		port_trace = 1;
	}
	
/*----------------------------------------------*/
/*                  TRACE7                      */
/* etc1 =                                       */
/* etc2 =                                       */
/* etc3 =                                       */
/*----------------------------------------------*/

	else if((id == HFC_FX_TRC_FORCE_ISOL)		||
			(id == HFC_FX_TRC_FORCE_ISOL_REC)	||
			(id == HFC_FX_TRC_FORCE_ISOL_REC_P)	||
			(id == HFC_FX_TRC_CHECK_ERRCOUNT)	)
	{/*-- trace format 7 --*/
		trc7 = (struct err_fx_trc7 *)trc_wk ;
		
		trc7->id = id ;
		trc7->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc7->seq_no, pp->seq_no);
			trc7->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc7->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc7->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc7->a_scsi_id[0], &buf, 4);
			HFC_4L_TO_4B(trc7->mck_err_cnt, pp->mck_err_cnt);
			HFC_4L_TO_4B(trc7->link_dead_cnt, pp->link_dead_cnt);
			
			trc7->isol_detail = pp->status_detail2;
			trc7->c_err = pp->c_err;
		}
		if (core != NULL)
		{
			trc7->c_core_no = core->core_no;
		}
		
		trc7->etc1 = (uint)etc1;
		trc7->etc2 = (uint)etc2;
		trc7->etc3 = (uint)etc3;
		
		port_trace = 1;
	}
	
/* FCLNX-GPL-393 */
/*----------------------------------------------*/
/*                  TRACE8                      */
/* etc1 =                                       */
/* etc2 =                                       */
/* etc3 =                                       */
/*----------------------------------------------*/

	else if((id == HFC_FX_TRC_MLPF_INT)				||
			(id == HFC_FX_TRC_MLPF_HWERR_INT)		||
			(id == HFC_FX_TRC_MLPF_FORCE_ISOL)		||
			(id == HFC_FX_TRC_MLPF_RECV_ISOL)		||
			(id == HFC_FX_TRC_MLPF_HWERR_INT_DET)	||
			(id == HFC_FX_TRC_MLPF_MIGRATION)		)
	{
		trc8 = (struct err_fx_trc8 *)trc_wk ;
		
		trc8->id = id ;
		trc8->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc8->seq_no, pp->seq_no);
			trc8->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc8->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc8->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc8->a_scsi_id[0], &buf, 4);
			HFC_4L_TO_4B(trc8->mck_err_cnt, pp->mck_err_cnt);
			HFC_4L_TO_4B(trc8->link_dead_cnt, pp->link_dead_cnt);
		}
		if (core != NULL)
		{
			trc8->c_core_no = core->core_no;
		}
		
		if( etc1 != 0)
			HFC_4L_TO_4B(trc8->hyp_status, etc1_wk);
		if( etc2 != 0)
			HFC_4L_TO_4B(trc8->etc2, etc2_wk);
		if( etc3 != 0)
			HFC_4L_TO_4B(trc8->etc3, etc3_wk);
		
		port_trace = 1;
	}
	
	/* Trace output */
	if (port_trace)
		hfc_fx_trace(pp, NULL, id, &trc_wk[1], 0);
	else
		hfc_fx_trace(pp, core, id, &trc_wk[1], 0);
}


/*
 * Function:    hfc_fx_clear_sticky_bit
 *
 * Purpose:     Clear "config registar sticky bit" for FIVE-EX
 *
 * Arguments:   
 *  pp          - Adapter Information
 *
 * Returns:     
 *
 * Notes:       pp and pp->pci_cfginf is not NULL
 */
void hfc_fx_clear_sticky_bit(struct port_info *pp)
{

	struct pci_dev *pdev; /* FCLNX-GPL-230 */
	
	pdev = pp->pci_cfginf; /* FCLNX-GPL-230 */
	
	/* Clear Status Register */
	hfc_fx_write_cnfg(pp, 0x06, 0x2, 0xffff); /* 0x06 -0x07 */
	/* Clear Device Status Register */
	hfc_fx_write_cnfg(pp, 0x76, 0x1, 0xff); /* 0x76 - 0x77 (0x77 is Reserved area) */
	
	/* Check CFG space size */ /* FCLNX-GPL-230 start */
	if( pdev->cfg_size > 0x114 ) /* 0x114 is (0x110 + 0x4) */
	{	/* Access was permited */
		
		/* Clear AER Uncrrectable Error Status */
		hfc_fx_write_cnfg(pp, 0x104, 0x4, 0xffffffff); /* 0x104 - 0x107 */
		/* Claer AER Correctable Error Status */
		hfc_fx_write_cnfg(pp, 0x110, 0x4, 0xffffffff); /* 0x110 - 0x113 */
	}
	/* FCLNX-GPL-230 end */

#if 0
	/* Set DUMP Core's FRZ_CLR bit */ /* HFC_DUMP_FRZ_CLR is 0x00400000 */
	hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_DUMP_CMD,( char )0x4, HFC_DUMP_FRZ_CLR); /* FCLNX-GPL-081 */
#endif

	return;
}

/*
 * Function:    hfc_fx_pcie_sram_ce_recovery
 *
 * Purpose:     This func recovery "PCIe IP core SRAM 1bit ERR".
 *
 * Arguments:   
 *  pp          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_pcie_sram_ce_recovery(struct port_info *pp)
{
	uint	i, wk;
	
	/******** Execute Logging process ******************/
	/* Check the parameter */
	if(pp->max_pcie_sram_ce_cnt != 0){
		/* Always count up when this param is not 0. */
		/* Save Log data */
		if(pp->pcie_sram_ce_cnt < HFC_FX_1BIT_LOG_ENTRY){ /* Array length check */
			hfc_fx_save_pcie_sram_log(pp); /* get "PCIe" sram log*/
			pp->pcie_sram_ce_cnt++; /* Count up */
			hfc_fx_set_sram_ce_log(pp); /* set "All" srem ce log */

			if(pp->pcie_sram_ce_cnt == pp->max_pcie_sram_ce_cnt){
				/*** Set SRAM CE Log ***/
				/* Set ErrNo */
				for(i=0; i<3; i++){
					wk = 0xfffe1100+i;
					hfc_fx_write_val( pp->ce_log[i].err_num, wk );
				}
				
				hfc_fx_write_val( pp->ce_fw_log.err_num, 0xfffe1103 );
				/* Count over */ /* log out only 1 time */
				hfc_fx_errlog( pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_SRAMCE,
					ERRID_HFCP_ERR2, 0x000000a5, NULL, 16 );
			}
		}
	}

	/******** Reset Registers **************************************/
	/* Set a bit to issue IPRES */ /* FCLNX-GPL-128 */
	hfc_fx_write_reg(pp, (uint)HFC_IOSPACE_KCMD_IPRES, (char)0x01, 0x01);

	return;
}

/*
 * Function:    hfc_fx_set_sram_ce_log
 *
 * Purpose:
 *
 * Arguments:
 *  pp         - pointer to port_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_set_sram_ce_log(struct port_info *pp)
{
	uint64_t time = 0;
	
	/* Now time */
	time = get_jiffies_64(); /* get jiffies_64 */
	
	switch( pp->pcie_sram_ce_cnt ){
		case 1 :
			/* time stamp */
			HFC_8L_TO_8B(pp->ce_log[0].pcie_sram_data[0].time_stamp, time);
			/* pcie_sram_cnt */
			HFC_4L_TO_4B(pp->ce_log[0].pcie_sram_data[0].pcie_sram_cnt, pp->pcie_sram_ce_cnt);
			memset(pp->ce_log[0].pcie_sram_data[0].resv, 0xee, 4);
			break;
		case 2 :
			/* time stamp */
			HFC_8L_TO_8B(pp->ce_log[0].pcie_sram_data[1].time_stamp, time);
			/* pcie_sram_cnt */
			HFC_4L_TO_4B(pp->ce_log[0].pcie_sram_data[1].pcie_sram_cnt, pp->pcie_sram_ce_cnt);
			memset(pp->ce_log[0].pcie_sram_data[1].resv, 0xee, 4);
			break;
		
		case 3 :
			/* time stamp */
			HFC_8L_TO_8B(pp->ce_log[1].pcie_sram_data[0].time_stamp, time);
			/* pcie_sram_cnt */
			HFC_4L_TO_4B(pp->ce_log[1].pcie_sram_data[0].pcie_sram_cnt, pp->pcie_sram_ce_cnt);
			memset(pp->ce_log[1].pcie_sram_data[0].resv, 0xee, 4);
			break;
		case 4 :
			/* time stamp */
			HFC_8L_TO_8B(pp->ce_log[1].pcie_sram_data[1].time_stamp, time);
			/* pcie_sram_cnt */
			HFC_4L_TO_4B(pp->ce_log[1].pcie_sram_data[1].pcie_sram_cnt, pp->pcie_sram_ce_cnt);
			memset(pp->ce_log[1].pcie_sram_data[1].resv, 0xee, 4);
			break;
		
		case 5 :
			/* time stamp */
			HFC_8L_TO_8B(pp->ce_log[2].pcie_sram_data[0].time_stamp, time);
			/* pcie_sram_cnt */
			HFC_4L_TO_4B(pp->ce_log[2].pcie_sram_data[0].pcie_sram_cnt, pp->pcie_sram_ce_cnt);
			memset(pp->ce_log[2].pcie_sram_data[0].resv, 0xee, 4);
			break;
		case 6 :
			/* time stamp */
			HFC_8L_TO_8B(pp->ce_log[2].pcie_sram_data[1].time_stamp, time);
			/* pcie_sram_cnt */
			HFC_4L_TO_4B(pp->ce_log[2].pcie_sram_data[1].pcie_sram_cnt, pp->pcie_sram_ce_cnt);
			memset(pp->ce_log[2].pcie_sram_data[1].resv, 0xee, 4);
			break;
	}

	return;
}


/* FCLNX-GPL-209 */
/*
 * Function:    hfc_fx_mck_prepare
 *
 * Purpose:     We prepare something for MCK recovery
 *                1. Give up waiting mailbox.
 *                2. Stop timer.
 *                3. Cancel SCSI command.(And, clear target_info_fx.)
 *
 * Arguments:   
 *  pp          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_mck_prepare(struct port_info *pp, uchar type)
{
	struct target_info_fx  *target = NULL;
	uint lp, i, rss=0;
	struct dev_info_fx 	*dev=NULL;
	struct core_info	*core=NULL;
	struct port_info	*vpp;
	uchar				link_reset=0;
	
	HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY);	
	pp->linkdown_occurred = 0;				/* FCLNX-GPL-FX-174 */
	hfc_fx_reset_all_timer(pp);										/* FCLNX-GPL-038 */
	
	for (i=0; i<=pp->pport->max_vport_count; i++) {
		vpp = pp->pport->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		if (test_bit( HFC_PD_LINK_RESET, (ulong *)&vpp->status_detail2 )) {
			link_reset = 1;
			break;
		}
	}
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)						
	{
		target = hfc_fx_hash_target_valid(pp, lp);	/* FCLNX-GPL-FX-316 */
		if (target != NULL)
		{
			if ( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ){	/* FCLNX-GPL-FX-316 */
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					hfc_fx_notify_tout(pp, core, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */
					hfc_fx_cancel_scsi_cmd(
						pp, core, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,
						TRUE, TRUE, HFC_FLASH_TARGET);
				}
				target->status = HFC_NON_STATUS ;
				target->tgt_core_stat.all = 0;	/* FCLNX-GPL-FX-014 */
				clear_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags);
				
				dev = target->dev;
				while( dev != NULL){
					dev->lustat = 0x00;
					dev->dev_core_stat.all = 0;	/* FCLNX-GPL-FX-014 *//* FCLNX-GPL-FX-073 */
					dev = dev->next;
				}
			}	/* FCLNX-GPL-FX-316 */
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) {
				if( ((type != HFC_ABEND_LINK_RESET) || (link_reset != 1) || (pp->link_reset == HFC_FX_LINK_RESET_MULTI))
				    && (pp->pport->issue_lip != HFC_SYSFS_ISSUE_LIP) ){	/* FCLNX-GPL-FX-328 */
					if (test_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 )){
						hfc_manage_info.npubp->hfc_fx_forced_offline_c(target, TRUE);
					}
					hfc_manage_info.npubp->hfc_fx_forced_offline_e(target, TRUE);	/* FCLNX-704 */
				}
			}
		}
	}
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {		/* FCLNX-GPL-FX-161 Start */
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		clear_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
		clear_bit(HFC_CS_MB_RETRY_DELAY, (ulong *)&core->status);
	}	/* FCLNX-GPL-FX-161 End */
	
	if( (type == HFC_ABEND_LINK_RESET) && (link_reset == 1)
		&& (pp->link_reset == HFC_FX_LINK_RESET_MULTI) 
		&& (pp->pport->issue_lip != HFC_SYSFS_ISSUE_LIP) ){
		clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
			pp->link_reset_multi_mode = 1;
#endif
	}
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)											/* FCLNX-GPL-038 */
	{
		target = hfc_fx_pseq_target_info_fx(pp, lp);
		if (target != NULL)
		{
			/* Release all target_info_fx */
			if( test_bit( HFC_PS_ISOL, (ulong *)&pp->status)){
				hfc_fx_clear_target_info_fx( pp, target, TRUE );
			}
			else if( test_bit( HFC_TS_WAIT_PLOGI, (ulong *)&target->status ) 
			  || test_bit( HFC_TS_WAIT_PRLI, (ulong *)&target->status ) 
			  || test_bit( HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status ) 	/* FCLNX-GPL-FX-014 */
			  || test_bit( HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status ) ){	/* FCLNX-GPL-FX-014 */
				clear_bit( HFC_TS_WAIT_PLOGI, (ulong *)&target->status );
				clear_bit( HFC_TS_WAIT_PRLI, (ulong *)&target->status );
				clear_bit( HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status ); 	/* FCLNX-GPL-FX-014 */
				clear_bit( HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status ); 		/* FCLNX-GPL-FX-014 */
				clear_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status);					/* FCLNX-GPL-FX-112 */
				hfc_fx_clear_target_info_fx( pp, target, TRUE );
			}
			else if( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
				clear_bit( HFC_TS_NEED_PLOGI, (ulong *)&target->status );
				clear_bit( HFC_TS_NEED_PRLI, (ulong *)&target->status );
				clear_bit(HFC_TS_CANCEL_SCSI_TARGET,(ulong *)&target->status);					/* FCLNX-GPL-FX-112 */
				hfc_fx_clear_target_info_fx( pp, target, TRUE );				/* FCLNX-GPL-0052 */
			}
		}
	}																				/* FCLNX-GPL-038 */
	
	/* Multi Queue */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				for (rss=0 ; rss< HFC_FX_MSIX_NVEC ; rss++) {
					hfc_fx_write_reg_rss_core(pp, i, rss, HFC_IOSPACE_RSS_INTA_RST,
							0x4, 0xffffffff, HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
				}
			}
		}
	}

	return;
}

/* FCLNX-GPL-209 */
/*
 * Function:    hfc_fx_mck_recovery
 *
 * Purpose:     -
 *
 * Arguments:   
 *  pp          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_mck_recovery(struct port_info *pp, uchar type)
{

	switch(pp->pkg.type){  /* FCLNX-GPL-081 */
		case HFC_PKTYPE_FIVE_FX:
			if( test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ) {				/* FCLNX-GPL-0157 */
				pp->mck_progress = HFC_MCK_PROGRESS;
			}
			else {
				pp->mck_progress = 0;
			}
			hfc_fx_mck_recovery_five_fx(pp,type);											/* FCLNX-GPL-0157 */
			break;
			
		default:
			/* NOP */
			break;
	}

	return;
}

void hfc_fx_clear_errinfo_i( struct port_info *pp )
{
	int	i,j;
	struct target_info_fx	*target;
	struct port_info		*vpp;

	HFC_ENTRY("hfc_fx_clear_errinfo_i");

	/* clear all error count and factor */
	pp->c_err = 0x00;
	
	pp->ld_err_count_s = 0;
	pp->if_err_count = 0;
	pp->to_err_count = 0;
	
	for (j=0; j<=pp->pport->max_vport_count; j++) {
		vpp = pp->pport->vport_ptr[j].vport_arg;
		if (vpp == NULL)
			continue;
		
		for (i=0;i<vpp->max_target;i++) {
			target = vpp->target_arg[i];
			if(target != NULL){
				target->tgt_ld_err_count_s = 0;
			}
		}
	}

	HFC_EXIT("hfc_fx_clear_errinfo_i");
}

int hfc_fx_get_isolparam_i( struct port_info *pp, struct hfc_isol_info *isolinfo, uchar pcm) {	/* FCLNX-GPL-393 */
	int	i;
	short	max_ld_err_count_s=0;
	struct	target_info_fx		*target;

	HFC_ENTRY("hfc_fx_get_isolparam");
	
	if(pcm == 0){	/* FCLNX-GPL-393 */
		isolinfo->ld_err_limit_s=	pp->ld_err_limit_s;
		
		max_ld_err_count_s = pp->ld_err_count_s;
		for(i=0; i<(pp->max_target); i++){
			target = hfc_fx_hash_target_valid(pp, i);
			if( target != NULL){
				if(max_ld_err_count_s < target->tgt_ld_err_count_s){
					max_ld_err_count_s = target->tgt_ld_err_count_s;
				}
			}
		}
		isolinfo->ld_err_count_s=	max_ld_err_count_s;
		
		isolinfo->if_err_limit	=	pp->if_err_limit;
		isolinfo->if_err_count	=	pp->if_err_count;
		isolinfo->to_err_limit	=	pp->to_err_limit;
		isolinfo->to_err_count	= 	pp->to_err_count;
		isolinfo->rt_err_enable	=	pp->rt_err_enable;
		isolinfo->to_reset_retry =	pp->to_reset_retry;
		isolinfo->total_abort_to =	pp->total_abort_to;		/* FCLNX-GPL-FX-014 */
		isolinfo->total_tgtrst_to =	pp->total_tgtrst_to;	/* FCLNX-GPL-FX-014 */
		if(pp->c_err == HFC_ISOLATE_RT){
			isolinfo->rt_err_count = 1;
		}
		else{
			isolinfo->rt_err_count = 0;
		}
	
		if((pp->ld_err_limit_s)||(pp->if_err_limit)||(pp->to_err_limit)||(pp->rt_err_enable)
		||(pp->total_abort_to)||(pp->total_tgtrst_to)){	/* FCLNX-GPL-FX-014 */
			if(pp->hba_isolation == HFC_ISOL_START)
				isolinfo->err_is_func	=	HFC_ENABLE_ISOLATE;
			else
				isolinfo->err_is_func	=	HFC_DISABLE_ISOLATE;
		}
		else{
			isolinfo->err_is_func	=	HFC_DISABLE_ISOLATE;
		}
	}	/* FCLNX-GPL-393 */
	
	isolinfo->err_is_hvm_spt = HFC_HVM_ISOLATE_NOT_SUPPORT;	/* FCLNX-GPL-401 */
	if( !( HFC_FX_MMODE_CHECK_SHARED(pp) ) ){
		isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_SUPPORT;
	}
	else{
		if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&pp->fw_support)){
			isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_SUPPORT;
		}else{
			isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_NOT_SUPPORT;
		}
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support)){
			isolinfo->err_is_hvm_spt = HFC_HVM_ISOLATE_SUPPORT;
		}else{
			isolinfo->err_is_hvm_spt = HFC_HVM_ISOLATE_NOT_SUPPORT;
		}
	}								/* FCLNX-GPL-401 */
	
	HFC_EXIT("hfc_fx_get_isolparam");
	return(0);
}

int hfc_fx_check_errcount(struct port_info *pp)
{												/* FCLNX-710 */
	uint hyp_status;				/* FCLNX-GPL-426 */

	if(!pp->c_err)
		return 0;
		
	hfc_fx_hand2_trace(
		HFC_FX_TRC_CHECK_ERRCOUNT, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0);
	
	if (test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ) )	/* FCLNX-GPL-349 */
		return 0;
	
	if (test_bit (HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2))
		return 0;
	
	if (test_bit(HFC_PD_WAIT_MIHLOG,(ulong *)&pp->status_detail2))
		return 0;
	
	/* Isolate to adapter */
	if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) ){				/* FCLNX-GPL-393 */
		hfc_fx_force_linkdown(pp->pport, FALSE);	/* FCLNX-GPL-FX-043 */
	}
	else if(HFC_FX_MMODE_CHECK_SHADOW(pp) ){				/* FCLNX-GPL-426 */
		if(pp->c_err == HFC_ISOLATE_SHADOW){
			if(test_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol)){
				hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
				if((!test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status)) &&
				(!test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)) &&
				(!test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol)) &&
				(!test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol))){
					hfc_fx_mlpf_isol_recovery_start_slpar(pp, hyp_status);	/* FCLNX-GPL-426 */
				}
			}
			if((test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol)) ||
			(test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol))){
				hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
				if((!test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status)) &&
				(!test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)) &&
				(!test_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol))){	/* FCLNX-GPL-432 */
					hfc_fx_mlpf_isol_start_slpar(pp, hyp_status);	/* FCLNX-GPL-426 */
				}
			}
		}
	}
	else{
		hfc_fx_mlpf_issue_fisolate(pp, HFC_ISSUE_ISOLREQ_ERR);
	}													/* FCLNX-GPL-393 */
	return 1;
}												/* FCLNX-710 */

void hfc_fx_watched_errcount_i(struct port_info *pp, struct target_info_fx *target, uchar err_flag)	/* FCLNX-GPL-349 */
{
	if (pp->pport->hba_isolation == HFC_ISOL_STOP)	/* FCLNX-GPL-393 */
		return;
	
	switch (err_flag) {
		case HFC_LDS_ERR:
			if (HFC_FX_PHYSICAL_PORT(pp)) {	/* FCLNX-GPL-FX-298, 301 */
				if (!pp->pport->ld_err_limit_s)
					break;
				pp->pport->ld_err_count_s++;
				if (pp->pport->ld_err_count_s >= pp->pport->ld_err_limit_s) {
					pp->pport->c_err = HFC_ISOLATE_LDS;
				}
			}
			break;
		case HFC_IF_ERR:
			if (!pp->pport->if_err_limit)
				break;
			pp->pport->if_err_count++;
			if (pp->pport->if_err_count >= pp->pport->if_err_limit) {
				pp->pport->c_err = HFC_ISOLATE_IF;
			}
			break;
		case HFC_TO_ERR:
			if (!pp->pport->to_err_limit)
				break;
			pp->pport->to_err_count++;
			if (pp->pport->to_err_count >= pp->pport->to_err_limit) {
				pp->pport->c_err = HFC_ISOLATE_TO;
			}
			break;
		case HFC_TGT_LDS_ERR:
			if (!pp->pport->ld_err_limit_s)
				break;
			target->tgt_ld_err_count_s++;
			if (target->tgt_ld_err_count_s >= pp->pport->ld_err_limit_s) {
				pp->pport->c_err = HFC_ISOLATE_TGT_LDS;
			}
			break;
		case HFC_RT_ERR:
			if (pp->pport->rt_err_enable) { 
				pp->pport->c_err = HFC_ISOLATE_RT;
			}
			break;
		default:
			break;
	}
	
	if (pp->pport->c_err) {
		set_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&pp->pport->status_detail2 );
	}
	
	hfc_fx_change_vport_isol_state(pp);
	
	return;
}																		/* FCLNX-GPL-349 */


/* FCLNX-GPL-430 */
/*
 * Function:    hfc_fx_check_cmnd_timeout
 *
 * Purpose:     -
 *
 * Arguments:   
 *  pp          - Adapter Information
 *  target      - Target Information
 *
 * Returns:     1 : There is scsi command occred time-out in weque.
 *				0 : There isn't scsi command time-out in weque.
 *
 * Notes:       Interruption level
 */
int hfc_fx_check_cmnd_timeout(struct port_info *pp, struct core_info *core, struct target_info_fx *target, ushort *lun){	/* FCLNX-GPL-FX-014 */
	int                         hash = 0;           /* FCLNX-0579 */
	struct hfc_pkt_fx           *hfcp_wk = NULL;	/* FCLNX-0579 */

	if((pp == NULL)||(core == NULL)||(target == NULL)) return(0);
	
	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		if (target->core_queue[core->core_no].we_que_top[hash] != NULL)
		{ 	/* hfcp exists in queue */
			hfcp_wk = target->core_queue[core->core_no].we_que_top[hash];
			while( hfcp_wk != NULL )
			{
				if ((lun != NULL) && (hfcp_wk->lun_id != *lun)) {	/* FCLNX-GPL-FX-014 */
					hfcp_wk = hfcp_wk->cmd_forw ;
					continue;
				}	/* FCLNX-GPL-FX-014 */
				if ( test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp_wk->cmd_flags) )
				{	/* If pkt has MIHLOG */
					return(1);
				}
				hfcp_wk = hfcp_wk->cmd_forw ; 
			}
		}       /* FCLNX-0557 */
	}
	return(0);
}
/* FCLNX-GPL-430 */


void hfc_fx_change_vport_isol_state(struct port_info *pp) {
	struct port_info	*vpp;
	int					i;
	uint				wk1,wk2;
	
	if (!HFC_FX_VPORT_EXIST(pp->pport)) {
		return;
	}
	
	for (i=1; i<=pp->pport->max_vport_count; i++) {
		vpp = pp->pport->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		vpp->c_err = pp->pport->c_err;
		
		if (test_bit(HFC_PS_ISOL, (ulong *)&pp->pport->status)) {
			set_bit(HFC_PS_ISOL, (ulong *)&vpp->status);
		}
		else {
			clear_bit(HFC_PS_ISOL, (ulong *)&vpp->status);
		}
		
		wk1 =  ((0x00000001 << HFC_PD_ISOLATE_PORT_C)	|
				(0x00000001 << HFC_PD_ISOLATE_PORT_E)	|
				(0x00000001 << HFC_PD_ISOLATE_SFPFAIL)	|
				(0x00000001 << HFC_PD_ISOLATE_SFPNOTSUPPORT) |
				(0x00000001 << HFC_PD_ISOLATE_SFPDOWN)	|
				(0x00000001 << HFC_PD_ISOLATE_CHKSTP)	|
				(0x00000001 << HFC_PD_ISOLATE_CHKSTP_C)	|
				(0x00000001 << HFC_PD_ISOLATE_RECOVERY)) ;
		wk2 = ~wk1;
		
		vpp->status_detail2 = (pp->pport->status_detail2 & wk1) |
							(vpp->status_detail2 & wk2);
	}
}


int hfc_fx_skip_vport_errlog(struct port_info *pp, uint err_num)
{
	int rtn = err_num;
	
	switch( err_num ) {
		case 0x14:
			if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				HFC_DBGPRT("hfcldd%d: HFC_EVNT3 FC Adapter Link Down (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			else {
				HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Link Down (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			break;
		case 0x15:
			if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				HFC_DBGPRT("hfcldd%d: HFC_EVNT1 FC Adapter Link Up (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			else {
				HFC_ERRPRT("hfcldd%d: HFC_EVNT1 FC Adapter Link Up (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			break;
		case 0x18:
		case 0x1a:
		case 0x1b:
			if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				HFC_DBGPRT("hfcldd%d: HFC_EVNT2 FC Adapter Link Changed (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			else {
				HFC_ERRPRT("hfcldd%d: HFC_EVNT2 FC Adapter Link Changed (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			break;
		case 0x9c:
		case 0x9d:
		case 0x9e:
		case 0x9f:
			if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				HFC_DBGPRT("hfcldd%d: HFC_EVNT3 Invalid Optical Module install (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			else {
				HFC_ERRPRT("hfcldd%d: HFC_EVNT3 Invalid Optical Module install (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			break;
		case 0xa7:
			if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				HFC_DBGPRT("hfcldd%d: HFC_EVNT4 FC Adapter Driver Request Log (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			else {
				HFC_ERRPRT("hfcldd%d: HFC_EVNT4 FC Adapter Driver Request Log (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			break;
		case 0xa4:	/* FCLNX-GPL-FX-234 *//* FCLNX-GPL-FX-242,272 */
			if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				HFC_DBGPRT("hfcldd%d: HFC_EVNT4 FC Adapter Driver Request Log (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			else {
				HFC_ERRPRT("hfcldd%d: HFC_EVNT4 FC Adapter Driver Request Log (ErrNo:0x%02x) (PORT_ID:0x%06llx) (0x%02x:0x%02x)\n",
					pp->pport->dev_minor, err_num, pp->scsi_id, pp->sub_rid, pp->rid );
			}
			break;	/* FCLNX-GPL-FX-234 *//* FCLNX-GPL-FX-242,272 */
		default:
			rtn = 0;
			break;
	}
	
	return rtn;
}

void hfc_fx_wdog_linkup_tmr(struct port_info *pp, struct core_info *core, ushort timer_id)
{
	struct target_info_fx	*target;
	struct core_info		*core_wk;
	int 					i=0, lp=0, status_update=0;
	uint					hfc_status=0;	/* FCLNX-GPL-FX-407 */
	
	clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
	clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
	clear_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status );
	
	if(timer_id == HFC_FX_LINKUP_TMR){ /* FCLNX-GPL-069 */
		/* "HFC_FX_LINKUP_TMR" needs these steps. */
		if(pp->ldl_errcnt_info!=NULL){
			hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, NULL, HFC_OCCURED_FAILURE, HFC_LDL_ERR); /* FCLNX-0506 */
		}
		if(!test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status)){
			if(pp->c_err==0){	/* FCLNX-GPL-FX-318 */
#if !(defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
				set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
				atomic_set(&pp->check_mbreq, 1);
#else
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
					set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
					atomic_set(&pp->check_mbreq, 1);
				}
#endif
			}	/* FCLNX-GPL-FX-318 */
			pp->linknego_tmo_boot=0;	/* FCLNX-GPL-FX-139 */
		}else{
#if !(defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
			if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-436 Start */
				memset((void *)pp->logdata, 0, 16);
				pp->logdata[7] = HFC_LINKUPTMR_TO ;
				hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
			}else if( (test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2))
					  &&  (!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
					  &&  (hfc_fx_mlpf_check_normal_hypsts(pp)) ){
				set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
				set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
				hfc_fx_mlpf_linkend_int_glpar(pp);
				return;
			}else{
				memset((void *)pp->logdata, 0, 16);
				pp->logdata[7] = HFC_LINKUPTMR_TO ;
				hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
			}	/* FCLNX-GPL-FX-436 End */
#else
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
				if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-436 Start */
					memset((void *)pp->logdata, 0, 16);
					pp->logdata[7] = HFC_LINKUPTMR_TO ;
					hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
				}else if( (test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2))
						  &&  (!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
						  &&  (hfc_fx_mlpf_check_normal_hypsts(pp)) ){
					set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
					set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
					hfc_fx_mlpf_linkend_int_glpar(pp);
					return;
				}else{
					memset((void *)pp->logdata, 0, 16);
					pp->logdata[7] = HFC_LINKUPTMR_TO ;
					hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
				}	/* FCLNX-GPL-FX-436 End */
			}
#endif
		}
	}else if(timer_id == HFC_FX_LINKINIT_TMR){
		if(!test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status)){
			set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
			atomic_set(&pp->check_mbreq, 1);
			pp->linknego_tmo_boot=1;	/* FCLNX-GPL-FX-139 */
		}else{
			if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-436 Start */
				memset((void *)pp->logdata, 0, 16);
				pp->logdata[7] = HFC_LINKUPTMR_TO ;
				hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_ERR6, 0x8b, pp->logdata, 16);
			}else if( (test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2))
			&&  (!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
			&&  (hfc_fx_mlpf_check_normal_hypsts(pp)) ){
				set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
				set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
				hfc_fx_mlpf_linkend_int_glpar(pp);
				return;
			}else{
				memset((void *)pp->logdata, 0, 16);
				pp->logdata[7] = HFC_LINKUPTMR_TO ;
				hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_ERR6, 0x8b, pp->logdata, 16);
			}
		}
	}else if(timer_id == HFC_FX_WLINKUP_MCK_TMR){
		if(!test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status)){
#if !(defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
			set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
			atomic_set(&pp->check_mbreq, 1);
#else
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
				set_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
				atomic_set(&pp->check_mbreq, 1);
			}
#endif
			pp->linknego_tmo_boot=0;	/* FCLNX-GPL-FX-139 */
		}else{
#if !(defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
			if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-436 Start */
				memset((void *)pp->logdata, 0, 16);
				pp->logdata[7] = HFC_LINKUPTMR_TO ;
				hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
			}else if( (test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2))
					  &&  (!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
					  &&  (hfc_fx_mlpf_check_normal_hypsts(pp)) ){
				set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
				set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
				hfc_fx_mlpf_linkend_int_glpar(pp);
				return;
			}else{
				memset((void *)pp->logdata, 0, 16);
				pp->logdata[7] = HFC_LINKUPTMR_TO ;
				hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
			}
#else
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
				if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-436 Start */
					memset((void *)pp->logdata, 0, 16);
					pp->logdata[7] = HFC_LINKUPTMR_TO ;
					hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
				}else if( (test_bit( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2))
						  &&  (!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE))
						  &&  (hfc_fx_mlpf_check_normal_hypsts(pp)) ){
					set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
					set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
					hfc_fx_mlpf_linkend_int_glpar(pp);
					return;
				}else{
					memset((void *)pp->logdata, 0, 16);
					pp->logdata[7] = HFC_LINKUPTMR_TO ;
					hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_LINKUPTOUT, ERRID_HFCP_EVNT3, 0x8c, pp->logdata, 16 );
				}
			}
#endif
		}
	}
	
	if(HFC_FX_MMODE_CHECK_SHADOW(pp) ){	/* FCLNX-GPL-FX-402 */
		hfc_status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);	/* FCLNX-GPL-FX-407 */
		
		if(pp->connect_type == HFC_FX_F_PORT){
			status_update=1;
			hfc_status &= ~HFC_HG_LPRSTATUS_LINKDOWN;	/* FCLNX-GPL-FX-407 */
			hfc_status |= (HFC_HG_LPRSTATUS_MPID_ENABLE | HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_FCSW | 0x00001000);	/* FCLNX-GPL-FX-407 */
		}else if((pp->switch_exist == 0)&&(pp->connect_type == HFC_FX_PT2PT)){
			status_update=1;
			hfc_status &= ~(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE);	/* FCLNX-GPL-FX-407 */
			hfc_status |= (HFC_HG_LPRSTATUS_UNSHARABLE | ((HFC_FX_PT2PT << 12) & 0x00003000));	/* FCLNX-GPL-FX-407 */
		}else if((pp->switch_exist)&&(pp->connect_type == HFC_FX_AL)){
			status_update=1;
			hfc_status &= ~(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE);	/* FCLNX-GPL-FX-407 */
			hfc_status |= (HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_FCSW | ((HFC_FX_AL << 12) & 0x00003000));	/* FCLNX-GPL-FX-407 */
		}else if(timer_id == HFC_FX_LINKINIT_TMR){
			status_update=1;
			hfc_status &= ~(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_SPACE);	/* FCLNX-GPL-FX-407 */
			hfc_status |= HFC_HG_LPRSTATUS_LINKDOWN;	/* FCLNX-GPL-FX-407 */
		}
		if(status_update){
			if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
				hfc_status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, hfc_status );	/* FCLNX-GPL-FX-407 */
		}
	}	/* FCLNX-GPL-FX-402 */
	
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
		clear_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2);
	}
	
	/* Cancel pending SCSI processes under this adapter */
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target = hfc_fx_hash_target_info(pp, lp);
		if( target != NULL )
		{
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				hfc_fx_cancel_scsi_cmd(pp,core_wk,target,0, NULL, SCS_LINKUP_TO, HFC_CSCSI_ERROR,	/* FCLNX-0429 */
					TRUE, TRUE, HFC_FLASH_TARGET);
			}
			target->status = HFC_NON_STATUS;
		}
	}
	hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);
	
	if (hfc_manage_info.npubp->hfc_fx_mp_hsd_enable)									/* FCLNX-0429 */
		hfc_manage_info.npubp->hfc_fx_make_fcinfo(pp, NULL, 0x14, SCS_LINKUP_TO, NULL, 0);
	
	if (pp->initialize == 1) 	/* FCLNX-0279 */
	{
		hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
	}																						/* FCLNX-0279 */
	
	if (HFC_FX_VIRTUAL_PORT(pp) && HFC_FX_VPORT_ENABLE(pp)) {
		hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_LINKDOWN);
	}
}

