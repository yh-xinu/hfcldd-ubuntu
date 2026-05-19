/*
 * hfcldd_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcldd_fx.h,v 1.1.2.41.2.7.2.8.2.9 2015/08/26 10:31:13 toyo Exp $
 */


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

#else

struct cpumask {
	unsigned long bits[1];
};

#endif

/************************************************************************/
/* include file                                                         */
/************************************************************************/

#include "hfclddioc.h" /* hfcddioc.h ->hfclddioc.h */
#include "hfclddcom.h" /* hfcddcom.h ->hfclddcom.h */

#include "hfclddcom_fx.h"
/**************************************************************************************************/
/* Macro																						  */
/**************************************************************************************************/

#define HFC_FX_MAILBOX_UNLOCK(pp,b)	clear_bit(b, (ulong *)&pp->region_arg[pp->rid]->mb_lock)

#define HFC_SYSLOCK_IRQSAVE(_PP, _FLAGS) {										\
	spin_lock_irqsave(&(_PP)->manage_info->hfcmp_lock, (_FLAGS));		\
}

#define HFC_SYSUNLOCK_IRQRESTORE(_PP, _FLAGS) {									\
	spin_unlock_irqrestore(&_PP->manage_info->hfcmp_lock, (_FLAGS));	\
}

#define HFC_SYSLOCK(_PP) {														\
	spin_lock(&(_PP)->manage_info->hfcmp_lock);							\
}

#define HFC_SYSUNLOCK(_PP) {													\
	spin_unlock(&_PP->manage_info->hfcmp_lock);							\
}

#define HFC_PORTLOCK_IRQSAVE(_PP, _FLAGS) {										\
			spin_lock_irqsave(&(_PP)->pport->port_lock, (_FLAGS));				\
}

#define HFC_PORTUNLOCK_IRQRESTORE(_PP, _FLAGS) {								\
			spin_unlock_irqrestore(&_PP->pport->port_lock, (_FLAGS));			\
}

#define HFC_PORTLOCK(_PP) {														\
			spin_lock(&(_PP)->pport->port_lock);								\
}

#define HFC_PORTUNLOCK(_PP) {													\
			spin_unlock(&_PP->pport->port_lock);								\
}

#define HFC_CORELOCK_IRQSAVE(_CORE, _CFLAGS) {									\
		if ((_CORE))															\
			spin_lock_irqsave(&(_CORE)->core_lock, (_CFLAGS));					\
}

#define HFC_COREUNLOCK_IRQRESTORE(_CORE, _CFLAGS) {								\
		if ((_CORE))															\
			spin_unlock_irqrestore(&(_CORE)->core_lock, (_CFLAGS));				\
}

#define HFC_CORELOCK(_CORE) {													\
		if ((_CORE))															\
			spin_lock(&(_CORE)->core_lock);										\
}

#define HFC_COREUNLOCK(_CORE) {													\
		if ((_CORE))															\
			spin_unlock(&(_CORE)->core_lock);									\
}

#define HFC_ALLCORELOCK(_RP) {													\
		if ((_RP)) {															\
			if ((_RP)->core_arg[0] != NULL) 									\
				spin_lock(&(_RP)->core_arg[0]->core_lock);						\
			if ((_RP)->core_arg[1] != NULL) 									\
				spin_lock(&(_RP)->core_arg[1]->core_lock);						\
			if ((_RP)->core_arg[2] != NULL) 									\
				spin_lock(&(_RP)->core_arg[2]->core_lock);						\
			if ((_RP)->core_arg[3] != NULL) 									\
				spin_lock(&(_RP)->core_arg[3]->core_lock);						\
		}																		\
}

#define HFC_ALLCOREUNLOCK(_RP) {												\
		if ((_RP)) {															\
			if ((_RP)->core_arg[3] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[3]->core_lock);					\
			if ((_RP)->core_arg[2] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[2]->core_lock);					\
			if ((_RP)->core_arg[1] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[1]->core_lock);					\
			if ((_RP)->core_arg[0] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[0]->core_lock);					\
		}																		\
}

#define HFC_ALLLOCK_IRQSAVE(_PP, _RP, _FLAGS) {									\
		if (!(_PP->manage_info->hfcldd_mp_mod)){								\
			spin_lock_irqsave(&(_PP)->pport->port_lock, (_FLAGS));				\
		}else{																	\
			spin_lock_irqsave(&(_PP)->manage_info->hfcmp_lock, (_FLAGS));		\
			spin_lock(&(_PP)->pport->port_lock);								\
		}																		\
		if ((_RP)) {															\
			if ((_RP)->core_arg[0] != NULL) 									\
				spin_lock(&(_RP)->core_arg[0]->core_lock);						\
			if ((_RP)->core_arg[1] != NULL) 									\
				spin_lock(&(_RP)->core_arg[1]->core_lock);						\
			if ((_RP)->core_arg[2] != NULL) 									\
				spin_lock(&(_RP)->core_arg[2]->core_lock);						\
			if ((_RP)->core_arg[3] != NULL) 									\
				spin_lock(&(_RP)->core_arg[3]->core_lock);						\
		}																		\
}

#define HFC_ALLUNLOCK_IRQRESTORE(_PP, _RP, _FLAGS) {							\
		if ((_RP)) {															\
			if ((_RP)->core_arg[3] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[3]->core_lock);					\
			if ((_RP)->core_arg[2] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[2]->core_lock);					\
			if ((_RP)->core_arg[1] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[1]->core_lock);					\
			if ((_RP)->core_arg[0] != NULL) 									\
				spin_unlock(&(_RP)->core_arg[0]->core_lock);					\
		}																		\
		if (!(_PP->manage_info->hfcldd_mp_mod)){								\
			spin_unlock_irqrestore(&_PP->pport->port_lock, (_FLAGS));			\
		}else{																	\
			spin_unlock(&(_PP)->pport->port_lock);								\
			spin_unlock_irqrestore(&(_PP)->manage_info->hfcmp_lock, (_FLAGS));	\
		}																		\
}


#define HFC_DETAIL_CLEAR_LINKDOWN(pp) {											\
	if(pp){																		\
		clear_bit(HFC_PD_NEED_FLOGI, (ulong *)&pp->status_detail1);				\
		clear_bit(HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_SCR, (ulong *)&pp->status_detail1);				\
		clear_bit(HFC_PD_NEED_PLOGI_N, (ulong *)&pp->status_detail1);			\
		clear_bit(HFC_PD_NEED_RFTID, (ulong *)&pp->status_detail1);				\
		clear_bit(HFC_PD_NEED_RFFID, (ulong *)&pp->status_detail1);				\
		clear_bit(HFC_PD_WAIT_RECEIVE_PLOGI, (ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1);			\
		clear_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2);			\
		clear_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);				\
	}																			\
}

#define HFC_DETAIL_CLEAR_ISOLREC(pp) {											\
	if(pp){																		\
		clear_bit(HFC_PD_ISOLATE_PORT_C,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_ISOLATE_PORT_E,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_ISOLATE_SFPFAIL,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT,(ulong *)&pp->status_detail2);	\
		clear_bit(HFC_PD_ISOLATE_SFPDOWN,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_ISOLATE_CHKSTP,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_ISOLATE_CHKSTP_C,	(ulong *)&pp->status_detail2);		\
	}																			\
}

