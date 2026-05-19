/*
 * hfcldd.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcldd.h,v 1.65.2.24.2.36.2.4.2.2.6.24.2.2.2.21.2.16.2.10.2.12.2.6.2.9 2015/12/24 21:14:09 toyo Exp $
 */

#ifndef _H_HFCLDD
#define _H_HFCLDD

#if !defined(LINUX_VERSION_CODE)
#include <linux/version.h>
#endif  /* LINUX_VERSION_CODE not defined */

#ifdef __KERNEL__

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include <linux/config.h>
#endif
#include <linux/module.h>

#include <asm/bitops.h>
#include <asm/byteorder.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
#include <asm/system.h>
#endif
#include <asm/uaccess.h>

#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/smp.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
//#include <linux/smp_lock.h>
#else
#include <linux/smp_lock.h>
#endif

#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/slab.h>

#include <linux/blkdev.h>
#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_tcq.h>

#include <scsi/scsi_transport_fc.h>

#include <linux/miscdevice.h>

// #include <linux/config.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
// #include <linux/module.h>
#include <linux/timex.h>
#include <linux/unistd.h>
#include <linux/utsname.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/types.h>

#endif

#ifndef __KERNEL__
struct timer_list {
	unsigned char aaa[64] ;
};
struct scsi_cmd {
	unsigned char aaa[480] ;
};
struct scsi_cmnd {
	unsigned char aaa ;
};
struct scsi_device {
	unsigned char aaa ;
};
struct semaphore {
	unsigned char aaa[32] ;
};
struct fc_host_statistics { /* FCLNX-GPL-176 */
	unsigned char aaa[160]; /* FCLNX-GPL-229 */
};
typedef unsigned long long dma_addr_t ;
typedef unsigned long long wait_queue_head_t ;
typedef unsigned int	uint32_t ;
typedef unsigned int	spinlock_t;
typedef unsigned int	atomic_t;
typedef unsigned int	irqreturn_t;

/* FCLNX-GPL-124 */
/* kernel 4.8+: pci_enable_msix removed; use pci_alloc_irq_vectors.
 * We retain a local hfc_msix_entry struct for vector bookkeeping. */
struct hfc_msix_entry {
	ushort  vector; /* assigned IRQ number */
	ushort  entry;  /* logical entry index */
};


#endif

#ifdef __KERNEL__

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define scsi_to_pci_dma_dir(scsi_dir) scsi_dir
#define SCSI_DATA_UNKNOWN	DMA_BIRECTIONAL
#define SCSI_DATA_WRITE		DMA_TO_DEVICE
#define SCSI_DATA_READ		DMA_FROM_DEVICE
#define SCSI_DATANOE		DMA_NONE

#define HOST_LOCK ap->hosts->host_lock

#define FX_HOST_LOCK pp->hosts->host_lock

#define CMND_LUN(cmnd)		cmnd->device->lun
#define CMND_CHANNEL(cmnd)	cmnd->device->channel
#define CMND_HOST(cmnd)		cmnd->device->host
#define CMND_HOSTDATA(cmnd)	cmnd->device->host->hostdata
#define CMND_HOST_NO(cmnd)	cmnd->device->host->host_no
#define CMND_TARGET(cmnd)	cmnd->device->id
#define CMND_DEV(cmnd)		cmnd->device->hostdata		/* FCLNX-GPL-0343 */

#define HFC_MB_EVENT_WAIT	1
#define HFC_IOCTL_EVENT_WAIT	2
#define HFC_MCK_EVENT_WAIT	3
#define HFC_LOOP_EVENT_WAIT	4
#define HFC_MB_LOCK_EVENT_WAIT	5

#define hfc_sleep_on(x,y)				_hfc_sleep_on(x,(atomic_t *)y)
#define hfc_wake_up(x,y)				_hfc_wake_up(x,(atomic_t *)y)

#endif



/************************************************************************/
/* include file                                                         */
/************************************************************************/

#include "hfclddioc.h" /* hfcddioc.h ->hfclddioc.h */
#include "hfclddcom.h" /* hfcddcom.h ->hfclddcom.h */
/**************************************************************************************************/
/* Macro																						  */
/**************************************************************************************************/
#define HFC_MEMCPY( _DST1, _SRC1, _SIZE_T ) ( memcpy( _DST1, _SRC1, _SIZE_T ) )
#define HFC_BZERO( _DST1,  _SIZE_T ) ( memset( _DST1, 0, _SIZE_T ) )

#define HFC_SEMAPHORE_LOCK(sem)   down_interruptible(&sem)
#define HFC_SEMAPHORE_UNLOCK(sem) up(&sem)

#define HFC_ADAP_LOCK(mpap,b)	{					\
	while ( test_and_set_bit(b, (ulong *)&mpap->tbl_lock) ) {	\
			schedule_timeout(HZ/1000+1);			\
	}								\
}						

#define HFC_ADAP_UNLOCK(mpap,b)	clear_bit(b, (ulong *)&mpap->tbl_lock)

#define HFC_MAILBOX_UNLOCK(ap,b)	clear_bit(b, (ulong *)&ap->mb_lock)

#define HFC_ADAPLOCK_IRQSAVE(FLAGS) {											\
		if (ap->manage_info->lg_target_info == NULL)							\
			spin_lock_irqsave(&ap->adap_lock, (FLAGS));							\
		else																	\
			spin_lock_irqsave(&ap->manage_info->hfcmp_lock, (FLAGS));			\
}

#define HFC_ADAPUNLOCK_IRQRESTORE(FLAGS) {										\
		if (ap->manage_info->lg_target_info == NULL)							\
			spin_unlock_irqrestore(&ap->adap_lock, (FLAGS));					\
		else																	\
			spin_unlock_irqrestore(&ap->manage_info->hfcmp_lock, (FLAGS));		\
}

#define HFC_NON_STATUS			0			/* HBA is not in process in the system 		*/
/****************************************************************************************************/
/* For Scsi_Host_Templatemessage print																*/
/****************************************************************************************************/

#ifdef HFC_EM64T_UP
#define HFC_DEFAULT_CAN_QUEUE	 32			/*	Boot driver for AS4 EM64T UP */
#define HFC_PKT_NUM              64
#else
#define HFC_DEFAULT_CAN_QUEUE	1536		/*	variable by hfcldd.conf */	/* FCLNX-0554 *//* FCLNX-GPL-0343 */
#define HFC_PKT_NUM             2048		/* FCLNX-0554 *//* FCLNX-GPL-0343 */
#endif
#define HFC_RST_PKT_NUM			512

#define HFC_PKT_POOL_SIZE		128
#define HFC_PKT_POOL_NUM		((HFC_PKT_NUM % HFC_PKT_POOL_SIZE) ? \
									((HFC_PKT_NUM / HFC_PKT_POOL_SIZE)+1) : \
										(HFC_PKT_NUM / HFC_PKT_POOL_SIZE))

#define HFC_THIS_ID				-1
#define HFC_MAX_THIS_ID			255		/* FCLNX-0651 */
#define HFC_SG_TABLESIZE		32
#define HFC_MAX_SG_TABLESIZE		256	
#define HFC_CMD_PER_LUN			1
#define HFC_MAX_CMD_PER_LUN		32
#define HFC_HIGHMEM_IO			1
#define HFC_VARY_IO				1
#define HFC_MAX_SECTORS			0
#define HFC_MAX_MAX_SECTORS		0xffff

#define HFC_PCI_DEVICE_ID_3009		0x3009
#define HFC_PCI_DEVICE_ID_300A		0x300A
#define HFC_PCI_DEVICE_ID_300B		0x300B
#define HFC_PCI_DEVICE_ID_300C		0x300C
#define HFC_PCI_DEVICE_ID_300D		0x300D
#define HFC_PCI_DEVICE_ID_3020		0x3020 /* FIVE-EX */
#define HFC_PCI_DEVICE_ID_3070		0x3070 /* FIVE-FX */

#define HFC_PCI_VENDOR_ID	0x1054		/* Hitachi vendor ID */
#define HFC_PCI_DEVICE_NO	5			/* Max adapter variation number */			
#define HFC_IOBASE_LEN      0x1000		/* 4kB */

#define HFC_CMND_NUM		256		/* dummy scsi_cmnd num 30 -> 256 *//* FCLNX-GPL-0343 */
#define HFC_MINUS_TIMOUT	2
#define HFC_SCSI_ALLOWED    	5		/* retry 4 times */

#define HFC_MAX_TRANSFER        16

#define HFC_MAX_INSTANCE_CNT	256		/* FCLNX-0468 64 -> 256 */
#define HFC_SCATTERLIST_NUM		32		/* Number of scatterlist of ioctl scsi_cmnd *//* FCLNX-GPL-0343 */
/**************************************************************************************************/
/* For message print																			  */
/**************************************************************************************************/
#ifdef __KERNEL__

#ifdef _HFC_DEBUG	/* FCLNX-0309 */
#ifdef _HFC_MSGLVL0
#define HFC_MESSAGE_LEVEL KERN_EMERG	/* 0 : system is unusable   */
#endif
#ifdef	_HFC_MSGLVL1
#define HFC_MESSAGE_LEVEL KERN_ALERT	/* 1 : action must be taken immediately */
#endif
#ifdef	_HFC_MSGLVL2
#define HFC_MESSAGE_LEVEL  KERN_CRIT	/* 2 : critical conditions */
#endif
#ifdef 	_HFC_MSGLVL3
#define HFC_MESSAGE_LEVEL KERN_ERR		/* 3 : error conditions */
#endif
#ifdef	_HFC_MSGLVL4
#define HFC_MESSAGE_LEVEL KERN_WARNING	/* 4 : warning conditions */
#endif
#ifdef	_HFC_MSGLVL5
#define HFC_MESSAGE_LEVEL KERN_NOTICE	/* 5 : normal but significant condition */
#endif
#ifdef	_HFC_MSGLVL6
#define HFC_MESSAGE_LEVEL KERN_INFO		/* 6 : informational */
#endif
#ifdef	_HFC_MSGLVL7
#define HFC_MESSAGE_LEVEL KERN_DEBUG	/* 7 : debug-level messages */
#endif
#ifndef HFC_MESSAGE_LEVEL
#define HFC_MESSAGE_LEVEL KERN_DEBUG	/* 7 : No particular specification makes the message output debug-level */
#endif
#endif

#ifdef _HFC_DEBUG
#define HFC_ENTRY(x)				printk(HFC_MESSAGE_LEVEL "*hfcldd : entry %s()\n", x)
#define HFC_EXIT(x)					printk(HFC_MESSAGE_LEVEL "@hfcldd : exit  %s()\n", x)
#define HFC_DBGPRT(format, args...)	printk(HFC_MESSAGE_LEVEL format, ## args)
#define HFC_INFPRT(format, args...)	printk(HFC_MESSAGE_LEVEL format, ## args)
#define HFC_WRNPRT(format, args...)	printk(HFC_MESSAGE_LEVEL format, ## args)
#define HFC_ERRPRT(format, args...)	printk(HFC_MESSAGE_LEVEL format, ## args)
#define HFC_DEFPRT(format, args...)	printk(KERN_ERR          format, ## args)

#else
#define HFC_ENTRY(x)				
#define HFC_EXIT(x)					
#define HFC_DBGPRT(format, args...)
#define HFC_DEFPRT(format, args...)	printk(KERN_ERR     format, ## args)
#define HFC_INFPRT(format, args...)	printk(KERN_INFO    format, ## args)
#define HFC_WRNPRT(format, args...)	printk(KERN_WARNING format, ## args)
#define HFC_ERRPRT(format, args...)	printk(KERN_ERR     format, ## args)
#endif


/* hfcl_handler.c */
#define _HFC_DEBUG_HAND_00 0
#define _HFC_DEBUG_HAND_01 0
#define _HFC_DEBUG_HAND_02 0
#define _HFC_DEBUG_HAND_03 0
#define _HFC_DEBUG_HAND_04 0
#define _HFC_DEBUG_HAND_05 0
#define _HFC_DEBUG_HAND_06 0

/* hfcl_top.c */
#define _HFC_DEBUG_TOP_00   0

/* hfcl_detect.c */
#define _HFC_DEBUG_DET_00   0

/* hfcl_timer_recovery.c */
#define _HFC_DEBUG_TIMER_00 0

/* hfcl_strategy.c */
#define _HFC_DEBUG_STRA_00  0
#define _HFC_DEBUG_STRA_01  0
#define _HFC_DEBUG_STRA_02  0
#define _HFC_DEBUG_STRA_03  0
#define _HFC_DEBUG_STRA_04  0
#define _HFC_DEBUG_STRA_05  0
#define _HFC_DEBUG_STRA_06  0
#define _HFC_DEBUG_STRA_07  0
#define _HFC_DEBUG_STRA_08  0
#define _HFC_DEBUG_STRA_09  0
#define _HFC_DEBUG_STRA_10  0
#define _HFC_DEBUG_STRA_11  0

/* hfcl_mlpf.c */
#define _HFC_DEBUG_MLPF_00  0

/* Error Injection */
#define _HFC_ERROR_INJ      0   /* _HFC_INTO_IOERROR */
#define _HFC_ERROR_INJ_00   0   /* _HFC_INH_MCK_RECOVERY */

#endif
/**************************************************************************************************/
/* literal																						  */
/**************************************************************************************************/
/*--- Time-out setting value ---*/
#define	HFC_LINKINT_TO			120		/* Link initialize watch time   */
#define	HFC_ELS_TO				20		/* start(login),pdisc,nmsrv,qwwn*/
										/* iolpayld watch time			*/
#define	HFC_MB_TO				5		/* Mailbox start watch time other than the*/
										/* above-mentioned such as loop mode/trace*/
#define	HFC_MB_PROC_TO			20		/* Mailbox Process level */ /* FCLNX-GPL-243 */
#define	HFC_MB_DIAG_TO			20		/* Mailbox			  FCLNX-320 */
										/* start watch time of Diag		*/
#define	HFC_SCSI_CMD_TO			5		/* scsi command default watch time	*/
#define	HFC_TARGET_RST_TO		20		/* Target_Reset watch time		*/
#define	HFC_ABORT_ACA_TO		8		/* Abort_T-Set/Clear_ACA watch time*/
#define	HFC_DELAY_TO			7		/* following Delay time of Reset*/
#define	HFC_MCK_RECOVERY_TO		120		/* MCK_Recovery Time Out		*/
#define	HFC_MCKINT_TO			15		/* MCK Interrupt Time Out		*//*FCLNX-287*/
#define	HFC_REBOOT_DELAY_TO		3		/* MCK recovery waiting time of */
#define	HFC_MCK_DELAY_TO		3		/* another port from REBOOT  	*/
										/* instruction to FW_START		*/
#define	HFC_LOOP_TEST_TO		20		/* LOOP TEST TIME OUT			*/
#define	HFC_WEXEC_TO			3		/* Start waiting cue watch time	*/

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#define	HFC_LINKUP_TO			0		/* LinkUp waiting time			*/
#else
#define	HFC_LINKUP_TO			15		/* LinkUp waiting time			*/
#endif
#define	HFC_PCM_LINKUP_TO		15		/* LinkUp waiting time for PCM	*/
#define HFC_LINKUP2_TO			50		/* LINKUP waiting time after MCK recovers*//*FCLNX-241*/
#define HFC_WAIT_MPAPLOCK_TO	1		/* MP_ADAP_INFO lock waiting time*/
#define HFC_FW_ISOL_TO			6		/* firmware isolation waiting time*/

#define HFC_ERRLOG_LEN			1024	/* Error Log length				*/
#define HFC_RASLOG_LEN			896		/* RASLOG Detail data length	*/

#define HFC_DEFAULT_TARGET_RESET 	1	/* default target reset			*//* FCLNX-GPL-0343 */
#define HFC_DEFAULT_QUEUE_DEPTH		32	/* default queue depth			*/
#define HFC_MAX_QUEUE_DEPTH		256	 /* max queue depth                 */

#define HFC_INT_TYPE_INTX			0 /* INT type is INTx */
#define HFC_INT_TYPE_MSI			1 /* INT type is MSI */
#define HFC_INT_TYPE_MSIX			2 /* INT type is MSI-X */
#define HFC_INT_TYPE_MSIX_MULTI		3 /* INT type is MSI-X Multi-Queue (RSS) */
#define HFC_INT_TYPE_MSI_SHORTAGE	4 /* INT type is MSI Vector Shortage */
#define HFC_INT_TYPE_MSIX_SHORTAGE	5 /* INT type is MSI-X Shared */

#define HFC_MAX_LUN			0xffff  	/* FCLNX-GPL-547 */
#define HFC_LOGFILE_TYPE_MAX		5	/* FCLNX-GPL-547 */

// @MLPF
#define HFC_MLPF_FMCK_STO       3
#define HFC_MLPF_MCKEND_STO     3
#define HFC_MLPF_FCSTP_STO      5
#define HFC_MLPF_FMCK_GTO       3
#define HFC_MLPF_MCKEND_GTO     300
#define HFC_MLPF_CSTPEND_GTO    300
#define HFC_MLPF_MCKINT_GTO     300
#define HFC_MLPF_ISOLEND_GTO	20

// Isolate Operation for MLPF
#define HFC_ISSUE_ISOLREQ_ERR		0	/* FCLNX-GPL-393 */
#define HFC_ISSUE_ISOLREQ_CMD		1	/* FCLNX-GPL-393 */

/*--- SCSI response judgment----*/
#define HFC_RSP_CODE_MASK 			0xff /* Mask to extract Response code*/
									     /* from FCP response data.	     */
#define HFC_RSP_SUCCESSFUL			0x00 /* Sucessful completion.	     */
#define HFC_RSP_LENGTH_DIFF			0x01 /* FCP data length different    */
									     /* from burst length	     */
#define HFC_RSP_INVALID_CMD_FIELDS	0x02 /* Invalid fields in FCP_CMND   */
#define HFC_RSP_DATARO_MISMATCH		0x03 /* FCP DATA RO mismatch with    */
									     /* FCP XFER RDY DATA RO.	     */
#define HFC_RSP_TSKMGM_UNSUPPORED	0x04 /* Task management function is  */
									     /* not supported by this device */
#define HFC_RSP_TSKMGM_FAILED		0x05 /* Task management function was */
									     /* not performed by this device,*/
									     /* but it is supported. 	     */
/*
 * error code 
 *
 */
#define ERRID_HFCP_ERR1      0x00000001 /* Permanent FC Adapter Hardware error */
#define ERRID_HFCP_ERR2      0x00000002 /* Temporary FC Adapter Hardware error */
#define ERRID_HFCP_ERR3      0x00000003 /* Permanent FC Adapter Firmware error */
#define ERRID_HFCP_ERR4      0x00000004 /* Temporary FC Adapter Firmware error */
#define ERRID_HFCP_ERR5      0x00000005 /* Permanent FC Link error */
#define ERRID_HFCP_ERR6      0x00000006 /* Temporary FC Link error */
#define ERRID_HFCP_ERR7      0x00000007 /* Permanent FC Adapter Unknown error */
#define ERRID_HFCP_ERR8      0x00000008 /* Temporary FC Adapter Unknown error */
#define ERRID_HFCP_ERR9      0x00000009 /* FC Adapter Driver error */
#define ERRID_HFCP_ERRA      0x0000000a /* FC Adapter Interrupt time-out */
#define ERRID_HFCP_ERRB      0x0000000b /* FC Adapter Link Down */
#define ERRID_HFCP_ERRC      0x0000000c /* FC Adapter Diagnostics error */
#define ERRID_HFCP_ERRD      0x0000000d /* FC Adapter PCI error */
#define ERRID_HFCP_ERRE      0x0000000e /* FC Adapter I/O Unavailable */
#define ERRID_HFCP_ERRF      0x0000000f /* FC Adapter Initialize error */
#define ERRID_HFCP_EVNT1     0x00000010 /* FC Adapter Link Up */
#define ERRID_HFCP_EVNT2     0x00000011 /* FC Adapter Link Changed */
#define ERRID_HFCP_EVNT3     0x00000012 /* FC Adapter Driver Warning Event */
#define ERRID_HFCP_EVNT4     0x00000013 /* FC Adapter Driver Request Log */
#define ERRID_HFCP_OPTERR0   0x00000014 /* HFC_OPTERR0 Invalid Optical Module install */
#define ERRID_HFCP_TBL_END   0x00000015 /* Not ERR. Use to set the end of data-table */ /* FCLNX-GPL-547 */

