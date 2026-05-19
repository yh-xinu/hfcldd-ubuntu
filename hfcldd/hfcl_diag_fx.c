/*
 * hfcl_diag_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char diag_fx_rcsid[] = "$Id: hfcl_diag_fx.c,v 1.1.2.37.2.6.2.6.2.9 2015/08/05 11:30:11 toyo Exp $";

#include "hfcldd.h"
#include "hfcl_detect.h"

/* forward declaration to suppress -Wmissing-prototypes */
int hfc_fx_diag(void *arg, struct port_info *pp);
#include "hfcl_top.h"
#include "hfcl_ioctl.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_hand_timer_trace.h"
#include "hfcl_mlpf.h"
#include "hfcl_version.h"
#include "hfcl_tbol.h" /* FCLNX-GPL-112 */
#include "hfcmpcfg.h"

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

extern void hfc_fx_top_trace(
	uchar					id,
	uchar 					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	uint64_t				etc1,
	uint64_t				etc2,
	uint64_t				etc3);

/* PCI access */
#define PCI_LENGTH_01			0x01	/* Access size  1byte */
#define PCI_LENGTH_02			0x02	/* Access size  2byte */
#define PCI_LENGTH_04			0x04	/* Access size  4byte */

const uint fx_pci_addr_max[2][5] = 
						{	/* PKTYPE */
							/* 0:none, 1:FPP, 2:FIVE, 3:FIVE-EX, 4:FIVE-FX */
							{   0xfff, 0xfff, 0x1fff,   0x1fff,    0x7fff  }, /* 0:BAR0 */
							{   0x000, 0x000, 0x0000,   0x07ff,    0x0fff  }  /* 1:BAR1 */
						};	/* FCLNX-GPL-154 */
const uint fx_pci_cnf_addr_max[5] = {
							0xff, 	/* PKTYPE is none */
							0xff, 	/* 1 HFC_PKTYPE_FPP */
							0xff, 	/* 2 HFC_PKTYPE_FIVE */
							0x1ff,  /* 3 HFC_PKTYPE_FIVE_EX */
							0xfff	/* 4 HFC_PKTYPE_FIVE_FX */
						};

const uint fx_hg_addr_max[5] = {         /* FCLNX-GPL-120 */
						0x0,    /* 0 PKTYPE is none */
						0x0,    /* 1 HFC_PKTYPE_FPP : Virtage is not supported */
						0x3ff,  /* 2 HFC_PKTYPE_FIVE *//* FCLNX-GPL-495 */
						0x3ff,  /* 3 HFC_PKTYPE_FIVE_EX *//* FCLNX-GPL-495 */
						0x3ff   /* 4 HFC_PKTYPE_FIVE_FX */
						};

//extern const uint pci_addr_max[2][5];

//extern const uint pci_cnf_addr_max[5];

//extern const uint hg_addr_max[5]; 

/*-- global variable --*/
extern int fx_set_fw_trace_mode(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_mih_log(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_load_ch_trace_log(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_forced_log(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_fw_start(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_fw_post(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_fw_init_tbl(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_xob(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_xrb(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_seg_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_port_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_target_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_hfctrace(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_mailbox(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_version(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_hwlog(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_core_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_coretrace(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_tree(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_pci_access(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_pci_cnf_access(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_stop_func(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_init_mode_set(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_mpadap_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_fcp_mode_set(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_manage_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_hfcpkt(struct port_info *pp,struct diag_ioctl *diag);	/*2004.11.30*/
extern int fx_read_dev_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_lg_target_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_lg_dev_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_lg_path_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_lg_path_info1(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_lg_path_info2(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_failover_info(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_online_update(struct port_info *pp, struct diag_ioctl *diag); /* FCLNX-GPL-112 */
extern int fx_hg_access(struct port_info *pp,struct diag_ioctl *diag); /* FCLNX-GPL-120 */
extern int fx_scan_target_fcsw(struct port_info *pp,struct diag_ioctl *diag); /* FCLNX-GPL-492 */
extern int fx_change_param(struct port_info *pp,struct diag_ioctl *diag); 		/* FCLNX-GPL-493 */
extern int fx_change_param_flash(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_change_param_flash2(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_payload(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_rcv_payload(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_performance(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_link_reset(struct port_info *pp,struct diag_ioctl *diag); /* FCLNX-GPL-FX-137 */
extern int fx_fw_trace_mode(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_hfcldd_conf_store(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_flash_update(struct port_info *pp, struct diag_ioctl *diag); /* FCLNX-GPL-146 */
extern int fx_read_mbtrace(struct port_info *pp,struct diag_ioctl *diag);	/* FCLNX-GPL-FX-139 */
extern int fx_vport_list(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_rsv_pkt(struct port_info *pp,struct diag_ioctl *diag);
extern int fx_read_pm_pkt(struct port_info *pp,struct diag_ioctl *diag);

const Typ_diag_cntl diag_fx_check[]={
/*----- Sub_Command ----------------------------- Mode --------------------- FPP ---- FIVE,FIVE-EX ---*/
	{ HFC_DG_MIHLOG   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xc0008000 , 0xc0008000 , (void*)fx_mih_log },
	{ HFC_DG_LDCHTRC  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xc0008000 , 0xc0008000 , (void*)fx_load_ch_trace_log },
	{ HFC_DG_FORCSLOG , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xc0000000 , 0xf0000000 , (void*)fx_forced_log },
	{ HFC_DG_FWSTART  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_fw_start },
	{ HFC_DG_FWPOST   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0x40008000 , 0x40008000 , (void*)fx_fw_post },
	{ HFC_DGRD_INITTBL, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_fw_init_tbl},
	{ HFC_DGRD_XOB	  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_xob },
	{ HFC_DGRD_XRB	  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_xrb },
	{ HFC_DGRD_SEGINFO, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_seg_info },
	{ HFC_DGRD_ADAP   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_port_info },
	{ HFC_DGRD_TARGET , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_target_info },
	{ HFC_DGRD_DDTRC  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0008000 , 0xf0008000 , (void*)fx_read_hfctrace },
	{ HFC_DGRD_MB     , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_mailbox },
	{ HFC_DGRD_VER    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_version },
	{ HFC_DGRD_HWLOG  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_hwlog },
	{ HFC_DGRD_TREE   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_tree },
	{ HFC_DGRD_CORE	  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_core_info },
	{ HFC_DGRD_CORETRC, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0008000 , 0xf0008000 , (void*)fx_read_coretrace },
	{ HFC_DGRD_SCMD   , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_hfcpkt }, /*2004.11.30 */
	{ HFC_DG_RDPCI    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_pci_access },
	{ HFC_DG_WRPCI    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_pci_access },
	{ HFC_DG_RDPCI_CFG, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_pci_cnf_access },
	{ HFC_DG_WRPCI_CFG, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_pci_cnf_access },
	{ HFC_DG_FSTOP    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_stop_func },
	{ HFC_DGRD_MPINFO , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_mpadap_info },
	{ HFC_DG_INITMDST , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xb0000000 , (void*)fx_init_mode_set },
	{ HFC_DG_FCPMDST  , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0x30000000 , (void*)fx_fcp_mode_set },
	{ HFC_DGRD_MNGINFO, HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_manage_info },
	{ HFC_DGRD_DEV    , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_dev_info },
	{ HFC_DGRD_LGTGT  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_lg_target_info },
	{ HFC_DGRD_LGDEV  , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_lg_dev_info },
	{ HFC_DGRD_PATHINFO, HFC_DIAG_FPP| HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_lg_path_info },
	{ HFC_DGRD_PATHINFO1,HFC_DIAG_FPP| HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_lg_path_info1 },
	{ HFC_DGRD_PATHINFO2,HFC_DIAG_FPP| HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_lg_path_info2 },
	{ HFC_DGRD_FOINFO , HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_read_failover_info },
	{ HFC_DG_ONLINE_UP, 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_online_update }, /* FCLNX-GPL-112 */
	{ HFC_DG_RDHG     , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_hg_access },/* FCLNX-GPL-120 */
	{ HFC_DG_WRHG     , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0000000 , 0xf0000000 , (void*)fx_hg_access },/* FCLNX-GPL-120 */
	{ HFC_DG_TGTSCAN  , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_scan_target_fcsw },/* FCLNX-GPL-492 *//* FCLNX-GPL-506 */
	{ HFC_DG_CHGPARM  , 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_change_param },/* FCLNX-GPL-493 *//* FCLNX-GPL-506 */
	{ HFC_DG_FLASH_CHGPARM, 		   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_change_param_flash },/* FCLNX-GPL-493 *//* FCLNX-GPL-506 */
	{ HFC_DG_FLASH2_CHGPARM,		   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_change_param_flash2 },/* FCLNX-GPL-493 *//* FCLNX-GPL-506 */
	{ HFC_DGRD_PAYLOAD, 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0000000 , (void*)fx_read_payload },
	{ HFC_DGRD_RCV_PAYLOAD, 		   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0000000 , (void*)fx_read_rcv_payload },
	{ HFC_DG_PERFORMANCE, 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_performance },
	{ HFC_DG_LINK_RESET,			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_link_reset },/* FCLNX-GPL-FX-137 */
	{ HFC_DG_SETFW_FX, 				   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_set_fw_trace_mode },
	{ HFC_DG_HFCLDD_CONF, 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_hfcldd_conf_store },
	{ HFC_DGRD_MBTRC , 	HFC_DIAG_FPP | HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0xf0008000 , 0xf0008000 , (void*)fx_read_mbtrace },	/* FCLNX-GPL-FX-139 */
	{ HFC_DG_FLASH_UPDATE,			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0008000 , (void*)fx_flash_update },	/* FCLNX-GPL-FX-146 */
	{ HFC_DGRD_VPORT_LIST, 			   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0000000 , (void*)fx_vport_list },
	{ HFC_DGRD_RSV_PKT,				   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0000000 , (void*)fx_read_rsv_pkt },
	{ HFC_DGRD_PM_PKT,				   HFC_DIAG_FIVE | HFC_DIAG_FIVE_FX , 0          , 0xf0000000 , (void*)fx_read_pm_pkt },
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_set_fw_trace_mode(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	region_info *rp = NULL;
	uint	frame_code;
	ulong	flags=0;
	int		i;
	
	HFC_ENTRY("fx_fw_trace_mode");
	
	rp = pp->region_arg[pp->rid];
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	if( test_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 ) )		/* FCLNX-GPL-FX-180 */
	{
		HFC_DBGPRT("diag fx_set_fw_trace_mode : pp->status\n"); 
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		return EIO;
	}
	
	frame_code = HFC_FX_FRAMEA_FW_TRC_MODE;
	frame_code |= (uint)(diag->uni.fw_trc_info_fx.trc_mode & 0x0000ffff);
	
	/* Initiate FRAME_A :  Online Update */
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_FRAMEA,
			(char)0x4, (int)frame_code, HFC_FX_CORE_OFFSET40);
	}
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	HFC_EXIT("fx_fw_trace_mode");
	
	return (0);
}


/*
 * Function:    mih_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (MIH Log)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_mih_log(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
#if 0
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

	HFC_ENTRY("mih_log") ;

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( pp );		/* Lock mailbox */

	/* Set up mailbox control block  */
	pp->mb->mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	pp->mb->mb_init.sub_cmd = HFC_MBSCMD_MIHLOG;	/* Sub-Command */
	hfc_fx_write_val( pp->mb->mb_init.dependent_code, 0x0010 );	/* LOGID */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if (( rtn = hfc_fx_mailbox_proc(pp, HFC_FX_MB_RSP_TMR, HFC_MB_PROC_TO, pp->els_retry) ) == 0 ) {	/* FCLNX-0523 */
		/* Copy data to Soft_Log_Area designated by ssn/son/sbc */
		ssn = (uchar)hfc_fx_read_val( pp->mb->mb_resp.type.drvlogb0.mih_log.ssn );
		son = (uchar)hfc_fx_read_val( pp->mb->mb_resp.type.drvlogb0.mih_log.son );
		sbc = (ushort)hfc_fx_read_val( pp->mb->mb_resp.type.drvlogb0.mih_log.sbc );
		usSlogLen = (ushort)hfc_fx_read_val( pp->fw_init_p->slog_len );
		usSlogNum = (ushort)hfc_fx_read_val( pp->fw_init_p->slog_num );

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
						{	/* Copy sbc-(1024*i) bytes data from pp->slog[slog_adr] to diag->addr+(1024*i) */
							if( COPYOUT( &pp->slog[slog_adr], ( (uchar *)(ulong)diag->addr+(usSlogLen*i) ), ( sbc - (usSlogLen*i) ) ) )
							{
								goto copyout_err;
							}
						}
						else
						{	/* Copy 1024 bytes data from pp->slog[slog_adr] to diag->addr+(1024*i) */
							if( COPYOUT( &pp->slog[slog_adr], ( (uchar *)(ulong)diag->addr+(usSlogLen*i) ), usSlogLen ) )
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
					if( COPYOUT( &pp->slog[slog_adr], (uchar *)(ulong)diag->addr, sbc ) )
					{	/* FCLNX-GPL-229 */
						goto copyout_err;
					}
				}
			}
		}

		/* Normal end without outputting data to buffer when ssn/son/sbc check is error  */
	}

	unlock_mailbox( pp );	/* Lock release of mailbox */


	HFC_EXIT("mih_log") ;

	return( rtn );
	
copyout_err:	/* FCLNX-GPL-229 start */

	unlock_mailbox( pp );	/* Lock release of mailbox */

	errlog_no = 0x24;
	hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
	
	HFC_EXIT("mih_log") ;

	return ( EFAULT );	/* FCLNX-GPL-229 end */
#endif
	return ENOTTY;
}


/*
 * Function:    load_ch_trace_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Load CH Trace Log)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_load_ch_trace_log(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct		logaddr_list *loglist;		/* Log address list virtual address */
	dma_addr_t	logaddr_busaddr;			/* Log address list bus address */
	char		*trc_log_area;				/* Log buffer area virtual address */
	dma_addr_t	trclog_busaddr;				/* Log buffer area bus address */
	int			i,j,k,rtn;
	char		*work_adr;					/* work pointer address */
	ushort		uswork;						/* work */
	int			loop_cnt;
	uchar		errlog_no ;
	struct		core_info *core=NULL;
	ulong		flags=0;					/* FCLNX-GPL-FX-353 */

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

	loglist = (struct logaddr_list *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf,
		(uint)sizeof(struct logaddr_list), &logaddr_busaddr);

	if (loglist == NULL) {
		errlog_no = 0x05 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)loglist, (uint)sizeof(struct logaddr_list));

	/*--------------------------------------------------------------*
	 * Allocate log buffer area
	 *--------------------------------------------------------------*/
	/* Internal partitioning & page mpaping */
	trc_log_area = (uchar *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf,
		LOG_LENGTH_LDCH, &trclog_busaddr);

	if (trc_log_area == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);

		errlog_no = 0x06 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
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
		hfc_fx_write_val(loglist->bus_addr[k], (uint64_t)(trclog_busaddr + HFC_PAGE_SIZE*j) );

		if ( k >= LOG_SEGNUM_LDCH ) {	
			hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
				(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
			hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
				LOG_LENGTH_LDCH, (void *)trc_log_area, trclog_busaddr);

			return ( EIO );
		}

		k++ ;
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_LDCHTRC, 0x06 );
		rtn = EINVAL;
		goto ldch_error_exit;
	}

	core = pp->region_arg[pp->rid]->core_arg[diag->core_no];
	
	if (core == NULL) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_LDCHTRC, 0x04 );
		rtn = EINVAL;
		goto ldch_error_exit;
	}
	
	if( test_bit(HFC_PS_ISOL,	(ulong *)&pp->status ) ||
		test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) ||
		hfc_fx_check_cs_disable(pp, core) ||
		test_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 ) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_LDCHTRC, 0x05 );
		rtn = EIO;
		goto ldch_error_exit;
	}
	
	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	/* Lock mailbox */
	if ( !(lock_fx_try_mailbox( pp )) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_LDCHTRC, 0x02 );
		rtn = EIO;
		goto ldch_error_exit;
	}
	
	/* Set up mailbox control block  */
	hfc_fx_write_val(core->mb->mb_init.mb_code, HFC_MBCMD_LDCHTRC);
	hfc_fx_write_val(core->mb->mb_init.timer, pp->mb_timer[ HFC_MBTIME_LOADCHTRC ].tout-1 ); 
	hfc_fx_write_val(core->mb->mb_init.type.load_ch_trace_log.logo_list_addr, (uint64_t)logaddr_busaddr);
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
	/* Mailbox start/completion waiting */
	if ((rtn = hfc_fx_mailbox_proc(pp, core, HFC_FX_MB_RSP_TMR, 
			pp->mb_timer[HFC_MBTIME_LOADCHTRC].tout, pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry)) != 0) {
		/* Error */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_LDCHTRC, 0x03 );

		HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		unlock_fx_mailbox(pp);	/* Unlock mailbox */
		goto ldch_error_exit;
	}
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	/* Set trace pointer */
	work_adr = (char *)&core->mb->mb_resp.type.load_ch_trace_log.fw_trc_ptr;
	BCOPY( (char *)(work_adr + FPP_TRC_SIZE_OFFSET),
		(char *)&diag->uni.ch_trc_log.trc_ptr[0], FPP_TRC_SIZE );
	for( i = 0; i < 4; i++ ){
		uswork = (ushort)hfc_fx_read_val( diag->uni.ch_trc_log.trc_ptr[i].sp );
		diag->uni.ch_trc_log.trc_ptr[i].sp = uswork;
		uswork = (ushort)hfc_fx_read_val( diag->uni.ch_trc_log.trc_ptr[i].ep );
		diag->uni.ch_trc_log.trc_ptr[i].ep = uswork;
		uswork = (ushort)hfc_fx_read_val( diag->uni.ch_trc_log.trc_ptr[i].cp );
		diag->uni.ch_trc_log.trc_ptr[i].cp = uswork;
	}

	BCOPY( (char *)(work_adr + FRM_TRC_SIZE_OFFSET),
		(char *)&diag->uni.ch_trc_log.frm_trc_ptr, FRM_TRC_SIZE );
	uswork = (ushort)hfc_fx_read_val( diag->uni.ch_trc_log.frm_trc_ptr.sp );
	diag->uni.ch_trc_log.frm_trc_ptr.sp = uswork;
	uswork = (ushort)hfc_fx_read_val( diag->uni.ch_trc_log.frm_trc_ptr.ep );
	diag->uni.ch_trc_log.frm_trc_ptr.ep = uswork;
	uswork = (ushort)hfc_fx_read_val( diag->uni.ch_trc_log.frm_trc_ptr.cp );
	diag->uni.ch_trc_log.frm_trc_ptr.cp = uswork;

	unlock_fx_mailbox(pp);	/* Unlock mailbox */

	/*--------------------------------------------------------------*
	 * Data copy 
	 *--------------------------------------------------------------*/
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
	if ( COPYOUT( (char *)trc_log_area, (char *)(ulong)diag->addr, LOG_LENGTH_LDCH ) != 0 ) {
		errlog_no = 0x0d ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		rtn = EFAULT;
	}
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

ldch_error_exit:
	/* FCLNX-GPL-FX-353 Start */
	if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, NULL);
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
	/* FCLNX-GPL-FX-353 End */
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
		(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
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
 *  pp         - struct port_info 
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
static __maybe_unused int fx_meint_log(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
#if 0
	struct	logaddr_list *loglist;			/* Log address list virtual address */
	dma_addr_t	logaddr_busaddr;			/* Log address list bus address */
	char	*trc_log_area;					/* Log buffer area virtual address */
	dma_addr_t	logout_busaddr;				/* Log buffer area bus address */

	int		rtn;

	uchar	errlog_no ;

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
	
	loglist = (struct logaddr_list *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf,
		(uint)sizeof(struct logaddr_list), &logaddr_busaddr);

	if (loglist == NULL) {
		errlog_no = 0x0e ;
		hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)loglist, (uint)sizeof(struct logaddr_list));

	/*--------------------------------------------------------------*
	 * Allocate log buffer area
	 *--------------------------------------------------------------*/

	trc_log_area = (uchar *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf,
		LOG_LENGTH_MEINT, &logout_busaddr);

	if (trc_log_area == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);

		errlog_no = 0x11 ;
		hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)trc_log_area, LOG_LENGTH_MEINT);

	/*--------------------------------------------------------------*
	 * Set up log addresses to log list
	 *--------------------------------------------------------------*/
	loglist->segnum = LOG_SEGNUM_MEINT;		/* Number of segments */
	hfc_fx_write_val(loglist->bus_addr[0], (uint64_t)logout_busaddr );

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( pp );		/* Lock mailbox */

	/* Set up mailbox control block  */
	pp->mb->mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	pp->mb->mb_init.sub_cmd = HFC_MBSCMD_SMEINT;	/* Sub-Command */
	hfc_fx_write_val( pp->mb->mb_init.dependent_code, 0x0000 );
	hfc_fx_write_val( pp->mb->mb_init.type.drvlogb1.log_list_addr,
			(uint64_t)logaddr_busaddr );/* Bus address of log address list */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if ((rtn = hfc_fx_mailbox_proc(pp, HFC_FX_MB_RSP_TMR, HFC_MB_PROC_TO, pp->els_retry)) != 0) {	/* FCLNX-0523 */
		/* Error */
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			LOG_LENGTH_MEINT, (void *)trc_log_area, logout_busaddr);

		unlock_mailbox( pp );	/* Unlock mailbox */
		return ( rtn );
	}

	unlock_mailbox( pp );	/* Unlock mailbox */

	/*--------------------------------------------------------------*
	 * Data copy 
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)trc_log_area, (char *)(ulong)diag->addr, LOG_LENGTH_MEINT ) != 0 ) {

		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			LOG_LENGTH_MEINT, (void *)trc_log_area, logout_busaddr);

		errlog_no = 0x14 ;
		hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
		(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
		LOG_LENGTH_MEINT, (void *)trc_log_area, logout_busaddr);

	HFC_EXIT("meint_log");

	return( rtn );
#endif
	return ENOTTY;
}


/*
 * Function:    forced_log
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Forced soft LOG)
 *
 * Arguments:   
 *  pp         - struct port_info 
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_forced_log(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
#if 0
	struct	logaddr_list *loglist;			/* Log address list virtual address */
	dma_addr_t	logaddr_busaddr;			/* Log address list bus address */
	char	*trc_log_area;					/* Log buffer area virtual address */
	dma_addr_t	trclog_busaddr;				/* Log buffer area bus address */
	int		j,k,rtn;

	int		loop_cnt;

	uchar	errlog_no ;

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

	loglist = (struct logaddr_list *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf,
		(uint)sizeof(struct logaddr_list), &logaddr_busaddr);

	if (loglist == NULL) {
		errlog_no = 0x05 ;
		hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
		return ( ENOMEM );
	}

	BZERO((char *)loglist, (uint)sizeof(struct logaddr_list));

	/*--------------------------------------------------------------*
	 *  Allocate log buffer area
	 *--------------------------------------------------------------*/

	trc_log_area = (uchar *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf,
		LOG_LENGTH_SOFT, &trclog_busaddr);

	if (trc_log_area == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);

		errlog_no = 0x08 ;
		hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x60, &errlog_no, 1) ;
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
		hfc_fx_write_val(loglist->bus_addr[k], (uint64_t)(trclog_busaddr + HFC_PAGE_SIZE*j ) );

		if ( k >= LOG_SEGNUM_SOFT ) {	/* Set number(It must not be usually) */
			hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
				(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
			hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
				LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

			return ( EIO );
		}

		k++ ;
	}

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	lock_mailbox( pp );		/* Lock of mailbox */

	/* The mailbox control block is assembled according to DRVLOGB1 */
	pp->mb-> mb_init.command = HFC_MBCMD_LOGTRACE;	/* Command */
	pp->mb-> mb_init.sub_cmd = HFC_MBSCMD_FORCSLOG;	/* Sub-Command */
	hfc_fx_write_val( pp->mb->mb_init.dependent_code, diag->uni.forced_log.dependent_code );
	hfc_fx_write_val( pp->mb->mb_init.type.drvlogb1.log_list_addr,
			(uint64_t)logaddr_busaddr );/* Bus address of log address list */

	/* Mailbox start/completion waiting */ /* FCLNX-GPL-243 */
	if ((rtn = hfc_fx_mailbox_proc(pp, HFC_FX_MB_RSP_TMR, HFC_MB_PROC_TO, pp->els_retry)) != 0) {	/* FCLNX-0524 */
		/* Error case */
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

		unlock_mailbox( pp );	/* Unlock mailbox */
		return ( rtn );
	}

	unlock_mailbox( pp );	/*  Unlock mailbox */


	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)trc_log_area, (char *)(ulong)diag->addr, LOG_LENGTH_SOFT ) != 0 ) {

		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
			LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

		errlog_no = 0x0d ;
		hfc_fx_errlog(pp, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
		(uint)sizeof(struct logaddr_list), (void *)loglist, logaddr_busaddr);
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf,
		LOG_LENGTH_SOFT, (void *)trc_log_area, trclog_busaddr);

	HFC_EXIT("forced_log");

	return( rtn );
#endif
	return ENOTTY;
}


/*
 * Function:    fw_post
 *
 * Purpose:     Processing of subcommand (F/W POST) of ioctl(HFCDIAG0)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_fw_post(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int		rtn=0, i;
	struct core_info	*core=NULL;
	struct mailbox_fx	*mb=NULL;
	uchar	core_no=0, core_cnt=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_DIAG ];
	uchar	post_cmd=0;
	uint	mb_code=0;
	ulong	flags = 0 ;

	HFC_ENTRY("fw_post");

	/*--------------------------------------------------------------*
	 * Parameter check (command check)
	 *--------------------------------------------------------------*/
	 
	 HFC_DBGPRT("hfcldd%d fw_post post_cmnd = %02x\n",pp->dev_minor, diag->uni.post.cmd);
	 
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
	
	/* get core# to execute the diag */
	core_no = diag->core_no;
	post_cmd = diag->uni.post.cmd;
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if( core_no & (0x80 >> i ) ){
			core_cnt++;
	    	if( core_cnt > 1) {
				return ( EIO );
			}
		}
	}

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if( (test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) )  /* FCLNX-GPL-177 */
	{
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWPOST, 0x01 );
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return ( EINVAL ) ;
	}
	
	lock_fx_mailbox( pp );		/* Lock mailbox */
	
	pp->diag = diag;
	
	set_bit(HFC_PS_DIAG, (ulong *)&pp->status);
	
	/* Start Diag mailbox for the selected cores */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */

		if( core_no & (0x80 >> i ) ){
			mb = core->mb;
			/* Setup mailbox control block */
			mb_code = (HFC_MBCMD_DIAG | (uint)diag->uni.post.cmd);
			hfc_fx_write_val( mb -> mb_init.mb_code, mb_code );
			hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
			
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
			
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_DIAG, 0x00, pp, NULL, core, NULL, NULL,
				0, 0, 0 );
			
			/* Mailbox start */
			hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );
			break;
		}
	}
	
	/* Stop timer and delete ID */
	hfc_fx_watchdog_enter( pp,core,NULL,NULL,0,HFC_FX_MB_RSP_TMR,0,1 );		/* FCLNX-GPL-FX-233,272 */
	/* Start timer and delete ID */
	hfc_fx_watchdog_enter( pp,core,NULL,NULL,0,HFC_FX_MB_RSP_TMR,HFC_FX_MB_DIAG_TO,0 );		/* FCLNX-GPL-FX-233,272 */

	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	/* Wait for mailbox request completion */
	hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);	
	HFC_PORTLOCK_IRQSAVE(pp,flags);

	/* Stop timer and delete ID */
	hfc_fx_watchdog_enter( pp,core,NULL,NULL,0,HFC_FX_MB_RSP_TMR,0,1 );
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_DIAG, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	/* Record time */
	pp->mb_prol_sleep_end_time = (uint)jiffies;
	
	if( test_bit(HFC_MCK_HW_INIT, (ulong *)&pp->mck_result ) ){
		pp->diag->uni.post.core_result[ pp->mck_core_no ] = 0x10;
		rtn = EIO;
	}

	clear_bit(HFC_PS_DIAG, (ulong *)&pp->status);

	unlock_fx_mailbox( pp );	/* Lock release of mailbox */
	
	/* FCLNX-GPL-FX-353 Start */
	if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, NULL);
	/* FCLNX-GPL-FX-353 End */

	/* Output IMLLOG (Timeout case) */
	if (core->mb_resp == HFC_MBR_TIMEOUT ){			/* FCLNX-GPL-FX-233,272 */
		memset(core->logdata,0,16);					/* FCLNX-GPL-391 */
		core->logdata[0] = 0x80;
		core->logdata[1] = 0x16;
		core->logdata[3] = (uchar)diag->uni.post.cmd;
		hfc_fx_save_hwlog_five_fx(pp, core, 0x78, HFC_ERRLOG_TYPE_IMLLOG);	/* FCLNX-GPL-FX-239 */
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRC, 0x78, core->logdata, 16);
	}
	
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	HFC_EXIT("fw_post") ;

	return( rtn );

	return ENOTTY;
}


