/*
 * hfclddioc.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfclddioc.h,v 1.6.2.15.2.6.2.1.2.1.6.9.6.4.2.9.4.21.2.2.2.3.2.1 2015/03/05 02:19:43 toyo Exp $
 */


#ifndef _H_HFCDDIOC             /* Double definition prevention */
#define _H_HFCDDIOC

/**************************************************************************************************/
/* data structure                                                                                 */
/**************************************************************************************************/
typedef unsigned char       uchar;
#ifdef __WIN
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned __int64    uint64_t ;  /* @0007 */
typedef __int64             int64_t ;   /* @0007 */
#else
#ifndef __KERNEL__
#ifdef __LINUX
#include <stdint.h>
#else  /*__LINUX*/
typedef unsigned long long    uint64_t ;
#endif /*__LINUX*/
#endif
/*typedef long long         int64_t ;     */
#endif

/******************************************************/
/* Linux x86-64 <-> Linux32                2005.02.23 */
/******************************************************/
#if defined(__LINUX) && defined(__GNUC__) && defined(__x86_64)
#define HFC_VAL_ATTR    __attribute__((packed))
#define HFC_VAL_ATTR_C  __attribute__((aligned(1)))
#else   /*__LINUX && __GNUC__ && __x86_64*/
#define HFC_VAL_ATTR_C
#define HFC_VAL_ATTR
#endif  /*__LINUX && __GNUC__ && __x86_64*/

/************************************************************************/
/*   ioctl                                                              */
/************************************************************************/
/* SIGNATURE */
#define IOCTL_SIGNATURE     "HFCDRV"

/************************************************************/
/*   command code (for drivers)                            */
/************************************************************/
#ifdef __WIN
#define HFC_FNC_SC_INQU                     0x0900
#define HFC_FNC_SC_CMD                      0x0904
#define HFC_FNC_SC_NMSRV                    0x0905
#define HFC_FNC_SC_GWWN                     0x0906
#define HFC_FNC_SC_PAYLD                    0x0907
#define HFC_FNC_SC_GRNID                    0x0908
#define HFC_FNC_SC_SRNID                    0x0909
#define HFC_FNC_SC_APSTAT                   0x090A
#define HFC_FNC_SC_API                      0x0940
#define HFC_FNC_SC_SPT                      0x0941
#define HFC_FNC_DIAG0                       0x09FF
#endif
#ifdef __LINUX
#define HFC_IOC_MAGIC   'F'

#define HFC_FNC_SC_INQU     _IOWR(HFC_IOC_MAGIC,0,struct hfc_ioctl_inquiry)
#define HFC_FNC_SC_CMD      _IOWR(HFC_IOC_MAGIC,1,struct hfc_ioctl_cdb)
#define HFC_FNC_SC_NMSRV    _IOWR(HFC_IOC_MAGIC,2,struct hfc_ioctl_gidft)
#define HFC_FNC_SC_GWWN     _IOWR(HFC_IOC_MAGIC,3,struct hfc_ioctl_gidpn)
#define HFC_FNC_SC_PAYLD    _IOWR(HFC_IOC_MAGIC,4,struct hfc_ioctl_payload)
#define HFC_FNC_SC_GRNID    _IOWR(HFC_IOC_MAGIC,5,struct hfc_ioctl_rnid)
#define HFC_FNC_SC_SRNID    _IOWR(HFC_IOC_MAGIC,6,struct hfc_ioctl_rnid)
#define HFC_FNC_SC_APSTAT   _IOWR(HFC_IOC_MAGIC,7,struct hfc_ioctl_adap_stat)
#define HFC_FNC_SC_API      _IOWR(HFC_IOC_MAGIC,8,struct hfc_ioctl_api)
#define HFC_FNC_DIAG0       _IOWR(HFC_IOC_MAGIC,9,struct diag_ioctl)
#define HFC_FNC_SC_SPT      _IOWR(HFC_IOC_MAGIC,10,struct hfc_ioctl_spt)
#define HFC_FNC_SC_TGTMAP   _IOWR(HFC_IOC_MAGIC,11,struct hfc_ioctl_tgt_map)        // 2005.01.19
#define HFC_FNC_SC_PBIND    _IOWR(HFC_IOC_MAGIC,12,struct hfc_ioctl_fcp_binding)    // 2005.01.19
#define HFC_FNC_SC_PRINFO   _IOWR(HFC_IOC_MAGIC,13,struct hfc_ioctl_proc_info)      // 2005.01.28
#define HFC_FNC_SC_GSID     _IOWR(HFC_IOC_MAGIC,14,struct hfc_ioctl_gpnid)
#define HFC_FNC_SC_FC4STAT  _IOWR(HFC_IOC_MAGIC,15,struct hfc_ioctl_fc4stat)        // FCLNX-0404
//#define HFC_FNC_MP_RDPARM	_IOWR(HFC_IOC_MAGIC,16,struct hfc_mp_parameters)		*** Attention!! : defined hfcddioc2.h ***
//#define HFC_FNC_MP_WRPARM	_IOWR(HFC_IOC_MAGIC,17,struct hfc_mp_parameters)		*** Attention!! : defined hfcddioc2.h ***
#define HFC_FNC_MP_TGTMAP	_IOWR(HFC_IOC_MAGIC,18,struct hfc_mp_target)
//#define HFC_FNC_MP_LUMAP	_IOWR(HFC_IOC_MAGIC,19,struct hfc_mp_lu)				*** Attention!! : defined hfcddioc2.h ***
//#define HFC_FNC_WWN_INFO	_IOWR(HFC_IOC_MAGIC,20,struct hfc_ioctl_wwn)			*** Attention!! : defined hfcddwwn.h ***
//#define HFC_FNC_MP_PTHEALTH	_IOWR(HFC_IOC_MAGIC,21,struct hfc_mp_path_health)	*** Attention!! : defined hfcddioc2.h ***
//#define HFC_FNC_MP_LGTGTMAP	_IOWR(HFC_IOC_MAGIC,22,struct hfc_mp_lgtarget)		*** Attention!! : defined hfcddioc2.h ***
//#define HFC_FNC_MP_LGPATH1	_IOWR(HFC_IOC_MAGIC,23,struct hfc_mp_lgpath_info1)	*** Attention!! : defined hfcddioc2.h ***
//#define HFC_FNC_MP_SETPATH	_IOWR(HFC_IOC_MAGIC,24,struct hfc_mp_set_path)		*** Attention!! : defined hfcddioc2.h ***
#define HFC_FNC_ADP_ENABLE  _IOWR(HFC_IOC_MAGIC,28,struct hfc_ioctl_adp_enable)		// hotplug
#define HFC_FNC_ADP_DISABLE _IOWR(HFC_IOC_MAGIC,29,struct hfc_ioctl_adp_enable)		// hotplug
#define HFC_FNC_SCSI_SCAN   _IOWR(HFC_IOC_MAGIC,30,struct hfc_ioctl_adp_enable)		// FCLNX-0303
//#define HFC_FNC_ADP_PATHCHG _IOWR(HFC_IOC_MAGIC,31,struct hfc_ioctl_adp_enable)	*** Attention!! : defined hfcddioc2.h ***
#define HFC_FNC_HBA_ISOLATION  _IOWR(HFC_IOC_MAGIC,32,struct hfc_isol_info)			/* FCLNX-0454 */	/* FCLNX-GPL-147 */
#define HFC_FNC_READ_APPARAM   _IOWR(HFC_IOC_MAGIC,33,struct hfc_ioctl_read_apparam)	/* FCLNX-0454 */
//#define HFC_FNC_MP_LGDEV_PARM _IOWR(HFC_IOC_MAGIC,34,struct hfc_mp_lg_dev_parameters)			/* FCLNX-0677 */	/* FCLNX-GPL-147 */
#endif

/************************************************************/
/*   command code (for tools)                               */
/************************************************************/
#ifdef __WIN
#define IOCTL_HFC_SC_INQU       CTL_CODE( 0x9090, HFC_FNC_SC_INQU,       METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_CMD        CTL_CODE( 0x9090, HFC_FNC_SC_CMD,        METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_NMSRV      CTL_CODE( 0x9090, HFC_FNC_SC_NMSRV,      METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_GWWN       CTL_CODE( 0x9090, HFC_FNC_SC_GWWN,       METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_PAYLD      CTL_CODE( 0x9090, HFC_FNC_SC_PAYLD,      METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_GRNID      CTL_CODE( 0x9090, HFC_FNC_SC_GRNID,      METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_SRNID      CTL_CODE( 0x9090, HFC_FNC_SC_SRNID,      METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_APSTAT     CTL_CODE( 0x9090, HFC_FNC_SC_APSTAT,     METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_API        CTL_CODE( 0x9090, HFC_FNC_SC_API,        METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_SC_SPT        CTL_CODE( 0x9090, HFC_FNC_SC_SPT,        METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#define IOCTL_HFC_DIAG0         CTL_CODE( 0x9090, HFC_FNC_DIAG0,         METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )
#else
#define IOCTL_HFC_SC_INQU       HFC_FNC_SC_INQU
#define IOCTL_HFC_SC_CMD        HFC_FNC_SC_CMD
#define IOCTL_HFC_SC_NMSRV      HFC_FNC_SC_NMSRV
#define IOCTL_HFC_SC_GWWN       HFC_FNC_SC_GWWN
#define IOCTL_HFC_SC_GSID       HFC_FNC_SC_GSID
#define IOCTL_HFC_SC_PAYLD      HFC_FNC_SC_PAYLD
#define IOCTL_HFC_SC_GRNID      HFC_FNC_SC_GRNID
#define IOCTL_HFC_SC_SRNID      HFC_FNC_SC_SRNID
#define IOCTL_HFC_SC_APSTAT     HFC_FNC_SC_APSTAT
#define IOCTL_HFC_SC_API        HFC_FNC_SC_API
#define IOCTL_HFC_SC_SPT        HFC_FNC_SC_SPT
#define IOCTL_HFC_SC_TGTMAP     HFC_FNC_SC_TGTMAP       // 2005.01.19
#define IOCTL_HFC_SC_PBIND      HFC_FNC_SC_PBIND        // 2005.01.19
#define IOCTL_HFC_SC_PRINFO     HFC_FNC_SC_PRINFO       // 2005.01.28
#define IOCTL_HFC_SC_FC4STAT    HFC_FNC_SC_FC4STAT      // FCLNX-0404
#define IOCTL_HFC_MP_RDPARM		HFC_FNC_MP_RDPARM
#define IOCTL_HFC_MP_WRPARM		HFC_FNC_MP_WRPARM
#define IOCTL_HFC_MP_TGTMAP		HFC_FNC_MP_TGTMAP
#define IOCTL_HFC_MP_LUMAP		HFC_FNC_MP_LUMAP
#define IOCTL_HFC_MP_SETPATH	HFC_FNC_MP_SETPATH
#define IOCTL_HFC_MP_PTHEALTH	HFC_FNC_MP_PTHEALTH
#define IOCTL_HFC_MP_LGTGTMAP	HFC_FNC_MP_LGTGTMAP
#define IOCTL_HFC_MP_LGPATH1	HFC_FNC_MP_LGPATH1
#define IOCTL_HFC_SC_APENABLE	HFC_FNC_ADP_ENABLE		// hotplug
#define IOCTL_HFC_SC_APDISABLE	HFC_FNC_ADP_DISABLE		// hotplug
#define IOCTL_HFC_SC_SCSISCAN	HFC_FNC_SCSI_SCAN		// FCLNX-0303
#define IOCTL_HFC_ADP_PATHCHG	HFC_FNC_ADP_PATHCHG
#define IOCTL_HFC_DIAG0         HFC_FNC_DIAG0
#define IOCTL_HFC_HBA_ISOLATION HFC_FNC_HBA_ISOLATION	/* FCLNX-0454 */
#define IOCTL_HFC_READ_APPARAM	HFC_FNC_READ_APPARAM	/* FCLNX-0454 */
#define IOCTL_HFC_DG_ONLINE_UP  HFC_DG_ONLINE_UP        /* FCLNX-GPL-112 */
#define IOCTL_HFC_MP_LG_DEV_PARM	HFC_FNC_MP_LGDEV_PARM	/* FCLNX-0677 */	/* FCLNX-GPL-147 */
#define IOCTL_HFC_DG_TGTSCAN  	HFC_DG_TGTSCAN        	/* FCLNX-GPL-492 */
#define IOCTL_HFC_DG_CHGPARM  	HFC_DG_CHGPARM        	/* FCLNX-GPL-493 */

// #define #define IOCTL_HFC_WWN_INFO	HFC_FNC_WWN_INFO    *** Attention!! : defined hfcddwwn.h ***
#endif

/************************************************************************/
/* include File                                                         */
/************************************************************************/
#ifdef _HFC_PCM
#include "hfclddioc2.h"
#endif

/************************************************************************/
/*   DIAG                                                               */
/************************************************************************/
struct performance_info {
	/* driver io counter */
	uint64_t			wr_exec_cnt;
	uint64_t			rd_exec_cnt;
	uint64_t			wr_cnt;
	uint64_t			rd_cnt;
	uint64_t			wr_end_cnt[6];
	uint64_t			rd_end_cnt[6];
	uint64_t			scsi_err_cnt;
	uint64_t			xrb_resp_cnt;
	uint64_t			wr_data_size;
	uint64_t			rd_data_size;
	
	/* portstatistics data */
	uint64_t			tx_frames;
	uint64_t			tx_words;
	uint64_t			rx_frames;
	uint64_t			rx_words;
	uint64_t			lip_count;
	uint64_t			nos_count;
	uint64_t			link_failure_count;
	uint64_t			loss_of_sync_count;
	uint64_t			loss_of_signal_count;
	uint64_t			error_frames;
	uint64_t			invalid_crc_count;
	uint				fw_store_count;
	
	/* resource busy counter */
	uint				xob_full_cnt;
	uint				iovmap_full_cnt;
	uint				frame_full_cnt;
	uint				dma_max_over_cnt;
	uint				hfcpkt_full_cnt;
	
	/* Max number of cmds per int */
	uint				max_cmd_num_int;

};

struct hfc_pm_pkt_fx {
	uint				cmd_flags;		/* scsi_pkt control flag (bit)				*/
	uint				data_size;		/* Total Data Size							*/

	/* TimeStampCounter */
	uint64_t			tsc_strategy;
	uint64_t			tsc_enq_xob;
	uint64_t			tsc_xrb_int;
	uint64_t			tsc_iodone;
	ushort				cpuno_strategy;
	ushort				cpuno_enq_xob;
	ushort				cpuno_xrb_int;
	ushort				cpuno_iodone;
	ushort				xrb_cmd_cnt;
	ushort				pm_pkt_no;		/* hfc_pm_pkt number						*/
	uchar				core_no;		/* core number								*/
	uchar				cdb;			/* cdb byte0								*/
	uchar				data_type;		/* FCP-CNTL Byte3							*/
};

struct vport_list {
	uchar				vport_info[256];
	uchar				rid[256];
	uchar				sub_rid[256];
};

struct trc_seg_info {
    uchar               seg_no ;
    uchar               resv0 ;
    uchar               mode ;
    uchar               ctl ;
    uint                top_port_id ;
    uint                bottom_port_id ;
    uint                resv1 ;
};