/*************************************/
/*   SCSI Status               		 */
/*************************************/
#define HFC_SCSISTAT_GOOD					0x00
#define HFC_SCSISTAT_CHECK_CONDITION		0x02
#define HFC_SCSISTAT_BUSY					0x08
#define HFC_RESERVATION_CONFLICT			0x18
#define HFC_SCSISTAT_COMMAND_TERMINATED 	0x22

/*************************************/
/*   SCSI Sense buffer size          */
/*************************************/
#define HFC_SCSI_SENSE_BUFFERSIZE			256		/* FCLNX-GPL-463 */

/*************************************/
/*	proc_info type					 */
/*************************************/
#define HFC_PROC_INFO_TYPE		1
#define HFC_SYSFS_INFO_TYPE		2


/************************************************************************/
/*	Trace area													        */
/************************************************************************/
struct hfctrace {
	uchar			trc_id ;			/* Trace ID 					*/
	uchar			trc_data[119] ; 	/* Trace Data Area				*/
	uint64_t		trc_time ;			/* current time			(ns)	*/
};
/************************************************************************/
/* struct free_iov_map													*/
/*		  The bit position and the bit length of a continuous iov_map empty area are set.			*/
/*		  Return value of hfc_get_free_iov								*/
/************************************************************************/
struct free_iov_map {
	uint	free_cnt ;				/* Number of continuous empty areas				*/
	uint	free_pos ;				/* Continuous empty area first position 			*/
};
/* Trace display information */
struct trc_info {
	uint				trc_cur;				/* Current number			*/
	uint				trc_num;				/* Number of maximum traces	*/
	struct hfctrace 	trc_data;				/* Trace data		*/
};
/************************************************************************/
/* err_rec   errlog format                                              */
/************************************************************************/
struct hfc_err_rec {
	uchar 				log_area[1024] ;
};

/**************************************************************************************************/
/* structure																					  */
/**************************************************************************************************/
/************************************************************************/
/* struct hfc_pkt / rst_pkt / com_pkt									*/
/*		hfc_pkt:HBA Private area in SRB(SRB Extension)					*/
/*		rst_pkt:SCSI control area for Abort Task Set/Target Reset		*/
/*      com_pkt:Common part of hfc_pkt/rst_pkt							*/
/*		 (note)Do not use members other than a common part in case of	*/
/*		      cmd_flags<CFLAG_TASK_MGM>!=0			  					*/
/************************************************************************/
struct xrb_rsp {
	uchar				flag;					/* Copy XRB flag		*/
	uchar				xcc;					/* Copy XRB xcc 		*/
	uchar				fsb;					/* Copy XRB fsb 		*/
	uchar				wk; 					/* Space				*/
	unsigned long		err_code;				/* Copy XRB err_code	*/
};

/************************************************************************/
/* watch timer															*/
/************************************************************************/
struct wtimer	 {
	struct	adap_info		*ap;		/* adap_info structure			*/
	struct	target_info 	*target;	/* target info structure		*/
	struct	hfc_pkt			*hfcpk;		/* hfc_pkt structure			*/
	ushort					timer_id;	/* Timer ID (31 from 0)			*/
	ushort					timer_flag;	/* indicate that timer is valid	*/
	int						ap_dev_minor;  /* adapter minor#  FCLNX-322 */
#define HFC_TIMER_VALID			0x80	/* Timer is valid 				*/


#define HFC_DELAY_TMR			1		/* SCSI Command Delay Timer each target	*/
#define HFC_SCSI_CMD_TMR		2		/* SCSI Command Timer 			*/
#define HFC_ABORT_TMR	  		3		/* Abort Task Set Timer			*/
#define HFC_TARGET_RST_TMR		4		/* Target_Reset timer			*/
#define HFC_ELS_TMR				6		/* ELS Timer					*/
#define HFC_LINKINIT_TMR		7		/* Link Initialize Timer		*/
#define HFC_MB_TMR				8		/* MailBox Used Timer			*/
#define	HFC_REBOOT_DELAY_TMR	10		/* REBOOT Delay					*/
#define HFC_LUN0_TMR			11		/* Timer ID for LOOP TEST(Lun#0)	*/
#define HFC_LUN1_TMR			12		/* Timer ID for LOOP TEST(Lun#1)	*/
#define HFC_LUN2_TMR			13		/* Timer ID for LOOP TEST(Lun#2)	*/
#define HFC_LUN3_TMR			14		/* Timer ID for LOOP TEST(Lun#3)	*/
#define HFC_LUN4_TMR			15		/* Timer ID for LOOP TEST(Lun#4)	*/
#define HFC_LUN5_TMR			16		/* Timer ID for LOOP TEST(Lun#5)	*/
#define HFC_LUN6_TMR			17		/* Timer ID for LOOP TEST(Lun#6)	*/
#define HFC_LUN7_TMR			18		/* Timer ID for LOOP TEST(Lun#7)	*/
#define HFC_WEXEC_TMR			19		/* Start waiting cue watch timer		*/
#define HFC_LINKUP_TMR			20		/* LinkUp waiting watch timer			*/
#define HFC_SCN_LINKUP_TMR		21		/* LinkUp waiting watch timer(SCN opportunity)*/
														  /* FCWIN-0082 */
#define HFC_RESTART_TMR	  		22		/* Task Mgm Restart Timer		*/
														  /* FCWIN-0153 */
#define	HFC_MCK_DELAY_TMR		23		/* MCK recovery processing Delay		*/
#define	HFC_DIAG_DELAY_TMR		24		/* Reboot when Diag is tested			*/
#define HFC_MPAP_LOCK_TMR		25		/* MP_ADAP_INFO Lock waiting 		*/
#define HFC_LOGIN_DELAY_TMR		26		/* State of LOGIN DELAY TIME			*/		/* FCLNX-0243 */
#define HFC_LINKUP2_TMR			27		/* LinkUp waiting watch timer after MCK recovers */ /* FCLNX-0241 */
#define	HFC_MCKINT_TMR			28		/* MCKINT watch timer				*/		/* FCLNX-0275 */
#define	HFC_CTLRST_DELAY_TMR	29		/* CTL RESET Delay				*/		/* FCLNX-0279 */

// @MLPF TMR
#define HFC_MLPF_FMCK_TMR		32
#define HFC_MLPF_MCKEND_TMR		33
#define HFC_MLPF_FCSTP_TMR		34
#define HFC_MLPF_CSTPEND_TMR	35

#define HFC_LDLERR_TMR			36		/* FCLNX-0454 */
#define HFC_LDSERR_TMR			37		/* FCLNX-0454 */
#define HFC_IFERR_TMR			38		/* FCLNX-0454 */
#define HFC_TOERR_TMR			39		/* FCLNX-0454 */
#define HFC_RTERR_TMR			40		/* FCLNX-0454 */

#define HFC_DELAY_TMR_DEV		41		/* SCSI Command Delay Timer	each device	*//* FCLNX-GPL-038 */
#define HFC_ISOLATE_DELAY_TMR	42		/* FCLNX-GPL-147 */

#define HFC_INT_CHECK_TMR		43		/* FCLNX-GPL-306 */

#define HFC_TGT_LDLERR_TMR		44		/* FCLNX-GPL-327 */
#define HFC_TGT_LDSERR_TMR		45		/* FCLNX-GPL-327 */

#define HFC_MLPF_ISOLEND_TMR	46
#define HFC_WLINKUP_CNT_TMR		47		/* FCLNX-GPL-FX-424 */

#define HFC_MAX_TMR				100

	struct dev_info			*dev;		/* dev_info structure		*/	/* FCLNX-GPL-047 */

	struct timer_list			dog;	/* Time-out value storage pointer	*/
};


/* 
 * hfc_pkt : I/O initiation information
 *
 *
 */

#define CMND_VALID 			0			/* This dummy_smnd is occupied					*/ 

/* The common area both for hfc_pkt and rst_pkt */
struct hfc_pkt {
	uint					cmd_flags;		/* scsi_pkt control flag (bit)				*/
#define CFLAG_VALID 			0			/* This hfc_pkt is occupied					*/
#define CFLAG_ABORT 			1			/* Abort Task Set request					*/
#define CFLAG_TARGET_RESET		2			/* Target reset request			   			*/
#define CFLAG_BUS_RESET 		3			/* Bus reset request						*/
#define CFLAG_LUN_RESET			4			/* request LUN reset request				*//* FCLNX-0429 */
#define CFLAG_SEGVALID			8			/* SEG_INFO is valid 						*/
#define CFLAG_CSCSI_LU_WITHOUT_DMA	9		/* CANCEL_SCSI Device request				*//* FCLNX-GPL-FX-014 */
#define CFLAG_CSCSI_LU_WAIT_DMA		10		/* CANCEL_SCSI Device Waiting DMA request	*//* FCLNX-GPL-FX-014 */
#define CFLAG_CSCSI_TGT_WITHOUT_DMA	11		/* CANCEL_SCSI Target request				*//* FCLNX-GPL-FX-014 */
#define CFLAG_CSCSI_TGT_WAIT_DMA	12		/* CANCEL_SCSI Target Waiting DMA request	*//* FCLNX-GPL-FX-014 */
#define CFLAG_TIMEOUT			16			/* Timeout occured							*/
#define CFLAG_SCMD_SLOGV		17			/* Timeout (softlog was corrected)  		*/
#define CFLAG_ABORT_TIMEOUT		18			/* Abort Timeout							*/
#define CFLAG_INH_ALTPATH		19			/* Inhibit alternate path					*/
#define CFLAG_HSDLDD_VALID 		20														  /* FCLNX-0429 */
#define CFLAG_COMMAND_DEV 		21														  /* FCLNX-0611 */
#define CFLAG_PFB_VALID 		22														  /* FCLNX-GPL-204 */
#define CFLAG_NOT_TYPE_DISK		23														  /* FCLNX-GPL-450 */

/* FCLNX-GPL-FX-014 Start */
#define CFLAG_CSCSI_ANY								\
	((0x00000001 << CFLAG_CSCSI_LU_WITHOUT_DMA)	|	\
	 (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA)	|	\
	 (0x00000001 << CFLAG_CSCSI_TGT_WITHOUT_DMA)|	\
	 (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))

#define CFLAG_RESET_ANY								\
	((0x00000001 << CFLAG_ABORT)				|	\
	 (0x00000001 << CFLAG_TARGET_RESET)			|	\
	 (0x00000001 << CFLAG_BUS_RESET)			|	\
	 (0x00000001 << CFLAG_LUN_RESET)			|	\
	 (0x00000001 << CFLAG_CSCSI_LU_WITHOUT_DMA)	|	\
	 (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA)	|	\
	 (0x00000001 << CFLAG_CSCSI_TGT_WITHOUT_DMA)|	\
	 (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))
/* FCLNX-GPL-FX-014 End */

/* FCLNX-GPL-FX-181 Start */
#define CFLAG_TGT_RESET_ANY							\
	((0x00000001 << CFLAG_TARGET_RESET)			|	\
	 (0x00000001 << CFLAG_BUS_RESET)			|	\
	 (0x00000001 << CFLAG_CSCSI_TGT_WITHOUT_DMA)|	\
	 (0x00000001 << CFLAG_CSCSI_TGT_WAIT_DMA))

#define CFLAG_LU_RESET_ANY							\
	((0x00000001 << CFLAG_ABORT)				|	\
	 (0x00000001 << CFLAG_LUN_RESET)			|	\
	 (0x00000001 << CFLAG_CSCSI_LU_WITHOUT_DMA)	|	\
	 (0x00000001 << CFLAG_CSCSI_LU_WAIT_DMA))
/* FCLNX-GPL-FX-181 End */
	uint					adap_status;			/* scsi_pkt status					*/

#define SCS_ERROR_VALID			0x80000000		/* for scsi_err_cnt		*/
#define SCS_BUSY_VALID			0x40000000		/* for scsi_busy_cnt		*/

#define SCS_NORMAL_END			0x00000001										/* Normal end 			*/
#define SCS_TRANSPORT_DEAD		0x00001002 | SCS_ERROR_VALID					/* XRB response XCC != 0x80	*/
																				/*	 /COMMAND_TERMINATED*/
#define SCS_IO_ICC_LINK_CHK		0x00001003 | SCS_ERROR_VALID					/* XRB response XCC != 0x80	*/
#define SCS_ADAP_FAILURE		0x00001004 | SCS_ERROR_VALID					/* XRB response FSB = PC 	*/
#define SCS_CMD_TIMEOUT 		0x00001006 | SCS_ERROR_VALID					/* timeout(wdog) cancel */
#define SCS_NO_DEV_RESP			0x00001007 | SCS_ERROR_VALID					/*	  Ditto				*/
#define SCS_IO_ICC_NO_RESP	 	0x00001008 | SCS_ERROR_VALID					/* XRB response FSB = ICC	*/
#define SCS_IO_ICC_TIMEOUT		0x00001009 | SCS_ERROR_VALID					/* XRB response FSB = ICC	*/
#define SCS_WAIT_ABORT			0x0000100A | SCS_BUSY_VALID						/* Abort Task Set is starting	*/
#define SCS_WAIT_RESET			0x0000100C | SCS_BUSY_VALID						/* Target Reset is starting	*/
#define SCS_OFFLINE 			0x0000100D | SCS_ERROR_VALID					/* Not ONLINE 	*/
#define SCS_LOGIN_START 		0x00001010										/* login Start			*/
#define SCS_LOGIN_BUSY			0x00001011 | SCS_BUSY_VALID						/* login Start fail		*/
#define SCS_LOGIN_WWCHG 		0x00001012 | SCS_ERROR_VALID					/* After login, the WWN disagreement	*/
#define SCS_LOGIN_SUCCESS		0x00001013										/* login success			*//*No*/
#define SCS_LOGIN_FAULT 		0x00001020										/* login fail			*//*No*/
#define SCS_PDISC_BUSY			0x00001021										/* pdisc start fail		*//*No*/
#define SCS_PDISC_WWCHG 		0x00001022 | SCS_ERROR_VALID					/* WWN disagreement(After pdisc. )	*/
#define SCS_PDISC_FAULT 		0x00001023										/* pdisc fail			*//*No*/
#define SCS_INTR_LINKDOWN		0x00001024 | SCS_ERROR_VALID					/* Linkdown (Asynchronous INT) */
#define SCS_INTR_PLOGI			0x00001025 | SCS_ERROR_VALID					/* PLOGIN reception(Asynchronous INT)*/
#define SCS_INTR_LOGO			0x00001026 | SCS_ERROR_VALID					/* LOGO reception(Asynchronous INT)	*/
#define SCS_LINKUP_TO			0x00001027 | SCS_ERROR_VALID					/* Linkup waiting timeout	*/
#define SCS_LINK_FAULT			0x00001028										/* Link ini fail 		*//*No*/
#define SCS_IO_CCC				0x00001029 | SCS_ERROR_VALID					/* SERR/PERR/SPERR		*/
#define SCS_MCK 				0x0000102A 					/* MCK_INT				*/
#define SCS_GIDFT_FAULT			0x0000102B										/* No GID_FT response			*//*No*/
#define SCS_LOGIN_NODEV 		0x0000102C										/* No login target 		*//*No*/
#define SCS_WWN_INVALID			0x0000102D | SCS_ERROR_VALID					/* target wwn is illegal		*/
#define SCS_SCSI_DELAY			0x0000102E | SCS_BUSY_VALID						/* SCSI DELAY TMR is starting */
#define SCS_WAIT_LOGIN			0x0000102F | SCS_BUSY_VALID						/* login Issue			*/
#define SCS_ATTACH_IMCOMP		0x00001030 | SCS_ERROR_VALID					/* attach incomplete end			*/
#define SCS_NO_TARGET			0x00001031										/* No login target		*//*P*/
#define SCS_TARGET_ABNORMAL		0x00001032 | SCS_BUSY_VALID						/* target is abnormal		*/
#define SCS_WAIT_LINKUP			0x00001033 | SCS_BUSY_VALID						/* link up waiting state		*/
#define SCS_PCI_BUS_SERR		0x00001034 | SCS_ERROR_VALID					/* PCI SERR				*/
#define SCS_PCI_BUS_PERR		0x00001035 | SCS_ERROR_VALID					/* PCI PERR				*/
#define SCS_PCI_BUS_SPERR		0x00001036 | SCS_ERROR_VALID					/* PCI SPERR			*/
#define SCS_CMD_FLUSH			0x00001037 | SCS_ERROR_VALID					/* Flush waiting state		*/
#define SCS_CMD_SHUTDOWN		0x00001038 | SCS_ERROR_VALID					/* Shutdown waiting state		*/
#define SCS_WAIT_BUS_RESET		0x00001039 | SCS_BUSY_VALID						/* Bus Reset waiting state	*/
#define SCS_CHG_SCSIID			0x0000103A | SCS_ERROR_VALID					/* SCSI ID change			*/
#define SCS_CMD_NEED_ABORT		0x00001040 | SCS_BUSY_VALID						/* ABORT issue waiting		*/
#define SCS_DATA_LENGTH_OVER	0x00001041										/* Forwarding data length  OVER		*//*P*/
#define SCS_INVALID_PATH_ID		0x00001042										/* BUS ID is illegal			*//*P*/
#define SCS_INVALID_TARGET_ID	0x00001043										/* Target ID is illegal		*//*P*/
#define	SCS_CDB_OVER			0x00001044										/* CDB is 17 bytes or more		*//*P*/
#define	SCS_CMD_ABORTED			0x00001045 | SCS_ERROR_VALID					/* Abort Task Set Cancel*/
#define SCS_CMD_RESET			0x00001046 | SCS_ERROR_VALID					/* Target Reset Cancel  */
#define SCS_INVALID_ABORT_SRB	0x00001047										/* Disagreement of SRB for Abort	*//*P*/
#define SCS_XOB_HALT			0x00001048 | SCS_BUSY_VALID						/* XOB HALT state			*/
#define SCS_LUN_EXT_NULL		0x00001049										/* LUN EXT NULL			*//*P*/
#define SCS_NO_NEED_FLUSH		0x00001050										/* FLUSH issue is unnecessary		*//*P*/
#define SCS_NO_NEED_SHUTDOWN	0x00001051										/* SHUTDOWN issue is unnecessary		*//*P*/
#define SCS_LOGIN_THAN_RESET	0x00001052 | SCS_ERROR_VALID					/* Reset demanding login	*/
#define SCS_LOGIN_THAN_ABORT	0x00001053 | SCS_ERROR_VALID					/* Abort demanding login	*/
#define SCS_LOGIN_BUSY_RESET	0x00001054 | SCS_ERROR_VALID					/* Login failure at Reset	*/
#define SCS_LOGIN_BUSY_ABORT	0x00001055 | SCS_ERROR_VALID					/* Login failure at Abort	*/
#define SCS_DATA_OVERRUN		0x00001056										/* Data Over Run		*/
#define SCS_DATA_UNDERRUN		0x00001057										/* Data Under Run		*/
#define SCS_ABORT_FAILED		0x00001058										/* Abort start fail		*/
#define SCS_XOB_FULL			0x00001059 | SCS_BUSY_VALID						/* XOB FULL				*/
#define SCS_IOVMAP_FULL			0x00001060 | SCS_BUSY_VALID						/* IOVMAP FULL			*/
#define SCS_PAGE_OVER			0x00001061 | SCS_BUSY_VALID						/* PAGE CNT OVER		*/
#define SCS_FRAME_CHK_ERROR		0x00001062 | SCS_BUSY_VALID						/* FRAME CHK ERROR		*/
#define SCS_RESET_FAILED		0x00001063 | SCS_BUSY_VALID						/* Target Reset ABEND FCWIN-0153 */
#define SCS_SCN_LINKDOWN		0x00001064 | SCS_ERROR_VALID					/* Link Down between SW_DEV   FCWIN-0153 */
#define SCS_NO_CMND				0x00001065 | SCS_ERROR_VALID
#define SCS_WAIT_MCK			0x00001066 | SCS_BUSY_VALID						/* MCK end waiting			*/
#define SCS_CANCEL_RESP			0x00001067
#define SCS_NARROW_DEV			0x00001068										/* FCLNX-0392 */
#define SCS_WAIT_DEV_RESET		0x00001069 | SCS_BUSY_VALID						/* FCLNX-0429 */

#define SCS_IO_CDC				0x0000106a										/* FCLNX-0534 */
#define SCS_ATTATCH_ERROR		0x0000106b										/* FCLNX-0534 */
#define SCS_ADAPTER_OFFLINE		0x0000106c										/* FCLNX-0534 */
#define SCS_NO_ADAPINFO			0x0000106d										/* FCLNX-0534 */
#define SCS_NO_HFCPKT			0x0000106e										/* FCLNX-0534 */
#define SCS_DMASIZE_OVER		0x0000106f										/* FCLNX-0534 */
#define SCS_REJCT_DIAG			0x00001070										/* FCLNX-0534 */
#define SCS_TARGET_NOTFOUND		0x00001071										/* FCLNX-0534 */
#define SCS_CMDLENGTH_OVER		0x00001072										/* FCLNX-0534 */
#define SCS_EXE_BUSRESET		0x00001073										/* FCLNX-0534 */
#define SCS_EXE_DEVICERESET		0x00001074										/* FCLNX-0534 */
#define SCS_NO_WE_QUE			0x00001076										/* FCLNX-0534 */
#define SCS_CHECK_CONDITION		0x00001077										/* FCLNX-0534 */
#define SCS_MP_NO_APINFO		0x00001078										/* FCLNX-0534 */
#define SCS_MP_BAD_TARGET		0x00001079										/* FCLNX-0534 */
#define SCS_MP_DEVICE_NOTFOUND	0x0000107a										/* FCLNX-0534 */
#define SCS_MP_HSDLDD_DISABLE	0x0000107b										/* FCLNX-0534 */
#define SCS_MP_DMASIZE_OVER		0x0000107c										/* FCLNX-0534 */
#define SCS_MP_TARGET_NOTFOUND	0x0000107d										/* FCLNX-0534 */
#define SCS_MP_NOHFCP			0x0000107e										/* FCLNX-0534 */
#define SCS_MP_STRT_DEVNOTFOUND	0x0000107f										/* FCLNX-0534 */
#define SCS_MP_STRT_HSD_DISABLE	0x00001080										/* FCLNX-0534 */
#define SCS_MP_STRT_TRGNOTFOUND	0x00001081										/* FCLNX-0534 */
#define SCS_MP_STRT_ATTACHFAIL	0x00001082										/* FCLNX-0534 */
#define SCS_MP_STRT_OFFLINE		0x00001083										/* FCLNX-0534 */
#define SCS_MP_STRT_WWN_INVALID	0x00001084										/* FCLNX-0534 */
#define SCS_MP_DEVRST_NO_APINFO	0x00001085										/* FCLNX-0534 */
#define SCS_MP_DEVRST_BADTARGET	0x00001086										/* FCLNX-0534 */
#define SCS_MP_DEVRST_SUCCESS	0x00001087
#define SCS_MP_BUSRST_NO_APINFO	0x00001088										/* FCLNX-0534 */
#define SCS_MP_BUSRST_BADCHNNL	0x00001089										/* FCLNX-0534 */
#define SCS_MP_BUSRST_SACCESS	0x0000108a										/* FCLNX-0534 */
#define SCS_MP_RT_TRG_NOTFOUND	0x0000108b										/* FCLNX-0534 */
#define SCS_MP_RT_DEV_NOTFOUND	0x0000108c										/* FCLNX-0534 */
#define SCS_MP_RT_NOHFCP		0x0000108d										/* FCLNX-0534 */
#define SCS_HSD_NO_APINFO		0x0000108e										/* FCLNX-0534 */
#define SCS_HSD_BAD_TARGET		0x0000108f										/* FCLNX-0534 */
#define SCS_HSD_TRG_NOTFOUND	0x00001090										/* FCLNX-0534 */
#define SCS_HSD_PATH_NOTFOUND	0x00001091										/* FCLNX-0534 */
#define SCS_HSD_DEV_NOTFOUND	0x00001092										/* FCLNX-0534 */
#define SCS_HSD_DISABLE			0x00001093										/* FCLNX-0534 */
#define SCS_HSD_DMASIZE_OVER	0x00001094										/* FCLNX-0534 */
#define SCS_HSD_LURST_NO_APINFO	0x00001095										/* FCLNX-0534 */
#define SCS_HSD_LURST_BADTARGET	0x00001096										/* FCLNX-0534 */
#define SCS_HSD_LURST_NO_TRG	0x00001097										/* FCLNX-0534 */
#define SCS_HSD_LURST_NO_DEV	0x00001098										/* FCLNX-0534 */
#define SCS_HSD_LURST_DISABLHSD	0x00001099										/* FCLNX-0534 */
#define SCS_HSD_LURST_NOTRGINFO	0x0000109a										/* FCLNX-0534 */
#define SCS_HSD_LURST_ATTACHERR	0x0000109b										/* FCLNX-0534 */
#define SCS_HSD_LURST_OFFLINE	0x0000109c										/* FCLNX-0534 */
#define SCS_HSD_LURST_WWN_INVAL	0x0000109d										/* FCLNX-0534 */
#define SCS_HSD_LURST_WAIT_RST	0x0000109e										/* FCLNX-0534 */
#define SCS_HSD_DVRST_NO_APINFO	0x000010a0										/* FCLNX-0534 */
#define SCS_HSD_DVRST_NO_TARGET	0x000010a1										/* FCLNX-0534 */
#define SCS_HSD_DVRST_SUCCESS	0x000010a2										/* FCLNX-0534 */
#define SCS_HSD_BSRST_NO_APINFO	0x000010a8										/* FCLNX-0534 */
#define SCS_HSD_BSRST_NO_TARGET	0x000010a9										/* FCLNX-0534 */
#define SCS_HSD_BSRST_SUCCESS	0x000010aa										/* FCLNX-0534 */	

#define SCS_SCSI_CHECK			0x000010b0										/* FCLNX-0594 */
#define SCS_WAIT_CANCEL			0x000010b1 | SCS_BUSY_VALID						/* Cancel login Issue FCLNX-GPL-038	*/
#define SCS_WAIT_LUNRST			0x000010b2										/* FCLNX-GPL-0343 */
#define SCS_NEED_LUNRST			0x000010b3										/* FCLNX-GPL-0343 */
#define SCS_FLASH_UPDATE		0x000010b4										/* FCLNX-GPL-FX-146 */

#define SCS_RSPCODE_FAILURE		0x000010c0										/* RSP_CODE byte3!=0  FCLNX-GPL-0312,0321 */

