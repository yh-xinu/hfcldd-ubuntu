/*
 * hfcl_diag.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char diag_rcsid[] = "$Id: hfcl_diag.c,v 1.32.2.10.2.3.4.3.6.10.4.15.2.9.6.10.2.4.2.5.2.4 2015/07/29 08:04:05 toyo Exp $";

#include "hfcldd.h"
#include "hfcl_detect.h"

/* forward declaration to suppress -Wmissing-prototypes */
int hfc_diag(void *arg, struct adap_info *ap);
#include "hfcl_top.h"
#include "hfcl_ioctl.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_hand_timer_trace.h"
#include "hfcl_mlpf.h"
#include "hfcl_version.h"
#include "hfcl_tbol.h" /* FCLNX-GPL-112 */
#include "hfcmpcfg.h"

/* Log size */
#define LOG_LENGTH_LDCH			0x8000	/* Load CH Trace Log(32Kbyte) */
#define LOG_LENGTH_MEINT		0x600	/* Send MEINT Log(1.5Kbyte)  */
#define LOG_LENGTH_SOFT			0x12000	/* Forced Soft Log(72Kbyte)  */

/* Log area segment number */
#define LOG_SEGNUM_LDCH			8	/* Load CH Trace Log */
#define LOG_SEGNUM_MEINT		1	/* Send MEINT Log  */
#define LOG_SEGNUM_SOFT			18	/* Forced Soft Log */

#define FPP_TRC_SIZE			32	/* Load CH Trace LOG(FPP Trace Pointer size) */
#define FRM_TRC_SIZE			8	/* Load CH Trace LOG(Frame Trace Pointer size) */

#define FPP_TRC_SIZE_OFFSET		8	/* Load CH Trace LOG(Response head offset of FPP Trace Pointer) */
#define FRM_TRC_SIZE_OFFSET		40	/* Load CH Trace LOG(Response head offset of Frame Trace Pointer) */

#define MAX_READ_CNT			1	/* Maximum Read Count */


/* PCI access */
#define PCI_LENGTH_01			0x01	/* Access size  1byte */
#define PCI_LENGTH_02			0x02	/* Access size  2byte */
#define PCI_LENGTH_04			0x04	/* Access size  4byte */

const uint pci_addr_max[2][5] = 
						{	/* PKTYPE */
							/* 0:none, 1:FPP, 2:FIVE, 3:FIVE-EX, 4:FIVE-FX */
							{   0xfff, 0xfff, 0x1fff,   0x1fff,    0x7fff  }, /* 0:BAR0 */
							{   0x000, 0x000, 0x0000,   0x07ff,    0x0fff  }  /* 1:BAR1 */
						};	/* FCLNX-GPL-154 */
const uint pci_cnf_addr_max[5] = {
							0xff, 	/* PKTYPE is none */
							0xff, 	/* 1 HFC_PKTYPE_FPP */
							0xff, 	/* 2 HFC_PKTYPE_FIVE */
							0x1ff,  /* 3 HFC_PKTYPE_FIVE_EX */
							0x7ff	/* 4 HFC_PKTYPE_FIVE_FX */
						};

const uint hg_addr_max[5] = {         /* FCLNX-GPL-120 */
						0x0,    /* 0 PKTYPE is none */
						0x0,    /* 1 HFC_PKTYPE_FPP : Virtage is not supported */
						0x3ff,  /* 2 HFC_PKTYPE_FIVE *//* FCLNX-GPL-495 */
						0x3ff,  /* 3 HFC_PKTYPE_FIVE_EX *//* FCLNX-GPL-495 */
						0x3ff   /* 4 HFC_PKTYPE_FIVE_FX */
						};


/*-- global variable --*/
extern int mih_log(struct adap_info *ap,struct diag_ioctl *diag);
extern int load_ch_trace_log(struct adap_info *ap,struct diag_ioctl *diag);
extern int forced_log(struct adap_info *ap,struct diag_ioctl *diag);
extern int fw_start(struct adap_info *ap,struct diag_ioctl *diag);
extern int fw_post(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_fw_init_tbl(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_xob(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_xrb(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_seg_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_adap_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_target_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_hfctrace(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_mailbox(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_version(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_hwlog(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_tree(struct adap_info *ap,struct diag_ioctl *diag);
extern int pci_access(struct adap_info *ap,struct diag_ioctl *diag);
extern int pci_cnf_access(struct adap_info *ap,struct diag_ioctl *diag);
extern int stop_func(struct adap_info *ap,struct diag_ioctl *diag);
extern int init_mode_set(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_mpadap_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int fcp_mode_set(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_manage_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_hfcpkt(struct adap_info *ap,struct diag_ioctl *diag);	/*2004.11.30*/
extern int read_dev_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_lg_target_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_lg_dev_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_lg_path_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_lg_path_info1(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_lg_path_info2(struct adap_info *ap,struct diag_ioctl *diag);
extern int read_failover_info(struct adap_info *ap,struct diag_ioctl *diag);
extern int online_update(struct adap_info *ap, struct diag_ioctl *diag); /* FCLNX-GPL-112 */
extern int hg_access(struct adap_info *ap,struct diag_ioctl *diag); /* FCLNX-GPL-120 */
extern int scan_target_fcsw(struct adap_info *ap,struct diag_ioctl *diag); /* FCLNX-GPL-492 */
extern int change_param(struct adap_info *ap,struct diag_ioctl *diag); 		/* FCLNX-GPL-493 */
extern int change_param_flash(struct adap_info *ap,struct diag_ioctl *diag);
extern int hfcldd_conf_store(struct adap_info *ap,struct diag_ioctl *diag);
extern int flash_update(struct adap_info *ap, struct diag_ioctl *diag); /* FCLNX-GPL-613 */
const Typ_diag_cntl diag_check[]={
/*----- Sub_Command ----------------------------- Mode --------------------- FPP ---- FIVE,FIVE-EX ---*/
	{ HFC_DG_MIHLOG   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xc0008000 , 0xc0008000 , (void*)mih_log },
	{ HFC_DG_LDCHTRC  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xc0008000 , 0xc0008000 , (void*)load_ch_trace_log },
	{ HFC_DG_FORCSLOG , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xc0000000 , 0xf0000000 , (void*)forced_log },
	{ HFC_DG_FWSTART  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)fw_start },
	{ HFC_DG_FWPOST   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0x40008000 , 0x40008000 , (void*)fw_post },
	{ HFC_DGRD_INITTBL, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_fw_init_tbl},
	{ HFC_DGRD_XOB	  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_xob },
	{ HFC_DGRD_XRB	  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_xrb },
	{ HFC_DGRD_SEGINFO, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_seg_info },
	{ HFC_DGRD_ADAP   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_adap_info },
	{ HFC_DGRD_TARGET , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_target_info },
	{ HFC_DGRD_DDTRC  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0008000 , 0xf0008000 , (void*)read_hfctrace },
	{ HFC_DGRD_MB     , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_mailbox },
	{ HFC_DGRD_VER    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_version },
	{ HFC_DGRD_HWLOG  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_hwlog },
	{ HFC_DGRD_TREE   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_tree },
	{ HFC_DGRD_SCMD   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_hfcpkt }, /*2004.11.30 */
	{ HFC_DG_RDPCI    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)pci_access },
	{ HFC_DG_WRPCI    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)pci_access },
	{ HFC_DG_RDPCI_CFG, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)pci_cnf_access },
	{ HFC_DG_WRPCI_CFG, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)pci_cnf_access },
	{ HFC_DG_FSTOP    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)stop_func },
	{ HFC_DGRD_MPINFO , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_mpadap_info },
	{ HFC_DG_INITMDST , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xb0000000 , (void*)init_mode_set },
	{ HFC_DG_FCPMDST  , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0x30000000 , (void*)fcp_mode_set },
	{ HFC_DGRD_MNGINFO, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_manage_info },
	{ HFC_DGRD_DEV    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_dev_info },
	{ HFC_DGRD_LGTGT  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_lg_target_info },
	{ HFC_DGRD_LGDEV  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_lg_dev_info },
	{ HFC_DGRD_PATHINFO, HFC_DIAG_FPP| HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_lg_path_info },
	{ HFC_DGRD_PATHINFO1,HFC_DIAG_FPP| HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_lg_path_info1 },
	{ HFC_DGRD_PATHINFO2,HFC_DIAG_FPP| HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_lg_path_info2 },
	{ HFC_DGRD_FOINFO , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)read_failover_info },
	{ HFC_DG_ONLINE_UP, 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xf0008000 , (void*)online_update }, /* FCLNX-GPL-112 */
	{ HFC_DG_RDHG     , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)hg_access },/* FCLNX-GPL-120 */
	{ HFC_DG_WRHG     , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0xf0000000 , 0xf0000000 , (void*)hg_access },/* FCLNX-GPL-120 */
	{ HFC_DG_TGTSCAN  , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xf0008000 , (void*)scan_target_fcsw },/* FCLNX-GPL-492 *//* FCLNX-GPL-506 */
	{ HFC_DG_CHGPARM  , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xf0008000 , (void*)change_param },/* FCLNX-GPL-493 *//* FCLNX-GPL-506 */
	{ HFC_DG_FLASH_CHGPARM, 		   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xf0008000 , (void*)change_param_flash },
	{ HFC_DG_HFCLDD_CONF,			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xf0008000 , (void*)hfcldd_conf_store },
	{ HFC_DG_FLASH_UPDATE,			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_EX , 0          , 0xf0008000 , (void*)flash_update },/* FCLNX-GPL-613 */
	{ 0               , 0                            , 0          , 0          , NULL }
};
/*--------------------------------------------------------------*
 * Log address list
 * Notes:
 *       Load ch Trace Log : log_addr[0-7] 
 *       Send MEINT Log    : log_addr[0]
 *       Forced soft Log   : log_addr[0-17] 
 *--------------------------------------------------------------*/
struct logaddr_list {
	uchar	segnum;				/* Number of segments */
	uchar	resv0;
	uchar	resv1;
	uchar	resv2;
	uint	resv3;
	uint64_t	bus_addr[18];	/* Bus address */
};

/*
 * Function:    set_fw_trace_mode
 *
 * Purpose:     Process for ioctl (HFCDIAG0) subcommand (Set FW Trace Mode) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
static __maybe_unused int set_fw_trace_mode(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	int		loop;
	int		rtn;
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */

	HFC_ENTRY("set_fw_trace_mode") ;
	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock mailbox */

	/*--------------------------------------------------------------*
	 * Set specified trace information to fw_init_tbl
	 *--------------------------------------------------------------*/
	BCOPY( (char *)&diag->uni.trc_mode , (char *)&ap->fw_init_p->trc_info, sizeof(struct fw_trc_info ));
	for( loop = 0; loop < 4; loop++ ){
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[loop].top_port_id, diag->uni.trc_mode.trc_seg[loop].top_port_id );
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[loop].bottom_port_id, diag->uni.trc_mode.trc_seg[loop].bottom_port_id );
	}

	/*--------------------------------------------------------------*
	 * Set up and initiate mailbox
	 *--------------------------------------------------------------*/
	/* Set up mailbox control block  */
	ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_SETFWTR;	/* Subcommand */
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );

	/* Initiate mailbox and wait for completion */ /* FCLNX-GPL-243 */
	rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry);	/*	FCLNX-0523 */
	
	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */

	HFC_EXIT("set_fw_trace_mode") ;
	return( rtn );
}


/*
 * Function:    mih_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (MIH Log)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int mih_log(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int		rtn;

	uchar	ssn = 0 ;					/* slog page No 	*/
	uchar	son = 0 ;					/* slog offset No	*/
	ushort	sbc = 0 ;					/* slog length		*/
	ushort	usSlogLen;
	ushort	usSlogNum;

	uint	slog_in_page ;
	uint	slog_adr ;

	int copy_cnt = 0 ;
	int i ;
	uchar	errlog_no ; /* FCLNX-GPL-229 */
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */

	HFC_ENTRY("mih_log") ;

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock mailbox */

	/* Set up mailbox control block  */
	ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_MIHLOG;	/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0010 );	/* LOGID */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if (( rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry) ) == 0 ) {	/* FCLNX-0523 */
		/* Copy data to Soft_Log_Area designated by ssn/son/sbc */
		ssn = (uchar)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.mih_log.ssn );
		son = (uchar)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.mih_log.son );
		sbc = (ushort)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.mih_log.sbc );
		usSlogLen = (ushort)hfc_read_val( ap->fw_init_p->slog_len );
		usSlogNum = (ushort)hfc_read_val( ap->fw_init_p->slog_num );

		if( sbc != 0 )
		{
			slog_in_page = HFC_PAGE_SIZE / (uint)usSlogLen ;
			if( (son < slog_in_page) && (ssn < usSlogNum) )
			{
				if ( sbc > usSlogLen )	/* sbc>1024 */
				{
					/* If the sbc is larger than 1024, copy every 1024 bytes */
					copy_cnt = sbc / usSlogLen;	/* Sbc/1024 */
					if( sbc % usSlogLen )		/* When residue exists, count up copy_cnt */
						copy_cnt++;

					for( i = 0 ; i < copy_cnt ; i++ )
					{
						slog_adr = (HFC_PAGE_SIZE * ssn)+(usSlogLen * son) ;

						if( i == (copy_cnt-1) ) /* FCLNX-GPL-229 start */
						{	/* Copy sbc-(1024*i) bytes data from ap->slog[slog_adr] to diag->addr+(1024*i) */
							if( COPYOUT( &ap->slog[slog_adr], ( (uchar *)(ulong)diag->addr+(usSlogLen*i) ), ( sbc - (usSlogLen*i) ) ) )
							{
								goto copyout_err;
							}
						}
						else
						{	/* Copy 1024 bytes data from ap->slog[slog_adr] to diag->addr+(1024*i) */
							if( COPYOUT( &ap->slog[slog_adr], ( (uchar *)(ulong)diag->addr+(usSlogLen*i) ), usSlogLen ) )
							{
								goto copyout_err;
							}
						} /* FCLNX-GPL-229 end */

						if( ( ssn == (usSlogNum-1) ) &&
							( son == (slog_in_page-1) ) )	/* ssn=63, son=3 */
						{
							ssn = 0;
							son = 0;
						}
						else
						{
							if( son == (slog_in_page-1) )	/* son=3 */
							{
								ssn++;
								son = 0;
							}
							else
								son++;
						}
					}
				}
				else
				{
					/* If sbc is 1024 or less, copy it at a time */
					slog_adr = (HFC_PAGE_SIZE * ssn)+(usSlogLen * son) ;
					if( COPYOUT( &ap->slog[slog_adr], (uchar *)(ulong)diag->addr, sbc ) )
					{	/* FCLNX-GPL-229 */
						goto copyout_err;
					}
				}
			}
		}

		/* Normal end without outputting data to buffer when ssn/son/sbc check is error  */
	}

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Lock release of mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */


	HFC_EXIT("mih_log") ;

	return( rtn );
	
