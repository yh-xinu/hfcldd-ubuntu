/*
 * hfcl_timer_recovery.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char timer_rcsid[] = "$Id: hfcl_timer_recovery.c,v 1.85.2.25.2.32.2.6.2.7.6.35.4.20.2.14.2.4.2.3.2.5.2.4 2015/07/29 08:04:06 toyo Exp $";

/*--------------------------------------------------------------------------*/
/*    hfc_handler()                                                         */
/*        +--hfc_hwerr_int()                                                */
/*        +--hfc_mb_resp()                                                  */
/*        |      +--hfc_issue_relogin()                                     */
/*        |      +--hfc_cancel_scsi_cmd()                                   */
/*        |      +--hfc_issue_pdisc()                                       */
/*        +--hfc_mb_intreq()                                                */
/*        |      +--hfc_cancel_scsi_cmd()                                   */
/*        +--hfc_xrb_resp()                                                 */
/*        |      +--hfc_abend()                                             */
/*        |      +--hfc_link_chk()                                          */
/*        |      +--hfc_task_mgm_chk()                                      */
/*        |      |      +--hfc_deque_we_que()                               */
/*        |      |      +--hfc_iov_update()                                 */
/*        |      |      +--hfc_dma_unmap()                                  */
/*        |      |      +--hfc_start()                                      */
/*        |      |      +--hfc_cancel_weque()                               */
/*        |      +--hfc_scsi_chk()                                          */
/*        |             +--hfc_enque_wr_que()                               */
/*        |             +--hfc_deque_we_que()                               */
/*        |             +--hfc_iov_update()                                 */
/*        |             +--hfc_dma_unmap()                                  */
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

#ifndef _HFC_NO_RASLOG
#include "hraslog.h"
#endif

#if defined(__x86_64)  /* FCLNX-0398 */ /* FCLNX-0629 */
#include <linux/nmi.h>
#endif			/* FCLNX-0398 */ /* FCLNX-GPL-564 */

const struct errlog_t errlog_info[] = {
	{ ERRID_HFCP_ERR1 ,		"HFC_ERR1 Permanent FC Adapter Hardware error" },
	{ ERRID_HFCP_ERR2 ,		"HFC_ERR2 Temporary FC Adapter Hardware error" },
	{ ERRID_HFCP_ERR3 ,		"HFC_ERR3 Permanent FC Adapter Firmware error" },
	{ ERRID_HFCP_ERR4 ,		"HFC_ERR4 Temporary FC Adapter Firmware error" },
	{ ERRID_HFCP_ERR5 ,		"HFC_ERR5 Permanent FC Link error" },
	{ ERRID_HFCP_ERR6 ,		"HFC_ERR6 Temporary FC Link error" },
	{ ERRID_HFCP_ERR7 ,		"HFC_ERR7 Permanent FC Adapter Unknown error" },
	{ ERRID_HFCP_ERR8 ,		"HFC_ERR8 Temporary FC Adapter Unknown error" },
	{ ERRID_HFCP_ERR9 ,		"HFC_ERR9 FC Adapter Driver error" },
	{ ERRID_HFCP_ERRA ,		"HFC_ERRA FC Adapter Interrupt time-out" },
	{ ERRID_HFCP_ERRB ,		"HFC_ERRB FC Adapter Link Down" },
	{ ERRID_HFCP_ERRC ,		"HFC_ERRC FC Adapter Diagnostics error" },
	{ ERRID_HFCP_ERRD ,		"HFC_ERRD FC Adapter PCI error" },
	{ ERRID_HFCP_ERRE ,		"HFC_ERRE FC Adapter I/O Unavailable" },
	{ ERRID_HFCP_ERRF ,		"HFC_ERRF FC Adapter Initialize error" },
	{ ERRID_HFCP_EVNT1,		"HFC_EVNT1 FC Adapter Link Up" },
	{ ERRID_HFCP_EVNT2,		"HFC_EVNT2 FC Adapter Link Changed" },
	{ ERRID_HFCP_EVNT3,		"HFC_EVNT3 FC Adapter Driver Warning Event" },
	{ ERRID_HFCP_EVNT4,		"HFC_EVNT4 FC Adapter Driver Request Log" },
	{ ERRID_HFCP_OPTERR0,	"HFC_OPTERR0 Invalid Optical Module install" },
	{ 0xFFFFFFFF      ,	"" },
};

const struct hraslog_t hraslog_info[] = {
	{ ERRID_HFCP_ERR1 ,	"E1", "R1" },
	{ ERRID_HFCP_ERR2 ,	"E2", "R2" },
	{ ERRID_HFCP_ERR3 ,	"E3", "R3" },
	{ ERRID_HFCP_ERR4 ,	"E4", "R4" },
	{ ERRID_HFCP_ERR5 ,	"E5", "R5" },
	{ ERRID_HFCP_ERR6 ,	"E6", "R6" },
	{ ERRID_HFCP_ERR7 ,	"E7", "R7" },
	{ ERRID_HFCP_ERR8 ,	"E8", "R8" },
	{ ERRID_HFCP_ERR9 ,	"E9", "R9" },
	{ ERRID_HFCP_ERRA ,	"EA", "RA" },
	{ ERRID_HFCP_ERRB ,	"EB", "RB" },
	{ ERRID_HFCP_ERRC ,	"EC", "RC" },
	{ ERRID_HFCP_ERRD ,	"ED", "RD" },
	{ ERRID_HFCP_ERRE ,	"EE", "RE" },
	{ ERRID_HFCP_ERRF ,	"EF", "RF" },
	{ ERRID_HFCP_EVNT1,	"I1", "N1" },
	{ ERRID_HFCP_EVNT2,	"I2", "N2" },
	{ ERRID_HFCP_EVNT3,	"I3", "N3" },
	{ ERRID_HFCP_EVNT4,	"I4", "N4" },
	{ ERRID_HFCP_OPTERR0, "P0", "T0" },
	{ 0xFFFFFFFF      ,	 "" ,  ""  },
};

/* FCLNX-GPL-547 start */
enum log_files {
	HFC_SYSLOG  = 0,
	HFC_DRV_LOG = 1,
	HFC_ALL_LOG = 2  /* syslog and driver_log */
};

enum log_parts {
	HFC_LOG_HEAD = 0,
	HFC_LOG_BODY = 1, /* Example: "hfcldd0: HFC_ERRB FC Adapter Link Down (ErrNo:0x15)"  */
	HFC_LOG_DUMP = 2
};

enum log_prefixs {
	HFC_LOG_ERR = 0,
	HFC_LOG_WRN = 1,
	HFC_LOG_INF = 2
};

const uchar
alarm_tbl[ERRID_HFCP_TBL_END] = {
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

const enum log_prefixs
log_prefix_tbl[ERRID_HFCP_TBL_END] = {
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

#define HFC_MAX_LOG_FILE_SELECTS 6
#define HFC_MAX_LOG_PARTS        3

/* for Alarm Log */
const enum log_files
log_file_tbl_alm[HFC_MAX_LOG_FILE_SELECTS][HFC_MAX_LOG_PARTS] = {
	/* Log-Head   , Log-Body    , Log-Dump    */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=0 */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=1 */
	{ HFC_SYSLOG  , HFC_SYSLOG  , HFC_SYSLOG  }, /* hfc_log_file=2 */
	{ HFC_ALL_LOG , HFC_ALL_LOG , HFC_DRV_LOG }, /* hfc_log_file=3 */
	{ HFC_DRV_LOG , HFC_DRV_LOG , HFC_DRV_LOG }, /* hfc_log_file=4 */
	{ HFC_ALL_LOG , HFC_ALL_LOG , HFC_DRV_LOG }  /* hfc_log_file=5 */
};

/* for Non-Alarm Log */
const enum log_files
log_file_tbl_non[HFC_MAX_LOG_FILE_SELECTS][HFC_MAX_LOG_PARTS] = {
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

static void hfc_error_common( struct adap_info *,
				  hfc_errfmt1_t *,
				  struct hfc_pkt *);
static void hfc_errsave( struct adap_info *,
				  int,	
				  uint,		
				  struct hfc_err_rec *,
				  uint,
				  uint );
static uint hfc_raslog( struct adap_info *,
				  struct hfc_err_rec *,
				  char	 *,
				  char	 *,
				  char	 *,
				  struct hfc_pkt		*hfcp,
				  struct scsi_cmnd		*cmnd,
				  uchar);
void hfc_hand2_trace(
	uchar               id,
	uchar               sub_id,
	struct adap_info    *ap,
	struct target_info  *target,
	struct hfc_pkt		*hfcp,
	uint64_t            etc1,
	uint64_t            etc2,
	uint64_t            etc3);
static void hfc_timeout_by_reset(
	struct adap_info        *ap,
	struct target_info      *target,
	struct hfc_pkt     		*hfcp);

static void hfc_timeout_by_scnlinkup(
	struct adap_info        *ap,
	struct target_info       *target);

static void hfc_timeout_by_restart(		/* FCLNX-0500 */
	struct adap_info        *ap,
	struct target_info 		*target,
	struct hfc_pkt			*hfcp);

/* FCLNX-GPL-547 start */
#if defined(HFC_STAR) && defined(CONFIG_NAS_SCSI_DRVLOG)

static enum log_files
hfc_log_file_selector(uint err_id, uchar f_select, enum log_parts log_part)
{
	enum log_files log_file = HFC_SYSLOG;
	uchar alarm             = FALSE;

	/* Table size check */
	if( err_id  < ERRID_HFCP_TBL_END )
	{	/* Table size is correct. */
		alarm = alarm_tbl[err_id];
	}
	
	/* Table size check */
	if( f_select < HFC_MAX_LOG_FILE_SELECTS )
	{	/* Table size is correct. */
		/* Select Non-Alarm Log or Alarm Log Table */
		if( alarm == TRUE ) {
			log_file = log_file_tbl_alm[f_select][log_part];
		} else {
			log_file = log_file_tbl_non[f_select][log_part];
		}
	}
	
	return ( log_file );
}

#define HFC_PRT_COM(PRT_FUNC, err_id, f_select, log_part, fmt, args...)	\
{									\
	switch( hfc_log_file_selector(err_id, f_select, log_part) )	\
	{								\
		case HFC_SYSLOG:					\
			PRT_FUNC(fmt, ##args);				\
			break;						\
		case HFC_DRV_LOG:					\
			printk(KERN_DRV_INFO fmt, ##args);		\
			break;						\
		case HFC_ALL_LOG:					\
		default:						\
			PRT_FUNC(fmt, ##args);				\
			printk(KERN_DRV_INFO fmt, ##args);		\
			break;						\
	}								\
}

#if 0
#define HFC_ERRPRT_S(err_id, f_select, log_part, fmt, args...)		\
	HFC_PRT_COM(HFC_ERRPRT, err_id, f_select, log_part, fmt, ##args)

#define HFC_WRNPRT_S(err_id, f_select, log_part, fmt, args...)		\
	HFC_PRT_COM(HFC_WRNPRT, err_id, f_select, log_part, fmt, ##args)

#define HFC_INFPRT_S(err_id, f_select, log_part, fmt, args...)		\
	HFC_PRT_COM(HFC_INFPRT, err_id, f_select, log_part, fmt, ##args)

#else /* defined(HFC_STAR) && defined(CONFIG_NAS_SCSI_DRVLOG) */

#define HFC_ERRPRT_S(err_id, f_select, log_part, fmt, args...) HFC_ERRPRT(fmt, ##args)
#define HFC_WRNPRT_S(err_id, f_select, log_part, fmt, args...) HFC_WRNPRT(fmt, ##args)
#define HFC_INFPRT_S(err_id, f_select, log_part, fmt, args...) HFC_INFPRT(fmt, ##args)
#endif

#endif /* defined(HFC_STAR) && defined(CONFIG_NAS_SCSI_DRVLOG) */
/* FCLNX-GPL-547 end */
				  
//static uchar	logdata[16] ;			/* FCLNX-GPL-391 */

/*
 * Function:    hfc_hwerr_int
 *
 * Purpose:     Process Hardware error
 *
 * Arguments:   
 *  ap          - 
 *  int_a_reg   - 
 *  status_reg0 - 
 *  status_reg1 - 
 *  detail_reg  - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_hwerr_int(struct adap_info *ap,
					uint int_a_reg,
					uint status_reg0,
					uint status_reg1,
					uint detail_reg)
{
	uchar		  abend_code=0 ;
	uint64_t      status_reg;

	status_reg = ((uint64_t)status_reg0 << 32) & 0xffffffff00000000ULL;
	status_reg |= (uint64_t)status_reg1 & 0x00000000ffffffffULL;
	
	/* Align detail_reg with FPP format if package type is FIVE */
	if( ap->pkg.type != HFC_PKTYPE_FPP)                                 /* FCWIN-0240 */
		detail_reg <<= 8;

	HFC_DBGPRT(" hfcldd%d : hfc_hwerr_int \n",ap->dev_minor);

	hfc_hand2_trace(HFC_TRC_HWERR, 0x00, ap, NULL, NULL, 
			(uint64_t)int_a_reg, status_reg, (uint64_t)detail_reg);

	if( int_a_reg & HFC_HWERR_MCK ) {
		
		clear_bit( HFC_WAIT_T3, (ulong *)&ap->status ); /* FCLNX-GPL-137 */
		
		if( detail_reg & HFC_HWERR_T3  )	/* FCLNX-0269 */ /* FCLNX-GPL-61 */
		{
			abend_code = HFC_ABEND_T3 ;
		}
		else if( status_reg0 & HFC_HWERR_MPCHK )
		{
			abend_code = HFC_ABEND_MPCHK ;
		}
		else
		{
			abend_code = HFC_ABEND_MCK_INT ;
		}
	}
	else if( int_a_reg & HFC_HWERR_PCI )
	{
		if( int_a_reg & HFC_HWERR_SERR )
			abend_code = HFC_ABEND_SERR ;
		if( int_a_reg & HFC_HWERR_PERR )
			abend_code = HFC_ABEND_PERR ;
		if( int_a_reg & HFC_HWERR_SPERR )
			abend_code = HFC_ABEND_SPERR ;
	}
	hfc_abend(ap,abend_code);

	HFC_DBGPRT(" hfcldd : hfc_hwerr_int - end\n");
	hfc_hand2_trace(HFC_TRC_HWERR, 0x10, ap, NULL, NULL, 0, 0, 0);

}


/*
 * Function:    hfc_errlog
 *
 * Purpose:     Collect error log data 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
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
void hfc_errlog( struct adap_info *ap,
				struct target_info	*target,
				struct hfc_pkt		*hfcp,
				uchar				type,
				uint				err_id,
				uint				err_num,
				uchar				*data,
				ushort				data_len )
{
	struct	mp_adap_info	*mpap=NULL;
	struct hfc_err_rec		*err_rec;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
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
	ushort	slog_num;
	ushort	wk_slog_len;
	ushort	wk_slog_num;
	uint	scsi_id;
	char	wkbuf[VPD_PN_LEN+13];  /* +13=NULL */								/* FCLNX-0337 *//* FCLNX-0368 */
	char	ras_error_id1[9], ras_error_id2[9];
	char	resource_name[16] ;
	uint	raslog=2;
	hfc_errfmt1_t	*err1;				/* errlog format1 area pointer */
	hfc_errfmt2_t	*err2;				/* errlog format2 area pointer */
	uint	slog_area_size = 0 ;
	uint	wk_err_num ;
	ushort	lun_id=0;					/* FCLNX-GPL-0343 */
	uint	dev_data=0;					/* FCLNX-GPL-0343 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	struct dev_info		*dev=NULL;
	struct scsi_device	*sdev=NULL;
	struct request_queue	*rq=NULL;
#endif
	ushort	wk_tmo_sec = 0;				/* FCLNX-GPL-621 */

	ushort	cfg_data;
	uint	pci_data;
	char	sfp_type_name[HFC_SFP_TYPE_NAME_LEN+1];	/* FCLNX-GPL-311 */
	char	sfp_serial_no[HFC_SFP_SERIAL_NO_LEN+1];	/* FCLNX-GPL-311 */
	char	sfp_date_code[HFC_SFP_DATE_CODE_LEN+1];	/* FCLNX-GPL-311 */
	
	if( ap != NULL ) {
		HFC_DBGPRT(" hfcldd%d : hfc_errlog - start\n",ap->dev_minor);
	}

	errlog_info_p = 0;
	while( errlog_info[errlog_info_p].type != 0xFFFFFFFF ){
		if( errlog_info[errlog_info_p].type == err_id ){
			break;
		}
		errlog_info_p++;
	}

//	err_rec = &ap->err_rec;								/* FCLNX-0337 *//* FCLNX-0368 */
	
	memset(ras_error_id1,0,sizeof(ras_error_id1));
	sprintf(&ras_error_id1[strlen(ras_error_id1)], "KALB%c%c%02X",
		hraslog_info[errlog_info_p].alart1[0], hraslog_info[errlog_info_p].alart1[1], err_num);

	memset(ras_error_id2,0,sizeof(ras_error_id2));
	sprintf(&ras_error_id2[strlen(ras_error_id2)], "KALB%c%c%02X",
		hraslog_info[errlog_info_p].alart2[0], hraslog_info[errlog_info_p].alart2[1], err_num);

	memset(resource_name,0,sizeof(resource_name));
	if( ap != NULL){
		sprintf(&resource_name[strlen(resource_name)], "hfcldd%d", ap->dev_minor);
	}
	else if( ap == NULL){
		sprintf(&resource_name[strlen(resource_name)], "hfcldd");
	}

	/* FCLNX-GPL-161 */
	if(	( ap == NULL )               ||
		 (ap->mp_adap_info == NULL ) || 
		 (ap->pci_cfginf == NULL )   ||
		 (ap->mem_base_addr == 0) 		){
		
		HFC_ERRPRT_S(err_id, hfc_manage_info.log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd : %s (ErrNo:0x%02x) \n",errlog_info[errlog_info_p].errmsg, err_num); 
		if( ( data != NULL ) && ( data_len != 0 ) ){
			if( data_len > 16 )
				data_len = 16;
//			memcpy(&(err1->err_detail_1.drv_log[0]), data, data_len) ; /* FCLNX-GPL-161 */
			HFC_ERRPRT_S(err_id, hfc_manage_info.log_file, HFC_LOG_DUMP, /* FCLNX-GPL-547 */
				"      [%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x]\n",
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
				data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);	/* FCLNX-GPL-161 */
		}
		return;
	}
	
	err_rec = &ap->err_rec;												/* FCLNX-GPL-161 */
	err1 = (hfc_errfmt1_t *)err_rec->log_area ;
	HFC_BZERO( (char*)err_rec, sizeof( struct hfc_err_rec ) );			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	errlog_size = sizeof( struct hfc_err_rec );							/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */	
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info && (hfcp != NULL) ) {
		if (hfc_manage_info.npubp->hfc_errlog_mp(hfcp)) return;
	}
	
	ap->raslog_cnt = 0;
	if( ap->raslog_install ){
		HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd%d: Firmware version %06x, Driver version %s, device %02x:%02x.%02x IRQ %d\n",
			ap->dev_minor,
			hfc_get_sysrev(ap), /* FCLNX-GPL-112 */
			hfc_manage_info.package_ver,
			ap->pci_cfginf->bus->number,
			PCI_SLOT(ap->pci_cfginf->devfn),
			PCI_FUNC(ap->pci_cfginf->devfn),
			ap->pci_cfginf->irq);
		HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd%d: Adapter wwpn : %llx \n", ap->dev_minor,(unsigned long long)ap->ww_name);
	}
	
	memset(wkbuf,0,sizeof(wkbuf));																	/* FCLNX-0337 *//* FCLNX-0368 */
	if(ap->pkg.type == HFC_PKTYPE_FPP)			/* FPP */
		memcpy(wkbuf, ((struct hfc_vpd *)(ap->mp_adap_info->vpd_buf))->pn_value, VPD_PN_LEN);		/* FCLNX-0337 *//* FCLNX-0368 */
	else if(ap->pkg.type == HFC_PKTYPE_FIVE)	/* FIVE */
		memcpy(wkbuf, ((struct hfc_vpd_five *)(ap->mp_adap_info->vpd_buf))->pn_value, VPD_PN_LEN);	/* FCLNX-0337 *//* FCLNX-0368 */
	else										/* FIVE-EX */
		memcpy(wkbuf, ((struct hfc_vpd_five_ex *)(ap->mp_adap_info->vpd_buf))->pn_value, VPD_PN_LEN);
	if( ap->raslog_install )
		HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
			"hfcldd%d: Parts number : %11s\n", ap->dev_minor, wkbuf);
		
	/* Output SFP Information *//* FCLNX-GPL-311 */
	
	
	if( ap->raslog_install ){
		if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support)||(ap->fw_init_p->func & HFC_FWF_SFPINF)){		/* FCLNX-GPL-325 *//* FCLNX-GPL-393 */
			if(!(ap->fw_init_p->sfp_info.sfp_status & HFC_SFP_INSTALL)){
				HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
					"hfcldd%d: SFP Information : N/A\n", ap->dev_minor);
			}else {
				memset(sfp_type_name,0,sizeof(sfp_type_name));
				memcpy(sfp_type_name, ap->fw_init_p->sfp_info.sfp_type_name, sizeof(ap->fw_init_p->sfp_info.sfp_type_name)); 
				hfc_delete_space(sfp_type_name);
				
				memset(sfp_serial_no,0,sizeof(sfp_serial_no));
				memcpy(sfp_serial_no, ap->fw_init_p->sfp_info.sfp_serial_no, sizeof(ap->fw_init_p->sfp_info.sfp_serial_no)); 
				hfc_delete_space(sfp_serial_no);

				memset(sfp_date_code,0,sizeof(sfp_date_code));
				memcpy(sfp_date_code, ap->fw_init_p->sfp_info.sfp_date_code, sizeof(ap->fw_init_p->sfp_info.sfp_date_code)); 
				hfc_delete_space(sfp_date_code);

				if(!(ap->fw_init_p->sfp_info.sfp_status & HFC_SFP_VALID)){
					HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
						"hfcldd%d: SFP Information : incorrect data(%s, %s, %s)\n",
						ap->dev_minor,
						sfp_type_name,
						sfp_serial_no,
						sfp_date_code);
				}else{
					HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_HEAD, /* FCLNX-GPL-547 */
						"hfcldd%d: SFP Information : %s, %s, %s\n",
						ap->dev_minor,
						sfp_type_name,
						sfp_serial_no,
						sfp_date_code);
				}
			}
		}
	}/* FCLNX-GPL-311 */
	
/*----------------------------------*/
/*--- 1st errlog (log format 1)  ---*/
/*----------------------------------*/
/* error detail infomation 1 */
	/* error number */
	HFC_4L_TO_4B(err1->err_detail_1.err_num, err_num);
	
	/* current sysrev */
	if( ap->fw_init_p != NULL ) {
		err1->err_detail_1.curr_sysrev = ap->fw_init_p->fls_hdr.sys_rev;
	}
	
	/* mp_adap_info */
	mpap = ap->mp_adap_info;
	if( mpap != NULL ) {
		HFC_4L_TO_4B(err1->err_detail_1.mp_adap_status, mpap->status);
		err1->err_detail_1.mp_adap_lock	= (uchar)mpap->lock;
		err1->err_detail_1.mck_err_cnt	= mpap->mck_err_cnt;
	}
	
	/* interruption informaion */ /* And, PCIe link width data */
	if( ap->pci_cfginf != NULL ) {
		err1->err_detail_1.int_a_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
		if( !((ap->pkg.type == HFC_PKTYPE_FPP) || (ap->pkg.type == HFC_PKTYPE_FIVE)) ) {
			cfg_data = hfc_read_cnfg(ap, 0x4a, 0x2);
			HFC_2L_TO_2B(err1->err_detail_1.msi_msg_ctl, cfg_data);
			cfg_data = hfc_read_cnfg(ap, 0x62, 0x2);
			HFC_2L_TO_2B(err1->err_detail_1.msix_msg_ctl, cfg_data);
			err1->err_detail_1.vector_ctl0 = hfc_read_reg_ext(ap, 0xc0f, 1);
			err1->err_detail_1.vector_ctl1 = hfc_read_reg_ext(ap, 0xc1f, 1);
			pci_data = hfc_read_reg_ext(ap, 0xc00, 0x4);
			HFC_4L_TO_4B(err1->err_detail_1.msg_addr0, pci_data);
			pci_data = hfc_read_reg_ext(ap, 0xc08, 0x4);
			HFC_4L_TO_4B(err1->err_detail_1.msg_data0, pci_data);
			pci_data = hfc_read_reg_ext(ap, 0xc10, 0x4);
			HFC_4L_TO_4B(err1->err_detail_1.msg_addr1, pci_data);
			pci_data = hfc_read_reg_ext(ap, 0xc18, 0x4);
			HFC_4L_TO_4B(err1->err_detail_1.msg_data1, pci_data);
			
			/* FCLNX-GPL-227 start */
			/* Get max_link_width (cfgadr 7B-78h ,bit9-4) */
			cfg_data   = (ushort)hfc_read_cnfg(ap, 0x78, 0x2);
			cfg_data >>= 4;
			cfg_data  &= 0x003f;
			err1->err_detail_1.max_link_width = (uchar)cfg_data;
			
			/* Get negotiated_link_width (cfgadr 7F-7Eh ,bit9-4) */
			cfg_data   = (ushort)hfc_read_cnfg(ap, 0x7e, 0x2);
			cfg_data >>= 4;
			cfg_data  &= 0x003f;
			err1->err_detail_1.negotiated_link_width = (uchar)cfg_data;
			/* FCLNX-GPL-227 end */
		}
	}
	
	/* error dependent data */
	if( ( data != NULL ) && ( data_len != 0 ) ){
		if( data_len > 16 )
			data_len = 16;
		memcpy(&(err1->err_detail_1.drv_log[0]), data, data_len) ;
	}
	
	if( ( ( type == HFC_ERRLOG_TYPE_MCK ) ||				/* FCLNX_GPL-0162 */
		 (type == HFC_ERRLOG_TYPE_CHKSTP) ||
		 (type == HFC_ERRLOG_TYPE_IMLLOG)) ) {
		/* Shadow LPAR driver saves driver log for Guest LPAR driver.*/
		if ( HFC_MMODE_CHECK_SHADOW(ap) ) {
			HFC_MEMCPY(ap->mlpf_drv_log, err_rec, 64 ); /* FCLNX-GPL-184 */
		}
	}														/* FCLNX_GPL-0162 */
	
/* Common error infomation                */
/* --- FW_INIT_TABLE,MAILBOX,XOB,XRB -----*/
	hfc_error_common(ap, err1, hfcp) ;
	
/* --- Common Driver Infomation ---------*/
/* adapter infomation */
	HFC_4L_TO_4B(err1->err_ap.status, ap->status);
	scsi_id = (uint)ap->scsi_id;
	HFC_4L_TO_4B(err1->err_ap.scsi_id, scsi_id);
	HFC_2L_TO_2B(err1->err_ap.xob_no, ap->xob_no);
	HFC_2L_TO_2B(err1->err_ap.xrb_no, ap->xrb_no);
	err1->err_ap.xob_wait_exec_cnt	= ap->xob_wait_exec_cnt ;
	err1->err_ap.xob_exec_cnt		= ap->xob_exec_cnt ;
	err1->err_ap.mb_status			= ap->mb_status ;
	err1->err_ap.open_status		= ap->open_status ;
	err1->err_ap.dev_minor			= ap->dev_minor ;
	err1->err_ap.host_no			= ap->host_no ;
	err1->err_ap.instance			= ap->instance ;
	err1->err_ap.rid				= ap->rid ;
	if( HFC_MMODE_CHECK_SHARED(ap)  && !(HFC_MMODE_CHECK_SHADOW(ap) ) ){ /* FCLNX-GPL-547 */
		err1->err_ap.rid_hg = (uchar)hfc_read_hg_reg(ap, HFC_IOHGSPC_RID, 0x4);
	}	/* FCLNX-GPL-549 */
	HFC_2L_TO_2B(err1->err_ap.mb_retry_cnt, ap->mb_retry_cnt);
	HFC_2L_TO_2B(err1->err_ap.cmnd_no, ap->cmnd_no);
	HFC_2L_TO_2B(err1->err_ap.target_cnt, ap->target_cnt);
	HFC_2L_TO_2B(err1->err_ap.next_dstart_cnt, ap->next_dstart_cnt);
	HFC_4L_TO_4B(err1->err_ap.iov_map_cnt, ap->iov_map_cnt);
	HFC_4L_TO_4B(err1->err_ap.curr_xob_outp, ap->curr_xob_outp);
	HFC_4L_TO_4B(err1->err_ap.frame_chkp, ap->frame_chkp);
	HFC_4L_TO_4B(err1->err_ap.frame_inp, ap->frame_inp);
	err1->err_ap.mlpf_mode			= ap->mlpf_mode ;
	err1->err_ap.vector0 = ap->entries[0].vector;
	err1->err_ap.vector1 = ap->entries[1].vector;
	HFC_4L_TO_4B(err1->err_ap.trc_num, ap->trc_num);
	HFC_2L_TO_2B(err1->err_ap.pkt_no, ap->pkt_no);
	HFC_2L_TO_2B(err1->err_ap.pkt_cnt, ap->pkt_cnt);
	HFC_8L_TO_8B(err1->err_ap.scsi_exec_cnt, ap->scsi_exec_cnt);
	for(i=0;i<MAX_FRAME_CNT;i++)
		HFC_4L_TO_4B(err1->err_ap.save_xob_outp[i], ap->save_xob_outp[i]);
	for(i=0;i<MAX_FRAME_CNT;i++)
		HFC_4L_TO_4B(err1->err_ap.xob_outp_end[i], ap->xob_outp_end[i]);

/* target infomation */
	if( (target == NULL) && (ap->target_arg[0] != NULL) )
		target = ap->target_arg[0] ;
	if( target != NULL )
	{
		HFC_4L_TO_4B(err1->err_target.status, target->status);
		err1->err_target.flags		= target->flags ;
		err1->err_target.target_id	= target->target_id ;
		err1->err_target.fc_class	= target->fc_class ;
		err1->err_target.pseq		= target->pseq ;
		HFC_4L_TO_4B(err1->err_target.wx_que_cnt, target->wx_que_cnt);
		HFC_4L_TO_4B(err1->err_target.we_que_cnt, target->we_que_cnt);
		HFC_8L_TO_8B(err1->err_target.scsi_id, target->scsi_id);
		HFC_8L_TO_8B(err1->err_target.ww_name, target->ww_name);
	}
	
/* device infomation */	/* FCLNX-GPL-0343 */
	if( hfcp != NULL ){
		dev = hfcp->dev;
	}
	if( dev != NULL ){
		err1->err_dev.flags			= dev->flags;
		err1->err_dev.status		= dev->status;
		err1->err_dev.lustat		= dev->lustat;
		err1->err_dev.target_id		= dev->target_id;
		err1->err_dev.owner_ctl		= dev->owner_ctl;
		err1->err_dev.priority 		= dev->priority;
		err1->err_dev.group_id		= dev->group_id;
		err1->err_dev.path_id 		= dev->path_id;

		dev_data = (uint)dev->ioerror;
		HFC_4L_TO_4B(err1->err_dev.ioerror, dev_data);
		dev_data = (uint)dev->iocount;
		HFC_4L_TO_4B(err1->err_dev.iocount, dev_data);
		dev_data = (uint)dev->id_size;
		HFC_4L_TO_4B(err1->err_dev.id_size, dev_data);
		dev_data = (uint)dev->io_status;
		HFC_4L_TO_4B(err1->err_dev.io_status, dev_data);
		dev_data = (uint)dev->wx_que_cnt;
		HFC_4L_TO_4B(err1->err_dev.wx_que_cnt, dev_data);
		dev_data = (uint)dev->we_que_cnt;
		HFC_4L_TO_4B(err1->err_dev.we_que_cnt, dev_data);

		lun_id = (ushort)dev->lun;
		HFC_2L_TO_2B(err1->err_dev.lun, lun_id);
	}	/* FCLNX-GPL-0343 */
	HFC_8L_TO_8B(err1->err_dev.org_ww_name, ap->org_ww_name);	/* FCLNX-GPL-489 */
	
/* error detail infomation 2 */
	if( (type == HFC_ERRLOG_TYPE_MCK) || (type == HFC_ERRLOG_TYPE_CHKSTP) ) {	/* FCLNX-GPL-0131 */
		if(ap->pkg.type == HFC_PKTYPE_FPP) {
			/* F-MCK code 4byte * 1port */
			pci_data = (uint) hfc_read_reg_ext(ap, 0x3C0, 0x4);
			HFC_4L_TO_4B(err1->err_detail_2.uni.mck.ws_func0, pci_data);
		}
		else {
			/* F-MCK code 4byte * 4port */
			pci_data = (uint) hfc_read_reg_ext(ap, 0x300+0x60, 0x4);
			HFC_4L_TO_4B(err1->err_detail_2.uni.mck.ws_func0, pci_data);
			pci_data = (uint) hfc_read_reg_ext(ap, 0x380+0x60, 0x4);
			HFC_4L_TO_4B(err1->err_detail_2.uni.mck.ws_func1, pci_data);
			pci_data = (uint) hfc_read_reg_ext(ap, 0x400+0x60, 0x4);
			HFC_4L_TO_4B(err1->err_detail_2.uni.mck.ws_func2, pci_data);
			pci_data = (uint) hfc_read_reg_ext(ap, 0x480+0x60, 0x4);
			HFC_4L_TO_4B(err1->err_detail_2.uni.mck.ws_func3, pci_data);
		}
	}																			/* FCLNX-GPL-0131 */
    else if( type == HFC_ERRLOG_TYPE_MBINIT ){		/* mb_initiate_type */
		memcpy(err1->err_detail_2.uni.mb.mb_initiate, &(ap->mb->mb_init.type.resvx[8]), 16) ;
	}
	else if( type == HFC_ERRLOG_TYPE_MBRESP ){		/* mb_response_type */
		memcpy(err1->err_detail_2.uni.mb.mb_initiate, &(ap->mb->mb_init.type.resvx[8]), 16) ;
		memcpy(err1->err_detail_2.uni.mb.mb_response, &(ap->mb->mb_resp.type.resvx[4]), 32) ;
		memcpy(err1->err_detail_2.uni.mb.init_table, (uchar *)&(ap->fw_init_p->max_exchange), 48) ;
	}
	else if( type == HFC_ERRLOG_TYPE_MBINT ){		/* mb_int_type */
		memcpy(err1->err_detail_2.uni.mbint.receive_frame, &(ap->mb->mb_intreq.type.resvx[512]), 96) ;
	}
	else if( hfcp != NULL ){						/* scsi type */
		if( hfcp->cmd_pkt != NULL ){
			memcpy(&(err1->err_detail_2.uni.scsi.cmnd[0]), hfcp->cmd_pkt->cmnd, 16) ;
			/* kernel 5.15+: scsi_cmnd->serial_number removed; use 0 for trace */
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.serial_number, 0);
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.retries, hfcp->cmd_pkt->retries);
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.allowed, hfcp->cmd_pkt->allowed);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
			sdev = hfcp->cmd_pkt->device;
			if( sdev != NULL ){
				rq = sdev->request_queue;
				if( rq != NULL ){
					wk_tmo_sec = (ushort)(rq->rq_timeout/HZ);					/* FCLNX-GPL-621 */
				}
			}
			HFC_2L_TO_2B(err1->err_detail_2.uni.scsi.rq_tmo_sec, wk_tmo_sec);
			
			wk_tmo_sec = 0;
			/* kernel 5.16+: cmnd->request removed; use scsi_cmd_to_rq() */
			{
				struct request *_rq = scsi_cmd_to_rq(hfcp->cmd_pkt);
				if (_rq != NULL)
					wk_tmo_sec = (ushort)(_rq->timeout/HZ);
			}
			HFC_2L_TO_2B(err1->err_detail_2.uni.scsi.req_tmo_sec, wk_tmo_sec);	/* FCLNX-GPL-621 */
			
			dev = hfcp->dev;
			if( dev != NULL ){
				err1->err_detail_2.uni.scsi.lustat = dev->lustat;
			}
#else
			wk_tmo_sec = (ushort)hfcp->cmd_pkt->timeout_per_command/HZ;
			HFC_2L_TO_2B(err1->err_detail_2.uni.scsi.tmo_sec, wk_tmo_sec);
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.resid, hfcp->cmd_pkt->resid);
			
			HFC_2L_TO_2B(err1->err_detail_2.uni.scsi.use_sg, hfcp->cmd_pkt->use_sg);