	/* XOB management information */
	uchar					cmd_xob;		/* XOB number for scsi_pkt					*/
	uchar					tout_slog_ssn; 	/* MIH-LOG number for timeout condition		*/
											/* It is valid when HFC_SCMD_SLOGV=1		*/
	uchar					tout_slog_son; 	/* MIH-LOG length							*/
	ushort					tout_slog_sbc; 	/* MIH-LOG length							*/
//	struct xrb_rsp			xrb_rsp;		/* XRB response								*/

	struct wtimer			cmd_timeout;	/* Timer  for SCSI command response check	*/
	struct scsi_cmnd		*cmd_pkt;		/* Pointer to SCSI_Cmd						*/

	struct hfc_pkt 			*cmd_forw;		/* Pointer to the next hfc_pkt				*/
	struct hfc_pkt 			*cmd_prev;		/* Pointer to the previous hfc_pkt			*/

	struct adap_info		*ap;			/* Pointer to adap_info 					*/
	struct target_info		*target;		/* Pointer to target_info 					*/
	struct dev_info			*dev;			/* dev_info pointer for SCSI				*/

//	struct port_info		*pp;			/* Pointer to port_info 					*//* FIVE-FX */
//	struct region_info		*rp;			/* Pointer to region_info 					*//* FIVE-FX */
//	struct core_info		*core;			/* Pointer to core_info						*//* FIVE-FX */

	dma_addr_t		        dma_handle;		/* Pointer to DMA area						*/
	uint					seg_cnt;		/* Number of Scatter-Gather list */
	uint32_t				data_size;		/* Total Data Size */

	/* The following elements are hfc_pkt only */
	uint 					iov_no;			/* Top of the seg_info (SRB)				*/
	uint					iov_cnt;		/* Total number of seg_infos (SRB) 			*/
	uint					timeout;		/* SCSI Command Timeout period				*//* FCLNX-GPL-575 */
//	struct seg_info			xobseg[2];		/* SEG_INFO(0)/(1) in XOB					*/

	uchar					target_id;		/* physical target id						*/
	uchar					reserve;		/* Reserve Area					 FCLNX-0603 *//* FCLNX-GPL-0343 */
	uchar					group_id;		/* group id									*/
	uchar 					err_count_enable;	/* Enable to Error Count     FCLNX-0454 */
	ushort					fail;			/* number of error end			 FCLNX-0603 */
	uchar					rid;			/* Region ID of I/O Request		 FIVE-FX	*/
	uchar					resv[1];		/* Reserve Area					 FCLNX-0603 */
	ushort					lun_id;			/* LU#										*//* FCLNX-GPL-0343 */
	ulong					upathmap;		/* path used map				 FCLNX-0429 */
	ulong					cpathmap;		/* path used map				 FCLNX-0429 */
};


struct test_dev_info {

	uchar				status;			/* State of dev					*/

#define	HFC_LOOP_TOUT	0x08				/* Loop test time-out	*/
											/* (Asynchronous interruption waiting)		*/
	uchar				flag;			/* Control flag                     */
#define	HFC_LOOP_EXEC	0x80			/* Inside and outside Loop test is being exec		*/ 
	wait_queue_head_t	loop_event ;	/* thread event				*/

	struct wtimer		loop_wdog;		/* wdog timer					*/
};

/*
 * target device search information
 *
 */
struct target_scan {
	uint					flags; 
#define HFC_SCAN_SCV			0x80000000		/* SCSI ID Effective							*/
#define HFC_SCAN_NEED			0x08000000		/* GPN_ID issue waiting 						*/
#define HFC_SCAN_WAIT			0x04000000		/* GPN_ID is being issued 						*/
#define HFC_SCAN_COMP			0x00000100		/* GPN_ID is compleate(Normal or Abnormal end) 		*/
#define HFC_SCAN_FAIL			0x00000200		/* GPN_ID is fail							*/
#define HFC_SCAN_LOGIN			0x00000001		/* LOGIN is compleate(Normal or Abnormal end)			*/
	uint					scsi_id;			/* SCSI ID to be retrieved					*/
	uint64_t				wwpn;				/* WWPN 								*/
};

#if 0		/* FCLNX-GPL-0449 */
struct hfc_wr_retry {													  /* FCLNX-0244 */
	char  instance;								/* instance#							*/
	char  channel;								/* channel# 							*/
	short scsi_id;								/* scsi id								*/
	short lun_id;								/* lun id  								*/
	uchar count;								/* retry count							*/
												/*  0x01-0x7E : retry& no panic			*/
												/*  0x80      : infinity       			*/
												/*  0x81-0xFE : retry& panic   			*/
												/*  0xFF      : default retry & panic 	*/
	char  rsv;
};
#endif

/* FCLNX-GPL-116 */
typedef struct{ /* PCIe SRAM 1bit error / Core SRAM 1bit error */
#define HFC_1BIT_LOG_ENTRY	16
	uint			err_num;								/* +0x000 */
	uchar			resv1[12];								/* +0x004 */
	uint			ecid[4];								/* +0x010 */
	uchar			pcie_sram_data[HFC_1BIT_LOG_ENTRY][16];	/* +0x020 */
	uint			pcie_sram_cnt;							/* +0x120 */
	uchar			resv2[12];								/* +0x124 */
	uchar			core_ce_data[HFC_1BIT_LOG_ENTRY][16];	/* +0x130 */
	uint			core_ce_cnt;							/* +0x230 */
	uchar			resv3[460];								/* +0x234 */
} hfc_err1bit_logout_t;

/* dev_info *//* FCLNX-GPL-0343 */
struct dev_info {
	uchar					flags;		/* dev_info status					*/
/* Flag for FIVE-EX/FIVE	*/
#define HFC_DEVINF_VALID	0
#define HFC_LU_LGINF_VALID	1
#define HFC_NOT_TYPE_DISK	2												/* FCLNX-0271 */
#define HFC_HSDLDD_VALID	3												/* FCLNX-0429 */
#define HFC_COMMAND_DEV		4
#define HFC_DF_DEVINFO_RSVPATH	5		/* HFC-PCM *//* FCLNX-FX-038 */
#define HFC_DEVINFO_SCAN	7

/* Flag for FIVE-FX			*/
#define HFC_D_DEVINF_VALID	0			/* Dev_info is Valid				*/
#define	HFC_D_LU_LGINF_VALID	1		/* Logical dev_info is valid		*/
#define	HFC_D_NOT_TYPE_DISK	2
#define	HFC_D_HSDLDD_VALID	3
#define	HFC_D_COMMAND_DEV	4

	uchar					status;		/* lu path status					*/
#define HFC_PATH_ONLINE		1
#define HFC_PATH_ONLINE_E	2
#define HFC_PATH_STANDBY	3
#define HFC_PATH_OFFLINE_C	4
#define HFC_PATH_OFFLINE_E	5
#define HFC_PATH_DELETE		7

	uchar 					lustat;		/* LU information for Abort Task Set*//* FCLNX-GPL-0343 */
/* LU Status for FIVE-EX	*/
#define HFC_WAIT_ABORT          0x80
#define HFC_LUNRST_DELAY		0x20						/* FCLNX-GPL-FX-152 */
#define HFC_NEED_ABORT			0x10                        /* FCWIN-0117 */
#define HFC_DEFER_ABORT			0x08
#define HFC_WAIT_LUN_RESET		0x04
#define HFC_NEED_LUN_RESET		0x02
#define HFC_WAIT_LUNRSP			0x01	/* FCLNX-GPL-0343 */

/* LU Status for FIVE-FX	*/
#define HFC_D_NEED_ABORT	0			/* Need to issue Abort Task Set		*/
#define	HFC_D_WAIT_ABORT	1			/* Waiting Abort Task Set Response	*/
#define	HFC_D_NEED_LUN_RESET	2		/* Need to issue LUN Reset			*/
#define	HFC_D_WAIT_LUN_RESET	3		/* LUN Reset is in progress			*/
#define	HFC_D_FAIL_LUN_RST	4			/* LUN Reset Failed					*/
#define	HFC_D_WAIT_ABTSRSP	5			/* Abort Task Set is in progress	*/
#define	HFC_D_WAIT_LUNRSP	6			/* LUN Reset is in progress			*/

	uchar					owner_ctl;	/* owner controller =1				*/
	uchar					target_id;	/* target id						*/
	uint					lun;		/* LU#								*/
	uint64_t				rsv_key;	/* reservation key					*//* FCLNX-GPL-FX-378 */

#define HFC_NOT_OWNER_PATH	0
#define HFC_OWNER_PATH		1
	uchar					priority;	/* path priority					*/
	uchar					group_id;	/* group id							*/
	uchar					path_id;	/* LU path id						*/
	uint					ioerror;	/* number of io errors				*/
	uint					iocount;	/* number of io counts				*/
	int					id_size;							/* FCLNX-0548 */
#define DEVINFO_COMP_LEN		128								/* FCLNX-0315 */
	uchar					compare[DEVINFO_COMP_LEN];	/* copy of inquiry byte8-47 	*/
	struct target_info		*target;	/* Pointer to the target info		*/
	struct dev_info			*next;		/* Pointer to the next dev_info 	*/
	struct dev_info			*prev;		/* Pointer to the previous dev_info */
	struct lg_dev_info		*lg_dev;	/* Pointer to the previous lg_dev_info FCLNX-0429 */
	struct wtimer			lun_delay_wdog;		/* LUN Reset delay timer	*/	/* FCLNX-0627 */
	uchar					io_status;			/* FCLNX-0627 */
#define HFC_LUN_RESET_DELAY_TO		0	/* Waiting while LUN Reset Delay Timer is executing */ /* FCLNX-0627 */
	int						wx_que_cnt;
	int						we_que_cnt;
	char					id_type;	/* Identifer Type */	/* FCLNX-0677 *//* FCLNX-710 */
#define HFC_INQ83_IDTYPE1	1	/* FCLNX-710 */
#define HFC_INQ83_IDTYPE3	3	/* FCLNX-710 */
//	uchar					write_retries;		/* FCLNX-GPL-0449 */
	uchar					curr_core;		/* To use the same core for next scsi cmd *//* FIVE-FX */
	uchar					curr_cmd_type;	/* To check if the next command type is same to previous one *//* FIVE-FX */
};								/* FCLNX-GPL-0343 */

/* 
 * target_info : target information
 *
 *  allocated by hfc_detect()
 *  released  by hfc_release()
 *
 */
struct target_info {
	uint						status;		/* target status						*/
/* Status for FIVE-EX/FIVE		*/
#define HFC_NEED_LOGIN			0  			/* Need NPLOGIN/PrLOGIN	req				*/
#define HFC_NEED_PDISC			1  			/* Need SCN/RSCN, Need PDISC 			*/
#define HFC_NEED_GIDPN			2  			/* Need WWN->SCSIID req					*/
#define	HFC_XOB_HALT			7  			/* XOB stop								*/
#define HFC_WAIT_LOGIN			8  			/* LOGIN is in progress					*/
#define HFC_WAIT_PDISC			9  			/* PDISC is in progress					*/
#define HFC_WAIT_GIDPN			10 			/* GIDPN is in progress					*/
#define HFC_WAIT_TARGET_RESET   11 			/* Target reset is in progress			*/
#define HFC_WAIT_BUS_RESET		16 			/* Other target reset is in prog		*/
#define HFC_WAIT_TARGET_STOP	17 			/* Close process is in progress			*/
#define HFC_SCN_RESP			18 			/* Received SCN/RSCN 					*/
#define HFC_WPDISC_LOGO_RESP	19 			/* LOGO receiver during PDISC			*/
#define HFC_SCSI_DELAY			20 			/* Supress SCSI after Linkdown  		*/
#define HFC_SCN_WLINKUP			21 			/* Linkup timer for SW to disk			*/
#define HFC_WAIT_CANCEL			22 			/* Cancel Login is in progress			*/ /* FCLNX-GPL-038 */
#define HFC_NEED_PLOGI_T		23
#define HFC_WAIT_PLOGI_T		24
#define HFC_NEED_PRLI			25
#define HFC_WAIT_PRLI			26

#define HFC_NEED_CANCEL			31 			/* It is being directed to cancel to FW */

	uint						flags;		/* target vaildity information				*/
/* Flag for FIVE-EX/FIVE		*/
#define HFC_TARGETINF_VALID		0			/* target_info is valid					*/
#define HFC_DEVFLG_VALID		4			/* LOGIN succeeded						*/
#define HFC_WWN_VALID			5			/* WW_Name is valid						*/
#define HFC_TGT_LGINF_VALID		6			/* tgt logical info is valid			*/
#define HFC_TARGET_GHOST		7			/* ghost target only FC-GW              */
#define HFC_WAITING_DEV_LOSS_TMO	8		/* use only during ld_err_limit_s ON	*/

	uchar					pseq;		/* target sequence number (0-255)			*/
	uint					target_id;	/* Target logical ID (0-255)				*/
	uchar					fc_class;	/* Service class							*/
#define HFC_FC_CLASS1			0x01		/* fc class 1 							*/
#define HFC_FC_CLASS2			0x02		/* fc class 2 							*/
#define HFC_FC_CLASS3			0x03		/* fc class 3 							*/
#define HFC_FC_CLASS4			0x04		/* fc class 2 							*/
#define HFC_FC_CLASS6			0x06		/* fc class 3 							*/

	/* These are gotten by LOGIN response												*/
	ushort					fc_class_mask;	/* Class mask								*/
	ushort					device_flags;	/* Least 2bytes parameter 					*/
	ushort					max_frame_size;	/* Max_frame_size 							*/
	ushort					prli_parm;		/* PRLI_PARM<2-3> :FIVE-FX					*/
	uint64_t 				scsi_id;		/* Destination ID							*/
	uint64_t 				ww_name;		/* World Wide Port Name		   				*/
	uint64_t 				node_name;		/* World Wide Node Name		   				*/

	/* SCSI command initiate_wait_queue 												*/
	int 					wx_que_cnt;		/* SCSI initiation wait queue				*/
	struct hfc_pkt			*wx_que_top;	/* Pointer to top				*/
	struct hfc_pkt			*wx_que_end;	/* Pointer to end				*/

	/* SCSI command wait_end_queue 														*/
	int 					we_que_cnt;		/* SCSI wait end queue count				*/
	struct hfc_pkt			*we_que_top[HASH_T_NUM];	/* Pointer to top				*/
	struct hfc_pkt			*we_que_end[HASH_T_NUM];	/* Pointer to end				*/

