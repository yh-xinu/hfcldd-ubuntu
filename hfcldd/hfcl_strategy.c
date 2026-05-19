/*
 * hfcl_strategy.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char stra_rcsid[] = "$Id: hfcl_strategy.c,v 1.71.2.23.2.18.2.4.2.5.6.9.2.2.2.6.2.23.2.2.2.2.2.4.2.3.2.1 2016/02/19 03:05:29 mhayashi Exp $";

/* include File */

#include "hfcldd.h"
#include "hfcl_tbol.h"
#include "hfcl_detect.h"
#include "hfcl_mlpf.h"
#include "hfcl_strategy.h"
#include "hfcl_handler.h"
#include "hfcl_stra_trace.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_top.h"
#include "hfcl_ioctl.h"
#include "hfcldd_conf.h"

#include "hfcl_strategy_fx.h"

/*-- global variable --*/


/*-- static --*/
static int hfc_resource_chk(struct adap_info *ap, 
	struct target_info *target,
	struct hfc_pkt *hfcp,
	uint   fw_xob_outp);

static void hfc_get_free_iov(struct adap_info *ap ,
	uint st_word,
	uint st_bit,
	uint req_bit_cnt,
	struct	free_iov_map *fmap);

static void hfc_iov_update(struct adap_info *ap ,
	uint pos ,
	uint cnt ,
	uchar type);

static int hfc_dma_map( struct adap_info *ap, 
	struct hfc_pkt *hfcp );

static void hfc_make_cmdiu( struct adap_info *ap, 
	struct hfc_pkt *hfcp );

static void hfc_xob_enque(struct adap_info *ap, 
	struct target_info *target,
	struct hfc_pkt *hfcp);

void hfc_enque_next_dstart(struct adap_info *ap,
	struct target_info *target);

static void hfc_cancel_tskmgm(struct adap_info *ap,
	struct target_info          *target,
	uint                      	lun,		/* FCLNX-GPL-0343 */
	struct hfc_pkt              *hfcp,
	uint                        adap_status,
	uint                        type);

static void hfc_stra_trace(uchar id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_stra_trc1( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_stra_trc2( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_stra_trc3( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_stra_trc4( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_stra_trc5( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);

#if _HFC_ERROR_INJ									/* FCLNX-0246 */
extern int hfc_debug_ioerr_code;					/* FCLNX-0246 */
#endif												/* FCLNX-0246 */

#define XOB_SEGNUM                  2

extern struct narrow_dev hfc_narrow_dev;			/* FCLNX-0392 */
uchar	logdata[16] ;


/*
 * Function:    hfc_strategy
 *
 * Purpose:     Initiate SCSI command 
 *
 * Arguments:   
 *  cmnd		Pointer to Scsi_Cmnd
 *  iodone		Pointer to done() (Return SCSI response)
 *
 * Returns:     
 *  			0 :  Completed SCSI initiation successfully 
 *  			EIO : Adap_info or dev_info does not exist or invalid.
 * 				(Only for ioctl)
 *
 * Notes:       Caller should be in process level or interruption level.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
int hfc_strategy_lck(struct scsi_cmnd *cmnd)
#else
int hfc_strategy(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */
{
	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		return ( hfc_strategy_pg(cmnd, NULL) );
	} else {
		return ( hfc_manage_info.npubp->hfc_mp_strategy(cmnd, NULL) );	/* FCLNX-GPL-204 */
	}
}

int hfc_eh_abort(struct scsi_cmnd *cmnd)
{
	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		return ( hfc_eh_abort_pg(cmnd) );
	} else {
		return ( hfc_manage_info.npubp->hfc_mp_abort(cmnd) );
	}
}

int hfc_eh_device_reset(struct scsi_cmnd *cmnd)
{
	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		return ( hfc_eh_device_reset_pg(cmnd) );
	} else {
		return ( hfc_manage_info.npubp->hfc_mp_device_reset(cmnd) );
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
int hfc_eh_target_reset(struct scsi_cmnd *cmnd)					/* FCLNX-GPL-0343 */
{
	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		return ( hfc_eh_target_reset_pg(cmnd) );
	} else {
		return ( SUCCESS );
	}
}																/* FCLNX-GPL-0343 */
#endif

int hfc_eh_bus_reset(struct scsi_cmnd *cmnd)
{
	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		return ( hfc_eh_bus_reset_pg(cmnd) );
	} else {
		return ( hfc_manage_info.npubp->hfc_mp_bus_reset(cmnd) );
	}
}

int hfc_strategy_pg(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
{
	int		func_rc = 0 ;
	struct	adap_info	*ap ; 	
	struct	hfc_pkt		*hfcp = NULL;
	uint			result, ioctl_mode=0;
	unsigned long	flags = 0;														/* FCLNX-0274 */
	struct dev_info		*dev = NULL;		/* FCLNX-GPL-0343 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	struct	request			*req=NULL;		/* FCLNX-GPL-409 */
	struct	request_queue	*rq=NULL;		/* FCLNX-GPL-575 */
	struct	scsi_device		*sdev=NULL;		/* FCLNX-GPL-575 */
	uint					timeout=60*HZ;	/* FCLNX-GPL-575 *//* FCLNX-GPL-585 */
#endif

	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);
	
	if (ap != NULL) {
		if (!strcmp(ap->name, "port_info")) {
			/* FIVE-FX */
			return (hfc_fx_strategy_pg(cmnd,iodone));
		}
	}
	
	hfcp = (struct hfc_pkt *)cmnd->host_scribble;
	if(hfcp != NULL)
	{
		if( test_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags ) )
		{
			HFC_DBGPRT("hfcldd%d : hfc_strategy - scsi_cmnd retried\n", ap->dev_minor);
			return(0);
		}
	}

	cmnd->scsi_done = iodone;
	if( iodone == (void *) hfc_ioctl_iodone ) ioctl_mode=1;
	cmnd->result = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	scsi_set_resid(cmnd, 0);
	req = cmnd->request;			/* FCLNX-GPL-409 */
	sdev = cmnd->device;			/* FCLNX-GPL-575 */
	if( sdev != NULL ){
		rq = sdev->request_queue;
	}

	if( !ioctl_mode ){ /* FCLNX-GPL-585,609 */
		if( (req != NULL )&&(rq != NULL) ){	/* FCLNX-GPL-621 */
			if( rq->rq_timeout == 0 ){
				if( req->timeout != 0 ){
					timeout = req->timeout;
				}
				else{
					timeout = 60*HZ;
				}
			}
			else{
				if( req->timeout != 0 ){
					timeout = req->timeout;
				}
				else{
					timeout = rq->rq_timeout;
				}
			}
		}			/* FCLNX-GPL-575,606 *//* FCLNX-GPL-621 */
	}
	else{
		timeout = rq->rq_timeout ;
	} /* FCLNX-GPL-585,609 */

	if( req != NULL ){
		if( req->timeout == 0 ){
			req->timeout = timeout;
		}
	}					/* FCLNX-GPL-409,563 */
#else
	cmnd->resid = 0;
#endif
	
	if( ap == NULL ) 
	{
		
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		if(hfcp != NULL)
			hfcp->adap_status = SCS_NO_ADAPINFO;			/* FCLNX-0534 */
		HFC_ERRPRT(" hfcldd : hfc_strategy - ap==NULL-error\n");

		cmnd->scsi_done(cmnd);
		return(func_rc);
	}

	HFC_ADAPLOCK_IRQSAVE(flags);

	ap->scsi_exec_cnt++;
	HFC_MLPF_STATISTICS_START(ap);	/* FCLNX-GPL-494 */

	hfcp = hfc_get_new_hfcp(ap);
	if( hfcp == NULL){
		
		if(ioctl_mode == 1) func_rc=EIO;

		result = DID_BUS_BUSY;
		cmnd->result |= ( result << 16);
//		hfcp->adap_status = SCS_NO_HFCPKT;		/* FCLNX-0534 */ /* FCLNX-0640 */

		HFC_DBGPRT(" hfcldd : hfc_strategy - hfcp==NULL-error\n");

		ap->scsi_err_cnt++ ;
		ap->scsi_end_cnt++;
		HFC_MLPF_STATISTICS_END(ap);	/* FCLNX-GPL-494 */
		hfc_iodone(ap, cmnd, NULL);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return(func_rc);
	}
	
	dev = (struct dev_info *)CMND_DEV(cmnd);

	memset(hfcp, 0, sizeof(struct hfc_pkt));
	set_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags );
	ap->pkt_no++;
	if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
	ap->pkt_cnt++;
	
	hfcp->cmd_pkt   = cmnd;
	hfcp->target_id = CMND_TARGET(cmnd);
	hfcp->lun_id    = CMND_LUN(cmnd);
	hfcp->ap        = ap;
	hfcp->target    = hfc_hash_target_valid(ap, CMND_TARGET(cmnd));
	hfcp->dev       = NULL;
	hfcp->group_id  = 0;
	hfcp->fail      = 0;
	hfcp->dev 		= dev;						/* FCLNX-GPL-0343 */
	hfcp->timeout	= timeout;					/* FCLNX-GPL-575  */

	cmnd->host_scribble = (void *)hfcp;

	func_rc = hfc_strategy_hfcp(hfcp);

	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	return ( func_rc );
}


int hfc_strategy_hfcp(struct hfc_pkt *hfcp)
{
	int		func_rc = 0 ;
	struct	mp_adap_info *mpap;
	struct	adap_info	 *ap ; 				
	struct	target_info	 *target ;			
	struct	scsi_cmnd 	 *cmnd=hfcp->cmd_pkt;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	struct	scsi_cmnd	 *wk_cmnd=NULL ;
	struct	scsi_device	 *sdev;
	struct	request_queue	*rq = NULL;
	struct	dev_info		*dev = hfcp->dev;
#endif
	
	uint			lun,hit,entry;							  /* FCLNX-0392 */
	char			buf1[32],buf2[32];
	uint			result, ioctl_mode=0;
	ushort			cmd_lun;								/* FCLNX-GPL-0548 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	struct fc_rport *rport = NULL;
	int				 err   = 0;
#endif /* KERNEL_VERSION(2,6,30) */
#endif /* SYSFS_SUPPORT */
#endif

	ap = hfcp->ap;

	if( cmnd->scsi_done == (void *) hfc_ioctl_iodone ) ioctl_mode=1;
	
    mpap = ap->mp_adap_info;
	
    cmd_lun = (ushort)CMND_LUN(cmnd);						/* FCLNX-GPL-0548 */
    cmd_lun = (cmd_lun & 0x3fff);							/* FCLNX-GPL-0548 */
    
	if( test_bit( HFC_DIAG_PROGRESS, (ulong *)&mpap->status ) )
	{
		/* Reject SCSI command while executing DIAG process */
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		hfcp->adap_status = SCS_REJCT_DIAG;					/* FCLNX-0534 */

		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
    }
	
	/* 
	 *Check each element of Scsi_Cmnd structure 
	 */
	if(cmnd->transfersize > (uint)ap->dma_max)
	{
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_DMASIZE_OVER;					/* FCLNX-0534 */

		hfc_iodone(ap, cmnd, hfcp);
		return(0);
	}
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= ap->max_target)
	  || (CMND_TARGET(cmnd) == ap->hosts->this_id)				/* FCLNX-0405 */
	  || (cmd_lun >= MAX_DEV_CNT) ) {							/* FCLNX-GPL-0548 */
		result = DID_BAD_TARGET;
		cmnd->result |= ( result << 16);
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_TARGET_NOTFOUND;					/* FCLNX-0534 */

		hfc_iodone(ap, cmnd, hfcp);
		return(0);
	}
	if(cmnd->cmd_len > 16)
	{
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_CMDLENGTH_OVER;	/* FCLNX-0534 */

		hfc_iodone(ap, cmnd, hfcp);
		return(0);
	}
	
	lun = hfcp->lun_id;

	if(!(test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ) )
    {
		/* Adapter is not intialized */
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		hfcp->adap_status = SCS_ATTATCH_ERROR;	/* FCLNX-0534 */

		HFC_DBGPRT(" hfcldd : hfc_strategy - ap->attach_status-error\n");		/* FCLNX-0244 */

		ap->scsi_err_cnt++ ;

		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
	}
	
	target = hfcp->target;
	
/* FCLNX-GPL-565 start */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if( atomic_read(&ap->rport_event_wait) == 1 )
	{	/* Remote Port Event Waiting */
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_IMM_RETRY;		/* Retry without decrementing retry count  */
		cmnd->result |= ( result << 16);
		hfcp->adap_status = SCS_ADAPTER_OFFLINE;	/* FCLNX-0534 */
		
		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
	}

#ifdef HFC_UBUNTU
	if( target ){
		if( test_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags)
			&& (target->fast_io_fail_tmo == -1) )
		{	/* Remote Port Event Waiting */
			if(ioctl_mode == 1) func_rc=EIO;
			result = DID_IMM_RETRY;		/* Retry without decrementing retry count  */
			cmnd->result |= ( result << 16);
			hfcp->adap_status = SCS_ADAPTER_OFFLINE;	/* FCLNX-0534 */
			
			hfc_iodone(ap, cmnd, hfcp);
			return(func_rc);
		}
	}
#endif

#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
/* FCLNX-GPL-565 end */
	
#if !(defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
	if( !( test_bit( HFC_ONLINE, (ulong *)&ap->status ) ) )
	{
		/* Adapter is not ONLINE */
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_NO_CONNECT;
		cmnd->result |= (result << 16);
		ap->scsi_err_cnt++ ;
		hfcp->adap_status = SCS_ADAPTER_OFFLINE;	/* FCLNX-0534 */
		
		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
	}
#else
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ){ /* FCLNX-GPL-FX-472 */
		if( !( test_bit( HFC_ONLINE, (ulong *)&ap->status ) ) )
		{
			/* Adapter is not ONLINE */
			if(ioctl_mode == 1) func_rc=EIO;
			result = DID_NO_CONNECT;
			cmnd->result |= (result << 16);
			ap->scsi_err_cnt++ ;
			hfcp->adap_status = SCS_ADAPTER_OFFLINE;	/* FCLNX-0534 */
			
			hfc_iodone(ap, cmnd, hfcp);
			return(func_rc);
		}
	}
#endif
	
	/* Is Target initiated? */
	if(target == NULL)
	{
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_BAD_TARGET;
		cmnd->result |= ( result << 16);

		hfcp->adap_status = SCS_NO_TARGET;
		
		hfc_stra_trace(HFC_TRC_STRATEGY ,0x31 ,ap , target, hfcp, (ulong)cmnd, 0,0);
		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
	} 

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		if(ioctl_mode == 0)
		{	/*** This case is Not ioctl_mode ***/
			rport = starget_to_rport(scsi_target(cmnd->device));
//			rport = target->rport;
			if(rport != NULL)
				err   = fc_remote_port_chkready(rport);
			if (err)
			{
				cmnd->result = err;
				hfcp->adap_status = SCS_NO_TARGET;
			
				HFC_DBGPRT(" hfcldd : hfc_strategy - fc_remote_port_chkready() set cmnd->result.\n");
			
				if( (err == DID_NO_CONNECT) || (err == DID_TRANSPORT_FAILFAST) )
				{	/* scsi_err count up */
					ap->scsi_err_cnt++ ;
				}
			
				hfc_stra_trace(HFC_TRC_STRATEGY ,0x71 ,ap , target, hfcp, (ulong)cmnd, 0,0);
				hfc_iodone(ap, cmnd, hfcp);
				return(func_rc);
			}
		}
	}
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
	
	if( !( test_bit( HFC_ONLINE, (ulong *)&ap->status ) ) )
	{
		/* Adapter is not ONLINE */
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_NO_CONNECT;
 		cmnd->result |= (result << 16);
 		ap->scsi_err_cnt++ ;
		hfcp->adap_status = SCS_ADAPTER_OFFLINE;        /* FCLNX-0534 */
		
		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
	}
 	
#endif

	if(!(HFC_TG_FLAGS_TEST( HFC_WWN_VALID, target )))
	{
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_NO_CONNECT;
		cmnd->result |= ( result << 16);
		hfcp->adap_status = SCS_WWN_INVALID;
		
		HFC_DBGPRT(" hfcldd : hfc_strategy - target->flags<HFC_WWN_VALID>=0. \n");
		
		hfc_stra_trace(HFC_TRC_STRATEGY ,0x32 ,ap , target, hfcp, (ulong)cmnd, 0,0);
		hfc_iodone(ap, cmnd, hfcp);
		return(func_rc);
	}
	
	if (ap->narrowmap == 1) {						/* DPM mode */	/* FCLNX-0392 */
		hit=0;
		for (entry=0;entry<8;entry++) {
			if ((ap->boot_priority[entry].ww_name == hfcp->target->ww_name)
			 && (ap->boot_priority[entry].lun     == hfcp->lun_id) ) {
				hit = 1;
				break;
			}
		}
		
		if (hit) {
			hit=0;
			
			if ( (ioctl_mode == 1) && (!hfc_narrow_dev.ap.ww_name
			 || (hfc_narrow_dev.ap.bus > ap->pci_cfginf->bus->number)
			 || ((hfc_narrow_dev.ap.bus == ap->pci_cfginf->bus->number)
			     && (hfc_narrow_dev.ap.devfn > ap->pci_cfginf->devfn))
			 || ((hfc_narrow_dev.ap.bus == ap->pci_cfginf->bus->number)
			     && (hfc_narrow_dev.ap.devfn == ap->pci_cfginf->devfn)
				 && (hfc_narrow_dev.priority > entry))) ) {

				hfc_narrow_dev.ap.ww_name   = ap->ww_name;
				hfc_narrow_dev.ap.bus       = ap->pci_cfginf->bus->number;
				hfc_narrow_dev.ap.devfn     = ap->pci_cfginf->devfn;
				hfc_narrow_dev.priority     = entry;
				hfc_narrow_dev.tgt.ww_name  = hfcp->target->ww_name;
				hfc_narrow_dev.tgt.lun      = hfcp->lun_id;
				hit=1;
				sprintf(buf1,"%llx", (unsigned long long)hfc_narrow_dev.tgt.ww_name);
				sprintf(buf2,"%x", hfc_narrow_dev.tgt.lun);

				HFC_INFPRT("hfcldd%d : narrow mode(lun) - HBA WWPN=%llx(%02x:%02x.%x), Priority=%d, Target WWPN,LU=%s,%s\n",
							ap->dev_minor,
							(unsigned long long) hfc_narrow_dev.ap.ww_name,
							ap->pci_cfginf->bus->number, 
							PCI_SLOT(ap->pci_cfginf->devfn),
							PCI_FUNC(ap->pci_cfginf->devfn),
							hfc_narrow_dev.priority,
							buf1,
							buf2 );
			}
			else {
				if ( (hfc_narrow_dev.ap.ww_name  == ap->ww_name)
				   && (hfc_narrow_dev.tgt.ww_name == hfcp->target->ww_name)
				   && (hfc_narrow_dev.tgt.lun     == hfcp->lun_id) ) {
					hit=1;
				}
			}
		}
		
		if (!hit && !ioctl_mode) {
			result = DID_BAD_TARGET;
			cmnd->result |= ( result << 16);
			hfcp->adap_status = SCS_NARROW_DEV;
			
			set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
			
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x33 ,ap , target, hfcp, (ulong)cmnd, 0,0);
			hfc_iodone(ap, cmnd, hfcp);
			return(func_rc);
		}
	}																/* FCLNX-0392 */

	if ( !test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags )
	  && !test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags ) 
	  && !test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags ) 
	  && !test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags )
	  && !test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) ) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
		wk_cmnd = hfcp->cmd_pkt;
		sdev = wk_cmnd->device;
		if( sdev != NULL ){
			rq = sdev->request_queue;
		}
		
		if( rq != NULL ){	/* FCLNX-GPL-575 */
//			if( rq->timeout != NULL ){
				if (!ioctl_mode && rq->timeout.function) {	/* FCLNX-GPL-409 */
//					mod_timer( &rq->timeout, ( jiffies + ( (rq->rq_timeout/HZ) + 60*60)*HZ ) );
					del_timer( &rq->timeout );
				}											/* FCLNX-GPL-409 */
//			}
		}					/* FCLNX-GPL-575 */
#else
		if (!ioctl_mode && cmnd->eh_timeout.function) {					/* FCLNX-0269 */
			mod_timer( &cmnd->eh_timeout, ( jiffies + ( (cmnd->timeout_per_command/HZ) + 60*60)*HZ ) );	/* FCLNX-0521 */
		}																/* FCLNX-0287 */