struct fw_trc_info {
    uchar               trc_num ;
    uchar               inst ;
    uchar               resv0[14] ;
    struct trc_seg_info trc_seg[4] ;
    uchar               resv1[48] ;
};

/*-------------------------*/
/*--- HFCDIAG0 struct  ----*/
/*-------------------------*/
struct diag_ioctl {
    uchar    minor                  HFC_VAL_ATTR_C;
    uchar    core_no                HFC_VAL_ATTR_C;
    uchar    sub_cmd                HFC_VAL_ATTR_C;
#define HFC_DG_SETFW        0x01
#define HFC_DG_MIHLOG       0x02
#define HFC_DG_LDCHTRC      0x03
#define HFC_DG_SMINT        0x04
#define HFC_DG_FORCSLOG     0x05
#define HFC_DG_FWSTART      0x09
#define HFC_DG_FWPOST       0x0a
#define HFC_DG_ONLINE_UP    0x0b    /* FCLNX-GPL-112 */
#define HFC_DG_TGTSCAN      0x0c    /* FCLNX-GPL-492 */
#define HFC_DG_CHGPARM      0x0d    /* FCLNX-GPL-493 */
#define HFC_DG_FLASH_CHGPARM    0x0e
#define HFC_DG_FLASH2_CHGPARM   0x0f
#define HFC_DGRD_INITTBL    0x10
#define HFC_DGRD_XOB        0x11
#define HFC_DGRD_XRB        0x12
#define HFC_DGRD_SEGINFO    0x13
#define HFC_DGRD_SCMD       0x14
#define HFC_DGRD_ADAP       0x15
#define HFC_DGRD_TARGET     0x16
#define HFC_DGRD_DEV        0x17
#define HFC_DGRD_HFCINFO    0x18
#define HFC_DGRD_DDTRC      0x19
#define HFC_DGRD_MB         0x1a
#define HFC_DGRD_VER        0x1b
#define HFC_DGRD_HWLOG      0x1c
#define HFC_DGRD_TREE       0x1d
#define HFC_DGRD_CORE       0x1e
#define HFC_DGRD_CORETRC    0x1f
#define HFC_DG_RDPCI        0x20
#define HFC_DG_WRPCI        0x21
#define HFC_DG_RDPCI_CFG    0x22
#define HFC_DG_WRPCI_CFG    0x23
#define HFC_DG_FSTOP        0x24 /* @1.86 */
#define HFC_DGRD_MPINFO     0x25
#define HFC_DG_INITMDST     0x26
#define HFC_DG_FCPMDST      0x27
#define HFC_DGRD_MNGINFO    0x28
#define HFC_DGRD_PKGTYPE    0x29
#define HFC_DGRD_WWN        0x2a
#define HFC_DGRD_UID        0x2b    /* FCWIN-XXXX */
#define HFC_DGRD_ERRLOG     0x2c    /* FCWIN-XXXX */
#define HFC_DG_LOGERS       0x2d    /* FCWIN-0184 */
#define HFC_DGRD_HBAINFO    0x2e    /* FCWIN-0183 */
#define HFC_DGRD_LGTGT		0x30
#define HFC_DGRD_LGDEV		0x31
#define HFC_DGRD_PATHINFO	0x32
#define HFC_DGRD_PATHINFO1	0x33
#define HFC_DGRD_PATHINFO2	0x34
#define HFC_DGRD_FOINFO		0x35
#define HFC_DG_RDHG         0x36
#define HFC_DG_WRHG         0x37
#define HFC_DGRD_ALLTRC     0x38
#define HFC_DGRD_PAYLOAD		0x40
#define HFC_DGRD_RCV_PAYLOAD	0x41
#define HFC_DG_PERFORMANCE	0x42
#define HFC_DG_SETFW_FX		0x43
#define HFC_DG_HFCLDD_CONF	0x44
#define HFC_DG_LINK_RESET	0x45	/* Dynamic Link Reset *//* FCLNX-GPL-FX-137 */
#define HFC_DGRD_MBTRC    	0x46	/* FCLNX-GPL-FX-139 */
#define HFC_DG_FLASH_UPDATE 0x47	/* FCLNX-GPL-FX-146 */
#define HFC_DGRD_VPORT_LIST	0x48
#define HFC_DGRD_RSV_PKT	0x49
#define HFC_DGRD_PM_PKT		0x4a

//    uchar    rid                    HFC_VAL_ATTR_C;
	uchar    vport_no               HFC_VAL_ATTR_C;
//    uchar    core_no                HFC_VAL_ATTR_C;
    union {

        struct fw_trc_info trc_mode HFC_VAL_ATTR;   /* --- Set FPP Trace Mode --- */

        struct {                                    /* --- Load CH Trace Log ---- */
            struct {                                /* FPP Trace Pointer */
                ushort  sp          HFC_VAL_ATTR;
                ushort  ep          HFC_VAL_ATTR;
                uchar   ctl         HFC_VAL_ATTR_C;
                uchar   resv0       HFC_VAL_ATTR_C;
                ushort  cp          HFC_VAL_ATTR;
            }trc_ptr[4];
            struct {                                /* Frame Trace Pointer */
                ushort  sp          HFC_VAL_ATTR;
                ushort  ep          HFC_VAL_ATTR;
                uchar   ctl         HFC_VAL_ATTR_C;
                uchar   resv0       HFC_VAL_ATTR_C;
                ushort  cp          HFC_VAL_ATTR;
            }frm_trc_ptr;
            uchar   resv4[88]       HFC_VAL_ATTR_C;
        }ch_trc_log;

        struct {                                    /* --- Read Xrb/Xob --------- */
            ushort   no             HFC_VAL_ATTR;   /* read no  */
            ushort   cnt            HFC_VAL_ATTR;   /* read cnt */
            ushort   current_no     HFC_VAL_ATTR;   /* current_no */
            uchar    resv4[122]     HFC_VAL_ATTR_C;
        }xob_xrb_scmd;

        struct {                                    /* --- Read Target/Dev ------ */
            uint64_t target_id      HFC_VAL_ATTR;   /* TARGET ID */
            uint64_t lun_id         HFC_VAL_ATTR;   /* LUN ID */
            uint64_t ww_name        HFC_VAL_ATTR;   /* World Wide Port Name @dyntrk */
            uchar   resv4[104]      HFC_VAL_ATTR_C;   /* @dyntrk */
        }dev_target;

        struct {                                    /* --- Read device tree ----- */
            uchar   type            HFC_VAL_ATTR_C;   /* sub_cmd                    */
                                                    /*  HFC_DGRD_MP(mp_info)      */
                                                    /*  HFC_DGRD_ADAP(adap_info)  */
                                                    /*  HFC_DGRD_TARGET(target)   */
                                                    /*  HFC_DGRD_DEV(device)      */
            uchar   resv3[3]        HFC_VAL_ATTR_C;
            uint    entry_num       HFC_VAL_ATTR;   /*  responce entry num        */
            uchar   resv4[120]      HFC_VAL_ATTR_C;
        }dev_tree;

        struct {                                    /* --- mih log -------------- */
            uchar   log[32]         HFC_VAL_ATTR_C;   /* log infomation */
            uchar   resv4[96]       HFC_VAL_ATTR_C;
        }mih_log;
#if 0
        struct {
			uchar	lun             HFC_VAL_ATTR_C;          /* --- Loop ----------------- */
            uchar   resv4[7]		HFC_VAL_ATTR_C;
            
            struct {
				uchar		cmd     HFC_VAL_ATTR_C;
				uchar		flags   HFC_VAL_ATTR_C;
				ushort		count;
				uint		resv5;
				uint64_t	data_addr;
				uchar		dsb     HFC_VAL_ATTR_C;
				uchar		csb     HFC_VAL_ATTR_C;
				ushort		resid;
				uchar		resv6[4] HFC_VAL_ATTR_C;
			}ccw_data;
			
			uchar	resv6[96]      HFC_VAL_ATTR_C;
        }loop;
#endif
        struct {                                    /* --- FW DIAG Post --------- */
            uchar   cmd             HFC_VAL_ATTR_C;   /* command */
#define POST_CMD_BOOT		0x11					/*  BOOTtest				*/
#define POST_CMD_IMEM 		0x12					/*  internal RAM test		*/
#define POST_CMD_ILOOP		0x13					/*  internal LOOP test		*/
#define POST_CMD_DMA		0x14					/*  DMA test				*/
#define POST_CMD_ELOOP		0x15					/*  external LOOPtest		*/
#define POST_CMD_LLOOP		0x16
#define POST_CMD_BOOT_MLT	0x17					/*  multi BOOT test			*/
#define POST_CMD_ILOOP_MLT	0x18					/*  multi internal LOOP test */
#define POST_CMD_ELOOP_MLT	0x19					/*  multi external LOOP test */
#define POST_CMD_LLOOP_MLT	0x1a					/*  multi WRAP test         */
            uchar   reslt           HFC_VAL_ATTR_C;   /* result this test         */
            uchar   err_code[3]     HFC_VAL_ATTR_C;   /* err_code                 */
            uchar	resv4[3]		HFC_VAL_ATTR_C;   
            uchar   core_result[4]  HFC_VAL_ATTR_C;   	/* result of this test for cores */
            uchar   core_err_code[4][3] HFC_VAL_ATTR_C;	/* err_code for cores     */
            uchar   resv5[104]      HFC_VAL_ATTR_C;
        }post;

        struct {                                    /* --- pci ------------------ */
            uint    addr            HFC_VAL_ATTR;   /* PCI offset address */
			uint    base_addr_type  HFC_VAL_ATTR;   /* PCI access type */ /* FCLNX-GPL-120 */
#define PCI_BAR0        0
#define PCI_BAR1        1
			uchar   err_code        HFC_VAL_ATTR_C; /* error code */ /* FCLNX-GPL-120 */
#define HG_SHARED       0x00
#define HG_DEDICATED    0x01
#define HG_SHADOW       0x10
#define HG_BASIC        0x20
#define HG_NONE         0xff
            uchar   resv4[119]      HFC_VAL_ATTR_C;
        }pci;

        struct {                                    /* --- FW Start ------------- */
            uchar   mode            HFC_VAL_ATTR_C;                   /* mode */
#define FW_MODE_NORMAL      0x00                    /* normal               */
#define FW_MODE_DIAG        0x08                    /* diag                 */
#define FW_MODE_NAI_LOOP    0x81                    /* internal loop        */
#define FW_MODE_GAI_LOOP    0x82                    /* external loop        */
#define FW_MODE_IOS         0x83                    /* IOS mode             */
#define FW_MODE_WRAP        0x84                    /* WRAP                 */
            uchar   err_code        HFC_VAL_ATTR_C;   /* filed DIAG test */
                                                    /*   err_code is set in DA.    */
                                                    /*   (only valid in FW_MODE_NORMAL) */
#define FW_DIAG_SUCCESS     0x00                    /*  succeed DIAG test   */
#define FW_DIAG_FAILED      0x80                    /*  failed  DIAG test   */
            uchar   resv4[126]      HFC_VAL_ATTR_C;
        }fw_start;
         /* @030131 */
        struct {                                    /* --- Forced sogt LOG ------- */
            ushort  dependent_code  HFC_VAL_ATTR;   /* dependent_code */
            uchar   resv0[2]        HFC_VAL_ATTR_C;
            uchar   resv1[124]      HFC_VAL_ATTR_C;
        }forced_log;

        struct {                                    /* --- INIT MODE SET ------- */
            union {
                uint mode           HFC_VAL_ATTR;
                uchar uc_mode[4]    HFC_VAL_ATTR_C;
                uchar al_pa[4]      HFC_VAL_ATTR_C;
            }mode_alpa;
            uint    connect_type    HFC_VAL_ATTR;   /* Connection Type */
#define FW_CONTYPE_PTOP     0x00000001              /*  PtoP  */
#define FW_CONTYPE_AL       0x00000003              /*  FC-AL */
            uchar   resv0[ 4 ]      HFC_VAL_ATTR_C;
            uint64_t wwpn           HFC_VAL_ATTR;   /* World Wide Port Name @dyntrk */
            uint64_t wwnn           HFC_VAL_ATTR;   /* World Wide Node Name @dyntrk */
            uint    buf_size        HFC_VAL_ATTR;   /* Buffer Size */
            uint    xrdy_div        HFC_VAL_ATTR;   /* XRDY Divsize */
            uint    param           HFC_VAL_ATTR;   /* Parameter */
            uchar   resv1[ 88 ]     HFC_VAL_ATTR_C;
        }init_mode_set;

        struct {                                    /* --- FCP MODE SET ------- */
            uint    act_ctl         HFC_VAL_ATTR;   /* ACT-CTL */
            uint    il_ctl          HFC_VAL_ATTR;   /* IL-CTL */
            uint    seq_ctl         HFC_VAL_ATTR;   /* SEQ-CTL */
            uint    rsp_delay       HFC_VAL_ATTR;   /* RESP-DELAY */
            uchar   resv[112]       HFC_VAL_ATTR_C;
        }fcp_mode_set;

		struct {		/* --- Online Update --- */	/* FCLNX-GPL-112 */
			uchar   errcode			HFC_VAL_ATTR_C;	/*  Response code */
									/*  0x00 : F/W initiation succeeded */

									/*  0x01 : Online update failed */
									/*  0x02 : F/W is busy */
			uchar   resv0[3]		HFC_VAL_ATTR_C;
			uint    before_sysrev;				/* Sysrev before update */
			uint    after_sysrev;				/* Sysrev after  update */
			uchar   resv1[116];
		}online_up ;