/*
 * Function:    fw_start
 *
 * Purpose:     Processing for ioctl(HFCDIAG0) subcommand (F/W Start) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_fw_start(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uint i , lp ;
	int rc=0;
	ulong	flags = 0 ;
	struct target_info_fx 	*target_work ;
	struct	core_info	*core=NULL;
	/* Dummy read */
	uint	wk_reg = 0;

	HFC_ENTRY("fw_start");

	/*--------------------------------------------------------------*
	 * Parameter check (flag)
	 *--------------------------------------------------------------*/
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if( (test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) )  /* FCLNX-GPL-177 */
	{

		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWSTART, 0x31 );
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return ( EINVAL ) ;
	}
	
	if( (diag->uni.fw_start.mode != FW_MODE_NORMAL)	&&
		(diag->uni.fw_start.mode != FW_MODE_DIAG) )	
	{
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWSTART, 0x01 );
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return ( EINVAL );
	}
	
	if( pp->fcp_mode != FW_MODE_NORMAL )
	{
		if( diag->uni.fw_start.mode != FW_MODE_NORMAL )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWSTART, 0x02 );
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			return ( EINVAL );
		}
	}
	if( diag->uni.fw_start.mode != FW_MODE_NORMAL )
	{
		if( (test_bit(HFC_PS_DIAG, (ulong *)&pp->status)) )  /* FCLNX-GPL-177 */
		{

			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWSTART, 0x03 );
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			return ( EBUSY ) ;
		}
		
		for(lp=0 ; lp<(pp->max_target) ; lp++)
		{
			target_work = pp->target_arg[lp] ;
			if( target_work != NULL )
			{
				if( test_bit(HFC_TF_DEVFLG_VALID,(ulong *)&target_work->flags) )
				{
				
					for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
						if(pp->region_arg[pp->rid] != NULL){
							if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL){
								continue;
							}
				
							if( (target_work->core_queue[core->core_no].we_que_cnt != 0) 
								|| (target_work->core_queue[core->core_no].wx_que_cnt != 0) )
							{
								HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
									HFC_TRC_IOCTL_FWSTART, 0x04 );
								HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
								return ( EBUSY ) ;
							}
						}
					}
				}
			}
		}

		if( test_bit( HFC_MAILBOX_BUSY, (ulong *)&( pp->region_arg[pp->rid]->mb_lock ) ) )
		{
			set_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
			/* 3s wait during processing mailbox */
			hfc_fx_w_stop(pp, core, HFC_FX_DIAG_DELAY_TMR) ;   
			hfc_fx_w_start(pp, core, HFC_FX_DIAG_DELAY_TMR, HFC_FX_REBOOT_DELAY_TO) ;
			
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);					/* FCLNX-0269 */
			HFC_PORTLOCK_IRQSAVE(pp,flags);
			clear_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
		}

		clear_bit( HFC_PS_ONLINE, (ulong *)&pp -> status );

		set_bit(HFC_PS_DIAG , (ulong *)&pp->status) ; /* FCLNX-GPL-177 */
	}

	switch (diag->uni.fw_start.mode)
	{
		case  FW_MODE_DIAG :
		case  FW_MODE_NAI_LOOP :	
		case  FW_MODE_GAI_LOOP :	
			/* Initiate boot microprogram and check POST result */
			HFC_DBGPRT("hfcldd%d fw_start MODE_DIAG\n",pp->dev_minor);
			mdelay(10);

//			if( pp->diag_cnt == 0 )											/* FCLNX-293 STR*/
//			{
				/* Set flag in CTL_RST */									/* FCLNX-282 STR*/
					
				/* Close INT_A mask */ /* FCLNX-GPL-257 */
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  PCI_LENGTH_04, (uint)0x00000000, HFC_FX_CORE_OFFSET10);
						}
					}
				}
				
				HFC_ALLCORELOCK(pp->region_arg[0]);
				/* Cancel pending SCSI commands, and stop timers. */
				hfc_fx_mck_prepare(pp, HFC_ABEND_MCK_INT);
				HFC_ALLCOREUNLOCK(pp->region_arg[0]);
				
				set_bit(HFC_PS_DIAG , (ulong *)&pp->status) ; /* FCLNX-GPL-177 */
				
				/* Start STOP_FUNCTION */
				hfc_fx_write_reg(pp, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x50);
				/* Wait 10ms */
				mdelay(10);
					
				hfc_fx_reset_start(pp, HFC_CTLRST);
				mdelay(10); 	/* Wait 10ms */

				/* Reset flag in CTL_RST */									/* FCLNX-282 STR*/
				if( pp != NULL )
				{
					clear_bit( HFC_MAILBOX_BUSY, (ulong *)&( pp->region_arg[pp->rid]->mb_lock ) );
				}
				
				/*** Reset PCI space to clear INT_A register ***/  /* FCLNX-GPL-257 */
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
							  PCI_LENGTH_04, (uint)0xffffffff, HFC_FX_CORE_OFFSET10);
						
							/* set diag mode */
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_CA_FLAG,
							  PCI_LENGTH_01, diag->uni.fw_start.mode, HFC_FX_CORE_OFFSET10);
						}
					}
				}
					
				/* Reset Receive Control Flag */								/* FCLNX-GPL-FX-079 */
				hfc_fx_reset_start(pp, HFC_RECEIVE_CTL_FLAG_CLEAR);				/* FCLNX-GPL-FX-079 */
				pp->fcp_mode = FW_MODE_DIAG;									/* FCLNX-GPL-FX-165 */
			
				hfc_fx_reset_start(pp, HFC_WSCA_CLEAR);
				hfc_fx_reset_port_info(pp) ;

				clear_bit(HFC_PS_ISOL , (ulong *)&pp->status) ; /* FCLNX-GPL-177 */

				hfc_fx_reset_start(pp,HFC_REBOOT);
				
				/* Dummy read */
				wk_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ZERO, (char)0x4);

				/* Wait 2s */
				i = 0;

				set_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
				/* Wait 2s */
				hfc_fx_w_stop(pp, core, HFC_FX_DIAG_DELAY_TMR) ;
				hfc_fx_w_start(pp, core, HFC_FX_DIAG_DELAY_TMR, HFC_FX_REBOOT_DELAY_TO) ;
				
				/* Wait timeout for completion of DIAG process */
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);				/* FCLNX-0296 */
				HFC_PORTLOCK_IRQSAVE(pp,flags);
				clear_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
				
				/* Start Function */
				hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1,(char)0xa0);
				
				/* Dummy read */
				wk_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ZERO, (char)0x4);
				
				/* Check POST result (no retry) */
				rc = hfc_fx_config_hw_set_five_fx(pp,HFC_CONFIG_HW_CHECK_RETRY_RECV);	/* FCLNX-GPL-FX-215 */