#endif
			HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.result, hfcp->cmd_pkt->result);
			err1->err_detail_2.uni.scsi.cmd_len = hfcp->cmd_pkt->cmd_len;
			/* kernel 5.4+: cmnd->tag removed; use scsi_cmd_to_rq()->tag */
			err1->err_detail_2.uni.scsi.tag =
				(uchar)scsi_cmd_to_rq(hfcp->cmd_pkt)->tag;
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
		if( target != NULL ) {
			err1->err_detail_2.uni.scsi.lustat = target->lustat[hfcp->lun_id];
		}
#endif
		
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.cmd_flags, hfcp->cmd_flags);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.adap_status, hfcp->adap_status);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.seg_cnt, hfcp->seg_cnt);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.data_size, hfcp->data_size);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.iov_no, hfcp->iov_no);
		HFC_4L_TO_4B(err1->err_detail_2.uni.scsi.iov_cnt, hfcp->iov_cnt);
		err1->err_detail_2.uni.scsi.cmd_xob = hfcp->cmd_xob;
		err1->err_detail_2.uni.scsi.target_id = hfcp->target_id;
		lun_id	= (ushort)hfcp->lun_id;								/* FCLNX-GPL-0343 */
		HFC_2L_TO_2B(err1->err_detail_2.uni.scsi.lun_id, lun_id);	/* FCLNX-GPL-0343 */
		err1->err_detail_2.uni.scsi.group_id = hfcp->group_id;
		err1->err_detail_2.uni.scsi.seg_info_count = ap->xob[hfcp->cmd_xob].seg_cnt;
		err1->err_detail_2.uni.scsi.seg_info[0].xob_segno = ap->xob[hfcp->cmd_xob].seg_info_xob[0].xob_segno;
		err1->err_detail_2.uni.scsi.seg_info[0].seg_len = ap->xob[hfcp->cmd_xob].seg_info_xob[0].seg_len;
		err1->err_detail_2.uni.scsi.seg_info[1].xob_segno = ap->xob[hfcp->cmd_xob].seg_info_xob[1].xob_segno;
		err1->err_detail_2.uni.scsi.seg_info[1].seg_len = ap->xob[hfcp->cmd_xob].seg_info_xob[1].seg_len;
	}
	
/* driver trace */
	trc_num = ap->trc_num ;
	if( trc_num == 0 )
		trc_num = HFC_MAX_TRCCNT - 1 ;
	else
		trc_num-- ;
	for(i=0 ; i<15 ; i++)
	{
		memcpy(err1->err_ddtrc1[i], (char*)&(ap->trc_ptr[trc_num].trc_id),32) ;
		if( trc_num == 0 )
			trc_num = HFC_MAX_TRCCNT - 1 ;
		else
			trc_num-- ;
	}
	for(i=0 ; i<16 ; i++)
	{
		err1->err_ddtrc2[i][0] = ap->trc_ptr[trc_num].trc_id ;
		err1->err_ddtrc2[i][1] = ap->trc_ptr[trc_num].trc_data[0] ;
		if( trc_num == 0 )
			trc_num = HFC_MAX_TRCCNT - 1 ;
		else
			trc_num-- ;
	}
/* logout format1 */
	if( !ap->raslog_install ){
		raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id1,
					 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
	}
	if( ( ap->raslog_install ) || (raslog != 0) ) {
		hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
	}

/*---------------------------------------*/
/*--- 2nd errlog (log format 2 or 3)  ---*/
/*---------------------------------------*/
	err2 = (hfc_errfmt2_t *)err_rec->log_area ;
	HFC_BZERO( (char*)err_rec, sizeof( struct hfc_err_rec ) );				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	errlog_info_p = 17;														/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	
/* HW_LOG ... MCK,IML,CHK-STP.... log format 3 */
	if( (type == HFC_ERRLOG_TYPE_IMLLOG) && (mpap != NULL) )
	{
		wk_err_num = 0xfffe0200;
		if ( ap->pkg.type == HFC_PKTYPE_FPP )
		{
			HFC_4L_TO_4B(err2->err_num, wk_err_num);
			memcpy(&err2->err_detail[60],(uchar *)mpap->hw_log,960) ;
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
			memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[240],1024);
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
			memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[496],1024);
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
		}
		else
		{
			if ( ap->pkg.type == HFC_PKTYPE_FIVE ) {
				HFC_4L_TO_4B(err2->err_num, wk_err_num);
				memcpy(&err2->err_detail[60],(uchar *)&mpap->hw_log[16],960) ;
			}
			else {
				memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[0],1024);
			}
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
			memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[256],1024);
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
			memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[512],1024);
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
			memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[768],1024);
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
		}
		if ( HFC_MMODE_CHECK_SHADOW(ap) ){					/* FCLNX_GPL-0113 */
			hfc_mlpf_errlog_slpar(ap);
		}
		return ;
	}
	if(  ((type == HFC_ERRLOG_TYPE_MCK) ||
		 (type == HFC_ERRLOG_TYPE_CHKSTP)) && (mpap != NULL) )
	{
		if( type == HFC_ERRLOG_TYPE_MCK ) {
			wk_err_num = 0xfffe0000 ;
		}
		if( type == HFC_ERRLOG_TYPE_CHKSTP ) {
			wk_err_num = 0xfffe0100 ;
		}
		if ( ap->pkg.type == HFC_PKTYPE_FPP )
		{
			HFC_4L_TO_4B(err2->err_num, wk_err_num);
			memcpy(&err2->err_detail[60],(uchar *)mpap->hw_log,960) ;
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
		}
		else if ( ap->pkg.type == HFC_PKTYPE_FIVE )
		{
			HFC_4L_TO_4B(err2->err_num, wk_err_num);
			memcpy(&err2->err_detail[60],(uchar *)&mpap->hw_log[16],960) ;
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
			memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[256],1024);
			if( !ap->raslog_install ){
				raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
			}
			if( ( ap->raslog_install ) || (raslog != 0) ) {
				hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
			}
		}
		else {
			for(i=0 ; i<ap->max_hwlog_cnt ; i++) {
				memcpy(err_rec->log_area,(uchar *)&mpap->hw_log[i*256],1024);
				if( !ap->raslog_install ){
					raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
							 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
				}
				if( ( ap->raslog_install ) || (raslog != 0) ) {
					hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
				}
			}
		}
		if ( HFC_MMODE_CHECK_SHADOW(ap) ){					/* FCLNX_GPL-0113 */
			hfc_mlpf_errlog_slpar(ap);
		}
		return ;
	}
	if ( type == HFC_ERRLOG_TYPE_SRAMCE ) { /* FCLNX-GPL-116 */
		memcpy(err_rec->log_area,(uchar *)&ap->ce_log,1024);
		if( !ap->raslog_install ){
			raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
					 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
		}
		if( ( ap->raslog_install ) || (raslog != 0) ) {
			hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
		}
	}
	
/* Soft Log ... ICC,etc... log format 2 */
	wk_err_num = 0;
	if( type == HFC_ERRLOG_TYPE_XRB )
	{
		if( ap->xrb[ap->xrb_no].esw & HFC_ESW_MEINT_REPO )
		{
			ssn = ap->xrb[ap->xrb_no].ssn ;					/* Soft_Log_Area Start Number	*/
			son = ap->xrb[ap->xrb_no].son ;					/* Soft_Log_Area offset Number	*/
			HFC_2B_TO_2L(sbc, ap->xrb[ap->xrb_no].sbc);		/* Soft_Log Byte Count			*/
			wk_err_num = 0xfffffffe ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_MBRESP )
	{
		if( ap->mb->mb_resp.esw & HFC_ESW_MEINT_REPO )
		{
			ssn = ap->mb->mb_resp.ssn ;
			son = ap->mb->mb_resp.son ;
			HFC_2B_TO_2L(sbc, ap->mb->mb_resp.sbc);
			wk_err_num = 0xfffffffe ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_MBINT )
	{
		if( ap->mb->mb_intreq.type.fwintreq0.esw & HFC_ESW_MEINT_REPO )
		{
			ssn = ap->mb->mb_intreq.type.fwintreq0.ssn ;
			son = ap->mb->mb_intreq.type.fwintreq0.son ;
			HFC_2B_TO_2L(sbc, ap->mb->mb_intreq.type.fwintreq0.sbc);
			wk_err_num = 0xfffffffd ;
		}
	}
	if( type == HFC_ERRLOG_TYPE_TOUTLOG )
	{
		if( hfcp != NULL )
		{
			ssn = hfcp->tout_slog_ssn ;
			son = hfcp->tout_slog_son ;
			sbc = hfcp->tout_slog_sbc ;
			wk_err_num = 0xfffffffe ;
		}
	}
	if( sbc != 0 )
	{
		/* Slog_Entry length = zero ? */
		wk_slog_len = ap->fw_init_p->slog_len;
		wk_slog_num = ap->fw_init_p->slog_num;
		HFC_2B_TO_2L(slog_len, wk_slog_len);
		HFC_2B_TO_2L(slog_num, wk_slog_num);
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
		if( ( ap->pkg.type == HFC_PKTYPE_FPP ) || ( ap->pkg.type == HFC_PKTYPE_FIVE ) ) {
			if( sbc_wk > 960 )
				sbc_wk = 960 ;
			if( wk_err_num )
				HFC_4L_TO_4B(err2->err_num, wk_err_num);
			memcpy(&err2->err_detail[60],(uchar *)(ap->slog_addr[ssn]+(slog_len * son)),sbc_wk) ;			/* FCLNX_GPL-0121 */
		}
		else {
			if( sbc_wk > 1024 )
				sbc_wk = 1024 ;
			memcpy((uchar *)&err2->err_num, (uchar *)(ap->slog_addr[ssn]+(slog_len * son)), sbc_wk) ;		/* FCLNX_GPL-0121 */
		}
		if( !ap->raslog_install ){
			raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
						 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
		}
		if( ( ap->raslog_install ) || (raslog != 0) ) {
			hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
		}

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
				memcpy((uchar *)&err2->err_num, (uchar *)(ap->slog_addr[ssn]+(slog_len * son)), sbc_wk) ;	/* FCLNX_GPL-0121 */
				if( !ap->raslog_install ){
					raslog = hfc_raslog( ap, err_rec, (char *)&ras_error_id2,
								 (char *)&ras_error_id2,(char *)&resource_name, NULL,NULL,type);
				}
				if( ( ap->raslog_install ) || (raslog != 0) ) {
					hfc_errsave( ap, errlog_info_p, err_num, err_rec, errlog_size, 0 );
				}
			}
		}
	}
	return ;
}

void hfc_error_common(struct adap_info *ap, hfc_errfmt1_t *err1, struct hfc_pkt *hfcp)
{
	uint	work ;
	uint	num_work = 0 ;
	uint	fw_xob_inp = 0 ;
	uint	fw_xrb_outp = 0 ;

	if( ap == NULL ) return ;
	if( err1 == NULL ) return ;
	if( ap->fw_init_p == NULL ) return ;
	if( ap->mb == NULL ) return ;
	if( ap->xob == NULL ) return ;
	if( ap->xrb == NULL ) return ;
/*-- FW_INIT_TABLE --*/
	err1->err_init_tbl.connect_type = ap->fw_init_p->fw_iocinfo.connect_type ;
	err1->err_init_tbl.trans_rate = ap->fw_init_p->fw_iocinfo.trans_rate ;
	err1->err_init_tbl.flag = ap->fw_init_p->fw_iocinfo.flag ;
	err1->err_init_tbl.port_id = ap->fw_init_p->fw_iocinfo.port_id ;
	err1->err_init_tbl.port_name = ap->fw_init_p->fw_iocinfo.port_name ;
	err1->err_init_tbl.xob_inp = ap->fw_init_p->xob_inp ;
	err1->err_init_tbl.xob_outp = ap->fw_init_p->xob_outp ;
	err1->err_init_tbl.xrb_inp = ap->fw_init_p->xrb_inp ;
	err1->err_init_tbl.xrb_outp = ap->fw_init_p->xrb_outp ;
/*-- Mailbox --*/
	/* Mailbox reqest */
	err1->err_mb.command = ap->mb->mb_init.command ;
	err1->err_mb.sub_cmd = ap->mb->mb_init.sub_cmd ;
	err1->err_mb.dependent_code = ap->mb->mb_init.dependent_code ;
	err1->err_mb.pseq_no = ap->mb->mb_init.pseq_no ;
	memcpy(err1->err_mb.req_info,ap->mb->mb_init.type.drvctl.resv0,8) ;
	/* Mailbox response */
	err1->err_mb.flag = ap->mb->mb_resp.flag ;
	err1->err_mb.xcc = ap->mb->mb_resp.xcc ;
	err1->err_mb.r_esw = ap->mb->mb_resp.esw ;
	err1->err_mb.r_ssn = ap->mb->mb_resp.ssn ;
	err1->err_mb.r_son = ap->mb->mb_resp.son ;
	err1->err_mb.r_sbc = ap->mb->mb_resp.sbc ;
	err1->err_mb.r_fsb = ap->mb->mb_resp.fsb ;
	err1->err_mb.err_code[0] = ap->mb->mb_resp.err_code[0] ;
	err1->err_mb.err_code[1] = ap->mb->mb_resp.err_code[1] ;
	err1->err_mb.err_code[2] = ap->mb->mb_resp.err_code[2] ;
	memcpy(err1->err_mb.rsp_info,(char*)&(ap->mb->mb_resp.type.respcmd.cmd.command),4) ;
	/* Mailbox int */
	err1->err_mb.int_code = ap->mb->mb_intreq.type.fwintreq0.int_code ;
	err1->err_mb.sub_int_code = ap->mb->mb_intreq.type.fwintreq0.sub_int_code ;
	err1->err_mb.i_esw = ap->mb->mb_intreq.type.fwintreq0.esw ;
	err1->err_mb.detail = ap->mb->mb_intreq.type.fwintreq0.detail ;
	err1->err_mb.i_ssn = ap->mb->mb_intreq.type.fwintreq0.ssn ;
	err1->err_mb.i_son = ap->mb->mb_intreq.type.fwintreq0.son ;
	err1->err_mb.i_sbc = ap->mb->mb_intreq.type.fwintreq0.sbc ;
	err1->err_mb.rfv = ap->mb->mb_intreq.type.fwintreq0.rfv ;
	err1->err_mb.length = ap->mb->mb_intreq.type.fwintreq0.length ;
/*-- Xob --*/
	work = ap->fw_init_p->xob_inp ;
	HFC_4B_TO_4L(fw_xob_inp, work);
	num_work = ((fw_xob_inp & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	num_work += (fw_xob_inp & 0x0000ffff) ;
	if( num_work == 0 ) {
		num_work = ap->xob_max -1 ;
	}
	else {
		num_work-- ;
	}
	if( hfcp != NULL ) {
		num_work = hfcp->cmd_xob;
	}
	memcpy(err1->err_xob.drv_used, (uchar *)&(ap->xob[num_work].drv_work.hfc_pkt),8);
	err1->err_xob.flag = ap->xob[num_work].flag ;
	err1->err_xob.skip = ap->xob[num_work].skip ;
	err1->err_xob.bflag = ap->xob[num_work].bflag ;
	err1->err_xob.cflag = ap->xob[num_work].cflag ;
	err1->err_xob.d_info = ap->xob[num_work].d_info ;
	err1->err_xob.fcp_lun.lun = ap->xob[num_work].fcp_cmd.fcp_lun.lun ;
	err1->err_xob.fcp_cntl.qtype = ap->xob[num_work].fcp_cmd.fcp_cntl.qtype ;
	err1->err_xob.fcp_cntl.task_mgm = ap->xob[num_work].fcp_cmd.fcp_cntl.task_mgm ;
	err1->err_xob.fcp_cntl.data_type = ap->xob[num_work].fcp_cmd.fcp_cntl.data_type ;
	memcpy(&err1->err_xob.fcp_cdb[0], ap->xob[num_work].fcp_cmd.fcp_cdb, 16);
	err1->err_xob.length = ap->xob[num_work].fcp_cmd.fcp_dl ;
/*-- Xrb --*/
	work = ap->fw_init_p->xrb_outp ;
	HFC_4B_TO_4L(fw_xrb_outp, work);
	num_work = ((fw_xrb_outp & 0x00ff0000)>>16)*HFC_XRB_PER_PAGE ;
	num_work += (fw_xrb_outp & 0x0000ffff) ;
	if( num_work == 0 ) {
		num_work = ap->xrb_max -1 ;
	}
	else {
		num_work-- ;
	}
	err1->err_xrb.fcp_status0 = ap->xrb[num_work].resp_iu.fcp_status0 ;
	err1->err_xrb.fcp_status1 = ap->xrb[num_work].resp_iu.fcp_status1 ;
	err1->err_xrb.fcp_status2 = ap->xrb[num_work].resp_iu.fcp_status2 ;
	err1->err_xrb.scsi_status = ap->xrb[num_work].resp_iu.scsi_status ;
	err1->err_xrb.resid = ap->xrb[num_work].resp_iu.resid ;
	memcpy(err1->err_xrb.resp_info,ap->xrb[num_work].resp_iu.resp_info,4) ;
	err1->err_xrb.flag = ap->xrb[num_work].flag ;
	err1->err_xrb.xcc = ap->xrb[num_work].xcc ;
	err1->err_xrb.esw = ap->xrb[num_work].esw ;
	err1->err_xrb.skip = ap->xrb[num_work].skip ;
	err1->err_xrb.ssn = ap->xrb[num_work].ssn ;
	err1->err_xrb.son = ap->xrb[num_work].son ;
	err1->err_xrb.sbc = ap->xrb[num_work].sbc ;
	err1->err_xrb.fsb = ap->xrb[num_work].fsb ;
	memcpy(err1->err_xrb.err_code,ap->xrb[num_work].err_code,11) ;

	return ;
}

/*
 * Function:    hfc_errsave
 *
 * Purpose:     Output error log data 
 *
 * Arguments:   
 *  ap            - Pointer to adap_info 
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
void hfc_errsave( struct adap_info *ap,
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
	enum log_prefixs log_prefix = HFC_LOG_INF; /* FCLNX-GPL-547 */

	HFC_DBGPRT(" hfcldd : hfc_errsave - start\n");
	
	/* FCLNX-GPL-547 start */
	/* Table size check */
	if( err_id < ERRID_HFCP_TBL_END )
	{	/* Table size is correct. */
		log_prefix = log_prefix_tbl[err_id];
	}
	/* FCLNX-GPL-547 end */
	
	if( ap != NULL ){
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
		sprintf( c_instance, "%x", ap->dev_minor );	
		HFC_BZERO( ap->log_wk, sizeof( ap->log_wk ) );	/* FCLNX-GPL-147 */

		HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_BODY, "%s%d: %s (ErrNo:0x%02x)\n", /* FCLNX-GPL-547 */
			 name, ap->dev_minor, errlog_info[errlog_info_p].errmsg, err_num );	

		if(err_num == 0xAF){/* FCLNX-GPL-551 */
			return;
		}/* FCLNX-GPL-551 */

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
			sprintf( (char *)&ap->log_wk[lp2],
				 "0x%04x:[ %08x %08x %08x %08x ]\n",
				 ( lp * 4 +  offset ),
				 log0, log1, log2, log3);

			lp += 4;
			lp2 = strlen( ap->log_wk );
			lp3++;
			if(lp3 == 10 )  /* Ten line output: 470 bytes */
			{
				/* FCLNX-GPL-547 start */
				switch( log_prefix ){
					case HFC_LOG_ERR:
						HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "%s", ap->log_wk );
						break;
					case HFC_LOG_WRN:
						HFC_WRNPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "%s", ap->log_wk );
						break;
					case HFC_LOG_INF:
					default :
						HFC_INFPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "%s", ap->log_wk );
						break;
				}
				/* FCLNX-GPL-547 end */
				lp2 = 0;
				lp3 = 0;
				HFC_BZERO( ap->log_wk, sizeof( ap->log_wk ) );	
			}
		}	

		switch( amari ){
			case 1 :
			case 2 :
			case 3 :
			case 4 :
				wlog0 = (uint)err_info[lp];
				HFC_4L_TO_4B(log0, wlog0);
				sprintf( (char *)&ap->log_wk[lp2],	
					 "0x%04x:[ %08x ] \n",
					 ( lp * 4 + offset ), wlog0 );
				lp++;
				lp2 = strlen( ap->log_wk );
				break;
			case 5 :
			case 6 :
			case 7 :
			case 8 :
				wlog0 = (uint)err_info[lp];
				wlog1 = (uint)err_info[lp+1];
				HFC_4L_TO_4B(log0, wlog0);
				HFC_4L_TO_4B(log1, wlog1);
				sprintf( (char *)&ap->log_wk[lp2],	
					 "0x%04x:[ %08x %08x ] \n",
					 ( lp * 4 + offset ),
					 wlog0, wlog1);
				lp += 2;
				lp2 = strlen( ap->log_wk );
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
				sprintf( (char *)&ap->log_wk[lp2],	
					 "0x%04x:[ %08x %08x %08x ] \n",
					 ( lp * 4 + offset ),
					 wlog0, wlog1, wlog2);
				lp += 3;
				lp2 = strlen( ap->log_wk );	
				break;
			default :
				break;
		}
		/* FCLNX-GPL-547 start */
		switch( log_prefix ){
			case HFC_LOG_ERR:
				HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "%s", ap->log_wk);
				HFC_ERRPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "\n");
				break;
			case HFC_LOG_WRN:
				HFC_WRNPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "%s", ap->log_wk);
				HFC_WRNPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "\n");
				break;
			case HFC_LOG_INF:
			default :
				HFC_INFPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "%s", ap->log_wk);
				HFC_INFPRT_S(err_id, ap->log_file, HFC_LOG_DUMP, "\n");
				break;
		}
		/* FCLNX-GPL-547 end */
	}

	return;
}


/*
 * Function:    hfc_raslog
 *
 * Purpose:     Output error log data to raslog files
 *
 * Arguments:   
 *  ap            - Pointer to adap_info 
 *  err_rec       - Pointer to hfc_err_rec 
 *  ras_error_id  - Pointer to hraslog_errid
 *  uchar         - Pointer to resource_name
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_raslog( struct adap_info *ap,
				  struct hfc_err_rec *err_rec,
				  char 	*ras_error_id1,
				  char 	*ras_error_id2,
				  char	 *resource_name,
				  struct hfc_pkt		*hfcp,
				  struct scsi_cmnd		*cmnd,
				  uchar	 type)
{
//	uchar detail_data[HFC_RASLOG_LEN];		/* FCLNX_GPL-0151 */
	uint raslog=2;
#ifndef _HFC_NO_RASLOG
	char	sfp_type_name[HFC_SFP_TYPE_NAME_LEN+1];	/* FCLNX-GPL-311 */
	char	sfp_serial_no[HFC_SFP_SERIAL_NO_LEN+1];	/* FCLNX-GPL-311 */
	char	sfp_date_code[HFC_SFP_DATE_CODE_LEN+1];	/* FCLNX-GPL-311 */
	uint raslog_retry;
	struct hfc_vpd			*vpd_info=NULL;
	struct hfc_vpd_five		*vpdf_info=NULL;
	struct hfc_vpd_five_ex	*vpdex_info=NULL;
#endif
	HFC_DBGPRT(" hfcldd : hfc_raslog - start\n");

	memset(ap->detail_data, 0, sizeof(ap->detail_data));		/* FCLNX_GPL-0151 */
	if( ap == NULL ){
#ifndef _HFC_NO_RASLOG
		memcpy( ap->detail_data, &err_rec->log_area[0], 64);
		raslog = _hraslog((char *)ras_error_id1, (char *)resource_name, 64, (char *)ap->detail_data );
#endif
		return(raslog);
	}

#ifndef _HFC_NO_RASLOG
	memcpy( ap->detail_data, &err_rec->log_area[0], 512);
	sprintf( &ap->detail_data[HFC_MFC], "%s", "Hitachi");
	sprintf( &ap->detail_data[HFC_MODELNAME], "%s", ap->mp_adap_info->model_name);
	if(ap->pkg.type == HFC_PKTYPE_FPP){
		vpd_info = (struct hfc_vpd *)ap->mp_adap_info->vpd_buf;
		sprintf( &ap->detail_data[HFC_FWVERSION], "%06x", vpd_info->fw_ver);
		sprintf( &ap->detail_data[HFC_DRVVERSION], "%s", vpd_info->driver_ver);
		sprintf( &ap->detail_data[HFC_PARTSNUM], "%s", vpd_info->pn_value);
		sprintf( &ap->detail_data[HFC_EC], "%c", vpd_info->ec_value[0]);
	}
	else if(ap->pkg.type == HFC_PKTYPE_FIVE){
		vpdf_info = (struct hfc_vpd_five *)ap->mp_adap_info->vpd_buf;
		sprintf( &ap->detail_data[HFC_FWVERSION], "%06x", hfc_get_sysrev(ap)); /* FCLNX-GPL-112 */
		sprintf( &ap->detail_data[HFC_DRVVERSION], "%s", vpdf_info->driver_ver);
		sprintf( &ap->detail_data[HFC_PARTSNUM], "%s", vpdf_info->pn_value);
		sprintf( &ap->detail_data[HFC_EC], "%c", vpdf_info->ec_level);
	}
	else {
		vpdex_info = (struct hfc_vpd_five_ex *)ap->mp_adap_info->vpd_buf;
		sprintf( &ap->detail_data[HFC_FWVERSION], "%06x", hfc_get_sysrev(ap)); /* FCLNX-GPL-112 */
		sprintf( &ap->detail_data[HFC_DRVVERSION], "%s", vpdex_info->driver_ver);
		sprintf( &ap->detail_data[HFC_PARTSNUM], "%s", vpdex_info->pn_value);
		sprintf( &ap->detail_data[HFC_EC], "%c", vpdex_info->ec_level);
	}
	sprintf( &ap->detail_data[HFC_LOCATION], "%02x:%02x.%02x", 
			ap->pci_cfginf->bus->number,PCI_SLOT(ap->pci_cfginf->devfn),PCI_FUNC(ap->pci_cfginf->devfn));
	sprintf( &ap->detail_data[HFC_LOGSEQNUM], "%d", ap->raslog_cnt);
	sprintf( &ap->detail_data[HFC_IRQNUM], "%d", ap->pci_cfginf->irq);
	sprintf( &ap->detail_data[HFC_ADAPWWPN], "%llx", (unsigned long long)ap->ww_name);
	sprintf( &ap->detail_data[HFC_DEVICEID], "%04x", (short)ap->pkg.device_id);

	/* Output SFP Information *//* FCLNX-GPL-311 */
	if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support)||(ap->fw_init_p->func & HFC_FWF_SFPINF)){			/* FCLNX-GPL-325 *//* FCLNX-GPL-393 */
		if(!(ap->fw_init_p->sfp_info.sfp_status & HFC_SFP_INSTALL)){
				sprintf( &ap->detail_data[HFC_SFP_INFO], "N/A");
		}else {
			memset(sfp_type_name,0,sizeof(sfp_type_name));
			memcpy(sfp_type_name, ap->fw_init_p->sfp_info.sfp_type_name, sizeof(ap->fw_init_p->sfp_info.sfp_type_name)); 
			hfc_delete_space(sfp_type_name);
				
			memset(sfp_serial_no,0,sizeof(sfp_serial_no));
			memcpy(sfp_serial_no, ap->fw_init_p->sfp_info.sfp_serial_no, sizeof(ap->fw_init_p->sfp_info.sfp_serial_no)); 
			hfc_delete_space(sfp_serial_no);

			memset(sfp_date_code,0,sizeof(sfp_date_code));
			memcpy(sfp_date_code, ap->fw_init_p->sfp_info.sfp_date_code, sizeof(ap->fw_init_p->sfp_info.sfp_date_code)); 
			hfc_delete_space(sfp_date_code);

			if(!(ap->fw_init_p->sfp_info.sfp_status & HFC_SFP_VALID)){
				sprintf( &ap->detail_data[HFC_SFP_INFO], "incorrect data(%s, %s, %s)",
						sfp_type_name,
						sfp_serial_no,
						sfp_date_code);
			}else{
				sprintf( &ap->detail_data[HFC_SFP_INFO], "%s, %s, %s",
						sfp_type_name,
						sfp_serial_no,
						sfp_date_code);
			}
		}
	}/* FCLNX-GPL-311 */
#endif

#ifndef _HFC_NO_RASLOG
	for(raslog_retry=0; raslog_retry < HFC_RASLOG_RETRY; raslog_retry++){
		raslog = _hraslog((char *)ras_error_id1, (char *)resource_name, HFC_RASLOG_LEN, (char *)ap->detail_data ); /* FCLNX-GPL-311 */
		if( raslog == 0 ){
			ap->raslog_cnt++; 
			break;

		}
	}
	if( raslog == 1 ){
		HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBD)\n", ap->dev_minor); 
	}
	else if( raslog == 2 ){
		HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBC)\n", ap->dev_minor); 
	}
	else if( raslog == 0 ){
		memcpy( ap->detail_data, &err_rec->log_area[512], 512);
		sprintf( &ap->detail_data[HFC_LOGSEQNUM], "%d", ap->raslog_cnt);
		for(raslog_retry=0; raslog_retry < HFC_RASLOG_RETRY; raslog_retry++){
			raslog = _hraslog((char *)ras_error_id2, (char *)resource_name, HFC_RASLOG_LEN, (char *)ap->detail_data ); /* FCLNX-GPL-311 */
			if( raslog == 0 ){
				ap->raslog_cnt++; 
				break;
			}
		}
		if( raslog == 1 ){
			HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBD)\n", ap->dev_minor); 
		}
		else if( raslog == 2 ){
			HFC_ERRPRT("hfcldd%d: HFC_EVNT3 FC Adapter Driver Warning Event (ErrNo:0xBC)\n", ap->dev_minor); 
		}
	}
#endif

	return(raslog);

}




/*
 * Function:    hfc_fpp_logout
 *
 * Purpose:     LOG OUT for FPPf
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  err_no     - Error code
 *  mode       - mode
 *
 * Returns:     
 *
 * Notes:       
 */
 /* FCLNX-GPL-147 */
const Type_mem fpp_iml_log[] = {
	/*---type-----address------size--*/
	{ TYPE_CFG , 0x0		, 80 } ,		/* PCI Config Reg.	*/
	{ TYPE_PCI , 0x0		, 32 } ,		/* PCI Memory		*/
	{ TYPE_PCI , 0x200		, 32 } ,		/* *				*/
	{ TYPE_PCI , 0x300		, 32 } ,		/*					*/
	{ TYPE_PCI , 0x800		, 256 } ,		/*					*/
	{ TYPE_TRC0, 0x41000000 , 528 } ,		/* TRC				*/
	{ TYPE_ELOG, 0			, 0   } ,		/*------------------*/
	{ TYPE_TRC1, 0x41000000 , 1024 } ,		/* TRC				*/
	{ TYPE_PCI , 0x900		, 1024 } ,		/* PCI				*/
	{ 0		   , 0			, 0    } ,
};

const Type_mem fpp_mck_loga0[] = {
	/*---type-----address------size--*/
	{ TYPE_SEQ , 0			, 8  }, 		/* Sequense No		*/
	{ TYPE_PCI , 0x0		, 48 } ,		/* PCI				*/
	{ TYPE_PCI , 0x200		, 16 } ,
	{ TYPE_PCI , 0x800		, 208 } ,
	{ TYPE_TRC0, 0x41000000 , 120 } ,		/* TRC0 			*/
	{ TYPE_TRC1, 0x41000000 , 512 } ,		/* TRC1 			*/
	{ TYPE_CFG , 0x4		, 48  } ,		/* PCI				*/
	{ TYPE_ELOG, 0			, 0   } ,		/*------------------*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x8d0		, 1072 } ,		/* PCI				*/
	{ TYPE_PCI , 0x210		, 16   } ,
	{ TYPE_TRC0, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_TRC1, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_0   , 0x00000200 , 512  } ,		/* WS				*/
	{ TYPE_0   , 0x00000800 , 256  } ,		/* WS				*/
	{ TYPE_0   , 0x00000a00 , 1536 } ,		/* WS				*/
	{ TYPE_01  , 0x41000000 , 4480 } ,		/* RHDR/THDR/TBUF	*/
	{ TYPE_0   , 0x43000000 , 512  } ,		/* BCR				*/
	{ TYPE_ILS , 0x08000000 , 2048 } ,		/* ILS				*/
	{ 0		   , 0			, 0    } ,
};

const Type_mem fpp_mck_log84[] = {
	/*---type-----address------size--*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x0		, 48 } ,		/* PCI				*/
	{ TYPE_PCI , 0x200		, 16 } ,
	{ TYPE_PCI , 0x800		, 208 } ,
	{ TYPE_TRC0, 0x41000000 , 168 } ,		/* TRC0 			*/
	{ TYPE_PCI , 0xb10		, 208 } ,		/* PCI				*/
	{ TYPE_PCI , 0xc40		, 48  } ,
	{ TYPE_0   , 0x03000000 , 256 } ,		/* BCR				*/
	{ TYPE_ELOG, 0			, 0   } ,		/*------------------*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x8d0		, 1072 } ,		/* PCI				*/
	{ TYPE_PCI , 0x210		, 16   } ,
	{ TYPE_TRC0, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_TRC1, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_0   , 0x00000200 , 512  } ,		/* WS				*/
	{ TYPE_0   , 0x00000800 , 256  } ,		/* WS				*/
	{ TYPE_0   , 0x00000a00 , 1536 } ,		/* WS				*/
	{ TYPE_01  , 0x41000000 , 4480 } ,		/* RHDR/THDR/TBUF	*/
	{ TYPE_0   , 0x43000000 , 512  } ,		/* BCR				*/
	{ TYPE_ILS , 0x08000000 , 2048 } ,		/* ILS				*/
	{ 0		   , 0			, 0    } ,
};

const Type_mem fpp_mck_log82[] = {
	/*---type-----address------size--*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x0		, 48 } ,		/* PCI				*/
	{ TYPE_PCI , 0x200		, 16 } ,		
	{ TYPE_PCI , 0x800		, 208 } ,		
	{ TYPE_TRC0 ,0x41000000 , 168 } ,		/* TRC0 			*/
	{ TYPE_PCI , 0x950		, 256 } ,		/* PCI				*/
	{ TYPE_ILS , 0x08000000 , 256 } ,		
	{ TYPE_ELOG, 0			, 0   } ,		/*------------------*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x8d0		, 1072 } ,		/* BCR				*/
	{ TYPE_PCI , 0x210		, 16   } ,		/* PCI				*/
	{ TYPE_TRC0, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_TRC1, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_0   , 0x00000200 , 512  } ,		/* WS				*/
	{ TYPE_0   , 0x00000800 , 256  } ,		/* WS				*/
	{ TYPE_0   , 0x00000a00 , 1536 } ,		/* WS				*/
	{ TYPE_01  , 0x41000000 , 4480 } ,		/* RHDR/THDR/TBUF	*/
	{ TYPE_0   , 0x43000000 , 512  } ,		/* BCR				*/
	{ TYPE_ILS , 0x08000000 , 2048 } ,		/* ILS				*/
	{ 0		   , 0			, 0    } ,
};