#endif
	}

	hfcp->target = target;
	hfc_enqueue_wx_que(target, hfcp);

	hfc_stra_trace(HFC_TRC_STRATEGY ,0x20 ,ap , target, hfcp, (ulong)cmnd, 0, 0);

	/* Start SCSI command if target is valid */
	if(  !( test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status ))
	  && !( test_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status ))
	  && !( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ))
	  && !( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ))
	  && !( dev->lustat )						/* FCLNX-GPL-0343 */
	  && !( test_bit( HFC_ISOL, (ulong *)&ap->status ))	  /* FCLNX-GPL-572 */
	  && ( !HFC_MMODE_CHECK_SHARED(ap) || !test_bit( HFC_WAIT_T3, (ulong *)&ap->status ))	/* FCLNX-GPL-0320 */
	  &&  (target->status == HFC_NON_STATUS))
	{
		hfc_start( ap, target, hfcp) ;	
	}
	else
	{
		if(target->next_dstart_flag == 0)					/* FCLNX-GPL-046 */
		{
			hfc_enque_next_dstart(ap,target);				/* FCLNX-GPL-046 */
		}
		if(  ( test_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status ))
		  || ( test_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status ))
		  || ( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ))
		  || ( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ))
		  || ( test_bit( HFC_ISOL, (ulong *)&ap->status ))){	  /* FCLNX-GPL-572 */
			HFC_DBGPRT(" hfcldd : hfc_strategy_hfcp - wait ap->status = 0x%08x \n", ap->status);

			hfcp->adap_status = SCS_SCSI_DELAY;
			return(func_rc);
		}

		if( HFC_TG_STATUS_TEST( HFC_SCSI_DELAY, target ) )
		{
			HFC_DBGPRT(" hfcldd : hfc_strategy_hfcp - target : HFC_SCSI_DELAY\n");
			
			hfcp->adap_status = SCS_SCSI_DELAY;
			
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x61 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
			return(func_rc);
		}
		
		if( HFC_TG_STATUS_TEST( HFC_SCN_WLINKUP, target ) )
		{
			HFC_DBGPRT(" hfcldd : hfc_strategy_hfcp - target : HFC_SCN_WLINKUP\n");
			
			hfcp->adap_status = SCS_SCSI_DELAY;
			
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x65 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
			return(func_rc);
		}
		
		if ( test_bit(HFC_WAIT_TGTRSP, (ulong *)&target->target_reset) )	/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
		{
			HFC_DBGPRT(" hfcldd : hfc_strategy_hfcp - target->target_reset : HFC_WAIT_TGTRSP\n");
			
			hfcp->adap_status = SCS_SCSI_DELAY;
			
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x66 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
			return(func_rc);
		}
		
		if( dev->lustat)
		{
			if(dev->lustat & HFC_WAIT_ABORT)
			{
				HFC_DBGPRT(" hfcldd : hfc_strategy - lustat : HFC_WAIT_ABORT\n");

				if(ioctl_mode == 1) func_rc=EIO;

				hfcp->adap_status = SCS_CMD_ABORTED;
				hfc_stra_trace(HFC_TRC_STRATEGY ,0x45 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
				return(func_rc);
			}
			else if(dev->lustat & HFC_NEED_ABORT)
			{
				HFC_DBGPRT(" hfcldd : hfc_strategy - lustat : HFC_NEED_ABORT\n");

				hfcp->adap_status = SCS_CMD_NEED_ABORT;
				hfc_stra_trace(HFC_TRC_STRATEGY ,0x46 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
				return(func_rc);
			}
			else if(dev->lustat & HFC_WAIT_LUN_RESET)
			{
				HFC_DBGPRT(" hfcldd : hfc_strategy - lustat : HFC_WAIT_LUN_RESET\n");

				hfcp->adap_status = SCS_WAIT_LUNRST;
				hfc_stra_trace(HFC_TRC_STRATEGY ,0x47 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
				return(func_rc);
			}
			else if(dev->lustat & HFC_NEED_LUN_RESET)
			{
				HFC_DBGPRT(" hfcldd : hfc_strategy - lustat : HFC_NEED_LUN_RESET\n");

				hfcp->adap_status = SCS_NEED_LUNRST;
				hfc_stra_trace(HFC_TRC_STRATEGY ,0x48 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
				return(func_rc);
			}
		
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x43 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
		}
		
		/* Check target status */
		if( HFC_TG_STATUS_TEST(HFC_WAIT_LOGIN, target))
		{
			hfcp->adap_status = SCS_WAIT_LOGIN;
			
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x62 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
			return(func_rc);
		}
		
		/* Is target in NEED_LOGIN or WAIT_PDISC state? */
		if( (HFC_TG_STATUS_TEST( HFC_NEED_LOGIN, target ) ) && !( HFC_TG_STATUS_TEST( HFC_WAIT_PDISC, target) ) )
		{
			if( hfc_issue_relogin(ap,target) ) 
			{	/* LOGIN initiation failed*/
				HFC_DBGPRT( " hfcldd : hfc_strategy - re_login failed-error\n");
				
				hfc_stra_trace(HFC_TRC_STRATEGY ,0x51 ,ap ,target ,hfcp, (ulong)cmnd, 0, 0);
				return(func_rc);
			}
		}

		
		if( HFC_TG_STATUS_TEST( HFC_WAIT_TARGET_RESET, target ))
		{
			hfcp->adap_status = SCS_CMD_RESET;
			
			hfc_stra_trace(HFC_TRC_STRATEGY ,0x63 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
			return(func_rc);
		}

		/* XOB HALT */
		HFC_DBGPRT(" hfcldd : hfc_strategy - DID_BUS_BUSY\n");

		hfcp->adap_status = SCS_TARGET_ABNORMAL;

		hfc_stra_trace(HFC_TRC_STRATEGY ,0x64 ,ap ,target ,hfcp, (ulong)cmnd,
					 0, 0);
		return(func_rc);
	}

	return(func_rc);

}  


/*
 * Function:    hfc_start
 *
 * Purpose:     Dequeue command from wait_xob_que and set it to xob, then start firmware.
 *              If xob initiation succeeds, enqueue it to wait_end_que.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  pkt        - Pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       Caller should be process level or interrupt level.
 *              Lock adap_info before calling this function.
 *    
 */
int hfc_start(struct adap_info    *ap ,
               struct target_info  *target,
               struct hfc_pkt      *pkt)
{
	int		rc = 0 ;

	uchar	xob_empty = FALSE ;
	struct	scsi_cmnd	*cmnd=NULL ;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	struct	scsi_cmnd	*wk_cmnd=NULL ;
	struct	scsi_device	*sdev;
	struct	request_queue	*rq;
	struct	dev_info	*dev = NULL;
#endif
	uint	save_outp_num ,
			save_xob_page ,
			save_xob_entry,
			end_outp_num ;
	uint	chk_xob_outp = 0, fw_xob_outp, work_xob_outp;

	uint	xob_exec ;
	int		func_rc, lp=0, lun, ioctl_mode=0 ;
	uint	total_we_cnt = 0 ;
	uint	wk_finp, wk_cnt ;
	uchar	xob_lap = FALSE, strat_xob_cnt ;
	struct	target_info		*target_wk;
	struct	hfc_pkt		*hfcp, *hfcp_wk;
	int		result;
	uchar 	area;
	/* uint	hyp_status = 0; */	/* FCLNX-GPL-428 */

	
	if(!(test_bit( HFC_ONLINE, (ulong *)&ap->status ) ) )
    {
		HFC_DBGPRT(" hfcldd : hfc_start - ap->status != HFC_ONLINE\n");

		hfc_errlog(ap,target,pkt,HFC_ERRLOG_TYPE_NONE,
							ERRID_HFCP_ERR9,0x04,NULL,0) ;
		return(FALSE) ;
	}

	if( test_bit( HFC_SCSI_DELAY, (ulong *)&target->status ) )
	{
		hfc_stra_trace(HFC_TRC_START ,0x61 ,ap , target, pkt, (ulong)cmnd, 0, 0);
		return(FALSE) ;
	}
	
	hfc_stra_trace(HFC_TRC_START,0x00, ap,target,NULL,(ulong)cmnd,0,0) ;

	/* 
	 * Repeat dequeuing command from Register wait_xob_queue and set it to xob 
	 * until wait_xob_queue is empty.
	 */
	 
	/* Setup hfc_pkt pointer */
	work_xob_outp = ap->fw_init_p->xob_outp;
	HFC_4B_TO_4L(fw_xob_outp, work_xob_outp);

	hfcp = target->wx_que_top;
	while( hfcp != NULL)
	{
		dev = hfcp->dev;		/* FCLNX-GPL-0343 */
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info) {
			if ( hfc_manage_info.npubp->hfc_queue_check(hfcp) ) {			/* FCLNX-0521 */
				hfcp_wk = hfcp->cmd_forw;
				hfcp = hfcp_wk;
//				printk(KERN_ERR "hfcldd%d hfc_queue_check()=1.\n", ap->dev_minor);
				continue;
			}
		}

		work_xob_outp = ap->fw_init_p->xob_outp;
		HFC_4B_TO_4L(fw_xob_outp, work_xob_outp);

		cmnd = hfcp->cmd_pkt;
		lun  = hfcp->lun_id;

		if(cmnd != NULL)
		{
			if( cmnd->scsi_done == (void *) hfc_ioctl_iodone ) ioctl_mode=1;
		}

		/* Not issue I/O command during executing Abort_Task_Set or LUN_Reset */ /* FCLNX-GPL-289 *//* FCLNX-GPL-336 */
		if( ( HFC_HFCP_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)			/* Target Reset Command */
				|| HFC_HFCP_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) )
				&& ( test_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset ) )){	/* Target Reset Command */
				/* NOP */
		}
		else if( ( HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp)			/* Abort Task Set Command */
				|| HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) )		/* LUN Reset Command	*/
				&& ( dev->lustat & (HFC_WAIT_ABORT | HFC_WAIT_LUN_RESET) ) ){
				hfcp_wk = hfcp->cmd_forw;
				hfcp = hfcp_wk;
				continue ;
		}
		else if( ( HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp)			/* Abort Task Set Command */
				|| HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) )		/* LUN Reset Command	*/
				&& ( dev->lustat & (HFC_NEED_ABORT | HFC_NEED_LUN_RESET) ) ){
				/* NOP */
		}
		else if( (!HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp))			/* SCSI Command */
			&& (!HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp))
			&& (!HFC_HFCP_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp))
			&& (!HFC_HFCP_CFLAG_TEST(CFLAG_BUS_RESET, hfcp))
			&& ( dev->lustat ) ){
				hfcp_wk = hfcp->cmd_forw;
				hfcp = hfcp_wk;
				continue ;
		}
		else if( (!HFC_HFCP_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp))
			&& (!HFC_HFCP_CFLAG_TEST(CFLAG_BUS_RESET, hfcp))
			&& ( target->status ) ){
				hfcp_wk = hfcp->cmd_forw;
				hfcp = hfcp_wk;
				continue ;
		}															/* FCLNX-GPL-289 *//* FCLNX-GPL-336 */

		/* One or more commands exist in wait_xob_que */
		if( (rc = hfc_resource_chk(ap, target, hfcp, fw_xob_outp)) != 0 )
		{
			if (rc == HFC_IOVMAP_FULL)
			{
				HFC_DBGPRT(" hfcldd : hfc_start - hfc_resource_chk = HFC_IOVMAP_FULL\n");
				ap->iovmap_full_cnt++;	/* FCLNX-GPL-143 */
				if ( !(HFC_MMODE_CHECK_BASIC(ap)) && (ap->hg_cca_p != NULL) )
					ap->hg_cca_p->iov_full   = ap->iovmap_full_cnt;	/* FCLNX-GPL-494 */
			}
			if (rc == HFC_XOB_FULL )	/* FCLNX-GPL-143 */
			{
				ap->xob_full_cnt++;		/* FCLNX-GPL-143 */
				if ( !(HFC_MMODE_CHECK_BASIC(ap)) && (ap->hg_cca_p != NULL) )
					ap->hg_cca_p->xob_full   = ap->xob_full_cnt;	/* FCLNX-GPL-494 */
			}	
			if( rc == HFC_PAGE_OVER )
			{
				/* Specified page is too large in number */
				ap->page_over_cnt++;	/* FCLNX-GPL-143 */
				if ( !(HFC_MMODE_CHECK_BASIC(ap)) && (ap->hg_cca_p != NULL) )
					ap->hg_cca_p->page_over  = ap->page_over_cnt;	/* FCLNX-GPL-494 */
				if(ioctl_mode == 1) func_rc=EIO;
				if(cmnd != NULL){
					result = DID_ERROR;
					cmnd->result |= ( result << 16);
				}
				hfcp->adap_status = SCS_PAGE_OVER;		/* FCLNX-0534 */

				HFC_ERRPRT(" hfcldd : hfc_start - hfc_resource_chk = HFC_PAGE_OVER\n");
				ap->scsi_err_cnt++ ;
				
				/* Dequeue command from wait_xob_que */
				hfc_deque_wx_que(target, hfcp);
				
				if(cmnd != NULL)
					hfc_iodone(ap, cmnd, hfcp);
				
				hfcp_wk = hfcp->cmd_forw; /* FCLNX-GPL-185 */
				hfcp = hfcp_wk; /* FCLNX-GPL-185 */
				
				continue ;
			}
			
			if( rc != HFC_XOB_EMPTY )
			{
				/* Break the loop in case response is not XOB_EMPTY*/
				hfc_enque_next_dstart(ap,target) ;
				break ;
			}
			/* Xob is empty */
			/* Save Xob_outp and clear start count */
			xob_empty = TRUE ;
			ap->xob_exec_cnt = 0 ;
			/* Use frame 4  */
			ap->frame_inp = 0 ;
			ap->frame_chkp = 0 ;
			ap->save_xob_outp[0] = fw_xob_outp;
		}
			
		/* Clear xob */
		HFC_BZERO(&ap->xob[ap->xob_no],sizeof(struct xob));	
		
		/* Map DMA area for data transfer */
		if(!(HFC_HFCP_CFLAG_TEST(CFLAG_SEGVALID, hfcp) ) )
		{
			if( (rc = hfc_dma_map(ap, hfcp)) != 0 )
			{
				if( rc == FAILED )  
					 /* No space for DMA. Break while loop */
				{
					HFC_ERRPRT(" hfcldd : hfc_start - hfc_dma_map = DMA_NORES | DMA_DIOFULL\n");

					/* 
					 * Check whether we_que is empty or not. 
					 * If no command exists in we_que, return EIO 
					 */
					total_we_cnt = 0 ;
					for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
					{
						target_wk = hfc_hash_target_info(ap, lp);
						while( target_wk != NULL )
						{
							if( target_wk->we_que_cnt != 0 )
								total_we_cnt += target_wk->we_que_cnt ;
							target_wk = target_wk->next ;
						}
					}
					if( total_we_cnt == 0 )
					{
						result = DID_ERROR;
						if(cmnd != NULL)
							cmnd->result |= ( result << 16);
						hfcp->adap_status = SCS_NO_WE_QUE;				/* FCLNX-0534 */

						HFC_ERRPRT(" hfcldd : hfc_start - total_we_cnt == 0\n");

						ap->scsi_err_cnt++ ;
						/* Dequeue command from wait_xob_que */
						hfc_deque_wx_que(target, hfcp);
						
						/* 
						 * If error occurs while handling task management command, 
						 * clear status flag
						 */
						if( (HFC_HFCP_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)) &&
							(HFC_TG_STATUS_TEST(HFC_WAIT_TARGET_RESET, target) ) )
							clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status );
						else if( (HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp)) &&
							(dev->lustat & HFC_WAIT_ABORT) )			/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
							dev->lustat &= ~HFC_WAIT_ABORT;				/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
						else if( (HFC_HFCP_CFLAG_TEST(CFLAG_BUS_RESET, hfcp)) &&
							(HFC_TG_STATUS_TEST(HFC_WAIT_BUS_RESET, target) ) )
							clear_bit(HFC_WAIT_BUS_RESET, (ulong *)&target->status );
						else if( (HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp)) &&
							(dev->lustat & HFC_WAIT_LUN_RESET ) )		/* FCLNX-GPL-0343 */
							dev->lustat &= ~HFC_WAIT_LUN_RESET;			/* FCLNX-GPL-0343 */

						if(cmnd != NULL) /* FCLNX-GPL-185 */
							hfc_iodone(ap, cmnd, hfcp);

						hfc_stra_trace(HFC_TRC_START,0x80,
							ap,target,hfcp,(ulong)cmnd,0,0) ;
							
						hfcp_wk = hfcp->cmd_forw; /* FCLNX-GPL-185 */
						hfcp = hfcp_wk; /* FCLNX-GPL-185 */
	
						continue ;
					}
					hfc_enque_next_dstart(ap,target) ;

					break ;
				}
			}
		}

		/* Not issue I/O command during executing Abort_Task_Set or LUN_Reset */ /* FCLNX-GPL-289 *//* FCLNX-GPL-336 */
		if( ( HFC_HFCP_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)			/* Target Reset Command */
				|| HFC_HFCP_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) )
				&& ( test_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset ) )){	/* Target Reset Command */
					clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );	
					set_bit( HFC_WAIT_TARGET_RESET, (ulong *)&target->status );				/* Issue Target Reset during Abort_Task_Set or LUN_Reset */
		}
		else if( HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp)				/* Abort Task Set Command */
				&& ( dev->lustat & HFC_NEED_ABORT ) ){
				dev->lustat &= ~HFC_NEED_ABORT;
				dev->lustat |=  HFC_WAIT_ABORT;
		}															/* FCLNX-GPL-289 *//* FCLNX-GPL-336 */
		else if( HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) 		/* LUN Reset Command	*/
				&& ( dev->lustat & HFC_NEED_LUN_RESET ) ){
				dev->lustat &= ~HFC_NEED_LUN_RESET;
				dev->lustat |=  HFC_WAIT_LUN_RESET;
		}															/* FCLNX-GPL-289 *//* FCLNX-GPL-336 */

		hfc_make_cmdiu( ap, hfcp );

		/* Setup Xob */
		hfcp_wk = hfcp->cmd_forw;
		hfc_xob_enque(ap, target, hfcp) ;
		
		if ( HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp)				/* Abort Task Set */
		  || HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ) {		/* LUN reset */
				
				hfc_watchdog_enter( ap,target, hfcp, 0, HFC_ABORT_TMR,0,FALSE );
		}
		else if ( HFC_HFCP_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)	/* Target Reset from eh_device_reset */
			   || HFC_HFCP_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) ) {/* Target Reset from eh_bus_reset */
				
				hfc_watchdog_enter( ap,target, hfcp, 0, HFC_TARGET_RST_TMR,0,FALSE );
		}
		else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
			if ( !test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) ) {
				wk_cmnd = hfcp->cmd_pkt;
				sdev = wk_cmnd->device;
				rq = sdev->request_queue;
				hfc_watchdog_enter( ap,target, hfcp, 0, HFC_SCSI_CMD_TMR, hfcp->timeout, FALSE );	/* FCLNX-GPL-575 */
			}
			else {
				if (hfc_manage_info.hfcldd_mp_mod) {
					hfc_manage_info.npubp->hfc_set_scsi_cmd_tmr(ap, target, hfcp);
				}
			}
#else
//			if ( test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) )				/* FCLNX-0521 */
				hfc_watchdog_enter( ap,target, hfcp, 0, HFC_SCSI_CMD_TMR, hfcp->cmd_pkt->timeout_per_command, FALSE );