	struct target_info		*next;			/* Pointer to the next target_info 			*/
	struct target_info		*prev;			/* Pointer to the prev. target_info			*/

	/* Pointer to the adap_info->login_target											*/
	struct target_info		*login_next;	/* The next LOGIN							*/
	struct target_info		*plogi_next;	/* The next PLOGI							*/
	struct target_info		*prli_next;		/* The next PRLI							*/
	struct target_info		*pdisc_next;	/* The next PDISC							*/

	/* watchdog timer 																	*/
	struct wtimer			wexec_wdog;		/* SCSI initiation queue 					*/
	struct wtimer			scnlinkup_wdog;	/* SW-Dev, Link_up 							*/
	struct wtimer			delay_wdog;		/* delay timer								*/
	struct wtimer			restart_wdog;		/* restart timer	*/


//	uchar lustat[MAX_DEV_CNT];				/* LU information for Abort Task Set*/
//#define HFC_WAIT_ABORT          0x80
//#define HFC_NEED_ABORT			0x10                        /* FCWIN-0117 */
//#define HFC_DEFER_ABORT			0x08
	uchar						   next_dstart_flag ;   /* 1.76						   	*/
														/* 1:It has registered in the next_dstart cue	*/
	struct target_info 		*nnext ;		/* Next pointer of next_dstart cue			*/
	struct target_info 		*nprev ;		/* Prev pointer of next_dstart cue			*/
	
	struct adap_info		*ap;			/* Pointer to the adap_info area			*/
//	struct port_info		*pp;			/* Pointer to the port_info area			*/
	struct dev_info			*dev;			/* Pointer to the dev_info area				*/
	uchar					group_id;		/* group id									*/
	uchar					attribute;		/* target path attribute					*/
#define HFC_ATTR_CONFIGURED		0x00
#define HFC_ATTR_CONFIGURED_H	0x01
#define HFC_ATTR_UNCONFIGURED	0x02
#define HFC_ATTR_UNCONTROLLED	0x03
	uchar					path_id;		/* target path id							*/
	struct lg_target_info	*lg_target;		/* connect lg_target_info pointer			*/
//	uchar					write_retries[256];							  /* FCLNX-0244 */
	
	struct fc_rport			*rport;

	uchar					target_reset;	/* target reset status					*/      /* FCLNX-GPL-036 */
#define HFC_NEED_TARGET_RESET   0  			/* Need Target_Reset					*/
#define HFC_DEFER_TARGET_RESET	1 			/* MIHLOG is being issued to concerned Target *//* FCLNX-GPL-036 */
//#define HFC_WAIT_DEVRSP         2			/* target reset is in prog	*/	/* FCLNX-GPL-153*//* FCLNX-GPL-0343 */
#define HFC_WAIT_TGTRSP         2			/* target reset is in prog	*/	/* FCLNX-GPL-153*//* FCLNX-GPL-0343 */
	uchar					rport_status;
#define HFC_NEED_RPORT_ADD		0
#define HFC_NEED_RPORT_DEL      1 /* FCLNX-GPL206 */

	ushort					send_frame_size;	/* FCLNX-GPL-261 */
	
	struct errcnt_info		*tgt_ldl_errcnt_info;		/* Counter for Long Link-down error */					/* FCLNX-GPL-327 */
	struct errcnt_info		*tgt_lds_errcnt_info;		/* Counter for Short Link-down error */					/* FCLNX-GPL-327 */
	struct wtimer			tgt_ldlerr_wdog;			/* Watchdog timer of Link-down Error */					/* FCLNX-GPL-327 */
	struct wtimer			tgt_ldserr_wdog;			/* Watchdog timer of Link-down Error */					/* FCLNX-GPL-327 */
	uint					tgt_ld_err_count_s;			/* Error Counter for Short Link-down error */  /* FCLNX-GPL-349 */
	uchar					link_recovered;				/* HBA - Target link is recovered.    */					/* FCLNX-GPL-334 */
	uint32_t				dev_loss_tmo;				/* Save rport->dev_loss_tmo			*/
	uint32_t				fast_io_fail_tmo;			/* Save rport->fast_io_fail_tmo		*/
};

struct pkg_tp {
	uchar					type;			/* Package Type								*/
#define HFC_PKTYPE_FPP			1			/* FPP 										*/
#define HFC_PKTYPE_FIVE			2			/* Five										*/
#define HFC_PKTYPE_FIVE_EX		3			/* Five-EX									*/
#define HFC_PKTYPE_FIVE_FX		4			/* Five-FX									*/

	uchar					port;			/* Port num (1/2/4)							*/
	uchar					code;			/* Package Code								*/
											/* Copy of PCI 005						*/
	struct pkg_map			*map;			/* PCI Reg offset map					*/

	ushort				vender_id;		/* Copy Config reg Byte0-1  */
	ushort				device_id;		/*                 Byte2-3  */
	ushort				sub_system_id;	/* Config reg Byte2E-2F     */	/* FIVE-EX */
	uchar				core_no;		/* Core Number				*/	/* FIVE-EX */
	uchar				lsi_rev;		/* LSI_Rev */ /* FCLNX-GPL-220 */
	uchar				one_core;		/* One core mode */ /* FCLNX-GPL-233 */
};

struct lpar_tp {
	uint				frame_cnt;			/* MAX_FRAME_CNT value			*/  /* FCLNX-0374 */
	struct hg_map		*hg_map;			/* MMIO-HG offset map	*/
};

struct slot_ww_name { /* FCLNX-GPL-201 */
	/* serch keys */
	int      domain_no;	/* 4byte */ /* domain (this slot) */
	uchar    bus_no;	/* 1byte */ /* bus (this slot) */
	uchar    devfn;		/* 1byte */ /* dev func (this slot) */
	uchar    port_no;	/* 1byte */ /* port_no (this port) */
	/* data */
	uint64_t ww_name;	/* 8byte */ 
};

/* FCLNX-0542 */
struct errcnt_info{  /* FCLNX-0454 */
        int            remain_count;                                   /* Number of times to isolate */
#define HFC_MAX_ERORR_COUNT		2048
        uint            *remain_time;                                 /* Array of monitaring time */
        uint            p_current_remain_time;                  /* Current monitaring time*/
        uint            p_top_remain_time;                              /* Head of Array of monitaring time*/
        uint            first_errcnt_time;                              /* Time of finding first error */
        uint            current_tmr_time;                               /* Curr nt monitaring time */
};

/* flag */
#define HFC_OCCURED_FAILURE		1		/* FCLNX-0506 */
#define HFC_WATCHERR_TMR_TO		0		/* FCLNX-0506 */

/* Err flag */
#define HFC_LDL_ERR				0		/* FCLNX-0506 */
#define HFC_LDS_ERR				1		/* FCLNX-0506 */
#define HFC_IF_ERR				2		/* FCLNX-0506 */
#define HFC_TO_ERR				3		/* FCLNX-0506 */
#define HFC_RT_ERR				4		/* FCLNX-0506 */
#define HFC_TGT_LDL_ERR			5		/* FCLNX-GPL-327 */
#define HFC_TGT_LDS_ERR			6		/* FCLNX-GPL-327 */


/* Isolate_Setting */
#define HFC_SET_NORMAL_PORT		0		/* FCLNX-0506 */
#define HFC_SET_ISOLATE_PORT	1		/* FCLNX-0506 */
#define HFC_SET_ISOLATE_ADAPT	2		/* FCLNX-0506 */
/* FCLNX-0542 */


/* 
 * adap_info : adapter information
 *
 *  allocated by hfc_detect()
 *  released  by hfc_release()
 *
 */
struct adap_info {
	uchar					name[16];  		/* Set "adap_info" characters			 	*/
	uint					status;			/* Port status information					*/
#define HFC_ENABLE				0			/* Detect f/w ready							*/
#define	HFC_ONLINE				1			/* Link initialization succeeded			*/
											/* Link is in link-up status				*/
#define HFC_NEED_LINK_INIT		2			/* Link initialization failed		 		*/
#define	HFC_NEED_NMSRV			3			/* Waiting request issue to the name server	*/
#define HFC_NEED_GPNID			6			/* Waiting request issue of GPNID			*/
#define HFC_WAIT_LINK_INIT		8			/* Initiate_Link_Initialization 			*/
											/* is in progress 							*/
#define	HFC_WAIT_NMSRV			9			/* Request to name server is in progress	*/
#define	HFC_WAIT_LINKUP			10			/* Waiting status change					*/
											/* from LINK_Down to LINK_Up				*/
#define	HFC_WAIT_GPNID			11			/* Waiting GPNID completion					*/
#define	HFC_WAIT_BUSRSP			12			/* Bus reset is in progress					*/
#define	HFC_WAIT_CLOSE			13			/* Shutdown pro. is in progress 			*/
#define HFC_WAIT_T3				14			/* Waiting MCK int after Frc-MCK			*/
#define HFC_WAIT_ISOL_LINKUP_CNT 15			/* Wait Link Up at Isol mode				*//* FCLNX-GPL-FX-424 */
#define HFC_MCK_RECOVERY		16			/* MCK recovery is in progress 				*/
#define HFC_CHK_STOP			17			/* Check-Stop	*/

//@MLPF
#define HFC_MLPF_WAIT_FMCK		18			/* waiting FMCK INT                         */
#define HFC_MLPF_WAIT_MCKEND	19			/* waiting MCKEND                           */
#define HFC_MLPF_WAIT_FCSTP		20			/* waiting FCSTP INT                        */
#define HFC_MLPF_WAIT_CSTPEND	21			/* waiting CSTPEND                          */
//@MLPF

#define	HFC_PLEVEL_MB			24			/* Waiting mailbox resp(ioctl)				*/
#define HFC_DIAG_END			25			/* Diag processing end. The LinkUp log is not gathered		*/
#define HFC_DIAG_DELAY			26			/* Diag processing end. DeLay is put		*/
#define HFC_LOGIN_DELAY			27			/* Wait does the Login DeLay time			*/	/* FCLNX-0243 */
#define HFC_WAIT_MIHLOG 		28			/* Waiting request issue of MIHLOG			*/	/* FCLNX-0454 */
#define HFC_ISOL				29			/* Into Isolated status */	/* FCLNX-0506 */
#define HFC_ISOL_RECOVERY		30			/* Isolation process in progress */ /* - FCLNX-546 - */
#define HFC_SUSPEND				31			/* HBA is Suspended 						*/	/* FCLNX-GPL-306 */

	struct pkg_tp			pkg;			/* package config infotmation				*/
											/*	pkg.type (FPP/Five)						*/
											/*	pkg.port (1/2/4)						*/
											/*	pkg.pkcode (0x80/0x81/0x82)				*/
											/*	pkg.map (pointer to reg map)			*/
										
	struct lpar_tp			lparmode ;		/* Information only for LPAR				*/	/* FCLNX-0374 */

/*	struct kdev_t			devno;			  device number (major/minor)				*/
	int						dev_major;		/* major number								*/
	int						dev_minor;		/* minor number								*/
										
	/* following elements are same as the contents of fw_init_tbl 						*/
	uint64_t				scsi_id; 		/* HBA SCSI-id 								*/
	uint64_t				ww_name; 		/* HBA world wide port name					*/
	uint64_t				node_name; 		/* HBA world wide node name					*/
	uint					rid;			/* LPAR RID(LPAR#)			*/	/* @MLPF STR */
	uchar					mlpf_mode;		/* MLPF mode				*/
#define HFC_MMODE_MLPF			0x80 		/*	 LPF effective(common & occupation)	*/
#define HFC_MMODE_DEDICATE		0x40 		/*	 Occupation bit				*/
#define HFC_MMODE_SHADOW		0x08		/* SHADOW LPAR              */
#define HFC_MMODE_REBOOT		0x04		/* SHADOW REBOOT            */	/* @MLPF END */
	uchar					connect_type;	/* connection type							*/
#define HFC_UNKN				0x0 			/*		 connection unknown				*/
#define HFC_PT2PT				0x1 			/*		 Point To Point 				*/
#define HFC_SWITCH				0x2 			/*		 Switch(Fabric) 				*/
#define HFC_AL					0x3 			/*		 FC_AL							*/
#define HFC_F_PORT				0xf0			/*		 F_PORT							*//* FIVE-FX */
#define HFC_MULTI_ALPA			0xf3			/*		 Multiple ALPA					*//* FIVE-FX */
	uint					max_data_rate;	/* transfer rate 							*/
#define HFC_1000MBS				1000			/* Transfer rate is 10Gbps				*/
#define HFC_800MBS				800				/* Transfer rate is 8Gbps				*/
#define HFC_400MBS				400				/* Transfer rate is 4Gbps				*/
#define HFC_200MBS				200				/* Transfer rate is 2Gbps				*/
#define HFC_100MBS				100				/* Transfer rate is 1Gbps				*/

	uchar					used_nmsrv;		/* System is using name server 				*/
	char					port_no;		/* port number of mp_adap_info				*/

	uint					attach_status;	/*Initialization has finished 				*/
#define HFC_ATTACH				0				/* detect() has finished				*/
#define HFC_DETACH				1				/* release() has finished 				*/

	uint				open_status; 	/* Availability of hfc_open func			*/
#define HFC_OPENED				1				/* diag_open() has finished				*/
#define HFC_CLOSED				0				/* diag_close() has finished			*/
#define HFC_DIAG_OPENED			2
	uchar					fcp_mode ;		/* Diag used								*/
	uint					open_cnt ;		/* Number of opens							*/

	uint					mb_status;		/* mailbox status 							*/
											/* If the value equal zero, it means		*/
											/* someone occupied mailbox	 				*/
#define HFC_MB_PROL				0				/* Used by ioctl process				*/
#define HFC_MB_INTL				1				/* Used by other process				*/
#define HFC_MB_LOCK				2				/* Locked by someone					*/
#define	HFC_PRE_PROL			4				/* PRO-LEVEL execution waiting 			*/

	uint					mpap_lock;
#define	HFC_LOCK_WAIT_1		1				/* failed to lock mp_adap_info at five_mck_recovery	*/
#define	HFC_LOCK_WAIT_2		2				/* failed to lock mp_adap_info at chk_stop	*/
#define	HFC_LOCK_WAIT_3		3				/* failed to lock mp_adap_info at fpp_logout	*/
#define	HFC_LOCK_WAIT_4		4				/* failed to lock mp_adap_info at fpp_logout	*/
#define	HFC_LOCK_WAIT_5		5				/* failed to lock mp_adap_info at five_logout	*/
#define HFC_LOCK_WAIT_6		6				/* failed to lock mp_adap_info at MMIO-HG ind acc @MLPF */								
#define	HFC_LOCK_WAIT_7		7				/* failed to lock mp_adap_info at fpp_mck_recovery	*/	/* FCLNX-GPL-0576 */
#define	HFC_LOCK_WAIT_8		8				/* failed to lock mp_adap_info at five_ex_logout */
	uchar					*mb_parm; 		/* Pointer to mailbox data area				*/
	uint					mb_resp;		/* Status of mailbox response				*/
#define HFC_MBR_COMPLETE		0x00000001 		/* Received mbox without error			*/
#define HFC_MBR_TIMEOUT			0x00000002 		/* No reply. Timeout happend			*/
	uint					mb_results;		/* Error code of mailbox response			*/
	ushort					mb_retry_cnt;	/*Retry counter of mailbox request			*/
	ushort					mb_retry_tid;	/*Timer ID for mailbox retry				*/
	ulong					mb_retry_tout;	/* Timer value of mailbox retry				*/
	ushort					xob_no;			/* xob number pointed by xop_inp			*/
	ushort					xrb_no;			/* xrb_number pointed by xrb_outp			*/
	uint					iov_no;			/* Pointer to seg_info						*/
	uint					iov_map_cnt;	/* bit width of iov_map						*/
	struct Scsi_Host		*hosts;			/* Pointer to the Scsi_Host struct. 		*/
	ulong					host_no;		/* Used for IOCTL_GET_IDLUN, /proc/scsi 	*/
	struct pci_dev			*pci_cfginf;	/* Pointer to PCI Config Base				*/
											/* return value of pci_find_device()		*/
	ulong					mem_base_addr;	/* Pointer to PCI Memory space				*/
	ulong					hg_mem_base_addr ;	/* MMIO-HG space Base addr	*/	/* @MLPF */ /* FCLNX-0352 */
										  	/* by pci_resouce_start()					*/

	struct Scsi_Host		*hfchsd_host;								/* FCLNX-0429 */
	void					*hfchsd_ap;									/* FCLNX-0429 */
	struct Scsi_Host		*hfcpfb_host;								/* FCLNX-GPL-204 */
	void					*hfcpfb_ap;									/* FCLNX-GPL-204 */
	struct scsi_cmnd		*bus_reset_pkt;								/* FCLNX-0429 */

	/* following elements are the physical address to assigned area						*/
	dma_addr_t				padr_init;		/* INIT_TBL 								*/
	dma_addr_t				seg_phys_addr;	/* SEGINFO									*/
	dma_addr_t				phys_mb_parm;	/* Mailbox Parater area						*/

	dma_addr_t				phys_xob;		/* XOB		 								*/
	dma_addr_t				phys_xrb;		/* XRB										*/
	dma_addr_t				phys_mb;		/* MAILBOX									*/
	dma_addr_t				phys_slog[256];	/* SOFT LOG	(release)		*/
	dma_addr_t				phys_iov;
	uchar					*slog_addr[256];	/* SOFT LOG (release)		*/

	/* following elements are the logical address to assigned area						*/
	struct fw_init_tbl		*fw_init_p;		/* INIT_TBL;								*/
	struct mailbox			*mb;			/* MAILBOX									*/
	struct xob				*xob;			/* XOB										*/
	struct xrb				*xrb;			/* XRB										*/
	struct seg_info			*seg_info;		/* SEGINFO									*/
	uchar					*slog;			/* SOFTLOG									*/
	
	struct mp_adap_info		*mp_adap_info;	/* Pointer to mp_adap_info which			*/
											/* is connected to this adap_inf			*/
	struct adap_info		*next;			/* Next adap_info				*/
	struct adap_info		*prev;			/* Previous adap_info				*/
	struct hfc_pkt			*pkt_pool[(HFC_PKT_NUM/HFC_PKT_POOL_SIZE)+1];	/* Pointer to hfc_pkt	*/	/* FCGW 0331 */
	struct scsi_cmnd		*cmnd_pool;		/* Scsi_Cmnd Pointer for Task Management	*/
	struct scsi_cmnd		*ioctl_cmnd;	/* Scsi_Cmnd Pointer for ioctl	*/ /* FCLNX-GPL-0343 */
	struct dev_info			*ioctl_dev;		/* Dev_info Pointer for ioctl	*/ /* FCLNX-GPL-0343 */
	uint					pkt_cnt;		/* Number of using hfc_pkts		*/
	uint					pkt_no;			/* ID of the end hfc_pkt		*/
	uint					cmnd_cnt;		/* Number of using dummy scsi_cmnd	*/
	uint					cmnd_no;		/* ID of the end dummy scsi_cmnd 	*/
	uint					target_cnt;		/* Number of the Valid targets		*/
	uint					max_target;		/* Number of target_infos		*/
	struct target_info		*target_arg[MAX_TARGET_PROBE];	/* Pointers to target_infos	*/
	struct target_info		*login_target;	/* Target is in LOGIN wait state			*/
	struct target_info		*next_tstart;	/* Target is in PDISC wait state			*/
	uchar					next_gidpn;		/* GID_PN request exists					*/
	uchar					xob_exec_cnt;	/* Number of initiated xobs					*/
	uchar					xob_wait_exec_cnt;/* Number of xobs in wait 				*/
	uint					save_xob_outp[MAX_MAX_FRAME_CNT];   /* FCLNX-0359 */
	uint					xob_outp_end[MAX_MAX_FRAME_CNT];    /* FCLNX-0359 */
											/* Xob control area							*/
	uint					curr_xob_outp;	/* Outpointer to the current xob			*/
	uint					frame_chkp;		/* pointer to save_xob_outp 				*/
	uint					frame_inp;		/* pointer to save_xob_outp 				*/
	uint64_t				scsi_exec_cnt;	/* Total number of SCSI cmds 				*/
	uint64_t				scsi_end_cnt;	/* Initiated SCSI cmds						*/
	uint64_t				scsi_err_cnt;	/* SCSI cmds with error						*/