#define HFC_CLEAR_PD_NEED(pp) {													\
	if(pp){																		\
		clear_bit(HFC_PD_NEED_CORE_START,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_LINK_INI,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_FLOGI,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_ADD_PORTID,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_SCR,			(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_PLOGI_N,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_RFTID,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_RFFID,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_LOGO_FCSW,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_DEL_PORTID,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_NEED_GPNFT,		(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_NEED_OFFLINE_MB,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_NEED_MIHLOG,		(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_NEED_LOAD_CH_TRACE,(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_NEED_DIAG,			(ulong *)&pp->status_detail2);		\
	}																			\
}

/* FCLNX-GPL-FX-161 Start */
#define HFC_CLEAR_PD_WAIT(pp) {													\
	if(pp){																		\
		clear_bit(HFC_PD_WAIT_CORE_START,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_LINK_INI,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_FLOGI,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_ADD_PORTID,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_SCR,			(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_PLOGI_N,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_RFTID,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_RFFID,		(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_LOGO_FCSW,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_DEL_PORTID,	(ulong *)&pp->status_detail1);		\
		clear_bit(HFC_PD_WAIT_GPNFT,		(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_WAIT_OFFLINE_MB,	(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_WAIT_MIHLOG,		(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_WAIT_LOAD_CH_TRACE,(ulong *)&pp->status_detail2);		\
		clear_bit(HFC_PD_WAIT_DIAG,			(ulong *)&pp->status_detail2);		\
	}																			\
}
/* FCLNX-GPL-FX-161 End */

#define HFC_CLEAR_TS_NEED(target) {												\
	if(target){																	\
		clear_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA,	(ulong *)&target->status);	\
		clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA,	(ulong *)&target->status);		\
		clear_bit(HFC_TS_NEED_PLOGI,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_NEED_PRLI,			(ulong *)&target->status);			\
		clear_bit(HFC_TS_NEED_PDISC,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_NEED_LOGO_TGT,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_NEED_TARGET_RESET,	(ulong *)&target->status);			\
		clear_bit(HFC_TS_CANCEL_SCSI_TARGET,(ulong *)&target->status);			\
	}																			\
}

/* FCLNX-GPL-FX-161 Start */
#define HFC_CLEAR_TS_WAIT(target) {												\
	if(target){																	\
		clear_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA,	(ulong *)&target->status);	\
		clear_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA,	(ulong *)&target->status);		\
		clear_bit(HFC_TS_WAIT_PLOGI,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_PRLI,			(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_PDISC,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_LOGO_TGT,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_TARGET_RESET,	(ulong *)&target->status);			\
		clear_bit(HFC_TS_CANCEL_SCSI_TARGET,(ulong *)&target->status);			\
	}																			\
}
/* FCLNX-GPL-FX-161 End */

/* FCLNX-GPL-FX-446 >>> */
#define HFC_CLEAR_TS_LOGINOUT(target) {											\
	if(target){																	\
		clear_bit(HFC_TS_NEED_PLOGI,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_NEED_PRLI,			(ulong *)&target->status);			\
		clear_bit(HFC_TS_NEED_LOGO_TGT,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_PLOGI,		(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_PRLI,			(ulong *)&target->status);			\
		clear_bit(HFC_TS_WAIT_LOGO_TGT,		(ulong *)&target->status);			\
	}																			\
}
/* FCLNX-GPL-FX-446 <<< */

#define	HFC_RID_NUM			MAX_REGION_PROBE_FX
#define	HFC_RID_CAP			(HFC_RID_NUM - 1)
#define	HFC_RID_SBIT		0x80
#define	HFC_RID_CHK			(HFC_RID_CAP | HFC_RID_SBIT)
#define	HFC_RID_MSK			HFC_RID_CAP
#define HFC_GET_RID(P)		((P)->rid & HFC_RID_MSK)
#define HFC_CHK_RID(P)		((P)->rid & ~HFC_RID_CHK)
#define HFC_SET_RID(P,R)	((P)->rid = R & HFC_RID_CHK)
#define HFC_GET_REGION_ADDR(P,R) ((P)->region_arg[(R)])
#define MAX_CORE_PROBE_FX	4

#define HFC_LOOP_CORE(L,P)						\
		(L)=0;									\
		(L)<MAX_CORE_PROBE_FX;					\
		(L)+=MAX_CORE_PROBE_FX/((P)->core_num)

#define HFC_GET_CORE_ADDR(P,N) \
	(struct core_info *)\
	(((struct region_info *)(P)->region_arg[(P)->rid])->core_arg[(N)])

#define hfc_fx_sleep_on(x,y)			_hfc_fx_sleep_on(x,(atomic_t *)y)
#define hfc_fx_wake_up(x,y)				_hfc_fx_wake_up(x,(atomic_t *)y)

/*---- Core# -----------------------------------------------------------------*/
#define	HFC_INTE_CORE_NUM	MAX_CORE_PROBE_FX
#define	HFC_INTE_CORE_MSK	(HFC_INTE_CORE_NUM - 1)
#define	HFC_INTE_CORE_SHR	0x80
#define	HFC_INTE_CORE_UDF	((uchar)-1)

/**************************************************************************************************/
/* literal																						  */
/**************************************************************************************************/
/*--- FX Time-out setting value ---*/
#define	HFC_FX_LINKINT_TO			120		/* Link initialize watch time   */
#define	HFC_FX_MB_TO				5		/* Mailbox start watch time other than the*/
											/* above-mentioned such as loop mode/trace*/
#define	HFC_FX_MB_PROC_TO			20		/* Mailbox Process level 		*/ 
#define	HFC_FX_MB_DIAG_TO			30		/* Mailbox			  FCLNX-320 */
											/* start watch time of Diag		*/
#define	HFC_FX_SCSI_CMD_TO			5		/* scsi command default watch time	*/
#define	HFC_FX_TARGET_RST_TO		20		/* Target_Reset watch time		*/
#define	HFC_FX_ABORT_ACA_TO			8		/* Abort_T-Set/Clear_ACA watch time*/
#define	HFC_FX_DELAY_TO				0		/* following Delay time of Target Reset*/
#define	HFC_FX_DELAY_DEV_TO			0		/* following Delay time of LUN Reset*/
#define	HFC_FX_MCKINT_TO			15		/* MCK Interrupt Time Out		*/
#define	HFC_FX_REBOOT_DELAY_TO		3		/* MCK recovery waiting time of */
#define	HFC_FX_MCK_DELAY_TO			3		/* another port from REBOOT  	*/
											/* instruction to FW_START		*/
#define	HFC_FX_WEXEC_TO				3		/* Start waiting cue watch time	*/

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#define	HFC_FX_LINKUP_TO			0		/* LinkUp waiting time			*/
#else
#define	HFC_FX_LINKUP_TO			15		/* LinkUp waiting time			*/
#endif
#define	HFC_FX_PCM_LINKUP_TO		15		/* LinkUp waiting time for PCM	*/

#define HFC_FX_MCK_RCV_TO			15		/* LINKUP waiting time after MCK recovers*/
#define HFC_FX_WAIT_MPAPLOCK_TO		1		/* MP_ADAP_INFO lock waiting time*/
#define HFC_FX_FW_ISOL_TO			6		/* firmware isolation waiting time*/
#define HFC_FX_LOGIN_DELAY_TO		3		/* following Delay time of LOGIN */
#define HFC_FX_MLPF_FCSTP_STO      	5
#define HFC_FX_PATH_RETRY_TO		0		/* rery to issue a scsi command to other path for HFC-PCM */
/**************************************************************************************************/
/* structure																					  */
/**************************************************************************************************/
/************************************************************************/
/* vport_info : FIVE-FX vport information								*/
/************************************************************************/
struct vport_info {
	uint					status;			/* status						*/
#define HFC_FX_VPORT_DISABLE	0
	struct port_info		*vport_arg;		/* Pointers to virt port_infos	*/
};

/************************************************************************/
/* dstart_info : FIVE-FX dstart information								*/
/************************************************************************/
struct hfc_queue {
	/* SCSI command initiate_wait_queue 									*/
	int 					wx_que_cnt;		/* SCSI initiation wait queue	*/
	struct hfc_pkt_fx		*wx_que_top;	/* Pointer to top				*/
	struct hfc_pkt_fx		*wx_que_end;	/* Pointer to end				*/

	/* SCSI command wait_end_queue 											*/
	int 					we_que_cnt;		/* SCSI wait end queue count	*/
	int 					wx_que_tskmgm_cnt; /* TaskMgmt cmd wait queue	*//* FCLNX-GPL-FX-014 */
	struct hfc_pkt_fx		*we_que_top[HASH_T_NUM];	/* Pointer to top	*/
	struct hfc_pkt_fx		*we_que_end[HASH_T_NUM];	/* Pointer to end	*/

  /* Next dstart queue */
	uchar					next_dstart_flag;
	struct target_info_fx	*nnext;
	struct target_info_fx	*nprev;

};

struct hfc_err1bit_fx{
	uint			pcie_sram_cnt;							/* +0x000 */
	uchar			resv[4];								/* +0x004 */
	uint64_t		time_stamp;								/* +0x008 */
	uint			pcie_sram_data[120];					/* +0x010 */
};

struct hfc_fw_err1bit_fx {
	uchar			resv[4];
	uint			core_fw_far;
	uint64_t		time_stamp;
};

typedef struct{ /* PCIe SRAM 1bit error */
#define HFC_FX_1BIT_LOG_ENTRY	6
	uint			err_num;								/* +0x000 */
	uchar			resv1[8];								/* +0x004 */
	uint			pcie_sram_current_cnt;					/* +0x00c */
	
	struct hfc_err1bit_fx	pcie_sram_data[2];				/* +0x010 */
	
	uchar			resv4[16];								/* +0x3f0 */
} hfc_fx_err1bit_logout_t;

typedef struct{ /* Core SRAM 1bit error */
#define HFC_FX_FW_1BIT_LOG_ENTRY	15
	uint			err_num;								/* +0x000 */
	uchar			resv1[12];								/* +0x004 */
	uint			core_ce_cnt[4];							/* +0x010 */
//	uint			core1_ce_cnt;							/* +0x014 */
//	uint			core2_ce_cnt;							/* +0x018 */
//	uint			core3_ce_cnt;							/* +0x01c */
	struct hfc_fw_err1bit_fx	core_fw_err1bit[4][HFC_FX_FW_1BIT_LOG_ENTRY];	/* +0x020 */
//	struct hfc_fw_err1bit_fx	core_fw_err1bit[HFC_FX_1BIT_LOG_ENTRY];	/* +0x110 */
//	struct hfc_fw_err1bit_fx	core_fw_err1bit[HFC_FX_1BIT_LOG_ENTRY];	/* +0x200 */
//	struct hfc_fw_err1bit_fx	core_fw_err1bit[HFC_FX_1BIT_LOG_ENTRY];	/* +0x2f0 */
	uchar			resv2[32];								/* +0x3e0 */
} hfc_fx_fw_err1bit_logout_t;


/************************************************************************/
/* watchdog timer														*/
/************************************************************************/
struct wtimer_fx	 {
	struct	port_info		*pp;		/* port_info structure			*/
	struct	target_info_fx 	*target;	/* target info structure		*/
	struct	hfc_pkt_fx		*hfcpk;		/* hfc_pkt structure			*/
	ushort					timer_id;	/* Timer ID (31 from 0)			*/
	ushort					timer_flag;	/* indicate that timer is valid	*/
	int						ap_dev_minor;  /* adapter minor#  FCLNX-322 */
#define HFC_TIMER_VALID			0x80	/* Timer is valid 				*/

	uint					wdog_time;	/* Timeout(s)					*/


#define HFC_FX_DELAY_TMR			1		/* SCSI Command Delay Timer each target	*/
#define HFC_FX_SCSI_CMD_TMR			2		/* SCSI Command Timer 			*/
#define HFC_FX_ABORT_TMR	  		3		/* Abort Task Set Timer			*/
#define HFC_FX_TARGET_RST_TMR		4		/* Target_Reset timer			*/
#define HFC_FX_CANCEL_SCSI_TMR		5		/* CANCEL SCSI Command Timer	*//* FCLNX-GPL-FX-014 */
#define HFC_FX_LINKINIT_TMR			7		/* Link Initialize Timer		*/
#define HFC_FX_TOTAL_ABORT_TMR	  	8		/* Abort Task Set process total Timer	*//* FCLNX-GPL-FX-014 */
#define HFC_FX_TOTAL_TGTRST_TMR		9		/* Target Reset process total timer		*//* FCLNX-GPL-FX-014 */
#define	HFC_FX_REBOOT_DELAY_TMR		10		/* REBOOT Delay					*/
#define HFC_FX_WEXEC_TMR			19		/* Start waiting cue watch timer		*/
#define HFC_FX_LINKUP_TMR			20		/* LinkUp waiting watch timer			*/
#define HFC_FX_SCN_LINKUP_TMR		21		/* LinkUp waiting watch timer(SCN opportunity)*/
											/* FCWIN-0082 */
#define HFC_FX_RESTART_TMR	  		22		/* Task Mgm Restart Timer		*/
														  /* FCWIN-0153 */
#define	HFC_FX_MCK_DELAY_TMR		23		/* MCK recovery processing Delay */
#define	HFC_FX_DIAG_DELAY_TMR		24		/* Reboot when Diag is tested	 */
#define HFC_FX_LOGIN_DELAY_TMR		26		/* State of LOGIN DELAY TIME	 */		/* FCLNX-0243 */
#define HFC_FX_WLINKUP_MCK_TMR		27		/* LinkUp waiting watch timer after MCK recovers */
#define	HFC_FX_MCKINT_TMR			28		/* MCKINT watch timer			 */		/* FCLNX-0275 */
#define	HFC_FX_CTLRST_DELAY_TMR		29		/* CTL RESET Delay				 */		/* FCLNX-0279 */

// @MLPF TMR
#define HFC_FX_MLPF_FMCK_TMR		32
#define HFC_FX_MLPF_FCSTP_TMR		34
#define HFC_FX_MLPF_CSTPEND_TMR		35

#define HFC_FX_LDLERR_TMR			36		/* FCLNX-0454 */
#define HFC_FX_LDSERR_TMR			37		/* FCLNX-0454 */
#define HFC_FX_IFERR_TMR			38		/* FCLNX-0454 */
#define HFC_FX_TOERR_TMR			39		/* FCLNX-0454 */
#define HFC_FX_RTERR_TMR			40		/* FCLNX-0454 */

#define HFC_FX_DELAY_TMR_DEV		41		/* SCSI Command Delay Timer	each device	*//* FCLNX-GPL-038 */

#define HFC_FX_INT_CHECK_TMR		43		/* FCLNX-GPL-306 */

#define HFC_FX_TGT_LDLERR_TMR		44		/* FCLNX-GPL-327 */
#define HFC_FX_TGT_LDSERR_TMR		45		/* FCLNX-GPL-327 */

#define HFC_FX_MLPF_ISOLEND_TMR		46

#define HFC_FX_MB_DELAY_TMR			47		/* mailbox delay before starting first Mailbox per core */
#define HFC_FX_MB_RETRY_TMR			48		/* mailbox retry continuation timer per core 			*/
#define HFC_FX_MB_RSP_TMR			49		/* mailbox response timer per core						*/
#define HFC_FX_MB_RETRY_DELAY_TMR	50		/* mailbox retry interval per core						*/
#define HFC_FX_RCV_PLOGI_TMR		51		/* Point to Point:waiting timer of Received PLOGI		*/

#define HFC_FX_PATH_RETRY_TMR		52		/* retry to issue a SCSI Command to oethr path. hfc-pcm only 	*/

#define HFC_FX_WLINKUP_CNT_TMR		53		/* timer for Link Down Error count						*//* FCLNX-GPL-FX-424 */


#define HFC_FX_MAX_TMR				100

	struct dev_info_fx			*dev;		/* dev_info_fx structure		*/	/* FCLNX-GPL-047 */

	struct region_info			*region;	/* Pointer to region_info 		*//* FIVE-FX */
	struct core_info			*core;		/* Pointer to core_info			*//* FIVE-FX */

	struct timer_list			dog;		/* Time-out value storage pointer	*/
};

/************************************************************************/
/* mb_timer_t : FIVE-FX mb_timer_t information							*/
/************************************************************************/
struct mb_timer_t {
	uchar delay; 						/* start delay timer									*/
	uchar tout;  						/* response timer										*/
	uchar retry; 						/* retry count or retry executing time					*/
										/* 0-5:retry count, more than 6:retry executing time	*/
#define HFC_MBTIME_RETRY_MIN	0
#define HFC_MBTIME_RETRY_MAX	5
#define HFC_MBTIME_RETRY_DF		3
	uchar intvl; 						/* retry interval										*/
};

#define HFC_FX_MB_DELAY_MIN			0
#define HFC_FX_MB_DELAY_MAX			10
#define HFC_FX_MB_TO_MIN			6
#define HFC_FX_MB_TO_MAX			60
#define HFC_FX_DF1_MB_TO			26
#define HFC_FX_DF2_MB_TO			26
#define HFC_FX_DF3_MB_TO			10
#define HFC_FX_DF4_MB_TO			10
#define HFC_FX_DF5_MB_TO			10
#define HFC_FX_DF6_MB_TO			10
#define HFC_FX_DF7_MB_TO			6
#define HFC_FX_MB_RETRY_MIN			6
#define HFC_FX_MB_RETRY_MAX			60
#define HFC_FX_DF1_MB_RETRY			26
#define HFC_FX_DF2_MB_RETRY			26
#define HFC_FX_DF3_MB_RETRY			10
#define HFC_FX_DF4_MB_RETRY			10
#define HFC_FX_DF5_MB_RETRY			10
#define HFC_FX_DF6_MB_RETRY			10
#define HFC_FX_DF7_MB_RETRY			6
#define HFC_FX_MB_LOOPBACK_RETRY	4	/* FCLNX-GPL-FX-179 */
#define HFC_FX_MB_INTVL_MIN			0
#define HFC_FX_MB_INTVL_MAX			10


#define HFC_MBTIME_CORE_START		0
#define HFC_MBTIME_LINK_INI			1
#define HFC_MBTIME_FLOGI			2
#define HFC_MBTIME_PLOGI			3
#define HFC_MBTIME_PDISC			4
#define HFC_MBTIME_CAN_SCSI			5
#define HFC_MBTIME_OFFLINE			6

#define HFC_MBTIME_PRLI				7	//FRMSNDRCV ELS
#define HFC_MBTIME_PRLO				8
#define HFC_MBTIME_SCR				9
#define HFC_MBTIME_LOGO				10
#define HFC_MBTIME_AUTH_RJT			11
#define HFC_MBTIME_AUTH_NEGO		12
#define HFC_MBTIME_DHCHAP_CHALLENGE	13
#define HFC_MBTIME_DHCHAP_REPLY		14
#define HFC_MBTIME_DHCHAP_SUCCESS	15
#define HFC_MBTIME_EVFP_SYNC		16
#define HFC_MBTIME_EVFP_COMMIT		17

#define HFC_MBTIME_GCS_ID			18	//FRMSNDRCV FC-GS
#define HFC_MBTIME_GID_PN			19
#define HFC_MBTIME_GPN_ID			20
#define HFC_MBTIME_GID_FT			21
#define HFC_MBTIME_RFT_ID			22
#define HFC_MBTIME_RFF_ID			23
#define HFC_MBTIME_GPN_FT			24

#define HFC_MBTIME_ADD_PORTID		25
#define HFC_MBTIME_DEL_PORTID		26
#define HFC_MBTIME_MIHLOG			27
#define HFC_MBTIME_LOADCHTRC		28
#define HFC_MBTIME_SHADOW_UP		29
#define HFC_MBTIME_DIAG				30	/* FCLNX-GPL-FX-126 */

#define HFC_MBTIME_MAX				31

/* 
 * core_info : FIVE-FX core information
 *
 *  allocated by hfc_probe_one_fx()
 *  released  by hfc_remove_one_fx()
 *
 */
struct core_info {
	uchar					name[16];  		/* Set "core_info" characters			 	*/
	uint					status;			/* Core status information					*/
//#define	HFC_NEED_CORE_START		0
//#define	HFC_WAIT_CORE_START		1
//#define	HFC_NEED_ADD_PORTID		2
//#define	HFC_WAIT_ADD_PORTID		3
//#define	HFC_NEED_DEL_PORTID		4
//#define	HFC_WAIT_DEL_PORTID		5
//#define HFC_CORE_ERROR_STOP		6

#define HFC_CS_HWINIT_COMP		0
#define HFC_CS_CORE_ENABLE		1
#define HFC_CS_CHK_STOP			2
#define HFC_CS_WAIT_MAILBOX		3
#define HFC_CS_MB_RETRY_DELAY	4			/* FCLNX-GPL-FX-161 */

	uint					core_no;		/* Number of core							*/
	uint					pcore_no;		/* Number of core							*/

	/* SCSI command initiate_wait_queue 												*/
	int 					wx_que_cnt_all;

	/* SCSI command wait_end_queue 														*/
	int						we_que_cnt_all;
	uint64_t				we_que_sizecnt; /* Summary of FCP-DLs in commands at WE_QUE */

	/* Next dstart queue																*/
	ushort					next_dstart_cnt;
	struct	target_info_fx	*next_dstart_top;
	struct	target_info_fx	*next_dstart_end;

	/* FW interface information															*/
	uint					mb_status;		/* mailbox status 							*/
											/* If the value equal zero, it means		*/
											/* someone occupied mailbox	 				*/
//#define	HFC_MB_PROL			0			/* Used by ioctl process					*/
//#define	HFC_MB_INTL			1			/* Used by other process					*/
//#define	HFC_MB_LOCK			2			/* Locked by someone						*/
//#define	HFC_PRE_PROL		4			/* PRO-LEVEL execution waiting 				*/

	uchar					*mb_parm; 		/* Pointer to mailbox data area				*/
	uint					mb_resp;		/* Status of mailbox response				*/
//#define	HFC_MBR_COMPLETE	0x00000001 		/* Received mbox without error			*/

	uint					mb_results;		/* Error code of mailbox response			*/
#define HFC_FX_MBRTY_POLICY_CNT		0x80
#define HFC_FX_MBRTY_VAL_MASK		0x7F
	ushort					mb_retry_cnt;	/*Retry counter of mailbox request			*/
	ushort					mb_retry_tid;	/*Timer ID for mailbox retry				*/
	ulong					mb_retry_tout;	/* Timer value of mailbox retry				*/
	uint					drv_next_xob ;	/* xob number to dequeue 					*/
	uint					drv_next_xrb ;	/* xrb number to dequeue 					*/
	
	uint					iov_no;			/* Pointer to seg_info						*/
	uint					iov_map_cnt;	/* bit width of iov_map						*/
#define	HFC_IOVMAP_CNT_1M		224
	uint					*iov_map;		/* Bitmap of seg_info control				*/

	uchar					*slog_addr[256];	/* SOFT LOG (release)					*/

//	struct payload_fx		*payload;		/* Payload Virtual Address for FRMSNDRCV	*/

	/* following elements are the logical address to assigned area						*/
	struct fw_init_tbl_fx	*fw_init_p;		/* INIT_TBL;								*/
	struct mailbox_fx		*mb;			/* MAILBOX									*/
	struct xob_fx			*xob;			/* XOB										*/
	struct xrb_fx			*xrb;			/* XRB										*/
	struct seg_info_fx		*seg_info;		/* SEGINFO									*/
	uchar					*slog;			/* SOFTLOG									*/
	struct payload_fx		*payload;		/* Payload Virtual Address for FRMSNDRCV	*/
	struct rcvfrm_payload_fx *rcvfrm_payload;	/* Receive Frame Payload Address		*/

	/* Xob control area	*/
	struct fx_frame_xob_t {
		uchar	start;
		uchar	num;
		ushort	pkt_no;
	} frame_start_xob[256];
	uint					frame_inp;		/* pointer to save_xob_outp 				*/

	/* io counter */
	uint64_t				scsi_exec_cnt;	/* Initiated SCSI all cmds					*/
	uint64_t				wr_exec_cnt;	/* Initiated SCSI write cmds				*/
	uint64_t				rd_exec_cnt;	/* Initiated SCSI read cmds					*/
	uint64_t				scsi_end_cnt;	/* Ended SCSI all cmds						*/
	uint64_t				wr_cnt;			/* Ended SCSI write cmds					*/
	uint64_t				rd_cnt;			/* Ended SCSI read cmds						*/
	uint64_t				wr_end_cnt[6];	/* Ended SCSI write cmds					*/
	uint64_t				rd_end_cnt[6];	/* Ended SCSI read cmds						*/
#define	HFC_IO_CNT_SIZE_512B	0
#define	HFC_IO_CNT_SIZE_2KB		1
#define	HFC_IO_CNT_SIZE_4KB		2
#define	HFC_IO_CNT_SIZE_16KB	3
#define	HFC_IO_CNT_SIZE_32KB	4
#define	HFC_IO_CNT_SIZE_OVER	5
	uint64_t				scsi_err_cnt;	/* Ended SCSI cmds with error				*/
	uint64_t				xrb_resp_cnt;	/* XRB_Resp_INT count						*/
	uint64_t				wr_data_size;	/* Total data_size for write cmd			*/
	uint64_t				rd_data_size;	/* Total data_size for read cmd				*/
	uint					max_cmd_num_int;	/* Max number of cmds per int			*/

	uint					pkt_cnt;		/* Number of using hfc_pkts					*/
	uint					cmnd_cnt;		/* Number of using dummy scsi_cmnd			*/

	struct hfctrace			*trc_ptr;		/* Pointer to the top trc area 				*/
	uint					trc_num;		/* Current pointer to trc area				*/
	uint					trc_max;		/* Max size of trc area						*/
	uint					mck_err_cnt;	/* MCK count								*/
	uint					post_err_cnt;	/* POST Error count of core					*/

	int						core_ce_cnt;	/* Core 1bit ERR(CE) 						*/

//	uint					mb_lock;		/* lock bit for mailbox process				*/
	uint					mb_sleep;
//#define	HFC_MAILBOX_BUSY		0		/*	This field is locked					*/
//#define	HFC_WAIT_LOCK_MB		1		/*	This field is locked					*/
	uint 					mb_prol_wake_up_time; 

	struct region_info		*rp;			/* region_info address 						*/

	uint					xob_full_cnt;
	uint					iovmap_full_cnt;
	uint					frame_full_cnt;
	uint					page_over_cnt;
	uint					dma_max_over_cnt;
	uint					tskmgm_cmd_num;
	uint					preserve_cmd_num;	/* FCLNX-FX-031 */
	
	/* HFC-PCM */
//	struct hfc_pkt_fx		*retry_hfcp_top;
//	struct hfc_pkt_fx		*retry_hfcp_end;
//	int						retry_hfcp_count;
	
	/* Timer watch */
	uint					timer_flag ;	/* Timer control flag						*/
	
	int		xrb_abend_code;					/* xrb_resp abend code						*/
	uchar	logdata[16];					/* Area to store Error Log Data 			*/
//	uchar	mailbox_pseq;					/* pseq for PLOGI and PRLI					*/
	uint					skip_icc_cnt;	/* skip errlog for icc						*/
	struct	icc_errlog		*icc_err;		/* errlog for icc							*/
	
	/* kernel structure */
	spinlock_t				core_lock;		/* lock to core_info						*/

//	wait_queue_head_t   	mb_lock_event;	/* waitqueue until the lock bit would get	*/
//	uint					mb_lock_event_wait;

	struct semaphore		ioctl_sem;		/* lock  for ioctl process					*/
	struct semaphore		sem;			/* adap_info lock for ioctl process			*/

	dma_addr_t				padr_init;		/* INIT_TBL 								*/
	dma_addr_t				seg_phys_addr;	/* SEGINFO									*/
	dma_addr_t				phys_mb_parm;	/* Mailbox Parater area						*/
	dma_addr_t				phys_xob;		/* XOB		 								*/
	dma_addr_t				phys_xrb;		/* XRB										*/
	dma_addr_t				phys_mb;		/* MAILBOX									*/
	dma_addr_t				phys_slog[256];	/* SOFT LOG	(release)						*/
	dma_addr_t				phys_iov;
	dma_addr_t				phys_payload;	/* Payload DMA Address for FRMSNDRCV		*/
	dma_addr_t				phys_rcvfrm_payload;	/* Receive Frame Payload Address	*/

	struct wtimer_fx		wexec_wdog;		/* SCSI initiation queue 					*/
	struct wtimer_fx		core_mb_wdog;	/* Mailbox timeout							*/
	struct wtimer_fx		mb_retry_intvl_wdog;	/* mailbox retry interval timer		*/
//	struct wtimer_fx		lgdelay_wdog;	/* Login Delay timer						*/
//	struct wtimer_fx		fwisol_wdog;	/* firmware isolation						*/
//	struct wtimer_fx		int_chk_wdog;	/* check interrupt timer					*/

	wait_queue_head_t		mb_event;		/* ioctl mailbox response wait  			*/
	wait_queue_head_t		ioctl_event;	/* ioctl mailbox lock wait					*/
	wait_queue_head_t		ioexe_event;	/* Mailbox/SCSI completion wait				*/
	wait_queue_head_t		mck_event;		/* MCK sleep during mailbox proc			*/
	wait_queue_head_t		init_event;		/* detect i/f during link init and LOGIN	*/

	atomic_t				int_a_poll;		/* interrupt polling						*/
	atomic_t				mb_event_wait;
	atomic_t				ioctl_event_wait;
	atomic_t				mck_event_wait;
	uint					mck_on_sleep;	
	uint					mb_callback;

};

/* 
 * region_info : FIVE-FX region information
 *
 *  allocated by hfc_probe_one_fx()
 *  released  by hfc_remove_one_fx()
 *
 */
struct region_info {
	uchar					name[16];  		/* Set "region_info" characters			 	*/
	uint					status;			/* Queue status information					*/
	uchar					rid;			/* Region ID for queue						*/
	uint					port_num;		/* Number of region_info for vport			*/
	struct port_info		*pport;			/* Physical port_info address				*/
	struct port_info		*mb_pp;			/* port_info address for mailbox			*/
	
	/* core_info pointer																*/
	struct core_info		*core_arg[MAX_CORE_PROBE_FX];	/* Pointers to core_infos	*/
	
	uint					mb_lock;		/* lock bit for mailbox process				*/
	uint					mb_lock_event_wait;
	wait_queue_head_t   	mb_lock_event;	/* waitqueue until the lock bit would get	*/
};

/* FCLNX-GPL-FX-420 */
/*
 * socket_info : Socket information
 */
struct socket_info {
	int						socket_no;
#define HFC_SOCKET_NO_INVALID -1
	int						cpu_info_list_num;
	struct cpu_info 		*cpu_into_list;
	struct cpumask			cpumask;
	int						map_count;
};

/*
 * cpu_info : CPU information
 */
struct cpu_info {
	int						cpu_no;
#define HFC_FX_CPU_NO_INVALID -1

	struct socket_info		*socket_info;
	int						map_count;
};


/* 
 * hfc_intr_entry : FIVE-FX interrupt entry information
 */
struct hfc_intr_entry {
	uint					entry_num;		/* Vector entry no				*/
	uint					vector;			/* Interrupt vector no			*/
	uchar					mode;			/* Interruption mode			*/
	uchar					core;			/* Core no						*/
	uint					shr_info;		/* Interrupt factor				*/
	struct port_info		*pp;			/* Address of port_info			*/
	irqreturn_t				*handle;		/* Address of interrupt handler	*/
	struct cpumask			affinity;		/* CPU_MASK for smp_affinity	*//* FCLNX-GPL-FX-420 */
	ushort					cpu_no;			/* Logical CPU Number			*//* FCLNX-GPL-FX-420 */
	ushort					socket_no;		/* Socket Number				*//* FCLNX-GPL-FX-420 */

#define HFC_FX_INTR_ENTRY_NAME_SIZE	32
	char					name[HFC_FX_INTR_ENTRY_NAME_SIZE];
};

struct lpar_tp_fx {
	uint				frame_cnt;			/* MAX_FRAME_CNT value			*/  /* FCLNX-0374 */
	struct hg_map_fx	*hg_map;			/* MMIO-HG offset map	*/
};


/* FCLNX-GPL-FX-139 */
/* 
 * hfc_mb_trace : FIVE-FX Mailbox Trace
 */
struct hfc_mb_trace {
	uchar	flag;						/* +0 mb_trace_flag */
#define HFC_MBTRC_MBREQ			0x01	/* Mailbox Request		*/
#define HFC_MBTRC_MBRSP			0x02	/* Mailbox Response		*/
#define HFC_MBTRC_FRMSND_REQ	0x03	/* FRMSNDRCV Request	*/
#define HFC_MBTRC_FRMSND_RSP	0x04	/* FRMSNDRCV Request	*/
#define HFC_MBTRC_MBINT			0x05	/* Mailbox Interrupt	*/
	uchar	resv1;						/* +1 Reserve  */
	union{
		struct {
			ushort	mb_command;			/* +2-3 Mailbox Command 	*/
			uchar	resv2;				/* +4 Reserve  			*/
			uchar	core_no;			/* +5 core_no  			*/
			uchar	resv3[6];			/* +6-b Reserve  			*/
		}mb_req;
		struct {
			ushort	mb_command;			/* +2-3 Mailbox Command 	*/
			uchar	resv2;				/* +4 Reserve 			*/
			uchar	core_no;			/* +5 core_no  			*/
			ushort	mb_retry_cnt;		/* +6-7 Mailbox Retry Count  */
			uchar	fsb;				/* +8 fsb 	 			*/
			uchar	errcode[3];			/* +9-b errcode 			*/
		}mb_rsp;
		struct {
			uchar	payload_cmd;		/* +2 Payload Command 	*/
			uchar	resv2;				/* +3 Reserve 			*/
			ushort	payload_type;		/* +4-5 Payload Type		*/
			uchar	resv3[6];			/* +6-b Reserve  			*/
		}frmsndrcv_req;
		struct {
			uchar	payload_cmd;		/* +2 Payload Command 	*/
			uchar	resp_code;			/* +3 Accept(=0x00) or Reject(=0x01) or Invalid Length(=0x02)	*/
			ushort	payload_type;		/* +4-5 Payload Type		*/
			ushort	mb_retry_cnt;		/* +6-7 Mailbox Retry Count  */
			uchar	fsb;				/* +8 fsb 	 			*/
			uchar	errcode[3];			/* +9-b errcode 			*/
		}frmsndrcv_rsp;
		struct {
			ushort	mbint_code;			/* +2-3 Mailbxo Interrput	code */
			uchar	rctl;				/* +4 rctl				*/
			uchar	els_command;		/* +5 els_command 		*/
			uchar	resv3[6];			/* +6 Reserve 			*/
		}mb_intreq;
	}trc;
	uint	time;				/* +C Jiffies 4Byte		*/
};
/* FCLNX-GPL-FX-139 */


/* 
 * port_info : FIVE-FX adapter port information
 *
 *  allocated by hfc_probe_one_fx()
 *  released  by hfc_remove_one_fx()
 *
 */
struct port_info {
	uchar					name[16];  			/* Set "port_info" characters			 	*/
	uint					status;				/* Port status information					*/
#define HFC_PS_ENABLE				0			/* Detect f/w ready							*/
#define	HFC_PS_ONLINE				1			/* Link initialization succeeded			*/
												/* Link is in link-up status				*/
#define HFC_PS_WAIT_LINKUP			2			/* Waiting status change					*/
												/* from LINK_Down to LINK_Up				*/
#define	HFC_PS_WAIT_INITIALIZE		3			/* Initiate_Link_Initialization 			*/
												/* is in progress 							*/
#define	HFC_PS_CONNECTED			4			/* Link Ini was successed.*/
#define HFC_PS_WAIT_MCKINT			5			/* Waiting MCK INT							*/
#define HFC_PS_MCK_RECOVERY			6			/* MCK recovery is in progress				*/
#define HFC_PS_ISOL					7			/* Isolation process in progress 			*/
#define HFC_PS_DIAG					8			/* Diag process in progress 				*//* FCLNX-GPL-FX-126 */

#define HFC_PS_BLOCKED_SCSI 					\
	((0x00000001 << HFC_PS_WAIT_LINKUP)		|	\
	 (0x00000001 << HFC_PS_WAIT_INITIALIZE)	|	\
	 (0x00000001 << HFC_PS_MCK_RECOVERY)	|	\
	 (0x00000001 << HFC_PS_WAIT_MCKINT))

#define HFC_PS_BLOCKED_SCSI_PPORT 				\
	(0x00000001 << HFC_PS_WAIT_LINKUP)

	uint					status_detail1;		/* 1st detail information of Port status 	*/
#define HFC_PD_AFTER_LINKUP			0			/* Link Initialization after Link Up		*/
#define HFC_PD_NEED_CORE_START		1			/* Need to issue Core_Start Mailbox			*/
#define HFC_PD_WAIT_CORE_START		2			/* Core_Start is in porgress				*/
#define HFC_PD_NEED_LINK_INI		3			/* Need to issue Link_Ini Mailbox			*/
#define HFC_PD_WAIT_LINK_INI		4			/* Link_Ini. Mailbox is in porgress			*/
#define HFC_PD_NEED_FLOGI			5			/* Need to issue Fabric Login Mailbox		*/
#define HFC_PD_WAIT_FLOGI			6			/* Fabric Login Mailbox is in porgress		*/
#define HFC_PD_NEED_ADD_PORTID		7			/* Need to issue add Port_ID Mailbox		*/
#define HFC_PD_WAIT_ADD_PORTID		8			/* add Port_ID Mailbox is in porgress		*/
#define HFC_PD_NEED_SCR				9			/* Need to issue SCR Mailbox				*/
#define HFC_PD_WAIT_SCR				10			/* SCR Mailbox is in porgress				*/
#define HFC_PD_NEED_PLOGI_N			11			/* Need to issue Port Login to Name Server	*/
#define HFC_PD_WAIT_PLOGI_N			12			/* Port Login to Name Server is in progress	*/
#define HFC_PD_NEED_RFTID			13			/* Need to issue RFT_ID Mailbox				*/
#define HFC_PD_WAIT_RFTID			14			/* RFT_ID Mailbox is in porgress			*/
#define HFC_PD_NEED_RFFID			15			/* Need to issue RFF_ID Mailbox				*/
#define HFC_PD_WAIT_RFFID			16			/* RFF_ID Mailbox is in porgress			*/
#define HFC_PD_WAIT_RECEIVE_PLOGI	17			/* Waiting to receive PLOGI from Target		*/
#define HFC_PD_NEED_LOGO_FCSW		18			/* Need to issue LOGO Mailbox to FC-SW		*/
#define HFC_PD_WAIT_LOGO_FCSW		19			/* LOGO Mailbox to FC-SW is in porgress		*/
#define HFC_PD_NEED_DEL_PORTID		20			/* Need to issue delete Port_ID Mailbox		*/
#define HFC_PD_WAIT_DEL_PORTID		21			/* delete Port_ID Mailbox is in porgress	*/
#define HFC_PD_MB_KEEP_RETRY		22			/* Mailbox retry timer in progress			*/
#define HFC_PD_MB_DELAY				23			/* Delay before sending Mailbox				*/
#define HFC_PD_MB_DIAG_DELAY		24			/* Delay before changing to Diag mode		*//* FCLNX-GPL-FX-126 */
//#define HFC_PD_NEED_GPNID			25			/* Need to issue GPN-ID Mailbox				*/
//#define HFC_PD_WAIT_GPNID			26			/* GPN-ID Mailbox is in porgress			*/
#define HFC_PD_WAIT_ISOL_LINKUP_CNT	27			/* Wait Link Up at Isol mode				*//* FCLNX-GPL-FX-424 */

	uint					status_detail2;		/* 2nd detail information of Port status 	*/
#define	HFC_PD_LOGIN_DELAYI			0			/* Delay before sending LOGIN				*/
#define HFC_PD_NEED_GPNFT			1
#define HFC_PD_WAIT_GPNFT			2
#define	HFC_PD_NEED_OFFLINE_MB		3			/* Need to issue Offline_Mailbox			*/
#define	HFC_PD_WAIT_OFFLINE_MB		4			/* Offline_Mailbox is in porgress			*/
#define	HFC_PD_NEED_MIHLOG			5			/* Need to issue MIHLOG Mailbox				*/
#define	HFC_PD_WAIT_MIHLOG			6			/* MIHLOG Mailbox is in porgress			*/
#define	HFC_PD_NEED_LOAD_CH_TRACE	7			/* Need to issue Load ch_trace Mailbox		*/
#define	HFC_PD_WAIT_LOAD_CH_TRACE	8			/* Load ch_trace Mailbox is in porgress		*/
#define	HFC_PD_NEED_DIAG			9			/* Need to issue Diag						*/
#define	HFC_PD_WAIT_DIAG			10			/* Diag is in porgress						*/
#define	HFC_PD_LINK_RESET			11			/* Need to issue Link Reset					*/
#define	HFC_PD_WAIT_BUSRSP			12			/* Link Reset is in porgress				*/
#define HFC_PD_WAIT_CLOSE			13			/* Remove process in progress				*/
#define	HFC_PD_ISOLATE_PORT_C		14			/* Isolate Status by Command				*/
#define	HFC_PD_ISOLATE_PORT_E		15			/* Isolate Status by a threshold excess		*/
#define	HFC_PD_ISOLATE_SFPFAIL		16			/* Isolate Status by SFP Failure			*/
#define	HFC_PD_ISOLATE_SFPNOTSUPPORT	17		/* Isolate Status by unsupported SFP		*/
#define	HFC_PD_ISOLATE_SFPDOWN		18			/* Isolate Status by uninstalled SFP		*/
#define	HFC_PD_ISOLATE_CHKSTP		19			/* Isolate Status by Check Stop				*/
#define	HFC_PD_ISOLATE_CHKSTP_C		20			/* Isolate Status by Check Stop with CMD	*/
#define	HFC_PD_ISOLATE_RECOVERY		21			/* Isolate Recovery							*/
#define	HFC_PD_NEED_SHADOW_UP		22			/* Need to issue Shadow_Up_Mailbox			*/
#define	HFC_PD_WAIT_SHADOW_UP		23			/* Shadow_Up Mailbox is in porgress			*/
#define HFC_PD_FLASH_UPDATE_PROCESS 24			/* Flash Update is in progress				*/

//@MLPF
#define HFC_PD_MLPF_WAIT_FMCK		25			/* waiting FMCK INT                         */
#define HFC_PD_MLPF_WAIT_MCKEND		26			/* waiting MCKEND                           */
#define HFC_PD_MLPF_WAIT_FCSTP		27			/* waiting FCSTP INT                        */
#define HFC_PD_MLPF_WAIT_CSTPEND	28			/* waiting CSTPEND                          */
#define HFC_PD_MLPF_WAIT_LINKEND	29			/* waiting Link Ini End                     */
//@MLPF

#define HFC_PD2_BLOCKED_SCSI 					\
	((0x00000001 << HFC_PD_WAIT_BUSRSP)	|		\
	 (0x00000001 << HFC_PD_WAIT_CLOSE))
	 
	/* FCLNX-GPL-FX-067 */
#define HFC_PD2_SFP_ERROR_ANY					\
	((0x00000001 << HFC_PD_ISOLATE_SFPFAIL)	|	\
	 (0x00000001 << HFC_PD_ISOLATE_SFPDOWN)	|	\
	 (0x00000001 << HFC_PD_ISOLATE_SFPNOTSUPPORT))
	/* FCLNX-GPL-FX-067 */

	struct region_info		*region_arg[MAX_REGION_PROBE];	/* Pointers to region_infos		*/
	struct target_info_fx	*target_arg[MAX_TARGET_PROBE];	/* Pointers to target_info_fxs	*/
	struct vport_info		*vport_ptr;						/* Pointers to vport_infos		*/
//	struct mp_adap_info		*mp_adap_info;	/* Pointer to mp_adap_info which				*/
//											/* is connected to this adap_inf				*/
//	struct card_info		*card_info;		/* Pointer to card_info which					*/
//											/* is connected to this adap_inf				*/
	struct port_info		*next;			/* Next port_info								*/
	struct port_info		*prev;			/* Previous port_info							*/

	struct pkg_tp			pkg;			/* package config infotmation					*/
											/*	pkg.type (FPP/Five)							*/
											/*	pkg.port (1/2/4)							*/
											/*	pkg.pkcode (0x80/0x81/0x82)					*/
											/*	pkg.map (pointer to reg map)				*/
	uint					rid;			/* Number of queue for the I/O					*/
	uint					sub_rid;		/* Number of queue for the I/O in region		*/
	uint					hvm_rid;		/* The number of LPAR at HVM					*/

	uint64_t				ww_name; 		/* HBA world wide port name						*/
	uint64_t				node_name; 		/* HBA world wide node name						*/
	uint64_t				scsi_id; 		/* HBA SCSI-id 									*/
#define HFC_PTOP_INIT_PORTID	0x00000001		/* FCLNX-GPL-FX-066 */

	struct manage_info		*manage_info;
	int						instance;
	int						unique_id;
	struct port_info		*pport;			/* Address of physical port_info				*/
	uint					vport_id;		/* vport id	for 255 npiv						*/

	struct lpar_tp			lparmode ;		/* Information only for LPAR					*/

	int						dev_major;		/* major number									*/
	int						dev_minor;		/* minor number									*/
										
	uchar					mlpf_mode;		/* MLPF mode									*/
//#define	HFC_MMODE_MLPF		0x80 		/*  LPF effective(common & occupation)			*/
//#define	HFC_MMODE_DEDICATE	0x40 		/*  Occupation bit								*/
//#define	HFC_MMODE_SHADOW	0x08		/*  SHADOW LPAR									*/
//#define	HFC_MMODE_REBOOT	0x04		/*  SHADOW REBOOT								*/

	uchar					npiv_mode;		/* NPIV  mode									*/
#define HFC_NPIV_ENABLE		0x80
#define HFC_NPIV_SHAREABLE	0x40
#define HFC_NPIV_EXT_MODE	0x20
#define	HFC_NPIV_VPORT		0x08

	uchar					mq_mode;
#define HFC_MQ_ENABLE		0x80
#define HFC_MQ_VALID		0x40
#define	HFC_MQ_VPORT		0x08

	uchar					connect_type;	/*  connection type								*/
#define	HFC_FX_CTUNKN		0x00 		/*		 connection unknown						*/
#define	HFC_FX_PT2PT		0x01 		/*		 Point To Point 						*/
#define	HFC_FX_SWITCH		0x02 		/*		 Switch(Fabric) 						*/
#define	HFC_FX_AL			0x03 		/*		 FC_AL									*/
#define HFC_FX_F_PORT		0xF0		/*		 F_Port									*/
#define	HFC_FX_MULTI_ALPA	0xF3 		/*		 FC_AL(Multi ALPA)						*/

	uint					max_data_rate;	/* transfer rate 								*/
#define HFC_4000MBS				4000			/* Transfer rate is 10Gbps					*/
#define HFC_1600MBS				1600			/* Transfer rate is 10Gbps					*/
//#define	HFC_1000MBS			1000			/* Transfer rate is 10Gbps					*/
//#define	HFC_800MBS			800				/* Transfer rate is 8Gbps					*/
//#define	HFC_400MBS			400				/* Transfer rate is 4Gbps					*/
//#define	HFC_200MBS			200				/* Transfer rate is 2Gbps					*/
//#define	HFC_100MBS			100				/* Transfer rate is 1Gbps					*/

	uchar					used_nmsrv;		/* System is using name server 					*/
	uchar					port_no;		/* port number of card (pci_cfginf->devfn)		*/

	uint					sys_rev;		/* Firmware system revision						*/
	uchar					vpd_buf[512];	/* FPP VPD information data area				*/
	uchar					adap_id[16];	/* Adapter info. identification number			*/
	uchar					model_name[16]; /* Model Name									*/
	uint					ecid[16];		/* ECID											*/

	ushort					flogi_max_frame_size;	/* FLOGI Max.Frame Size acquired by FLOGI Responce 	*/
	uchar					flogi_rsp_param;		/* FLOGI RSP_Param acquired by FLOGI Responce 		*/
	uchar					flogi_config_flag;		/* FLOGI Configure Flag acquired by FLOGI Responce	*/

	uint64_t				flogi_ww_name;			/* target WWPN in F_PORT mode			*/
	uint64_t				flogi_node_name;		/* target WWNN of F_PORT mode			*/

	uint64_t				fabric_ww_name;			/* WWPN of Name Server					*/
	uint64_t				fabric_node_name;		/* WWNN of Name Server					*/
	uint64_t				fabric_d_id;			/* D_ID od Name Server					*/
	uint64_t				fabric_s_id;			/* S_ID od Name Server					*/
	uint					fabric_flag;			/* Flag of Fabric						*/

	uint					attach_status;	/*Initialization has finished 					*/
//#define HFC_ATTACH			0			/* detect() has finished						*/
//#define HFC_DETACH			1			/* release() has finished 						*/

	uint					open_status; 	/* Availability of hfc_open func				*/
//#define HFC_OPENED			1				/* diag_open() has finished					*/
//#define HFC_CLOSED			0				/* diag_close() has finished				*/
//#define HFC_DIAG_OPENED		2

	uchar					fcp_mode ;		/* Diag used									*/
	uchar					link_reset_multi_mode;	/* Set the flag to want the path change	*/
	
	struct file				*open_file;		/* File address of write processing				*/
	uchar					ioctl_status;	/* ioctl status									*/
#define HFC_FX_WRITE_PROCESS	0
	uint					open_cnt ;		/* Number of opens								*/

	struct Scsi_Host		*hosts;			/* Pointer to the Scsi_Host struct. 			*/
	ulong					host_no;		/* Used for IOCTL_GET_IDLUN, /proc/scsi 		*/
	struct pci_dev			*pci_cfginf;	/* Pointer to PCI Config Base					*/
											/* return value of pci_find_device()			*/
	ulong					mem_base_addr;	/* Pointer to PCI Memory space					*/
	ulong					hg_mem_base_addr ;	/* MMIO-HG space Base addr					*/
										  	/* by pci_resouce_start()						*/

	struct Scsi_Host		*hfchsd_host;
	void					*hfchsd_pp;
	struct Scsi_Host		*hfcpfb_host;
	void					*hfcpfb_pp;
	struct scsi_cmnd		*bus_reset_pkt;

	struct hfc_pkt_fx		*pkt_top;		/* Top pointer for hfc_pkt pool					*/
	struct hfc_pkt_fx		*pkt_next;		/* Next pointer for hfc_pkt pool				*/
	struct hfc_pkt_fx		*pkt_end;		/* End pointer for hfc_pkt pool					*/
	struct hfc_pkt_fx		*rsv_pkt_pool;	/* Pointer for reserve hfc_pkt pool				*/
	struct hfc_pm_pkt_fx	*pm_pkt_pool;	/* Pointer for hfc_pm_pkt pool					*/
//	struct scsi_cmnd		*cmnd_pool;		/* Scsi_Cmnd Pointer for Task Management		*/
	struct scsi_cmnd		*ioctl_cmnd;	/* Scsi_Cmnd Pointer for ioctl					*/
	struct dev_info_fx		*ioctl_dev;		/* Dev_info Pointer for ioctl					*/
	uint					*hw_log;		/* Pointer to hw_log							*/
#define HFC_FX_HWLOG_SIZE   0x20000			/* 128KB				 						*/
	uint					pm_pkt_no;		/* ID of the end hfc_pm_pkt						*/
	uint					rsv_pkt_no;		/* ID of the end reserve hfc_pkt				*/
	uint					target_cnt;		/* Number of the Valid targets					*/
	uint					max_target;		/* Number of target_info_fxs						*/

	struct target_info_fx		*login_target;	/* Target_info list to send Login				*/
	struct target_info_fx		*logo_target;	/* Target_info list to send LOGO				*/
//	struct target_info_fx		*plogi_target;	/* Target is in LOGIN wait state				*/
//	struct target_info_fx		*prli_target;	/* Target is in LOGIN wait state				*/
	struct target_info_fx		*next_tstart;	/* Target is in PDISC wait state				*/
	uchar					next_gidpn;		/* GID_PN request exists						*/

	uint					master_core_no; /* The number of Master Core					*/
	uint					available_pcore;
	uint					core_num;		/* A number of Cores							*/

	uint64_t				scsi_exec_cnt;	/* Total number of SCSI cmds 					*/
	uint64_t				scsi_end_cnt;	/* Initiated SCSI cmds							*/
	uint64_t				scsi_err_cnt;	/* SCSI cmds with error							*/
	uint					hfcpkt_full_cnt;	/* hfcpkt full count						*/
	uint					dummy_int_rst_cnt;	/* dummy int reset count					*/

	/* Timer watch */
	uint					timer_flag ;	/* Timer control flag							*/

	uint					vport_num;		/* Number of port_info for vport				*/

	uint					vector_num;		/* Number of vector num							*/

	uchar					curr_core;		/* Just used core num counter
											   to choose next core							*/
	uchar					numa_node0_curr_core;	/* Just used core num counter
											   to choose next core at Numa Node				*/
	uchar					numa_node1_curr_core;	/* Just used core num counter
											   to choose next core at Numa Node				*/

	uint 					link_dead_cnt;	/* xrb=83,fsb=ccc or xrb v=0					*/
//#define	HFC_LINK_DEAD_CNT		5
	uint					pci_err_cnt;	/* PCI error count								*/
//#define	HFC_PCIERR_CNT			2
	uint					mck_err_cnt;	/* MCKINT error count							*/
//#define	HFC_MCKERR_CNT			8
	int						pcie_sram_ce_cnt;	/* PCIe IP Core SRAM ERR(CE)				*/
//#define	HFC_PCIE_SRAM_CE_CNT	4
//	int						core_ce_cnt;	/* Core 1bit ERR(CE) 							*/
//#define	HFC_CORE_CE_CNT			9

	uint					io_status;
//#define	HFC_LPTEST_ALRDY_INTREQ	0		/* Throw away INTRQ1							*/
//#define	HFC_HWLOG_VALID			1		/* HW LOG area valid							*/
	uchar					issue_d3hot;	/* change power state in suspend process 		*/

	uint					fw_parm;		/* Firmware Parameter							*/
//#define	HFC_FWP_SEGTRC_V	0x00000001	/* Valid SEG Fetch Trace						*/

	uint					seq_num_tbl[((MAX_TARGET_PROBE+31)/32)];
											/* PSEQ number control table					*/
	uchar					tid_map[MAX_TARGET_PROBE];
											/* Table for translation from					*/
											/* TARGET ID to PSEQ number						*/
	struct target_scan		*target_scan;	/* Control for target search					*/
	uchar					*errlog_buf;	/* Pointer to the error log area				*/
	uint					errlog_num;		/* Number of error logs							*/
	uint					errlog_max;		/* Max size of error log area					*/
	uchar                   mck_result;     /* mck execution result							*/
//#define	HFC_MCK_SUCCESS            0
//#define	HFC_MCK_HW_INIT            1
//#define	HFC_MCK_HWCHKSTOP          2
//#define	HFC_MCK_CNT_OVER           3
//#define	HFC_MCK_END                4
//#define	HFC_MCK_EXEC               5

	struct hfctrace			*trc_ptr;		/* Pointer to the top trc area 					*/
	uint					trc_num;		/* Current pointer to trc area					*/
	uint					trc_max;		/* Max size of trc area							*/
#define HFC_MAX_TRCCNT  256					/* Max trace count	 							*/
	ushort					seq_no;			/* trc entry number								*/

	uchar					initialize;		/* Initialize() is being executed(!=0)			*/
	uchar					no_target;		/* Support Link_Ini Delay						*/

	uchar					min_port_in_region;	/* minimum port in region					*/

	struct hg_cca_fx		*hg_cca_p;		/* uncached memory area shared Hypervisor		*//* FCLNX-GPL-FX-433 */
	struct payload_fx		*payload;		/* Payload Virtual Address for FRMSNDRCV		*/
	uchar					*rcvfrm_payload;	/* Receive Frame Payload Address			*/

	uint                    err_no;			/* It uses it with MCK LOGOUT					*/
	uchar                   mode;			/* It uses it with MCK LOGOUT					*/
	uchar					mck_type;		/* It uses it with MCK LOGOUT					*/
	uchar					mck_core_no;	/* It uses it with MCK LOGOUT					*/

	uint64_t				add_ww_name; 	/* HBA world wide port name						*/
	uint64_t				add_node_name; 	/* HBA world wide node name						*/
	uint64_t				org_ww_name; 	/* HBA world wide port name						*/
	uint64_t				org_node_name; 	/* HBA world wide node name						*/
	
	uint64_t				vfc_ww_name;	/* HBA world wide port name for MLPF			*/
	uint64_t				vfc_node_name;	/* HBA world wide node name for MLPF			*/

	uint					ioctl_lock;		/* lock bit for ioctl_event						*/
//#define	HFC_IOCTL_BUSY		0

	uint					diag_cnt;		/* Diag Exec. Count								*/

	struct lg_target_info_fx	*lg_target;		/* Chain lg_target in adap_info					*/

	struct hfc_pkt_fx		*retry_hfcp_top;
	struct hfc_pkt_fx		*retry_hfcp_end;
	int						retry_hfcp_count;
	uchar					host_scan;
//#define HFC_SCSI_HOST_RESCAN	0
	uchar					mlpf_drv_log[64];

	uchar					narrowmap;		/* Narrow MAp 0:Disable,1:Enable 				*/
	struct {
		uint64_t			ww_name;
		uint				lun;
	}boot_priority[8];

	uchar					spinup_delay;			/* Spinup Delay Enable					*/
	uchar					opt_vendor_name[32];	/* Optical module vendor name			*/
	uchar					opt_parts_number[32];	/* Optical module parts number			*/
	uchar					opt_serial_number[32];	/* Optical module sirial number			*/
	int						scsi_timeout_fail;		/* SCSI timeout fail count				*/
	uint64_t 				inputrequests;			/* SCSI command count(read)				*/
	uint64_t				outputrequests;			/* SCSI command count(write)			*/
	uint64_t				controlrequests;		/* SCSI command count(not data move)	*/
	uint64_t				inputbytes;				/* number of bytes of read data			*/
	uint64_t				outputbytes;			/* number of bytes of write data		*/
	
	uint					retry_cnt[HFC_MAX_ERROR_NUM];
											/* Threshold of SCSI Command Retry				*/
//#define	HFC_MAX_ERR_RETRY		2048
//#define	HFC_MIN_ERR_RETRY		0

	uchar					c_err;			/* Factor of isolation		 					*/
//#define	HFC_ISOLATE_LDL			1
//#define	HFC_ISOLATE_LDS			2
//#define	HFC_ISOLATE_IF			3
//#define	HFC_ISOLATE_TO			4
//#define	HFC_ISOLATE_RT			5
//#define	HFC_ISOLATE_FA			6
//#define	HFC_ISOLATE_FP			7
//#define	HFC_ISOLATE_TGT_LDL		8
//#define	HFC_ISOLATE_TGT_LDS		9
//#define	HFC_ISOLATE_SHADOW		10

	int						hba_isolation;
#define HFC_ISOL_STOP			0
#define HFC_ISOL_START			1
	int						isol_type;		/* Type to isolate  							*/
//#define	HFC_ISOL_TYPE_ADPT		0
//#define	HFC_ISOL_TYPE_PORT		1
	int						isol_force;		/* Forced isolation 							*/
//#define	HFC_NO_FRC_ISOL			0 		/* Do not isolate 								*/
//#define	HFC_PRT_FRC_ISOL		1 		/* Force port into isolated state 				*/
//#define	HFC_CHKSTP_FRC_ISOL		2		/* Force adapter into isolated state 			*/
//#define	HFC_SHARED_PRT_FRC_ISOL	3		/* Isolate physical port						*/

	uint					ld_err_count_s;		/* Error Counter for Short Link-down error	*/
	uint					if_err_count;		/* Error Counter for FC Interface error		*/
	uint					to_err_count;		/* Error Counter for SCSI T.O error			*/
	struct errcnt_info		*ldl_errcnt_info;	/* Counter for Long Link-down error			*/
	struct errcnt_info		*lds_errcnt_info;	/* Counter for Short Link-down error		*/
	struct errcnt_info		*if_errcnt_info;	/* Counter for FC Interface error			*/
	struct errcnt_info		*to_errcnt_info;	/* Counter for SCSI T.O error				*/
	
	short					login_retry;		/* Number of Retry for Login and Pdisc 		*/
	short					els_retry;			/* Number of Retry for ELS					*/
	short					to_reset_retry;		/* Number of Retry for LOGIN for After SCSI T.O */
	short					scsi_to_retry;		/* Number of Retry for SCSI Command			*/
	short					post_retry_cnt;		/* Number of retry for POST					*/

	uint					raslog_install;		/* raslog support 							*/
	uint					raslog_cnt;
	int						raslog_ver;
	int						raslog_rev;
	int						raslog_rver;
	int						raslog_wver;
	uchar					ioctl32;

//	uchar					isol_detail;
//#define	HFC_NO_ISOLATE				0x00
//#define	HFC_ISOLATE_PORT_C			0x01
//#define	HFC_ISOLATE_PORT_E			0x02
//#define	HFC_ISOLATE_SFPFAIL			0x05
//#define	HFC_ISOLATE_SFPNOTSUPPORT	0x06
//#define	HFC_ISOLATE_SFPDOWN			0x07
//#define	HFC_ISOLATE_CHKSTP			0x08
//#define	HFC_ISOLATE_CHKSTP_C		0x09
	uint					wait_isol;
//#define	HFC_WAIT_ISOL_ERR		0
//#define	HFC_WAIT_ISOL_CMD		1
//#define	HFC_WAIT_ISOL_REC		2
//#define	HFC_RASLOG_RETRY		3

	uint64_t				adapt_wwpn[4];

	uchar					login_type;
//#define	HFC_SCAN_DEVICE			0
//#define	HFC_CANCEL_IO			1
//#define	HFC_CANCEL_LOGIN		2

	uchar					mck_point; 
//#define	HFC_NO_MCK_POINT			0
//#define	HFC_BEFORE_POSTCHK			7
//#define	HFC_BEFORE_LINKINITIALIZE	12
//#define	HFC_AFTER_LINKINITIALIZE	13
//#define	HFC_AFTER_SCSI_HOST_RESCAN	18
//#define	HFC_MAX_MCK_POINT			255

	uchar					imllog_logout;
	uchar					after_isolrec;	/* FCLNX-GPL-FX-462 */

	uint					mck_linkup;
//#define	HFC_LINKUP_NOMCK		0
//#define	HFC_LINKUP_MCK			1

	uint64_t				dma_mask;

	uchar					log_wk[512];
	uchar					detail_data[HFC_RASLOG_LEN];
	struct hfc_err_rec		err_rec;

	int 					msi_flag; 		/* Running INT type. 							*/
/* Value entries for msi_enable and msi_flag
 *      0: INTx
 *      1: MSI
 *      2: MSI-X
 *      3: MSI-X (Multi Queue)
 *      4: MSI (Vector Shortage)
 *      5; MSI-X (Vector Share)
 */

	/* [FIVE-EX] We use these values to stock data of before H/W initialize. */
	uint					hw_init_status0;
	uint					hw_init_status1;
	uint					hw_init_detail0;
	uint					hw_init_pcierr;

	uchar					mck_progress;
//#define	HFC_MCK_PROGRESS	0x80

	uchar					kthread_status;
//#define	HFC_KTHREAD_RUN			0
	ulong					reset_stat_time;
	uchar					sysfs_control;
//#define	HFC_SYSFS_RPORT			0
//#define	HFC_SYSFS_STATISTICS	1
	
	uchar					issue_lip;
#define	HFC_SYSFS_ISSUE_LIP		1
	
	uchar					pm_control;
#define	HFC_FX_PM_OFF			0			/* performance monitor off */
#define	HFC_FX_PM_ON			1			/* performance monitor on  */
	
	uint64_t				tx_frames[MAX_CORE_PROBE_FX];
	uint64_t				rx_frames[MAX_CORE_PROBE_FX];
	uint64_t				tx_words[MAX_CORE_PROBE_FX];
	uint64_t				rx_words[MAX_CORE_PROBE_FX];

	uint					mb_prol_wake_up_time;
	uint					mb_prol_sleep_end_time;
	
	int						int_check;
	uint					is_busmaster:1;	/* This device is busmaster						*/
	int						pm_event;
	uint					fw_support;
//#define	HFC_SUPPORT_FW_ISOL			0	/* HBA FW Support bit Isolate Operation & SFP Inf	*/
//#define	HFC_SUPPORT_HVM_ISOL		1	/* HVM FW Support bit 							*/
//#define 	HFC_SUPPORT_LINKINI_DELAY	2	/* Link Ini Delay Support bit					*/
	uchar					isol_cmd_mck_cnt;
	uchar					logdata[16] ;

	uchar					frame_ctl;
	uchar					fc_class;
	uchar					flogi_param;
	uchar					plogi_param;
	uchar					switch_exist;
#define	HFC_SWITCH_EXIST		0x01
	uchar					mailbox_pseq;	/* pseq for PLOGI and PRLI			*/
	uchar 					vfab_enable;
	uchar 					vfab_mode;
	uchar 					fcsp_flag;
	
	uchar					multiple_portid;	/* 0:disable 1:enable *//* FCLNX-GPL-135 */

	ushort					socket_num;			/* the number of Socket at this server 			*//* FCLNX-GPL-FX-201 */
//	ushort					cpu_core_num;		/* the number of Physical CPUs at this server 	*//* FCLNX-GPL-FX-201 */
//	uint					logical_cpu_num;	/* the number of logical CPUs at this server 	*//* FCLNX-GPL-FX-201 */
//	uint					online_cpu_num;		/* the number of online CPUs at this server 	*//* FCLNX-GPL-FX-201 */

	struct mb_timer_t		mb_timer[HFC_MBTIME_MAX];	/* Parameter related to Mailbox timer */
	int						core_deg_mode;
#define	HFC_FX_CORE_DEG_ENABLE		0
#define	HFC_FX_CORE_DEG_DISABLE		1

	uint 					issue_mailbox;				/* Mailbox to be issued at next time. */
#define HFC_NMB_CANCEL_SCSI_T_WITHOUT_DMA		1
#define HFC_NMB_CANCEL_SCSI_T_WAIT_DMA			2
#define HFC_NMB_CANCEL_SCSI_D_WITHOUT_DMA		3
#define HFC_NMB_CANCEL_SCSI_D_WAIT_DMA			4
#define HFC_NMB_OFFLINE_MB			5
#define HFC_NMB_CORE_START			6
#define HFC_NMB_LINK_INI			7
#define HFC_NMB_FLOGI				8
#define HFC_NMB_LOGO_FCSW			9
#define HFC_NMB_DEL_PORTID			10
#define HFC_NMB_ADD_PORTID			11
#define HFC_NMB_SCR					12
#define HFC_NMB_PLOGI_NS			13
#define HFC_NMB_RFTID				14
#define HFC_NMB_RFFID				15
#define HFC_NMB_GPNFT				16
#define HFC_NMB_PRLI_T				17
#define HFC_NMB_PLOGI_T				18
#define HFC_NMB_LOGO_T				19
#define HFC_NMB_MIHLOG				20
#define HFC_NMB_LOAD_CH_TRACE		21

	/* for common parameters */
	uchar					inta_dummy_read;
											/* "1:Do" or "0:Not" dummy read.(for MSI/MSIX)	*/
//#define	HFC_DUMMY_READ_OFF	0
	uint					debug_func;
//#define	HFC_DEBUG_CTLRST			0x80
//#define	HFC_DEBUG_MEM_LEAK			0x40
//#define	HFC_DEBUG_RW_BAR1			0x20
//#define	HFC_DEBUG_LINK_WIDTH_CHK	0x10
//#define	HFC_DEBUG_FIVE_MSI			0x08
//#define	HFC_DEBUG_POST_LOGOUT		0x04
//#define	HFC_DEBUG_HYP_INT_CHK		0x02
#define	HFC_FX_DEBUG_IOTRACE_OFF	0x00000100
#define	HFC_FX_DEBUG_HOST_LOCK_OFF	0x00000200
#define	HFC_FX_DEBUG_SKIP_RW_BAR1	0x00000400
#define	HFC_FX_DEBUG_SKIP_DS_CHECK	0x00000800

	int						linkup_tmo;		/* Timer value for linkup wait					*/
	int						mck_rcv_tmo;	/* Timer value for linkup wait	(MCK)			*/
//#define HFC_LINKUP_TM_MIN	0
//#define HFC_LINKUP_TM_MAX	60
	int						link_initialize_tmo;	/* Timer valus for link initialization process */
	int						link_ini_mb_tmo;		/* Timer valus for link_ini mailbox		*/
	int						scsi_reset_delay;/* Supress SCSI after linkup					*/
//#define HFC_SCSI_RESET_DELAY_MIN	0
//#define HFC_SCSI_RESET_DELAY_MAX	60
	int						dma_max;		/* Max DMA transfer length						*/
	ushort					xob_max;		/* Max xob numbers								*/
	ushort					xrb_max;		/* Max xrb numbers								*/
	ushort					slog_max;		/* Max softlog numbers							*/
	uchar					pref_alpa;		/* AL-PA number during link_ini 				*/
	uchar					host_alpa;		/* AL-PA number when link_ini 					*/
											/* had finished									*/
	uchar					enable_tgtrst;	/* Target Reset Effective/invalidity			*/
	uchar					automap;		/* Automap 0:Disable,1:Enable					*/
	uchar					defparam;		/* Forced Default Parameter 0:Dis,1:Enb			*/
	uchar					topology;		/* 0:Auto,1:P2P,2:FC-AL 						*/
	uchar					linkspeed;		/* 1/2/4/10 Gbps								*/
	int 					max_mck_cnt;	/* Max MCKINT Error 							*/
#define HFC_FX_MAX_MCK_CNT_MIN	0
#define HFC_FX_DF_MAX_MCK_CNT	8
#define HFC_FX_MAX_MCK_CNT_MAX	10
	int						max_transfer;	/* Max transfer length 							*/
	int						target_reset_tmo;	/* target reset timeout 					*/
	int						abort_tmo;		/* abort timeout 								*/
	int						lun_reset_tmo;	/* lun reset timeout 							*/
	int						queue_depth;	/* queue depth									*/
	int						wmsg;			/* message enable								*/
	int						pkt_num;		/* hfc_pkt num									*/
	int						pre_pkt_num;	/* pre reserve hfc_pkt num						*/
	int						rsv_pkt_num;	/* reserve hfc_pkt num							*/
	int						pm_pkt_num;		/* performance hfc_pkt num						*/
	int						can_queue;		/* can_queue									*/
	int						sg_tblsize;		/* sg tablesize									*/
	int						cmnd_num;		/* cmnd num										*/
	int						minus_tout;		/* minus timeout								*/
	int						scsi_allowed;	/* scsi allowed									*/
//#define HFC_SCSI_ALLOWED_MIN	0
//#define HFC_SCSI_ALLOWED_MAX	30
	int						ioctl_scsi_timeout;	/* ioctl timeout period 					*/
	int						cmd_per_lun;	/* cmd_per_lun									*/
	int						max_sectors;	/* max_sectors									*/
	int						vary_io;		/* vary_io										*/
	int						login_wait;		/* Login Delay Time								*/
//#define HFC_LOGIN_DELAY_TIME_MIN	0
//#define HFC_LOGIN_DELAY_TIME_MAX	60
	char					limit_log;		/* Limit Log 									*/
//#define	HFC_DISABLE_LIMITLOG	0
//#define	HFC_ENABLE_LIMITLOG		1
#define	HFC_LOGMODE_EXT			2
	char					filter_target;	/* Filtering Login Target 						*/
#define	HFC_FX_MB_LOGIN_FILTER_ON	0
#define	HFC_FX_MB_LOGIN_FILTER_OFF	1

	char					hg_stats_disable;	/* Disable function to get statistics
																				for Virtage */
//#define	HFC_ENABLE_HGSTATS		0
//#define	HFC_DISABLE_HGSTATS		1
	char					port_chkstp;	/* Port Check-Stop Mode */ /* FIVE-FX */
#define 	HFC_DISABLE_PORT_CHKSTP	0
#define 	HFC_ENABLE_PORT_CHKSTP	1
	int						max_pcie_sram_ce_cnt;	/* Max PCIe IP Core SRAM ERR(CE) count	*/
	int						max_core_ce_cnt;		/* Max Core 1bit ERR(CE) count			*/
	int						max_hwlog_cnt;			/* Max hwlog count						*/
#define	HFC_FX_HWLOG_CNT			4
	uint					scsi_time_out;		/* SCSI TIME OUT return code change			*/
	int						lun_reset_delay;	/* Supress SCSI after LUN Reset				*/
//#define	HFC_LUN_DELAY			0
	int						abort_t_restrain;	/* Abort Task Set restrain					*/
//#define	HFC_ABORT_T_RESTRAIN	0
	int						login_restrain;		/* login restrain							*/
//#define	HFC_LOGIN_RESTRAIN	0
//#define	HFC_MAX_ERROR_NUM		1000
	uint					ld_err_intvl;	/* Interval of Error Count for Link-down Error  */
//#define	HFC_DF_LD_ERR_INTVL		30
//#define	HFC_MIN_LD_ERR_INTVL	1
//#define	HFC_MAX_LD_ERR_INTVL	60
	uint					ld_err_limit_l;	/* Maximum error count for long Link-down Error */
//#define	HFC_LDLERR_NOT_SPPRTD	0
//#define	HFC_MIN_LD_ERR_LIMIT_L	0
//#define	HFC_MAX_LD_ERR_LIMIT_L	30
	uint					ld_err_limit_s;	/* Maximum error count for Short Link-down Error*/
//#define	HFC_LDSERR_NOT_SPPRTD	0
//#define	HFC_MIN_LD_ERR_LIMIT_S	0
//#define	HFC_MAX_LD_ERR_LIMIT_S	30
	uint					if_err_intvl;	/* Interval of Error Count for Short Link-down Error */
//#define	HFC_DF_IF_ERR_INTVL		30
//#define	HFC_MIN_IF_ERR_INTVL	1
//#define	HFC_MAX_IF_ERR_INTVL	60
	uint					if_err_limit;	/* Maximum error count for Interface Error		*/
//#define	HFC_IFERR_NOT_SPPRTD	0
//#define	HFC_MIN_IF_ERR_LIMIT	0
//#define	HFC_MAX_IF_ERR_LIMIT	2048
	uint					to_err_intvl;	/* Interval of Error Count for Short interface error */
//#define	HFC_DF_TO_ERR_INTVL		30
//#define	HFC_MIN_TO_ERR_INTVL	1
//#define	HFC_MAX_TO_ERR_INTVL	60
	uint					to_err_limit;	/* Maximum error count for SCSI timeout error	*/
//#define	HFC_TOERR_NOT_SPPRTD	0
//#define	HFC_MIN_TO_ERR_LIMIT	0
//#define	HFC_MAX_TO_ERR_LIMIT	2048
	uint					rt_err_enable;	/* Maximum error count for Reset timeout error	*/
//#define	HFC_RTERR_NOT_SPPRTD	0
//#define	HFC_RT_ERR_NOT_SPPRTD	0
//#define	HFC_RT_ERR_SPPRTD		1
	int						msi_enable;		/* Set INT type.								*/
/* Value entries for msi_enable and msi_flag
 *      0: INTx
 *      1: MSI
 *      2: MSI-X
 *      3: MSI-X (Multi Queue)
 *      4: MSI (Vector Shortage)
 *      5; MSI-X (Vector Share)
 */
	uint					dev_loss_tmo;
//#define HFC_MIN_DEV_LOSS_TMO	1
//#define HFC_MAX_DEV_LOSS_TMO	255
//#endif /* SYSFS_SUPPORT

	int						scan_finished_tmo;
//#define HFC_SCAN_FINISHED_TMO		30
//#define HFC_MIN_SCAN_FINISHED_TMO	0
//#define HFC_MAX_SCAN_FINISHED_TMO	3600

	uint					rport_lu_scan;
//#define HFC_DISABLE_RPORT_LU_SCAN 0
//#define HFC_ENABLE_RPORT_LU_SCAN  1

	uint					ctl_change_qdepth;
//#define HFC_ENABLE_CTL_CHANGE_QDEPTH	1
//#define HFC_DISABLE_CTL_CHANGE_QDEPTH	0


	/* for only FIVE-FX parameters */
	char					core_control;		/* Core choice mode								*/
#define	HFC_FX_CORECTL_ENHANCE_RR	0
#define	HFC_FX_CORECTL_SEQUENTIAL	1
#define	HFC_FX_CORECTL_CORE_LIST	2
#define	HFC_FX_CORECTL_LARGE_DATA	3
#define	HFC_FX_CORECTL_SINGLECORE	4
#define	HFC_FX_CORECTL_AT_ASSGN_LU	5
#define	HFC_FX_CORECTL_CPU_TO_CORE	6	/* FCLNX-GPL-FX-193 */

	uint					cc_cnt;
#define	HFC_FX_MIN_CC_CNT			0
#define	HFC_FX_MAX_CC_CNT			65535
#define	HFC_FX_DEF_CC_CNT			32

	uint					cc_size;
#define	HFC_FX_MIN_CC_SIZE			1
#define	HFC_FX_MAX_CC_SIZE			32768	/* FCLNX-GPL-FX-292, 302 */
#define	HFC_FX_DEF_CC_SIZE			1024

	uchar					cc_core;
#define	HFC_FX_DEF_CC_CORE			0x00

	uchar					link_reset;
#define	HFC_FX_LINK_RESET_MULTI		0
#define	HFC_FX_LINK_RESET_SINGLE	1

	int						tgtrst_restrain;	/* Target Reset restrain						*/
#define	HFC_FX_TGTRST_RESTRAIN_DISABLE	0
#define	HFC_FX_TGTRST_RESTRAIN_ENABLE	1

	uchar					rft_id_skip;		/* rft id skip 									*/
#define	HFC_FX_RFT_ID_SKIP_DISABLE	0
#define	HFC_FX_RFT_ID_SKIP_ENABLE	1

	uchar					isol_cmd;			/* command blockade								*/
#define	HFC_FX_ISOL_CMD_OFF			0
#define	HFC_FX_ISOL_CMD_ON			1

#define	HFC_FX_DF_LINK_INIT_TO		120
#define	HFC_FX_LINK_INIT_TO_MIN		1
#define	HFC_FX_LINK_INIT_TO_MAX		255

	uchar					mailbox_force_retry;
#define	HFC_FX_MB_FORCE_RETRY_OFF	0
#define	HFC_FX_MB_FORCE_RETRY_ON	1

	uchar					security_enable;	/* FC-SP */
#define	HFC_FX_SECURITY_DISABLE		0
#define	HFC_FX_SECURITY_ENABLE		1

	uchar					peer_password[40];	/* FC-SP */
	uchar					local_password[40];	/* FC-SP */

	uchar					vf_enable;			/* Virtual Fabric */
#define	HFC_FX_VF_DISABLE			0
#define	HFC_FX_VF_ENABLE			1

	uchar					vf_mode_tagging;	/* Virtual Fabric */
#define	HFC_FX_VF_MODE_TAGGING_OFF	1
#define	HFC_FX_VF_MODE_TAGGING_ON	2
#define	HFC_FX_VF_MODE_TAGGING_AUTO	3

	uchar					wait_plogi_recv_tmo;
#define	HFC_FX_PLOGI_RECV_TMO_MIN	0
#define	HFC_FX_DF_PLOGI_RECV_TMO	30
#define	HFC_FX_PLOGI_RECV_TMO_MAX	60

	uchar					max_vport_count;
#define HFC_FX_DF_VPORT_COUNT		30
#define	HFC_FX_MIN_VPORT_COUNT		0
#define	HFC_FX_MAX_VPORT_COUNT		255
#define	HFC_FX_MAX_RID_COUNT		32
#define	HFC_FX_MAX_SUB_RID_COUNT	8

	ushort					max_frame_count;
#define HFC_FX_DF_FRAME_COUNT		30		/* FCLNX-GPL-FX-241,272 */
#define	HFC_FX_MIN_FRAME_COUNT		1
#define	HFC_FX_MAX_FRAME_COUNT		256
#define	HFC_FX_VP_MAX_FRAME_COUNT	30

	uchar					mq_num;
#define HFC_FX_DF_MQ_NUM			4
#define HFC_FX_MIN_MQ_NUM			1
#define HFC_FX_MAX_MQ_NUM			32

	uchar					intdelay;
#define	HFC_FX_INTDELAY_MIN		0
#define	HFC_FX_INTDELAY_MAX		10000

/* FCLNX-GPL-FX-014 Start */
	uchar					total_abort_to;
#define HFC_FX_DISABLE_TOTAL_ABORT_TO	0
#define HFC_FX_MIN_TOTAL_ABORT_TO		0
#define HFC_FX_MAX_TOTAL_ABORT_TO		60

	uchar					total_tgtrst_to;
#define HFC_FX_DISABLE_TOTAL_TGTRST_TO	0
#define HFC_FX_MIN_TOTAL_TGTRST_TO		0
#define HFC_FX_MAX_TOTAL_TGTRST_TO		60
/* FCLNX-GPL-FX-014 End */

	uchar					flogi_retry_change;				/* FCLNX-GPL-FX-179 */

	uchar					mck_errno;						/* Shadow driver only *//*FCLNX-GPL-FX-376 */
	
#define HFC_FX_MAX_MB_TRACE				100					/* FCLNX-GPL-FX-139 */
	struct hfc_mb_trace		mb_trace[HFC_FX_MAX_MB_TRACE];	/* FCLNX-GPL-FX-139 */
	uchar					current_mbtrc_no;				/* FCLNX-GPL-FX-139 */
	uchar					linknego_tmo_boot;				/* FCLNX-GPL-FX-139 */
	uchar					unfnd_tgtlist[32];				/* FCLNX-GPL-FX-139 */
	
	uchar					linkdown_occurred;				/* FCLNX-GPL-FX-174 */


	uint					max_io;		/* FCLNX-GPL-FX-147 */
#define	HFC_FX_DEFAULT_MAX_IO		0
#define	HFC_FX_MAX_IO				1536

	uchar					login_seq_retry_cnt;	/* FCLNX-GPL-FX-446 >>> */
#define HFC_FX_DEFAULT_LOGIN_SEQ_RETRY_CNT	3
#define HFC_FX_MIN_LOGIN_SEQ_RETRY_CNT		0
#define HFC_FX_MAX_LOGIN_SEQ_RETRY_CNT		10			/* FCLNX-GPL-FX-446 <<< */

	uchar					*hfclddconf;
	
	uchar					log_file;	/* FCLNX-GPL-547 */
	int						max_lun;	/* FCLNX-GPL-547 */

	/* kernel structure */
	spinlock_t				port_lock;		/* lock to port_lock,target_info_fx			*/
	struct semaphore		ioctl_sem;		/* lock  for ioctl process					*/
	struct semaphore		sem;			/* adap_info lock for ioctl process			*/

//	uint					mb_lock;		/* lock bit for mailbox process				*/
//	uint					mb_lock_event_wait;

//	wait_queue_head_t   	mb_lock_event;	/* waitqueue until the lock bit would get	*/
	wait_queue_head_t		mb_event;		/* ioctl mailbox response wait  			*/
	wait_queue_head_t		ioctl_event;	/* ioctl mailbox lock wait					*/
	wait_queue_head_t		ioexe_event;	/* Mailbox/SCSI completion wait				*/
	wait_queue_head_t		mck_event;		/* MCK sleep during mailbox proc			*/
	wait_queue_head_t		init_event;		/* detect i/f during link init and LOGIN	*/
	wait_queue_head_t		rport_event;

	atomic_t				int_a_poll;		/* interrupt polling						*/
	atomic_t				mb_event_wait;
	atomic_t				ioctl_event_wait;
	atomic_t				mck_event_wait;
	atomic_t				check_mbreq;
	uint					mck_on_sleep;
	atomic_t				rport_event_wait;
	
	dma_addr_t				padr_hg_cca;	/* psycal address of uncached memory area shared Hypervisor */
	dma_addr_t				phys_rcvfrm_payload;	/* Receive Frame Payload Address	*/
	dma_addr_t				phys_payload;	/* Payload DMA Address for FRMSNDRCV		*/

	struct wtimer_fx		port_mb_wdog;	/* Mailbox timeout							*//* FIVE-FX */
	struct wtimer_fx		linkup_wdog;	/* Linkup timeout 							*//* FIVE-FX */
	struct wtimer_fx		reboot_wdog;	/* Reboot timeout 							*//* FIVE-FX */
//	struct wtimer_fx		mck_wdog;		/* Mck Recovery timeout 					*//* FIVE-FX */
	struct wtimer_fx		mckint_wdog;	/* Mck Interrupt timeout					*//* FIVE-FX */
	struct wtimer_fx		lgdelay_wdog;	/* Login Delay timer						*//* FIVE-FX */
//	struct wtimer_fx		mpap_lock_wdog;	/* mp_adap_info lock wait					*//* FIVE-FX */
	struct wtimer_fx		fwisol_wdog;	/* firmware isolation						*//* FIVE-FX */
	struct wtimer_fx		int_chk_wdog;	/* check interrupt timer 					*//* FIVE-FX */
	struct wtimer_fx		mckend_wdog;    /* MCK END recovery timer                   */
	struct wtimer_fx		fcstp_wdog;     /* Force Check Stop INT timer               */
	struct wtimer_fx        fmck_wdog;      /* Force MCK INT timer                      */
	struct wtimer_fx		isolend_wdog;	/* ISOL END timer							*/
	struct wtimer_fx		mb_delay_wdog;	/* Mailbox Delay Timer						*/
	struct wtimer_fx		mb_retry_wdog;	/* Mailbox Retry continuation timer			*/
	struct wtimer_fx		link_init_wdog;	/* Link Initialization timer				*/
	struct wtimer_fx		rcv_plogi_wdog;	/* Link Initialization timer				*/

	struct wtimer_fx		ldlerr_wdog;	/* Watchdog timer of Link-down Error 		*/
	struct wtimer_fx		ldserr_wdog;	/* Watchdog timer of Link-down Error 		*/
	struct wtimer_fx		iferr_wdog;		/* Watchdog timer of Link-down Error		*/
	struct wtimer_fx		toerr_wdog;		/* Watchdog timer of Link-down Error		*/
	struct wtimer_fx		rterr_wdog;		/* Watchdog timer of Link-down Error 		*/
	struct wtimer_fx		ld_err_wdog;	/* Linkup timeout for Link Down Error count *//* FCLNX-GPL-FX-424 */

// @MLPF
#define HFC_FX_MLPF_FMCK_STO       3
#define HFC_FX_MLPF_MCKEND_STO     3
#define HFC_FX_MLPF_FCSTP_STO      5
#define HFC_FX_MLPF_FMCK_GTO       3
#define HFC_FX_MLPF_MCKEND_GTO     300
#define HFC_FX_MLPF_CSTPEND_GTO    300
#define HFC_FX_MLPF_MCKINT_GTO     300
#define HFC_FX_MLPF_ISOLEND_GTO	20

	struct fc_host_statistics	port_statistics;
	struct fc_vport 		*fc_vport;
	struct hfc_pkt_fx		*hfcp_to;

	/* SRAM CE log area */
	hfc_fx_err1bit_logout_t		ce_log[3];
	hfc_fx_fw_err1bit_logout_t	ce_fw_log;

#define HFC_FX_MSIX_NVEC		32			/* The number of MSI-X Vectors 				*/
#define HFC_FX_MSIX_MULTIQUEUE	pp->mq_num
#define HFC_FX_NVEC_PER_PORT	pp->core_num + 1
#define HFC_FX_NVEC_ONE			1
	struct msix_entry		entries[HFC_FX_MSIX_NVEC];
											/* The size of "struct msix_entry" is 4byte	*/
	struct hfc_intr_entry	intr_entries[HFC_FX_MSIX_NVEC];
	
	struct	diag_ioctl		*diag;		/* ioctl_diag pointer 	*/	/* FCLNX-GPL-FX-126 */

	struct task_struct		*worker_thread;
	/* PROTO */
	int	wtimer_cnt[HFC_FX_MAX_TMR];
	uint	cpu_map;
#define	HFC_VEC_CPU_MAP_ENABLE		0
#define	HFC_VEC_CPU_MAP_DISABLE		1
	atomic_t	selected_target_id;		/* for sysfs 'selected_target_id' attribute */

};


/* dev_info *//* FCLNX-GPL-0343 */
struct dev_info_fx {
	uchar					flags;		/* dev_info status					*/
/* Flag for FIVE-FX			*/
#define HFC_DF_DEVINF_VALID	0			/* Dev_info is Valid				*/
#define	HFC_DF_LU_LGINF_VALID	1		/* Logical dev_info is valid		*/
#define	HFC_DF_NOT_TYPE_DISK	2
#define	HFC_DF_HSDLDD_VALID	3
#define	HFC_DF_COMMAND_DEV	4
#define HFC_DF_DEVINFO_RSVPATH	5		/* HFC-PCM *//* FCLNX-FX-038 */
#define HFC_DF_DEVINFO_SCAN	7			/* HFC-PCM */

	uchar					status;		/* lu path status					*/
#define HFC_PATH_ONLINE		1
#define HFC_PATH_ONLINE_E	2
#define HFC_PATH_STANDBY	3
#define HFC_PATH_OFFLINE_C	4
#define HFC_PATH_OFFLINE_E	5
#define HFC_PATH_DELETE		7

	ushort 					lustat;		/* LU information for Abort Task Set*//* FCLNX-GPL-0343 */
/* LU Status for FIVE-FX	*/
#define HFC_DS_NEED_ABORT					0		/* Need to issue Abort Task Set		*/
#define	HFC_DS_WAIT_ABORT					1		/* Waiting Abort Task Set Response	*/
#define	HFC_DS_FAIL_ABORT					2		/* Abort Task Set Failed			*/
#define	HFC_DS_NEED_LUN_RESET				3		/* Need to issue LUN Reset			*/
#define	HFC_DS_WAIT_LUN_RESET				4		/* LUN Reset is in progress			*/
#define	HFC_DS_FAIL_LUN_RST					5		/* LUN Reset Failed					*/
#define	HFC_DS_NEED_CANCEL_SCSI_WITHOUT_DMA	6		/* Abort Task Set is in progress	*//* FCLNX-GPL-FX-014 */
#define	HFC_DS_WAIT_CANCEL_SCSI_WITHOUT_DMA	7		/* LUN Reset is in progress			*//* FCLNX-GPL-FX-014 */
#define	HFC_DS_NEED_CANCEL_SCSI_WAIT_DMA	8		/* Abort Task Set is in progress	*//* FCLNX-GPL-FX-014 */
#define	HFC_DS_WAIT_CANCEL_SCSI_WAIT_DMA	9		/* LUN Reset is in progress			*//* FCLNX-GPL-FX-014 */
#define	HFC_DS_WAIT_ABTSRSP					10		/* Abort Task Set is in progress	*/
#define	HFC_DS_WAIT_LUNRSP					11		/* LUN Reset is in progress			*/
#define HFC_DS_LUNRST_DELAY					12		/* LUN Reset Delay Timer 			*//* FCLNX-GPL-FX-152 */

#define HFC_DS_BLOCKED_SCSI 					\
	(0xffffffff &								\
	~((0x00000001 << HFC_DS_FAIL_ABORT)		|	\
	  (0x00000001 << HFC_DS_FAIL_LUN_RST)	|	\
	  (0x00000001 << HFC_DS_WAIT_ABTSRSP)	|	\
	  (0x00000001 << HFC_DS_WAIT_LUNRSP)) )
/* FCLNX-GPL-FX-014 */

#define HFC_DS_ISSUE_ANY_RESET 					\
	(0xffffffff &								\
	~((0x00000001 << HFC_DS_FAIL_ABORT)		|	\
	  (0x00000001 << HFC_DS_FAIL_LUN_RST)	|	\
	  (0x00000001 << HFC_DS_WAIT_ABTSRSP)	|	\
	  (0x00000001 << HFC_DS_WAIT_LUNRSP)) )
/* FCLNX-GPL-FX-177 */

	uchar					curr_core;		/* To use the same core for next scsi cmd *//* FIVE-FX */
	uchar					curr_cmd_type;	/* To check if the next command type is same to previous one *//* FIVE-FX */
	uint					seq_cnt;		/* To check if core_control is HFC_FX_CORECTL_SEQUENTIAL *//* FIVE-FX */

	uchar					owner_ctl;	/* owner controller =1				*/
	uchar					target_id;	/* target id						*/
	uint					lun;		/* LU#								*/
	uint64_t				rsv_key;	/* reservation key					*//* FCLNX-GPL-FX-378 */

/* FCLNX-GPL-FX-014 Start */
	union{
		uint	all;
		uchar	core[MAX_CORE_PROBE_FX];
	}dev_core_stat;	/* Core Status for LUN Reset/Abort Task Set *//* FCLNX-GPL-FX-014 */
#define	HFC_DC_NEED_CSCSI_LU_WAIT_DMA		0	/* Need Cancel SCSI CMND Waiting DMA */
#define	HFC_DC_NEED_CSCSI_LU_WITHOUT_DMA	1	/* Need Cancel SCSI CMND without Waiting DMA */
#define	HFC_DC_NEED_LUN_RESET_OR_ABORT		2	/* Need LUN Reset or Need AbortTaskSet */
#define	HFC_DC_WAIT_CSCSI_LU_WAIT_DMA		3	/* Wait Cancel SCSI CMND Waiting DMA */
#define	HFC_DC_WAIT_CSCSI_LU_WITHOUT_DMA	4	/* Wait Cancel SCSI CMND without Waiting DMA */
#define	HFC_DC_WAIT_LUN_RESET_OR_ABORT		5	/* Wait LUN Reset or Wait AbortTaskSet */
#define	HFC_DC_WAIT_MIHLOG					6	/* Wait MIHLOG */
	uint	abtcmd_core_no;
	struct scsi_cmnd		*abort_cmnd;
	struct scsi_cmnd		*lun_reset_cmnd;
/* FCLNX-GPL-FX-014 End */
	struct scsi_cmnd		*dummy_cmnd[MAX_CORE_PROBE_FX];

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
	struct target_info_fx	*target;	/* Pointer to the target info		*/
	struct dev_info_fx		*next;		/* Pointer to the next dev_info 	*/
	struct dev_info_fx		*prev;		/* Pointer to the previous dev_info */
	struct lg_dev_info_fx	*lg_dev;	/* Pointer to the previous lg_dev_info FCLNX-0429 */
	struct wtimer_fx		lun_delay_wdog;		/* LUN Reset delay timer	*/	/* FCLNX-0627 */
	struct wtimer_fx		total_abort_wdog;	/* Total Abort Timer		*/	/* FCLNX-GPL-FX-014 */
	struct wtimer_fx		path_retry_wdog;	/* retry a scsi command to other path timer for HFC-PCM */
	uchar					io_status;			/* FCLNX-0627 */
#define HFC_LUN_RESET_DELAY_TO		0	/* Waiting while LUN Reset Delay Timer is executing */ /* FCLNX-0627 */
	atomic_t				wx_que_cnt;	/* FCLNX-FX-007 */
	atomic_t				we_que_cnt;	/* FCLNX-FX-007 */
	char					id_type;	/* Identifer Type */	/* FCLNX-0677 *//* FCLNX-710 */
#define HFC_INQ83_IDTYPE1	1	/* FCLNX-710 */
#define HFC_INQ83_IDTYPE3	3	/* FCLNX-710 */
//	uchar					write_retries;		/* FCLNX-GPL-0449 */
};								/* FCLNX-GPL-0343 */

/* 
 * target_info : target information
 *
 *  allocated by hfc_detect()
 *  released  by hfc_release()
 *
 */
struct target_info_fx {
	uint						status;		/* target status						*/
/* Status for FIVE-FX			*/
#define HFC_TS_SCSI_DELAY		0			/* Waiting status after Target Reset	*/
#define	HFC_TS_SCN_RESP			1			/* Received RSCN or SCN					*/
#define	HFC_TS_SCN_WLINKUP		2			/* Linkup timer for SW to disk			*/
#define	HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA	3	/* Need to issue Cancel_SCSI Mailbox	*//* FCLNX-GPL-FX-014 */
#define	HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA	4	/* Cancel_SCSI Mailbox is in progress	*//* FCLNX-GPL-FX-014 */
#define	HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA	5	/* Need to issue Cancel_SCSI Mailbox	*//* FCLNX-GPL-FX-014 */
#define	HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA	6	/* Cancel_SCSI Mailbox is in progress	*//* FCLNX-GPL-FX-014 */
#define	HFC_TS_NEED_PLOGI		7			/* Need to issue Port Login Mailbox		*/
#define	HFC_TS_WAIT_PLOGI		8			/* Port Login Mailbox is in progress	*/
#define	HFC_TS_NEED_PRLI		9			/* Need to issue Process Login Mailbox	*/
#define	HFC_TS_WAIT_PRLI		10			/* Process Login Mailbox is in progress	*/
#define	HFC_TS_NEED_PDISC		11			/* Need to issue Process Login Mailbox	*/
#define	HFC_TS_WAIT_PDISC		12			/* Process Login Mailbox is in progress	*/
#define	HFC_TS_NEED_LOGO_TGT	13			/* Need to issue LOGO Mailbox			*/
#define	HFC_TS_WAIT_LOGO_TGT	14			/* LOGO is in progress					*/
#define HFC_TS_NEED_TARGET_RESET	15  	/* Need Target_Reset					*/
#define	HFC_TS_WAIT_TARGET_RESET	16		/* Target Reset Command is in progress	*/
#define HFC_TS_WAIT_TGTRSP			17		/* target reset process is in prog		*/
#define HFC_TS_WPDISC_LOGO_RESP	18 			/* LOGO receiver during PDISC			*/
#define HFC_TS_CANCEL_SCSI_TARGET	19		/* Cancel Process for Target by SCSIT.O */ /* FCLNX-GPL-FX-112 */

#define HFC_TS_BLOCKED_SCSI 						\
	(0xffffffff &									\
	~((0x00000001 << HFC_TS_NEED_TARGET_RESET) |	\
	  (0x00000001 << HFC_TS_WAIT_TGTRSP)) )

/* FCLNX-GPL-FX-181 Start */
#define HFC_TS_BLOCKED_TGT_RESET 					\
	(0xffffffff &									\
	~((0x00000001 << HFC_TS_WAIT_TARGET_RESET) |	\
	  (0x00000001 << HFC_TS_NEED_TARGET_RESET) |	\
	  (0x00000001 << HFC_TS_CANCEL_SCSI_TARGET) |	\
	  (0x00000001 << HFC_TS_WAIT_TGTRSP)) )
/* FCLNX-GPL-FX-181 End */

#define HFC_TS_ISSUE_ANY_RESET 									\
	((0x00000001 << HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA)	|	\
	 (0x00000001 << HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA)	|	\
	 (0x00000001 << HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA)		|	\
	 (0x00000001 << HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA)		|	\
	 (0x00000001 << HFC_TS_NEED_PLOGI)						|	\
	 (0x00000001 << HFC_TS_WAIT_PLOGI)						|	\
	 (0x00000001 << HFC_TS_NEED_PRLI)						|	\
	 (0x00000001 << HFC_TS_WAIT_PRLI)						|	\
	 (0x00000001 << HFC_TS_NEED_TARGET_RESET)				|	\
	 (0x00000001 << HFC_TS_WAIT_TARGET_RESET)				|	\
	 (0x00000001 << HFC_TS_CANCEL_SCSI_TARGET))
/* FCLNX-GPL-FX-177 */

	uint						flags;		/* target vaildity information			*/
/* Flag for FIVE-FX				*/
#define	HFC_TF_WWN_VALID		0			/* WW_Name is valid						*/
#define	HFC_TF_DEVFLG_VALID		1			/* LOGIN succeeded						*/
#define	HFC_TF_FAIL_TARGET_RESET	3		/* Failed Target Reset					*/
#define HFC_TF_LGINF_VALID		4			/* tgt logical info is valid			*/
#define HFC_TF_WAITING_DEV_LOSS_TMO	5		/* use only during ld_err_limit_s ON	*//* FCLNX-GPL-FX-424 */

/* FCLNX-GPL-FX-014 Start */
	union{
		uint all;
		uchar core[MAX_CORE_PROBE_FX];
	}tgt_core_stat;	/* Core Status for Target Reset *//* FCLNX-GPL-FX-014 */
#define	HFC_TC_NEED_CSCSI_TGT_WAIT_DMA		0	/* Need Cancel SCSI CMND Waiting DMA */
#define	HFC_TC_NEED_CSCSI_TGT_WITHOUT_DMA	1	/* Need Cancel SCSI CMND without Waiting DMA */
#define	HFC_TC_NEED_TGTRST					2	/* Need Target Reset */
#define	HFC_TC_WAIT_CSCSI_TGT_WAIT_DMA		3	/* Wait Cancel SCSI CMND Waiting DMA */
#define	HFC_TC_WAIT_CSCSI_TGT_WITHOUT_DMA	4	/* Wait Cancel SCSI CMND without Waiting DMA */
#define	HFC_TC_WAIT_TGTRST					5	/* Wait Target Reset */
#define	HFC_TC_WAIT_MIHLOG					6	/* Wait MIHLOG */
	uint	tgtrst_core_no;
	struct scsi_cmnd		*target_reset_cmnd;
/* FCLNX-GPL-FX-014 End */
	struct scsi_cmnd		*dummy_cmnd[MAX_CORE_PROBE_FX];
	struct hfc_pkt_fx		*reset_pkt;
	
	uchar					pseq;		/* target sequence number (0-255)			*/
	uint					target_id;	/* Target logical ID (0-255)				*/
	uchar					fc_class;	/* Service class							*/
#define HFC_FC_CLASS1			0x01		/* fc class 1 							*/
#define HFC_FC_CLASS2			0x02		/* fc class 2 							*/
#define HFC_FC_CLASS3			0x03		/* fc class 3 							*/
#define HFC_FC_CLASS4			0x04		/* fc class 2 							*/
#define HFC_FC_CLASS6			0x06		/* fc class 3 							*/

	uchar					plogi_param;

	/* These are gotten by LOGIN response												*/
	ushort					fc_class_mask;	/* Class mask								*/
	ushort					device_flags;	/* Least 2bytes parameter 					*/
	ushort					max_frame_size;	/* Max_frame_size 							*/
	ushort					prli_parm;		/* PRLI_PARM<2-3> :FIVE-FX					*/

	uint64_t 				scsi_id;		/* Destination ID							*/
#define HFC_PTOP_TGT_PORTID	0x00000002		/* FCLNX-GPL-FX-066 */
	uint64_t 				ww_name;		/* World Wide Port Name		   				*/
	uint64_t 				node_name;		/* World Wide Node Name		   				*/
	
	struct hfc_queue		core_queue[MAX_CORE_PROBE_FX];

	struct target_info_fx	*next;			/* Pointer to the next target_info 			*/
	struct target_info_fx	*prev;			/* Pointer to the prev. target_info			*/
	
	/* Pointer to the adap_info->login_target											*/
	struct target_info_fx	*login_next;	/* The next LOGIN							*/
	struct target_info_fx	*plogi_next;	/* The next PLOGI							*/
	struct target_info_fx	*prli_next;		/* The next PRLI							*/
	struct target_info_fx	*pdisc_next;	/* The next PDISC							*/

//	struct adap_info		*ap;			/* Pointer to the adap_info area			*/
	struct port_info		*pp;			/* Pointer to the port_info area			*/
	struct dev_info_fx		*dev;			/* Pointer to the dev_info area				*/
	uchar					group_id;		/* group id									*/
	uchar					attribute;		/* target path attribute					*/
#define HFC_ATTR_CONFIGURED		0x00
#define HFC_ATTR_CONFIGURED_H	0x01
#define HFC_ATTR_UNCONFIGURED	0x02
#define HFC_ATTR_UNCONTROLLED	0x03
	uchar					path_id;		/* target path id							*/
	struct lg_target_info_fx	*lg_target;		/* connect lg_target_info pointer			*/
//	uchar					write_retries[256];							  /* FCLNX-0244 */
	
	uchar					rport_status;
#define HFC_NEED_RPORT_ADD		0
#define HFC_NEED_RPORT_DEL      1 /* FCLNX-GPL206 */

	ushort					send_frame_size;	/* FCLNX-GPL-261 */
	ushort					mfsize;			/* FIVE-FX */
	
	struct errcnt_info		*tgt_ldl_errcnt_info;		/* Counter for Long Link-down error */					/* FCLNX-GPL-327 */
	struct errcnt_info		*tgt_lds_errcnt_info;		/* Counter for Short Link-down error */					/* FCLNX-GPL-327 */
	struct wtimer_fx		tgt_ldlerr_wdog;			/* Watchdog timer of Link-down Error */					/* FCLNX-GPL-327 */
	struct wtimer_fx		tgt_ldserr_wdog;			/* Watchdog timer of Link-down Error */					/* FCLNX-GPL-327 */
	uint					tgt_ld_err_count_s;			/* Error Counter for Short Link-down error */  /* FCLNX-GPL-349 */
	uchar					link_recovered;				/* HBA - Target link is recovered.    */					/* FCLNX-GPL-334 */
	uint32_t				dev_loss_tmo;
	uint32_t				fast_io_fail_tmo;
	
	char					login_seq_retry_cnt;	/* FCLNX-GPL-FX-446 >>> */

	/* watchdog timer 																	*/
	struct wtimer_fx		wexec_wdog;		/* SCSI initiation queue 					*/
	struct wtimer_fx		scnlinkup_wdog;	/* SW-Dev, Link_up 							*/
	struct wtimer_fx		delay_wdog;		/* delay timer								*/
	struct wtimer_fx		restart_wdog;	/* restart timer							*/
	struct wtimer_fx		total_tgtrst_wdog;	/* Total Target Reset timer		*//* FCLNX-GPL-FX-014 */

	struct fc_rport			*rport;

};

/* 
 * hfc_pkt_fx : I/O initiation information
 *
 *
 */

//#define CMND_VALID 			0			/* This dummy_smnd is occupied					*/ 

/* The common area both for hfc_pkt and rst_pkt */
struct hfc_pkt_fx {
	uint					cmd_flags;		/* scsi_pkt control flag (bit)				*/
//#define CFLAG_VALID 			0			/* This hfc_pkt is occupied					*/
//#define CFLAG_ABORT 			1			/* Abort Task Set request					*/
//#define CFLAG_TARGET_RESET	2			/* Target reset request			   			*/
//#define CFLAG_BUS_RESET 		3			/* Bus reset request						*/
//#define CFLAG_LUN_RESET		4			/* request LUN reset request				*//* FCLNX-0429 */
//#define CFLAG_SEGVALID		8			/* SEG_INFO is valid 						*/
//#define CFLAG_TIMEOUT			16			/* Timeout occured							*/
//#define CFLAG_SCMD_SLOGV		17			/* Timeout (softlog was corrected)  		*/
//#define CFLAG_ABORT_TIMEOUT	18			/* Abort Timeout							*/
//#define CFLAG_INH_ALTPATH		19			/* Inhibit alternate path					*/
//#define CFLAG_HSDLDD_VALID 	20														  /* FCLNX-0429 */
//#define CFLAG_COMMAND_DEV 	21														  /* FCLNX-0611 */
//#define CFLAG_PFB_VALID 		22														  /* FCLNX-GPL-204 */
//#define CFLAG_NOT_TYPE_DISK	23														  /* FCLNX-GPL-450 */

	uint					adap_status;			/* scsi_pkt status					*/

//#define SCS_ERROR_VALID			0x80000000										/* for scsi_err_cnt		*/
//#define SCS_BUSY_VALID			0x40000000										/* for scsi_busy_cnt		*/

//#define SCS_NORMAL_END			0x00000001										/* Normal end 			*/
//#define SCS_TRANSPORT_DEAD		0x00001002 | SCS_ERROR_VALID					/* XRB response XCC != 0x80	*/
																					/*	 /COMMAND_TERMINATED*/
//#define SCS_IO_ICC_LINK_CHK		0x00001003 | SCS_ERROR_VALID					/* XRB response XCC != 0x80	*/
//#define SCS_ADAP_FAILURE			0x00001004 | SCS_ERROR_VALID					/* XRB response FSB = PC 	*/
//#define SCS_CMD_TIMEOUT 			0x00001006 | SCS_ERROR_VALID					/* timeout(wdog) cancel */
//#define SCS_NO_DEV_RESP			0x00001007 | SCS_ERROR_VALID					/*	  Ditto				*/
//#define SCS_IO_ICC_NO_RESP	 	0x00001008 | SCS_ERROR_VALID					/* XRB response FSB = ICC	*/
//#define SCS_IO_ICC_TIMEOUT		0x00001009 | SCS_ERROR_VALID					/* XRB response FSB = ICC	*/
//#define SCS_WAIT_ABORT			0x0000100A | SCS_BUSY_VALID						/* Abort Task Set is starting	*/
//#define SCS_WAIT_RESET			0x0000100C | SCS_BUSY_VALID						/* Target Reset is starting	*/
//#define SCS_OFFLINE 				0x0000100D | SCS_ERROR_VALID					/* Not ONLINE 	*/
//#define SCS_LOGIN_START 			0x00001010										/* login Start			*/
//#define SCS_LOGIN_BUSY			0x00001011 | SCS_BUSY_VALID						/* login Start fail		*/
//#define SCS_LOGIN_WWCHG 			0x00001012 | SCS_ERROR_VALID					/* After login, the WWN disagreement	*/
//#define SCS_LOGIN_SUCCESS			0x00001013										/* login success			*//*No*/
//#define SCS_LOGIN_FAULT 			0x00001020										/* login fail			*//*No*/
//#define SCS_PDISC_BUSY			0x00001021										/* pdisc start fail		*//*No*/
//#define SCS_PDISC_WWCHG 			0x00001022 | SCS_ERROR_VALID					/* WWN disagreement(After pdisc. )	*/
//#define SCS_PDISC_FAULT 			0x00001023										/* pdisc fail			*//*No*/
//#define SCS_INTR_LINKDOWN			0x00001024 | SCS_ERROR_VALID					/* Linkdown (Asynchronous INT) */
//#define SCS_INTR_PLOGI			0x00001025 | SCS_ERROR_VALID					/* PLOGIN reception(Asynchronous INT)*/
//#define SCS_INTR_LOGO				0x00001026 | SCS_ERROR_VALID					/* LOGO reception(Asynchronous INT)	*/
//#define SCS_LINKUP_TO				0x00001027 | SCS_ERROR_VALID					/* Linkup waiting timeout	*/
//#define SCS_LINK_FAULT			0x00001028										/* Link ini fail 		*//*No*/
//#define SCS_IO_CCC				0x00001029 | SCS_ERROR_VALID					/* SERR/PERR/SPERR		*/
//#define SCS_MCK 					0x0000102A 										/* MCK_INT				*/
//#define SCS_GIDFT_FAULT			0x0000102B										/* No GID_FT response			*//*No*/
//#define SCS_LOGIN_NODEV 			0x0000102C										/* No login target 		*//*No*/
//#define SCS_WWN_INVALID			0x0000102D | SCS_ERROR_VALID					/* target wwn is illegal		*/
//#define SCS_SCSI_DELAY			0x0000102E | SCS_BUSY_VALID						/* SCSI DELAY TMR is starting */
//#define SCS_WAIT_LOGIN			0x0000102F | SCS_BUSY_VALID						/* login Issue			*/
//#define SCS_ATTACH_IMCOMP			0x00001030 | SCS_ERROR_VALID					/* attach incomplete end			*/
//#define SCS_NO_TARGET				0x00001031										/* No login target		*//*P*/
//#define SCS_TARGET_ABNORMAL		0x00001032 | SCS_BUSY_VALID						/* target is abnormal		*/
//#define SCS_WAIT_LINKUP			0x00001033 | SCS_BUSY_VALID						/* link up waiting state		*/
//#define SCS_PCI_BUS_SERR			0x00001034 | SCS_ERROR_VALID					/* PCI SERR				*/
//#define SCS_PCI_BUS_PERR			0x00001035 | SCS_ERROR_VALID					/* PCI PERR				*/
//#define SCS_PCI_BUS_SPERR			0x00001036 | SCS_ERROR_VALID					/* PCI SPERR			*/
//#define SCS_CMD_FLUSH				0x00001037 | SCS_ERROR_VALID					/* Flush waiting state		*/
//#define SCS_CMD_SHUTDOWN			0x00001038 | SCS_ERROR_VALID					/* Shutdown waiting state		*/
//#define SCS_WAIT_BUS_RESET		0x00001039 | SCS_BUSY_VALID						/* Bus Reset waiting state	*/
//#define SCS_CHG_SCSIID			0x0000103A | SCS_ERROR_VALID					/* SCSI ID change			*/
//#define SCS_CMD_NEED_ABORT		0x00001040 | SCS_BUSY_VALID						/* ABORT issue waiting		*/
//#define SCS_DATA_LENGTH_OVER		0x00001041										/* Forwarding data length  OVER		*//*P*/
//#define SCS_INVALID_PATH_ID		0x00001042										/* BUS ID is illegal			*//*P*/
//#define SCS_INVALID_TARGET_ID		0x00001043										/* Target ID is illegal		*//*P*/
//#define	SCS_CDB_OVER			0x00001044										/* CDB is 17 bytes or more		*//*P*/
//#define	SCS_CMD_ABORTED			0x00001045 | SCS_ERROR_VALID					/* Abort Task Set Cancel*/
//#define SCS_CMD_RESET				0x00001046 | SCS_ERROR_VALID					/* Target Reset Cancel  */
//#define SCS_INVALID_ABORT_SRB		0x00001047										/* Disagreement of SRB for Abort	*//*P*/
//#define SCS_XOB_HALT				0x00001048 | SCS_BUSY_VALID						/* XOB HALT state			*/
//#define SCS_LUN_EXT_NULL			0x00001049										/* LUN EXT NULL			*//*P*/
//#define SCS_NO_NEED_FLUSH			0x00001050										/* FLUSH issue is unnecessary		*//*P*/
//#define SCS_NO_NEED_SHUTDOWN		0x00001051										/* SHUTDOWN issue is unnecessary		*//*P*/
//#define SCS_LOGIN_THAN_RESET		0x00001052 | SCS_ERROR_VALID					/* Reset demanding login	*/
//#define SCS_LOGIN_THAN_ABORT		0x00001053 | SCS_ERROR_VALID					/* Abort demanding login	*/
//#define SCS_LOGIN_BUSY_RESET		0x00001054 | SCS_ERROR_VALID					/* Login failure at Reset	*/
//#define SCS_LOGIN_BUSY_ABORT		0x00001055 | SCS_ERROR_VALID					/* Login failure at Abort	*/
//#define SCS_DATA_OVERRUN			0x00001056										/* Data Over Run		*/
//#define SCS_DATA_UNDERRUN			0x00001057										/* Data Under Run		*/
//#define SCS_ABORT_FAILED			0x00001058										/* Abort start fail		*/
//#define SCS_XOB_FULL				0x00001059 | SCS_BUSY_VALID						/* XOB FULL				*/
//#define SCS_IOVMAP_FULL			0x00001060 | SCS_BUSY_VALID						/* IOVMAP FULL			*/
//#define SCS_PAGE_OVER				0x00001061 | SCS_BUSY_VALID						/* PAGE CNT OVER		*/
//#define SCS_FRAME_CHK_ERROR		0x00001062 | SCS_BUSY_VALID						/* FRAME CHK ERROR		*/
//#define SCS_RESET_FAILED			0x00001063 | SCS_BUSY_VALID						/* Target Reset ABEND FCWIN-0153 */
//#define SCS_SCN_LINKDOWN			0x00001064 | SCS_ERROR_VALID					/* Link Down between SW_DEV   FCWIN-0153 */
//#define SCS_NO_CMND				0x00001065 | SCS_ERROR_VALID
//#define SCS_WAIT_MCK				0x00001066 | SCS_BUSY_VALID						/* MCK end waiting			*/
//#define SCS_CANCEL_RESP			0x00001067
//#define SCS_NARROW_DEV			0x00001068										/* FCLNX-0392 */
//#define SCS_WAIT_DEV_RESET		0x00001069 | SCS_BUSY_VALID						/* FCLNX-0429 */

//#define SCS_IO_CDC				0x0000106a										/* FCLNX-0534 */
//#define SCS_ATTATCH_ERROR			0x0000106b										/* FCLNX-0534 */
//#define SCS_ADAPTER_OFFLINE		0x0000106c										/* FCLNX-0534 */
//#define SCS_NO_ADAPINFO			0x0000106d										/* FCLNX-0534 */
//#define SCS_NO_HFCPKT				0x0000106e										/* FCLNX-0534 */
//#define SCS_DMASIZE_OVER			0x0000106f										/* FCLNX-0534 */
//#define SCS_REJCT_DIAG			0x00001070										/* FCLNX-0534 */
//#define SCS_TARGET_NOTFOUND		0x00001071										/* FCLNX-0534 */
//#define SCS_CMDLENGTH_OVER		0x00001072										/* FCLNX-0534 */
//#define SCS_EXE_BUSRESET			0x00001073										/* FCLNX-0534 */
//#define SCS_EXE_DEVICERESET		0x00001074										/* FCLNX-0534 */
//#define SCS_NO_WE_QUE				0x00001076										/* FCLNX-0534 */
//#define SCS_CHECK_CONDITION		0x00001077										/* FCLNX-0534 */
//#define SCS_MP_NO_APINFO			0x00001078										/* FCLNX-0534 */
//#define SCS_MP_BAD_TARGET			0x00001079										/* FCLNX-0534 */
//#define SCS_MP_DEVICE_NOTFOUND	0x0000107a										/* FCLNX-0534 */
//#define SCS_MP_HSDLDD_DISABLE		0x0000107b										/* FCLNX-0534 */
//#define SCS_MP_DMASIZE_OVER		0x0000107c										/* FCLNX-0534 */
//#define SCS_MP_TARGET_NOTFOUND	0x0000107d										/* FCLNX-0534 */
//#define SCS_MP_NOHFCP				0x0000107e										/* FCLNX-0534 */
//#define SCS_MP_STRT_DEVNOTFOUND	0x0000107f										/* FCLNX-0534 */
//#define SCS_MP_STRT_HSD_DISABLE	0x00001080										/* FCLNX-0534 */
//#define SCS_MP_STRT_TRGNOTFOUND	0x00001081										/* FCLNX-0534 */
//#define SCS_MP_STRT_ATTACHFAIL	0x00001082										/* FCLNX-0534 */
//#define SCS_MP_STRT_OFFLINE		0x00001083										/* FCLNX-0534 */
//#define SCS_MP_STRT_WWN_INVALID	0x00001084										/* FCLNX-0534 */
//#define SCS_MP_DEVRST_NO_APINFO	0x00001085										/* FCLNX-0534 */
//#define SCS_MP_DEVRST_BADTARGET	0x00001086										/* FCLNX-0534 */
//#define SCS_MP_DEVRST_SUCCESS		0x00001087
//#define SCS_MP_BUSRST_NO_APINFO	0x00001088										/* FCLNX-0534 */
//#define SCS_MP_BUSRST_BADCHNNL	0x00001089										/* FCLNX-0534 */
//#define SCS_MP_BUSRST_SACCESS		0x0000108a										/* FCLNX-0534 */
//#define SCS_MP_RT_TRG_NOTFOUND	0x0000108b										/* FCLNX-0534 */
//#define SCS_MP_RT_DEV_NOTFOUND	0x0000108c										/* FCLNX-0534 */
//#define SCS_MP_RT_NOHFCP			0x0000108d										/* FCLNX-0534 */
//#define SCS_HSD_NO_APINFO			0x0000108e										/* FCLNX-0534 */
//#define SCS_HSD_BAD_TARGET		0x0000108f										/* FCLNX-0534 */
//#define SCS_HSD_TRG_NOTFOUND		0x00001090										/* FCLNX-0534 */
//#define SCS_HSD_PATH_NOTFOUND		0x00001091										/* FCLNX-0534 */
//#define SCS_HSD_DEV_NOTFOUND		0x00001092										/* FCLNX-0534 */
//#define SCS_HSD_DISABLE			0x00001093										/* FCLNX-0534 */
//#define SCS_HSD_DMASIZE_OVER		0x00001094										/* FCLNX-0534 */
//#define SCS_HSD_LURST_NO_APINFO	0x00001095										/* FCLNX-0534 */
//#define SCS_HSD_LURST_BADTARGET	0x00001096										/* FCLNX-0534 */
//#define SCS_HSD_LURST_NO_TRG		0x00001097										/* FCLNX-0534 */
//#define SCS_HSD_LURST_NO_DEV		0x00001098										/* FCLNX-0534 */
//#define SCS_HSD_LURST_DISABLHSD	0x00001099										/* FCLNX-0534 */
//#define SCS_HSD_LURST_NOTRGINFO	0x0000109a										/* FCLNX-0534 */
//#define SCS_HSD_LURST_ATTACHERR	0x0000109b										/* FCLNX-0534 */
//#define SCS_HSD_LURST_OFFLINE		0x0000109c										/* FCLNX-0534 */
//#define SCS_HSD_LURST_WWN_INVAL	0x0000109d										/* FCLNX-0534 */
//#define SCS_HSD_LURST_WAIT_RST	0x0000109e										/* FCLNX-0534 */
//#define SCS_HSD_DVRST_NO_APINFO	0x000010a0										/* FCLNX-0534 */
//#define SCS_HSD_DVRST_NO_TARGET	0x000010a1										/* FCLNX-0534 */
//#define SCS_HSD_DVRST_SUCCESS		0x000010a2										/* FCLNX-0534 */
//#define SCS_HSD_BSRST_NO_APINFO	0x000010a8										/* FCLNX-0534 */
//#define SCS_HSD_BSRST_NO_TARGET	0x000010a9										/* FCLNX-0534 */
//#define SCS_HSD_BSRST_SUCCESS		0x000010aa										/* FCLNX-0534 */	

//#define SCS_SCSI_CHECK			0x000010b0										/* FCLNX-0594 */
//#define SCS_WAIT_CANCEL			0x000010b1 | SCS_BUSY_VALID						/* Cancel login Issue FCLNX-GPL-038	*/
//#define SCS_WAIT_LUNRST			0x000010b2										/* FCLNX-GPL-0343 */
//#define SCS_NEED_LUNRST			0x000010b3										/* FCLNX-GPL-0343 */

//#define SCS_RSPCODE_FAILURE		0x000010c0										/* RSP_CODE byte3!=0  FCLNX-GPL-0312,0321 */

	uint					seg_cnt;		/* Number of Scatter-Gather list */
	uint					data_size;		/* Total Data Size */
	uint					iov_no;			/* Top of the seg_info (SRB)				*/
	uint					iov_cnt;		/* Total number of seg_infos (SRB) 			*/
	uint					timeout;		/* SCSI Command Timeout period				*//* FCLNX-GPL-575 */

	ushort					pkt_no;			/* hfc_pkt number							*/
	ushort					pm_pkt_no;		/* hfc_pm_pkt number						*/
	ushort					fail;			/* number of error end			 FCLNX-0603 */
	ushort					lun_id;			/* LU#										*//* FCLNX-GPL-0343 */
	ushort					tout_slog_sbc; 	/* MIH-LOG length							*/

	uchar					tout_slog_ssn; 	/* MIH-LOG number for timeout condition		*/
	uchar					core_no;		/* core number								*/
	uchar					cmd_xob;		/* XOB number for scsi_pkt					*/
	uchar					target_id;		/* physical target id						*/
	ushort					cpu_no;			/* cpu number					 FCLNX-0603 *//* FCLNX-GPL-FX0343 */
	uchar					group_id;		/* group id									*/
	uchar					err_count_enable;	/* Enable to Error Count     FCLNX-0454 */
	uchar					rid;			/* Region ID of I/O Request		 FIVE-FX	*/
	uchar					frame_no;		/* flame number to use this hfc_pkt			*//* FCLNX-GPL-FX-014 */

	struct scsi_cmnd		*cmd_pkt;		/* Pointer to SCSI_Cmd						*/

	struct hfc_pkt_fx 		*cmd_forw;		/* Pointer to the next hfc_pkt				*/
	struct hfc_pkt_fx 		*cmd_prev;		/* Pointer to the previous hfc_pkt			*/

	struct hfc_pkt_fx 		*pkt_prev;		/* Pointer to previous hfc_pkt for pool		*/
	struct hfc_pkt_fx 		*pkt_next;		/* Pointer to next hfc_pkt for pool			*/

	struct target_info_fx	*target;		/* Pointer to target_info 					*/
	struct dev_info_fx		*dev;			/* dev_info pointer for SCSI				*/

	struct port_info		*pp;			/* Pointer to port_info 					*//* FIVE-FX */
	struct region_info		*rp;			/* Pointer to region_info 					*//* FIVE-FX */
	struct core_info		*core;			/* Pointer to core_info						*//* FIVE-FX */

	struct wtimer_fx		cmd_timeout;	/* Timer  for SCSI command response check	*/
	dma_addr_t		        dma_handle;		/* Pointer to DMA area						*/

	ulong					upathmap;		/* path used map				 FCLNX-0429 */
	ulong					cpathmap;		/* path used map				 FCLNX-0429 */
};

struct dummy_scsi_cmnd {
	struct scsi_cmnd		cmnd;
	struct scsi_device		device;
	uchar					cdb[16];
};

struct icc_errlog {
	uint	err_no;
	uint	err_id;
	uchar	logdata[16];
	uchar	pseq;
	uchar	first_icc;
	struct	hfc_pkt_fx		hfcp;
	struct	port_info		*icc_pp;
};