//			}
//			else{
//				rc = 0;
//			}
			pp->diag_cnt++;													/* FCLNX-293 END*/

			/* POST result check */
			if( rc != 0 ) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_FWSTART, 0x01 );
				diag->uni.post.reslt = 0x10;
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				return ( EIO );
			}
			else
			{
				/* H/W Initialize succeeded. */
				/* WS Comu Area(0x318) */
				hfc_fx_reset_start(pp, HFC_SET_WS80);
						
				/* INITTBL ADR(0x310)  */
				hfc_fx_reset_start(pp, HFC_SET_INIADR);
				
				set_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
				atomic_set(&pp->check_mbreq, 1);

				/* Reconfigure interrupt mask */
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  PCI_LENGTH_04, hfc_inta_mask[pp->pkg.type], HFC_FX_CORE_OFFSET10);
						}
					}
				}
				
				if((atomic_read(&pp->check_mbreq))&&!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
					start_fx_next_mailbox(pp, NULL);
				
				set_bit(HFC_PS_DIAG, (ulong *)&pp->status );
				
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

				/* Wait timeout for completion of DIAG process */
				hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);				/* FCLNX-0296 */
				HFC_PORTLOCK_IRQSAVE(pp,flags);
				clear_bit(HFC_PS_DIAG, (ulong *)&pp->status );
			}
			
			HFC_DBGPRT("hfcldd%d fw_start MODE_DIAG end pp->status = %08x pp->status_detail1 = %08x status_detail2 = %08x\n",
				pp->dev_minor, pp->status, pp -> status_detail1, pp -> status_detail2);
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			
			break ;

		case  FW_MODE_NORMAL :
			HFC_DBGPRT("hfcldd%d fw_start MODE_NORMAL\n",pp->dev_minor);
			if (diag->uni.fw_start.err_code == FW_DIAG_FAILED)
			{
				/* Check stop if DIAG fails */
				HFC_FX_ISSUE_CSTP( pp, HFC_ABEND_FCSTP ) ;				/* FCLNX-0279 *//* @MLPF *//* FCLNX-GPL-316 */
				/* Return to previous interruption level */
				/* Clear error code */
				diag->uni.fw_start.err_code = 0x00 ;
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				return( 0 );
			}
			else
			{
				if( test_bit(HFC_PS_ISOL , (ulong *)&pp->status ) ) /* FCLNX-GPL-177 */
				{
					HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
					return(0);
				}
				
				/* Close INT_A mask */ /* FCLNX-GPL-257 */
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  PCI_LENGTH_04, (uint)0x00000000, HFC_FX_CORE_OFFSET10);
						}
					}
				}
					
				/* Initiate STOP_FUNCTION */
				hfc_fx_write_reg(pp, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x50);
				mdelay(1); 	/* 1ms wait */
					
				hfc_fx_reset_start(pp, HFC_CTLRST);

				mdelay(1); 	/* 1ms wait */

				/* Reset flag in CTL_RST */										/* FCLNX-282 STR*/
//				clear_bit( HFC_CTLRST_PROCESS, (ulong *)&mpap->status );
					
				/*** Reset PCI space to clear INT_A register ***/  /* FCLNX-GPL-257 */
				for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
					if(pp->region_arg[pp->rid] != NULL){
						if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
							  PCI_LENGTH_04, (uint)0xffffffff, HFC_FX_CORE_OFFSET10);
						}
					}
				}
					
				hfc_fx_reset_start(pp, HFC_WSCA_CLEAR);
				hfc_fx_reset_port_info(pp) ;
				hfc_fx_reset_start(pp, HFC_REBOOT);
				
				/* Dummy read */
				wk_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ZERO, (char)0x4);

				set_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
				/* 2s wait */
				hfc_fx_w_stop(pp, core, HFC_FX_DIAG_DELAY_TMR) ;
				hfc_fx_w_start(pp, core, HFC_FX_DIAG_DELAY_TMR, HFC_FX_REBOOT_DELAY_TO) ;
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				/* Wait timeout for completion of DIAG process */
				hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait );				/* FCLNX-0269 */
				
				HFC_PORTLOCK_IRQSAVE(pp,flags);

				clear_bit(HFC_PD_MB_DIAG_DELAY, (ulong *)&pp->status_detail1 );
				pp->fcp_mode = FW_MODE_NORMAL;									/* FCLNX-GPL-FX-165 */
					
				/* Start Function */
				hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1,(char)0xa0);
				
				/* Dummy read */
				wk_reg = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_ZERO, (char)0x4);

				/* Check POST result (no retry) */
				rc = hfc_fx_config_hw_set_five_fx(pp,HFC_CONFIG_HW_CHECK_RETRY_RECV);	/* FCLNX-GPL-FX-215 */
					
				if(rc)
				{
					/*-- init fail --*/
					HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
							HFC_TRC_IOCTL_FWSTART, 0x21 );
					diag->uni.post.reslt = 0x10;
					HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
					return ( EIO );
				}
				else
				{
					clear_bit(HFC_PS_DIAG , (ulong *)&pp->status); /* FCLNX-GPL-177 */
					
					/* H/W Initialize succeeded. */
					/* WS Comu Area(0x318) */
					hfc_fx_reset_start(pp, HFC_SET_WS80);
						
					/* INITTBL ADR(0x310)  */
					hfc_fx_reset_start(pp, HFC_SET_INIADR);
				
					set_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
					atomic_set(&pp->check_mbreq, 1);

					/* Reconfigure interrupt mask */
					for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
						if(pp->region_arg[pp->rid] != NULL){
							if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
								hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
								  PCI_LENGTH_04, hfc_inta_mask[pp->pkg.type], HFC_FX_CORE_OFFSET10);
							}
						}
					}
				
					if((atomic_read(&pp->check_mbreq))&&!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
						start_fx_next_mailbox(pp, NULL);
				
					set_bit(HFC_PS_DIAG, (ulong *)&pp->status );

					/* Wait timeout for completion of DIAG process */
					HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
					HFC_DBGPRT("fw_start - sleep_on\n");
					hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);				/* FCLNX-0296 */
					HFC_DBGPRT("fw_start - wake_up\n");
					HFC_PORTLOCK_IRQSAVE(pp,flags);

					clear_bit(HFC_PS_DIAG, (ulong *)&pp->status );
					
//					set_bit( HFC_PS_WAIT_LINKUP, (ulong *)&pp->status );
//					set_bit( HFC_DIAG_END, (ulong *)&pp->status );

//					set_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1);
//					atomic_set(&pp->check_mbreq, 1);
					HFC_DBGPRT("fw_start - atomic_set check_mbreq");
				}
			}

			diag->uni.post.reslt = 0;
			
			HFC_DBGPRT("hfcldd%d fw_start MODE_NORMAL end pp->status = %08x pp->status_detail1 = %08x status_detail2 = %08x\n",
				pp->dev_minor, pp->status, pp -> status_detail1, pp -> status_detail2);
			/* Core_start of all cores have finished. */
			unlock_fx_mailbox(pp);
			/* FCLNX-GPL-FX-353 Start */
			if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
				start_fx_next_mailbox(pp, NULL);
			/* FCLNX-GPL-FX-353 End */
			
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *             - 0
 * Notes:       
 */
int fx_stop_func(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
#if 0
	HFC_ENTRY("stop_func") ;

	/* Initiste STOP_FUNCTION  */
	hfc_fx_write_reg(pp, HFC_IOSPACE_CMDCTL, PCI_LENGTH_01, 0x40);

	/* Drop ONLINE bit to prevent from initiating SCSI and ioctl */
	clear_bit(HFC_PS_ONLINE,(ulong *)&pp->status) ;

	HFC_EXIT("stop_func") ;

	return( 0 );
#endif
	return ENOTTY;
}


																					/* FIVE STR */
/*
 * Function:    init_mode_set
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (INIT MODE SET)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl 
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_init_mode_set(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
#if 0
	int rtn = 0;

	HFC_ENTRY("init_mode_set") ;

	/*--------------------------------------------------------------*
	 * Lock Mailbox 
	 *--------------------------------------------------------------*/
	lock_mailbox( pp );		/* Lock of mailbox */

	/*--------------------------------------------------------------*
	 * Mailbox processing
	 *--------------------------------------------------------------*/
	/* Setup mailbox control block */
	pp->mb->mb_init.command = HFC_MBCMD_FPPCTL;						/* Command */
	pp->mb->mb_init.sub_cmd = HFC_MBSCMD_INITMDSET;					/* Sub-Command */
	hfc_fx_write_val( pp->mb->mb_init.dependent_code, 0x0000 );		/* Test number */

	pp->mb->mb_init.type.drvctl.un.init_mode_set.mode[0] = diag->uni.init_mode_set.mode_alpa.uc_mode[3];
	pp->mb->mb_init.type.drvctl.un.init_mode_set.mode[1] = diag->uni.init_mode_set.mode_alpa.uc_mode[2];
	pp->mb->mb_init.type.drvctl.un.init_mode_set.mode[2] = diag->uni.init_mode_set.mode_alpa.uc_mode[1];
	pp->mb->mb_init.type.drvctl.un.init_mode_set.al_pa = 0;
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[0] = (uchar)'H';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[1] = (uchar)'I';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[2] = (uchar)'T';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[3] = (uchar)'A';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[4] = (uchar)'C';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[5] = (uchar)'H';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[6] = (uchar)'I';
	pp->mb->mb_init.type.drvctl.un.init_mode_set.magic_num[7] = (uchar)' ';
	if( ( ( diag->uni.init_mode_set.mode_alpa.mode >> 8 ) & 0x00FFFFFF ) != HFC_INITMDSET_FCP ){
		pp->mb->mb_init.type.drvctl.un.init_mode_set.al_pa = diag->uni.init_mode_set.mode_alpa.al_pa[0];
		hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.init_mode_set.connect_type, diag->uni.init_mode_set.connect_type );
		hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.init_mode_set.wwpn, diag->uni.init_mode_set.wwpn );
		hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.init_mode_set.wwnn, diag->uni.init_mode_set.wwnn );

		hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.init_mode_set.buf_size, diag->uni.init_mode_set.buf_size );
		hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.init_mode_set.xrdy_div, diag->uni.init_mode_set.xrdy_div );
		hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.init_mode_set.param, diag->uni.init_mode_set.param );
	}

	/* Start Mailbox process and wait for process completion */ /* FCLNX-GPL-243 */
	rtn = hfc_fx_mailbox_proc(pp, HFC_FX_MB_RSP_TMR, HFC_MB_PROC_TO, pp->els_retry);	/* FCLNX-0523 */

	unlock_mailbox( pp );	/* Unlock mailbox */

	HFC_EXIT("init_mode_set") ;

	return( rtn );
#endif
	return ENOTTY;

}	/* end of init_mode_set */


/*
 * Function:    fcp_mode_set
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (FCP MODE SET)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  ETIMEDOUT  - Mailbox initiation time out
 *  EIO        - Other errors
 * Notes:       
 */
int fx_fcp_mode_set(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
#if 0
	int rtn = 0;

	HFC_ENTRY("fcp_mode_set") ;

	/*--------------------------------------------------------------*
	 * Lock Mailbox 
	 *--------------------------------------------------------------*/
	lock_mailbox( pp );		/* Lock of mailbox */

	/*--------------------------------------------------------------*
	 * Mailbox processing 
	 *--------------------------------------------------------------*/
	/* Setup mailbox control block */
	pp->mb->mb_init.command = HFC_MBCMD_FPPCTL;						/* Command */
	pp->mb->mb_init.sub_cmd = HFC_MBSCMD_FCPMDSET;					/* Sub-Command */
	hfc_fx_write_val( pp->mb->mb_init.dependent_code, 0x0000 );		/* Test no. */

	hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.fcp_mode_set.act_ctl, diag->uni.fcp_mode_set.act_ctl );
	hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.fcp_mode_set.il_ctl, diag->uni.fcp_mode_set.il_ctl );
	hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.fcp_mode_set.seq_ctl, diag->uni.fcp_mode_set.seq_ctl );
	hfc_fx_write_val( pp->mb->mb_init.type.drvctl.un.fcp_mode_set.rsp_delay, diag->uni.fcp_mode_set.rsp_delay );

	/* Start mailbox process and wait for completion */ /* FCLNX-GPL-243 */
	rtn = hfc_fx_mailbox_proc(pp, HFC_FX_MB_RSP_TMR, HFC_MB_PROC_TO, pp->els_retry);	/* FCLNX-0523 */

	unlock_mailbox( pp );	/* Unlock mailbox */

	HFC_EXIT("fcp_mode_set") ;

	return( rtn );
#endif
	return ENOTTY;

}	/* end of fcp_mode_set */
																					/* FIVE END */


/*==================================================================*
 * pci_access
 *
 *    func    : Process for ioctl(HFCDIAG0) subcommand (PCI Read)
 *            : Process for ioctl(HFCDIAG0) subcommand (PCI Write) 
 *
 *==================================================================*/
int fx_pci_access(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	int		return_code = 0;		/* FCLNX-GPL-154 */

	uint	wk_data;			/* Internal data area */
	ushort	wk2 ;
	uint	wk4 ;
	ulong	base_addr   = 0;		/* FCLNX-GPL-154 */
	uint	addr_max    = 0;		/* FCLNX-GPL-154 */
	int		addr_type   = 0;		/* FCLNX-GPL-154 */

//	HFC_ENTRY("pci_access") ;
//	HFC_ERRPRT("pci_access 0\n");

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
	base_addr = pp->mem_base_addr;	/* Default parm */
	addr_type = PCI_BAR0;			/* Default parm */
	if( pp->debug_func & HFC_FX_DEBUG_SKIP_RW_BAR1 ) {
		/* skip PCI memory BAR# check */
	}
	else {
		addr_type = diag->uni.pci.base_addr_type;
		switch( addr_type )
		{
			case PCI_BAR0:
				base_addr = pp->mem_base_addr;
				break;
				
			case PCI_BAR1:
				base_addr = hfc_fx_remap_pci_bar(pp->pci_cfginf, 2);
				if( base_addr == 0x00 )
				{	/* mpaping err */
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
	
//	HFC_ERRPRT("pci_access 1\n");
	/* PCI memory space address check */ /* FCLNX-GPL-154 */
	addr_max = fx_pci_addr_max[addr_type][pp->pkg.type];
	if( diag->uni.pci.addr > addr_max )
	{	/* out of range */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x01 );
		return_code = EINVAL; /* FCLNX-GPL-154 */
		goto err_end;
	}
	
//	HFC_ERRPRT("pci_access 2\n");
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
	addr_max = fx_pci_addr_max[addr_type][pp->pkg.type] +1;
	if( (diag->uni.pci.addr + diag->length) > addr_max )
	{	/* out of range */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x03 );
		return_code = EINVAL; /* FCLNX-GPL-154 */
		goto err_end;
	}
	
//	HFC_ERRPRT("pci_access 3\n");

	if( diag->sub_cmd == HFC_DG_RDPCI ) {
	/*--------------------------------------------------------------*
	 * PCI Read
	 *--------------------------------------------------------------*/

//		HFC_ERRPRT("pci_access 4 read\n");
		/* PCI memory read */
		wk_data = hfc_fx_read_reg_ext2(pp, base_addr, diag->uni.pci.addr, diag->length) ;
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_fx_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_fx_read_val(wk2); break;
		}
		/* Copy read data to user area (wk_data -> diag->addr) */
		if ( COPYOUT( (char *)&wk_data, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x04 );
			errlog_no = 0x2a ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return_code = EFAULT; /* FCLNX-GPL-154 */
			goto err_end;
		}
//		HFC_ERRPRT("pci_access 5 read\n");
	}
	else {
	/*--------------------------------------------------------------*
	 * PCI Write
	 *--------------------------------------------------------------*/

//		HFC_ERRPRT("pci_access 4 write\n");
		/* Copy write data to internal area (diag->addr -> wk_data) */
		wk_data = 0 ;
		if ( COPYIN( (char *)(ulong)diag->addr, (char *)&wk_data, diag->length )  != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x05 );
			errlog_no = 0x2b ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
			return_code = EFAULT; /* FCLNX-GPL-154 */
			goto err_end;
		}
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_fx_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_fx_read_val(wk2); break;
		}
		/* PCI memory write */
		hfc_fx_write_reg_ext2(pp, base_addr, diag->uni.pci.addr, diag->length, wk_data);
//		HFC_ERRPRT("pci_access 5 write\n");
	}
	
err_end: /* FCLNX-GPL-154 */

	/* PCI memory BAR# check */ /* FCLNX-GPL-154 start */
	if( pp->debug_func & HFC_FX_DEBUG_SKIP_RW_BAR1 ) {
		/* skip PCI memory BAR# check */
	}
	else {
		switch( diag->uni.pci.base_addr_type )
		{
			case PCI_BAR1:
				hfc_fx_unmap_pci_bar(pp->pci_cfginf, base_addr);
				base_addr = 0x00;
				break;

			case PCI_BAR0:
			default: /* others */
				/* NOP */
				break;
		}
	} /* FCLNX-GPL-154 end */

//	HFC_EXIT("pci_access");
//	HFC_ERRPRT("pci_access 6\n");

	return( return_code );
}


/*==================================================================*
 * pci_cnf_access
 *
 *    func    : Process for ioctl(HFCDIAG0) subcommand (PCI Config Read) 
 *            : Process for ioctl(HFCDIAG0) subcommand (PCI Config Write) 
 *
 *==================================================================*/