		struct {		/* --- scan target for FCSW --- */	/* FCLNX-GPL-492 */
			uchar   errcode		HFC_VAL_ATTR_C;	/*  Response code */
#define	HFC_TGTSCAN_START		0		/*  0x00 : Issue GIDFT */
#define	HFC_TGTSCAN_LINKDOWN	1		/*  0x01 : Adapter Status is Linkdown. */
#define	HFC_TGTSCAN_ADAP_STATUS	2		/*  0x02 : F/W is busy */
#define	HFC_TGTSCAN_NOT_SWITCH	3		/*  0x03 : Without FCSwitch. */
			uchar   resv[127]	HFC_VAL_ATTR_C;
		}tgtscan ;
		struct {		/* --- change parameter --- */	/* FCLNX-GPL-493 */
			uchar   errcode		HFC_VAL_ATTR_C;	/*  Response code */
#define	HFC_CHGPRM_NORMAL		0		/*  0x00 : Normal END */
#define	HFC_CHGPRM_FORCE_DEF	1		/*  0x01 : Force Default Parameter Enable. */
#define	HFC_CHGPRM_BUSY			2		/*  0x02 : Physival Port Busy 		*/
#define	HFC_CHGPRM_VPORT_BUSY	3		/*  0x03 : Virtual Port Busy		*/
			uchar	version		HFC_VAL_ATTR_C;
			uchar	ignore_force_default	HFC_VAL_ATTR_C;
			uchar	resv3[1]				HFC_VAL_ATTR_C;

#define	HFC_CHGPRM_OPR_NONE		0		/*  0x00 : Nop */
#define	HFC_CHGPRM_OPR_SET		1		/*  0x01 : Set Parameter */
#define	HFC_CHGPRM_OPR_DEL		2		/*  0x02 : Delete Parameter */
#define	HFC_CHGPRM_OPR_READ		3		/*  0x03 : READ Parameter */
			uchar	opr_limit_log			HFC_VAL_ATTR_C;
			uchar	val_limit_log			HFC_VAL_ATTR_C;
			uchar	opr_filter_target		HFC_VAL_ATTR_C;
			uchar	val_filter_target		HFC_VAL_ATTR_C;
			uchar	resv0[2]				HFC_VAL_ATTR_C;
			uchar	opr_core_control		HFC_VAL_ATTR_C;
			uchar	val_core_control		HFC_VAL_ATTR_C;
			uchar	opr_link_reset			HFC_VAL_ATTR_C;
			uchar	val_link_reset			HFC_VAL_ATTR_C;
			uchar	opr_link_down			HFC_VAL_ATTR_C;
			uchar	val_link_down			HFC_VAL_ATTR_C;
			uchar	opr_reset_delay			HFC_VAL_ATTR_C;
			uchar	val_reset_delay			HFC_VAL_ATTR_C;
			uchar	opr_mck_retry			HFC_VAL_ATTR_C;
			uchar	val_mck_retry			HFC_VAL_ATTR_C;
			uchar	opr_reset_timeout		HFC_VAL_ATTR_C;
			uchar	val_reset_timeout		HFC_VAL_ATTR_C;
			uchar	opr_abort_timeout		HFC_VAL_ATTR_C;
			uchar	val_abort_timeout		HFC_VAL_ATTR_C;
			uchar	opr_queue_depth			HFC_VAL_ATTR_C;
			uchar	val_queue_depth			HFC_VAL_ATTR_C;
			uchar	opr_scsi_allowed		HFC_VAL_ATTR_C;
			uchar	val_scsi_allowed		HFC_VAL_ATTR_C;
			uchar	opr_ld_err_limit_s		HFC_VAL_ATTR_C;
			uchar	val_ld_err_limit_s		HFC_VAL_ATTR_C;
			uchar	opr_if_err_limit		HFC_VAL_ATTR_C;
			uchar	resv1[1]				HFC_VAL_ATTR_C;
			uint	val_if_err_limit		HFC_VAL_ATTR;
			uchar	opr_to_err_limit		HFC_VAL_ATTR_C;
			uchar	opr_ld_err_limit_l		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	val_ld_err_limit_l		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	resv2					HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uint	val_to_err_limit		HFC_VAL_ATTR;
			uchar	opr_rt_err_enable		HFC_VAL_ATTR_C;
			uchar	val_rt_err_enable		HFC_VAL_ATTR_C;
			uchar	opr_hba_isolation		HFC_VAL_ATTR_C;
			uchar	val_hba_isolation		HFC_VAL_ATTR_C;
			uchar	opr_rdtsc				HFC_VAL_ATTR_C;
			uchar	val_rdtsc				HFC_VAL_ATTR_C;
			uchar	opr_intdisable			HFC_VAL_ATTR_C;
			uchar	val_intdisable			HFC_VAL_ATTR_C;
			uchar	opr_intenable			HFC_VAL_ATTR_C;
			uchar	val_intenable			HFC_VAL_ATTR_C;
			uchar	opr_total_abort_to		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014 */
			uchar	val_total_abort_to		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014 */
			uchar	opr_total_tgtrst_to		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014 */
			uchar	val_total_tgtrst_to		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014 */
			uchar	opr_cc_cnt				HFC_VAL_ATTR_C;
			uchar	resv6[1]				HFC_VAL_ATTR_C;
			uint	val_cc_cnt				HFC_VAL_ATTR;
			uchar	opr_cc_size				HFC_VAL_ATTR_C;
			uchar	resv7[3]				HFC_VAL_ATTR_C;
			uint	val_cc_size				HFC_VAL_ATTR;
			uchar	opr_cc_core				HFC_VAL_ATTR_C;
			uchar	val_cc_core				HFC_VAL_ATTR_C;
			uchar	opr_maxio				HFC_VAL_ATTR_C;
			uchar	val_maxio				HFC_VAL_ATTR_C;
			uchar	opr_abort_t_restrain	HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-175 */
			uchar	val_abort_t_restrain	HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-175 */
			uchar	opr_tgtrst_restrain		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-175 */
			uchar	val_tgtrst_restrain		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-175 */
			uchar	opr_lun_reset_delay		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-175 */
			uchar	val_lun_reset_delay		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-175 */
			uchar	opr_ld_err_intvl		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	val_ld_err_intvl		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	opr_if_err_intvl		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	val_if_err_intvl		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	opr_to_err_intvl		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar	val_to_err_intvl		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-314 */
			uchar   resv5[40]				HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014,314 */
		}changeparm ;
		
		struct {		/* --- change parameter flash --- */	/* FCLNX-GPL- */
			uchar   errcode					HFC_VAL_ATTR_C;	/*  Response code */
#define	HFC_CHGPRM_NORMAL		0		/*  0x00 : Issue GIDFT */
#define	HFC_CHGPRM_FORCE_DEF	1		/*  0x01 : Force Default Parameter Enable. */
			uchar	version					HFC_VAL_ATTR_C;

#define	HFC_CHGPRM_OPR_NONE		0		/*  0x00 : Nop */
#define	HFC_CHGPRM_OPR_SET		1		/*  0x01 : Set Parameter */
#define	HFC_CHGPRM_OPR_DEL		2		/*  0x02 : Delete Parameter */
#define	HFC_CHGPRM_OPR_READ		3		/*  0x03 : READ Parameter */
			uchar	opr_mck_linkup_timer	HFC_VAL_ATTR_C;
			uchar	val_mck_linkup_timer	HFC_VAL_ATTR_C;
			uchar	opr_core_degradation	HFC_VAL_ATTR_C;
			uchar	val_core_degradation	HFC_VAL_ATTR_C;
			uchar	opr_rft_id_skip			HFC_VAL_ATTR_C;
			uchar	val_rft_id_skip			HFC_VAL_ATTR_C;
			uchar	opr_hba_isol_cmd		HFC_VAL_ATTR_C;
			uchar	val_hba_isol_cmd		HFC_VAL_ATTR_C;
			uchar	opr_link_init_timer		HFC_VAL_ATTR_C;
			uchar	val_link_init_timer		HFC_VAL_ATTR_C;
			uchar	opr_mbgrp_rsp_timer[7]	HFC_VAL_ATTR_C;
			uchar	val_mbgrp_rsp_timer[7]	HFC_VAL_ATTR_C;
			uchar	opr_mbgrp_retry_func[7]	HFC_VAL_ATTR_C;
			uchar	val_mbgrp_retry_func[7]	HFC_VAL_ATTR_C;
			uchar	opr_mbgrp_retry_delay[7]  HFC_VAL_ATTR_C;
			uchar	val_mbgrp_retry_delay[7]  HFC_VAL_ATTR_C;
			uchar	opr_mailbox_delay[25]	HFC_VAL_ATTR_C;
			uchar	val_mailbox_delay[25]	HFC_VAL_ATTR_C;
			uchar	opr_wait_plogi_recv		HFC_VAL_ATTR_C;
			uchar	val_wait_plogi_recv		HFC_VAL_ATTR_C;
			uchar	opr_mailbox_force_retry	HFC_VAL_ATTR_C;
			uchar	val_mailbox_force_retry	HFC_VAL_ATTR_C;
			uchar	opr_login_filter		HFC_VAL_ATTR_C;
			uchar	val_login_filter		HFC_VAL_ATTR_C;
			uchar	opr_vf_enable			HFC_VAL_ATTR_C;
			uchar	val_vf_enable			HFC_VAL_ATTR_C;
			uchar	opr_vf_mode				HFC_VAL_ATTR_C;
			uchar	val_vf_mode				HFC_VAL_ATTR_C;
			uchar	opr_security_enable		HFC_VAL_ATTR_C;
			uchar	val_security_enable		HFC_VAL_ATTR_C;
			uchar	opr_login_delay_time	HFC_VAL_ATTR_C;
			uchar	val_login_delay_time	HFC_VAL_ATTR_C;
			uchar	resv1[10]				HFC_VAL_ATTR_C;
		}changeparm_flash ;
		struct {
			uchar   errcode					HFC_VAL_ATTR_C;	/*  Response code */
#define	HFC_CHGPRM_NORMAL		0		/*  0x00 : Normal END */
#define	HFC_CHGPRM_FORCE_DEF	1		/*  0x01 : Force Default Parameter Enable. */
			uchar	version					HFC_VAL_ATTR_C;

#define	HFC_CHGPRM_OPR_NONE		0		/*  0x00 : Nop */
#define	HFC_CHGPRM_OPR_SET		1		/*  0x01 : Set Parameter */
#define	HFC_CHGPRM_OPR_DEL		2		/*  0x02 : Delete Parameter */
#define	HFC_CHGPRM_OPR_READ		3		/*  0x03 : READ Parameter */
			uchar	opr_peer_password		HFC_VAL_ATTR_C;
			uchar	resv1[1]				HFC_VAL_ATTR_C;
			uchar	val_peer_password[40]	HFC_VAL_ATTR_C;
			uchar	opr_local_password		HFC_VAL_ATTR_C;
			uchar	resv2[3]				HFC_VAL_ATTR_C;
			uchar	val_local_password[40]	HFC_VAL_ATTR_C;
			uchar	resv3[40]				HFC_VAL_ATTR_C;
		}changeparm_flash2 ;

		struct {		/* --- Performance --- */
			uchar   errcode					HFC_VAL_ATTR_C;	/*  Response code */
#define	HFC_PFM_OK				0x00	/*  0x00 :  Normal End */
#define HFC_PFM_ERR_PARAM		0x01	/*  0x01 :  Parameter Error */
			uchar	version					HFC_VAL_ATTR_C;
			uchar	opcode					HFC_VAL_ATTR_C;
#define	HFC_PFM_OPR_GET			0x80
#define	HFC_PFM_OPR_CLR			0x40
#define	HFC_PFM_OPR_START		0x20
#define	HFC_PFM_OPR_IOCNT		0x10
#define	HFC_PFM_OPR_CNT_CLR		0x08
#define	HFC_PFM_OPR_IO_CLR		0x04
			uchar   pm_control				HFC_VAL_ATTR_C;
										/* 0x00: performance monitor on  */
										/* 0x01: performance monitor off */

			uchar   resv1[124]				HFC_VAL_ATTR_C;
		}performance ;
		
		struct {		/* --- Dynamic Link Reset --- *//* FCLNX-GPL-FX-137 */
			uchar	errcode					HFC_VAL_ATTR_C;
#define HFC_LINKRESET_START				0	/* Normal. Link Reset Start. */
#define HFC_LINKRESET_ADAP_STATUS		1	/* Busy. Require retry. */
#define HFC_LINKRESET_NOT_ACCEPTABLE	2	/* not acceptable. */
			uchar	npiv_config				HFC_VAL_ATTR_C;		/* 0x00:NPIV disable, 0x80:NPIV enable */
			uchar	connect_type			HFC_VAL_ATTR_C;		/* 0x00:Auto, 0x01:PtoP, 0x03:Loop */
			uchar	link_speed				HFC_VAL_ATTR_C;		/* 0x00:Auto, 0x04:4Gbps, 0x08:8Gbps, 0x10:16Gbps */
			uchar	multiple_portid			HFC_VAL_ATTR_C;		/* 0:disable 1:enable */
			uchar	resv1[123]				HFC_VAL_ATTR_C;
		} link_reset ;	/* FCLNX-GPL-FX-137 */
		
		struct {		/* --- fw trace mode --- */
			ushort	trc_mode				HFC_VAL_ATTR;
			uchar	resv1[126]				HFC_VAL_ATTR_C;
		}fw_trc_info_fx ;

		struct {		/* --- fw trace mode --- */
			uchar	resv1[128]				HFC_VAL_ATTR_C;
		}hfcldd_conf ;
		
		struct {							/* Nofity Flash Update *//* FCLNX-GPL-FX-146 */
			uchar	errcode					HFC_VAL_ATTR_C;
#define HFC_FLASHUP_SUCCESS			0		/* Start Flash Update normaly	*/
#define HFC_FLASHUP_ADAP_STATUS		1		/* Failed to start Flash Update	*/
			uchar	opcode					HFC_VAL_ATTR_C;
#define HFC_FLASHUP_START		1
#define HFC_FLASHUP_FINISH		2
			uchar	resv1[126] 				HFC_VAL_ATTR_C;
		} flash_update ;

    }uni;
    uint                length      HFC_VAL_ATTR;   /* --- output size     ----- */
    uint64_t            addr        HFC_VAL_ATTR;   /* --- output address  ----- */
};


/************************************************************************/
/* IOCTL                                                                */
/************************************************************************/
/********************************************/
/* IOCTL_HFC_SC_INQU : issue INQUIRY 2005.01.19      */
/********************************************/
struct hfc_ioctl_inquiry {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       resv1           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */
//    uchar       resv1[2]        HFC_VAL_ATTR_C;   /* reserved                             */

    uchar       extended        HFC_VAL_ATTR_C;   /* TRUE / FALSE                         */
    uchar       page_code       HFC_VAL_ATTR_C;   /* page code # when extended = TRUE     */
    ushort      data_length     HFC_VAL_ATTR;   /* pointer to inquiry data length       */

    /******  8 byte boundary ********/
    uint64_t    buffer          HFC_VAL_ATTR;   /* pointer to inquiry data buffer       */

        /******  8 byte boundary ********/
    uint64_t    Port_WWN        HFC_VAL_ATTR;   /* Port_WWN                             */
    uint64_t    lun_id          HFC_VAL_ATTR;   /* Lun ID                               */

    /******  8 byte boundary ********/
    uchar       scsi_status     HFC_VAL_ATTR_C;   /* SCSI status                          */
    uchar       resid_flags     HFC_VAL_ATTR_C;
#define HFC_RESID_UNDERFLOW 0x80                  /* underflow                            */
    ushort      resid           HFC_VAL_ATTR;     /* remain byte                          */

    uchar       resv2[3]        HFC_VAL_ATTR_C;
    uchar       did_err         HFC_VAL_ATTR_C;   /* adapter status                       */
    /******  8 byte boundary ********/
    uint64_t    sense_ptr       HFC_VAL_ATTR;     /* if scsi_status<check condition>=1    */

    uchar       resv5[2]        HFC_VAL_ATTR_C;
    ushort      sense_len       HFC_VAL_ATTR;     /* if scsi_status<check condition>=1    */
};

/********************************************/
/* IOCTL_HFC_SC_CMD : issue SCSI command 2005.01.19*/
/********************************************/
struct hfc_ioctl_cdb {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       rsv             HFC_VAL_ATTR_C;
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */
#define SCSI_READ_CDB       0x80
#define SCSI_WRITE_CDB      0x40
#define SCSI_PATH_HEALTH    0x01                /* FCLNX-0408 */

//    uchar       resv1[2]        HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       resv2[2]        HFC_VAL_ATTR_C;
    ushort      data_length     HFC_VAL_ATTR;   /* bytes of data to be transfered       */

    /******  8 byte boundary ********/
    uint64_t    buffer          HFC_VAL_ATTR;   /* pointer to transfer data buffer      */

