/*
 * hfcl_strategy_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char stra_fx_rcsid[] = "$Id: hfcl_strategy_fx.c,v 1.1.2.30.2.16.2.4.2.9.2.1 2016/02/19 03:05:30 mhayashi Exp $";

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

/*-- global variable --*/


/*-- static --*/
static int hfc_fx_resource_chk(struct port_info *pp, 
	struct core_info	*core,
	struct hfc_pkt_fx	*hfcp);

static void hfc_fx_get_free_iov(struct core_info *core ,
	uint st_word,
	uint st_bit,
	uint req_bit_cnt,
	struct	free_iov_map *fmap);

static void hfc_fx_iov_update(struct core_info *core ,
	uint pos ,
	uint cnt ,
	uchar type);

static void hfc_fx_dma_map( struct port_info *pp,
	struct core_info	*core,
	struct hfc_pkt_fx	*hfcp );

static void hfc_fx_make_cmdiu( struct port_info *pp, 
	struct hfc_pkt_fx	*hfcp );

static void hfc_fx_xob_enque(struct port_info *pp, 
	struct core_info *core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp);

static void hfc_fx_cancel_tskmgm(struct port_info *pp,
	struct target_info_fx	*target,
	uint					lun,		/* FCLNX-GPL-0343 */
	struct hfc_pkt_fx		*hfcp,
	uint					adap_status,
	uint					type);

static void hfc_fx_stra_trace(uchar id,
					uchar	sub_id,
					struct	port_info *pp,
					struct	region_info *rp,
					struct	core_info *core,
					struct	target_info_fx *target,
					struct	hfc_pkt_fx *hfcp,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_fx_stra_trc1( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	port_info *pp,
					struct	region_info *rp,
					struct	core_info *core,
					struct	target_info_fx *target,
					struct	hfc_pkt_fx *hfcp,
					struct	scsi_cmnd *cmnd,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_fx_stra_trc2( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	port_info *pp,
					struct	region_info *rp,
					struct	core_info *core,
					struct	target_info_fx *target,
					struct	hfc_pkt_fx *hfcp,
					struct	scsi_cmnd *cmnd,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_fx_stra_trc3( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	port_info *pp,
					struct	region_info *rp,
					struct	core_info *core,
					struct	target_info_fx *target,
					struct	hfc_pkt_fx *hfcp,
					struct	scsi_cmnd *cmnd,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);
static void hfc_fx_stra_trc4( uchar   *trc_wk,
					uchar	id,
					uchar	sub_id,
					struct	port_info *pp,
					struct	region_info *rp,
					struct	core_info *core,
					struct	target_info_fx *target,
					struct	hfc_pkt_fx *hfcp,
					struct	scsi_cmnd *cmnd,
					unsigned long long	etc1,
					unsigned long long	etc2,
					unsigned long long	etc3);

#if _HFC_ERROR_INJ									/* FCLNX-0246 */
extern int hfc_fx_debug_ioerr_code;					/* FCLNX-0246 */
#endif												/* FCLNX-0246 */

extern struct narrow_dev hfc_narrow_dev;			/* FCLNX-0392 */
//uchar	logdata[16] ;


/*
 * Function:    hfc_fx_strategy
 *
 * Purpose:     Initiate SCSI command 
 *
 * Arguments:   
 *  cmnd		Pointer to Scsi_Cmnd
 *  iodone		Pointer to done() (Return SCSI response)
 *
 * Returns:     
 *  			0 :  Completed SCSI initiation successfully 
 *  			EIO : Adpp_info or dev_info_fx does not exist or invalid.
 * 				(Only for ioctl)
 *
 * Notes:       Caller should be in process level or interruption level.
 */
int hfc_fx_strategy(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
{
	if (!hfc_manage_info.hfcldd_mp_mod) {
		return ( hfc_fx_strategy_pg(cmnd,iodone) );
	} else {
		return ( hfc_manage_info.npubp->hfc_fx_mp_strategy(cmnd,iodone) );
	}
}

int hfc_fx_eh_abort(struct scsi_cmnd *cmnd)
{
	if (!hfc_manage_info.hfcldd_mp_mod) {
		return ( hfc_fx_eh_abort_pg(cmnd) );
	} else {
		return ( hfc_manage_info.npubp->hfc_fx_mp_abort(cmnd) );
	}
}

int hfc_fx_eh_device_reset(struct scsi_cmnd *cmnd)
{
	if (!hfc_manage_info.hfcldd_mp_mod) {
		return ( hfc_fx_eh_device_reset_pg(cmnd) );
	} else {
		return ( hfc_manage_info.npubp->hfc_fx_mp_device_reset(cmnd) );
	}
}

int hfc_fx_eh_target_reset(struct scsi_cmnd *cmnd)					/* FCLNX-GPL-0343 */
{
	if (!hfc_manage_info.hfcldd_mp_mod) {
		return ( hfc_fx_eh_target_reset_pg(cmnd) );
	} else {
		return ( SUCCESS );
	}
}																/* FCLNX-GPL-0343 */

int hfc_fx_eh_bus_reset(struct scsi_cmnd *cmnd)
{
	if (!hfc_manage_info.hfcldd_mp_mod) {
		return ( hfc_fx_eh_bus_reset_pg(cmnd) );
	} else {
		return ( hfc_manage_info.npubp->hfc_fx_mp_bus_reset(cmnd) );
	}
}

int hfc_fx_strategy_pg(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
{
	struct	port_info		*pp, *org_pp;
	struct	hfc_pkt_fx		*hfcp=NULL;
	struct	dev_info_fx		*dev;
	struct	region_info		*rp;
	struct	request			*req;
	struct	core_info		*core=NULL;
	struct	request_queue	*rq=NULL;
	struct	scsi_device		*sdev=NULL;

	int		func_rc = 0;
	uint	result, ioctl_mode = 0;
	uint	timeout=60*HZ;
	unsigned long	flags = 0;
	uint64_t		time = 0;
	uint			cpuno = 0;
	uint			queue_no = 0;
	struct Scsi_Host  *s_host = cmnd->device->host;		/* FCLNX-GPL-FX-445 */
	
	pp = (struct port_info *) CMND_HOSTDATA(cmnd);
	/* kernel 5.16+: scsi_done member removed */
	if( iodone == (void *) hfc_fx_ioctl_iodone ) ioctl_mode=1;
	cmnd->result = 0;

	if( pp == NULL ) 
	{
		
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		if(hfcp != NULL)
			hfcp->adap_status = SCS_NO_ADAPINFO;
		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - pp==NULL-error\n");

		if (iodone) iodone(cmnd); else scsi_done(cmnd);	/* kernel 5.16+ */
		return(func_rc);
	}
	
	/* workaround */
	if (HFC_FX_MQ_VALID(pp)) {
		/* select queue */
		org_pp = pp;
		if (smp_processor_id()) {
			queue_no = smp_processor_id()%org_pp->mq_num;
		}
		pp = org_pp->vport_ptr[queue_no].vport_arg;
		if (pp == NULL) {
			pp = org_pp;
		}
		else if (!test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)) {	/* FCLNX-GPL-FX-246,272 */
			pp = org_pp;
		}
	}

	hfcp = (struct hfc_pkt_fx *)cmnd->host_scribble;
	if(hfcp != NULL)
	{
		if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_VALID, hfcp))
		{
			HFC_DBGPRT("hfcldd%d : hfc_fx_strategy - scsi_cmnd retried\n", pp->dev_minor);
			return(0);
		}
	}

	scsi_set_resid(cmnd, 0);
	req = scsi_cmd_to_rq(cmnd);	/* kernel 5.16+: cmnd->request removed */
	sdev = cmnd->device;
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

	/* get processor_id and timestamp */
	if ( (pp->pm_control == HFC_FX_PM_ON) 
		|| (pp->core_control == HFC_FX_CORECTL_CPU_TO_CORE)
		|| (pp->cpu_map == HFC_VEC_CPU_MAP_ENABLE ) ){	/* FCLNX-GPL-FX-193 */
		cpuno = smp_processor_id();
		rdtscll(time);
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);

	rp = pp->region_arg[pp->rid];

	if( rp == NULL ) 
	{
		if(ioctl_mode == 1) func_rc=EIO;
		result = DID_ERROR;
		cmnd->result |= ( result << 16);
		if(hfcp != NULL)
			hfcp->adap_status = SCS_NO_ADAPINFO;
		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - rp==NULL-error\n");

		cmnd->scsi_done(cmnd);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return(func_rc);
	}

	hfcp = hfc_fx_get_new_hfcp(pp);
	if( hfcp == NULL){
		
		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - hfcp==NULL-error\n");
		
		pp->scsi_exec_cnt++;
		pp->hfcpkt_full_cnt++;
		/* FCLNX-GPL-FX-312,335 */
		func_rc = SCSI_MLQUEUE_HOST_BUSY;
		if(ioctl_mode == 1)
		{	/* for ioctl_mode */
			func_rc=EIO;
			hfc_fx_set_cmnd_res(pp, NULL, cmnd, NULL, DID_BUS_BUSY);
			hfc_fx_iodone(pp, NULL, cmnd, NULL);
		}
		/* FCLNX-GPL-FX-312,335 */
		
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return(func_rc);/* FCLNX-GPL-FX-335 */
	}
	
	dev = (struct dev_info_fx *)CMND_DEV(cmnd);

//	memset(hfcp, 0, sizeof(struct hfc_pkt_fx));
//	set_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags );
	
	/* get processor_id */
	if (pp->pm_control == HFC_FX_PM_ON) {
		if (hfcp->pm_pkt_no != 0xffff) {
			pp->pm_pkt_pool[hfcp->pm_pkt_no].tsc_strategy = time;
			pp->pm_pkt_pool[hfcp->pm_pkt_no].cpuno_strategy = cpuno;
			pp->pm_pkt_pool[hfcp->pm_pkt_no].cdb = cmnd->cmnd[0];
		}
	}
	
	hfcp->cmd_pkt   = cmnd;
	hfcp->target_id = CMND_TARGET(cmnd);
	hfcp->lun_id    = CMND_LUN(cmnd);
	hfcp->pp        = pp;
	hfcp->target    = hfc_fx_hash_target_valid(pp, CMND_TARGET(cmnd));
	hfcp->dev 		= dev;
	hfcp->timeout	= timeout;
	hfcp->rp		= rp;
	hfcp->rid		= rp->rid;
//	if( pp->core_control == HFC_FX_CORECTL_CPU_TO_CORE ){
		hfcp->cpu_no	= (ushort)cpuno;
//	}

	cmnd->host_scribble = (void *)hfcp;

	func_rc = hfc_fx_strategy_port(hfcp, -1); /* FCLNX-GPL-FX-266 */
	if (func_rc) {
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return (func_rc);
	}
	
	if (hfcp->core) {
		core = hfcp->core;
		HFC_CORELOCK(core);
		HFC_PORTUNLOCK(pp);
		if (!ioctl_mode) {
			if (!(pp->debug_func & HFC_FX_DEBUG_HOST_LOCK_OFF)) {
				spin_unlock(s_host->host_lock);		/* FCLNX-GPL-FX-445 */
			}
		}
		hfc_fx_strategy_core(hfcp);
		HFC_COREUNLOCK_IRQRESTORE(core,flags);
		if (!ioctl_mode) {
			if (!(pp->debug_func & HFC_FX_DEBUG_HOST_LOCK_OFF)) {
				spin_lock(s_host->host_lock);		/* FCLNX-GPL-FX-445 */
			}
		}
	}
	else {
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	}
	
	return (func_rc);
}


static inline struct socket_info* hfc_fx_get_socket_info(struct port_info *pp, uint cpu_no){
	int i;
	struct socket_info *socket_info = NULL;
	struct socket_info *socket_info_wk = NULL;

	for(i=0; i < pp->manage_info->socket_num; i++){
		if((socket_info_wk=pp->manage_info->socket_info+i)==NULL){
			continue;
		}
		if(!cpumask_test_cpu(cpu_no,&socket_info_wk->cpumask)){
			continue;
		}
		socket_info = socket_info_wk;
		break;
	}
	return socket_info;
}


uchar hfc_fx_choose_core_minque(struct port_info * pp, struct hfc_pkt_fx *hfcp, uchar *use_core_list)
{
	uchar core_no = 0, wk_no = 0;
	int i = 0;
	struct socket_info *socket_info_wk = NULL;	/* FCLNX-GPL-FX-420 */
	uchar socket_number=0;
	
	core_no = pp->curr_core; // Set previously used core number to core_no
	
	if((pp->cpu_map == HFC_VEC_CPU_MAP_ENABLE) && (pp->socket_num > 1) && (pp->core_control == HFC_FX_CORECTL_ENHANCE_RR) ){	/* FCLNX-GPL-FX-420 */
		/* Select queue based on Socket# which issued I/O command */
		socket_info_wk = hfc_fx_get_socket_info(pp, hfcp->cpu_no);
		if(socket_info_wk != NULL){
			socket_number = socket_info_wk->socket_no;
		}
		if (pp->pkg.port == 2) { // 2port card (2core/port)
			core_no = (socket_number % pp->core_num );
			if( pp->core_num == 2 ){
				if( core_no ) {
					core_no = 2;
				}
				else{
					core_no = 0;
				}
			}
			if( hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[core_no]) ) {
				core_no = pp->master_core_no;
			}
		} else { // 1port card (4core/port)
			//search next online core
			if( !(socket_number % pp->core_num ) ){ /* socket number 0 */
				core_no = pp->numa_node0_curr_core;
				for (i=0; i< (MAX_CORE_PROBE_FX/2); i++) {
					core_no++;
					if (core_no >= (MAX_CORE_PROBE_FX/2)) { 
						core_no = 0;
					}
					if ( (use_core_list != NULL) && (use_core_list[core_no] == 0) )
						continue;
					
					if( !hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[core_no]) )
						break;
				}

				// search minmum we-que core
				wk_no = core_no;
				for (i=0; i< ((MAX_CORE_PROBE_FX/2) - 1); i++) {
					wk_no++;
					if (wk_no >= (MAX_CORE_PROBE_FX/2)) { 
						wk_no = 0;
					}

					if( hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[wk_no]) )
						continue;

					if ( (use_core_list != NULL) && (use_core_list[wk_no] == 0) )
						continue;
					
					if (hfcp->rp->core_arg[core_no]->we_que_cnt_all >
						hfcp->rp->core_arg[wk_no]->we_que_cnt_all) {
						core_no = wk_no;
					}
				}
				pp->numa_node0_curr_core = core_no;
			}
			else{	/* socket number 1 */
				core_no = pp->numa_node1_curr_core;
				for (i=(MAX_CORE_PROBE_FX/2); i< MAX_CORE_PROBE_FX; i++) {
					core_no++;
					if (core_no >= MAX_CORE_PROBE_FX) { 
						core_no = (MAX_CORE_PROBE_FX/2);
					}
					if ( (use_core_list != NULL) && (use_core_list[core_no] == 0) )
						continue;
					
					if( !hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[core_no]) )
						break;
				}

				// search minmum we-que core
				wk_no = core_no;
				for (i=(MAX_CORE_PROBE_FX/2); i< (MAX_CORE_PROBE_FX - 1); i++) {
					wk_no++;
					if (wk_no >= MAX_CORE_PROBE_FX) { 
						wk_no = MAX_CORE_PROBE_FX/2;
					}

					if( hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[wk_no]) )
						continue;

					if ( (use_core_list != NULL) && (use_core_list[wk_no] == 0) )
						continue;
					
					if (hfcp->rp->core_arg[core_no]->we_que_cnt_all >
						hfcp->rp->core_arg[wk_no]->we_que_cnt_all) {
						core_no = wk_no;
					}
				}
				pp->numa_node1_curr_core = core_no;
			}
		} // 1port card (4core/port)
	}
	else{
		if (pp->pkg.port == 2) { // 2port card (2core/port)
			if (use_core_list != NULL) {
				core_no = (use_core_list[0] == 1) ? 0 : 2;
			}
			else {
				wk_no = (core_no == 0 ? 2 : 0);
				if (hfcp->rp->core_arg[core_no]->we_que_cnt_all >=
					hfcp->rp->core_arg[wk_no]->we_que_cnt_all)
				{
					core_no = wk_no;
				}
			}
			
		} else { // 1port card (4core/port)
			//search next online core
			for (i=0; i< MAX_CORE_PROBE_FX; i++) {
				core_no++;
				if (core_no >= MAX_CORE_PROBE_FX) { 
					core_no = 0;
				}
				if ( (use_core_list != NULL) && (use_core_list[core_no] == 0) )
					continue;
				
				if( !hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[core_no]) )
					break;
			}

			// search minmum we-que core
			wk_no = core_no;
			for (i=0; i< (MAX_CORE_PROBE_FX - 1); i++) {
				wk_no++;
				if (wk_no >= MAX_CORE_PROBE_FX) { 
					wk_no = 0;
				}

				if( hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[wk_no]) )
					continue;

				if ( (use_core_list != NULL) && (use_core_list[wk_no] == 0) )
					continue;
				
				if (hfcp->rp->core_arg[core_no]->we_que_cnt_all >
					hfcp->rp->core_arg[wk_no]->we_que_cnt_all) {
					core_no = wk_no;
				}
			}
		} // 1port card (4core/port)
	}

	return core_no;
}


/*
 * Function:    hfc_fx_choose_core
 *
 * Purpose:     choose a core and set chosen core_info address to hfc_pkt_fx
 *
 * Arguments:
 *  pp          Pointer to port_info
 *  hfcp        Pointer to hfc_pkt_fx
 *
 * Returns:
 *              Pointer to chosen core_info address
 *
 * Notes:
 */
struct core_info *hfc_fx_choose_core(struct port_info *pp,struct hfc_pkt_fx *hfcp)
{
	uchar core_no = pp->master_core_no; /* variable to keep temporary result */
	uchar i = 0, skip_update_curr_core = 0;
	uchar online_core_num = 0;
	uchar use_core_num = 0;
	uchar use_core_list[MAX_CORE_PROBE_FX];
	struct socket_info *socket_info_wk = NULL;	/* FCLNX-GPL-FX-420 */
	uchar socket_number=0;

	/* 4-port adapter */
	if( pp->core_num == 1 ){
		hfcp->core = hfcp->rp->core_arg[core_no];
		hfcp->core_no = core_no;
		
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				pp->pm_pkt_pool[hfcp->pm_pkt_no].core_no = core_no;
			}
		}
		
		return hfcp->rp->core_arg[core_no];
	}
	
	/* for FW I/O performance at LOOP *//* FCLNX-GPL-FX-148,163 */
	if (( pp->connect_type == HFC_FX_AL )||( pp->connect_type == HFC_FX_MULTI_ALPA )){
		hfcp->core = hfcp->rp->core_arg[core_no];	/* master core */
		hfcp->core_no = core_no;
		
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				pp->pm_pkt_pool[hfcp->pm_pkt_no].core_no = core_no;
			}
		}
		
		return hfcp->rp->core_arg[core_no];
	}

	/* Count online_core_num */
	for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
		if( !hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[i]))
			online_core_num++;
	}

	if(online_core_num == 1){
		hfcp->core = hfcp->rp->core_arg[core_no];
		hfcp->core_no = core_no;
		
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				pp->pm_pkt_pool[hfcp->pm_pkt_no].core_no = core_no;
			}
		}
		
		return hfcp->rp->core_arg[core_no];
	}

	switch(pp->core_control)
	{
	case HFC_FX_CORECTL_SEQUENTIAL:
		/* Use the same core for successive write or read commands */
		
		if ( (hfcp->cmd_pkt->sc_data_direction == SCSI_DATA_WRITE ) &&
			 (hfcp->dev->curr_cmd_type == 1) )
		{	/* Successive Write Commands */
			core_no = hfcp->dev->curr_core;
			hfcp->dev->seq_cnt++;
			if (hfcp->dev->seq_cnt > pp->cc_cnt) {
				core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
				hfcp->dev->seq_cnt = 0;
			}
		}
		else if( (hfcp->cmd_pkt->sc_data_direction != SCSI_DATA_WRITE ) &&
				 (hfcp->dev->curr_cmd_type == 0) )
		{	/* Successive Read Commands */
			core_no = hfcp->dev->curr_core;
			hfcp->dev->seq_cnt++;
			if (hfcp->dev->seq_cnt > pp->cc_cnt) {
				core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
				hfcp->dev->seq_cnt = 0;
			}
		}
		else
		{	/* Use round robin */
			core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
			hfcp->dev->seq_cnt = 0;
		}
		
		/* Save current command type(write:1, read:0) */
		if ( hfcp->cmd_pkt->sc_data_direction == SCSI_DATA_WRITE )
			hfcp->dev->curr_cmd_type = 1;
		else
			hfcp->dev->curr_cmd_type = 0;
		break;
	
	case HFC_FX_CORECTL_LARGE_DATA: 
		/* Use a master_core for command with large data */
		
		memset(use_core_list, 0x01, sizeof(use_core_list)); // initialize to all core available
		
		if (scsi_bufflen(hfcp->cmd_pkt) >= pp->cc_size*1024) {
			core_no = pp->master_core_no;
			skip_update_curr_core = 1;
		} 
		else { //  minmum we_que_cnt (or round robin) EXCEPT master_core
			if (hfcp->rp->core_arg[pp->master_core_no]->we_que_cnt_all) {
				// if cmd exist in we-que, not use master-core
				use_core_list[pp->master_core_no] = 0;  
				core_no = hfc_fx_choose_core_minque(pp, hfcp, use_core_list);
			} else {
				core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
			}
		}
		break;
	
	case HFC_FX_CORECTL_SINGLECORE:
		/* Use master_core everytime */
		break;
	
	case HFC_FX_CORECTL_CORE_LIST:
		use_core_num = 0;
		memset(use_core_list, 0x01, sizeof(use_core_list)); // initialize to all core available
		
		for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
			if (hfcp->rp->core_arg[i] == NULL) { continue; }
			if (hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[core_no])) { continue; }
			if (pp->cc_core & (0x01 << i)) {
				use_core_num++;
			} else {
				use_core_list[ i ] = 0;
			}
		}
		
		if (use_core_num == 0) {
			core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
		} else {
			core_no = hfc_fx_choose_core_minque(pp, hfcp, use_core_list);
		}
		break;
	
	case HFC_FX_CORECTL_AT_ASSGN_LU:
		/* Use the same core for each LU(dev) */
		if( hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[hfcp->dev->curr_core]) ){
			core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
		}
		else {
			core_no = hfcp->dev->curr_core;
		}
		break;
	
	case HFC_FX_CORECTL_CPU_TO_CORE:	/* FCLNX-GPL-FX-193 */
		/* Select queue based on Socket# which issued I/O command */
		socket_info_wk = hfc_fx_get_socket_info(pp, hfcp->cpu_no);
		if(socket_info_wk != NULL){
			socket_number = socket_info_wk->socket_no;
		}
		
		if( pp->socket_num < pp->core_num ){
			/* Select core based on the number of CPU which issued I/O command */
			core_no = (hfcp->cpu_no % pp->core_num );
		}
		else{
			/* Select core based on the number of Socket which issued I/O command */
			core_no = (socket_number % pp->core_num );
		}
		
		if( pp->core_num == 2 ){
			if( core_no ) {
				core_no = 2;
			}
			else{
				core_no = 0;
			}
		}
		if( hfc_fx_check_cs_disable(pp, hfcp->rp->core_arg[core_no]) ){
			core_no = pp->master_core_no;
		}
		break;
	
	case HFC_FX_CORECTL_ENHANCE_RR:
		/* Use the core which have minimum we_que_cnt or use round robin */
	default:
		core_no = hfc_fx_choose_core_minque(pp, hfcp, NULL);
		break;
	}

	if (pp->core_control != HFC_FX_CORECTL_AT_ASSGN_LU) {
		pp->curr_core = core_no;
	}
	hfcp->dev->curr_core = core_no;
	hfcp->core = hfcp->rp->core_arg[core_no];
	hfcp->core_no = core_no;
	
	if (pp->pm_control == HFC_FX_PM_ON) {
		if (hfcp->pm_pkt_no != 0xffff) {
			pp->pm_pkt_pool[hfcp->pm_pkt_no].core_no = core_no;
		}
	}

	/* Return core_info pointer */
	return hfcp->rp->core_arg[core_no];
}


int hfc_fx_strategy_port(struct hfc_pkt_fx *hfcp, int core_no)
{
	struct	port_info		*pp;
	struct	core_info		*core;
	struct	target_info_fx	*target, *target_wk;
	struct	scsi_cmnd 		*cmnd=hfcp->cmd_pkt;
	struct	scsi_cmnd		*wk_cmnd=NULL;
	struct	scsi_device		*sdev;
	struct	request_queue	*rq;
	struct	dev_info_fx		*dev=hfcp->dev;

	int			func_rc = 0 ;
	uint		lun,hit,entry;
	char		buf1[32],buf2[32];
	uint		ioctl_mode=0;
	uchar		id;
	ushort		cmd_lun;								/* FCLNX-GPL-0548 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	struct fc_rport *rport = NULL;
	int				 err   = 0;
#endif /* KERNEL_VERSION(2,6,30) */
#endif /* SYSFS_SUPPORT */
#endif

	pp = hfcp->pp;
	
	/* kernel 5.16+: detect ioctl via ap->ioctl_cmnd */
	if( cmnd == ap->ioctl_cmnd ) ioctl_mode=1;
	
    cmd_lun = (ushort)CMND_LUN(cmnd);						/* FCLNX-GPL-0548 */
    cmd_lun = (cmd_lun & 0x3fff);							/* FCLNX-GPL-0548 */
    
    if( test_bit( HFC_PS_DIAG, (ulong *)&pp->status ) )		/* FCLNX-GPL-FX-126 */
	{
		/* Reject SCSI command while executing DIAG process */
		if(ioctl_mode == 1) func_rc=EIO;
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_OFFLINE;
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_ERROR);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
    }														/* FCLNX-GPL-FX-126 */

	/* Check each element of Scsi_Cmnd structure */
	if(cmnd->transfersize > (uint)pp->dma_max)
	{
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_DMASIZE_OVER;
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_ERROR);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(0);
	}
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= pp->max_target)
	  || (CMND_TARGET(cmnd) == pp->hosts->this_id)
	  || (cmd_lun >= MAX_DEV_CNT) ) {							/* FCLNX-GPL-0548 */
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_TARGET_NOTFOUND;
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_BAD_TARGET);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(0);
	}
	if(cmnd->cmd_len > 16)
	{
		set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
		hfcp->adap_status = SCS_CMDLENGTH_OVER;
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_ERROR);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(0);
	}
	
	lun = hfcp->lun_id;

	if (!HFC_PP_FX_ATTACH_STATUS_TEST(HFC_ATTACH ,pp))
    {
		/* Adapter is not intialized */
		if(ioctl_mode == 1) func_rc=EIO;
		hfcp->adap_status = SCS_ATTATCH_ERROR;

		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - pp->attach_status-error\n");
		
		pp->scsi_exec_cnt++;
		
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_ERROR);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
    }
	
	target = hfcp->target;
	
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if( atomic_read(&pp->pport->rport_event_wait) == 1 )
	{	/* Remote Port Event Waiting */
		if(ioctl_mode == 1) func_rc=EIO;
		hfcp->adap_status = SCS_ADAPTER_OFFLINE;
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_IMM_RETRY);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
	}

#ifdef HFC_UBUNTU
	if( target ){
		if( (test_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags) )
			&& (target->fast_io_fail_tmo == -1))
		{	/* Remote Port Event Waiting */
			if(ioctl_mode == 1) func_rc=EIO;
			hfcp->adap_status = SCS_ADAPTER_OFFLINE;
			
			pp->scsi_exec_cnt++;
			hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_IMM_RETRY);
			hfc_fx_iodone(pp, NULL, cmnd, hfcp);
			return(func_rc);
		}
	}
#endif

#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */

#if !( defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
		if (!HFC_PP_FX_STATUS_TEST(HFC_PS_ONLINE ,pp))
		{
			/* Adapter is not ONLINE */
			if(ioctl_mode == 1) func_rc=EIO;
			hfcp->adap_status = SCS_ADAPTER_OFFLINE;
		
//			hfc_fx_stra_trace(
//				HFC_FX_TRC_STRATEGY ,0x22 ,pp ,hfcp->rp, NULL, NULL, hfcp,
//				(ulong)cmnd, 0,0);
//			}
		
			pp->scsi_exec_cnt++;
			hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_NO_CONNECT);
			hfc_fx_iodone(pp, NULL, cmnd, hfcp);
			return(func_rc);
		}
#else
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
			if (!HFC_PP_FX_STATUS_TEST(HFC_PS_ONLINE ,pp))
			{
				/* Adapter is not ONLINE */
				if(ioctl_mode == 1) func_rc=EIO;
				hfcp->adap_status = SCS_ADAPTER_OFFLINE;
				
//				hfc_fx_stra_trace(
//					HFC_FX_TRC_STRATEGY ,0x22 ,pp ,hfcp->rp, NULL, NULL, hfcp,
//					(ulong)cmnd, 0,0);
//				}
				
				pp->scsi_exec_cnt++;
				hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_NO_CONNECT);
				hfc_fx_iodone(pp, NULL, cmnd, hfcp);
				return(func_rc);
			}
		}
#endif

#if 0	/* FCLNX-GPL-FX-188 */
	if ( test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) && test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) )
    {
		/* MCK Recovery & Flash Updating */
		if(ioctl_mode == 1) func_rc=EIO;
		hfcp->adap_status = SCS_FLASH_UPDATE;

		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - MCK & Flash update\n");
		
		pp->scsi_exec_cnt++;
		
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_ERROR);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
    }
#endif	/* FCLNX-GPL-FX-188 */
	
	/* Is Target initiated? */
	if(target == NULL)
	{
		if(ioctl_mode == 1) func_rc=EIO;
		hfcp->adap_status = SCS_NO_TARGET;
		
		hfc_fx_stra_trace(
			HFC_FX_TRC_STRATEGY, 0x31 ,pp ,hfcp->rp, NULL, NULL, hfcp,
			(ulong)cmnd, 0,0);
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_BAD_TARGET);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
	} 

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
		if(ioctl_mode == 0)
		{	/*** This case is Not ioctl_mode ***/
			rport = starget_to_rport(scsi_target(cmnd->device));
//			rport = target->rport;
			if(rport != NULL)
				err   = fc_remote_port_chkready(rport);
			if (err)
			{
//				cmnd->result = err;
				hfcp->adap_status = SCS_NO_TARGET;
				hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, err);
			
				HFC_DBGPRT(" hfcldd : hfc_strategy - fc_remote_port_chkready() set cmnd->result.\n");
			
				if( (err == DID_NO_CONNECT) || (err == DID_TRANSPORT_FAILFAST) )
				{	/* scsi_err count up */
					pp->scsi_err_cnt++ ;
				}
			
				hfc_fx_stra_trace(
					HFC_FX_TRC_STRATEGY, 0x33 ,pp ,hfcp->rp, NULL, NULL, hfcp,
					(ulong)cmnd, 0,0);
				hfc_fx_iodone(pp, NULL, cmnd, hfcp);
				return(func_rc);
			}
		}
	}
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
	
	if( !( HFC_PP_FX_STATUS_TEST(HFC_PS_ONLINE ,pp) ) )
	{
		/* Adapter is not ONLINE */
		if(ioctl_mode == 1) func_rc=EIO;
//		result = DID_NO_CONNECT;
// 		cmnd->result |= (result << 16);
 		pp->scsi_err_cnt++ ;
		hfcp->adap_status = SCS_ADAPTER_OFFLINE;        /* FCLNX-0534 */
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_NO_CONNECT);
		
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
	}
 	
#endif
	if (!HFC_TG_FX_FLAGS_TEST(HFC_TF_WWN_VALID, target))
	{
		if(ioctl_mode == 1) func_rc=EIO;
		hfcp->adap_status = SCS_WWN_INVALID;
		
		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - target->flags<HFC_TF_WWN_VALID>=0. \n");
		
		hfc_fx_stra_trace(
			HFC_FX_TRC_STRATEGY ,0x32 ,pp ,hfcp->rp, NULL, target, hfcp,
			(ulong)cmnd, 0,0);
		
		pp->scsi_exec_cnt++;
		hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_NO_CONNECT);
		hfc_fx_iodone(pp, NULL, cmnd, hfcp);
		return(func_rc);
	}

	if (pp->narrowmap == 1) {
		hit=0;
		for (entry=0;entry<8;entry++) {
			if ((pp->boot_priority[entry].ww_name == hfcp->target->ww_name)
			 && (pp->boot_priority[entry].lun     == hfcp->lun_id) ) {
				hit = 1;
				break;
			}
		}
		
		if (hit) {
			hit=0;
			
			if ( (ioctl_mode == 1) && (!hfc_narrow_dev.ap.ww_name
			 || (hfc_narrow_dev.ap.bus > pp->pci_cfginf->bus->number)
			 || ((hfc_narrow_dev.ap.bus == pp->pci_cfginf->bus->number)
			     && (hfc_narrow_dev.ap.devfn > pp->pci_cfginf->devfn))
			 || ((hfc_narrow_dev.ap.bus == pp->pci_cfginf->bus->number)
			     && (hfc_narrow_dev.ap.devfn == pp->pci_cfginf->devfn)
				 && (hfc_narrow_dev.priority > entry))) ) {

				hfc_narrow_dev.ap.ww_name   = pp->ww_name;
				hfc_narrow_dev.ap.bus       = pp->pci_cfginf->bus->number;
				hfc_narrow_dev.ap.devfn     = pp->pci_cfginf->devfn;
				hfc_narrow_dev.priority     = entry;
				hfc_narrow_dev.tgt.ww_name  = hfcp->target->ww_name;
				hfc_narrow_dev.tgt.lun      = hfcp->lun_id;
				hit=1;
				sprintf(buf1,"%llx", (unsigned long long)hfc_narrow_dev.tgt.ww_name);
				sprintf(buf2,"%x", hfc_narrow_dev.tgt.lun);

				HFC_INFPRT("hfcldd%d : narrow mode(lun) - HBA WWPN=%llx(%02x:%02x.%x), Priority=%d, Target WWPN,LU=%s,%s\n",
							pp->dev_minor,
							(unsigned long long) hfc_narrow_dev.ap.ww_name,
							pp->pci_cfginf->bus->number, 
							PCI_SLOT(pp->pci_cfginf->devfn),
							PCI_FUNC(pp->pci_cfginf->devfn),
							hfc_narrow_dev.priority,
							buf1,
							buf2 );
			}
			else {
				if ( (hfc_narrow_dev.ap.ww_name  == pp->ww_name)
				   && (hfc_narrow_dev.tgt.ww_name == hfcp->target->ww_name)
				   && (hfc_narrow_dev.tgt.lun     == hfcp->lun_id) ) {
					hit=1;
				}
			}
		}
		
		/* FCLNX-GPL-FX-269 start */
		if ( hfc_manage_info.hfcldd_mp_mod  )
		{
			if ( hfc_manage_info.npubp->hfc_fx_tmp_dev_check(dev) )
			{	/* temporary dev_info */
				hit = 1; /* IO start -> Accept */
			}
		}
		/* FCLNX-GPL-FX-269 end */
		
		if (!hit && !ioctl_mode) {
			hfcp->adap_status = SCS_NARROW_DEV;
			
			set_bit(CFLAG_INH_ALTPATH, (ulong *)&hfcp->cmd_flags);
			
			hfc_fx_stra_trace(
				HFC_FX_TRC_STRATEGY ,0x33 ,pp ,hfcp->rp ,NULL ,target, hfcp,
				(ulong)cmnd, 0,0);
			
			pp->scsi_exec_cnt++;
			hfc_fx_set_cmnd_res(pp, NULL, cmnd, hfcp, DID_BAD_TARGET);
			hfc_fx_iodone(pp, NULL, cmnd, hfcp);
			return(func_rc);
		}
	}
	
	/* core choice */
	/* FCLNX-GPL-FX-269 start */
	if( core_no < 0 )	/* FCLNX-GPL-FX-266 start */
	{
		core = hfc_fx_choose_core(pp,hfcp);
	}
	else
	{	/* HFC-PCM will select core_no. */
		if (pp->core_control != HFC_FX_CORECTL_AT_ASSGN_LU) {
			pp->curr_core = core_no;
		}
		hfcp->dev->curr_core = core_no;
		hfcp->core = hfcp->rp->core_arg[core_no];
		hfcp->core_no = core_no;
		
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				pp->pm_pkt_pool[hfcp->pm_pkt_no].core_no = core_no;
			}
		}
		
		core = hfcp->rp->core_arg[core_no];
	}					/* FCLNX-GPL-FX-266 end */
	/* FCLNX-GPL-FX-269 end */

	if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_HSDLDD_VALID, hfcp)) {
		wk_cmnd = hfcp->cmd_pkt;
		sdev = wk_cmnd->device;
		rq = sdev->request_queue;
		
		if( rq != NULL ){
			if (!ioctl_mode && rq->timeout.function) {
				del_timer( &rq->timeout );
			}
		}
	}

	hfcp->target = target;
	
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {	/* FCLNX-GPL-FX-206 */
		/* use physical target for sstatus check */
		target_wk = pp->pport->target_arg[target->pseq];
	}
	else {
		target_wk = target;
	}
	
	/* status busy check */
	if ( !(pp->status & HFC_PS_BLOCKED_SCSI)
	  && !(pp->pport->status & HFC_PS_BLOCKED_SCSI_PPORT)
	  && !(pp->status_detail2 & HFC_PD2_BLOCKED_SCSI)
	  && !(target_wk->status & HFC_TS_BLOCKED_SCSI)
	  && !(dev->lustat & HFC_DS_BLOCKED_SCSI) )
	{
		/* normal status */
		if (!(pp->debug_func & HFC_FX_DEBUG_IOTRACE_OFF)) {
			hfc_fx_stra_trace(
				HFC_FX_TRC_STRATEGY ,0x20 ,pp , hfcp->rp, core, target, hfcp,
				(ulong)cmnd, 0,0);
		}
		
		return(func_rc);
	}
	else {
		/* busy status */
		hfcp->adap_status = SCS_TARGET_ABNORMAL;
		id = 0x64;
		
		if ( (pp->status & HFC_PS_BLOCKED_SCSI)
		  || (pp->status_detail2 & HFC_PD2_BLOCKED_SCSI) ) {
			hfcp->adap_status = SCS_SCSI_DELAY;
			id = 0x67;
		}
		
		if (target_wk->status & HFC_TS_BLOCKED_SCSI) {
			hfcp->adap_status = SCS_SCSI_DELAY;
			id = 0x61;
		}
		
		if (test_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat)) {
			hfcp->adap_status = SCS_CMD_ABORTED;
			id = 0x45;
		}
		else if(test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat)) {
			hfcp->adap_status = SCS_CMD_NEED_ABORT;
			id = 0x46;
		}
		else if(test_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat)) {
			hfcp->adap_status = SCS_WAIT_LUNRST;
			id = 0x47;
		}
		else if(test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat)) {
			hfcp->adap_status = SCS_NEED_LUNRST;
			id = 0x48;
		}
		
		if (HFC_TG_FX_STATUS_TEST(HFC_TS_WAIT_PLOGI, target_wk)) {
			hfcp->adap_status = SCS_WAIT_LOGIN;
			id = 0x62;
		}
		
		if (HFC_TG_FX_STATUS_TEST(HFC_TS_WAIT_TARGET_RESET, target_wk)) {
			hfcp->adap_status = SCS_CMD_RESET;
			id = 0x63;
		}
		
		HFC_DBGPRT(" hfcldd : hfc_fx_strategy - STATUS_BUSY, id=0x%02x\n", id);
		hfc_fx_stra_trace(
			HFC_FX_TRC_STRATEGY, id, pp, hfcp->rp, core, target, hfcp,
			(ulong)cmnd, 0, 0);
		
		return(func_rc);
	}
}

void hfc_fx_strategy_core(struct hfc_pkt_fx *hfcp)
{
	if ( hfcp->cmd_pkt->sc_data_direction == SCSI_DATA_WRITE ) {
		hfcp->core->wr_exec_cnt++;
		if (hfcp->pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				hfcp->pp->pm_pkt_pool[hfcp->pm_pkt_no].data_type = SCSI_DATA_WRITE;
			}
		}
	}
	else {
		hfcp->core->rd_exec_cnt++;
		if (hfcp->pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				hfcp->pp->pm_pkt_pool[hfcp->pm_pkt_no].data_type = SCSI_DATA_READ;
			}
		}
	}
	hfcp->core->scsi_exec_cnt++;

	hfcp->core->pkt_cnt++;
	hfc_fx_enqueue_wx_que(hfcp->core, hfcp);

	if (!hfcp->adap_status) {
		hfc_fx_start(hfcp->pp, hfcp->rp, hfcp->core, hfcp->target);
	}
	else {
		if (hfcp->target->core_queue[hfcp->core->core_no].next_dstart_flag == 0) {
			hfc_fx_enque_next_dstart(hfcp->pp, hfcp->rp, hfcp->core, hfcp->target);
		}
	}
}

/* FCLNX-GPL-FX-014 Start */
#define HFC_FX_FRAME_BUSY(CORE, XOB, FINP)                                        \
	( ((CORE)->frame_start_xob[(FINP)].num != 0)                                  \
	  && ((XOB)->flag & HFC_XOB_VALID)                                            \
	  && ( ((XOB)->drv_work == 0)                                                 \
	    || (!test_bit(CFLAG_VALID, (ulong *)&(((struct hfc_pkt_fx *)(ulong)(XOB)->drv_work)->cmd_flags) ))  \
	    || (((struct hfc_pkt_fx *)(ulong)(XOB)->drv_work)->core_no != (CORE)->core_no)   \
	    || (((struct hfc_pkt_fx *)(ulong)(XOB)->drv_work)->frame_no == (FINP)) )         \
	)
/* FCLNX-GPL-FX-014 End */

/*
 * Function:    hfc_fx_start
 *
 * Purpose:     Dequeue command from wait_xob_que and set it to xob, then start firmware.
 *              If xob initiation succeeds, enqueue it to wait_end_que.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  pkt        - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *
 * Notes:       Caller should be process level or interrupt level.
 *              Lock core_info before calling this function.
 *    
 */
void hfc_fx_start(struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target)
{
	struct	scsi_cmnd		*cmnd=NULL ;
	struct	scsi_cmnd		*wk_cmnd=NULL ;
	struct	scsi_device		*sdev;
	struct	request_queue	*rq;
	struct	dev_info_fx		*dev = NULL;
	struct	hfc_pkt_fx		*hfcp, *hfcp_wk, *hfcp_trc=NULL;
	struct	target_info_fx	*target_wk;
	
	int			rc = 0 ;
	uint		xob_exec ;
	int			func_rc, lp=0, lun, ioctl_mode=0 ;
	uint		initial_xob_no;
	uchar		xob_wait_exec_cnt = 0;
	uint		cpuno = 0;
	uchar		frame_busy = 0, skip_deq_wx = 0;/* FCLNX-GPL-FX-014,181 */
	uchar		use_reserve_reset_frame = 0;	/* FCLNX-GPL-FX-014 */
	uchar		use_reserve_reset_xob = 0;		/* FCLNX-GPL-FX-014 */
	struct xob_fx		*xob_p = NULL;			/* FCLNX-GPL-FX-014 */
	struct c_xob_fx		*cancel_xob_p = NULL;	/* FCLNX-GPL-FX-014 */

	initial_xob_no = core->drv_next_xob;
	
	hfcp = target->core_queue[core->core_no].wx_que_top;
	
	/* Frame Busy Check */
	lp = core->frame_inp;

	if (HFC_FX_FRAME_BUSY(core, &core->xob[core->frame_start_xob[lp].start], lp)) {/* FCLNX-GPL-FX-014 */
		HFC_DBGPRT("hfcldd : hfc_start - 1 \n");
		/* The number of executing frame is full *//* FCLNX-GPL-FX-014 Start */
		if (target->core_queue[core->core_no].wx_que_tskmgm_cnt == 0) {
			HFC_DBGPRT("hfcldd : hfc_start - 2 \n");
			frame_busy = 1;
		}
		else {// serach reset cmd from wx_que
			hfcp_wk = target->core_queue[core->core_no].wx_que_top;
			
			while( hfcp_wk != NULL ){
				if( hfcp_wk->cmd_flags & CFLAG_RESET_ANY ){
					HFC_DBGPRT("hfcldd : hfc_start - 3 \n");
					break;
				}
				hfcp_wk = hfcp_wk->cmd_forw;
			}
			
			if (hfcp_wk == NULL) { // not found reset cmd
				HFC_DBGPRT("hfcldd : hfc_start - 4 \n");
				frame_busy = 1;
			} else {
				lp = pp->lparmode.frame_cnt -1; // use reserve frame for reset
				if (HFC_FX_FRAME_BUSY(core, &core->xob[core->frame_start_xob[lp].start], lp)) {
					HFC_DBGPRT("hfcldd : hfc_start - 6 \n");
					frame_busy = 1;
				}
				else {
					use_reserve_reset_frame = 1;
				}
			}
		}/* FCLNX-GPL-FX-014 End */
		if(frame_busy ){
			hfc_fx_stra_trace(
				HFC_FX_TRC_START ,0x13 ,pp ,rp ,core ,target ,NULL,
				0, 0, 0);
			core->frame_full_cnt++;
			if ( !(HFC_FX_MMODE_CHECK_BASIC(pp)) && (pp->hg_cca_p != NULL) )
				pp->hg_cca_p[core->core_no].frame_full = core->frame_full_cnt;	/* FCLNX-GPL-FX-433 */
			hfc_fx_enque_next_dstart(pp, rp, core, target);
			return;
		}
	}
	
	/* get processor_id */
	if (pp->pm_control == HFC_FX_PM_ON) {
		cpuno = smp_processor_id();
	}
	
	while( hfcp != NULL )
	{
		dev  = hfcp->dev;
		cmnd = hfcp->cmd_pkt;
		lun  = hfcp->lun_id;
		
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) {
			if ( hfc_manage_info.npubp->hfc_fx_queue_check(hfcp, lun) ) {		/* FCLNX-0521 */
				hfcp_wk = hfcp->cmd_forw;
				hfcp = hfcp_wk;
//				printk(KERN_ERR "hfcldd%d hfc_queue_check()=1.\n", ap->dev_minor);
				continue;
			}
		}
		
		if(cmnd != NULL)
		{
			/* kernel 5.16+: detect ioctl via ap->ioctl_cmnd */
			if( cmnd == ap->ioctl_cmnd ) ioctl_mode=1;
		}
		
		if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
			/* use physical target for sstatus check */
			target_wk = pp->pport->target_arg[target->pseq];
		}
		else {
			target_wk = target;
		}
		if (((dev->lustat & HFC_DS_BLOCKED_SCSI) || (target_wk->status & HFC_TS_BLOCKED_SCSI) || (pp->status_detail2 & HFC_PD2_BLOCKED_SCSI))
		&& (!(hfcp->cmd_flags & CFLAG_RESET_ANY))){	/* FCLNX-GPL-FX-014,181,224 */
			skip_deq_wx = 1;
		}else if((target_wk->status & HFC_TS_BLOCKED_TGT_RESET)&&(hfcp->cmd_flags & CFLAG_TGT_RESET_ANY)){
			skip_deq_wx = 1;
		}else if(((target_wk->status & HFC_TS_BLOCKED_SCSI) || (pp->status_detail2 & HFC_PD2_BLOCKED_SCSI))
		         &&(hfcp->cmd_flags & CFLAG_LU_RESET_ANY)){	/* FCLNX-GPL-FX-224 */
			skip_deq_wx = 1;
		}
		
		if(skip_deq_wx == 1){	/* FCLNX-GPL-FX-181 */
			hfcp_wk = hfcp->cmd_forw;
			hfcp = hfcp_wk;
			skip_deq_wx = 0;	/* FCLNX-GPL-FX-249,272 */
			continue;
		}
		
		/* FCLNX-GPL-FX-147 */
		/* Restrict outstanding I/Os if max_io is effective.
		 * (Performance tuning purpose)
		 */
		if ((pp->max_io != 0) &&
			((core->we_que_cnt_all + xob_wait_exec_cnt) >= pp->max_io))
		{
			break;
		}
		
		/* One or more commands exist in wait_xob_que */
		if( (rc = hfc_fx_resource_chk(pp, core, hfcp)) != 0 )
		{
			if (rc == HFC_IOVMAP_FULL)
			{
				core->iovmap_full_cnt++;
				if ( !(HFC_FX_MMODE_CHECK_BASIC(pp)) && (pp->hg_cca_p != NULL) )
					pp->hg_cca_p[core->core_no].iov_full   = core->iovmap_full_cnt;	/* FCLNX-GPL-FX-433 */
			}
			if (rc == HFC_XOB_FULL )
			{
				HFC_DBGPRT("hfcldd : hfc_start - 7 \n");
				core->xob_full_cnt++;
				/* FCLNX-GPL-FX-014 Start */
				if (hfcp != NULL) {
					HFC_DBGPRT("hfcldd : hfc_start - 8 \n");
					if( !(hfcp->cmd_flags & CFLAG_RESET_ANY) )
					{	/* scsi FCLNX-GPL-FX-337 */
						HFC_DBGPRT("hfcldd : hfc_start - 9 \n");
						if (target->core_queue[core->core_no].wx_que_tskmgm_cnt > 0) 
						{	// serach reset cmd from wx_que
							HFC_DBGPRT("hfcldd : hfc_start - 11 \n");
							if ((hfcp_wk = target->core_queue[core->core_no].wx_que_top) != NULL)
							{ 	/* hfcp exists in queue */
								HFC_DBGPRT("hfcldd : hfc_start - 12 \n");
								while( hfcp_wk != NULL )
								{
									if(hfcp_wk->cmd_flags & CFLAG_RESET_ANY) break;
									hfcp_wk = hfcp_wk->cmd_forw ;
								}
							}
							if (hfcp_wk != NULL) {
								HFC_DBGPRT("hfcldd : hfc_start - 13 \n");
								hfcp = hfcp_wk; // try reset_cmd next loop
								use_reserve_reset_xob = 1;
								continue;
							}
						}
					}
				}
				/* FCLNX-GPL-FX-014 End */
				if ( !(HFC_FX_MMODE_CHECK_BASIC(pp)) && (pp->hg_cca_p != NULL) )
					pp->hg_cca_p[core->core_no].xob_full   = core->xob_full_cnt;	/* FCLNX-GPL-FX-433 */
			}
			if (rc == HFC_DMA_MAX_OVER )
			{
				core->dma_max_over_cnt++;
			}
			if( rc == HFC_PAGE_OVER )
			{
				/* Specified page is too large in number */
				core->page_over_cnt++;
				if ( !(HFC_FX_MMODE_CHECK_BASIC(pp)) && (pp->hg_cca_p != NULL) )
					pp->hg_cca_p[core->core_no].page_over  = core->page_over_cnt;	/* FCLNX-GPL-FX-433 */
				if(ioctl_mode == 1) func_rc=EIO;
				hfcp->adap_status = SCS_PAGE_OVER;

				/* Dequeue command from wait_xob_que */
				hfc_fx_deque_wx_que(core, hfcp);

				if(cmnd != NULL) {
					hfc_fx_set_cmnd_res(pp, core, cmnd, hfcp, DID_ERROR);
					hfc_fx_iodone(pp, core, cmnd, hfcp);
				}

				hfcp_wk = hfcp->cmd_forw;
				hfcp = hfcp_wk;

				continue ;
			}
			
			break ;
		}
		
		/* Clear xob */
		HFC_BZERO(&core->xob[core->drv_next_xob],sizeof(struct xob_fx));
		
//		/* Mpp DMA area for data transfer */
//		if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_SEGVALID, hfcp))
//		{
			hfc_fx_dma_map(pp, core, hfcp);