int fx_pci_cnf_access(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	int		rtn = 0;

	uint	wk_data;			/* Internal data area */

//	HFC_ENTRY("pci_cnf_access" ) ;
	
//	HFC_ERRPRT("pci_cnf_access 0\n");

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
//	HFC_ERRPRT("pci_cnf_access 1\n");
	/* PCI config space address check */
	if( diag->uni.pci.addr > fx_pci_cnf_addr_max[pp->pkg.type] ) {
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
//	HFC_ERRPRT("pci_cnf_access 1\n");
	/* Access over check */
	if( (diag->uni.pci.addr + diag->length) > fx_pci_cnf_addr_max[pp->pkg.type] +1 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCICNFACC, 0x03 );
		return( EINVAL );
	}
	
//	HFC_ERRPRT("pci_cnf_access 2\n");

	if( diag->sub_cmd == HFC_DG_RDPCI_CFG ) {
	/*--------------------------------------------------------------*
	 * PCI Config Read
	 *--------------------------------------------------------------*/
		/* PCI config reading */
//		HFC_ERRPRT("pci_cnf_access 3 pci config read diag->uni.pci.addr = %08x, diag->length = %08x\n",diag->uni.pci.addr, diag->length);
		wk_data = hfc_fx_read_cnfg(pp, diag->uni.pci.addr, diag->length);
//		HFC_ERRPRT("pci_cnf_access 3 pci config read wk_data = %08x\n",wk_data);
		wk_data = hfc_fx_read_val(wk_data) ;
		if( diag->length == PCI_LENGTH_01 ) {
			wk_data <<= 24;
		}
		else if( diag->length == PCI_LENGTH_02 ) {
			wk_data <<= 16;
		}
//		HFC_ERRPRT("pci_cnf_access 4 pci config read\n");
		/* Copy read data to user area (wk_datad -> iag->addr) */
		if ( COPYOUT( (char *)&wk_data, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCICNFACC, 0x04 );
			errlog_no = 0x2c ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return ( EFAULT );
		}
//		HFC_ERRPRT("pci_cnf_access 5 pci config read\n");
	}
	else {
	/*--------------------------------------------------------------*
	 * PCI Config Write
	 *--------------------------------------------------------------*/
		/* Copy write data to internal area (diag->addr -> wk_data) */
//		HFC_ERRPRT("pci_cnf_access 3 pci config write\n");
		if ( COPYIN( (char *)(ulong)diag->addr, (char *)&wk_data, diag->length )  != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCICNFACC, 0x05 );
			errlog_no = 0x2d ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
			return ( EFAULT );
		}
		wk_data = hfc_fx_read_val(wk_data) ;
		
		HFC_ERRPRT("pci_cnf_access 4 pci config write\n");
		/* Shift write data */
		if( diag->length == PCI_LENGTH_01 ) {
			wk_data >>= 24;
		}
		else if( diag->length == PCI_LENGTH_02 ) {
			wk_data >>= 16;
		}
		/* PCI config write */
		hfc_fx_write_cnfg(pp, diag->uni.pci.addr, diag->length, wk_data);
//		HFC_ERRPRT("pci_cnf_access 5 pci config write\n");
	}
//	HFC_EXIT("pci_cnf_access" ) ;

	return( rtn );
}


/*
 * Function:    read_fw_init_tbl
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read fw_init_tbl) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    - 0
 *
 * Notes:       
 */
int fx_read_fw_init_tbl(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */

//	HFC_ENTRY("read_fw_init_tbl");
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
	/*--------------------------------------------------------------*
	 * Log area specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct fw_init_tbl_fx) ) {	/* Is data size short for fw_init_tbl ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x04 );
		return( EINVAL );
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) && !HFC_FX_MMODE_CHECK_SHADOW (pp) ){
		if((!test_bit(HFC_CS_CHK_STOP, (ulong *)&core->status))&&(!test_bit(HFC_CS_CORE_ENABLE, (ulong *)&core->status))){
			core = rp->core_arg[pp->master_core_no];
			if(!core) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_FWINIT, 0x06 );
				return( EINVAL );
			}
		}
	}
	
	if ( COPYOUT( (char *)core->fw_init_p, (char *)(ulong)diag->addr, sizeof(struct fw_init_tbl_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x05 );
		errlog_no = 0x15 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_fw_init_tbl");

	return( 0 );
}


/*
 * Function:    read_xob
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read xob) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    - 0
 *
 * Notes:       
 */
int fx_read_xob(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */

//	HFC_ENTRY("read_xob") ;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
	/*--------------------------------------------------------------*
	 * Specificaton check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct xob_fx) ) {	/* Is data size short for xob ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x04 );
		return( EINVAL );
	}
	if( 
		(diag->uni.xob_xrb_scmd.no >= hfc_fx_read_val(core->fw_init_p->xob_num)) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x05 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number  */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x06 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(core->xob + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct xob_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XOB, 0x07 );
		errlog_no = 0x16 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_xob");

	return( 0 );
}


/*
 * Function:    read_xrb
 *
 * Purpose:     Processing for ioctl(HFCDIAG0) subcommand (Read xrb) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    - 0
 *           
 * Notes:       
 */
int fx_read_xrb(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */

//	HFC_ENTRY("read_xrb");
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {		/* Is virtual address unassigned ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct xrb_fx) ) {	/* Is size short for xrb? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x04 );
		return( EINVAL );
	}
	if( (diag->uni.xob_xrb_scmd.no >= hfc_fx_read_val(core->fw_init_p->xrb_num)) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x05 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x06 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(core->xrb + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct xrb_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_XRB, 0x07 );
		errlog_no = 0x17 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_xrb");

	return( 0 );
}


/*
 * Function:    read_hfcpkt
 *
 * Purpose:     Process for ioctl(HFCDIAG0)subcommand (Read hfc_pkt)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0
 *
 * Notes:       2004.11.30
 */
int fx_read_hfcpkt(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct port_info	*wkpp;
	struct hfc_pkt_fx	*hfcp, *hfcp_next;
	uchar	errlog_no ;
	
//	HFC_ENTRY("read_hfc_pkt");
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfc_pkt_fx) ) {	/* Is size short for hfc_pkt_fx? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x01 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.no >= wkpp->pkt_num ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x03 );
		return( EINVAL );
	}
	
	if (HFC_FX_VIRTUAL_PORT(pp)) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x05 );
		return( EINVAL );
	}
	
	hfcp = wkpp->pkt_top;
	while (hfcp != NULL) {
		hfcp_next = hfcp->pkt_next;
		if (hfcp->pkt_no == diag->uni.xob_xrb_scmd.no)
			break;
		hfcp = hfcp_next;
	}
	
	if (hfcp == NULL) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x06 );
		return( EINVAL );
	}
	
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( hfcp, (char *)(ulong)diag->addr, sizeof(struct hfc_pkt_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCPKT, 0x04 );
		errlog_no = 0x17 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}
	
//	HFC_EXIT("read_hfcpkt");

	return( 0 );
}


/*
 * Function:    read_seg_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read seg_info) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *             0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_seg_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int seg_info_cnt;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */

	uchar	errlog_no ;

//	HFC_ENTRY("read_seg_info");
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	seg_info_cnt = (wkpp->dma_max / HFC_PAGE_SIZE);
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */

	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {		/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct seg_info_fx) ) {	/* Is size short for seg_info? */
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

	rp = wkpp->region_arg[wkrid];
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x04 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x05 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_FWINIT, 0x06 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(core->seg_info + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct seg_info_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SEG, 0x07 );
		errlog_no = 0x18 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_seg_info");

	return( 0 );
}


																					/* FIVE STR */
/*
 * Function:    read_mpadap_info
 *
 * Purpose:     Processing of subcommand (Read hfc_fx_mpadap_info) of ioctl(HFCDIAG0)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_mpadap_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	return( ENOTTY );
}	/* end of read_mpadap_info */
																					/* FIVE END */


/*
 * Function:    read_port_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read port_info)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_port_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct port_info	*wkpp;

//	HFC_ENTRY("read_port_info") ;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_APINFO, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct port_info) ) {	/* Is size short for port_info? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_APINFO, 0x01 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)wkpp, (char *)(ulong)diag->addr, sizeof(struct port_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_APINFO, 0x02 );
		errlog_no = 0x1a ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}
//	HFC_EXIT("read_port_info") ;

	return( 0 );
}


/*
 * Function:    read_target_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read target_info_fx) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *    
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_target_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct target_info_fx	*target;		/* Retrieval target */
	struct port_info		*wkpp;

	uchar	errlog_no ;
	int		i;

//	HFC_ENTRY("read_target_info") ;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct target_info_fx) ) {	/* Is size short for target_info_fx? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Target search
	 *--------------------------------------------------------------*/
	target = NULL;
	
#if 0
	if ( diag->uni.dev_target.ww_name ) {
		/* WWN?*/
		target = hfc_fx_hash_target_info_wwn( wkpp, diag->uni.dev_target.ww_name );
	}
	else {
		/* Scsi_id? */
		target = hfc_fx_hash_target_info( wkpp, (uint)diag->uni.dev_target.target_id );
	}
#endif
	
	for (i=0; i<MAX_TARGET_PROBE; i++) {
		target = wkpp->target_arg[i];
		if (target != NULL) {
			if ( diag->uni.dev_target.ww_name ) {
				/* WWN?*/
				if ( diag->uni.dev_target.ww_name == target->ww_name ) {
					if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ) {
						break;
					}
				}
			}
			else {
				/* target_id? */
				if ( diag->uni.dev_target.target_id == target->target_id ) {
					if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ) {
						break;
					}
				}
			}
		}
	}
	
	if ( target == NULL ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x02 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)target, (char *)(ulong)diag->addr, sizeof(struct target_info_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_TGINFO, 0x03 );
		errlog_no = 0x1b ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_target_info");

	return( 0 );
}


/*
 * Function:    read_manage_info
 *
 * Purpose:     Processing for ioctl(HFCDIAG0) subcommand (Read manage_info) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_manage_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

//	HFC_ENTRY("read_manage_info");

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
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_manage_info") ;

	return( 0 );
}


/*
 * Function:    read_hfctrace
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read hfctrace)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0		Normal end
 *             EINVAL	Specification error
 * Notes:       
 */
int fx_read_hfctrace(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct port_info	*wkpp;

//	HFC_ENTRY("read_hfctrace");
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
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

	if( (diag->uni.xob_xrb_scmd.no >= wkpp->trc_max) ) {
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
	if ( COPYOUT( (char *)(wkpp->trc_ptr + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct hfctrace) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCTRC, 0x04 );
		errlog_no = 0x1e ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	diag->uni.xob_xrb_scmd.current_no = wkpp->trc_num ;

//	HFC_EXIT("read_hfctrace");

	return( 0 );
}


/*
 * Function:    read_mailbox
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read mailbox) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 *
 * Notes:       
 */
int fx_read_mailbox(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
//	HFC_ENTRY("read_mailbox") ;
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct mailbox_fx) ) {	/* Is size short for mailbox? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x04 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)core->mb, (char *)(ulong)diag->addr, sizeof(struct mailbox_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MB, 0x02 );
		errlog_no = 0x1f ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}
	
//	scsi_scan_host(pp->hosts);

//	HFC_EXIT("read_mailbox") ;

	return( 0 );
}


/*
 * Function:    read_version
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read version)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_version(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	uchar	errlog_no ;

//	HFC_ENTRY("read_version");

	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VER, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(hfc_ver) ) {	/* Is size short for hfc_fx_ver? */
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
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_version") ;

	return( 0 );
}


/*
 * Function:    read_hwlog
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read hw_log) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail
 * Notes:       
 */
int fx_read_hwlog(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	uchar	errlog_no ;

//	HFC_ENTRY("read_hwlog") ;

	/*--------------------------------------------------------------*
	 * Specified check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HWLOG, 0x00 );
		return( EINVAL );
	}
	if( diag->length < HFC_FX_HWLOG_SIZE ) {	/* Is size short for hw_log? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HWLOG, 0x01 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(pp->hw_log), (char *)(ulong)diag->addr, HFC_FX_HWLOG_SIZE ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HWLOG, 0x02 );
		errlog_no = 0x21 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	set_bit(HFC_HWLOG_VALID , (ulong *)&pp->io_status);

//	HFC_EXIT("read_hwlog") ;


//	scsi_scan_host(pp->hosts);
	
	return( 0 );
}


/*
 * Function:    read_tree
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read tree)
 *               - Response data is device configuration information
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail 
 * Notes:       
 */
int fx_read_tree(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{

	return( ENOTTY );
}

int fx_read_core_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */

//	HFC_ENTRY("read_core") ;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
	/*--------------------------------------------------------------*
	 * Specificaton check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORE, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct core_info) ) {	/* Is data size short for core ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORE, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORE, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORE, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORE, 0x04 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/	
	
	if ( COPYOUT( (char *)core, (char *)(ulong)diag->addr, sizeof(struct core_info) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORE, 0x05 );
		errlog_no = 0x16 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("read_core_info");

	return( 0 );
}

/*
 * Function:    read_coretrace
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read coretrace)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0		Normal end
 *             EINVAL	Specification error
 * Notes:       
 */
int fx_read_coretrace(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct	core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
//	HFC_ENTRY("read_coretrace");
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfctrace) ) {	/* Is size short for hfctrace? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x04 );
		return( EINVAL );
	}
	
	if( (diag->uni.xob_xrb_scmd.no >= core->trc_max) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x05 );
		return( EINVAL );
	}

	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x06 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)(core->trc_ptr + diag->uni.xob_xrb_scmd.no),
		(char *)(ulong)diag->addr, sizeof(struct hfctrace) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_CORETRC, 0x07 );
		errlog_no = 0x1e ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	diag->uni.xob_xrb_scmd.current_no = core->trc_num ;

//	HFC_EXIT("read_coretrace");

	return( 0 );
}


/* FCLNX-GPL-FX-139 Start */
/*
 * Function:    read_mbtrace
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read coretrace)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0		Normal end
 *             EINVAL	Specification error
 * Notes:       
 */
int fx_read_mbtrace(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct port_info	*wkpp;
	uchar	*tmp_trc;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
//	HFC_ENTRY("read_mbtrace");
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MBTRC, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfc_mb_trace)*100 ) {	/* Is size short for hfctrace? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MBTRC, 0x01 );
		return( EINVAL );
	}
	
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	tmp_trc=(char *)&wkpp->mb_trace[0];
	if ( COPYOUT( (char *)tmp_trc,(char *)(ulong)diag->addr, sizeof(struct hfc_mb_trace)*100 ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_MBTRC, 0x04 );
		errlog_no = 0x1e ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

	diag->uni.xob_xrb_scmd.current_no = wkpp->current_mbtrc_no ;

//	HFC_EXIT("read_mbtrace");

	return( 0 );
}
/* FCLNX-GPL-FX-139 End */


/*
 * Function:    read_dev_info
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (Read dev_info_fx)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT   Copy process fail  *
 * Notes:       
 */
int fx_read_dev_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;
	int hit;
	int i;
	struct target_info_fx *target;		/* Retrieval target */
	struct dev_info_fx *dev;			/* Retrieval LU */
	struct port_info	*wkpp;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		ret = hfc_manage_info.npubp->read_fx_dev_info_mp(pp, diag);
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
		if( diag->length < sizeof(struct dev_info_fx) ) {	/* Size shortage? */
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x01 );
			return( EINVAL );
		}
	/*--------------------------------------------------------------*
	 * Target retrieval
	 *--------------------------------------------------------------*/
		target = NULL;
		
#if 0
		if ( diag->uni.dev_target.ww_name ) {
		/* WWN specification */
			target = hfc_fx_hash_target_info_wwn( wkpp, diag->uni.dev_target.ww_name );
		}
		else {
		/* Scsi_id specification */
			target = hfc_fx_hash_target_info( wkpp, (uint)diag->uni.dev_target.target_id );
		}
#endif
		for (i=0; i<MAX_TARGET_PROBE; i++) {
			target = wkpp->target_arg[i];
			if (target != NULL) {
				if ( diag->uni.dev_target.ww_name ) {
					/* WWN?*/
					if ( diag->uni.dev_target.ww_name == target->ww_name ) {
						if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ) {
							break;
						}
					}
				}
				else {
					/* target_id? */
					if ( diag->uni.dev_target.target_id == target->target_id ) {
						if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ) {
							break;
						}
					}
				}
			}
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
			if ( dev->lun == (uint)diag->uni.dev_target.lun_id ) {
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
		if ( COPYOUT( (char *)dev, (char *)(ulong)diag->addr, sizeof(struct dev_info_fx) ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DEV, 0x05 );
			return ( EFAULT );
		}

//		HFC_EXIT("read_dev_info_mp");

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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_read_lg_target_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;

	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_fx_lg_target_info_mp(pp, diag);
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_read_lg_dev_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;

	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_fx_lg_dev_info_mp(pp, diag);
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_read_lg_path_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;

	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_fx_lg_path_info_mp(pp, diag);
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_read_lg_path_info1(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;

	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_fx_lg_path_info1_mp(pp, diag);
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_read_lg_path_info2(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;

	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_fx_lg_path_info2_mp(pp, diag);
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_read_failover_info(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	int ret;

	if(hfc_manage_info.hfcldd_mp_mod) {
		ret = hfc_manage_info.npubp->read_fx_failover_info_mp(pp, diag);
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
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     
 *  0          - Normal end
 *  EIO        - Other errors
 * Notes:       
 */
int fx_online_update(
	struct	port_info	*pp,	/* struct port_info */
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
	struct		region_info *rp = NULL;
	struct		core_info *core[MAX_CORE_PROBE_FX] = {0};
	ulong		flags=0;	/* FCLNX-GPL-517 */
	int			i;
	
	HFC_DBGPRT("fx_online_update\n");
	
	rp = pp->region_arg[pp->rid];
	if( !rp ){
		return( EINVAL );
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

	/* Online updadate ppplicable? ( Check adapter status ) */
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core[i] = rp->core_arg[i];
		if(test_bit(HFC_CS_CHK_STOP, (ulong *)&core[i]->status)){ /* FCLNX-GPL-FX-438 */
			HFC_DBGPRT("diag online_update : core[%d]->status\n",i); 
			diag->uni.online_up.errcode = 0x01;
			goto online_update_end;
		}
	}
	if( test_bit(HFC_PS_ISOL,		(ulong *)&pp->status ) || /* Adppter HW isolation state *//* FCLNX-GPL-387 */
		test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) ||
		test_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 ) )		/* FCLNX-GPL-FX-180 */
	{
		HFC_DBGPRT("diag online_update : pp->status\n"); 
		diag->uni.online_up.errcode = 0x01;
		goto online_update_end;
	}
	
	/* Online updadate ppplicable? ( Check HW status ) */
	status_reg.l = 0;
	wk = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS0, 0x4 );
	HFC_4B_TO_4L(status_reg.l, wk);
	
	/* FCLNX-GPL-282 */
	/* We check 3 bits. "HFC_PCI_EXGMCK", "HFC_PCI_BOOTRUN", and "HFC_PCI_FCNSTOP". */
	/* But, never check the bit of "HFC_PCI_PCIERR_DETECTED". */
	/* Because, the bit will be standing, in spite of HBA's condition which is correctable error. */
	if( status_reg.c[1] & ( HFC_PCI_EXGMCK | HFC_PCI_BOOTRUN | HFC_PCI_FCNSTOP ) )
	{
		HFC_DBGPRT("diag online_update : status_reg.c\n");
		/* Online update is not ppplicable */
		diag->uni.online_up.errcode = 0x01;
		goto online_update_end;
	}
	
	/* Is FW operation mode is not NORMAL */
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
//		fpp_mode = hfc_fx_read_reg_ext(pp, hfc_flag_of_core[i], 0x1);
				fpp_mode = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_CA_FLAG,
											  0x04, HFC_FX_CORE_OFFSET80);
				if ( fpp_mode != FW_MODE_NORMAL ) {
					HFC_DBGPRT("diag online_update : fpp_mode != FW_MODE_NORMAL\n");
					/* OnlineUpdate */
					diag->uni.online_up.errcode = 0x01;
					goto online_update_end;
				}
			}
		}
	}
	
	/* Online update process is waiting (busy state) */
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
//		online_up_state = hfc_fx_read_reg_ext(pp, hfc_online_update_of_core[i], 0x1);
		online_up_state = (uint)hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_CA_ONUP_STATE,
													 0x04, HFC_FX_CORE_OFFSET80);
		if ( online_up_state == 0x01 ) {
			HFC_DBGPRT("diag online_update : online_up_state == 0x01\n");
			diag->uni.online_up.errcode = 0x02;
			goto online_update_end;
		}
	}
	
	/*--------------------------------------*/
	/* Start F/W online update operation	*/
	/*--------------------------------------*/
	/* Initiate FRAME_A :  Online Update */
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_FRAMEA,
			(char)0x4, (int)HFC_FRAMEA_ONLINEUP, HFC_FX_CORE_OFFSET40);
	}
	
	/* Collect error log ( ErrNo = 0xa6 ) */
	memset(logdata,0,16);
	memcpy(logdata, (uchar*)&diag->uni.online_up.before_sysrev,4);
	memcpy(&logdata[4],(uchar *)&diag->uni.online_up.after_sysrev,4) ;
	hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4, 0xa6, logdata, 16);

	diag->uni.online_up.errcode = 0x00;
	
	HFC_DBGPRT("diag online_update : end\n");