        /******  8 byte boundary ********/
    uint64_t    Port_WWN        HFC_VAL_ATTR;   /* Port_WWN                             */
    uint64_t    lun_id          HFC_VAL_ATTR;   /* Lun ID                               */

    /******  8 byte boundary ********/
    uchar       tag_q           HFC_VAL_ATTR_C;   /* tag-Q type                           */
    uchar       command_length  HFC_VAL_ATTR_C;   /* length of SCSI CDB                   */
    uchar       resv3[6]        HFC_VAL_ATTR_C;

    /******  8 byte boundary ********/
    uchar       cdb[16]         HFC_VAL_ATTR_C;   /* SCSI command descriptor block        */

    uchar       scsi_status     HFC_VAL_ATTR_C;   /* SCSI status                          */
    uchar       resid_flags     HFC_VAL_ATTR_C;
#define HFC_RESID_UNDERFLOW 0x80                /* The transfer length is insufficient. */
    ushort      resid           HFC_VAL_ATTR;   /* remained byte count                  */

    uchar       resv4[3]        HFC_VAL_ATTR_C;
    uchar       did_err         HFC_VAL_ATTR_C;   /* adapter status                       */

    /******  8 byte boundary ********/
    uint64_t    sense_ptr       HFC_VAL_ATTR;   /* if scsi_status<check condition>=1    */

    uchar       resv7[2]        HFC_VAL_ATTR_C;
    ushort      sense_len       HFC_VAL_ATTR;   /* if scsi_status<check condition>=1    */
};

/********************************************/
/* IOCTL_HFC_SC_NMSRV : issue GID_FT        */
/********************************************/
struct hfc_ioctl_gidft {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       resv2           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */
    uchar       set_flags       HFC_VAL_ATTR_C;   /* set flags                            */
#define NM_LIST_SHORT   0x01
//    uchar       resv1[1]        HFC_VAL_ATTR_C;   /* reserved                             */

//    uchar       resv2           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       adapter_status  HFC_VAL_ATTR_C;   /* adapter status                       */
    uchar       resv3[2]        HFC_VAL_ATTR_C;   /* reserved                             */

    uchar       scsi_id_size    HFC_VAL_ATTR_C;   /* size in bytes of a SCSI ID           */
    uchar       resv4[3]        HFC_VAL_ATTR_C;   /* reserved                             */
    uint        timeout_value   HFC_VAL_ATTR;   /* timeout value for command.(sec)      */

    /******  8 byte boundary ********/
    uint        list_len        HFC_VAL_ATTR;   /* length of scsi_id_list.(byte)        */
    uint        num_ids         HFC_VAL_ATTR;   /* Number of SCSI ID in the list        */

    /******  8 byte boundary ********/
    uint64_t    scsi_id_list    HFC_VAL_ATTR;   /* SCSI ID list pointer                 */
};

/* parameter */
#define HFC_GS_INV_CODE                     0x01        /* Invalid Command Code      */
#define HFC_GS_INV_VERS                     0x02        /* Invalid Version           */
#define HFC_GS_LOG_ERR                      0x03        /* Logical Error             */
#define HFC_GS_INV_IU_SIZE                  0x04        /* Invalid IU Size           */
#define HFC_GS_BSY                          0x05        /* Logical Busy              */
#define HFC_GS_PROTO_ERR                    0x07        /* Protocol Error            */
#define HFC_GS_FAILED                       0x09        /* Unable to perform request */
#define HFC_GS_CMD_NOT_SPT                  0x0b        /* Command Not supported     */

#define HFS_GS_NO_EXP                       0x00        /* No additional explanation */
#define HFS_GS_EXP_NO_ID                    0x01        /* Port Identifier not registered       */
#define HFS_GS_EXP_NO_WWN                   0x02        /* Port Name not registered   */
#define HFS_GS_EXP_NO_NODE                  0x03        /* Node Name not registered   */
#define HFS_GS_EXP_NO_CLASS                 0x04        /* Class of Service not registered      */
#define HFS_GS_EXP_NO_IP_ADDR               0x05        /* IP address not registered  */
#define HFS_GS_EXP_NO_PA                    0x06        /* Initial Process Associator not registered */
#define HFS_GS_EXP_NO_FC4S                  0x07        /* FC-4 Types not registered  */
#define HFS_GS_EXP_NO_SYM_PORT              0x08        /* Symbolic Port Name not registered    */
#define HFS_GS_EXP_NO_SYM_NODE              0x09        /* Symbolic Node Name not registered    */
#define HFS_GS_EXP_NO_PORT_TYPE             0x0a        /* Port Type not registered   */
#define HFS_GS_EXP_NO_ACCESS                0x10        /* Access Denied              */
#define HFS_GS_EXP_INV_PORT                 0x11        /* Unacceptable Port Identifier         */
#define HFS_GS_EXP_DB_EMPTY                 0x12        /* Database empty.            */

/********************************************/
/* IOCTL_HFC_SC_GWWN : issue GID_PN         */
/********************************************/
struct hfc_ioctl_gidpn {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       resv1           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */
//    uchar       resv1[2]        HFC_VAL_ATTR_C;   /* reserved                             */

    uchar       resv2           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       adapter_status  HFC_VAL_ATTR_C;   /* adapter status                       */
    uchar       resv3[2]        HFC_VAL_ATTR_C;   /* reserved                             */

    /******  8 byte boundary ********/
    uint64_t world_wide_name    HFC_VAL_ATTR;   /* World Wide Port Name                 */

    /******  8 byte boundary ********/
    uint64_t scsi_id            HFC_VAL_ATTR;   /* return SCSI ID                       */
};

/********************************************/
/* IOCTL_HFC_SC_GSID : issue GPN_ID         */
/********************************************/
struct hfc_ioctl_gpnid {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       resv1           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */
//    uchar       resv1[2]        HFC_VAL_ATTR_C;   /* reserved                             */

    uchar       resv2           HFC_VAL_ATTR_C;   /* reserved                             */
    uchar       adapter_status  HFC_VAL_ATTR_C;   /* adapter status                       */
    uchar       resv3[2]        HFC_VAL_ATTR_C;   /* reserved                             */

    /******  8 byte boundary ********/
    uint64_t scsi_id            HFC_VAL_ATTR;   /* SCSI ID                              */

    /******  8 byte boundary ********/
    uint64_t world_wide_name    HFC_VAL_ATTR;   /* return World Wide Port Name          */
};

/********************************************/
/* IOCTL_HFC_SC_PAYLD : issue SCSI PAYLD    */
/********************************************/
struct hfc_ioctl_payload {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       resv0           HFC_VAL_ATTR_C;
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */

    uchar       type            HFC_VAL_ATTR_C;   /* payload type                         */
    uchar       ctl             HFC_VAL_ATTR_C;
    uchar       class           HFC_VAL_ATTR_C;
    uchar       resv3           HFC_VAL_ATTR_C;

    uint64_t    scsi_id         HFC_VAL_ATTR;

    uint64_t    payld_buffer    HFC_VAL_ATTR;   /* send payload                         */
    uchar       resv1[4]        HFC_VAL_ATTR_C;
    int         payld_size      HFC_VAL_ATTR;

    uint64_t    response_buffer HFC_VAL_ATTR;   /* responce payload                     */
    uchar       resv2[4]        HFC_VAL_ATTR_C;
    int         response_size   HFC_VAL_ATTR;
};

struct PAYLD {
    uchar flags;
    uchar resv0[ 3 ];
    uchar payld_class;
    uchar resv1[ 3 ];
    uint resv2;
    ushort resv3;
    ushort payload_length;
    uchar tseq;
    uchar nfw[ 3 ];
    uint resv4[ 3 ];
    union {
        uchar rctl;
        uint d_id;
    } dst;
    union {
        uchar cs_ctl;
        uint s_id;
    } src;
    uchar type;
    uchar f_ctl[ 3 ];
    uchar seq_id;
    uchar df_ctl;
    ushort seq_cnt;
    ushort ox_id;
    ushort rx_id;
    uint paramater;
    uint resv5[ 242 ];
    uchar frame_payload[ 2048 ];
};

/********************************************/
/* IOCTL_HFC_SC_RNID : get & put RNID       */
/********************************************/
struct hfc_ioctl_rnid {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       resv0           HFC_VAL_ATTR_C;
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */

    uchar       cmd             HFC_VAL_ATTR_C;
    uchar       adapter_status  HFC_VAL_ATTR_C;

    uchar       common_node_len HFC_VAL_ATTR_C;
#define RNID_COM_NODE_LEN   16
    uchar       spcfic_node_len HFC_VAL_ATTR_C;
#define RNID_SPF_NODE_LEN   52
//    uchar       resv0[2]        HFC_VAL_ATTR_C;

    /******  8 byte boundary ********/
    uint64_t    port_name       HFC_VAL_ATTR;
    uint64_t    node_name       HFC_VAL_ATTR;

    /******  8 byte boundary ********/
    uint64_t    vendor_unique[2]    HFC_VAL_ATTR;

    /******  8 byte boundary ********/
    uint        node_type       HFC_VAL_ATTR;
    uint        port_number     HFC_VAL_ATTR;

    /******  8 byte boundary ********/
    uint        num_att_nodes   HFC_VAL_ATTR;
    uchar       node_mgmt       HFC_VAL_ATTR_C;
    uchar       ip_ver          HFC_VAL_ATTR_C;
    ushort      udp_port        HFC_VAL_ATTR;

    /******  8 byte boundary ********/
    uint64_t    ip_addr[2]      HFC_VAL_ATTR;

    /******  8 byte boundary ********/
    ushort      disc_flags      HFC_VAL_ATTR;
    uchar       resv1[6]        HFC_VAL_ATTR_C;
};

/********************************************/
/* IOCTL_HFC_SC_ADPST : adapter statistics  */
/********************************************/
struct hfc_ioctl_adap_stat {
    uchar       minor           HFC_VAL_ATTR_C;
    uchar       rsv             HFC_VAL_ATTR_C;
    uchar       version         HFC_VAL_ATTR_C;   /* version code                         */
    uchar       flags           HFC_VAL_ATTR_C;   /* control flags                        */
    uchar       cmd             HFC_VAL_ATTR_C;
    uchar       adapter_status  HFC_VAL_ATTR_C;
    uchar       resv[2]         HFC_VAL_ATTR_C;

    uint64_t seconds_since_last_reset   HFC_VAL_ATTR;
    uint64_t tx_frames          HFC_VAL_ATTR;
    uint64_t tx_words           HFC_VAL_ATTR;
    uint64_t rx_frames          HFC_VAL_ATTR;
    uint64_t rx_words           HFC_VAL_ATTR;
    uint64_t lip_count          HFC_VAL_ATTR;
    uint64_t nos_count          HFC_VAL_ATTR;
    uint64_t error_frames       HFC_VAL_ATTR;
    uint64_t dumped_frames      HFC_VAL_ATTR;
    uint64_t link_failure_count HFC_VAL_ATTR;
    uint64_t loss_of_sync_count HFC_VAL_ATTR;
    uint64_t loss_of_signal_count   HFC_VAL_ATTR;
    uint64_t primitive_seq_protocol_err_count   HFC_VAL_ATTR;
    uint64_t invalid_tx_word_count  HFC_VAL_ATTR;
    uint64_t invalid_crc_count  HFC_VAL_ATTR;
};


/********************************************/
/* IOCTL_HFC_SC_API : get adapter attribute */
/********************************************/
struct hfc_ioctl_api {
    uchar       minor               HFC_VAL_ATTR_C;
    uchar       rsv                 HFC_VAL_ATTR_C;
    uchar       sub_cmd             HFC_VAL_ATTR_C;   /* sub-command              */
#define HFC_GET_ADAPTER_ATTR        0x01            /* Adapter Attribute        */
#define HFC_GET_PORT_ATTR           0x02            /* Port Attribute           */

    uchar       valid               HFC_VAL_ATTR_C;   /* Valid bit                */
#define HFC_API_SCSI_ID_VALID       0x80            /*   vaild scsi_id          */
#define HFC_API_WWN_VALID           0x40            /*   vaild world_wide_name  */

//    uchar       resv1[2]            HFC_VAL_ATTR_C;
    uint        scsi_id             HFC_VAL_ATTR;
    uint64_t    world_wide_name     HFC_VAL_ATTR;
    uchar       resv2[100]          HFC_VAL_ATTR_C;
    uint        length              HFC_VAL_ATTR;
    uint64_t    addr                HFC_VAL_ATTR;
};

/************************************/
/*  report adapter attribute        */
/*   sub_cmd=HFC_GET_ADAPTER_ATTR   */
/************************************/
struct hfc_ioctl_adap_attr {
#define HFC_MANUFACTURE_SIZE        32
#define HFC_SERIALNUMBER_SIZE       32
#define HFC_NODE_SYMBOL_SIZE        128
#define HFC_HW_VER_SIZE             128
#define HFC_DD_VER_SIZE             128
#define HFC_ROM_VER_SIZE            128
#define HFC_FW_VER_SIZE             128

    uchar       manufacturer[HFC_MANUFACTURE_SIZE]       HFC_VAL_ATTR_C;
    uchar       serialnumber[HFC_SERIALNUMBER_SIZE]      HFC_VAL_ATTR_C;
    uint64_t    node_wwn                                 HFC_VAL_ATTR;
    uchar       node_symbolic_name[HFC_NODE_SYMBOL_SIZE] HFC_VAL_ATTR_C;
    uchar       hardware_version[HFC_HW_VER_SIZE]        HFC_VAL_ATTR_C;
    uchar       driver_version[HFC_DD_VER_SIZE]          HFC_VAL_ATTR_C;
    uchar       option_rom_version[HFC_ROM_VER_SIZE]     HFC_VAL_ATTR_C;
    uchar       firmware_version[HFC_FW_VER_SIZE]        HFC_VAL_ATTR_C;
    uint        vendor_specific_id                       HFC_VAL_ATTR;
    uint        number_of_ports                          HFC_VAL_ATTR;
#define HFC_NUM_OF_PORTS_1          1
};

/************************************/
/*  report port attribute           */
/*   sub_cmd=HFC_GET_PORT_ATTR      */
/************************************/
struct hfc_ioctl_port_attr {
    uchar       minor       HFC_VAL_ATTR_C;
    uchar       rsv         HFC_VAL_ATTR_C;
    uchar       flags       HFC_VAL_ATTR_C;
#define HFC_GET_ADAP_PORT           0x01
#define HFC_GET_DISC_PORT           0x02
    uchar       resv0[5]    HFC_VAL_ATTR_C;

    uint64_t    node_wwn    HFC_VAL_ATTR;
    uint64_t    port_wwn    HFC_VAL_ATTR;
    uint        port_fcid   HFC_VAL_ATTR;

    uint        port_type	HFC_VAL_ATTR;
#define HFC_PORTTYPE_UNKNOWN        1
#define HFC_PORTTYPE_OTHER          2
#define HFC_PORTTYPE_NOTPRESENT     3
#define HFC_PORTTYPE_NPORT          5
#define HFC_PORTTYPE_NLPORT         6
#define HFC_PORTTYPE_FLPORT         7
#define HFC_PORTTYPE_FPORT          8
#define HFC_PORTTYPE_LPORT          20
#define HFC_PORTTYPE_PTP            21