//		}
		
		/* FCLNX-GPL-FX-014 Start */
		xob_p = &core->xob[core->drv_next_xob];
		cancel_xob_p = (struct c_xob_fx *) xob_p;
		
		if( hfcp->cmd_flags & CFLAG_RESET_ANY){
			HFC_DBGPRT("hfcldd : hfc_start - 14 \n");
			
			if( hfcp->cmd_flags & CFLAG_CSCSI_ANY){
				
				HFC_DBGPRT("hfcldd : hfc_start - 15 \n");
				xob_p->flag |= HFC_XOB_C;
				
				if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_LU_WITHOUT_DMA, hfcp))
				{
					HFC_DBGPRT("hfcldd : hfc_start - 16 \n");
					cancel_xob_p->cancel_ctl   = HFC_CANCEL_WITHOUT_DMA;
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITLNEXUS;
					clear_bit(HFC_DC_NEED_CSCSI_LU_WITHOUT_DMA, (ulong *)&hfcp->dev->dev_core_stat.core[ core->core_no ]);
					set_bit(HFC_DC_WAIT_CSCSI_LU_WITHOUT_DMA, (ulong *)&hfcp->dev->dev_core_stat.core[ core->core_no ]);
				} 
				else if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_LU_WAIT_DMA, hfcp))
				{
					HFC_DBGPRT("hfcldd : hfc_start - 17 \n");
					cancel_xob_p->cancel_ctl   = 0; // Wait stop DMA
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITLNEXUS;
					clear_bit(HFC_DC_NEED_CSCSI_LU_WAIT_DMA, (ulong *)&hfcp->dev->dev_core_stat.core[ core->core_no ]);
					set_bit(HFC_DC_WAIT_CSCSI_LU_WAIT_DMA, (ulong *)&hfcp->dev->dev_core_stat.core[ core->core_no ]);
				} 
				else if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_TGT_WITHOUT_DMA, hfcp))
				{
					HFC_DBGPRT("hfcldd : hfc_start - 18 \n");
					cancel_xob_p->cancel_ctl   = HFC_CANCEL_WITHOUT_DMA;
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITNEXUS;
					clear_bit(HFC_TC_NEED_CSCSI_TGT_WITHOUT_DMA, (ulong *)&hfcp->target->tgt_core_stat.core[core->core_no]);
					set_bit(HFC_TC_WAIT_CSCSI_TGT_WITHOUT_DMA, (ulong *)&hfcp->target->tgt_core_stat.core[core->core_no]);
				}
				else if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_TGT_WAIT_DMA, hfcp))
				{
					HFC_DBGPRT("hfcldd : hfc_start - 19 \n");
					cancel_xob_p->cancel_ctl   = 0; // Wait stop DMA
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITNEXUS;
					clear_bit(HFC_TC_NEED_CSCSI_TGT_WAIT_DMA, (ulong *)&hfcp->target->tgt_core_stat.core[core->core_no]);
					set_bit(HFC_TC_WAIT_CSCSI_TGT_WAIT_DMA, (ulong *)&hfcp->target->tgt_core_stat.core[core->core_no]);
				}
				HFC_DBGPRT("hfc_fx_start xob dump\n");
				structdump( 0xef, (uchar *)xob_p, sizeof(struct xob_fx) );	
				/* FCLNX-GPL-FX-014 End */
			}else{
				
				xob_p->flag |= HFC_XOB_CI;	/* FCLNX-GPL-FX-014 */
				
				if ( HFC_HFCP_FX_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)
				|| HFC_HFCP_FX_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) ){	/* FCLNX-GPL-FX-176 */
					/* issue Target Reset Command */
					HFC_DBGPRT("hfcldd : hfc_start - 20 \n");
					cancel_xob_p->cancel_ctl = 0; /* Wait stop DMA FCLNX-GPL-FX-014 */
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITNEXUS;	/* FCLNX-GPL-FX-014 */
					
					clear_bit(HFC_TC_NEED_TGTRST, (ulong *)&hfcp->target->tgt_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-014 */
					set_bit(HFC_TC_WAIT_TGTRST, (ulong *)&hfcp->target->tgt_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-014 */
					clear_bit( HFC_TS_NEED_TARGET_RESET, (ulong *)&hfcp->target->status );
					set_bit( HFC_TS_WAIT_TARGET_RESET, (ulong *)&hfcp->target->status );
				}
				else if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_ABORT, hfcp)) {	/* FCLNX-GPL-FX-176 */
					/* issue Abort Task Set Command */
					HFC_DBGPRT("hfcldd : hfc_start - 21 \n");
					cancel_xob_p->cancel_ctl = 0;  /* Wait stop DMA FCLNX-GPL-FX-014 */
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITLNEXUS;	/* FCLNX-GPL-FX-014 */
					
					clear_bit(HFC_DC_NEED_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-014 */
					set_bit(HFC_DC_WAIT_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-014 */
					clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
					set_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
				}
				else if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp)){	/* FCLNX-GPL-FX-176 */
					/* issue LUN Reset Command */
					HFC_DBGPRT("hfcldd : hfc_start - 22 \n");
					cancel_xob_p->cancel_ctl = 0; /* Wait stop DMA FCLNX-GPL-FX-014 */
					cancel_xob_p->cancel_nexus = HFC_CANCEL_ITLNEXUS;	/* FCLNX-GPL-FX-014 */
					
					clear_bit(HFC_DC_NEED_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-014 */
					set_bit(HFC_DC_WAIT_LUN_RESET_OR_ABORT, (ulong *)&dev->dev_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-014 */
					clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
					set_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
				}
				HFC_DBGPRT("hfc_fx_start xob dump\n");
				structdump( 0xef, (uchar *)xob_p, sizeof(struct xob_fx) );	
			}
			core->tskmgm_cmd_num++;
		}else{
			xob_p->flag |= HFC_XOB_I;	/* FCLNX-GPL-FX-014 */
		}
		
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) {	/* FCLNX-FX-031 */
			if((cmnd != NULL) 
			&& (((uchar)cmnd->cmnd[0] == 0x5E) || ((uchar)cmnd->cmnd[0] == 0x5F))){
				HFC_DBGPRT("hfc_fx_start : core->ppreserve_cmd_num = %d\n",core->preserve_cmd_num);
				core->preserve_cmd_num++;
			}
		}	/* FCLNX-FX-031 */
		
		hfc_fx_make_cmdiu( pp, hfcp );
		
		/* Setup Xob */
		hfcp_wk = hfcp->cmd_forw;
		hfc_fx_xob_enque(pp, core, target, hfcp) ;
		
		if (pp->pm_control == HFC_FX_PM_ON) {
			if (hfcp->pm_pkt_no != 0xffff) {
				rdtscll(pp->pm_pkt_pool[hfcp->pm_pkt_no].tsc_enq_xob);
				pp->pm_pkt_pool[hfcp->pm_pkt_no].cpuno_enq_xob = cpuno;
			}
		}
		
		if ( HFC_HFCP_FX_CFLAG_TEST(CFLAG_ABORT, hfcp)				/* Abort Task Set */
		  || HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ) {		/* LUN reset */
			hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_ABORT_TMR,0,TRUE );
			hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_ABORT_TMR,0,FALSE );
			HFC_DBGPRT("hfc_fx_start xob dump\n");
			structdump( 0xef, (uchar *)xob_p, sizeof(struct xob_fx) );	
		}

		else if ( HFC_HFCP_FX_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)	/* Target Reset from eh_device_reset */
			   || HFC_HFCP_FX_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) ) {	/* Target Reset from eh_bus_reset */
			hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_TARGET_RST_TMR,0,TRUE );
			hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_TARGET_RST_TMR,0,FALSE );
			HFC_DBGPRT("hfc_fx_start xob dump\n");
			structdump( 0xef, (uchar *)xob_p, sizeof(struct xob_fx) );	
		}
		else if( hfcp->cmd_flags & CFLAG_CSCSI_ANY){	/* FCLNX-GPL-FX-014 */
			hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_CANCEL_SCSI_TMR,0,TRUE );
			hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_CANCEL_SCSI_TMR,0,FALSE );
			HFC_DBGPRT("hfc_fx_start xob dump\n");
			structdump( 0xef, (uchar *)xob_p, sizeof(struct xob_fx) );	
		}	/* FCLNX-GPL-FX-014 */
		else {
			if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_HSDLDD_VALID, hfcp)) {
				wk_cmnd = hfcp->cmd_pkt;
				sdev = wk_cmnd->device;
				rq = sdev->request_queue;
				hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_SCSI_CMD_TMR, hfcp->timeout, TRUE );	/* FCLNX-GPL-FX-262,272 */
				hfc_fx_watchdog_enter( pp, core, target, hfcp, 0, HFC_FX_SCSI_CMD_TMR, hfcp->timeout, FALSE );	/* FCLNX-GPL-FX-262,272 */
			}
			else {
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
					hfc_manage_info.npubp->hfc_fx_set_scsi_cmd_tmr(pp, core, target, hfcp);	/* FCLNX-GPL-FX-446 */
				}
			}
		}
		
		xob_wait_exec_cnt++;
		hfcp_trc = hfcp;		/* FCLNX-GPL-FX-061 */
		hfcp = hfcp_wk;
		if (use_reserve_reset_xob) {	/* FCLNX-GPL-FX-014 */
			break;
		}	/* FCLNX-GPL-FX-014 */
	}

	/* Check commands in xob waiting for initiation */
	if( xob_wait_exec_cnt == 0 )
	{
		hfc_fx_stra_trace(
			HFC_FX_TRC_START ,0x12 ,pp ,rp ,core, target ,NULL ,
				0, 0, 0) ;
		hfc_fx_enque_next_dstart(pp, rp, core, target) ;
		return;
	}
	
	if (use_reserve_reset_frame == 0) {	/* FCLNX-GPL-FX-014 */
		/* update frame pointer */
		if ((core->frame_inp + 1) >= MAX_FX_FRAME_CNT-1) {	/* FCLNX-GPL-FX-014 */
			/* frame_inp wraparound case */
			core->frame_inp = 0;
		}
		else {
			core->frame_inp++;
		}
	}	/* FCLNX-GPL-FX-014 */
	
	core->frame_start_xob[lp].start = initial_xob_no;
	core->frame_start_xob[lp].num = xob_wait_exec_cnt;
	core->frame_start_xob[lp].pkt_no = ((struct hfc_pkt_fx *)(ulong)core->xob[initial_xob_no].drv_work)->pkt_no;
	((struct hfc_pkt_fx *)(ulong)core->xob[initial_xob_no].drv_work)->frame_no = lp;	/* FCLNX-GPL-FX-014 */
	
	/* Initiate XOB */
	/* Set XOB_ENQ command and the number of initiated XOB in frame A */
	xob_exec = xob_wait_exec_cnt ;
	xob_exec |= (uint)( (initial_xob_no << 8) & 0x0000ff00) ;
	xob_exec |= (uint)( (pp->rid << 16) & 0x00ff0000) ;
	xob_exec |= HFC_FRAMEA_ENQ_XOB ;
	
	/* Issue Enqueue_XOB */
//	hfc_fx_write_reg_ext(pp,( uint )hfc_framea_of_core[core->core_no],( char )0x4, ( int )xob_exec );
	hfc_fx_write_reg_core(pp, core->core_no, (uint)HFC_IOSPACE_FRAMEA,
						  (char)0x4, (int)xob_exec, HFC_FX_CORE_OFFSET40);
	
	if (target->core_queue[core->core_no].wx_que_cnt) {
		if (!target->core_queue[core->core_no].next_dstart_flag) {
			hfc_fx_enque_next_dstart(pp, rp, core, target) ;
		}
	}
	else {
		if (target->core_queue[core->core_no].next_dstart_flag) {
			hfc_fx_deque_next_dstart(pp, rp, core, target);
		}
	}
	
	if (!(pp->debug_func & HFC_FX_DEBUG_IOTRACE_OFF)) {
		hfc_fx_stra_trace(
			HFC_FX_TRC_START,0x10, pp, rp, core, target, hfcp_trc,		/* FCLNX-GPL-FX-061 */
			(uint64_t)xob_wait_exec_cnt, (uint64_t)initial_xob_no, 0);	/* FCLNX-GPL-FX-061 */
	}

	return;
}


/*
 * Function:    hfc_fx_get_new_hfcp
 *
 * Purpose:     Search empty hfc_pkt_fx 
 *
 * Arguments:   
 *  pp          Pointer to port_info
 *
 * Returns:     
 *
 * Notes:       
 */
struct hfc_pkt_fx *
hfc_fx_get_new_hfcp(struct port_info *pp)
{
	int   i;
	ushort pkt_no;
	struct hfc_pkt_fx		*hfcp;
	struct hfc_pkt_fx		*pkt_prev;
	struct hfc_pkt_fx		*pkt_next;
	struct hfc_pm_pkt_fx	*pm_hfcp;
	
	for (i=0;i<pp->pport->pkt_num;i++)
	{
		hfcp = pp->pport->pkt_next;
		if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_VALID, hfcp)) {
			pp->pport->pkt_next = hfcp->pkt_next;
			if (hfcp->pkt_next == NULL) {
				pp->pport->pkt_next = pp->pport->pkt_top;
			}
			pkt_prev = hfcp->pkt_prev;
			pkt_next = hfcp->pkt_next;
			pkt_no = hfcp->pkt_no;
			memset(hfcp, 0, sizeof(struct hfc_pkt_fx));
			set_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags );
			hfcp->pkt_prev = pkt_prev;
			hfcp->pkt_next = pkt_next;
			hfcp->pkt_no = pkt_no;
			if (pp->pm_control == HFC_FX_PM_ON) {
				pm_hfcp = hfc_fx_get_new_pm_hfcp(pp);
				if (pm_hfcp != NULL) {
					hfcp->pm_pkt_no = pm_hfcp->pm_pkt_no;
				}
				else {
					hfcp->pm_pkt_no = 0xffff;
				}
			}
			return (hfcp);
		}
		pp->pport->pkt_next = hfcp->pkt_next;
		if (hfcp->pkt_next == NULL) {
			pp->pport->pkt_next = pp->pport->pkt_top;
		}
	}
	
	if (HFC_FX_VPORT_EXIST(pp)) {
		return (hfc_fx_get_new_rsv_hfcp(pp));
	}
	else {
		return (NULL);
	}
}


struct hfc_pkt_fx *
hfc_fx_get_new_rsv_hfcp(struct port_info *pp)
{
	int   i;
	struct hfc_pkt_fx	*hfcp;
	
	for (i=0;i<pp->rsv_pkt_num;i++)
	{
		hfcp = &pp->rsv_pkt_pool[pp->rsv_pkt_no];
		if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_VALID, hfcp)) {
			pp->rsv_pkt_no++;
			if (pp->rsv_pkt_no >= pp->rsv_pkt_num) {
				pp->rsv_pkt_no = 0;
			}
			/* FCLNX-GPL-FX-312,335 */
			memset(hfcp, 0, sizeof(struct hfc_pkt_fx));
			set_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags );
			hfcp->pm_pkt_no = 0xffff;
			
			return (hfcp);
		}
		
		pp->rsv_pkt_no++;
		if (pp->rsv_pkt_no >= pp->rsv_pkt_num) 
			pp->rsv_pkt_no = 0;
	}

	return (NULL);
}


struct hfc_pm_pkt_fx *
hfc_fx_get_new_pm_hfcp(struct port_info *pp)
{
	struct hfc_pm_pkt_fx	*pm_hfcp;
	
	pm_hfcp = &pp->pm_pkt_pool[pp->pm_pkt_no];
	if (pm_hfcp == NULL)
		return(NULL);
	if (!HFC_HFCP_FX_CFLAG_TEST(CFLAG_VALID, pm_hfcp)) {
		memset(pm_hfcp, 0x00, sizeof(struct hfc_pm_pkt_fx));
		set_bit(CFLAG_VALID, (ulong *)&pm_hfcp->cmd_flags );	/* FCLNX-GPL-FX-343 */
		pm_hfcp->pm_pkt_no = pp->pm_pkt_no;
		pp->pm_pkt_no++;
		if (pp->pm_pkt_no >= pp->pm_pkt_num) {
			pp->pm_pkt_no = 0;
		}
		return (pm_hfcp);
	}
	
	return (NULL);
}


/*
 * Function:    
 *
 * Purpose:     Search empty dummy command 
 *
 * Arguments:   
 *  pp          Pointer to port_info
 *
 * Returns:     
 *
 * Notes:       
 */
struct scsi_cmnd *
hfc_fx_get_new_cmnd(struct port_info *pp)
{
	struct scsi_cmnd		*cmnd;
	struct dummy_scsi_cmnd	*dummy_cmnd;
	uchar					logdata[16];
	
	dummy_cmnd = hfc_fx_kmalloc(pp, sizeof(struct dummy_scsi_cmnd), GFP_ATOMIC);
	if(dummy_cmnd == NULL){
		memset(logdata, 0, 16);
		logdata[0] = 0xe0;
		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return (NULL);
	}
	memset(dummy_cmnd, 0, sizeof(struct dummy_scsi_cmnd));
	cmnd = (struct scsi_cmnd *)&dummy_cmnd->cmnd;
	cmnd->device = (struct scsi_device *)&dummy_cmnd->device;
	cmnd->cmnd   = (uchar *)&dummy_cmnd->cdb[0];
	
	return (cmnd);
}


/*
 * Function:    hfc_fx_dummy_copy
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
void hfc_fx_dummy_copy(struct port_info *pp, struct scsi_cmnd *cmnd, struct scsi_cmnd *dummy_cmnd )
{
//	dummy_cmnd->device = (struct scsi_device*)hfc_fx_kmalloc(pp, sizeof(struct scsi_device), GFP_ATOMIC);
//	dummy_cmnd->cmnd = (uchar *)hfc_fx_kmalloc(pp, 16, GFP_ATOMIC);

	if(dummy_cmnd->device == NULL) 
	{
		return;
	}

	if(dummy_cmnd->cmnd == NULL) 
	{
		return;
	}

	CMND_TARGET(dummy_cmnd) = CMND_TARGET(cmnd);

	CMND_LUN(dummy_cmnd) = CMND_LUN(cmnd);
	CMND_CHANNEL(dummy_cmnd) = CMND_CHANNEL(cmnd);

	dummy_cmnd->cmd_len = cmnd->cmd_len;
	dummy_cmnd->cmnd[0] = cmnd->cmnd[0];
	dummy_cmnd->sc_data_direction = cmnd->sc_data_direction;
	/* kernel 5.16+: scsi_cmnd->scsi_done removed */
	dummy_cmnd->result = 0;		/* FCLNX-GPL-0343 */
	set_bit(CMND_VALID, (ulong *)&dummy_cmnd->eh_eflags );

}

/*
 * Function:    hfc_fx_eh_abort_pg
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
int hfc_fx_eh_abort_pg(struct scsi_cmnd *cmnd)
{
	return(SUCCESS);
#if 0
	struct	port_info	*pp ;
	struct	core_info	*core = NULL;
	struct	target_info_fx	*target ;
	struct hfc_pkt_fx	*hfcp = NULL, *abort_pkt=NULL;
	struct hfc_pkt_fx	*wx_hfcp = NULL;
	struct scsi_cmnd	*dummy_cmnd=NULL;
	uint                lun,find;
	unsigned long		flags = 0;									/* FCLNX-0274 */
	struct dev_info_fx		*dev=NULL;			/* FCLNX-GPL-0343 */
	ushort				cmd_lun;			/* FCLNX-GPL-0548 */

	/* Check argument(NULL?) */
	if( cmnd == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - invalid argument \n");
		return(FAILED);
    }

	spin_lock_irq(cmnd->device->host->host_lock);

	pp = (struct port_info *) CMND_HOSTDATA(cmnd);
	dev = (struct dev_info_fx *) CMND_DEV(cmnd);

	HFC_DBGPRT(" hfcldd%d : hfc_fx_eh_abort - start channel=%d, tid=%d, lun=%d. \n",
					pp->dev_minor, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd), CMND_LUN(cmnd));
	
	/* Check port_info (NULL?) */
	if( pp == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - 1\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }
    
    HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	hfc_fx_stra_trace(HFC_FX_TRC_ABORT ,0x00 ,pp ,NULL , NULL, (ulong)cmnd, 0, 0);
	
	/* Find hfc_pkt_fx */
	hfcp = (struct hfc_pkt_fx *)cmnd->host_scribble;
	if( hfcp == NULL ){
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - already complete \n");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}
	
	cmd_lun = (ushort)CMND_LUN(cmnd);				/* FCLNX-GPL-0548 */
    cmd_lun = (cmd_lun & 0x3fff);					/* FCLNX-GPL-0548 */
    
	/* Check each element of scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= pp->max_target)
	  || (CMND_TARGET(cmnd) == pp->hosts->this_id)
	  || (cmd_lun >= MAX_DEV_CNT) )					/* FCLNX-GPL-0548 */
	{
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}

	if ( !(test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) ) )
    {
		/* Adapter is not initialized */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - not attach status \n");

		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }

	if ( !test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) )
	{
		/* Adapter is not online (link down) */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - HFC_PS_ONLINE=0 \n");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}

	if( test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 ) )
	{
		/* Adapter is executing BUS RESET */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - exec bus reset\n");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}

	target = hfc_fx_hash_target_info(pp, CMND_TARGET(cmnd));
	lun = CMND_LUN(cmnd);

	/* Check target_info_fx */
	if(target == NULL)
	{
		HFC_DBGPRT(KERN_ERR"hfcldd : hfc_fx_eh_abort - target == NULL \n");

		hfc_fx_stra_trace(HFC_FX_TRC_ABORT ,0x31 ,pp , target, hfcp, (ulong)cmnd, 0, 0);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	} 
	
	/* Check Timeout command */									/* FCLNX-GPL-0153 */
	wx_hfcp = hfcp->core->wx_que_top[target->target_id];
	
	while( wx_hfcp != NULL )
	{
		if( wx_hfcp == hfcp )
			break;
		wx_hfcp = wx_hfcp->cmd_forw;
	}
	
	if( wx_hfcp != NULL )
	{
		hfc_fx_deque_wx_que(hfcp->core, wx_hfcp);
		set_bit( CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );
		hfc_fx_iodone(pp, wx_hfcp->cmd_pkt, wx_hfcp);
		HFC_DBGPRT(KERN_ERR"hfcldd : hfc_fx_eh_abort - Timeout command remains in wx_que.\n");
		hfc_fx_stra_trace(HFC_FX_TRC_ABORT ,0x32 ,pp , target, hfcp, (ulong)cmnd, 0, 0);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}															/* FCLNX-GPL-0153 */
	
	/* Target state is in NEED_LOGIN or PDISC is running */
	if (target->status)
	{
		/* target status is non zero */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - target status non zero \n");
	
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}

	set_bit( CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags );

	if ((test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat))
		||(test_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat)))
	{
		/* Already issue abort task set */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - HFC_DS_WAIT_ABORT/HFC_NEED_ABORT \n");

		hfc_fx_stra_trace(HFC_FX_TRC_ABORT ,0x45 ,pp , target, hfcp, (ulong)cmnd, 0, 0);
		goto WAIT_ABORT_TS;
	}
	
	if( hfc_fx_toutchk_xob(pp, target, hfcp, lun, HFC_ISSUE_ABORT) )
	{
		/* Timeout command remains in xob. */
		hfc_fx_stra_trace(HFC_FX_TRC_ABORT, 0x35, pp, target, hfcp, (ulong)cmnd, 0, 0);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED); ;
	}

	if ( !test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ) {	/* FCLNX-GPL-035 */
		if ( hfc_fx_issue_mihlog( pp, hfcp) == 0)
		{
			/* Initiate MIH-LOG successfully */
			HFC_DBGPRT("eh_abort mihlog successed. lun = %d \n",lun);
//			dev->lustat |= HFC_NEED_ABORT | HFC_DEFER_ABORT;
			set_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);

			hfc_fx_stra_trace(HFC_FX_TRC_ABORT, 0x70, pp, target, hfcp, (ulong)cmnd, 0, 0);
			goto WAIT_ABORT_TS;
		}
	}
	else {
		HFC_DBGPRT("skip eh_abort mihlog. lun = %d \n",lun);
	}																	/* FCLNX-GPL-035 */

	/* Initiate ABORT TASK SET */
	abort_pkt = hfc_fx_get_new_hfcp(pp);
	if( abort_pkt == NULL)
	{
		/* Hfc_pkt_fx is empty */
		HFC_DBGPRT(" hfcldd : hfc_fx_eh_abort - hfcp==NULL-error\n");

		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	dummy_cmnd = hfc_fx_get_new_cmnd(pp);
	if( dummy_cmnd == NULL )
	{
		/* Dummy scsi_cmnd is empty */
		HFC_DBGPRT(" hfcldd : hfc_fx_eh_abort - dummy_cmnd==NULL-error\n");

		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	memset(abort_pkt, 0, sizeof(struct hfc_pkt_fx));
//	memset(dummy_cmnd, 0, sizeof(struct scsi_cmnd));				/* FCLNX-GPL-0343 */
	memset(dummy_cmnd->cmnd, 0, 16 );								/* FCLNX-GPL-0343 */

	set_bit(CFLAG_VALID, (ulong *)&abort_pkt->cmd_flags);
	set_bit(CFLAG_ABORT, (ulong *)&abort_pkt->cmd_flags);

	abort_pkt->cmd_pkt   = dummy_cmnd;
	abort_pkt->target_id = target->target_id;
	abort_pkt->lun_id    = CMND_LUN(cmnd);
	abort_pkt->pp        = pp;
	abort_pkt->target    = target;
	abort_pkt->dev       = NULL;
	core = hfc_fx_choose_core(pp,abort_pkt);
	dummy_cmnd->host_scribble = (void *)abort_pkt;
	hfc_fx_dummy_copy(pp, cmnd, dummy_cmnd );

	hfc_fx_enqueue_wx_que(core, abort_pkt);
	
	if ( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ) {			/* FCLNX-GPL-0153 */
		HFC_DBGPRT(" hfcldd : hfc_fx_eh_abort - mck is progress\n");
		set_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
		
		hfc_fx_stra_trace(HFC_FX_TRC_ABORT, 0x75, pp, target, abort_pkt, (ulong)dummy_cmnd, 0, 0);
		goto WAIT_ABORT_TS;
	}																	/* FCLNX-GPL-0153 */
	
	/* Issue SCSI command if target is normal */
	set_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);

	hfc_fx_start( pp, hfcp->core, target );

		hfc_fx_stra_trace(HFC_FX_TRC_ABORT, 0x74, pp, target, abort_pkt, (ulong)dummy_cmnd, 0, 0);
	
WAIT_ABORT_TS:

	HFC_DBGPRT("hfc_fx_eh_abort : wait abort task set complete \n");

	do {
		find = FALSE;
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(cmnd->device->host->host_lock);
		HFC_PORTLOCK_IRQSAVE(pp,flags);

		if ( (test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat))
		  || (test_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat)) )
			find = TRUE;

	} while (find != FALSE);

	hfc_fx_stra_trace(HFC_FX_TRC_ABORT ,0x10 ,pp ,target ,abort_pkt, (ulong)dummy_cmnd, 0, 0);
	HFC_DBGPRT("hfcldd : hfc_fx_eh_abort - end\n");
	if( (test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status ))||
		(test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status ))||
		(test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status ))||		/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
		(test_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status ))||		/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
		(test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status ))||			/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
		(test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status )))			/* FCLNX-GPL-038 *//* FCLNX-GPL-FX-014 */
	{
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
#endif
}

/*
 * Function:    hfc_fx_eh_device_reset_pg
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
int hfc_fx_eh_device_reset_pg(struct scsi_cmnd *cmnd)
{
	struct	port_info	*pp=NULL;
	struct	port_info	*vpp=NULL;
	struct	core_info	*core=NULL;
	struct	region_info *rp=NULL;
	struct	target_info_fx	*target=NULL ;
	int                 rtn;
	ulong				flags = 0;
	uint				lun_id=0, i=0, j=0;
	struct dev_info_fx	*dev=NULL;		/* FCLNX-GPL-0343 */
	ushort				cmd_lun;		/* FCLNX-GPL-0548 */
	int					pkt_cnt;

	/*-- check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		/*-- invalid argument --*/
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - invalid argument \n");
		return(FAILED);
    }
	
	if( cmnd->device == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - scsi_device null \n");
		return(FAILED);
	}
	
	if( cmnd->device->host == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - Scsi_Host null \n");
		return(FAILED);
	}
	
	dev = (struct dev_info_fx *) CMND_DEV(cmnd);				/* FCLNX-GPL-0343 */
	if( dev == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - dev_info_fx null \n");
		return(FAILED);
	}														/* FCLNX-GPL-0343 */
	
	cmd_lun = (ushort)CMND_LUN(cmnd);				/* FCLNX-GPL-0548 */
    cmd_lun = (cmd_lun & 0x3fff);					/* FCLNX-GPL-0548 */

	spin_lock_irq(cmnd->device->host->host_lock);

	pp = (struct port_info *) CMND_HOSTDATA(cmnd);

	HFC_DBGPRT(" hfcldd%d : hfc_fx_eh_device_reset_pg - start rid=0x%02x, channel=%d, tid=%d, lun=%d. \n",
						pp->dev_minor, pp->rid, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd), CMND_LUN(cmnd));

	if( pp == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - pp null\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }
	
	rp = pp->region_arg[pp->rid];
	
    if ( rp == NULL ) { /* region_info null */
		HFC_DBGPRT( "hfcldd : hfc_fx_eh_device_reset_pg - region_info==NULL.\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}

	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x00 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);

	/* Check scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= pp->max_target)
	  || (CMND_TARGET(cmnd) == pp->hosts->this_id)
	  || (cmd_lun >= MAX_DEV_CNT) )					/* FCLNX-GPL-0548 */
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - scsi_cmnd invalid\n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	lun_id = CMND_LUN(cmnd);
	
	if ( !test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - not attach status \n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }
    
	target = hfc_fx_hash_target_info(pp, CMND_TARGET(cmnd));

	/* Target Reset Mode */
	if( !test_bit( HFC_PS_ONLINE, (ulong *)&pp->status )
	||   test_bit( HFC_PS_ISOL, (ulong *)&pp->status ) )	/* FCLNX-GPL-572 */
	{
		/* Executing machine check recovery */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - Link Down \n");
		hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x21 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);
			
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);											/* FCLNX-GPL-469 */
	}
	
	if( test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 ) )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - bus reset is progress\n");

		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			hfc_fx_cancel_scsi_cmd(pp, core, target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, TRUE,
				FALSE,FALSE, HFC_FLASH_DEV );
		}
		if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
			hfc_fx_mq_cancel_scsi_cmd(pp, target, CMND_LUN(cmnd), NULL, SCS_WAIT_RESET, TRUE,
				TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_DEV);
		}
		hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x30 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	if (target == NULL)
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - target null\n");
		hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x31 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);

		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}
	
	if ( test_bit(HFC_TS_WAIT_TGTRSP, (ulong *)&target->status) )			/* FCLNX_GPL-0050 */
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - device reset is progress\n");

		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			hfc_fx_cancel_scsi_cmd(pp,core,target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, TRUE,
				FALSE,FALSE, HFC_FLASH_DEV );
		}
		if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
			hfc_fx_mq_cancel_scsi_cmd(pp, target, CMND_LUN(cmnd), NULL, SCS_WAIT_RESET, TRUE,
				TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_DEV);
		}
		hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x34 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	/* check pkt_pool */
	pkt_cnt = 0;
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
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - pkt_num=%d, pkt_cnt=%d\n",
						pp->pport->pkt_num, pkt_cnt);
		
		hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x36 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - pkt_num=%d, pkt_cnt=%d\n",
						pp->pport->pkt_num, pkt_cnt);

	if ( test_bit(HFC_DS_WAIT_LUNRSP, (ulong *)&dev->lustat) )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - lun reset is in progress\n");

		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			hfc_fx_cancel_scsi_cmd(pp,core,target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, TRUE,
				FALSE,FALSE, HFC_FLASH_DEV );
		}
		if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
			hfc_fx_mq_cancel_scsi_cmd(pp, target, CMND_LUN(cmnd), NULL, SCS_WAIT_RESET, TRUE,
				TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_DEV);
		}
		hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x35 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);
		goto WAIT_LUN_RST;
	}
	
	/* Cancel wait queue of SCSI command and Task Management */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		hfc_fx_cancel_scsi_cmd(pp,core,target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
			FALSE,TRUE, HFC_FLASH_DEV );
	}
	if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
		hfc_fx_mq_cancel_scsi_cmd(pp, target, CMND_LUN(cmnd), NULL, SCS_WAIT_RESET, TRUE,
			TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_DEV);
	}
	
	/* FCLNX-GPL-FX-014 Start */
	/* Start LUN Reset Process */
	set_bit(HFC_DS_WAIT_LUNRSP, (ulong *)&dev->lustat);
	set_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
		
	dev->lun_reset_cmnd = cmnd;
	
	/* Issue CANCEL_SCSI_WITHOUT_DMA */
	dev->abtcmd_core_no = pp->master_core_no;
	if(hfc_fx_issue_devrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_CSCSI_LU_WITHOUT_DMA))){
		hfc_fx_issue_devrst_cscsi(pp, target, dev, (0x00000001 << CFLAG_LUN_RESET));
	}
	/* FCLNX-GPL-FX-014 End */
	