const Type_mem fpp_mck_log81[] = {
	/*---type-----address------size--*/ 	
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x0		, 48 } ,		
	{ TYPE_PCI , 0x200		, 16 } ,		
	{ TYPE_PCI , 0x800		, 208 } ,		
	{ TYPE_TRC0, 0x41000000 , 168 } ,		/* TRC0 			*/
	{ TYPE_PCI , 0xa00		, 256 } ,		
	{ TYPE_ILS , 0x08000000 , 256 } ,		/* BCR				*/
	{ TYPE_ELOG, 0			, 0   } ,		/*------------------*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x8d0		, 1072 } ,		/* PCI				*/
	{ TYPE_PCI , 0x210		, 16   } ,		
	{ TYPE_TRC0, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_TRC1, 0x41000000 , 2048 } ,		/* TRC				*/
	{ TYPE_0   , 0x00000200 , 512  } ,		/* WS				*/
	{ TYPE_0   , 0x00000800 , 256  } ,		/* WS				*/
	{ TYPE_0   , 0x00000a00 , 1536 } ,		/* WS				*/
	{ TYPE_01  , 0x41000000 , 4480 } ,		/* RHDR/THDR/TBUF	*/
	{ TYPE_0   , 0x43000000 , 512  } ,		/* BCR				*/
	{ TYPE_ILS , 0x08000000 , 2048 } ,		/* ILS				*/
	{ 0		   , 0			, 0    } ,
};

const Type_mem fpp_mck_other[] = {
	/*---type-----address------size--*/
	{ TYPE_SEQ , 0			, 8 },			/* Sequense No		*/
	{ TYPE_PCI , 0x0		, 48 } ,		 /* PCI 			*/
	{ TYPE_PCI , 0x200		, 16 } ,			
	{ TYPE_PCI , 0x800		, 208 } ,			
	{ TYPE_PCI , 0x300		, 32  } ,		 /* TRC0			*/
	{ TYPE_TRC0, 0x41000000 , 648 } ,		 /* PCI 			*/
	{ TYPE_ELOG, 0			, 0   } ,		 /*-----------------*/
	{ TYPE_SEQ , 0			, 8 },			 /* Sequense No 	*/
	{ TYPE_PCI , 0x8d0		, 1072 } ,			
	{ TYPE_PCI , 0x210		, 16   } ,		 /* BCR 			*/
	{ TYPE_TRC0, 0x41000000 , 2048 } ,		 /* TRC 			*/
	{ TYPE_TRC1, 0x41000000 , 2048 } ,		 /* TRC 			*/
	{ TYPE_0   , 0x00000200 , 512  } ,			
	{ TYPE_0   , 0x00000800 , 256  } ,		 /* TRC 	Type0/1 */
	{ TYPE_0   , 0x00000a00 , 1536 } ,		 /* WS				*/
	{ TYPE_01  , 0x41000000 , 4480 } ,		 /* RHDR/THDR/TBUF	*/
	{ TYPE_0   , 0x43000000 , 512  } ,		 /* BCR 			*/
	{ TYPE_ILS , 0x08000000 , 2048 } ,		 /* ILS 			*/
	{ 0		   , 0			, 0    } ,
};
 /* FCLNX-GPL-147 */
 
void hfc_fpp_logout( struct adap_info *ap, uint err_no, uchar mode )
{

	struct mp_adap_info     *mpap ;
	Type_mem	*logmap;
	uchar		err_detail;
	int 		log_offset;
	int 		num;
/*	int 		i, rc;*/ /* FCLNX-GPL-111 */
	int 		i;
	uint		reg_adr;
	uint		reg_adr_curr;
	uint        wk4;

	HFC_DBGPRT(" hfcldd%d : hfc_logout - start\n",ap->dev_minor);

	mpap = ap->mp_adap_info;

#if 0
	/* FCLNX-GPL-111 */
	rc = lock_try_mpap( mpap );										/* FCLNX-0279 */
	
	if( rc == 0 ){	/* Lock acquisition failure	*/
		set_bit( HFC_LOCK_WAIT_3, (ulong *)&ap->mpap_lock );
		ap->err_no = err_no;
		ap->mode = mode;
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}

	if ( test_bit( HFC_LOCK_WAIT_3, (ulong *)&ap->mpap_lock ) ) {
		/* stop retry timer */
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		clear_bit( HFC_LOCK_WAIT_3, (ulong *)&ap->mpap_lock );
	}																/* FCLNX-0279 */
#endif

	if( mode == HFC_ERRLOG_TYPE_IMLLOG ){
		logmap = (Type_mem *) fpp_iml_log;		/* FCLNX-GPL-147 */
	} else {
		err_detail =
			 hfc_read_reg_ext( ap, (uint)HFC_IOSPACE_ERRDETAIL0, 0x1 );
		if( err_detail == 0xa0 )
			logmap = (Type_mem *) fpp_mck_loga0; /* FCLNX-GPL-147 */
		else if( err_detail == 0x84 )
			logmap = (Type_mem *) fpp_mck_log84; /* FCLNX-GPL-147 */
		else if( err_detail == 0x82 )
			logmap =(Type_mem *)  fpp_mck_log82; /* FCLNX-GPL-147 */
		else if( err_detail == 0x81 )
			logmap = (Type_mem *) fpp_mck_log81; /* FCLNX-GPL-147 */
		else
			logmap = (Type_mem *) fpp_mck_other; /* FCLNX-GPL-147 */
		mpap->mck_seq_no++;
	}
	
	log_offset = 0;
	num = 0;

	while( (log_offset < HFC_HWLOG_SIZE/4) && (logmap[num].type != 0) )
	{
		if( logmap[num].type == TYPE_SEQ )
		{
			mpap->hw_log[log_offset] = err_no ;
			mpap->hw_log[log_offset+1] = mpap->mck_seq_no ;
			log_offset+=2 ;
		}
		else if( logmap[num].type == TYPE_PCI )
		{
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				wk4 = (uint)hfc_read_reg_ext(									/* FCWIN-0197 */
					ap, logmap[num].reg_adr+reg_adr, (char)0x4) ;
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
		}
		else if( logmap[num].type == TYPE_CFG )
		{
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				wk4 = (uint)hfc_read_cnfg(										/* FCWIN-0197 */
					ap, logmap[num].reg_adr+reg_adr, (char)0x4) ;
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
		}
		else if( (logmap[num].type == TYPE_0) || (logmap[num].type == TYPE_1) ||
				 (logmap[num].type == TYPE_01) )
		{
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,logmap[num].type);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,logmap[num].reg_adr);
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				wk4 = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4);			/* FCWIN-0197 */
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
		}
		else if( logmap[num].type == TYPE_ILS )
		{
			if( logmap[num].size < 2048 )
			{
				/* FRAME TRC */
				hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_0);
				hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,(uint)0x00000340); /* @1.82 */
				reg_adr_curr = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4) & 0x0000ffff;
				if( (reg_adr_curr < 0x9000) || (reg_adr_curr > 0x9fff) )
				{
					/* A current pointer is outside of the range */
					for(i = 0 ; i < logmap[num].size ; i+=4 )
					{
						mpap->hw_log[log_offset] = 0 ;
						log_offset++ ;
					}
				}
				else
				{
					reg_adr = reg_adr_curr - logmap[num].size/4 ;
					if( reg_adr < 0x9000 )
						reg_adr = 0xa000 - (0x9000 - reg_adr) ;
					reg_adr = reg_adr * 4 ;
					hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
					while( reg_adr != (reg_adr_curr*4) )
					{
						wk4 = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4);	/* FCWIN-0197 */
						HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
						reg_adr += 4 ;
						log_offset++ ;
						if( reg_adr >= (0xa000*4) )
						{
							reg_adr = 0x9000*4 ;
							hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
						}
					}
				}
			}
			else
			{
				/* EVENT TRC */
				hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_0);
				hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,(uint)0x00000324);
				reg_adr_curr = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4) & 0x0000ffff;
				if( (reg_adr_curr < 0x8000) || (reg_adr_curr > 0x8fff) )
				{
					/* A current pointer is outside of the range */
					for(i = 0 ; i < 1024 ; i+=4 )
					{
						mpap->hw_log[log_offset] = 0 ;
						log_offset++ ;
					}
				}
				else
				{
					reg_adr = reg_adr_curr - 0x100 ;
					if( reg_adr < 0x8000 )
						reg_adr = 0x9000 - (0x8000 - (reg_adr_curr - 0x100)) ;
					reg_adr = reg_adr * 4 ;
					hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
					while( reg_adr != (reg_adr_curr*4) )
					{
						wk4 = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4);	/* FCWIN-0197 */
						HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
						reg_adr += 4 ;
						log_offset++ ;
						if( reg_adr >= (0x9000*4) )
						{
							reg_adr = 0x8000*4 ;
							hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
						}
					}
				}
				/* FRAME TRC */
				hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_0);
				hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,(uint)0x00000340); /* @1.82 */
				reg_adr_curr = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4) & 0x0000ffff;
				if( (reg_adr_curr < 0x9000) || (reg_adr_curr > 0x9fff) )
				{
					/* A current pointer is outside the range */
					for(i = 0 ; i < 1024 ; i+=4 )
					{
						mpap->hw_log[log_offset] = 0 ;
						log_offset++ ;
					}
				}
				else
				{
					reg_adr = reg_adr_curr - 0x100 ;
					if( reg_adr < 0x9000 )
						reg_adr = 0xa000 - (0x9000 - reg_adr) ;
					reg_adr = reg_adr * 4 ;
					hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
					while( reg_adr != (reg_adr_curr*4) )
					{
						wk4 = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4);		/* FCWIN-0197 */
						HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
						reg_adr += 4 ;
						log_offset++ ;
						if( reg_adr >= (0xa000*4) )
						{
							reg_adr = 0x9000*4 ;
							hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
						}
					}
				}
			}
		}
		else if( logmap[num].type == TYPE_TRC0 )
		{
			reg_adr_curr = (uint)hfc_read_reg_ext(ap,(uint)0xf0,(char)0x2) + 0x800 ;
			reg_adr = reg_adr_curr - logmap[num].size/4 ;
			if( reg_adr < 0x800 )
				reg_adr = 0xc00 - ( 0x800 - reg_adr ) ;
			reg_adr = reg_adr * 4 ;
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_1);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
			for(i=0 ; i<logmap[num].size ; i+=4)
			{
				wk4 = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4);	/* FCWIN-0197 */
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
		}
		else if( logmap[num].type == TYPE_TRC1 )
		{
			reg_adr_curr = (uint)hfc_read_reg_ext(ap,(uint)0xf2,(char)0x2) + 0xc00 ;
			reg_adr = reg_adr_curr - logmap[num].size/4 ; ;
			if( reg_adr < 0xc00 )
				reg_adr = 0x1000 - ( 0xc00 - reg_adr ) ;
			reg_adr = reg_adr * 4 ;
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_1);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,reg_adr+logmap[num].reg_adr);
			for(i=0 ; i<logmap[num].size ; i+=4)
			{
				wk4 = (uint)hfc_read_reg_ext(ap,REG_INDAREA,(char)0x4);	/* FCWIN-0197 */
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
		}
		else if( logmap[num].type == TYPE_ELOG )
		{
			if( mode == HFC_ERRLOG_TYPE_CHKSTP )
				break ;
			if( test_bit(HFC_HWLOG_VALID, (ulong *)&ap->io_status) )
				break ;
			else
				set_bit(HFC_HWLOG_VALID, (ulong *)&ap->io_status );
		}
		num++ ;
	}
	hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(char)0x00);

/*	HFC_ADAP_UNLOCK( mpap, HFC_MP_ADAP_BUSY ); */ /* FCLNX-GPL-111 */
	HFC_DBGPRT(" hfcldd : hfc_fpp_logout - end\n");
}

 /* FCLNX-GPL-147 */
const Type_mem five_iml_log[] = {
	/*---type-----address------size--*/
	{ TYPE_CFG    , 0x0        , 544   } , /* 0x0100 PCI Config Reg.  */    /*  FCWIN-0284  */
	{ TYPE_PCI    , 0x0        , 48    } , /* 0x0320 PCI Memory       */
	{ TYPE_PCI    , 0x10A0     , 64    } , /* 0x0350 INT Information  */
	{ TYPE_RSV    , 0x0        , 112   } , /* 0x0390 Reserve          */    /*  FCWIN-0284  */
	{ TYPE_TRC0   , 0x06000000 , 384   } , /* 0x0400 TRC              */
	{ TYPE_0      , 0x00000000 , 32    } , /* 0x0580 Com Area         */
	{ TYPE_0      , 0x00000080 , 32    } , /* 0x05A0                  */
	{ TYPE_0      , 0x00000100 , 32    } , /* 0x05C0                  */
	{ TYPE_0      , 0x00000180 , 32    } , /* 0x05E0                  */
	{ TYPE_TRC1   , 0x06002000 , 512   } , /* 0x0600 TRC              */
	{ TYPE_PCI    , 0x00001800 , 176   } , /* 0x0800 WA               */
	{ TYPE_PCI    , 0x00001040 , 64    } , /* 0x08B0                  */
	{ TYPE_PCI    , 0x000010E0 , 80    } , /* 0x08F0                  */
	{ TYPE_PCI    , 0x000011C0 , 64    } , /* 0x0940                  */
	{ TYPE_PCI    , 0x0210     , 40    } , /* 0x0980 MCW              */
	{ TYPE_PCI    , 0x000018B0 , 592   } , /* 0x09A8 WA               */
	{ TYPE_RSV    , 0x0        , 8     } , /* 0x0BF8 Reserve          */
	{ TYPE_PCI    , 0x00001B00 , 1024  } , /* 0x0C00 WA               */    /*  FCWIN-0265  */
	{ 0           , 0          , 0     } ,
};
	
	/* Mck log format for each installation port */
const Type_mem five_mck_iport1[] = {
	{ TYPE_SEQ    , 0x0        , 8     } , /* 0x0100 Sequence No      */
	{ TYPE_PCI    , 0x0        , 48    } , /* 0x0108 PCI              */
	{ TYPE_PCI    , 0x10A0     , 64    } , /* 0x0138 INT information  */
	{ TYPE_PCI    , 0x00001800 , 176   } , /* 0x0178 WA               */
	{ TYPE_PCI    , 0x00001040 , 64    } , /* 0x0228                  */
	{ TYPE_PCI    , 0x00001920 , 64    } , /* 0x0268                  */
	{ TYPE_TRC1   , 0x06002000 , 344   } , /* 0x02A8 TRC              */
	{ TYPE_TRC0   , 0x06000000 , 1024  } , /* 0x0400                  */
	{ TYPE_SEQ    , 0x0        , 8     } , /* 0x0800 Sequence No      */
	{ TYPE_CFG    , 0x0        , 544   } , /* 0x0808 PCI Config Reg.  */
	{ TYPE_0      , 0x00002050 , 48    } , /* 0x0A28                  */
	{ TYPE_0      , 0x00002450 , 48    } , /* 0x0A58                  */
	{ TYPE_0      , 0x00002850 , 48    } , /* 0x0A88                  */
	{ TYPE_0      , 0x00002C50 , 48    } , /* 0x0AB8                  */
	{ TYPE_PCI    , 0x00001090 , 624   } , /* 0x0AE8                  */
	{ TYPE_PCI    , 0x000018B0 , 1616  } , /* 0x0D58                  */
	{ TYPE_2      , 0x0E000000 , 1280  } , /* 0x13A8                  */
	{ TYPE_2      , 0x0E000780 , 240   } , /* 0x18A8                  */
	{ TYPE_2      , 0x0E000890 , 208   } , /* 0x1998                  */
	{ TYPE_2      , 0x0E000980 , 112   } , /* 0x1A68                  */
	{ TYPE_2      , 0x0E000A10 , 208   } , /* 0x1AD8                  */
	{ TYPE_2      , 0x0E000B00 , 112   } , /* 0x1BA8                  */
	{ TYPE_2      , 0x0E000B90 , 208   } , /* 0x1C18                  */
	{ TYPE_2      , 0x0E000C80 , 112   } , /* 0x1CE8                  */
	{ TYPE_2      , 0x0E000D10 , 208   } , /* 0x1D58                  */
	{ TYPE_2      , 0x0E000E00 , 80    } , /* 0x1E28                  */
	{ TYPE_2      , 0x0E000E80 , 256   } , /* 0x1E78                  */
	{ TYPE_RSV    , 0x0        , 136   } , /* 0x1F78                  */
	{ TYPE_EVT0   , 0x04020000 , 4096  } , /* 0x2000 Event Trace      */
	{ TYPE_FRM0   , 0x04030000 , 4096  } , /* 0x3000 Frame Trace      */
	{ 0           , 0          , 0     } ,
};
	
const Type_mem five_mck_iport2[] = {
	{ TYPE_SEQ    , 0x0        , 8     } , /* 0x0100 Sequence No      */
	{ TYPE_PCI    , 0x0        , 48    } , /* 0x0108 PCI              */
	{ TYPE_PCI    , 0x10A0     , 64    } , /* 0x0138 INT information  */
	{ TYPE_PCI    , 0x00001800 , 176   } , /* 0x0178 WA               */
	{ TYPE_PCI    , 0x00001040 , 64    } , /* 0x0228                  */
	{ TYPE_PCI    , 0x00001920 , 64    } , /* 0x0268                  */
	{ TYPE_TRC1   , 0x06002000 , 344   } , /* 0x02A8 TRC              */
	{ TYPE_TRC0   , 0x06000000 , 1024  } , /* 0x0400                  */    /**/
	{ TYPE_SEQ    , 0x0        , 8     } , /* 0x0800 Sequence No      */
	{ TYPE_CFG    , 0x0        , 544   } , /* 0x0808 PCI Config Reg.  */
	{ TYPE_0      , 0x00002050 , 48    } , /* 0x0A28                  */
	{ TYPE_0      , 0x00002450 , 48    } , /* 0x0A58                  */
	{ TYPE_0      , 0x00002850 , 48    } , /* 0x0A88                  */
	{ TYPE_0      , 0x00002C50 , 48    } , /* 0x0AB8                  */
	{ TYPE_PCI    , 0x00001090 , 624   } , /* 0x0AE8                  */
	{ TYPE_PCI    , 0x000018B0 , 1616  } , /* 0x0D58                  */
	{ TYPE_2      , 0x0E000000 , 1280  } , /* 0x13A8                  */
	{ TYPE_2      , 0x0E000780 , 240   } , /* 0x18A8                  */
	{ TYPE_2      , 0x0E000890 , 208   } , /* 0x1998                  */
	{ TYPE_2      , 0x0E000980 , 112   } , /* 0x1A68                  */
	{ TYPE_2      , 0x0E000A10 , 208   } , /* 0x1AD8                  */
	{ TYPE_2      , 0x0E000B00 , 112   } , /* 0x1BA8                  */
	{ TYPE_2      , 0x0E000B90 , 208   } , /* 0x1C18                  */
	{ TYPE_2      , 0x0E000C80 , 112   } , /* 0x1CE8                  */
	{ TYPE_2      , 0x0E000D10 , 208   } , /* 0x1D58                  */
	{ TYPE_2      , 0x0E000E00 , 80    } , /* 0x1E28                  */
	{ TYPE_2      , 0x0E000E80 , 256   } , /* 0x1E78                  */
	{ TYPE_RSV    , 0x0        , 136   } , /* 0x1F78                  */
	{ TYPE_EVT0   , 0x04020000 , 2048  } , /* 0x2000 Event Trace      */
	{ TYPE_EVT1   , 0x04024000 , 2048  } , /* 0x2800 Frame Trace      */
	{ TYPE_FRM0   , 0x04030000 , 2048  } , /* 0x3000 Event Trace      */
	{ TYPE_FRM1   , 0x04034000 , 2048  } , /* 0x3800 Frame Trace      */
	{ 0           , 0          , 0     } ,
};
	
const Type_mem five_mck_iport4[] = {
	{ TYPE_SEQ    , 0x0        , 8     } , /* 0x0100 Sequence No      */
	{ TYPE_PCI    , 0x0        , 48    } , /* 0x0108 PCI              */
	{ TYPE_PCI    , 0x10A0     , 64    } , /* 0x0138 INT information  */
	{ TYPE_PCI    , 0x00001800 , 176   } , /* 0x0178 WA               */
	{ TYPE_PCI    , 0x00001040 , 64    } , /* 0x0228                  */
	{ TYPE_PCI    , 0x00001920 , 64    } , /* 0x0268                  */
	{ TYPE_TRC1   , 0x06002000 , 344   } , /* 0x02A8 TRC              */
	{ TYPE_TRC0   , 0x06000000 , 1024  } , /* 0x0400                  */    /**/
	{ TYPE_SEQ    , 0x0        , 8     } , /* 0x0800 Sequence No      */
	{ TYPE_CFG    , 0x0        , 544   } , /* 0x0808 PCI Config Reg.  */
	{ TYPE_0      , 0x00002050 , 48    } , /* 0x0A28                  */
	{ TYPE_0      , 0x00002450 , 48    } , /* 0x0A58                  */
	{ TYPE_0      , 0x00002850 , 48    } , /* 0x0A88                  */
	{ TYPE_0      , 0x00002C50 , 48    } , /* 0x0AB8                  */
	{ TYPE_PCI    , 0x00001090 , 624   } , /* 0x0AE8                  */
	{ TYPE_PCI    , 0x000018B0 , 1616  } , /* 0x0D58                  */
	{ TYPE_2      , 0x0E000000 , 1280  } , /* 0x13A8                  */
	{ TYPE_2      , 0x0E000780 , 240   } , /* 0x18A8                  */
	{ TYPE_2      , 0x0E000890 , 208   } , /* 0x1998                  */
	{ TYPE_2      , 0x0E000980 , 112   } , /* 0x1A68                  */
	{ TYPE_2      , 0x0E000A10 , 208   } , /* 0x1AD8                  */
	{ TYPE_2      , 0x0E000B00 , 112   } , /* 0x1BA8                  */
	{ TYPE_2      , 0x0E000B90 , 208   } , /* 0x1C18                  */
	{ TYPE_2      , 0x0E000C80 , 112   } , /* 0x1CE8                  */
	{ TYPE_2      , 0x0E000D10 , 208   } , /* 0x1D58                  */
	{ TYPE_2      , 0x0E000E00 , 80    } , /* 0x1E28                  */
	{ TYPE_2      , 0x0E000E80 , 256   } , /* 0x1E78                  */
	{ TYPE_RSV    , 0x0        , 136   } , /* 0x1F78                  */
	{ TYPE_EVT0   , 0x04020000 , 1024  } , /* 0x2000 Event Trace      */
	{ TYPE_EVT1   , 0x04024000 , 1024  } , /* 0x2400 Frame Trace      */
	{ TYPE_EVT2   , 0x04028000 , 1024  } , /* 0x2800 Event Trace      */
	{ TYPE_EVT3   , 0x0402C000 , 1024  } , /* 0x2C00 Frame Trace      */
	{ TYPE_FRM0   , 0x04030000 , 1024  } , /* 0x3000 Event Trace      */
	{ TYPE_FRM1   , 0x04034000 , 1024  } , /* 0x3400 Frame Trace      */
	{ TYPE_FRM2   , 0x04038000 , 1024  } , /* 0x3800 Event Trace      */
	{ TYPE_FRM3   , 0x0403C000 , 1024  } , /* 0x3C00 Frame Trace      */
	{ 0           , 0          , 0     } ,
};
 /* FCLNX-GPL-147 */
void hfc_five_logout(
	struct adap_info        *ap,
	uint                    err_no,
	uchar                   mode)
{	
	const uint  mck_point[] = {
		0x00002108,         /* Port0 Event Trace */
		0x00002508,         /* Port1 Event Trace */
		0x00002908,         /* Port2 Event Trace */
		0x00002D08,         /* Port3 Event Trace */
		0x00002100,         /* Port0 Frame Trace */
		0x00002500,         /* Port1 Frame Trace */
		0x00002900,         /* Port2 Frame Trace */
		0x00002D00};        /* Port3 Frame Trace */
	
	struct mp_adap_info     *mpap;
	struct adap_info        *wk_ap;
	Type_mem                *logmap=NULL ;
	int                     log_offset ;
/*	int                     num, rc;*/ /* FCLNX-GPL-111 */
	int						num;
	uint                    i, j ;
	uint                    reg_adr /*, wk_reg_adr */ ;
	uint                    wk4, wk_cfg;
	uint                    read_pos;
	uint                    point_adr;
	uint                    ram_adr;
	uint                    trc_range=0;
	uchar                   /* func_num=0, */ wk_func_num=0;
	uchar                   iport, wk_iport;


//	struct hfc_err_head_plus head_plus;
	
	HFC_DBGPRT(" hfcldd%d : hfc_five_logout - start\n",ap->dev_minor);
	
	mpap = ap->mp_adap_info;

#if 0
	/* FCLNX-GPL-111 */
	rc = lock_try_mpap( mpap );											/* FCLNX-0276 */
	
	if( rc == 0 ){		/* Lock failed */
		set_bit( HFC_LOCK_WAIT_4, (ulong *)&ap->mpap_lock );
		ap->err_no = err_no;
		ap->mode = mode;
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}

	if( test_bit( HFC_LOCK_WAIT_4, (ulong *)&ap->mpap_lock ) )
	{
		/* Stop retry timer */
		clear_bit( HFC_LOCK_WAIT_4, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
	}																	/* FCLNX-0276 */

	set_bit(HFC_HWLOG_SAVING, (ulong *)&mpap->status );

	mpap->hwlog_result |= ( 0x00000001 << ap->port_no );
#endif
	
	if( ( mode == HFC_ERRLOG_TYPE_IMLLOG ) || ( mode == HFC_ERRLOG_TYPE_IMLLOG_DIAG) )
	{
		logmap = (Type_mem *) five_iml_log ;	/* FCLNX-GPL-147 */
	}
	else
	{
		wk_iport = (uchar)hfc_read_reg_ext(ap,(uint)0x1e,(char)1);

		iport = (wk_iport >> 4) & 0x03;                                         /* FCWIN-0232 */

		if( iport == 0x03 )
		{
			logmap = (Type_mem *) five_mck_iport4 ; /* FCLNX-GPL-147 */
			trc_range = 0x1000;
		}
		else if( iport == 0x01 )
		{
			logmap = (Type_mem *) five_mck_iport2 ; /* FCLNX-GPL-147 */
			trc_range = 0x1000;
		}
		else
		{
			logmap = (Type_mem *) five_mck_iport1 ; /* FCLNX-GPL-147 */
			trc_range = 0x1000;
		}
			
		if (mode == HFC_ERRLOG_TYPE_MCK)
		{
			/* Count mck_seq_no only for MCK log */
			mpap->mck_seq_no++ ;
		}
	}

	log_offset = 0 ;
	num = 0 ;
	
	for (j=0; j<4;j++)
	{
//		head_plus.type = mode;
//		head_plus.errno = err_no;
		
//		HFC_MEMCPY(&mpap->hw_log[log_offset], &head_plus, 64);
//		memset(&head_plus, 0, sizeof(struct hfc_err_head_plus));
		log_offset += 16;
	}
	
	while( (log_offset < (HFC_HWLOG_SIZE - 256)/4) && (logmap[num].type != 0) )
	{
		if( logmap[num].type == TYPE_SEQ )
		{
			HFC_4L_TO_4B( mpap->hw_log[log_offset], err_no ) ;
			HFC_4L_TO_4B( mpap->hw_log[log_offset+1], mpap->mck_seq_no ) ;
			log_offset+=2 ;
		}
		else if( logmap[num].type == TYPE_RSV )
		{
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				mpap->hw_log[log_offset] = 0xffffffff;
				log_offset++;
			}
		}
		else if( logmap[num].type == TYPE_PCI )
		{
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				wk4 = (uint)hfc_read_reg_ext(
					ap, logmap[num].reg_adr+reg_adr, (char)0x4) ;
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
		}
#if 0 /* "TYPE_ROTATE" was never used. */
		else if( logmap[num].type == TYPE_ROTATE )
		{
			/* Sort the area where ULP rotates in ULP number order */
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				if (reg_adr + (func_num * (logmap[num].size / 4)) < logmap[num].size)
					wk_reg_adr = reg_adr + (func_num * (logmap[num].size / 4));
				else
					wk_reg_adr = reg_adr + (func_num * (logmap[num].size / 4)) - logmap[num].size;
				
				wk4 = (uint)hfc_read_reg_ext(ap, logmap[num].reg_adr+wk_reg_adr, 0x4);     /* FCWIN-0233 STR   */
				HFC_4L_TO_4B(mpap->hw_log[log_offset], wk4);
				log_offset++;                                                                           /* FCWIN-0233 END   */
			}
		}
#endif
		else if( logmap[num].type == TYPE_CFG )
		{
			for( j=0 ; j<4 ; j++)
			{
				wk_ap = ap;
				while( wk_ap->prev != NULL){ /* FCLNX-GPL-079 */
					wk_ap = wk_ap->prev;
				}
				
				for( i=0 ; i<4 ; i++)
				{
					wk_func_num = wk_ap->port_no;
					if ( wk_func_num == j)
						break;
					
					wk_ap = wk_ap->next;
					if (wk_ap == NULL)
						break;
				}
				
				if ( wk_ap == NULL ) /* FCLNX-GPL-079 */
				{
					/* 0 pading */
					for(reg_adr = 0 ; reg_adr<(logmap[num].size/4) ; reg_adr+=4)
					{
						memset(&mpap->hw_log[log_offset], 0, 4);
						log_offset ++;                                           /*  FCWIN-0232 */
					}
				}
				else
				{
					for(reg_adr=0 ; reg_adr<(logmap[num].size/4 + 0x10 ) ; reg_adr+=4)
					{
						/* No output for config area address 0x80 - 0x8F */
						if ( reg_adr == 0x80 )
							reg_adr = 0x90;                                     /*  FCWIN-0283  */

						wk_cfg = (uint) hfc_read_cnfg( wk_ap, reg_adr, 0x04);
						HFC_4L_TO_4B(mpap->hw_log[log_offset], wk_cfg);
						log_offset++ ;
					}
				}
			}
		}
		else if( (logmap[num].type == TYPE_0) || (logmap[num].type == TYPE_1) ||
				 (logmap[num].type == TYPE_01) || (logmap[num].type == TYPE_2) )
		{
			/* Set indirect access flag */
			hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x08);
			
			hfc_read_reg_ext(ap, (uint)REG_RAMADR, (char)0x1);                                              /* FCWIN-0236 */
			
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,logmap[num].type);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,logmap[num].reg_adr);
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				read_pos = ( reg_adr + logmap[num].reg_adr + REG_INDAREA_FIVE ) % 0x80 + REG_INDAREA_FIVE;  /* FCWIN-0234 */
				
				wk4 = (uint)hfc_read_reg_ext(ap, read_pos, (char)0x4);
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				log_offset++ ;
			}
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,0x00);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x1,0x80);
			
			/* Reset indirect access flag */
			hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x00);
		}
		else if ( ( logmap[num].type == TYPE_TRC0 ) || ( logmap[num].type == TYPE_TRC1 ) )
		{
			if ( logmap[num].type == TYPE_TRC0 )
				ram_adr = (uint)hfc_read_reg_ext(ap,(uint)0xf0,(char)0x2);
			else
				ram_adr = (uint)hfc_read_reg_ext(ap,(uint)0xf2,(char)0x2);
			
			if ( logmap[num].type == TYPE_TRC0 )
			{
				if ( ram_adr < logmap[num].size/4)                                 /* FCWIN-255 */
					ram_adr = logmap[num].reg_adr + 0x2000 + ram_adr * 4 - logmap[num].size;
				else
					ram_adr = logmap[num].reg_adr + ram_adr * 4 - logmap[num].size;
			}
			else
			{
				if ( ram_adr < logmap[num].size/8)
					ram_adr = logmap[num].reg_adr + 0x2000 + ram_adr * 8 - logmap[num].size ;
				else
					ram_adr = logmap[num].reg_adr + ram_adr * 8 - logmap[num].size ;
			}
			
			/* Set indirect access flag */
			hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x08);
			
			hfc_read_reg_ext(ap, (uint)REG_RAMADR, (char)0x1);
			
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_1);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,ram_adr);
			
			read_pos = ram_adr % 0x80 + REG_INDAREA_FIVE;
			for( reg_adr =0 ; reg_adr <logmap[num].size ; reg_adr +=4)
			{
				if ( ram_adr == (logmap[num].reg_adr + 0x2000) )                                  /*  FCWIN-0284  */
				{
					hfc_write_reg_ext(ap, (uint)REG_RAMADR, (char)0x4, logmap[num].reg_adr);
					ram_adr = logmap[num].reg_adr;                                              /*  FCWIN-0284  */
					read_pos = logmap[num].reg_adr % 0x80 + REG_INDAREA_FIVE;
				}                                                                               /*  FCWIN-0284  */
				
				wk4 = (uint)hfc_read_reg_ext(ap,read_pos,(char)0x4);
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				
				ram_adr += 4;
				hfc_write_reg_ext(ap, (uint)REG_RAMADR, (char)0x4, ram_adr );    /*  FCWIN-0284  */
				read_pos = ram_adr % 0x80 + REG_INDAREA_FIVE;
				
				log_offset++ ;
			}
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,0x00);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x1,0x80);
			
			/* Reset indirect access flag */
			hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x00);
		}
		else if( ( logmap[num].type >= TYPE_EVT0 ) && ( logmap[num].type <= TYPE_FRM3 ) )
		{
			/* Set indirect access flag */
			hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x08);
			
			hfc_read_reg_ext(ap, (uint)REG_RAMADR, (char)0x1);                                              /* FCWIN-0236 */
			
			point_adr = mck_point[logmap[num].type - TYPE_EVT0];
			
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(uchar)TYPE_0);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x4,point_adr);
			read_pos = ( point_adr + REG_INDAREA_FIVE ) % 0x80 + REG_INDAREA_FIVE;  /* FCWIN-0234 */
			wk4 = (uint)hfc_read_reg_ext(ap, read_pos, (char)0x4);
			wk4 &= 0x00ffffff;
			
			if( wk4 * 4 - logmap[num].size <= logmap[num].reg_adr - 0x04000000)
				ram_adr = (wk4 + trc_range - logmap[num].size/4) * 4 + 0x04000000;
			else
				ram_adr = (wk4 - logmap[num].size/4) * 4 + 0x04000000;
			
			hfc_write_reg_ext(ap, (uint)REG_RAMADR, (char)0x4, ram_adr);
			
			read_pos = ram_adr % 0x80 + REG_INDAREA_FIVE;
			for( reg_adr = 0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				if ( ram_adr == logmap[num].reg_adr + trc_range * 4 )
				{
					hfc_write_reg_ext(ap, (uint)REG_RAMADR, (char)0x4, logmap[num].reg_adr);
					ram_adr = logmap[num].reg_adr;                              /* FCWIN-0284 */
					read_pos = logmap[num].reg_adr % 0x80 + REG_INDAREA_FIVE;   /* FCWIN-0234 */
				}
				
				wk4 = (uint)hfc_read_reg_ext(ap, read_pos, (char)0x4);
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				
				ram_adr += 4;
				hfc_write_reg_ext(ap, (uint)REG_RAMADR, (char)0x4, ram_adr );    /*  FCWIN-0284  */
				read_pos = ram_adr % 0x80 + REG_INDAREA_FIVE;
								
				log_offset++ ;
			}
			hfc_write_reg_ext(ap,( uint )REG_RAMMSK,( char )0x1,(char)0x00);
			hfc_write_reg_ext(ap,( uint )REG_RAMADR,( char )0x1,0x80);
			
			/* Reset indirect access flag */
			hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x00);
		}
		num++ ;
	}