    uint        port_state 	HFC_VAL_ATTR;
#define HFC_PORTSTATE_UNKNOWN       1
#define HFC_PORTSTATE_ONLINE        2
#define HFC_PORTSTATE_OFFLINE       3
#define HFC_PORTSTATE_BYPASSED      4
#define HFC_PORTSTATE_DIAGNOSTICS   5
#define HFC_PORTSTATE_LINKDOWN      6
#define HFC_PORTSTATE_ERROR         7
#define HFC_PORTSTATE_LOOPBACK      8

    uint        port_support_class_of_service HFC_VAL_ATTR;
#define HFC_SUPPORT_CLASS           0x00000008

    char        port_support_fc4_types[32]    HFC_VAL_ATTR_C;
    char        port_active_fc4_types[32]     HFC_VAL_ATTR_C;

#define HFC_NODE_SYMBOL_SIZE        128
    char        port_symbol_name[HFC_NODE_SYMBOL_SIZE] HFC_VAL_ATTR_C;

    uint        port_supported_speed          HFC_VAL_ATTR;
#define HFC_PORTSPEED_UNKNOWN       0x00
#define HFC_PORTSPEED_1GBIT         0x01
#define HFC_PORTSPEED_2GBIT         0x02
#define HFC_PORTSPEED_10GBIT        0x04
#define HFC_PORTSPEED_4GBIT         0x08
#define HFC_PORTSPEED_8GBIT         0x10
#define HFC_PORTSPEED_16GBIT         0x20

    uint        port_speed          HFC_VAL_ATTR;

    uint        port_max_frame_size HFC_VAL_ATTR;
#define HFC_PORT_MAX_FRAME          2048

    uint        num_of_disc_port    HFC_VAL_ATTR;

#define HFC_MAX_DISC_PORT           256
    uint64_t    disc_wwn[HFC_MAX_DISC_PORT] HFC_VAL_ATTR;
};

struct hfc_ioctl_spt {
	uchar   minor                   HFC_VAL_ATTR_C;
    uchar   resv0[15]               HFC_VAL_ATTR_C;
    uchar   support_flag[0x10]      HFC_VAL_ATTR_C;
};

/************************************************/
/* IOCTL_HFC_SC_TGTMAP : for targetMapping      2005.01.19*/
/************************************************/

typedef struct hfc_scsiid {
  uchar         rsv0[3]             HFC_VAL_ATTR_C;
  uchar         target_id           HFC_VAL_ATTR_C;
  uint          target_valid        HFC_VAL_ATTR;
#define  HFC_TGTWWN_VALID               0x00000080
#define  HFC_SCSIID_VALID               0x00000040
} HFC_SCSIID;

typedef struct hfc_fcpid {
  uint64_t      node_wwn            HFC_VAL_ATTR;
  uint64_t      port_wwn            HFC_VAL_ATTR;
  uint          scsi_id             HFC_VAL_ATTR;
  uchar         rsv2[4]             HFC_VAL_ATTR_C;   /* 2005.02.23 [3]->[4] */
} HFC_FCPID;

typedef struct hfc_tgtmap_entry {
  HFC_SCSIID    scsiid;
  HFC_FCPID     fcpid;
} HFC_TGTMAP_ENTRY;

typedef struct hfc_tgtmap_hedder {
  uchar         minor               HFC_VAL_ATTR_C;
  uchar         rsv                 HFC_VAL_ATTR_C;
  uchar         version             HFC_VAL_ATTR_C;   /* version code                         */
  uchar         flags               HFC_VAL_ATTR_C;   /* control flags                        */
  uchar         resv1[4]            HFC_VAL_ATTR_C;
  uint          number_of_entries   HFC_VAL_ATTR;
  uint          number_of_target    HFC_VAL_ATTR;
  uchar         resv2[16]           HFC_VAL_ATTR_C;
} HFC_TGTMAP_HEDDER;

struct hfc_ioctl_tgt_map {
  HFC_TGTMAP_HEDDER     hedder[1];
  HFC_TGTMAP_ENTRY      entry[1];
};

/************************************************/
/* IOCTL_HFC_SC_PBIND : for fcpBinding2005.02.08*/
/************************************************/
typedef struct hfc_fcpbind_entry {
  uint         fcp_binding_type     HFC_VAL_ATTR;
  uchar        rsv0[3]              HFC_VAL_ATTR_C;
  uchar        target_id            HFC_VAL_ATTR_C;
  uint64_t     data                 HFC_VAL_ATTR;
  uint         N_port_id            HFC_VAL_ATTR;
  uchar        rsv1[4]              HFC_VAL_ATTR_C;
} HFC_FCPBIND_ENTRY;

typedef struct hfc_fcpbind_hedder {
  uchar        minor                HFC_VAL_ATTR_C;
  uchar        rsv                  HFC_VAL_ATTR_C;
  uchar        version              HFC_VAL_ATTR_C;   /* version code                         */
  uchar        flags                HFC_VAL_ATTR_C;   /* control flags                        */
  uchar        resv1[4]             HFC_VAL_ATTR_C;
  uint         number_of_entries    HFC_VAL_ATTR;
  uint         number_of_target     HFC_VAL_ATTR;
  uchar        resv2[16]            HFC_VAL_ATTR_C;
} HFC_FCPBIND_HEDDER;

struct hfc_ioctl_fcp_binding {
  HFC_FCPBIND_HEDDER    hedder[1];
  HFC_FCPBIND_ENTRY     entry[1];
};

/************************************************/
/* IOCTL_HFC_SC_PRINFO : for proc_info2005.01.28*/
/************************************************/
struct hfc_ioctl_proc_info {
  uchar     minor               HFC_VAL_ATTR_C;
  uchar     rsv                 HFC_VAL_ATTR_C;
  uchar     version             HFC_VAL_ATTR_C;   /* version code           */
  uchar     flags               HFC_VAL_ATTR_C;   /* control flags          */

  uchar     vport_no            HFC_VAL_ATTR_C;   /* vport no               */
  uchar     rsv0[3]             HFC_VAL_ATTR_C;   /*                        */
                                                  /*                        */
  int       dev_major           HFC_VAL_ATTR;     /* Major_number           */
  int       dev_minor           HFC_VAL_ATTR;     /* Minor_number           */
  uchar     instance            HFC_VAL_ATTR_C;   /* instance               */
  uchar     rsv1[1]             HFC_VAL_ATTR_C;   /*                        */
  ushort    host_no             HFC_VAL_ATTR;     /* Host#                  */
  uint      unique_id           HFC_VAL_ATTR;     /* Unique_id              */
  uchar     rsv2[2]             HFC_VAL_ATTR_C;   /*                        */
  ushort    vmhba_no            HFC_VAL_ATTR;     /* vmhba#                 */
  ushort    vender_id           HFC_VAL_ATTR;     /* Vender ID              */
  ushort    device_id           HFC_VAL_ATTR;     /* Device ID              */
                                                  /*                        */
  uint64_t  ww_name             HFC_VAL_ATTR;     /* Port name              */
  uint64_t  node_name           HFC_VAL_ATTR;     /* Node name              */
  uint64_t  scsi_id             HFC_VAL_ATTR;     /* DID                    */
  uchar     rsv3[8]             HFC_VAL_ATTR_C;   /*                        */
                                                  /*                        */
  uchar     adap_id[16]         HFC_VAL_ATTR_C;   /* adapter ID             */
                                                  /*                        */
  uchar     model_name[16];    				      /* Model Name             */ //FCLNX-0148
  uchar     driver_ver[16];    					  /* driver_ver             */ //FCLNX-0148
  uint      firmware_ver;      					  /* firmware_ver           */ //FCLNX-0148
  uchar     rsv6[12]			HFC_VAL_ATTR_C;   /*                        */ /* FCLNX-0404 */
  uchar		parts_number[16]	HFC_VAL_ATTR_C;   /* parts number           */ /* FCLNX-0404 */
  uchar		pkgtype				HFC_VAL_ATTR_C;   /* Package Type           */ /* FCLNX-0404 */
  uchar		pkgcode				HFC_VAL_ATTR_C;   /* Package ID             */ /* FCLNX-0404 */
  uchar		ec_level			HFC_VAL_ATTR_C;   /* ec level               */ /* FCLNX-0404 */
  uchar		bus_dev_func[3]     HFC_VAL_ATTR_C;   /* location(Bus Dev Func) */ /* FCLNX-0404 */
  uchar		port_no				HFC_VAL_ATTR_C;   /* port number            */ /* FCLNX-0404 */
  uchar		mlpf_mode			HFC_VAL_ATTR_C;	  /* MLPF mode              */ /* FCLNX-0454 */
  uchar		port_status			HFC_VAL_ATTR_C;	  /* Link Status	    	*/ /* FCLNX-0488 */
#define HFC_LINKUP				0x00				/* Link is in link-up status */
#define	HFC_LINKDOWN			0x02				/* Link is in link-down status */
#define HFC_WAITLINKUP			0x01				/*Waiting status change from LINK_Down to LINK_Up*/	/* FCLNX-0514 */
// #define HFC_HWISOLATE_E			0x84				/* Adapter is isolated */					/* FCLNX-GPL-147 */
// #define HFC_HWISOLATE_C			0x83				/* Forced adapter into isolated state */	/* FCLNX-GPL-147 */
#define HFC_ISOLATE_E			0x04				/* Adapter port is isolated */					/* FCLNX-GPL-147 */
#define HFC_ISOLATE_C			0x03				/* Forced adapter port into isolated state */	/* FCLNX-GPL-147 */
#define HFC_SFPFAIL				0x05				/* Transfer Error */	/* FCLNX-0514 */
#define HFC_SFPNOTSUPPORT		0x06				/* Not Supported */		/* FCLNX-0514 */
#define	HFC_SFPDOWN				0x07 				/* SFP Down */			/* FCLNX-0514 */
#define HFC_CHKSTP_E			0x08				/* Check Stop */		/* FCLNX-GPL-147 */
#define HFC_CHKSTP_C			0x09				/* Check Stop (C) */	/* FCLNX-GPL-147 */
#define HFC_UNKNOWN_STATUS		0xff				/* Unknown Status */	/* FCLNX-0514 */
  uchar		highconf_opt		HFC_VAL_ATTR_C;   /* Enable|Disable to Blockade Operation */ /* FCLNX-0488 */
  uchar		abort_t_restrain	HFC_VAL_ATTR_C;	  /* Restrain to AbortT.S */	/* FCLNX-0542 */  
  uchar		msi_flag			HFC_VAL_ATTR_C;	  /* Current interrupt type */ /* FCLNX-GPL-126 */
  uchar		npiv_mode			HFC_VAL_ATTR_C;   /* NPIV mode              */
  uchar		rid					HFC_VAL_ATTR_C;   /* rid	                */
  uchar		rsv7[2]				HFC_VAL_ATTR_C;   /*                        */ /* FCLNX-0542 */ /* FCLNX-GPL-126 */
  uchar		opt_vendor_name[32] HFC_VAL_ATTR_C;   /* Opt mod vendor name    */ /* FCLNX-0404 */
  uchar		opt_parts_number[32] HFC_VAL_ATTR_C;  /* Opt mod parts num    */ /* FCLNX-0404 */
  uchar		opt_serial_number[32]HFC_VAL_ATTR_C;  /* Opt mod serial num   */ /* FCLNX-0404 */
  uint64_t	scsi_exec_cnt_c[4]	HFC_VAL_ATTR;		/* SCSI cmd start count (core)	*/
  uint64_t	scsi_exec_cnt_p		HFC_VAL_ATTR;		/* SCSI cmd start count(port)	*/
  uint64_t	scsi_end_cnt_c[4]	HFC_VAL_ATTR;		/* SCSI cmd end count (core)	*/
  uint64_t	scsi_end_cnt_p		HFC_VAL_ATTR;		/* SCSI cmd end count (port)	*/
  uint64_t	enque_xob_cnt_c[4]	HFC_VAL_ATTR;		/* Enque_XOB issue count (core) */
  uint64_t	xrb_resp_cnt_c[4]	HFC_VAL_ATTR;		/* XRB_int count (core) */
  uint64_t	we_que_cnt_c[4]		HFC_VAL_ATTR;		/* wx_que_cnt + xob_wait_cnt (core) */
  uchar     ecid[64]            HFC_VAL_ATTR_C;
  uchar		core_status[4]		HFC_VAL_ATTR_C;   /* core_status  0x00:online, 0x04:chkstop */
  uchar     rsv8[92]			HFC_VAL_ATTR_C;   /*                        */ /* FCLNX-0404 */
  uchar     connection_type     HFC_VAL_ATTR_C;   /*                        */
#define HFC_CON_TYP_PtoPF 0x02                  /* Point to Point(fabric) */
#define HFC_CON_TYP_PtoP  0x03                  /* Point to Point         */
#define HFC_CON_TYP_FcAlF 0x04                  /* FC-AL(fabric)          */
#define HFC_CON_TYP_FcAl  0x05                  /* FC-AL                  */
#define HFC_CON_TYP_FPORT 0xf0                  /* F-Port                 */
#define HFC_CON_TYP_MULTIAL 0xf3                /* F-AL(Multi)            *//* FCLNX-GPL-FX-135 */
#define HFC_CON_TYP_NONE  0x00                  /* UnKnown                */
  uchar     core_num            HFC_VAL_ATTR_C;
  uchar     multiple_portid     HFC_VAL_ATTR_C;   /* 0:disable 1:enable     *//* FCLNX-GPL-FX-135 */
  uchar     rsv4[1]             HFC_VAL_ATTR_C;   /*                        */
  uint      max_data_rate       HFC_VAL_ATTR;     /* ex. 100->1Gbps         */
                                                  /*                        */
  uchar		login_delay			HFC_VAL_ATTR_C;   /* LOGIN Delay Time       */ /* FCLNX-0404 */
  uchar		spinup_delay		HFC_VAL_ATTR_C;   /* Spinup Delay           */ /* FCLNX-0404 */
  uchar		automap				HFC_VAL_ATTR_C;   /* Automap                */ /* FCLNX-0404 */
  uchar		pref_alpa			HFC_VAL_ATTR_C;   /* Preferred AL-PA Number */ /* FCLNX-0404 */
  uchar		rsv9[12]			HFC_VAL_ATTR_C;   /*                        */ /* FCLNX-0404 */
  ushort	xob_max				HFC_VAL_ATTR;     /* Max xob number         */ /* FCLNX-0404 */
  ushort	xrb_max				HFC_VAL_ATTR;     /* Max xrb number         */ /* FCLNX-0404 */
  ushort	slog_max			HFC_VAL_ATTR;     /* Max soft log number    */ /* FCLNX-0404 */
  uchar		rsv10[10]			HFC_VAL_ATTR_C;   /*                        */ /* FCLNX-0404 */
  uint		dma_max				HFC_VAL_ATTR;     /* Max DMA Size			  */ /* FCLNX-0404 */
  uint		queue_depth			HFC_VAL_ATTR;     /* Queue depth            */ /* FCLNX-0404 */
  uint		linkup_tmo			HFC_VAL_ATTR;     /* Link Down Time         */ /* FCLNX-0404 */
  uint		scsi_reset_delay	HFC_VAL_ATTR;     /* Reset Delay Time       */ /* FCLNX-0404 */
  uint		target_reset_tmo	HFC_VAL_ATTR;     /* Reset Timeout          */ /* FCLNX-0404 */
  uint		abort_tmo			HFC_VAL_ATTR;     /* Abort timeout          */ /* FCLNX-0404 */
  uint		max_target			HFC_VAL_ATTR;     /* Max target number      */ /* FCLNX-0404 */
  uint		trc_max				HFC_VAL_ATTR;     /* Max trace number       */ /* FCLNX-0404 */
  uchar		rsv11[12]			HFC_VAL_ATTR_C;   /*                        */ /* FCLNX-0404 */
  int		pm_pkt_num			HFC_VAL_ATTR;     /* Max pm_pkt number      */
  int		max_mck_cnt			HFC_VAL_ATTR;     /* MCK Retry Count        */ /* FCLNX-0404 */
  int		wmsg				HFC_VAL_ATTR;     /* Enable Message         */ /* FCLNX-0404 */
  int		linkup2_tmo			HFC_VAL_ATTR;     /* Link Down Time (MCK)   */ /* FCLNX-0404 */
  int		pkt_num				HFC_VAL_ATTR;     /* Max pkt number         */ /* FCLNX-0404 */
  int		can_queue			HFC_VAL_ATTR;     /* Max can queue          */ /* FCLNX-0404 */
  int		sg_tblsize			HFC_VAL_ATTR;     /* Max sg tablesize       */ /* FCLNX-0404 */
  int		cmnd_num			HFC_VAL_ATTR;     /* Max cmnd number        */ /* FCLNX-0404 */
  int		minus_tout			HFC_VAL_ATTR;     /* Minus timeout          */ /* FCLNX-0404 */
  int		scsi_allowed		HFC_VAL_ATTR;     /* scsi_cmnd->allowed     */ /* FCLNX-0404 */
  int		cmd_per_lun			HFC_VAL_ATTR;     /* Max cmd per lun        */ /* FCLNX-0404 */
  int		max_sectors			HFC_VAL_ATTR;     /* Max sectors            */ /* FCLNX-0404 */
  int		scsi_timeout_fail	HFC_VAL_ATTR;     /* scsi_timeout count     */ /* FCLNX-0404 */
  uint		tmt_delay			HFC_VAL_ATTR;								   /* FCLNX-0542 */
  uint		lun_reset_delay		HFC_VAL_ATTR;	  /* LUN Reset Delay Time	*/ /* FCLNX-0542 */ /* FCLNX-GPL-038 */
  uchar		target_reset_mode	HFC_VAL_ATTR_C;	  /* Target Reset Mode 		*/ /* FCLNX-0660 */
  char		limit_log			HFC_VAL_ATTR_C;	  /* Limit Log Mode 		*/ /* FCLNX-GPL-491 */
  char		filter_target		HFC_VAL_ATTR_C;	  /* Filtering Login Target */ /* FCLNX-GPL-491 */  
  char		hg_stats_disable	HFC_VAL_ATTR_C;	  /* Statistics for Virtage */ /* FCLNX-GPL-494 */   
  int		tgtrst_restrain     HFC_VAL_ATTR;
  int		hfc_pm_pkt_size		HFC_VAL_ATTR;
  uchar		max_vport_count     HFC_VAL_ATTR_C;
  uchar		mq_mode				HFC_VAL_ATTR_C;
  uchar		mq_num				HFC_VAL_ATTR_C;
  uchar     rsv5[273]           HFC_VAL_ATTR_C;
};