copyout_err:	/* FCLNX-GPL-229 start */

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Lock release of mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */

	errlog_no = 0x24;
	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
	
	HFC_EXIT("mih_log") ;
	
	return ( EFAULT );	/* FCLNX-GPL-229 end */
}


/*
 * Function:    load_ch_trace_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Load CH Trace Log)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int load_ch_trace_log(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	logaddr_list *loglist;			/* Log address list virtual address */
	dma_addr_t	logaddr_busaddr;			/* Log address list bus address */
	char	*trc_log_area;					/* Log buffer area virtual address */
	dma_addr_t	trclog_busaddr;				/* Log buffer area bus address */

	char	*work_adr;						/* work pointer address */
	ushort	uswork;							/* work */

	int		i,j,k,rtn;
	int		loop_cnt;

	uchar	errlog_no ;
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */

	HFC_ENTRY("load_ch_trace_log") ;
	/*--------------------------------------------------------------*
	 * Log area specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unallocated ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_LDCHTRC, 0x00 );
		return( EINVAL );
	}
	if( diag->length != LOG_LENGTH_LDCH ) {	/* Is length 32K bytes ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_LDCHTRC, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Allocate logout list area 
	 *--------------------------------------------------------------*/

	loglist = (struct logaddr_list *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf,
		(uint)sizeof(struct logaddr_list), &logaddr_busaddr);

	if (loglist == NULL) {
		errlog_no = 0x05 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)loglist, (uint)sizeof(struct logaddr_list));

	/*--------------------------------------------------------------*
	 * Allocate log buffer area
	 *--------------------------------------------------------------*/
	/* Internal partitioning & page mapping */
	trc_log_area = (uchar *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf,
		LOG_LENGTH_LDCH, &trclog_busaddr);

	if (trc_log_area == NULL) {
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);

		errlog_no = 0x06 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)trc_log_area, LOG_LENGTH_LDCH);

	/*--------------------------------------------------------------*
	 * Set up log addresses to log list 
	 *--------------------------------------------------------------*/
	loglist->segnum = LOG_SEGNUM_LDCH;		/* Total number of segments */

	k = 0 ;
	loop_cnt = 0 ;

	if ( LOG_LENGTH_LDCH % HFC_PAGE_SIZE ) {
		/* If residue exists, loop number is (Page number + 1) */
		loop_cnt = (LOG_LENGTH_LDCH / HFC_PAGE_SIZE) + 1 ;
	}
	else {
		/* If no residue exists, loop number is page number */
		loop_cnt = LOG_LENGTH_LDCH / HFC_PAGE_SIZE ;
	}

	/* Set bus address to log list */
	for ( j = 0 ; j < loop_cnt ; j++) {
		hfc_write_val(loglist->bus_addr[k], (uint64_t)(trclog_busaddr + HFC_PAGE_SIZE*j) );

		if ( k >= LOG_SEGNUM_LDCH ) {	
			hfc_pci_free_consistent(ap, ap->pci_cfginf,
				(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
			hfc_pci_free_consistent(ap, ap->pci_cfginf,
				LOG_LENGTH_LDCH, (void *)trc_log_area, trclog_busaddr);

			return ( EIO );
		}

		k++ ;
	}

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock mailbox */

	/* Set up mailbox control block  */
	ap->mb-> mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	ap->mb-> mb_init.sub_cmd = HFC_MBSCMD_LDCHTRC;	/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );
	hfc_write_val( ap->mb->mb_init.type.drvlogb1.log_list_addr,
			(uint64_t)logaddr_busaddr );/* Bus address of log address list */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if ((rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry)) != 0) {	/* FCLNX-0523 */
		/* Error */
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			LOG_LENGTH_LDCH, (void *)trc_log_area, trclog_busaddr);

		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_LDCHTRC, 0x00 );

		spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
		unlock_mailbox( ap );	/* Unlock mailbox */
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */
		return ( rtn );
	}

	/* Set trace pointer */
	work_adr = (char *)&ap->mb->mb_resp.type.drvlogb0.mih_log;
	BCOPY( (char *)(work_adr + FPP_TRC_SIZE_OFFSET),
		(char *)&diag->uni.ch_trc_log.trc_ptr[0], FPP_TRC_SIZE );
	for( i = 0; i < 4; i++ ){
		uswork = (ushort)hfc_read_val( diag->uni.ch_trc_log.trc_ptr[i].sp );
		diag->uni.ch_trc_log.trc_ptr[i].sp = uswork;
		uswork = (ushort)hfc_read_val( diag->uni.ch_trc_log.trc_ptr[i].ep );
		diag->uni.ch_trc_log.trc_ptr[i].ep = uswork;
		uswork = (ushort)hfc_read_val( diag->uni.ch_trc_log.trc_ptr[i].cp );
		diag->uni.ch_trc_log.trc_ptr[i].cp = uswork;
	}

	BCOPY( (char *)(work_adr + FRM_TRC_SIZE_OFFSET),
		(char *)&diag->uni.ch_trc_log.frm_trc_ptr, FRM_TRC_SIZE );
	uswork = (ushort)hfc_read_val( diag->uni.ch_trc_log.frm_trc_ptr.sp );
	diag->uni.ch_trc_log.frm_trc_ptr.sp = uswork;
	uswork = (ushort)hfc_read_val( diag->uni.ch_trc_log.frm_trc_ptr.ep );
	diag->uni.ch_trc_log.frm_trc_ptr.ep = uswork;
	uswork = (ushort)hfc_read_val( diag->uni.ch_trc_log.frm_trc_ptr.cp );
	diag->uni.ch_trc_log.frm_trc_ptr.cp = uswork;

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */

	/*--------------------------------------------------------------*
	 * Data copy 
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)trc_log_area, (char *)(ulong)diag->addr, LOG_LENGTH_LDCH ) != 0 ) {

		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			LOG_LENGTH_LDCH, (void *)trc_log_area, trclog_busaddr);

		errlog_no = 0x0d ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	hfc_pci_free_consistent(ap, ap->pci_cfginf,
		(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
	hfc_pci_free_consistent(ap, ap->pci_cfginf,
		LOG_LENGTH_LDCH, (void *)trc_log_area, trclog_busaddr);

	HFC_EXIT("load_ch_trace_log");
	return( rtn );

}


/*
 * Function:    meint_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Send MEINT Log) 
 *
 * Arguments:   
 *  ap         - struct adap_info 
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
static __maybe_unused int meint_log(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	logaddr_list *loglist;			/* Log address list virtual address */
	dma_addr_t	logaddr_busaddr;			/* Log address list bus address */
	char	*trc_log_area;					/* Log buffer area virtual address */
	dma_addr_t	logout_busaddr;				/* Log buffer area bus address */

	int		rtn;

	uchar	errlog_no ;
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */

	HFC_ENTRY("meint_log") ;

	/*--------------------------------------------------------------*
	 * Log area specification check (one segment?)
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	 /* Is virtual address unallocated ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MEINTLOG, 0x00 );
		return( EINVAL );
	}
	if( diag->length != LOG_LENGTH_MEINT ) {/* Is length 1.5K bytes ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MEINTLOG, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Allocate logout list area
	 *--------------------------------------------------------------*/
	
	loglist = (struct logaddr_list *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf,
		(uint)sizeof(struct logaddr_list), &logaddr_busaddr);

	if (loglist == NULL) {
		errlog_no = 0x0e ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)loglist, (uint)sizeof(struct logaddr_list));

	/*--------------------------------------------------------------*
	 * Allocate log buffer area
	 *--------------------------------------------------------------*/

	trc_log_area = (uchar *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf,
		LOG_LENGTH_MEINT, &logout_busaddr);

	if (trc_log_area == NULL) {
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);

		errlog_no = 0x11 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)trc_log_area, LOG_LENGTH_MEINT);

	/*--------------------------------------------------------------*
	 * Set up log addresses to log list
	 *--------------------------------------------------------------*/
	loglist->segnum = LOG_SEGNUM_MEINT;		/* Number of segments */
	hfc_write_val(loglist->bus_addr[0], (uint64_t)logout_busaddr );

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock mailbox */

	/* Set up mailbox control block  */
	ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_SMEINT;	/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );
	hfc_write_val( ap->mb->mb_init.type.drvlogb1.log_list_addr,
			(uint64_t)logaddr_busaddr );/* Bus address of log address list */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if ((rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry)) != 0) {	/* FCLNX-0523 */
		/* Error */
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			LOG_LENGTH_MEINT, (void *)trc_log_area, logout_busaddr);

		spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
		unlock_mailbox( ap );	/* Unlock mailbox */
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */
		return ( rtn );
	}

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */

	/*--------------------------------------------------------------*
	 * Data copy 
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)trc_log_area, (char *)(ulong)diag->addr, LOG_LENGTH_MEINT ) != 0 ) {

		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			LOG_LENGTH_MEINT, (void *)trc_log_area, logout_busaddr);

		errlog_no = 0x14 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	hfc_pci_free_consistent(ap, ap->pci_cfginf,
		(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
	hfc_pci_free_consistent(ap, ap->pci_cfginf,
		LOG_LENGTH_MEINT, (void *)trc_log_area, logout_busaddr);

	HFC_EXIT("meint_log");
	return( rtn );
}


/*
 * Function:    forced_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Forced soft LOG)
 *
 * Arguments:   
 *  ap         - struct adap_info 
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int forced_log(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	logaddr_list *loglist;			/* Log address list virtual address */
	dma_addr_t	logaddr_busaddr;			/* Log address list bus address */
	char	*trc_log_area;					/* Log buffer area virtual address */
	dma_addr_t	trclog_busaddr;				/* Log buffer area bus address */

	int		j,k,rtn;
	int		loop_cnt;

	uchar	errlog_no ;
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */


	HFC_ENTRY("forced_log") ;

	/*--------------------------------------------------------------*
	 * Log area specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unallocated? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FRCLOG, 0x00 );
		return( EINVAL );
	}
	if( diag->length != LOG_LENGTH_SOFT ) {	/* Is length 72K bytes ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FRCLOG, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Allocate logout list area
	 *--------------------------------------------------------------*/

	loglist = (struct logaddr_list *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf,
		(uint)sizeof(struct logaddr_list), &logaddr_busaddr);

	if (loglist == NULL) {
		errlog_no = 0x05 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)loglist, (uint)sizeof(struct logaddr_list));

	/*--------------------------------------------------------------*
	 *  Allocate log buffer area
	 *--------------------------------------------------------------*/

	trc_log_area = (uchar *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf,
		LOG_LENGTH_SOFT, &trclog_busaddr);

	if (trc_log_area == NULL) {
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);

		errlog_no = 0x08 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)trc_log_area, LOG_LENGTH_SOFT);

	/*--------------------------------------------------------------*
	 * Set up log addresses to log list
	 *--------------------------------------------------------------*/
	loglist->segnum = LOG_SEGNUM_SOFT;		/* Number of segments */

	k = 0 ;
	loop_cnt = 0 ;

	if ( LOG_LENGTH_SOFT % HFC_PAGE_SIZE ) {
		/* If residue exists, loop number is (Page number + 1) */
		loop_cnt = (LOG_LENGTH_SOFT / HFC_PAGE_SIZE) + 1 ;
	}
	else {
		/* If no residue exists, loop number is page number */
		loop_cnt = LOG_LENGTH_SOFT / HFC_PAGE_SIZE ;
	}

	 /* Set bus address to log list */
	for ( j = 0 ; j < loop_cnt ; j++) {
		hfc_write_val(loglist->bus_addr[k], (uint64_t)(trclog_busaddr + HFC_PAGE_SIZE*j ) );

		if ( k >= LOG_SEGNUM_SOFT ) {	/* Set number(It must not be usually) */
			hfc_pci_free_consistent(ap, ap->pci_cfginf,
				(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
			hfc_pci_free_consistent(ap, ap->pci_cfginf,
				LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

			return ( EIO );
		}

		k++ ;
	}

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock of mailbox */

	/* The mailbox control block is assembled according to DRVLOGB1 */
	ap->mb-> mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	ap->mb-> mb_init.sub_cmd = HFC_MBSCMD_FORCSLOG;	/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, diag->uni.forced_log.dependent_code );
	hfc_write_val( ap->mb->mb_init.type.drvlogb1.log_list_addr,
			(uint64_t)logaddr_busaddr );/* Bus address of log address list */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if ((rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry)) != 0) {	/* FCLNX-0524 */
		/* Error case */
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

		spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
		unlock_mailbox( ap );	/* Unlock mailbox */
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */
		return ( rtn );
	}

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/*  Unlock mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */


	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)trc_log_area, (char *)(ulong)diag->addr, LOG_LENGTH_SOFT ) != 0 ) {

		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf,
			LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

		errlog_no = 0x0d ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	hfc_pci_free_consistent(ap, ap->pci_cfginf,
		(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
	hfc_pci_free_consistent(ap, ap->pci_cfginf,
		LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

	HFC_EXIT("forced_log");

	return( rtn );
}


/*
 * Function:    fw_post
 *
 * Purpose:     Processing of subcommand (F/W POST) of ioctl(HFCDIAG0)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fw_post(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int		rtn;

	HFC_ENTRY("fw_post");

	/*--------------------------------------------------------------*
	 * Parameter check (command check)
	 *--------------------------------------------------------------*/
	if( (diag->uni.post.cmd == POST_CMD_IMEM)	||		/* Internal RAM test */
	    (diag->uni.post.cmd == POST_CMD_ILOOP)	||		/* Internal LOOP test */
	    (diag->uni.post.cmd == POST_CMD_DMA)	||		/* DMA test */
	    (diag->uni.post.cmd == POST_CMD_ELOOP)  ||		/* External LOOP test */
	    (diag->uni.post.cmd == POST_CMD_BOOT)  ||		/* BOOT test */
	    (diag->uni.post.cmd == POST_CMD_BOOT_MLT)  ||	/* Multi BOOT test */
	    (diag->uni.post.cmd == POST_CMD_ILOOP_MLT)  ||	/* Multi port internal LOOP test */
	    (diag->uni.post.cmd == POST_CMD_ELOOP_MLT)) {	/* Multi port external LOOP test */
	}
	else {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWPOST, 0x00 );
		return ( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock mailbox */

	/* The mailbox control block is assembled according to DRVLOGB0 */
	ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;				/* Command */
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_DIAG;					/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, (ushort)diag->uni.post.cmd );/* Test number */

	/* Set up mailbox control block  */ /* FCLNX-GPL-243 */
	if (( rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry) ) == 0 ){
		/* Copy RSLT information to internal DIAG area */
		diag->uni.post.reslt = ap->mb->mb_resp.type.drvlogb0.diag.reslt;
		/* Set error code */
		diag->uni.post.err_code[0] = ap->mb->mb_resp.type.drvlogb0.diag.resv0[0];
		diag->uni.post.err_code[1] = ap->mb->mb_resp.type.drvlogb0.diag.resv0[1];
		diag->uni.post.err_code[2] = ap->mb->mb_resp.type.drvlogb0.diag.resv0[2];
		/* IMLLOG output (POST ERROR) */
		if ( ( diag->uni.post.reslt == 0xff )||( (diag->uni.post.reslt & 0xf0) == 0x90 ) ){  
			HFC_ADAP_LOCK(ap->mp_adap_info,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-111 */
			hfc_logout(ap, (uint)0x77, HFC_ERRLOG_TYPE_IMLLOG);
			HFC_ADAP_UNLOCK(ap->mp_adap_info,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-111 */

			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRC, 0x77, NULL, 0);
		}
	}

	unlock_mailbox( ap );	/* Lock release of mailbox */

	/* Output IMLLOG (Timeout case) */
	if (rtn == ETIMEDOUT ){
		HFC_ADAP_LOCK(ap->mp_adap_info,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-111 */
		hfc_logout(ap, (uint)0x78, HFC_ERRLOG_TYPE_IMLLOG);
		HFC_ADAP_UNLOCK(ap->mp_adap_info,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-111 */

		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRC, 0x78, NULL, 0);
	}

	HFC_EXIT("fw_post") ;

	return( rtn );
}


/*
 * Function:    fw_start
 *
 * Purpose:     Processing for ioctl(HFCDIAG0) subcommand (F/W Start) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fw_start(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uint i , lp ;
	int rc=0;
	int	lun_no;
	ulong	flags = 0 ;
	struct adap_info 	*ap_work ;
	struct target_info 	*target_work ;
	struct	mp_adap_info	*mpap; 	/* struct mp_adap_info */
	uint	dummy_read_reg=0;
	
	HFC_ENTRY("fw_start");

	mpap = ap->mp_adap_info;

	/*--------------------------------------------------------------*
	 * Parameter check (flag)
	 *--------------------------------------------------------------*/
	if ( ap->pkg.type == HFC_PKTYPE_FPP )
	{
		if( (diag->uni.fw_start.mode != FW_MODE_NORMAL)		&&
		    (diag->uni.fw_start.mode != FW_MODE_DIAG)		&&
	    	(diag->uni.fw_start.mode != FW_MODE_NAI_LOOP)	&&
	    	(diag->uni.fw_start.mode != FW_MODE_GAI_LOOP) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWSTART, 0x00 );
			return ( EINVAL );
		}
	}
	else /* FIVE,FIVE-EX */
	{
		if( (diag->uni.fw_start.mode != FW_MODE_NORMAL)	&&
		    (diag->uni.fw_start.mode != FW_MODE_DIAG) )	
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWSTART, 0x01 );
			return ( EINVAL );
		}
	}
	
	if( ap->fcp_mode != FW_MODE_NORMAL )
	{
		if( diag->uni.fw_start.mode != FW_MODE_NORMAL )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWSTART, 0x02 );
			return ( EINVAL );
		}
	}
	if( diag->uni.fw_start.mode != FW_MODE_NORMAL )
	{

		HFC_ADAP_LOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
		if( (test_bit(HFC_DIAG_PROGRESS, (ulong *)&mpap->status))||
			(test_bit(HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock)) )  /* FCLNX-GPL-177 */
		{

			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWSTART, 0x03 );
			HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
			return ( EBUSY ) ;
		}
		ap_work = mpap->ap ; /* FCLNX-GPL-177 */
		while( ap_work != NULL )
		{
			spin_lock_irqsave(&ap_work->adap_lock,flags) ;
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target_work = ap_work->target_arg[lp] ;
				while( target_work != NULL )
				{
					if( !test_bit(HFC_TARGETINF_VALID,(ulong *)&target_work->flags) )
					{
						target_work = target_work->next ;
						continue ;
					}
					if( (target_work->we_que_cnt != 0) || (target_work->wx_que_cnt != 0) )
					{
						HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
											HFC_TRC_IOCTL_FWSTART, 0x04 );
						HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
						spin_unlock_irqrestore( &ap_work->adap_lock,flags ) ;
						return ( EBUSY ) ;
					}
					target_work = target_work->next ;
				}
			}

			while( test_bit( HFC_MAILBOX_BUSY, (ulong *)&( ap_work -> mb_lock ) ) )
			{
				set_bit(HFC_DIAG_DELAY, (ulong *)&ap_work->status );
				/* 3s wait during processing mailbox */
				hfc_w_stop(ap_work,HFC_DIAG_DELAY_TMR) ;   
				hfc_w_start(ap_work,HFC_DIAG_DELAY_TMR) ;
				spin_unlock_irqrestore(&ap_work->adap_lock,flags) ;

				hfc_sleep_on(&ap->mb_event, &ap->mb_event_wait);					/* FCLNX-0269 */
				clear_bit(HFC_DIAG_DELAY, (ulong *)&ap_work->status );
				spin_lock_irqsave(&ap_work->adap_lock,flags) ;
			}

			clear_bit( HFC_ONLINE, (ulong *)&ap_work -> status );

			spin_unlock_irqrestore(&ap_work->adap_lock,flags) ;
			ap_work = ap_work->next ;
		}
		set_bit(HFC_DIAG_PROGRESS , (ulong *)&mpap->status) ; /* FCLNX-GPL-177 */
		HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
	}

	switch (diag->uni.fw_start.mode)
	{
		case  FW_MODE_DIAG :
		case  FW_MODE_NAI_LOOP :	
		case  FW_MODE_GAI_LOOP :	
			/* Initiate boot microprogram and check POST result */
			if ( ap->pkg.type == HFC_PKTYPE_FPP )
			{
				mdelay(10);

				/* Set flag in INI_RST */									/* FCLNX-282 STR */
				HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
				set_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );		/* FCLNX-282 END */

				/* Start STOP_FUNCTION */
				hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x50);
				/* Wait 10ms */
				mdelay(10);

				hfc_reset_start(ap,HFC_INI_RESET);
				mdelay(5); 	/* Wait 5ms */

				/* Reset flag in INI_RST */									/* FCLNX-282 STR*/
				clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );
/*				HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);*/ /* FCLNX-282 END*/ /* FCLNX-GPL-177 */

				clear_bit( HFC_MAILBOX_BUSY, (ulong *)&( ap -> mb_lock ) );
				hfc_reset_start(ap,HFC_WSCA_CLEAR);

				clear_bit(HFC_CHK_STOP , (ulong *)&mpap->status) ; /* FCLNX-GPL-177 */
				HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */

				hfc_reset_start(ap,HFC_REBOOT);

				/* 500ms wait */
				i = 0;
				mdelay(500);
				
				/* Check POST result (no retry) */
				rc = hfc_config_hw_set(ap,0);
			}
			else /* FIVE, FIVE-EX */
			{
				mdelay(10);

				/* Set flag in CTL_RST */									/* FCLNX-282 STR*/
				HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
				set_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );		/* FCLNX-282 END*/
				
				/* Close INT_A mask */ /* FCLNX-GPL-257 */
				hfc_write_reg(ap, HFC_IOSPACE_INTA_MSK, PCI_LENGTH_04, (uint)0x00000000 );
				
				/* Cancel pending SCSI commands, and stop timers. */
				hfc_mck_prepare( ap );
				
				/* Start STOP_FUNCTION */
				hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x50);
				/* Wait 10ms */
				mdelay(10);
				
				/* for only FIVE-EX */ /* FCLNX-GPL-220 */ /* FCLNX-GPL-240 */
				if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
				{
					/* Set MCW */ /* FCLNX-GPL-242 */
					/* -> We set PCI target T.O timer to 3-4msec. */
					/*    And, the timer will be stop when it's flash rom accessing. */
					hfc_write_reg_ext(ap, 0x210, 0x1, 0xb0);
					/* Clear GR0-F */
					hfc_reset_start( ap, HFC_GR_CLEAR );
				}
				
				hfc_reset_start(ap, HFC_CTLRST);
				if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
				{
					/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
					dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);
				}
				
				mdelay(10); 	/* Wait 10ms */

				/* Reset flag in CTL_RST */									/* FCLNX-282 STR*/
				clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );

				ap_work = mpap->ap ; /* FCLNX-GPL-177 */
				while( ap_work != NULL )
				{
					clear_bit( HFC_MAILBOX_BUSY, (ulong *)&( ap_work -> mb_lock ) );
					ap_work = ap_work->next;
				}
				
				/*** Reset PCI space to clear INT_A register ***/  /* FCLNX-GPL-257 */
				hfc_write_reg(ap, HFC_IOSPACE_INTA_RST, PCI_LENGTH_04, (uint)0xffffffff);
				
				hfc_reset_start(ap,HFC_WSCA_CLEAR);

				clear_bit(HFC_CHK_STOP , (ulong *)&mpap->status) ; /* FCLNX-GPL-177 */
				HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-177 */

				hfc_reset_start(ap,HFC_REBOOT);

				/* Wait 2s */
				i = 0;

				set_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
				/* Wait 2s */
				hfc_w_stop(ap,HFC_DIAG_DELAY_TMR) ;
				hfc_w_start(ap,HFC_DIAG_DELAY_TMR) ;
				/* Wait timeout for completion of DIAG process */
				hfc_sleep_on(&ap->mb_event, &ap->mb_event_wait);				/* FCLNX-0296 */
				clear_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
					
				/* Check POST result (no retry) */
				if(ap->pkg.type == HFC_PKTYPE_FIVE)
				{
					rc = hfc_config_hw_set_five(ap,0);
				}
				else /* FIVE-EX */
				{
					rc = hfc_config_hw_set_five_ex(ap,0);
				}
				ap->diag_cnt++;													/* FCLNX-293 END*/
			}

			/* POST result check */
			if( rc != 0 ) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_FWSTART, 0x01 );
				return ( EIO );
			}
			else
			{
				hfc_reset_start(ap,HFC_SET_INIADR);			/* INITTBL ADR(0x310)  */
															/* ALPA(0x319)         */
				hfc_write_reg(ap, HFC_IOSPACE_CA_FLAG, PCI_LENGTH_01, diag->uni.fw_start.mode);
				/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
				dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);
				
				hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, PCI_LENGTH_04, HFC_FRAMEA_FW_START);
				/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
				dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);

				/* Reconfigure interrupt mask */
				hfc_write_reg( ap, HFC_IOSPACE_INTA_MSK,PCI_LENGTH_04, hfc_inta_mask[ap->pkg.type] );
			}
			if( (diag->uni.fw_start.mode == FW_MODE_NAI_LOOP)	||
			    (diag->uni.fw_start.mode == FW_MODE_GAI_LOOP) )
			{
				for ( lun_no = 0 ; lun_no < MAX_LOOP_LUN ; lun_no++ )
				{
					ap->loop_dev_info[lun_no].flag |= HFC_LOOP_EXEC;		/* LOOP test start */
				}
			}
			break ;

		case  FW_MODE_NORMAL :
			if (diag->uni.fw_start.err_code == FW_DIAG_FAILED)
			{
				spin_lock_irqsave( &ap->adap_lock,flags );
				/* Check stop if DIAG fails */
				HFC_ISSUE_CSTP( ap, FALSE, HFC_ABEND_FCSTP ) ;				/* FCLNX-0279 *//* @MLPF *//* FCLNX-GPL-316 */
				/* Return to previous interruption level */
				spin_unlock_irqrestore( &ap->adap_lock,flags );

				/* Clear error code */
				diag->uni.fw_start.err_code = 0x00 ;

				return( 0 );
			}
			else
			{
				if ( ap->pkg.type == HFC_PKTYPE_FPP )
				{
					HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-177 */
					ap_work = mpap->ap ; /* FCLNX-GPL-177 */
					while( ap_work != NULL )
					{
						hfc_reset_adap_info(ap_work) ;
						ap_work = ap_work->next ;
					}

					/* Set flag in INI_RST */									/* FCLNX-282 STR*/
/*					HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);*/ /* FCLNX-GPL-177 */
					set_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );		/* FCLNX-282 END*/

					/* Initiate STOP_FUNCTION  */
					hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x50);
					/* 1ms wait */
					mdelay(1);

					hfc_reset_start(ap,HFC_INI_RESET);
					hfc_reset_start(ap,HFC_WSCA_CLEAR);
					hfc_reset_start(ap,HFC_SET_WS04);			/* WS Comu Area(0x318) */
					hfc_reset_start(ap,HFC_SET_INIADR);			/* INITTBL ADR(0x310)  */
																/* ALPA(0x319)         */
					/* Reset flag in INI_RST */									/* FCLNX-282 STR*/
					clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );
					HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);						/* FCLNX-282 END*/

					hfc_reset_start(ap,HFC_REBOOT);
				
					/* Wait 500ms */
					mdelay(500);	/* FCLNX_0012 */
		
					/* Check POST result (no retry) */
					if( hfc_config_hw_set(ap,0) )
					{
						/*-- Init fail --*/
						HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
								HFC_TRC_IOCTL_FWSTART, 0x01 );
						return ( EIO );
					}
					else
					{
						HFC_ADAP_LOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 start */
						clear_bit(HFC_DIAG_PROGRESS , (ulong *)&mpap->status);
/*						HFC_ADAP_UNLOCK(ap->mp_adap_info , HFC_MP_ADAP_BUSY) ;*/
						ap_work = mpap->ap ; /* FCLNX-GPL-177 end */
						while( ap_work != NULL )
						{
							set_bit(HFC_WAIT_LINKUP,(ulong *)&ap_work->status) ;
							set_bit(HFC_DIAG_END,(ulong *)&ap_work->status) ;
							hfc_inta_mask_set(ap_work, hfc_inta_mask[ap->pkg.type]) ;
							ap_work = ap_work->next ;
						}
						HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
					}

					for ( lun_no = 0 ; lun_no < MAX_LOOP_LUN ; lun_no++ ){
						if( ap->loop_dev_info[lun_no].flag & HFC_LOOP_EXEC) {
							ap -> loop_dev_info[lun_no].flag &= ~(HFC_LOOP_EXEC);	/* LOOP test end */
						}
					}
				}
				else /* FIVE, FIVE-EX */
				{
					if( test_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status ) ) /* FCLNX-GPL-177 */
					{
						return(0);
					}
					
					/* Set flag in CTL_RST */										/* FCLNX-282 STR*/
					HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
					set_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );			/* FCLNX-282 END*/
					
					/* Close INT_A mask */ /* FCLNX-GPL-257 */
					hfc_write_reg(ap, HFC_IOSPACE_INTA_MSK, PCI_LENGTH_04, (uint)0x00000000 );
					
					/* Initiate STOP_FUNCTION */
					hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x50);
					mdelay(1); 	/* 1ms wait */
					
					/* for only FIVE-EX */ /* FCLNX-GPL-220 */ /* FCLNX-GPL-240 */
					if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
					{
						/* Set MCW */ /* FCLNX-GPL-242 */
						/* -> We set PCI target T.O timer to 3-4msec(default). */
						/* hfc_write_reg_ext(ap, 0x210, 0x1, 0x80); */
						/* Clear GR0-F */
						hfc_reset_start( ap, HFC_GR_CLEAR );
					}
					
					hfc_reset_start(ap, HFC_CTLRST);

					if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
					{
						/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
						dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);
					}

					mdelay(1); 	/* 1ms wait */

					/* Reset flag in CTL_RST */										/* FCLNX-282 STR*/
					clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );
					HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-282 END*/
					
					/*** Reset PCI space to clear INT_A register ***/  /* FCLNX-GPL-257 */
					hfc_write_reg(ap, HFC_IOSPACE_INTA_RST, PCI_LENGTH_04, (uint)0xffffffff);
					
					hfc_reset_start(ap, HFC_WSCA_CLEAR);
					hfc_reset_adap_info(ap) ;
					hfc_reset_start(ap, HFC_REBOOT);

					set_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
					/* 2s wait */
					hfc_w_stop(ap,HFC_DIAG_DELAY_TMR) ;
					hfc_w_start(ap,HFC_DIAG_DELAY_TMR) ;
					/* Wait timeout for completion of DIAG process */
					hfc_sleep_on(&ap->mb_event, &ap->mb_event_wait );				/* FCLNX-0269 */

					clear_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
					
					if(ap->pkg.type == HFC_PKTYPE_FIVE)
					{
						rc = hfc_config_hw_set_five(ap,0);
					}
					else /* FIVE-EX */
					{
						rc = hfc_config_hw_set_five_ex(ap,0);
					}
					
					if(rc)
					{
						/*-- init fail --*/
						HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
								HFC_TRC_IOCTL_FWSTART, 0x21 );
						return ( EIO );
					}
					else
					{
						HFC_ADAP_LOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
						clear_bit(HFC_DIAG_PROGRESS , (ulong *)&mpap->status); /* FCLNX-GPL-177 */
						HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
						/* WS Comu Area(0x318) */
						hfc_reset_start(ap, HFC_SET_WS80);
						/* INITTBL ADR(0x310)  */
						/* ALPA(0x319)         */
						hfc_reset_start(ap, HFC_SET_INIADR);
						
						hfc_reset_start(ap, HFC_FW_START);
						/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
						dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);
						
						set_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status );
						set_bit( HFC_DIAG_END, (ulong *)&ap->status );
						/* Set interrupt mask */
						hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask[ap->pkg.type]);
					}
					HFC_ADAP_LOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */
					ap_work = mpap->ap ; /* FCLNX-GPL-177 */
					while( ap_work != NULL )
					{
						if( ap_work != ap )
							break;
						
						ap_work = ap_work->next ;
					}
					HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; /* FCLNX-GPL-177 */

					if( ap_work != NULL )
					{
						/* WS Comu Area(0x318) */
						hfc_reset_start(ap_work, HFC_SET_WS80);
						/* INITTBL ADR(0x310)  */
						/* ALPA(0x319)         */
						hfc_reset_start(ap_work, HFC_SET_INIADR);
						hfc_reset_start(ap_work, HFC_FW_START);                      /* FCWIN-0237 */
						/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
						dummy_read_reg = (uint)hfc_read_reg(ap_work, (uint)HFC_IOSPACE_ZERO, (char)0x4);
						
						set_bit( HFC_WAIT_LINKUP, (ulong *)&ap_work->status );
						set_bit( HFC_DIAG_END, (ulong *)&ap_work->status );
						/* Set interrupt mask */
						hfc_write_reg(ap_work,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask[ap->pkg.type]);
					}
				}
				set_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
				hfc_w_stop(ap,HFC_DIAG_DELAY_TMR) ;
				hfc_w_start(ap,HFC_DIAG_DELAY_TMR) ;
				/* Wait timeout for completion of DIAG process */
				hfc_sleep_on(&ap->mb_event, &ap->mb_event_wait );					/* FCLNX-0269 */
				clear_bit(HFC_DIAG_DELAY, (ulong *)&ap->status );
			}
			break ;

	}	/*-- end of switch --*/
	HFC_EXIT("fw_start");

	return( 0 );
}