#if _HFC_DEBUG_TIMER_00
	memorydump("LOGOUT", (uchar *)mpap->hw_log, HFC_HWLOG_SIZE);
#endif
/*	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); */ /* FCLNX-0279 */ /* FCLNX-GPL-111 */
	HFC_DBGPRT(" hfcldd : hfc_five_logout - end\n");
}


/*
 * Function:    hfc_five_ex_logout
 *
 * Purpose:     HW logout for FIVE-EX
 *
 * Arguments:   
 *  ap          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
const Type_mem fex_iml_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1440     , 16    } , /* BOOTFAR                 */
	{ TYPE_PCI    , 0x19A0     , 16    } , /* WR,WoR                  */
	{ TYPE_PCI    , 0x19C0     , 112   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_TRC0   , 0x6000000  , 208   } , /* ZTR0 Micro event trc    */
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_TRC2   , 0x6400000  , 624   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_2      , 0xE000700  , 64    } , /* Indirect reg Core BU    */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_RSV    , 0x0        , 208   } ,
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_mpck_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1920     , 96    } , /* BRA-E                   */
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_0      , 0x2000     , 64    } , /* WS Port0 ctl            */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port0 ctl            */
	{ TYPE_0      , 0x2400     , 64    } , /* WS Port1 ctl            */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port1 ctl            */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_TRC0   , 0x6000000  , 720   } , /* ZTR0 Micro event trc    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_EVT0   , 0x28000    , 960   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_FRM0   , 0x2A000    , 960   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_TRC2   , 0x6400000  , 272   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC1   , 0x6002000  , 496   } , /* ZTR1 DMA evt trc        */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_RSV    , 0x0        , 72    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_RSV    , 0x0        , 8     } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 496   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_pu_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_TRC0   , 0x6000000  , 544   } , /* ZTR0 Micro event trc    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_TRC2   , 0x6400000  , 608   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_EVT0   , 0x28000    , 608   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_RSV    , 0x0        , 96    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_FRM0   , 0x2A000    , 992   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC1   , 0x6002000  , 496   } , /* ZTR1 DMA evt trc        */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_RSV    , 0x0        , 72    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_RSV    , 0x0        , 8     } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 496   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_bu_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 48    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_PCI    , 0x1BC0     , 64    } , /*                         */
	{ TYPE_2      , 0xE000700  , 64    } , /* Indirect reg Core BU    */
	{ TYPE_TRC1   , 0x6002000  , 368   } , /* ZTR1 DMA evt trc        */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_TRC0   , 0x6000000  , 480   } , /* ZTR0 Micro event trc    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_EVT0   , 0x28000    , 608   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_RSV    , 0x0        , 96    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_FRM0   , 0x2A000    , 992   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC2   , 0x6400000  , 496   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_RSV    , 0x0        , 72    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_RSV    , 0x0        , 8     } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 496   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_turu_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_2      , 0xE000800  , 192   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_2      , 0xE0008C0  , 576   } , /* Indirect reg Core TRU   */
	{ TYPE_TRC0   , 0x6000000  , 432   } , /* ZTR0 Micro event trc    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_EVT0   , 0x28000    , 608   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_RSV    , 0x0        , 64    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_FRM0   , 0x2A000    , 992   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC1   , 0x6002000  , 496   } , /* ZTR1 DMA evt trc        */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_RSV    , 0x0        , 72    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_RSV    , 0x0        , 8     } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_euzu_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 112   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_PCI    , 0x19F0     , 112   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_TRC0   , 0x6000000  , 512   } , /* ZTR0 Micro event trc    */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_TRC2   , 0x6400000  , 272   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_EVT0   , 0x28000    , 608   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_RSV    , 0x0        , 96    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_FRM0   , 0x2A000    , 992   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC1   , 0x6002000  , 496   } , /* ZTR1 DMA evt trc        */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_RSV    , 0x0        , 72    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_RSV    , 0x0        , 8     } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 496   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_dumpcore_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_TRC0   , 0x6000000  , 212   } , /* ZTR0 Micro event trc    */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_TRC2   , 0x6400000  , 488   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_EVT0   , 0x28000    , 608   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_RSV    , 0x0        , 96    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_FRM0   , 0x2A000    , 992   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC1   , 0x6002000  , 496   } , /* ZTR1 DMA evt trc        */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 272   } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_RSV    , 0x0        , 256   } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 496   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

const Type_mem fex_mck_cmn_log[] = {
	/*---type-----address------size--*/
	{ TYPE_ERRID  , 0x00       , 4     } , /********* 1st *************/
	{ TYPE_PCI    , 0x10A0     , 12    } , /* INTA(Scan Space)        */
	{ TYPE_CFG0   , 0x0        , 52    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x3C       , 68    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x90       , 16    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xB8       , 4     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xC0       , 8     } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0xE0       , 32    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x100      , 44    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1C8      , 28    } , /* PCI Config (own func)   */
	{ TYPE_CFG0   , 0x1F0      , 16    } , /* PCI Config (own func)   */
	{ TYPE_RSV    , 0x0        , 4     } ,
	{ TYPE_PCI    , 0x0        , 48    } , /* install/status/mode     */
	{ TYPE_0      , 0x0        , 32    } , /* Comm Area  port0        */
	{ TYPE_0      , 0x80       , 32    } , /* Comm Area  port1        */
	{ TYPE_PCI    , 0x200      , 16    } , /* ECID                    */
	{ TYPE_PCI    , 0x1300     , 128   } , /* Common checker          */
	{ TYPE_PCI    , 0x13A0     , 32    } , /* SRAM UCE/CE             */
	{ TYPE_PCI    , 0x1800     , 176   } , /* Core checker            */
	{ TYPE_PCI    , 0x1040     , 64    } , /* GR0-F                   */
	{ TYPE_PCI    , 0x1090     , 16    } , /* BR8,9                   */
	{ TYPE_PCI    , 0x1970     , 16    } , /* BRC,E                   */
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 112   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 2nd *************/
	{ TYPE_PCI    , 0x10B0     , 12    } , /* INTB(Scan Space)        */
	{ TYPE_PCI    , 0x19F0     , 112   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_TRC0   , 0x6000000  , 512   } , /* ZTR0 Micro event trc    */
	{ TYPE_PCI    , 0xC00      , 8     } , /* MSI-X table(INTA vec#0) */
	{ TYPE_PCI    , 0xC10      , 8     } , /* MSI-X table(INTA vec#1) */
	{ TYPE_PCI    , 0xE00      , 8     } , /* MSI-X table(INTB vec#0) */
	{ TYPE_PCI    , 0xE10      , 8     } , /* MSI-X table(INTB vec#1) */
	{ TYPE_TRC2   , 0x6400000  , 272   } , /* PTR0 PCIe pkt trc       */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 3rd *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x2000     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2180     , 64    } , /* WS Port ctl             */
	{ TYPE_0      , 0x2400     , 128   } , /* WS Port ctl             */
	{ TYPE_0      , 0x2580     , 64    } , /* WS Port ctl             */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_EVT0   , 0x28000    , 608   } , /* F/W evt trc             */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 4th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_0      , 0x1080     , 128   } , /* WS LV work LV0          */
	{ TYPE_0      , 0x1180     , 128   } , /* WS LV work LV1A         */
	{ TYPE_0      , 0x1280     , 128   } , /* WS LV work LV2A         */
	{ TYPE_0      , 0x1380     , 128   } , /* WS LV work LV3          */
	{ TYPE_0      , 0x1480     , 128   } , /* WS LV work LV1B         */
	{ TYPE_0      , 0x1580     , 128   } , /* WS LV work LV2B         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x200      , 128   } , /* MCW                     */
	{ TYPE_RSV    , 0x0        , 96    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 5th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_FRM0   , 0x2A000    , 992   } , /* F/W farme trc           */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 6th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC1   , 0x6002000  , 496   } , /* ZTR1 DMA evt trc        */
	{ TYPE_0      , 0x8000000  , 512   } , /* BCR                     */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 7th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_TRC3   , 0x6402000  , 1008  } , /* PTR1 PU-BU evt trc      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 8th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD000000  , 160   } , /* Dump core(SRAM ECC ERR) */
	{ TYPE_2      , 0xD038000  , 296   } , /* Dump core(OREG)         */
	{ TYPE_2      , 0xD03A000  , 60    } , /*                         */
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD080000  , 408   } , /* Dump core(OMA(PCLK))    */
	{ TYPE_RSV    , 0x0        , 72    } ,
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 9th *************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xD082000  , 520   } , /* Dump core(OMA(BCLK))    */
	{ TYPE_RSV    , 0x0        , 8     } ,
	{ TYPE_PCI    , 0x900      , 48    } , /* UTL                     */
	{ TYPE_PCI    , 0x960      , 112   } , /*                         */
	{ TYPE_PCI    , 0xA0       , 32    } , /* INT                     */
	{ TYPE_PCI    , 0xC0       , 32    } , /* MSI-X PBA               */
	{ TYPE_PCI    , 0x300      , 256   } , /* WS                      */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 10th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_RSV    , 0x0        , 496   } ,
	{ TYPE_PCI    , 0xC00      , 512   } , /* MSI-X tbl (INTA)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 11th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_CFG1   , 0x0        , 52    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x3C       , 68    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x90       , 16    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xB8       , 4     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xC0       , 8     } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0xE0       , 32    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x100      , 44    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1C8      , 28    } , /* PCI Config (other func) */
	{ TYPE_CFG1   , 0x1F0      , 16    } , /* PCI Config (other func) */
	{ TYPE_RSV    , 0x0        , 228   } ,
	{ TYPE_PCI    , 0xE00      , 512   } , /* MSI-X tbl (INTB)        */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 12th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x11C0     , 64    } , /* ULP                     */
	{ TYPE_PCI    , 0x1980     , 224   } , /* TIMER,BICTL,CS/LSAR     */
	{ TYPE_PCI    , 0x1A70     , 16    } , /* micro                   */
	{ TYPE_PCI    , 0x1A90     , 48    } , /* Timer,ULP,REG           */
	{ TYPE_PCI    , 0x1AE0     , 16    } , /* FEEND                   */
	{ TYPE_PCI    , 0x1B00     , 128   } , /*                         */
	{ TYPE_PCI    , 0x1B90     , 112   } , /*                         */
	{ TYPE_PCI    , 0x1D00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1D40     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1E00     , 32    } , /*                         */
	{ TYPE_PCI    , 0x1EF0     , 16    } , /*                         */
	{ TYPE_PCI    , 0x1F00     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1F80     , 64    } , /*                         */
	{ TYPE_RSV    , 0x0        , 16    } ,
	{ TYPE_PCI    , 0x1F40     , 64    } , /*                         */
	{ TYPE_PCI    , 0x1FC0     , 64    } , /*                         */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 13th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_PCI    , 0x1D50     , 176   } , /*                         */
	{ TYPE_PCI    , 0x1E20     , 208   } , /*                         */
	{ TYPE_2      , 0xE8000D0  , 48    } , /* Indirect reg FLASH ctrl */
	{ TYPE_2      , 0xE800400  , 224   } , /* Indirect reg PU         */
	{ TYPE_2      , 0xE000000  , 352   } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 14th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000160  , 912   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000480  , 96    } , /* Indirect reg Core BU    */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 15th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE0004E0  , 608   } , /* Indirect reg Core BU    */
	{ TYPE_2      , 0xE000800  , 400   } , /* Indirect reg Core TRU   */
	{ TYPE_ERRID  , 0x0        , 4     } , /********* 16th ************/
	{ TYPE_RSV    , 0x0        , 12    } ,
	{ TYPE_2      , 0xE000990  , 688   } , /* Indirect reg Core TRU   */
	{ TYPE_2      , 0xE000C80  , 320   } , /* Indirect reg Core TRU   */
	{ 0           , 0          , 0     } 
};

void hfc_five_ex_logout(struct adap_info *ap, uint err_no, uchar mode)
{
	const uchar pass1	= 0x00;

	struct mp_adap_info		*mpap;
	struct adap_info		*wk_ap;
	struct pci_dev			*pdev; /* FCLNX-GPL-230 */
	Type_mem				*logmap = NULL;
	int                     log_offset = 0 ;
/*	int                     num, rc; */ /* FCLNX-GPL-111 */
	int                     num;
	uint                    i;
	uint                    reg_adr;
	uint                    start_adr, end_adr, log_adr; /* F/W event/frame Trace */
	uint                    wk4;
	uint                    read_pos;
	uint                    point_adr;
	uint                    ram_adr=0;
	uint                    trc_range=0;
	uchar                   iport, wk_iport;
	uint                    errorid = 0;
	uint					indirect_access = 0;
	uint					type;
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} status_reg, errdet_reg;

	mpap = ap->mp_adap_info;
	pdev = ap->pci_cfginf; /* FCLNX-GPL-230 */

#if 0
	/* FCLNX-GPL-111 */
	/*** Try to lock mpap ***********/
	rc = lock_try_mpap( mpap );											/* FCLNX-0276 */
	
	if( rc == 0 ){		/* Lock failed */
		set_bit( HFC_LOCK_WAIT_8, (ulong *)&ap->mpap_lock );
		ap->err_no = err_no;
		ap->mode = mode;
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}

	if( test_bit( HFC_LOCK_WAIT_8, (ulong *)&ap->mpap_lock ) )
	{
		/* Stop retry timer */
		clear_bit( HFC_LOCK_WAIT_8, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
	}																	/* FCLNX-0276 */
#endif

	/****** Set Tables for Logout ***************/
	if(( mode == HFC_ERRLOG_TYPE_IMLLOG ) || ( mode == HFC_ERRLOG_TYPE_IMLLOG_DIAG)) {
		logmap  = (Type_mem *) fex_iml_log ;
		errorid = 0x1000;
	}
	else
	{
		status_reg.l = 0;
		errdet_reg.l = 0;

		wk4 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
		HFC_4B_TO_4L(status_reg.l, wk4);

		wk4 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
		HFC_4B_TO_4L(errdet_reg.l, wk4);

		if (status_reg.c[3] & HFC_PCI_COMMON_MCK) /* MCK Logout*/
		{
			if (errdet_reg.c[3] & HFC_PCI_USERMCK)
			{
				wk_iport = (uint)hfc_read_reg_ext(ap, 0x1307, 0x1);

				if (wk_iport & 0x20) { /* Is "Common part" HPUMCKST ? */
					logmap  = (Type_mem *) fex_mck_pu_log; 
					errorid = 0x0100;
				} else {
					logmap  = (Type_mem *) fex_mck_cmn_log;
					errorid = 0x0600;
				}
			} else {
				logmap = (Type_mem *) fex_mck_dumpcore_log;
				errorid = 0x0500;
			}
		} else {
			if ( (errdet_reg.c[2] & HFC_PCI_XHMPCK) || (errdet_reg.c[2] & HFC_PCI_XHTO3CK) ) {
				logmap  = (Type_mem *) fex_mck_mpck_log;
				errorid = 0x0000;
			} else {
				if (errdet_reg.c[1] & HFC_PCI_HPUMCKST) {
					logmap  = (Type_mem *) fex_mck_pu_log;
					errorid = 0x0100;
				} else if (errdet_reg.c[1] & HFC_PCI_HBUMCKST) {
					logmap  = (Type_mem *) fex_mck_bu_log;
					errorid = 0x0200;
				} else if (errdet_reg.c[1] & (HFC_PCI_HTUMCKST | HFC_PCI_HRUMCKST) ) {
					logmap  = (Type_mem *) fex_mck_turu_log;
					errorid = 0x0300;
				} else {
					logmap  = (Type_mem *)fex_mck_euzu_log;
					errorid = 0x0400;
				}
			}
		}
		if (mode == HFC_ERRLOG_TYPE_CHKSTP)
			errorid += 0x800;

		if (mode == HFC_ERRLOG_TYPE_MCK)
		{
			/* Count mck_seq_no only for MCK log */
			mpap->mck_seq_no++ ;
		}
	}

	num = 0 ;

	/******** Get Log Data ****************/
	/* Set indirect access flag */
	/* FCLNX-GPL-116 */
	if (!(hfc_read_reg(ap, HFC_IOSPACE_RAMADR, (char)0x1) & 0x80))
	{
		hfc_write_reg(ap, HFC_IOSPACE_IDFLGEN, 0x01, 0x08);
		
		if( (hfc_read_reg(ap, HFC_IOSPACE_RAMADR, (char)0x1) & 0x80) ){
			hfc_write_reg(ap, HFC_IOSPACE_IDFLGEN, 0x01, 0x00); /* Clear Flag */
		} else {
			indirect_access = 1; /* Indirect Access Admited  */
		}
	}

	while( (log_offset < (HFC_HWLOG_SIZE/4)) && (logmap[num].type != 0) )
	{
		type = logmap[num].type;
		
		if( ap->pkg.lsi_rev == pass1 ) /* FCLNX-GPL-220 */
		{
 			/* for Only FIVE-EX pass1 */
 			/* "TYPE_0 or TYPE_TRCx or TYPE_EVT0 or TYPE_FRM0" -> RSV*/
			switch (type) {
			case TYPE_0:
			case TYPE_EVT0:
			case TYPE_FRM0:
				type = TYPE_RSV;
				break;
			default: break;
			}
		}
		
		if (indirect_access == 0) { /* FCLNX-GPL-116 */
			/* Indirect Access Prohibited */
			switch (type) {
				case TYPE_0:
				case TYPE_1:
				case TYPE_2: 
				case TYPE_TRC0:
				case TYPE_TRC1:
				case TYPE_TRC2:
				case TYPE_TRC3:
				case TYPE_EVT0:
				case TYPE_FRM0:
					type = TYPE_RSV; /* FCLNX-GPL-139 */
					break;
				default:
					break;
			}
		}

		if( type == TYPE_ERRID )
		{
			wk4 = 0xFFFE0000 + errorid + (log_offset/256);
			HFC_4L_TO_4B(mpap->hw_log[log_offset], wk4);
			log_offset++;
		}
		else if( type == TYPE_RSV )
		{
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				mpap->hw_log[log_offset] = 0xfefefefe; /* FCLNX-GPL-139 */
				log_offset++;
			}
		}
		else if( type == TYPE_PCI )
		{
			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{
				read_pos = logmap[num].reg_adr+reg_adr;
				wk4 = (uint)hfc_read_reg_ext(ap, read_pos, (char)0x4) ;
				HFC_4L_TO_4B(mpap->hw_log[log_offset], wk4);
				log_offset++ ;
			}
		}
		else if( (type == TYPE_CFG0) || (type == TYPE_CFG1) )
		{
			wk_ap = NULL;
			if ( type == TYPE_CFG0 ) { /* Read "CFG Space" from own port. */
				wk_ap = ap;
			}
			if ( type == TYPE_CFG1 ) { /* Read "CFG Space" form other port. */
				wk_ap = mpap->ap;
				while (wk_ap != NULL)
				{
					if (wk_ap != ap){
						break;
					}
					wk_ap = wk_ap->next;
				}
			}

			if ( wk_ap == NULL )
			{
				/* 0 pading */
				for(reg_adr = 0 ; reg_adr < logmap[num].size ; reg_adr+=4)
				{
					memset(&mpap->hw_log[log_offset], 0, 4);
					log_offset ++;
				}
			}
			else
			{
				for(reg_adr = 0 ; reg_adr < logmap[num].size ; reg_adr+=4)
				{
					read_pos = logmap[num].reg_adr+reg_adr;
					/* Check the range of CFG space */ /* FCLNX-GPL-230 start */
					if( pdev->cfg_size > (read_pos+4) )
					{	/* Access was permited */
						wk4 = hfc_read_cnfg(wk_ap, read_pos, (char)0x4) ;
					}
					else
					{	/* range over */
						wk4 = 0xfefefefe ;
					}
					/* FCLNX-GPL-230 end */
					
					/* set data */
					HFC_4L_TO_4B(mpap->hw_log[log_offset], wk4);
					log_offset++ ;
				}
			}
		}
		else if( (type == TYPE_0) || (type == TYPE_1) ||
				 (type == TYPE_2) )
		{
			hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,( char )0x1,type);
/*			hfc_write_reg(ap, HFC_IOSPACE_RAMADR,( char )0x4,logmap[num].reg_adr); */ /* FCLNX-GPL-250 */

			for(reg_adr=0 ; reg_adr<logmap[num].size ; reg_adr+=4)
			{   /* Set "indirect ram addr" everytime */ /* FCLNX-GPL-250 */
				hfc_write_reg(ap, HFC_IOSPACE_RAMADR,( char )0x4, (logmap[num].reg_adr + reg_adr)); 
				wk4 = (uint)hfc_read_reg(ap, HFC_IOSPACE_INDAREA, (char)0x4);
				HFC_4L_TO_4B(mpap->hw_log[log_offset], wk4);
				log_offset++ ;
			}
		}
		else if ( ( type == TYPE_TRC0 ) || ( type == TYPE_TRC1 ) || 
		          ( type == TYPE_TRC2 ) || ( type == TYPE_TRC3 ) )
		{
			if ( type == TYPE_TRC0 ) { /* Micro Event Trace */
				ram_adr = (uint)hfc_read_reg(ap, HFC_IOSPACE_TRCA0,(char)0x2);
				hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,( char )0x1,(uchar)TYPE_1);
			}
			if ( type == TYPE_TRC1 ) { /* DMA Event Trace */
				ram_adr = (uint)hfc_read_reg(ap, HFC_IOSPACE_TRCA1,(char)0x2);
				hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,( char )0x1,(uchar)TYPE_1);
			}
			if ( type == TYPE_TRC2 ) { /* PCIe Packet Trace */
				ram_adr = (uint)hfc_read_reg_ext(ap, 0x8f0,(char)0x2);
				hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,( char )0x1,(uchar)TYPE_3);
			}
			if ( type == TYPE_TRC3 ) { /* PU-BU IF Trace */
				ram_adr = (uint)hfc_read_reg_ext(ap, 0x8f2,(char)0x2);
				hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,( char )0x1,(uchar)TYPE_3);
			}
			
			if ( ram_adr <= logmap[num].size/8) /* FCLNX-GPL-110 */
				ram_adr = logmap[num].reg_adr + 0x2000 + ram_adr * 8 - logmap[num].size ;
			else
				ram_adr = logmap[num].reg_adr + ram_adr * 8 - logmap[num].size ;
			
			hfc_write_reg(ap, HFC_IOSPACE_RAMADR,( char )0x4,ram_adr);
			
			for( reg_adr =0 ; reg_adr <logmap[num].size ; reg_adr +=4)
			{
				if ( ram_adr >= logmap[num].reg_adr + 0x2000 ) /* FCLNX-GPL-110 */
				{
					hfc_write_reg(ap,HFC_IOSPACE_RAMADR,( char )0x4,logmap[num].reg_adr);
					ram_adr = logmap[num].reg_adr;
				}
				wk4 = (uint)hfc_read_reg(ap, HFC_IOSPACE_INDAREA, (char)0x4);
				HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				
				ram_adr += 4;
				hfc_write_reg(ap,HFC_IOSPACE_RAMADR,( char )0x4,ram_adr);
				
				log_offset++ ;
			}
		}
		else if( ( type == TYPE_EVT0 ) || ( type == TYPE_FRM0 ) )
		{
			trc_range = logmap[num].size;  /* Get log size  */
			iport = 1;                     /* The number of ports on this core. */
			
			if (type == TYPE_EVT0) {
				point_adr = 0x2108;
			} else {
				point_adr = 0x2100;
			}

			wk_iport = hfc_read_reg(ap, HFC_IOSPACE_HWINF, (char)1);
			if (wk_iport & 0x10) /* 2port/core */ /* FCLNX-GPL-110 */
			{
				/* We get 2 half size logs, when HBA has 2ports on 1core. */
				iport = 2;
				trc_range /= 2;
			}

			for (i = 0 ; i < iport ; i++ )
			{
				start_adr = logmap[num].reg_adr + (i * 0x1000); /* The Start Address of Trace Area */
				end_adr   = start_adr + 0x1000;                 /* The End Address of Trace Area */
				point_adr += (0x400 * i);                       /* The address of Current Pointer Stoked Area */
			
				hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,( char )0x1,(uchar)TYPE_0);
				hfc_write_reg(ap, HFC_IOSPACE_RAMADR,( char )0x4,point_adr);
				wk4 = (uint)hfc_read_reg(ap, HFC_IOSPACE_INDAREA, (char)0x4);
				wk4 &= 0x00ffffff; /* Current Address */
			
				if( (wk4 - trc_range/4) < start_adr) /* Check address */
					log_adr = end_adr - (trc_range/4 - (wk4 - start_adr)); /* Rap Arround */
				else
					log_adr = wk4 - trc_range/4;
				
				for (reg_adr = 0 ; reg_adr < trc_range ; reg_adr+=4 )
				{
					ram_adr = (log_adr * 4) - 0x40000;
					ram_adr += 0x2000000; /* Add this number before indirect access for RB */
					hfc_write_reg(ap, HFC_IOSPACE_RAMADR,( char )0x4,ram_adr);
				
					wk4 = (uint)hfc_read_reg(ap, HFC_IOSPACE_INDAREA, (char)0x4);
					HFC_4L_TO_4B(mpap->hw_log[log_offset],wk4);
				
					log_adr += 1;
					if (log_adr >= end_adr) {
						log_adr = start_adr; 
					}
					log_offset++ ;
				}
			}
		}
		num++ ;
	}

	/**** Finalize process of indirect access ****/
	if (indirect_access == 1) { /* FCLNX-GPL-116 */
		hfc_write_reg(ap, HFC_IOSPACE_RAMMSK, 0x01, 0x00);	
		hfc_write_reg(ap, HFC_IOSPACE_RAMADR, 0x01, 0x80);	
		hfc_write_reg(ap, HFC_IOSPACE_IDFLGEN, 0x01, 0x00); /* Clear indirect access flag */
	}

	/***** Un-Lock map **************/
/*	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); */ /* FCLNX-GPL-111 */
	HFC_DBGPRT(" hfcldd : hfc_five_ex_logout - end\n");

	return;
}

/*
 * Function:    hfc_abend
 *
 * Purpose:     FIVE-EX SRAM 1bit CE Log out
 *
 * Arguments:   
 *  ap         - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_save_pcie_sram_log(struct adap_info *ap) 
{
	hfc_err1bit_t *ptr = NULL;
	int idx = 0;
	int cnt = 0;
	uint reg = 0;
	uint ram_adr = 0;
	uint64_t time = 0;

	struct ram_adr_t {
		uint pci_adr;
		uint bit;
		uint ram_adr;
	} ram_adr_tbl[] = { /* FCLNX-GPL-132 */
		{0x13ab, 0x80, 0x0d000010}, /* TXPD 	bit0-3 */
		{0x13aa, 0x80, 0x0d000030}, /* TXCPL	bit0-3 */
		{0x13a9, 0x80, 0x0d000050}, /* RXPD 	bit0-3 */
		{0x13a8, 0x80, 0x0d000070}, /* RXCPL	bit0-3 */
		{0x13ac, 0x80, 0x0d000090}, /* TXRB 	bit0-3 */

		{0x13ab, 0x08, 0x0d000000}, /* TXPD 	bit4-7 */
		{0x13aa, 0x08, 0x0d000020}, /* TXCPL	bit4-7 */
		{0x13a9, 0x08, 0x0d000040}, /* RXPD 	bit4-7 */
		{0x13a8, 0x08, 0x0d000060}, /* RXCPL	bit4-7 */
		{0x13ac, 0x08, 0x0d000080}, /* TXRB 	bit4-7 */
		{     0,    0,          0}  /* stopper     */
	};

	if (ap->pcie_sram_ce_cnt >= HFC_1BIT_LOG_ENTRY)
		return;

	ptr = (hfc_err1bit_t *) &ap->ce_log.pcie_sram_data[ ap->pcie_sram_ce_cnt ][0];
	
	/* Search RAM Address */
	for (idx=0; ram_adr_tbl[idx].pci_adr != 0 ; idx++)
	{
		reg = (uchar) hfc_read_reg_ext(ap, ram_adr_tbl[idx].pci_adr, 1);

		for (cnt = 0; cnt < 4 ; cnt++) {
			if ( reg & (ram_adr_tbl[idx].bit >> cnt) )
			{
				ram_adr = ram_adr_tbl[idx].ram_adr + (cnt*4);
				HFC_4L_TO_4B(ptr->ram_adr, ram_adr);
				goto search_end;
			}
		}
	}

search_end:
	/* Now time */
	time = get_jiffies_64(); /* get jiffies_64 */
	HFC_8L_TO_8B(ptr->time_stamp, time);

	if (ram_adr_tbl[idx].pci_adr == 0) {
		/* Searching RAM Address process has failed.*/
		ptr->far = 0;
		return;
	}
	
	/*--- Get FAR ---*/
	if (hfc_read_reg_ext(ap, (uint)REG_RAMADR, (char)0x1) & 0x80) {
		/* Indirect RAM accessing */
		ptr->far = 0xffffffff;
		return;
	}

	hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x08); /* Set indirect access flag */

	if (hfc_read_reg_ext(ap, (uint)REG_RAMADR, (char)0x1) & 0x80) {
		/* Indirect RAM accessing */
		hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x00); /* Clear indirect access flag */
		ptr->far = 0xffffffff;
		return;
	}
	hfc_write_reg_ext(ap,(uint)REG_RAMMSK,( char )0x1, TYPE_2);
	hfc_write_reg_ext(ap,(uint)REG_RAMADR,( char )0x4, ram_adr);
			
	reg = (uint)hfc_read_reg_ext(ap, REG_INDAREA_FIVE , (char)0x4);
	HFC_4L_TO_4B(ptr->far, reg);

	/**** Finalize process of indirect access ****/
	hfc_write_reg_ext(ap, (uint)REG_RAMMSK, (char)0x1, 0x00);
	hfc_write_reg_ext(ap, (uint)REG_RAMADR, (char)0x1, 0x80);
	hfc_write_reg_ext(ap, 0x2fb, 0x01, 0x00); /* Clear indirect access flag */
	
	return;
}

/*
 * Function:    hfc_abend
 *
 * Purpose:     Abend processing
 *
 * Arguments:   
 *  ap         - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_abend(
	struct adap_info        *ap,
	uchar                   type)
{
	struct mp_adap_info     *mpap;
	uint                    dump_reg[4] ;
	uint                    err_no=0 ;
	uint                    adap_status=0;
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	struct target_info  *target = NULL;
	int rsp;
	uint lp;
	struct dev_info 	*dev=NULL;
#endif
	/* FCLNX-GPL-209 */

	HFC_DBGPRT(" hfcldd%d : hfc_abend - start type = %x \n", ap->dev_minor, type);

	mpap = ap->mp_adap_info;
	hfc_hand2_trace(HFC_TRC_ABEND, type, ap, NULL, NULL, 0, 0, 0);
	switch( type )
	{
		case HFC_ABEND_XRB_INVALID:
		case HFC_ABEND_CCC:
		case HFC_ABEND_MB_RSPERR:
			/* (1) In the mailbox response, xcc=83 or fsb=CCC    */
			/* (2) In the SCSI response, xcc=83 or fsb=CCC       */
			/* (3) In the SCSI response, xrb Valid=0             */
			/*==> Process <==                                    */
			/* Count over (ap->link_dead_cnt >HFC_LINK_DEAD_CNT) */
			/*   -> Execute force check stop                     */
			/* Otherwise                                         */
			/*   -> Return (Counter Up)                          */

			ap->link_dead_cnt++ ;
			if( ap->link_dead_cnt > HFC_LINK_DEAD_CNT )
			{
				HFC_ISSUE_FMCK(ap, type);									/* FCLNX-0279 */
			}
			break ;
		case HFC_ABEND_SERR:
		case HFC_ABEND_PERR:
		case HFC_ABEND_SPERR:
			/* (1) PCI ERROR INT                                 */
			/* Issue Force Machine Check                         */
			
			/* Get Log for MLPF */
			if( HFC_MMODE_CHECK_SHARED(ap) && !( HFC_MMODE_CHECK_SHADOW(ap) ) ) /* @MLPF */
				hfc_mlpf_pci_error(ap, type);
			
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
			
			dump_reg[0] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4) ;
			dump_reg[1] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
			dump_reg[2] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
			dump_reg[3] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
			
/*			hfc_logout(ap,err_no,HFC_ERRLOG_TYPE_MCK); */ /* FCLNX-GPL-111 */
			HFC_MEMCPY(ap->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_ERRD,err_no,ap->logdata,16) ;	/* FCLNX-GPL-391 */

 			/* For hsdldd */
			if (hfc_manage_info.npubp->hfc_mp_hsd_enable)						/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_make_fcinfo(ap, NULL, err_no, adap_status, NULL, 0);
			
			/* Issue Force-MCK */
			HFC_ISSUE_FMCK(ap, type);										/* FCLNX-0279 */
			
			break ;
			
		case HFC_ABEND_MCK_INT:
		case HFC_ABEND_MPCHK:
		case HFC_ABEND_T3:
			/* (1) MCK INT										*/
			/*==> Processing <==								*/
			/* Machine check recovery and increment counter 	*/
			/* Cancel pending SCSI commands under this adapter	*/

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
				/* Stop timers. */
				HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY);	
				hfc_reset_watchdog(ap);										/* FCLNX-GPL-038 */
			} else {
				/* Cancel pending SCSI commands, and stop timers. */
				hfc_mck_prepare(ap); /* FCLNX-GPL-209 */
			}
#else
			/* Cancel pending SCSI commands, and stop timers. */
			hfc_mck_prepare(ap); /* FCLNX-GPL-209 */
#endif
			
			/* Close INT_A mask */
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000000 );	/* FCLNX-0275 */
			
			/* Machine check recovery and increment counter */
			if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
			{	/* "PCI BUS ERR" has happen. */
				HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
			}
			else
			{
				hfc_mck_recovery(ap, type); /* FCLNX-GPL-209 */
			}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
				/*------------------------------------
				-* Cancel pending SCSI commands
				-*------------------------------------*/
				for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)						
				{
					target = hfc_hash_target_info(ap, lp);
					if (target != NULL)
					{
						hfc_cancel_scsi_cmd(
							ap, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,
							TRUE, TRUE, HFC_FLASH_TARGET);
						target->status = HFC_NON_STATUS ;
					
						dev = target->dev;
						while( dev != NULL){
							dev->lustat = 0x00;
							dev = dev->next;
						}
					
						if ( hfc_manage_info.hfcldd_mp_mod ) {
							hfc_manage_info.npubp->hfc_forced_offline_e(target, TRUE);
						}
					}
				}
			
				for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)											/* FCLNX-GPL-038 */
				{
					target = hfc_pseq_target_info(ap, lp);
					if (target != NULL)
					{
						/* Release all target_info */
						if( test_bit( HFC_WAIT_LOGIN, (ulong *)&target->status ) 
							|| test_bit( HFC_WAIT_CANCEL, (ulong *)&target->status ) ){	
							clear_bit( HFC_WAIT_LOGIN, (ulong *)&target->status );
							clear_bit( HFC_WAIT_CANCEL, (ulong *)&target->status );
							if (test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)) {
								rsp = hfc_clear_target_info( ap, target, TRUE, TRUE );
							}
						}
						else if( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags) &&		/* FCLNX-GPL-0052 */
								 !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) ){
							clear_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
							clear_bit( HFC_NEED_CANCEL, (ulong *)&target->status );
							rsp = hfc_clear_target_info( ap, target, TRUE, TRUE );				/* FCLNX-GPL-0052 */
						}
					}
				}																				/* FCLNX-GPL-038 */
			}