#endif
		}
		
		hfc_stra_trace(HFC_TRC_START,0x20,
				ap,target,hfcp, 0, 0, 0) ;
		hfcp = hfcp_wk;
		
	}	

	/* Check commands in xob waiting for initiation */
	if( ap->xob_wait_exec_cnt == 0 )
 	{
		hfc_stra_trace(HFC_TRC_START,0x12,
				ap,target,NULL, 0, 0, 0) ;
		hfc_enque_next_dstart(ap,target) ;								/* FCLNX-0105 */
		return(TRUE) ;
	}

	strat_xob_cnt = ap->xob_wait_exec_cnt;

	if( xob_empty == FALSE )
	{
		/* Caliculate the number of xobs in save_xob_outp */
		if( ap->frame_inp > ap->frame_chkp )
		{
			lp = ap->frame_inp - ap->frame_chkp;
		}
		else if( ap->frame_inp < ap->frame_chkp )
		{
			lp = ap->frame_inp + (MAX_FRAME_CNT - ap->frame_chkp) ;
		}
		else
		{
			lp = MAX_FRAME_CNT;
		}
		/* Check whether XOB queue is wraparound or not */
		chk_xob_outp = fw_xob_outp;
		/* 
		 * Compare current xob_outp with saved xob_outp and make sure that fw 
		 * have aldeay handled one or more xob in queue. 
		 */
		do 
		{
			area=TRUE;

			if( ap->save_xob_outp[ap->frame_chkp] > ap->xob_outp_end[ap->frame_chkp] )
				xob_lap = TRUE;
			else
				xob_lap = FALSE;

			/* 
			 * xob_outp > save_xob_outp	 
			 *  In this case FW have already handled one or more xob then 
			 *  check next xob_save_outp with incrementing ap->frame_chkp
			 * 
			 * Break from loop in case frame_inp = frame_chkp 
			 *  (This case has already checked)
			 */

			if(xob_lap != TRUE)
			{
				if( (chk_xob_outp < ap->save_xob_outp[ap->frame_chkp])
				 || (chk_xob_outp >= ap->xob_outp_end[ap->frame_chkp]) )
					area = FALSE;
			}
			else {
				if( (chk_xob_outp < ap->save_xob_outp[ap->frame_chkp])
				 && (chk_xob_outp >= ap->xob_outp_end[ap->frame_chkp]) )
					area = FALSE;
			}

			if (area == TRUE) {
				if( lp==MAX_FRAME_CNT ) { 
					/* The number of executing frame is full */
					hfc_stra_trace(HFC_TRC_START,0x13,
						ap,target,NULL, 0, 0, 0);
					hfc_enque_next_dstart(ap,target) ;
					ap->frame_full_cnt++; /* FCLNX-GPL-143 */
					if ( !(HFC_MMODE_CHECK_BASIC(ap)) && (ap->hg_cca_p != NULL) )
						ap->hg_cca_p->frame_full = ap->frame_full_cnt;	/* FCLNX-GPL-494 */
					return(TRUE) ;
				}
				
				/* Check xob lap in frame */
				wk_finp = (ap->frame_inp) ? (ap->frame_inp-1) : (MAX_FRAME_CNT-1) ;
				save_outp_num = ((ap->save_xob_outp[ap->frame_chkp] & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
				save_outp_num += (ap->save_xob_outp[ap->frame_chkp] & 0x0000ffff) ;
				end_outp_num  = ((ap->xob_outp_end[wk_finp] & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
				end_outp_num  += (ap->xob_outp_end[wk_finp] & 0x0000ffff) ;
				
				if ( save_outp_num > end_outp_num )
					wk_cnt = ap->xob_max - (save_outp_num - end_outp_num);
				else
					wk_cnt = end_outp_num - save_outp_num;

				/* Limit executable xob number not to wraparound xob queue in save_xop_outp */
				if ((ap->xob_wait_exec_cnt + wk_cnt) > (ap->xob_max-1))
					strat_xob_cnt = (ap->xob_max-1) - wk_cnt;

				/* Limit executable xob number by half not to concentrate access in one frame */
				if (strat_xob_cnt > (ap->xob_max/2))
					strat_xob_cnt = (ap->xob_max/2);

				if ( strat_xob_cnt == 0 ) { 
					/* Unable to initiate frame */
					hfc_stra_trace(HFC_TRC_START,0x13,
						ap,target,NULL, 0, 0, 0);
					hfc_enque_next_dstart(ap,target) ;
					ap->frame_full_cnt++; /* FCLNX-GPL-143 */
					if ( !(HFC_MMODE_CHECK_BASIC(ap)) && (ap->hg_cca_p != NULL) )
						ap->hg_cca_p->frame_full = ap->frame_full_cnt;	/* FCLNX-GPL-494 */
					return(TRUE) ;
				}

				break;
			}
			else
			{
				ap->frame_chkp++ ;
				if( ap->frame_chkp == MAX_FRAME_CNT )
					ap->frame_chkp = 0 ;
			}
			lp-- ;
		}while( lp ) ;
	}
	
	/*-- Collect bstart trace (Presence of F/W initiation) */

	
	/* Initiate XOB */
	/* Set XOB_ENQ command and the number of initiated XOB in frame A */
	xob_exec = 0 ;
	xob_exec = (uint) strat_xob_cnt ;
	xob_exec |= HFC_FRAMEA_ENQ_XOB ;

	hfc_write_reg(ap,( uint )HFC_IOSPACE_FRAMEA,( char )0x4, ( int )xob_exec );

	/*-- xob_outp format   --*/
	/*--   +0   : not used. --*/
	/*--   +1   : page_no --*/
	/*--   +2,3 : entry No.--*/

	if( xob_empty == TRUE )
	/* 
	 * Only reset flag here because initp->xob_outp is already set in save_xob_outp[0] 
	 * when xob is empty. 
	 */
	{
		xob_empty = FALSE ;
	}
	else
	{	
		/* 
		 * Caliculate sequential number of save_outp from page number and entry number
		 *
		 */
		if( ap->frame_inp == 0 )
			wk_finp = MAX_FRAME_CNT - 1 ;
		else
			wk_finp = ap->frame_inp - 1 ;
		
		ap->save_xob_outp[ap->frame_inp] = ap->xob_outp_end[wk_finp];
	}


	/*-----------------------------------------------------------------------------*/
	save_outp_num = ((ap->save_xob_outp[ap->frame_inp] & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	save_outp_num += (ap->save_xob_outp[ap->frame_inp] & 0x0000ffff) ;
	/* 
	 * Caliculate sequential number of outp including xob_wait_exec_cnt.
	 */
	save_outp_num = (save_outp_num + strat_xob_cnt)%ap->xob_max;

	/* Convert sequential number into xob_outp format and set it to ap->xob_outp_end */ 

	save_xob_page = save_outp_num / HFC_XOB_PER_PAGE ;
	save_xob_entry = save_outp_num % HFC_XOB_PER_PAGE ;
	ap->xob_outp_end[ap->frame_inp] = ((save_xob_page << 16) & 0x00ff0000) | 
						(save_xob_entry & 0x0000ffff) ;
	ap->frame_inp++ ;
	if( ap->frame_inp == MAX_FRAME_CNT )
		ap->frame_inp = 0 ;
	/*-----------------------------------------------------------------------------*/


	ap->xob_exec_cnt = strat_xob_cnt ;
	ap->xob_wait_exec_cnt -= strat_xob_cnt ;
	
	if (ap->xob_wait_exec_cnt)											/* FCLNX-0105 STR*/
	{
		/* Several XOBs remain in queue */
		hfc_enque_next_dstart(ap,target) ;
	}																	/* FCLNX-0105 END*/
	else {																/* FCLNX-GPL-453 STR*/
		if (target->wx_que_cnt) {
			if (!target->next_dstart_flag) {
				hfc_enque_next_dstart(ap,target);
			}
		}
		else {
			if (target->next_dstart_flag) {
				hfc_deque_next_dstart(ap,target);
			}
		}
	}																	/* FCLNX-GPL-453 END*/
	
	hfc_stra_trace(HFC_TRC_START,0x10,
			ap, target, NULL, 0, 0, 0);
	
	return(TRUE) ;
}		


/*
 * Function:    hfc_get_new_hfcp
 *
 * Purpose:     Search empty hfc_pkt 
 *
 * Arguments:   
 *  ap          Pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
struct hfc_pkt *
hfc_get_new_hfcp(struct adap_info *ap)
{
	int   i;
	struct hfc_pkt *hfcp;

	if (ap->pkt_pool[0] == NULL) {
		return(NULL);
	}

	for (i=0;i<ap->pkt_num;i++)
	{
		hfcp = &ap->pkt_pool[ap->pkt_no / HFC_PKT_POOL_SIZE][ap->pkt_no % HFC_PKT_POOL_SIZE];	/* FCLNX-0521 */
		if(!(test_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags)))
			return (hfcp);
		
		ap->pkt_no++;
		if (ap->pkt_no >= ap->pkt_num) 
			ap->pkt_no = 0;
	}

	return(NULL);
}


/*
 * Function:    
 *
 * Purpose:     Search empty dummy command 
 *
 * Arguments:   
 *  ap          Pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
struct scsi_cmnd *
hfc_get_new_cmnd(struct adap_info *ap)
{
	int   i;

	for (i=0;i<ap->cmnd_num;i++)
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
		if(!(test_bit(CMND_VALID, (ulong *)&ap->cmnd_pool[ap->cmnd_no].sc_magic)))
#else
		if(!(test_bit(CMND_VALID, (ulong *)&ap->cmnd_pool[ap->cmnd_no].eh_eflags)))
#endif
			return (&ap->cmnd_pool[ap->cmnd_no]);
		
		ap->cmnd_no++;
		if (ap->cmnd_no >= ap->cmnd_num) 
			ap->cmnd_no = 0;
	}

	return(NULL);
}


/*
 * Function:    hfc_dummy_copy
 *
 * Purpose:     
 *
 * Arguments:   
 *  cmnd       -
 *  dummy_cmnd -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_dummy_copy(struct adap_info *ap, struct scsi_cmnd *cmnd, struct scsi_cmnd *dummy_cmnd )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
//	dummy_cmnd->device = (struct scsi_device*)hfc_kmalloc(ap, sizeof(struct scsi_device), GFP_ATOMIC);
//	dummy_cmnd->cmnd = (uchar *)hfc_kmalloc(ap, 16, GFP_ATOMIC);
#else
	dummy_cmnd->timeout_per_command = cmnd->timeout_per_command;
	dummy_cmnd->device = (struct scsi_device*)hfc_kmalloc(ap, sizeof(struct scsi_device), GFP_ATOMIC);
#endif
	
	if(dummy_cmnd->device == NULL) 
	{
		return;
	}
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	if(dummy_cmnd->cmnd == NULL) 
	{
		return;
	}
#endif
	
	CMND_TARGET(dummy_cmnd) = CMND_TARGET(cmnd);

	CMND_LUN(dummy_cmnd) = CMND_LUN(cmnd);
	CMND_CHANNEL(dummy_cmnd) = CMND_CHANNEL(cmnd);

	dummy_cmnd->cmd_len = cmnd->cmd_len;
	dummy_cmnd->cmnd[0] = cmnd->cmnd[0];
	dummy_cmnd->sc_data_direction = cmnd->sc_data_direction;
	dummy_cmnd->scsi_done = cmnd->scsi_done;
	dummy_cmnd->result = 0;		/* FCLNX-GPL-0343 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
	set_bit(CMND_VALID, (ulong *)&dummy_cmnd->sc_magic );
#else
	set_bit(CMND_VALID, (ulong *)&dummy_cmnd->eh_eflags );
#endif
	
}

/*
 * Function:    hfc_eh_abort_pg
 *
 * Purpose:     Issue ABORT TASK SET
 *
 * Arguments:   
 *  cmnd       - Pointer to Scsi_Cmnd
 *
 * Returns:     
 *  SUCCESS    - Start succeeded
 *  FAILED     - Start failed
 *
 *
 * Notes:       
 */
int hfc_eh_abort_pg(struct scsi_cmnd *cmnd)
{
	struct	adap_info	*ap ;
	struct	target_info	*target ;
	struct hfc_pkt		*hfcp = NULL, *abort_pkt=NULL;
	struct hfc_pkt		*wx_hfcp = NULL;
	struct scsi_cmnd	*dummy_cmnd=NULL;
	uint                lun,find;
	unsigned long		flags = 0;									/* FCLNX-0274 */
	struct dev_info		*dev=NULL;			/* FCLNX-GPL-0343 */
	ushort				cmd_lun;			/* FCLNX-GPL-0548 */
	
	/* Check argument(NULL?) */
	if( cmnd == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_abort - invalid argument \n");
		return(FAILED);
    }
	
	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);
	
	if (ap != NULL) {
		if (!strcmp(ap->name, "port_info")) {
			/* FIVE-FX */
			return (hfc_fx_eh_abort_pg(cmnd));
		}
	}
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_lock_irq(cmnd->device->host->host_lock);
#endif

//	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);
	dev = (struct dev_info *) CMND_DEV(cmnd);

	
	cmd_lun = (ushort)CMND_LUN(cmnd);				/* FCLNX-GPL-0548 */
    cmd_lun = (cmd_lun & 0x3fff);					/* FCLNX-GPL-0548 */

	HFC_DBGPRT(" hfcldd%d : hfc_eh_abort - start channel=%d, tid=%d, lun=%d. \n",
					ap->dev_minor, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd), CMND_LUN(cmnd));
	
	/* Check adap_info (NULL?) */
	if( ap == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_abort - 1\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }
    
    HFC_ADAPLOCK_IRQSAVE(flags);
	
	hfc_stra_trace(HFC_TRC_ABORT ,0x00 ,ap ,NULL , NULL, (ulong)cmnd, 0, 0);
	
	/* Find hfc_pkt */
	hfcp = (struct hfc_pkt *)cmnd->host_scribble;
	if( hfcp == NULL ){
		HFC_DBGPRT("hfcldd : hfc_eh_abort - already complete \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}
	
	/* Check each element of scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= ap->max_target)
	  || (CMND_TARGET(cmnd) == ap->hosts->this_id)
	  || (cmd_lun >= MAX_DEV_CNT) )					/* FCLNX-GPL-0548 */
	{
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}

	if ( !(test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ) )
    {
		/* Adapter is not initialized */
		HFC_DBGPRT("hfcldd : hfc_eh_abort - not attach status \n");

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }

	if ( !test_bit(HFC_ONLINE, (ulong *)&ap->status) )
	{
		/* Adapter is not online (link down) */
		HFC_DBGPRT("hfcldd : hfc_eh_abort - HFC_ONLINE=0 \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}

	if( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ) )
	{
		/* Adapter is executing BUS RESET */
		HFC_DBGPRT("hfcldd : hfc_eh_abort - exec bus reset\n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}

	target = hfc_hash_target_info(ap, CMND_TARGET(cmnd));
	lun = CMND_LUN(cmnd);

	/* Check target_info */
	if(target == NULL)
	{
		HFC_DBGPRT(KERN_ERR"hfcldd : hfc_eh_abort - target == NULL \n");

		hfc_stra_trace(HFC_TRC_ABORT ,0x31 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	} 
	
	/* Check Timeout command */									/* FCLNX-GPL-0153 */
	wx_hfcp = target->wx_que_top;
	
	while( wx_hfcp != NULL )
	{
		if( wx_hfcp == hfcp )
			break;
		wx_hfcp = wx_hfcp->cmd_forw;
	}
	
	if( wx_hfcp != NULL )
	{
		hfc_deque_wx_que(target, wx_hfcp);
		set_bit( CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );
		hfc_iodone(ap, wx_hfcp->cmd_pkt, wx_hfcp);
		HFC_DBGPRT(KERN_ERR"hfcldd : hfc_eh_abort - Timeout command remains in wx_que.\n");
		hfc_stra_trace(HFC_TRC_ABORT ,0x32 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}															/* FCLNX-GPL-0153 */
	
	/* Target state is in NEED_LOGIN or PDISC is running */
	if (target->status)
	{
		/* target status is non zero */
		HFC_DBGPRT("hfcldd : hfc_eh_abort - target status non zero \n");
	
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}

	set_bit( CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );

	if (dev->lustat & (HFC_NEED_ABORT | HFC_WAIT_ABORT) )
	{
		/* Already issue abort task set */
		HFC_DBGPRT("hfcldd : hfc_eh_abort - HFC_WAIT_ABORT/HFC_NEED_ABORT \n");

		hfc_stra_trace(HFC_TRC_ABORT ,0x45 ,ap , target, hfcp, (ulong)cmnd, 0, 0);
		goto WAIT_ABORT_TS;
	}
	
	if( hfc_toutchk_xob(ap, target, hfcp, lun, HFC_ISSUE_ABORT) )
	{
		/* Timeout command remains in xob. */
		hfc_stra_trace(HFC_TRC_ABORT, 0x35, ap, target, hfcp, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED); ;
	}

	if ( !test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status ) ) {	/* FCLNX-GPL-035 */
		if ( hfc_issue_mihlog( ap, target, hfcp ) == 0)
		{
			/* Initiate MIH-LOG successfully */
			HFC_DBGPRT("eh_abort mihlog successed. lun = %d \n",lun);
			dev->lustat |= HFC_NEED_ABORT | HFC_DEFER_ABORT;

			hfc_stra_trace(HFC_TRC_ABORT, 0x70, ap, target, hfcp, (ulong)cmnd, 0, 0);
			goto WAIT_ABORT_TS;
		}
	}
	else {
		HFC_DBGPRT("skip eh_abort mihlog. lun = %d \n",lun);
	}																	/* FCLNX-GPL-035 */

	/* Initiate ABORT TASK SET */
	abort_pkt = hfc_get_new_hfcp(ap);
	if( abort_pkt == NULL)
	{
		/* Hfc_pkt is empty */
		HFC_DBGPRT(" hfcldd : hfc_eh_abort - hfcp==NULL-error\n");

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}
	
	dummy_cmnd = hfc_get_new_cmnd(ap);
	if( dummy_cmnd == NULL )
	{
		/* Dummy scsi_cmnd is empty */
		HFC_DBGPRT(" hfcldd : hfc_eh_abort - dummy_cmnd==NULL-error\n");

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}
	
	memset(abort_pkt, 0, sizeof(struct hfc_pkt));
//	memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));				/* FCLNX-GPL-0343 */
	memset(dummy_cmnd->cmnd, 0, 16 );								/* FCLNX-GPL-0343 */

	set_bit(CFLAG_VALID, (ulong *)&abort_pkt->cmd_flags);
	set_bit(CFLAG_ABORT, (ulong *)&abort_pkt->cmd_flags);
	ap->pkt_no++;
	if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
	ap->pkt_cnt++;

	abort_pkt->cmd_pkt   = dummy_cmnd;
	abort_pkt->target_id = target->target_id;
	abort_pkt->lun_id    = CMND_LUN(cmnd);
	abort_pkt->ap        = ap;
	abort_pkt->target    = target;
	abort_pkt->dev       = NULL;
	dummy_cmnd->host_scribble = (void *)abort_pkt;
	hfc_dummy_copy(ap, cmnd, dummy_cmnd );
	ap->cmnd_no++;
	if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
	ap->cmnd_cnt++;

	hfc_enqueue_wx_que(target, abort_pkt);
	
	if ( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status ) ) {			/* FCLNX-GPL-0153 */
		HFC_DBGPRT(" hfcldd : hfc_eh_abort - mck is progress\n");
		dev->lustat |= HFC_NEED_ABORT;
		
		hfc_stra_trace(HFC_TRC_ABORT, 0x75, ap, target, abort_pkt, (ulong)dummy_cmnd, 0, 0);
		goto WAIT_ABORT_TS;
	}																	/* FCLNX-GPL-0153 */
	
	/* Issue SCSI command if target is normal */
	dev->lustat |= HFC_NEED_ABORT;								/* FCLNX-GPL-289  */

	if(hfc_start( ap, target, abort_pkt)== TRUE)
	{
//		target->lustat[lun] &= ~HFC_NEED_ABORT;							/* FCLNX-GPL-289  */
//		target->lustat[lun] |= HFC_WAIT_ABORT;							/* FCLNX-GPL-289  */
		hfc_stra_trace(HFC_TRC_ABORT, 0x73, ap, target, abort_pkt, (ulong)dummy_cmnd, 0, 0);
	}
	else
	{	/* Failed to issue Abort Task Set because of resource busy */
		dev->lustat |= HFC_NEED_ABORT;

		hfc_stra_trace(HFC_TRC_ABORT, 0x74, ap, target, abort_pkt, (ulong)dummy_cmnd, 0, 0);
	}
	
WAIT_ABORT_TS:

	HFC_DBGPRT("hfc_eh_abort : wait abort task set complete \n");

	do {
		find = FALSE;
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(cmnd->device->host->host_lock);
		HFC_ADAPLOCK_IRQSAVE(flags);

		if ( (dev->lustat & HFC_NEED_ABORT)
		  || (dev->lustat & HFC_WAIT_ABORT) )
			find = TRUE;

	} while (find != FALSE);

	hfc_stra_trace(HFC_TRC_ABORT ,0x10 ,ap ,target ,abort_pkt, (ulong)dummy_cmnd, 0, 0);
	HFC_DBGPRT("hfcldd : hfc_eh_abort - end\n");
	if( (test_bit(HFC_NEED_LOGIN, (ulong *)&target->status ))||
		(test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status ))||
		(test_bit(HFC_NEED_CANCEL, (ulong *)&target->status ))||		/* FCLNX-GPL-038 */
		(test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status )))			/* FCLNX-GPL-038 */
	{
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
	return(SUCCESS);
//	return(FAILED); /* Error injection */
}    

/*
 * Function:    hfc_eh_device_reset_pg
 *
 * Purpose:     
 *		RHEL6 : Issue LUN Reset to reset the specified LUN.
 *		RHEL5 : Issue Target Reset to reset the specified target.
 *
 * Arguments:   
 *  cmnd        Pointer to Scsi_Cmnd
 *
 * Returns:     
 *  SUCCESS     Start succeeded
 *  FAILED      Start failed *
 * Notes:       
 */
int hfc_eh_device_reset_pg(struct scsi_cmnd *cmnd)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
	struct	adap_info	*ap ;
	struct	target_info	*target ;
	struct hfc_pkt		*reset_pkt=NULL;
	unsigned long		flags = 0;										/* FCLNX-0274 */
	struct scsi_cmnd 	*dummy_cmnd=NULL;
#endif
	struct	adap_info	*ap ;
	int                 rtn;

	/*-- check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		/*-- invalid argument --*/
		HFC_DBGPRT("hfcldd : hfc_device_reset - invalid argument \n");
		return(FAILED);
    }
	
	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);
	
	if (ap != NULL) {
		if (!strcmp(ap->name, "port_info")) {
			/* FIVE-FX */
			return (hfc_fx_eh_device_reset_pg(cmnd));
		}
	}
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
	rtn = hfc_lun_reset(cmnd);
	return(rtn);
#else													/* FCLNX-GPL-0343 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_lock_irq(cmnd->device->host->host_lock);
#endif

	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);

	HFC_DBGPRT(" hfcldd%d : hfc_eh_device_reset - start channel=%d, tid=%d. \n",
						ap->dev_minor, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd));

	if( ap == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_device_reset - adap_info null\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }

	HFC_ADAPLOCK_IRQSAVE(flags);
	hfc_stra_trace(HFC_TRC_RESET ,0x00 ,ap ,NULL , NULL, (ulong)cmnd, 0, 0);

	/* Check scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= ap->max_target)
	  || (CMND_TARGET(cmnd) == ap->hosts->this_id) )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_device_reset - invalid parameter\n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}


	if ( !test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_eh_device_reset - not attach status \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }
    
	target = hfc_hash_target_info(ap, CMND_TARGET(cmnd));

	if ( !ap->enable_tgtrst )
	{	/* LOGIN Mode */
		if( ( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ) )
			|| ( !test_bit( HFC_ONLINE, (ulong *)&ap->status ) )
			|| ( test_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status ) )
			|| ( test_bit( HFC_ISOL, (ulong *)&ap->status ) ) ) /* FCLNX-GPL-572 */
		{
			/* Executing machine check recovery */
			HFC_DBGPRT("hfcldd : hfc_eh_device_reset - mck recovery / link down \n");
			hfc_stra_trace(HFC_TRC_RESET,0x20, ap, target, NULL, (ulong)cmnd, 0, 0);
			
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			spin_unlock_irq(cmnd->device->host->host_lock);
#endif
			return(SUCCESS);
		}
		if(!hfc_mlpf_check_normal_hypsts(ap)){							/* FCLNX-GPL-428 */
			/* Executing machine check recovery */
			HFC_DBGPRT("hfcldd : hfc_eh_device_reset - mck recovery / link down in MLPF Shared Mode.\n");
			hfc_stra_trace(HFC_TRC_RESET,0x22, ap, target, NULL, (ulong)cmnd, 0, 0);

			HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			spin_unlock_irq(cmnd->device->host->host_lock);
#endif
			return(SUCCESS);
		}																/* FCLNX-GPL-428 */
	}
	else 
	{	/* Target Reset Mode */
		if( !test_bit( HFC_ONLINE, (ulong *)&ap->status )
		||   test_bit( HFC_ISOL, (ulong *)&ap->status ) )	/* FCLNX-GPL-572 */
		{
			/* Executing machine check recovery */
			HFC_DBGPRT("hfcldd : hfc_eh_device_reset - Link Down \n");
			hfc_stra_trace(HFC_TRC_RESET,0x21, ap, target, NULL, (ulong)cmnd, 0, 0);
			
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			spin_unlock_irq(cmnd->device->host->host_lock);
#endif
			return(SUCCESS);											/* FCLNX-GPL-469 */
		}
	}
	
	if( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ) )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_device_reset - already reset bus \n");

		hfc_stra_trace(HFC_TRC_RESET ,0x30 , ap, target, NULL, (ulong)cmnd, 0, 0) ;
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}
	

	if (target == NULL)
	{
		HFC_DBGPRT("hfcldd : hfc_eh_device_reset - target_info null\n");
		hfc_stra_trace(HFC_TRC_RESET ,0x31 ,ap , target, NULL, (ulong)cmnd, 0, 0);

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}
	
	if ( test_bit(HFC_WAIT_TGTRSP, (ulong *)&target->target_reset) )	/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
	{
		HFC_DBGPRT("hfcldd : hfc_eh_device_reset - in target reset prog\n");

		hfc_stra_trace(HFC_TRC_RESET ,0x34 ,ap , target, NULL, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}
	
	set_bit( HFC_WAIT_TGTRSP, (ulong *)&target->target_reset );			/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
	
	/* Cancel wait queue of SCSI command and Task Management */
	hfc_cancel_scsi_cmd(ap,target,0,NULL,SCS_WAIT_BUS_RESET, HFC_CSCSI_RESET,
			FALSE,TRUE, HFC_FLASH_TARGET );

	if ( !ap->enable_tgtrst )
	{
		hfc_stra_trace(HFC_TRC_RESET, 0x76, ap, target, NULL, (ulong)cmnd, 0, 0);

		if ( !test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )
		{
		
			if( (test_bit(HFC_SCN_WLINKUP,(ulong *)&target->status))
			  &&(!test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status) ) ){			/* FCLNX-GPL-038 */
				set_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
				clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);				/* FCLNX-GPL-038 */
			}

			if ( hfc_issue_relogin(ap,target) )
			{
				HFC_DBGPRT("hfcldd : hfc_eh_device_reset - wait issue LOGIN \n");
				set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );	/* FCLNX-GPL-038 */ /* FCLNX-GPL-197 */
				hfc_enque_login_req(ap, target);
		
				hfc_stra_trace(HFC_TRC_RESET, 0x77, ap, target, NULL, (ulong)cmnd, 0, 0);
			}
		}
	}
	else
	{
		reset_pkt = hfc_get_new_hfcp(ap);
		if( reset_pkt == NULL )
		{
			/* No reserved space for hfc_pkt */
			HFC_DBGPRT(" hfcldd : hfc_device_reset - hfcp==NULL-error\n");
			
			set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-036 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);	/* FCLNX-GPL-036 */	/* FCLNX-GPL-328 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);					/* FCLNX-GPL-328 */
			goto WAIT_DEV_RST;
		}
		
		dummy_cmnd = hfc_get_new_cmnd(ap);
		if( dummy_cmnd == NULL)
		{
			/* No reserved space for scsi_cmnd structure */
			HFC_DBGPRT(" hfcldd : hfc_device_reset - dummy_cmnd==NULL-error\n");
			
			set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-036 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);	/* FCLNX-GPL-036 */
			hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);
			goto WAIT_DEV_RST;
		}

		memset(reset_pkt, 0, sizeof(struct hfc_pkt));
		memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));

		set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
		set_bit(CFLAG_TARGET_RESET, (ulong *)&reset_pkt->cmd_flags);
		ap->pkt_no++;
		if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
		ap->pkt_cnt++;

		reset_pkt->cmd_pkt   = dummy_cmnd;
		reset_pkt->target_id = target->target_id;
		reset_pkt->lun_id    = 0;
		reset_pkt->ap        = ap;
		reset_pkt->target    = target;
		reset_pkt->dev       = NULL;
		dummy_cmnd->host_scribble = (void *)reset_pkt;
		hfc_dummy_copy(ap, cmnd, dummy_cmnd );
		ap->cmnd_no++;
		if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
		ap->cmnd_cnt++;

		hfc_enqueue_wx_que(target, reset_pkt);
		
		set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );				/* FCLNX-GPL-0155 */
		
		if ( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status )
		  || ( HFC_MMODE_CHECK_SHARED(ap) && test_bit( HFC_WAIT_T3, (ulong *)&ap->status )) ) 
		{						/* FCLNX-GPL-0153 */ /* FCLNX-GPL-0320 */
			HFC_DBGPRT(" hfcldd : hfc_device_reset - mck is progress\n");
			if(target->next_dstart_flag == 0)											/* FCLNX-GPL-0155 */
			{
				hfc_enque_next_dstart(ap,target);
			}																			/* FCLNX-GPL-0155 */
			hfc_stra_trace(HFC_TRC_RESET, 0x78, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
			goto WAIT_DEV_RST;
		}																				/* FCLNX-GPL-0153 */
		
		/* Initiate Target Reset */
		if( hfc_start( ap, target, reset_pkt) == TRUE )
		{
//			clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-036 */	/* FCLNX-GPL-289 */
//			set_bit( HFC_WAIT_TARGET_RESET, (ulong *)&target->status );										/* FCLNX-GPL-289 */
			hfc_stra_trace(HFC_TRC_RESET, 0x73, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
		}
		else {																			/* FCLNX-GPL-0155 */
			if(target->next_dstart_flag == 0)
			{
				hfc_enque_next_dstart(ap,target);
			}
		}																				/* FCLNX-GPL-0155 */
	}


WAIT_DEV_RST :
	HFC_DBGPRT("hfc_eh_device_reset : wait LOGIN complete \n");

	while (1) {
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(cmnd->device->host->host_lock);
		HFC_ADAPLOCK_IRQSAVE(flags);

		if ( !test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_WWN_VALID, (ulong *)&target->flags)  )
		{
			rtn = SUCCESS;							/* FCLNX-0500 */
			break;
		}

		if ( !test_bit(HFC_NEED_LOGIN, (ulong *)&target->status)
		 &&  !test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) 
		 &&  !test_bit(HFC_NEED_CANCEL, (ulong *)&target->status)		/* FCLNX-GPL-038 */
		 &&  !test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status) )		/* FCLNX-GPL-038 */
		{
			if (!ap->enable_tgtrst)
			{
				rtn = SUCCESS;
				break;
			}
			else
			{
				if ( !test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset)		/* FCLNX-GPL-036 */
				 &&  !test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status) ) 
				{
					rtn = SUCCESS;
					break;
				}
			}
		}
	}

	clear_bit(HFC_WAIT_TGTRSP, (ulong *)&target->target_reset);			/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
	HFC_DBGPRT("hfcldd : hfc_eh_device_reset - end \n");
	
	hfc_stra_trace(HFC_TRC_RESET, 0x10, ap, target, NULL, (ulong)cmnd, 0, 0);
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_unlock_irq(cmnd->device->host->host_lock);
#endif
	return(rtn);
#endif
}    

/*
 * Function:    hfc_lun_reset
 *
 * Purpose:     Issue LUN_RESET to reset the specified lun.
 *
 * Arguments:   
 *  cmnd        Pointer to Scsi_Cmnd
 *
 * Returns:     
 *  SUCCESS     Start succeeded
 *  FAILED      Start failed *
 * Notes:       
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
int hfc_lun_reset(struct scsi_cmnd *cmnd)
{
	struct	adap_info	*ap ;
	struct	target_info	*target ;
	struct hfc_pkt		*reset_pkt=NULL;
	int                 rtn;
	unsigned long		flags = 0;
	struct scsi_cmnd 	*dummy_cmnd=NULL;
	struct Scsi_Host	*host;
	uint				lun_id;
	struct dev_info		*dev=NULL;		/* FCLNX-GPL-0343 */
	ushort				cmd_lun;		/* FCLNX-GPL-0548 */

	/*-- check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		/*-- invalid argument --*/
		HFC_DBGPRT("hfcldd : hfc_lun_reset - invalid argument \n");
		return(FAILED);
    }
	
	if( cmnd->device == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - scsi_device null \n");
		return(FAILED);
	}
	
	host = cmnd->device->host;
	
	if( host == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - Scsi_Host null \n");
		return(FAILED);
	}
	
	dev = (struct dev_info *) CMND_DEV(cmnd);				/* FCLNX-GPL-0343 */
	if( dev == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - dev_info null \n");
		return(FAILED);
	}														/* FCLNX-GPL-0343 */
	
	cmd_lun = (ushort)CMND_LUN(cmnd);				/* FCLNX-GPL-0548 */
    cmd_lun = (cmd_lun & 0x3fff);					/* FCLNX-GPL-0548 */

	spin_lock_irq(cmnd->device->host->host_lock);

	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);

	HFC_DBGPRT(" hfcldd%d : hfc_lun_reset - start rid=0x%02x, channel=%d, tid=%d, lun=%d. \n",
						ap->dev_minor, ap->rid, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd), CMND_LUN(cmnd));

	if( ap == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - ap null\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }

	HFC_ADAPLOCK_IRQSAVE(flags);
	hfc_stra_trace(HFC_TRC_LUN_RESET ,0x00 ,ap ,NULL , NULL, (ulong)cmnd, 0, 0);

	/* Check scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= ap->max_target)
	  || (CMND_TARGET(cmnd) == ap->hosts->this_id)
	  || (cmd_lun >= MAX_DEV_CNT) )					/* FCLNX-GPL-0548 */
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - scsi_cmnd invalid\n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	lun_id = CMND_LUN(cmnd);
	
	if ( !test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_lun_reset - not attach status \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }
    
	target = hfc_hash_target_info(ap, CMND_TARGET(cmnd));

	/* Target Reset Mode */
	if( !test_bit( HFC_ONLINE, (ulong *)&ap->status )
	||   test_bit( HFC_ISOL, (ulong *)&ap->status ) )	/* FCLNX-GPL-572 */
	{
		/* Executing machine check recovery */
		HFC_DBGPRT("hfcldd : hfc_lun_reset - Link Down \n");
		hfc_stra_trace(HFC_TRC_LUN_RESET, 0x21, ap, NULL, NULL, (ulong)cmnd, 0, 0);
			
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);											/* FCLNX-GPL-469 */
	}
	
	if( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ) )
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - bus reset is progress\n");

		hfc_stra_trace(HFC_TRC_LUN_RESET ,0x30 , ap, NULL, NULL, (ulong)cmnd, 0, 0) ;
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	

	if (target == NULL)
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - target null\n");
		hfc_stra_trace(HFC_TRC_LUN_RESET ,0x31 ,ap , NULL, NULL, (ulong)cmnd, 0, 0);

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}
	
	if ( test_bit(HFC_WAIT_TGTRSP, (ulong *)&target->target_reset) )			/* FCLNX_GPL-0050 */
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - device reset is progress\n");

		hfc_stra_trace(HFC_TRC_LUN_RESET ,0x34 ,ap , target, NULL, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}

	if (dev->lustat & HFC_WAIT_LUNRSP)
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - lun reset is in progress\n");

		hfc_stra_trace(HFC_TRC_LUN_RESET ,0x35 ,ap , target, NULL, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}
	
	reset_pkt = hfc_get_new_hfcp(ap);
	if( reset_pkt == NULL )
	{
		/* No reserved space for hfc_pkt */
		HFC_DBGPRT(" hfcldd : hfc_lun_reset - hfcp==NULL-error\n");
		
		hfc_stra_trace(HFC_TRC_LUN_RESET ,0x36 ,ap , target, NULL, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	dummy_cmnd = hfc_get_new_cmnd(ap);
	if( dummy_cmnd == NULL)
	{
		/* No reserved space for scsi_cmnd structure */
		HFC_DBGPRT(" hfcldd : hfc_lun_reset - dummy_cmnd==NULL-error\n");
		
		hfc_stra_trace(HFC_TRC_LUN_RESET ,0x37 ,ap , target, NULL, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	dev->lustat |= HFC_WAIT_LUNRSP;
	
	/* Cancel wait queue of SCSI command and Task Management */
	hfc_cancel_scsi_cmd(ap,target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, TRUE,
			FALSE,TRUE, HFC_FLASH_DEV );
	
	memset(reset_pkt, 0, sizeof(struct hfc_pkt));
//	memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));				/* FCLNX-GPL-0343 */
	memset(dummy_cmnd->cmnd, 0, 16 );								/* FCLNX-GPL-0343 */

	set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
	set_bit(CFLAG_LUN_RESET, (ulong *)&reset_pkt->cmd_flags);
	ap->pkt_no++;
	if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
	ap->pkt_cnt++;

	reset_pkt->cmd_pkt   = dummy_cmnd;
	reset_pkt->target_id = CMND_TARGET(cmnd);
	reset_pkt->lun_id    = CMND_LUN(cmnd);
	reset_pkt->ap        = ap;
	reset_pkt->target    = target;
	reset_pkt->dev       = dev;
	dummy_cmnd->host_scribble = (void *)reset_pkt;
	hfc_dummy_copy(ap, cmnd, dummy_cmnd );
	ap->cmnd_no++;
	if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
	ap->cmnd_cnt++;

	hfc_enqueue_wx_que(target, reset_pkt);
	
	dev->lustat |= HFC_NEED_LUN_RESET;
	
	if ( ( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status) )
	  || ( test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status) )
	  || ( target->status != HFC_NON_STATUS ) ) {
		HFC_DBGPRT(" hfcldd : hfc_lun_reset - only enque to wx_que\n");
		if(target->next_dstart_flag == 0)
		{
			hfc_enque_next_dstart(ap,target);
		}
		hfc_stra_trace(HFC_TRC_LUN_RESET, 0x78, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
		goto WAIT_LUN_RST;
	}
	
	/* Initiate Lun Reset */
	if( hfc_start( ap, target, reset_pkt) == TRUE )
	{
		hfc_stra_trace(HFC_TRC_LUN_RESET, 0x73, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
	}
	else {
		if(target->next_dstart_flag == 0)
		{
			hfc_enque_next_dstart(ap,target);
		}
	}


WAIT_LUN_RST :

	while (1) {
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(host->host_lock);
		HFC_ADAPLOCK_IRQSAVE(flags);

		if ( !test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_WWN_VALID, (ulong *)&target->flags)  )
		{
			rtn = SUCCESS;
			break;
		}

		if ( !test_bit(HFC_NEED_LOGIN, (ulong *)&target->status)
		 &&  !test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) )
		{
			if ( !test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset)
		 	&&  !test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status) )
			{
				if ( !( dev->lustat & HFC_NEED_LUN_RESET )
				 &&  !( dev->lustat & HFC_WAIT_LUN_RESET ) ) 
				{
					rtn = SUCCESS;
					break;
				}
			}
		}
	}

	dev->lustat &= ~HFC_WAIT_LUNRSP;
	HFC_DBGPRT("hfcldd : hfc_lun_reset - end \n");
	
	hfc_stra_trace(HFC_TRC_LUN_RESET, 0x10, ap, target, NULL, 0, 0, 0);
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	spin_unlock_irq(host->host_lock);
	return(rtn);
}
#endif
/* FCLNX-GPL-0343 */

/* FCLNX-GPL-0343 */
/*
 * Function:    hfc_eh_target_reset_pg
 *
 * Purpose:     Issue Target Reset to reset the specified target.
 *
 * Arguments:   
 *  cmnd        Pointer to Scsi_Cmnd
 *
 * Returns:     
 *  SUCCESS     Start succeeded
 *  FAILED      Start failed *
 * Notes:       
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
int hfc_eh_target_reset_pg(struct scsi_cmnd *cmnd)
{
	struct	adap_info	*ap ;
	struct	target_info	*target ;
	struct hfc_pkt		*reset_pkt=NULL;
	int                 rtn;
	unsigned long		flags = 0;	
	struct scsi_cmnd 	*dummy_cmnd=NULL;
	struct Scsi_Host	*host=NULL;			/* FCLNX-GPL-0343 */
	struct dev_info		*dev;

	/*-- check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		/*-- invalid argument --*/
		HFC_DBGPRT("hfcldd : hfc_target_reset - invalid argument \n");
		return(FAILED);
    }
    
    host = cmnd->device->host;								/* FCLNX-GPL-0343 */
	
	if( host == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_lun_reset - Scsi_Host null \n");
		return(FAILED);
	}														/* FCLNX-GPL-0343 */
	
	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);
	
	if (ap != NULL) {
		if (!strcmp(ap->name, "port_info")) {
			/* FIVE-FX */
			return (hfc_fx_eh_target_reset_pg(cmnd));
		}
	}
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_lock_irq(cmnd->device->host->host_lock);
#endif