/*
 * Function:    stop_func
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Stop_Function) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *             - 0
 * Notes:       
 */
int stop_func(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	HFC_ENTRY("stop_func") ;

	/* Initiste STOP_FUNCTION  */
	hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x40);

	/* Drop ONLINE bit to prevent from initiating SCSI and ioctl */
	clear_bit(HFC_ONLINE,(ulong *)&ap->status) ;

	HFC_EXIT("stop_func") ;
	return( 0 );
}


																					/* FIVE STR */
/*
 * Function:    init_mode_set
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (INIT MODE SET)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl 
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int init_mode_set(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int rtn = 0;
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */

	HFC_ENTRY("init_mode_set") ;

	/*--------------------------------------------------------------*
	 * Lock Mailbox 
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock of mailbox */

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	/* Setup mailbox control block */
	ap->mb->mb_init.command = HFC_MBCMD_FPPCTL;						/* Command */
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_INITMDSET;					/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );		/* Test number */

	ap->mb->mb_init.type.drvctl.un.init_mode_set.mode[0] = diag->uni.init_mode_set.mode_alpa.uc_mode[3];
	ap->mb->mb_init.type.drvctl.un.init_mode_set.mode[1] = diag->uni.init_mode_set.mode_alpa.uc_mode[2];
	ap->mb->mb_init.type.drvctl.un.init_mode_set.mode[2] = diag->uni.init_mode_set.mode_alpa.uc_mode[1];
	ap->mb->mb_init.type.drvctl.un.init_mode_set.al_pa = 0;
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[0] = (uchar)'H';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[1] = (uchar)'I';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[2] = (uchar)'T';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[3] = (uchar)'A';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[4] = (uchar)'C';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[5] = (uchar)'H';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[6] = (uchar)'I';
	ap->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[7] = (uchar)' ';
	if( ( ( diag->uni.init_mode_set.mode_alpa.mode >> 8 ) & 0x00FFFFFF ) != HFC_INITMDSET_FCP ){
		ap->mb->mb_init.type.drvctl.un.init_mode_set.al_pa = diag->uni.init_mode_set.mode_alpa.al_pa[0];
		hfc_write_val( ap->mb->mb_init.type.drvctl.un.init_mode_set.connect_type, diag->uni.init_mode_set.connect_type );
		hfc_write_val( ap->mb->mb_init.type.drvctl.un.init_mode_set.wwpn, diag->uni.init_mode_set.wwpn );
		hfc_write_val( ap->mb->mb_init.type.drvctl.un.init_mode_set.wwnn, diag->uni.init_mode_set.wwnn );

		hfc_write_val( ap->mb->mb_init.type.drvctl.un.init_mode_set.buf_size, diag->uni.init_mode_set.buf_size );
		hfc_write_val( ap->mb->mb_init.type.drvctl.un.init_mode_set.xrdy_div, diag->uni.init_mode_set.xrdy_div );
		hfc_write_val( ap->mb->mb_init.type.drvctl.un.init_mode_set.param, diag->uni.init_mode_set.param );
	}

	/* Start Mailbox process and wait for process completion */ /* FCLNX-GPL-243 */
	rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry);	/* FCLNX-0523 */

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */

	HFC_EXIT("init_mode_set") ;
	return( rtn );

}	/* end of init_mode_set */


/*
 * Function:    fcp_mode_set
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (FCP MODE SET)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fcp_mode_set(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int rtn = 0;
	ulong		flags=0;	/* FCLNX-GPL-FX-466 */

	HFC_ENTRY("fcp_mode_set") ;

	/*--------------------------------------------------------------*
	 * Lock Mailbox 
	 *--------------------------------------------------------------*/
	lock_mailbox( ap );		/* Lock of mailbox */

	/*--------------------------------------------------------------*
	 * Mailbox processing 
	 *--------------------------------------------------------------*/
	/* Setup mailbox control block */
	ap->mb->mb_init.command = HFC_MBCMD_FPPCTL;						/* Command */
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_FCPMDSET;					/* Sub-Command */
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );		/* Test no. */

	hfc_write_val( ap->mb->mb_init.type.drvctl.un.fcp_mode_set.act_ctl, diag->uni.fcp_mode_set.act_ctl );
	hfc_write_val( ap->mb->mb_init.type.drvctl.un.fcp_mode_set.il_ctl, diag->uni.fcp_mode_set.il_ctl );
	hfc_write_val( ap->mb->mb_init.type.drvctl.un.fcp_mode_set.seq_ctl, diag->uni.fcp_mode_set.seq_ctl );
	hfc_write_val( ap->mb->mb_init.type.drvctl.un.fcp_mode_set.rsp_delay, diag->uni.fcp_mode_set.rsp_delay );

	/* Start mailbox process and wait for completion */ /* FCLNX-GPL-243 */
	rtn = hfc_mailbox_proc(ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry);	/* FCLNX-0523 */

	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-FX-466 */

	HFC_EXIT("fcp_mode_set") ;
	return( rtn );

}	/* end of fcp_mode_set */
																					/* FIVE END */


/*==================================================================*
 * pci_access
 *
 *    func    : Process for ioctl(HFCDIAG0) subcommand (PCI Read)
 *            : Process for ioctl(HFCDIAG0) subcommand (PCI Write) 
 *
 *==================================================================*/