#endif

			if (hfc_manage_info.npubp->hfc_mp_hsd_enable)						/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_make_fcinfo(ap, NULL, err_no, SCS_MCK, NULL, 0);
			
			break ;
			
		case HFC_ABEND_MB_TOUT:
		case HFC_ABEND_TOUTCHK_XOB:
		case HFC_ABEND_RID_INVALID:
		case HFC_ABEND_HFCPKT_CHK:											/* FCLNX-GPL-0135 */
		case HFC_ABEND_PIC_ERROR:											/* FCLNX-GPL-0576 */
			/* (1) Mail Box timeout */
			/*==> Process <==                                    */
			/*   -> Execute force check stop                     */
			if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
			{	/* "PCI BUS ERR" has happen. */
				HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
			}
			else
			{
				HFC_ISSUE_FMCK(ap, type);										/* FCLNX-0279 */
			}
			break ;
			
		case HFC_ABEND_SRAM_CE:
			hfc_pcie_sram_ce_recovery(ap);
			break;
		default:
			HFC_ISSUE_FMCK(ap, type);										/* FCLNX-0279 */
			break ;
	}
	
	hfc_hand2_trace(HFC_TRC_ABEND, 0x10, ap, NULL, NULL, 0, 0, 0);
	return;
}


/*
 * Function:    hfc_pcibus_chk
 *
 * Purpose:     Check the status of PCI bus
 *
 * Arguments:   
 *  ap         -
 *
 * Returns:     
 *  0          - OK
 *  -1         - NG
 *
 * Notes:       
 */
int hfc_pcibus_chk(struct adap_info *ap)
{
	uint			read_reg = 0 ;

	HFC_DBGPRT(" hfcldd : hfc_pcibus_chk - start\n");

	if( test_bit(HFC_CHK_STOP, (ulong *)&ap->status ) )	/* FCLNX-GPL-550 */
		return(-1);

	read_reg = hfc_read_reg_ext( ap,(uint)0, (char)0x4) ;
	
	if( read_reg == 0xffffffff )
	{
		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_EVNT4,
													0x5c,NULL,0) ;
		return(-1) ;
	}
	return(0) ;
}


/*
 * Function:    hfc_mck_recovery_fpp
 *
 * Purpose:     Machine check recovery process for FPP
 *
 * Arguments:   
 *  ap         - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mck_recovery_fpp(struct adap_info *ap,uchar type)
{
	uint				dump_reg[4] ;
	uint				err_no,rc=0 ;
	struct mp_adap_info		*mpap;

	HFC_DBGPRT(" hfcldd : hfc_mck_recovery - start\n");

	hfc_hand2_trace(HFC_TRC_MCKREC,0x10,
					ap,NULL,NULL,0,(uint64_t)type,0);
	
	mpap = ap->mp_adap_info;
	
	ap->mck_type = type;

	rc = lock_try_mpap( mpap );												/* FCLNX-GPL-057 */
	
	if( rc == 0 ){		/* Lock failed */
		set_bit( HFC_LOCK_WAIT_7, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}
	
	if( test_bit( HFC_LOCK_WAIT_7, (ulong *)&ap->mpap_lock ) )
	{
		/* Stop retry timer */
		clear_bit( HFC_LOCK_WAIT_7, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );

		hfc_hand2_trace(HFC_TRC_MCKREC,0x12,
				ap,NULL,NULL,0,(uint64_t)type,0);
	}																		/* FCLNX-GPL-0576 */

	if( test_bit(HFC_DIAG_DELAY, (ulong *)&ap->status ) )
	{
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);					/* FCLNX-0269 */
		clear_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
	}

	if( test_bit( HFC_DIAG_PROGRESS, (ulong *)&mpap->status ) )
	{
		/*--- DIAG Mode ---*/
		if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
		{
			hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
		}
		clear_bit(HFC_ONLINE, (ulong *)&ap->status);
		
		/* Log out */ /* FCLNX-GPL-111 */
		dump_reg[0] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4) ;
		dump_reg[1] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
		dump_reg[2] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
		dump_reg[3] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
			
		HFC_MEMCPY(ap->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
		hfc_logout(ap, 0x36, HFC_ERRLOG_TYPE_IMLLOG); /* ErrNo:0x36 POST Err */
		hfc_errlog(
			ap,NULL,NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF,0x36,ap->logdata,16) ;	/* FCLNX-GPL-391 */

		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-GPL-0576 */
		return ;
	}
	if( test_bit(HFC_CHK_STOP, (ulong *)&ap->status ) )
	{
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-GPL-0576 */
		return ;
	}
	mpap->mck_err_cnt++ ;	
	if( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status ) )
	{
		/* Is MCK recovery executing */
		hfc_chk_stop( ap, FALSE ) ;								/* FCLNX-0276 */
	}
	else if( (ap->max_mck_cnt) && (mpap->mck_err_cnt >= ap->max_mck_cnt) )
	{
		/* MCK occured more than specified count by user (ap->max_mck_cnt) */
		/*  ( Do not force HBA checkstop if ap->max_mck_cnt is zero        */
		hfc_chk_stop( ap, FALSE ) ;								/* FCLNX-0276 */
	}
	else
	{
			/* MCK recovery flag ON */
			dump_reg[0] = hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4) ; /* FCLNX-GPL-067 */
			dump_reg[1] = hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
			dump_reg[2] = hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
			dump_reg[3] = hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
			if( type == HFC_ABEND_MPCHK )
				err_no = 0x0000002c ;
			else if( type == HFC_ABEND_T3 )
				err_no = 0x0000002d ;
			else
				err_no = 0x0000002b ;
			memcpy(ap->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
			hfc_fpp_logout(ap,err_no,HFC_ERRLOG_TYPE_MCK);
			if( type == HFC_ABEND_MCK_INT )
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR2,
															err_no,ap->logdata,16) ;	/* FCLNX-GPL-391 */
			else
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR4,
														err_no,ap->logdata,16) ;		/* FCLNX-GPL-391 */
			if( dump_reg[1] & 0x00800000 )
			{
				/* 
				 * Reset device status in config register (0xffff write) 
				 * if PCI ERR DETECTED in status register equals 1.
				 */
				
				hfc_write_cnfg(ap,6,0x2,0xffff);
			}

		set_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status );
		hfc_reset_start(ap,HFC_INI_RESET);
		hfc_reset_adap_info(ap) ;

		/* WS Comu Area(0x318) */
		hfc_reset_start(ap,HFC_WSCA_CLEAR);
		hfc_reset_start(ap,HFC_SET_WS80);			/* WS Comu Area(0x318) */
		hfc_reset_start(ap,HFC_SET_INIADR);			/* INITTBL ADR(0x310)  */
													/* ALPA(0x319)         */

		hfc_w_stop(ap,HFC_REBOOT_DELAY_TMR) ;
		hfc_w_start(ap,HFC_REBOOT_DELAY_TMR) ;	

		hfc_reset_start(ap,HFC_REBOOT);
		hfc_hand2_trace(HFC_TRC_MCKREC,0x11,
					ap,NULL,NULL, 0,(uint64_t)type,0);
	}
	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-GPL-0576 */
}


/*
 * Function:    hfc_mck_recovery_five
 *
 * Purpose:     Machine check recovery(for Five)
 *
 * Arguments:   
 *  ap         - Pointer to dap_info
 *  type       - error type
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mck_recovery_five(                                                         /* FCWIN-0220 */
	struct adap_info            *ap,
	uchar                        type)
{
	struct mp_adap_info     *mpap;
	uint                    dump_reg[4], wk4 ;
	uint                    status0;
	uint                    err_no ;
	int						rc ;
	
	mpap = ap->mp_adap_info;
	
	hfc_hand2_trace(HFC_TRC_MCKREC,0x20,
					ap,NULL,NULL,0,(uint64_t)type,0);
	
	ap->mck_type = type;
	set_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );						/* FCLNX-GPL-0157 */
	
	rc = lock_try_mpap( mpap );												/* FCLNX-0276 */
	
	if( rc == 0 ){		/* Lock failed */
		set_bit( HFC_LOCK_WAIT_1, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}

																			/* FCLNX-0287 */
	hfc_w_stop(ap,HFC_MCKINT_TMR) ;

	if( test_bit( HFC_LOCK_WAIT_1, (ulong *)&ap->mpap_lock ) )
	{
		/* Stop retry timer */
		clear_bit( HFC_LOCK_WAIT_1, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );

		hfc_hand2_trace(HFC_TRC_MCKREC,0x22,
				ap,NULL,NULL,0,(uint64_t)type,0);
	}																		/* FCLNX-0276 */
	
#if _HFC_ERROR_INJ_00
	set_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
	set_bit(HFC_MCK_HW_INIT, (ulong *)&ap->mck_result );
	set_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status );
	clear_bit(HFC_ONLINE, (ulong *)&ap->status);
	
	if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
	{
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
	}
	if( (test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status)) || (ap->initialize == 1) )
	{
		unlock_mailbox( ap ) ; /*goto*/
		hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		clear_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status);		/*goto*/
	}
	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); 
	return ;
#endif

	if( test_bit(HFC_DIAG_DELAY, (ulong *)&ap->status ) )
	{
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);					/* FCLNX-0296 */
		clear_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
	}

	if( test_bit( HFC_DIAG_PROGRESS, (ulong *)&mpap->status ) )
	{
		/*--- DIAG Mode ---*/
		set_bit(HFC_MCK_HW_INIT, (ulong *)&ap->mck_result );
		set_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status );
		clear_bit(HFC_ONLINE, (ulong *)&ap->status);
		clear_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status );				/* FCLNX-GPL-0517 */
		/* Timer stop */
		if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
		{
			hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
		}
		
		/* Log out */ /* FCLNX-GPL-111 */
		dump_reg[0] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4) ;
		dump_reg[1] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
		dump_reg[2] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
		dump_reg[3] = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
		
		HFC_MEMCPY(ap->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
		hfc_logout(ap, 0x36, HFC_ERRLOG_TYPE_IMLLOG); /* ErrNo:0x36 POST Err */
		hfc_errlog(
			ap,NULL,NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF,0x36,ap->logdata,16) ;	/* FCLNX-GPL-391 */

		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); 
		return ;
	}
	if ( test_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status ) )
	{
		set_bit(HFC_MCK_HWCHKSTOP, (ulong *)&ap->mck_result );
		clear_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status );					/* FCLNX-GPL-0517 */
		
		if(test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status )) 
		{
			hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
		}
		
		if( (test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status)) || (ap->initialize == 1) )
		{
			unlock_mailbox( ap ) ; 
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
			clear_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status);		
		}
		
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); 
		return ;
	}
	
	
	wk4 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4) ;		/* FCWIN-0197 STR */
	HFC_4L_TO_4B(dump_reg[0],wk4);
	wk4 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
	HFC_4L_TO_4B(dump_reg[1],wk4);
	wk4 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS1, (char)0x4) ;
	HFC_4L_TO_4B(dump_reg[2],wk4);
	wk4 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_ERRDETAIL0, (char)0x4) ;
	HFC_4L_TO_4B(dump_reg[3],wk4);											/* FCWIN-0197 END */
	
	if( type == HFC_ABEND_MPCHK )
		err_no = 0x0000002c ;
	else if( type == HFC_ABEND_T3 )
		err_no = 0x0000002d ;
	else
		err_no = 0x0000002b ;
	
	HFC_MEMCPY(ap->logdata,(uchar*)dump_reg,16);	/* FCLNX-GPL-391 */
	
	if( ( ap->mck_progress == HFC_MCK_PROGRESS ) || 						/* FCLNX-GPL-0517 */
		( test_bit(HFC_SUBMCK_RECOVERY, (ulong *)&mpap->status ) ) ) 
	{
		/* MCK occured during MCK, or MCK recovery is in progress sub port. */
		HFC_ISSUE_CSTP( ap , TRUE, HFC_ABEND_FCSTP ) ;						/* FCLNX-0276 *//* @MLPF *//* FCLNX-GPL-316 */
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);								/* FCLNX-0276 */
		
		/* FCLNX-GPL-209 */
		
		ap->mck_progress = 0;												/* FCLNX-GPL-0517 */
		return;
	}
	
	if(ap->pkg.type == HFC_PKTYPE_FIVE)
	{
		if( dump_reg[1] & 0x00800000 )
		{
			/* 
			 * Reset device status in config register (0xffff write) 
			 * if PCI ERR DETECTED in status register equals 1.
			 */
			hfc_write_cnfg(ap, 0x06, 0x2, 0xffff);
			hfc_write_cnfg(ap, 0x6c, 0x4, 0xffffffff);
			hfc_write_cnfg(ap, 0x70, 0x4, 0x0fffffff);
		}
	}
	else /* FIVE-EX */
	{
		/* NOP */
	}
	
	status0 = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_STATUS0, (char)0x4) ;
	if (!(status0 & 0x00400000 )
	 || test_bit(HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock) )				/* FCLNX-0279 */
	{
		/* Other port is executing Machine Check recovery process */
		set_bit(HFC_SUBMCK_RECOVERY, (ulong *)&mpap->status );			/* FCLNX-0493	*/
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); 
		hfc_reset_adap_info(ap) ;
		set_bit(HFC_MCK_END, (ulong *)&ap->mck_result );

		hfc_hand2_trace(HFC_TRC_MCKREC,0x25, ap,NULL,NULL,(uint64_t)status0, mpap->lock, 0);	/* FCLNX-0287 */
		
		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,err_no,ap->logdata,16) ;	/* FCLNX-GPL-391 */
		
		hfc_w_stop(ap,HFC_MCK_DELAY_TMR) ;
		hfc_w_start(ap,HFC_MCK_DELAY_TMR) ;
		return ;
	}
	/* FCLNX-GPL-057 */
	mpap->mck_err_cnt++ ;
	/* FCLNX-GPL-213 */	
	
	if( test_bit(HFC_FLASH_UPDATE_PROGRESS, (ulong *)&mpap->status) )	/* FCLNX-GPL-613 */
	{
		/* When processing flash update, if a mck occuers, driver issues F-CSTOP. */

		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xf2,ap->logdata,16) ;
//		mpap->mck_err_cnt = ap->max_mck_cnt;
	}	/* FCLNX-GPL-613 */
	
	if( ((ap->max_mck_cnt) && (mpap->mck_err_cnt >= ap->max_mck_cnt))		/* FCLNX-GPL-057 */
		|| (test_bit(HFC_FLASH_UPDATE_PROGRESS, (ulong *)&mpap->status)) )	/* FCLNX-GPL-FX-238,272 */
	{
		/* MCK occured eight times or more */

		HFC_ISSUE_CSTP( ap, TRUE, HFC_ABEND_FCSTP ) ;					/* FCLNX-0276 *//* FCLNX-GPL-316 */
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-0276 */
		set_bit(HFC_MCK_CNT_OVER, (ulong *)&ap->mck_result );
		
		/* FCLNX-GPL-209 */
		
		return;
	}
	
	set_bit( HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock );
	
	/* Get HW ERR Log */
	hfc_logout(ap, err_no, HFC_ERRLOG_TYPE_MCK);

	/* Get Dump Log */
	if( type == HFC_ABEND_MCK_INT )
	{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR2,
			err_no,ap->logdata,16) ;	/* FCLNX-GPL-391 */
	}
	else{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR4,
			err_no,ap->logdata,16) ;	/* FCLNX-GPL-391 */
	}
	
	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	
	/* Clear Registers for FIVE-EX */
	if(ap->pkg.type == HFC_PKTYPE_FIVE_EX){ /* Check pkg.type */
		/* Set DUMP Core's FRZ_CLR bit */ /* HFC_DUMP_FRZ_CLR is 0x00400000 */
/*		hfc_write_reg(ap,( uint )HFC_IOSPACE_DUMP_CMD,( char )0x4, HFC_DUMP_FRZ_CLR); */ /* FCLNX-GPL-081 */
		/* Clear Config Register Sticky bit */
		hfc_clear_sticky_bit(ap);
	}
	
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_CTLRST_DELAY_TMR, 0, TRUE);	/* FCLNX-0276 */
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_CTLRST_DELAY_TMR, 1, FALSE);	/* FCLNX-0276 */

	return;
}


/*
 * Function:    hfc_mck_port_recovery
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - 
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_mck_port_recovery(struct adap_info	*ap)
{
	HFC_DBGPRT(" hfcldd : hfc_port_recovery - start\n");

    if( ( test_bit(HFC_MCK_SUCCESS,   (ulong *)&ap->mck_result) ) ||
        ( test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status  )   ) )
	{
		/* Mck process succeeded */
		hfc_reset_start(ap, HFC_FW_START);                      /* FCWIN-0237 */
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		
		//@MLPF
		if( ! ( HFC_MMODE_CHECK_SHARED(ap) ) )
			set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
		else if (HFC_MMODE_CHECK_SHADOW (ap) )
			set_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status);
		
		if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
			if(ap->mck_on_sleep) {
				hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Interruption mask setting */
		//@MLPF
		if( HFC_MMODE_CHECK_SHARED(ap) )
		{
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask_mlpf[ap->pkg.type]);
		}
		else
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask[ap->pkg.type]);

		/* Link Up Timer Set */
		//@MLPF
		if( ! ( HFC_MMODE_CHECK_SHARED(ap) ) )
		{
			hfc_w_stop( ap, HFC_LINKUP2_TMR );						/* FCLNX-0241 */
			hfc_w_start( ap, HFC_LINKUP2_TMR );						/* FCLNX-0241 */
		}
		else if (HFC_MMODE_CHECK_SHADOW (ap) )
		{
			hfc_w_stop( ap, HFC_MLPF_MCKEND_TMR );
			hfc_w_start( ap, HFC_MLPF_MCKEND_TMR );
		}
	}
	else if( test_bit(HFC_MCK_HW_INIT, (ulong *)&ap->mck_result) )
	{
		/* HW initialization is in progress */
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
			if(ap->mck_on_sleep) {
				hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Set interruption mask */
		set_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status );
	}
	else if( test_bit(HFC_MCK_HWCHKSTOP, (ulong *)&ap->mck_result) )
	{
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
			if(ap->mck_on_sleep) {
				hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Set interruption mask */
		clear_bit( HFC_ONLINE, (ulong *)&ap->status );
	}
	else if( test_bit(HFC_MCK_CNT_OVER, (ulong *)&ap->mck_result) )
	{
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
			if(ap->mck_on_sleep) {
				hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Set interruption mask */
		clear_bit( HFC_ONLINE, (ulong *)&ap->status );
	}
	else if( test_bit(HFC_MCK_END, (ulong *)&ap->mck_result)||test_bit(HFC_MCK_EXEC, (ulong *)&ap->mck_result) )
	{
		hfc_reset_adap_info(ap);
		/* WS Comu Area(0x318) */
		hfc_reset_start(ap, HFC_SET_WS80);
		/* INITTBL ADR(0x310)  */
		/* ALPA(0x319)         */
		hfc_reset_start(ap, HFC_SET_INIADR);
		hfc_reset_start(ap, HFC_FW_START);                      /* FCWIN-0237 */
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		//@MLPF
		if( ! ( HFC_MMODE_CHECK_SHARED(ap) ) )
			set_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status );
		else if (HFC_MMODE_CHECK_SHADOW (ap) )
			set_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status);
		/* Set interruption mask */
		//@MLPF
		if( HFC_MMODE_CHECK_SHARED(ap) )
		{
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask_mlpf[ap->pkg.type]);
		}
		else
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask[ap->pkg.type]);

		/* Link Up Timer Set */
		//@MLPF
		if( ! ( HFC_MMODE_CHECK_SHARED(ap) ) )
		{
			hfc_w_stop( ap, HFC_LINKUP2_TMR );						/* FCLNX-0241 */
			hfc_w_start( ap, HFC_LINKUP2_TMR );						/* FCLNX-0241 */
		}
		else if (HFC_MMODE_CHECK_SHADOW (ap) )
		{
			hfc_w_stop( ap, HFC_MLPF_MCKEND_TMR );
			hfc_w_start( ap, HFC_MLPF_MCKEND_TMR );
		}
	}
	
	unlock_mailbox(ap);
	
	return TRUE;
}


/*
 * Function:    hfc_chk_stop
 *
 * Purpose:     Check stop processing
 *
 * Arguments:   
 *  ap         - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_chk_stop(struct adap_info *ap, uchar mpap_lock)
{
	struct mp_adap_info         *mpap;
	struct adap_info            *wk_ap;             /* FCWIN-0220 */
	int                         i, rc; /* FCWIN-0220 */ /* FCLNX-GPL-209 */
	/* FCLNX-GPL-209 */
	int chkstp_retry = FALSE; /* FCLNX-GPL-221 */
	struct target_info			*target=NULL; /* FCLNX-GPL-327 */


	HFC_DBGPRT(" hfcldd : hfc_chk_stop - start\n");

	hfc_hand2_trace(HFC_TRC_MCKREC,0x40,
					ap,NULL,NULL,0,0,0);
	
	mpap = ap->mp_adap_info;
	HFC_DBGPRT(" hfcldd%d : hfc_chk_stop \n",ap -> dev_minor);

	if ( mpap_lock == FALSE ) {										/* FCLNX-0279 */
		rc = lock_try_mpap( mpap );

		if( rc == 0 ){
			set_bit( HFC_LOCK_WAIT_2, (ulong *)&ap->mpap_lock );
			hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
			hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
			goto  mp_adap_info_lock_fail; /* FCLNX-GPL-221 */
		}
	}

	if( test_bit( HFC_LOCK_WAIT_2, (ulong *)&ap->mpap_lock ) )
	{
		/* Stop retry timer */
		clear_bit( HFC_LOCK_WAIT_2, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
	}																/* FCLNX-0279 */

	set_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status ) ;

//	if( hfc_manage_info.hfcplus_enable ){	/* FCLNX-GPL-349 */
	set_bit(HFC_HWISOL,(ulong *)&mpap->status);					/*FCLNX-0506*/
	
	/* FCLNX-GPL-221 */
	
	if (hfc_manage_info.npubp->hfc_mp_hsd_enable)					/* FCLNX-0429 */
		hfc_manage_info.npubp->hfc_make_fcinfo(ap, NULL, 0x31, SCS_MCK, NULL, 0);

	if ( ap->pkg.type == HFC_PKTYPE_FPP)
	{
		/*** Log out ***/ /* FCLNX-GPL-221 */
		hfc_logout(ap,0x00000031,HFC_ERRLOG_TYPE_CHKSTP);
		hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_CHKSTP,ERRID_HFCP_ERR1,0x31,ap->logdata,16) ;	/* FCLNX-GPL-391 */
		
		clear_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status);
		if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
			if(ap->mck_on_sleep) {
				hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);				/* FCLNX-0296 */
			}
		} 
		/* Set interruption mask */
		set_bit(HFC_CHK_STOP, (ulong *)&ap->status );
		clear_bit(HFC_ONLINE, (ulong *)&ap->status );
		hfc_reset_start(ap,HFC_INI_RESET);
		hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDFCIF,( char )0x1,(char)0x80);
		hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000000 );
		/*-- 1.67 LED 0x44 dummy write(H/W Bug) --*/
		hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
		hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDLED,( char )0x1,HFC_WAKE_UP_FAILURE);
	}
	else	/* pkg.type is FIVE or FIVE-EX */
	{
		/* Inhibit interruption in all ports */	
//		if(hfc_manage_info.hfcplus_enable){
		if(hfc_manage_info.hfcldd_mp_mod){	/*FCLNX-GPL-349*/
			hfc_w_stop(ap,HFC_LDLERR_TMR);	/*FCLNX-0506*/
			hfc_w_stop(ap,HFC_LDSERR_TMR);	/*FCLNX-0506*/
			hfc_w_stop(ap,HFC_IFERR_TMR);	/*FCLNX-0506*/
			hfc_w_stop(ap,HFC_TOERR_TMR);	/*FCLNX-0506*/
			for (i=0;i<ap->max_target;i++){	/* FCLNX-GPL-327 */
				target = ap -> target_arg[i];
				if(target != NULL){
					hfc_watchdog_enter( ap, target, NULL, 0, HFC_TGT_LDLERR_TMR, 0,1);
					hfc_watchdog_enter( ap, target, NULL, 0, HFC_TGT_LDSERR_TMR, 0,1);
				}
			}								/* FCLNX-GPL-327 */
		}		
		wk_ap = ap->mp_adap_info->ap;
		
		for( i=0 ; i<4 ; i++)
		{
			if (wk_ap == NULL)
			{	/* There are no other ports in this core. */
				break;
			}
			
			/* FCLNX-GPL-221 start */
			/*** adap_info status check ***/
			if( test_bit(HFC_CHK_STOP, (ulong *)&wk_ap->status) )
			{	/* That port's status has been CHK_STOP. */
				/* Go next */
				wk_ap = wk_ap->next;
				continue;
			}
			
			/*** Port(adap_info) lock  ***/
			rc = 1;
			if(wk_ap != ap)
			{	/* for other port */
				if( !hfc_manage_info.hfcldd_mp_mod )
				{	/* for only GPL */
					rc = spin_trylock(&wk_ap->adap_lock);
				}
			}
			/*** Did this process really lock that port ? ***/
			if( rc == 0 )
			{	/* No, other process is using that port, now.     */
				/* This process will challenge to lock that port, */
				/* after waiting 1sec.                            */
				chkstp_retry = TRUE;
				/* Go next */
				wk_ap = wk_ap->next;
				continue;
			}
			
			/*** Log out ***/
			hfc_logout(wk_ap,0x00000031,HFC_ERRLOG_TYPE_CHKSTP);
			hfc_errlog(wk_ap,NULL,NULL,HFC_ERRLOG_TYPE_CHKSTP,ERRID_HFCP_ERR1,0x31,ap->logdata,16) ;	/* FCLNX-GPL-391 */
			/* FCLNX-GPL-221 end */
			
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
				set_bit(HFC_CHK_STOP, (ulong *)&wk_ap->status );
				set_bit(HFC_ISOL,(ulong *)&wk_ap->status );	/* FCLNX-707 */
				wk_ap->isol_detail = HFC_ISOLATE_CHKSTP;	/* FCLNX-707 */
			}
#endif

			/* Cancel pending SCSI commands, and stop timers. */
			hfc_mck_prepare(wk_ap); /* FCLNX-GPL-209 */
			if (ap->rport_lu_scan == 1) {
				hfc_wwnverify_linkup_timeout(ap, NULL, 0);	/* FCLNX-GPL-565 */
			}
			
			clear_bit(HFC_MCK_RECOVERY, (ulong *)&wk_ap->status);
			if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&wk_ap->status)){
				if(wk_ap->mck_on_sleep) { /* FCLNX-GPL-209 */
					hfc_wake_up(&wk_ap->mck_event, &wk_ap->mck_event_wait);					/* FCLNX-0296 */
				}
			} 

#if !( defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
			set_bit(HFC_CHK_STOP, (ulong *)&wk_ap->status );
			set_bit(HFC_ISOL,(ulong *)&wk_ap->status );	/* FCLNX-707 */
			wk_ap->isol_detail = HFC_ISOLATE_CHKSTP;	/* FCLNX-707 */
#else
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ){ /* FCLNX-GPL-FX-472 */
				set_bit(HFC_CHK_STOP, (ulong *)&wk_ap->status );
				set_bit(HFC_ISOL,(ulong *)&wk_ap->status );	/* FCLNX-707 */
				wk_ap->isol_detail = HFC_ISOLATE_CHKSTP;	/* FCLNX-707 */
			}
#endif

			/* Ctrl Reset */
			if( !(wk_ap->debug_func & HFC_DEBUG_CTLRST) ){ /* FCLNX-GPL-209 */
				/* FCLNX-GPL-554 BS500 TX_Disable disconnect */
				if ( (ap->pkg.type == HFC_PKTYPE_FIVE_EX) &&
					(0x94 <= ap->pkg.code) && (ap->pkg.code <= 0x97) )
				{
					hfc_reset_start(ap, HFC_INI_RESET);
				} else {
					hfc_reset_start(ap, HFC_CTLRST);
					udelay(1000); /* FCLNX-GPL-081 */
				}
			}
			/* Stop optical transmission */
			if(!(HFC_MMODE_CHECK_SHARED(ap))){
				hfc_write_reg(wk_ap,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
			}
			else{
				hfc_mlpf_set_fcif(ap, 0x80808080);	/* FCLNX-GPL-399 */
			}
			
			/* Turn LED (Yellow and Green) off */
			if(!(HFC_MMODE_CHECK_SHARED(ap))){
				hfc_write_reg(wk_ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
			}
			else{
				hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
			}
			
			hfc_write_reg(wk_ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000000 );
			clear_bit(HFC_ONLINE, (ulong *)&wk_ap->status );
			
			if( HFC_MMODE_CHECK_SHADOW(wk_ap) )
			{
				HFC_ERRPRT("hfcldd%d : shared FC driver enable status off by HW error.\n",wk_ap->dev_minor);
				hfc_mlpf_change_state(wk_ap, HFC_HG_HYPSTATUS_ENABLE, HFC_DISABLE_HYPER_STATE );   /* FCLNX-0385 */
				hfc_write_hg_reg(wk_ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_CSTPEND);     //@MLPF
			}
			
			/*** Wake up sleep event  ***/
			if(test_bit(HFC_MB_PROL, (ulong *)&wk_ap->mb_status ))	/* FCLNX-GPL-209 */
			{
				hfc_wake_up(&wk_ap->mb_event, &wk_ap->mb_event_wait);
			}
			if( (test_bit( HFC_WAIT_LINK_INIT, (ulong *)&wk_ap->status)) || (wk_ap->initialize == 1) )
			{
				unlock_mailbox( wk_ap ) ; 
				hfc_wake_up(&wk_ap->init_event,&wk_ap->int_a_poll);
				clear_bit( HFC_WAIT_LINK_INIT, (ulong *)&wk_ap->status);
			}
			
			/*** Port unlock  ***/ /* FCLNX-GPL-221 */
			if(wk_ap != ap)
			{	/* for other port */
				if( !hfc_manage_info.hfcldd_mp_mod )
				{	/* for only GPL */
					spin_unlock(&wk_ap->adap_lock);
				}
			}
			
			/*** Go next  ***/
			wk_ap = wk_ap->next;
		}
	}
	
	/*** unlock mp_adap_info ***/ /* FCLNX-0279 */ /* FCLNX-GPL-111,221 */
	if ( mpap_lock == FALSE ) {
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	}
	
	
	/*** Check necessity to retry. ***/		/* FCLNX-GPL-221 start */
	if( chkstp_retry == TRUE )
	{	/* This process must retry to chage other ports status(wk_ap->status). */
		set_bit( HFC_LOCK_WAIT_2, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
	}										/* FCLNX-GPL-221 end */
	
	
mp_adap_info_lock_fail:
	
	hfc_hand2_trace(HFC_TRC_MCKREC,0x21,ap,NULL,NULL,0,0,0);
	
	return;
}


/*
 * Function:    hfc_logout
 *
 * Purpose:     LOG OUT processing
 *
 * Arguments:   
 *  ap         -
 *  err_no     -
 *  mode       -
 *
 * Returns:     
 *
 * Notes:       
 */
 void hfc_logout(                          /* FCWIN-0220 STR */
	struct adap_info        *ap,
	uint                    err_no,
	uchar                   mode)
{
	switch(ap->pkg.type){
		case HFC_PKTYPE_FPP:
			hfc_fpp_logout(ap, err_no, mode);
			break;
		
		case HFC_PKTYPE_FIVE:
			hfc_five_logout(ap, err_no, mode);
			break;
		
		case HFC_PKTYPE_FIVE_EX:
			hfc_five_ex_logout(ap, err_no, mode);
			break;
		
		default:
			/* NOP */
			break;
	}
}


/*
 * Function:    hfc_watchdog
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
void hfc_watchdog(struct wtimer *w_timer)
{
	struct adap_info *ap=NULL;
	struct target_info *target=NULL ;
	struct hfc_pkt *hfcp=NULL,*wx_hfcp=NULL ;
	struct mp_adap_info		*mpap;
	unsigned long	flags = 0;													/* FCLNX-0274 */
	int		lp;
	uint	xob_no, dummy_read_reg=0;	/* FCLNX-GPL-FX-195 */

	int		lun=0 ;
	uint	int_a_rst;
	int 	rc=0 ;
	uint	hyp_status = 0;
	
	struct dev_info *dev=NULL;													/* FCLNX-GPL-038 */

	HFC_DBGPRT(" hfcldd : hfc_watchdog - start id=%x\n",w_timer->timer_id);

	if (hfc_manage_info.adap_info_arg[w_timer->ap_dev_minor] == NULL)			/* FCLNX-0322 */
		return;																	/* FCLNX-0322 */
	
	ap = w_timer->ap ;

	target = w_timer->target;
	hfcp = w_timer->hfcpk;
	if(hfcp != NULL)
	{
		lun = hfcp->lun_id;
	}
	dev = w_timer->dev;															/* FCLNX-GPL-047 */

	if(ap == NULL) return;
	
    HFC_ADAPLOCK_IRQSAVE(flags);

	hfc_hand2_trace(HFC_TRC_WDOG,0x00,
					ap,target,hfcp,0,
					(uint64_t)w_timer->timer_id,
					0);
					
	w_timer->timer_flag &= ~HFC_TIMER_VALID;
	mpap = ap->mp_adap_info;
	
	switch(w_timer->timer_id)
	{
		case HFC_DELAY_TMR:			/* DELAY timer after executing TARGET RESET		*/
			if(target == NULL) break;	/* FCLNX-GPL-038 */ /* FCLNX-GPL-189 */
			clear_bit( HFC_SCSI_DELAY, (ulong *)&target->status );

			if( (test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status) ) && (test_bit(HFC_ONLINE, (ulong *)&ap->status) ) )
			{
				/* If adapter is waiting LINK_UP, I/O start can not be executed. */
				break;																/* FCLNX-GPL-038 */
			}
			else if( !(test_bit(HFC_ONLINE, (ulong *)&ap->status) ) )
			{
				/* 
				 * If adapter is not online, target should have been already canceled.
				 * Break this process 
				 */
				break ;																/* FCLNX-GPL-038 */
			}
			else if( !(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status) )  && (test_bit(HFC_ONLINE, (ulong *)&ap->status) ) )
			{
				/* Restart waiting request if adapter is ONLINE */
				if(!(test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status) )  
				&& test_bit(HFC_NEED_LOGIN, (ulong *)&target->status)  )			/* FCLNX-GPL-038 */
				{
					clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );
					if( hfc_issue_relogin(ap, target) )
					{
						/* Initiated LOGIN but failed with mailbox busy */
						set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );			/* FCLNX-GPL-038 */
						clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );		/* FCLNX-GPL-038 */
						hfc_enque_login_req(ap, target);
					}
					break;
				}
				else if( test_bit(HFC_NEED_CANCEL, (ulong *)&target->status) )  	/* FCLNX-GPL-038 */
				{
					clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status );
					if( hfc_issue_relogin(ap, target) )
					{
						/* Initiated CANCEL LOGIN but failed with mailbox busy */
						set_bit(HFC_NEED_CANCEL, (ulong *)&target->status );		/* FCLNX-GPL-038 */
						clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status );		/* FCLNX-GPL-038 */
						hfc_enque_login_req(ap, target);
					}
					break;
				}																	/* FCLNX-GPL-038 */
				if( ( target->wx_que_cnt > 0 ) || ( ap->xob_wait_exec_cnt > 0 )){					/*-- FCLNX-019 STR--*/
					hfc_issue_intl_start(ap,target) ;
				}												/*-- FCLNX-019 END--*/
			}
			break ;
			
		case HFC_SCSI_CMD_TMR:		/* SCSI Command(Normal) Time-Out	*/
			if(target == NULL) break; /* FCLNX-GPL-189 */
			if( ( target->we_que_cnt == 0 )&&( target->wx_que_cnt == 0 ) )
				break ;
			if( hfcp == NULL ) break;
			if( dev == NULL ) break;		/* FCLNX-GPL-0343 */
			