//	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);

	HFC_DBGPRT(" hfcldd%d : hfc_eh_target_reset - start channel=%d, tid=%d. \n",
						ap->dev_minor, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd));

	if( ap == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - 1\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }

	HFC_ADAPLOCK_IRQSAVE(flags);
	hfc_stra_trace(HFC_TRC_TGT_RESET ,0x00 ,ap ,NULL , NULL, (ulong)cmnd, 0, 0);

	/* Check scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= ap->max_target)
	  || (CMND_TARGET(cmnd) == ap->hosts->this_id) )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - 3\n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}


	if ( !test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - not attach status \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }
    
	target = hfc_hash_target_info(ap, CMND_TARGET(cmnd));

	/* Target Reset Mode */
	if( !test_bit( HFC_ONLINE, (ulong *)&ap->status )
	||   test_bit( HFC_ISOL, (ulong *)&ap->status ) )	/* FCLNX-GPL-572 */
	{
		/* Executing machine check recovery */
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - Link Down \n");
		hfc_stra_trace(HFC_TRC_TGT_RESET,0x21, ap, target, NULL, (ulong)cmnd, 0, 0);
			
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);											/* FCLNX-GPL-469 */
	}
	
	if( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ) )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - already reset bus \n");

		hfc_stra_trace(HFC_TRC_TGT_RESET ,0x30 , ap, target, NULL, (ulong)cmnd, 0, 0) ;
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}
	

	if (target == NULL)
	{
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - 10\n");
		hfc_stra_trace(HFC_TRC_TGT_RESET ,0x31 ,ap , target, NULL, (ulong)cmnd, 0, 0);

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}

	if ( test_bit(HFC_WAIT_TGTRSP, (ulong *)&target->target_reset) )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_target_reset - 13\n");

		hfc_stra_trace(HFC_TRC_TGT_RESET ,0x34 ,ap , target, NULL, (ulong)cmnd, 0, 0);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}
	
	set_bit( HFC_WAIT_TGTRSP, (ulong *)&target->target_reset );	
	
	/* Cancel wait queue of SCSI command and Task Management */
	hfc_cancel_scsi_cmd(ap,target,0,NULL,SCS_WAIT_BUS_RESET, HFC_CSCSI_RESET,
			FALSE,TRUE, HFC_FLASH_TARGET );

	reset_pkt = hfc_get_new_hfcp(ap);
	if( reset_pkt == NULL )
	{
		/* No reserved space for hfc_pkt */
		HFC_DBGPRT(" hfcldd : hfc_target_reset - hfcp==NULL-error\n");
			
		set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );
		hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);
		hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);
		goto WAIT_DEV_RST;
	}
		
	dummy_cmnd = hfc_get_new_cmnd(ap);
	if( dummy_cmnd == NULL)
	{
		/* No reserved space for scsi_cmnd structure */
		HFC_DBGPRT(" hfcldd : hfc_target_reset - dummy_cmnd==NULL-error\n");

		set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );
		hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);	
		hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);
		goto WAIT_DEV_RST;
	}

	memset(reset_pkt, 0, sizeof(struct hfc_pkt));
//	memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));		/* FCLNX-GPL-0343 */
	memset(dummy_cmnd->cmnd, 0, 16 );						/* FCLNX-GPL-0343 */

	dev = (struct dev_info *) CMND_DEV(cmnd);				/* FCLNX-GPL-0343 */

	set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
	set_bit(CFLAG_TARGET_RESET, (ulong *)&reset_pkt->cmd_flags);
	ap->pkt_no++;
	if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
	ap->pkt_cnt++;

	reset_pkt->cmd_pkt   = dummy_cmnd;
	reset_pkt->target_id = target->target_id;
	reset_pkt->lun_id    = 0;
	reset_pkt->ap        = ap;
	reset_pkt->target    = target;
	reset_pkt->dev       = dev;
	dummy_cmnd->host_scribble = (void *)reset_pkt;
	hfc_dummy_copy(ap, cmnd, dummy_cmnd );
	ap->cmnd_no++;
	if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
	ap->cmnd_cnt++;

	hfc_enqueue_wx_que(target, reset_pkt);

	set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );

	if ( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status )
	  || ( HFC_MMODE_CHECK_SHARED(ap) && test_bit( HFC_WAIT_T3, (ulong *)&ap->status )) ) 
	{
		HFC_DBGPRT(" hfcldd : hfc_target_reset - mck is progress\n");
		if(target->next_dstart_flag == 0)
		{
			hfc_enque_next_dstart(ap,target);
		}
		hfc_stra_trace(HFC_TRC_TGT_RESET, 0x78, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
		goto WAIT_DEV_RST;
	}
	
	/* Initiate Target Reset */
	if( hfc_start( ap, target, reset_pkt) == TRUE )
	{
//		clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );	
//		set_bit( HFC_WAIT_TARGET_RESET, (ulong *)&target->status );	
		hfc_stra_trace(HFC_TRC_TGT_RESET, 0x73, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
	}
	else {
		if(target->next_dstart_flag == 0)
		{
			hfc_enque_next_dstart(ap,target);
		}
	}


WAIT_DEV_RST :
	HFC_DBGPRT("hfc_eh_target_reset : wait LOGIN complete \n");

	while (1) {
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(cmnd->device->host->host_lock);
		HFC_ADAPLOCK_IRQSAVE(flags);

		if ( !test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_WWN_VALID, (ulong *)&target->flags)  )
		{
			rtn = SUCCESS;	
			break;
		}

		if ( !test_bit(HFC_NEED_LOGIN, (ulong *)&target->status)
		 &&  !test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status) 
		 &&  !test_bit(HFC_NEED_CANCEL, (ulong *)&target->status)	
		 &&  !test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status) )	
		{
			if ( !test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset)
			   &&  !test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status) ) 
			{
				rtn = SUCCESS;
				break;
			}
		}
	}

	clear_bit(HFC_WAIT_TGTRSP, (ulong *)&target->target_reset);	
	HFC_DBGPRT("hfcldd : hfc_eh_target_reset - end \n");
	
	hfc_stra_trace(HFC_TRC_TGT_RESET, 0x10, ap, target, NULL, (ulong)cmnd, 0, 0);
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_unlock_irq(cmnd->device->host->host_lock);
#endif
	return(rtn);
}
#endif    
/* FCLNX-GPL-0343 */

/*
 * Function:    hfc_eh_bus_reset_pg
 *
 * Purpose:     Issue LOGIN to reset all targets 
 *
 * Arguments:   
 *  cmnd        Pointer to Scsi_Cmnd
 *
 * Returns:     
 *  SUCCESS     Start succeeded
 *  FAILED      Start failed 
 * Notes:       
 */
int hfc_eh_bus_reset_pg(struct scsi_cmnd *cmnd)
{
	struct adap_info	*ap ;
	struct target_info	*target, *tgt=NULL ;
	struct hfc_pkt		*reset_pkt = NULL;
	int					lp;
	unsigned long		flags = 0;										/* FCLNX-0274 */
	uchar bus_reset_imcomplete = FALSE;
	uchar				find;
	struct scsi_cmnd 	*dummy_cmnd=NULL;
	struct dev_info		*dev;

	HFC_DBGPRT(" hfcldd : hfc_eh_bus_reset - start channel=%d. \n" ,CMND_CHANNEL(cmnd));

	/*-- Check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - invalid argument \n");
		return(FAILED);
    }
	
	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);
	
	if (ap != NULL) {
		if (!strcmp(ap->name, "port_info")) {
			/* FIVE-FX */
			return (hfc_fx_eh_bus_reset_pg(cmnd));
		}
	}
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_lock_irq(cmnd->device->host->host_lock);
#endif

//	ap = (struct adap_info *) CMND_HOSTDATA(cmnd);

	if ( ap == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - invalid adap_info pointer \n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}

    HFC_ADAPLOCK_IRQSAVE(flags);

	hfc_stra_trace(HFC_TRC_BUS_RESET ,0x00 ,ap ,NULL , NULL, (ulong)cmnd, 0, 0);

	if (CMND_CHANNEL(cmnd) != 0)
	{
		/* Channel number is invalid */
		HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - invalid channel number \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
	}
	
	if( !test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - not attach status \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(FAILED);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	if( !test_bit( HFC_ONLINE, (ulong *)&ap->status ) )
	{
		HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - Link Down in Target Reset \n");
		hfc_stra_trace(HFC_TRC_BUS_RESET,0x32, ap,NULL, NULL, (ulong)cmnd, 0, 0);

		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
#else
	if ( !ap->enable_tgtrst )
	{
		if( ( test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ) )
			|| ( !test_bit( HFC_ONLINE, (ulong *)&ap->status ) )
			|| ( test_bit( HFC_WAIT_LINKUP, (ulong *)&ap->status ) )
			|| ( test_bit( HFC_ISOL, (ulong *)&ap->status ) ) ) /* FCLNX-GPL-572 */
		{
			HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - Link Down \n");
			hfc_stra_trace(HFC_TRC_BUS_RESET,0x31, ap,NULL, NULL, (ulong)cmnd, 0, 0);

			HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			spin_unlock_irq(cmnd->device->host->host_lock);
#endif
			return(SUCCESS);
    	}
    	if(!hfc_mlpf_check_normal_hypsts(ap)){							/* FCLNX-GPL-428 */
			HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - Link Down in MLPF Shared Mode.\n");
			hfc_stra_trace(HFC_TRC_BUS_RESET,0x33, ap,NULL, NULL, (ulong)cmnd, 0, 0);

			HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			spin_unlock_irq(cmnd->device->host->host_lock);
#endif
			return(SUCCESS);
		}																/* FCLNX-GPL-428 */
	}
	else {
		if( !test_bit( HFC_ONLINE, (ulong *)&ap->status )
		||   test_bit( HFC_ISOL, (ulong *)&ap->status ) )		/* FCLNX-GPL-572 */
		{
			HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - Link Down in Target Reset \n");
			hfc_stra_trace(HFC_TRC_BUS_RESET,0x32, ap,NULL, NULL, (ulong)cmnd, 0, 0);
			
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			spin_unlock_irq(cmnd->device->host->host_lock);
#endif
			return(SUCCESS);											/* FCLNX-GPL-469 */
		}
	}
#endif

	if( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ) )
	{
		/* Already issued BUS RESET */
		HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - already reset bus \n");
		hfc_stra_trace(HFC_TRC_BUS_RESET ,0x30 ,ap , NULL, NULL, (ulong)cmnd, 0, 0) ;
		
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
		return(SUCCESS);
	}

	/* Start BUS RESET */
	HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - start reset bus \n");
	set_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status );		/* Set wait bus reset flag */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	if( ap->enable_tgtrst )
	{	/* Target Reset effective mode unsupport */
#endif
		for ( lp=0; lp<MAX_TARGET_PROBE; lp++ )
		{
			if ( (target = hfc_hash_target_info(ap, lp)) != NULL )
			{
				/* 
				 * (1) Cancel waiting status of SCSI initiation.
				 * (2) Cancel waiting status of SCSI response when LOGIN completes 
				 */
				/* Cancel waiting queue of SCSI initialization and Task Management */
				hfc_cancel_scsi_cmd(ap,target,0,NULL,SCS_WAIT_BUS_RESET, HFC_CSCSI_RESET,
					FALSE,TRUE, HFC_FLASH_TARGET );
				
				reset_pkt = hfc_get_new_hfcp(ap);
				if( reset_pkt == NULL )
				{
				/* No free space for hfc_pkt */
					HFC_DBGPRT(" hfcldd : hfc_device_reset - hfcp==NULL-error\n");
					
					set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
					hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, 0, TRUE);		/* FCLNX-GPL-036 */
					hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, 0, FALSE);
					goto WAIT_BUS_RST;
				}
				
				dummy_cmnd = hfc_get_new_cmnd(ap);
				if( dummy_cmnd == NULL)
				{
					/* No space for dummy scsi_cmnd */
					HFC_DBGPRT(" hfcldd : hfc_device_reset - dummy_cmnd==NULL-error\n");
					
					set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
					hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, 0, TRUE);		/* FCLNX-GPL-036 */
					hfc_watchdog_enter(ap, target, NULL, 0, HFC_RESTART_TMR, 0, FALSE);
					goto WAIT_BUS_RST;
				}
				
				memset(reset_pkt, 0, sizeof(struct hfc_pkt));
//				memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));		/* FCLNX-GPL-0343 */
				memset(dummy_cmnd->cmnd, 0, 16 );						/* FCLNX-GPL-0343 */
				
				set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
				set_bit(CFLAG_BUS_RESET, (ulong *)&reset_pkt->cmd_flags);
				ap->pkt_no++;
				if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
				ap->pkt_cnt++;
				
				dev = (struct dev_info *) CMND_DEV(cmnd);				/* FCLNX-GPL-0343 */
				reset_pkt->cmd_pkt   = dummy_cmnd;
				reset_pkt->target_id = target->target_id;
				reset_pkt->lun_id    = 0;
				reset_pkt->ap        = ap;
				reset_pkt->target    = target;
				reset_pkt->dev       = dev;
				dummy_cmnd->host_scribble = (void *)reset_pkt;
				hfc_dummy_copy(ap, cmnd, dummy_cmnd );
				ap->cmnd_no++;
				if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
				ap->cmnd_cnt++;
				
				hfc_enqueue_wx_que(target, reset_pkt);
				
				set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );				/* FCLNX-GPL-0156 */
				
				if ( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status )
				  || ( HFC_MMODE_CHECK_SHARED(ap) && test_bit( HFC_WAIT_T3, (ulong *)&ap->status )) ) 
				{						/* FCLNX-GPL-0153 */ /* FCLNX-GPL-0320 */
					HFC_DBGPRT(" hfcldd : hfc_bus_reset - mck is progress\n");
					if(target->next_dstart_flag == 0)											/* FCLNX-GPL-0156 */
					{
						hfc_enque_next_dstart(ap,target);
					}																			/* FCLNX-GPL-0156 */
					
					hfc_stra_trace(HFC_TRC_BUS_RESET, 0x75, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
					goto WAIT_BUS_RST;
				}																				/* FCLNX-GPL-0153 */
				
				/* Start Target Reset */
				if( hfc_start( ap, target, reset_pkt) == TRUE )
				{
//					clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-036 */	/* FCLNX-GPL-289 */
//					set_bit( HFC_WAIT_TARGET_RESET, (ulong *)&target->status );										/* FCLNX-GPL-289 */
					hfc_stra_trace(HFC_TRC_BUS_RESET, 0x73, ap, target, reset_pkt, (ulong)dummy_cmnd, 0, 0);
				}
				else {																			/* FCLNX-GPL-0156 */
					if(target->next_dstart_flag == 0)
					{
						hfc_enque_next_dstart(ap,target);
					}
				}
WAIT_BUS_RST:
				bus_reset_imcomplete = TRUE;
			}
		}

		if (bus_reset_imcomplete != FALSE)
		{
			HFC_DBGPRT("hfc_eh_bus_reset : wait bus reset \n");
			
			do {
				find = FALSE;
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
				spin_unlock_irq(cmnd->device->host->host_lock);
	
				set_current_state(TASK_UNINTERRUPTIBLE);
				schedule_timeout(HZ/10+1);					/* Wait 100ms */

				spin_lock_irq(cmnd->device->host->host_lock);
				HFC_ADAPLOCK_IRQSAVE(flags);

				for (lp = 0; lp<MAX_TARGET_PROBE; lp++)	{
					if ((tgt = hfc_hash_target_info(ap, lp)) != NULL) {
						if (( test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&tgt->status) ) ||
							( test_bit(HFC_NEED_TARGET_RESET, (ulong *)&tgt->target_reset) )) {	/* FCLNX-GPL-0158 */
							find = TRUE;
						}
					}
				}
			
			} while (find != FALSE);
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	}
#endif

	/* Issue LOGIN */
	for ( lp=0; lp<MAX_TARGET_PROBE; lp++ )
	{
		if ( (target = hfc_hash_target_info(ap, lp)) != NULL )
		{
			/* 
			 * (1) Cancel waiting status of SCSI initiation.
			 * (2) Cancel waiting status of SCSI response when LOGIN completes 
			 */
			/* Cancel waiting queue of SCSI initialization and Task Management */
			
			hfc_cancel_scsi_cmd(ap,target,0,NULL,SCS_WAIT_BUS_RESET, HFC_CSCSI_RESET,
				FALSE,TRUE, HFC_FLASH_TARGET );
			
			set_bit( HFC_WAIT_BUS_RESET, (ulong *)&target->status );
			
			if( HFC_TG_STATUS_TEST( HFC_WAIT_LOGIN, target ) || HFC_TG_STATUS_TEST( HFC_WAIT_CANCEL, target ) )	/* FCLNX-GPL-038 */
			{	/* Already LOGIN waiting status -> Set bus reset flag */
				bus_reset_imcomplete = TRUE;
				continue;
			}
			
			if( test_bit(HFC_SCN_WLINKUP,(ulong *)&target->status) ){
				set_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
				clear_bit(HFC_NEED_LOGIN, (ulong *)&target->status);	/* FCLNX-GPL-038 */
			}
			
			if( hfc_issue_relogin(ap,target) )
			{
				set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );	/* FCLNX-GPL-038 *//* FCLNX-GPL-197 */
				hfc_enque_login_req(ap, target);
			}
			
			bus_reset_imcomplete = TRUE;
		}
	}
	
	if (bus_reset_imcomplete != FALSE)
	{
		HFC_DBGPRT("hfc_eh_bus_reset : wait bus reset \n");
	
		do {
			find = FALSE;
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			spin_unlock_irq(cmnd->device->host->host_lock);

			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ/10+1);					/* Wait 100ms */

			spin_lock_irq(cmnd->device->host->host_lock);
			HFC_ADAPLOCK_IRQSAVE(flags);

			for (lp = 0; lp<MAX_TARGET_PROBE; lp++)	{
				if ((tgt = hfc_hash_target_info(ap, lp)) != NULL) {
					if ( test_bit(HFC_WAIT_BUS_RESET, (ulong *)&tgt->status) )
						find = TRUE;
				}
			}
		
		}while (find != FALSE);
	}

	clear_bit(HFC_WAIT_BUSRSP, (ulong *)&ap->status);
	HFC_DBGPRT("hfcldd : hfc_eh_bus_reset - end\n");

	hfc_stra_trace(HFC_TRC_BUS_RESET ,0x10 ,ap ,NULL ,NULL, (ulong)cmnd, 0, 0 );
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	spin_unlock_irq(cmnd->device->host->host_lock);
#endif
	return(SUCCESS);

}   


/*
 * Function:    hfc_issue_task_mgm
 *
 * Purpose:     Issue Task Management command
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  rstp       - Pointer to rst_pkt
 *  lun        - Lun which issues Abort start
 *  mode       - HFC_ISSUE_ABORT 		: Issue ABORT
 *               HFC_ISSUE_TARGET_RESET : Issue TARGET RESET
 *
 * Returns:     
 *
 * Notes:       This function is called in the following timings.
 *              (1) Bus Reset is required 
 *              (2) Adapter is executing BUS RESET when target responds.
 *              (3) Adapter is executing BUS RESET when receive LOGIN response.
 *
 */
void hfc_issue_task_mgm(
	struct adap_info            *ap,
	struct target_info          *target,
	struct hfc_pkt              *hfcp,
	uint                        lun,
	uchar                       mode)
{
	struct scsi_cmnd            *cmnd=NULL, *dummy_cmnd;
	struct hfc_pkt              *abort_pkt,*reset_pkt;
	struct dev_info				*dev=NULL;	/* FCLNX-GPL-0343 */

	HFC_DBGPRT(" hfcldd : hfc_issue_task_mgm - start\n");
	
	hfc_stra_trace(
			HFC_TRC_ISSUE_TMGM, 0x00, ap, target, hfcp,
			 0, 0, 0);

	if( hfcp != NULL ){			/* FCLNX-GPL-328 */
		cmnd = hfcp->cmd_pkt;
		dev = hfcp->dev;		/* FCLNX-GPL-0343 */
	}

	if ( !( test_bit( HFC_ONLINE, (ulong *)&ap->status ) ) )
	{
		return;
	}

	if ( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status )
	  || ( HFC_MMODE_CHECK_SHARED(ap) && test_bit( HFC_WAIT_T3, (ulong *)&ap->status )) 
	  || test_bit(HFC_ISOL, (ulong *)&ap->status ) ) 
	{
		return;
	}	/* FCLNX-GPL-572 */

	if ( test_bit( HFC_SCN_WLINKUP, (ulong *)&target->status ) )
	{
		return;
	}
	else if( test_bit( HFC_WAIT_LOGIN, (ulong *)&target->status ) )
	{
		return;
	}
	else if( test_bit( HFC_WAIT_CANCEL, (ulong *)&target->status ) )	/* FCLNX-GPL-038 */
	{
		return;
	}																	/* FCLNX-GPL-038 */
	else if ( test_bit( HFC_NEED_LOGIN, (ulong *)&target->status )
	       || (mode == HFC_ISSUE_LOGIN) )		/* FCLNX-0500 */
	{	/* Require LOGIN to the target */
		if( hfc_issue_relogin(ap, target) )
		{
			/* 
			 * If LOGIN initiation fails, enqueue next_login queue
			 * and terminate with error. 
			 */
			set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
			clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
			hfc_enque_login_req(ap, target);
			hfc_stra_trace(
				HFC_TRC_ISSUE_TMGM, 0x12, ap, target, hfcp, (ulong)cmnd, 0, 0);
		}
	}
	else if (mode == HFC_ISSUE_TARGET_RESET)
	{	/* Issue Target Reset */
#if 0
		if ( !ap->enable_tgtrst )
		{			
			/* Issue LOGIN */
			if ( hfc_issue_relogin(ap,target) )
			{
				set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
				clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-036 */
				hfc_enque_login_req(ap, target);
				
				hfc_stra_trace(HFC_TRC_ISSUE_TMGM, 0x77, ap, target, hfcp, (ulong)cmnd, 0, 0);
				return;
			}
			clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );				/* FCLNX-GPL-036 */
		
		}
#endif
		reset_pkt = hfc_get_new_hfcp(ap);
    	if( reset_pkt == NULL){
			/* No space for hfc_pkt */
			HFC_DBGPRT(" hfcldd : hfc_issue_task_mgm - hfcp==NULL-error\n");
			set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-328 */
			hfc_watchdog_enter(ap, target, hfcp, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);	
			hfc_watchdog_enter(ap, target, hfcp, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);/* FCLNX-GPL-328 */
			return;
		}
			
		dummy_cmnd = hfc_get_new_cmnd(ap);
    	if( dummy_cmnd == NULL){
			/* No dummy space for scsi_cmnd */
			HFC_DBGPRT(" hfcldd : hfc_issue_task_mgm - dummy_cmnd==NULL-error\n");
			set_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );			/* FCLNX-GPL-328 */
			hfc_watchdog_enter(ap, target, hfcp, 0, HFC_RESTART_TMR, (HZ/100+1), TRUE);	
			hfc_watchdog_enter(ap, target, hfcp, 0, HFC_RESTART_TMR, (HZ/100+1), FALSE);/* FCLNX-GPL-328 */
			return;
		}

		memset(reset_pkt, 0, sizeof(struct hfc_pkt));
//		memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));				/* FCLNX-GPL-0343 */
		memset(dummy_cmnd->cmnd, 0, 16 );								/* FCLNX-GPL-0343 */

		set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
		set_bit(CFLAG_TARGET_RESET, (ulong *)&reset_pkt->cmd_flags);
		ap->pkt_no++;
		if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
		ap->pkt_cnt++;

		reset_pkt->cmd_pkt   = dummy_cmnd;
		reset_pkt->target_id = target->target_id;
		reset_pkt->lun_id    = 0;
		reset_pkt->ap        = ap;
		reset_pkt->target    = target;
		reset_pkt->dev       = dev;
		dummy_cmnd->host_scribble = (void *)reset_pkt;
		if( cmnd != NULL ){		/* FCLNX-GPL-328 */
			hfc_dummy_copy(ap, cmnd, dummy_cmnd );
		}
		ap->cmnd_no++;
		if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
		ap->cmnd_cnt++;
		
		hfc_enqueue_wx_que(target, reset_pkt);
		
		/* Initiate Target Reset */
		if (hfc_start( ap, target, reset_pkt) == TRUE)
		{
			clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
			set_bit( HFC_WAIT_TARGET_RESET, (ulong *)&target->status );

			hfc_stra_trace(
				HFC_TRC_ISSUE_TMGM, 0x13, ap, target, reset_pkt, 
				(ulong)dummy_cmnd, 0, 0);
		}
	}
	else if(mode == HFC_ISSUE_ABORT)
	{/* Issue Abort Task Set issue */

		/* cancel xob */
		hfc_cancel_xob(ap, target, lun, hfcp, HFC_FLASH_DEV);    /* FCWIN-XX */

		abort_pkt = hfc_get_new_hfcp(ap);
    	if( abort_pkt == NULL){
			/* No space for hfc_pkt */
			HFC_DBGPRT(" hfcldd : hfc_eh_abort - hfcp==NULL-error\n");
			ap->scsi_err_cnt++ ;
			if( dev != NULL ){
				dev->lustat &= ~HFC_NEED_ABORT;					/* FCLNX-GPL-328 */
				dev->lustat &= ~HFC_WAIT_ABORT;
			}
			set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
			if( hfc_issue_relogin(ap, target) )
			{
				/* 
				 * If LOGIN initiation fails, enqueue next_login queue
				 * and terminate with error. 
				 */
				set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
				hfc_enque_login_req(ap, target);
				hfc_stra_trace(
					HFC_TRC_ISSUE_TMGM, 0x14, ap, target, hfcp, (ulong)cmnd, 0, 0);
			}														/* FCLNX-GPL-328 */
			return;
		}
	
		dummy_cmnd = hfc_get_new_cmnd(ap);
    	if( dummy_cmnd == NULL){
			/* No space for dummy scsi_cmnd */
			HFC_DBGPRT(" hfcldd : hfc_eh_abort - dummy_cmnd==NULL-error\n");
			ap->scsi_err_cnt++ ;
			if( dev != NULL ){
				dev->lustat &= ~HFC_NEED_ABORT;					/* FCLNX-GPL-328 */
				dev->lustat &= ~HFC_WAIT_ABORT;
			}
			set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
			if( hfc_issue_relogin(ap, target) )
			{
				/* 
				 * If LOGIN initiation fails, enqueue next_login queue
				 * and terminate with error. 
				 */
				set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);
				hfc_enque_login_req(ap, target);
				hfc_stra_trace(
					HFC_TRC_ISSUE_TMGM, 0x15, ap, target, hfcp, (ulong)cmnd, 0, 0);
			}														/* FCLNX-GPL-328 */
			return;
		}

		memset(abort_pkt, 0, sizeof(struct hfc_pkt));
//		memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));				/* FCLNX-GPL-0343 */
		memset(dummy_cmnd->cmnd, 0, 16 );								/* FCLNX-GPL-0343 */

		set_bit(CFLAG_VALID, (ulong *)&abort_pkt->cmd_flags);
		set_bit(CFLAG_ABORT, (ulong *)&abort_pkt->cmd_flags);
		ap->pkt_no++;
		if (ap->pkt_no >= ap->pkt_num) ap->pkt_no = 0;
		ap->pkt_cnt++;

		abort_pkt->cmd_pkt   = dummy_cmnd;
		abort_pkt->target_id = target->target_id;
		if( cmnd != NULL ){											/* FCLNX-GPL-328 */
			hfc_dummy_copy(ap, cmnd, dummy_cmnd );
		}
		abort_pkt->lun_id    = CMND_LUN(dummy_cmnd);
		abort_pkt->ap        = ap;
		abort_pkt->target    = target;
		abort_pkt->dev       = dev;
		dummy_cmnd->host_scribble = (void *)abort_pkt;
		ap->cmnd_no++;
		if (ap->cmnd_no >= ap->cmnd_num) ap->cmnd_no = 0;
		ap->cmnd_cnt++;

		hfc_enqueue_wx_que(target, abort_pkt);

		if( dev != NULL ){
			dev->lustat |=  HFC_NEED_ABORT;						/* FCLNX-GPL-289 *//* FCLNX-GPL-0343 */
		}