int pci_access(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	uint	wk_data;			/* Internal data area */
	ushort	wk2 ;
	uint	wk4 ;
	ulong	base_addr   = 0;		/* FCLNX-GPL-154 */
	uint	addr_max    = 0;		/* FCLNX-GPL-154 */
	int		addr_type   = 0;		/* FCLNX-GPL-154 */
	int		return_code = 0;		/* FCLNX-GPL-154 */

	HFC_ENTRY("pci_access");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	/* Virtual address check */
	if( !diag->addr ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x00 );
		return_code = EINVAL; /* FCLNX-GPL-154 */
		goto err_end;
	}
	
	/* PCI memory BAR# check */ /* FCLNX-GPL-154 start */
	base_addr = ap->mem_base_addr;	/* Default parm */
	addr_type = PCI_BAR0;			/* Default parm */
	if( ap->debug_func & HFC_DEBUG_RW_BAR1 )
	{	/* for Debug Mode */
		addr_type = diag->uni.pci.base_addr_type;
		switch( addr_type )
		{
			case PCI_BAR0:
				base_addr = ap->mem_base_addr;
				break;
				
			case PCI_BAR1:
				base_addr = hfc_remap_pci_bar(ap->pci_cfginf, 2);
				if( base_addr == 0x00 )
				{	/* mapping err */
					return_code = ENOMEM;
					goto err_end;
				}
				break;
				
			default: /* others */
				/* out of range */
				return_code = EINVAL;
				goto err_end;
		}
	}
	
	/* PCI memory space address check */ /* FCLNX-GPL-154 */
	addr_max = pci_addr_max[addr_type][ap->pkg.type];
	if( diag->uni.pci.addr > addr_max )
	{	/* out of range */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x01 );
		return_code = EINVAL; /* FCLNX-GPL-154 */
		goto err_end;
	}
	
	/* Access length check */
	if( (diag->length == PCI_LENGTH_01) ||
	    (diag->length == PCI_LENGTH_02) ||
	    (diag->length == PCI_LENGTH_04) ) {
	}
	else {	/* Invalid access length check */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x02 );
		return_code = EINVAL; /* FCLNX-GPL-154 */
		goto err_end;
	}
	/* Access over check */ /* FCLNX-GPL-154 */
	addr_max = pci_addr_max[addr_type][ap->pkg.type] +1;
	if( (diag->uni.pci.addr + diag->length) > addr_max )
	{	/* out of range */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x03 );
		return_code = EINVAL; /* FCLNX-GPL-154 */
		goto err_end;
	}

	if( diag->sub_cmd == HFC_DG_RDPCI ) {
	/*--------------------------------------------------------------*
	 * PCI Read
	 *--------------------------------------------------------------*/

		/* PCI memory read */
		wk_data = hfc_read_reg_ext2(ap, base_addr, diag->uni.pci.addr, diag->length) ;
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_read_val(wk2); break;
		}
		/* Copy read data to user area (wk_datad -> iag->addr) */
		if ( COPYOUT( (char *)&wk_data, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x04 );
			errlog_no = 0x2a ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return_code = EFAULT; /* FCLNX-GPL-154 */
			goto err_end;
		}
	}
	else {
	/*--------------------------------------------------------------*
	 * PCI Write
	 *--------------------------------------------------------------*/

		/* Copy write data to internal area (diag->addr -> wk_data) */
		wk_data = 0 ;
		if ( COPYIN( (char *)(ulong)diag->addr, (char *)&wk_data, diag->length )  != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x05 );
			errlog_no = 0x2b ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
			return_code = EFAULT; /* FCLNX-GPL-154 */
			goto err_end;
		}
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_read_val(wk2); break;
		}
		/* PCI memory write */
		hfc_write_reg_ext2(ap, base_addr, diag->uni.pci.addr, diag->length, wk_data);
	}
	
err_end: /* FCLNX-GPL-154 */
	
	/* PCI memory BAR# check */ /* FCLNX-GPL-154 start */
	if( ap->debug_func & HFC_DEBUG_RW_BAR1 )
	{	/* for Debug Mode */
		switch( diag->uni.pci.base_addr_type )
		{
			case PCI_BAR1:
				hfc_unmap_pci_bar(ap->pci_cfginf, base_addr);
				base_addr = 0x00;
				break;

			case PCI_BAR0:
			default: /* others */
				/* NOP */
				break;
		}
	} /* FCLNX-GPL-154 end */

	HFC_EXIT("pci_access");
	return( return_code );
}


/*==================================================================*
 * pci_cnf_access
 *
 *    func    : Process for ioctl(HFCDIAG0) subcommand (PCI Config Read) 
 *            : Process for ioctl(HFCDIAG0) subcommand (PCI Config Write) 
 *
 *==================================================================*/
int pci_cnf_access(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	uint	wk_data;			/* Internal data area */
	int		rtn = 0;

	HFC_ENTRY("pci_cnf_access" ) ;

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	/* Virtual address check */
	if( !diag->addr )
	{
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCICNFACC, 0x00 );
		return( EINVAL );
	}
	/* PCI config space address check */
	if( diag->uni.pci.addr > pci_cnf_addr_max[ap->pkg.type] ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCICNFACC, 0x01 );
		return( EINVAL );
	}
	/* Access length check */
	if( (diag->length == PCI_LENGTH_01) ||
	    (diag->length == PCI_LENGTH_02) ||
	    (diag->length == PCI_LENGTH_04) ) {
	}
	else {	/* Invalid access length check */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCICNFACC, 0x02 );
		return( EINVAL );
	}
	/* Access over check */
	if( (diag->uni.pci.addr + diag->length) > pci_cnf_addr_max[ap->pkg.type] +1 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCICNFACC, 0x03 );
		return( EINVAL );
	}

	if( diag->sub_cmd == HFC_DG_RDPCI_CFG ) {
	/*--------------------------------------------------------------*
	 * PCI Config Read
	 *--------------------------------------------------------------*/
		/* PCI config reading */
		wk_data = hfc_read_cnfg(ap, diag->uni.pci.addr, diag->length);
		wk_data = hfc_read_val(wk_data) ;
		if( diag->length == PCI_LENGTH_01 ) {
			wk_data <<= 24;
		}
		else if( diag->length == PCI_LENGTH_02 ) {
			wk_data <<= 16;
		}
		/* Copy read data to user area (wk_datad -> iag->addr) */
		if ( COPYOUT( (char *)&wk_data, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCICNFACC, 0x04 );
			errlog_no = 0x2c ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return ( EFAULT );
		}
	}
	else {
	/*--------------------------------------------------------------*
	 * PCI Config Write
	 *--------------------------------------------------------------*/
		/* Copy write data to internal area (diag->addr -> wk_data) */
		if ( COPYIN( (char *)(ulong)diag->addr, (char *)&wk_data, diag->length )  != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCICNFACC, 0x05 );
			errlog_no = 0x2d ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
			return ( EFAULT );
		}
		wk_data = hfc_read_val(wk_data) ;
		/* Shift write data */
		if( diag->length == PCI_LENGTH_01 ) {
			wk_data >>= 24;
		}
		else if( diag->length == PCI_LENGTH_02 ) {
			wk_data >>= 16;
		}
		/* PCI config write */
		hfc_write_cnfg(ap, diag->uni.pci.addr, diag->length, wk_data);
	}
	HFC_EXIT("pci_cnf_access" ) ;

	return( rtn );
}


/*
 * Function:    read_fw_init_tbl
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read fw_init_tbl) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    - 0
 *
 * Notes:       
 */
int read_fw_init_tbl(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_fw_init_tbl");

	/*--------------------------------------------------------------*
	 * Log area specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct fw_init_tbl) ) {	/* Is data size short for fw_init_tbl ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)ap->fw_init_p, (char *)(ulong)diag->addr, sizeof(struct fw_init_tbl) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x02 );
		errlog_no = 0x15 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_fw_init_tbl");

	return( 0 );
}


/*
 * Function:    read_xob
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read xob) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    - 0
 *
 * Notes:       
 */
int read_xob(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_xob") ;

	/*--------------------------------------------------------------*
	 * Specificaton check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct xob) ) {	/* Is data size short for xob ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x01 );
		return( EINVAL );
	}
	if( 
		(diag->uni.xob_xrb_scmd.no >= hfc_read_val(ap->fw_init_p->xob_num)) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number  */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x03 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(ap->xob + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct xob) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x04 );
		errlog_no = 0x16 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_xob");
	return( 0 );
}


/*
 * Function:    read_xrb
 *
 * Purpose:     Processing for ioctl(HFCDIAG0) subcommand (Read xrb) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    - 0
 *           
 * Notes:       
 */
int read_xrb(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_xrb");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {		/* Is virtual address unassigned ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct xrb) ) {	/* Is size short for xrb? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x01 );
		return( EINVAL );
	}
	if( (diag->uni.xob_xrb_scmd.no >= hfc_read_val(ap->fw_init_p->xrb_num)) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x03 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(ap->xrb + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct xrb) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x04 );
		errlog_no = 0x17 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_xrb");

	return( 0 );
}


/*
 * Function:    read_hfcpkt
 *
 * Purpose:     Process for ioctl(HFCDIAG0)subcommand (Read hfc_pkt)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0
 *
 * Notes:       2004.11.30
 */
int read_hfcpkt(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	char *ptr;

	HFC_ENTRY("read_hfc_pkt");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfc_pkt) ) {	/* Is size short for hfc_pkt? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x01 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.no >= ap->pkt_num ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x03 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	/* FCLNX-0521 */ /* FCLNX-GPL-166 */
	ptr = (char *)&ap->pkt_pool[diag->uni.xob_xrb_scmd.no / HFC_PKT_POOL_SIZE][ diag->uni.xob_xrb_scmd.no % HFC_PKT_POOL_SIZE];
	if ( COPYOUT( ptr, (char *)(ulong)diag->addr, sizeof(struct hfc_pkt) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x04 );
		errlog_no = 0x17 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_hfcpkt");

	return( 0 );
}


/*
 * Function:    read_seg_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read seg_info) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *             0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_seg_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int seg_cnt;				/* Number of segments */
	int seg_info_cnt;			/* DMA segment info array */

	uchar	errlog_no ;

	HFC_ENTRY("read_seg_info");

	seg_cnt = (ap->dma_max / HFC_PAGE_SIZE);
	seg_info_cnt = seg_cnt + seg_cnt / ( HFC_PAGE_SIZE / sizeof( struct seg_info ) );

	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {		/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct seg_info) ) {	/* Is size short for seg_info? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x01 );
		return( EINVAL );
	}
	if( (diag->uni.xob_xrb_scmd.no >= seg_info_cnt) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number  */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x03 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(ap->seg_info + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct seg_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x04 );
		errlog_no = 0x18 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_seg_info");
	return( 0 );
}


																					/* FIVE STR */
/*
 * Function:    read_mpadap_info
 *
 * Purpose:     Processing of subcommand (Read hfc_mpadap_info) of ioctl(HFCDIAG0)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_mpadap_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_mpadap_info");
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MPADAP, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct mp_adap_info) ) {	/* Is size short for mp_adap_info? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MPADAP, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)ap->mp_adap_info, (char *)(ulong)diag->addr, sizeof(struct mp_adap_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MPADAP, 0x02 );
		errlog_no = 0x1d ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_mpadap_info") ;
	return( 0 );
}	/* end of read_mpadap_info */
																					/* FIVE END */


/*
 * Function:    read_adap_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read adap_info)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_adap_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_adap_info") ;

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_APINFO, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct adap_info) ) {	/* Is size short for adap_info? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_APINFO, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)ap, (char *)(ulong)diag->addr, sizeof(struct adap_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_APINFO, 0x02 );
		errlog_no = 0x1a ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}
	HFC_EXIT("read_adap_info") ;

	return( 0 );
}


/*
 * Function:    read_target_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read target_info) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *    
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_target_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct target_info *target;		/* Retrieval target */

	uchar	errlog_no ;

	HFC_ENTRY("read_target_info") ;

	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct target_info) ) {	/* Is size short for target_info? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Target search
	 *--------------------------------------------------------------*/
	target = NULL;
	if ( diag->uni.dev_target.ww_name ) {
		/* WWN?*/
		target = hfc_hash_target_info_wwn( ap, diag->uni.dev_target.ww_name );
	}
	else {
		/* Scsi_id? */
		target = hfc_hash_target_info( ap, (uint)diag->uni.dev_target.target_id );
	}

	if ( target == NULL ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x02 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)target, (char *)(ulong)diag->addr, sizeof(struct target_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x03 );
		errlog_no = 0x1b ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_target_info");

	return( 0 );
}


/*
 * Function:    read_manage_info
 *
 * Purpose:     Processing for ioctl(HFCDIAG0) subcommand (Read manage_info) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_manage_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_manage_info");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MNG, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct manage_info) ) {	/* Is size short for manage_info? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MNG, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data Copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (void *)&hfc_manage_info, (void *)(ulong)diag->addr, sizeof(struct manage_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MNG, 0x02 );
		errlog_no = 0x1d ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_manage_info") ;

	return( 0 );
}


/*
 * Function:    read_hfctrace
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read hfctrace)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0		Normal end
 *             EINVAL	Specification error
 * Notes:       
 */
int read_hfctrace(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_hfctrace");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCTRC, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfctrace) ) {	/* Is size short for hfctrace? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCTRC, 0x01 );
		return( EINVAL );
	}

	if( (diag->uni.xob_xrb_scmd.no >= ap->trc_max) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCTRC, 0x02 );
		return( EINVAL );
	}

	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCTRC, 0x03 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(ap->trc_ptr + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct hfctrace) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCTRC, 0x04 );
		errlog_no = 0x1e ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	diag->uni.xob_xrb_scmd.current_no = ap->trc_num ;

	HFC_EXIT("read_hfctrace");

	return( 0 );
}


/*
 * Function:    read_mailbox
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read mailbox) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 *
 * Notes:       
 */