//			if((hfc_manage_info.hfcplus_enable)&&(ap->to_errcnt_info!=NULL)){                              /* FCLNX-0506 */
			if(ap->to_err_limit){                              /* FCLNX-GPL-349 */
				if(hfc_manage_info.hfcldd_mp_mod){
					hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_TO_ERR);  /* FCLNX-0506 */
				}
				else{
					hfc_watched_errcount_i(ap, NULL, HFC_TO_ERR);  /* FCLNX-GPL-349 */
				}
			}
				
			set_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );
			ap->scsi_timeout_fail++;		/* FCLNX-GPL-0343 */

			if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
			{	/* "PCI BUS ERR" has happen. */
				HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
			}
			else
			{
				wx_hfcp = target->wx_que_top;
				
				while( wx_hfcp != NULL )
				{
					if( wx_hfcp == hfcp )
						break;
					
					wx_hfcp = wx_hfcp->cmd_forw;
				}
				
				if( wx_hfcp != NULL )
				{
					if( (target->status == HFC_NON_STATUS) && !(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status)) )
					{
#if 0
						timeout = (uint)(wx_hfcp->cmd_pkt->timeout_per_command/HZ);
#endif
					}
					hfc_deque_wx_que(target, wx_hfcp);
					hfc_iodone(ap, wx_hfcp->cmd_pkt, wx_hfcp);	/* FCLNX-0429 */
					break ;
				}
				
				if( hfc_toutchk_xob(ap, target, hfcp, lun, HFC_ISSUE_ABORT) )
				{
					/* 
					 * Timed-out xob remain in queue so do not initiate
					 * MIH-LOG and Abort-Task-Set 
					 */
					break ;
				}

				/* MIH-LOG Request */
				/* Issue Abort_Task_Set after collecting MIH-LOG in case SCSI command is timeout */
			    if( hfc_issue_mihlog(ap, target, hfcp) )
			    {
					/* Failed to initiate mailbox request */
					/* Execute Abort without collecting MIH-LOG */
					if(ap->c_err!=0) break;
					if(!ap->abort_t_restrain){	/* FCLNX-0506 */
						if( test_bit(HFC_NEED_LOGIN, (ulong *)&target->status )  )		/* FCLNX-GPL-038 */
						{
							if( hfc_issue_relogin(ap,target) )
							{
								set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );		/* FCLNX-GPL-038 */
								clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
								hfc_enque_login_req(ap, target);
							}
							break ;
						}
						else if( test_bit(HFC_NEED_CANCEL, (ulong *)&target->status ) )	/* FCLNX-GPL-038 */
						{
							if( hfc_issue_relogin(ap,target) )
							{
								set_bit(HFC_NEED_CANCEL, (ulong *)&target->status );	/* FCLNX-GPL-038 */
								clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status );	/* FCLNX-GPL-038 */
								hfc_enque_login_req(ap, target);
							}
							break ;
						}
					
						if( (test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status ) ) ||
							(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ) ) ||
							(test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status ) ) ||	/* FCLNX-GPL-038 */
							(dev->lustat & HFC_WAIT_ABORT)  ||							/* FCLNX-GPL-0343 */
							(dev->lustat & HFC_WAIT_LUN_RESET) )						/* FCLNX-GPL-0343 */
							break ;                                            
						hfc_issue_task_mgm(ap, target, hfcp, lun, HFC_ISSUE_ABORT) ;
					/* If initiation succeeds, set HFC_WAIT_HALT in hfc_isuue_task_mgm() */
					}
					else{
						if(ap->login_restrain)	/* FCLNX-0506 */
						{
							set_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
							clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);		/* FCLNX-GPL-038 */
						}
						if( test_bit(HFC_NEED_LOGIN, (ulong *)&target->status )  )		/* FCLNX-GPL-038 */
						{
							if( hfc_issue_relogin(ap,target) )
							{
								set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );		/* FCLNX-GPL-038 */
								clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
								hfc_enque_login_req(ap, target);
							}
							break ;
						}
						else if( test_bit(HFC_NEED_CANCEL, (ulong *)&target->status ) )	/* FCLNX-GPL-038 */
						{
							if( hfc_issue_relogin(ap,target) )
							{
								set_bit(HFC_NEED_CANCEL, (ulong *)&target->status );	/* FCLNX-GPL-038 */
								clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status );	/* FCLNX-GPL-038 */
								hfc_enque_login_req(ap, target);
							}
							break ;
						}
						if( (test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status ) ) ||
							(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ) ) ||
							(test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status ) )||	/* FCLNX-GPL-337 */
							(dev->lustat & HFC_WAIT_ABORT) )					
							break ;                                            
						hfc_issue_task_mgm(ap, target, hfcp, lun, HFC_ISSUE_LOGIN) ;	/* FCLNX-0500 */
					}
				}	
				if(ap->c_err!=0) break; /* FCLNX-0597 */
				if(ap->abort_t_restrain){
					set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );
					clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );				/* FCLNX-GPL-038 */
				 	if(ap->login_restrain){
						set_bit(HFC_NEED_CANCEL, (ulong *)&target->status );
						clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status );			/* FCLNX-GPL-038 */
					}
				}
				else{
					if( (test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status ) ) ||
						(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ) ) ||
						(test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status ) ) ||		/* FCLNX-GPL-337 */
						(dev->lustat & HFC_WAIT_ABORT) )
						break ;
					dev->lustat |= HFC_NEED_ABORT;
				}
			}
			
			break ;
			
		case HFC_ABORT_TMR:		/* Abort Task Set/Clear ACA Time-Out*/
			if(target == NULL) break; /* FCLNX-GPL-189 */
			if( target->we_que_cnt == 0 )
				break ;
			if( hfcp == NULL ) break;
			if( dev == NULL ) break;		/* FCLNX-GPL-0343 */
		
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-GPL-430 */
				if(ap->rt_err_enable){                              /* FCLNX-0506 */
					if(hfc_check_cmnd_timeout(ap,target)){			/* FCLNX-GPL-430 */
						hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);  /* FCLNX-0506 */
					}
				}
				else{
					if(ap->to_errcnt_info!=NULL)              /* FCLNX-0506 */
						 hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_TO_ERR);
				}
			}
			else if(ap->rt_err_enable){                              /* FCLNX-0506 */
				hfc_watched_errcount_i(ap, NULL, HFC_RT_ERR);
			}
									
			ap->scsi_err_cnt++ ;
			xob_no = hfcp->cmd_xob;
			hfcp->adap_status |= SCS_CMD_TIMEOUT;

			if( HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ) {	/* FCLNX-GPL-0343 */
				dev->lustat &= ~HFC_NEED_LUN_RESET;
				dev->lustat &= ~HFC_WAIT_LUN_RESET;
			}
			else {
				dev->lustat &= ~HFC_NEED_ABORT;
				dev->lustat &= ~HFC_WAIT_ABORT;
			}													/* FCLNX-GPL-0343 */

			if( hfc_toutchk_xob(ap, target, hfcp, lun, HFC_ISSUE_TARGET_RESET) )
			{
					/*
					 * Timed-out xob remain in queue so do not initiate
					 * MIH-LOG and Abort-Task-Set 
					 */
				break ;
			}

			if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
			{	/* "PCI BUS ERR" has happen. */
				HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
			}
			else
			{
				/* MIH-LOG Request */
				/* Issue Abort_Task_Set after collecting MIH-LOG in case SCSI command is timeout */
				if(ap->login_restrain) {
					set_bit(HFC_NEED_CANCEL, (ulong *)&target->status);					/* FCLNX-0506 */
					clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);				/* FCLNX-0506 */ /* FCLNX-GPL-038 */
				}
				if( hfc_issue_mihlog(ap, target, hfcp) )
				{
					if(ap->c_err!=0) break; /* FCLNX-0597 */
					/* Failed to initiate mailbox request       */
					/* Execute Abort without collecting MIH-LOG */
					if( test_bit( HFC_NEED_LOGIN, (ulong *)&target->status ) )
					{
						if( hfc_issue_relogin(ap, target) )
						{
							set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
							clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
							hfc_enque_login_req(ap, target);
						}
						break ;														/* FCLNX-GPL-038 */
					}
					if( test_bit( HFC_NEED_CANCEL, (ulong *)&target->status ) )
					{
						if( hfc_issue_relogin(ap, target) )
						{
							set_bit( HFC_NEED_CANCEL, (ulong *)&target->status );
							clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);	/* FCLNX-GPL-038 */
							hfc_enque_login_req(ap, target);			/* 1.107 */
						}
						break;
					}
					if( (test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status ) ) ||
						(dev->lustat & HFC_WAIT_LUN_RESET ) ||		/* FCLNX-GPL-0343 */
						(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ) ) ||
						(test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status ) ) )		/* FCLNX-GPL-038 */
						break ;                                            
					hfc_issue_task_mgm(ap, target, hfcp, lun, HFC_ISSUE_LOGIN) ;		/* FCLNX-0500 */
					/* If initiation succeeds, set HFC_WAIT_HALT in hfc_isuue_task_mgm() */
				}
				if(ap->c_err!=0) break; /* FCLNX-0597 */
				if( (test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status ) ) ||
					(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ) ) ||
					(test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status ) ) )		/* FCLNX-GPL-337 */
					break ;
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );					/* FCLNX-GPL-038 */
			}
			
			break ;

		case HFC_TARGET_RST_TMR:	/* Target Reset Time-Out			*/
			if(target == NULL) break;
//			if((hfc_manage_info.hfcplus_enable)&&(ap->to_errcnt_info!=NULL)){
			/* FCLNX-0506 */
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-GPL-430 */
				if(ap->rt_err_enable){                              /* FCLNX-0506 */
					if(hfc_check_cmnd_timeout(ap,target)){			/* FCLNX-GPL-430 */
						hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_RT_ERR);  /* FCLNX-0506 */
					}
				}
				else{
					if(ap->to_errcnt_info!=NULL)              /* FCLNX-0506 */
						 hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_TO_ERR);
				}
			}
			else if(ap->to_err_limit){	/* FCLNX-GPL-349 */
				hfc_watched_errcount_i(ap, NULL, HFC_TO_ERR);  /* FCLNX-GPL-349 */
			}
			hfc_timeout_by_reset(ap, target, hfcp);
			break ;
			
		case HFC_ELS_TMR:			/* ELS Time_Out						*/
		case HFC_LINKINIT_TMR:		/* Link Initialize Time-Out			*/
			if ( HFC_MMODE_CHECK_SHADOW(ap) ) {							/* FCLNX-GPL-393 */
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				if ( test_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol) ) {	/* FCLNX-GPL-427 */
					hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
					clear_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol);
				}														/* FCLNX-GPL-393 */
			}
			fallthrough;	/* kernel 6.x: suppress -Wimplicit-fallthrough */
		case HFC_MB_TMR:			/* Other MailBox Time-Out			*/
			if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status ) )
			{
				ap->mb_status = 0 ;				/* FCLNX-0402 */
				ap -> mb_resp = HFC_MBR_TIMEOUT;
				hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
				break ;
			}
			ap->mb_status = 0 ;
			ap -> mb_resp = HFC_MBR_TIMEOUT;

			memset(ap->logdata,0,16);					/* FCLNX-GPL-391 */
			ap->logdata[0] = ap->mb->mb_init.command ;	/* FCLNX-GPL-391 */
			ap->logdata[1] = ap->mb->mb_init.sub_cmd ;	/* FCLNX-GPL-391 */
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MBINIT,ERRID_HFCP_EVNT4,0x2a,ap->logdata,16) ;	/* FCLNX-GPL-391 */
            hfc_abend(ap,HFC_ABEND_MB_TOUT);

			break ;
			
		case HFC_CTLRST_DELAY_TMR :											/* FCLNX-0279 */
			/* for FIVE and FIVE-EX */
			
			if( !lock_try_mpap( mpap ) ) {									/* FCLNX-0287 */
				hfc_watchdog_enter( ap,NULL,NULL,0,HFC_CTLRST_DELAY_TMR,1,FALSE);
				break;
			}	
			/* Aapter recovery process from isolation */
			if ( !test_bit ( HFC_ISOL_RECOVERY,   (ulong *)&ap->status) ){
				if ( !test_bit( HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock ) ) {
					HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
					break;
				}
			}
			
			/* Clear GR0-F */ /* FCLNX-GPL-220 */
			if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
			{	/* for only FIVE-EX */
				hfc_reset_start( ap, HFC_GR_CLEAR );
			}
			
			set_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );

			hfc_reset_start(ap, HFC_CTLRST);
			
			if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
			{	/* for only FIVE-EX */
				/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
				dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);
			}
			
			udelay(1000); 	/* 1ms wait */
			
			clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );
			HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-0287 */
			
			hfc_reset_start(ap, HFC_WSCA_CLEAR);
			
			if( HFC_MMODE_CHECK_MLPF(ap) )                                  /* FCLNX-0379 */
				hfc_reset_start(ap, HFC_SET_MLPF_MODE);
			
			hfc_reset_adap_info(ap) ;
			
			hfc_w_stop(ap,HFC_REBOOT_DELAY_TMR) ;
			hfc_w_start(ap,HFC_REBOOT_DELAY_TMR) ;

			/* Aapter recovery process from isolation */
			if ( test_bit ( HFC_ISOL_RECOVERY,   (ulong *)&ap->status) ){

			 /** Clear Interruption register (This process is different from machine check recovery */
				hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_RST,( char )0x4,  0xffffffff);
			}
			
			hfc_reset_start(ap, HFC_REBOOT);
			set_bit(HFC_MCK_SUCCESS, (ulong *)&ap->mck_result );

			
			break;

		case HFC_REBOOT_DELAY_TMR :

			/* Check HW status and POST result (retry count is zero) */
			if( ap->pkg.type == HFC_PKTYPE_FPP )
			{
				if( hfc_config_hw_set(ap,0) )
				{
					/* H/W Initialize failed. */
					hfc_chk_stop( ap, FALSE ) ;											/* FCLNX-0276 */
				}
				else
				{
					/* H/W Initialize succeeded. */
					clear_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status);
					if( test_bit (HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
						if(ap->mck_on_sleep) {
							hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);				/* FCLNX-0296 */
						}
					}
					set_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status);
					hfc_w_stop( ap, HFC_LINKUP2_TMR );									/* FCLNX-0241 */
					hfc_w_start( ap, HFC_LINKUP2_TMR );									/* FCLNX-0241 */
					hfc_inta_mask_set(ap, hfc_inta_mask[ap->pkg.type]) ;
				}
			}
			else /* FIVE, FIVE-EX */
			{
				rc = lock_try_mpap( mpap );	
				
				if( rc == 0 ){
					/* restart HFC_REBOOT_DELAY_TMR (1s) */
					hfc_w_stop( ap, HFC_REBOOT_DELAY_TMR );
					hfc_watchdog_enter( ap,NULL,NULL,0,HFC_REBOOT_DELAY_TMR,1,0);
					break;
				}																		/* FCLNX-0279 */
				
				/* Check POST result (1 time retry) */
				if(ap->pkg.type == HFC_PKTYPE_FIVE)
				{
					rc = hfc_config_hw_set_five(ap,1);
				}
				else /* FIVE-EX */
				{
					rc = hfc_config_hw_set_five_ex(ap,1);
				}
					
				if ( rc )                                     /* FCWIN-0286 */
				{	
//					if ( test_bit ( HFC_ISOL_RECOVERY, (ulong *)&ap->status) ){
 
//						hfc_force_linkdown(ap, FALSE, FALSE);	/* FCLNX-GPL-147 */
//					}else{
					/* H/W Initialize failed. */
					HFC_ISSUE_CSTP( ap, TRUE, HFC_ABEND_FCSTP) ; /* FCLNX-0279 *//* @MLPF *//* FCLNX-GPL-316 */
				}				
				else
				{
					if((HFC_MMODE_CHECK_SHADOW (ap) )&&( test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status  )   )){/* FCLNX-GPL-417 */
						
						hfc_reset_start(ap, HFC_SET_INIADR);
						
						hfc_write_reg(ap, HFC_IOSPACE_CA_FLAG, 0x1, 0x00);
						hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_FW_START);
						hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask_mlpf[ap->pkg.type]);

						set_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status);
						clear_bit( HFC_HWISOL, (ulong *)&mpap->status );
						clear_bit(HFC_ISOL, (ulong *)&ap->status);
						clear_bit ( HFC_ISOL_RECOVERY, (ulong *)&ap->status);
						ap->isol_detail = HFC_NO_ISOLATE;
						ap->c_err = 0x00;

						/* FCLNX-GPL-219 */
						ap->mck_type = 0;
						ap->mck_result = 0;

						clear_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
						clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
#else
						if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) /* FCLNX-GPL-FX-472 */
							clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
#endif	/* FCLNX-GPL-FX-424 */

							if ( hfc_issue_linkini(ap) ) {
							return;
						}
																	/* @MLPF STR */
						hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask_mlpf[ap->pkg.type] );
																	/* @MLPF END */	
						hfc_errlog(
							ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
							ERRID_HFCP_EVNT2, 0xD3, NULL, 0);

						HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is recovered \n",
						ap->pci_cfginf->bus->number,
						PCI_SLOT(ap->pci_cfginf->devfn),
						PCI_FUNC(ap->pci_cfginf->devfn));
					}else{/* FCLNX-GPL-417 */
						/* H/W Initialize succeeded. */
						/* WS Comu Area(0x318) */
						hfc_reset_start(ap, HFC_SET_WS80);
						
						/* INITTBL ADR(0x310)  */
						/* ALPA(0x319)         */
						hfc_reset_start(ap, HFC_SET_INIADR);
						
						/* SET LINK_INI_OPT(0x31f) */
						/* FCLNX-GPL-FX-366 */
						hfc_reset_start(ap, HFC_SET_LINK_INI_OPT);

						hfc_mck_port_recovery(ap);
					
						clear_bit(HFC_MCK_SUCCESS, (ulong *)&ap->mck_result );
						clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock );

						ap->mck_linkup = HFC_LINKUP_MCK; /* FCLNX-0595 */
						ap->isol_err_mck_cnt = (uchar)mpap->mck_err_cnt;		/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */
						if ( test_bit ( HFC_HWISOL, (ulong *)&mpap->status) ){
							clear_bit( HFC_HWISOL, (ulong *)&mpap->status );
							ap->isol_detail = HFC_NO_ISOLATE;
							ap->c_err = 0x00;
							clear_bit( HFC_ISOL,  (ulong *)&ap->status );
							clear_bit( HFC_ISOL_RECOVERY, (ulong *)&ap->status );

							/* FCLNX-GPL-219 */
							ap->mck_type = 0;
							ap->mck_result = 0;

							hfc_errlog(
								ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
								ERRID_HFCP_EVNT2, 0xD3, NULL, 0);

							HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is recovered \n",
								ap->pci_cfginf->bus->number,
								PCI_SLOT(ap->pci_cfginf->devfn),
								PCI_FUNC(ap->pci_cfginf->devfn));
						}
					}
				}

				
				HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
			}
			
			if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
			{
				hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
			}
			
			break ;
			
		case HFC_MCK_DELAY_TMR :
			/* Wait for executing MCK recovery process issued by other port. */
			mpap = ap->mp_adap_info;
			if( test_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status) ){
			}
			else if( test_bit(HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock ) )
			{
				hfc_w_stop(ap,HFC_MCK_DELAY_TMR) ;
				hfc_w_start(ap,HFC_MCK_DELAY_TMR) ;
				break;
			}
			
			/* Clear the state flag of HFC_SUBMCK_RECOVERY  */
			/* This flag means "Now Sub-MCK process is going.". */
			rc = lock_try_mpap( mpap );				/* FCLNX-0493	*/
			if( rc == 0 ) /* FCLNX-GPL-177 */
			{
				/* restart HFC_MCK_DELAY_TMR (3s) */
				hfc_w_stop(ap,HFC_MCK_DELAY_TMR) ;
				hfc_w_start(ap,HFC_MCK_DELAY_TMR) ;
				break;
			}  /* FCLNX-GPL-177 */
			clear_bit(HFC_SUBMCK_RECOVERY, (ulong *)&mpap->status );
			HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-0493	*/
			
			/* Clear "config registar sticky bit". It's for only FIVE-EX */
			if(ap->pkg.type == HFC_PKTYPE_FIVE_EX){
				hfc_clear_sticky_bit(ap);
			}
		
			/* WS Comu Area(0x318) */
			hfc_reset_start(ap, HFC_SET_WS80);
			/* INITTBL ADR(0x310)  */
			/* ALPA(0x319)         */
			hfc_reset_start(ap, HFC_SET_INIADR);
			
			/* SET LINK_INI_OPT(0x31f) */
			/* FCLNX-GPL-FX-366 */
			hfc_reset_start(ap, HFC_SET_LINK_INI_OPT);
			
			set_bit(HFC_MCK_SUCCESS, (ulong *)&ap->mck_result );
			ap->mck_linkup = HFC_LINKUP_MCK;			/* FCLNX-GPL-363 */
			ap->isol_err_mck_cnt = (uchar)mpap->mck_err_cnt;		/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */
			
			hfc_mck_port_recovery( ap );
			clear_bit(HFC_MCK_SUCCESS, (ulong *)&ap->mck_result );
			
			if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
			{
				hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
			}
			break;
			
		case HFC_MCKINT_TMR :															/* FCLNX-0275 */
			
			int_a_rst = hfc_inta_mask[ap->pkg.type];
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_RST,( char )0x4, int_a_rst);

			if( test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) )
			{
				clear_bit( HFC_WAIT_T3, (ulong *)&ap->status );
				if( HFC_MMODE_CHECK_SHADOW(ap) )
					hfc_mlpf_issue_ffcstp(ap, HFC_ABEND_FCSTP);
				else
					hfc_abend(ap, HFC_ABEND_T3);
			}
			else {
				if( HFC_MMODE_CHECK_SHADOW(ap) )
					hfc_mlpf_issue_ffcstp(ap, HFC_ABEND_FCSTP);
				else
					hfc_abend(ap, HFC_ABEND_MCK_INT);
			}
			break;																		/* FCLNX-0275 */
			
// @MLPF
		case HFC_MLPF_FMCK_TMR :
			clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
			
			if( HFC_MMODE_CHECK_SHADOW(ap) )
				hfc_issue_forced_mck(ap, HFC_ABEND_T3);
			
			break;
		
		case HFC_MLPF_MCKEND_TMR :
			clear_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status );
			
			if( HFC_MMODE_CHECK_SHADOW(ap) )
				hfc_mlpf_issue_ffcstp(ap, HFC_ABEND_FCSTP);
			else // Guest LPAR
				hfc_mlpf_cstpend_int(ap);
			
			break;
		
		case HFC_MLPF_FCSTP_TMR :
			clear_bit(HFC_MLPF_WAIT_FCSTP, (ulong *)&ap->status );
			if( HFC_MMODE_CHECK_SHADOW(ap) )
				hfc_mlpf_fcstp_int(ap, HFC_ABEND_FCSTP);
			
			break;

		case HFC_RESTART_TMR :						/* FCLNX-0500 */
			hfc_timeout_by_restart(ap, target, hfcp);
			break;
			
		case HFC_WEXEC_TMR:
			if(target == NULL) break; /* FCLNX-GPL-189 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_WEXEC_TMR, 0, TRUE);
			if( ( target->wx_que_cnt == 0 ) && ( ap->xob_wait_exec_cnt == 0 ) )
				break ;
			if( test_bit( HFC_SCSI_DELAY, (ulong *)&target->status ) )
				break ;
			if( test_bit( HFC_NEED_LOGIN, (ulong *)&target->status ) )
			{
				if( hfc_issue_relogin(ap, target) )
				{
					set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
					clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
					hfc_enque_login_req(ap, target);			/* 1.107 */
				}
			}
			
			if( test_bit( HFC_NEED_CANCEL, (ulong *)&target->status ) )
			{
				if( hfc_issue_relogin(ap, target) )
				{
					set_bit( HFC_NEED_CANCEL, (ulong *)&target->status );
					clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);	/* FCLNX-GPL-038 */
					hfc_enque_login_req(ap, target);			/* 1.107 */
				}
			}
			
			if( ( target->wx_que_cnt > 0 ) || ( ap->xob_wait_exec_cnt > 0 ) )
			{
				hfc_issue_intl_start(ap,target) ;
			}
			break ;
					
		case HFC_LINKUP_TMR :			/* LINK UP waiting timer */
		case HFC_LINKUP2_TMR :			/* LINK UP waiting timer after MCK recovers */ /* FCLNX-241 */
			
			if(w_timer->timer_id == HFC_LINKUP_TMR){ /* FCLNX-GPL-069 */
				/* "HFC_LINKUP_TMR" needs these steps. */
//				if((hfc_manage_info.hfcplus_enable)&&(ap->ldl_errcnt_info!=NULL)){                              /* FCLNX-0506 */
				if(ap->ldl_errcnt_info!=NULL){                              /* FCLNX-GPL-349 */                             /* FCLNX-0506 */
					hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_OCCURED_FAILURE, HFC_LDL_ERR);  /* FCLNX-0506 */
				}
			}
			
			clear_bit(HFC_ONLINE, (ulong *)&ap->status);
			set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status );

			/* Cancel pending SCSI processes under this adapter */
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = hfc_hash_target_info(ap, lp);
				if( target != NULL )
				{
					hfc_cancel_scsi_cmd(ap,target,0, NULL, SCS_LINKUP_TO, HFC_CSCSI_ERROR,	/* FCLNX-0429 */
							TRUE, TRUE, HFC_FLASH_TARGET);
					target->status = HFC_NON_STATUS;
				}
			}
			hfc_wwnverify_linkup_timeout(ap, NULL, 0);
			
			if (hfc_manage_info.npubp->hfc_mp_hsd_enable)									/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_make_fcinfo(ap, NULL, 0x14, SCS_LINKUP_TO, NULL, 0);
			
			if( (test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status)) || (ap->initialize == 1) ) 	/* FCLNX-0279 */
			{
				HFC_DBGPRT(KERN_ERR "hfcldd%d : hfc_watchdog() lock fail. \n", ap->dev_minor);
				unlock_mailbox( ap ) ; 
				hfc_wake_up(&ap->init_event,&ap->int_a_poll);
				clear_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status);
			}																						/* FCLNX-0279 */
			break ;
			
		case HFC_SCN_LINKUP_TMR :
			hfc_timeout_by_scnlinkup(ap, target);
			break;
		
		case HFC_WLINKUP_CNT_TMR :	/* FCLNX-GPL-FX-424 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
				clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
#endif	/* FCLNX-GPL-FX-424 */
			break;
		
		case HFC_DIAG_DELAY_TMR:			/* Delayed Time-Out when diag process completes	*/
			if( test_bit(HFC_DIAG_DELAY, (ulong *)&ap->status ) )
			{
				hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);										/* FCLNX-0296 */
				break ;
			}
			break ;
			
		case HFC_MPAP_LOCK_TMR:				/* Wait for acquiring lock in MP_ADAP_INFO	*/
			if( test_bit( HFC_LOCK_WAIT_1, (ulong *)&ap->mpap_lock ) )
			{
				hfc_mck_recovery_five( ap, ap->mck_type );
			}
			if( test_bit( HFC_LOCK_WAIT_2, (ulong *)&ap->mpap_lock ) )
			{
				hfc_chk_stop( ap, FALSE );															/* FCLNX-0279 */
			}
/*			if( test_bit( HFC_LOCK_WAIT_3, (ulong *)&ap->mpap_lock ) ) */
/*			{ */
/*				hfc_fpp_logout( ap, ap->err_no, ap->mode ); */
/*			} */
/*			if( (test_bit( HFC_LOCK_WAIT_4, (ulong *)&ap->mpap_lock )) || */
/*						(test_bit( HFC_LOCK_WAIT_5, (ulong *)&ap->mpap_lock ))	 ) */
/*			{ */
/*				hfc_five_logout( ap, ap->err_no, ap->mode ); */
/*			} */ /* FCLNX-GPL-111 */
			if( test_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock ) )     // @MLPF
			{
				if( HFC_MMODE_CHECK_SHADOW(ap) )
					hfc_mlpf_errlog_slpar( ap );
				else if( HFC_MMODE_CHECK_SHARED(ap) && !( HFC_MMODE_CHECK_SHADOW(ap) ) )
					hfc_mlpf_errlog_glpar( ap );
			}
			if( test_bit( HFC_LOCK_WAIT_7, (ulong *)&ap->mpap_lock ) )     /* FCLNX-GPL-0576 */ /* FCLNX-GPL-068 */
			{
				hfc_mck_recovery_fpp( ap, ap->mck_type );
			}
/*			if( test_bit( HFC_LOCK_WAIT_8, (ulong *)&ap->mpap_lock ) ) */
/*			{ */
/*				hfc_five_ex_logout( ap, ap->err_no, ap->mode ); */
/*			} */ /* FCLNX-GPL-111 */
			
			break;
			
		case HFC_LOGIN_DELAY_TMR:		/* Delayed Time-Out when LOGIN process completes	*/ 	/* FCLNX-0243 */
			clear_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status );
			
			if (ap->initialize){											/* FCLNX-0270 */
				if( ap->no_target == 1){									/* FCLNX-GPL-570 */
					ap->no_target = 0;
					hfc_wake_up(&ap->init_event,&ap->int_a_poll);
				}															/* FCLNX-GPL-570 */
				start_next_mailbox( ap );									/* FCLNX-0270 */
			}
			else {															/* FCLNX-0270 */
				if( !(test_bit(HFC_ONLINE, (ulong *)&ap -> status ) ) )
					break;
				
				if ( (ap->connect_type == HFC_SWITCH )
					|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00)) ) {
					if (!test_bit(HFC_WAIT_NMSRV, (ulong *)&ap->status) ) {
						set_bit(HFC_NEED_NMSRV, (ulong *)&ap->status);
//						hfc_issue_gidft( ap );								/* FCLNX-GPL-038 */
						start_next_mailbox( ap );							/* FCLNX-GPL-038 */
					}
				}
				else {
					start_next_mailbox( ap );								/* FCLNX-0279 */
				}
			}																/* FCLNX-0270 */
			break;
		case HFC_LDLERR_TMR:	/* Long Link Down Error count timer time-out*/
			hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_WATCHERR_TMR_TO, HFC_LDL_ERR);		/* FCLNX-0506 */
			break;		/* FCLNX-0506 */
		case HFC_LDSERR_TMR:	/* Short Link Down Error count timer time-out*/
			hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_WATCHERR_TMR_TO, HFC_LDS_ERR);		/* FCLNX-0506 */
			break;		/* FCLNX-0506 */
		case HFC_IFERR_TMR:	/* Interface Error count timer time-out*/
			hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_WATCHERR_TMR_TO, HFC_IF_ERR);			/* FCLNX-0506 */
                        break;		/* FCLNX-0506 */
		case HFC_TOERR_TMR:	/* SCSI T.O Error count timer time-out*/
			hfc_manage_info.npubp->hfc_watched_errcount(ap, NULL, HFC_WATCHERR_TMR_TO, HFC_TO_ERR);			/* FCLNX-0506 */
                        break;		/* FCLNX-0506 */
		case HFC_TGT_LDLERR_TMR:	/* Long Link Down Error count timer time-out between FCSW and Disk Array */
			if(target == NULL) break;
			hfc_manage_info.npubp->hfc_watched_errcount(ap, target, HFC_WATCHERR_TMR_TO, HFC_TGT_LDL_ERR);		/* FCLNX-GPL-327 */
			break;		/* FCLNX-GPL-327 */
		case HFC_TGT_LDSERR_TMR:	/* Short Link Down Error count timer time-out between FCSW and Disk Array */
			if(target == NULL) break;
			hfc_manage_info.npubp->hfc_watched_errcount(ap, target, HFC_WATCHERR_TMR_TO, HFC_TGT_LDS_ERR);		/* FCLNX-GPL-327 */
			break;		/* FCLNX-GPL-327 */

		case HFC_DELAY_TMR_DEV:			/* DELAY timer after executing LUN RESET	*/
			if(target == NULL) break;	/* FCLNX-GPL-189 */	/* FCLNX-GPL-038 */
			if(dev == NULL) break;		/* FCLNX-GPL-189 */	/* FCLNX-GPL-038 */
//			hfc_manage_info.npubp->hfc_clear_dev_info( dev );	/* FCLNX-GPL-0343 */
			hfc_clear_dev_info( dev );							/* FCLNX-GPL-0343 */

			if( test_bit( HFC_SCSI_DELAY, (ulong *)&target->status ) )
				break;
			if( (test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status) ) && (test_bit(HFC_ONLINE, (ulong *)&ap->status) ) )
			{
				/* If adapter is waiting LINK_UP, I/O start can not be executed. */
				break;
			}
			else if( !(test_bit(HFC_ONLINE, (ulong *)&ap->status) ) )
			{
				/* 
				 * If adapter is not online, target should have been already canceled.
				 * Break this process 
				 */
				break ;
			}
			else if( !(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status) )  && (test_bit(HFC_ONLINE, (ulong *)&ap->status) ) )
			{
				/* Restart waiting request if adapter is ONLINE */
				if(!(test_bit(HFC_SCN_WLINKUP, (ulong *)&target->status) )  
				&& test_bit(HFC_NEED_LOGIN, (ulong *)&target->status)  )			/* FCLNX-GPL-038 */
				{
					if( hfc_issue_relogin(ap, target) )
					{
						/* Initiated LOGIN but failed with mailbox busy */
						set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );			/* FCLNX-GPL-038 */
						clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );		/* FCLNX-GPL-038 */
						hfc_enque_login_req(ap, target);
					}
					break;
				}
				else if( test_bit(HFC_NEED_CANCEL, (ulong *)&target->status) )  	/* FCLNX-GPL-038 */
				{
					if( hfc_issue_relogin(ap, target) )
					{
						/* Initiated CANCEL LOGIN but failed with mailbox busy */
						set_bit(HFC_NEED_CANCEL, (ulong *)&target->status );		/* FCLNX-GPL-038 */
						clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status );		/* FCLNX-GPL-038 */
						hfc_enque_login_req(ap, target);
					}
					break;
				}																	/* FCLNX-GPL-038 */
				if( ( target->wx_que_cnt > 0 ) || ( ap->xob_wait_exec_cnt > 0 )){					/*-- FCLNX-019 STR--*/
					hfc_issue_intl_start(ap,target) ;
				}												/*-- FCLNX-019 END--*/
			}
			break ;
			
		case HFC_ISOLATE_DELAY_TMR:
			hfc_force_linkdown(ap, FALSE, TRUE);
			if( HFC_MMODE_CHECK_SHADOW(ap) ){
				hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_END);		/* FCLNX-GPL-393 */
				clear_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol);
				clear_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol);
			}
			break;
			
		case HFC_INT_CHECK_TMR: /* FCLNX-GPL-306 */
			HFC_DBGPRT( "hfcldd : hfc_watchdog - HFC_INT_CHECK_TMR is Time-out.");
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			/* Check interrupts */
			hfc_intr(0, (void *)ap);
			HFC_ADAPLOCK_IRQSAVE(flags);

			if(ap->int_check == TRUE)
			{	/* timer restart */
				HFC_DBGPRT( "hfcldd : hfc_watchdog - Restore HFC_INT_CHECK_TMR.");
				hfc_w_stop(ap, HFC_INT_CHECK_TMR);
				hfc_w_start(ap, HFC_INT_CHECK_TMR);
			}
			else
			{	/* timer stop */
				HFC_DBGPRT( "hfcldd : hfc_watchdog - Stop HFC_INT_CHECK_TMR.");
				hfc_w_stop(ap, HFC_INT_CHECK_TMR);
			}
			break;
		case HFC_MLPF_ISOLEND_TMR:
			if ( (HFC_MMODE_CHECK_SHARED(ap)) && (!HFC_MMODE_CHECK_SHADOW(ap))) {
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				hfc_mlpf_isol_end_glpar(ap, hyp_status);
			}
			break;
	}	/*-- end of switch --*/