	spinlock_t				adap_lock;		/* lock to adap_lock,target_info			*/
	uint					mb_lock;		/* lock bit for mailbox process				*/
	uint					mb_sleep;
#define HFC_MAILBOX_BUSY		0				/*	This field is locked				*/
#define HFC_WAIT_LOCK_MB		1				/*	This field is locked				*/
	wait_queue_head_t   	mb_lock_event;	/* waitqueue until the lock bit would get	*/

	uint			mb_lock_event_wait;


	struct semaphore		ioctl_sem;		/* lock  for ioctl process					*/
	struct semaphore		sem;			/* adap_info lock for ioctl process			*/
	
	/* Timer watch */
	uint					timer_flag ;	/* Timer control flag			*/

	struct wtimer			mb_wdog;		/* Mailbox timeout							*/
	struct wtimer			linkup_wdog;	/* Linkup timeout 							*/
	struct wtimer			reboot_wdog;	/* Reboot timeout 							*/
	struct wtimer			mck_wdog;		/* Mck Recovery timeout 					*/
	struct wtimer			mckint_wdog;	/* Mck Interrupt timeout 		 FCLNX-0275 */
	struct wtimer			lgdelay_wdog;	/* Login Delay timer	 		 FCLNX-0270 */
	struct wtimer			mpap_lock_wdog;	/* mp_adap_info lock wait					*/
	struct wtimer			fwisol_wdog;	/* firmware isolation						*/
	struct wtimer			int_chk_wdog;	/* check interrupt timer */ /* FCLNX-GPL-306 */
// @MLPF
	struct wtimer			mckend_wdog;    /* MCK END recovery timer                   */
	struct wtimer			fcstp_wdog;     /* Force Check Stop INT timer               */
	struct wtimer           fmck_wdog;      /* Force MCK INT timer                      */
	struct wtimer			isolend_wdog;	/* ISOL END timer							*/
	struct wtimer			ld_err_wdog;	/* Linkup timeout for Link Down Error count *//* FCLNX-GPL-FX-424 */
	uint 					link_dead_cnt;	/* xrb=83,fsb=ccc or xrb v=0				*/
#define HFC_LINK_DEAD_CNT	5
	uint					pci_err_cnt;	/* PCI error count							*/
#define HFC_PCIERR_CNT		2
	uint					mck_err_cnt;	/* MCKINT error count						*/
#define HFC_MCKERR_CNT		8
	int						pcie_sram_ce_cnt;	/* PCIe IP Core SRAM ERR(CE)*/
#define HFC_PCIE_SRAM_CE_CNT	4
	int						core_ce_cnt;		/* Core 1bit ERR(CE) */
#define HFC_CORE_CE_CNT			9
	uchar					inta_dummy_read;	/* "1:Do" or "0:Not" dummy read.(for MSI/MSIX) */
#define HFC_DUMMY_READ_OFF		0
	uchar					debug_func;			/* bit0:CTLRST, bit1:Mem leak check */
#define HFC_DEBUG_CTLRST			0x80
#define HFC_DEBUG_MEM_LEAK			0x40
#define HFC_DEBUG_RW_BAR1			0x20		/* FCLNX-GPL-154 */
/* #define HFC_DEBUG_LINK_WIDTH_CHK	0x10 */		/* FCLNX-GPL-227 */ /* FCLNX-GPL-246 */
#define HFC_DEBUG_FIVE_MSI			0x08		/* FCLNX-GPL-228 */
#define HFC_DEBUG_POST_LOGOUT		0x04		/* FCLNX-GPL-236 */
#define HFC_DEBUG_HYP_INT_CHK		0x02		/* FCLNX-GPL-427 */
	uint				io_status;
#define HFC_LPTEST_ALRDY_INTREQ	0				/* Throw away INTRQ1					*/
#define HFC_HWLOG_VALID			1		 		/* HW LOG area valid					*/
	uchar					issue_d3hot;		/* change power state in suspend process */ /* FCLNX-GPL-306 */
	
	wait_queue_head_t		mb_event;		/* ioctl mailbox response wait  			*/
	wait_queue_head_t		ioctl_event;	/* ioctl mailbox lock wait					*/
	wait_queue_head_t		ioexe_event;	/* Mailbox/SCSI completion wait				*/
	wait_queue_head_t		mck_event;		/* MCK sleep during mailbox proc			*/
	wait_queue_head_t		init_event;		/* detect i/f during link init and LOGIN	*/

	atomic_t				int_a_poll;		/* interrupt polling  FCLNX_029				*/
	atomic_t				mb_event_wait;
	atomic_t				ioctl_event_wait;
	atomic_t				mck_event_wait;
	uint					mck_on_sleep;	

	
	struct test_dev_info	loop_dev_info[MAX_LOOP_LUN];
	uint			loop_dev_wait[MAX_LOOP_LUN];
										/* Loop test device info 					*/
	int						linkup_tmo;		/* Timer value for linkup wait				*/
#define HFC_LINKUP_TM_MIN	0
#define HFC_LINKUP_TM_MAX	60
	int						linkup2_tmo;	/* Timer value for linkup wait	(MCK)		*//*FCLNX-0241*/
	int						scsi_reset_delay;/* Supress SCSI after linkup				*/
#define HFC_SCSI_RESET_DELAY_MIN	0
#define HFC_SCSI_RESET_DELAY_MAX	60
	int						dma_max;		/* Max DMA transfer length					*/
	ushort					xob_max;		/* Max xob numbers							*/
	ushort					xrb_max;		/* Max xrb numbers							*/
	ushort					slog_max;		/* Max softlog numbers						*/
	uchar					pref_alpa;		/* AL-PA number during link_ini 			*/
	uchar					host_alpa;		/* AL-PA number when link_ini 				*/
											/* had finished								*/
	uchar					enable_tgtrst;	/* Target Reset Effective/invalidity					*/

	uint					fw_parm;		/* Firmware Parameter		*//* FCWIN-0192 */
#define HFC_FWP_SEGTRC_V		0x00000001	/* Valid SEG Fetch Trace	*//* FCWIN-0192 */

	uint					seq_num_tbl[((MAX_TARGET_PROBE+31)/32)];
											/* PSEQ number control table				*/
	uchar					tid_map[MAX_TARGET_PROBE];
											/* Table for translation from				*/
											/* TARGET ID to PSEQ number					*/
	uint					*iov_map;
											/* Bitmap of seg_info control				*/
	struct target_scan		*target_scan;	/* Control for target search				*/
	uchar					*errlog_buf;	/* Pointer to the error log area			*/
	uint					errlog_num;		/* Number of error logs						*/
	uint					errlog_max;		/* Max size of error log area				*/
	uchar                   mck_result;     /* mck execution result						*/
#define HFC_MCK_SUCCESS             0
#define HFC_MCK_HW_INIT             1
#define HFC_MCK_HWCHKSTOP           2
#define HFC_MCK_CNT_OVER            3
#define HFC_MCK_END                 4
#define HFC_MCK_EXEC                5
	struct hfctrace			*trc_ptr;		/* Pointer to the top trc area 				*/
	uint					trc_num;		/* Current pointer to trc area				*/
	uint					trc_max;		/* Max size of trc area						*/
#define HFC_MAX_TRCCNT  256					/* Max trace count	 						*/
	uchar					initialize;		/* Initialize() is being executed(!=0)		*/
	uchar					no_target;		/* Support Link_Ini Delay					*//* FCLNX-GPL-570 */
	ushort					next_dstart_cnt ;	/* Number of start waiting devices		*/
	struct	target_info		*next_dstart_top ;	/* Following Decu object device 		*/
												/* Head of cue 							*/
	struct	target_info		*next_dstart_end ;	/* following Decu object device 		*/

	/* for persistent bindings and common parameters*/
	uchar					automap;		/* Automap 0:Disable,1:Enable				*/
	uchar					defparam;		/* Forced Default Parameter 0:Dis,1:Enb     */
	uchar					topology;		/* 0:Auto,1:P2P,2:FC-AL 					*/
	uchar					linkspeed;		/* 1/2/4/10 Gbps							*/
	int 					max_mck_cnt;	/* Max MCKINT Error 						*/
#define HFC_MAX_MCK_CNT_MIN	0
#define HFC_MAX_MCK_CNT_MAX	10
	int						max_transfer;	/* Max transfer length 						*/
	int						target_reset_tmo;	/* target reset timeout 				*/
#define HFC_TARGET_RESET_TMO_MIN	0
#define HFC_TARGET_RESET_TMO_MAX	60
	int						abort_tmo;			/* abort timeout 						*/
#define HFC_ABORT_TMO_MIN	0
#define HFC_ABORT_TMO_MAX	60
	int						lun_reset_tmo;		/* lun reset timeout 					*//* FCLNX-GPL-0343 */
	int						queue_depth;	/* queue depth								*/
	int						wmsg;			/* message enable */
	int						pkt_num;		/* hfc_pkt num */
	int						can_queue;		/* can_queue */
	int						sg_tblsize;		/* sg tablesize */
	int						cmnd_num;		/* cmnd num */
	int						minus_tout;		/* minus timeout */
	int						scsi_allowed;	/* scsi allowed */
#define HFC_SCSI_ALLOWED_MIN	0
#define HFC_SCSI_ALLOWED_MAX	30
	int						ioctl_scsi_timeout;	/* ioctl timeout period */	/* FCLNX-GPL-0343 */
	int						cmd_per_lun;	/* cmd_per_lun	*/
	int						max_sectors;	/* max_sectors	*/
	int						vary_io;		/* vary_io		*/
	
	int						instance;
	int						unique_id;
	int						login_wait;		/* Login Delay Time */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#define	HFC_DEF_LOGIN_WAIT		0
#endif

#define HFC_LOGIN_DELAY_TIME_MIN	0
#define HFC_DF_LOGIN_DELAY_TIME		2
#define HFC_LOGIN_DELAY_TIME_MAX	60

	char					limit_log;		/* Limit Log *//* FCLNX-GPL-491 */
#define	HFC_DISABLE_LIMITLOG	0
#define	HFC_ENABLE_LIMITLOG		1
	char					filter_target;	/* Filtering Login Target *//* FCLNX-GPL-491 */
#define	HFC_DISABLE_FILTERTGT	0
#define	HFC_ENABLE_FILTERTGT	1
	char					hg_stats_disable;	/* Disable function to get statistics for Virtage *//* FCLNX-GPL-494 */
#define	HFC_ENABLE_HGSTATS		0
#define	HFC_DISABLE_HGSTATS		1
	struct hg_cca			*hg_cca_p;		/* uncached memory area shared Hypervisor *//* FCLNX-GPL-494 */
	dma_addr_t				padr_hg_cca;	/* psycal address of uncached memory area shared Hypervisor *//* FCLNX-GPL-494 */


	uint                    err_no;			/* It uses it with MCK LOGOU	*/
	uchar                   mode;			/* It uses it with MCK LOGOU	*/
	uchar					mck_type;		/* It uses it with MCK LOGOU	*/

	uint64_t				add_ww_name; 	/* HBA world wide port name					*/
	uint64_t				add_node_name; 	/* HBA world wide node name					*/
	uint64_t				org_ww_name; 	/* HBA world wide port name					*/
	uint64_t				org_node_name; 	/* HBA world wide node name					*/
	
	uint64_t				vfc_ww_name;	/* HBA world wide port name for MLPF		*//* @MLPF */
	uint64_t				vfc_node_name;	/* HBA world wide node name for MLPF		*//* @MLPF */
#if _HFC_DEBUG_MLPF_00
	uchar                   mmio_dummy[1024];
#endif
	uint					ioctl_lock;		/* lock bit for ioctl_event					*/
#define HFC_IOCTL_BUSY			0

	uint					diag_cnt;		/* Diag Exec. Count		*/	/* FCLNX-0293 */

	struct lg_target_info	*lg_target;		/* Chain lg_target in adap_info				*/

	struct hfc_pkt			*retry_hfcp_top;
	struct hfc_pkt			*retry_hfcp_end;
	int						retry_hfcp_count;
	uchar					host_scan;									/* FCLNX-0252 */
#define HFC_SCSI_HOST_RESCAN	0										/* FCLNX-0252 */
	uchar					mlpf_drv_log[64];

	uchar					narrowmap;		/* Narrow MAp 0:Disable,1:Enable FCLNX-0392 */
	struct {															  /* FCLNX-0392 */
		uint64_t			ww_name;
		uint				lun;
	}boot_priority[8];													  /* FCLNX-0392 */

	uchar					spinup_delay;			/* Spinup Delay Enable 				*//* FCLNX-0404 */
	uchar					opt_vendor_name[32];	/* Optical module vendor name		*//* FCLNX-0404 */
	uchar					opt_parts_number[32];	/* Optical module parts number		*//* FCLNX-0404 */
	uchar					opt_serial_number[32];	/* Optical module sirial number		*//* FCLNX-0404 */
	int						scsi_timeout_fail;		/* SCSI timeout fail count			*//* FCLNX-0404 */
	uint64_t 				inputrequests;			/* SCSI command count(read)			*//* FCLNX-0404 */
	uint64_t				outputrequests;			/* SCSI command count(write)		*//* FCLNX-0404 */
	uint64_t				controlrequests;		/* SCSI command count(not data move)*//* FCLNX-0404 */
	uint64_t				inputbytes;				/* number of bytes of read data		*//* FCLNX-0404 */
	uint64_t				outputbytes;			/* number of bytes of write data	*//* FCLNX-0404 */
	
	uint					scsi_time_out;			/* SCSI TIME OUT return code change */
	
	int						max_pcie_sram_ce_cnt;	/* Max PCIe IP Core SRAM ERR(CE) count */
	int						max_core_ce_cnt;		/* Max Core 1bit ERR(CE) count */
	int						max_hwlog_cnt;			/* Max hwlog count */
#define HFC_HWLOG_CNT			2
	
	struct manage_info		*manage_info;
	int						lun_reset_delay;		/* Supress SCSI after LUN Reset */	/*FCLNX-0506*/ /* FCLNX-GPL-038 */
#define HFC_LUN_DELAY			0
	int						abort_t_restrain;		/* Abort Task Set restrain */		/* FCLNX-0454 */
#define HFC_ABORT_T_RESTRAIN	0												/*FCLNX-0506*/
	int						login_restrain;		/* Abort Task Set restrain */		/* FCLNX-0454 */
#define HFC_LOGIN_RESTRAIN		0												/*FCLNX-0506*/
#define HFC_MAX_ERROR_NUM		1000											/*FCLNX-0506*/
	uint					retry_cnt[HFC_MAX_ERROR_NUM];	/*Threshold of SCSI Command Retry*/	/*FCLNX-506*/
#define HFC_MAX_ERR_RETRY		2048
#define HFC_MIN_ERR_RETRY		0
	uchar				c_err;					/* Factor of isolation - FCLNX-546 - */
#define HFC_ISOLATE_LDL			1
#define HFC_ISOLATE_LDS			2
#define HFC_ISOLATE_IF			3
#define HFC_ISOLATE_TO			4
#define HFC_ISOLATE_RT			5
#define HFC_ISOLATE_FA			6
#define HFC_ISOLATE_FP			7
#define HFC_ISOLATE_TGT_LDL		8					/* FCLNX-GPL-327 */
#define HFC_ISOLATE_TGT_LDS		9					/* FCLNX-GPL-327 */
#define HFC_ISOLATE_SHADOW		10					/* FCLNX-GPL-426 */
	struct wtimer			ldlerr_wdog;			/* Watchdog timer of Link-down Error */					/* FCLNX-0454 */
	struct wtimer			ldserr_wdog;			/* Watchdog timer of Link-down Error */					/* FCLNX-0454 */
	struct wtimer			iferr_wdog;				/* Watchdog timer of Link-down Error */					/* FCLNX-0454 */
	struct wtimer			toerr_wdog;				/* Watchdog timer of Link-down Error */					/* FCLNX-0454 */
	struct wtimer			rterr_wdog;				/* Watchdog timer of Link-down Error */					/* FCLNX-0454 */
	int						hba_isolation;  		/* FCLNX-GPL-349 */
#define HFC_ISOL_STOP			0					/* FCLNX-GPL-349 */
#define HFC_ISOL_START			1					/* FCLNX-GPL-349 */
	int						isol_type;				/* Type to isolate  */ 					/* FCLNX-0454 */
#define HFC_ISOL_TYPE_ADPT		0					/* FCLNX-710 */
#define HFC_ISOL_TYPE_PORT		1
	int					isol_force;			/* Forced isolation */ /* - FCLNX-546 - */
#define HFC_NO_FRC_ISOL			0 					/* Do not isolate */
#define HFC_PRT_FRC_ISOL		1 					/* Force port into isolated state */
#define HFC_CHKSTP_FRC_ISOL		2					/* Force adapter into isolated state */	/* FCLNX-GPL-147 */
#define HFC_SHARED_PRT_FRC_ISOL	3					/* Isolate physical port*/	/* FCLNX-GPL-393 */
	uint					ld_err_intvl;			/* Interval of Error Count for Link-down Error  */		/* FCLNX-0454 */
#define HFC_DF_LD_ERR_INTVL		30
#define	HFC_MIN_LD_ERR_INTVL	1
#define	HFC_MAX_LD_ERR_INTVL	60
	uint					ld_err_limit_l;			/* Maximum error count for long Link-down Error */		/* FCLNX-0454 */
#define HFC_LDLERR_NOT_SPPRTD	0
#define	HFC_MIN_LD_ERR_LIMIT_L	0
#define	HFC_MAX_LD_ERR_LIMIT_L	30
	uint					ld_err_limit_s;			/* Maximum error count for Short Link-down Error */ 	/* FCLNX-0454 */
#define HFC_LDSERR_NOT_SPPRTD	0
#define	HFC_MIN_LD_ERR_LIMIT_S	0
#define	HFC_MAX_LD_ERR_LIMIT_S	30
	uint					if_err_intvl;			/* Interval of Error Count for Short Link-down Error */	/* FCLNX-0454 */
#define HFC_DF_IF_ERR_INTVL		30
#define	HFC_MIN_IF_ERR_INTVL	1
#define	HFC_MAX_IF_ERR_INTVL	60
	uint					if_err_limit;			/* Maximum error count for Interface Error */ 			/* FCLNX-0454 */
#define HFC_IFERR_NOT_SPPRTD	0
#define	HFC_MIN_IF_ERR_LIMIT	0
#define	HFC_MAX_IF_ERR_LIMIT	2048
	uint					to_err_intvl;			/* Interval of Error Count for Short interface error */	/* FCLNX-0454 */
#define HFC_DF_TO_ERR_INTVL		30
#define	HFC_MIN_TO_ERR_INTVL	1
#define	HFC_MAX_TO_ERR_INTVL	60
	uint					to_err_limit;			/* Maximum error count for SCSI timeout error	*/		/* FCLNX-0454 */
#define HFC_TOERR_NOT_SPPRTD	0
#define	HFC_MIN_TO_ERR_LIMIT	0
#define	HFC_MAX_TO_ERR_LIMIT	2048
/* FCLNX-0542 */
	uint					rt_err_enable;			/* Maximum error count for Reset timeout error */		/* FCLNX-0454 */
#define HFC_RTERR_NOT_SPPRTD	0
#define	HFC_RT_ERR_NOT_SPPRTD	0
#define	HFC_RT_ERR_SPPRTD		1
/* FCLNX-0542 */
	uint					ld_err_count_s;			/* Error Counter for Short Link-down error */  /* FCLNX-GPL-349 */
	uint					if_err_count;			/* Error Counter for FC Interface error */  /* FCLNX-GPL-349 */
	uint					to_err_count;			/* Error Counter for SCSI T.O error */  /* FCLNX-GPL-349 */
	struct errcnt_info		*ldl_errcnt_info;		/* Counter for Long Link-down error */					/* FCLNX-0454 */
	struct errcnt_info		*lds_errcnt_info;		/* Counter for Short Link-down error */					/* FCLNX-0506 */
	struct errcnt_info		*if_errcnt_info;		/* Counter for FC Interface error */					/* FCLNX-0506 */
	struct errcnt_info		*to_errcnt_info;		/* Counter for SCSI T.O error */						/* FCLNX-0506 */
	