/************************************************/
/* IOCTL_HFC_SC_FC4STAT : for hbaapi            */
/************************************************/
struct hfc_ioctl_fc4stat {
  uchar     minor               HFC_VAL_ATTR_C;
  uchar     rsv                 HFC_VAL_ATTR_C;
  uchar     version             HFC_VAL_ATTR_C; /* structure vertion code */ /* FCLNX-0404 */
  uchar     flags               HFC_VAL_ATTR_C; /* control flags("0")     */ /* FCLNX-0404 */

  uchar     rsv0[4]             HFC_VAL_ATTR_C; /* reserved ("0")         */ /* FCLNX-0404 */
  uint64_t	inputrequests		HFC_VAL_ATTR;   /* SCSI cmd cnt(read)     */ /* FCLNX-0404 */
  uint64_t	outputrequests		HFC_VAL_ATTR;   /* SCSI cmd cnt(write)    */ /* FCLNX-0404 */
  uint64_t	controlrequests		HFC_VAL_ATTR;   /* SCSI cmd cnt(not move) */ /* FCLNX-0404 */
  uint64_t	inputmegabytes		HFC_VAL_ATTR;   /* num of Mbytes of read  */ /* FCLNX-0404 */
  uint64_t	outputmegabytes		HFC_VAL_ATTR;   /* num of Mbytes of write */ /* FCLNX-0404 */
};

/************************************************/
/* IOCTL_HFC_SC_APENABLE/IOCTL_HFC_SC_APDISABLE */
/* /IOCTL_HFC_SC_SCSISCAN            2006.06.09 */
/************************************************/
struct hfc_ioctl_adp_enable {
  uchar     minor               HFC_VAL_ATTR_C;
  uchar     rsv                 HFC_VAL_ATTR_C;
  uchar     version             HFC_VAL_ATTR_C;   /* structure vertion code */
  uchar     flags               HFC_VAL_ATTR_C;   /* control flags("0")     */

  uchar     rsv0[4]             HFC_VAL_ATTR_C;   /* reserved ("0")         */
};

/************************************************/
/* IOCTL_HFC_READ_APPARAM 						*/
/*									 2008.01.31 */
/************************************************/
struct hfc_ioctl_read_apparam {					/* FCLNX-0514 */
    uchar   minor               HFC_VAL_ATTR_C;
	uchar	rsv[127]			HFC_VAL_ATTR_C;	
};



/********************************************/
/* IOCTL_HFC_MP_TGTMAP						*/
/********************************************/
typedef struct mp_target_entry {
  uint64_t	nodename			HFC_VAL_ATTR;	/* target WWNN				*/
  uint64_t	portname			HFC_VAL_ATTR;	/* target WWPN				*/
  uchar		flags				HFC_VAL_ATTR_C;	/* target flags				*/
#define HFC_MP_WWN_VALID	0
#define HFC_MP_LINKUP		1
#define HFC_MP_TGT_GHOST	7					/* ghost target only FC-GW  */
  uchar		attribute			HFC_VAL_ATTR_C;	/* target path attribute	*/
  uchar		groupid				HFC_VAL_ATTR_C;	/* group id				  	*/
  uchar		targetid			HFC_VAL_ATTR_C;	/* target id			 	*/
  uchar		pathid				HFC_VAL_ATTR_C;	/* target path id			*/
  uchar		instance			HFC_VAL_ATTR_C;	/* instance					*/
  uchar		rsv[2]				HFC_VAL_ATTR_C;	/*						  	*/
} MP_TARGET_ENTRY;

typedef struct mp_target_header {
  uchar     minor               HFC_VAL_ATTR_C;
  uchar     rsv[7]              HFC_VAL_ATTR_C;
  uint		entry_count			HFC_VAL_ATTR;	/* entry count				*/
  uint		target_count		HFC_VAL_ATTR;	/* target count 			*/
} MP_TARGET_HEADER;

struct hfc_mp_target {
  MP_TARGET_HEADER	header[1];					/* target header			*/
  MP_TARGET_ENTRY	entry[1];					/* target entry				*/
};

/* FCLNX-GPL-147 */
/********************************************/
/* IOCTL_HFC_HBA_ISOLATION					*/	/* FCLNX_GPL-105 */
/********************************************/
typedef struct hfc_isol_info{  /* FCLNX-0454*/
  uchar     minor               HFC_VAL_ATTR_C;
  uchar     sub_cmd             HFC_VAL_ATTR_C;
  uchar		version 			HFC_VAL_ATTR_C; /* versioncode */
  uchar		set_opr 			HFC_VAL_ATTR_C; /* Operation No.						*/
#define HFC_READ_ISOLPARAM	0					/* show the error count and adapter status */
#define HFC_FORCE_ISOLATE	1					/* Isolate the adapter forcibly. 					 */
#define HFC_FORCE_CHKSTOP	2					/* Force Check Stop				 					 */
#define HFC_RECOV_ISOLATE	3					/* Recover of Isolate status 						 */
#define HFC_STOP_ISOLATE	4					/* Stop of hba Isolate facility 					 *//* FCLNX-GPL-349 */
#define HFC_START_ISOLATE	5					/* Sart of hba Isolate facility 					 *//* FCLNX-GPL-349 */
  uchar		isol_type			HFC_VAL_ATTR_C; /* Type to Isolate adapter or Isolate adapter port		*/
  uchar		resv1[11]			HFC_VAL_ATTR_C;	 /* FCLNX-0628 */
  short		ld_err_intvl		HFC_VAL_ATTR;	/* Monitaring time for Link-Down Error					*/
  short		ld_err_limit_l		HFC_VAL_ATTR;	/* Limit to Isolate for Long Link-Down Error	*/
  short		ld_err_count_l		HFC_VAL_ATTR;	/* Count to Isolate for Long Link-Down Error	*/
  short		ld_err_limit_s		HFC_VAL_ATTR;	/* Limit to Isolate for Short Link-Down Error	*/
  short		ld_err_count_s		HFC_VAL_ATTR;	/* Count to Isolate for Short Link-Down Error	*/
  short		if_err_intvl		HFC_VAL_ATTR;	/* Monitaring time for FC Interface Error				*/
  short		if_err_limit		HFC_VAL_ATTR;	/* Limit to Isolate for FC Interface Error		*/
  short   	if_err_count 		HFC_VAL_ATTR;   /* Count to Isolate for FC Interface Error	   */
  short		to_err_intvl		HFC_VAL_ATTR;	/* Monitaring time for SCSI Time-Out Error				*/
  short		to_err_limit		HFC_VAL_ATTR;	/* Limit to Isolate for SCSI Time-Out Error 	*/
  short		to_err_count		HFC_VAL_ATTR;	/* Count to Isolate for SCSI Time-Out Error 	*/
  short		rt_err_enable		HFC_VAL_ATTR;	 /* Limit to Isolate for Reset Error					 */
  short		rt_err_count		HFC_VAL_ATTR;	/* Count to Isolate for Reset Error 					*/
  uchar		resv2[35]			HFC_VAL_ATTR_C;	/* FCLNX-0628 */
  uchar		immdt_cmd			HFC_VAL_ATTR_C;	/* FCLNX-GPL-349 */
  uchar		force_recv			HFC_VAL_ATTR_C;	/* FCLNX-GPL-349 */
  uchar		err_is_func			HFC_VAL_ATTR_C;	/* FCLNX-GPL-349 */
#define HFC_ENABLE_ISOLATE			1			/* FCLNX-GPL-393 */
#define HFC_DISABLE_ISOLATE			0			/* FCLNX-GPL-393 */
  uchar		err_is_fw_spt		HFC_VAL_ATTR_C;	/* FCLNX-GPL-349 */
#define HFC_FW_ISOLATE_SUPPORT		1			/* FCLNX-GPL-393 */
#define HFC_FW_ISOLATE_NOT_SUPPORT	0			/* FCLNX-GPL-393 */
  uchar		err_is_hvm_spt		HFC_VAL_ATTR_C;	/* FCLNX-GPL-349 */
#define HFC_HVM_ISOLATE_SUPPORT		1			/* FCLNX-GPL-393 */
#define HFC_HVM_ISOLATE_NOT_SUPPORT	0			/* FCLNX-GPL-393 */
  short		init_retry			HFC_VAL_ATTR;	/* Numberof Retry for Link Initialize */
  short		login_retry 		HFC_VAL_ATTR;	/* Numberof Retry for Login and Pdisc */
  short		els_retry			HFC_VAL_ATTR;	/* Numberof Retry for ELS */		
  short		to_reset_retry		HFC_VAL_ATTR;	/* Number of Retry for LOGIN for After SCSI T.O */
  short		scsi_to_retry		HFC_VAL_ATTR;	/* Number of Retry for SCSI Command */		 
  uchar		total_abort_to		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014 */
  uchar		total_tgtrst_to		HFC_VAL_ATTR_C;	/* FCLNX-GPL-FX-014 */
  uchar		resv3[17]			HFC_VAL_ATTR_C;	/* FCLNX-0628 *//* FCLNX-GPL-FX-014 */
  uchar		errcode				HFC_VAL_ATTR_C;
#define HFC_ISOL_NORMAL			0				/* Normal End */
#define HFC_ISOL_NOTSUPPORT		1				/* Isolate Not Support */
#define HFC_ISOL_PARMARR		2				/* Parameter Error */
  short		retry_cnt[1024] 	HFC_VAL_ATTR;	/* Threshhold to Change Path	*/ /* HFC-PCM	*/
}HFC_ISOL_INFO;
/* FCLNX-GPL-147 */

/************************************************************************/
/* err_rec   errlog format                                              */
/************************************************************************/
#define MAX_FRAME_CNT		(ap->lparmode.frame_cnt)
#define MAX_MAX_FRAME_CNT	4
#define MAX_FX_FRAME_CNT	(pp->max_frame_count)