//	if(hfc_manage_info.hfcplus_enable){
//
//		hfc_manage_info.npubp->hfc_check_errcount(ap);
//		/* FCLNX-0506 */
//	}
	hfc_check_errcount(ap);	/* FCLNX-GPL-349 */

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info )
	{
		if ( ap->retry_hfcp_top )
			hfc_manage_info.npubp->hfc_retry_strategy(ap);
	
		if ( hfc_manage_info.wait_reset_mp )								/* FCLNX-0429 */
		{
			hfc_manage_info.npubp->hfc_check_dev_reset_complete();			/* FCLNX-0429 */
			hfc_manage_info.npubp->hfc_check_bus_reset_complete();			/* FCLNX-0429 */
		}
	}

	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	return;
	
}


/*
 * Function:    hfc_timeout_by_reset
 *
 * Purpose:     Responce Time out of Target Reset in Bus Reset
 *
 * Arguments:   
 *  ap         - Pointer to dap_info 
 *  target     - Pointer to arget_info 
 *  hfcp       - Pointer to hfc_pkt 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_timeout_by_reset(												/* FCLNX-0500 */
	struct adap_info        *ap,
	struct target_info      *target,
	struct hfc_pkt     		*hfcp)
{
	int	   lun=0;
	
	if(hfcp != NULL)		/* FCLNX-0608 */
	{
		lun = hfcp->lun_id;
	}						/* FCLNX-0608 */
									
	memset( ap->logdata, 0, 16 );			/* FCLNX-0608 *//* FCLNX-GPL-391 */
	memcpy( ap->logdata, (uchar*)&ap->xob[hfcp->cmd_xob].fcp_cmd.fcp_cntl, 4 );	/* FCLNX-GPL-391 */
	hfc_errlog( ap, target, hfcp, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRA, 0x29, ap->logdata, 16 );	/* FCLNX-GPL-391 */

	clear_bit( HFC_WAIT_TARGET_RESET, (ulong *)&target->status );

	if( hfc_toutchk_xob(ap, target, hfcp, lun, HFC_ISSUE_TARGET_RESET) )
	{
		/*
		 * Timed-out xob remain in queue so do not initiate
		 * MIH-LOG and Abort-Task-Set
		 */
		return ;
	}

	if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has happen. */
		HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
	}
	else /* FCLNX-0608 */
	{	
		if(ap->c_err!=0) return;
		/* Failed to initiate mailbox request       */
		/* Execute Abort without collecting MIH-LOG */
		if( test_bit(HFC_NEED_CANCEL, (ulong *)&target->status ) ){
			set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );
			clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );	/* FCLNX-GPL-038 */
		}
		if( test_bit(HFC_NEED_LOGIN, (ulong *)&target->status ) )
		{
			if( hfc_issue_relogin(ap,target) )
			{
				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status );
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status );	/* FCLNX-GPL-038 */
				hfc_enque_login_req(ap, target);
			}
			return ;
		}
		if(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ))  return ;                                            
		hfc_issue_task_mgm(ap, target, hfcp, lun, HFC_ISSUE_LOGIN) ;
		/* If initiation succeeds, set HFC_WAIT_HALT in hfc_isuue_task_mgm() */
		if(ap->c_err!=0) return; 
	}			/* FCLNX-0608 */
	return;
}

/*
 * Function:    hfc_occurred_mck
 *
 * Purpose:      occured Machin Check
 *
 * Arguments:
 *  ap         - Pointer to adap_info structure
 *  event          - cause of MCK
 *
 * Returns:
 *
 * Notes:
 */
void hfc_occurred_mck(struct adap_info *ap, uchar point)
{        /* FCLNX-0533 */
	HFC_DBGPRT("hfc_mck_point() : Machine check occurred  hfcldd%d (",ap->instance);
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
//      hfc_issue_forced_mck(ap, HFC_ABEND_T3);

        hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDCTL,( char )0x1, 0x08 );

        return;
}       /* FCLNX-0533 */

/*
 * Function:    hfc_reset_start
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  type       - 1:ctlrst , 2:reboot , 3:f_start, 4:fw_start
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_reset_start(struct adap_info *ap,uchar type)
{
	uint			wk1,wk2 ;
	uint			length ;  
	unsigned long long	wkp;
	int				dma_size;
	uint			wk_reg; /* FCLNX-GPL-220 */

	HFC_DBGPRT(" hfcldd : hfc_reset_start - start\n");

	switch( type )
	{
		case HFC_CTLRST : /* FCLNX-GPL-220 start */
			if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
			{
				if( (ap->pkg.lsi_rev == 0x00) || (ap->pkg.lsi_rev == 0x01) )
				{	/* FIVE-EX Pass1 and Pass 2 */
					hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x10); /* PONRES */
				}
				else
				{	/* FIVE-EX Pass2.1 and others */
					hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x02); /* CTLRES */
				}
			}
			else
			{	/* FPP, FIVE and others */
				hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x02); /* CTLRES */
			}
			break ; /* FCLNX-GPL-220 end */
			
		case HFC_REBOOT :
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDBOOT,( char )0x1,(char)0x20);

			if(ap->pkg.type == HFC_PKTYPE_FIVE_EX) /* FEX-Prob 217 */ /* FCLNX-GPL-94 */
			{
				/* Wait 100ms */
				mdelay(100);
				/* Function start & start trace */
				hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, 0x1, 0xa0);
			}

			break ;
		case HFC_F_START :
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDCTL,( char )0x1,(char)0x80);
			break ;
		case HFC_SET_INIADR :
			wk1 = 0;
			wk2 = (uint)ap->padr_init ;
			/* 
			 * Reboot with AL_PA=0 in MCK recovery process before link initialization 
			 *  -- always set pref_alpa. 
			 */
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_CA_RSTINFO+1,( char )0x1,(char)ap->pref_alpa);
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_CA_LNKSPD,( char )0x1,(char)ap->linkspeed); 
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_CA_CNTTYP,( char )0x1,(char)ap->topology); 

			if( test_bit(HFC_SUPPORT_LINKINI_DELAY, (ulong *)&ap->fw_support) ){	/* FCLNX-GPL-570 */
				hfc_write_reg(ap, HFC_IOSPACE_LINK_INI_OPT, 0x1, HFC_DISABLE_LINKINI_DELAY); 
			}																		/* FCLNX-GPL-570 */
			
// @MLPF
			if( HFC_MMODE_CHECK_SHARED (ap) )
				hfc_write_reg_ext(ap, 0x0308, (char)0x1, 0x80);
			else if( HFC_MMODE_CHECK_DEDICATE(ap) )
				hfc_write_reg_ext(ap, 0x0308, (char)0x1, 0x40);
// @MLPF
			dma_size = sizeof(dma_addr_t);
			wkp = ap->padr_init;
			if(dma_size == 8){
				wkp >>=32;
			}
			else{
				wkp = 0;
			}
			hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR0, 0x4, wkp);
			hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR1, 0x4, ap->padr_init);

			if ( ap->pkg.type != HFC_PKTYPE_FPP )										/* FCWIN-0200 */
				hfc_write_reg(ap,( uint )HFC_IOSPACE_CA_PORTNO,( char )0x1,ap->port_no);/* FCWIN-0200 */

			break ;
		case HFC_SET_WS80 :
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x80);		
			break ;
		case HFC_SET_WS40 :
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x40);		
			break ;
		case HFC_SET_WS04 :    
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x04);		
			break ;
		case HFC_FW_START :
			if ( HFC_MMODE_CHECK_SHARED(ap) )
			{
				hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA, 0x4,(int)HFC_FRAMEA_FW_START_MCK);
			}
			else
			{
				if(!test_bit(HFC_ISOL, (ulong *)&ap->status))	/* FCLNX-0639 */
					hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA, 0x4,(int)0x40000000);
			}
			break ;
		case HFC_INI_RESET :
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDRES,( char )0x1,(char)0x04);
			/* 1ms dummy wait after iniRes */
			wk1 = 0;
			udelay(1000); /* 1ms wait */
			break ;
		case HFC_WSCA_CLEAR :
			/* clear 256 bytes from PCI address 0x300 */
			wk2 = ap->pkg.map->iosp.reg[HFC_IOSPACE_CA_POSTRESULT] ;

			if(ap->pkg.type == HFC_PKTYPE_FPP)
			{
				length = 256;
				for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )
				{
					hfc_write_reg_ext(ap,( uint )wk1,( char )0x4,0); /* FCLNX-401 */
				}
			}
			else /* FIVE, FIVE-EX */
			{
				length = 128;
				for(wk1= wk2 ; wk1 < wk2+length ; wk1+=4 )                    
				{
					if( wk1 == 0x0308 ){ continue; }				/* FCLNX-GPL-309 */
					if( wk1 == 0x0330 ){ continue; }
					if( wk1 == 0x0334 ){ continue; }
					if( wk1 == 0x0338 ){ continue; }
					if( wk1 == 0x033C ){ continue; }
					
					hfc_write_reg_ext(ap,( uint )wk1,( char )0x4,0);
				}
			}
			break;
			
		case HFC_SET_MLPF_MODE :                                            /* FCLNX-0379 */
			if( HFC_MMODE_CHECK_SHARED (ap) )
				hfc_write_reg_ext(ap, 0x0308, (char)0x1, 0x80);
			else if( HFC_MMODE_CHECK_DEDICATE(ap) )
				hfc_write_reg_ext(ap, 0x0308, (char)0x1, 0x40);
			break;
			
		case HFC_GR_CLEAR : /* FCLNX-GPL-220 */
			/* Set ER'PTYP */ /* FCLNX-GPL-231 */
			if( !(HFC_MMODE_CHECK_SHARED (ap)) ){	/* FCLNX-GPL-399 */
				hfc_write_reg_ext(ap, 0x110, (char)0x02, 0x0404);
			}else{
				hfc_write_reg_ext(ap, 0x110, (char)0x01, 0x04);
				hfc_write_reg_ext(ap, 0x111, (char)0x01, 0x04);
			}										/* FCLNX-GPL-399 */
			
			/* Fill all standing bits, from 0x1040 to 0x107f (64byte) */
			for( wk_reg = 0x1040 ; wk_reg < 0x1080 ; wk_reg+=0x4 )
			{
				hfc_write_reg_ext(ap, (uint)wk_reg, 0x4, (uint)0xffffffff); /* FCLNX-GPL-254 */
			}
			break;
		case HFC_SET_LINK_INI_OPT :
			/* FCLNX-GPL-FX-366 */
			
			if ( !HFC_MMODE_CHECK_SHARED(ap) || (HFC_MMODE_CHECK_SHADOW(ap) ) )
				break;
			
			if ((ap->drvctl & HFC_LOGIN_TARGET_FILTER_EXT)  &&
				( (uchar)hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1) & 0x10)) {
				/* Invalidate PortID Guard */
				wk_reg = 0x40;
			}
			else {
				/* PortID Guard */
				wk_reg = 0;
			}
			hfc_write_reg(ap, HFC_IOSPACE_LINK_INI_OPT, 1, (char)wk_reg);
			break;
	}
}


/*
 * Function:    hfc_issue_forced_mck
 *
 * Purpose:     Issue forced MCK and initiate timer of all ports for supervision.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *  = 0        - Forced MCK initiation succeeded 
 *  !=0        - Forced MCK initiation failed 
 *
 * Notes:       Lock mpap->lock<HFC_MLOCK_MCKTMR_ENTRY to initiate timer. 
 */
int hfc_issue_forced_mck(struct adap_info *ap, uchar type)					/* FCLNX-0279 */
{
	struct mp_adap_info		*mpap = ap->mp_adap_info;
	uint				wk_reg;

	if( !test_bit( HFC_WAIT_T3, (ulong *)&ap->status )
	 && !test_bit( HFC_HWCHKSTOP, (ulong *)&mpap->status ) )
	{
		if ( test_bit(HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock) || test_bit(HFC_SUBMCK_RECOVERY, (ulong *)&mpap->status)  ) {	/* FCLNX-GPL-034 */
			/* skip issue forced mck */
			HFC_DBGPRT(" hfc_issue_forced_mck() - skip issue forced mck\n");
		}																														/* FCLNX-GPL-034 */
		else {
			set_bit( HFC_WAIT_T3, (ulong *)&ap->status );
			
			hfc_w_stop(ap,HFC_MCKINT_TMR) ;
			hfc_w_start(ap,HFC_MCKINT_TMR) ;

			wk_reg = 0;
			wk_reg |= type & 0xff;
			wk_reg <<= 16;
			wk_reg |= ap->mb->mb_init.command & 0xff;
			wk_reg <<= 8;
			wk_reg |= ap->mb->mb_init.sub_cmd & 0xff;
			hfc_write_reg(ap,( uint )HFC_IOSPACE_DRV_USED0,( char )0x4, wk_reg);

			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDCTL,( char )0x1, 0x08 );
		}
	}
	return (1);
}																			/* FCLNX-0279 */


/*
 * Function:    hfc_reset_adap_info
 *
 * Purpose:     Initiate adap_info
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_reset_adap_info(
	struct adap_info        *ap)
{
	uint                    lp;

	HFC_DBGPRT(" hfcldd : hfc_reset_adap_info - start\n");

	
	for(lp=0 ; lp<ap->xob_max ; lp++)       /*--    FCWIN-0071      --*/
	{
		ap->xob[lp].flag = 0 ;
		ap->xob[lp].skip = 0 ;
	}
	ap->fw_init_p->xob_inp = 0 ;
	ap->fw_init_p->xob_outp = 0 ;
	
	for(lp=0 ; lp<ap->xrb_max ; lp++)       /*--    FCWIN-0071      --*/
	{
		ap->xrb[lp].flag = 0 ;
		ap->xrb[lp].skip = 0 ;				/*--    FCLNX-GPL-038   --*/
	}
	ap->fw_init_p->xrb_inp = 0 ;
	ap->fw_init_p->xrb_outp = 0 ;
	
	ap->mb_retry_cnt = 0 ;
	ap->xob_no = 0 ;
	ap->xrb_no = 0 ;
	ap->iov_no = 0 ;
//	ap->iov_map_cnt = 0 ;					/*--	LINUX-049		--*/
	ap->xob_exec_cnt = 0 ;
	ap->xob_wait_exec_cnt = 0 ;
	
	for(lp=0 ; lp<MAX_FRAME_CNT ; lp++)
		ap->save_xob_outp[lp] = 0 ;
	ap->frame_inp = 0 ;
	ap->frame_chkp = 0 ;
	
	ap->next_tstart = NULL ;
	ap->login_target = NULL ;               /*--    FCLNX-GPL-038      --*/
	ap->next_gidpn = FALSE ;                /*--    FCLNX-GPL-038      --*/
	clear_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status );			/* FCLNX-GPL-038 */
	clear_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status );		/* FCLNX-GPL-038 */
	clear_bit(HFC_NEED_NMSRV, (ulong *)&ap->status );			/* FCLNX-GPL-038 */
	clear_bit(HFC_NEED_GPNID, (ulong *)&ap->status );			/* FCLNX-GPL-038 */
	clear_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status );		/* FCLNX-GPL-038 */
	clear_bit(HFC_WAIT_NMSRV, (ulong *)&ap->status );			/* FCLNX-GPL-038 */
	clear_bit(HFC_WAIT_GPNID, (ulong *)&ap->status );			/* FCLNX-GPL-038 */
	clear_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status );			/* FCLNX-GPL-038 */
	clear_bit(HFC_WAIT_MIHLOG, (ulong *)&ap->status);			/* FCLNX-GPL-351 */

	return;
}


/*
 * Function:    hfc_timeout_by_scnlinkup
 *
 * Purpose:     Process when HFC_LINKUP_TMR is timed out.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *  target     - Pointer to target_info 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_timeout_by_scnlinkup(
	struct adap_info        *ap,
	struct target_info      *target)
{
	HFC_DBGPRT( "hfc_timeout_by_scnlinkup() - timeout.");

//	if((hfc_manage_info.hfcplus_enable)&&(ap->ldl_errcnt_info!=NULL)){	/* FCLNX-GPL-327 */
	if(ap->ldl_errcnt_info!=NULL){	/* FCLNX-GPL-349 */
		hfc_manage_info.npubp->hfc_watched_errcount(ap, target, HFC_OCCURED_FAILURE, HFC_TGT_LDL_ERR);
	}																	/* FCLNX-GPL-327 */

	target->link_recovered = 1;											/* FCLNX-GPL-334 */

#if !( defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
	clear_bit(HFC_SCN_WLINKUP, (ulong *)&target -> status );
#else
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_SCN_WLINKUP, (ulong *)&target -> status );
#endif

	/* Discards XRB when SCN_WLINKUP timer timed out 	*/

	hfc_cancel_scsi_cmd(ap, target, 0, NULL, SCS_SCN_LINKDOWN, 		/* FCLNX-GPL-038 */
			HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);			/* FCLNX-GPL-038 */

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	clear_bit(HFC_SCN_WLINKUP, (ulong *)&target -> status );
#endif
	/* Invalidates this target device					*/

	hfc_clear_target_info( ap, target, TRUE, TRUE );				/* FCLNX-GPL-038 */
}


/*
 * Function:    hfc_timeout_by_restart
 *
 * Purpose:     The task management is reactivated
 *
 * Arguments:   
 *  ap         - Pointer to adap_info structure
 *  target     - 
 *  hfcp       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_timeout_by_restart(								/* FCLNX-0500 */
	struct adap_info        *ap,
	struct target_info 		*target,
	struct hfc_pkt			*hfcp)
{
	uint                    find=FALSE;
	
	if(target != NULL)
	{
		if( test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset ) ){		/* FCLNX-GPL-036 */
			/* 
			 * Target Reset is requested but MIHLOG is not requested  
			 *  --> Issue Target Reset
			 */
			hfc_issue_task_mgm(ap, target, hfcp, 0, HFC_ISSUE_TARGET_RESET);		/* FCLNX-GPL-036 *//* FCLNX-GPL-328 */
		}
		if ( test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset) )		/* FCLNX-GPL-036 */
			find = TRUE;
	}

	if (find == TRUE){
		hfc_watchdog_enter(ap, target, hfcp, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);	/* FCLNX-GPL-036 *//* FCLNX-GPL-328 */
		hfc_watchdog_enter(ap, target, hfcp, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);/* FCLNX-GPL-036 *//* FCLNX-GPL-328 */
	}
}


/*
 * Function:    hfc_reset_watchdog
 *
 * Purpose:     Release all executing timers
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_reset_watchdog(struct adap_info *ap)
{
	uint                            lp;
	struct target_info              *target;
	struct dev_info					*dev;				/* FCLNX-GPL-038	*/
	
	HFC_DBGPRT("hfcldd%d : hfc_reset_watchdog\n",ap->dev_minor);

	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_ELS_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_REBOOT_DELAY_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN0_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN1_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN2_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN3_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN4_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN5_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN6_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LUN7_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LINKUP_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_LOGIN_DELAY_TMR, 0, TRUE);				/* FCLNX-GPL-038 */
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_ISOLATE_DELAY_TMR, 0, TRUE);				/* FCLNX-GPL-038 */
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_MLPF_MCKEND_TMR, 0, TRUE);				/* FCLNX-GPL-317 */
	hfc_watchdog_enter(ap, 0, NULL, 0, HFC_MLPF_ISOLEND_TMR, 0, TRUE);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		hfc_watchdog_enter(ap, 0, NULL, 0, HFC_WLINKUP_CNT_TMR, 0, TRUE);
#endif	/* FCLNX-GPL-FX-424 */

	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)			/* FCWIN-0083 */
	{
		target = hfc_hash_target_info(ap, lp);
		if (target != NULL)
		{
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, TRUE);
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_DELAY_TMR, 0, TRUE);
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_WEXEC_TMR, 0, TRUE);
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, 0, TRUE);		/* FCLNX-GPL-328 */
			dev = target->dev;										/* FCLNX-GPL-038	*/
			if(dev != NULL) {
				/* stop LUN Reset Delay Timer */
//				hfc_manage_info.npubp->hfc_all_clear_dev_info( ap, dev );			/* FCLNX-GPL-0343 */
				hfc_all_clear_dev_info( ap, dev );									/* FCLNX-GPL-0343 */
			}														/* FCLNX-GPL-038	*/
		}
	}
}


/*
 * Function:    hfc_toutchk_xob 
 *
 * Purpose:     Search timed-out requests remain in xob.
 *              If so, force FW to forced MCK status 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  hfcp       - Pointer to hfc_pkt
 *  lun        - lun number
 *  mode       - 
 *
 * Returns:     
 *  0          - No timed-out requests remain in xobs
 *  -1         - One or more timed-out requests remain in xobs.
 *
 * Notes:       
 */
int hfc_toutchk_xob(
	struct adap_info		*ap,
	struct target_info		*target,
	struct hfc_pkt			*hfcp,
	uint					lun,			/* FCLNX-GPL-0343 */
	uchar					mode)
{
	uint                        outp_num ;
	uint                        save_outp_num;
	uint                        inp_num ;
	struct xob                  *cancel_xob ;
	uint                        save_xob_outp ;	
	uint                        save_xob_inp ;
	uint                        wk_save_xob_outp ;
	uint                        wk_save_xob_inp ;
	
	
	HFC_DBGPRT("hfcldd%d : hfc_toutchk_xob start", ap->dev_minor);
	
	wk_save_xob_inp = ap->fw_init_p->xob_inp ;
	wk_save_xob_outp = ap->fw_init_p->xob_outp ;
	
	HFC_4L_TO_4B(save_xob_inp, wk_save_xob_inp);
	HFC_4L_TO_4B(save_xob_outp, wk_save_xob_outp);
	if( save_xob_outp == save_xob_inp )
	{
		/* xob empty */
		return(0) ;
	}

	HFC_DBGPRT("hfcldd%d : hfc_toutchk_xob xob non empty", ap->dev_minor);

	/* Find sequential number of xob_inp and xob_outp */

	inp_num = ((save_xob_inp & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	inp_num += (save_xob_inp & 0x0000ffff) ;
	save_outp_num = ((save_xob_outp & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	save_outp_num += (save_xob_outp & 0x0000ffff) ;

	cancel_xob = ap->xob ;	/* Xob top address */
			
	if(hfcp != NULL)
	{
		if ( (mode == HFC_ISSUE_ABORT)
		  && (hfcp->lun_id != lun) ) {
			return(0);
		}

		HFC_DBGPRT("hfcldd%d : hfc_toutchk_xob find timeout", ap->dev_minor);

		outp_num = save_outp_num;
		do {
			if( hfcp == (struct hfc_pkt *)(ulong)cancel_xob[outp_num].drv_work.hfc_pkt )
			{
//				if( (!(cancel_xob[outp_num].skip & HFC_XOB_SKIP)) && ( (hfcp->cmd_pkt->timeout_per_command/HZ) > 10 ) ){
				if( !(cancel_xob[outp_num].skip & HFC_XOB_SKIP) ){
					/* Timed-out requests remains in xobs */
					HFC_DBGPRT("hfcldd%d : hfc_toutchk_xob timeout pkt results in xob", ap->dev_minor);

					memset(ap->logdata,0,16);							/* FCLNX-GPL-391 */
					HFC_MEMCPY(ap->logdata, (uchar*)&outp_num, 4);		/* FCLNX-GPL-391 */
					HFC_MEMCPY(&ap->logdata[4], (uchar*)&inp_num, 4) ;	/* FCLNX-GPL-391 */
					hfc_errlog(
						ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4,
						0x8d, ap->logdata, 16) ;						/* FCLNX-GPL-391 */
					hfc_abend(ap,HFC_ABEND_TOUTCHK_XOB);
					return(-1) ;
				}
			}

			outp_num++ ;
			if( outp_num >= ap->xob_max )
				outp_num = 0 ;
		}while( outp_num != inp_num ) ;
	}

	HFC_DBGPRT("hfcldd%d : hfc_toutchk_xob end", ap->dev_minor);

	return(0);
}

int hfc_force_linkdown( struct adap_info *ap, uchar proc, uchar skip)				/* FCLNX-GPL-147 */
{
	struct target_info		*target=NULL ;
	struct mp_adap_info		*mpap;
	uint					lp,i;									/* FCLNX-GPL-428 */
	uint					ctlres_flag = 0;
//	uchar					logdata[16],eno,support ;	/* FCLNX-GPL-391 */
	uchar					eno;
	
	mpap = ap->mp_adap_info;

	HFC_ENTRY("hfc_force_linkdown");                 //FCLNX-0488
	hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x00, ap, NULL, NULL, proc, skip, 0);

	if (skip != TRUE) {
		
		if( test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) ) {		/* FCLNX-GPL-352 */
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x04, ap, NULL, NULL, 0, 0, 0);
			return EINVAL;
		}
		
		if ( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ) ) {
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x02, ap, NULL, NULL, 0, 0, 0);
			return EINVAL;
		}

		if (test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)) {	/* FCLNX-GPL-393 */
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x03, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-0707 */
			return EINVAL;
		}
		
		if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
		{	/* "PCI BUS ERR" has happen. */
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x09, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-0707 */
			HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
			return EINVAL;
		}
		
		if (proc == TRUE) {
			if (test_bit(HFC_ISOL, (ulong *)&ap->status)) {
				if ((ap->isol_detail == HFC_ISOLATE_PORT_C)
				 || (ap->isol_detail == HFC_ISOLATE_CHKSTP_C)) {
					hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x05, ap, NULL, NULL, 0, 0, 0);
					return 0;
				}
			}
		}
		else {
			if (HFC_MMODE_CHECK_SHARED(ap)) {	/* FCLNX-GPL-421 */
				if (test_bit(HFC_ISOL, (ulong *)&ap->status)) {
					hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 *//* FCLNX-GPL-435 */
					if (ap->isol_detail == HFC_ISOLATE_PORT_C){
						eno = (ap->pkg.port <= 1) ? 0x8E : 0xD4;
						hfc_errlog(
							ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
							ERRID_HFCP_EVNT2, eno, ap->logdata, 16);

						HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
							ap->pci_cfginf->bus->number,
							PCI_SLOT(ap->pci_cfginf->devfn),
							PCI_FUNC(ap->pci_cfginf->devfn));
					}
					return EINVAL;
				}
			}								/* FCLNX-GPL-421 */

			if ((ap->pkg.port > 1) && (!(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support)))) {	/* FCLNX-GPL-393 */
				hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x06, ap, NULL, NULL, 0, 0, 0);
				return(EINVAL);
			}
			
			/* This adapter has already isolated */
			if (test_bit ( HFC_HWISOL, (ulong *)&mpap->status)) {
				hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x07, ap, NULL, NULL, 0, 0, 0);
				return 0;
			}
			
			/* This port has already isolated, but the adapter has not isolated yet */
			if (test_bit ( HFC_ISOL, (ulong *)&ap->status)) {
				hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x08, ap, NULL, NULL, 0, 0, 0);
				return EINVAL;
			}
		}

		if ( HFC_MMODE_CHECK_SHARED(ap) )
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, (int)(HFC_MLPF_REC_END | HFC_MLPF_HWERR));
		else
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, (int)(0x00000000));

		hfc_reset_all_timer(ap);

		if (ap->pkg.port <= 1) {
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x20, ap, NULL, NULL, 0, 0, 0);

			/* Issue forced machine check request */
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDCTL,( char )0x1, 0x08 );
			
			/* Set HFC_HWWISOL to mp_adap_info */
			set_bit(HFC_HWISOL,(ulong *)&mpap->status);
			set_bit(HFC_ISOL, (ulong *)&ap->status);
			
			if (proc == TRUE)
				ap->isol_detail = HFC_ISOLATE_PORT_C;
			
			/* Wait until DMA transfer ends */
			udelay(1000);
		}
		else {
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x30, ap, NULL, NULL, 0, 0, 0);
			
			if ( HFC_MMODE_CHECK_SHADOW(ap) ){
				for (i=0;i<500;i++)
					udelay(1000);	/* FCLNX-GPL-428 */
			}
			
			/* Issue firmware isolate request */
			hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA,( char )0x4, HFC_FRAMEA_FW_ISOL_PORT );
			set_bit(HFC_ISOL, (ulong *)&ap->status);

			if (proc == TRUE) {
				ap->isol_detail = HFC_ISOLATE_PORT_C;
				ap->isol_cmd_mck_cnt = (uchar)mpap->mck_err_cnt; /* FCWIN-GPL-357 */
			}
			else {
				/* Issue control reset */
				hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_ISOLATE_DELAY_TMR, 0, TRUE);
				hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_ISOLATE_DELAY_TMR, 0, FALSE);

				hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x31, ap, NULL, NULL, 0, 0, 0);
				return 0;
			}
		}
		
		if (proc == TRUE) {
			hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x41, ap, NULL, NULL, 0, 0, 0);
			return 0;
		};
	}
	
	hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x50, ap, NULL, NULL, 0, 0, 0);
	/* Cancel scsi command and mailbox request */
	HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY );

	/* Cancel scsi command */
	for (lp=0 ; lp<MAX_TARGET_PROBE ; lp++) {

		target = hfc_hash_target_valid(ap, lp);		/* FCLNX-703 */
		if (target != NULL)
		{
			if ( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) ){	/* FCLNX-703 */
				hfc_notify_tout(ap, target);	/* FCLNX-GPL-573 output SCSI time-out log. */
				hfc_cancel_scsi_cmd(
					ap, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,
							TRUE, TRUE, HFC_FLASH_TARGET);
				target->status = HFC_NON_STATUS ;
			}
			
			if ( hfc_manage_info.hfcldd_mp_mod ) {
				if ((ap->isol_detail == HFC_ISOLATE_PORT_C)
				 || (ap->isol_detail == HFC_ISOLATE_CHKSTP_C)) {
					hfc_manage_info.npubp->hfc_forced_offline_c(target, TRUE);
				}
				hfc_manage_info.npubp->hfc_forced_offline_e(target, TRUE);	/* FCLNX-704 */
			}
		}
	}
	hfc_wwnverify_linkup_timeout(ap, NULL, 0);		/* FCLNX-GPL-331 */
	
	/* wake up mailbox request */

	if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) ){
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
	}

	if( (test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status)) ||
		 (ap->initialize == 1) )
	{
		unlock_mailbox( ap ) ;
		hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		clear_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status);
	}
	
	ap->status = 0;         					/* FCLNX-GPL-572 */
	set_bit(HFC_ENABLE,  (ulong *)&ap->status);	/* FCLNX-GPL-572 */
	set_bit(HFC_ISOL, (ulong *)&ap->status);	/* FCLNX-GPL-572 */

	if (ap->pkg.port <= 1) {
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x55, ap, NULL, NULL, 0, 0, 0);

		/* Issue control reset */
		set_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );

		if ( ctlres_flag == 0 ){ /* Issue control reset only once */
			/* FCLNX-GPL-554 BS500 TX_Disable disconnect */
			if ( (ap->pkg.type == HFC_PKTYPE_FIVE_EX) &&
				(0x94 <= ap->pkg.code) && (ap->pkg.code <= 0x97) )
			{
				hfc_reset_start(ap, HFC_INI_RESET);
			} else {
				hfc_reset_start(ap, HFC_CTLRST);
				udelay(1000);
			}
			ctlres_flag = 1;
		}

		/* wait until completion of CTLRST */
	 	clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );
 
		/*-- 1.67 LED 0x44 dummy write(H/W Bug) --*/
		/* Stop optical transmission */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(ap,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
		}
		else{
			hfc_mlpf_set_fcif(ap, 0x80808080);	/* FCLNX-GPL-399 */
		}
			
		/* Turn LED (Yellow and Green) off */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(
				ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
		}
		else{
			hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
		}
	}
	else{	/* FCLNX-GPL-363 */
		/* Turn LED (Yellow and Green) off */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(
				ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
		}
		else{
			hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
		}
	}		/* FCLNX-GPL-363 */
	
	memset(ap->logdata, 0, 16);		/* FCLNX-GPL-391 */
	ap->logdata[0]=ap->c_err;		/* FCLNX-GPL-391 */
	ap->logdata[1]=ap->isol_detail;	/* FCLNX-GPL-391 */

	if ((ap->isol_detail == HFC_ISOLATE_PORT_C)
	 || (ap->isol_detail == HFC_ISOLATE_CHKSTP_C)) {
		eno = (ap->pkg.port <= 1) ? 0x8E : 0xD4;

		hfc_errlog(
				ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
				ERRID_HFCP_EVNT2, eno, ap->logdata, 16);	/* FCLNX-GPL-391 */

		HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
				ap->pci_cfginf->bus->number,
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn));
	}
	else {
		eno = (ap->pkg.port <= 1) ? 0x8F : 0xD5;

		hfc_errlog(
				ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
				ERRID_HFCP_EVNT2, eno, ap->logdata, 16);	/* FCLNX-GPL-391 */
		
		HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by error \n",
				ap->pci_cfginf->bus->number,
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn));
	}

	hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x10, ap, NULL, NULL, 0, 0, 0);

	HFC_EXIT("hfc_force_linkdown");                  //FCLNX-0488
	return 0;                                      //FCLNX-0488
}				/* FCLNX-GPL-147 */


/*
 * Function:    hfc_force_linkdown_recovery
 *
 * Purpose:     Recovery for isolated Aaapter port
 *
 * Arguments:
 *  ap         -
 *  err_no     -
 *  mode       -
 *
 * Returns:
 *
 * Notes:
 */
int hfc_force_linkdown_recovery(struct adap_info *ap){				/* FCLNX-GPL-147 */
	struct	mp_adap_info 	*mpap;

	HFC_ENTRY("hfc_force_linkdown_recovery");					/*FCLNX-0506*/
	
	hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC, 0x00, ap, NULL, NULL, 0, 0, 0);
	
	if (test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)) {	/* FCLNX-GPL-393 */
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC, 0x04, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-0707 */
		return EINVAL;
	}

	if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has happen. */
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC, 0x05, ap, NULL, NULL, 0, 0, 0);
		HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
		return EINVAL;
	}
	
	if (ap->pkg.port > 1) { /* FCLNX-583 */
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC, 0x02, ap, NULL, NULL, 0, 0, 0);
		return EINVAL;
	}

	/* This adapter is not an isolated state */
	mpap = ap->mp_adap_info;
	if (!test_bit ( HFC_HWISOL, (ulong *)&mpap->status)){
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC, 0x03, ap, NULL, NULL, 0, 0, 0);
		return EINVAL;
	}

	/* Adapter isolation recovery process is in progress */
	clear_bit ( HFC_ISOL, (ulong *)&ap->status);
	ap->isol_detail = HFC_NO_ISOLATE;
	ap->c_err = 0x00;
	set_bit ( HFC_ISOL_RECOVERY, (ulong *)&ap->status);
	
	/* Issue control reset */
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_CTLRST_DELAY_TMR, 0, TRUE);   /* FCLNX-0276 */
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_CTLRST_DELAY_TMR, 1, FALSE);  /* FCLNX-0276 */
	
	hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC, 0x10, ap, NULL, NULL, 0, 0, 0);

	HFC_EXIT("hfc_force_linkdown_recovery");	/*FCLNX-0506*/
	return (0);
}				/* FCLNX-GPL-147 */