online_update_end : 
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

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
int fx_hg_access(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;

	uint	wk_data;			/* Internal data area */
	ushort	wk2 ;
	uint	wk4 ;

//	HFC_ENTRY("hg_access");

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
	if( HFC_FX_MMODE_CHECK_BASIC (pp) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x01 );
		diag->uni.pci.err_code = HG_BASIC;
		return( EINVAL );
	}
	if (HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-223 *//* FCLNX-GPL-495 */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x02 );
		diag->uni.pci.err_code = HG_SHADOW;
	}
	else if (HFC_FX_MMODE_CHECK_DEDICATE(pp) ){ /* FCLNX-GPL-223 *//* FCLNX-GPL-495 */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL_PCIACC, 0x03 );
		diag->uni.pci.err_code = HG_DEDICATED;
	}
	else if (HFC_FX_MMODE_CHECK_SHARED(pp) ){ /* FCLNX-GPL-223 *//* FCLNX-GPL-495 */
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

	if( pp->pkg.type == HFC_PKTYPE_FPP ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PCIACC, 0x02 );
		diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
		return( EINVAL );			
	}
	
	/* Address check */
	if( diag->uni.pci.addr > fx_hg_addr_max[pp->pkg.type] ) {
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
	if( (diag->uni.pci.addr + diag->length) > fx_hg_addr_max[pp->pkg.type] +1 ) {
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
		wk_data = hfc_fx_read_reg_hg_ext(pp, diag->uni.pci.addr, diag->length) ;
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_fx_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_fx_read_val(wk2); break;
		}
		/* Copy read data to user area (wk_datad -> iag->addr) */
		if ( COPYOUT( (char *)&wk_data, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_PCIACC, 0x05 );
			errlog_no = 0x22 ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
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
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
			diag->uni.pci.err_code = HG_NONE; /* FCLNX-GPL-223 */
			return ( EFAULT );
		}
		switch( diag->length )
		{
			case PCI_LENGTH_04: wk4 = (uint)   wk_data; wk_data = hfc_fx_read_val(wk4); break;
			case PCI_LENGTH_02: wk2 = (ushort) wk_data; wk_data = hfc_fx_read_val(wk2); break;
		}
		/* PCI memory write */
		hfc_fx_write_reg_hg_ext(pp, diag->uni.pci.addr, diag->length, wk_data);
	}

//	HFC_EXIT("hg_access");

	return( 0 );
}