int read_mailbox(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	HFC_ENTRY("read_mailbox") ;
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct mailbox) ) {	/* Is size short for mailbox? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)ap->mb, (char *)(ulong)diag->addr, sizeof(struct mailbox) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x02 );
		errlog_no = 0x1f ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_mailbox") ;

	return( 0 );
}


/*
 * Function:    read_version
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read version)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_version(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;


	HFC_ENTRY("read_version");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VER, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(hfc_ver) ) {	/* Is size short for hfc_ver? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VER, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)hfc_ver, (char *)(ulong)diag->addr, sizeof(hfc_ver) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VER, 0x02 );
		errlog_no = 0x20 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	HFC_EXIT("read_version") ;

	return( 0 );
}


/*
 * Function:    read_hwlog
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read hw_log) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int read_hwlog(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	uchar	errlog_no ;

	HFC_ENTRY("read_hwlog") ;

	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HWLOG, 0x00 );
		return( EINVAL );
	}
	if( diag->length < HFC_HWLOG_SIZE ) {	/* Is size short for hw_log? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HWLOG, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(ap->mp_adap_info->hw_log), (char *)(ulong)diag->addr, HFC_HWLOG_SIZE ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HWLOG, 0x02 );
		errlog_no = 0x21 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	set_bit(HFC_HWLOG_VALID , (ulong *)&ap->io_status);

	HFC_EXIT("read_hwlog") ;

	return( 0 );
}


/*
 * Function:    read_tree
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read tree)
 *               - Response data is device configuration information
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail 
 * Notes:       
 */
int read_tree(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct a_info	adap ;
	struct t_info	tg ;
	struct t_info	*t_ptr ;
	struct m_info	mp ;
	struct target_info *target ;

	uint			entry_num , entry_num_wk ;
	uint			i ;

	HFC_ENTRY("read_tree") ;

	diag->uni.dev_tree.entry_num = 0 ;

	if( diag->uni.dev_tree.type == HFC_DGRD_MPINFO ){
		if( diag->length < sizeof( struct m_info ) ){
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_TREE, 0x00 );
			return( EINVAL );
		}
		BZERO((char *)&mp, sizeof(struct m_info));

		mp.lock = ap->mp_adap_info->lock;										/* @0005 */
		mp.status = ap->mp_adap_info->status;									/* @0005 */
		mp.mck_result = ap->mp_adap_info->mck_result;							/* @0005 */
		mp.port_cnt = ap->mp_adap_info->port_cnt;								/* @0005 */
		mp.mck_seq_no = ap->mp_adap_info->mck_seq_no;							/* @0005 */
		mp.sys_rev = hfc_get_sysrev (ap);  /* change from ap->mp_adap_info->sys_rev; */
											/* FCLNX-GPL-112 */

		if ( COPYOUT( (char *)&mp, (char *)(ulong)diag->addr, sizeof(struct m_info) ) != 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TREE, 0x02 );
			return ( EFAULT );
		}
		diag->uni.dev_tree.entry_num = 1 ;

		return( 0 );
	}

	if( diag->uni.dev_tree.type == HFC_DGRD_ADAP )
	{
		if( diag->length < sizeof(struct a_info) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_TREE, 0x00 );
			return(EINVAL) ;
		}
		adap.status = ap->status ;
		adap.scsi_id = ap->scsi_id ;
		adap.ww_name = ap->ww_name ;
		adap.node_name = ap->node_name ;
		adap.connect_type = ap->connect_type ;
		adap.max_data_rate = ap->max_data_rate ;
		adap.port_no = ap->port_no ;
		adap.attach_status = ap->attach_status ;
		adap.open_status = ap->open_status ;
		adap.mb_status = ap->mb_status ;
		adap.mb_resp = ap->mb_resp ;
		adap.mb_results = ap->mb_results;
		adap.mb_retry_cnt = ap->mb_retry_cnt ;
		adap.pkt_no = ap->pkt_no ;
		adap.pkt_cnt = ap->pkt_cnt ;
		adap.xob_no = ap->xob_no ;
		adap.xrb_no = ap->xrb_no ;
		adap.iov_no = ap->iov_no ;
		adap.iov_map_cnt = ap->iov_map_cnt ;
		adap.target_cnt = ap->target_cnt ;
		adap.xob_exec_cnt = ap->xob_exec_cnt ;
		adap.xob_wait_exec_cnt = ap->xob_wait_exec_cnt ;
		for(i=0 ; i<MAX_FRAME_CNT ; i++)
		{
			adap.save_xob_outp[i] = ap->save_xob_outp[i] ;
			adap.xob_outp_end[i]  = ap->xob_outp_end[i] ;
		}
		adap.curr_xob_outp = ap->curr_xob_outp ;
		adap.frame_chkp = ap->frame_chkp ;
		adap.frame_inp = ap->frame_inp ;
		adap.scsi_exec_cnt = ap->scsi_exec_cnt ;
		adap.scsi_end_cnt = ap->scsi_end_cnt ;
		adap.scsi_err_cnt = ap->scsi_err_cnt ;
		adap.link_dead_cnt = ap->link_dead_cnt ;
		adap.pci_err_cnt = 0 ;
		adap.mck_err_cnt = ap->mp_adap_info->mck_err_cnt ;	/* FCLNX-GPL-057 */
		adap.io_status = ap->io_status ;
		adap.trc_num = ap->trc_num ;
		adap.next_dstart_cnt = ap->next_dstart_cnt ;

		adap.xob_full_cnt = ap->xob_full_cnt;				/* FCLNX-GPL-143 */
		adap.iovmap_full_cnt = ap->iovmap_full_cnt;			/* FCLNX-GPL-143 */
		adap.frame_full_cnt = ap->frame_full_cnt;			/* FCLNX-GPL-143 */
		adap.page_over_cnt = ap->page_over_cnt;				/* FCLNX-GPL-143 */

		if ( COPYOUT( (char *)&adap, (char *)(ulong)diag->addr, sizeof(struct a_info) ) != 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_TREE, 0x00 );
			return ( EFAULT );
		}

		diag->uni.dev_tree.entry_num = 1 ;

		HFC_EXIT("read_tree");
		return( 0 );
	}

	if( diag->uni.dev_tree.type == HFC_DGRD_TARGET )
	{
		entry_num = diag->length / sizeof(struct t_info) ;
		if( entry_num == 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_TREE, 0x00 );
			return(EINVAL) ;
		}
		entry_num_wk = 0 ;

		t_ptr = (struct t_info *)(ulong)diag->addr ;

		for(i=0; i<MAX_TARGET_PROBE; i++)
		{
			if( ap->target_arg[i] == NULL )
				continue ;
			target = ap->target_arg[i] ;

			while( (target != NULL) && (entry_num_wk < entry_num) )
			{
				if( !test_bit(HFC_TARGETINF_VALID , (ulong *)&target->flags) )
				{
					target = target->next ;
					continue ;
				}
				tg.status = target->status ;
				tg.flags = target->flags ;
				tg.device_flags = target->device_flags ;
				tg.pseq = target->pseq ;
				tg.target_id = target->target_id ;
				tg.scsi_id = target->scsi_id ;
				tg.ww_name = target->ww_name ;
				tg.node_name = target->node_name ;
				tg.we_que_cnt = target->we_que_cnt ;
				tg.wx_que_cnt = target->wx_que_cnt ;
				tg.next_dstart_flag = target->next_dstart_flag ;

				if ( COPYOUT( (char *)&tg, (char *)t_ptr, sizeof(struct t_info) ) != 0 )
				{
					HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
							HFC_TRC_IOCTL_TREE, 0x00 );
					diag->uni.dev_tree.entry_num = entry_num_wk ;

					return ( EFAULT );
				}
				entry_num_wk++ ;
				t_ptr++ ;
				target = target->next ;
			}
		}
		diag->uni.dev_tree.entry_num = entry_num_wk ;

		HFC_EXIT("read_tree");

		return( 0 );
	}
	HFC_EXIT("read_tree");

	return( 0 );
}


/*
 * Function:    read_dev_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read dev_info)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail  *
 * Notes:       
 */
int read_dev_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	int hit;
	struct target_info *target;			/* Retrieval target */
	struct dev_info *dev;				/* Retrieval LU */
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_dev_info_mp(ap, diag);
	}
	else {
	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
		if( !diag->addr ) {									/* Virtual address unsecuring? */
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x00 );
			return( EINVAL );
		}
		if( diag->length < sizeof(struct dev_info) ) {	/* Size shortage? */
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x01 );
			return( EINVAL );
		}
	/*--------------------------------------------------------------*
	 * Target retrieval
	 *--------------------------------------------------------------*/
		target = NULL;
		if ( diag->uni.dev_target.ww_name ) {
		/* WWN specification */
			target = hfc_hash_target_info_wwn( ap, diag->uni.dev_target.ww_name );
		}
		else {
		/* Scsi_id specification */
			target = hfc_hash_target_info( ap, (uint)diag->uni.dev_target.target_id );
		}

		if ( target == NULL ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x02 );
			return( EINVAL );
		}

	/*--------------------------------------------------------------*
	 * LU retrieval
	 *--------------------------------------------------------------*/
		dev = target->dev;
		if ( dev == NULL ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x03 );
			return( EINVAL );
		}
	
		hit = 0;
		while( dev != NULL) {
			if ( dev->lun == (uchar)diag->uni.dev_target.lun_id ) {
				hit++;
				break;
			}
			dev = dev->next;
		}
	
		if ( hit == 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x04 );
			return( EINVAL );
		}
	
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
		if ( COPYOUT( (char *)dev, (char *)(ulong)diag->addr, sizeof(struct dev_info) ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x05 );
			return ( EFAULT );
		}

		HFC_EXIT("read_dev_info_mp");

		return( 0 );
	}
	return (ret);
}


/*
 * Function:    read_lg_target_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read lg_target_info)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int read_lg_target_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_lg_target_info_mp(ap, diag);
	}
	else {
		return (EIO);
	}
	return (ret);
}


/*
 * Function:    read_lg_dev_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read lg_target_info) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int read_lg_dev_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_lg_dev_info_mp(ap, diag);
	}
	else {
		return (EIO);
	}
	return (ret);
}


/*
 * Function:    read_lg_path_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read lg_target_info) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int read_lg_path_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_lg_path_info_mp(ap, diag);
	}
	else {
		return (EIO);
	}
	return (ret);
}


/*
 * Function:    read_lg_path_info1
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read lg_path_info1) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int read_lg_path_info1(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_lg_path_info1_mp(ap, diag);
	}
	else {
		return (EIO);
	}
	return (ret);
}


/*
 * Function:    read_lg_path_info2
 *
 * Purpose:     Process for ioctl(HFCDIAG0) ubcommand (Read lg_path_info2)
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int read_lg_path_info2(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_lg_path_info2_mp(ap, diag);
	}
	else {
		return (EIO);
	}
	return (ret);
}


/*
 * Function:    read_failover_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read failover_info) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int read_failover_info(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_failover_info_mp(ap, diag);
	}
	else {
		return (EIO);
	}
	return (ret);
}

/* FCLNX-GPL-112 
 * Function:    online_update
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Online update ) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int online_update(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	online_up_state = 0;
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} status_reg;
	uint		wk = 0;
	uchar		fpp_mode = 0;
	uchar		logdata[16];
	ulong		flags=0;	/* FCLNX-GPL-517 */
	
	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-517 */

	/* Online updadate applicable? ( Check adapter status ) */
	
	if(	test_bit(HFC_HWCHKSTOP,		(ulong *)&ap->mp_adap_info->status ) || /* Check stop state */
		test_bit(HFC_ISOL,		(ulong *)&ap->status ) || /* Adapter HW isolation state *//* FCLNX-GPL-387 */
		test_bit(HFC_HMCK_RECOVRTY,	(ulong *)&ap->mp_adap_info->lock)    || /* Recovering Machine check (other) */ 
		test_bit(HFC_MCK_RECOVERY,	(ulong *)&ap->status) 	||		/* Recovering Machine check (own) */            
		!hfc_mlpf_check_normal_hypsts(ap))	/* FCLNX-GPL-428 */
	{
		printk("diag online_update : Online update is not applicable because of driver status\n"); 
		diag->uni.online_up.errcode = 0x01;
		goto online_update_end;
	}
	
	/* Online updadate applicable? ( Check HW status ) */
	status_reg.l = 0;
	wk = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4 );
	HFC_4B_TO_4L(status_reg.l, wk);
	
	/* FCLNX-GPL-282 */
	/* We check 3 bits. "HFC_PCI_EXGMCK", "HFC_PCI_BOOTRUN", and "HFC_PCI_FCNSTOP". */
	/* But, never check the bit of "HFC_PCI_PCIERR_DETECTED". */
	/* Because, the bit will be standing, in spite of HBA's condition which is correctable error. */
	if( status_reg.c[1] & ( HFC_PCI_EXGMCK | HFC_PCI_BOOTRUN | HFC_PCI_FCNSTOP ) )
	{
		/* Online update is not applicable */
		diag->uni.online_up.errcode = 0x01;
		goto online_update_end;
	}

	/* Online update is not supported */
	if ( !(ap->fw_init_p->func & HFC_FWF_ONLINEUP) ) {
		diag->uni.online_up.errcode = 0x01;
		goto online_update_end;
	}
	
	/* Is FW operation mode is not NORMAL */
	fpp_mode = hfc_read_reg(ap, HFC_IOSPACE_CA_FLAG, 0x1);
	if ( fpp_mode != FW_MODE_NORMAL ) {
		/* OnlineUpdate */
		diag->uni.online_up.errcode = 0x01;
		goto online_update_end;
	}
	
	/* Online update process is waiting (busy state) */
	online_up_state = hfc_read_reg(ap, HFC_IOSPACE_CA_ONUP_STATE, 0x1);
	if ( online_up_state == 0x01 ) {
		diag->uni.online_up.errcode = 0x02;
		goto online_update_end;
	}
	
	/*--------------------------------------*/
	/* Start F/W online update operation	*/
	/*--------------------------------------*/
	
	/* Initiate FRAME_A :  Online Update */
	hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_ONLINEUP);
	
	/* Collect error log ( ErrNo = 0xa6 ) */
	memset(logdata,0,16);
	memcpy(logdata, (uchar*)&diag->uni.online_up.before_sysrev,4);
	memcpy(&logdata[4],(uchar *)&diag->uni.online_up.after_sysrev,4) ;
	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4, 0xa6, logdata, 16);

	diag->uni.online_up.errcode = 0x00;
	
online_update_end : 
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */

	return (0);

}