/*
 * Function:    hfc_force_linkdown_port_recovery
 *
 * Purpose:     Recovery for isolated Aaapter port
 *
 * Arguments:
 *  ap         -
 *  err_no     -
 *  mode       -
 *
 * Returns:
 *
 * Notes:
 */
int hfc_force_linkdown_recovery_port(struct adap_info *ap){				/* FCLNX-GPL-147 *//* FCLNX-GPL-402 */
	struct	mp_adap_info	*mpap;

	HFC_ENTRY("hfc_force_linkdown_port_recovery");

	mpap = ap->mp_adap_info;
	
	hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x00, ap, NULL, NULL, 0, 0, 0);

	if (test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)) {
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x02, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-0707 */
		return EINVAL;
	}
	
	if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-258 */
	{	/* "PCI BUS ERR" has happen. */
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x03, ap, NULL, NULL, 0, 0, 0);
		HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
		return EINVAL;
	}
	
	if ( (ap->isol_err_mck_cnt != (uchar)mpap->mck_err_cnt) ||	/* FCLNX-GPL-357 */
	     (ap->isol_cmd_mck_cnt != (uchar)mpap->mck_err_cnt) )
	{
		hfc_reset_adap_info(ap);
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x04, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */
	}																		/* FCLNX-GPL-357 */
	
	hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_RST,( char )0x4, (int)(0xffffffff));
	hfc_write_reg(ap,( uint )HFC_IOSPACE_CA_RSTINFO,( char )0x1,(char)0x00);
	hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA,( char )0x4, HFC_FRAMEA_FW_START_PORT );
	hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask_mlpf[ap->pkg.type]);

	set_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status);
	clear_bit(HFC_ISOL, (ulong *)&ap->status);
	ap->isol_detail = HFC_NO_ISOLATE;
	ap->c_err = 0x00;

	hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x01, ap, NULL, NULL, 0, 0, 0);

	clear_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
#endif	/* FCLNX-GPL-FX-424 */

	if ( hfc_issue_linkini(ap) ) {
		return EINVAL;
	}
																	/* @MLPF STR */
	if (HFC_MMODE_CHECK_SHARED(ap))
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask_mlpf[ap->pkg.type] );
	else
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask[ap->pkg.type] );
																	/* @MLPF END */	
	hfc_errlog(
		ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
		ERRID_HFCP_EVNT2, 0xD3, NULL, 0);

	HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is recovered \n",
		ap->pci_cfginf->bus->number,
		PCI_SLOT(ap->pci_cfginf->devfn),
		PCI_FUNC(ap->pci_cfginf->devfn));

	hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x10, ap, NULL, NULL, 0, 0, 0);

	HFC_EXIT("hfc_force_linkdown_port_recovery");
	return(0);
}				/* FCLNX-GPL-147 *//* FCLNX-GPL-402 */


/*
 * Function:    hfc_hand2_trace
 *
 * Purpose:     Collection of trace
 *
 * Arguments:   
 *  id         - 
 *  sub_id     - 
 *  ap         - 
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
void hfc_hand2_trace(
	uchar               id,
	uchar               sub_id,
	struct adap_info    *ap,
	struct target_info  *target,
	struct hfc_pkt		*hfcp,
	uint64_t            etc1,
	uint64_t            etc2,
	uint64_t            etc3)
{
	uchar               trc_wk[128] ;
	struct err_trc1    *trc1 ;
	struct err_trc2    *trc2 ;
	struct err_trc3    *trc3 ;
	struct err_trc4    *trc4 ;
	struct err_trc5    *trc5 ;
	struct err_trc6    *trc6 ;
	struct err_trc7    *trc7 ;
	struct err_trc8    *trc8 ;	/* FCLNX-GPL-393 */
	struct scsi_cmnd    		*cmnd = NULL;
	uint				etc1_wk=0, etc2_wk=0, etc3_wk=0;
	ushort				etc2_uwk, cmd_xob=0, lun=0;
	uint64_t			cmnd64=0;
	uint				xob_no;
	uint				ser_num;
	uchar				*ptr; /* FCLNX-GPL-069 */


	memset(trc_wk,0,128) ;
	
/*----------------------------------------------*/
/*                  TRACE1                      */
/* etc1 = int_a_reg                             */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/
	etc1_wk = (uint)etc1;
	etc2_wk = (uint)etc2;
	etc2_uwk = (ushort)etc2;
	if(hfcp != NULL ) cmnd = hfcp->cmd_pkt;
	if( id == HFC_TRC_HANDLER )
	{/*-- trace format 1 --*/
		trc1 = (struct err_trc1 *)trc_wk ;
		
		trc1->id = id ;
		trc1->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc1->a_status, ap->status);
			HFC_8L_TO_8B(trc1->a_scsi_id, ap->scsi_id);
		}
		if( etc1 != 0)
			HFC_4L_TO_4B(trc1->int_a_reg, etc1_wk);
	}/*-- trace format 1 --*/
	
/*----------------------------------------------*/
/*                  TRACE2                      */
/* etc1 = NULL                                  */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	if( (id == HFC_TRC_MBRESP) ||
		(id == HFC_TRC_LINKRSP) ||
		(id == HFC_TRC_PDISCRSP)||
		(id == HFC_TRC_GIDFTRSP) ||
		(id == HFC_TRC_MIHLGRSP) ||
		(id == HFC_TRC_GPNIDRSP) ||
		(id == HFC_TRC_LGINRSP ) ||
		(id == HFC_TRC_GIDPNRSP))
	{/*-- trace format 2 --*/
		trc2 = (struct err_trc2 *)trc_wk ;
		
		trc2->id = id ;
		trc2->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc2->a_status, ap->status) ;
			HFC_8L_TO_8B(trc2->a_scsi_id, ap->scsi_id);
			trc2->mb_status = ap->mb_status ;
			HFC_2L_TO_2B(trc2->mb_retry_cnt, ap->mb_retry_cnt);
//			HFC_LP_TO_BP(trc2->a_next_tstart, ap->next_tstart);	/* FCLNX-GPL-0343 */
			/* FCLNX-GPL-069 */
			ptr = (uchar *)&ap->mb->mb_resp.flag;
			HFC_MEMCPY(trc2->mb_resp, ptr, 68);
		}
		if( target != NULL )
		{
			HFC_4L_TO_4B(trc2->t_status, target->status);
			trc2->t_flags = target->flags ;
			trc2->t_pseq = target->pseq ;
			HFC_2L_TO_2B(trc2->t_id, target->device_flags);	/* FCLNX-0659 */
			HFC_8L_TO_8B(trc2->t_ww_name, target->ww_name);
			HFC_8L_TO_8B(trc2->t_node_name, target->node_name);
		}
		if( etc1 != 0)
			HFC_4L_TO_4B(trc2->passthrough_rsp, etc1_wk);
	}/*-- trace format 2 --*/
	
	if( (id == HFC_TRC_MBINT) ||
		(id == HFC_TRC_LINKDOWN_INT) ||
		(id == HFC_TRC_LINKUP_INT)||
		(id == HFC_TRC_PLOGI_INT) ||
		(id == HFC_TRC_LOGO_INT) ||
		(id == HFC_TRC_SCN_INT) ||
		(id == HFC_TRC_RSCN_INT))
	{/*-- trace format 2 --*/
		trc2 = (struct err_trc2 *)trc_wk ;
		
		trc2->id = id ;
		trc2->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc2->a_status, ap->status) ;
			HFC_8L_TO_8B(trc2->a_scsi_id, ap->scsi_id);
			trc2->mb_status = ap->mb_status ;
			HFC_2L_TO_2B(trc2->mb_retry_cnt, ap->mb_retry_cnt);
//			HFC_LP_TO_BP(trc2->a_next_tstart, ap->next_tstart);
			/* FCLNX-GPL-069 */
			ptr = (uchar *)&ap->mb->mb_intreq.type.fwintreq0.int_code;
			HFC_MEMCPY(trc2->mb_resp, ptr, 32);
			ptr = (uchar *)&ap->mb->mb_intreq.type.fwintreq0.un.scn.els_type;
			HFC_MEMCPY(&trc2->mb_resp[32], ptr, 36) ;
		}
		if( target != NULL )
		{
			HFC_4L_TO_4B(trc2->t_status, target->status);
			trc2->t_flags = target->flags ;
			trc2->t_pseq = target->pseq ;
			HFC_2L_TO_2B(trc2->t_id, target->device_flags);	/* FCLNX-0659 */
			HFC_8L_TO_8B(trc2->t_ww_name, target->ww_name);
			HFC_8L_TO_8B(trc2->t_node_name, target->node_name);
		}
	}/*-- trace format 2 --*/

/*----------------------------------------------*/
/*                  TRACE3                      */
/* etc1 = xrb_cnt                               */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	if( id == HFC_TRC_XRBRSP )
	{/*-- trace format 3 --*/
		trc3 = (struct err_trc3 *)trc_wk ;
		
		trc3->id = id ;
		trc3->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc3->a_status, ap->status);
			HFC_8L_TO_8B(trc3->a_scsi_id, ap->scsi_id);
			trc3->xrb_outp=ap->fw_init_p->xrb_outp;
			trc3->xrb_inp=ap->fw_init_p->xrb_inp;
			HFC_2L_TO_2B(trc3->xrb_no,ap->xrb_no);	/* FCLNX-0659 */
			if(hfcp != NULL)
			{
				xob_no = (uint)hfcp->cmd_xob;
				HFC_4L_TO_4B(trc3->xob_no, xob_no);
			}
			/* FCLNX-GPL-069 */
			ptr = (char *)&ap->xrb[ap->xrb_no].resp_iu.fcp_status2;
			HFC_MEMCPY(trc3->xrb, ptr, 6);
			ptr = (char*)&ap->xrb[ap->xrb_no].resp_iu.sns_len;
			HFC_MEMCPY(&trc3->xrb[6], ptr, 32);
			ptr = (char*)&ap->xrb[ap->xrb_no].flag;
			HFC_MEMCPY(&trc3->xrb[38], ptr, 32);
		}
		if( etc2 != 0 )
		{
			HFC_2L_TO_2B(trc3->xrb_no, etc2_uwk);		/* FCLNX-0659 */
		}
		if( hfcp != NULL ){ /*  */
			trc3->adap_status = hfcp->adap_status;
		}
	}/*-- trace format 3 --*/
	
/*----------------------------------------------*/
/*                  TRACE4                      */
/* etc1 = cmnd                                  */
/* etc2 = hfc_pkt                               */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	if( (id == HFC_TRC_SCSI_CHK) ||
		(id == HFC_TRC_MGM_CHK) ||
		(id == HFC_TRC_LINK_CHK) ||
		(id == HFC_TRC_DEQ_WE))
	{/*-- trace format 4 --*/
		trc4 = (struct err_trc4 *)trc_wk ;
		
		trc4->id = id ;
		trc4->sub_id = sub_id ;
		if( hfcp != NULL)
		{
			cmnd = (struct scsi_cmnd *)hfcp->cmd_pkt;
		}
		
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc4->a_status, ap->status);
			HFC_8L_TO_8B(trc4->a_scsi_id, ap->scsi_id);
			trc4->fcp_status = ap->xrb[ap->xrb_no].resp_iu.fcp_status2;
			trc4->scsi_status = ap->xrb[ap->xrb_no].resp_iu.scsi_status;
		}
		if( target != NULL )
		{
			HFC_4L_TO_4B(trc4->t_status, target->status);
			trc4->t_flags = target->flags ;
			trc4->t_pseq = target->pseq ;
			HFC_4L_TO_4B(trc4->t_we_que_cnt, target->we_que_cnt);
			trc4->s_target = (uchar)target->target_id;
		}
		if( cmnd != NULL )
		{
			cmnd64 = (ulong)cmnd;
			HFC_LP_TO_BP(trc4->srb, cmnd64);
			trc4->s_function = (uchar)cmnd->cmnd[0];
//			trc4->s_target = CMND_TARGET(cmnd);
			trc4->s_quetag = 0;
//			trc4->s_queaction = CMND_CHANNEL(cmnd);
			trc4->s_cdblen = cmnd->cmd_len;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
			trc4->s_senselen = cmnd -> sdb.length;

			HFC_4L_TO_4B(trc4->s_datatransferlength, cmnd -> sdb.length);
#else
			trc4->s_senselen = cmnd->request_bufflen;

			HFC_4L_TO_4B(trc4->s_datatransferlength, cmnd->request_bufflen);
#endif
			HFC_4L_TO_4B(trc4->result, cmnd->result);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
			HFC_4L_TO_4B(trc4->resid, cmnd->resid);
#endif
			/* kernel 5.15+: scsi_cmnd->serial_number removed; use 0 for trace */
			ser_num = 0;
			HFC_4L_TO_4B(trc4->serial_number, ser_num);
		}
		if( hfcp != NULL)
		{
			HFC_LP_TO_BP(trc4->hfcp, hfcp);
			HFC_4L_TO_4B(trc4->h_adap_status, hfcp->adap_status);
			HFC_4L_TO_4B(trc4->h_iov_no, hfcp->iov_no);
			HFC_4L_TO_4B(trc4->h_iov_cnt, hfcp->iov_cnt);
			lun = (ushort)hfcp->lun_id;		/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
			HFC_2L_TO_2B(trc4->s_lun, lun);	/* FCLNX-GPL-0382 */
		}
	}/*-- trace format 4 --*/
	
/*----------------------------------------------*/
/*                  TRACE5                      */
/* etc1 = cmnd                                  */
/* etc2 = timer_id                              */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	if( id == HFC_TRC_WDOG )
	{/*-- trace format 5 --*/
		trc5 = (struct err_trc5 *)trc_wk ;
		trc5->id = id ;
		trc5->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc5->a_status, ap->status);
			HFC_8L_TO_8B(trc5->a_scsi_id, ap->scsi_id);
			HFC_2L_TO_2B(trc5->xrb_no, ap->xrb_no);
		}
		if( target != NULL )
		{
			HFC_4L_TO_4B(trc5->t_status, target->status);
			trc5->t_flags = target->flags ;
			trc5->t_pseq = target->pseq ;
			trc5->s_target = (uchar)target->target_id;
		}
		if( hfcp != NULL)
		{
			cmnd = (struct scsi_cmnd *)hfcp->cmd_pkt;
			cmd_xob = (ushort)hfcp->cmd_xob;		/* FCLNX-0659 */
			HFC_2L_TO_2B(trc5->xrb_no, cmd_xob);	/* FCLNX-0659 */
			lun = (ushort)hfcp->lun_id;				/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
			HFC_2L_TO_2B(trc5->s_lun, lun);			/* FCLNX-GPL-0382 */
		}
		if( cmnd != NULL )
		{
			cmnd64 = (ulong)cmnd;
			HFC_LP_TO_BP(trc5->srb, cmnd64);
			trc5->s_function = (uchar)cmnd->cmnd[0];
//			trc5->s_target = CMND_TARGET(cmnd);
//			trc5->s_lun = CMND_LUN(cmnd);
		}
		if( etc2 != 0 )
		{
			HFC_2L_TO_2B(trc5->timer_id, etc2_uwk);
		}
	}/*-- trace format 5 --*/
	
/*----------------------------------------------*/
/*                  TRACE6                      */
/* etc1 = int_a_reg                             */
/* etc2 = status_reg                            */
/* etc3 = detail_reg                            */
/*----------------------------------------------*/

	if( (id == HFC_TRC_CHKSTP) ||
		(id == HFC_TRC_ABEND) ||
		(id == HFC_TRC_MCKREC) ||
		(id == HFC_TRC_HWERR) )
	{/*-- trace format 6 --*/
		trc6 = (struct err_trc6 *)trc_wk ;
		
		trc6->id = id ;
		trc6->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc6->a_status, ap->status);
			HFC_8L_TO_8B(trc6->a_scsi_id, ap->scsi_id);
			HFC_4L_TO_4B(trc6->link_dead_cnt, ap->link_dead_cnt);
/*			HFC_4L_TO_4B(trc6->pci_err_cnt, ap->pci_err_cnt); */
			HFC_4L_TO_4B(trc6->mck_err_cnt, ap->mp_adap_info->mck_err_cnt);	/* FCLNX-GPL-057 */
		}
		if( etc1 != 0)
			trc6->int_a_reg = (uint)etc1;
		if( etc2 != 0)
			trc6->status_reg = etc2 ;
		if( etc3 != 0)
			trc6->detail_reg = (uint)etc3;
	}
	
/*----------------------------------------------*/
/*                  TRACE7                      */
/* etc1 =                                       */
/* etc2 =                                       */
/* etc3 =                                       */
/*----------------------------------------------*/

	if( (id == HFC_TRC_FORCE_ISOL)  ||
		(id == HFC_TRC_FORCE_ISOL_REC)  ||
		(id == HFC_TRC_FORCE_ISOL_REC_P)  ||
		(id == HFC_TRC_CHECK_ERRCOUNT))		/* FCLNX-GPL-349 */
	{/*-- trace format 7 --*/
		trc7 = (struct err_trc7 *)trc_wk ;
		
		trc7->id = id ;
		trc7->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc7->a_status, ap->status);
			HFC_8L_TO_8B(trc7->a_scsi_id, ap->scsi_id);
			HFC_4L_TO_4B(trc7->link_dead_cnt, ap->link_dead_cnt);
			HFC_4L_TO_4B(trc7->pci_err_cnt, ap->pci_err_cnt);
			if (ap->mp_adap_info != NULL) {
				HFC_4L_TO_4B(trc7->mck_err_cnt, ap->mp_adap_info->mck_err_cnt);
				HFC_4L_TO_4B(trc7->mpap_sts, ap->mp_adap_info->status);
			}
			else {
				trc7->mck_err_cnt = 0xffffffff;
				trc7->mpap_sts    = 0xffffffff;
			}
			trc7->isol_detail = ap->isol_detail;
			trc7->c_err = ap->c_err;
		}
		trc7->etc1 = (uint)etc1;
		trc7->etc2 = (uint)etc2;
		trc7->etc3 = (uint)etc3;
	}
	
/* FCLNX-GPL-393 */
/*----------------------------------------------*/
/*                  TRACE8                      */
/* etc1 =                                       */
/* etc2 =                                       */
/* etc3 =                                       */
/*----------------------------------------------*/

	if( (id == HFC_TRC_MLPF_INT ) ||
		(id == HFC_TRC_MLPF_HWERR_INT) ||
		(id == HFC_TRC_MLPF_FORCE_ISOL) ||
		(id == HFC_TRC_MLPF_RECV_ISOL) ||
		(id == HFC_TRC_MLPF_HWERR_INT_DET) ||
		(id == HFC_TRC_MLPF_MIGRATION) )
	{
		trc8 = (struct err_trc8 *)trc_wk ;
		trc8->id = id ;
		trc8->sub_id = sub_id ;
		if( ap != NULL )
		{
			HFC_4L_TO_4B(trc8->a_status, ap->status);
			HFC_8L_TO_8B(trc8->a_scsi_id, ap->scsi_id);
			HFC_4L_TO_4B(trc8->link_dead_cnt, ap->link_dead_cnt);
			HFC_4L_TO_4B(trc8->pci_err_cnt, ap->pci_err_cnt);
			if (ap->mp_adap_info != NULL) {
				HFC_4L_TO_4B(trc8->mck_err_cnt, ap->mp_adap_info->mck_err_cnt);
				HFC_4L_TO_4B(trc8->mpap_sts, ap->mp_adap_info->status);
			}
			else {
				trc8->mck_err_cnt = 0xffffffff;
				trc8->mpap_sts    = 0xffffffff;
			}
		}
		etc3_wk =(uint)etc3;
		
		if( etc1 != 0)
			HFC_4L_TO_4B(trc8->hyp_status, etc1_wk);
		if( etc2 != 0)
			HFC_4L_TO_4B(trc8->etc2, etc2_wk);
		if( etc3 != 0)
			HFC_4L_TO_4B(trc8->etc3, etc3_wk);
	}
/* FCLNX-GPL-393 */

	hfc_trace(ap,id,&trc_wk[1],0);
}


/*
 * Function:    hfc_clear_sticky_bit
 *
 * Purpose:     Clear "config registar sticky bit" for FIVE-EX
 *
 * Arguments:   
 *  ap          - Adapter Information
 *
 * Returns:     
 *
 * Notes:       ap and ap->pci_cfginf is not NULL
 */
void hfc_clear_sticky_bit(struct adap_info *ap)
{
	struct pci_dev *pdev; /* FCLNX-GPL-230 */
	
	pdev = ap->pci_cfginf; /* FCLNX-GPL-230 */
	
	/* Clear Status Register */
	hfc_write_cnfg(ap, 0x06, 0x2, 0xffff); /* 0x06 -0x07 */
	/* Clear Device Status Register */
	hfc_write_cnfg(ap, 0x76, 0x1, 0xff); /* 0x76 - 0x77 (0x77 is Reserved area) */
	
	/* Check CFG space size */ /* FCLNX-GPL-230 start */
	if( pdev->cfg_size > 0x114 ) /* 0x114 is (0x110 + 0x4) */
	{	/* Access was permited */
		
		/* Clear AER Uncrrectable Error Status */
		hfc_write_cnfg(ap, 0x104, 0x4, 0xffffffff); /* 0x104 - 0x107 */
		/* Claer AER Correctable Error Status */
		hfc_write_cnfg(ap, 0x110, 0x4, 0xffffffff); /* 0x110 - 0x113 */
	}
	/* FCLNX-GPL-230 end */
	
	/* Set DUMP Core's FRZ_CLR bit */ /* HFC_DUMP_FRZ_CLR is 0x00400000 */
	hfc_write_reg(ap,( uint )HFC_IOSPACE_DUMP_CMD,( char )0x4, HFC_DUMP_FRZ_CLR); /* FCLNX-GPL-081 */
	
	return;
}

/*
 * Function:    hfc_pcie_sram_ce_recovery
 *
 * Purpose:     This func recovery "PCIe IP core SRAM 1bit ERR".
 *
 * Arguments:   
 *  ap          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_pcie_sram_ce_recovery(struct adap_info *ap)
{
	/******** Execute Logging process ******************/
	/* Check the parameter */
	if(ap->max_pcie_sram_ce_cnt != 0){
		/* Always count up when this param is not 0. */
		/* Save Log data */
		if(ap->pcie_sram_ce_cnt < HFC_1BIT_LOG_ENTRY){ /* Array length check */
			hfc_save_pcie_sram_log(ap); /* get "PCIe" sram log*/
			ap->pcie_sram_ce_cnt++; /* Count up */
			hfc_set_sram_ce_log(ap); /* set "All" srem ce log */

			if(ap->pcie_sram_ce_cnt == ap->max_pcie_sram_ce_cnt){
				/* Count over */ /* log out only 1 time */
				hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_SRAMCE,
					ERRID_HFCP_ERR2, 0x000000a5, NULL, 16 );
			}
		}
	}

	/******** Reset Registers **************************************/
	/* Set a bit to issue IPRES */ /* FCLNX-GPL-128 */
	hfc_write_reg(ap, (uint)HFC_IOSPACE_KCMD_IPRES, (char)0x01, 0x01);

	return;
}

/*
 * Function:    hfc_set_sram_ce_log
 *
 * Purpose:
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_set_sram_ce_log(struct adap_info *ap)
{
	int i;
	uint wk;
	
	/*** Set SRAM CE Log ***/
	/* Reserved Space */
	memset(ap->ce_log.resv1, 0xff, 12);
	memset(ap->ce_log.resv2, 0xff, 12);
	memset(ap->ce_log.resv3, 0xff, 460);
	/* ECID */
	for (i=0; i<4 ;i++) {
		wk = (uint)hfc_read_reg_ext(ap, 0x200+(i*4), (char)0x4) ;
		HFC_4L_TO_4B(ap->ce_log.ecid[i], wk);
	}
	/* pcie_sram_cnt */
	HFC_4L_TO_4B(ap->ce_log.pcie_sram_cnt, ap->pcie_sram_ce_cnt);
	/* core_ce_cnt */
	HFC_4L_TO_4B(ap->ce_log.core_ce_cnt, ap->core_ce_cnt);
	
	return;
}

/* FCLNX-GPL-209 */
/*
 * Function:    hfc_mck_prepare
 *
 * Purpose:     We prepare something for MCK recovery
 *                1. Give up waiting mailbox.
 *                2. Stop timer.
 *                3. Cancel SCSI command.(And, clear target_info.)
 *
 * Arguments:   
 *  ap          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_mck_prepare(struct adap_info *ap)
{
	struct target_info  *target = NULL;
	int rsp;
	uint lp;
	struct dev_info 	*dev=NULL;
	
	
	HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY);	
	hfc_reset_watchdog(ap);										/* FCLNX-GPL-038 */
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)						
	{
		target = hfc_hash_target_info(ap, lp);
		if (target != NULL)
		{
			hfc_cancel_scsi_cmd(
				ap, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,
				TRUE, TRUE, HFC_FLASH_TARGET);
			target->status = HFC_NON_STATUS ;
			
			dev = target->dev;
			while( dev != NULL){
				dev->lustat = 0x00;
				dev = dev->next;
			}
			
			if ( hfc_manage_info.hfcldd_mp_mod ) {
				hfc_manage_info.npubp->hfc_forced_offline_e(target, TRUE);
			}
		}
	}
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)											/* FCLNX-GPL-038 */
	{
		target = hfc_pseq_target_info(ap, lp);
		if (target != NULL)
		{
			/* Release all target_info */
			if( test_bit( HFC_WAIT_LOGIN, (ulong *)&target->status ) 
			  || test_bit( HFC_WAIT_CANCEL, (ulong *)&target->status ) ){	
				clear_bit( HFC_WAIT_LOGIN, (ulong *)&target->status );
				clear_bit( HFC_WAIT_CANCEL, (ulong *)&target->status );
				if (test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)) {
					rsp = hfc_clear_target_info( ap, target, TRUE, TRUE );
				}
			}
			else if( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags) &&		/* FCLNX-GPL-0052 */
			  !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) ){
				clear_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
				clear_bit( HFC_NEED_CANCEL, (ulong *)&target->status );
				rsp = hfc_clear_target_info( ap, target, TRUE, TRUE );				/* FCLNX-GPL-0052 */
			}
		}
	}																				/* FCLNX-GPL-038 */
	
	return;
}

/* FCLNX-GPL-209 */
/*
 * Function:    hfc_mck_recovery
 *
 * Purpose:     -
 *
 * Arguments:   
 *  ap          - Adapter Information
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_mck_recovery(struct adap_info *ap, uchar type)
{
	
	switch(ap->pkg.type){  /* FCLNX-GPL-081 */
		case HFC_PKTYPE_FPP:
			hfc_mck_recovery_fpp(ap,type);
			break;
			
		case HFC_PKTYPE_FIVE:
		case HFC_PKTYPE_FIVE_EX:
			if( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ) ) {				/* FCLNX-GPL-0157 */
				ap->mck_progress = HFC_MCK_PROGRESS;
			}
			else {
				ap->mck_progress = 0;
			}
			hfc_mck_recovery_five(ap,type);											/* FCLNX-GPL-0157 */
			break;
			
		default:
			/* NOP */
			break;
	}

	return;
}

void hfc_clear_errinfo_i( struct adap_info *ap )
{
	int	i;
	struct target_info *target;

	HFC_ENTRY("hfc_clear_errinfo_i");

	/* clear all error count and factor */
	ap->c_err = 0x00;
	
	ap->ld_err_count_s = 0;
	ap->if_err_count = 0;
	ap->to_err_count = 0;
	
	for (i=0;i<ap->max_target;i++) {
		target = ap->target_arg[i];
		if(target != NULL){
			target->tgt_ld_err_count_s = 0;
		}
	}

	HFC_EXIT("hfc_clear_errinfo_i");
}

int hfc_get_isolparam_i( struct adap_info *ap, struct hfc_isol_info *isolinfo, uchar pcm) {	/* FCLNX-GPL-393 */
	int	i;
	short	max_ld_err_count_s=0;
	struct	target_info		*target;
	
	HFC_ENTRY("hfc_get_isolparam");
	
	if(pcm == 0){	/* FCLNX-GPL-393 */
		isolinfo->ld_err_limit_s=	ap->ld_err_limit_s;
		
		max_ld_err_count_s = ap->ld_err_count_s;
		for(i=0; i<(ap->max_target); i++){
			target = hfc_hash_target_valid(ap, i);
			if( target != NULL){
				if(max_ld_err_count_s < target->tgt_ld_err_count_s){
					max_ld_err_count_s = target->tgt_ld_err_count_s;
				}
			}
		}
		isolinfo->ld_err_count_s=	max_ld_err_count_s;
		
		isolinfo->if_err_limit	=	ap->if_err_limit;
		isolinfo->if_err_count	=	ap->if_err_count;
		isolinfo->to_err_limit	=	ap->to_err_limit;
		isolinfo->to_err_count	= 	ap->to_err_count;
		isolinfo->rt_err_enable	=	ap->rt_err_enable;
		isolinfo->to_reset_retry =	ap->to_reset_retry;
		if(ap->c_err == HFC_ISOLATE_RT){
			isolinfo->rt_err_count = 1;
		}
		else{
			isolinfo->rt_err_count = 0;
		}
	
		if((ap->ld_err_limit_s)||(ap->if_err_limit)||(ap->to_err_limit)||(ap->rt_err_enable)){
			if(ap->hba_isolation == HFC_ISOL_START)
				isolinfo->err_is_func	=	HFC_ENABLE_ISOLATE;
			else
				isolinfo->err_is_func	=	HFC_DISABLE_ISOLATE;
		}
		else{
			isolinfo->err_is_func	=	HFC_DISABLE_ISOLATE;
		}
	}	/* FCLNX-GPL-393 */
	
	isolinfo->err_is_hvm_spt = HFC_HVM_ISOLATE_NOT_SUPPORT;	/* FCLNX-GPL-401 */
	if( !( HFC_MMODE_CHECK_SHARED(ap) ) ){
		if ((ap->pkg.port <= 1) || (test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))) {
			isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_SUPPORT;
		}
		else {
			isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_NOT_SUPPORT;
		}
	}
	else{
		if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support)){
			isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_SUPPORT;
		}else{
			isolinfo->err_is_fw_spt = HFC_FW_ISOLATE_NOT_SUPPORT;
		}
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support)){
			isolinfo->err_is_hvm_spt = HFC_HVM_ISOLATE_SUPPORT;
		}else{
			isolinfo->err_is_hvm_spt = HFC_HVM_ISOLATE_NOT_SUPPORT;
		}
	}								/* FCLNX-GPL-401 */
	
	HFC_EXIT("hfc_get_isolparam");
	
	return(0);
}

void hfc_check_errcount(struct adap_info *ap)
{												/* FCLNX-710 */
	uint hyp_status;				/* FCLNX-GPL-426 */
	
	if(!ap->c_err)
		return;
		
	hfc_hand2_trace(HFC_TRC_CHECK_ERRCOUNT, 0x00, ap, NULL, NULL, 0, 0, 0);
	
	if (test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) )	/* FCLNX-GPL-349 */
		return;
	
	if (test_bit (HFC_ISOL_RECOVERY, (ulong *)&ap->status))
		return;
	
	if (test_bit(HFC_WAIT_MIHLOG,(ulong *)&ap->status))
		return;
	
	/* Isolate to adapter */
	if ( !(HFC_MMODE_CHECK_SHARED(ap)) ){				/* FCLNX-GPL-393 */
		hfc_force_linkdown(ap, FALSE, FALSE);
	}
	else if(HFC_MMODE_CHECK_SHADOW(ap) ){				/* FCLNX-GPL-426 */
		if(ap->c_err == HFC_ISOLATE_SHADOW){
			if(test_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol)){
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				if((!test_bit(HFC_WAIT_T3, (ulong *)&ap->status)) &&
				(!test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status)) &&
				(!test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol)) &&
				(!test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol))){
					hfc_mlpf_isol_recovery_start_slpar(ap, hyp_status);	/* FCLNX-GPL-426 */
				}
			}
			if((test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol)) ||
			(test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol))){
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				if((!test_bit(HFC_WAIT_T3, (ulong *)&ap->status)) &&
				(!test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status)) &&
				(!test_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol))){	/* FCLNX-GPL-432 */
					hfc_mlpf_isol_start_slpar(ap, hyp_status);	/* FCLNX-GPL-426 */
				}
			}
		}
	}
	else{
		hfc_mlpf_issue_fisolate(ap, HFC_ISSUE_ISOLREQ_ERR);
	}													/* FCLNX-GPL-393 */
	
}												/* FCLNX-710 */

void hfc_watched_errcount_i(struct adap_info *ap, struct target_info *target, uchar err_flag)	/* FCLNX-GPL-349 */
{
	if (ap->hba_isolation == HFC_ISOL_STOP)	/* FCLNX-GPL-393 */
		return;
	
	switch (err_flag) {
		case HFC_LDS_ERR:
			if (!ap->ld_err_limit_s)
				break;
			ap->ld_err_count_s++;
			if (ap->ld_err_count_s >= ap->ld_err_limit_s) {
				ap->c_err = HFC_ISOLATE_LDS;
			}
			break;
		case HFC_IF_ERR:
			if (!ap->if_err_limit)
				break;
			ap->if_err_count++;
			if (ap->if_err_count >= ap->if_err_limit) {
				ap->c_err = HFC_ISOLATE_IF;
			}
			break;
		case HFC_TO_ERR:
			if (!ap->to_err_limit)
				break;
			ap->to_err_count++;
			if (ap->to_err_count >= ap->to_err_limit) {
				ap->c_err = HFC_ISOLATE_TO;
			}
			break;
		case HFC_TGT_LDS_ERR:
			if (!ap->ld_err_limit_s)
				break;
			target->tgt_ld_err_count_s++;
			if (target->tgt_ld_err_count_s >= ap->ld_err_limit_s) {
				ap->c_err = HFC_ISOLATE_TGT_LDS;
			}
			break;
		case HFC_RT_ERR:
			if (ap->rt_err_enable) { 
				ap->c_err = HFC_ISOLATE_RT;
			}
			break;
		default:
			break;
	}
	
	if (ap->c_err) {
		ap->isol_detail = HFC_ISOLATE_PORT_E;
	}
	
	return;
}																		/* FCLNX-GPL-349 */


/* FCLNX-GPL-430 */
/*
 * Function:    hfc_check_cmnd_timeout
 *
 * Purpose:     -
 *
 * Arguments:   
 *  ap          - Adapter Information
 *  target      - Target Information
 *
 * Returns:     1 : There is scsi command occred time-out in weque.
 *				0 : There isn't scsi command time-out in weque.
 *
 * Notes:       Interruption level
 */
int hfc_check_cmnd_timeout(struct adap_info *ap, struct target_info *target){
	int                         hash = 0;           /* FCLNX-0579 */
	struct hfc_pkt              *hfcp_wk = NULL;	/* FCLNX-0579 */
	
	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		if (target->we_que_top[hash] != NULL)
		{ 	/* hfcp exists in queue */
			hfcp_wk = target->we_que_top[hash];
			while( hfcp_wk != NULL )
			{
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