/* FCLNX-GPL-492 */
/*
 * Function:    scan_target_fcsw
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (scan_target_fcsw ) 
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
int fx_scan_target_fcsw(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	port_info		*wkpp;
	struct	region_info		*rp;
	struct	target_info_fx	*target=NULL;	/* FCLNX-GPL-FX-112 */
	
	int		i,lp;	/* FCLNX-GPL-FX-112 */
	ulong	flags=0;
	
	for (i=0; i<=pp->max_vport_count; i++) {
		wkpp = pp->vport_ptr[i].vport_arg;
		if (wkpp == NULL)
			continue;
		
		rp = pp->region_arg[wkpp->rid];
		if (rp == NULL)
			continue;
		
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		
		if (test_bit(HFC_PS_ONLINE, (ulong *)&wkpp->status)) {
			if( !((wkpp ->connect_type==HFC_FX_SWITCH ) || 
				( (wkpp->connect_type==HFC_AL) && (wkpp->scsi_id & 0x00ffff00) )) ) {
				/* not FC-SW */
				HFC_DBGPRT("fx_scan_target_fcsw - HFC_TGTSCAN_NOT_SWITCH\n");
				for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++){	/* FCLNX-GPL-FX-112 Start */
					target = wkpp->target_arg[lp];
					if( target != NULL ){
						if(test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags))continue;	/* FCLNX-GPL-FX-475 */
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
						atomic_set(&wkpp->check_mbreq, 1);
						start_fx_next_mailbox(wkpp, NULL);
					}
				}										/* FCLNX-GPL-FX-112 End */
//				diag->uni.tgtscan.errcode = HFC_TGTSCAN_NOT_SWITCH;
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				return( 0 );
			}
			
			if ((wkpp->status & (~((0x00000001 << HFC_PS_ENABLE) |
				(0x00000001 << HFC_PS_ONLINE) |
				(0x00000001 << HFC_PS_CONNECTED)))) ||
				(wkpp->status_detail1) ||
				(wkpp->status_detail2)) {
				/* status busy  */
				HFC_DBGPRT("fx_scan_target_fcsw - HFC_TGTSCAN_ADAP_STATUS\n");
				diag->uni.tgtscan.errcode = HFC_TGTSCAN_ADAP_STATUS;
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				return( 0 );
			}
		}
		else {
			/* link down */
			HFC_DBGPRT("fx_scan_target_fcsw - HFC_TGTSCAN_LINKDOWN\n");
			diag->uni.tgtscan.errcode = HFC_TGTSCAN_LINKDOWN;
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			return( 0 );
		}
		
		set_bit(HFC_PD_NEED_GPNFT, (ulong *)&wkpp->status_detail2);
		atomic_set(&wkpp->check_mbreq, 1);
		start_fx_next_mailbox(wkpp, NULL);
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	}
	
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
int fx_change_param(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct port_info		*vpp;
	struct region_info		*rp;
	struct target_info_fx	*target;
	struct dev_info_fx		*dev;
	ulong		flags=0;	/* FCLNX-GPL-517 */
	int			i,j;
	int			clear_seq_cnt = 0;
	int			assign_core_no = 0;
	int			large_cmd = 0;
	uchar		cur_cc_core = 0;
	uchar		pm_pool_alloc = 0;
	uchar		logdata[16];
	
	if ((pp->defparam) && (diag->uni.changeparm.ignore_force_default == 0)) {
		diag->uni.changeparm.errcode = HFC_CHGPRM_FORCE_DEF;
		return(0);
	}
	
	diag->uni.changeparm.errcode = HFC_CHGPRM_NORMAL;
	
	rp = pp->region_arg[pp->rid];
	if( !rp ){
		return( EINVAL );
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	HFC_ALLCORELOCK(rp);
	
	if (diag->uni.changeparm.version >= 1) {
		switch (diag->uni.changeparm.opr_limit_log) { /* Limit Log Mode */
			case HFC_CHGPRM_OPR_SET:
				pp->limit_log = diag->uni.changeparm.val_limit_log;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->limit_log = HFC_DISABLE_LIMITLOG;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm.val_limit_log = pp->limit_log;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
#if 0
		switch (diag->uni.changeparm.opr_filter_target_fx) { /* Filtering Login Target */
		case HFC_CHGPRM_OPR_SET:
			pp->filter_target = diag->uni.changeparm.val_filter_target_fx;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->filter_target = HFC_DISABLE_FILTERTGT;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_filter_target_fx = pp->filter_target;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
#endif		
		switch(diag->uni.changeparm.opr_core_control){ /* Core Control */
		case HFC_CHGPRM_OPR_SET:
			pp->core_control = diag->uni.changeparm.val_core_control;
			if (pp->core_control == HFC_FX_CORECTL_SEQUENTIAL) {
				clear_seq_cnt = 1;
			}
			else if (pp->core_control == HFC_FX_CORECTL_AT_ASSGN_LU) {
				assign_core_no = 1;
			}
			if (clear_seq_cnt || assign_core_no) {
				for(i=0 ; i<MAX_TARGET_PROBE ; i++) {
					target = hfc_fx_hash_target_info(pp, i);
					if (target != NULL) {
						dev = target->dev;
						while( dev != NULL){
							if (clear_seq_cnt) {
								dev->seq_cnt = 0;
							}
							else if (assign_core_no) {
								hfc_fx_assign_core_no(pp, dev);
							}
							dev = dev->next;
						}
					}
				}
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->core_control = HFC_FX_CORECTL_ENHANCE_RR;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_core_control = pp->core_control;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_link_reset){ /* Link Reset */
		case HFC_CHGPRM_OPR_SET:
			pp->link_reset = diag->uni.changeparm.val_link_reset;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->link_reset = HFC_FX_LINK_RESET_MULTI;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_link_reset = pp->link_reset;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
			
		switch(diag->uni.changeparm.opr_link_down){ /* Link UP Wait time */
		case HFC_CHGPRM_OPR_SET:
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
				pp->dev_loss_tmo = diag->uni.changeparm.val_link_down;
				hfc_fx_change_dev_loss_tmo(pp);
			} else {
				pp->linkup_tmo = diag->uni.changeparm.val_link_down;
			}
#else
			pp->linkup_tmo = diag->uni.changeparm.val_link_down;
#endif
			break;
		case HFC_CHGPRM_OPR_DEL:
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
				pp->dev_loss_tmo = HFC_DEF_DEV_LOSS_TMO;
			else
				pp->linkup_tmo = HFC_PCM_LINKUP_TO;
#else
			pp->linkup_tmo = HFC_PCM_LINKUP_TO;
#endif
			break;
		case HFC_CHGPRM_OPR_READ:
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
				diag->uni.changeparm.val_link_down = pp->dev_loss_tmo;
			else
				diag->uni.changeparm.val_link_down = pp->linkup_tmo;
#else
			diag->uni.changeparm.val_link_down = pp->linkup_tmo;
#endif
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_reset_delay){ /* Reset Delay Wait time */
		case HFC_CHGPRM_OPR_SET:
			pp->scsi_reset_delay = diag->uni.changeparm.val_reset_delay;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->scsi_reset_delay = HFC_SCSI_RESET_DELAY_MIN;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_reset_delay = pp->scsi_reset_delay;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
			
		switch(diag->uni.changeparm.opr_mck_retry){ /* Machine Check Retry */
		case HFC_CHGPRM_OPR_SET:
			pp->mck_err_cnt = 0;
			for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				pp->region_arg[pp->rid]->core_arg[i]->mck_err_cnt = 0;
			}
			pp->max_mck_cnt = diag->uni.changeparm.val_mck_retry;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->max_mck_cnt = HFC_FX_DF_MAX_MCK_CNT;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_mck_retry = pp->max_mck_cnt;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}

		switch(diag->uni.changeparm.opr_reset_timeout){ /* Target Reset timeout */
		case HFC_CHGPRM_OPR_SET:
			pp->target_reset_tmo = diag->uni.changeparm.val_reset_timeout;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->target_reset_tmo = HFC_TARGET_RST_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_reset_timeout = pp->target_reset_tmo;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_abort_timeout){ /* Abort TS timeout */
		case HFC_CHGPRM_OPR_SET:
			pp->abort_tmo = diag->uni.changeparm.val_abort_timeout;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->abort_tmo = HFC_ABORT_ACA_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_abort_timeout = pp->abort_tmo;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_queue_depth){ /* Queue Depth */
		case HFC_CHGPRM_OPR_SET:
			pp->queue_depth = diag->uni.changeparm.val_queue_depth;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->queue_depth = HFC_DEFAULT_QUEUE_DEPTH;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_queue_depth = pp->queue_depth;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_scsi_allowed){ /* SCSI retry allowed */
		case HFC_CHGPRM_OPR_SET:
			pp->scsi_allowed = diag->uni.changeparm.val_scsi_allowed;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->scsi_allowed = HFC_SCSI_ALLOWED;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_scsi_allowed = pp->scsi_allowed;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_ld_err_intvl){ /* Link down Error intvl */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				
				pp->ld_err_intvl = diag->uni.changeparm.val_ld_err_intvl;
				
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				pp->ld_err_intvl = HFC_DF_LD_ERR_INTVL;
				
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_ld_err_intvl = pp->ld_err_intvl;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_ld_err_limit_l){ /* Link down Error limit */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				pp->ld_err_limit_l = diag->uni.changeparm.val_ld_err_limit_l;
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				pp->ld_err_limit_l = HFC_MIN_LD_ERR_LIMIT_L;
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_ld_err_limit_l = pp->ld_err_limit_l;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_ld_err_limit_s){ /* Link down Error limit */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->ld_err_limit_s = diag->uni.changeparm.val_ld_err_limit_s;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->ld_err_limit_s = HFC_MIN_LD_ERR_LIMIT_S;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_ld_err_limit_s = pp->ld_err_limit_s;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_if_err_intvl){ /* FC Error Error intvl */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				
				pp->if_err_intvl = diag->uni.changeparm.val_if_err_intvl;
				
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				pp->if_err_intvl = HFC_DF_IF_ERR_INTVL;
				
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_if_err_intvl = pp->if_err_intvl;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_if_err_limit){ /* FC Error limit */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->if_err_limit = diag->uni.changeparm.val_if_err_limit;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->if_err_limit = HFC_MIN_IF_ERR_LIMIT;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_if_err_limit = pp->if_err_limit;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_to_err_intvl){ /* Timeout Error intvl */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				
				pp->to_err_intvl = diag->uni.changeparm.val_to_err_intvl;
				
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
				pp->to_err_intvl = HFC_DF_TO_ERR_INTVL;
				
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_to_err_intvl = pp->to_err_intvl;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}	/* FCLNX-GPL-FX-314 End */
		
		switch(diag->uni.changeparm.opr_to_err_limit){ /* Timeout Error limit */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->to_err_limit = diag->uni.changeparm.val_to_err_limit;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->to_err_limit = HFC_MIN_TO_ERR_LIMIT;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-314 Start */
				hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info(pp);
			}	/* FCLNX-GPL-FX-314 End */
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_to_err_limit = pp->to_err_limit;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_rt_err_enable){ /* Reset Error Enable */
		case HFC_CHGPRM_OPR_SET:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->rt_err_enable = diag->uni.changeparm.val_rt_err_enable;
			break;
		case HFC_CHGPRM_OPR_DEL:
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
			pp->rt_err_enable = HFC_RT_ERR_NOT_SPPRTD;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_rt_err_enable = pp->rt_err_enable;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_hba_isolation){ /* hba_isolation */
		case HFC_CHGPRM_OPR_SET:
			if(diag->uni.changeparm.val_hba_isolation == HFC_ISOL_START){
				hfc_fx_start_isolate(pp);
			}else if(diag->uni.changeparm.val_hba_isolation == HFC_ISOL_STOP){
				if(hfc_fx_check_hba_isolation(pp)){								/* FCLNX-GPL-414 */
					pp->hba_isolation = HFC_ISOL_STOP;
				}
			}
			break;
		case HFC_CHGPRM_OPR_DEL:
			if(hfc_fx_check_hba_isolation(pp)){								/* FCLNX-GPL-414 */
				pp->hba_isolation = HFC_ISOL_STOP;
			}
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_hba_isolation = pp->hba_isolation;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_rdtsc){ /* rdtsc */
		case HFC_CHGPRM_OPR_SET:
			pp->pm_control = diag->uni.changeparm.val_rdtsc;
			if (pp->pm_control)
				pm_pool_alloc = 1;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->pm_control = HFC_FX_PM_OFF;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_rdtsc = pp->pm_control;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		if (pm_pool_alloc) {
			/* alloc pm_pkt_pool */
			if (pp->pm_pkt_pool == NULL) {
				pp->pm_pkt_pool = (struct hfc_pm_pkt_fx *)hfc_fx_kmalloc(pp, (sizeof(struct hfc_pm_pkt_fx)*pp->pm_pkt_num), GFP_ATOMIC);
				if (pp->pm_pkt_pool == NULL) {
					memset(logdata, 0, 16);
					logdata[0] = 0xf0;
					hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
				}
				else {
					memset( pp->pm_pkt_pool, 0, sizeof(struct hfc_pm_pkt_fx)*pp->pm_pkt_num );
				}
			}
		}
		
		switch(diag->uni.changeparm.opr_intdisable){ /* intdisable */
		case HFC_CHGPRM_OPR_SET:
			hfc_fx_write_reg_ext(pp, 0x23f, 0x01, 0x01);
			hfc_fx_write_reg_ext(pp, 0x23e, 0x01, (uchar)diag->uni.changeparm.val_intdisable);
			break;
		case HFC_CHGPRM_OPR_DEL:
			hfc_fx_write_reg_ext(pp, 0x23e, 0x01, 0);
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_intdisable = hfc_fx_read_reg_ext(pp, 0x23e, 0x01);
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_intenable){ /* intenable */
		case HFC_CHGPRM_OPR_SET:
			hfc_fx_write_reg_ext(pp, 0x23f, 0x01, (uchar)diag->uni.changeparm.val_intenable);
			break;
		case HFC_CHGPRM_OPR_DEL:
			hfc_fx_write_reg_ext(pp, 0x23f, 0x01, 0);
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_intenable = hfc_fx_read_reg_ext(pp, 0x23f, 0x01);
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		/* FCLNX-GPL-FX-014 Start */
		switch(diag->uni.changeparm.opr_total_abort_to){ /* Total Abort Timer */
		case HFC_CHGPRM_OPR_SET:
			pp->total_abort_to = diag->uni.changeparm.val_total_abort_to;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->total_abort_to = HFC_FX_DISABLE_TOTAL_ABORT_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_total_abort_to = pp->total_abort_to;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_total_tgtrst_to){ /* Total Target Reset Timer */
		case HFC_CHGPRM_OPR_SET:
			pp->total_tgtrst_to = diag->uni.changeparm.val_total_tgtrst_to;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->total_tgtrst_to = HFC_FX_DISABLE_TOTAL_TGTRST_TO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_total_tgtrst_to = pp->total_tgtrst_to;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		/* FCLNX-GPL-FX-014 End */
		
		switch(diag->uni.changeparm.opr_cc_cnt){ /* same core seq cnt for core_ctl */
		case HFC_CHGPRM_OPR_SET:
			pp->cc_cnt = diag->uni.changeparm.val_cc_cnt;
			clear_seq_cnt = 1;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->cc_cnt = HFC_FX_DEF_CC_CNT;
			clear_seq_cnt = 1;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_cc_cnt = (ushort) pp->cc_cnt;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		if (clear_seq_cnt) {
			for(i=0 ; i<MAX_TARGET_PROBE ; i++) {
				target = hfc_fx_hash_target_info(pp, i);
				if (target != NULL) {
					dev = target->dev;
					while( dev != NULL){
						dev->seq_cnt = 0;
						dev = dev->next;
					}
				}
			}
		}
		
		switch(diag->uni.changeparm.opr_cc_size){ /* large cmd size cnt for core_ctl */
		case HFC_CHGPRM_OPR_SET:
			if (pp->region_arg[pp->rid]->core_arg[pp->master_core_no]->we_que_cnt_all != 0) {	/* FCLNX-GPL-FX-385 */
				diag->uni.changeparm.errcode = HFC_CHGPRM_BUSY;
				break;
			}
			
			pp->cc_size = diag->uni.changeparm.val_cc_size;
			large_cmd = 1;
			break;
		case HFC_CHGPRM_OPR_DEL:
			if (pp->region_arg[pp->rid]->core_arg[pp->master_core_no]->we_que_cnt_all != 0) {	/* FCLNX-GPL-FX-385 */
				diag->uni.changeparm.errcode = HFC_CHGPRM_BUSY;
				break;
			}
			
			pp->cc_size = HFC_FX_DEF_CC_SIZE;
			large_cmd = 1;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_cc_size = (ushort)pp->cc_size;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_cc_core){ /* core_list for core_ctl */
		case HFC_CHGPRM_OPR_SET:
			pp->cc_core = diag->uni.changeparm.val_cc_core;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->cc_core = HFC_FX_DEF_CC_CORE;
			break;
		case HFC_CHGPRM_OPR_READ:
			for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
				if (rp->core_arg[i] == NULL) { continue; }
				if (rp->core_arg[i]->status & (0x00000001 << HFC_CS_CHK_STOP)) { continue; }
				cur_cc_core |= (pp->cc_core & (0x01 << i));
			}
			diag->uni.changeparm.val_cc_core = cur_cc_core;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_maxio){ /* max io */
		case HFC_CHGPRM_OPR_SET:
			pp->max_io = diag->uni.changeparm.val_maxio;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->max_io = HFC_FX_DEFAULT_MAX_IO;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_maxio = (ushort)pp->max_io;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		/* FCLNX-GPL-FX-175 Start *//* FCLNX-GPL-FX-243,272 Start */
		switch(diag->uni.changeparm.opr_abort_t_restrain){ /* abort_t_restrain */
		case HFC_CHGPRM_OPR_SET:
			pp->abort_t_restrain = diag->uni.changeparm.val_abort_t_restrain;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->abort_t_restrain = HFC_ABORT_T_RESTRAIN;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_abort_t_restrain = (uchar)pp->abort_t_restrain;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_tgtrst_restrain){ /* tgtrst_restrain */
		case HFC_CHGPRM_OPR_SET:
			pp->tgtrst_restrain = diag->uni.changeparm.val_tgtrst_restrain;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->tgtrst_restrain = HFC_FX_TGTRST_RESTRAIN_DISABLE;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_tgtrst_restrain = (uchar)pp->tgtrst_restrain;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		
		switch(diag->uni.changeparm.opr_lun_reset_delay){ /* lun_reset_delay */
		case HFC_CHGPRM_OPR_SET:
			pp->lun_reset_delay = diag->uni.changeparm.val_lun_reset_delay;
			break;
		case HFC_CHGPRM_OPR_DEL:
			pp->lun_reset_delay = HFC_LUN_DELAY;
			break;
		case HFC_CHGPRM_OPR_READ:
			diag->uni.changeparm.val_lun_reset_delay = (uchar)pp->lun_reset_delay;
			break;
		case HFC_CHGPRM_OPR_NONE:
		default:
			break;
		}
		/* FCLNX-GPL-FX-175 End *//* FCLNX-GPL-FX-243,272 End */
		
	}
	
	HFC_ALLCOREUNLOCK(rp);
	
	for (i=1; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		rp = pp->region_arg[vpp->rid];
		if (rp == NULL)
			continue;
		
		HFC_ALLCORELOCK(rp);
		
		if (large_cmd) {
			if (rp->core_arg[pp->master_core_no]->we_que_cnt_all != 0) {
				diag->uni.changeparm.errcode = HFC_CHGPRM_VPORT_BUSY;
			}
			else {
				hfc_fx_param_copy(pp, vpp);
			}
		}
		else {
			hfc_fx_param_copy(pp, vpp);
		}
		
		if (clear_seq_cnt || assign_core_no) {
			for(j=0 ; j<MAX_TARGET_PROBE ; j++) {
				target = hfc_fx_hash_target_info(vpp, j);
				if (target != NULL) {
					dev = target->dev;
					while( dev != NULL){
						if (clear_seq_cnt) {
							dev->seq_cnt = 0;
						}
						else if (assign_core_no) {
							hfc_fx_assign_core_no(vpp, dev);
						}
						dev = dev->next;
					}
				}
			}
		}
		
		if (pm_pool_alloc) {
			/* alloc pm_pkt_pool */
			if (vpp->pm_pkt_pool == NULL) {
				vpp->pm_pkt_pool = (struct hfc_pm_pkt_fx *)hfc_fx_kmalloc(vpp, (sizeof(struct hfc_pm_pkt_fx)*vpp->pm_pkt_num), GFP_ATOMIC);
				if (vpp->pm_pkt_pool == NULL) {
					memset(logdata, 0, 16);
					logdata[0] = 0xf1;
					hfc_fx_errlog(vpp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
					HFC_ALLCOREUNLOCK(rp);
					break;
				}
				memset( vpp->pm_pkt_pool, 0, sizeof(struct hfc_pm_pkt_fx)*vpp->pm_pkt_num );
			}
		}
		
		HFC_ALLCOREUNLOCK(rp);
	}
	
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	
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
int fx_change_param_flash(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct port_info	*vpp;
	struct region_info	*rp;
	int   i;
	uchar wk;
	ulong	flags = 0;	/* FCLNX-GPL-517 */
	
	rp = pp->region_arg[pp->rid];
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	HFC_ALLCORELOCK(rp);
	
	if (diag->uni.changeparm_flash.version >= 1) {
		switch (diag->uni.changeparm_flash.opr_mck_linkup_timer) { /* MCK Linkup timer */
			case HFC_CHGPRM_OPR_SET:
				pp->mck_rcv_tmo = diag->uni.changeparm_flash.val_mck_linkup_timer;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mck_rcv_tmo = HFC_FX_MCK_RCV_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mck_linkup_timer = pp->mck_rcv_tmo;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_core_degradation) { /* CORE degration mode */
			case HFC_CHGPRM_OPR_SET:
				pp->core_deg_mode = diag->uni.changeparm_flash.val_core_degradation;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->core_deg_mode = HFC_FX_CORE_DEG_ENABLE;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_core_degradation = pp->core_deg_mode;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_rft_id_skip) { /* RFT_ID_SKIP */
			case HFC_CHGPRM_OPR_SET:
				pp->rft_id_skip = diag->uni.changeparm_flash.val_rft_id_skip;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->rft_id_skip = HFC_FX_RFT_ID_SKIP_DISABLE;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_rft_id_skip = pp->rft_id_skip;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_hba_isol_cmd) { /* ISOL cmd */
			case HFC_CHGPRM_OPR_SET:
				pp->isol_cmd = diag->uni.changeparm_flash.val_hba_isol_cmd;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->isol_cmd = HFC_FX_ISOL_CMD_OFF;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_hba_isol_cmd = pp->isol_cmd;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_link_init_timer) { /* Link init timer */
			case HFC_CHGPRM_OPR_SET:
				pp->link_initialize_tmo = diag->uni.changeparm_flash.val_link_init_timer;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->link_initialize_tmo = HFC_FX_DF_LINK_INIT_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_link_init_timer = pp->link_initialize_tmo;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[0]) { /* MB response timer (Core start - Link Ini) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_CORE_START].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[0];
				pp->mb_timer[HFC_MBTIME_LINK_INI].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[0];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_CORE_START].tout = HFC_FX_DF1_MB_TO;
				pp->mb_timer[HFC_MBTIME_LINK_INI].tout = HFC_FX_DF1_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[0] = pp->mb_timer[HFC_MBTIME_CORE_START].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[0]) { /* MB response timer (Core start - Link Ini) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[0] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[0] & 0x80) {
					if((wk >= HFC_MBTIME_RETRY_MIN)&& (wk <= HFC_MBTIME_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_CORE_START].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[0];
						pp->mb_timer[HFC_MBTIME_LINK_INI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[0];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_CORE_START].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_LINK_INI].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_CORE_START].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[0];
						pp->mb_timer[HFC_MBTIME_LINK_INI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[0];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_CORE_START].retry = HFC_FX_DF1_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_LINK_INI].retry = HFC_FX_DF1_MB_RETRY;
					}
				}
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_CORE_START].retry = HFC_FX_DF1_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_LINK_INI].retry = HFC_FX_DF1_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[0] = pp->mb_timer[HFC_MBTIME_CORE_START].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[0]) { /* MB retry delay (Core start - Link Ini) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_CORE_START].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[0];
				pp->mb_timer[HFC_MBTIME_LINK_INI].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[0];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_CORE_START].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_LINK_INI].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[0] = pp->mb_timer[HFC_MBTIME_CORE_START].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[1]) { /* MB response timer (FLOGI) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_FLOGI].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[1];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_FLOGI].tout = HFC_FX_DF2_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[1] = pp->mb_timer[HFC_MBTIME_FLOGI].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[1]) { /* MB response timer (FLOGI) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[1] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[1] & 0x80) {
					if((HFC_MBTIME_RETRY_MIN <= wk) && (wk <= HFC_MBTIME_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_FLOGI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[1];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_FLOGI].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_FLOGI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[1];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_FLOGI].retry = HFC_FX_DF2_MB_RETRY;
					}
				}
				break;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_FLOGI].retry = HFC_FX_DF2_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[1] = pp->mb_timer[HFC_MBTIME_FLOGI].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[1]) { /* MB retry delay (FLOGI) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_FLOGI].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[1];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_FLOGI].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[1] = pp->mb_timer[HFC_MBTIME_FLOGI].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[2]) { /* MB response timer (PLODI - PDISK) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_PLOGI].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[2];
				pp->mb_timer[HFC_MBTIME_PDISC].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[2];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_PLOGI].tout = HFC_FX_DF3_MB_TO;
				pp->mb_timer[HFC_MBTIME_PDISC].tout = HFC_FX_DF3_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[2] = pp->mb_timer[HFC_MBTIME_PLOGI].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[2]) { /* MB response timer (PLODI - PDISK) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[2] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[2] & 0x80) {
					if((HFC_MBTIME_RETRY_MIN <= wk) && (wk <= HFC_MBTIME_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_PLOGI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[2];
						pp->mb_timer[HFC_MBTIME_PDISC].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[2];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_PLOGI].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_PDISC].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_PLOGI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[2];
						pp->mb_timer[HFC_MBTIME_PDISC].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[2];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_PLOGI].retry = HFC_FX_DF3_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_PDISC].retry = HFC_FX_DF3_MB_RETRY;
					}
				}
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_PLOGI].retry = HFC_FX_DF3_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_PDISC].retry = HFC_FX_DF3_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[2] = pp->mb_timer[HFC_MBTIME_PLOGI].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[2]) { /* MB retry delay (PLODI - PDISK) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_PLOGI].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[2];
				pp->mb_timer[HFC_MBTIME_PDISC].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[2];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_PLOGI].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_PDISC].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[2] = pp->mb_timer[HFC_MBTIME_PLOGI].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[3]) { /* MB response timer (Offline mb - CANCEL SCSI) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_OFFLINE].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[3];
				pp->mb_timer[HFC_MBTIME_CAN_SCSI].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[3];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_OFFLINE].tout = HFC_FX_DF4_MB_TO;
				pp->mb_timer[HFC_MBTIME_CAN_SCSI].tout = HFC_FX_DF4_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[3] = pp->mb_timer[HFC_MBTIME_OFFLINE].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[3]) { /* MB response timer (Offline mb - CANCEL SCSI) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[3] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[3] & 0x80) {
					if((HFC_MBTIME_RETRY_MIN <= wk) && (wk <= HFC_MBTIME_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_OFFLINE].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[3];
						pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[3];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_OFFLINE].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_OFFLINE].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[3];
						pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[3];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_OFFLINE].retry = HFC_FX_DF4_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = HFC_FX_DF4_MB_RETRY;
					}
				}
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_OFFLINE].retry = HFC_FX_DF4_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = HFC_FX_DF4_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[3] = pp->mb_timer[HFC_MBTIME_OFFLINE].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[3]) { /* MB retry delay (Offline mb - CANCEL SCSI) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_OFFLINE].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[3];
				pp->mb_timer[HFC_MBTIME_CAN_SCSI].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[3];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_OFFLINE].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_CAN_SCSI].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[3] = pp->mb_timer[HFC_MBTIME_OFFLINE].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[4]) { /* MB response timer (ELS) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_PRLI].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_PRLO].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_SCR].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_LOGO].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_AUTH_RJT].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_AUTH_NEGO].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_EVFP_SYNC].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_PRLI].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_PRLO].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_SCR].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_LOGO].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_AUTH_RJT].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_AUTH_NEGO].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_EVFP_SYNC].tout = HFC_FX_DF5_MB_TO;
				pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].tout = HFC_FX_DF5_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[4] = pp->mb_timer[HFC_MBTIME_PRLI].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[4]) { /* MB response timer (ELS) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[4] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[4] & 0x80) {
					if((HFC_MBTIME_RETRY_MIN <= wk) && (wk <= HFC_MBTIME_RETRY_MAX)){
						pp->mb_timer[HFC_MBTIME_PRLI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_PRLO].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_SCR].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_LOGO].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_PRLI].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_PRLO].retry =HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_SCR].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_LOGO].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_PRLI].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_PRLO].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_SCR].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_LOGO].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
						pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[4];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_PRLI].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_PRLO].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_SCR].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_LOGO].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry = HFC_FX_DF5_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry = HFC_FX_DF5_MB_RETRY;
					}
				}
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_PRLI].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_PRLO].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_SCR].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_LOGO].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry = HFC_FX_DF5_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry = HFC_FX_DF5_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[4] = pp->mb_timer[HFC_MBTIME_PRLI].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[4]) { /* MB retry delay (ELS) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_PRLI].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_PRLO].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_SCR].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_LOGO].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_AUTH_RJT].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_AUTH_NEGO].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_EVFP_SYNC].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[4];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_PRLI].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_PRLO].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_SCR].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_LOGO].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_AUTH_RJT].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_AUTH_NEGO].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_EVFP_SYNC].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[4] = pp->mb_timer[HFC_MBTIME_PRLI].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[5]) { /* MB response timer (FC-GS) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_GCS_ID].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				pp->mb_timer[HFC_MBTIME_GID_PN].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				pp->mb_timer[HFC_MBTIME_GPN_ID].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				pp->mb_timer[HFC_MBTIME_GID_FT].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				pp->mb_timer[HFC_MBTIME_RFT_ID].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				pp->mb_timer[HFC_MBTIME_RFF_ID].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				pp->mb_timer[HFC_MBTIME_GPN_FT].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_GCS_ID].tout = HFC_FX_DF6_MB_TO;
				pp->mb_timer[HFC_MBTIME_GID_PN].tout = HFC_FX_DF6_MB_TO;
				pp->mb_timer[HFC_MBTIME_GPN_ID].tout = HFC_FX_DF6_MB_TO;
				pp->mb_timer[HFC_MBTIME_GID_FT].tout = HFC_FX_DF6_MB_TO;
				pp->mb_timer[HFC_MBTIME_RFT_ID].tout = HFC_FX_DF6_MB_TO;
				pp->mb_timer[HFC_MBTIME_RFF_ID].tout = HFC_FX_DF6_MB_TO;
				pp->mb_timer[HFC_MBTIME_GPN_FT].tout = HFC_FX_DF6_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[5] = pp->mb_timer[HFC_MBTIME_GCS_ID].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[5]) { /* MB response timer (FC-GS) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[5] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[5] & 0x80) {
					if((HFC_MBTIME_RETRY_MIN <= wk) && (wk <= HFC_MBTIME_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_GCS_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GID_PN].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GPN_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GID_FT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_RFT_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_RFF_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GPN_FT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_GCS_ID].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_GID_PN].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_GPN_ID].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_GID_FT].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_RFT_ID].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_RFF_ID].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_GPN_FT].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_GCS_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GID_PN].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GPN_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GID_FT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_RFT_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_RFF_ID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
						pp->mb_timer[HFC_MBTIME_GPN_FT].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[5];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_GCS_ID].retry = HFC_FX_DF6_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_GID_PN].retry = HFC_FX_DF6_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_GPN_ID].retry = HFC_FX_DF6_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_GID_FT].retry = HFC_FX_DF6_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_RFT_ID].retry = HFC_FX_DF6_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_RFF_ID].retry = HFC_FX_DF6_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_GPN_FT].retry = HFC_FX_DF6_MB_RETRY;
					}
				}
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_GCS_ID].retry = HFC_FX_DF6_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_GID_PN].retry = HFC_FX_DF6_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_GPN_ID].retry = HFC_FX_DF6_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_GID_FT].retry = HFC_FX_DF6_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_RFT_ID].retry = HFC_FX_DF6_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_RFF_ID].retry = HFC_FX_DF6_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_GPN_FT].retry = HFC_FX_DF6_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[5] = pp->mb_timer[HFC_MBTIME_GCS_ID].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[5]) { /* MB retry delay (FC-GS) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_GCS_ID].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				pp->mb_timer[HFC_MBTIME_GID_PN].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				pp->mb_timer[HFC_MBTIME_GPN_ID].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				pp->mb_timer[HFC_MBTIME_GID_FT].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				pp->mb_timer[HFC_MBTIME_RFT_ID].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				pp->mb_timer[HFC_MBTIME_RFF_ID].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				pp->mb_timer[HFC_MBTIME_GPN_FT].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[5];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_GCS_ID].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_GID_PN].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_GPN_ID].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_GID_FT].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_RFT_ID].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_RFF_ID].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_GPN_FT].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[5] = pp->mb_timer[HFC_MBTIME_GCS_ID].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_rsp_timer[6]) { /* MB response timer (add/del PORT/ID - MIHLOG - Shadow Up - Load CH TraceLog) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_ADD_PORTID].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[6];
				pp->mb_timer[HFC_MBTIME_DEL_PORTID].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[6];
				pp->mb_timer[HFC_MBTIME_MIHLOG].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[6];
				pp->mb_timer[HFC_MBTIME_LOADCHTRC].tout = diag->uni.changeparm_flash.val_mbgrp_rsp_timer[6];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_ADD_PORTID].tout = HFC_FX_DF7_MB_TO;
				pp->mb_timer[HFC_MBTIME_DEL_PORTID].tout = HFC_FX_DF7_MB_TO;
				pp->mb_timer[HFC_MBTIME_MIHLOG].tout = HFC_FX_DF7_MB_TO;
				pp->mb_timer[HFC_MBTIME_LOADCHTRC].tout = HFC_FX_DF7_MB_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_rsp_timer[6] = pp->mb_timer[HFC_MBTIME_LOADCHTRC].tout;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_func[6]) { /* MB response timer (add/del PORT/ID - MIHLOG - Shadow Up - Load CH TraceLog) */
			case HFC_CHGPRM_OPR_SET:
				wk = diag->uni.changeparm_flash.val_mbgrp_retry_func[6] & 0x7f;
				if (diag->uni.changeparm_flash.val_mbgrp_retry_func[6] & 0x80) {
					if((HFC_MBTIME_RETRY_MIN <= wk) && (wk <= HFC_MBTIME_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
						pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
						pp->mb_timer[HFC_MBTIME_MIHLOG].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
						pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_MIHLOG].retry = HFC_MBTIME_RETRY_DF | 0x80;
						pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry = HFC_MBTIME_RETRY_DF | 0x80;
					}
				}
				else {
					if((HFC_FX_MB_RETRY_MIN <= wk) && (wk <= HFC_FX_MB_RETRY_MAX))
					{
						pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
						pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
						pp->mb_timer[HFC_MBTIME_MIHLOG].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
						pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry = diag->uni.changeparm_flash.val_mbgrp_retry_func[6];
					}
					else
					{
						pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry = HFC_FX_DF7_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry = HFC_FX_DF7_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_MIHLOG].retry = HFC_FX_DF7_MB_RETRY;
						pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry = HFC_FX_DF7_MB_RETRY;
					}
				}
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry = HFC_FX_DF7_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry = HFC_FX_DF7_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_MIHLOG].retry = HFC_FX_DF7_MB_RETRY;
				pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry = HFC_FX_DF7_MB_RETRY;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_func[6] = pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mbgrp_retry_delay[6]) { /* MB retry delay (add/del PORT/ID - MIHLOG - Shadow Up - Load CH TraceLog) */
			case HFC_CHGPRM_OPR_SET:
				pp->mb_timer[HFC_MBTIME_ADD_PORTID].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[6];
				pp->mb_timer[HFC_MBTIME_DEL_PORTID].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[6];
				pp->mb_timer[HFC_MBTIME_MIHLOG].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[6];
				pp->mb_timer[HFC_MBTIME_LOADCHTRC].intvl = diag->uni.changeparm_flash.val_mbgrp_retry_delay[6];
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mb_timer[HFC_MBTIME_ADD_PORTID].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_DEL_PORTID].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_MIHLOG].intvl = HFC_FX_MB_INTVL_MIN;
				pp->mb_timer[HFC_MBTIME_LOADCHTRC].intvl = HFC_FX_MB_INTVL_MIN;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mbgrp_retry_delay[6] = pp->mb_timer[HFC_MBTIME_ADD_PORTID].intvl;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		for(i=0 ; i<25 ; i++){
			switch (diag->uni.changeparm_flash.opr_mailbox_delay[i]) { /* MB delay */
				case HFC_CHGPRM_OPR_SET:
					pp->mb_timer[i].delay = diag->uni.changeparm_flash.val_mailbox_delay[i];
					break;
				case HFC_CHGPRM_OPR_DEL:
					pp->mb_timer[i].delay = HFC_FX_MB_DELAY_MIN;
					break;
				case HFC_CHGPRM_OPR_READ:
					diag->uni.changeparm_flash.val_mailbox_delay[i] = pp->mb_timer[i].delay;
					break;
				case HFC_CHGPRM_OPR_NONE:
				default:
					break;
			}
		}
		
		switch (diag->uni.changeparm_flash.opr_wait_plogi_recv) { /* wait plogi recv tmo */
			case HFC_CHGPRM_OPR_SET:
				pp->wait_plogi_recv_tmo = diag->uni.changeparm_flash.val_wait_plogi_recv;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->wait_plogi_recv_tmo = HFC_FX_DF_PLOGI_RECV_TMO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_wait_plogi_recv = pp->wait_plogi_recv_tmo;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_mailbox_force_retry) { /* mailbox force retry */
			case HFC_CHGPRM_OPR_SET:
				pp->mailbox_force_retry = diag->uni.changeparm_flash.val_mailbox_force_retry;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->mailbox_force_retry = HFC_FX_MB_FORCE_RETRY_OFF;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_mailbox_force_retry = pp->mailbox_force_retry;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_login_filter) { /* login filter */
			case HFC_CHGPRM_OPR_SET:
				pp->filter_target = diag->uni.changeparm_flash.val_login_filter;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->filter_target = HFC_FX_MB_LOGIN_FILTER_ON;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_login_filter = pp->filter_target;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_vf_enable) { /* vf enable */
			case HFC_CHGPRM_OPR_SET:
				pp->vf_enable = diag->uni.changeparm_flash.val_vf_enable;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->vf_enable = HFC_FX_VF_DISABLE;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_vf_enable = pp->vf_enable;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_vf_mode) { /* vf mode */
			case HFC_CHGPRM_OPR_SET:
				pp->vf_mode_tagging = diag->uni.changeparm_flash.val_vf_mode;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->vf_mode_tagging = HFC_FX_VF_MODE_TAGGING_AUTO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_vf_mode = pp->vf_mode_tagging;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_security_enable) { /* security enable */
			case HFC_CHGPRM_OPR_SET:
				pp->security_enable = diag->uni.changeparm_flash.val_security_enable;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->security_enable = HFC_FX_SECURITY_DISABLE;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_security_enable = pp->security_enable;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash.opr_login_delay_time) { /* login delay time */
			case HFC_CHGPRM_OPR_SET:
				pp->login_wait = diag->uni.changeparm_flash.val_login_delay_time;
				break;
			case HFC_CHGPRM_OPR_DEL:
				pp->login_wait = HFC_FX_LOGIN_DELAY_TO;
				break;
			case HFC_CHGPRM_OPR_READ:
				diag->uni.changeparm_flash.val_login_delay_time = pp->login_wait;
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
	}
	
	HFC_ALLCOREUNLOCK(rp);
	
	for (i=1; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		rp = pp->region_arg[vpp->rid];
		if (rp == NULL)
			continue;
		
		HFC_ALLCORELOCK(rp);
		/* copy dynamic parameter from physical port */
		hfc_fx_param_copy(pp, vpp);
		HFC_ALLCOREUNLOCK(rp)
	}
	
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	
	diag->uni.changeparm_flash.errcode = HFC_CHGPRM_NORMAL; /* Normal End */

	return( 0 );
}
/* FCLNX-GPL-493 */
/*
 * Function:    change parameter 2
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
int fx_change_param_flash2(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct port_info	*vpp;
	struct region_info	*rp;
	int   		i;
	ulong		flags=0;	/* FCLNX-GPL-517 */
	
	rp = pp->region_arg[pp->rid];
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	HFC_ALLCORELOCK(rp);
	
	if (diag->uni.changeparm_flash2.version >= 1) {
		switch (diag->uni.changeparm_flash2.opr_peer_password) { /* peer password */
			case HFC_CHGPRM_OPR_SET:
				memcpy(pp->peer_password,diag->uni.changeparm_flash2.val_peer_password,40);
				break;
			case HFC_CHGPRM_OPR_DEL:
				memset(pp->peer_password,0,40);
				break;
			case HFC_CHGPRM_OPR_READ:
				memcpy(diag->uni.changeparm_flash2.val_peer_password,pp->peer_password,40);
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
		switch (diag->uni.changeparm_flash2.opr_local_password) { /* security enable */
			case HFC_CHGPRM_OPR_SET:
				memcpy(pp->local_password,diag->uni.changeparm_flash2.val_local_password,40);
				break;
			case HFC_CHGPRM_OPR_DEL:
				memset(pp->local_password,0,40);
				break;
			case HFC_CHGPRM_OPR_READ:
				memcpy(diag->uni.changeparm_flash2.val_local_password,pp->local_password,40);
				break;
			case HFC_CHGPRM_OPR_NONE:
			default:
				break;
		}
		
	}
	
	HFC_ALLCOREUNLOCK(rp);
	
	for (i=1; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		rp = pp->region_arg[vpp->rid];
		if (rp == NULL)
			continue;
		
		HFC_ALLCORELOCK(rp);
		/* copy dynamic parameter from physical port */
		hfc_fx_param_copy(pp, vpp);
		HFC_ALLCOREUNLOCK(rp);
	}
	
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	
	diag->uni.changeparm_flash2.errcode = HFC_CHGPRM_NORMAL; /* Normal End */

	return( 0 );
}


/*
 * Function:    read payload for frame send recieve
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (read payload) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int fx_read_payload(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
//	HFC_ENTRY("read_payload") ;
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PAYLOAD, 0x00 );
		return( EINVAL );
	}
	if( diag->length < (uint)HFC_PAGE_SIZE ) {	/* Is size short for payload ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PAYLOAD, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PAYLOAD, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PAYLOAD, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PAYLOAD, 0x04 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)core->payload, (char *)(ulong)diag->addr, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PAYLOAD, 0x02 );
		errlog_no = 0x30 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}
	
//	HFC_EXIT("read_payload") ;

	return( 0 );
}


/*
 * Function:    read payload for recieve frame
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (read recieve payload) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int fx_read_rcv_payload(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;
	struct port_info	*wkpp;
	uchar				wkrid=0;	/* FCLNX-GPL-FX-403 */
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) || !HFC_FX_MMODE_CHECK_SHADOW (pp) ){ /* FCLNX-GPL-FX-403 Start */
		wkrid = pp->rid;
	}else{
		wkrid = diag->vport_no;
	} /* FCLNX-GPL-FX-403 End */
	
//	HFC_ENTRY("read_rcv_payload") ;
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RCV_PAYLOAD, 0x00 );
		return( EINVAL );
	}
	if( diag->length < (uint)(HFC_PAGE_SIZE/2) ) {	/* Is size short for payload ? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RCV_PAYLOAD, 0x01 );
		return( EINVAL );
	}
	
	rp = wkpp->region_arg[wkrid];	/* FCLNX-GPL-FX-403 */
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RCV_PAYLOAD, 0x02 );
		return( EINVAL );
	}
	
	if( diag->core_no > 4 ) {	/* Is core# invalid ?*/
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RCV_PAYLOAD, 0x03 );
		return( EINVAL );
	}
	
	core = rp->core_arg[diag->core_no];
	if( !core ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RCV_PAYLOAD, 0x04 );
		return( EINVAL );
	}
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	if ( COPYOUT( (char *)core->rcvfrm_payload, (char *)(ulong)diag->addr, (uint)(HFC_PAGE_SIZE/2) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RCV_PAYLOAD, 0x02 );
		errlog_no = 0x31 ;
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}
	
//	HFC_EXIT("read_payload") ;

	return( 0 );
}