	short					login_retry;			/* Number of Retry for Login and Pdisc */				/* FCLNX-0523 */
	short					els_retry;				/* Number of Retry for ELS */							/* FCLNX-0523 */
	short					to_reset_retry;			/* Number of Retry for LOGIN for After SCSI T.O */		/* FCLNX-0523 */
	short					scsi_to_retry;			/* Number of Retry for SCSI Command  */					/* FCLNX-0523 */

	uint					raslog_install;			/* raslog support */
	uint					raslog_cnt;
	int						raslog_ver;
	int						raslog_rev;
	int						raslog_rver;
	int						raslog_wver;
	uchar					ioctl32;

	uchar					isol_detail;
#define HFC_NO_ISOLATE				0x00
#define	HFC_ISOLATE_PORT_C			0x01		/* FCLNX-0537 */	/* FCLNX-GPL-147 */
#define	HFC_ISOLATE_PORT_E			0x02		/* FCLNX-0537 */	/* FCLNX-GPL-147 */
// #define	HFC_HWISOLATE_HBA_C			0x83		/* FCLNX-0537 */	/* FCLNX-GPL-147 */
// #define	HFC_HWISOLATE_HBA_E			0x84		/* FCLNX-0537 */	/* FCLNX-GPL-147 */
#define	HFC_ISOLATE_SFPFAIL			0x05
#define	HFC_ISOLATE_SFPNOTSUPPORT	0x06
#define	HFC_ISOLATE_SFPDOWN			0x07
#define HFC_ISOLATE_CHKSTP			0x08	/* FCLNX-GPL-147 */
#define HFC_ISOLATE_CHKSTP_C		0x09	/* FCLNX-GPL-147 */
	uint					wait_isol;		/* FCLNX-GPL-413 */
#define HFC_WAIT_ISOL_ERR		0			/* FCLNX-GPL-413 */
#define HFC_WAIT_ISOL_CMD		1			/* FCLNX-GPL-413 */	
#define HFC_WAIT_ISOL_REC		2			/* FCLNX-GPL-413 */	

#define HFC_RASLOG_RETRY	3

	uint64_t		adapt_wwpn[4];	/* FCLNX-0506 */	/* FCLNX-0542 */


	uchar					login_type;	/* FCLNX-0571 */
#define HFC_SCAN_DEVICE				0		/* FCLNX-0571 */
#define HFC_CANCEL_IO				1		/* FCLNX-0571 */
#define HFC_CANCEL_LOGIN			2		/* FCLNX-0571 */

	uint				mck_linkup;		/* FCLNX-595 */
#define HFC_LINKUP_NOMCK			0
#define HFC_LINKUP_MCK				1

        uchar                           mck_point;      /* FCLNX-0535 */
#define HFC_NO_MCK_POINT                        0       /* FCLNX-0535 */
#define HFC_BEFORE_POSTCHK                      7       /* FCLNX-0535 */
#define HFC_BEFORE_LINKINITIALIZE               12      /* FCLNX-0535 */
#define HFC_AFTER_LINKINITIALIZE                13      /* FCLNX-0535 */
#define HFC_AFTER_SCSI_HOST_RESCAN              18      /* FCLNX-0535 */
#define HFC_MAX_MCK_POINT                       255     /* FCLNX-0535 */

	uint64_t dma_mask;									/* FCLNX-0671 */;

	uchar	log_wk[512];							/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	uchar	detail_data[HFC_RASLOG_LEN];			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	struct hfc_err_rec		err_rec;				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

#define HFC_MSIX_NVEC			2		/* The number of MSI-X Vectors */
	struct hfc_msix_entry	entries[HFC_MSIX_NVEC]; /* renamed to avoid kernel conflict */
	int msi_enable; /* Set INT type. 0:INTx 1:MSI 2:MSI-X */
	int msi_flag; /* Running INT type. 0:INTx 1:MSI 2:MSI-X */
	
	/* [FIVE-EX] We use these values to stock data of before H/W initialize. */
	uint		hw_init_status0;
	uint		hw_init_status1;
	uint		hw_init_detail0;
	uint		hw_init_pcierr;

	/* FIVE-EX SRAM CE log area */ /* FCLNX-GPL-116 */
	hfc_err1bit_logout_t	ce_log;
	
	uint		xob_full_cnt;		/* FCLNX-GPL-143 */
	uint		iovmap_full_cnt;	/* FCLNX-GPL-143 */
	uint		frame_full_cnt;		/* FCLNX-GPL-143 */
	uint		page_over_cnt;		/* FCLNX-GPL-143 */
	
	uchar		mck_progress;
#define HFC_MCK_PROGRESS	0x80

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* for kthread */
	uchar					kthread_status;
#define HFC_KTHREAD_RUN			0
	struct task_struct		*worker_thread;
	wait_queue_head_t		rport_event;
	atomic_t				rport_event_wait;	/* FCLNX-GPL-259 */
	/* FCLNX-GPL-205 */
	
	/* for port statistics */
	struct fc_host_statistics	port_statistics;
	ulong						reset_stat_time;
	uchar						sysfs_control;
#define HFC_SYSFS_RPORT			0
#define HFC_SYSFS_STATISTICS	1
	
	/* for rport */
	uint						dev_loss_tmo;	/* FCLNX-GPL-260 */
#define HFC_MIN_DEV_LOSS_TMO	1				/* FCLNX-GPL-260 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#define HFC_DEF_DEV_LOSS_TMO    15				/* FCLNX-GPL-616 */
#endif
#define HFC_MAX_DEV_LOSS_TMO	INT_MAX			/* FCLNX-GPL-260,616 */ /* INT_MAX = 2147483647 */

	int						scan_finished_tmo;	/* FCLNX-GPL-565 *//* FCLNX-GPL-575 */
#define HFC_SCAN_FINISHED_TMO		30			/* FCLNX-GPL-565 */
#define HFC_MIN_SCAN_FINISHED_TMO	0			/* FCLNX-GPL-565 */
#define HFC_MAX_SCAN_FINISHED_TMO	3600		/* FCLNX-GPL-565 */

#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

	uint						rport_lu_scan;	/* FCLNX-GPL-575 */
#define HFC_DISABLE_RPORT_LU_SCAN 0				/* FCLNX-GPL-575 */
#define HFC_ENABLE_RPORT_LU_SCAN  1				/* FCLNX-GPL-575 */

	uint64_t				tx_frames; 		/* FCLNX-GPL-261 Statistical information */
	uint64_t				rx_frames; 		/* FCLNX-GPL-261 Statistical information */
	uint64_t				tx_words; 		/* FCLNX-GPL-261 Statistical information */
	uint64_t				rx_words; 		/* FCLNX-GPL-261 Statistical information */

	uint mb_prol_wake_up_time; /* FCLNX-GPL-243 */
	uint mb_prol_sleep_end_time; /* FCLNX-GPL-243 */
	
	int			int_check; 		/* FCLNX-GPL-306 */
	uint		is_busmaster:1; /* This device is busmaster */ /* FCLNX-GPL-306 */
	int			pm_event;		/* FCLNX-GPL-306 */
	uint		fw_support;	/* FCLNX-GPL-311 */
#define HFC_SUPPORT_FW_ISOL			0	/* HBA FW Support bit Isolate Operation & SFP Inf *//* FCLNX-GPL-393 */
#define HFC_SUPPORT_HVM_ISOL		1	/* HVM FW Support bit *//* FCLNX-GPL-393 */
#define HFC_SUPPORT_LINKINI_DELAY	2	/* Link Ini Delay Support bit *//* FCLNX-GPL-570 */

	uint		ctl_change_qdepth;		/* FCLNX-GPL-574 */
#define HFC_ENABLE_CTL_CHANGE_QDEPTH 1	/* FCLNX-GPL-574 */
#define HFC_DISABLE_CTL_CHANGE_QDEPTH 0	/* FCLNX-GPL-574 */

	uchar		isol_err_mck_cnt;	/* FCLNX-GPL-357 */
	uchar		isol_cmd_mck_cnt;	/* FCLNX-GPL-357 */
	uchar		logdata[16] ;	/* FCLNX-GPL-391 */
	
	uchar		log_file;		/* FCLNX-GPL-547 */
	int			max_lun;		/* FCLNX-GPL-547 */

	uchar		*hfclddconf;
	uchar		drvctl;				/* FCLNX-GPL-FX-366 */
#define HFC_LOGIN_TARGET_FILTER_EXT	0x10

	atomic_t	selected_target_id;		/* for sysfs 'selected_target_id' attribute */
#define HFC_INVALID_SELECTED_TARGET_ID	-1
};

/* 
 * mp_adap_info : adapter information
 *
 *  allocated by hfc_detect()
 *  released  by hfc_release()
 *
 */
struct mp_adap_info {
	uint					lock;			/* HBA status information for all ports 	*/
/*#define HFC_NON_STATUS		0			* HBA is not in process in the system 		*/
#define HFC_HW_INITIALIZE		1			/* HW initialization is in progress 		*/
#define HFC_HMCK_RECOVRTY		2			/* MCK Recovery by Master is in progress 	*/
#define HFC_MLOCK_LOGOUT		3			/* HWLOG is being acquired					*/
#define HFC_MLOCK_ERRLOG		4			/* ERRLOG is being acquired					*/
#define HFC_MLOCK_CHKSTOP		5			/* CHKSTOP is being acquired				*/

	uint					status;			/* HBA status flag for all ports 			*/
#define HFC_HWINIT_COMP			0			/* HW init. has already finished 			*/
#define HFC_HWINIT_FAIL			1			/* HW init. ended with error 				*/
#define HFC_DIAG_PROGRESS		2			/* Diag progress			 				*/
#define HFC_HWCHKSTOP			3			/* HW is in check stop state				*/
#define HFC_EXEC_PCI_RESET  	4			/* State of PCI ERROR RECOVER				*/
/* #define HFC_HWLOG_SAVING		5 */		/* H/W LOG is being acquired */ /* FCLNX-GPL-111 */
//#define HFC_WRITE_PROCESS		6			/* FW write process is in progress			*/
#define HFC_CTLRST_PROCESS		7			/* CTLRST process is in progress			*/ /* FCLNX-0287 */
#define HFC_IOCTL_WR_CHECK		8			/* Write process is in progress	at ioctl	*/
#define HFC_SUBMCK_RECOVERY		9			/* MCK Recovery by Sub is in progress		*/ /* FCLNX-0493 */
#define HFC_HWISOL				10			/* H/W is into isolation status */ /* FCLNX-0506 */
#define HFC_HWISOL_RECOVERY		11			/* H/W isolation recovery process is in progress */ /* - FCLNX-546 - */ 
#define HFC_FLASH_UPDATE_PROGRESS 12		/* State of Flash Updating					*//* FCLNX-GPL-613 */

	uint					tbl_lock;		/* lock bit for manage_info and adap_info	*/
#define HFC_MP_ADAP_BUSY	0				/* mp_adap_info is locked 					*/

	wait_queue_head_t   	lock_event;	/* waitqueue until the lock bit would get		*/
	
	uint					mck_result;		/* MCK process has already finished			*/
#define HFC_MCK_SUCCESS            	0
#define HFC_MCK_HW_INIT            	1
#define HFC_MCK_HWCHKSTOP          	2
#define HFC_MCK_CNT_OVER           	3
#define HFC_MCK_END                	4
#define HFC_MCK_EXEC               	5

	ushort					port_cnt;		/* HBA Port number per HBA 					*/
											/*	1(single)/2(dual)/4						*/
	int						mck_err_cnt ;	/* MCKINT Error 				*/
	int						max_mck_cnt;	/* Max MCKINT Error FCWIN-0081  */
#define HFC_MCKERR_CNT		8

	struct adap_info		*ap;			/* Pointer to the top adap_info entry		*/
	struct adap_info		*locked_ap;		/* Pointer to the adap_info which is not 	*/

	struct port_info		*pp;			/* Pointer to the top port_info entry		*/
	struct port_info		*locked_pp;		/* Pointer to the port_info which is not 	*/

											/*	in normal state							*/
	uint					mck_seq_no;		/* MCK Log sequence number					*/
	uint					open_cnt;		/*											*/
	uint					*hw_log;
#define HFC_HWLOG_SIZE          0x4000		
	struct mp_adap_info		*next;			/* Pointer to the previous mp_adap_info		*/
	struct mp_adap_info		*prev;			/* Pointer to the next mp_adap_info			*/
/*	uint					hwlog_result;*/	/* Hwlog acquisition flag */ /* FCLNX-GPL-111 */
	uint					sys_rev;		/* Firmware system revision					*/
	uchar					vpd_buf[512];	/* FPP VPD information data area			*/
	uchar					adap_id[16];	/* Adapter info. identification number		*/
	uchar					model_name[16]; /* Model Name								*/	/* FCLNX-0292*/
	struct file				*open_file;		/* File address of write processing				*/
	uchar					core_no;		/* code no */ /* FIVE-EX */
};


/* 
 * hfc_manage_info : driver information
 *
 *  allocated by hfc_detect()
 *  released  by hfc_release()
 *
 */
struct manage_info{
	uchar					name[16];  			/* Set "manage_info" characters			 	*/
	uint					status;				/* HBA status flag for all ports 			*/
#define HFC_WRITE_PROCESS		6				/* write process is in progress				*/
	uchar					package_ver[16];  	/* Driver Package version 					*/
	ushort					mp_adap_cnt;      	/* Registered HBA number  					*/
	uint					adap_cnt;			/* the number of adap_info					*/
	uint					port_cnt;			/* the number of port_info					*/
	uint					card_cnt;			/* Installed Card number					*/
	struct mp_adap_info 	*mp_adap_info;		/* Pointer to mp_adap_info					*/
	int						major;				/* Major number (device special file)		*/
	struct adap_info		*adap_info_arg[MAX_ADAP_CNT];
												/* Pointer to adap_info reffered by minor	*/
												/*  number									*/
	struct port_info		*port_info_arg[MAX_ADAP_CNT];
												/* Pointer to port_info reffered by minor	*/
												/*  number									*/
	uint64_t 				adap_bind[MAX_ADAP_CNT]; /* binding info for adapter			*/
	struct port_info		*pp;				/* Pointer to the top port_info entry		*/
	int						instance;
	struct lg_path_info		*lg_path_info;		/* Pointer to lg_path_info					*/
	struct lg_target_info	*lg_target_info;	/* Pointer to lg_target_info				*/
	struct nonpub_symbol_list	*npubp;
	struct pub_symbol_list	*pubp;
	struct mp_manage_info	*mp_manage_info;
	struct scsi_host_template	*hfcldd_driver_template;
	struct scsi_host_template	*hfcldd_driver_template_mp;
	struct scsi_host_template	*hfcldd_driver_template_platform_bus;
	uchar  					hfcldd_mp_mod;		/* hfcldd_mp module state					*/
#define HFCLDD_MP_MOD_ENABLE		0x80
#define HFCLDD_MP_MOD_DISABLE		0x00
	uchar  wait_reset_mp;						/* Wait Device Reset on hfcmp				*//* FCLNX-0429 */
	uchar  wait_reset_mp_fx;					/* FIVE-FX */
#define HFCLDD_MP_DEVRESET			0x80													  /* FCLNX-0429 */
#define HFCLDD_MP_BUSRESET			0x40													  /* FCLNX-0429 */
	uchar					hfcplus_enable; 	/*Phase.2 Option */ 	/* FCLNX-0454 */
	struct Scsi_Host		*hsd_host;			/* Phase.3 Hot Plug Support */ /* FCLNX-0651 */
	struct Scsi_Host		*pfb_host;			/* Phase.3 Hot Plug Support */ /* FCLNX-GPL-204 */
	struct Scsi_Host		*hsd_host_fx;		/* FIVE-FX */
	struct Scsi_Host		*pfb_host_fx;		/* FIVE-FX */
	uchar					nm;					/* =1:Enable n/m alter path, n/m offline path	 FCLNX-0677 *//* FCLNX-710 */

	spinlock_t				hfcmp_lock;			/* lock to lg_xxxx_info						*/
	spinlock_t				hfcmp_fx_lock;		/* lock to lg_xxxx_info						*/
	struct semaphore		sem;				/* lock for manage_info and adap_info		*/

	atomic_t				kmalloc_cnt;		/* Memory Leak check(kmalloc) */
	atomic_t				kmalloc_cnt_ap[MAX_ADAP_CNT];
	atomic_t				dma_alloc_cnt;		/* Memory Leak check(dma_alloc) */
	atomic_t				dma_alloc_cnt_ap[MAX_ADAP_CNT];
	atomic_t				pci_alloc_cnt;		/* Memory Leak check(pci_alloc_cnt) */
	atomic_t				pci_alloc_cnt_ap[MAX_ADAP_CNT];
	atomic_t				host_alloc_cnt;		/* Memory Leak check(host_alloc) */
	
	struct slot_ww_name		slot_add_wwn[MAX_ADAP_CNT]; /* add world wide port name */ /* FCLNX-GPL-201 */
	uint					dev_cnt;			/* MAX DEV CNT						*/ /* FCLNX-GPL-201 */

	uint					open_status;		/* Availability of hfc_open func		*/
#define HFC_OPENED				1				/* diag_open() has finished				*/
#define HFC_CLOSED				0				/* diag_close() has finished			*/
#define HFC_DIAG_OPENED			2
	uint					open_cnt ;			/* Number of opens						*/
	struct file				*open_file;			/* File address of write processing		*/
	
	uchar					log_file;			/* FCLNX-GPL-547 */
	atomic_t				selected_irq;					/* for sysfs 'selected_irq' attribute */
#define HFC_INVALID_SELECTED_IRQ	-1

	ushort					socket_num;			/* Number of Socket					*/
	ushort					cpu_core_num;		/* Number of Physical CPUs			*/
	uint					logical_cpu_num;	/* Number of logical CPUs			*/
	uint					online_cpu_num;		/* Number of online CPUs		 	*/

#define HFC_MAX_CPU_NUM 4096
	struct socket_info		*socket_info;
};


/*----------------------------------------------------------------------------*/
/*---- "dddu command" device tree structure 							  ----*/
/*----------------------------------------------------------------------------*/
/* Mp_adap_info display of information */
struct m_info {
	uint				lock ;
	uchar				status ;
	uint				mck_result ;		/* Flag that has executed MCK		*/
	ushort				port_cnt ;			/* Number of adap_info in mp_adap_info*/
	uint				mck_seq_no ;		/* MCK Log Seq. No.				*/
	uint				sys_rev ;			/* FW SYSREV					*/
};

/* Adap_info display of information */
struct a_info {								/* Adap_info display information*/
	uint				status ;			/* State of adaptor			*/
	uint64_t			scsi_id ;			/* Scsi_id of adaptor			*/
	uint64_t			ww_name ;			/* Ww_name of adaptor		*/
	uint64_t			node_name ;			/* Node_name of adaptor		*/
	uchar				connect_type ;		/* Connection type		*/
	uint				max_data_rate ;		/* Maximum forwarding Rate				*/
	uchar				port_no ;			/* Port# in mp_adap_info(arbitrariness)*/
	uchar				attach_status ;
	uchar				open_status ;		/* Right or wrong of hfc_open(initial value '0') */
	uchar				mb_status ; 		/* Mailbox start level		*/
	uchar				mb_resp ;			/* Mailbox response reception		*/
	int 				mb_results ; 		/* Mailbox result(Error Code)	*/
	int					mb_retry_cnt ;		/* Retrycounter of mailbox start*/
	ushort				xob_no ;			/* Xob number that xob_inp indicates	*/
	ushort				xrb_no ;			/* Xrb number that xrb_outp indicates	*/
	uint				iov_no ;			/* Seg_info list retrieval No		*/
	uint				iov_map_cnt;		/* Number of bits of iov_map	*/
	uint				target_cnt; 		/* Number of effective targets		*/
	uchar				xob_exec_cnt ;		/* Number of xob starts			*/
	uchar				xob_wait_exec_cnt ;
											/* Xob queuing end and number of start waiting*/
	uint				save_xob_outp[MAX_MAX_FRAME_CNT] ;  /* FCLNX-0359 */
	uint				curr_xob_outp ;		/* Present xob_outp				*/
	uint				frame_chkp ;
	uint				frame_inp ;
	uint64_t			scsi_exec_cnt ; 	/* Number of SCSI starts				*/
	uint64_t			scsi_err_cnt ;		/* Number of SCSI errors(b_error!=0)	*/
	uint				link_dead_cnt ;	 	/* xrb=83 or fsb=ccc or xrb v=0 */
	uint				pci_err_cnt ;		/* PCI Error					*/
	uint				mck_err_cnt ;		/* MCKINT Error 				*/
	uint				ioctl_status ;
	uint				io_status ;
	uint				trc_num ;			/* Trace area current number	*/
	ushort				next_dstart_cnt ;