/* FCLNX-GPL-120 */
/*==================================================================*
 * hg_access
 *
 *    func    : Process for ioctl(HFCDIAG0) subcommand (MMIO-HG Read)
 *            : Process for ioctl(HFCDIAG0) subcommand (MMIO-HG Write) 
 *
 *==================================================================*/
int hg_access(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	uint	wk_data;			/* Internal data area */
	ushort	wk2 ;
	uint	wk4 ;

	HFC_ENTRY("hg_access");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	
	/* Virtual address check */
	if( !diag->addr ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x00 );
		diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
		return( EINVAL );
	}
	
	/* Is this ioctl initiated by Basic, Shadow, Shared or Dedicated? */
	if( HFC_MMODE_CHECK_BASIC (ap) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x01 );
		diag->uni.pci.err_code = HG_BASIC;
		return( EINVAL );
	}
	if (HFC_MMODE_CHECK_SHADOW (ap) ){ /* FCLNX-GPL-223 *//* FCLNX-GPL-495 */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x02 );
		diag->uni.pci.err_code = HG_SHADOW;
	}
	else if (HFC_MMODE_CHECK_DEDICATE(ap) ){ /* FCLNX-GPL-223 *//* FCLNX-GPL-495 */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x03 );
		diag->uni.pci.err_code = HG_DEDICATED;
	}
	else if (HFC_MMODE_CHECK_SHARED(ap) ){ /* FCLNX-GPL-223 *//* FCLNX-GPL-495 */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x04 );
		diag->uni.pci.err_code = HG_SHARED;
	}
	else{ /* Unknown type */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x05 );
		diag->uni.pci.err_code = HG_NONE;
		return( EINVAL );
	}

	if( ap->pkg.type == HFC_PKTYPE_FPP ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x02 );
		diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
		return( EINVAL );			
	}
	
	/* Address check */
	if( diag->uni.pci.addr > hg_addr_max[ap->pkg.type] ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x03 );
		diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
		return( EINVAL );
	}

	/* Access length check */
	if( (diag->length == PCI_LENGTH_01) ||
	    (diag->length == PCI_LENGTH_02) ||
	    (diag->length == PCI_LENGTH_04) ) {
	}
	else {	/* Invalid access length check */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x04 );
		diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
		return( EINVAL );
	}
	/* Access over check */
	if( (diag->uni.pci.addr + diag->length) > hg_addr_max[ap->pkg.type] +1 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x04 );
		diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
		return( EINVAL );
	}

	if( diag->sub_cmd == HFC_DG_RDHG ) {
	/*--------------------------------------------------------------*
	 * MMIO-HG Read
	 *--------------------------------------------------------------*/

		/* PCI memory read */
		wk_data = hfc_read_reg_hg_ext(ap, diag->uni.pci.addr, diag->length) ;
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_read_val(wk2); break;
		}
		/* Copy read data to user area (wk_datad -> iag->addr) */
		if ( COPYOUT( (char *)&wk_data, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x05 );
			errlog_no = 0x22 ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
			return ( EFAULT );
		}
	}
	else {
	/*--------------------------------------------------------------*
	 * MMIO-HG Write
	 *--------------------------------------------------------------*/

		/* Copy write data to internal area (diag->addr -> wk_data) */
		wk_data = 0 ;
		if ( COPYIN( (char *)(ulong)diag->addr, (char *)&wk_data, diag->length )  != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x06 );
			errlog_no = 0x23 ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
			diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
			return ( EFAULT );
		}
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_read_val(wk2); break;
		}
		/* PCI memory write */
		hfc_write_reg_hg_ext(ap, diag->uni.pci.addr, diag->length, wk_data);
	}

	HFC_EXIT("hg_access");
	return( 0 );
}

/* FCLNX-GPL-492 */
/*
 * Function:    scan_target_fcsw
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (scan_target_fcsw ) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 *
 * Notes:      - Be sure to lock (spin_lock) adap_info->tbl_lock
 *             before calling this subroutine.
 */
int scan_target_fcsw(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	ulong		flags=0;	/* FCLNX-GPL-517 */
	
	spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-517 */
	
	if (test_bit(HFC_ONLINE, (ulong *)&ap->status)) {
		if( !(( ap ->connect_type==HFC_SWITCH ) || ( (ap->connect_type==HFC_AL) && (ap->scsi_id & 0x00ffff00) )) ) {
			diag->uni.tgtscan.errcode = HFC_TGTSCAN_NOT_SWITCH;
			spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
			return( 0 );
		}
		if ( !test_bit(HFC_ENABLE, (ulong *)&ap->status)
		  || test_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status)
		  || test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)
		  || test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status)
		  || test_bit(HFC_WAIT_BUSRSP, (ulong *)&ap->status)
		  || test_bit(HFC_WAIT_CLOSE, (ulong *)&ap->status)
		  || test_bit(HFC_WAIT_T3, (ulong *)&ap->status)
		  || test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status)
		  || test_bit(HFC_CHK_STOP, (ulong *)&ap->status)
		  || test_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status)
		  || test_bit(HFC_ISOL, (ulong *)&ap->status) ) {
			diag->uni.tgtscan.errcode = HFC_TGTSCAN_ADAP_STATUS;
			spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
			return( 0 );
		}
	} else {
		diag->uni.tgtscan.errcode = HFC_TGTSCAN_LINKDOWN;
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
		return( 0 );
	}

	if (hfc_issue_gidft( ap ) ) { 	/* Issue GID_FT */
		diag->uni.tgtscan.errcode = HFC_TGTSCAN_ADAP_STATUS;
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
		return( 0 );
	}

	diag->uni.tgtscan.errcode = HFC_TGTSCAN_START; /* Normal End */
	spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
	
	return( 0 );
}

/* FCLNX-GPL-493 */
/*
 * Function:    change parameter
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (change parameter ) 
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 *
 * Notes:      - Be sure to lock (spin_lock) adap_info->tbl_lock
 *             before calling this subroutine.
 */