//		target->lustat[lun] &= ~HFC_NEED_ABORT;					/* FCLNX-GPL-289 */
//		target->lustat[lun] |=  HFC_WAIT_ABORT;					/* FCLNX-GPL-289 */
		
		if (hfc_start(ap, target, abort_pkt) == TRUE)
		{	/* Target Reset initiation failed (resource busy) */
//			target->lustat[lun] &= ~HFC_NEED_ABORT;				/* FCLNX-GPL-289 */
//			target->lustat[lun] |=  HFC_WAIT_ABORT;				/* FCLNX-GPL-289 */
		}
	}
	else { /* mode == HFC_ISSUE_LOGIN */			/* FCLNX-0500 */
		if ( hfc_issue_relogin(ap,target) )
		{
			set_bit( HFC_NEED_LOGIN, (ulong *)&target->status );
			clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
			hfc_enque_login_req(ap, target);
			
			hfc_stra_trace(HFC_TRC_ISSUE_TMGM, 0x78, ap, target, hfcp, (ulong)cmnd, 0, 0);
			return;
		}
	}												/* FCLNX-0500 */

	hfc_stra_trace(
			HFC_TRC_ISSUE_TMGM, 0x10, ap, target, hfcp, (ulong)cmnd, 0, 0);
	return;
}


/*
 * Function:    hfc_cancel_scsi_cmd
 *
 * Purpose:     Cancel Scsi_cmds which have already stored in xob, wait_end_que, wait_xob_que.
 *
 * Arguments:   
 *  ap            - Pointer to adap_info 
 *  target        - Pointer to target_info 
 *  lun           - lun number
 *  hfcp          - Pointer to hfc_pkt 
 *  adap_status   - Cause of cancellation
 *  we_que_cancel - Flag to specify cancellation of wait_end_que
 *  type          - Targets of Cancellation 
 *                    = HFC_FLASH_PACKET	  : ap,hfcp
 *                    = HFC_FLASH_DEV		  : ap,target,lun
 *                    = HFC_FLASH_TARGET	  : ap,target
 *
 * Returns:     
 *
 * context : user / kernel / interrupt
 *
 * Notes:			Lock adap_info before calling this function
 */
void hfc_cancel_scsi_cmd(struct adap_info	*ap,
						 struct target_info *target,
						 uint				lun,
						 struct hfc_pkt 	*hfcp,
						 uint				adap_status,
						 uchar				inh_altpath,
						 uchar				we_que_cancel,
						 uchar				tm_que_cancel,
						 uchar				type)
{

	HFC_DBGPRT(" hfcldd : hfc_cancel_scsi_cmd - start\n");
	
	hfc_cancel_xob(ap,target,lun,hfcp,type) ;
	
	if( we_que_cancel == TRUE )
		hfc_cancel_weque(ap,target,lun,hfcp,adap_status,inh_altpath,type) ;
		
	if( tm_que_cancel == TRUE )
		hfc_cancel_tskmgm(ap, target, lun, hfcp, adap_status, type);
	
	hfc_cancel_wxque(ap,target,lun,hfcp,adap_status,inh_altpath,type) ;
	return;
}


#define SC_TARGET_RESET 0x20	 	/* SCSI Device Head wants the adapter 	*/
									/* driver to issue a Target Reset task	*/
									/* request to this device				*/
/*
 * Function:    hfc_cancel_xob
 *
 * Purpose:     Turn on the skip flag of xob with range from otp to inp - 1.
 *              to inp - 1.
 *              No specific response to caller in this function -- Commands 
 *              stored in xob are also stored in wait_end_queue, so cancellation 
 *              of wait_end_que will report the completion of process to caller 
 *              afterward.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt
 *  type       - Targets of Cancellation
 *                    = HFC_FLASH_PACKET  : ap,hfcp
 *                    = HFC_FLASH_DEV     : ap,target,lun
 *                    = HFC_FLASH_TARGET  : ap,target
 * Returns:     
 *  0          - No xob exists for cancellation.
 *  1          - Canceled one or more xobs.
 *
 * Notes:			Lock adap_info before calling this function
 */
int hfc_cancel_xob( struct adap_info	*ap,
					struct target_info	*target,
					uint				lun,
					struct hfc_pkt		*hfcp,
					uchar				type)
{
	uint	save_in_num, save_out_num, wk_save_in_num, wk_save_out_num ;
	uint	outp_num, inp_num ;
	int		cancel = 0;
	struct	xob 	*xob_ptr;

	HFC_DBGPRT(" hfcldd : hfc_cancel_xob - start\n");

#if _HFC_DEBUG_STRA_00
		hfc_stra_trace(HFC_TRC_CAN_XOB,0x00,
			ap,target,hfcp,NULL,type,lun,NULL) ;
#endif

	wk_save_in_num = ap->fw_init_p->xob_inp;
	wk_save_out_num = ap->fw_init_p->xob_outp;
	HFC_4B_TO_4L(save_in_num, wk_save_in_num);
	HFC_4B_TO_4L(save_out_num, wk_save_out_num);

	if( save_out_num == save_in_num )
	{
		/* Xob is empty */

		hfc_stra_trace(HFC_TRC_CAN_XOB,0x20,
			ap,target,hfcp,(unsigned long long)type,(unsigned long long)lun, 0) ;

		return (0);
	}

	/* Caliculate sequential number of xob_inp and xob_outp */
	inp_num  = ((save_in_num & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	inp_num  += (save_in_num & 0x0000ffff) ;
	outp_num = ((save_out_num & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	outp_num += (save_out_num & 0x0000ffff) ;

	do {
		xob_ptr = &ap->xob[outp_num];

		if ( type == HFC_FLASH_DEV )	
		{
			/* Cancel each lun */
			if(( xob_ptr->drv_work.target_id ==				
				 ((uint)(target->target_id & 0xffffffff)) )
			 &&( xob_ptr->drv_work.lun_id  == lun ))
			{
				if(!(xob_ptr->bflag & SC_TARGET_RESET))
				{
					xob_ptr->skip |= HFC_XOB_SKIP ;
					cancel = 1;
				}
			}
		}
		else if( type == HFC_FLASH_TARGET )
		{
			/* Cancel target */
			if( xob_ptr->drv_work.target_id == 
				((uint)(target->target_id & 0xffffffff)))
			{
				if( !(xob_ptr->bflag & SC_TARGET_RESET) )
				{
					xob_ptr->skip |= HFC_XOB_SKIP ;
					cancel = 1;
				}
			}
		}
		else
		{
			break;
		}
		
		outp_num++ ;
		if( outp_num >= ap->xob_max )
			outp_num = 0 ;

	} while( outp_num != inp_num ) ;

#if _HFC_DEBUG_STRA_01			
		hfc_stra_trace(HFC_TRC_CAN_XOB,0x10,
			ap,target,hfcp,NULL,type,lun,NULL) ;
#endif

	return (cancel); 
}


/*
 * Function:    hfc_cancel_weque
 *
 * Purpose:     Dequeue hfc_pkt in we_que
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt
 *  inh_altpath- 
 *  adap_status- Cause of cancellation
 *  ap         - Adap_info structure pointer
 *  type       - Range of cancellation
 *               type = HFC_FLASH_PACKET  : ap,hfcp
 *                    = HFC_FLASH_DEV     : ap,target,lun
 *                    = HFC_FLASH_TARGET  : ap,target
 * Notes: Lock adap_info before calling this function
 */
void hfc_cancel_weque( struct adap_info    *ap,
					   struct target_info  *target,
					   uint 			   lun,
					   struct hfc_pkt	   *hfcp,
					   uint 			   adap_status,
					   uchar 			   inh_altpath,
					   uchar			   type )
{
	int hash, result=0;
	struct hfc_pkt	*wk_hfcp, *next_hfcp;
	struct dev_info	*dev=NULL;	/* FCLNX-GPL-0343 */

#if _HFC_DEBUG_STRA_02	
		hfc_stra_trace(HFC_TRC_CAN_WE,0x00,
			ap,target,hfcp,NULL,type,lun,adap_status);
#endif

	HFC_DBGPRT(" hfcldd : hfc_cancel_weque - start\n");

	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		wk_hfcp = target->we_que_top[hash];

		while ( wk_hfcp != NULL )
		{
			next_hfcp = wk_hfcp->cmd_forw;		/* Preserve next hfc_pkt */

			result = 0;
			
			if ( (type == HFC_FLASH_TARGET) 	/* Cancel each target	*/
			  || ((type == HFC_FLASH_DEV) 		/* Cancel each lun	*/
					&& (wk_hfcp->cmd_pkt != NULL) && (wk_hfcp->lun_id == lun) ))
			{
				/* Dequeue from we_que */
				if ((target->we_que_top[hash] == wk_hfcp)
				 && (target->we_que_end[hash] == wk_hfcp) )	/* Only one	*/
				{
					target->we_que_top[hash] = NULL;
					target->we_que_end[hash] = NULL;
				}
				else
				{
					if ( wk_hfcp == target->we_que_top[hash] )			/* queue top */
					{
						target->we_que_top[hash] = wk_hfcp->cmd_forw;
						target->we_que_top[hash]->cmd_prev = NULL;
					}
					else if ( wk_hfcp == target->we_que_end[hash])		/* queue end */
					{
						target->we_que_end[hash] = wk_hfcp->cmd_prev;
						target->we_que_end[hash]->cmd_forw = NULL;
					}
					else												/* Otherwise */
					{
						wk_hfcp->cmd_forw->cmd_prev = wk_hfcp->cmd_prev ;
						wk_hfcp->cmd_prev->cmd_forw = wk_hfcp->cmd_forw ;
					}
				}
				target->we_que_cnt-- ;

				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info && wk_hfcp->dev)
					hfc_manage_info.npubp->hfc_queue_count(wk_hfcp, 1, 1);	/* we dequeue */

				wk_hfcp->cmd_forw = NULL;
				wk_hfcp->cmd_prev = NULL;
				dev = wk_hfcp->dev;									/* FCLNX-GPL-0343 */

				if ( test_bit(CFLAG_ABORT,(ulong *)&wk_hfcp->cmd_flags) )
				{
					/* Turn off HFC_WAIT_ABORT when cancel Abort Task Set request */
					if( dev != NULL ){									/* FCLNX-GPL-0343 */
						dev->lustat &= ~HFC_WAIT_ABORT;
					}													/* FCLNX-GPL-0343 */
				}
				else if (test_bit(CFLAG_LUN_RESET,(ulong *)&wk_hfcp->cmd_flags) )
				{
					/* Turn off HFC_WAIT_LUN_RESET when cancel LUN RESET request */
					if( dev != NULL ){									/* FCLNX-GPL-0343 */
						dev->lustat &= ~HFC_WAIT_LUN_RESET;
					}													/* FCLNX-GPL-0343 */
				}
				else if ( test_bit(CFLAG_TARGET_RESET, (ulong *)&wk_hfcp->cmd_flags) ){
					/* Turn off HFC_WAIT_TARGET_RESET when cancel Target Reset request */
					clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status );
				}
				else if ( test_bit(CFLAG_BUS_RESET, (ulong *)&wk_hfcp->cmd_flags) ){
					/* Turn off HFC_WAIT_TARGET_RESET when cancel Target Reset request */
					clear_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status );
				}

				/* Store cancellation factor and scsi response */
				wk_hfcp->adap_status = adap_status ;

				/* Set reason code and initialize statistics *//* FCLNX-GPL-571 */
				if( test_bit( CFLAG_TIMEOUT, (ulong *)&wk_hfcp->cmd_flags ) )
				{
					result = DID_TIME_OUT;
				}
				else if (inh_altpath == HFC_CSCSI_RESET)
				{
					result = DID_RESET;
				}
				else 
				{
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
					if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ){ /* FCLNX-GPL-FX-472 */
						result = DID_ERROR;
					}
					else if( test_bit( HFC_CHK_STOP, (ulong *)&ap->status) || test_bit( HFC_ISOL, (ulong *)&ap->status) )
					{
						result = DID_NO_CONNECT;
					}
					else if( (target != NULL) && test_bit( HFC_SCN_WLINKUP, (ulong *)&target -> status ))
					{	/*** RSCN -> Target lost case ***/
						result = DID_ERROR;
					}
					else if( (target != NULL) && test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status))
					{	/*** PIC scceed case ***/
						result = DID_NO_CONNECT;
					}
					else
					{
						result = DID_ERROR;
					}
#else
					result = DID_ERROR;
#endif
				}
				
				wk_hfcp->cmd_pkt->result &= 0xff00ffff;
				wk_hfcp->cmd_pkt->result |= (result << 16);
				ap->scsi_err_cnt++ ;						/* Error termination count */
				hfc_iodone(ap, wk_hfcp->cmd_pkt, wk_hfcp);	/* Callback */
			}

			wk_hfcp = next_hfcp;				/* Move to the next hfc_pkt */
		}
	}

	return;
}


/*
 * Function:    hfc_cancel_wxque
 *
 * Purpose:     Dequeue hfc_pkt in wx_que
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt
 *  inh_altpath- 
 *  adap_status- Cause of cancellation
 *  ap         - Adap_info structure pointer
 *  type       - Range of cancellation
 *               type = HFC_FLASH_PACKET  : ap,hfcp
 *                    = HFC_FLASH_DEV     : ap,target,lun
 *                    = HFC_FLASH_TARGET  : ap,target
 * Notes: Lock adap_info before calling this function
 */
void hfc_cancel_wxque( struct adap_info    *ap,
					   struct target_info  *target,
					   uint 			   lun,
					   struct hfc_pkt	   *hfcp,
					   uint 			   adap_status,
					   uchar 			   inh_altpath,
					   uchar			   type)
{
	struct hfc_pkt	   *wk_hfcp, *next_hfcp;
	int			   		result;
	struct dev_info		*dev=NULL;


	HFC_DBGPRT(" hfcldd : hfc_cancel_wxque - start\n");

	hfc_stra_trace(HFC_TRC_CAN_WX,0x00,
			ap,target,hfcp,type,lun,adap_status);
			
	wk_hfcp = target->wx_que_top;
	
	while ( wk_hfcp != NULL )
	{
		next_hfcp = wk_hfcp->cmd_forw;	/* Preserve next hfc_pkt */
		result = 0;

		if ( (type == HFC_FLASH_TARGET) 		/* Cancel each target	*/
	  	|| ((type == HFC_FLASH_DEV) 			/* Cancel each lun	*/
				&& (wk_hfcp->cmd_pkt != NULL) && (wk_hfcp->lun_id == lun) ))
		{
			/* Dequeue from wx_que */
			hfc_deque_wx_que(target, wk_hfcp);
			
			dev = wk_hfcp->dev;

			/* Store cancellation factor and scsi response */
			wk_hfcp->adap_status = adap_status ;
			
			if ( test_bit(CFLAG_ABORT,(ulong *)&wk_hfcp->cmd_flags) )					/* FCLNX-GPL-0343 */
			{
				/* Turn off HFC_NEED_ABORT when cancel Abort Task Set request */
				dev->lustat &= ~HFC_NEED_ABORT;
			}
			else if (test_bit(CFLAG_LUN_RESET,(ulong *)&wk_hfcp->cmd_flags) )
			{
				/* Turn off HFC_NEED_LUN_RESET when cancel LUN RESET request */
				dev->lustat &= ~HFC_NEED_LUN_RESET;
			}																			/* FCLNX-GPL-0343 */
			else if ( test_bit(CFLAG_TARGET_RESET, (ulong *)&wk_hfcp->cmd_flags) ){
				/* 
				 * Turn off _DEFER_TARGET_RESET and NEED_TARGET_RESET 
				 * when cancel Target Reset request 
				 */
				clear_bit(HFC_DEFER_TARGET_RESET, (ulong *)&target->target_reset);		/* FCLNX-GPL-036 */
				clear_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
			}
			else if ( test_bit(CFLAG_BUS_RESET, (ulong *)&wk_hfcp->cmd_flags) ){
				/* 
				 * Turn off _DEFER_TARGET_RESET and NEED_TARGET_RESET 
				 * when cancel Target Reset request 
				 */
				clear_bit(HFC_DEFER_TARGET_RESET, (ulong *)&target->target_reset);		/* FCLNX-GPL-036 */
				clear_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
			}
			
			/* Set reason code and initialize statistics *//* FCLNX-GPL-571 */
			if (inh_altpath == HFC_CSCSI_RESET)
			{
				result = DID_RESET;
			}
			else
			{
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
					result = DID_ERROR;
				}
				else if( test_bit( HFC_CHK_STOP, (ulong *)&ap->status) || test_bit( HFC_ISOL, (ulong *)&ap->status) )
				{
					result = DID_NO_CONNECT;
				}
				else if( (target != NULL) && test_bit( HFC_SCN_WLINKUP, (ulong *)&target -> status ))
				{	/*** RSCN -> Target lost case ***/
					result = DID_ERROR;
				}
				else if( (target != NULL) && test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status))
				{	/*** PIC scceed case ***/
					result = DID_NO_CONNECT;
				}
				else
				{
					result = DID_ERROR;
				}
#else
				result = DID_ERROR;
#endif
			}
			
			wk_hfcp->cmd_pkt->result &= 0xff00ffff;
			wk_hfcp->cmd_pkt->result |= (result << 16);
			ap->scsi_err_cnt++ ;							/* Error termination count  */
			hfc_iodone(ap, wk_hfcp->cmd_pkt, wk_hfcp);		/* Callback */
		}

		wk_hfcp = next_hfcp;					/* Move to the next hfc_pkt */
	}
	
	hfc_stra_trace(HFC_TRC_CAN_WX,0x10,
		ap,target,hfcp,(unsigned long long)type,(unsigned long long)lun,(unsigned long long)adap_status);
}

/*
 * Function:    hfc_refer_reset_buf
 *
 * Purpose:     
 *
 * Arguments:   
 *  target     - Pointer to target_info
 *  hfcp       - Pointer to hfc_pkt 
 *
 * Returns:     
 *
 * Notes:       
 */
struct hfc_pkt *hfc_refer_reset_buf(struct target_info *target, struct	hfc_pkt *hfcp)
{
	struct	hfc_pkt		*hfcp_wk=NULL ;
	int 	i;

	HFC_DBGPRT(" hfcldd : hfc_refer_reset_buf - start\n");

	for(i=0; i<HASH_T_NUM; i++)
	{
		hfcp_wk = target -> wx_que_top ;

		while( hfcp_wk != NULL )
		{
			if( hfcp_wk == hfcp )
			{
				/* The Target Reset request exists in wx_que */
				return(hfcp_wk);
			}
			hfcp_wk = (struct hfc_pkt *) hfcp_wk->cmd_forw ;
		}
	}
	return(hfcp_wk);
}


/*
 * Function:    hfc_cancel_tskmgm
 *
 * Purpose:     Cancel we_que
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt
 *  adap_status- Cause of cancellation
 *  ap         - Adap_info structure pointer
 *  type       - Range of cancellation
 *               type = HFC_FLASH_PACKET  : ap,hfcp
 *                    = HFC_FLASH_DEV     : ap,target,lun
 *                    = HFC_FLASH_TARGET  : ap,target
 * Returns:     
 *
 * Notes:       
 */
void hfc_cancel_tskmgm(
	struct adap_info            *ap,
	struct target_info          *target,
	uint                       	lun,			/* FCLNX-GPL-0343 */
	struct hfc_pkt              *hfcp,
	uint                        adap_status,
	uint                        type)
{
	struct dev_info				*dev=NULL;		/* FCLNX-GPL-0343 */

	if (type == HFC_FLASH_TARGET)
	{
		if (test_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset) )		/* FCLNX-GPL-036 */
		{
			clear_bit(HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
			clear_bit(HFC_DEFER_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
		}
	}

	if (type == HFC_FLASH_TARGET)				/* FCLNX-GPL-0343 */
	{
		dev = target->dev;
		while( dev != NULL ){
			if (dev->lustat & HFC_NEED_ABORT)
			{
				dev->lustat &= ~(HFC_NEED_ABORT | HFC_DEFER_ABORT);
			}
			if (dev->lustat & HFC_NEED_LUN_RESET)
			{
				dev->lustat &= ~HFC_NEED_LUN_RESET;
			}
			dev = dev->next;
		}
	}

	if ( type == HFC_FLASH_DEV )
	{
		dev = target->dev;
		while( dev != NULL){
			if( dev->lun == lun){
				if (dev->lustat & HFC_NEED_ABORT)
				{
					dev->lustat &= ~(HFC_NEED_ABORT | HFC_DEFER_ABORT);
				}
				if (dev->lustat & HFC_NEED_LUN_RESET)
				{
					dev->lustat &= ~HFC_NEED_LUN_RESET;
				}
			}
			dev = dev->next;
		}
	}											/* FCLNX-GPL-0343 */
}


/*
 * Function:    hfc_deque_wx_que
 *
 * Purpose:     
 *
 * Arguments:   
 *  target     - Pointer to target_info
 *  hfcp       - Pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_deque_wx_que(struct target_info *target, struct hfc_pkt *hfcp)
{
	struct hfc_pkt *hfcp_wk ;

	/* Two or more packets exist in queue-*/
	if( hfcp == target->wx_que_top )
	{
		if (hfcp == target->wx_que_end) {
			target->wx_que_top = NULL ;
			target->wx_que_end = NULL ;
		}
		else {
			/* Dequeue from the top */
			target->wx_que_top = (struct hfc_pkt *)hfcp->cmd_forw ;
			if(target->wx_que_top != NULL) /* FCLNX-GPL-185 */
			{
				target->wx_que_top->cmd_prev = NULL ;
			}
		}
	}
	else if( hfcp == target->wx_que_end )
	{
		/* Dequeue from the end */
		target->wx_que_end = (struct hfc_pkt *)hfcp->cmd_prev ;
		if(target->wx_que_end != NULL) /* FCLNX-GPL-185 */
		{
			target->wx_que_end->cmd_forw = NULL ;
		}
	}
	else
	{
		/* Otherwise */
		hfcp_wk = (struct hfc_pkt *)hfcp->cmd_prev ;
		if( hfcp_wk != NULL) /* FCLNX-GPL-185 */
		{
			hfcp_wk->cmd_forw = hfcp->cmd_forw ;
		}
		
		hfcp_wk = (struct hfc_pkt *)hfcp->cmd_forw ;
		if( hfcp_wk != NULL) /* FCLNX-GPL-185 */
		{
			hfcp_wk->cmd_prev = hfcp->cmd_prev ;
		}
	}
	target->wx_que_cnt--;
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info && hfcp->dev)
		hfc_manage_info.npubp->hfc_queue_count(hfcp, 0, 1);	/* wx dequeue */

	hfcp->cmd_forw = NULL ;
	hfcp->cmd_prev = NULL ;
	return ;
}


/*
 * Function:    hfc_enqueue_wx_que
 *
 * Purpose:     
 *
 * Arguments:   
 *  target     - Pointer to target_info
 *  hfcp       - Pointer to hfc_pkt
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_enqueue_wx_que(struct target_info *target, struct hfc_pkt *hfcp)
{
//	HFC_DBGPRT(" hfcldd : hfc_enqueue_wx - start\n");

	if( target->wx_que_top == NULL ) {
		/* Add packet to empty queue */	
		target->wx_que_top = hfcp ;	
		target->wx_que_end = hfcp ;
		hfcp->cmd_forw = NULL;
		hfcp->cmd_prev = NULL;
	}
	else {
		/* Enqueue packet to the end of the cue */
		target->wx_que_end->cmd_forw = hfcp;
		hfcp->cmd_prev = target->wx_que_end ;
		target->wx_que_end = hfcp ;
	}
	target->wx_que_cnt++ ;
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info && hfcp->dev)
		hfc_manage_info.npubp->hfc_queue_count(hfcp, 0, 0);	/* wx enqueue */

}


/*
 * Function:    hfc_resource_chk
 *
 * Purpose:     Check remaining resource of xob, scmd_buf, and iov_map.
 *              Completing this function means ap->iov_no and hfcp->iov_cnt
 *              is set and iov_map is renewed.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  hfcp       - Pointer to hfc_pkt
 *  fw_xob_outp- 
 *
 * Returns:     
 *  0                  - Success
 *  HFC_XOB_EMPTY(1)   - Success (Xob is empty)
 *  HFC_XOB_FULL(4)    - Xob is full
 *  HFC_SCMD_FULL(5)   - Scmd_buf is full
 *  HFC_IOVMAP_FULL(6) - Bus address area is full for DMA mapping
 *  HFC_PAGE_OVER(-1)  - The number of Pages is larger than iov_map_cnt
 *  
 *
 * Notes: This routine is called from the process level and the interruption level.

 */