	uint				xob_outp_end[MAX_MAX_FRAME_CNT] ;   /* FCLNX-0359 */
	uint64_t			scsi_end_cnt;
	uint				pkt_no;
	uint				pkt_cnt;

	uint				xob_full_cnt;		/* FCLNX-GPL-143 */
	uint				iovmap_full_cnt;	/* FCLNX-GPL-143 */
	uint				frame_full_cnt;		/* FCLNX-GPL-143 */
	uint				page_over_cnt;		/* FCLNX-GPL-143 */



};

/* Target_info display information */
struct t_info {
	uint				entry_num ;			/* Number of effective targets	*/
	uint				status ;			/* State of target		*/
	uchar				flags ;
	uchar				pseq ;				/* Each target is allocated within */
											/* range of 0-255	*/
	ushort				device_flags ;
	uint				target_id ;			/* target_id			*/
	uint64_t			scsi_id ;			/* Destination ID		*/
	uint64_t			ww_name ;			/* World Wide Port Name */
	uint64_t			node_name ;			/* World Wide Node Name */

	/* SCSI command response waiting cue */
	int					we_que_cnt ;		/* WE count			*/
	int					wx_que_cnt ;		/* WE count			*/
	uchar				next_dstart_flag ;
};

#ifdef _HFC_PCM
#include "hfcldd2.h"
#include "hfcldd2_fx.h"
#else
/* dev_info */
//struct dev_info {};

/* lg_path_info */
struct lg_path_info {};

/* lg_path_info1 */
struct lg_path_info1 {};

/* lg_target_info */
struct lg_target_info {};

/* lg_dev_info */
struct lg_dev_info {};
/* lg_target_info_fx */
struct lg_target_info_fx {};

/* lg_dev_info_fx */
struct lg_dev_info_fx {};
#endif // _HFC_PCM

struct narrow_dev {											  /* FCLNX-0392 */
	struct {
		uint64_t	ww_name;
		uint		bus;
		uint		devfn;
	}ap;
	
	struct {
		uint64_t	ww_name;
		uint		lun;
	}tgt;
	
	uchar priority;
};															  /* FCLNX-0392 */

struct mp_manage_info {
	uchar				name[16];  		/* Set "mp_manage_info" characters	*/
	struct manage_info	*manage_info;
	struct module		*hfcldd_mp;
	int					instance;							/* FCLNX-GPL-204 */
};

#ifdef __KERNEL__

#define HFC_VRTHOST_ALLOC_SUCCS	0	/* FCNLNX-GPL-473 */
#define HFC_VRTHOST_ALLOC_EXIST	1	/* FCNLNX-GPL-473 */
#define HFC_VRTHOST_ALLOC_FAIL	2	/* FCNLNX-GPL-473 */

struct dummy {
	struct port_info		*pp;
	struct region_info		*rp;
	struct core_info		*core;
	struct target_info_fx	*target;
	struct dev_info_fx		*dev;
	struct hfc_pkt_fx		*hfcp;
	struct wtimer_fx		*w_timer;
};

struct nonpub_symbol_list {
	/* detect */
	struct mp_manage_info *(*hfc_get_mp_manage_info) (void);
	int (*hfc_check_hop) (int mp);														/* FCLNX-0429 */
	int (*hfc_mp_scan_dev) (struct adap_info *ap);										/* FCGW 0331 */
	void (*hfc_make_lgpath) (void);
	void (*hfc_remove_lgpath) (struct adap_info *ap);
	void (*hfc_update_lgtarget) (uchar instance, uchar attribute, uchar tid, uchar group, struct lg_target_info *lgtp);
	void (*hfc_update_lgdev) (struct adap_info *ap, struct lg_target_info *lgtp, uchar group, uchar tid);
	int (*hfc_search_and_update_target) (struct adap_info *ap, uchar tid, uchar pathid, uchar group, uchar attribute, struct lg_target_info *lgtp);
	int (*hfc_search_and_update_dev) (struct adap_info *ap, struct dev_info *device, struct dev_info *old_device, uchar group, uchar tid, uint lun, uchar path_cnt);	/* FCLNX-GPL-0449 */
	struct dev_info *(*hfc_search_dev_compare) (struct dev_info *cur_dev,int inst_old, int tg_old);
	void (*hfc_automap_lgpath) (ulong flags);
	void (*hfc_mp_scsi_host_rescan) (void);
	void (*hfc_free_mp_table) (void);
	int (*hfc_wait_mp_ioend) (struct adap_info *ap);	/* FCLNX-0459 */
	int (*hfc_write_retries) (struct dev_info *dev);	/* FCLNX-GPL-0449 */
	void (*hfc_update_attribute) (struct adap_info *ap, struct target_info *target);
	char *(*hfc_proc_info_lun) (struct adap_info *ap, struct target_info *tp, char *data, off_t offset, int length, int *posp);	/* FCLNX-GPL-FX-483 */
	char *(*hfc_proc_info_option) (struct adap_info *ap, char *data, off_t offset, int length, int *posp);
	void (*hfc_fo_check_and_offline) (struct target_info *target, int pmsg);
	void (*hfc_forced_offline_e) (struct target_info *target, int pmsg);
	void (*hfc_forced_offline_c) (struct target_info *target, int pmsg);	/* FCLNX-GPL-147 */
	int (*hfc_mp_queue_depth) (struct scsi_device *sdev, int *queue_depth);	/* FCGW 0331 */ /* FCLNX-GPL-255 */
	void (*hfc_isolparam_setup) (void);						/* FCLNX-0542 */
	void (*hfc_isolconf_setup) (struct adap_info *ap);		/* FCLNX-0542 */
	int (*hfc_allocate_errcnt_info) (struct adap_info *ap, int flags);	/* FCLNX-0542 */
	void (*hfc_free_errcnt_info) (struct adap_info *ap);	/* FCLNX-0542 */
	void(*hfc_set_retry_cnt) (struct adap_info *ap);		/* FCLNX-0542 */
	int (*hfc_mp_module_init) (int instance, char *hfcldd_conf, int automap, int narrowmap);	/* FCLNX-GPL-204 */
	void (*hfc_mp_set_parm)(struct Scsi_Host *shost, struct adap_info *ap, 
							struct Scsi_Host *hsd_host, struct Scsi_Host *pfb_host, uint64_t dma_mask);	/* FCGW 0331 */						/* FCLNX-GPL-204 */
	int (*hfc_mp_scsi_host_alloc) (struct scsi_host_template *temp, 
					struct Scsi_Host *shost, struct adap_info *ap, struct Scsi_Host **hsd_host, struct Scsi_Host **pfb_host);
	int (*hfc_mp_scsi_add_host) (struct Scsi_Host *shost, struct adap_info *ap, struct Scsi_Host *hsd_host, struct Scsi_Host *pfb_host);	/* FCLNX-GPL-204 */
	int (*hfc_mp_scsi_scan_host) (struct Scsi_Host *shost, struct adap_info *ap, struct Scsi_Host *hsd_host, struct Scsi_Host *pfb_host);	/* FCLNX-GPL-204 */
	void (*hfc_mp_scsi_remove_host) (struct Scsi_Host *host);									/* FCLNX-GPL-204 */
	int  (*hfc_free_and_allocate_errcnt_info)(struct adap_info *ap);	/* FCLNX-GPL-FX-314 */