WAIT_LUN_RST :

	/* Wait CANCEL_SCSI witch*/
	while (1) {
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(cmnd->device->host->host_lock);
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

		
		if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags)  )
		{
			rtn = SUCCESS;
			HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - 1 %d, %04x\n", dev->lun, dev->lustat);
			break;
		}
		
		if ( !test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status)
		&&  !test_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status) 
		&&  !test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)
		&&  !test_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)
		&&  !test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status)
		&&  !test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status)){
			
			if ( !test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat)
			  && !test_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat)
			  && !test_bit(HFC_DS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&dev->lustat)
			  && !test_bit(HFC_DS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&dev->lustat)
			  && !test_bit(HFC_DS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat)
			  && !test_bit(HFC_DS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat)  )
			{
				if(test_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat)){	/* FCLNX-GPL-FX-085 */
					rtn = FAILED;
					clear_bit(HFC_DS_FAIL_LUN_RST, (ulong *)&dev->lustat);
					HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - 3 %d, %04x\n", dev->lun, dev->lustat);
				}else{	/* FCLNX-GPL-FX-085 */
					rtn = SUCCESS;
					HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - 2 %d, %04x\n", dev->lun, dev->lustat);
				}
				break;
			}
		}
	}
	
	clear_bit(HFC_DS_WAIT_LUNRSP, (ulong *)&dev->lustat);
	HFC_DBGPRT("hfcldd : hfc_fx_eh_device_reset_pg - end \n");
	
	hfc_fx_stra_trace(HFC_FX_TRC_LUN_RESET ,0x10 ,pp ,NULL , NULL, NULL, NULL, (ulong)cmnd, 0, 0);
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	spin_unlock_irq(cmnd->device->host->host_lock);

	return(rtn);
}


/* FCLNX-GPL-0343 */
/*
 * Function:    hfc_fx_eh_target_reset_pg
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
int hfc_fx_eh_target_reset_pg(struct scsi_cmnd *cmnd)
{
	struct	port_info	*pp=NULL ;
	struct	target_info_fx	*target=NULL ;
	struct	core_info	*core=NULL ;
	struct	region_info *rp=NULL;
	int                 rtn=0, i=0;
	ulong				flags = 0;
	
	/*-- check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		/*-- invalid argument --*/
		HFC_DBGPRT("hfcldd : hfc_fx_target_reset - invalid argument \n");
		return(FAILED);
    }
	
	if( cmnd->device == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_target_reset - scsi_device null \n");
		return(FAILED);
	}
	
	if( cmnd->device->host == NULL )
	{
		HFC_DBGPRT("hfcldd : fx_eh_target_reset - Scsi_Host null \n");
		return(FAILED);
	}														/* FCLNX-GPL-0343 */

	spin_lock_irq(cmnd->device->host->host_lock);

	pp = (struct port_info *) CMND_HOSTDATA(cmnd);

	HFC_DBGPRT(" hfcldd%d : hfc_fx_eh_target_reset - start channel=%d, tid=%d. \n",
						pp->dev_minor, CMND_CHANNEL(cmnd), CMND_TARGET(cmnd));

	if( pp == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - 1\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }
	
	rp = pp->region_arg[pp->rid];
	
    if ( rp == NULL ) { /* region_info null */
		HFC_DBGPRT( "hfcldd : fx_eh_target_reset - region_info==NULL.\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x00 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);

	/* Check scsi_cmnd structure */
	if ( (CMND_CHANNEL(cmnd) != 0)
	  || (CMND_TARGET(cmnd) >= pp->max_target)
	  || (CMND_TARGET(cmnd) == pp->hosts->this_id) )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - 3\n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}


	if ( !test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - not attach status \n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }
    
	target = hfc_fx_hash_target_info(pp, CMND_TARGET(cmnd));

	/* Target Reset Mode */
	if( !test_bit( HFC_PS_ONLINE, (ulong *)&pp->status )
	||   test_bit( HFC_PS_ISOL, (ulong *)&pp->status ) )	/* FCLNX-GPL-572 */
	{
		/* Executing machine check recovery */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - Link Down \n");
		hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x21 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);
			
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);											/* FCLNX-GPL-469 */
	}
	
	if( test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 ) )
	{
		hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x30 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	

	if (target == NULL)
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - 10\n");
		hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x31 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);

		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}

	/* FCLNX-GPL-FX-014 Start */
	if (target->dev == NULL)
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - 11\n");
		hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x32 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);

		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(SUCCESS);
	}
	/* FCLNX-GPL-FX-014 End */
	
	if ( test_bit(HFC_TS_WAIT_TGTRSP, (ulong *)&target->status) )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - 13\n");

		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			hfc_fx_cancel_scsi_cmd(pp,core,target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, TRUE,
				FALSE,FALSE, HFC_FLASH_TARGET );
		}
		if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
			hfc_fx_mq_cancel_scsi_cmd(pp, target, CMND_LUN(cmnd), NULL, SCS_WAIT_RESET, TRUE,
				TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_TARGET);
		}
		hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x34 ,pp , rp, NULL, target, NULL, (ulong)cmnd, 0, 0);
		goto WAIT_DEV_RST;
	}
	
	/* Cancel wait queue of SCSI command and Task Management */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		hfc_fx_cancel_scsi_cmd(pp,core,target,CMND_LUN(cmnd),NULL,SCS_WAIT_RESET, HFC_CSCSI_RESET,
			FALSE,TRUE, HFC_FLASH_TARGET );
	}
	if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
		hfc_fx_mq_cancel_scsi_cmd(pp, target, CMND_LUN(cmnd), NULL, SCS_WAIT_RESET, HFC_CSCSI_RESET,
			TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_TARGET);
	}
	
	/* FCLNX-GPL-FX-014 Start */
	set_bit( HFC_TS_WAIT_TGTRSP, (ulong *)&target->status );		
	target->target_reset_cmnd = cmnd;
	target->tgtrst_core_no = pp->master_core_no;
	set_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status);
	
	/* Issue CANCEL_SCSI */
	if(hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_CSCSI_TGT_WITHOUT_DMA))){
		hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_TARGET_RESET));
	}
	/* FCLNX-GPL-FX-014 End */
	
WAIT_DEV_RST :
	HFC_DBGPRT("hfc_fx_eh_target_reset : wait Target Reset complete \n");

	while (1) {
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ/10+1);					/* Wait 100ms */

		spin_lock_irq(cmnd->device->host->host_lock);
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

		if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags)
		  || !test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags)  )
		{
			rtn = SUCCESS;	
			HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset_pg - 1 %d, %04x\n", target->dev->lun, target->status);
			break;
		}

		if ( !test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)	
		 &&  !test_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)
		 &&  !test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status)	
		 &&  !test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status)
		 &&  !test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status)
		 &&  !test_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status) ) 
		{	/* FCLNX-GPL-FX-014 */
			if(test_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags)){	/* FCLNX-GPL-FX-085 */
				rtn = FAILED;
				clear_bit(HFC_TF_FAIL_TARGET_RESET, (ulong *)&target->flags);
				HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset_pg - 3 %d, %04x\n", target->dev->lun, target->status);
			}else{	/* FCLNX-GPL-FX-085 */
				rtn = SUCCESS;
				HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset_pg - 2 %d, %04x\n", target->dev->lun, target->status);
			}
			break;
		}
	}

	clear_bit(HFC_TS_WAIT_TGTRSP, (ulong *)&target->status);	
	HFC_DBGPRT("hfcldd : hfc_fx_eh_target_reset - end \n");
	
	hfc_fx_stra_trace(HFC_FX_TRC_TGT_RESET ,0x10 ,pp , rp, NULL, target, NULL, (ulong)cmnd, 0, 0);
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	spin_unlock_irq(cmnd->device->host->host_lock);
	return(rtn);
}

/*
 * Function:    hfc_fx_eh_bus_reset_pg
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
int hfc_fx_eh_bus_reset_pg(struct scsi_cmnd *cmnd)
{
	struct port_info	*pp ;
	struct target_info_fx	*target=NULL;
	struct core_info	*core;
	struct	region_info *rp=NULL;
	int					lp=0, i=0;
	ulong				flags = 0;
	uchar bus_reset_imcomplete = FALSE;
	uchar				find;

	HFC_DBGPRT(" hfcldd : hfc_fx_eh_bus_reset - start channel=%d. \n" ,CMND_CHANNEL(cmnd));

	/*-- Check argument(NULL?) --*/
	if( cmnd == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - invalid argument \n");
		return(FAILED);
    }
	
	if( cmnd->device == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - scsi_device null \n");
		return(FAILED);
	}
	
	if( cmnd->device->host == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - Scsi_Host null \n");
		return(FAILED);
	}														/* FCLNX-GPL-0343 */
	
	spin_lock_irq(cmnd->device->host->host_lock);

	pp = (struct port_info *) CMND_HOSTDATA(cmnd);

	if ( pp == NULL )
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - invalid port_info pointer \n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	rp = pp->region_arg[pp->rid];
	
    if ( rp == NULL ) { /* region_info null */
		HFC_DBGPRT( "hfcldd : hfc_fx_eh_bus_reset - region_info==NULL.\n");
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	hfc_fx_stra_trace(HFC_FX_TRC_BUS_RESET ,0x00 ,pp , rp, NULL , NULL, NULL, (ulong)cmnd, 0, 0);
	
	if (CMND_CHANNEL(cmnd) != 0)
	{
		/* Channel number is invalid */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - invalid channel number \n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}
	
	if( !test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) )
    {
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - not attach status \n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
    }

	if( !test_bit( HFC_PS_ONLINE, (ulong *)&pp->status )
		||   test_bit( HFC_PS_ISOL, (ulong *)&pp->status ) )		/* FCLNX-GPL-572 */
	{
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - Link Down in Target Reset \n");
		hfc_fx_stra_trace(HFC_FX_TRC_BUS_RESET ,0x32 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);

		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		spin_unlock_irq(cmnd->device->host->host_lock);
		return(FAILED);
	}

	if( test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 ) )
	{
		/* Already issued BUS RESET */
		HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - already reset bus \n");
		hfc_fx_stra_trace(HFC_FX_TRC_BUS_RESET ,0x30 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);
		
		bus_reset_imcomplete = TRUE;
		goto WAIT_BUS_RST;
	}

	/* Start BUS RESET */
	HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - start reset bus \n");
	set_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 );		/* Set wait bus reset flag */
	
	for ( lp=0; lp<MAX_TARGET_PROBE; lp++ )
	{
		if ( (target = hfc_fx_hash_target_info(pp, lp)) != NULL )
		{
			/* 
			 * (1) Cancel waiting status of SCSI initiation.
			 * (2) Cancel waiting status of SCSI response when LOGIN completes 
			 */
			/* Cancel waiting queue of SCSI initialization and Task Management */
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				hfc_fx_cancel_scsi_cmd(pp,core,target,0,NULL,SCS_WAIT_BUS_RESET, HFC_CSCSI_RESET,
					FALSE,TRUE, HFC_FLASH_TARGET );
			}
			if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {
				hfc_fx_mq_cancel_scsi_cmd(pp, target, 0, NULL, SCS_WAIT_BUS_RESET, TRUE,
					TRUE, FALSE, FALSE, FALSE, FALSE, HFC_FLASH_TARGET);
			}
			
			if (!test_bit(HFC_TS_WAIT_TGTRSP, (ulong *)&target->status)) {	/* FCLNX-GPL-FX-284, 290 */
				/* FCLNX-GPL-FX-014 Start */
				target->target_reset_cmnd = cmnd;
				target->tgtrst_core_no = pp->master_core_no;
				set_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status);
				
				if(hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_CSCSI_TGT_WITHOUT_DMA))){
					hfc_fx_issue_tgtrst_cscsi(pp, target, target->dev, (0x00000001 << CFLAG_TARGET_RESET));
				}
				/* FCLNX-GPL-FX-014 End */
			}
			
			bus_reset_imcomplete = TRUE;
		}
	}

WAIT_BUS_RST :
	if (bus_reset_imcomplete != FALSE)
	{
		HFC_DBGPRT("hfc_fx_eh_bus_reset : wait bus reset \n");
		
		do {
			find = FALSE;
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			spin_unlock_irq(cmnd->device->host->host_lock);
			
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ/10+1);					/* Wait 100ms */

			spin_lock_irq(cmnd->device->host->host_lock);
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

			for (lp = 0; lp<MAX_TARGET_PROBE; lp++)	{
				if ((target = hfc_fx_hash_target_info(pp, lp)) != NULL) {
					if ( test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)	
					 ||  test_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)
					 ||  test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status)
					 ||  test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status)
					 ||  test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status)
					 ||  test_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status) ) {	/* FCLNX-GPL-FX-014 */
						find = TRUE;
					}
				}
			}
		} while (find != FALSE);
	}
	
	clear_bit(HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2);
	HFC_DBGPRT("hfcldd : hfc_fx_eh_bus_reset - end\n");

	hfc_fx_stra_trace(HFC_FX_TRC_BUS_RESET ,0x10 ,pp , rp, NULL, NULL, NULL, (ulong)cmnd, 0, 0);
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	spin_unlock_irq(cmnd->device->host->host_lock);
	return(SUCCESS);
}


/* FCLNX-GPL-FX-014 Start */
int hfc_fx_issue_tgtrst_cscsi(
	struct port_info            *pp,
	struct target_info_fx       *target,
	struct dev_info_fx			*dev,
	uint						flags)
{
	int i = 0, j = 0, hash=0;
	struct core_info * core_wk = NULL;
	uint	issue_cscsi=0;
	struct	hfc_pkt_fx			*hfcp_wk=NULL;
	struct	port_info			*vpp;
	
	if (test_bit(CFLAG_TARGET_RESET, (ulong *)&flags)||test_bit(CFLAG_BUS_RESET, (ulong *)&flags)) {
		// Target Reset (wait stop DMA)
		hfc_fx_issue_task_mgm(pp, pp->region_arg[pp->rid]->core_arg[target->tgtrst_core_no], 
									target, NULL, dev, HFC_ISSUE_TARGET_RESET);
	}
	
	// Cancel SCSI
	if (flags & CFLAG_CSCSI_ANY)
	{ 
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if( hfc_fx_check_cs_disable(pp, core_wk) )
				continue; 
			if (((test_bit( HFC_TS_WAIT_TGTRSP, (ulong *)&target->status ))&&(i == target->tgtrst_core_no))	/* FCLNX-GPL-FX-112 */
			||  ((test_bit( HFC_PD_WAIT_BUSRSP, (ulong *)&pp->status_detail2 ))&&(i == target->tgtrst_core_no)))	/* FCLNX-GPL-FX-158 */
				continue;
			if (HFC_FX_MQ_VALID(pp)) {	/* FCLNX-GPL-FX-279, 286 */
				hfcp_wk = NULL;
				for (j=0; j<=pp->pport->max_vport_count; j++) {
					vpp = pp->pport->vport_ptr[j].vport_arg;
					if (vpp == NULL)
						continue;
					if (vpp->target_arg[target->pseq] == NULL)
						continue;
					
					for (hash=0;hash<HASH_T_NUM;hash++)
					{
						if (vpp->target_arg[target->pseq]->core_queue[core_wk->core_no].we_que_top[hash] != NULL)
						{ 	/* hfcp exists in queue */
							hfcp_wk = vpp->target_arg[target->pseq]->core_queue[core_wk->core_no].we_que_top[hash];
							break;
						}
					}
				}
				if( hfcp_wk == NULL )
					continue;
			}
			else {
				for (hash=0;hash<HASH_T_NUM;hash++)
				{
					hfcp_wk = NULL;	/* FCLNX-GPL-FX-112 */
					if (target->core_queue[core_wk->core_no].we_que_top[hash] != NULL)
					{ 	/* hfcp exists in queue */
						hfcp_wk = target->core_queue[core_wk->core_no].we_que_top[hash];
						break;
					}
				}
				if( hfcp_wk == NULL )
					continue; 
			}	/* FCLNX-GPL-FX-279, 286 */
			
			if (test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&flags)) {
				hfc_fx_issue_task_mgm(pp, core_wk, target, NULL, dev, HFC_ISSUE_CSCSI_TGT_WITHOUT_DMA);
				issue_cscsi=1;
			} else {
				hfc_fx_issue_task_mgm(pp, core_wk, target, NULL, dev, HFC_ISSUE_CSCSI_TGT_WAIT_DMA);
				issue_cscsi=1;
			}
		}
		if(!issue_cscsi){
			return(1);
		}
	}
	
	return 0;	
}


int hfc_fx_issue_devrst_cscsi(
	struct port_info            *pp,
	struct target_info_fx       *target,
	struct dev_info_fx          *dev,
	uint						flags)
{
	int i = 0, j = 0, hash=0;
	struct core_info * core_wk = NULL;
	struct hfc_pkt_fx * hfcp_wk = NULL;
	struct	port_info			*vpp;
	uint	issue_cscsi=0;
		
	if (test_bit(CFLAG_ABORT, (ulong *)&flags)) {
		// Abort Task Set (wait stop DMA)
		hfc_fx_issue_task_mgm(pp, pp->region_arg[pp->rid]->core_arg [dev->abtcmd_core_no], 
									target, NULL, dev, HFC_ISSUE_ABORT);
	}
	
	if (test_bit(CFLAG_LUN_RESET, (ulong *)&flags)) {
		// Abort Task Set (wait stop DMA)
		hfc_fx_issue_task_mgm(pp, pp->region_arg[pp->rid]->core_arg [dev->abtcmd_core_no], 
									target, NULL, dev, HFC_ISSUE_LUN_RESET);
	}
	
	// Cancel SCSI
	if (flags & CFLAG_CSCSI_ANY)
	{ 
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if( hfc_fx_check_cs_disable(pp, core_wk) )
				continue; 
			if (i == dev->abtcmd_core_no)
				continue;

			if (HFC_FX_MQ_VALID(pp)) {	/* FCLNX-GPL-FX-279, 286 */
				hfcp_wk = NULL;
				for (j=0; j<=pp->pport->max_vport_count; j++) {
					vpp = pp->pport->vport_ptr[j].vport_arg;
					if (vpp == NULL)
						continue;
					if (vpp->target_arg[target->pseq] == NULL)
						continue;
					
					for (hash=0;hash<HASH_T_NUM;hash++)
					{
						if (vpp->target_arg[target->pseq]->core_queue[core_wk->core_no].we_que_top[hash] != NULL)
						{ 	/* hfcp exists in queue */
							hfcp_wk = vpp->target_arg[target->pseq]->core_queue[core_wk->core_no].we_que_top[hash];
							while( hfcp_wk != NULL )
							{
								if(hfcp_wk->lun_id == dev->lun) break;
								hfcp_wk = hfcp_wk->cmd_forw ;
							}
						}
						if(hfcp_wk != NULL)break;
					}
				}
				if(hfcp_wk == NULL) continue;
			}
			else {
				for (hash=0;hash<HASH_T_NUM;hash++)
				{
					hfcp_wk = NULL;	/* FCLNX-GPL-FX-112 */
					if (target->core_queue[core_wk->core_no].we_que_top[hash] != NULL)
					{ 	/* hfcp exists in queue */
						hfcp_wk = target->core_queue[core_wk->core_no].we_que_top[hash];
						while( hfcp_wk != NULL )
						{
							if(hfcp_wk->lun_id == dev->lun) break;
							hfcp_wk = hfcp_wk->cmd_forw ;
						}
					}
					if(hfcp_wk != NULL)break;
				}
				if(hfcp_wk == NULL) continue;
			}	/* FCLNX-GPL-FX-279, 286 */
			
			if (test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&flags)) {
				hfc_fx_issue_task_mgm(pp, core_wk, target, NULL, dev, HFC_ISSUE_CSCSI_LU_WITHOUT_DMA);
				issue_cscsi=1;
			} else {
				hfc_fx_issue_task_mgm(pp, core_wk, target, NULL, dev, HFC_ISSUE_CSCSI_LU_WAIT_DMA);
				issue_cscsi=1;
			}
		}
		if(!issue_cscsi){
			return(1);
		}
	}

	return (0);	
}
/* FCLNX-GPL-FX-014 End */