/*
 * Function:    Performance Monitor
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (performance monitor) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int fx_performance(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	region_info			*rp=NULL;
	struct	hfc_pm_pkt_fx		*hfcp=NULL;
	struct	port_info			*wkpp;
	struct	core_info			*core;
	struct	performance_info	*pm, *pmc;
	
	uchar	errlog_no ;
	ulong	flags=0;
	int		i, j;
	int		rtn = 0;
	uint	port_exec = 0;
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	HFC_ENTRY("performance monitor") ;
	
	rp = wkpp->region_arg[wkpp->rid];
	if( !rp ){
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PERFORMANCE, 0x01 );
		return( EINVAL );
	}
	
	switch( diag->uni.performance.opcode )
	{
		case HFC_PFM_OPR_GET:
			if( !diag->addr ) {	/* Is virtual address unassigned? */
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x02 );
				rtn = EINVAL;
				break;
			}
			if( diag->length < sizeof(struct hfc_pm_pkt_fx)*wkpp->pm_pkt_num ) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x03 );
				rtn = EINVAL;
				break;
			}
			
			diag->uni.performance.pm_control = wkpp->pm_control;
			
			for (i=0;i<wkpp->pm_pkt_num;i++)
			{
				hfcp = &wkpp->pm_pkt_pool[i];
				if (hfcp == NULL)
					break;
				if ( COPYOUT( (char *)hfcp, (char *)(ulong)(diag->addr+sizeof(struct hfc_pm_pkt_fx)*i), (uint)(sizeof(struct hfc_pm_pkt_fx)) ) != 0 ) {
					HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x04 );
					errlog_no = 0x32 ;
					hfc_fx_errlog(wkpp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
					
					rtn = EINVAL;
					break;
				}
			}
			break;
			
		case HFC_PFM_OPR_START:
		case HFC_PFM_OPR_IOCNT:
			if( !diag->addr ) {	/* Is virtual address unassigned? */
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x05 );
				rtn = EINVAL;
				break;
			}
			
			if( diag->length < sizeof(struct performance_info)*MAX_CORE_PROBE_FX ) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x06 );
				rtn = EINVAL;
				break;
			}
			
			pm = (struct performance_info *)hfc_fx_kmalloc(pp, sizeof(struct performance_info)*MAX_CORE_PROBE_FX, GFP_ATOMIC);
			
			if (pm == NULL) {
				HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x09 );
				rtn = ENOMEM;
				break;
			}
			
			memset(pm, 0, sizeof(struct performance_info)*MAX_CORE_PROBE_FX);
			diag->uni.performance.pm_control = wkpp->pm_control;
			
			port_exec |= (uint)(0x00001000) ;
			port_exec |= (uint)( (wkpp->rid << 16) & 0x00ff0000) ;
			port_exec |= HFC_FRAMEA_PORTSTATISTICS ;
			
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/wkpp->core_num){
				core = rp->core_arg[i];
				pmc = &pm[i];
				
				if(!hfc_fx_check_cs_disable(pp, core)){	/* FCLNX-GPL-FX-438 */
					pmc->fw_store_count = core->fw_init_p->portstatistics.fw_store_count;
					
					hfc_fx_write_reg_core(wkpp, i, (uint)HFC_IOSPACE_FRAMEA,
						(char)0x4, (int)port_exec, HFC_FX_CORE_OFFSET40);
				}
			}
			
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			
			if (diag->uni.performance.opcode == HFC_PFM_OPR_START) {
				if ( COPYOUT( (char *)pm, (char *)(ulong)diag->addr, sizeof(struct performance_info)*MAX_CORE_PROBE_FX ) != 0 ) {
					HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
							HFC_TRC_IOCTL_PERFORMANCE, 0x07 );
					errlog_no = 0x34 ;
					hfc_fx_errlog(wkpp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
					
					rtn = EINVAL;
				}
				hfc_fx_kfree(pp, pm);
				break;
			}
			
			for(i=0;i<MAX_CORE_PROBE_FX;i++){
				core = rp->core_arg[i];
				if (core == NULL)
					continue;
				
				pmc = &pm[i];
				
				/* io counter */
				pmc->wr_exec_cnt			= core->wr_exec_cnt;
				pmc->rd_exec_cnt			= core->rd_exec_cnt;
				pmc->wr_cnt					= core->wr_cnt;
				pmc->rd_cnt					= core->rd_cnt;
				pmc->scsi_err_cnt			= core->scsi_err_cnt;
				pmc->xrb_resp_cnt			= core->xrb_resp_cnt;
				pmc->wr_data_size			= core->wr_data_size;
				pmc->rd_data_size			= core->rd_data_size;
				for(j=0; j<6; j++){
					pmc->wr_end_cnt[j]		= core->wr_end_cnt[j];
					pmc->rd_end_cnt[j]		= core->rd_end_cnt[j];
				}
				
				/* portstatistics data */
				if (wkpp->master_core_no == core->core_no) {
					pmc->tx_frames			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.tx_frames);
					pmc->tx_words			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.tx_words);
					pmc->rx_frames			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.rx_frames);
					pmc->rx_words			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.rx_words);
					pmc->lip_count			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.lip_count);
					pmc->nos_count			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.nos_count);
					pmc->link_failure_count	= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.link_failure_count);
					pmc->loss_of_sync_count	= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.loss_of_sync_count);
					pmc->loss_of_signal_count	= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.loss_of_signal_count);
				}
				pmc->error_frames			= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.error_frames);
				pmc->invalid_crc_count		= (uint64_t)hfc_fx_read_val(core->fw_init_p->portstatistics.invalid_crc_count);
				
				/* resource busy counter */
				pmc->xob_full_cnt			= core->xob_full_cnt;
				pmc->iovmap_full_cnt		= core->iovmap_full_cnt;
				pmc->frame_full_cnt			= core->frame_full_cnt;
				pmc->dma_max_over_cnt		= core->dma_max_over_cnt;
				if (wkpp->master_core_no == core->core_no) {
					pmc->hfcpkt_full_cnt	= wkpp->hfcpkt_full_cnt;
				}
				
				/* Max number of cmds per int */
				pmc->max_cmd_num_int		= core->max_cmd_num_int;
			}
			
			if ( COPYOUT( (char *)pm, (char *)(ulong)diag->addr, sizeof(struct performance_info)*MAX_CORE_PROBE_FX ) != 0 ) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_PERFORMANCE, 0x08 );
				errlog_no = 0x35 ;
				hfc_fx_errlog(wkpp, core, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
				
				rtn = EINVAL;
			}
			hfc_fx_kfree(pp, pm);
			break;
			
		case HFC_PFM_OPR_CLR:
		case HFC_PFM_OPR_CNT_CLR:
			port_exec |= (uint)(0x00008000) ;
			port_exec |= (uint)( (wkpp->rid << 16) & 0x00ff0000) ;
			port_exec |= HFC_FRAMEA_PORTSTATISTICS ;
			
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			
			for(i=0;i<MAX_CORE_PROBE_FX;i++){
				core = rp->core_arg[i];
				if (core == NULL)
					continue;
				
				if(!hfc_fx_check_cs_disable(pp, core)){	/* FCLNX-GPL-FX-438 */
					hfc_fx_write_reg_core(wkpp, i, (uint)HFC_IOSPACE_FRAMEA,
							(char)0x4, (int)port_exec, HFC_FX_CORE_OFFSET40);
				}
				
				/* io counter */
				core->scsi_exec_cnt		= 0;
				core->wr_exec_cnt		= 0;
				core->rd_exec_cnt		= 0;
				core->scsi_end_cnt		= 0;
				core->wr_cnt			= 0;
				core->rd_cnt			= 0;
				core->scsi_err_cnt		= 0;
				core->xrb_resp_cnt		= 0;
				core->wr_data_size		= 0;
				core->rd_data_size		= 0;
				
				/* resource busy counter */
				core->xob_full_cnt		= 0;
				core->iovmap_full_cnt	= 0;
				core->frame_full_cnt	= 0;
				core->dma_max_over_cnt	= 0;
				wkpp->hfcpkt_full_cnt	= 0;
				wkpp->dummy_int_rst_cnt	= 0;
				
				/* Max number of cmds per int */
				core->max_cmd_num_int	= 0;
				
				if (diag->uni.performance.opcode == HFC_PFM_OPR_CLR) {
					for(j=0; j<6; j++){
						core->wr_end_cnt[j]	= 0;
						core->rd_end_cnt[j]	= 0;
					}
				}
			}
			
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			break;
			
		case HFC_PFM_OPR_IO_CLR:
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			
			for(i=0;i<MAX_CORE_PROBE_FX;i++){
				core = rp->core_arg[i];
				if (core == NULL)
					continue;
				
				for(j=0; j<6; j++){
					core->wr_end_cnt[j]	= 0;
					core->rd_end_cnt[j]	= 0;
				}
			}
			
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			break;
		
		default:
			diag->uni.performance.errcode = HFC_PFM_ERR_PARAM;
	}
	HFC_EXIT("perormance monitor") ;
	
	return (rtn);
}