	/* strategy */
	int (*hfc_mp_strategy) (struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
	int (*hfc_mp_abort) (struct scsi_cmnd *cmnd);
	int (*hfc_mp_device_reset) (struct scsi_cmnd *cmnd);
	int (*hfc_mp_target_reset) (struct scsi_cmnd *cmnd);						/* FCLNX-GPL-0449 */
	int (*hfc_mp_bus_reset) (struct scsi_cmnd *cmnd);
	int (*hfc_retry_strategy) (struct adap_info *phys_ap);
	void (*hfc_ioerror_check) (struct scsi_cmnd *cmnd, struct hfc_pkt *hfcp);
	int (*hfc_queue_check) (struct hfc_pkt *hfcp);								/* FCGW 0331 */
	void (*hfc_queue_count) (struct hfc_pkt *hfcp, int we, int deq);
	int (*hfc_mp_iodone) (struct scsi_cmnd *cmnd, struct adap_info *ap, struct hfc_pkt *hfcp);
	int (*hfc_check_io_reset_complete)(struct hfc_pkt *hfcp);
	void (*hfc_check_dev_reset_complete) (void);								/* FCLNX-0429 */
	void (*hfc_check_bus_reset_complete) (void);								/* FCLNX-0429 */
	void (*hfc_make_fcinfo) (struct adap_info *ap, struct hfc_pkt *hfcp, 
				unsigned int err_no, unsigned int errcode, uchar *err_detail, int err_detail_cnt);
	void (*hfc_set_scsi_cmd_tmr) (struct adap_info *ap, struct target_info *target, struct hfc_pkt *hfcp);	/* FCLNX-GPL-0449 */
	
	/* top */
	void (*hfc_convert_rptluns) (struct lg_target_info *lgtp, struct scsi_cmnd *, struct adap_info *);		/* FCLNX-0611 */
	void (*hfc_check_luconfig) (struct target_info *target, struct scsi_cmnd *cmnd);							/* FCLNX-0611 *//* FCLNX-GPL-0449 */
	
	/* timer_recovery */
	int (*hfc_errlog_mp) (struct hfc_pkt *hfcp);
	void (*hfc_watched_errcount)(struct adap_info *ap, struct target_info *target, uchar flag, uchar err_flag);	/* FCLNX-0542 *//* FCLNX-GPL-327 */
	void (*hfc_clear_errinfo)( struct adap_info *ap );								/* FCLNX-GPL-147 */
	
	/* ioctl */
	int (*hfc_ioctl_mp) (struct adap_info *ap, unsigned int cmd, void *arg);
	int (*hfc_mp_rd_param) (struct adap_info *ap, void *arg );
    int (*hfc_mp_wr_param) (struct adap_info *ap, void *arg );
	int (*hfc_mp_lu_map) (struct adap_info *ap, void *arg );
	int (*hfc_mp_setpath) (struct adap_info *ap, void *arg );
	int (*hfc_mp_path_health) (struct adap_info *ap, void *arg );
	int (*hfc_mp_lgtarget_map) (struct adap_info *ap, void *arg );
	int (*hfc_mp_lgpath_info1) (struct adap_info *ap, void *arg );
	int (*hfc_read_isolparam) (struct adap_info *ap);				/*FCLNX-0542*/
	int (*hfc_read_retry_cnt) (struct adap_info *ap);               /*FCLNX-0542*/
	int (*hfc_get_isolparam) ( struct adap_info *ap, struct hfc_isol_info *isolinfo );	/* FCLNX-GPL-147 */

	/* diag */
	int (*read_dev_info_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	int (*read_lg_target_info_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	int (*read_lg_dev_info_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	int (*read_lg_path_info_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	int (*read_lg_path_info1_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	int (*read_lg_path_info2_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	int (*read_failover_info_mp) (struct adap_info *ap, struct diag_ioctl *diag);
	/* FCLNX-0429 */
	int (*hfc_mp_proc_info_pfb) (struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
	int (*hfc_mp_proc_info_pfb_com) (struct Scsi_Host *host, char *buffer, off_t offset, int length, uint proc_type);	/* FCLNX-GPL-FX-481 */
	char *(*hfc_mp_info_pfb) (struct Scsi_Host *host);
	struct Scsi_Host * (*hfc_mp_search_pfb_host) (struct Scsi_Host *shost, struct adap_info *ap);
	int (*hfc_mp_strategy_pfb) (struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
	int (*hfc_mp_abort_pfb) (struct scsi_cmnd *cmnd);
	int (*hfc_mp_device_reset_pfb) (struct scsi_cmnd *cmnd);
	int (*hfc_mp_lun_reset) (struct scsi_cmnd *cmnd);							/* FCLNX-GPL-0449 */
	int (*hfc_mp_target_reset_pfb) (struct scsi_cmnd *cmnd);					/* FCLNX-GPL-0449 */
	int (*hfc_mp_bus_reset_pfb) (struct scsi_cmnd *cmnd);
	
	void (*hfc_mp_hsd_enable) (void);
	int (*hfc_mp_scsi_scan_target) (struct Scsi_Host *shost,
			 unsigned int channel, unsigned int scsi_id, unsigned int lun, int rescan);
	int (*hfc_mp_scsi_adjust_queue_depth) (struct scsi_device *sdev, int tagged, int tags);
	int (*hfc_mp_scsi_host_put) (struct Scsi_Host *shost);
	int (*hfc_mp_scsi_remove_device) (struct scsi_device *sdev);
	struct scsi_device * (*hfc_mp_scsi_device_lookup) (struct Scsi_Host *shost, uint channel, uint scsi_id, uint lun);
	int (*hfc_mp_scsi_block_requests) (struct Scsi_Host *shost);
	int (*hfc_mp_scsi_unblock_requests) (struct Scsi_Host *shost);

	/* version */
	char *(*hfc_get_pcm_rcsid) (void);

	/* FIVE-FX */
	int (*hfc_fx_mp_scan_dev) (struct port_info *pp);										/* FCGW 0331 */
	void (*hfc_fx_make_lgpath) (void);
	void (*hfc_fx_remove_lgpath) (struct port_info *pp);
	void (*hfc_fx_scsi_rescan_check) (void);
	void (*hfc_fx_mp_scsi_host_rescan) (void);
	int (*hfc_fx_wait_mp_ioend) (struct port_info *pp);	/* FCLNX-0459 */
	int (*hfc_fx_write_retries) (struct dev_info_fx *dev);	/* FCLNX-GPL-0449 */
	void (*hfc_fx_update_attribute) (struct port_info *pp, struct target_info_fx *target);
	char *(*hfc_fx_proc_info_lun) (struct port_info *pp, struct target_info_fx *tp, char *data, off_t offset, int length, int *posp);	/* FCLNX-GPL-FX-483 */
	char *(*hfc_fx_proc_info_option) (struct port_info *pp, char *data, off_t offset, int length, int *posp);
	void (*hfc_fx_free_dev) (struct target_info_fx *target);
	void (*hfc_fx_fo_check_and_offline) (struct target_info_fx *target, int pmsg);
	void (*hfc_fx_forced_offline_e) (struct target_info_fx *target, int pmsg);
	void (*hfc_fx_forced_offline_c) (struct target_info_fx *target, int pmsg);	/* FCLNX-GPL-147 */
	int (*hfc_fx_mp_queue_depth) (struct scsi_device *sdev, int *queue_depth);	/* FCGW 0331 */ /* FCLNX-GPL-255 */
	void (*hfc_fx_isolconf_setup) (struct port_info *pp);		/* FCLNX-0542 */
	int (*hfc_fx_allocate_errcnt_info) (struct port_info *pp, int flags);	/* FCLNX-0542 */
	void (*hfc_fx_free_errcnt_info) (struct port_info *pp);	/* FCLNX-0542 */
	void(*hfc_fx_set_retry_cnt) (struct port_info *pp);		/* FCLNX-0542 */
	void (*hfc_fx_mp_set_parm)(struct Scsi_Host *shost, struct port_info *pp, 
							struct Scsi_Host *hsd_host, struct Scsi_Host *pfb_host, uint64_t dma_mask);	/* FCGW 0331 */						/* FCLNX-GPL-204 */
	int (*hfc_fx_mp_scsi_host_alloc) (struct scsi_host_template *temp, 
					struct Scsi_Host *shost, struct port_info *pp, struct Scsi_Host **hsd_host, struct Scsi_Host **pfb_host);
	int (*hfc_fx_mp_scsi_add_host) (struct Scsi_Host *shost, struct port_info *pp, struct Scsi_Host *hsd_host, struct Scsi_Host *pfb_host);	/* FCLNX-GPL-204 */
	int (*hfc_fx_mp_scsi_scan_host) (struct Scsi_Host *shost, struct Scsi_Host *hsd_host, struct Scsi_Host *pfb_host);	/* FCLNX-GPL-204 */
	void (*hfc_fx_mp_scsi_remove_host) (struct Scsi_Host *host);									/* FCLNX-GPL-204 */
	int (*hfc_fx_check_mp_enable) (void);
	int  (*hfc_fx_mp_slave_alloc) (struct scsi_device *sdev);
	void (*hfc_fx_mp_slave_destroy) (struct scsi_device *sdev);
	int  (*hfc_fx_mp_change_queue_depth) (struct scsi_device *sdev, int qdepth, int reason);
	int  (*hfc_fx_free_and_allocate_errcnt_info)(struct port_info *pp);	/* FCLNX-GPL-FX-314 */

	/* strategy */
	int (*hfc_fx_mp_strategy) (struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
	int (*hfc_fx_mp_abort) (struct scsi_cmnd *cmnd);
	int (*hfc_fx_mp_device_reset) (struct scsi_cmnd *cmnd);
	int (*hfc_fx_mp_target_reset) (struct scsi_cmnd *cmnd);						/* FCLNX-GPL-0449 */
	int (*hfc_fx_mp_bus_reset) (struct scsi_cmnd *cmnd);
	int (*hfc_fx_retry_strategy) (struct hfc_pkt_fx *retry_hfcp_top);
	void (*hfc_fx_ioerror_check) (struct scsi_cmnd *cmnd, struct hfc_pkt_fx *hfcp);
	int (*hfc_fx_queue_check) (struct hfc_pkt_fx *hfcp, short lun_id);				/* FCGW 0331 */
	void (*hfc_fx_queue_count) (struct hfc_pkt_fx *hfcp, int we, int deq);
	int (*hfc_fx_mp_iodone) (struct scsi_cmnd *cmnd, struct port_info *pp, struct hfc_pkt_fx *hfcp);
	int (*hfc_fx_check_issuing_io_reset)(struct target_info_fx *target, struct dev_info_fx *dev);	/* FCLNX-GPL-FX-333 */
	int (*hfc_fx_check_io_reset_complete)(struct hfc_pkt_fx *hfcp);
	void (*hfc_fx_check_dev_reset_complete) (void);								/* FCLNX-0429 */
	void (*hfc_fx_check_bus_reset_complete) (void);								/* FCLNX-0429 */
	void (*hfc_fx_make_fcinfo) (struct port_info *pp, struct hfc_pkt_fx *hfcp, 
				unsigned int err_no, unsigned int errcode, uchar *err_detail, int err_detail_cnt);
	void (*hfc_fx_set_scsi_cmd_tmr) (struct port_info *pp, struct core_info *core, struct target_info_fx *target, struct hfc_pkt_fx *hfcp);	/* FCLNX-GPL-0449 */
	void (*hfc_fx_mp_set_cmnd_res) (struct port_info *pp, struct core_info *core, struct scsi_cmnd *cmnd, struct hfc_pkt_fx *hfcp,uint result);
	void (*hfc_fx_mp_intr_xrb) (struct hfc_pkt_fx *wait_iodone_hfcp);
	int (*hfc_fx_tmp_dev_check) (struct dev_info_fx *dev); /* FCLNX-GPL-FX-269 */
	
	/* top */
	void (*hfc_fx_convert_rptluns) (struct lg_target_info_fx *lgtp, struct scsi_cmnd *);		/* FCLNX-0611 */
	void (*hfc_fx_check_luconfig) (struct target_info_fx *target, struct scsi_cmnd *cmnd);							/* FCLNX-0611 *//* FCLNX-GPL-0449 */
	
	/* timer_recovery */
	int (*hfc_fx_errlog_mp) (struct hfc_pkt_fx *hfcp);
	void (*hfc_fx_watched_errcount)(struct port_info *pp, struct target_info_fx *target, uchar flag, uchar err_flag);	/* FCLNX-0542 *//* FCLNX-GPL-327 */
	void (*hfc_fx_clear_errinfo)( struct port_info *pp );								/* FCLNX-GPL-147 */
	void (*hfc_fx_deque_retry_hfcp)(struct port_info *pp, struct hfc_pkt_fx *(*issue_hfcp_top));
	
	/* ioctl */
	int (*hfc_fx_ioctl_mp) (struct port_info *pp, unsigned int cmd, void *arg);
	int (*hfc_fx_mp_rd_param) (struct port_info *pp, void *arg );
    int (*hfc_fx_mp_wr_param) (struct port_info *pp, void *arg );
	int (*hfc_fx_mp_lu_map) (struct port_info *pp, void *arg );
	int (*hfc_fx_mp_setpath) (struct port_info *pp, void *arg );
	int (*hfc_fx_mp_path_health) (struct port_info *pp, void *arg );
	int (*hfc_fx_mp_lgtarget_map) (struct port_info *pp, void *arg );
	int (*hfc_fx_mp_lgpath_info1) (struct port_info *pp, void *arg );
	int (*hfc_fx_read_isolparam) (struct port_info *pp);				/*FCLNX-0542*/
	int (*hfc_fx_read_retry_cnt) (struct port_info *pp);               /*FCLNX-0542*/
	int (*hfc_fx_get_isolparam) ( struct port_info *pp, struct hfc_isol_info *isolinfo );	/* FCLNX-GPL-147 */

	/* diag */
	int (*read_fx_dev_info_mp) (struct port_info *pp, struct diag_ioctl *diag);
	int (*read_fx_lg_target_info_mp) (struct port_info *pp, struct diag_ioctl *diag);
	int (*read_fx_lg_dev_info_mp) (struct port_info *pp, struct diag_ioctl *diag);
	int (*read_fx_lg_path_info_mp) (struct port_info *pp, struct diag_ioctl *diag);
	int (*read_fx_lg_path_info1_mp) (struct port_info *pp, struct diag_ioctl *diag);
	int (*read_fx_lg_path_info2_mp) (struct port_info *pp, struct diag_ioctl *diag);
	int (*read_fx_failover_info_mp) (struct port_info *pp, struct diag_ioctl *diag);
	/* FCLNX-0429 */
	int (*hfc_fx_mp_proc_info_pfb) (struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
	int (*hfc_fx_mp_proc_info_pfb_com) (struct Scsi_Host *host, char *buffer, off_t offset, int length, uint proc_type);	/* FCLNX-GPL-FX-481 */
	int (*hfc_fx_mp_strategy_pfb) (struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
	int (*hfc_fx_mp_lun_reset) (struct scsi_cmnd *cmnd);							/* FCLNX-GPL-0449 */
	int (*hfc_fx_mp_target_reset_pfb) (struct scsi_cmnd *cmnd);					/* FCLNX-GPL-0449 */
	int (*hfc_fx_mp_bus_reset_pfb) (struct scsi_cmnd *cmnd);
	
	void (*hfc_fx_mp_hsd_enable) (void);
	int (*hfc_fx_mp_scsi_scan_target) (struct Scsi_Host *shost,
			 unsigned int channel, unsigned int scsi_id, unsigned int lun, int rescan);
	int (*hfc_fx_mp_scsi_adjust_queue_depth) (struct scsi_device *sdev, int tagged, int tags);
	int (*hfc_fx_mp_scsi_host_put) (struct Scsi_Host *shost);
	int (*hfc_fx_mp_scsi_remove_device) (struct scsi_device *sdev);
	struct scsi_device * (*hfc_fx_mp_scsi_device_lookup) (struct Scsi_Host *shost, uint channel, uint scsi_id, uint lun);
	int (*hfc_fx_mp_scsi_block_requests) (struct Scsi_Host *shost);
	int (*hfc_fx_mp_scsi_unblock_requests) (struct Scsi_Host *shost);
	
	/* version */
	char *(*hfc_fx_get_pcm_rcsid) (void);

};


struct pub_symbol_list {
	/* detect */
	int (*hfc_param_search) (char *search_str, int *value);
	char (*hfc_cnvc) (char C);
	int (*hfc_parse_string) (char *string, char *keyword, uint64_t *value);
	int (*hfc_convert_string) (char *string, uint64_t *value);
	int  (*hfc_initialize) (struct adap_info *ap,int immdt_cmd); /* FCLNX-0526 */
	void (*hfc_chk_stop) (struct adap_info *ap, uchar lock);	/* FCLNX-0534 */
	int (*hfc_issue_forced_mck) (struct adap_info *ap, uchar type);	/* FCLNX-0534 */
	int (*hfc_chk_conf_val) (int min, int max, int val);	/* FCLNX-0542 */
	int (*hfc_config_hw_set_five) (struct adap_info *ap, uint retry_maxcnt); /* FCLNX-0542 */
	
	/* strategy */
	int (*hfc_strategy_pg) (struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
	int (*hfc_eh_abort_pg) (struct scsi_cmnd *cmnd);
	int (*hfc_eh_device_reset_pg) (struct scsi_cmnd *cmnd);
	int (*hfc_eh_target_reset_pg) (struct scsi_cmnd *cmnd);		/* FCLNX-GPL-0343 */
	int (*hfc_eh_bus_reset_pg) (struct scsi_cmnd *cmnd);
	void (*hfc_iodone) (struct adap_info *ap, struct scsi_cmnd *cmnd, struct hfc_pkt *hfcp);	/* FCLNX-0429 */
	int (*hfc_strategy_hfcp) (struct hfc_pkt *hfcp);
	struct hfc_pkt *(*hfc_get_new_hfcp) (struct adap_info *ap);
	int (*hfc_start) (struct adap_info *ap, struct target_info *target, struct hfc_pkt *pkt);	/* FCLNX-0429 */
	void (*hfc_cancel_scsi_cmd) (struct adap_info	*ap, struct target_info *target,			/* FCLNX-0429 */
									 uint lun, struct hfc_pkt *hfcp, uint adap_status,
									 uchar inh_altpath, uchar we_que_cancel, uchar tm_que_cancel,
									 uchar type);
	struct scsi_cmnd *(*hfc_get_new_cmnd) (struct adap_info *ap);
	void (*hfc_dummy_copy) (struct adap_info *ap, struct scsi_cmnd *cmnd, struct scsi_cmnd *dummy_cmnd);
	void (*hfc_enqueue_wx_que) (struct target_info *target, struct hfc_pkt *hfcp);		/* FCLNX-FX-031 */
	void (*hfc_enque_next_dstart) (struct adap_info *ap,struct target_info *target);	/* FCLNX-FX-031 */

	/* timer_recovery */
	void (*hfc_reset_adap_info) (struct adap_info *ap);
	void (*hfc_reset_start) (struct adap_info *ap,uchar type);
	void (*hfc_errlog)( struct adap_info *ap,
		struct target_info      *target,
		struct hfc_pkt          *hfcp,
		uchar                           type,
		uint                            err_id,
		uint                            err_num,
		uchar                           *data,
		ushort                          data_len );/* - FCLNX-546 - */
	void (*hfc_watchdog) (struct wtimer *w_timer);			/* FCLNX-GPL-038 */
	int (*hfc_force_linkdown)(struct adap_info *ap, uchar proc, uchar skip);		/* FCLNX-0542 */
	int (*hfc_force_linkdown_recovery)(struct adap_info *ap);				/* FCLNX-0542 */

	/* top */
	struct target_info *(*hfc_hash_target_valid) ( struct adap_info *, uint );
	struct target_info *(*hfc_hash_target_info) ( struct adap_info *, uint );
	struct target_info *(*hfc_hash_target_info_wwn) (struct adap_info *, uint64_t );
	uint64_t (*hfc_read_tbl) ( void *, char );
	int (*hfc_issue_relogin) (struct adap_info *ap,struct target_info *target );				/* FCLNX-0429 */
	void (*hfc_enque_login_req) (struct adap_info *ap,struct target_info *target);				/* FCLNX-0429 */
	void (*lock_mailbox) ( struct adap_info *ap );          /* FCLNX-0526 */
	void (*unlock_mailbox) ( struct adap_info *ap );        /* FCLNX-0526 */
	void (*hfc_write_reg_ext) (struct adap_info *ap, uint offset,char reg_size, uint64_t data);
	uint64_t (*hfc_read_reg_ext) (struct adap_info *ap, uint offset,char reg_size);				/* FCLNX-GPL-451 */
	void (*hfc_trace) (struct adap_info *,uchar ,uchar *,uchar );
	/* FCLNX-GPL-047 */
	int (*hfc_watchdog_enter) ( struct adap_info *ap, struct target_info *target,struct hfc_pkt *hfcp, uint lun, uchar timer_id, uint tout, int cancel);	/* FCLNX-GPL-0343 */
	struct dev_info * (*hfc_search_dev_info) (struct target_info *target, struct hfc_pkt *hfcp);/* FCLNX-GPL-0343 */
	void *(*hfc_kmalloc) (struct adap_info *ap, size_t size, gfp_t flag );
	void (*hfc_kfree) (struct adap_info *ap, const void *block );
	int (*hfc_mp_watchdog_enter) ( struct adap_info *ap, struct target_info *target,struct hfc_pkt *hfcp, struct dev_info *dev, uint lun, uchar timer_id, uint tout, int cancel);

	/* ioctl */
	int (*hfc_sciocmd) ( struct adap_info *ap, void *arg, int internal, int tiomeout );
	void (*hfc_ioctl_iodone) (struct scsi_cmnd *cmnd);
	void (*structdump) ( int loc, uchar *p, int size );

	/* mlpf */
	uint64_t (*hfc_read_reg_hg_ext)  (struct adap_info *ap, uint offset, char reg_size);				/* FCLNX-GPL-451 */
	void     (*hfc_write_reg_hg_ext) (struct adap_info *ap, uint offset, char reg_size, uint64_t data);	/* FCLNX-GPL-451 */

	/* conf */
	void (*hfc_reset_all_timer) (struct adap_info *ap);
	void (*_hfc_wake_up)(wait_queue_head_t *, atomic_t *condition);

	/* FIVE-FX */
	int (*hfc_fx_param_search) (char *search_str, int *value);
	char (*hfc_fx_cnvc) (char C);
	int (*hfc_fx_parse_string) (char *string, char *keyword, uint64_t *value);
	int (*hfc_fx_convert_string) (char *string, uint64_t *value);
	int  (*hfc_fx_initialize) (struct port_info *pp,int immdt_cmd); /* FCLNX-0526 */
	void (*hfc_fx_chk_stop) (struct port_info *pp);	/* FCLNX-0534 */
	int (*hfc_fx_issue_forced_mck) (struct port_info *pp, struct core_info *core, uchar type);	/* FCLNX-0534 */
	int (*hfc_fx_chk_conf_val) (int min, int max, int val);	/* FCLNX-0542 */
	int (*hfc_fx_config_hw_set_five_fx) (struct port_info *pp, uint retry_maxcnt); /* FCLNX-0542 */
	
	/* strategy */
	int (*hfc_fx_strategy_pg) (struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
	int (*hfc_fx_eh_abort_pg) (struct scsi_cmnd *cmnd);
	int (*hfc_fx_eh_device_reset_pg) (struct scsi_cmnd *cmnd);
	int (*hfc_fx_eh_target_reset_pg) (struct scsi_cmnd *cmnd);		/* FCLNX-GPL-0343 */
	int (*hfc_fx_eh_bus_reset_pg) (struct scsi_cmnd *cmnd);
	void (*hfc_fx_set_cmnd_res) (struct port_info *pp, struct core_info *core, struct scsi_cmnd *cmnd, struct hfc_pkt_fx *hfcp, uint result);
	void (*hfc_fx_iodone) (struct port_info *pp, struct core_info *core, struct scsi_cmnd *cmnd, struct hfc_pkt_fx *hfcp);
	int (*hfc_fx_strategy_port) (struct hfc_pkt_fx *hfcp, int core_no); /* FCLNX-GPL-FX-266 */
	void (*hfc_fx_strategy_core) (struct hfc_pkt_fx *hfcp);
	struct hfc_pkt_fx *(*hfc_fx_get_new_hfcp) (struct port_info *pp);
	void (*hfc_fx_start) (struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target);
	void (*hfc_fx_cancel_scsi_cmd) (struct port_info	*pp, struct core_info *core, struct target_info_fx *target,			/* FCLNX-0429 */
									 uint lun, struct hfc_pkt_fx *hfcp, uint adap_status,
									 uchar inh_altpath, uchar we_que_cancel, uchar tm_que_cancel,
									 uchar type);
	struct scsi_cmnd *(*hfc_fx_get_new_cmnd) (struct port_info *pp);
	void (*hfc_fx_dummy_copy) (struct port_info *pp, struct scsi_cmnd *cmnd, struct scsi_cmnd *dummy_cmnd);
	int (*hfc_fx_issue_devrst_cscsi) (struct port_info *pp, struct target_info_fx *target, struct dev_info_fx *dev, uint flags);
	int (*hfc_fx_issue_tgtrst_cscsi) (struct port_info *pp, struct target_info_fx *target, struct dev_info_fx *dev, uint flags);
	void (*hfc_fx_enque_next_dstart) (struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target);
	void (*hfc_fx_enqueue_wx_que) (struct core_info *core, struct hfc_pkt_fx *hfcp);

	/* timer_recovery */
	void (*hfc_fx_reset_port_info) (struct port_info *pp);
	void (*hfc_fx_reset_start) (struct port_info *pp,uchar type);
	void (*hfc_fx_errlog)( struct port_info	*pp,
		struct core_info					*core,
		struct target_info_fx				*target,
		struct hfc_pkt_fx          			*hfcp,
		uchar								type,
		uint								err_id,
		uint								err_num,
		uchar								*data,
		ushort								data_len );/* - FCLNX-546 - */
	void (*hfc_fx_watchdog) (struct wtimer_fx *w_timer);			/* FCLNX-GPL-038 */
	int (*hfc_fx_force_linkdown)(struct port_info *pp, uchar proc);		/* FCLNX-0542 *//* FCLNX-GPL-FX-043 */
	int (*hfc_fx_force_linkdown_recovery)(struct port_info *pp);					/* FCLNX-0542 */

	/* top */
	struct target_info_fx *(*hfc_fx_hash_target_valid) ( struct port_info *, uint );
	struct target_info_fx *(*hfc_fx_hash_target_info) ( struct port_info *, uint );
	struct target_info_fx *(*hfc_fx_hash_target_info_wwn) (struct port_info *, uint64_t );
	uint64_t (*hfc_fx_read_tbl) ( void *, char );
	void (*hfc_fx_enque_plogi_req) (struct port_info *pp,struct target_info_fx *target);			/* FCLNX-0429 */
	void (*hfc_fx_enque_prli_req) (struct port_info *pp,struct target_info_fx *target);
	void (*lock_fx_mailbox) ( struct port_info *pp);          /* FCLNX-0526 */
	void (*unlock_fx_mailbox) ( struct port_info *pp);        /* FCLNX-0526 */
	void (*hfc_fx_write_reg_ext) (struct port_info *pp, uint offset,char reg_size, uint64_t data);
	uint64_t (*hfc_fx_read_reg_ext) (struct port_info *pp, uint offset,char reg_size);				/* FCLNX-GPL-451 */
	void (*hfc_fx_trace) (struct port_info *,struct core_info *,uchar ,uchar *,uchar );
	int (*hfc_fx_watchdog_enter) ( struct port_info *pp, struct core_info *core, struct target_info_fx *target,
		struct hfc_pkt_fx *hfcp, uint lun, uchar timer_id, uint tout, int cancel);	/* FCLNX-GPL-0343 */
	/* FCLNX-GPL-047 */
	struct dev_info_fx * (*hfc_fx_search_dev_info) (struct target_info_fx *target, short lun_id);/* FCLNX-GPL-0343 */
	void *(*hfc_fx_kmalloc) (struct port_info *pp, size_t size, gfp_t flag );
	void (*hfc_fx_kfree) (struct port_info *pp, const void *block );
	int (*hfc_fx_mp_watchdog_enter) ( struct port_info *pp, struct core_info *core, struct target_info_fx *target,
		struct hfc_pkt_fx *hfcp, struct dev_info_fx *dev, uint lun, uchar timer_id, uint tout, int cancel);

	/* ioctl */
	int (*hfc_fx_sciocmd) ( struct port_info *pp, void *arg, int internal, int tiomeout );
	void (*hfc_fx_ioctl_iodone) (struct scsi_cmnd *cmnd);

	/* FCLNX-GPL-038 *//* FCLNX-GPL-0343 */
	void (*hfc_fx_clear_dev_info) (struct dev_info_fx *dev);
	void (*hfc_fx_all_clear_dev_info) (struct port_info *pp, struct dev_info_fx *dev);
	void (*hfc_fx_set_dev_info) (struct dev_info_fx *dev);
	struct dev_info_fx * (*hfc_fx_get_dev_info) (struct target_info_fx *target, uint lun);/* FCLNX-GPL-0343 */
	void (*hfc_fx_free_dev) (struct target_info_fx *target);/* FCLNX-GPL-0343 */

	int (*hfc_fx_issue_relogin) (struct port_info *ap,struct target_info_fx *target );				/* FCLNX-0429 */
	void (*hfc_fx_enque_login_req) (struct port_info *ap,struct target_info_fx *target);				/* FCLNX-0429 */

	/* mlpf */
	uint64_t (*hfc_fx_read_reg_hg_ext)  (struct port_info *pp, uint offset, char reg_size);				/* FCLNX-GPL-451 */
	void     (*hfc_fx_write_reg_hg_ext) (struct port_info *pp, uint offset, char reg_size, uint64_t data);	/* FCLNX-GPL-451 */

	/* conf */
	void (*hfc_fx_reset_all_timer) (struct port_info *pp);
	void (*_hfc_fx_wake_up)(wait_queue_head_t *, atomic_t *condition);

};

#if 0
const struct cr_PartsNumber cr_pn[] = {				/* FCLNX-0329 */
	{ 0x85, 1, 0, "3", "3", "-C", "HFC0401",   },	/* FIVE 2G 1Port */
	{ 0x85, 2, 0, "3", "2", "-B", "HFC0402",   },	/* FIVE 2G 2Port */
	{ 0x85, 4, 0, "3", "1", "-A", "HFC0404",   },	/* FIVE 2G 4Port */
	{ 0x82, 1, 0, "3", "2", "-B", "HFC0401",   },	/* FIVE 4G 1Port */
	{ 0x82, 2, 0, "3", "1", "-A", "HFC0402",   },	/* FIVE 4G 2Port */
	{ 0x86, 1, 0, "3", "2", "-B", "HFC0401",   },	/* FIVE 4G 1Port */
	{ 0x86, 2, 0, "3", "1", "-A", "HFC0402",   },	/* FIVE 4G 2Port */
	{ 0x87, 1, 0, "3", "1", "-A", "HFC1001",   },	/* FIVE 10G */
	{ 0x88, 1, 0, "3", "2", "-B", "HFC0401-C", },	/* Type3 Combo 1Port */
	{ 0x88, 2, 0, "3", "1", "-A", "HFC0402-C", },	/* Type3 Combo 2Port */
	{ 0x89, 1, 0, "3", "1", "-A", "HFC0401-C", },	/* FCSWM 1Port */
	{ 0x8a, 1, 0, "3", "2", "-B", "HFC0401-M", },	/* MEZANINE 1Port */
	{ 0x8a, 2, 0, "3", "1", "-A", "HFC0402-M", },	/* MEZANINE 2Port */
	{ 0x8b, 1, 0, "3", "2", "-B", "HFC0401-M", },	/* Enhanced MEZANINE 1Port *//* FCLNX-GPL-054 */
	{ 0x8b, 2, 0, "3", "1", "-A", "HFC0402-M", },	/* Enhanced MEZANINE 2Port *//* FCLNX-GPL-054 */
	{ 0x8f, 2, 0, "3", "1", "-A", "HFC0402-N", },	/* FC-GW 2Port */	/* FC-GW */
	{ 0x8f, 1, 0, "3", "2", "-B", "HFC0401-N", },	/* FC-GW 1Port */	/* FC-GW */
	{ 0x84, 1, 0, "3", "2", "-B", "HFC0401-E", },   /* FIVE 4G PCIe 1 port */    /* FCLNX-GPL-054 */
	{ 0x84, 2, 0, "3", "1", "-A", "HFC0402-E", },   /* FIVE 4G PCIe 2 port */    /* FCLNX-GPL-054 */
	{ 0,    0, 0, "",  "",  "",   "UNKNOWN"    }	/* END */
};

/* driver information */
const struct manage_info hfc_manage_info;			/* driver information */
const int instance = 0;
//const int hfc_fx_major = 0;
const int hba_num = 0;


const int adapter_bindings = 0;
const struct narrow_dev hfc_narrow_dev;					/* FCLNX-0392 */

const char *hfclddconf = NULL;

const uint	raslog_install = 2;
#endif
#endif /* __KERNEL__ */

#endif				/* !INCLUDE_HFCLDD_H */