/*
 * Function:    hfc_fx_issue_task_mgm
 *
 * Purpose:     Issue Task Management command
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
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
void hfc_fx_issue_task_mgm(
	struct port_info			*pp,
	struct core_info			*core,
	struct target_info_fx		*target,
	struct hfc_pkt_fx			*hfcp,
	struct dev_info_fx			*dev,
	uchar						mode)
{
	struct scsi_cmnd			*cmnd=NULL, *dummy_cmnd=NULL;
	struct hfc_pkt_fx			*reset_pkt=NULL;
	struct region_info			*rp=NULL;
	
	HFC_DBGPRT(" hfcldd : hfc_fx_issue_task_mgm - start\n");
	
	if( target==NULL){
		hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x01 ,pp ,pp->region_arg[pp->rid], core , target, hfcp, 0, 0, 0);
		return;
	}

	if (  !( test_bit( HFC_PS_ONLINE, (ulong *)&pp->status ) ) 
	 	|| test_bit( HFC_PS_ISOL, (ulong *)&pp->status) )	/* FCLNX-GPL-FX178 */
	{
		hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x22 ,pp ,pp->region_arg[pp->rid] ,core , target, hfcp, 0, 0, 0);
		return;
	}
	
	rp = pp->region_arg[pp->rid];
	
	if (mode == HFC_ISSUE_TARGET_RESET)
	{	/* Issue Target Reset */
		if( hfcp != NULL ){			/* FCLNX-GPL-328 */
			cmnd = hfcp->cmd_pkt;
			if(dev == NULL) dev = hfcp->dev;
		}else{
			cmnd = target->target_reset_cmnd;	/* FCLNX-GPL-FX-014 */
			if(dev == NULL) dev = (struct dev_info_fx *)CMND_DEV(cmnd);
		}
		
		dummy_cmnd = target->dummy_cmnd[core->core_no];
		reset_pkt = target->reset_pkt + core->core_no;
		
		if(( reset_pkt == NULL)||( dummy_cmnd == NULL)){
			clear_bit( HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );	/* FCLNX-GPL-328 */
			clear_bit( HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat );			/* FCLNX-GPL-328 */
			clear_bit( HFC_DS_NEED_ABORT, (ulong *)&dev->lustat );				/* FCLNX-GPL-328 */
			set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
			hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x27 ,pp ,pp->region_arg[pp->rid] ,core , target, hfcp, 0, 0, 0);
			return;
		}
		
		memset(reset_pkt, 0, sizeof(struct hfc_pkt_fx));
		set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
		set_bit(CFLAG_TARGET_RESET, (ulong *)&reset_pkt->cmd_flags);
		if(test_bit(HFC_DF_HSDLDD_VALID, (ulong *)&dev->flags)){ /* FCLNX-GPL-FX-261 */
			set_bit(CFLAG_HSDLDD_VALID, (ulong *)&reset_pkt->cmd_flags );
		} /* FCLNX-GPL-FX-261 */
		core->pkt_cnt++;
		
		reset_pkt->cmd_pkt   = dummy_cmnd;
		reset_pkt->target_id = target->target_id;
		reset_pkt->lun_id    = dev->lun;
		reset_pkt->pp        = pp;
		reset_pkt->target    = target;
		reset_pkt->dev       = dev;
		reset_pkt->rp        = rp;
		reset_pkt->rid		 = rp->rid;
		reset_pkt->core      = core;
		dummy_cmnd->host_scribble = (void *)reset_pkt;

		if(cmnd != NULL){
			hfc_fx_dummy_copy(pp, cmnd, dummy_cmnd );
		}
		
		hfc_fx_enqueue_wx_que(core, reset_pkt);
		
		/* Initiate Target Reset *//* FCLNX-GPL-FX-178 Start */
		if ( !(pp->status & HFC_PS_BLOCKED_SCSI)
		  && !( test_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2))
		  && !( test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status))
		  && !( test_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status)))
		{
			hfc_fx_start( pp, rp, core, target);
		}else{
			if (target->core_queue[core->core_no].next_dstart_flag == 0) {
				hfc_fx_enque_next_dstart(pp, rp, core, target);
			}
		}	/* FCLNX-GPL-FX-178 End */
		
		hfc_fx_stra_trace(
			HFC_FX_TRC_ISSUE_TMGM, 0x11, pp, pp->region_arg[pp->rid], core, target, reset_pkt,
			 0, 0, 0);
	}
	else if (mode == HFC_ISSUE_LUN_RESET){
		/* Make LUN Reset Cmnd and Paket. */
		if( hfcp != NULL ){			/* FCLNX-GPL-328 */
			cmnd = hfcp->cmd_pkt;
			if(dev == NULL) dev = hfcp->dev;
		}else{
			if(dev == NULL) dev = (struct dev_info_fx *)CMND_DEV(cmnd);
			cmnd = dev->lun_reset_cmnd;	/* FCLNX-GPL-FX-014 */
		}
		
		if (dev->dummy_cmnd[core->core_no] == NULL) {
			dev->dummy_cmnd[core->core_no] = hfc_fx_get_new_cmnd(pp);
		}
		dummy_cmnd = dev->dummy_cmnd[core->core_no];
		reset_pkt = hfc_fx_get_new_hfcp(pp);
		
		if(( reset_pkt == NULL)||( dummy_cmnd == NULL)){
			clear_bit( HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );	/* FCLNX-GPL-328 */
			clear_bit( HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat );			/* FCLNX-GPL-328 */
			clear_bit( HFC_DS_NEED_ABORT, (ulong *)&dev->lustat );				/* FCLNX-GPL-328 */
			set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
			hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x28 ,pp ,pp->region_arg[pp->rid] ,core , target, hfcp, 0, 0, 0);
			return;
		}

		set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
		set_bit(CFLAG_LUN_RESET, (ulong *)&reset_pkt->cmd_flags);
		if(test_bit(HFC_DF_HSDLDD_VALID, (ulong *)&dev->flags)){  /* FCLNX-GPL-FX-261 */
			set_bit(CFLAG_HSDLDD_VALID, (ulong *)&reset_pkt->cmd_flags );
		}  /* FCLNX-GPL-FX-261 */
		core->pkt_cnt++;
		
		reset_pkt->cmd_pkt   = dummy_cmnd;
		reset_pkt->target_id = target->target_id;
		reset_pkt->lun_id    = dev->lun;
		reset_pkt->pp        = pp;
		reset_pkt->target    = target;
		reset_pkt->dev       = dev;
		reset_pkt->rp        = rp;
		reset_pkt->core      = core;
		reset_pkt->rid		 = rp->rid;
		dummy_cmnd->host_scribble = (void *)reset_pkt;
		
		if(cmnd != NULL){
			hfc_fx_dummy_copy(pp, cmnd, dummy_cmnd );
		}
		
		hfc_fx_enqueue_wx_que(core, reset_pkt);
		
		/* Initiate Lun Reset *//* FCLNX-GPL-FX-178 Start */
		if ( !(pp->status & HFC_PS_BLOCKED_SCSI)
		  && !(pp->status_detail2 & HFC_PD2_BLOCKED_SCSI)
		  && !(target->status & HFC_TS_BLOCKED_SCSI) )
		{
			hfc_fx_start( pp, rp, core, target);
		}else{
			if (target->core_queue[core->core_no].next_dstart_flag == 0) {
				hfc_fx_enque_next_dstart(pp, rp, core, target);
			}
		}	/* FCLNX-GPL-FX-178 End */
		hfc_fx_stra_trace(
			HFC_FX_TRC_ISSUE_TMGM, 0x12, pp, pp->region_arg[pp->rid], core, target, reset_pkt,
			 0, 0, 0);
	}
	else if(mode == HFC_ISSUE_ABORT)
	{	/* Issue Abort Task Set issue */
		if( hfcp != NULL ){			/* FCLNX-GPL-328 */
			cmnd = hfcp->cmd_pkt;
			if(dev == NULL) dev = hfcp->dev;
		}else{
			if(dev == NULL) dev = (struct dev_info_fx *)CMND_DEV(cmnd);
			cmnd = dev->abort_cmnd;	/* FCLNX-GPL-FX-014 */
		}
		
		if (dev->dummy_cmnd[core->core_no] == NULL) {
			dev->dummy_cmnd[core->core_no] = hfc_fx_get_new_cmnd(pp);
		}
		dummy_cmnd = dev->dummy_cmnd[core->core_no];
		reset_pkt = hfc_fx_get_new_hfcp(pp);
		
		if(( reset_pkt == NULL)||( dummy_cmnd == NULL)){
			clear_bit( HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );	/* FCLNX-GPL-328 */
			clear_bit( HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat );			/* FCLNX-GPL-328 */
			clear_bit( HFC_DS_NEED_ABORT, (ulong *)&dev->lustat );				/* FCLNX-GPL-328 */
			set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
			hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
			hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x29 ,pp ,pp->region_arg[pp->rid] ,core , target, hfcp, 0, 0, 0);
			return;
		}

		set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
		set_bit(CFLAG_ABORT, (ulong *)&reset_pkt->cmd_flags);
		if(test_bit(HFC_DF_HSDLDD_VALID, (ulong *)&dev->flags) && test_bit(HFC_DS_WAIT_ABTSRSP, (ulong *)&dev->lustat)){	/* FCLNX-GPL-FX-261 */
			set_bit(CFLAG_HSDLDD_VALID, (ulong *)&reset_pkt->cmd_flags );
		}	/* FCLNX-GPL-FX-261 */
		core->pkt_cnt++;

		reset_pkt->cmd_pkt   = dummy_cmnd;
		reset_pkt->target_id = target->target_id;
		reset_pkt->lun_id    = dev->lun;	/* FCLNX-GPL-FX-112 */
		reset_pkt->pp        = pp;
		reset_pkt->target    = target;
		reset_pkt->dev       = dev;
		reset_pkt->rp        = rp;
		reset_pkt->core      = core;
		reset_pkt->rid		 = rp->rid;
		dummy_cmnd->host_scribble = (void *)reset_pkt;

		if(cmnd != NULL){	/* FCLNX-GPL-FX-251,272 */
			hfc_fx_dummy_copy(pp, cmnd, dummy_cmnd );
		}					/* FCLNX-GPL-FX-251,272 */
		
		hfc_fx_enqueue_wx_que(core, reset_pkt);

		if ( !(pp->status & HFC_PS_BLOCKED_SCSI)	/* FCLNX-GPL-FX-178 Start */
		  && !(pp->status_detail2 & HFC_PD2_BLOCKED_SCSI)
		  && !(target->status & HFC_TS_BLOCKED_SCSI) )
		{
			hfc_fx_start( pp, rp, core, target);
		}else{
			if (target->core_queue[core->core_no].next_dstart_flag == 0) {
				hfc_fx_enque_next_dstart(pp, rp, core, target);
			}
		}	/* FCLNX-GPL-FX-178 End */
		hfc_fx_stra_trace(
			HFC_FX_TRC_ISSUE_TMGM, 0x13, pp, pp->region_arg[pp->rid], core, target, reset_pkt,
			 0, 0, 0);
	}
	
	if( (mode == HFC_ISSUE_CSCSI_TGT_WAIT_DMA)
	 || (mode == HFC_ISSUE_CSCSI_TGT_WITHOUT_DMA)
	 || (mode == HFC_ISSUE_CSCSI_LU_WAIT_DMA)
	 || (mode == HFC_ISSUE_CSCSI_LU_WITHOUT_DMA) ){	/* FCLNX-GPL-FX-014 Start */
		/* Issue C_XOB  */
		
		if ((mode == HFC_ISSUE_CSCSI_TGT_WAIT_DMA) || (mode == HFC_ISSUE_CSCSI_TGT_WITHOUT_DMA)){
			if( hfcp != NULL ){			/* FCLNX-GPL-328 */
				cmnd = hfcp->cmd_pkt;
				if(dev == NULL) dev = hfcp->dev;
			}else{
				cmnd = target->target_reset_cmnd;
				if(dev == NULL) dev = (struct dev_info_fx *)CMND_DEV(cmnd);
			}
			
			dummy_cmnd = target->dummy_cmnd[core->core_no];
			if (mode == HFC_ISSUE_CSCSI_TGT_WITHOUT_DMA) {
				reset_pkt = target->reset_pkt + MAX_CORE_PROBE_FX*1 + core->core_no;
			}
			else {
				reset_pkt = target->reset_pkt + MAX_CORE_PROBE_FX*2 + core->core_no;
			}
			
			if(( reset_pkt == NULL)||( dummy_cmnd == NULL)){
				clear_bit( HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );	/* FCLNX-GPL-328 */
				clear_bit( HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat );			/* FCLNX-GPL-328 */
				clear_bit( HFC_DS_NEED_ABORT, (ulong *)&dev->lustat );				/* FCLNX-GPL-328 */
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
				hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x30 ,pp ,pp->region_arg[pp->rid] ,core , target, hfcp, 0, 0, 0);
				return;
			}
			
			memset(reset_pkt, 0, sizeof(struct hfc_pkt_fx));
			
			if (mode == HFC_ISSUE_CSCSI_TGT_WAIT_DMA){
				/* Issue CANCEL SCSI for Target waiting DMA */
				set_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&reset_pkt->cmd_flags);
				set_bit(HFC_TC_NEED_CSCSI_TGT_WAIT_DMA, (ulong *)&target->tgt_core_stat.core[core->core_no]);
			}else{
				set_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&reset_pkt->cmd_flags);
				set_bit(HFC_TC_NEED_CSCSI_TGT_WITHOUT_DMA, (ulong *)&target->tgt_core_stat.core[core->core_no]);
			}
			if(test_bit(HFC_DF_HSDLDD_VALID, (ulong *)&dev->flags)){  /* FCLNX-GPL-FX-261 */
				set_bit(CFLAG_HSDLDD_VALID, (ulong *)&reset_pkt->cmd_flags );
			}  /* FCLNX-GPL-FX-261 */
		}
		if ((mode == HFC_ISSUE_CSCSI_LU_WAIT_DMA) || (mode == HFC_ISSUE_CSCSI_LU_WITHOUT_DMA)){
			if( hfcp != NULL ){			/* FCLNX-GPL-328 */
				cmnd = hfcp->cmd_pkt;
				if(dev == NULL) dev = hfcp->dev;
			}else{
				if(dev != NULL){
					if(test_bit(HFC_DS_WAIT_LUNRSP, (ulong *)&dev->lustat)){
						cmnd = dev->lun_reset_cmnd;	/* FCLNX-GPL-FX-014 */
					}else{
						cmnd = dev->abort_cmnd;
					}
				}
			}
			
			if (dev != NULL){
				if(dev->dummy_cmnd[core->core_no] == NULL) {
					dev->dummy_cmnd[core->core_no] = hfc_fx_get_new_cmnd(pp);
				}
				dummy_cmnd = dev->dummy_cmnd[core->core_no];
				reset_pkt = hfc_fx_get_new_hfcp(pp);
			}
			
			if(( reset_pkt == NULL)||( dummy_cmnd == NULL)){
				clear_bit( HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );	/* FCLNX-GPL-328 */
				clear_bit( HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat );			/* FCLNX-GPL-328 */
				clear_bit( HFC_DS_NEED_ABORT, (ulong *)&dev->lustat );				/* FCLNX-GPL-328 */
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
				hfc_fx_abend(pp, core, HFC_ABEND_LINK_RESET);
				hfc_fx_stra_trace(HFC_FX_TRC_ISSUE_TMGM ,0x31 ,pp ,pp->region_arg[pp->rid] ,core , target, hfcp, 0, 0, 0);
				return;
			}
			
			if(mode == HFC_ISSUE_CSCSI_LU_WAIT_DMA){
				/* Issue CANCEL SCSI for Target */
				set_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&reset_pkt->cmd_flags);
				set_bit(HFC_DC_NEED_CSCSI_LU_WAIT_DMA, (ulong *)&dev->dev_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-255,272 */
			}else{
				/* Issue CANCEL SCSI for Target */
				set_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&reset_pkt->cmd_flags);
				set_bit(HFC_DC_NEED_CSCSI_LU_WITHOUT_DMA, (ulong *)&dev->dev_core_stat.core[core->core_no]);	/* FCLNX-GPL-FX-255,272 */
			}
			if(test_bit(HFC_DF_HSDLDD_VALID, (ulong *)&dev->flags) && 
			(test_bit(HFC_DS_WAIT_ABTSRSP, (ulong *)&dev->lustat)||test_bit(HFC_DS_WAIT_LUNRSP, (ulong *)&dev->lustat))){	/* FCLNX-GPL-FX-261 */
				set_bit(CFLAG_HSDLDD_VALID, (ulong *)&reset_pkt->cmd_flags );
			}	/* FCLNX-GPL-FX-261 */
		}
		
		set_bit(CFLAG_VALID, (ulong *)&reset_pkt->cmd_flags);
		core->pkt_cnt++;
		
		reset_pkt->cmd_pkt   = dummy_cmnd;
		reset_pkt->target_id = target->target_id;
		reset_pkt->lun_id    = dev->lun;
		reset_pkt->pp        = pp;
		reset_pkt->target    = target;
		reset_pkt->dev       = dev;
		reset_pkt->rp        = rp;
		reset_pkt->rid		 = rp->rid;
		reset_pkt->core      = core;
		dummy_cmnd->host_scribble = (void *)reset_pkt;

		if(cmnd != NULL){
			hfc_fx_dummy_copy(pp, cmnd, dummy_cmnd );
		}
		
		hfc_fx_enqueue_wx_que(core, reset_pkt);
		
		
		if((mode == HFC_ISSUE_CSCSI_TGT_WAIT_DMA)
		||(mode == HFC_ISSUE_CSCSI_TGT_WITHOUT_DMA)){	/* FCLNX-GPL-FX-178 Start */
			/* Initiate Target Reset */
			if ( !(pp->status & HFC_PS_BLOCKED_SCSI)
			  && !( test_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2))
			  && !( test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status))
			  && !( test_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status)))
			{
				hfc_fx_start( pp, rp, core, target);
			}else{
				if (target->core_queue[core->core_no].next_dstart_flag == 0) {
					hfc_fx_enque_next_dstart(pp, rp, core, target);
				}
			}
			hfc_fx_stra_trace(
				HFC_FX_TRC_ISSUE_TMGM, 0x14, pp, pp->region_arg[pp->rid], core, target, reset_pkt,
				 0, 0, 0);
		}else{
			/* Initiate LUN Reset or Abort Task Set */
			if ( !(pp->status & HFC_PS_BLOCKED_SCSI)
			  && !(pp->status_detail2 & HFC_PD2_BLOCKED_SCSI)
			  && !(target->status & HFC_TS_BLOCKED_SCSI) )
			{
				hfc_fx_start( pp, rp, core, target);
			}else{
				if (target->core_queue[core->core_no].next_dstart_flag == 0) {
					hfc_fx_enque_next_dstart(pp, rp, core, target);
				}
			}
			hfc_fx_stra_trace(
				HFC_FX_TRC_ISSUE_TMGM, 0x15, pp, pp->region_arg[pp->rid], core, target, reset_pkt,
				 0, 0, 0);
		}	/* FCLNX-GPL-FX-178 End */
	}	/* FCLNX-GPL-FX-014 End */

	hfc_fx_stra_trace(
			HFC_FX_TRC_ISSUE_TMGM, 0x10, pp,pp->region_arg[pp->rid], core, target, hfcp, (ulong)cmnd, 0, 0);
	
	return;
}


/*
 * Function:    hfc_fx_cancel_scsi_cmd
 *
 * Purpose:     Cancel Scsi_cmds which have already stored in xob, wait_end_que, wait_xob_que.
 *
 * Arguments:   
 *  pp            - Pointer to port_info 
 *  target        - Pointer to target_info_fx 
 *  lun           - lun number
 *  hfcp          - Pointer to hfc_pkt_fx 
 *  adap_status   - Cause of cancellation
 *  we_que_cancel - Flag to specify cancellation of wait_end_que
 *  type          - Targets of Cancellation 
 *                    = HFC_FLASH_PACKET	  : pp,hfcp
 *                    = HFC_FLASH_DEV		  : pp,target,lun
 *                    = HFC_FLASH_TARGET	  : pp,target
 *
 * Returns:     
 *
 * context : user / kernel / interrupt
 *
 * Notes:			Lock port_info before calling this function
 */
void hfc_fx_cancel_scsi_cmd(struct port_info	*pp,
							struct core_info	*core,
							struct target_info_fx	*target,
							uint				lun,
							struct hfc_pkt_fx	*hfcp,
							uint				adap_status,
							uchar				inh_altpath,
							uchar				we_que_cancel,
							uchar				tm_que_cancel,
							uchar				type)
{
	HFC_DBGPRT(" hfcldd : hfc_fx_cancel_scsi_cmd - start\n");
	
	hfc_fx_cancel_xob(pp, core, target,lun,hfcp,type) ;
	
	if( we_que_cancel == TRUE )
		hfc_fx_cancel_weque(pp,core,target,lun,hfcp,adap_status,inh_altpath,type) ;
		
	if( tm_que_cancel == TRUE )
		hfc_fx_cancel_tskmgm(pp, target, lun, hfcp, adap_status, type);
	
	hfc_fx_cancel_wxque(pp,core,target,lun,hfcp,adap_status,inh_altpath,type) ;

	return;
}


#define SC_TARGET_RESET 0x20	 	/* SCSI Device Head wants the adapter 	*/
									/* driver to issue a Target Reset task	*/
									/* request to this device				*/
/*
 * Function:    hfc_fx_cancel_xob
 *
 * Purpose:     Turn on the skip flag of xob with range from otp to inp - 1.
 *              to inp - 1.
 *              No specific response to caller in this function -- Commands 
 *              stored in xob are also stored in wait_end_queue, so cancellation 
 *              of wait_end_que will report the completion of process to caller 
 *              afterward.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt_fx
 *  type       - Targets of Cancellation
 *                    = HFC_FLASH_PACKET  : pp,hfcp
 *                    = HFC_FLASH_DEV     : pp,target,lun
 *                    = HFC_FLASH_TARGET  : pp,target
 * Returns:     
 *  0          - No xob exists for cancellation.
 *  1          - Canceled one or more xobs.
 *
 * Notes:			Lock port_info before calling this function
 */
int hfc_fx_cancel_xob( struct port_info	*pp,
					struct core_info	*core,
					struct target_info_fx	*target,
					uint				lun,
					struct hfc_pkt_fx	*hfcp,
					uchar				type)
{
	uint	outp_num=0, inp_num=0, i=0, tmp_outp=0;
	int		cancel = 0, valid_cnt=0;
	uchar	wk_s_id[3];
	struct	xob_fx 	*xob_ptr;

	HFC_DBGPRT(" hfcldd : hfc_fx_cancel_xob - start\n");

#if _HFC_DEBUG_STRA_00
	hfc_fx_stra_trace(HFC_FX_TRC_CAN_XOB,0x00,
					  pp,target,hfcp,NULL,type,lun,NULL) ;
#endif

	if( core->drv_next_xob == 0 ){
		outp_num = pp->xob_max-1;
	}
	else {
		outp_num = core->drv_next_xob - 1;
	}
	tmp_outp = outp_num;
	
	for(i=0; i < pp->xob_max; i++)
	{
		if( core->xob[outp_num].flag & HFC_XOB_VALID ){
			tmp_outp = outp_num;
			if( outp_num == 0 ){
				/* Wrap around case */
				outp_num = pp->xob_max-1;
			}
			else{
				outp_num--;
			}
			valid_cnt++;
		}
		else{
			outp_num = tmp_outp;
			break;
		}
	}
	
	inp_num = core->drv_next_xob ;
	
	HFC_DBGPRT(" hfcldd : hfc_fx_cancel_xob - outp_num =%02x inp_num(xob_no) = %02x we_que_cnt = %d valid_cnt = %d\n", 
		outp_num, inp_num, core->we_que_cnt_all, valid_cnt);
	
	if( valid_cnt == 0 )
	{	/* Xob is empty */
		HFC_DBGPRT(" hfcldd : hfc_fx_cancel_xob - outp_num == inp_num\n");
		hfc_fx_stra_trace(
			HFC_FX_TRC_CAN_XOB ,0x20 ,pp ,NULL ,NULL ,target ,hfcp ,
			(unsigned long long)type,(unsigned long long)lun, 0) ;
		return (0);
	}

	for(i=0;i<3;i++) wk_s_id[i] =
		(uchar)((pp->scsi_id     & 0x0000000000ffffff) >> ((2-i)*8)); /* Trans S_ID */
	
	do {
		HFC_DBGPRT(" hfcldd : hfc_fx_cancel_xob - xob#%02x skip off\n",outp_num);
		xob_ptr = &core->xob[outp_num];

		if ( type == HFC_FLASH_DEV )
		{
			/* Cancel each lun */
			if(( ((struct hfc_pkt_fx *)(ulong)xob_ptr->drv_work)->target_id ==
				 ((uint)(target->target_id & 0xffffffff)) )	/* FCLNX-GPL-FX-014 */
			   &&( ((struct hfc_pkt_fx *)(ulong)xob_ptr->drv_work)->lun_id  == lun ))	/* FCLNX-GPL-FX-014 */
			{
				if (memcmp(&wk_s_id[0], &xob_ptr->trans_s_id[0], 3) == 0)
				{
					xob_ptr->skip |= HFC_XOB_SKIP ;
					cancel = 1;
				}
			}
		}
		else if( type == HFC_FLASH_TARGET )
		{
			/* Cancel target */
			if( ((struct hfc_pkt_fx *)(ulong)xob_ptr->drv_work)->target_id ==
				((uint)(target->target_id & 0xffffffff)))	/* FCLNX-GPL-FX-014 */
			{
				if (memcmp(&wk_s_id[0], &xob_ptr->trans_s_id[0], 3) == 0)
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
		if( outp_num >= pp->xob_max )
			outp_num = 0 ;

	} while( outp_num != inp_num ) ;

#if _HFC_DEBUG_STRA_01			
		hfc_fx_stra_trace(HFC_FX_TRC_CAN_XOB,0x10,
			pp,hfcp->target,hfcp,NULL,type,lun,NULL) ;
#endif
	return (cancel);
}


/* FCLNX-GPL-FX-228,272 Start */
/*
 * Function:     hfc_fx_cancel_xrb
 *
 * Purpose:      Set skip bit to the XRB entries of the device
 *
 * Arguments:
 *   pp          - Pointer to port_info
 *   target      - Pointer to target_info_fx
 *   type        - Type of cancellation
 *                 = HFC_FLASH_TARGET
 *                 = HFC_FLASH_ADAP
 *
 * Returns:      None
 *
 */
void hfc_fx_cancel_xrb(
	struct port_info		*pp,
	struct target_info_fx	*target,
	uchar					type)
{
	struct core_info	*core	  = NULL;
	struct xrb_fx		*xrb	  = NULL;
	struct hfc_pkt_fx	*hfcp = NULL;

	uint	xrb_no	  = 0, i=0;
	uchar	wk_s_id[3];	/* FCLNX-GPL-FX-285 */

	HFC_ENTRY("hfc_fx_cancel_xrb");

	/* Validate arguement */
	if (pp == NULL) {
		return;
	}

	if (type == HFC_FLASH_TARGET) {
		if (target == NULL) {
			return;
		}
	}
	else if (type == HFC_FLASH_ADAP) {
		/* NOP */
	}
	else {
		return;	/* Invalid type */
	}

	for(i=0;i<3;i++) wk_s_id[i] =
		(uchar)((pp->scsi_id & 0x0000000000ffffff) >> ((2-i)*8)); /* Trans S_ID */ /* FCLNX-GPL-FX-274, 285 */

	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		if( hfc_fx_check_cs_disable(pp, core) )
			continue; 

		HFC_DBGPRT("hfcldd%d : hfc_fx_cancel_xrb - core_no=%d\n",pp->dev_minor, core->core_no);
		
		xrb	   = (struct xrb_fx *)core->xrb;
		xrb_no = core->drv_next_xrb;

		/* Check xrbs while xrb valid bit is enabled */
		while (1) {
			if (!(xrb[xrb_no].xcrbchk.valid & HFC_XRB_VALID)) {
				break;
			}

			if (type == HFC_FLASH_ADAP) { /* skip XRB for all target */
				if (memcmp(&wk_s_id[0], &xrb[xrb_no].xcrb.trans_s_id[0], 3) == 0) {	/* FCLNX-GPL-FX-274, 285 */
					HFC_DBGPRT("hfcldd%d : hfc_fx_cancel_xrb - xrb_no=%02x\n",pp->dev_minor, xrb_no);
					xrb[xrb_no].xcrb.skip |= HFC_XRB_SKIP;
				}
				else {
					HFC_DBGPRT("hfcldd%d : hfc_fx_cancel_xrb - initiator unmatch xrb_no=%02x\n",pp->dev_minor, xrb_no);
				}
			}
			else { /* skip XRB for one target */
				hfcp = (struct hfc_pkt_fx *)(ulong)xrb[xrb_no].xcrb.hfc_pkt;
				if ((hfcp != NULL) && (hfcp->rid == pp->rid) && (HFC_HFCP_FX_CFLAG_TEST(CFLAG_VALID ,hfcp))){
					if (hfcp->target_id == target->target_id) {
						if (memcmp(&wk_s_id[0], &xrb[xrb_no].xcrb.trans_s_id[0], 3) == 0) {	/* FCLNX-GPL-FX-274, 285 */
							xrb[xrb_no].xcrb.skip |= HFC_XRB_SKIP;
							HFC_DBGPRT("hfcldd%d : hfc_fx_cancel_xrb - xrb_no=%02x target_id\n",pp->dev_minor, xrb_no);
						}
						else {
							HFC_DBGPRT("hfcldd%d : hfc_fx_cancel_xrb - initiator unmatch xrb_no=%02x target_id\n",pp->dev_minor, xrb_no);
						}
					}
				}
			}

			xrb_no++;
			if (xrb_no >= pp->xrb_max) {
				xrb_no = 0;
			}
		}
	}

	return;

}
/* FCLNX-GPL-FX-228,272 End */


/*
 * Function:    hfc_fx_cancel_weque
 *
 * Purpose:     Dequeue hfc_pkt_fx in we_que
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt_fx
 *  inh_altpath- 
 *  adap_status- Cause of cancellation
 *  pp         - Adpp_info structure pointer
 *  type       - Range of cancellation
 *               type = HFC_FLASH_PACKET  : pp,hfcp
 *                    = HFC_FLASH_DEV     : pp,target,lun
 *                    = HFC_FLASH_TARGET  : pp,target
 * Notes: Lock port_info before calling this function
 */
void hfc_fx_cancel_weque( struct port_info		*pp,
						  struct core_info		*core,
						  struct target_info_fx	*target,
						  uint					lun,
						  struct hfc_pkt_fx		*hfcp,
						  uint					adap_status,
						  uchar					inh_altpath,
						  uchar					type )
{
	int hash, result=0;
	struct hfc_pkt_fx	*wk_hfcp, *next_hfcp;
	struct dev_info_fx	*dev=NULL;	/* FCLNX-GPL-0343 */

#if _HFC_DEBUG_STRA_02	
		hfc_fx_stra_trace(HFC_FX_TRC_CAN_WE,0x00,
			pp,target,hfcp,NULL,type,lun,adap_status);
#endif

	HFC_DBGPRT(" hfcldd : hfc_fx_cancel_weque - start\n");
	HFC_DBGPRT(" hfcldd : hfc_fx_cancel_weque - we_que_cnt_all=%d\n", core->we_que_cnt_all);

	for (hash=0;hash<HASH_T_NUM;hash++)
	{
		wk_hfcp = target->core_queue[core->core_no].we_que_top[hash];

		while ( wk_hfcp != NULL )
		{
			next_hfcp = wk_hfcp->cmd_forw;		/* Preserve next hfc_pkt_fx */

			result = 0;
			
			if ( (type == HFC_FLASH_TARGET) 	/* Cancel each target	*/
			  || ((type == HFC_FLASH_DEV) 		/* Cancel each lun	*/
					&& (wk_hfcp->cmd_pkt != NULL) && (wk_hfcp->lun_id == lun) ))
			{
				/* Dequeue from we_que */
				if ((target->core_queue[core->core_no].we_que_top[hash] == wk_hfcp)
				 && (target->core_queue[core->core_no].we_que_end[hash] == wk_hfcp) )	/* Only one	*/
				{
					target->core_queue[core->core_no].we_que_top[hash] = NULL;
					target->core_queue[core->core_no].we_que_end[hash] = NULL;
				}
				else
				{
					if ( wk_hfcp == target->core_queue[core->core_no].we_que_top[hash] )			/* queue top */
					{
						target->core_queue[core->core_no].we_que_top[hash] = wk_hfcp->cmd_forw;
						target->core_queue[core->core_no].we_que_top[hash]->cmd_prev = NULL;
					}
					else if ( wk_hfcp == target->core_queue[core->core_no].we_que_end[hash])		/* queue end */
					{
						target->core_queue[core->core_no].we_que_end[hash] = wk_hfcp->cmd_prev;
						target->core_queue[core->core_no].we_que_end[hash]->cmd_forw = NULL;
					}
					else												/* Otherwise */
					{
						wk_hfcp->cmd_forw->cmd_prev = wk_hfcp->cmd_prev ;
						wk_hfcp->cmd_prev->cmd_forw = wk_hfcp->cmd_forw ;
					}
				}
				
				core->we_que_sizecnt -= wk_hfcp->data_size;
				core->we_que_cnt_all-- ;
				target->core_queue[core->core_no].we_que_cnt-- ;
				if( wk_hfcp->cmd_flags & CFLAG_RESET_ANY){	/* FCLNX-GPL-FX-172 */
					if (core->tskmgm_cmd_num != 0) {
						core->tskmgm_cmd_num--;
					}
				}	/* FCLNX-GPL-FX-172 */

				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() && wk_hfcp->dev){
					hfc_manage_info.npubp->hfc_fx_queue_count(wk_hfcp, 1, 1);	/* we dequeue */
					if((wk_hfcp->cmd_pkt != NULL) 
					&& (((uchar)wk_hfcp->cmd_pkt->cmnd[0] == 0x5E) || ((uchar)wk_hfcp->cmd_pkt->cmnd[0] == 0x5F))){
						HFC_DBGPRT("hfc_fx_cancel_weque : core->ppreserve_cmd_num = %d\n",core->preserve_cmd_num);
						core->preserve_cmd_num--;
					}
				}

				wk_hfcp->cmd_forw = NULL;
				wk_hfcp->cmd_prev = NULL;
				dev = wk_hfcp->dev;									/* FCLNX-GPL-0343 */

				if ((( test_bit(CFLAG_ABORT,(ulong *)&wk_hfcp->cmd_flags) )
				|| test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&wk_hfcp->cmd_flags))	/* FCLNX-GPL-FX-112 */
				&& (( dev != NULL )&&(test_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat)))){	/* FCLNX-GPL-FX-178 */
					/* Turn off HFC_DS_WAIT_ABORT when cancel Abort Task Set request */
					clear_bit(HFC_DS_WAIT_ABORT, (ulong *)&dev->lustat);
				}
				else if (((test_bit(CFLAG_LUN_RESET,(ulong *)&wk_hfcp->cmd_flags) )
				|| test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&wk_hfcp->cmd_flags))	/* FCLNX-GPL-FX-112 */
				&& (( dev != NULL )&&(test_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat)))){	/* FCLNX-GPL-FX-178 */
					/* Turn off HFC_WAIT_LUN_RESET when cancel LUN RESET request */
					clear_bit(HFC_DS_WAIT_LUN_RESET, (ulong *)&dev->lustat);
				}
				else if ((( test_bit(CFLAG_TARGET_RESET, (ulong *)&wk_hfcp->cmd_flags) )
				|| test_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&wk_hfcp->cmd_flags))	/* FCLNX-GPL-FX-112 */
				&& (test_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status ))){	/* FCLNX-GPL-FX-178 */
					/* Turn off HFC_TS_WAIT_TARGET_RESET when cancel Target Reset request */
					clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status );
				}
				else if (( test_bit(CFLAG_BUS_RESET, (ulong *)&wk_hfcp->cmd_flags) )
				&& (test_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status ))){	/* FCLNX-GPL-FX-178 */
					/* Turn off HFC_TS_WAIT_TARGET_RESET when cancel Target Reset request */
					clear_bit(HFC_TS_WAIT_TARGET_RESET, (ulong *)&target->status );
				}
				else if((test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&wk_hfcp->cmd_flags))
				&&(( dev != NULL )&&(test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat)))){	/* FCLNX-GPL-FX-178 Start */
					/* Turn off HFC_DS_NEED_ABORT when cancel Abort Task Set request */
					clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
				}
				else if((test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&wk_hfcp->cmd_flags))
				&&(( dev != NULL )&&(test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat)))){
					/* Turn off HFC_DS_NEED_LUN_RESET when cancel LUN RESET request */
					clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
				}
				else if((test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&wk_hfcp->cmd_flags))
				&&(test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status ))){
					/* Turn off HFC_DS_NEED_ABORT when cancel Target Reset request */
					clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );
				}	/* FCLNX-GPL-FX-178 End */

				/* Store cancellation factor and scsi response */
				wk_hfcp->adap_status = adap_status ;

				/* Set reason code and initialize statistics *//* FCLNX-GPL-571 */
				if( test_bit( CFLAG_TIMEOUT, (ulong *)&wk_hfcp->cmd_flags ) && !( wk_hfcp->cmd_flags & CFLAG_RESET_ANY )) /* FCLNX-GPL-FX-091 */
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
					if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
						result = DID_ERROR;
					}
					else if( test_bit( HFC_PS_ISOL, (ulong *)&pp->status) )
					{
						result = DID_NO_CONNECT;
					}
					else if( (target != NULL) && test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status) )
					{	/*** RSCN -> Target lost case ***/
						result = DID_ERROR;
					}
					else if( (target != NULL) && test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status))/********************/
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
				
				HFC_DBGPRT(" hfcldd : hfc_fx_cancel_weque - rid=%d\n", pp->rid);
				hfc_fx_set_cmnd_res(pp, core, wk_hfcp->cmd_pkt, wk_hfcp, result);
				hfc_fx_iodone(pp, core, wk_hfcp->cmd_pkt, wk_hfcp);	/* Callback */
			}

			wk_hfcp = next_hfcp;				/* Move to the next hfc_pkt_fx */
		}
	}

	return;
}