/* driver detail information 1 offset +00--3f */
typedef struct {
	uint				err_num;		/* +0  error num				*/
	uint				curr_sysrev;	/* +4  current sysrev			*/
	uint				mp_adap_status;	/* +8  mp_adap_info->mp_adap_status	*/
	uchar				mp_adap_lock;	/* +c  mp_adap_info->mp_adap_lock	*/
	uchar				mck_err_cnt;	/* +d  mp_adap_info->mck_err_cnt	*/
	uchar				resv1[2];		/* +e  reserve					*/
	uchar				resv2[4];		/* +10 reserve					*/
	uint				int_a_reg;		/* +14 INT_A_REG				*/
	ushort				msi_msg_ctl;	/* +18 MSI Message Control		*/
	uchar				max_link_width;			/* +1a max_link_width (cfg 7B-78h ,bit9-4)			*/ /* FCLNX-GPL-227 */
	uchar				negotiated_link_width;	/* +1b negotiated_link_width (cfg 7F-7Eh ,bit9-4)	*/ /* FCLNX-GPL-227 */
	ushort				msix_msg_ctl;	/* +1c MSI-X Message Control	*/
	uchar				vector_ctl0;	/* +1e Entry0 Vector Control	*/
	uchar				vector_ctl1;	/* +1f Entry1 Vector Control	*/
	uint				msg_addr0;		/* +20 Entry0 Msg Addr			*/
	uint				msg_data0;		/* +24 Entry0 Msg Data			*/
	uint				msg_addr1;		/* +28 Entry1 Msg Addr			*/
	uint				msg_data1;		/* +2c Entry1 Msg Data			*/
	uchar				drv_log[16];	/* +30 different format by errors	*/
}Typ_err_detail_1 ;
/* fw_init_table infomation offset +40--5f */
typedef struct {
	uchar				connect_type ;	/* +40 fw_init_p->connect_type	*/
	uchar				trans_rate ;	/* +41 fw_init_p->trans_rate	*/
	uchar				resv4 ;			/* +42 */
	uchar				flag ;			/* +43 fw_init_p->flag			*/
	uint				port_id ;		/* +44 fw_init_p->port_id		*/
	uint64_t 			port_name ;		/* +48 fw_init_p->port_name		*/
	uint				xob_inp ;		/* +50 */
	uint				xob_outp ;		/* +54 */
	uint				xrb_inp ;		/* +58 */
	uint				xrb_outp ;		/* +5c */
}Typ_err_init_tbl ;
/* mailbox infomation offset +60--8f */
typedef struct {
/* MB-Req */
	uchar				command;		/* +60 mb_req->command			*/
	uchar				sub_cmd;		/* +61 mb_req->sub_cmd			*/
	ushort				dependent_code;	/* +62 mb_req->dependent_code	*/
	uchar				pseq_no;		/* +64 mb_req->pseq_no			*/
	uchar				resv0[3];		/* +65							*/
	uchar				req_info[8];	/* +68 mb_req->....				*/
/* MB-Resp */
	uchar				flag ;			/* +70 mb_resp->flag			*/
	uchar				xcc ;			/* +71 mb_resp->xcc				*/
	uchar				r_esw ;			/* +72 mb_resp->esw				*/
	uchar				resv1 ;			/* +73	*/
	uchar				r_ssn ;			/* +74 mb_resp->ssn				*/
	uchar				r_son ;			/* +75 mb_resp->son				*/
	ushort				r_sbc ;			/* +76 mb_resp->sbc				*/
	uchar				r_fsb ;			/* +78 mb_resp->fsb				*/
	uchar				err_code[3] ;	/* +79 mb_resp->err_code		*/
	uchar				rsp_info[4];	/* +7c mb_resp->....			*/
/* MB-INT */
	uchar				int_code ;		/* +80 mb_int->int_code			*/
	uchar				sub_int_code ;  /* +81 mb_int->sub_int_code		*/
	uchar				i_esw ;			/* +82 mb_int->esw				*/
	uchar				resv2 ;			/* +83							*/
	ushort				detail ;		/* +84 mb_int->detail			*/
	ushort				resv3 ;			/* 								*/
	uchar				i_ssn ;			/* +88 mb_int->ssn				*/
	uchar				i_son ;			/* +89 mb_int->son				*/
	ushort				i_sbc ;			/* +8a mb_int->sbc				*/
	uchar				rfv ;			/* +8c mb_int->rfv				*/
	uchar				resv4 ;			/* +8d 							*/
	ushort				length ;		/* +8e mb_int->length			*/
} Typ_err_mb ;
/* xob infomation offset +90--bf */
typedef struct {				/* The infomation is fw_init_p->xob_inp */
	uchar				drv_used[8] ;	/* +90 xob->drv_work			*/
	uchar				flag ;			/* +98 xob->flag				*/
	uchar				skip ;			/* +99 xob->skip				*/
	uchar				bflag ;			/* +9a xob->bflag				*/
	uchar				cflag ;			/* +9b xob->cflag				*/
	uint				d_info ;		/* +9c xob->d_info				*/
	struct {							/* +a0 xob->fcp_cdb.fcp_lun		*/
//			uchar		rsv0 ;
			ushort		lun ;			/* FCLNX-GPL-0382 				*/
			uchar		sv2_7[6] ;
	}fcp_lun ;
	struct {							/* +a8 xob->fcp_cdb.fcp_cntl	*/
			uchar		rsv0;
			uchar		qtype;
			uchar		task_mgm ;
			uchar		data_type ;
	} fcp_cntl;
	uchar				fcp_cdb[16];	/* +ac xob->fcp_cdb.fcp_cdb		*/
	uint				length ;		/* +bc xob->fcp_cdb.fcp_dl		*/
} Typ_err_xob ;
/* xrb infomation offset +c0--df */
typedef struct {				/* The infomation is fw_init_p->xrb_outp*/
	uchar				fcp_status0 ;	/* +c0 xrb->fcp_status0			*/
	uchar				fcp_status1 ;	/* +c1 xrb->fcp_status1			*/
	uchar				fcp_status2 ;	/* +c2 xrb->fcp_status2			*/
	uchar				scsi_status ;	/* +c3 xrb->scsi_status			*/
	uint				resid ;			/* +c4 xrb->resid				*/
	uchar				resp_info[4];	/* +c8 xrb->resp_info(sns)		*/
	uchar				flag ;			/* +cc xrb->flag				*/
	uchar				xcc ;			/* +cd xrb->xcc					*/
	uchar				esw ;			/* +ce xrb->esw					*/
	uchar				skip ;			/* +cf xrb->skip				*/
	uchar				ssn ;			/* +d0 xrb->ssn					*/
	uchar				son ;			/* +d1 xrb->son					*/
	ushort				sbc ;			/* +d2 xrb->sbc					*/
	uchar				fsb ;			/* +d4 xrb->fsb					*/
	uchar				err_code[3] ;	/* +d5 xrb->err_code			*/
	uchar				drv_used[8] ;	/* +d8 xrb->drv_work			*/
}Typ_err_xrb ;
/* adapter infomation  offset +e0--11f */
typedef struct {
	uint				status ;			/* +e0			*/
	uint 				scsi_id ;			/* +e4			*/
	ushort				xob_no ;			/* +e8			*/
	ushort				xrb_no ;			/* +xea			*/
	uchar				xob_wait_exec_cnt ;	/* +xec			*/
	uchar				xob_exec_cnt ;		/* +xed			*/
	uchar				mb_status ;			/* +xee			*/
	uchar				open_status ;		/* +xef			*/
	uchar				dev_minor ;			/* +xf0			*/
	uchar				host_no ;			/* +xf1			*/
	uchar				instance ;			/* +xf2			*/
	uchar				rid ;				/* +xf3			*/
	ushort				mb_retry_cnt ;		/* +xf4 		*/
	ushort				cmnd_no ;			/* +xf6			*/
	ushort				target_cnt ;		/* +xf8			*/
	ushort				next_dstart_cnt ;	/* +xfa			*/
	uint				iov_map_cnt ;		/* +xfc			*/
	uint				curr_xob_outp ;		/* +x100		*/
	uint				frame_chkp ;		/* +x104		*/
	uint				frame_inp ;			/* +x108		*/
	uchar				rid_hg;				/* +x10c		*/	/* FCLNX-GPL-549 */
	uchar				mlpf_mode ;			/* +x10d		*/
	uchar				vector0 ;			/* +x10e		*/
	uchar				vector1 ;			/* +x10f		*/
	uint				trc_num ;			/* +x110		*/
	ushort				pkt_no ;			/* +x114		*/
	ushort				pkt_cnt ;			/* +x116		*/
	uint64_t			scsi_exec_cnt ;		/* +x118		*/
	uint				save_xob_outp[4] ;	/* +x120		*/
	uint				xob_outp_end[4] ;	/* +x130		*/
} Typ_err_ap ;
/* target_info  offset +140--15f*/
typedef struct {
	uint				status ;			/* +x140		*/
	uchar				flags ;				/* +x144		*/
	uchar				target_id ;			/* +x145		*/
	uchar				fc_class ;			/* +x146		*/
	uchar				pseq ;				/* +x147		*/
	uint				wx_que_cnt ;		/* +x148		*/
	uint				we_que_cnt ;		/* +x14c		*/
	uint64_t			scsi_id ;			/* +x150		*/
	uint64_t			ww_name ;			/* +x158		*/
} Typ_err_target ;
/* dev_info  offset +160--19f */
typedef struct {
	uchar				flags ;				/* +x160		*/
	uchar				status ;			/* +x161		*/
	uchar				target_id ;			/* +x162		*/
	uchar				lustat ;			/* +x163		*//* FCLNX-GPL-0382 */
//	uchar				resev0;				/* +x163		*//* FCLNX-GPL-0382 */
	uchar				owner_ctl ;			/* +x164		*/
	uchar				priority ;			/* +x165		*/
	uchar				group_id ;			/* +x166		*/
	uchar				path_id ;			/* +x167		*/
	uint				ioerror ;			/* +x168		*/
	uint				iocount ;			/* +x16c		*/
	uint				id_size ;			/* +x170		*/
	uint				io_status ;			/* +x174		*/
	uint				wx_que_cnt ;		/* +x178		*/
	uint				we_que_cnt ;		/* +x17c		*/
	ushort				lun;				/* +x180		*//* FCLNX-GPL-0382 */
	uchar				resv1[6] ;			/* +x182		*//* FCLNX-GPL-489 */
	uint64_t			org_ww_name;		/* +x188		*//* FCLNX-GPL-489 */
	uchar				resv2[16] ;			/* +x190		*//* FCLNX-GPL-489 */
} Typ_err_dev ;
/* driver detail information 2 offset +1a0--1ff */
typedef struct {
	union {
		struct {
			uchar		mb_initiate[16];	/* +x1a0		*/
			uchar		mb_response[32];	/* +x1b0		*/
			uchar		init_table[48];		/* +x1d0		*/
		} mb ;
		struct {
			uchar		receive_frame[96];	/* +x1a0		*/
		} mbint ;
		struct {
			uchar		cmnd[16] ;			/* +x1a0		*/
			uint		serial_number ;		/* +x1b0		*/
			uint 		retries ;			/* +x1b4		*/
			uint		allowed ;			/* +x1b8		*/
			ushort		rq_tmo_sec ;		/* +x1bc		*/
			ushort		req_tmo_sec ;		/* +x1be		*/
			uint		resid ;				/* +x1c0		*/
			uint		result ;			/* +x1c4		*/
			ushort		use_sg ;			/* +x1c8		*/
			uchar		cmd_len ;			/* +x1ca		*/
			uchar		tag ;				/* +x1cb		*/
			uchar		lustat ;			/* +x1cc		*/
			uchar		resv1 ;				/* +x1cd		*//* FCLNX-GPL-0382 */
			ushort		lun_id ;			/* +x1ce		*//* FCLNX-GPL-0382 */
			uint		cmd_flags ;			/* +x1d0		*/
			uint		adap_status ;		/* +x1d4		*/
			uint		seg_cnt ;			/* +x1d8		*/
			uint		data_size ;			/* +x1dc		*/
			uint		iov_no ;			/* +x1e0		*/
			uint		iov_cnt ;			/* +x1e4		*/
			uchar		cmd_xob ;			/* +x1e8		*/
			uchar		target_id ;			/* +x1e9		*/
//			uchar		lun_id ;			/* +x1ea		*//* FCLNX-GPL-0382 */
			uchar		resv2;				/* +x1ea		*//* FCLNX-GPL-0382 */
			uchar		group_id ;			/* +x1eb		*/
			uint		seg_info_count ;	/* +x1ec		*/
			struct {						/* +x1f0		*/
					uint	xob_segno ;
					uint	seg_len	;
			} seg_info[2] ;
		} scsi ;
		struct {
			uint		ws_func0;           /* +x1a0 		*/
			uint		ws_func1;           /* +x1a4 		*/
			uint		ws_func2;           /* +x1a8 		*/
			uint		ws_func3;           /* +x1ac 		*/
			uchar		resv1[80] ;			/* +x1b0		*/
		} mck;
	} uni ;
} Typ_err_detail_2 ;
/*----------------------------*/
/* errlog format1 length 1024 */
typedef struct {
	Typ_err_detail_1	err_detail_1 ;		/* +x0   error detail info1		*/
	Typ_err_init_tbl	err_init_tbl ;		/* +x40  init_tbl info			*/
	Typ_err_mb			err_mb ;			/* +x60  mailbox info			*/
	Typ_err_xob			err_xob ;			/* +x90  xob info				*/
	Typ_err_xrb			err_xrb ;			/* +xc0  xrb info				*/
	Typ_err_ap			err_ap ;			/* +xe0  Adapter info			*/
	Typ_err_target		err_target ;		/* +x140 Target info			*/
	Typ_err_dev			err_dev ;			/* +x160 Device info			*/
	Typ_err_detail_2	err_detail_2 ;		/* +x1a0 error detail info2		*/
	uchar				err_ddtrc1[15][32];	/* +x200 Driver trace info		*/
	uchar				err_ddtrc2[16][2];	/* +x3e0 Driver trace info		*/
} hfc_errfmt1_t ;
/* errlog free format length 1024 */
typedef struct {
	uint				err_num ;
	uchar				err_detail[1020] ;
} hfc_errfmt2_t ;

typedef struct{ /* used by "PCIe SRAM 1bit CE" , "F/W 1bit CE" */
		uint64_t	time_stamp;
		uint		ram_adr;
		uint		far;
} hfc_err1bit_t ;

/************************************************************************/
/* err_rec   errlog format for FIVE-FX                                  */
/************************************************************************/

/* driver detail information 1 offset +00--1f */
typedef struct {
	uint			err_num;			/* +0  error num				*/
	uint			curr_sysrev;		/* +4  current sysrev			*/
	uint64_t		org_ww_name;		/* +8  original ww_name			*/
	uchar			drv_log[16];		/* +10 different format by err	*/
}Typ_fx_err_detail_1 ;

/* fw_init_table infomation offset +20--2f */
typedef struct {
	uchar			connect_type;		/* +20 init_p->connect_type		*/
	uchar			trans_rate ;		/* +21 init_p->trans_rate		*/
	uchar			configure_flag;		/* +22 init_p->configure_flag	*/
	uchar			fabric_param;		/* +23 init_p->fabric_param		*/
	uchar			alpa_count;			/* +24 init_p->alpa_count		*/
	uchar			p2p_tgt_port_id[3];	/* +25 init_p->p2p_tgt_port_id	*/
	uchar			assign_alpa;		/* +28 init_p->assign_alpa		*/
	uchar			resv1[1];			/* +29 reserve					*/
	ushort			vf_id;				/* +2a init_p->vf_id			*/
	uchar			resv2[1];			/* +2c reserve					*/
	uchar			self_port_id[3];	/* +2d init_p->self_port_id		*/
}Typ_fx_err_init_tbl ;

/* xob infomation offset +30--5f */
typedef struct {
	uchar			flag;				/* +30 xob->flag				*/
	uchar			skip;				/* +31 xob->skip				*/
	ushort			seg_cnt;			/* +32 xob->seg_cnt				*/
	uchar			queue_no;			/* +34 xob->queue_no			*/
	uchar			trans_s_id[3];		/* +35 xob->trans_s_id			*/
	uint64_t		drv_work;			/* +38 xob->drv_work			*/
	uchar			fcp_cmd[32];		/* +40 FCP_CMND_IU				*/
} Typ_fx_err_xob ;

/* xrb infomation offset +60--7f */
typedef struct {				/* The infomation is fw_init_p->xrb_outp*/
	ushort			retry_delay;		/* +60 retry delay timer		*/
	uchar			fcp_status2;		/* +62 fcp_status2				*/
	uchar			scsi_status;		/* +63 scsi_status				*/
	uint			resid;				/* +64 resid					*/
	uchar			fcp_info[4];		/* +68 resp_info(sns)			*/
	uchar			esw;				/* +6c esw						*/
	uchar			softlog;			/* +6d softlog					*/
	ushort			sbc;				/* +6e sbc						*/
	uchar			fsb;				/* +70 fsb						*/
	uchar			err_code[3];		/* +71 err_code					*/
	uchar			valid;				/* +74 valid					*/
	uchar			trans_s_id[3];		/* +75 trans_s_id				*/
	uint64_t		drv_work;			/* +78 drv_work					*/
}Typ_fx_err_xrb ;