int hfc_resource_chk(struct adap_info *ap, 
					 struct target_info *target,
					 struct hfc_pkt *hfcp,
					 uint   fw_xob_outp)
{
	struct scsi_cmnd	*cmnd ;
	uint				xob_out_num ;
	uint				page_cnt = 0 ;
	uint				bit_pos = 0 ;
	uint				start_pos_word = 0 ;
	uint				start_pos_in_word = 0 ;
	struct free_iov_map	free_iov ;
	uchar				find ;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	uint				seg_cnt, ioctl_mode=0;
	uint32_t			data_size;
#else
	uint				seg_cnt,  i;
	uint32_t			data_size, transfer_size;
	dma_addr_t			b_dma_a;
#endif
	uint64_t			dma_a;


#if _HFC_DEBUG_STRA_03
	hfc_stra_trace(HFC_TRC_RES_CHK,0x00,
		ap,target,dev,(unsigned long long)dev->wx_que_top,
		NULL,NULL) ;
#endif
	/* Caliculate sequential number of xob_outp from page and entry number */
	xob_out_num = ((fw_xob_outp & 0x00ff0000)>>16) * HFC_XOB_PER_PAGE ;
	xob_out_num += (fw_xob_outp & 0x0000ffff) ;
	
	cmnd = hfcp->cmd_pkt ;
	
	if( cmnd == NULL){
		HFC_DBGPRT(" hfcldd%d : hfc_resource_chk - cmnd == NULL \n", ap->dev_minor);
		return( HFC_XOB_FULL ) ;
	}
	
	if( cmnd->scsi_done == (void *) hfc_ioctl_iodone ) ioctl_mode=1; 	/* FCLNX-GPL-0343 */

	/* Is xob full? */
	if( (ap->xob_no == (ap->xob_max-1)) &&
		(xob_out_num == 0) ) 
	{
		HFC_DBGPRT(" hfcldd : hfc_resource_chk - (ap->xob_no == (ap->xob_max-1)) &&	(xob_out_num == 0)\n");

		hfc_stra_trace(HFC_TRC_RES_CHK,0x11, ap,target,hfcp, (ulong)cmnd, 0, 0);
		/* Inp is the last entry of xob and outp is the top of xob */
		return( HFC_XOB_FULL ) ;
	}
	if( (ap->xob_no + 1) == xob_out_num )
	{
		HFC_DBGPRT(" hfcldd : hfc_resource_chk - (ap->xob_no + 1) == xob_out_num \n");
		
		hfc_stra_trace(HFC_TRC_RES_CHK,0x12, ap,target,hfcp, (ulong)cmnd, 0, 0);

		/* outp = inp + 1*/
		return( HFC_XOB_FULL ) ;
	}

	/* Caliculate necessary number of pages */
	page_cnt = 0 ;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	hfcp->seg_cnt = scsi_sg_count(cmnd);
	if (hfcp->seg_cnt)
	{
		if( !ioctl_mode ){					/* FCLNX-GPL-0343 */
			hfcp->seg_cnt = dma_map_sg(&ap->pci_cfginf->dev, scsi_sglist(cmnd), scsi_sg_count(cmnd),
				cmnd->sc_data_direction);
		}
	}
#else
	if(cmnd->use_sg != 0)
	{
		hfcp->seg_cnt = pci_map_sg(ap->pci_cfginf, cmnd->request_buffer, cmnd->use_sg, 
				scsi_to_pci_dma_dir(cmnd->sc_data_direction));

		if (!hfcp->seg_cnt) {
			HFC_DBGPRT(" hfcldd : hfc_resource_chk - pci_map_sg() == 0\n");
			return( HFC_XOB_FULL ) ;
		}
	}
#endif
	else{
		hfcp->seg_cnt = 0;
	}
	seg_cnt = hfcp->seg_cnt;

	if(seg_cnt == 0){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
		data_size = 0;
		page_cnt = 0;
#else
		data_size = (int)cmnd->request_bufflen;
		if(data_size == 0)
		{
			page_cnt = 0;
		}
		else 
		{
			page_cnt = 1;
		}
#endif
	}
	else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
		data_size = scsi_bufflen(cmnd);
		page_cnt = seg_cnt;
#else
		struct	scatterlist *cur_seg;
		struct	scatterlist *scatter_list;

		scatter_list = (struct scatterlist *)cmnd->request_buffer;
		cur_seg = scatter_list;
		transfer_size = 0;

		for(i=0; i<seg_cnt; i++) {
			uint32_t	b_length;

			/* Allocate additional packets */
			b_length = sg_dma_len(cur_seg);
			transfer_size += b_length;
			cur_seg++;
			page_cnt++;
		}
		
		
		data_size = transfer_size;
#endif
	}
	hfcp->data_size = data_size;

	if( page_cnt > ap->iov_map_cnt )
	{
		HFC_ERRPRT(" hfcldd : hfc_resource_chk - page_cnt > ap->iov_map_cnt\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
//		dma_a = (uint64_t)NULL;
		dma_a = 0;
#else
		b_dma_a = sg_dma_address((struct scatterlist *)cmnd->request_buffer);
		dma_a = (uint64_t)b_dma_a;
#endif
		HFC_BZERO(logdata,16) ;
		HFC_MEMCPY(logdata,(uchar*)&dma_a, 8) ;
		HFC_MEMCPY(&logdata[8],(uchar*)&(hfcp->data_size),4) ;
		HFC_MEMCPY(&logdata[12],(uchar*)&page_cnt,4) ;
		hfc_errlog(ap,target,hfcp,HFC_ERRLOG_TYPE_NONE,
								ERRID_HFCP_ERR9,0x05,logdata,16) ;
		return( HFC_PAGE_OVER ) ;
	}
	
	hfcp->iov_cnt = 0 ;
	hfcp->iov_no = 0 ;
	if( ( page_cnt > 0 ) && ( !( HFC_HFCP_CFLAG_TEST(CFLAG_SEGVALID, hfcp) ) ) )
	{
		/* Search necessary consecutive area from iov_map */
		start_pos_in_word = 0 ;		/* Start bit for search */
		start_pos_word = 0 ;		/* Start word for search */
									/* The top address of iov_map		*/
		bit_pos = 0 ;
		find = FALSE ;
		while( (bit_pos < ap->iov_map_cnt) && (find == FALSE) )
		{
			/* Caliculate bit position and length of consecutive area */
			hfc_get_free_iov(ap,start_pos_word,
								start_pos_in_word,page_cnt,&free_iov) ;

			/* No enough space in iov_map */
			if( free_iov.free_cnt == 0 )
			{
				hfc_stra_trace(HFC_TRC_RES_CHK,0x14, ap,target,hfcp, (ulong)cmnd, 0, 0);
				return( HFC_IOVMAP_FULL ) ;
			}
			if( free_iov.free_cnt >= page_cnt )
			{
				ap->iov_no = free_iov.free_pos ;
				hfcp->iov_no = ap->iov_no;
				hfcp->iov_cnt = page_cnt ;
				find = TRUE ;
				break ;
			}
			else
			{
				start_pos_word = bit_pos / 32 ;
				start_pos_in_word = bit_pos % 32 ;
			}
			bit_pos = free_iov.free_cnt + free_iov.free_pos ;
		}
		if( find == FALSE )
		{
			hfc_stra_trace(HFC_TRC_RES_CHK,0x14, ap,target,hfcp, (ulong)cmnd, 0, 0);
			return( HFC_IOVMAP_FULL ) ;
		}
		
		if( ( hfcp->iov_cnt+hfcp->iov_no ) > ap->iov_map_cnt )
		{
			HFC_ERRPRT(" hfcldd : hfc_resource_chk - ( hfcp->iov_cnt+hfcp->iov_no ) > ap->iov_map_cnt \n");

			hfc_stra_trace(HFC_TRC_RES_CHK,0x14,
				ap,target,hfcp, (ulong)cmnd, 0, 0);
			return( HFC_IOVMAP_FULL ) ;
		}
		hfc_iov_update(ap,free_iov.free_pos,page_cnt,0) ;
    }
	if( ap->xob_no == xob_out_num )
	{
#if _HFC_DEBUG_STRA_04
		hfc_stra_trace(HFC_TRC_RES_CHK,0x15,
			dev->ap,target,dev,(unsigned long long)dev->wx_que_top,
			NULL,NULL) ;
#endif
		return( HFC_XOB_EMPTY ) ;
	
	}
#if _HFC_DEBUG_STRA_05
	hfc_stra_trace(HFC_TRC_RES_CHK,0x10,
		ap,target,(unsigned long long)target->wx_que_top,
		NULL,NULL) ;
#endif	

	return(0) ;
}


/*
 * Function:    hfc_get_free_iov
 *
 * Purpose:     Find first available consecutive bits from the position of
 *              st_word and st_bit in iov_map. 
 *              Iov_map is full when free_iov_map.free_cnt is zero.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  st_word    - Start position of word search
 *  st_bit     - Start position of bit search
 *  req_bit_cnt - 
 *  fmap        -
 *
 * Returns:     
 *  free_iov_map.free_cnt - Number of available consecutive bits
 *  free_iov_map.free_pos - Bit start position of free_cnt in iov_map
 *
 * Notes:		Lock adap_info before calling this function
 */
void hfc_get_free_iov(struct adap_info *ap ,
									 uint st_word,
									 uint st_bit,
									 uint req_bit_cnt,
									 struct	free_iov_map *fmap)
{
	uint	*iov ;					/* iov_map				*/
	uint	word_pos = 0 ;			/* word position of iov_map  */
	uint    bit_pos = 0 ;			/* bit position of iov_map  */
	uint	chk_bit_cnt = 0 ;		/* number of checked bits */
	uint	shift = 0x80000000 ;    /* check bit		*/
	uint	lp ;
	uchar	find = FALSE ;			/* Empty aera detection flag */
	struct	free_iov_map	free_map ;
	iov = ap->iov_map ;			
	word_pos = st_word ;			
	bit_pos = st_word*32 + st_bit ;	
	free_map.free_pos = 0 ;			
	free_map.free_cnt = 0 ;         
	chk_bit_cnt = 0 ;				


	if( st_bit != 0 )
	{
		for(lp=st_bit ; lp<32 ; lp++)
		{
			if( (shift >> lp) & iov[word_pos] )
			{
				/* Detect "1" (unused) */
				if( find == FALSE )
				{
					/* Detect unused area for the first time? */
					find = TRUE ;
					/* Set bit position to free_map */
					free_map.free_pos = bit_pos ;
				}
				
				bit_pos++ ;
				/* Set the number of free bits to free_map */
				free_map.free_cnt++ ;
			}
			else
			{
				/* Detect "0" (already used) */
				if( find == TRUE )
				{	
					/* Return current bits stored in free_map */
					*fmap = free_map;
					return;
				}
				else
					/* increment bit position */
					bit_pos++ ;
			}
			if( bit_pos >= ap->iov_map_cnt )
			{
				/* Bit position exceeded the maximum bit position */
				*fmap = free_map;
				return;
			}
			if( free_map.free_cnt > req_bit_cnt ) {
				*fmap = free_map;
				return;
			}
		}

		word_pos++ ; /* Move to the next word */
	}

	while( bit_pos < ap->iov_map_cnt )
	{
		if( free_map.free_cnt > req_bit_cnt ) {
			*fmap = free_map;
			return;
		}
		
		if( iov[word_pos] == 0 )
		{
			/* This word is full */
			if( find == TRUE )
			{
				*fmap = free_map;
				return;
			}
			else {
				bit_pos += 32 ;
				word_pos++ ;
				continue ;
			}
		}
		if( iov[word_pos] == 0xffffffff )
		{
			/*This word is empty */
			if( find == TRUE ) {
				free_map.free_cnt += 32 ;
				bit_pos += 32 ;
				word_pos++ ;
				continue ;
			}
			else {
				/* Detect "1" for the first time. */
				find = TRUE ;
				free_map.free_pos = bit_pos ;
				free_map.free_cnt += 32 ;
				bit_pos += 32 ;
				word_pos++ ;
				continue ;
			}
		}
		/* Iov word_pos is neither all zeros nor all 1. --*/
		for(lp=0 ; lp<32 ; lp++)
		{
			if( (shift >> lp) & iov[word_pos] )
			{
				if( find == FALSE )
				{
					find = TRUE ;
					free_map.free_pos = bit_pos ;
				}
				free_map.free_cnt++ ;
				bit_pos++ ;
			}
			else
			{
				if( find == TRUE )
				{
					*fmap = free_map;
					return;
				}
				else
					bit_pos++ ;
			}
			
			if( bit_pos >= ap->iov_map_cnt )
			{
				*fmap = free_map;
				return;
			}

			if( free_map.free_cnt > req_bit_cnt ) {
				*fmap = free_map;
				return;
			}

		}
		word_pos++ ;
	}

	*fmap = free_map;
	return;
}


/*
 * Function:    hfc_iov_update
 *
 * Purpose:     Update bits of ap->iov_map with specified type starting from 'pos'
 *              bit to 'pos + cnt' bit.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  pos        - Update start bit in iov_map
 *  cnt        - Number of update counts in iov_map
 *  type       - 0 : This area is used 1: This area is free
 *
 * Returns:     
 *
 * Notes:		Lock adap_info before calling this function
 */
void hfc_iov_update(struct adap_info *ap ,
					uint pos ,
					uint cnt ,
					uchar type)
{
	uint	*iov ;					/* iov_map				*/
	uint	word_pos = 0 ;			/* Word position of iov_map  */
	uint    bit_pos_in_word ;		/* Bit position in word   */
	uint	shift = 0x80000000 ;    /* check bit		*/
	uint	bit_cnt = 0 ;
	uint	bit_pos = 0 ;
#if _HFC_DEBUG_STRA_06
	uchar	trc_iov[128] ;	
#endif
#if _HFC_DEBUG_STRA_07
	hfc_stra_trace(HFC_TRC_IOVUP,0x00,
		ap,NULL,NULL,(unsigned long long)type,
		(unsigned long long)pos,(unsigned long long)cnt ) ;
#endif
	word_pos = pos/32 ;
	bit_pos_in_word = pos%32 ;
	iov = ap->iov_map ;
	bit_cnt = cnt ;
	bit_pos = pos ;

#if _HFC_DEBUG_STRA_06	
	memset(trc_iov,0,128);
	memcpy(&trc_iov[16],(char *)iov,32);
#endif

	while( bit_cnt != 0 )
	{
		if( bit_pos_in_word != 0 )
		{
			if( type == 0 )
				iov[word_pos] &= ~(shift >> bit_pos_in_word) ;
			else
				iov[word_pos] |= (shift >> bit_pos_in_word) ;
			
			bit_pos++ ;
			bit_cnt-- ;
		}
		else
		{
			if( bit_cnt > 32 )
			{
				if( type == 0 )
					iov[word_pos] = 0 ;
				else
					iov[word_pos] = 0xffffffff ;
				bit_pos+=32 ;
				bit_cnt-=32 ;
			}
			else
			{
				if( type == 0 )
					iov[word_pos] &= ~(shift >> bit_pos_in_word) ;
				else
					iov[word_pos] |= (shift >> bit_pos_in_word) ;
				bit_pos++ ;
				bit_cnt-- ;
			}
		}
		if( bit_pos < ap->iov_map_cnt )
		{
			word_pos = bit_pos/32 ;
			bit_pos_in_word = bit_pos%32 ;
		}
		else
		{
#if _HFC_DEBUG_STRA_08
			hfc_stra_trace(HFC_TRC_IOVUP,0x11,
				ap,NULL,NULL,(unsigned long long)type,
				(unsigned long long)pos,(unsigned long long)cnt ) ;
#endif
#if _HFC_DEBUG_STRA_06	
			memcpy(&trc_iov[48],(char *)iov,32);
			hfc_trace(ap,0xFF,&trc_iov[1],0);
#endif
			return ;
		}
	}
	
#if _HFC_DEBUG_STRA_09
	hfc_stra_trace(HFC_TRC_IOVUP,0x10,
			ap,NULL,NULL,(unsigned long long)type,
			(unsigned long long)pos,(unsigned long long)cnt ) ;
#endif
#if _HFC_DEBUG_STRA_06	
	memcpy(&trc_iov[48],(char *)iov,32);
	hfc_trace(ap,0xFF,&trc_iov[1],0);
#endif
	return ;
}


/*
 * Function:    hfc_dma_map
 *
 * Purpose:     Setup DMA cookie from seg_info
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  hfcp       - Pointer to hfc_pkt
 *
 * Returns:     
 *  DMA_SUCC(0) - success
 *  DMA_NOACC   - Unable to access
 *
 * context:   user / interrupt
 *
 * Notes:		Lock adap_info before calling this function
 *              This function set xob->seg_1st, xob->seg_cnt and xob->seg_info_xob
 *              in addition to seg_info of hfc_pkt.
 */
int hfc_dma_map( struct adap_info *ap, struct hfc_pkt *hfcp )
{
	uint				seg_info_no=0 ;				/* seg_info area offset number */
	uint				seg_no ;					/* seg_infonumber in xob 	*/
	uint64_t			seg_info_baddr = 0; 		/* seg_info badr in xob */
	uint				lp, page_cnt=0 ,xob_no,cn=0;
	struct seg_info 	*seg_info_ptr=NULL;
	struct scsi_cmnd	*cmnd;
	dma_addr_t	b_dma_a;
	uint32_t	b_length;
	uint		xob_segno;
	uint64_t	dma_add;
	struct scatterlist *cur_seg;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	struct scatterlist *scatter_list;
	dma_addr_t			req_dma;
	uint32_t				req_len;	
	struct page			*page;
	unsigned long			offset;
#endif
	uint		copy_seg_num;

	if( hfcp->iov_cnt ) /* Iov has already allocated  for this hfcp --> use seginfo */
	{	/* Iov has already allocated  for this hfcp --> use seginfo */
	
		/* Add 'page number - 1' for the seg_info_no after the page of 2 */
		seg_info_no = hfcp->iov_no + (hfcp->iov_no / (HFC_SEG_PER_PAGE-1)) ;

		/* Caliculate page number of this seg_info */
		lp = seg_info_no / HFC_SEG_PER_PAGE ;
		if( lp == 0 )
		{
			/* Setup the start address of seg_info if pages number is zero */
			seg_info_baddr = ap->seg_phys_addr;
		}
		else
		{
			/* 
			 * Page address is set in the last entry of previous page 
			 * if page number is not zero.
			 */ 
			lp = (lp * HFC_SEG_PER_PAGE) - 1 ;
			if( !(ap->seg_info[lp].seg_len & HFC_SEG_LEN_F))
			{

				/* Failed if HFC_SEG_LEN_F is zero 0. */
				HFC_MEMCPY(logdata,(uchar*)&(hfcp->data_size),4) ; 
				HFC_MEMCPY(&logdata[8],(uchar*)&page_cnt,4) ;
				HFC_MEMCPY(&logdata[12],(uchar*)&seg_info_no,4) ;
				hfc_errlog(ap,NULL,hfcp,HFC_ERRLOG_TYPE_NONE,
									ERRID_HFCP_ERR9,0x08,logdata,16) ;

				hfc_iov_update(ap, hfcp->iov_no, hfcp->iov_cnt, 1) ;

				hfc_stra_trace(HFC_TRC_DMA_MAP,0x20,
					ap,NULL,hfcp, 0, 0, 0);

				return( FAILED ) ;
			}

			HFC_8B_TO_8L(seg_info_baddr, ap->seg_info[lp].seg_addr);
		}

		/* Add page offset byte to seg_info bus address */
		seg_info_baddr += ((seg_info_no % HFC_SEG_PER_PAGE)*
									sizeof(struct seg_info)) ;
	}

	/* Set seg_info */
	xob_no = ap->xob_no;
	hfcp->cmd_xob = xob_no;
	cmnd = hfcp->cmd_pkt;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	if ( hfcp->data_size > 0 )	/* Transfer data exists? */
	{
		if (hfcp->seg_cnt)
		{
			/* Copy XOB_SEGNUM to XOB. */
			cn = 0; 								/* DMA cookie#			*/
			seg_no = seg_info_no; 					/* seg_info offset		*/

			if(hfcp->seg_cnt > 0)
			{
				scsi_for_each_sg(cmnd, cur_seg, hfcp->seg_cnt, cn) {
					seg_info_ptr = &ap->seg_info[seg_no];
					
					if (seg_info_ptr->seg_len & HFC_SEG_LEN_F)
					{
						seg_no++;
						seg_info_ptr = &ap->seg_info[seg_no];
					}
					
					b_dma_a = sg_dma_address(cur_seg);
					b_length = sg_dma_len(cur_seg);
					
					if(cn == 0 )
					{
						hfcp->dma_handle = b_dma_a;
					}
					
					dma_add = (uint64_t)b_dma_a;
					HFC_8L_TO_8B(seg_info_ptr->seg_addr, dma_add);
					HFC_4L_TO_4B(seg_info_ptr->seg_len, b_length);
					xob_segno = (uint)((xob_no << 24) & 0xff000000) | cn ; 
					HFC_4L_TO_4B(seg_info_ptr->xob_segno, xob_segno);
					
					seg_no++;
				}
				
				/* Turn on L bit of final seg_info */
				seg_info_ptr->seg_len |= HFC_SEG_LEN_L ;
				
				/* Copy seg_info to xob. */
				if( hfcp->seg_cnt <= XOB_SEGNUM )
				{
					copy_seg_num = hfcp->seg_cnt;
				}
				else
				{
					copy_seg_num = XOB_SEGNUM;
				}
				HFC_MEMCPY(ap->xob[xob_no].seg_info_xob,
						&ap->seg_info[seg_info_no],
						(sizeof(struct seg_info)* copy_seg_num));
			}
		}
		set_bit(CFLAG_SEGVALID, (ulong *)&hfcp->cmd_flags);

		/* Set all allocated iov_cnt in seg_info.	*/
		/* F bit should be zero in the final seg_info.	*/
	}
#else
	scatter_list = (struct scatterlist *)cmnd->request_buffer;
	

	if ( hfcp->data_size > 0 )	/* Transfer data exists? */
	{

		cur_seg = scatter_list;
		if (hfcp->seg_cnt == 0)
		{
			cn = 0;
			seg_no = seg_info_no; 					/* seg_info offset		*/
			do
			{
				seg_info_ptr = &ap->seg_info[seg_no];
				if (!(seg_info_ptr->seg_len & HFC_SEG_LEN_F))
				{

					page = virt_to_page(cmnd->request_buffer);
					offset = ((unsigned long)
							 cmnd->request_buffer 
							 & ~PAGE_MASK);

					req_dma = pci_map_page(ap->pci_cfginf,
						page,
						offset,
						cmnd->request_bufflen,
						scsi_to_pci_dma_dir(
						cmnd->sc_data_direction));
					req_len = cmnd->request_bufflen;
			
					dma_add = (uint64_t)req_dma;
				
					hfcp->dma_handle = req_dma;
			
					HFC_8L_TO_8B(seg_info_ptr->seg_addr, dma_add);
					HFC_4L_TO_4B(seg_info_ptr->seg_len, req_len);
			
					xob_segno = (uint)((xob_no << 24) & 0xff000000) | cn ;
					HFC_4L_TO_4B(seg_info_ptr->xob_segno, xob_segno);
			
					cn++;
				}
				seg_no++;
			}
			while (cn < hfcp->seg_cnt);
			
			/* Turn on L bit of final seg_info */
				seg_info_ptr->seg_len |= HFC_SEG_LEN_L ;
		
			/* Copy seg_info to xob. */
			HFC_MEMCPY(ap->xob[xob_no].seg_info_xob,
				&ap->seg_info[seg_info_no],
				(sizeof(struct seg_info)));
		}
		else
		{

			/* Copy XOB_SEGNUM to XOB. */
			cn = 0; 								/* DMA cookie#			*/
			seg_no = seg_info_no; 					/* seg_info offset		*/

			if(hfcp->seg_cnt > 0)
			{

				do
				{
					seg_info_ptr = &ap->seg_info[seg_no];
					if (!(seg_info_ptr->seg_len & HFC_SEG_LEN_F))
					{
						b_dma_a = sg_dma_address(cur_seg);
						b_length = sg_dma_len(cur_seg);
						
						if(cn == 0 )
						{
							hfcp->dma_handle = b_dma_a;
						}
				
						dma_add = (uint64_t)b_dma_a;
						HFC_8L_TO_8B(seg_info_ptr->seg_addr, dma_add);
						HFC_4L_TO_4B(seg_info_ptr->seg_len, b_length);
						xob_segno = (uint)((xob_no << 24) & 0xff000000) | cn ; 
						HFC_4L_TO_4B(seg_info_ptr->xob_segno, xob_segno);
						cur_seg++;									
						cn++;
						
				
					}
					seg_no++;
				}
				while (cn < hfcp->seg_cnt);		
				
				/* Turn on L bit of final seg_info */
				seg_info_ptr->seg_len |= HFC_SEG_LEN_L ;
				
				/* Copy seg_info to xob. */
				if( hfcp->seg_cnt <= XOB_SEGNUM )
				{
					copy_seg_num = hfcp->seg_cnt;
				}
				else
				{
					copy_seg_num = XOB_SEGNUM;
				}
				HFC_MEMCPY(ap->xob[xob_no].seg_info_xob,
						&ap->seg_info[seg_info_no],
						(sizeof(struct seg_info)* copy_seg_num));

			}
		}

		set_bit(CFLAG_SEGVALID, (ulong *)&hfcp->cmd_flags);

		/* Set all allocated iov_cnt in seg_info.	*/
		/* F bit should be zero in the final seg_info.	*/
	}
#endif

	HFC_8L_TO_8B(ap->xob[xob_no].seg_1st, seg_info_baddr);
	HFC_4L_TO_4B(ap->xob[xob_no].seg_cnt, cn);	/* No count for F=1 case */
	

#if _HFC_DEBUG_STRA_10
	hfc_dump_hex("hfc_dma_map() - xob     "
								,&ap->xob[xob_no],sizeof(struct xob));
	cmn_err(CE_CONT,"hfcp->iov_no = %x,hfcp->iov_cnt = %x",
									hfcp->iov_no,hfcp->iov_cnt);
#endif	

	return( SUCCESS ) ;
}


/*
 * Function:    hfc_make_cmdiu
 *
 * Purpose:     Set FCP-CMD-IU payload to the area pointed by hfc_pkt->cmd_xob
 *              Generate setting data from scsi_pkt and hfcpkt.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  hfcp       - Pointer to hfc_pkt 
 *
 * Returns:     
 *
 * context:     user / kernel / interrupt
 *
 * Notes:		Lock adap_info before calling this function
 */
void hfc_make_cmdiu( struct adap_info *ap, struct hfc_pkt *hfcp )
{
	struct fcp_cmd_iu  fcp_cmd;
	struct scsi_cmnd *cmnd = hfcp->cmd_pkt;
	ushort		lun_id=0;							/* FCLNX-GPL-0343 */
	
	/* Caliculate pointer to FCP-CMD-IU area of xob and clear the area */
	HFC_BZERO( &fcp_cmd, sizeof(struct fcp_cmd_iu) );
	
	/* FCP-LUN Byte0,1 */
	lun_id = (ushort)hfcp->lun_id;
	HFC_2L_TO_2B(fcp_cmd.fcp_lun.lun, lun_id);		/* FCLNX-GPL-0343 */

	/* FCP-CNTL Byte1 */
	if( !test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)
	 && !test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) 
	 && !test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags) 
	 && !test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) ) 
	{
  		if (cmnd->device->tagged_supported) {
    		switch (cmnd->tag) {
    		case HEAD_OF_QUEUE_TAG:
      			fcp_cmd.fcp_cntl.qtype = 1;			/* Head-of-Q			*/
      			break;
    		case ORDERED_QUEUE_TAG:
      			fcp_cmd.fcp_cntl.qtype = 2;			/* Ordered-Q			*/
      			break;
    		default:
      			fcp_cmd.fcp_cntl.qtype = 0;			/* Simple-Q 			*/
    			break;
    		}
  		}
  		else{
    		fcp_cmd.fcp_cntl.qtype = 5;				/* Untagged-Q			*/
    	}
  	}
  	else{
    	fcp_cmd.fcp_cntl.qtype = 5;				/* Untagged-Q			*/
    }


	/* FCP-CNTL Byte2 */
	if(test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags))
	{
		/* Abort Task Set */
		fcp_cmd.fcp_cntl.task_mgm |= HFC_ABORT_TASK;
	}
	else if(test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags))
	{
		/* LUN Reset */
		fcp_cmd.fcp_cntl.task_mgm |= HFC_LUN_RST;
	}
	else if(test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)
	     || test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
	{
		/* Target Reset */
		fcp_cmd.fcp_cntl.task_mgm |= HFC_TARGET_RST;
	}
	else
	{
		/* FCP-CDB */
		HFC_MEMCPY(fcp_cmd.fcp_cdb, (uchar*)&(cmnd->cmnd[0]), cmnd->cmd_len);
	}

	/* FCP-CNTL Byte3 */
	if ( cmnd->sc_data_direction == SCSI_DATA_WRITE )
		fcp_cmd.fcp_cntl.data_type |= HFC_WRITE_DATA;	/* bit7 */
	else
		fcp_cmd.fcp_cntl.data_type |= HFC_READ_DATA;	/* bit6 */

	/* FCP-DL */
	/* fcp_cmd->fcp_data_len = cmnd->bufflen; */
	HFC_4L_TO_4B(fcp_cmd.fcp_dl, hfcp->data_size );

	/* Copy hfc_pkt to XOB */
	HFC_MEMCPY((fcp_cmd_iu_t *)(&ap->xob[hfcp->cmd_xob].fcp_cmd),
										&fcp_cmd, sizeof(struct fcp_cmd_iu));

	if (hfcp->seg_cnt == 0) {													/* FCLNX-0404 */
		if (hfcp->data_size == 0) {
			ap->controlrequests++;
		}
		else {
			if (cmnd->sc_data_direction == SCSI_DATA_WRITE) {
				ap->outputrequests++;
				ap->outputbytes += hfcp->data_size;
			}
			else if (cmnd->sc_data_direction == SCSI_DATA_READ) {
				ap->inputrequests++;
				ap->inputbytes += hfcp->data_size;
			}
		}
	}	
	else {
		if (cmnd->sc_data_direction == SCSI_DATA_WRITE) {
			ap->outputrequests++;
			ap->outputbytes += hfcp->data_size;
		}
		else if (cmnd->sc_data_direction == SCSI_DATA_READ) {
			ap->inputrequests++;
			ap->inputbytes += hfcp->data_size;
		}
	}																			/* FCLNX-0404 */

	return ;
}