/*
 * Function:    hfc_fx_cancel_wxque
 *
 * Purpose:     Dequeue hfc_pkt_fx in wx_que
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt_fx
 *  inh_altpath- 
 *  adap_status- Cause of cancellation
 *  pp         - Adpp_info structure pointer
 *  type       - Range of cancellation
 *               type = HFC_FLASH_PACKET  : pp,hfcp
 *                    = HFC_FLASH_DEV     : pp,target,lun
 *                    = HFC_FLASH_TARGET  : pp,target
 * Notes: Lock port_info before calling this function
 */
void hfc_fx_cancel_wxque(
					struct port_info		*pp,
					struct core_info		*core,
					struct target_info_fx	*target,
					uint 					lun,
					struct hfc_pkt_fx		*hfcp,
					uint 					adap_status,
					uchar 					inh_altpath,
					uchar					type)
{
	struct hfc_pkt_fx	*wk_hfcp, *next_hfcp;
	int					result;
	struct dev_info_fx	*dev=NULL;

	HFC_DBGPRT(" hfcldd : hfc_fx_cancel_wxque - start\n");

	hfc_fx_stra_trace(
		HFC_FX_TRC_CAN_WX ,0x00 ,pp ,NULL ,NULL ,target ,hfcp ,
		type,lun,adap_status);
			
	wk_hfcp = target->core_queue[core->core_no].wx_que_top;
	
	while ( wk_hfcp != NULL )
	{
		next_hfcp = wk_hfcp->cmd_forw;	/* Preserve next hfc_pkt_fx */
		result = 0;

		if ( (type == HFC_FLASH_TARGET) 		/* Cancel each target	*/
	  	|| ((type == HFC_FLASH_DEV) 			/* Cancel each lun	*/
				&& (wk_hfcp->cmd_pkt != NULL) && (wk_hfcp->lun_id == lun) ))
		{
			/* Dequeue from wx_que */
			hfc_fx_deque_wx_que(core, wk_hfcp);
			
			dev = wk_hfcp->dev;

			/* Store cancellation factor and scsi response */
			wk_hfcp->adap_status = adap_status ;
			
			if (( test_bit(CFLAG_ABORT,(ulong *)&wk_hfcp->cmd_flags) 
			|| test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&wk_hfcp->cmd_flags)
			|| test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&wk_hfcp->cmd_flags) )
			&& ((dev != NULL)&&test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat)))					/* FCLNX-GPL-0343, FCLNX-GPL-FX-178 */
			{
				/* Turn off HFC_NEED_ABORT when cancel Abort Task Set request */
				clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
			}
			else if ((test_bit(CFLAG_LUN_RESET,(ulong *)&wk_hfcp->cmd_flags) 
			|| test_bit(CFLAG_CSCSI_LU_WAIT_DMA, (ulong *)&wk_hfcp->cmd_flags)
			|| test_bit(CFLAG_CSCSI_LU_WITHOUT_DMA, (ulong *)&wk_hfcp->cmd_flags) )
			&& ((dev != NULL)&&test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat)))					/* FCLNX-GPL-FX-178 */
			{
				/* Turn off HFC_NEED_LUN_RESET when cancel LUN RESET request */
				clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
			}																			/* FCLNX-GPL-0343 */
			else if (( test_bit(CFLAG_TARGET_RESET, (ulong *)&wk_hfcp->cmd_flags)
			|| test_bit(CFLAG_CSCSI_TGT_WAIT_DMA, (ulong *)&wk_hfcp->cmd_flags)
			|| test_bit(CFLAG_CSCSI_TGT_WITHOUT_DMA, (ulong *)&wk_hfcp->cmd_flags) )
			&& test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status ))
			{
				/* 
				 * Turn off NEED_TARGET_RESET 
				 * when cancel Target Reset request 
				 */
				clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );		/* FCLNX-GPL-036 */
			}
			else if ( test_bit(CFLAG_BUS_RESET, (ulong *)&wk_hfcp->cmd_flags) 
			&& test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status )){
				/* 
				 * Turn off NEED_TARGET_RESET 
				 * when cancel Target Reset request 
				 */
				clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );		/* FCLNX-GPL-036 */
			}
			
			/* Set reason code and initialize statistics *//* FCLNX-GPL-571 */
			if (inh_altpath == HFC_CSCSI_RESET)
			{
				result = DID_RESET;
			}
			else
			{
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
					result = DID_ERROR;
				}
				else if( test_bit( HFC_PS_ISOL, (ulong *)&pp->status) )
				{
					result = DID_NO_CONNECT;
				}
				else if( (target != NULL) && test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status) )
				{	/*** RSCN -> Target lost case ***/
					result = DID_ERROR;
				}
				else if( (target != NULL) && test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status))/********************/
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
			
//			pp->scsi_err_cnt++ ;							/* Error termination count  */
			hfc_fx_set_cmnd_res(pp, core, wk_hfcp->cmd_pkt, wk_hfcp, result);
			hfc_fx_iodone(pp, core, wk_hfcp->cmd_pkt, wk_hfcp);		/* Callback */
		}

		wk_hfcp = next_hfcp;					/* Move to the next hfc_pkt_fx */
	}
	
	hfc_fx_stra_trace(
		HFC_FX_TRC_CAN_WX,0x10, pp, NULL, NULL, target, hfcp,
		(unsigned long long)type,(unsigned long long)lun,(unsigned long long)adap_status);

}

/*
 * Function:    hfc_fx_cancel_tskmgm
 *
 * Purpose:     Cancel we_que
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  lun        - lun number
 *  hfcp       - Pointer to hfc_pkt_fx
 *  adap_status- Cause of cancellation
 *  type       - Range of cancellation
 *               type = HFC_FLASH_PACKET  : pp,hfcp
 *                    = HFC_FLASH_DEV     : pp,target,lun
 *                    = HFC_FLASH_TARGET  : pp,target
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_cancel_tskmgm(
	struct port_info			*pp,
	struct target_info_fx		*target,
	uint						lun,			/* FCLNX-GPL-0343 */
	struct hfc_pkt_fx			*hfcp,
	uint						adap_status,
	uint						type)
{
	struct dev_info_fx				*dev=NULL;		/* FCLNX-GPL-0343 */

	if (type == HFC_FLASH_TARGET)
	{
		if (test_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status) )		/* FCLNX-GPL-036 */
		{
			clear_bit(HFC_TS_NEED_TARGET_RESET, (ulong *)&target->status );		/* FCLNX-GPL-036 */
		}
		target->tgt_core_stat.all = 0;	/* FCLNX-GPL-FX-014 */
	}

	if (type == HFC_FLASH_TARGET)				/* FCLNX-GPL-0343 */
	{
		dev = target->dev;
		while( dev != NULL ){
			if (test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat))
			{
				clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
			}
			if (test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat))
			{
				clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
			}
			dev->dev_core_stat.all = 0;	/* FCLNX-GPL-FX-014 */
			dev = dev->next;
		}
	}

	if ( type == HFC_FLASH_DEV )
	{
		dev = target->dev;
		while( dev != NULL){
			if( dev->lun == lun){
				if (test_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat))
				{
					clear_bit(HFC_DS_NEED_ABORT, (ulong *)&dev->lustat);
				}
				if (test_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat))
				{
					clear_bit(HFC_DS_NEED_LUN_RESET, (ulong *)&dev->lustat);
				}
				dev->dev_core_stat.all = 0;	/* FCLNX-GPL-FX-014 */
			}
			dev = dev->next;
		}
	}											/* FCLNX-GPL-0343 */
}


/*
 * Function:    hfc_fx_deque_wx_que
 *
 * Purpose:     
 *
 * Arguments:   
 *  core     - Pointer to core_info
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_deque_wx_que(struct core_info *core, struct hfc_pkt_fx *hfcp)
{
	struct hfc_pkt_fx		*hfcp_wk ;
	struct target_info_fx	*target;
	
//	HFC_DBGPRT(" hfcldd : hfc_fx_deque_wx_que - start\n");
	
	target = (struct target_info_fx *)hfcp->target;
	
	/* Two or more packets exist in queue-*/
	if( hfcp == target->core_queue[core->core_no].wx_que_top )
	{
		if (hfcp == target->core_queue[core->core_no].wx_que_end) {
			target->core_queue[core->core_no].wx_que_top = NULL ;
			target->core_queue[core->core_no].wx_que_end = NULL ;
		}
		else {
			/* Dequeue from the top */
			target->core_queue[core->core_no].wx_que_top = (struct hfc_pkt_fx *)hfcp->cmd_forw ;
			if(target->core_queue[core->core_no].wx_que_top != NULL) /* FCLNX-GPL-185 */
			{
				target->core_queue[core->core_no].wx_que_top->cmd_prev = NULL ;
			}
		}
	}
	else if( hfcp == target->core_queue[core->core_no].wx_que_end )
	{
		/* Dequeue from the end */
		target->core_queue[core->core_no].wx_que_end = (struct hfc_pkt_fx *)hfcp->cmd_prev ;
		if(target->core_queue[core->core_no].wx_que_end != NULL) /* FCLNX-GPL-185 */
		{
			target->core_queue[core->core_no].wx_que_end->cmd_forw = NULL ;
		}
	}
	else
	{
		/* Otherwise */
		hfcp_wk = (struct hfc_pkt_fx *)hfcp->cmd_prev ;
		if( hfcp_wk != NULL) /* FCLNX-GPL-185 */
		{
			hfcp_wk->cmd_forw = hfcp->cmd_forw ;
		}
		
		hfcp_wk = (struct hfc_pkt_fx *)hfcp->cmd_forw ;
		if( hfcp_wk != NULL) /* FCLNX-GPL-185 */
		{
			hfcp_wk->cmd_prev = hfcp->cmd_prev ;
		}
	}
	core->wx_que_cnt_all--;
	target->core_queue[core->core_no].wx_que_cnt--;

	if( hfcp->cmd_flags & CFLAG_RESET_ANY  ){	/* FCLNX-GPL-FX-014 */
		target->core_queue[core->core_no].wx_que_tskmgm_cnt-- ;	
	}	/* FCLNX-GPL-FX-014 */
	
	if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() && hfcp->dev)
		hfc_manage_info.npubp->hfc_fx_queue_count(hfcp, 0, 1);	/* wx dequeue */

	hfcp->cmd_forw = NULL ;
	hfcp->cmd_prev = NULL ;

	return ;
}


/*
 * Function:    hfc_fx_enqueue_wx_que
 *
 * Purpose:     
 *
 * Arguments:   
 *  core       - Pointer to core_info
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_enqueue_wx_que(struct core_info *core, struct hfc_pkt_fx *hfcp)
{
	struct target_info_fx	*target;
	
//	HFC_DBGPRT(" hfcldd : hfc_fx_enqueue_wx_que - start\n");
	
	target = (struct target_info_fx *)hfcp->target;
	
	if( target->core_queue[core->core_no].wx_que_top == NULL ) {
		/* Add packet to empty queue */	
		target->core_queue[core->core_no].wx_que_top = hfcp ;	
		target->core_queue[core->core_no].wx_que_end = hfcp ;
		hfcp->cmd_forw = NULL;
		hfcp->cmd_prev = NULL;
	}
	else {
		/* Enqueue packet to the end of the cue */
		target->core_queue[core->core_no].wx_que_end->cmd_forw = hfcp;
		hfcp->cmd_prev = target->core_queue[core->core_no].wx_que_end ;
		target->core_queue[core->core_no].wx_que_end = hfcp ;
	}
	core->wx_que_cnt_all++;
	target->core_queue[core->core_no].wx_que_cnt++ ;
	
	if( hfcp->cmd_flags & CFLAG_RESET_ANY ){	/* FCLNX-GPL-FX-014 */
		target->core_queue[core->core_no].wx_que_tskmgm_cnt++ ;	
	}	/* FCLNX-GPL-FX-014 */
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() && hfcp->dev)
		hfc_manage_info.npubp->hfc_fx_queue_count(hfcp, 0, 0);	/* wx enqueue */
	
//	HFC_DBGPRT(" hfcldd : hfc_fx_enqueue_wx_que - end\n");
}


void hfc_fx_mq_cancel_scsi_cmd(struct port_info	*pp,
						struct target_info_fx	*target,
						uint					lun,
						struct hfc_pkt_fx		*hfcp,
						uint					adap_status,
						uchar					inh_altpath,
						uchar					cancel_scsi_cmd,
						uchar					wx_que_cancel,
						uchar					xob_cancel,
						uchar					we_que_cancel,
						uchar					tm_que_cancel,
						uchar					type)
{
	struct port_info		*vpp;
	struct target_info_fx  	*target_wk=NULL;
	struct core_info 		*core_wk=NULL;
	int i,j;
	
	HFC_DBGPRT(" hfcldd : hfc_fx_mq_cancel_scsi_cmd - start\n");
	
	if (HFC_FX_VIRTUAL_PORT(pp))
		return;
	if (!HFC_FX_MQ_VALID(pp))
		return;
	
	for (i=1; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		if (pp->region_arg[vpp->rid] == NULL)
			continue;
		target_wk = vpp->target_arg[target->pseq];
		if (target_wk == NULL)
			continue;
		
		for (j=0 ; j< MAX_CORE_PROBE_FX ; j += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core_wk = pp->region_arg[vpp->rid]->core_arg[j]) == NULL)
				continue;
			
			if (cancel_scsi_cmd == TRUE) {
				hfc_fx_cancel_scsi_cmd(vpp, core_wk, target_wk, lun, hfcp, adap_status, inh_altpath, we_que_cancel, tm_que_cancel, type);
				HFC_DBGPRT("hfcldd : hfc_fx_mq_cancel_scsi_cmd - cancel_scsi_cmd, rid=%d, target_id=%d, lun_id=%d\n",
							vpp->rid, target_wk->target_id, lun);
			}
			else {
				if (xob_cancel == TRUE) {
					hfc_fx_cancel_xob(vpp, core_wk, target_wk, lun, hfcp , type);
					HFC_DBGPRT("hfcldd : hfc_fx_mq_cancel_scsi_cmd - xob_cancel, rid=%d, target_id=%d, lun_id=%d\n",
								vpp->rid, target_wk->target_id, lun);
				}
				
				if (we_que_cancel == TRUE) {
					hfc_fx_notify_tout(vpp, core_wk, target_wk, lun, type);
					hfc_fx_cancel_weque(vpp, core_wk, target_wk, lun, NULL, adap_status, inh_altpath, type);
					HFC_DBGPRT("hfcldd : hfc_fx_mq_cancel_scsi_cmd - we_que_cancel, rid=%d, target_id=%d, lun_id=%d\n",
								vpp->rid, target_wk->target_id, lun);
				}
				
				if (tm_que_cancel == TRUE) {
					hfc_fx_cancel_tskmgm(vpp, target_wk, lun, hfcp, adap_status, type);
					HFC_DBGPRT("hfcldd : hfc_fx_mq_cancel_scsi_cmd - tm_que_cancel, rid=%d, target_id=%d, lun_id=%d\n",
								vpp->rid, target_wk->target_id, lun);
				}
				
				if (wx_que_cancel == TRUE) {
					hfc_fx_cancel_wxque(vpp, core_wk, target_wk, lun, hfcp, adap_status, inh_altpath, type);
					HFC_DBGPRT("hfcldd : hfc_fx_mq_cancel_scsi_cmd - wx_que_cancel, rid=%d, target_id=%d, lun_id=%d\n",
								vpp->rid, target_wk->target_id, lun);
				}
			}
		}
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_mq_cancel_scsi_cmd - end\n");
	
	return;
}


/* FCLNX-GPL-FX-274, 285 */
void hfc_fx_mq_cancel_xrb(struct port_info	*pp,
						struct target_info_fx	*target,
						uchar					type)
{
	struct port_info		*vpp;
	struct target_info_fx  	*target_wk=NULL;
	int i;
	
	HFC_DBGPRT(" hfcldd : hfc_fx_mq_cancel_xrb - start\n");
	
	if (HFC_FX_VIRTUAL_PORT(pp))
		return;
	if (!HFC_FX_MQ_VALID(pp))
		return;
	
	for (i=1; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		if (pp->region_arg[vpp->rid] == NULL)
			continue;
		target_wk = vpp->target_arg[target->pseq];
		if (target_wk == NULL)
			continue;
		
		hfc_fx_cancel_xrb(vpp, target_wk, type);
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_mq_cancel_xrb - end\n");
}
/* FCLNX-GPL-FX-274, 285 */

/*
 * Function:    hfc_fx_resource_chk
 *
 * Purpose:     Check remaining resource of xob, scmd_buf, and iov_map.
 *              Completing this function means core->iov_no and hfcp->iov_cnt
 *              is set and iov_map is renewed.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  hfcp       - Pointer to hfc_pkt_fx
 *  fw_xob_outp- 
 *
 * Returns:     
 *  0                  - Success
 *  HFC_XOB_EMPTY(1)   - Success (Xob is empty)
 *  HFC_XOB_FULL(4)    - Xob is full
 *  HFC_SCMD_FULL(5)   - Scmd_buf is full
 *  HFC_IOVMAP_FULL(6) - Bus address area is full for DMA mpaping
 *  HFC_PAGE_OVER(-1)  - The number of Pages is larger than iov_map_cnt
 *  
 *
 * Notes: This routine is called from the process level and the interruption level.

 */
int hfc_fx_resource_chk(struct port_info *pp, struct core_info *core, struct hfc_pkt_fx *hfcp)
{
	struct scsi_cmnd	*cmnd ;
	struct free_iov_map	free_iov ;
	
	uint				page_cnt = 0, page_cnt1=0 ;
	uint				bit_pos = 0 ;
	uint				start_pos_word = 0 ;
	uint				start_pos_in_word = 0 ;
	uchar				find ;
	uint				seg_cnt, ioctl_mode=0;
	uint				next_xob;
	uint32_t			data_size, data_size1;
	uint64_t			dma_a;
	uint				dma_size;
	int					i;
	ushort				chk_next = 0;	/* FCLNX-GPL-FX-014 */

	cmnd = hfcp->cmd_pkt ;
	
//	if( cmnd == NULL){
//		hfc_fx_stra_trace(
//			HFC_FX_TRC_RES_CHK ,0x17 , pp, hfcp->rp, core ,hfcp->target ,hfcp,
//			0, 0, 0);
//		return( HFC_XOB_FULL ) ;
//	}
	
	/* kernel 5.16+: detect ioctl via ap->ioctl_cmnd */
	if( cmnd == ap->ioctl_cmnd ) ioctl_mode=1;

	if( hfcp->cmd_flags & CFLAG_RESET_ANY ){	/* FCLNX-GPL-FX-014 */
		chk_next = 1; // reset
	} else {
		chk_next = 2; // scsi
	}	/* FCLNX-GPL-FX-014 */
	
	/* xob full check */
	if((core->drv_next_xob+chk_next) >= pp->xob_max) {	/* FCLNX-GPL-FX-014 Start */
		next_xob = (core->drv_next_xob + chk_next) - pp->xob_max;
	}
	else {
		next_xob = core->drv_next_xob+chk_next;
	}	/* FCLNX-GPL-FX-014 End */
	if ((core->xob[core->drv_next_xob].flag & HFC_XOB_VALID) || // current
	    (core->xob[next_xob].flag           & HFC_XOB_VALID))   // next
	{
		hfc_fx_stra_trace(
		HFC_FX_TRC_RES_CHK ,0x12 , pp, hfcp->rp, core ,hfcp->target ,hfcp,
			(ulong)cmnd, 0, 0);
		return( HFC_XOB_FULL );
	}

	/* Calculate the number of seg_info(seg_cnt) */
	seg_cnt = scsi_sg_count(cmnd);
	if (seg_cnt)
	{
		if( !ioctl_mode ){
			seg_cnt = dma_map_sg(&pp->pci_cfginf->dev, scsi_sglist(cmnd), scsi_sg_count(cmnd),
				cmnd->sc_data_direction);
		}
	}
	hfcp->seg_cnt = seg_cnt;

	/* Calculate data_size */
	if(seg_cnt)
		data_size = scsi_bufflen(cmnd);
	else
		data_size = 0;
	
	if (pp->pm_control == HFC_FX_PM_ON) {
		if (hfcp->pm_pkt_no != 0xffff) {
			pp->pm_pkt_pool[hfcp->pm_pkt_no].data_size = data_size;
		}
	}
	hfcp->data_size = data_size;

	/* Calculate how many page number will be used in SEG-INFO-LIST */
	if(seg_cnt > 4)
		page_cnt = seg_cnt - 4;
	else /* seg_info in XOB area will be used. */
		page_cnt = 0;

	if( page_cnt > core->iov_map_cnt )
	{
		dma_a = 0;
		HFC_BZERO(core->logdata,16) ;
		HFC_MEMCPY(core->logdata,(uchar*)&dma_a, 8) ;
		HFC_4L_TO_4B(data_size1, hfcp->data_size);
		HFC_MEMCPY(&core->logdata[8],(uchar*)&(data_size1),4) ;
		HFC_4L_TO_4B(page_cnt1, page_cnt);
		HFC_MEMCPY(&core->logdata[12],(uchar*)&(page_cnt1),4) ;
		hfc_fx_errlog(pp,core,hfcp->target,hfcp,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_ERR9,0x05,core->logdata,16) ;
		return( HFC_PAGE_OVER ) ;
	}
	
	/* calculate data_size in we queue */
	if (!(pp->debug_func & HFC_FX_DEBUG_SKIP_DS_CHECK)) {
		dma_size = 0;
		for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num) {
			dma_size += hfcp->rp->core_arg[i]->we_que_sizecnt;
		}
		dma_size += hfcp->data_size;
		if (dma_size > pp->dma_max) {
			core->dma_max_over_cnt++;
			hfc_fx_stra_trace(
				HFC_FX_TRC_RES_CHK ,0x17 ,pp ,hfcp->rp ,core ,hfcp->target ,hfcp,
				(ulong)cmnd, 0, 0);
			return( HFC_DMA_MAX_OVER ) ;
		}
	}
	
	hfcp->iov_cnt = 0 ;
	hfcp->iov_no = 0 ;
	if( ( page_cnt > 0 ) && ( !( HFC_HFCP_FX_CFLAG_TEST(CFLAG_SEGVALID, hfcp) ) ) )
	{
		/* Search necessary consecutive area from iov_map */
		start_pos_in_word = 0 ;		/* Start bit for search */
		start_pos_word = 0 ;		/* Start word for search */
									/* The top address of iov_map		*/
		bit_pos = 0 ;
		find = FALSE ;
		while( (bit_pos < core->iov_map_cnt) && (find == FALSE) )
		{
			/* Caliculate bit position and length of consecutive area */
			hfc_fx_get_free_iov(core,start_pos_word,
								start_pos_in_word,page_cnt,&free_iov) ;

			/* No enough space in iov_map */
			if( free_iov.free_cnt == 0 )
			{
				hfc_fx_stra_trace(
					HFC_FX_TRC_RES_CHK ,0x14 ,pp ,hfcp->rp ,core ,hfcp->target ,hfcp,
					(ulong)cmnd, 0, 0);
				return( HFC_IOVMAP_FULL ) ;
			}
			if( free_iov.free_cnt >= page_cnt )
			{
				core->iov_no = free_iov.free_pos ;
				hfcp->iov_no = core->iov_no;
				hfcp->iov_cnt = page_cnt ; /* FIVE-FX */
				find = TRUE ;
				break ;
			}
			bit_pos = free_iov.free_cnt + free_iov.free_pos ;
			start_pos_word = bit_pos / 32 ;
			start_pos_in_word = bit_pos % 32 ;
		}
		if( find == FALSE )
		{
			hfc_fx_stra_trace(
				HFC_FX_TRC_RES_CHK ,0x15 ,pp ,hfcp->rp ,core ,hfcp->target ,hfcp,
				(ulong)cmnd, 0, 0);
			return( HFC_IOVMAP_FULL ) ;
		}
		
		if( ( hfcp->iov_cnt+hfcp->iov_no ) > core->iov_map_cnt )
		{
			HFC_ERRPRT(" hfcldd : hfc_fx_resource_chk - ( hfcp->iov_cnt+hfcp->iov_no ) > core->iov_map_cnt \n");

			hfc_fx_stra_trace(
				HFC_FX_TRC_RES_CHK ,0x16 ,pp ,hfcp->rp ,core ,hfcp->target ,hfcp,
				(ulong)cmnd, 0, 0);
			return( HFC_IOVMAP_FULL ) ;
		}
		hfc_fx_iov_update(core,hfcp->iov_no,hfcp->iov_cnt,0) ;
	}
	
	return(0) ;
}


/*
 * Function:    hfc_fx_get_free_iov
 *
 * Purpose:     Find first available consecutive bits from the position of
 *              st_word and st_bit in iov_map. 
 *              Iov_map is full when free_iov_map.free_cnt is zero.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  st_word    - Start position of word search
 *  st_bit     - Start position of bit search
 *               (st_bit % 4) MUST BE 0
 *  req_bit_cnt - 
 *  fmap        -
 *
 * Returns:     
 *  free_iov_map.free_cnt - Number of available consecutive bits
 *  free_iov_map.free_pos - Bit start position of free_cnt in iov_map (64byte boundary)
 *                          (free_iov_map.free_pos % 4) MUST BE 0
 *
 * Notes:		Lock port_info before calling this function
 */