/* FCLNX-GPL-FX-137 Start */
/*
 * Function:    fx_link_reset
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (link reset) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int fx_link_reset(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	struct	region_info		*rp;
	ulong	flags=0;
	
	rp = pp->region_arg[pp->rid];
	if( !rp ){
		return( EINVAL );
	}
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	if ((test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status))
	|| (test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status))
	|| (test_bit( HFC_PS_ISOL, (ulong *)&pp->status))
	|| (test_bit(HFC_MAILBOX_BUSY, (ulong *)&pp->region_arg[pp->rid]->mb_lock )))
	{
		diag->uni.link_reset.errcode = HFC_LINKRESET_ADAP_STATUS;
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return(0);
	} 
	
	if (HFC_FX_VPORT_EXIST(pp)) {
		/* vport exists in physical port */
		diag->uni.link_reset.errcode = HFC_LINKRESET_NOT_ACCEPTABLE;
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return(0);
	}
	
	// Dynamic Reflection Parameter for Link Negotiation
	if ((diag->uni.link_reset.npiv_config == 0) ||
		(diag->uni.link_reset.npiv_config == 0x80)) {
		pp->npiv_mode	= diag->uni.link_reset.npiv_config;
	}
	else {
		pp->npiv_mode	= 0;
	}
	pp->topology		= diag->uni.link_reset.connect_type;
	pp->linkspeed		= diag->uni.link_reset.link_speed;
	pp->multiple_portid	= diag->uni.link_reset.multiple_portid;

	/* execute Link Reset */
	set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
	hfc_fx_abend(pp, rp->core_arg[pp->master_core_no], HFC_ABEND_LINK_RESET);
	
	diag->uni.link_reset.errcode = HFC_LINKRESET_START; /* Normal */

	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	return(0);

}
/* FCLNX-GPL-FX-137 End */


/*
 * Function:    fx_hfcldd_conf_store
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (performance monitor) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int fx_hfcldd_conf_store(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	int		rtn = 0;
	
	HFC_ENTRY("fx_hfcldd_conf_store") ;
	
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
	
	if( pp->hfclddconf == NULL) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCLDD_CONF_STORE, 0x02 );
		return EIO;
	}
	
	if ( COPYIN( (char *)(ulong)diag->addr, (char *)pp->hfclddconf, diag->length ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_HFCLDD_CONF_STORE, 0x04 );
		errlog_no = 0x33 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		
		rtn = EFAULT;
	}
	
	HFC_EXIT("fx_hfcldd_conf_store") ;
	
	return (rtn);
}

/* FCLNX-GPL-FX-146 */
/*
 * Function:    fx_flash_update
 *
 * Purpose:     Mark/unmark flash update status
 *
 * Arguments:   
 *  pp          - struct port_info
 *  diag        - struct diag_ioctl
 *
 * Returns:     0		Normal end
 */
int fx_flash_update(
	struct	port_info	*pp,
	struct	diag_ioctl	*diag)
{
	struct region_info *rp = NULL;
	ulong		flags=0;

	HFC_DBGPRT("fx_flash_update\n");
	
	if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){	/* FCLNX-GPL-FX-385 */
		rp = pp->region_arg[0];
	}else{
		rp = pp->region_arg[pp->rid];
	}	/* FCLNX-GPL-FX-385 */
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

	if (diag->uni.flash_update.opcode == HFC_FLASHUP_START) {
		if( test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) || test_bit(HFC_PS_WAIT_MCKINT,	(ulong *)&pp->status) ){
			diag->uni.flash_update.errcode = HFC_FLASHUP_ADAP_STATUS; 	/* Status Error */
		} else {
			set_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);
			diag->uni.flash_update.errcode = HFC_FLASHUP_SUCCESS; 		/* Normal */
		}
	}
	else { 	/* HFC_FLASHUP_FINISH */
		if( test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) ){
			clear_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);

			if (hfc_fx_pcibus_chk(pp) != 0)	{
				/* "PCI BUS ERR" has hpppen. */
				HFC_FX_ISSUE_CSTP_PCIERR(pp);
			}
			else if ( test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) ) {
				hfc_fx_mck_recovery_five_fx(pp, HFC_ABEND_MCK_RESUME); /* resume MCK recovery */
			}
		}
		diag->uni.flash_update.errcode = HFC_FLASHUP_SUCCESS; /* Normal */
	}

	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	return 0;

} /* end of fx_flash_update */


/*
 * Function:    fx_vport_list
 *
 * Purpose:     Process for ioctl(HFCDIAG0) subcommand (vport_list) 
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:    0		Normal end
 *             EINVAL	Specification error
 *             EFAULT	Copy process fail
 */
int fx_vport_list(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no;
	int		rtn = 0;
	int		i;
	ulong	flags = 0;
	struct	port_info		*wkpp;
	struct	region_info		*rp;
	struct	vport_list		vlist;
	
	HFC_ENTRY("fx_vport_list") ;
	
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VPORT_LIST, 0x00 );
		return EINVAL;
	}
	
	if( diag->length < sizeof(struct vport_list) ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VPORT_LIST, 0x01 );
		return EINVAL;
	}
	
	memset(&vlist, 0, sizeof(struct vport_list));
	
	if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){	/* FCLNX-GPL-FX-385 */
		rp = pp->region_arg[0];
	}else{
		rp = pp->region_arg[pp->rid];
	}	/* FCLNX-GPL-FX-385 */
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	for (i=0; i<=pp->max_vport_count; i++) {
		wkpp = pp->vport_ptr[i].vport_arg;
		if (wkpp == NULL)
			continue;
		
		vlist.vport_info[i] = 1;
		vlist.rid[i] = wkpp->rid;
		vlist.sub_rid[i] = wkpp->sub_rid;
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	if ( COPYOUT( (char *)&vlist, (char *)(ulong)diag->addr, diag->length ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_VPORT_LIST, 0x02 );
		errlog_no = 0x40 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		
		rtn = EFAULT;
	}
	
	HFC_EXIT("fx_vport_list") ;
	
	return (rtn);
}


/*
 * Function:    read_rsv_pkt
 *
 * Purpose:     Process for ioctl(HFCDIAG0)subcommand (Read reserve hfc_pkt)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0
 *
 */
int fx_read_rsv_pkt(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	char *ptr;
	struct port_info	*wkpp;
	
//	HFC_ENTRY("fx_read_rsv_pkt");
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RSV_PKT, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfc_pkt_fx) ) {	/* Is size short for reserve hfc_pkt_fx? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RSV_PKT, 0x01 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.no >= wkpp->rsv_pkt_num ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RSV_PKT, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RSV_PKT, 0x03 );
		return( EINVAL );
	}

	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	ptr = (char *)&wkpp->rsv_pkt_pool[diag->uni.xob_xrb_scmd.no];
	if ( COPYOUT( ptr, (char *)(ulong)diag->addr, sizeof(struct hfc_pkt_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_RSV_PKT, 0x04 );
		errlog_no = 0x50 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("fx_read_rsv_pkt");

	return( 0 );
}


/*
 * Function:    read_pm_pkt
 *
 * Purpose:     Process for ioctl(HFCDIAG0)subcommand (Read hfc_pm_pkt)
 *
 * Arguments:   
 *  pp         - struct port_info
 *  diag       - struct diag_ioctl
 *
 * Returns:     0
 *
 */
int fx_read_pm_pkt(
	struct	port_info	*pp,	/* struct port_info */
	struct	diag_ioctl	*diag)	/* struct diag_ioctl */
{
	uchar	errlog_no ;
	char *ptr;
	struct port_info	*wkpp;
	
//	HFC_ENTRY("fx_read_pm_pkt");
	
	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag->vport_no != 0)) {
		wkpp = pp->vport_ptr[diag->vport_no].vport_arg;
	}
	else {
		wkpp = pp;
	}
	
	/*--------------------------------------------------------------*
	 * Specification check
	 *--------------------------------------------------------------*/
	if( !diag->addr ) {	/* Is virtual address unassigned? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PM_PKT, 0x00 );
		return( EINVAL );
	}
	if( diag->length < sizeof(struct hfc_pm_pkt_fx) ) {	/* Is size short for reserve hfc_pm_pkt_fx? */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PM_PKT, 0x01 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.no >= wkpp->pm_pkt_num ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PM_PKT, 0x02 );
		return( EINVAL );
	}
	if( diag->uni.xob_xrb_scmd.cnt != MAX_READ_CNT ) {	/* Read number */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PM_PKT, 0x03 );
		return( EINVAL );
	}
	if (wkpp->pm_pkt_pool == NULL) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PM_PKT, 0x05 );
		return( EINVAL );
	}
	
	/*--------------------------------------------------------------*
	 * Data copy
	 *--------------------------------------------------------------*/
	ptr = (char *)&wkpp->pm_pkt_pool[diag->uni.xob_xrb_scmd.no];
	if ( COPYOUT( ptr, (char *)(ulong)diag->addr, sizeof(struct hfc_pm_pkt_fx) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_PM_PKT, 0x04 );
		errlog_no = 0x51 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
		return ( EFAULT );
	}

//	HFC_EXIT("fx_read_rsv_pkt");

	return( 0 );
}


/*
 * Function:    hfc_fx_diag
 *
 * Purpose:     Process each ioctl(HFCDIAG0) subcommand
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
int hfc_fx_diag(
	void	*arg,				/* struct diag_ioctl */
	struct	port_info	*pp)	/* struct port_info */
{

	struct	diag_ioctl	diag;	/* DIAD internal area */
	int		rtn ;
	uint	i = 0 ;
	uchar	errlog_no ;

//	HFC_ENTRY("hfc_fx_diag") ;

	/*--------------------------------------------------------------*
	* Copy internal DIAG arguments from caller
	*--------------------------------------------------------------*/
	if ( COPYIN( (char *)arg, (char *)&diag, sizeof(struct diag_ioctl) ) != 0 ) {
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x00 );
		errlog_no = 0x03 ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5E, &errlog_no, 1) ;
		return ( EFAULT );
	}

#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(pp->ioctl32) {
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

	if(!(test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) ) ) {				//FCLNX-0294
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

//#ifdef _HFC_FX_PROTO
//	pp->fcp_mode = hfc_fx_read_reg(pp, HFC_IOSPACE_CA_FLAG, PCI_LENGTH_01);
//#endif
	
//	HFC_ERRPRT("hfcldd%d hfc_fx_diag fcp_mode = %d, pp->pkg.type = %d sub_cmd = %02x\n",
//		pp->dev_minor, pp->fcp_mode, pp->pkg.type, diag.sub_cmd);

	i = 0 ;
	while( diag_fx_check[i].sub_command != 0 )
	{
		if( diag_fx_check[i].sub_command == diag.sub_cmd )
			break ;
		i++ ;
	}
	
//	HFC_ERRPRT("hfcldd%d hfc_fx_diag fcp_mode = %d, pp->pkg.type = %d sub_cmd = %02x i = %d\n",
//		pp->dev_minor, pp->fcp_mode, pp->pkg.type, diag.sub_cmd, i);
	
	if( pp->pkg.type == HFC_PKTYPE_FIVE_FX )
	{
		if( !(diag_fx_check[i].support_pk & HFC_DIAG_FIVE_FX ) )
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
	
	if( pp->fcp_mode == FW_MODE_NORMAL )
	{
		if ( pp->pkg.type == HFC_PKTYPE_FPP ) {
			if( !(diag_fx_check[i].flags_fpp & HFC_DIAG_NORMAL_MODE) )
			{
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_DIAG, 0x06 ); 
				return( EINVAL );
			}
		} 
	}
	if( pp->fcp_mode == FW_MODE_DIAG )
	{
		if( !(diag_fx_check[i].flags_fpp & HFC_DIAG_DIAG_MODE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x07 ); 
			return( EINVAL );
		}
	}
	if( (pp->fcp_mode == FW_MODE_NAI_LOOP) || (pp->fcp_mode == FW_MODE_GAI_LOOP) )
	{
		if( !(diag_fx_check[i].flags_fpp & HFC_DIAG_LOOP_MODE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x08 );
			return( EINVAL );
		}
	}
	if( pp->fcp_mode == FW_MODE_IOS )
	{
		if( !(diag_fx_check[i].flags_fpp & HFC_DIAG_IOS_MODE) )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x09 ); 
			return( EINVAL );
		}
	}
	
//	if ((HFC_FX_NPIV_ENABLE(pp) | HFC_FX_MQ_ENABLE(pp)) && (diag.rid != 0)) {	/* FCLNX-GPL-FX-196 */
	if (diag.vport_no != 0) {
		if  ((  diag.sub_cmd == HFC_DGRD_INITTBL)
			|| (diag.sub_cmd == HFC_DGRD_XOB)
			|| (diag.sub_cmd == HFC_DGRD_XRB)
			|| (diag.sub_cmd == HFC_DGRD_SEGINFO)
			|| (diag.sub_cmd == HFC_DGRD_ADAP)
			|| (diag.sub_cmd == HFC_DGRD_TARGET)
			|| (diag.sub_cmd == HFC_DGRD_DDTRC)
			|| (diag.sub_cmd == HFC_DGRD_MB)
			|| (diag.sub_cmd == HFC_DGRD_SCMD)
			|| (diag.sub_cmd == HFC_DGRD_CORE)
			|| (diag.sub_cmd == HFC_DGRD_CORETRC)
			|| (diag.sub_cmd == HFC_DGRD_PAYLOAD)
			|| (diag.sub_cmd == HFC_DGRD_MBTRC)		/* FCLNX-GPL-FX-139 */
			|| (diag.sub_cmd == HFC_DGRD_DEV)		/* FCLNX-GPL-FX-196 */
			|| (diag.sub_cmd == HFC_DG_PERFORMANCE)	/* FCLNX-GPL-FX-196 */
			|| (diag.sub_cmd == HFC_DGRD_RCV_PAYLOAD)) {
			/* rid valid ? */
			if ((diag.vport_no > pp->max_vport_count) || (pp->vport_ptr[diag.vport_no].vport_arg == NULL)) {
				HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x0c );
				return( EINVAL );
			}
		}
	}
	
	rtn = diag_fx_check[i].cmd_func( pp , &diag) ;

	
	/* FCLNX-GPL-223 start */
	if( (diag.sub_cmd == HFC_DG_RDHG) || (diag.sub_cmd == HFC_DG_WRHG) )
	{
		if ( COPYOUT( (char *)&diag, (char *)arg, sizeof(struct diag_ioctl) ) != 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x0c ); 
			errlog_no = 0x0 ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
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
	if( (diag_fx_check[i].flags_fpp & HFC_DIAG_ARG_RESP) ||
		(diag_fx_check[i].flags_five & HFC_DIAG_ARG_RESP) )
	{
		if ( COPYOUT( (char *)&diag, (char *)arg, sizeof(struct diag_ioctl) ) != 0 )
		{
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_DIAG, 0x0b ); 
			errlog_no = 0x0 ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRC, 0x5F, &errlog_no, 1) ;
			return ( EFAULT );
		}
	}

//	HFC_EXIT("hfc_fx_diag") ;

	return( 0 );	/* Normal end */
}