/*
 * Function:    hfc_xob_enque
 *
 * Purpose:     Create xob
 *              Dequeue hfc_pkt from the wx_que and enqueue it to we_que
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  hfcp       - Pointer to hfc_pkt  
 *  target     - Pointer to target_info
 * Returns:     
 *  0          - No need for starting timer
 *
 * context:     user / kernel / interrupt
 *
 * Notes:		Lock adap_info before calling this function
 */
void hfc_xob_enque(struct adap_info *ap, struct target_info *target,
						 struct hfc_pkt *hfcp)
{
	struct xob			*xob_p ;
	uint				inp_num = 0 ;
	uint				inp_page = 0 ;
	uint				inp_entry = 0 ;
	uint				hash ;
	uint                        	fw_xob_inp;
	uint                        	work_xob_inp;
	uint                        	wk_inp_entry;
	uint                        	wk_inp_entry2;
	struct hfc_pkt		**we_que_top_p;
	struct hfc_pkt		**we_que_end_p;
	uint                d_info = 0;
	ushort				t_fc_class = 0 ;/* FCLNX-0659 */

	/* Caliculate sequential number of xob from fw_init_tbl.xob_inp */
	work_xob_inp = ap->fw_init_p->xob_inp;
	HFC_4B_TO_4L(fw_xob_inp, work_xob_inp);
	inp_num = ((fw_xob_inp & 0x00ff0000)>>16)*HFC_XOB_PER_PAGE ;
	inp_num += (fw_xob_inp & 0x0000ffff) ;

	ap->xob_no = (ushort)inp_num ;	

	xob_p = &ap->xob[ap->xob_no] ;
	
	/* XOB - Driver Used Area */
	memset(&xob_p->drv_work, 0, sizeof(xob_p->drv_work));
	xob_p->drv_work.hfc_pkt   = (unsigned long long) (ulong) hfcp ;	/*  */
	xob_p->drv_work.target_id = (ushort)(target->target_id & 0xffffffff) ;
	xob_p->drv_work.lun_id	  = (ushort) hfcp->lun_id;
	
	if( HFC_MMODE_CHECK_SHARED(ap)  && !(HFC_MMODE_CHECK_SHADOW(ap) ) )     /* FCLNX-0371 */
	{
		xob_p->drv_work.rid       = (uchar) ap->rid;
	}                                                                       /* FCLNX-0371 */
	
	/* XOB - DEST_INFO */
	d_info = (uint)(target->scsi_id & 0x00ffffff);
	if (ap->connect_type == HFC_AL)
		d_info |= (uint)((target->scsi_id & 0x000000ff)<<24);
	HFC_4L_TO_4B(xob_p->d_info, d_info);

	/* XOB - DEST_INFO */
	xob_p->pseq = target->pseq ;

	/* XOB - Class */
	t_fc_class = (ushort)target->fc_class;	/* FCLNX-0659 */
	HFC_2L_TO_2B(xob_p->class, t_fc_class);	/* FCLNX-0659 */

	/* XOB - BURST_LEN */
	xob_p->burst_len = 0;

	/* XOB - FLAG,SKIP */
	xob_p->bflag = 0 ;
	xob_p->cflag = 0 ;
	xob_p->skip &= ~HFC_XOB_SKIP ;		/* Reset skip flag */
	xob_p->flag |= HFC_XOB_VALID ;		/* Set valid flag */

	/* Count up xob inp */
	ap->xob_no++ ;
	if( ap->xob_no >= ap->xob_max )
		ap->xob_no = 0 ;

	inp_page = ap->xob_no / HFC_XOB_PER_PAGE ;
	inp_entry = ap->xob_no % HFC_XOB_PER_PAGE ;
	wk_inp_entry = ((inp_page << 16) & 0x00ff0000) | (inp_entry & 0x0000ffff) ;
	HFC_4L_TO_4B(wk_inp_entry2, wk_inp_entry);
	ap->fw_init_p->xob_inp = wk_inp_entry2;

	/* Count up Xob execution wait count */
	ap->xob_wait_exec_cnt++ ;

#if _HFC_DEBUG_STRA_11
	/* Set timeout value */
	hfcp->cmd_timeout = gethrtime();				/* Read system timer */
																/* v1.32 STR */
	if (pkt->pkt_time)			
		wtime = pkt->pkt_time;						/* Unit of 1min 			*/
	else
		wtime = 24*60*60;							/* pkt_time=0 -> one day	*/

	hfcp->cmd_timeout += (wtime * 1000000000);	
#endif

	hfc_deque_wx_que(target, hfcp);

	hash = hfcp->lun_id % HASH_T_NUM;
	/* Enqueue wait_end_que */
	/* Move packet from wait_xob_que to wait_end_que. */
	we_que_top_p = &target->we_que_top[hash];
	we_que_end_p = &target->we_que_end[hash];

	if( *we_que_top_p == NULL )
	{
		/* Wait_end_que is empty */
		hfcp->cmd_forw = NULL;
		hfcp->cmd_prev = NULL;
		target->we_que_top[hash] = hfcp ;
		target->we_que_end[hash] = hfcp ;
		
	}
	else
	{
		/* Enqueue packet to the last of wait_end_que. */
		target->we_que_end[hash]->cmd_forw = (struct hfc_pkt *)hfcp ;
		hfcp->cmd_prev = (struct hfc_pkt *)target->we_que_end[hash] ;
		target->we_que_end[hash] = hfcp ;
				
		target->we_que_end[hash]->cmd_forw = NULL ;
	}
	target->we_que_cnt++;

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info && hfcp->dev)
		hfc_manage_info.npubp->hfc_queue_count(hfcp, 1, 0);	/* we enqueue */

	return ;
}


/*
 * Function:    hfc_enque_next_dstart
 *
 * Purpose:     Enqueue target to the next start target queue (ap->next_dstart)
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info *
 * Returns:     
 *
 *
 * Notes:		Lock adap_info before calling this function
 *				This function update following elements;
 * 			 		ap->next_dstart_top - The top of next start target queue
 *              	ap->next_dstart_end - The end of next start target queue
 *              	ap->next_dstart_cnt - Number of enqueued target
 *              	target->next_dstart_flag 
 *						- Flag means this target has already registered.
 */
void hfc_enque_next_dstart(struct adap_info *ap,struct target_info *target)
{

	if( target->next_dstart_flag )	/* Target has already registered in queue	*/
		return ;

	if( ap->next_dstart_cnt == 0 )
	{
		/* Enqueue to the empty queue */
		ap->next_dstart_top = target ;
		ap->next_dstart_end = target ;

		target->nnext = NULL ;
		target->nprev = NULL ;
	}
	else
	{
		/* Enqueue as a last element */
		ap->next_dstart_end->nnext = target ;
		target->nprev = ap->next_dstart_end ;
		ap->next_dstart_end = target ;
		target->nnext = NULL ;
	}

	hfc_watchdog_enter(ap, target, NULL, 0,HFC_WEXEC_TMR, 0, TRUE);		/* FCLNX-GPL-046	*/
	hfc_watchdog_enter(ap, target, NULL, 0,HFC_WEXEC_TMR, 0, FALSE);	/* FCLNX-GPL-046	*/
	ap->next_dstart_cnt++  ;
	target->next_dstart_flag = 1 ;
}

/*
 * Function:    hfc_deque_next_dstart
 *
 * Purpose:     Dequeue target from next start target queue (ap->next_dstart)
 *
 * Arguments:   
 *  ap          - Pointer to adap_info
 *  target      - Pointer to target_info *
 *
 * Returns:     -
 *
 * Notes:		Lock adap_info before calling this function
 *				This function update following elements;
 * 			 		ap->next_dstart_top - The top of next start target queue
 *              	ap->next_dstart_end - The end of next start target queue
 *              	ap->next_dstart_cnt - Number of enqueued target
 *              	target->next_dstart_flag 
 *						- Flag means this target has already registered.
 */
void hfc_deque_next_dstart(struct adap_info *ap,struct target_info *target)
{

	if( ap->next_dstart_cnt == 0 )		/* There is no target for dequeuing */
		return ;

	if( !target->next_dstart_flag ) 	/* This target is not enqueued	*/
		return ;									

	if( (target->wx_que_cnt == 0)		/* We_que empty? */
		&& (ap->xob_wait_exec_cnt != 0) /* Any xob waiting for execution? */
		&& (ap->next_dstart_cnt == 1) ) /* Any target waiting for initiation? */
	{
		target->next_dstart_flag = 2 ;
		return ;
	}

	if( ap->next_dstart_cnt == 1 )		/* The number of queue element is one */
	{
		ap->next_dstart_top = NULL ;
		ap->next_dstart_end = NULL ;
		ap->next_dstart_cnt = 0 ;
	}
	else
	{
		if( target == ap->next_dstart_top ) 	/* Dequeue from the top of the queue */
		{
			ap->next_dstart_top = target->nnext ;
			ap->next_dstart_top->nprev = NULL ;
			ap->next_dstart_cnt--;
		}
		else if( target == ap->next_dstart_end ) /* Dequeue from the last of the queue */
		{
			ap->next_dstart_end = target->nprev ;
			ap->next_dstart_end->nnext = NULL ;
			ap->next_dstart_cnt--;
		}
		else										/* Otherwise */
		{
			if( (target->nprev != NULL)
			 && (target->nnext != NULL) )
			{
				target->nprev->nnext = target->nnext ;
				target->nnext->nprev = target->nprev ;
				ap->next_dstart_cnt--;
			}
		}
	}

	target->nnext = NULL ;
	target->nprev = NULL ;
	target->next_dstart_flag = 0 ;
	hfc_watchdog_enter(ap, target, NULL, 0,HFC_WEXEC_TMR, 0, TRUE);		/* FCLNX-GPL-046	*/
}


/*
 * Function:    hfc_iodone
 *
 * Purpose:     Termination process of SCSI initiation.
 *
 * Arguments:   
 *  ap          - Pointer to adap_info
 *  cmnd        - Pointer to scsi_cmnd 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_iodone(
	struct adap_info        *ap,
	struct scsi_cmnd        *cmnd,
	struct hfc_pkt          *hfcp )
{
	struct target_info		*target=NULL;
	uchar warning_msg=0,retry_internal=0,retryout=0,cmd_type=0;	/* FCLNX-0244,0246 */
	struct target_info *wktg=NULL;
	uchar					seq_no;
	struct dev_info			*dev=NULL;	/* FCLNX-GPL-0449 */
	uchar					command;
	uchar					write_retries=0;	/* FCLNX-GPL-0449 */
	uint					ioctl_mode=0;  		/* FCLNX-GPL-464 */
	
	if( cmnd->scsi_done == (void *) hfc_ioctl_iodone ) ioctl_mode=1;	/* FCLNX-GPL-464 */

	if (hfcp != NULL)
	{
		dev = hfcp->dev;	/* FCLNX-GPL-0449 */

		/* Update iov only when normal SCSI initiation or IOCTL request occurs */
		if(	HFC_HFCP_CFLAG_TEST(CFLAG_SEGVALID, hfcp) )
		{
			if (hfcp->iov_cnt > 0)
			{
				hfc_iov_update(ap, hfcp->iov_no, hfcp->iov_cnt, 1);
			}
		}
		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
		if (hfcp->seg_cnt)
		{
			if( ioctl_mode !=1 ){		/* FCLNX-GPL-464 */
				scsi_dma_unmap(cmnd);
			}							/* FCLNX-GPL-464 */
		}
#else
		if (hfcp->seg_cnt)
		{
			pci_unmap_sg(ap->pci_cfginf, cmnd->request_buffer, cmnd->use_sg, 
				scsi_to_pci_dma_dir(cmnd->sc_data_direction));
		}
		else if(  (cmnd->request_bufflen )&&(hfcp->dma_handle) )
		{
			pci_unmap_page(ap->pci_cfginf,
					hfcp->dma_handle,
					cmnd->request_bufflen,
					scsi_to_pci_dma_dir(
					cmnd->sc_data_direction));
		}
#endif
		if ( test_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags) )
		{	/* Packet has MIHLOG */
			target = hfcp->target;
			if( test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags) )
			{
				memset( logdata, 0, 16 );
				memcpy( logdata, (uchar*)&ap->xob[ap->xob_no].fcp_cmd.fcp_cntl, 4 );
				hfc_errlog( ap, target, hfcp, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, 0x24, logdata, 16 );
				clear_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags);
			}
			else
			{
				memset( logdata, 0, 16 );
				memcpy( logdata, (uchar*)&ap->xob[ap->xob_no].fcp_cmd.fcp_cntl, 4 );
				hfc_errlog( ap, target, hfcp, HFC_ERRLOG_TYPE_TOUTLOG, ERRID_HFCP_ERRA, 0x26, logdata, 16 );
				clear_bit(CFLAG_SCMD_SLOGV, (ulong *)&hfcp->cmd_flags);
			}
			
		}
		
		if ( !test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) )			/* FCLNX-0554 STR */
		{
			cmnd->host_scribble = 0;
			
			if ((cmnd->device != NULL)
			 && (cmnd->device->type == TYPE_DISK))								/* FCLNX-0441 */
			{
				if( test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags) )
				{
					if(ap->scsi_to_retry > 0) {
						cmnd->allowed = ap->scsi_to_retry;						/* FCLNX-0523   */
					}
					else if ( cmnd->allowed < ap->scsi_allowed ){
						cmnd->allowed = ap->scsi_allowed;
					}
				}
				else if ( cmnd->allowed < ap->scsi_allowed ){
					cmnd->allowed = ap->scsi_allowed;
				}
			}
		}
		
#if _HFC_ERROR_INJ
		/*
		 * Pseudo FC Error injection process     
		 */
		if (CMND_TARGET(cmnd) == 0 && CMND_LUN(cmnd) == 1) {
			int fcerr_code = hfc_debug_ioerr_code;

			if (((fcerr_code > 0) && (fcerr_code < 100))
			 && ((uchar)cmnd->cmnd[0] == 0x2A)) {
				static int fcerr_count = 0;
				int fcerr_issue        = 10000;
				int fcerr_num          = 100;

				if (fcerr_count == 0)
					HFC_ERRPRT("hfcldd : Wait I/O Error (fcerr_code=%d, err_max=%d).\n",fcerr_code, fcerr_num);
				
				if ((fcerr_count % 1000) == 0)
					HFC_ERRPRT("fcerr_count = %d /%d.\n",fcerr_count,fcerr_issue);
				
				if (fcerr_count < fcerr_issue)
				{
					fcerr_count++;
				}
				else if (fcerr_count > (fcerr_issue + fcerr_num)) {
					fcerr_count = 0;
				}
				else {
					if (cmnd->result & 0x00ff00ff) {
						HFC_ERRPRT("Unexpected I/O Error. (cmnd->result=0x%08x).\n", cmnd->result);
					}
					else {
						cmnd->result &= 0xFF00FFFF;
						
						switch (fcerr_code) {
							case 1   : cmnd->result |= (DID_NO_CONNECT << 16);	break;
							case 2   : cmnd->result |= (DID_BUS_BUSY << 16);	break;
							case 3   : cmnd->result |= (DID_TIME_OUT << 16);	break;
							case 4   : cmnd->result |= (DID_BAD_TARGET << 16);	break;
							case 5   : cmnd->result |= (DID_ABORT << 16);		break;
							case 6   : cmnd->result |= (DID_PARITY << 16);		break;
							case 7   : cmnd->result |= (DID_ERROR << 16);		break;
							case 8   : cmnd->result |= (DID_RESET << 16);		break;
							case 9   : cmnd->result |= (DID_BAD_INTR << 16);	break;
							case 10  : cmnd->result |= (DID_PASSTHROUGH << 16);	break;
							case 11  : cmnd->result |= (DID_SOFT_ERROR << 16);	break;
							default  : HFC_ERRPRT("Illigal _HFC_ERROR_INJ( value=%d? ).\n",fcerr_code);
						}
						
					}
					fcerr_count++;
				}
			}
		}
#endif																			/* FCLNX-0554 END */

		if ( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags )
		  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags ) 
		  || test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags ) 
		  || test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags ) )
		{
			if ( test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)
			  || test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
				hfc_watchdog_enter( ap, hfcp->target, hfcp, 0, HFC_TARGET_RST_TMR, 0, TRUE );
			else
				hfc_watchdog_enter( ap, hfcp->target, hfcp, 0, HFC_ABORT_TMR, 0, TRUE );
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) {
				hfc_manage_info.npubp->hfc_ioerror_check(cmnd, hfcp);

				if ( !hfc_manage_info.npubp->hfc_check_io_reset_complete(hfcp) ) {	/* FCLNX-0606 */
					return;			/* continue reset process */
				}
			}
			
			if ( !test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) ) {		/* only hsdldd */
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_iodone - CFLAG_ABORT \n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
				clear_bit(CMND_VALID, (ulong *)&cmnd->sc_magic);
#else
				clear_bit(CMND_VALID, (ulong *)&cmnd->eh_eflags);
#endif
				ap->cmnd_cnt--;
				clear_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags);
				ap->pkt_cnt--;

//				if(cmnd->device != NULL) hfc_kfree(ap, cmnd->device);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
//				if(cmnd->cmnd != NULL) hfc_kfree(ap, cmnd->cmnd);
#endif
			}
			else {
				if ( cmnd->scsi_done != NULL )
#ifdef	HFC_DEBUG_IODONE	/* FCLNX-0616 */
					hfc_stra_trace(HFC_TRC_IODONE ,0x11 ,ap ,target , hfcp, (ulong)cmnd, 0, 0);
#endif /* HFC_DEBUG_IODONE */	/* FCLNX-0616 */
					cmnd->scsi_done(cmnd);
			}
		}
		else 
		{
			hfc_watchdog_enter( ap, hfcp->target, hfcp, 0, HFC_SCSI_CMD_TMR, 0, TRUE );
			
			if ( test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags ) )
			{
				HFC_DBGPRT(" hfcldd : hfc_iodone - Skip iodone (CFLAG_TIMEOUT) \n");
				cmnd->result &= 0xff00ffff;
				cmnd->result |= (DID_TIME_OUT << 16);
			}
			else if ( (hfcp->dev != NULL) && ( hfcp->dev->lustat & HFC_WAIT_LUNRSP ) )
			{
				HFC_DBGPRT(" hfcldd : hfc_iodone - Skip iodone (HFC_WAIT_LUNRSP) \n");
				cmnd->result &= 0xff00ffff;
				cmnd->result |= DID_RESET << 16;
			}
			else if ( (hfcp->target != NULL) && test_bit(HFC_WAIT_TGTRSP, (ulong *)&hfcp->target->target_reset) )	/* FCLNX-GPL-0153 *//* FCLNX-GPL-0343 */
			{
				HFC_DBGPRT(" hfcldd : hfc_iodone - Skip iodone (HFC_WAIT_TGTRSP) \n");
				cmnd->result &= 0xff00ffff;
				cmnd->result |= DID_RESET << 16;
			}
			else if ( test_bit( HFC_WAIT_BUSRSP, (ulong *)&ap->status ) )
			{
				HFC_DBGPRT(" hfcldd : hfc_iodone - Skip iodone (HFC_WAIT_BUSRSP) \n");
				cmnd->result &= 0xff00ffff;
				cmnd->result |= DID_RESET << 16;
			}

			if ( hfc_manage_info.hfcldd_mp_mod ) {							/* FCLNX-GPL-204 */
				if (hfc_manage_info.npubp->hfc_mp_iodone(cmnd, ap, hfcp)) {	/* FCLNX-GPL-204 */
					return;													/* FCLNX-GPL-204 */
				}															/* FCLNX-GPL-204 */
				
				write_retries = hfc_manage_info.npubp->hfc_write_retries(dev);	/* FCLNX-GPL-0449 */
			}																/* FCLNX-GPL-204 */
			
																			/* FCLNX-GPL-204 */
			if (cmnd->result && !test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) ) {
				cmd_type = 0;
				command = (uchar)cmnd->cmnd[0];
				switch (command) {
					case WRITE_6               : cmd_type = 3; break;
					case WRITE_FILEMARKS       : cmd_type = 3; break;
					case RECOVER_BUFFERED_DATA : cmd_type = 3; break;
					case COPY                  : cmd_type = 3; break;
					case ERASE                 : cmd_type = 3; break;
					case WRITE_10              : cmd_type = 3; break;
					case WRITE_VERIFY          : cmd_type = 3; break;
					case SYNCHRONIZE_CACHE     : cmd_type = 3; break;
					case COPY_VERIFY           : cmd_type = 3; break;
					case WRITE_BUFFER          : cmd_type = 3; break;
					case WRITE_LONG            : cmd_type = 3; break;
					case WRITE_SAME            : cmd_type = 3; break;
					case WRITE_12              : cmd_type = 3; break;
					case WRITE_VERIFY_12       : cmd_type = 3; break;
					case WRITE_LONG_2          : cmd_type = 3; break;
					
					case INQUIRY               : cmd_type = 1; break;			/* FCLNX-0327 */
					case 0xA0                  : cmd_type = 1; break;			/* FCLNX-0327 */
					case READ_CAPACITY         : cmd_type = 1; break;			/* FCLNX-0327 */
					
					default                    : cmd_type = 0;
				}
				
				if (cmd_type == 1)			/* CHECK COMMAND  */				/* FCLNX-0327 */
				{																/* FCLNX-0327 */
					if (((cmnd->result >> 16) & 0xff) == DID_TIME_OUT )			/* FCLNX-GPL-204 */
					{															/* FCLNX-0327 */
						/* Need command retry */								/* FCLNX-0327 */
						cmnd->result &= 0xff00ffff;								/* FCLNX-0327 */
						cmnd->result |= (DID_ERROR << 16);	/* retry */			/* FCLNX-0327 */
					}															/* FCLNX-0327 */
				}																/* FCLNX-0327 */
				else if ((cmd_type == 3)		/* WRITE COMMAND */				/* FCLNX-0327 */
					&& !test_bit(CFLAG_COMMAND_DEV, (ulong *)&hfcp->cmd_flags)	/* FCLNX-0611 */
					&& !test_bit(CFLAG_NOT_TYPE_DISK, (ulong *)&hfcp->cmd_flags) ) /* FCLNX-GPL-450 */
				{																/* FCLNX-0611 */
					warning_msg = FALSE;										/* FCLNX-0246 */
					retry_internal = FALSE;										/* FCLNX-0246 */
					retryout = FALSE;
					
					if (hfc_manage_info.lg_target_info == NULL)
						wktg = hfcp->target;
					else
						wktg = hfc_hash_target_valid(ap, CMND_TARGET(cmnd));
					
//					if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)])) {
					if (write_retries) {															/* FCLNX-GPL-0449 */
//						if (wktg->write_retries[CMND_LUN(cmnd)] == 0x80) {		/* retry  */
						if (write_retries == 0x80) {							/* retry  */		/* FCLNX-GPL-0449 */
							cmnd->retries = 0;
							cmnd->allowed = 10;
							hfcp->fail = 0;										/* FCLNX-0246 */
						}
//						else if (wktg->write_retries[CMND_LUN(cmnd)] == 0xff);	/* retry(default) */
						else if (write_retries == 0xff);						/* retry(default) *//* FCLNX-GPL-0449 */
						else 
						{
//							int rcnt = wktg->write_retries[CMND_LUN(cmnd)] & 0x7f;
							int rcnt = write_retries & 0x7f;										/* FCLNX-GPL-0449 */
							
							if (cmnd->allowed != rcnt) {						/* FCLNX-0327 */
								if (cmnd->allowed != ap->scsi_allowed) {		/* FCLNX-0327 */
									seq_no = 0x01;																					/* FCLNX-GPL-0412 */
									memset(logdata,0,16);																			/* FCLNX-GPL-0412 */
									memcpy(&logdata[0],(uchar *)&seq_no,1);															/* FCLNX-GPL-0412 */
									hfc_errlog(ap, NULL, hfcp, HFC_ERRLOG_TYPE_IODONE_WARN1, ERRID_HFCP_EVNT3, 0x93, logdata, 16) ;	/* FCLNX-GPL-0412 */
								}
								cmnd->allowed = rcnt;
							}
						}
					}
					
					switch ( (cmnd->result >> 16) & 0xff)
					{
						/* no retry */
						case DID_NO_CONNECT :	/* Couldn't connect before timeout period  */
						case DID_TIME_OUT   :	/* TIMED OUT for other reason              */
							warning_msg = TRUE;									/* FCLNX-0246 */
//							if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)])) {		/* FCLNX-GPL-0412 */
							if (write_retries) {												/* FCLNX-GPL-0412 *//* FCLNX-GPL-0449 */
								retry_internal = TRUE;
								
								if ( hfcp->fail >= cmnd->allowed)
									retryout = TRUE;
							}
							else {
								if (( hfc_manage_info.lg_target_info == NULL ) || ( !hfc_manage_info.hfcldd_mp_mod )) {
									if ( cmnd->retries >= cmnd->allowed )
										retryout = TRUE;
								}
								else {
									retryout = TRUE;
								}
							}																	/* FCLNX-GPL-0412 */
							break;
							
						/* use retries */
						case DID_RESET      :	/* Reset by somebody.                      */
						case DID_BUS_BUSY   :	/* BUS remain busy through time out period */
						case DID_PARITY     :	/* Parity error                            */
						case DID_ERROR      :	/* Internal error                          */
						case DID_SOFT_ERROR :	/* The low level driver just wish a retry  */
							warning_msg = TRUE;									/* FCLNX-0246 */
//							if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)])) {		/* FCLNX-GPL-0412 */
							if (write_retries) {												/* FCLNX-GPL-0412 *//* FCLNX-GPL-0449 */
								retry_internal = TRUE;
								
								if ( hfcp->fail >= cmnd->allowed)
									retryout = TRUE;
							}
							else {
								if (( hfc_manage_info.lg_target_info == NULL ) || ( !hfc_manage_info.hfcldd_mp_mod )) {
									if ( cmnd->retries >= cmnd->allowed )
										retryout = TRUE;
								}
								else {
									if ( hfcp->fail >= cmnd->allowed )
										retryout = TRUE;
								}
							}																	/* FCLNX-GPL-0412 */
							break;
							
						case DID_OK         :	/* Normal end                              */
							if (((cmnd->result & STATUS_MASK)>>1) != CHECK_CONDITION)
								break;
							
							if ( ((cmnd->sense_buffer[2] & 0xf) == NOT_READY)	/* FCLNX-0246 */
							  || ((cmnd->sense_buffer[2] & 0xf) == MEDIUM_ERROR)
							  || ((cmnd->sense_buffer[2] & 0xf) == ABORTED_COMMAND) )
							{
								warning_msg = TRUE;								/* FCLNX-0246 */
//								if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)])) {	/* FCLNX-GPL-0412 */
								if (write_retries) {											/* FCLNX-GPL-0412 *//* FCLNX-GPL-0449 */
									retry_internal = TRUE;
									
									if ( hfcp->fail >= cmnd->allowed)
										retryout = TRUE;
								}
								else {
									if (( hfc_manage_info.lg_target_info == NULL ) || ( !hfc_manage_info.hfcldd_mp_mod )) {
										if ( cmnd->retries >= cmnd->allowed )
											retryout = TRUE;
									}
									else {
										if ( hfcp->fail >= cmnd->allowed )
											retryout = TRUE;
									}
								}																/* FCLNX-GPL-0412 */
							}
							else if ((cmnd->sense_buffer[2] & 0xf) == HARDWARE_ERROR)
							{
								warning_msg = TRUE;								/* FCLNX-0246 */
//								if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)])) {	/* FCLNX-GPL-0412 */
								if (write_retries) {											/* FCLNX-GPL-0412 *//* FCLNX-GPL-0449 */
									retry_internal = TRUE;
									
									if ( hfcp->fail >= cmnd->allowed)
										retryout = TRUE;
								}
								else {
									if (( hfc_manage_info.lg_target_info == NULL ) || ( !hfc_manage_info.hfcldd_mp_mod )) {
										if ( cmnd->retries >= cmnd->allowed )
											retryout = TRUE;
									}
									else {
										retryout = TRUE;
									}
								}																/* FCLNX-GPL-0412 */
							}
							break;
							
						default :
							/* DID_ABORT       */	/* Failed Abort request                    */
							/* DID_BAD_TARGET  */	/* No target						       */
							/* DID_BAD_INTR    */	/* Got an interrupt we did not expect	   */
							/* DID_PASSTHROUGH */	/* Force command past mid-layer            */
							break;
					}
				}																/* FCLNX-0244 END */
				if ( ((cmnd->result >> 16) & 0xff) == DID_TIME_OUT ) {			/* FCLNX-0404 */
//					ap->scsi_timeout_fail++;									/* FCLNX-0404 */ /* FCLNX-GPL-0343 */
					
					if (!ap->scsi_time_out) {									/* FCLNX-GPL-204 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
						if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
							cmnd->result &= 0xff00ffff;
							cmnd->result |= (DID_ERROR << 16);
						}
						else if( test_bit( HFC_CHK_STOP, (ulong *)&ap->status) || test_bit( HFC_ISOL, (ulong *)&ap->status) )
						{
							cmnd->result &= 0xff00ffff;
							cmnd->result |= (DID_NO_CONNECT << 16);
						}
						else if( (hfcp->target != NULL) && test_bit( HFC_SCN_WLINKUP, (ulong *)&hfcp->target->status))
						{	/*** RSCN -> Target lost case ***/
							cmnd->result &= 0xff00ffff;
							cmnd->result |= (DID_ERROR << 16);
						}
						else if( (hfcp->target != NULL) && test_bit(HFC_WAIT_CANCEL, (ulong *)&hfcp->target->status))
						{	/*** PIC scceed case ***/
							cmnd->result &= 0xff00ffff;
							cmnd->result |= (DID_NO_CONNECT << 16);
						}
						else
						{
							cmnd->result &= 0xff00ffff;
							cmnd->result |= (DID_ERROR << 16);
						}
#else
						cmnd->result &= 0xff00ffff;
						cmnd->result |= (DID_ERROR << 16);
#endif
					}
				}
				if (cmd_type == 3)		/* WRITE COMMAND */					/* FCLNX-0327 */
				{
					if (warning_msg == TRUE)									/* FCLNX-0246 STR */
					{
//						if ( ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)]))
						if ( (write_retries)														/* FCLNX-GPL-0449 */
						  || (ap->wmsg && (retryout == TRUE)) ) {									/* FCLNX-GPL-0412 */
//							if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)] == 0x80)) {
							if (write_retries == 0x80) {											/* FCLNX-GPL-0449 */
								seq_no = 0x03;
							}
							else if (retry_internal == TRUE) {
								seq_no = 0x04;
							}
							else {
								seq_no = 0x05;
							}
							memset(logdata,0,16);
							memcpy(&logdata[0],(uchar *)&seq_no,1);	
							hfc_errlog(ap, NULL, hfcp, HFC_ERRLOG_TYPE_IODONE_WARN1, ERRID_HFCP_EVNT3, 0x93, logdata, 16) ;	
						}																			/* FCLNX-GPL-0412 */

						if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {	/* FCLNX-GPL-0449 */
							retry_internal = FALSE;	
							retryout = FALSE;
						}																						/* FCLNX-GPL-0449 */

//						if ( (wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)]) )
						if (write_retries)										/* FCLNX-GPL-0449 */
						{
							if (retryout == TRUE) {
								HFC_ERRPRT(" DONE LAST RETRY. GIVE UP!\n");

//								if ((wktg != NULL) && (wktg->write_retries[CMND_LUN(cmnd)] > 0x80))
								if (write_retries > 0x80)						/* FCLNX-GPL-0449 */
								{
									panic("DONE LAST RETRY");
								}
							}
							else if (retry_internal == TRUE)			/* Enqueue internal retry */
							{
								hfcp->cmd_forw = NULL;
								hfcp->cmd_prev = NULL;
								
								if (ap->retry_hfcp_top == NULL) {
									ap->retry_hfcp_top = hfcp;
									ap->retry_hfcp_end = hfcp;
								}
								else {
									hfcp->cmd_prev = ap->retry_hfcp_end;
									ap->retry_hfcp_end->cmd_forw = hfcp;
									ap->retry_hfcp_end = hfcp;
								}
								ap->retry_hfcp_count++;
								return;
							}
							
						}
					}
				}
			}

			if ( cmnd->scsi_done != NULL ) {
#ifdef	HFC_DEBUG_IODONE	/* FCLNX-0616 */
			hfc_stra_trace(HFC_TRC_IODONE ,0x10 ,ap ,target , hfcp, (ulong)cmnd, 0, 0);
#endif /* HFC_DEBUG_IODONE */	/* FCLNX-0616 */
				cmnd->scsi_done(cmnd);
			}

			if ( !test_bit(CFLAG_HSDLDD_VALID, (ulong *)&hfcp->cmd_flags) )		/* only hsdldd */
				ap->pkt_cnt--;
			
			ap->scsi_end_cnt++;
			HFC_MLPF_STATISTICS_END(ap);	/* FCLNX-GPL-494 */
			clear_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags);
		}
	}
	else
	{
		if ( cmnd->scsi_done != NULL ) {
#ifdef	HFC_DEBUG_IODONE	/* FCLNX-0616 */
			hfc_stra_trace(HFC_TRC_IODONE ,0x12 ,ap ,target , NULL, (ulong)cmnd, 0, 0);
#endif /* HFC_DEBUG_IODONE */	/* FCLNX-0616 */
			cmnd->scsi_done(cmnd);
		}
	}

}