void hfc_fx_get_free_iov(struct core_info *core ,
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
	iov = core->iov_map ;			
	word_pos = st_word ;			
	free_map.free_pos = 0 ;			
	free_map.free_cnt = 0 ;         
	chk_bit_cnt = 0 ;				

	bit_pos = st_word * 32 + st_bit ;

	if( st_bit != 0 )
	{
		for(lp=st_bit ; lp<32 ; lp += 4)
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
				bit_pos += 4 ;
				/* Set the number of free bits to free_map */
				free_map.free_cnt += 4 ;
			}
			else
			{
				/* Detect "0" (already used) */
				if( find == TRUE )
				{
					/* Return current bits stored in free_map */
					fmap->free_cnt = free_map.free_cnt;
					fmap->free_pos = free_map.free_pos;
					return;
				}
				else
				{
					/* increment bit position */
					bit_pos += 4 ;
				}
			}
			if( bit_pos >= core->iov_map_cnt )
			{
				/* Bit position exceeded the maximum bit position */
				fmap->free_cnt = free_map.free_cnt;
				fmap->free_pos = free_map.free_pos;
				return;
			}
			if( free_map.free_cnt > req_bit_cnt ) {
				fmap->free_cnt = free_map.free_cnt;
				fmap->free_pos = free_map.free_pos;
				return;
			}
		}
		word_pos++ ; /* Move to the next word */
	}

	while( bit_pos < core->iov_map_cnt )
	{
		if( free_map.free_cnt > req_bit_cnt ) {
			fmap->free_cnt = free_map.free_cnt;
			fmap->free_pos = free_map.free_pos;
			return;
		}
		if( (iov[word_pos] & 0xFFFFFFFF) == 0 )
		{
			/* This word is full */
			if( find == TRUE )
			{
				fmap->free_cnt = free_map.free_cnt;
				fmap->free_pos = free_map.free_pos;
				return;
			}
			else {
				bit_pos += 32 ;
				word_pos++ ;
				continue ;
			}
		}
		if( (iov[word_pos] & 0xFFFFFFFF) == 0xFFFFFFFF )
		{
			/*This word is empty */
			if( find == TRUE ) {
				free_map.free_cnt += 32 ;
				bit_pos += 32 ;
				word_pos++ ;
			}
			else {
				/* Detect "1" for the first time. */
				find = TRUE ;
				free_map.free_pos = bit_pos ;
				free_map.free_cnt += 32 ;
				bit_pos += 32 ;
				word_pos++ ;
			}
			continue;
		}
		/* Iov word_pos is neither all zeros nor all 1. --*/
		for(lp=0 ; lp<32 ; lp += 4)
		{
			if( (shift >> lp) & iov[word_pos] )
			{
				if( find == FALSE )
				{
					find = TRUE ;
					free_map.free_pos = bit_pos ;
				}
				free_map.free_cnt += 4 ;
				bit_pos += 4 ;
			}
			else
			{
				if( find == TRUE )
				{
					fmap->free_cnt = free_map.free_cnt;
					fmap->free_pos = free_map.free_pos;
					return;
				}
				else
					bit_pos += 4 ;
			}
			if( bit_pos >= core->iov_map_cnt )
			{
				fmap->free_cnt = free_map.free_cnt;
				fmap->free_pos = free_map.free_pos;
				return;
			}
			if( free_map.free_cnt > req_bit_cnt ) {
				fmap->free_cnt = free_map.free_cnt;
				fmap->free_pos = free_map.free_pos;
				return;
			}

		}
		word_pos++ ;
	}

	fmap->free_cnt = free_map.free_cnt;
	fmap->free_pos = free_map.free_pos;

	return;
}


/*
 * Function:    hfc_fx_iov_update
 *
 * Purpose:     Update bits of core->iov_map with specified type starting from 'pos'
 *              bit to 'pos + cnt' bit.
 *
 * Arguments:   
 *  core       - Pointer to core_info
 *  pos        - Update start bit in iov_map
 *  cnt        - Number of update counts in iov_map
 *  type       - 0 : This area is used 1: This area is free
 *
 * Returns:     
 *
 * Notes:		Lock port_info before calling this function
 */
void hfc_fx_iov_update(struct core_info *core ,
					uint pos ,
					uint cnt ,
					uchar type)
{
	uint	*iov ;					/* iov_map				*/
	uint	word_pos = 0 ;			/* Word position of iov_map  */
	uint    bit_pos_in_word ;		/* Bit position in word   */
	uint	shift = 0xF0000000 ;    /* update bits		*/
	int		bit_cnt = 0 ;
	uint	bit_pos = 0 ;

	word_pos = pos/32 ;
	bit_pos_in_word = pos%32 ;
	iov = core->iov_map ;
	bit_cnt = cnt ;
	bit_pos = pos ;

	while( bit_cnt > 0 )
	{
		if( bit_pos_in_word != 0 )
		{
			if( type == 0 )
				iov[word_pos] &= ~(shift >> bit_pos_in_word) ;
			else
				iov[word_pos] |= (shift >> bit_pos_in_word) ;
			
			bit_pos += 4 ;
			bit_cnt -= 4 ;
		}
		else
		{
			if( bit_cnt >= 32 )
			{
				if( type == 0 )
					iov[word_pos] = 0 ;
				else
					iov[word_pos] = 0xFFFFFFFF ;
				bit_pos+=32 ;
				bit_cnt-=32 ;
			}
			else
			{
				if( type == 0 )
					iov[word_pos] &= ~(shift >> bit_pos_in_word) ;
				else
					iov[word_pos] |= (shift >> bit_pos_in_word) ;
				bit_pos += 4 ;
				bit_cnt -= 4 ;
			}
		}
		if( bit_pos < core->iov_map_cnt )
		{
			word_pos = bit_pos/32 ;
			bit_pos_in_word = bit_pos%32 ;
		}
		else
		{
			return ;
		}
	}

	return ;
}


/*
 * Function:    hfc_fx_dma_map
 *
 * Purpose:     Setup DMA cookie from seg_info
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *  DMA_SUCC(0) - success
 *  DMA_NOACC   - Unable to access
 *
 * context:   user / interrupt
 *
 * Notes:		Lock port_info before calling this function
 *              This function set xob->seg_5th, xob->seg_cnt and xob->seg_info_xob
 *              in addition to seg_info of hfc_pkt_fx.
 */
void hfc_fx_dma_map(struct port_info *pp, struct core_info *core, struct hfc_pkt_fx *hfcp)
{
	uint				seg_no ;				/* seg_info counter for SEG-INFO-LIST */
	uint64_t			seg_info_baddr = 0; 	/* seg_info badr in xob */
	uint				xob_no,cn=0;
	struct seg_info_fx 	*seg_info_ptr=NULL;
	struct scsi_cmnd	*cmnd;
	dma_addr_t			b_dma_a;
	uint32_t			b_length;
	uint				xob_segno;
	uint64_t			dma_add;
	struct scatterlist	*cur_seg;
	ushort				seg_cnt;

	/* Set seg_info */
	xob_no = core->drv_next_xob;
	hfcp->cmd_xob = xob_no;
	cmnd = hfcp->cmd_pkt;

	if ( hfcp->data_size > 0 )	/* Transfer data exists? */
	{
		if (hfcp->seg_cnt)
		{
			cn = 0; 								/* DMA cookie#			*/
			seg_no = hfcp->iov_no; 					/* seg_info offset number in SEG-INFO-LIST */

			if(hfcp->seg_cnt > 0)
			{
				scsi_for_each_sg(cmnd, cur_seg, hfcp->seg_cnt, cn) {
					if(cn < 4)
					{	/* If hfcp->iov_no is smaller than 4, use SEG-INFO in XOB area */
						seg_info_ptr = (struct seg_info_fx *)&(core->xob[xob_no].seg_info_xob[cn]);
						b_dma_a = sg_dma_address(cur_seg);
						b_length = sg_dma_len(cur_seg);
						dma_add = (uint64_t)b_dma_a;	/* Bug */
						if(cn == 0)
						{
							hfcp->dma_handle = b_dma_a;
						}
						HFC_8L_TO_8B(seg_info_ptr->seg_addr, dma_add);
						HFC_4L_TO_4B(seg_info_ptr->seg_len, b_length);
//						dma_add = (uint64_t)b_dma_a;
						xob_segno = (uint)((xob_no << 24) & 0xff000000) | cn ;
						HFC_4L_TO_4B(seg_info_ptr->xob_segno, xob_segno);	/* Bug */
					}
					else
					{	/* Otherwise, use SEG-INFO-LIST */
						seg_info_ptr = &core->seg_info[seg_no];
//						if (seg_info_ptr->seg_len & HFC_SEG_LEN_F)
//						{
//							seg_no++;
//							seg_info_ptr = &core->seg_info[seg_no];
//						}
						b_dma_a = sg_dma_address(cur_seg);
						b_length = sg_dma_len(cur_seg);
						dma_add = (uint64_t)b_dma_a;
						HFC_8L_TO_8B(seg_info_ptr->seg_addr, dma_add);
						HFC_4L_TO_4B(seg_info_ptr->seg_len, b_length);
						xob_segno = (uint)((xob_no << 24) & 0xff000000) | cn ;
						HFC_4L_TO_4B(seg_info_ptr->xob_segno, xob_segno);
						
						seg_no++;
					}
				}
				/* Turn on L bit of final seg_info */
				seg_info_ptr->seg_len |= HFC_SEG_LEN_L ;
			}
		}
		set_bit(CFLAG_SEGVALID, (ulong *)&hfcp->cmd_flags);
	}

	if(cn < 4) {
		/* seg_5th null */
	}
	else {
		/* set seg_5th */
		seg_info_baddr = core->seg_phys_addr + (hfcp->iov_no*sizeof(struct seg_info_fx));
		HFC_8L_TO_8B(core->xob[xob_no].seg_5th, seg_info_baddr);
	}
	
	seg_cnt = (ushort)cn;
	HFC_2L_TO_2B(core->xob[xob_no].seg_cnt, seg_cnt);	/* No count for F=1 case */	/* Bug */

	return;
}


/*
 * Function:    hfc_fx_make_cmdiu
 *
 * Purpose:     Set FCP-CMD-IU payload to the area pointed by hfc_pkt_fx->cmd_xob
 *              Generate setting data from scsi_pkt and hfcpkt.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  hfcp       - Pointer to hfc_pkt_fx 
 *
 * Returns:     
 *
 * context:     user / kernel / interrupt
 *
 * Notes:		Lock port_info before calling this function
 */
void hfc_fx_make_cmdiu( struct port_info *pp, struct hfc_pkt_fx *hfcp )
{
	struct fcp_cmd_iu_fx  fcp_cmd;
	struct scsi_cmnd *cmnd = hfcp->cmd_pkt;
	ushort		lun_id=0;							/* FCLNX-GPL-0343 */
	
//	HFC_DBGPRT(" hfcldd%d : hfc_fx_make_cmdiu - start\n", pp->dev_minor);

	/* Caliculate pointer to FCP-CMD-IU area of xob and clear the area */
	HFC_BZERO( &fcp_cmd, sizeof(struct fcp_cmd_iu_fx) );
	
	if( hfcp->cmd_flags & CFLAG_CSCSI_ANY){	/* FCLNX-GPL-FX-014 */
		HFC_2L_TO_2B(fcp_cmd.fcp_lun.lun, hfcp->lun_id);		/* FCLNX-GPL-FX-112 */
		HFC_MEMCPY((fcp_cmd_iu_fx_t *)(&hfcp->core->xob[hfcp->cmd_xob].fcp_cmd),
										&fcp_cmd, sizeof(struct fcp_cmd_iu_fx));	/* FCLNX-GPL-FX-112 */
		return;
	}	/* FCLNX-GPL-FX-014 */

	/* FCP-LUN Byte0,1 */
	lun_id = (ushort)cmnd->device->lun;
	HFC_2L_TO_2B(fcp_cmd.fcp_lun.lun, lun_id);		/* FCLNX-GPL-0343 */

	/* FCP-CNTL Byte1 */
	if ( !HFC_HFCP_FX_CFLAG_TEST(CFLAG_ABORT, hfcp)
	  && !HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp)
	  && !HFC_HFCP_FX_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)
	  && !HFC_HFCP_FX_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) )
	{
  		/* kernel 5.4+: cmnd->tag removed; use Simple-Q */
  		if (cmnd->device->tagged_supported) {
    			fcp_cmd.fcp_cntl.qtype = 0;			/* Simple-Q */
  		}
  		else{
    		fcp_cmd.fcp_cntl.qtype = 0;				/* Simple-Q : FIVE-FX */
    	}
  	}
  	else{
    	fcp_cmd.fcp_cntl.qtype = 0;				/* Simple-Q : FIVE-FX		*/
    }


	/* FCP-CNTL Byte2 */
	if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_ABORT, hfcp))
	{
		/* Abort Task Set */
		fcp_cmd.fcp_cntl.task_mgm |= HFC_ABORT_TASK;
	}
	else if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp))
	{
		/* LUN Reset */
		fcp_cmd.fcp_cntl.task_mgm |= HFC_LUN_RST;
	}
	else if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp)
		 ||  HFC_HFCP_FX_CFLAG_TEST(CFLAG_BUS_RESET, hfcp))
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
	else if ( cmnd->sc_data_direction == SCSI_DATA_READ)
		fcp_cmd.fcp_cntl.data_type |= HFC_READ_DATA;	/* bit6 */

	/* FCP-DL */
	/* fcp_cmd->fcp_data_len = cmnd->bufflen; */
	HFC_4L_TO_4B(fcp_cmd.fcp_dl, hfcp->data_size );

	/* Copy hfc_pkt_fx to XOB */
	HFC_MEMCPY((fcp_cmd_iu_fx_t *)(&hfcp->core->xob[hfcp->cmd_xob].fcp_cmd),
										&fcp_cmd, sizeof(struct fcp_cmd_iu_fx));

	if (hfcp->seg_cnt == 0) {													/* FCLNX-0404 */
		if (hfcp->data_size == 0) {
			pp->controlrequests++;
		}
		else {
			if (cmnd->sc_data_direction == SCSI_DATA_WRITE) {
				pp->outputrequests++;
				pp->outputbytes += hfcp->data_size;
			}
			else if (cmnd->sc_data_direction == SCSI_DATA_READ) {
				pp->inputrequests++;
				pp->inputbytes += hfcp->data_size;
			}
		}
	}	
	else {
		if (cmnd->sc_data_direction == SCSI_DATA_WRITE) {
			pp->outputrequests++;
			pp->outputbytes += hfcp->data_size;
		}
		else if (cmnd->sc_data_direction == SCSI_DATA_READ) {
			pp->inputrequests++;
			pp->inputbytes += hfcp->data_size;
		}
	}																			/* FCLNX-0404 */

	return ;
}


/*
 * Function:    hfc_fx_xob_enque
 *
 * Purpose:     Create xob
 *              Dequeue hfc_pkt_fx from the wx_que and enqueue it to we_que
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  hfcp       - Pointer to hfc_pkt_fx  
 *  target     - Pointer to target_info_fx
 * Returns:     
 *  0          - No need for starting timer
 *
 * context:     user / kernel / interrupt
 *
 * Notes:		Lock port_info before calling this function
 */
void hfc_fx_xob_enque(struct port_info *pp,
							 struct core_info *core,
							 struct target_info_fx *target,
							 struct hfc_pkt_fx *hfcp)
{
	struct xob_fx		*xob_p ;
	uint				hash ;
	struct hfc_pkt_fx	**we_que_top_p;
	struct hfc_pkt_fx	**we_que_end_p;
	uchar				i;

	xob_p = &core->xob[core->drv_next_xob];

	/* Set Valid bit of XOB_Flag */
	core->xob[core->drv_next_xob].flag |= HFC_XOB_VALID;

	/* Clear Skip bit of XOB_SKIP */
	xob_p->skip &= ~HFC_XOB_SKIP;

	/* XOB - Driver Used Area */
	memset(&xob_p->drv_work, 0, sizeof(xob_p->drv_work));
	xob_p->drv_work   = (unsigned long long) (ulong) hfcp ;	/* FIVE-FX */

	/* Reset skip flag */
	xob_p->skip &= ~HFC_XOB_SKIP ;
	
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
		xob_p->queue_no = (uchar)pp->rid;	/* Multi Queue */
	}

	/* This variable was set in hfc_fx_dma_map
	   xob_p->seg_cnt = ((struct xob_fx *)xob_p->drv_work)->seg_cnt ; */
	HFC_2L_TO_2B(xob_p->prli_parm, target->prli_parm);
	HFC_2L_TO_2B(xob_p->mfsize, target->mfsize) ;

	for(i=0;i<3;i++) xob_p->trans_d_id[i] =
						 (uchar)((target->scsi_id & 0x0000000000ffffff) >> ((2-i)*8)); /* Trans D_ID */
	for(i=0;i<3;i++) xob_p->trans_s_id[i] =
						 (uchar)((pp->scsi_id     & 0x0000000000ffffff) >> ((2-i)*8)); /* Trans S_ID */
	
	/* This variable was set in hfc_fx_dma_map
	xob_p->seg_5th = 0; */

	/* Count up xob inp */
	core->drv_next_xob++ ;
	if ( core->drv_next_xob >= pp->xob_max )
		core->drv_next_xob = 0 ;

	hfc_fx_deque_wx_que(core, hfcp);

	hash = hfcp->lun_id % HASH_T_NUM;
	/* Enqueue wait_end_que */
	/* Move packet from wait_xob_que to wait_end_que. */
	we_que_top_p = &target->core_queue[core->core_no].we_que_top[hash];
	we_que_end_p = &target->core_queue[core->core_no].we_que_end[hash];

	if( *we_que_top_p == NULL )
	{
		/* Wait_end_que is empty */
		hfcp->cmd_forw = NULL;
		hfcp->cmd_prev = NULL;
		target->core_queue[core->core_no].we_que_top[hash] = hfcp ;
		target->core_queue[core->core_no].we_que_end[hash] = hfcp ;
	}
	else
	{
		/* Enqueue packet to the last of wait_end_que. */
		target->core_queue[core->core_no].we_que_end[hash]->cmd_forw = (struct hfc_pkt_fx *)hfcp ;
		hfcp->cmd_prev = (struct hfc_pkt_fx *)target->core_queue[core->core_no].we_que_end[hash] ;
		target->core_queue[core->core_no].we_que_end[hash] = hfcp ;
		target->core_queue[core->core_no].we_que_end[hash]->cmd_forw = NULL ;
	}
	
	core->we_que_sizecnt += hfcp->data_size;
	core->we_que_cnt_all++;
	target->core_queue[core->core_no].we_que_cnt++;

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() && hfcp->dev)
		hfc_manage_info.npubp->hfc_fx_queue_count(hfcp, 1, 0);	/* we enqueue */

	return ;
}


/*
 * Function:    hfc_fx_enque_next_dstart
 *
 * Purpose:     Enqueue target to the next start target queue (core->next_dstart)
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *  target     - Pointer to target_info
 * Returns:     
 *
 *
 * Notes:		Lock port_info before calling this function
 *				This function update following elements;
 * 			 		core->next_dstart_top - The top of next start target queue
 *              	core->next_dstart_end - The end of next start target queue
 *              	core->next_dstart_cnt - Number of enqueued target
 *              	target_info->next_dstart_flag 
 *						- Flag means this target has already registered.
 */
void hfc_fx_enque_next_dstart(struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target)
{
	if( target->core_queue[core->core_no].next_dstart_flag )	/* Target has already registered in queue	*/
		return ;

	if( core->next_dstart_cnt == 0 )
	{
		/* Enqueue to the empty queue */
		core->next_dstart_top = target ;
		core->next_dstart_end = target ;

		target->core_queue[core->core_no].nnext = NULL ;
		target->core_queue[core->core_no].nprev = NULL ;
	}
	else
	{
		/* Enqueue as a last element */
		core->next_dstart_end->core_queue[core->core_no].nnext = target ;
		target->core_queue[core->core_no].nprev = core->next_dstart_end ;
		core->next_dstart_end = target ;
		target->core_queue[core->core_no].nnext = NULL ;
	}

	hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0,HFC_FX_WEXEC_TMR, 0, FALSE);
	core->next_dstart_cnt++  ;
	target->core_queue[core->core_no].next_dstart_flag = 1 ;
}

/*
 * Function:    hfc_fx_deque_next_dstart
 *
 * Purpose:     Dequeue target from next start target queue (core->next_dstart)
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *  target     - Pointer to target_info
 *
 * Returns:     -
 *
 * Notes:		Lock port_info before calling this function
 *				This function update following elements;
 * 			 		core->next_dstart_top - The top of next start target queue
 *              	core->next_dstart_end - The end of next start target queue
 *              	core->next_dstart_cnt - Number of enqueued target
 *              	dstart_info->next_dstart_flag 
 *						- Flag means this target has already registered.
 */
void hfc_fx_deque_next_dstart(struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target)
{
	if( core->next_dstart_cnt == 0 )		/* There is no target for dequeuing */
		return ;

	if( !target->core_queue[core->core_no].next_dstart_flag  ) 	/* This target is not enqueued	*/
		return ;

//	if( (target->core_queue[core->core_no].wx_que_cnt == 0)		/* Wx_que empty? */
//		&& (core->next_dstart_cnt == 1) )	/* Any target waiting for initiation? */
//	{
//		target->core_queue[core->core_no].next_dstart_flag = 2 ;
//		return ;
//	}

	if( core->next_dstart_cnt == 1 )		/* The number of queue element is one */
	{
		core->next_dstart_top = NULL ;
		core->next_dstart_end = NULL ;
		core->next_dstart_cnt = 0 ;
	}
	else
	{
		if( target == core->next_dstart_top ) 		/* Dequeue from the top of the queue */
		{
			core->next_dstart_top = target->core_queue[core->core_no].nnext ;
			target->core_queue[core->core_no].nprev = NULL ;
			core->next_dstart_cnt--;
		}
		else if( target == core->next_dstart_end ) /* Dequeue from the last of the queue */
		{
			core->next_dstart_end = target->core_queue[core->core_no].nprev ;
			target->core_queue[core->core_no].nnext = NULL ;
			core->next_dstart_cnt--;
		}
		else										/* Otherwise */
		{
			if( (target->core_queue[core->core_no].nprev != NULL)
			 && (target->core_queue[core->core_no].nnext != NULL) )
			{
				target->core_queue[core->core_no].nprev->core_queue[core->core_no].nnext
					= target->core_queue[core->core_no].nnext ;
				target->core_queue[core->core_no].nnext->core_queue[core->core_no].nprev
					= target->core_queue[core->core_no].nprev ;
				core->next_dstart_cnt--;
			}
		}
	}

	target->core_queue[core->core_no].nnext = NULL ;
	target->core_queue[core->core_no].nprev = NULL ;
	target->core_queue[core->core_no].next_dstart_flag = 0 ;
	if (core->next_dstart_cnt == 0) {
		hfc_fx_watchdog_enter(pp, core, NULL, NULL, 0,HFC_FX_WEXEC_TMR, 0, TRUE);
	}
}

void hfc_fx_set_cmnd_res(
	struct port_info		*pp,
	struct core_info		*core,
	struct scsi_cmnd		*cmnd,
	struct hfc_pkt_fx		*hfcp,
	uint					result )
{
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) {
		hfc_manage_info.npubp->hfc_fx_mp_set_cmnd_res(pp, core, cmnd, hfcp, result);
		return;
}

	if (hfcp != NULL) {
		if ( hfcp->cmd_flags & CFLAG_RESET_ANY )	/* FCLNX-GPL-FX-014 */
		{
			if (cmnd != NULL) {
				clear_bit(CMND_VALID, (ulong *)&cmnd->eh_eflags);
			}
		}
	}
	
	if (cmnd != NULL) {
		cmnd->result &= 0xff00ffff;
		cmnd->result |= (result << 16);
	}
}

/*
 * Function:    hfc_fx_iodone
 *
 * Purpose:     Termination process of SCSI initiation.
 *
 * Arguments:   
 *  pp          - Pointer to port_info
 *  cmnd        - Pointer to scsi_cmnd 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_iodone(
	struct port_info		*pp,
	struct core_info		*core,
	struct scsi_cmnd		*cmnd,
	struct hfc_pkt_fx		*hfcp )
{
	struct dev_info_fx		*dev=NULL;
	
	uchar					write_retries=0;
	uint					ioctl_mode=0;
	uint					data_size;
	uchar					size;
	
	/* kernel 5.16+: detect ioctl via ap->ioctl_cmnd */
	if( cmnd == ap->ioctl_cmnd ) ioctl_mode=1;

	if (hfcp != NULL)
	{
		dev = hfcp->dev;
		/* Update iov only when normal SCSI initiation or IOCTL request occurs */
		if(	HFC_HFCP_FX_CFLAG_TEST(CFLAG_SEGVALID, hfcp) )
		{
			if (hfcp->iov_cnt > 0)
			{
				hfc_fx_iov_update(hfcp->core, hfcp->iov_no, hfcp->iov_cnt, 1);
			}
		}
		if (hfcp->seg_cnt)
		{
			if( ioctl_mode !=1 ){
				scsi_dma_unmap(cmnd);
			}
		}
		
		if ( !HFC_HFCP_FX_CFLAG_TEST(CFLAG_HSDLDD_VALID, hfcp)){	/* FCLNX-GPL-FX-250 */
			cmnd->host_scribble = 0;
			
			if ((cmnd->device != NULL)
			 && (cmnd->device->type == TYPE_DISK))
			{
				if (HFC_HFCP_FX_CFLAG_TEST(CFLAG_TIMEOUT, hfcp)&&(!( hfcp->cmd_flags & CFLAG_RESET_ANY )))	/* FCLNX-GPL-FX-091 */
				{
					if(pp->scsi_to_retry > 0) {
						cmnd->allowed = pp->scsi_to_retry;
					}
					else if ( cmnd->allowed < pp->scsi_allowed ){
						cmnd->allowed = pp->scsi_allowed;
					}
				}
				else if ( cmnd->allowed < pp->scsi_allowed ){
					cmnd->allowed = pp->scsi_allowed;
				}
			}
		}	/* FCLNX-GPL-FX-250 */

		if ( hfcp->cmd_flags & CFLAG_RESET_ANY )	/* FCLNX-GPL-FX-014 */
		{
			if ( test_bit(CFLAG_TARGET_RESET, (ulong *)&hfcp->cmd_flags)
			  || test_bit(CFLAG_BUS_RESET, (ulong *)&hfcp->cmd_flags) )
				hfc_fx_watchdog_enter( pp, hfcp->core, hfcp->target, hfcp, 0, HFC_FX_TARGET_RST_TMR, 0, TRUE );
			else if( test_bit(CFLAG_ABORT, (ulong *)&hfcp->cmd_flags)
			  || test_bit(CFLAG_LUN_RESET, (ulong *)&hfcp->cmd_flags) )	/* FCLNX-GPL-FX-014 */
				hfc_fx_watchdog_enter( pp, hfcp->core, hfcp->target, hfcp, 0, HFC_FX_ABORT_TMR, 0, TRUE );
			else
				hfc_fx_watchdog_enter( pp, hfcp->core, hfcp->target, hfcp, 0, HFC_FX_CANCEL_SCSI_TMR, 0, TRUE );	/* FCLNX-GPL-FX-014 */
			
			if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_ABORT, hfcp))
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_ABORT \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp))
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_LUN_RESET \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_TARGET_RESET, hfcp))
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_TARGET_RESET \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_BUS_RESET, hfcp) )
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_BUS_RESET \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_LU_WITHOUT_DMA, hfcp))	/* FCLNX-GPL-FX-014 */
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_CSCSI_LU_WITHOUT_DMA \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_LU_WAIT_DMA, hfcp))	/* FCLNX-GPL-FX-014 */
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_CSCSI_LU_WAIT_DMA \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_TGT_WITHOUT_DMA, hfcp))	/* FCLNX-GPL-FX-014 */
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_CSCSI_TGT_WITHOUT_DMA \n");
			if(HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_TGT_WAIT_DMA, hfcp))	/* FCLNX-GPL-FX-014 */
				HFC_DBGPRT(KERN_ERR " hfcldd : hfc_fx_iodone - CFLAG_CSCSI_TGT_WAIT_DMA \n");
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) {	/* FCLNX-GPL-FX-261 */
				if ( !hfc_manage_info.npubp->hfc_fx_check_io_reset_complete(hfcp) ) {
					return;			/* continue reset process */
				}
			}	/* FCLNX-GPL-FX-261 */
			
			clear_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags);
			if (core)
				core->pkt_cnt--;
		}
		else 
		{
			hfc_fx_watchdog_enter( pp, hfcp->core, hfcp->target, hfcp, 0, HFC_FX_SCSI_CMD_TMR, 0, TRUE );
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) {
				if (hfc_manage_info.npubp->hfc_fx_mp_iodone(cmnd, pp, hfcp)) {
					return;
				}
				
				write_retries = hfc_manage_info.npubp->hfc_fx_write_retries(dev);
			}
			
			if (cmnd->result && !HFC_HFCP_FX_CFLAG_TEST(CFLAG_HSDLDD_VALID, hfcp)) {
				if(hfc_fx_cmd_retry_check(pp, cmnd, hfcp, write_retries))	/* FCLNX-GPL-FX-325 */
					return;
			}
			
			
			if (((cmnd->result >> 16) & 0xff) != DID_OK ) {
				if (core) {
					core->scsi_err_cnt++;
				}
				else
					pp->scsi_err_cnt++;
			}
			
			if (core) {
				data_size = scsi_bufflen(cmnd);
				
				if (data_size <= 512)			/* FCLNX-GPL-FX-244,272 */
					size = HFC_IO_CNT_SIZE_512B;
				else if (data_size <= 2048)
					size = HFC_IO_CNT_SIZE_2KB;
				else if (data_size <= 4096)
					size = HFC_IO_CNT_SIZE_4KB;
				else if (data_size <= 16384)
					size = HFC_IO_CNT_SIZE_16KB;
				else if (data_size <= 32768)	/* FCLNX-GPL-FX-244,272 */
					size = HFC_IO_CNT_SIZE_32KB;
				else
					size = HFC_IO_CNT_SIZE_OVER;
				
				if ( cmnd->sc_data_direction == SCSI_DATA_WRITE ) {
					core->wr_cnt++;
					core->wr_end_cnt[size]++;
					core->wr_data_size += data_size;
				}
				else {
					core->rd_cnt++;
					core->rd_end_cnt[size]++;
					core->rd_data_size += data_size;
				}
				core->scsi_end_cnt++;
			}
			else {
				pp->scsi_end_cnt++;
			}
			
			/* kernel 5.16+: use scsi_done() */
			scsi_done(cmnd);
			
			if (pp->pm_control == HFC_FX_PM_ON) {
				if (hfcp->pm_pkt_no != 0xffff) {
					clear_bit(CFLAG_VALID, (ulong *)&pp->pm_pkt_pool[hfcp->pm_pkt_no].cmd_flags);
				}
			}
			
			clear_bit(CFLAG_VALID, (ulong *)&hfcp->cmd_flags);
			if (core)
				core->pkt_cnt--;
		}
	}
	else
	{
		/* kernel 5.16+: use scsi_done() */
		scsi_done(cmnd);
		
		pp->scsi_err_cnt++;
		pp->scsi_end_cnt++;
	}
}