int change_param(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	ulong		flags=0;	/* FCLNX-GPL-517 */
	
	if ((ap->defparam) && (diag->uni.changeparm.ignore_force_default == 0)) {
		diag->uni.changeparm.errcode = HFC_CHGPRM_FORCE_DEF;
		return(0);
	}

	if (diag->uni.changeparm.version >= 1) {
		spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-517 */
		switch (diag->uni.changeparm.opr_limit_log) { /* Limit Log Mode */
			case HFC_CHGPRM_OPR_DEL:
				ap->limit_log = HFC_DISABLE_LIMITLOG;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm.val_limit_log = ap->limit_log;
				break;
			case HFC_CHGPRM_OPR_SET:				/* val_limit_log !=2 */
				if((diag->uni.changeparm.val_limit_log == 0) 
				 ||(diag->uni.changeparm.val_limit_log == 1)){
					ap->limit_log = diag->uni.changeparm.val_limit_log;
				}
				else {
					ap->limit_log = HFC_DISABLE_LIMITLOG;
				}
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}

		switch (diag->uni.changeparm.opr_filter_target) { /* Filtering Login Target */
		case HFC_CHGPRM_OPR_SET:
			ap->filter_target = diag->uni.changeparm.val_filter_target;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->filter_target = HFC_DISABLE_FILTERTGT;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_filter_target = ap->filter_target;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_link_down){ /* Link UP Wait time */
		case HFC_CHGPRM_OPR_SET:
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
				ap->dev_loss_tmo = diag->uni.changeparm.val_link_down;
				hfc_change_dev_loss_tmo(ap);
			} else {
				ap->linkup_tmo = diag->uni.changeparm.val_link_down;
			}
#else
			ap->linkup_tmo = diag->uni.changeparm.val_link_down;
#endif
			break;
		case HFC_CHGPRM_OPR_DEL:
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
				ap->dev_loss_tmo = HFC_DEF_DEV_LOSS_TMO;
			else
				ap->linkup_tmo = HFC_PCM_LINKUP_TO;
#else
			ap->linkup_tmo = HFC_PCM_LINKUP_TO;
#endif
			break;
		case HFC_CHGPRM_OPR_READ:
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
				diag->uni.changeparm.val_link_down = ap->dev_loss_tmo;
			else
				diag->uni.changeparm.val_link_down = ap->linkup_tmo;
#else
			diag->uni.changeparm.val_link_down = ap->linkup_tmo;
#endif
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		
		switch(diag->uni.changeparm.opr_reset_delay){ /* Reset Delay Wait time */
		case HFC_CHGPRM_OPR_SET:
			ap->scsi_reset_delay = diag->uni.changeparm.val_reset_delay;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->scsi_reset_delay = HFC_DELAY_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_reset_delay = ap->scsi_reset_delay;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
			
		switch(diag->uni.changeparm.opr_mck_retry){ /* Machine Check Retry */
		case HFC_CHGPRM_OPR_SET:
			ap->mck_err_cnt = 0;
			ap->mp_adap_info->mck_err_cnt = 0;
			ap->max_mck_cnt = diag->uni.changeparm.val_mck_retry;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->max_mck_cnt = HFC_MCKERR_CNT;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_mck_retry = ap->max_mck_cnt;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		
		switch(diag->uni.changeparm.opr_reset_timeout){ /* Target Reset timeout */
		case HFC_CHGPRM_OPR_SET:
			ap->target_reset_tmo = diag->uni.changeparm.val_reset_timeout;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->target_reset_tmo = HFC_TARGET_RST_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_reset_timeout = ap->target_reset_tmo;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_abort_timeout){ /* Abort TS timeout */
		case HFC_CHGPRM_OPR_SET:
			ap->abort_tmo = diag->uni.changeparm.val_abort_timeout;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->abort_tmo = HFC_ABORT_ACA_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_abort_timeout = ap->abort_tmo;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		
		switch(diag->uni.changeparm.opr_queue_depth){ /* Queue Depth */
		case HFC_CHGPRM_OPR_SET:
			ap->queue_depth = diag->uni.changeparm.val_queue_depth;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->queue_depth = HFC_DEFAULT_QUEUE_DEPTH;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_queue_depth = ap->queue_depth;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_scsi_allowed){ /* SCSI retry allowed */
		case HFC_CHGPRM_OPR_SET:
			ap->scsi_allowed = diag->uni.changeparm.val_scsi_allowed;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->scsi_allowed = HFC_SCSI_ALLOWED;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_scsi_allowed = ap->scsi_allowed;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_ld_err_intvl){ /* Link down Error intvl *//* FCLNX-GPL-FX-314 Start */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				
				ap->ld_err_intvl = diag->uni.changeparm.val_ld_err_intvl;
				
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				ap->ld_err_intvl = HFC_DF_LD_ERR_INTVL;
				
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_ld_err_intvl = ap->ld_err_intvl;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_ld_err_limit_l){ /* Link down Error limit *//* FCLNX-GPL-FX-314 Start */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				ap->ld_err_limit_l = diag->uni.changeparm.val_ld_err_limit_l;
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				ap->ld_err_limit_l = HFC_MIN_LD_ERR_LIMIT_L;
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_ld_err_limit_l = ap->ld_err_limit_l;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_ld_err_limit_s){ /* Link down Error limit */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->ld_err_limit_s = diag->uni.changeparm.val_ld_err_limit_s;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->ld_err_limit_s = HFC_MIN_LD_ERR_LIMIT_S;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_ld_err_limit_s = ap->ld_err_limit_s;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_if_err_intvl){ /* FC Error intvl *//* FCLNX-GPL-FX-314 Start */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				
				ap->if_err_intvl = diag->uni.changeparm.val_if_err_intvl;
				
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				ap->if_err_intvl = HFC_DF_IF_ERR_INTVL;
				
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_if_err_intvl = ap->if_err_intvl;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_if_err_limit){ /* FC Error limit */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->if_err_limit = diag->uni.changeparm.val_if_err_limit;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 *//* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->if_err_limit = HFC_MIN_IF_ERR_LIMIT;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 *//* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_if_err_limit = ap->if_err_limit;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_to_err_intvl){ /* Timeout Error intvl *//* FCLNX-GPL-FX-314 Start */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				
				ap->to_err_intvl = diag->uni.changeparm.val_to_err_intvl;
				
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
				ap->to_err_intvl = HFC_DF_TO_ERR_INTVL;
				
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_to_err_intvl = ap->to_err_intvl;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_to_err_limit){ /* Timeout Error limit */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->to_err_limit = diag->uni.changeparm.val_to_err_limit;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->to_err_limit = HFC_MIN_TO_ERR_LIMIT;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info(ap);
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_to_err_limit = ap->to_err_limit;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_rt_err_enable){ /* Reset Error Enable */
		case HFC_CHGPRM_OPR_SET:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->rt_err_enable = diag->uni.changeparm.val_rt_err_enable;
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
			ap->rt_err_enable = HFC_RT_ERR_NOT_SPPRTD;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_rt_err_enable = ap->rt_err_enable;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_hba_isolation){ /* hba_isolation */
		case HFC_CHGPRM_OPR_SET:
			if(diag->uni.changeparm.val_hba_isolation == HFC_ISOL_START){
				hfc_start_isolate(ap);
			}else if(diag->uni.changeparm.val_hba_isolation == HFC_ISOL_STOP){
				if(hfc_check_hba_isolation(ap)){								/* FCLNX-GPL-414 */
					ap->hba_isolation = HFC_ISOL_STOP;
				}
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_check_hba_isolation(ap)){								/* FCLNX-GPL-414 */
				ap->hba_isolation = HFC_ISOL_STOP;
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_hba_isolation = ap->hba_isolation;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		/* FCLNX-GPL-FX-175 Start */
		switch(diag->uni.changeparm.opr_abort_t_restrain){ /* abort_t_restrain */
		case HFC_CHGPRM_OPR_SET:
			ap->abort_t_restrain = diag->uni.changeparm.val_abort_t_restrain;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->abort_t_restrain = HFC_ABORT_T_RESTRAIN;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_abort_t_restrain = (uchar)ap->abort_t_restrain;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_lun_reset_delay){ /* lun_reset_delay */
		case HFC_CHGPRM_OPR_SET:
			ap->lun_reset_delay = diag->uni.changeparm.val_lun_reset_delay;
			break;
		case HFC_CHGPRM_OPR_DEL:
			ap->lun_reset_delay = HFC_LUN_DELAY;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_lun_reset_delay = (uchar)ap->lun_reset_delay;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		/* FCLNX-GPL-FX-175 End */
		
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
	}

	diag->uni.changeparm.errcode = HFC_CHGPRM_NORMAL; /* Normal End */
	return( 0 );
}
/* FCLNX-GPL-493 */
/*
 * Function:    change parameter
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (change parameter ) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 *
 * Notes:      - Be sure to lock (spin_lock) port_info->tbl_lock
 *             before calling this subroutine.
 */
int change_param_flash(
	struct	adap_info	*ap,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	ulong		flags=0;	/* FCLNX-GPL-517 */
	
	if (diag->uni.changeparm_flash.version >= 1){
		spin_lock_irqsave(&ap->adap_lock,flags) ;	/* FCLNX-GPL-517 */
		switch (diag->uni.changeparm_flash.opr_login_delay_time) { /* login delay time */
			case HFC_CHGPRM_OPR_SET:
				ap->login_wait = diag->uni.changeparm_flash.val_login_delay_time;
				break;
			case HFC_CHGPRM_OPR_DEL:
				ap->login_wait = HFC_DF_LOGIN_DELAY_TIME;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_login_delay_time = ap->login_wait;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		/* FCLNX-GPL-FX-366 */
		switch (diag->uni.changeparm_flash.opr_login_filter) { /* login filter */
			case HFC_CHGPRM_OPR_SET:
				break;
			case HFC_CHGPRM_OPR_DEL:
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_login_filter = 1;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		spin_unlock_irqrestore( &ap->adap_lock,flags );	/* FCLNX-GPL-517 */
	}
	diag->uni.changeparm_flash.errcode = HFC_CHGPRM_NORMAL; /* Normal End */
	return( 0 );
}


/*
 * Function:    hfcldd_conf_store
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (performance monitor) 
 *
 * Arguments:   
 *  pp         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int hfcldd_conf_store(
	struct	adap_info	*ap,	/* struct adap_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	int		rtn = 0;
	
	HFC_ENTRY("hfcldd_conf_store") ;
	
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCLDD_CONF_STORE, 0x00 );
		return EINVAL;
	}
	
	if( diag->length > CFG_ENT_SIZE ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCLDD_CONF_STORE, 0x01 );
		return EINVAL;
	}
	
	if( ap->hfclddconf == NULL) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCLDD_CONF_STORE, 0x02 );
		return EIO;
	}
	
	if ( COPYIN( (char *)(ulong)diag->addr, (char *)ap->hfclddconf, diag->length ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCLDD_CONF_STORE, 0x04 );
		errlog_no = 0x33 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		
		rtn = EFAULT;
	}
	
	HFC_EXIT("hfcldd_conf_store") ;
	
	return (rtn);
}

/* FCLNX-GPL-FX-146 *//* FCLNX-GPL-613 */
/*
 * Function:    flash_update
 *
 * Purpose:     Mark/unmark flash update status
 *
 * Arguments:   
 *  ap          - struct adap_info
 *  diag        - struct diag_ioctl
 *
 * Returns:     0		Normal end
 */
int flash_update(
	struct	adap_info	*ap,
	struct	diag_ioctl	*diag)
{
	struct mp_adap_info     *mpap;
	ulong		flags=0;

	HFC_DBGPRT("flash_update\n");
	
	mpap = ap->mp_adap_info;
	
	spin_lock_irqsave(&ap->adap_lock,flags) ;
	
	HFC_ADAP_LOCK(mpap , HFC_MP_ADAP_BUSY) ; 

	if (diag->uni.flash_update.opcode == HFC_FLASHUP_START) {
		if( test_bit(HFC_MCK_RECOVERY,	(ulong *)&ap->status) || test_bit(HFC_WAIT_T3,	(ulong *)&ap->status) ){
			diag->uni.flash_update.errcode = HFC_FLASHUP_ADAP_STATUS; 	/* Status Error */
		} else {
			set_bit( HFC_FLASH_UPDATE_PROGRESS, (ulong *)&mpap->status );
			diag->uni.flash_update.errcode = HFC_FLASHUP_SUCCESS; 		/* Normal */
		}
	}
	else { 	/* HFC_FLASHUP_FINISH */
		if( test_bit(HFC_FLASH_UPDATE_PROGRESS, (ulong *)&mpap->status) ){
			clear_bit(HFC_FLASH_UPDATE_PROGRESS, (ulong *)&mpap->status);

			if (hfc_pcibus_chk(ap) != 0)	{
				/* "PCI BUS ERR" has happen. */
				HFC_ISSUE_CSTP_PCIERR(ap, FALSE);
			}
			else if ( test_bit(HFC_MCK_RECOVERY,	(ulong *)&ap->status) ) {
				hfc_mck_recovery_five(ap, HFC_ABEND_MCK_RESUME); /* resume MCK recovery */
			}
		}
		diag->uni.flash_update.errcode = HFC_FLASHUP_SUCCESS; /* Normal */
	}
	HFC_ADAP_UNLOCK(mpap , HFC_MP_ADAP_BUSY) ; 

	spin_unlock_irqrestore( &ap->adap_lock,flags );

	return 0;

} /* end of flash_update */


/*
 * Function:    hfc_diag
 *
 * Purpose:     Process each ioctl(HFCDIAG0) subcommand
 *
 * Arguments:   
 *  ap         - struct adap_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 *
 * Notes:      - Be sure to lock (spin_lock) adap_info->tbl_lock
 *             before calling this subroutine.
 */
int hfc_diag(
	void	*arg,				/* struct diag_ioctl */
	struct	adap_info	*ap)	/* struct adap_info */
{

	struct	diag_ioctl	diag;	/* DIAD internal area */
	struct  mp_adap_info *mpap; /* mp_adap_info area  */
	int		rtn ;
	uint	i = 0 ;
	uchar	errlog_no ;

	HFC_ENTRY("hfc_diag") ;
	/*--------------------------------------------------------------*
	* Copy internal DIAG arguments from caller
	*--------------------------------------------------------------*/
	if ( COPYIN( (char *)arg, (char *)&diag, sizeof(struct diag_ioctl) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x00 );
		errlog_no = 0x03 ;
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
		return ( EFAULT );
	}
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(ap->ioctl32) {
		diag.addr = diag.addr & 0xffffffffU;
	}
#endif

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
	if( hfc_manage_info.open_status != HFC_OPENED )
	{
		/* Designated adapter has not opened. */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x01 ); 

		return( EIO );
	}
#endif

	mpap = ap->mp_adap_info;
	if( mpap == NULL ) {
		/* No corresponding mp_adap_info -> error */
		return( EIO );
	}

	if(!(test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ) ) {				//FCLNX-0294
		if (!((diag.sub_cmd == HFC_DGRD_ADAP)
		   || (diag.sub_cmd == HFC_DGRD_TARGET)
		   || (diag.sub_cmd == HFC_DGRD_VER)
		   || (diag.sub_cmd == HFC_DGRD_TREE)
		   || (diag.sub_cmd == HFC_DG_RDPCI)
		   || (diag.sub_cmd == HFC_DG_WRPCI)
		   || (diag.sub_cmd == HFC_DG_RDPCI_CFG)
		   || (diag.sub_cmd == HFC_DG_WRPCI_CFG)
		   || (diag.sub_cmd == HFC_DGRD_MPINFO)
		   || (diag.sub_cmd == HFC_DGRD_MNGINFO)
		   || (diag.sub_cmd == HFC_DG_RDHG)
		   || (diag.sub_cmd == HFC_DG_WRHG)) )
		    {
			/* Is adapter disable? */
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x02 );
			return( EIO );
		}
	}

	/*----------------------------------------------------------------
	 * Read F/W operation mode and check
	 *---------------------------------------------------------------*/

	ap->fcp_mode = hfc_read_reg(ap, HFC_IOSPACE_CA_FLAG, PCI_LENGTH_01);

	i = 0 ;
	while( diag_check[i].sub_command != 0 )
	{
		if( diag_check[i].sub_command == diag.sub_cmd )
			break ;
		i++ ;
	}
	if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
		if( !(diag_check[i].support_pk & HFC_DIAG_FPP) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x03 );  
			return( EINVAL );
		}
	}
	else if( ap->pkg.type == HFC_PKTYPE_FIVE )
	{
		if( !(diag_check[i].support_pk & HFC_DIAG_FIVE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x04 ); 
			return( EINVAL );
		}
	}
	else if( ap->pkg.type == HFC_PKTYPE_FIVE_EX )
	{
		if( !(diag_check[i].support_pk & HFC_DIAG_FIVE_EX ) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x04 ); 
			return( EINVAL );
		}
	}
	else {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x05 ); 
		return( EINVAL );
	}
	
	if( ap->fcp_mode == FW_MODE_NORMAL )
	{
		if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
			if( !(diag_check[i].flags_fpp & HFC_DIAG_NORMAL_MODE) )
			{
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_DIAG, 0x06 ); 
				return( EINVAL );
			}
		} 
	}
	if( ap->fcp_mode == FW_MODE_DIAG )
	{
		if( !(diag_check[i].flags_fpp & HFC_DIAG_DIAG_MODE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x07 ); 
			return( EINVAL );
		}
	}
	if( (ap->fcp_mode == FW_MODE_NAI_LOOP) || (ap->fcp_mode == FW_MODE_GAI_LOOP) )
	{
		if( !(diag_check[i].flags_fpp & HFC_DIAG_LOOP_MODE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x08 );
			return( EINVAL );
		}
	}
	if( ap->fcp_mode == FW_MODE_IOS )
	{
		if( !(diag_check[i].flags_fpp & HFC_DIAG_IOS_MODE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x09 ); 
			return( EINVAL );
		}
	}

	switch( diag.sub_cmd )
	{
		case HFC_DG_SETFW :
		case HFC_DG_MIHLOG :
		case HFC_DG_LDCHTRC :
		case HFC_DG_FORCSLOG :
		case HFC_DG_FWSTART :
		case HFC_DG_FWPOST :
		case HFC_DG_RDPCI :
		case HFC_DG_WRPCI :
		case HFC_DG_RDPCI_CFG :
		case HFC_DG_WRPCI_CFG :
		case HFC_DG_FSTOP :
		case HFC_DG_INITMDST :
		case HFC_DG_FCPMDST :
		case HFC_DG_ONLINE_UP :  /* FCLNX-GPL-112 */
		case HFC_DG_RDHG : /* FCLNX-GPL-120 */
		case HFC_DG_WRHG : /* FCLNX-GPL-120 */
		case HFC_DG_TGTSCAN :	/* FCLNX-GPL-492 */
		case HFC_DG_CHGPARM :	/* FCLNX-GPL-493 */
		case HFC_DG_FLASH_CHGPARM :
		case HFC_DG_HFCLDD_CONF :
			HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
			if( test_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status) )
			{
				HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
				return -EBUSY ;
			}
			else{
				set_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status);
			}
			HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
			break;
		default:
			break;

	}

	rtn = diag_check[i].cmd_func( ap , &diag) ;

	switch( diag.sub_cmd )
	{
		case HFC_DG_SETFW :
		case HFC_DG_MIHLOG :
		case HFC_DG_LDCHTRC :
		case HFC_DG_FORCSLOG :
		case HFC_DG_FWSTART :
		case HFC_DG_FWPOST :
		case HFC_DG_RDPCI :
		case HFC_DG_WRPCI :
		case HFC_DG_RDPCI_CFG :
		case HFC_DG_WRPCI_CFG :
		case HFC_DG_FSTOP :
		case HFC_DG_INITMDST :
		case HFC_DG_FCPMDST :
		case HFC_DG_ONLINE_UP :  /* FCLNX-GPL-112 */
		case HFC_DG_RDHG : /* FCLNX-GPL-120 */
		case HFC_DG_WRHG : /* FCLNX-GPL-120 */
		case HFC_DG_TGTSCAN :	/* FCLNX-GPL-492 */
		case HFC_DG_CHGPARM :	/* FCLNX-GPL-493 */
		case HFC_DG_FLASH_CHGPARM :
		case HFC_DG_HFCLDD_CONF :
			if( mpap != NULL ){
				if( test_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status) )
				{
					HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
					clear_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status);
					HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
				}
			}
			break;
		default:
			break;
	}
	
	/* FCLNX-GPL-223 start */
	if( (diag.sub_cmd == HFC_DG_RDHG) || (diag.sub_cmd == HFC_DG_WRHG) )
	{
		if ( COPYOUT( (char *)&diag, (char *)arg, sizeof(struct diag_ioctl) ) != 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x0c ); 
			errlog_no = 0x0 ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return ( EFAULT );
		}
	}
	/* FCLNX-GPL-223 end */
	
	if( rtn != 0 ) {	/* Does any of this routine have an error? */

		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x0a ); 
		return( rtn );
	}

	/*--------------------------------------------------------------*
	 * Copy back internal diag arguments to caller 
	 *  (only for diag_ioctl resp)
	 *--------------------------------------------------------------*/
	if( (diag_check[i].flags_fpp & HFC_DIAG_ARG_RESP) ||
		(diag_check[i].flags_five & HFC_DIAG_ARG_RESP) )
	{
		if ( COPYOUT( (char *)&diag, (char *)arg, sizeof(struct diag_ioctl) ) != 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x0b ); 
			errlog_no = 0x0 ;
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return ( EFAULT );
		}
	}

	HFC_EXIT("hfc_diag") ;

	return( 0 );	/* Normal end */
}