/*
 * Function:    hfc_stra_trace
 *
 * Purpose:     Collection of trace
 *
 * Arguments:   
 *  id         - 
 *  sub_id     - 
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_stra_trace(uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt	*hfcp,
					unsigned long long 	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3)
{
	uchar trc_wk[128];
	ushort	lun=0 ;
	struct trc_com *trc_com ;
	struct scsi_cmnd		*cmnd;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
	struct dev_info		*dev=NULL;
	struct scsi_device	*sdev=NULL;
	struct request_queue	*rq=NULL;
#endif													/* FCLNX-GPL-0343 */

	HFC_BZERO(trc_wk,sizeof(trc_wk)) ;					/* Clear work area */
	trc_com = (struct trc_com *) trc_wk;

	trc_com->id 		= id;
	trc_com->sub_id 	= sub_id ;

	/*-- Target ID/LU setting --*/
	if ( hfcp != NULL )
	{
		cmnd = hfcp->cmd_pkt;
		lun = (ushort)hfcp->lun_id;
		HFC_2L_TO_2B(trc_com->lun_id, lun) ;	/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
		if( cmnd != NULL)
		{
			trc_com->target_id = hfcp->target_id ;	/* FCLNX-0659 */
			trc_com->cmnd = (uchar)cmnd->cmnd[0];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
			sdev = cmnd->device;
			if( sdev != NULL ){
				rq = sdev->request_queue;
				if( rq != NULL ){
					trc_com->timeout = (uchar)(rq->rq_timeout/HZ);
				}
			}
			trc_com->use_sg = (uchar)hfcp->seg_cnt;
#else
			trc_com->timeout = (uchar)(cmnd->timeout_per_command/HZ);
			trc_com->use_sg = (uchar)cmnd->use_sg;
#endif													/* FCLNX-GPL-0343 */
		}
		dev = hfcp->dev;
		if( dev != NULL ){
			trc_com->lunstat			= dev->lustat ;
		}
	}
	if ( target != NULL )
	{
		trc_com->target_id = (uchar)target->target_id;		/* FCLNX-0659 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
//		HFC_2L_TO_2B(trc_com->target_id, tid) ;	/* FCLNX-0659 */
	}

	if ( target != NULL )
	{
		HFC_4L_TO_4B(trc_com->target_status, target->status) ;
		trc_com->target_flags		= target->flags ;
		trc_com->target_pseq		= target->pseq ;
		HFC_2L_TO_2B(trc_com->target_dev_flags, target->device_flags) ;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
		trc_com->lunstat			= target->lustat[lun] ;
#endif
	}

	if ( ap != NULL )
	{
		HFC_4L_TO_4B(trc_com->adap_status, ap->status) ;
		trc_com->open_status = ap->open_status ;
	}

	switch (id)
	{
		case HFC_TRC_STRATEGY :
		case HFC_TRC_ABORT :
		case HFC_TRC_RESET :
		case HFC_TRC_BUS_RESET :
		case HFC_TRC_ISSUE_TMGM :
		case HFC_TRC_RST_NF :
		case HFC_TRC_RST_CBACK :
		case HFC_TRC_RST_FREE :
		case HFC_TRC_IODONE :
			hfc_stra_trc1(trc_wk,id,sub_id,ap,target,hfcp,etc1,etc2,etc3);
			break;

		case HFC_TRC_START :
			hfc_stra_trc2(trc_wk,id,sub_id,ap,target,hfcp,etc1,etc2,etc3);
			break;

		case HFC_TRC_CAN_XOB :
		case HFC_TRC_CAN_WE :
		case HFC_TRC_CAN_WX :
			hfc_stra_trc3(trc_wk,id,sub_id,ap,target,hfcp,etc1,etc2,etc3);
			break;

		case HFC_TRC_RES_CHK :
		case HFC_TRC_IOVUP :
		case HFC_TRC_DMA_MAP :
			hfc_stra_trc4(trc_wk,id,sub_id,ap,target,hfcp,etc1,etc2,etc3);
			break;

		case HFC_TRC_MK_CMD :
		case HFC_TRC_ENQ_XOB :
		case HFC_TRC_ENQ_WX :
			hfc_stra_trc5(trc_wk,id,sub_id,ap,target,hfcp,etc1,etc2,etc3);
			break;
	}

	hfc_trace(ap,id,&trc_wk[1],0) ;					/* Trace output */
}


/*
 * Function:    hfc_stra_trc1
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_stra_trc1( uchar	 *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3)
{
	struct stra_trc1 *trc1 = (struct stra_trc1 *)trc_wk;
	struct scsi_cmnd *cmnd;
	uint	pkt_time=0;
	uint    a_scsi_id;
	uint    d_scsi_id;
	uint	ser_num;
	ushort wx_que_cnt=0, we_que_cnt=0;	/* FCLNX-0659 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
	struct scsi_device	*sdev=NULL;
	struct request_queue	*rq=NULL;
#endif													/* FCLNX-GPL-0343 */


	/*-- etc1 : s_bp	  --*/
	/*-- etc2 : type flag --*/
	/*-- etc3 : done flag --*/
	if ( ap != NULL )
	{
		a_scsi_id = (uint)ap->scsi_id;
		HFC_4L_TO_4B(trc1->a_scsi_id, a_scsi_id) ;				/* adaptor scsi_id */
		HFC_8L_TO_8B(trc1->scsi_exec_cnt, ap->scsi_exec_cnt) ;	/* Number of SCSI activate requests */
		HFC_8L_TO_8B(trc1->scsi_end_cnt, ap->scsi_end_cnt) ;	/* Number of SCSI responses */
		HFC_8L_TO_8B(trc1->scsi_err_cnt, ap->scsi_err_cnt) ;	/* Number of abnormal terminations */ 
																/*  of scsi command */
	}

	if ( target != NULL )
	{
		d_scsi_id = (uint)target->scsi_id;
		HFC_4L_TO_4B(trc1->d_scsi_id, d_scsi_id) ;				/* target  scsi_id */
		wx_que_cnt = (ushort)target->wx_que_cnt;								/* FCLNX-0659 */
		HFC_2L_TO_2B(trc1->wx_que_cnt, wx_que_cnt) ;	/* Scsi wx_que count */ /* FCLNX-0659 */
		we_que_cnt = (ushort)target->we_que_cnt;								/* FCLNX-0659 */
		HFC_2L_TO_2B(trc1->we_que_cnt, we_que_cnt) ;	/* Scsi we_que count*/	/* FCLNX-0659 */
	}

	if ( hfcp != NULL )
	{
		cmnd = hfcp->cmd_pkt;
//		trc1->cmd_status	= hfcp->cmd_pkt->result ;			/* hfc_pkt Status information */
		trc1->cmd_xob = hfcp->cmd_xob;							/* hfc_pkt allocation xob# */	/* FCLNX-0659 */
//		trc1->cmd_timeout	= hfcp->cmd_timeout ;				/* hfc_pkt timeout time */
		HFC_4L_TO_4B(trc1->cmd_flags, hfcp->cmd_flags) ;		/* hfc_pkt control flag  */
		HFC_4L_TO_4B(trc1->cmd_iov_no, hfcp->iov_no) ;			/* hfc_pkt - seg_info#	*/
		HFC_4L_TO_4B(trc1->cmd_iov_cnt, hfcp->iov_cnt) ;		/* hfc_pkt - number of seg_info */
		HFC_4L_TO_4B(trc1->adap_status, hfcp->adap_status) ;	/* hfc_pkt adap_status */
		if( cmnd != NULL )
		{
			trc1->pkt_flag = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)			/* FCLNX-GPL-0343 */
			sdev = cmnd->device;
			if( sdev != NULL ){
				rq = sdev->request_queue;
				if( rq != NULL ){
					pkt_time = (rq->rq_timeout/HZ);
				}
			}
			HFC_4L_TO_4B(trc1->pkt_resid, cmnd->sdb.resid) ;	/* struct scsi_cmnd remainder byte */
#else
			pkt_time = cmnd->timeout_per_command/HZ;
			HFC_4L_TO_4B(trc1->pkt_resid, cmnd->resid) ;	/* struct scsi_cmnd remainder byte */
#endif														/* FCLNX-GPL-0343 */
			HFC_4L_TO_4B(trc1->pkt_time, pkt_time) ;		/* struct scsi_cmnd time set value */


//			HFC_4L_TO_4B(trc1->pkt_state, cmnd->state) ;	/* struct scsi_cmnd state */ /* FCLNX-0308 */
			ser_num = (uint)cmnd->serial_number;
			HFC_4L_TO_4B(trc1->serial_number, ser_num) ;	/* struct scsi_cmnd serial_number */
			HFC_4L_TO_4B(trc1->pkt_result, cmnd->result) ;	/* struct scsi_cmnd result */
		}
	}
}


/*
 * Function:    hfc_stra_trc2
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_stra_trc2( uchar	*trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3)
{

	struct stra_trc2 *trc2 = (struct stra_trc2 *)trc_wk;
	uint 	i;
	uint a_scsi_id, d_scsi_id;
	uint 	timeout=0;

	ushort wx_que_cnt=0, we_que_cnt=0;	/* FCLNX-0659 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	struct scsi_device	*sdev=NULL;
	struct request_queue	*rq=NULL;
#endif												/* FCLNX-GPL-0343 */

	/*-- etc1 : cmnd --*/
	/*-- etc2 : NULL --*/
	/*-- etc3 : NULL --*/
	if( ap != NULL )
	{
		a_scsi_id = (uint)ap->scsi_id;
		HFC_2L_TO_2B(trc2->xob_no, ap->xob_no) ;
		trc2->xob_exec_cnt 		= ap->xob_exec_cnt ;
		trc2->xob_wait_exec_cnt = ap->xob_wait_exec_cnt ;
		HFC_4L_TO_4B(trc2->iov_no, ap->iov_no) ;
		HFC_4L_TO_4B(trc2->iov_map_cnt, ap->iov_map_cnt) ;
		i = ap->frame_inp;
		if(i == 0)
			i = MAX_FRAME_CNT-1;
		else i--;
		HFC_4L_TO_4B(trc2->save_xob_outp, ap->save_xob_outp[ i ]) ;
		HFC_4L_TO_4B(trc2->xob_outp_end, ap->xob_outp_end[ i ]) ;
		trc2->xob_outp		= ap->fw_init_p->xob_outp;
		trc2->xob_inp		= ap->fw_init_p->xob_inp;
		HFC_8L_TO_8B(trc2->new_login_need, ap->login_target) ;
		HFC_2L_TO_2B(trc2->next_dstart_cnt, ap->next_dstart_cnt) ;			/* FCLNX-0659 */
		HFC_4L_TO_4B(trc2->frame_chkp, ap->frame_chkp) ;
		i = ap->frame_inp;
		if(i == 0)
			i = MAX_FRAME_CNT-1;
		else i--;
		HFC_4L_TO_4B(trc2->frame_inp, i) ;
	}
	if ( target != NULL )
	{
		d_scsi_id = (uint)target->scsi_id;
		HFC_4L_TO_4B(trc2->d_scsi_id, d_scsi_id) ;		/* target  scsi_id 		*/
		wx_que_cnt = (ushort)target->wx_que_cnt;										/* FCLNX-0659 */
		HFC_2L_TO_2B(trc2->wx_que_cnt, wx_que_cnt) ;	/* scsi Start waiting count */ 	/* FCLNX-0659 */
		we_que_cnt = (ushort)target->we_que_cnt;										/* FCLNX-0659 */
		HFC_2L_TO_2B(trc2->we_que_cnt, we_que_cnt) ;	/* scsi Response waiting count */	/* FCLNX-0659 */
	}

	if ( hfcp != NULL )
	{
//		trc2->cmd_status	= hfcp->cmd_status ;	/* hfc_pkt Status information*/
		trc2->cmd_xob = hfcp->cmd_xob;		/* hfc_pkt Allocation xob # 	*/		/* FCLNX-0659 */
		if( hfcp->cmd_pkt != NULL){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)			/* FCLNX-GPL-0343 */
			sdev = hfcp->cmd_pkt->device;
			if( sdev != NULL ){
				rq = sdev->request_queue;
				if( rq != NULL ){
					timeout = (rq->rq_timeout/HZ);
				}
			}
#else
			timeout = hfcp->cmd_pkt->timeout_per_command/HZ;
#endif
			HFC_4L_TO_4B(trc2->cmd_timeout, timeout) ;	/* hfc_pkt timeout time 	*/

			trc2->cmnd = (uchar)hfcp->cmd_pkt->cmnd[0];
		}
//		trc2->cmd_timeout	= (uint64_t)hfcp->cmd_timeout.dog.expires ;	/* hfc_pkt timeout time 	*/
		HFC_4L_TO_4B(trc2->cmd_flags, hfcp->cmd_flags) ;		/* hfc_pkt control flag  	*/
		HFC_4L_TO_4B(trc2->cmd_iov_no, hfcp->iov_no) ;			/* hfc_pkt - seg_info#	*/
		HFC_4L_TO_4B(trc2->cmd_iov_cnt, hfcp->iov_cnt) ;		/* hfc_pkt - number of seg_info */
		HFC_4L_TO_4B(trc2->cmd_data_size, hfcp->data_size) ;	/* hfc_pkt - data_size  */
		HFC_4L_TO_4B(trc2->adap_status, hfcp->adap_status) ;	/* hfc_pkt adap_status 	*/
	}
}


/*
 * Function:    hfc_stra_trc3
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_stra_trc3( uchar	*trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3)
{
	struct stra_trc3 *trc3 = (struct stra_trc3 *)trc_wk;
	uint adap_status=0;									/* FCLNX-0659 */

	/*-- etc1 : NULL --*/
	/*-- etc2 : type --*/
	/*-- etc3 : NULL --*/
	if( ap != NULL )
	{
		HFC_8L_TO_8B(trc3->a_scsi_id, ap->scsi_id) ;				/* adaptor scsi_id */
		trc3->xob_outp = ap->fw_init_p->xob_outp;
		trc3->xob_inp	= ap->fw_init_p->xob_inp ;
	}
	if( target != NULL )
	{
		HFC_8L_TO_8B(trc3->d_scsi_id, target->scsi_id) ;			/* target  scsi_id */
		HFC_4B_TO_4L(trc3->we_que_cnt, target->we_que_cnt) ;		/* v1.35   */
		HFC_4B_TO_4L(trc3->wx_que_cnt, target->wx_que_cnt) ;		/* v1.35   */
	}

	if( hfcp != NULL )
	{
	}

	trc3->type			= etc1 ;							/* cancel type 	*/
	trc3->lun			= etc2 ;							/* lun#		 	*/
	adap_status = (uint)etc3;								/* FCLNX-0659 */
	HFC_4B_TO_4L(trc3->adap_status, adap_status) ;			/* adap_status	*/ /* FCLNX-0659 */

}


/*
 * Function:    hfc_stra_trc4
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_stra_trc4( uchar	*trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3)
{
	struct stra_trc4 *trc4 = (struct stra_trc4 *)trc_wk;
	uint	a_scsi_id;
	int		i;
	uint	pos=0,cnt=0;	/* FCLNX-0659 */
	ushort	cmd_xob = 0;	/* FCLNX-0659 */
	ushort wx_que_cnt=0, we_que_cnt=0;	/* FCLNX-0659 */

	/*-- etc1 : s_bp/type/s_bp		--*/
	/*-- etc2 : NULL/pos/rc 		--*/
	/*-- etc3 : NULL/cnt/NULL		--*/
	if( ap != NULL )
	{
		
		a_scsi_id = (uint)ap->scsi_id;
		HFC_4L_TO_4B(trc4->a_scsi_id, a_scsi_id);			/* adaptor scsi_id */
		trc4->xob_outp		= ap->fw_init_p->xob_outp;
		trc4->xob_inp		= ap->fw_init_p->xob_inp ;
		trc4->xob_exec_cnt 		= ap->xob_exec_cnt ;
		trc4->xob_wait_exec_cnt = ap->xob_wait_exec_cnt ;
		HFC_4B_TO_4L(trc4->xob_no, ap->xob_no);
		HFC_4B_TO_4L(trc4->iov_no, ap->iov_no);
		HFC_4B_TO_4L(trc4->iov_map_cnt, ap->iov_map_cnt);
		i = ap->frame_inp;
		if(i == 0)
			i = MAX_FRAME_CNT-1;
		else i--;
		HFC_4L_TO_4B(trc4->save_xob_outp, ap->save_xob_outp[ i ]) ;
		HFC_4L_TO_4B(trc4->xob_outp_end, ap->xob_outp_end[ i ]) ;
		HFC_4L_TO_4B(trc4->frame_chkp, ap->frame_chkp) ;
		i = ap->frame_inp;
		if(i == 0)
			i = MAX_FRAME_CNT-1;
		else i--;
		HFC_4L_TO_4B(trc4->frame_inp, i) ;
	}

	if ( target != NULL )
	{
		wx_que_cnt = (ushort)target->wx_que_cnt;										/* FCLNX-0659 */
		HFC_2L_TO_2B(trc4->wx_que_cnt, wx_que_cnt) ;	/* scsi start waiting count */ 	/* FCLNX-0659 */
		we_que_cnt = (ushort)target->we_que_cnt;										/* FCLNX-0659 */
		HFC_2L_TO_2B(trc4->we_que_cnt, we_que_cnt) ;	/* scsi Response waiting count */	/* FCLNX-0659 */
	}

	if ( hfcp != NULL )
	{
		HFC_4B_TO_4L(trc4->hfcp_iov_no, hfcp->iov_no) ;
		HFC_4B_TO_4L(trc4->hfcp_iov_cnt, hfcp->iov_cnt) ;
		HFC_4B_TO_4L(trc4->cmd_flags, hfcp->cmd_flags) ;		/* hfc_pkt control flag  */
		HFC_4B_TO_4L(trc4->cmd_dmacount, hfcp->data_size) ;		/* Data transfer length	   */
		HFC_4B_TO_4L(trc4->cmd_cookiecnt, hfcp->seg_cnt) ;		/* cookie count 	   */
		cmd_xob	= (ushort)hfcp->cmd_xob;						/* FCLNX-0659 */
		HFC_2B_TO_2L(trc4->xob_no, cmd_xob) ;		/* hfc_pkt Allocation xob # */ /* FCLNX-0659*/
	}

	if (id == HFC_TRC_IOVUP)
	{
		trc4->type = etc1 ;							/* 0:alloc,1:cancel		*/
		pos = (uint)etc2;												       /* FCLNX-0659 */
		cnt = (uint)etc3;													   /* FCLNX-0659 */
		HFC_4B_TO_4L(trc4->pos, pos) ;				/* bit position 		*/ /* FCLNX-0659 */
		HFC_4B_TO_4L(trc4->cnt, cnt) ;				/* bit count			*/ /* FCLNX-0659 */
	}

	if (id == HFC_TRC_RES_CHK)
	{
		pos = (uint)etc2;												       /* FCLNX-0659 */
		cnt = (uint)etc3;													   /* FCLNX-0659 */
		HFC_4B_TO_4L(trc4->pos, pos) ;							/* xob_out_num	 		*/ /* FCLNX-0659 */
		HFC_4B_TO_4L(trc4->cnt, cnt) ;							/* xob_in_num			*/ /* FCLNX-0659 */
	}
}


/*
 * Function:    hfc_stra_trc5
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info 
 *  hfcp       - Pointer to hfc_pkt 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_stra_trc5( uchar	*trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	adap_info *ap,
					struct	target_info *target,
					struct	hfc_pkt *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3)
{
	struct stra_trc5 *trc5 = (struct stra_trc5 *)trc_wk;
	ushort	xob_no ;
	uint	d_scsi_id;
	ushort wx_que_cnt=0, we_que_cnt=0;	/* FCLNX-0659 */

	/*-- etc1 : NULL	  --*/
	/*-- etc2 : type --*/
	/*-- etc3 : NULL --*/
	if( ap != NULL )
	{
		trc5->xob_outp			= ap->fw_init_p->xob_outp ;
		HFC_2B_TO_2L(trc5->xob_no, ap->xob_no) ;
		trc5->xob_exec_cnt		= ap->xob_exec_cnt ;
		trc5->xob_wait_exec_cnt = ap->xob_wait_exec_cnt ;

		xob_no = ap->xob_no ;
		if (sub_id == 0x10)				
		{								
			if( xob_no == 0 )
				xob_no = ap->xob_max - 1 ;
			else
				xob_no-- ;
		}

		HFC_4B_TO_4L(trc5->fcp_cntl, *(uint *) &(ap->xob[xob_no].fcp_cmd.fcp_cntl)) ;
		HFC_4B_TO_4L(trc5->fcp_dl, ap->xob[xob_no].fcp_cmd.fcp_dl) ;
		HFC_MEMCPY(trc5->cdb,ap->xob[xob_no].fcp_cmd.fcp_cdb,16) ;
		HFC_MEMCPY(trc5->xob,(uchar *)&ap->xob[xob_no],48) ;
	}

	if ( target != NULL )
	{
		d_scsi_id = (uint)target->scsi_id;
		HFC_4B_TO_4L(trc5->d_scsi_id, d_scsi_id) ;		/* target  scsi_id		*/
		wx_que_cnt = (ushort)target->wx_que_cnt;										/* FCLNX-0659 */
		HFC_2L_TO_2B(trc5->wx_que_cnt, wx_que_cnt) ;	/* scsi start waiting count */ 	/* FCLNX-0659 */
		we_que_cnt = (ushort)target->we_que_cnt;										/* FCLNX-0659 */
		HFC_2L_TO_2B(trc5->we_que_cnt, we_que_cnt) ;	/* scsi response waiting count */	/* FCLNX-0659 */

	}
}