/* port infomation  offset +80--af */
typedef struct {
	uint			status;				/* +80 pp->status				*/
	uint			status_detail1;		/* +84 pp->status_detail1		*/
	uint			status_detail2;		/* +88 pp->status_detail2		*/
	uint 			issue_mailbox;		/* +8c pp->issue_mailbox		*/
	uchar			rid;				/* +90 pp->rid					*/
	uchar			vport_id;			/* +91 pp->vport_id				*/
	uchar			npiv_mode;			/* +92 pp->npiv_mode			*/
	uchar			resv1[1];			/* +93 reserve					*/
	uchar			rid_hg;				/* +94 pp->rid_hg				*/
	uchar			mlpf_mode;			/* +95 pp->mlpf_mode			*/
	uchar			dev_major;			/* +96 pp->dev_major			*/
	uchar			dev_minor;			/* +97 pp->dev_minor			*/
	uchar			host_no;			/* +98 pp->host_no				*/
	uchar			instance;			/* +99 pp->instance				*/
	uchar			master_core_no;		/* +9a pp->master_core_no 		*/
	uchar			available_pcore;	/* +9b pp->available_pcore		*/
	uchar			core_num;			/* +9c pp->core_num 			*/
	uchar			target_cnt;			/* +9d pp->target_cnt			*/
	uchar			resv2[2];			/* +9e reserve					*/
	uchar			open_status;		/* +a0 pp->open_status			*/
	uchar			frame_ctl;			/* +a1 pp->frame_ctl			*/
	uchar			fc_class;			/* +a2 pp->fc_class				*/
	uchar			flogi_param;		/* +a3 pp->flogi_param			*/
	uchar			plogi_param;		/* +a4 pp->plogi_param			*/
	uchar			switch_exist;		/* +a5 pp->switch_exist			*/
	uchar			mailbox_pseq;		/* +a6 pp->mailbox_pseq			*/
	uchar			resv3[1];			/* +a7 reserve					*/
	ushort			seq_no;				/* +a8 pp->seq_no				*/
	ushort			pkt_no;				/* +aa pp->pkt_no				*/
	ushort			trc_num;			/* +ac pp->trc_num				*/
	ushort			resv4[1];			/* +ae reserve					*/
} Typ_fx_err_pp ;

/* core infomation  offset +b0--df */
typedef struct {
	uint			status;				/* +b0 core->status				*/
	uchar			core_no;			/* +b4 core->core_no			*/
	uchar			pcore_no;			/* +b5 core->pcore_no			*/
	uchar			rid;				/* +b6 rp->rid					*/
	uchar			mb_status;			/* +b7 core->mb_status			*/
	uint			mb_resp;			/* +b8 core->mb_resp			*/
	uint			mb_results;			/* +bc core->mb_results			*/
	uchar			mb_retry_cnt;		/* +c0 core->mb_retry_cnt		*/
	uchar			mb_retry_tid;		/* +c1 core->mb_retry_tid		*/
	uchar			drv_next_xob;		/* +c2 core->drv_next_xob		*/
	uchar			drv_next_xrb;		/* +c3 core->drv_next_xrb		*/
	ushort 			wx_que_cnt_all;		/* +c4 core->wx_que_cnt_all		*/
	ushort			we_que_cnt_all;		/* +c6 core->we_que_cnt_all		*/
	uint			we_que_sizecnt;		/* +c8 core->we_que_sizecnt		*/
	uchar			next_dstart_cnt;	/* +cc core->next_dstart_cnt	*/
	uchar			frame_inp;			/* +cd core->frame_inp			*/
	uchar			mck_err_cnt;		/* +ce core->mck_err_cnt		*/
	uchar			resv1[1];			/* +cf reserve					*/
	uint			scsi_exec_cnt;		/* +d0 core->scsi_exec_cnt		*/
	uint			scsi_end_cnt;		/* +d4 core->scsi_end_cnt		*/
	ushort			iov_no;				/* +d8 core->iov_no				*/
	ushort			trc_num;			/* +da core->trc_num			*/
	ushort			scsi_err_cnt;		/* +dc core->scsi_err_cnt		*/
	ushort			mb_retry_tout;		/* +de core->mb_retry_tout		*/
} Typ_fx_err_core ;

/* target_info_fx  offset +e0--ff */
typedef struct {
	uint			status;				/* +e0 target->status			*/
	uchar			flags;				/* +e4 target->flags			*/
	uchar			target_id;			/* +e5 target->target_id		*/
	uchar			fc_class;			/* +e6 target->fc_class			*/
	uchar			pseq;				/* +e7 target->pseq				*/
	uchar			wx_que_cnt[4];		/* +e8 target->wx_que_cnt		*/
	uchar			we_que_cnt[4];		/* +ec target->we_que_cnt		*/
	uint64_t		scsi_id ;			/* +f0 target->scsi_id			*/
	uint64_t		ww_name ;			/* +f8 target->ww_name			*/
} Typ_fx_err_target ;

/* dev_info_fx  offset +100--11f */
typedef struct {
	uchar			flags;				/* +100	dev->flags				*/
	uchar			lustat;				/* +101	dev->lustat				*/
	uchar			lun[2];				/* +102	dev->lun				*/
	uchar			curr_core;			/* +104	dev->curr_core			*/
	uchar			curr_cmd_type;		/* +105	dev->curr_cmd_type		*/
	uchar			target_id;			/* +106	dev->target_id			*/
	uchar			resv1[1];			/* +107 reserve					*/

	/* for HFC-PCM */
	uchar			status;				/* +108	dev->status				*/
	uchar			owner_ctl;			/* +109	dev->owner_ctl			*/
	uchar			priority;			/* +10a	dev->priority			*/
	uchar			group_id;			/* +10b	dev->group_id			*/
	uchar			path_id;			/* +10c	dev->path_id			*/
	uchar			resv2[3];			/* +10d reserve					*/
	uint			ioerror;			/* +110	dev->ioerror			*/
	uint			iocount;			/* +114	dev->iocount			*/
	uint			id_size;			/* +118	dev->id_size			*/
	uint			io_status;			/* +11c	dev->io_status			*/
} Typ_fx_err_dev ;


/* driver detail information 2 offset +120--17f */
typedef struct {
	union {
		struct {
			uchar		mb_initiate[64];		/* +120			*/
			uchar		send_payload[32];		/* +160			*/
		} mbinit ;
		struct {
			uchar		mb_initiate[16];		/* +120			*/
			uchar		mb_response[48];		/* +130			*/
			uchar		send_payload[16];		/* +160			*/
			uchar		receive_payload[16];	/* +170			*/
		} mbresp ;
		struct {
			uchar		mb_intreq[64];			/* +120			*/
			uchar		rcvfrm_payload[32];		/* +160			*/
		} mbint ;
		struct {
			uchar		cmnd[16];				/* +120			*/
			uint		serial_number;			/* +130			*/
			uint 		retries;				/* +134			*/
			uint		allowed;				/* +138			*/
			uint		rq_tmo_sec;				/* +13c			*/
			uint		req_tmo_sec;			/* +140			*/
			uint		result;					/* +144			*/
			uchar		resv2[2];				/* +148			*/
			uchar		cmd_len;				/* +14a			*/
			uchar		tag;					/* +14b			*/
			uchar		resv3[1];				/* +14c			*/
			uchar		core_no;				/* +14d			*/
			uchar		lun_id[2];				/* +14e			*/
			uint		cmd_flags;				/* +150			*/
			uint		adap_status;			/* +154			*/
			uint		seg_cnt;				/* +158			*/
			uint		data_size;				/* +15c			*/
			uint		iov_no;					/* +160			*/
			uint		iov_cnt;				/* +164			*/
			uchar		cmd_xob;				/* +168			*/
			uchar		target_id;				/* +169			*/
			uchar		resv4[1];				/* +16a			*/
			uchar		group_id;				/* +16b			*/
			uint		seg_info_count;			/* +16c			*/
			struct {							/* +170			*/
					uint	xob_segno;
			} seg_info[4] ;
		} scsi ;
		struct {
			uint		ws_func0;				/* +120 		*/
			uint		ws_func1;           	/* +124 		*/
			uint		ws_func2;           	/* +128 		*/
			uint		ws_func3;				/* +12c 		*/
			uchar		resv1[80];				/* +130			*/
		} mck;
	} uni ;
} Typ_fx_err_detail_2 ;

/*----------------------------*/
/* errlog format1 length 1024 */
/*----------------------------*/
typedef struct {
	Typ_fx_err_detail_1		err_detail_1;			/* +000  error detail info1		*/
	Typ_fx_err_init_tbl		err_init_tbl;			/* +020  init_tbl info			*/
	Typ_fx_err_xob			err_xob;				/* +030  xob info				*/
	Typ_fx_err_xrb			err_xrb;				/* +060  xrb info				*/
	Typ_fx_err_pp			err_pp;					/* +080  port_info				*/
	Typ_fx_err_core			err_core;				/* +0b0  core_info				*/
	Typ_fx_err_target		err_target;				/* +0e0	 target_info_fx			*/
	Typ_fx_err_dev			err_dev;				/* +100	 dev_info_fx			*/
	Typ_fx_err_detail_2		err_detail_2 ;			/* +120	 error detail info2		*/
	uchar					err_coretrc1[7][32];	/* +180	 Driver core trace info	*/
	uchar					err_coretrc2[16][2];	/* +260  Driver core trace info	*/
	uchar					err_ddtrc1[11][32];		/* +280	 Driver port trace info	*/
	uchar					err_ddtrc2[16][2];		/* +3e0  Driver port trace info	*/
} hfc_fx_errfmt1_t ;

/*----------------------------*//* FCLNX-GPL-FX-139 */
/* errlog format3 length 1024 */
/*----------------------------*/
typedef struct {
	Typ_fx_err_detail_1		err_detail_1;			/* +000  error detail info1		*/
	Typ_fx_err_init_tbl		err_init_tbl;			/* +020  init_tbl info			*/
	Typ_fx_err_xob			err_xob;				/* +030  xob info				*/
	Typ_fx_err_xrb			err_xrb;				/* +060  xrb info				*/
	Typ_fx_err_pp			err_pp;					/* +080  port_info				*/
	uchar					err_mb_trc[53][16];		/* +0b0  mb_trace				*/
} hfc_fx_errfmt3_t ;			/* FCLNX-GPL-FX-139 */

/*----------------------------*//* FCLNX-GPL-FX-139 */
/* errlog format4 length 1024 */
/*----------------------------*/
typedef struct {
	Typ_fx_err_detail_1		err_detail_1;			/* +000  error detail info1		*/
	Typ_fx_err_init_tbl		err_init_tbl;			/* +020  init_tbl info			*/
	Typ_fx_err_xob			err_xob;				/* +030  xob info				*/
	Typ_fx_err_xrb			err_xrb;				/* +060  xrb info				*/
	Typ_fx_err_pp			err_pp;					/* +080  port_info				*/
	Typ_fx_err_core			err_core;				/* +0b0  core_info				*/
	Typ_fx_err_target		err_target;				/* +0e0	 target_info_fx			*/
	uchar					err_unfnd_tgtlist[32];	/* +100  list of unfound targets *//* FCLNX-GPL-FX-139 */
	Typ_fx_err_detail_2		err_detail_2 ;			/* +120	 error detail info2		*/
	uchar					err_coretrc1[7][32];	/* +180	 Driver core trace info	*/
	uchar					err_coretrc2[16][2];	/* +260  Driver core trace info	*/
	uchar					err_ddtrc1[11][32];		/* +280	 Driver port trace info	*/
	uchar					err_ddtrc2[16][2];		/* +3e0  Driver port trace info	*/
} hfc_fx_errfmt4_t ;			/* FCLNX-GPL-FX-139 */

/*--------------------------------*/
/* errlog free format length 1024 */
/*--------------------------------*/
typedef struct {
	uint				err_num ;
	uchar				err_detail[1020] ;
} hfc_fx_errfmt2_t ;

typedef struct{ /* used by "PCIe SRAM 1bit CE" , "F/W 1bit CE" */
		uint64_t	time_stamp;
		uint		ram_adr;
		uint		far;
} hfc_fx_err1bit_t ;


/* 1.92 */
typedef struct {
#ifdef __WIN
    uint64_t            performance_counter;
    uint64_t            performance_frequency;
    uint64_t            current_time;
    uchar               resv[40];
#define HFC_TRC_HEAD_CNT          24
#define HFC_TRC_ID_CNT            64
#else
    uchar               resv[64];
#define HFC_TRC_HEAD_CNT          24
#define HFC_TRC_ID_CNT            64
#endif
    /* trace */
    uchar               trace_head[HFC_TRC_HEAD_CNT][32] ;
    uchar               trace_id[HFC_TRC_ID_CNT][2] ;
    /* struct end                           offset 0x0400 */
}Typ_dderrlog ;

/* errlog (in ioctl) */
#define HFC_MAX_ERRLOG_CAP  1024
#define HFC_MAX_ERRLOG_CNT  16
struct hfc_ioctl_errlog {
    uchar               logmap[2];
    uchar               resv[14];

    struct {
        uchar           logdata[HFC_MAX_ERRLOG_CAP];
    }num[HFC_MAX_ERRLOG_CNT];
};

struct hfc_hba_info {
    struct {
        uchar topology;
#define HFC_TPG_P2P     1       /* Point to Point */
#define HFC_TPG_P2P_SW  2       /* Point to Point (Fublic) */
#define HFC_TPG_FCAL    3       /* FC-AL */
#define HFC_TPG_FCAL_SW 4       /* FC-AL (Fublic) */

        uchar linkspeed;        /* 1/2/4/10 Gbps */
        uchar rsv1[2];
        uint  rsv[15];
    }curr_value;

    struct {
        uchar topology;
        uchar linkspeed;
        uchar pref_alpa;
        uchar enable_tgtrst;
        uchar rsv1[12];
        uint  linkup_tmo;
        uint  scsi_reset_delay;
        uint  max_mck_err_cnt;
        uint  rsv2[1];
        uint  rsv3[8];
    }set;
};
/*-- Diag Sub Command ---*/
struct diag_cntl {
    uchar   sub_command HFC_VAL_ATTR_C;	/*-- see struct diag_ioctl ---*/
    uchar   support_pk  HFC_VAL_ATTR_C;	/*-- see struct diag_ioctl ---*/
#define HFC_DIAG_FPP            0x01
#define HFC_DIAG_FIVE           0x02
#define HFC_DIAG_FIVE_EX        0x04
#define HFC_DIAG_FIVE_FX        0x05
    uint    flags_fpp 	HFC_VAL_ATTR;
    uint    flags_five  HFC_VAL_ATTR; /* We use this flag for FIVE and FIVE-EX. */
#define HFC_DIAG_NORMAL_MODE    0x80000000      /* Normal       */
#define HFC_DIAG_DIAG_MODE      0x40000000      /* DIAG         */
#define HFC_DIAG_LOOP_MODE      0x20000000      /* LOOP         */
#define HFC_DIAG_IOS_MODE       0x10000000      /* IOS          */
#define HFC_DIAG_ARG_RESP       0x00008000      /* diag_ioctl resp */
    int     (*cmd_func)(void *, struct diag_ioctl*) ;
};
typedef struct diag_cntl Typ_diag_cntl ;

#endif /* _H_HFCDDIOC */