int hfc_fx_cmd_retry_check(
	struct port_info		*pp,
	struct scsi_cmnd		*cmnd,
	struct hfc_pkt_fx		*hfcp,
	uchar  					write_retries)
{	/* FCLNX-GPL-FX-325 */
	struct target_info_fx		*wktg=NULL;
	uchar warning_msg=0, retry_internal=0, retryout=0, cmd_type=0;
	uchar seq_no;
	uchar command;
	uchar					logdata[16];
	
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
		
		case INQUIRY               : cmd_type = 1; break;
		case 0xA0                  : cmd_type = 1; break;
		case READ_CAPACITY         : cmd_type = 1; break;
		
		default                    : cmd_type = 0;
	}
	if (cmd_type == 1)			/* CHECK COMMAND  */
	{
		if (((cmnd->result >> 16) & 0xff) == DID_TIME_OUT )
		{
			/* Need command retry */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
				cmnd->result &= 0xff00ffff;
				cmnd->result |= (DID_ERROR << 16);
			}
			else if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status) )
			{
				cmnd->result &= 0xff00ffff;
				cmnd->result |= (DID_NO_CONNECT << 16);
			}
			else if( (hfcp->target != NULL) && test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&hfcp->target->status))
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
	else if ((cmd_type == 3)		/* WRITE COMMAND */
		&& !test_bit(CFLAG_COMMAND_DEV, (ulong *)&hfcp->cmd_flags)
		&& !test_bit(CFLAG_NOT_TYPE_DISK, (ulong *)&hfcp->cmd_flags) )
	{
		warning_msg = FALSE;
		retry_internal = FALSE;
		retryout = FALSE;
		
		if (hfc_manage_info.lg_target_info == NULL)
			wktg = hfcp->target;
		else
			wktg = hfc_fx_hash_target_valid(pp, CMND_TARGET(cmnd));
		
		if (write_retries) {
			if (write_retries == 0x80) {							/* retry  */
				cmnd->retries = 0;
				cmnd->allowed = 10;
				hfcp->fail = 0;
			}
			else if (write_retries == 0xff);						/* retry(default) */
			else 
			{
				int rcnt = write_retries & 0x7f;
				
				if (cmnd->allowed != rcnt) {
					if (cmnd->allowed != pp->scsi_allowed) {
						seq_no = 0x01;
						memset(logdata,0,16);
						memcpy(&logdata[0],(uchar *)&seq_no,1);
						hfc_fx_errlog(pp, hfcp->core, NULL, hfcp, HFC_ERRLOG_TYPE_IODONE_WARN1, ERRID_HFCP_EVNT3, 0x93, logdata, 16) ;
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
				warning_msg = TRUE;
				if (write_retries) {
					retry_internal = TRUE;
					
					if ( hfcp->fail >= cmnd->allowed)
						retryout = TRUE;
				}
				else {
					if (!hfc_manage_info.hfcldd_mp_mod) {
						if ( cmnd->retries >= cmnd->allowed )
							retryout = TRUE;
					}
					else {
						retryout = TRUE;
					}
				}
				break;
				
			/* use retries */
			case DID_RESET      :	/* Reset by somebody.                      */
			case DID_BUS_BUSY   :	/* BUS remain busy through time out period */
			case DID_PARITY     :	/* Parity error                            */
			case DID_ERROR      :	/* Internal error                          */
			case DID_SOFT_ERROR :	/* The low level driver just wish a retry  */
				warning_msg = TRUE;
				if (write_retries) {
					retry_internal = TRUE;
					
					if ( hfcp->fail >= cmnd->allowed)
						retryout = TRUE;
				}
				else {
					if (!hfc_manage_info.hfcldd_mp_mod ) {
						if ( cmnd->retries >= cmnd->allowed )
							retryout = TRUE;
					}
					else {
						if ( hfcp->fail >= cmnd->allowed )
							retryout = TRUE;
					}
				}
				break;
				
			case DID_OK         :	/* Normal end                              */
				/* kernel 5.14+: CHECK_CONDITION removed */
				if ((cmnd->result & 0xfe) != SAM_STAT_CHECK_CONDITION)
					break;
				
				if ( ((cmnd->sense_buffer[2] & 0xf) == NOT_READY)
				  || ((cmnd->sense_buffer[2] & 0xf) == MEDIUM_ERROR)
				  || ((cmnd->sense_buffer[2] & 0xf) == ABORTED_COMMAND) )
				{
					warning_msg = TRUE;
					if (write_retries) {
						retry_internal = TRUE;
						
						if ( hfcp->fail >= cmnd->allowed)
							retryout = TRUE;
					}
					else {
						if (!hfc_manage_info.hfcldd_mp_mod ) {
							if ( cmnd->retries >= cmnd->allowed )
								retryout = TRUE;
						}
						else {
							if ( hfcp->fail >= cmnd->allowed )
								retryout = TRUE;
						}
					}
				}
				else if ((cmnd->sense_buffer[2] & 0xf) == HARDWARE_ERROR)
				{
					warning_msg = TRUE;
					if (write_retries) {
						retry_internal = TRUE;
						
						if ( hfcp->fail >= cmnd->allowed)
							retryout = TRUE;
					}
					else {
						if (!hfc_manage_info.hfcldd_mp_mod ) {
							if ( cmnd->retries >= cmnd->allowed )
								retryout = TRUE;
						}
						else {
							retryout = TRUE;
						}
					}
				}
				break;
				
			default :
				/* DID_ABORT       */	/* Failed Abort request                    */
				/* DID_BAD_TARGET  */	/* No target						       */
				/* DID_BAD_INTR    */	/* Got an interrupt we did not expect	   */
				/* DID_PASSTHROUGH */	/* Force command past mid-layer            */
				break;
		}
	}
	if ( ((cmnd->result >> 16) & 0xff) == DID_TIME_OUT ) {
		
		if (!pp->scsi_time_out) {
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){ /* FCLNX-GPL-FX-472 */
				cmnd->result &= 0xff00ffff;
				cmnd->result |= (DID_ERROR << 16);
			}
			else if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status) )
			{
				cmnd->result &= 0xff00ffff;
				cmnd->result |= (DID_NO_CONNECT << 16);
			}
			else if( (hfcp->target != NULL) && test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&hfcp->target->status))
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
	if (cmd_type == 3)		/* WRITE COMMAND */
	{
		if (warning_msg == TRUE)
		{
			if ( (write_retries)
			  || (pp->wmsg && (retryout == TRUE)) ) {
				if (write_retries == 0x80) {
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
				hfc_fx_errlog(pp, hfcp->core, NULL, hfcp, HFC_ERRLOG_TYPE_IODONE_WARN1, ERRID_HFCP_EVNT3, 0x93, logdata, 16) ;	
			}

			if (!hfc_manage_info.hfcldd_mp_mod) {
				retry_internal = FALSE;	
				retryout = FALSE;
			}

			if (write_retries)
			{
				if (retryout == TRUE) {
					HFC_ERRPRT(" DONE LAST RETRY. GIVE UP!\n");

					if (write_retries > 0x80)
					{
						panic("DONE LAST RETRY");
					}
				}
				else if (retry_internal == TRUE)			/* Enqueue internal retry */
				{
					hfcp->cmd_forw = NULL;
					hfcp->cmd_prev = NULL;
					
					if (pp->retry_hfcp_top == NULL) {
						pp->retry_hfcp_top = hfcp;
						pp->retry_hfcp_end = hfcp;
					}
					else {
						hfcp->cmd_prev = pp->retry_hfcp_end;
						pp->retry_hfcp_end->cmd_forw = hfcp;
						pp->retry_hfcp_end = hfcp;
					}
					pp->retry_hfcp_count++;
					
					hfc_fx_mp_watchdog_enter(pp, hfcp->core, hfcp->target, hfcp, hfcp->dev, hfcp->lun_id, HFC_FX_PATH_RETRY_TMR, 0, TRUE);	/* FCLNX-GPL-FX-325 */
					hfc_fx_mp_watchdog_enter(pp, hfcp->core, hfcp->target, hfcp, hfcp->dev, hfcp->lun_id, HFC_FX_PATH_RETRY_TMR, 0, FALSE);	/* FCLNX-GPL-FX-325 */
					return(1);	/* FCLNX-GPL-FX-325 */
				}
				
			}
		}
	}
	return(0);	/* FCLNX-GPL-FX-325 */
}

/*
 * Function:    hfc_fx_stra_trace
 *
 * Purpose:     Collection of trace
 *
 * Arguments:   
 *  id         - 
 *  sub_id     - 
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_stra_trace(
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	unsigned long long	 	etc1,
	unsigned long long		etc2,
	unsigned long long		etc3)
{
	uchar				trc_wk[128] ;
	uchar				port_trace=0;
	struct scsi_cmnd    *cmnd = NULL;
	
	memset(trc_wk,0,128) ;
	if(hfcp != NULL ) cmnd = hfcp->cmd_pkt;
	
	switch (id)
	{
		/*-- trace format 1 --*/
		case HFC_FX_TRC_STRATEGY :
		case HFC_FX_TRC_ABORT :
		case HFC_FX_TRC_RESET :
		case HFC_FX_TRC_BUS_RESET :
		case HFC_FX_TRC_ISSUE_TMGM :
		case HFC_FX_TRC_IODONE :
		case HFC_FX_TRC_TGT_RESET :
		case HFC_FX_TRC_LUN_RESET :
			hfc_fx_stra_trc1(trc_wk,id,sub_id,pp,rp,core,target,hfcp,cmnd,etc1,etc2,etc3);
			port_trace = 1;
			break;

		/*-- trace format 2 --*/
		case HFC_FX_TRC_START :
			hfc_fx_stra_trc2(trc_wk,id,sub_id,pp,rp,core,target,hfcp,cmnd,etc1,etc2,etc3);
			break;

		/*-- trace format 3 --*/
		case HFC_FX_TRC_CAN_XOB :
		case HFC_FX_TRC_CAN_WE :
		case HFC_FX_TRC_CAN_WX :
			hfc_fx_stra_trc3(trc_wk,id,sub_id,pp,rp,core,target,hfcp,cmnd,etc1,etc2,etc3);
			port_trace = 1;
			break;

		/*-- trace format 4 --*/
		case HFC_FX_TRC_RES_CHK :
		case HFC_FX_TRC_IOVUP :
		case HFC_FX_TRC_DMA_MAP :
			hfc_fx_stra_trc4(trc_wk,id,sub_id,pp,rp,core,target,hfcp,cmnd,etc1,etc2,etc3);
			break;
	}
	
	/* Trace output */
	if (port_trace)
		hfc_fx_trace(pp, NULL, id, &trc_wk[1], 0);
	else
		hfc_fx_trace(pp, core, id, &trc_wk[1], 0);
}


/*
 * Function:    hfc_fx_stra_trc1
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_stra_trc1(
	uchar					*trc_wk,
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct scsi_cmnd		*cmnd,
	unsigned long long 		etc1,
	unsigned long long		etc2,
	unsigned long long		etc3)
{
	struct scsi_device		*sdev=NULL;
	struct request_queue	*rq=NULL;
	uchar					buf[4];
	
	struct stra_fx_trc1 *trc1 = (struct stra_fx_trc1 *)trc_wk ;
	
	trc1->id = id ;
	trc1->sub_id = sub_id ;
	if (pp != NULL)
	{
		HFC_2L_TO_2B(trc1->seq_no, pp->seq_no);
		trc1->a_status = (uchar)pp->status;
		HFC_4L_TO_4B(trc1->a_status_d1, pp->status_detail1);
		HFC_4L_TO_4B(trc1->a_status_d2, pp->status_detail2);
		HFC_4L_TO_4B(buf, pp->scsi_id);
		HFC_MEMCPY(&trc1->a_scsi_id[0], &buf[1], 3);
	}
	if (rp != NULL)
	{
		trc1->r_rid = rp->rid;
	}
	if (core != NULL)
	{
		trc1->c_core_no = core->core_no;
		
		if (target != NULL) {
			HFC_2L_TO_2B(trc1->ct_wx_que_cnt, target->core_queue[core->core_no].wx_que_cnt);
			HFC_2L_TO_2B(trc1->ct_we_que_cnt, target->core_queue[core->core_no].we_que_cnt);
		}
		
		HFC_8L_TO_8B(trc1->c_scsi_exec_cnt, core->scsi_exec_cnt);
		HFC_8L_TO_8B(trc1->c_scsi_end_cnt, core->scsi_end_cnt);
		
		if (hfcp != NULL) {
			HFC_LP_TO_BP(trc1->hfcp, hfcp);
		}
	}
	if (target != NULL) {
		trc1->t_flag = (uchar)target->flags;
		HFC_4L_TO_4B(trc1->t_status, target->status);
		trc1->t_id = (uchar)target->target_id;
		HFC_4L_TO_4B(buf, target->scsi_id);
		HFC_MEMCPY(&trc1->t_scsi_id[0], &buf[1], 3);
		trc1->t_pseq = (uchar)target->pseq;
	}
	if( hfcp != NULL)
	{
		trc1->h_cmd_flags = hfcp->cmd_flags;
		HFC_4L_TO_4B(trc1->h_adap_status, hfcp->adap_status);
		HFC_4L_TO_4B(trc1->h_iov_no, hfcp->iov_no);
		HFC_4L_TO_4B(trc1->h_iov_cnt, hfcp->iov_cnt);
		trc1->h_cmd_xob = hfcp->cmd_xob;
		trc1->h_rid = hfcp->rid;
		trc1->timeout = (uchar)hfcp->timeout;
	}
	if( cmnd != NULL )
	{
		trc1->cmnd0 = (uchar)cmnd->cmnd[0];
		HFC_2L_TO_2B(trc1->lun_id, CMND_LUN(cmnd));
		HFC_4L_TO_4B(trc1->retries, cmnd->retries);
		HFC_4L_TO_4B(trc1->allowed, cmnd->allowed);
		HFC_MEMCPY(&trc1->cmnd[0], cmnd->cmnd, 16);
		/* kernel 5.x+: sdb.resid → resid_len; serial_number removed */
		HFC_4L_TO_4B(trc1->resid, cmnd->resid_len);
		trc1->serial_number = 0;	/* 0 is endian-neutral; direct assign replaces HFC_4L_TO_4B(,0) */
		HFC_4L_TO_4B(trc1->result, cmnd->result);
		sdev = cmnd->device;
		if( sdev != NULL ){
			rq = sdev->request_queue;
			if( rq != NULL ){
				trc1->timeout = (rq->rq_timeout/HZ);
			}
		}
	}
}


/*
 * Function:    hfc_fx_stra_trc2
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_stra_trc2(
	uchar					*trc_wk,
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct scsi_cmnd		*cmnd,
	unsigned long long 		etc1,
	unsigned long long		etc2,
	unsigned long long		etc3)
{
	struct stra_fx_trc2 *trc2 = (struct stra_fx_trc2 *)trc_wk ;
	
	trc2->id = id ;
	trc2->sub_id = sub_id ;
	
	trc2->c_xob_w_exec_cnt = (uchar)etc1;	/* FCLNX-GPL-FX-061 */
	trc2->c_initial_xob_no = (uchar)etc2;	/* FCLNX-GPL-FX-061 */
	
	if (pp != NULL)
	{
		HFC_2L_TO_2B(trc2->seq_no, pp->seq_no);
	}
	if (rp != NULL)
	{
		trc2->r_rid = rp->rid;
	}
	if (core != NULL)
	{
		trc2->c_core_no = core->core_no;
		trc2->c_next_dstart_cnt = core->next_dstart_cnt;
			
		if (target != NULL) {
			HFC_2L_TO_2B(trc2->ct_wx_que_cnt, target->core_queue[core->core_no].wx_que_cnt);
			HFC_2L_TO_2B(trc2->ct_we_que_cnt, target->core_queue[core->core_no].we_que_cnt);
		}
		
		HFC_4L_TO_4B(trc2->c_iov_no, core->iov_no);
		HFC_4L_TO_4B(trc2->c_frame_inp, core->frame_inp);
		HFC_4L_TO_4B(trc2->c_frame_start_xob, core->frame_start_xob);
		HFC_2L_TO_2B(trc2->c_drv_next_xob, core->drv_next_xob);
		HFC_2L_TO_2B(trc2->c_drv_next_xrb, core->drv_next_xrb);
		HFC_8L_TO_8B(trc2->c_scsi_exec_cnt, core->scsi_exec_cnt);
		HFC_8L_TO_8B(trc2->c_scsi_end_cnt, core->scsi_end_cnt);
		
		if (hfcp != NULL) {
			HFC_LP_TO_BP(trc2->hfcp, hfcp);
		}
	}
	if (target != NULL) {
		trc2->t_id = (uchar)target->target_id;
		trc2->t_pseq = (uchar)target->pseq;
	}
	if( hfcp != NULL)
	{
		trc2->h_cmd_flags = hfcp->cmd_flags;
		HFC_4L_TO_4B(trc2->h_adap_status, hfcp->adap_status);
		HFC_4L_TO_4B(trc2->h_iov_no, hfcp->iov_no);
		HFC_4L_TO_4B(trc2->h_iov_cnt, hfcp->iov_cnt);
		HFC_4L_TO_4B(trc2->h_data_size, hfcp->data_size);
		trc2->h_cmd_xob = hfcp->cmd_xob;
	}
	if( cmnd != NULL )
	{
		HFC_4L_TO_4B(trc2->retries, cmnd->retries);
		HFC_4L_TO_4B(trc2->allowed, cmnd->allowed);
		HFC_MEMCPY(&trc2->cmnd[0], cmnd->cmnd, 16);
		/* kernel 5.x+: sdb.resid → resid_len; serial_number removed */
		HFC_4L_TO_4B(trc2->resid, cmnd->resid_len);
		trc2->serial_number = 0;	/* 0 is endian-neutral */
		HFC_4L_TO_4B(trc2->result, cmnd->result);
		trc2->scsi_cmnd = (uchar)cmnd->cmnd[0];
		HFC_2L_TO_2B(trc2->lun_id, CMND_LUN(cmnd));	/* FCLNX-GPL-FX-202 */
	}
}


/*
 * Function:    hfc_fx_stra_trc3
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_stra_trc3(
	uchar					*trc_wk,
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct scsi_cmnd		*cmnd,
	unsigned long long 		etc1,
	unsigned long long		etc2,
	unsigned long long		etc3)
{
	struct scsi_device		*sdev=NULL;
	struct request_queue	*rq=NULL;
	uchar					buf[4];
	
	struct stra_fx_trc3 *trc3 = (struct stra_fx_trc3 *)trc_wk ;
	
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
		
		if (target != NULL) {
			HFC_2L_TO_2B(trc3->ct_wx_que_cnt, target->core_queue[core->core_no].wx_que_cnt);
			HFC_2L_TO_2B(trc3->ct_we_que_cnt, target->core_queue[core->core_no].we_que_cnt);
		}
		
		HFC_8L_TO_8B(trc3->c_scsi_exec_cnt, core->scsi_exec_cnt);
		HFC_8L_TO_8B(trc3->c_scsi_end_cnt, core->scsi_end_cnt);
		
		if (hfcp != NULL) {
			HFC_LP_TO_BP(trc3->hfcp, hfcp);
		}
	}
	if (target != NULL) {
		trc3->t_flag = (uchar)target->flags;
		HFC_4L_TO_4B(trc3->t_status, target->status);
		trc3->t_id = (uchar)target->target_id;
		HFC_4L_TO_4B(buf, target->scsi_id);
		HFC_MEMCPY(&trc3->t_scsi_id[0], &buf[1], 3);
		trc3->t_pseq = (uchar)target->pseq;
	}
	if( hfcp != NULL)
	{
		trc3->h_cmd_flags = hfcp->cmd_flags;
		HFC_4L_TO_4B(trc3->h_adap_status, hfcp->adap_status);
		HFC_4L_TO_4B(trc3->h_iov_no, hfcp->iov_no);
		HFC_4L_TO_4B(trc3->h_iov_cnt, hfcp->iov_cnt);
		trc3->h_cmd_xob = hfcp->cmd_xob;
	}
	if( cmnd != NULL )
	{
		trc3->cmnd0 = (uchar)cmnd->cmnd[0];
		HFC_2L_TO_2B(trc3->lun_id, CMND_LUN(cmnd));
		HFC_4L_TO_4B(trc3->retries, cmnd->retries);
		HFC_4L_TO_4B(trc3->allowed, cmnd->allowed);
		HFC_MEMCPY(&trc3->cmnd[0], cmnd->cmnd, 16);
		/* kernel 5.x+: sdb.resid → resid_len; serial_number removed */
		HFC_4L_TO_4B(trc3->resid, cmnd->resid_len);
		trc3->serial_number = 0;	/* 0 is endian-neutral */
		HFC_4L_TO_4B(trc3->result, cmnd->result);
		sdev = cmnd->device;
		if( sdev != NULL ){
			rq = sdev->request_queue;
			if( rq != NULL ){
				trc3->timeout = (rq->rq_timeout/HZ);
			}
		}
	}
}


/*
 * Function:    hfc_fx_stra_trc4
 *
 * Purpose:     
 *
 * Arguments:   
 *  trc_wk     - 
 *  id         - 
 *  sub_id     - 
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx 
 *  hfcp       - Pointer to hfc_pkt_fx 
 *  etc1       - 
 *  etc2       - 
 *  etc3       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_stra_trc4(
	uchar					*trc_wk,
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct scsi_cmnd		*cmnd,
	unsigned long long 		etc1,
	unsigned long long		etc2,
	unsigned long long		etc3)
{
	struct stra_fx_trc4 *trc4 = (struct stra_fx_trc4 *)trc_wk ;
	
	trc4->id = id ;
	trc4->sub_id = sub_id ;
	if (pp != NULL)
	{
		HFC_2L_TO_2B(trc4->seq_no, pp->seq_no);
	}
	if (rp != NULL)
	{
		trc4->r_rid = rp->rid;
	}
	if (core != NULL)
	{
		trc4->c_core_no = core->core_no;
//		trc4->c_xob_exec_cnt = core->xob_exec_cnt;
//		trc4->c_xob_w_exec_cnt = core->xob_wait_exec_cnt;
		trc4->c_next_dstart_cnt = core->next_dstart_cnt;
			
		if (target != NULL) {
			HFC_2L_TO_2B(trc4->ct_wx_que_cnt, target->core_queue[core->core_no].wx_que_cnt);
			HFC_2L_TO_2B(trc4->ct_we_que_cnt, target->core_queue[core->core_no].we_que_cnt);
		}
		
		HFC_4L_TO_4B(trc4->c_iov_no, core->iov_no);
		HFC_4L_TO_4B(trc4->c_frame_inp, core->frame_inp);
		HFC_4L_TO_4B(trc4->c_frame_start_xob, core->frame_start_xob);
		HFC_2L_TO_2B(trc4->c_drv_next_xob, core->drv_next_xob);
		HFC_2L_TO_2B(trc4->c_drv_next_xrb, core->drv_next_xrb);
		HFC_8L_TO_8B(trc4->c_scsi_exec_cnt, core->scsi_exec_cnt);
		HFC_8L_TO_8B(trc4->c_scsi_end_cnt, core->scsi_end_cnt);
		
		if (hfcp != NULL) {
			HFC_LP_TO_BP(trc4->hfcp, hfcp);
		}
	}
	if (target != NULL) {
		trc4->t_id = (uchar)target->target_id;
		trc4->t_pseq = (uchar)target->pseq;
	}
	if( hfcp != NULL)
	{
		trc4->h_cmd_flags = hfcp->cmd_flags;
		HFC_4L_TO_4B(trc4->h_adap_status, hfcp->adap_status);
		HFC_4L_TO_4B(trc4->h_iov_no, hfcp->iov_no);
		HFC_4L_TO_4B(trc4->h_iov_cnt, hfcp->iov_cnt);
		HFC_4L_TO_4B(trc4->h_data_size, hfcp->data_size);
		trc4->h_cmd_xob = hfcp->cmd_xob;
	}
	if( cmnd != NULL )
	{
		HFC_4L_TO_4B(trc4->retries, cmnd->retries);
		HFC_4L_TO_4B(trc4->allowed, cmnd->allowed);
		HFC_MEMCPY(&trc4->cmnd[0], cmnd->cmnd, 16);
		/* kernel 5.x+: sdb.resid → resid_len; serial_number removed */
		HFC_4L_TO_4B(trc4->resid, cmnd->resid_len);
		trc4->serial_number = 0;	/* 0 is endian-neutral */
		HFC_4L_TO_4B(trc4->result, cmnd->result);
		HFC_2L_TO_2B(trc4->lun_id, CMND_LUN(cmnd));	/* FCLNX-GPL-FX-202 */
	}
}
