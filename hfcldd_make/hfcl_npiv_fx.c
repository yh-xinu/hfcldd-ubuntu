/*
 * hfcl_npiv.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 *
 */

char npiv_rcsid[] = "$Id: hfcl_npiv_fx.c,v 1.1.2.17.2.3.2.3.2.1 2015/04/21 11:49:02 toyo Exp $";

#include <linux/kthread.h>

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
#include "hfcl_npiv_fx.h"

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


#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern struct scsi_transport_template *hfc_vport_fc_attach_transport;
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#endif /* SYSFS_SUPPORT */

extern uint	raslog_install;

int HFC_FX_NPIV_ENABLE(struct port_info *pp)
{
	if (pp->pport->npiv_mode & HFC_NPIV_ENABLE)
		return 1;
	else
		return 0;
}

int HFC_FX_NPIV_EXT_MODE(struct port_info *pp)
{
	if (pp->pport->npiv_mode & HFC_NPIV_EXT_MODE)
		return 1;
	else
		return 0;
}

int HFC_FX_NPIV_SHAREABLE(struct port_info *pp)
{
	if (pp->pport->npiv_mode & HFC_NPIV_SHAREABLE)
		return 1;
	else
		return 0;
}

int HFC_FX_VPORT_EXIST(struct port_info *pp)
{
	if (pp->pport->vport_num)
		return 1;
	else
		return 0;
}

int HFC_FX_EXT_VPORT_EXIST(struct region_info *rp)
{
	if (rp == NULL)
		return 0;
	
	if (rp->port_num > 1)
		return 1;
	else
		return 0;
}

int HFC_FX_MIN_PORT_IN_REGION(struct port_info *pp)
{
	if (pp->min_port_in_region)
		return 1;
	else
		return 0;
}

int HFC_FX_PHYSICAL_PORT(struct port_info *pp)
{
	if (pp->npiv_mode & HFC_NPIV_VPORT)
		return 0;
	else
		return 1;
}

int HFC_FX_VIRTUAL_PORT(struct port_info *pp)
{
	if (pp->npiv_mode & HFC_NPIV_VPORT)
		return 1;
	else
		return 0;
}

int HFC_FX_MQ_VIRTUAL_PORT(struct port_info *pp)
{
	if (pp->mq_mode & HFC_MQ_VPORT)
		return 1;
	else
		return 0;
}

int HFC_FX_VPORT_ENABLE(struct port_info *pp)
{
	if (HFC_FX_PHYSICAL_PORT(pp))
		return 0;
	
	if (test_bit(HFC_FX_VPORT_DISABLE, (ulong *)&pp->pport->vport_ptr[pp->rid].status))
		return 0;
	else
		return 1;
}

int HFC_FX_MQ_ENABLE(struct port_info *pp)
{
	if (pp->pport->mq_mode & HFC_MQ_ENABLE)
		return 1;
	else
		return 0;
}

int HFC_FX_MQ_VALID(struct port_info *pp)
{
	if (pp->pport->mq_mode & HFC_MQ_VALID)
		return 1;
	else
		return 0;
}

struct port_info *HFC_FX_GET_MIN_PORT_IN_REGION(struct port_info *pp)
{
	struct port_info *wkpp = NULL;
	int i;
	
	for (i=0; i<=pp->pport->max_vport_count; i++)
	{
		if ((wkpp = pp->pport->vport_ptr[i].vport_arg) == NULL)
			continue;
		
		if (pp->rid != wkpp->rid)
			continue;
		
		if (HFC_FX_MIN_PORT_IN_REGION(wkpp))
			return (wkpp);
	}
	
	return (wkpp);
}

void hfc_fx_npiv_config_check(struct port_info *pp, struct core_info *core)
{
	uchar	wk1;
	uchar	wk2;
	
	HFC_ENTRY("hfc_fx_npiv_config_check");
	
	wk1 = hfc_fx_read_val( core->fw_init_p->fw_iocinfo.configure_flag);
	wk2 = hfc_fx_read_val( core->fw_init_p->fw_iocinfo.connect_type);
	
	if ( !(wk1 & HFC_FX_FABRIC_VALID) ) 
	{	/* Fabric not exist */
		HFC_DBGPRT("hfcldd : hfc_fx_npiv_config_check - Fabric not exist\n");
		if (HFC_FX_PHYSICAL_PORT(pp)) {
			pp->npiv_mode &= ~HFC_NPIV_SHAREABLE;
		}
		else {
			hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_NO_FABRIC_SUPP);
		}
	}
	else if ( wk2 == HFC_FX_AL)
	{	/* Multi ALPA */
		HFC_DBGPRT("hfcldd : hfc_fx_npiv_config_check - Multi ALPA\n");
		if (HFC_FX_PHYSICAL_PORT(pp)) {
			pp->npiv_mode &= ~HFC_NPIV_SHAREABLE;
		}
		else {
			hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_NO_FABRIC_SUPP);
		}
	}
	else
	{	/* Fabric exist & PtoP */
		HFC_DBGPRT("hfcldd : hfc_fx_npiv_config_check - Fabric exist & PtoP\n");
		
		if ( pp->flogi_rsp_param & FLOGI_PARAM_NPIV )
		{	/* NPIV valid */
			HFC_DBGPRT("hfcldd : hfc_fx_npiv_config_check - NPIV valid\n");
			if (HFC_FX_PHYSICAL_PORT(pp)) {
				if (HFC_FX_MQ_ENABLE(pp)) {
					pp->npiv_mode &= ~HFC_NPIV_SHAREABLE;
				}
				else {
					pp->npiv_mode |= HFC_NPIV_SHAREABLE;
				}
			}
			else {
				hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_ACTIVE);
			}
		}
		else
		{	/* NPIV invalid */
			HFC_DBGPRT("hfcldd : hfc_fx_npiv_config_check - NPIV invalid\n");
			if (HFC_FX_PHYSICAL_PORT(pp)) {
				pp->npiv_mode &= ~HFC_NPIV_SHAREABLE;
			}
			else {
				hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_ACTIVE);
			}
		}
	}
	
	HFC_EXIT("hfc_fx_npiv_config_check");
}

int hfc_fx_rid_register(struct port_info *ppp, struct port_info *vpp)
{
	uint	i;
	uchar	hit = 0;
	
	HFC_ENTRY("hfc_rid_register");
	
	if (vpp == NULL) {
		ppp->vport_ptr[0].vport_arg = ppp;
		ppp->min_port_in_region = 1;
		return 0;
	}
	
	for (i=1; i<=ppp->max_vport_count; i++)
	{
		if (ppp->vport_ptr[i].vport_arg == NULL) {
			ppp->vport_num++;
			ppp->vport_ptr[i].vport_arg = vpp;
			vpp->pport = ppp;
			
			if (HFC_FX_MQ_ENABLE(ppp)) {
				vpp->npiv_mode = ppp->npiv_mode;
				vpp->npiv_mode |= HFC_NPIV_VPORT;
				vpp->mq_mode |= HFC_MQ_VPORT;
				vpp->rid = i%32;
				vpp->sub_rid = i/32;
				vpp->vport_id = i;
				vpp->min_port_in_region = 1;
				HFC_DBGPRT("hfcldd : hfc_fx_rid_register - discovered new rid, rid=%02x, sub_rid=%02x, vport_id=%02x\n",
					vpp->rid, vpp->sub_rid, vpp->vport_id);
				
				hit = 1;
				break;
			}
			else if (HFC_FX_NPIV_EXT_MODE(ppp)) {
				vpp->npiv_mode = ppp->npiv_mode;
				vpp->npiv_mode |= HFC_NPIV_VPORT;
				vpp->rid = 0;
				vpp->sub_rid = i;
				vpp->vport_id = i;
				HFC_DBGPRT("hfcldd : hfc_fx_rid_register - discovered new rid, rid=%02x, sub_rid=%02x, vport_id=%02x\n",
					vpp->rid, vpp->sub_rid, vpp->vport_id);
				
				hit = 1;
				break;
			}
			else if (HFC_FX_NPIV_ENABLE(ppp)) {
				vpp->npiv_mode = ppp->npiv_mode;
				vpp->npiv_mode |= HFC_NPIV_VPORT;
				vpp->rid = i%32;
				vpp->sub_rid = i/32;
				vpp->vport_id = i;
				HFC_DBGPRT("hfcldd : hfc_fx_rid_register - discovered new rid, rid=%02x, sub_rid=%02x, vport_id=%02x\n",
					vpp->rid, vpp->sub_rid, vpp->vport_id);
				
				hit = 1;
				break;
			}
		}
	}
	
	if (hit) {
		hfc_fx_serach_minimum_port_in_region(ppp, vpp->rid);
		return 0;
	}
	else
		return 1;
}

int hfc_fx_rid_unregister(struct port_info *ppp, struct port_info *vpp)
{
	HFC_ENTRY("hfc_rid_unregister");
	
	if (vpp == NULL) {
		ppp->vport_ptr[0].vport_arg = NULL;
		return 0;
	}
	
	if (vpp->rid > ppp->max_vport_count) {
		return 1;
	}
	
	HFC_DBGPRT("hfc_fx_rid_unregister() - rid=0x%02x\n", vpp->rid);
	
	if (ppp->npiv_mode & HFC_NPIV_EXT_MODE) {
		ppp->vport_ptr[vpp->sub_rid].vport_arg = NULL;
	}
	else {
		ppp->vport_ptr[vpp->sub_rid*HFC_FX_MAX_RID_COUNT+vpp->rid].vport_arg = NULL;
	}
	ppp->vport_num--;
	hfc_fx_serach_minimum_port_in_region(ppp, vpp->rid);
	
	HFC_EXIT("hfc_rid_unregister");
	
	return 0;
}

void hfc_fx_serach_minimum_port_in_region(struct port_info *ppp, int rid)
{
	uint	i;
	struct port_info 	*wkpp;
	
	HFC_ENTRY("hfc_fx_serach_minimum_port_in_region");
	
	if (HFC_FX_MQ_ENABLE(ppp))
		return;
	
	if (HFC_FX_NPIV_EXT_MODE(ppp))
		return;
	
	for (i=0; i<=ppp->max_vport_count; i++) {
		if (ppp->vport_ptr[i].vport_arg == NULL) {
			continue;
		}
		else {
			wkpp = ppp->vport_ptr[i].vport_arg;
			if (rid != wkpp->rid)
				continue;
			wkpp->min_port_in_region = 0;
		}
	}
	
	for (i=0; i<HFC_FX_MAX_SUB_RID_COUNT; i++)
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_serach_minimum_port_in_region - vport_id=%d\n",
					i*HFC_FX_MAX_RID_COUNT+rid);
		
		if (i*HFC_FX_MAX_RID_COUNT+rid > ppp->max_vport_count)
			break;
		
		if (ppp->vport_ptr[i*HFC_FX_MAX_RID_COUNT+rid].vport_arg == NULL) {
			continue;
		}
		else {
			HFC_DBGPRT( "hfcldd : hfc_fx_serach_minimum_port_in_region - min vport_id=%d\n",
					i*HFC_FX_MAX_RID_COUNT+rid);
			wkpp = ppp->vport_ptr[i*HFC_FX_MAX_RID_COUNT+rid].vport_arg;
			wkpp->min_port_in_region = 1;
			break;
		}
	}
	
	HFC_EXIT("hfc_fx_serach_minimum_port_in_region");
}

void hfc_fx_vport_initialize(struct port_info *pp)
{
	unsigned long		flags = 0;
	struct region_info	*rp;
	
	rp = pp->region_arg[pp->rid];
	if ( rp == NULL ) { /* region_info null */
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_initialize - region_info==NULL.\n");
		return;
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	pp->initialize = 1;
	atomic_set(&pp->int_a_poll, 0);
	hfc_fx_issue_flogi(pp);
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	hfc_fx_sleep_on(&pp->init_event, &pp->int_a_poll );
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	atomic_set(&pp->int_a_poll, 0);
	pp->initialize = 0;	
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_param_copy(struct port_info *pp, struct port_info *vpp)
{
	int i;
	
	/* driver parameter */
	vpp->limit_log				= pp->limit_log;
	vpp->filter_target			= pp->filter_target;
	vpp->core_control			= pp->core_control;
	vpp->cc_cnt					= pp->cc_cnt;
	vpp->cc_size				= pp->cc_size;
	vpp->cc_core				= pp->cc_core;
	vpp->link_reset				= pp->link_reset;
	vpp->linkup_tmo				= pp->linkup_tmo;
	vpp->scsi_reset_delay		= pp->scsi_reset_delay;
	vpp->max_mck_cnt			= pp->max_mck_cnt;
	vpp->target_reset_tmo		= pp->target_reset_tmo;
	vpp->abort_tmo				= pp->abort_tmo;
	vpp->scsi_allowed			= pp->scsi_allowed;
	vpp->ld_err_limit_s			= pp->ld_err_limit_s;
	vpp->if_err_limit			= pp->if_err_limit;
	vpp->to_err_limit			= pp->to_err_limit;
	vpp->rt_err_enable			= pp->rt_err_enable;
	vpp->hba_isolation			= pp->hba_isolation;
	vpp->pm_control				= pp->pm_control;
	vpp->total_abort_to			= pp->total_abort_to;	/* FCLNX-GPL-FX-014 */
	vpp->total_tgtrst_to		= pp->total_tgtrst_to;	/* FCLNX-GPL-FX-014 */
	vpp->debug_func				= pp->debug_func;
	
	/* flash parameter */
	vpp->login_wait				= pp->login_wait;
	vpp->mck_rcv_tmo			= pp->mck_rcv_tmo;
	vpp->core_deg_mode			= pp->core_deg_mode;
	vpp->rft_id_skip			= pp->rft_id_skip;
	vpp->link_initialize_tmo	= pp->link_initialize_tmo;
	for(i=0 ; i<HFC_MBTIME_MAX ; i++) {
		vpp->mb_timer[i].tout	= pp->mb_timer[i].tout;
		vpp->mb_timer[i].retry	= pp->mb_timer[i].retry;
		vpp->mb_timer[i].intvl	= pp->mb_timer[i].intvl;
		vpp->mb_timer[i].delay	= pp->mb_timer[i].delay;
	}
	vpp->wait_plogi_recv_tmo	= pp->wait_plogi_recv_tmo;
	vpp->mailbox_force_retry	= pp->mailbox_force_retry;
	vpp->vf_enable				= pp->vf_enable;
	vpp->vf_mode_tagging		= pp->vf_mode_tagging;
	vpp->security_enable		= pp->security_enable;
	vpp->max_io					= pp->max_io;
	vpp->abort_t_restrain		= pp->abort_t_restrain;	/* FCLNX-GPL-FX-175 */
	vpp->tgtrst_restrain 		= pp->tgtrst_restrain;	/* FCLNX-GPL-FX-175 */
	vpp->lun_reset_delay 		= pp->lun_reset_delay;	/* FCLNX-GPL-FX-175 */
	vpp->dev_loss_tmo			= pp->dev_loss_tmo;
	
	vpp->login_seq_retry_cnt	= pp->login_seq_retry_cnt;
	
	memcpy(vpp->peer_password, pp->peer_password, 40);
}


#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
int hfc_vport_create(struct fc_vport *fc_vport, bool disable)
{
	struct Scsi_Host	*host;
	struct Scsi_Host	*vhost;
	struct adap_info	*ap;
	struct port_info	*pp;
	struct port_info	*vpp;
	struct port_info	*wkpp;
	struct region_info	*rp;
	uchar				logdata[16];
	ulong				flags = 0;
	int					i,j;
	int					rtn;
	uint				init_addr;
	uchar				rid;
	
	HFC_ENTRY("hfc_vport_create");
	
	memset(logdata,0,16);
	
	if (disable) {
		HFC_DBGPRT( "hfcldd : hfc_vport_create - disable"); 
		return VPORT_ERROR;
	}
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		/* HFC-PCM */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - HFC-PCM"); 
		return VPORT_ERROR;
	}
	
	host = fc_vport->shost;
	
	if (host == NULL) {
		HFC_DBGPRT( "hfcldd : hfc_vport_create - host null"); 
		return VPORT_ERROR;
	}
	
	ap = (struct adap_info *)host->hostdata;
	pp = (struct port_info *)host->hostdata;
	
	if (!ap || !pp) {
		HFC_DBGPRT( "hfcldd : hfc_vport_create - ap, pp null"); 
		return VPORT_ERROR;
	}
	
	if (!strcmp(ap->name, "adap_info")) {
		/* FIVE-EX */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - FIVE-EX"); 
		return VPORT_ERROR;
	}
	
	rp = pp->region_arg[0];
	if (!rp) {
		HFC_DBGPRT( "hfcldd : hfc_vport_create - rp null"); 
		return VPORT_ERROR;
	}
	
	if ((fc_vport->port_name == 0x0000000000000000LL) ||
		(fc_vport->port_name == 0xffffffffffffffffLL)) {
		/* invalid wwpn */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - invalid wwpn"); 
		logdata[0] = 0x09;
		HFC_MEMCPY((uchar*)&logdata[8], (uchar*)&fc_vport->port_name, 8);
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return VPORT_NORESOURCES;
	}
	
	if ((fc_vport->node_name == 0x0000000000000000LL) ||
		(fc_vport->node_name == 0xffffffffffffffffLL)) {
		/* invalid wwnn */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - invalid wwnn"); 
		logdata[0] = 0x0a;
		HFC_MEMCPY((uchar*)&logdata[8], (uchar*)&fc_vport->node_name, 8);
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return VPORT_NORESOURCES;
	}
	
	if (HFC_FX_MMODE_CHECK_MLPF(pp))
	{	/* mlpf mode */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - mlpf mode"); 
		logdata[0] = 0x07;
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return VPORT_ERROR;
	}
	
	if (!HFC_FX_NPIV_ENABLE(pp))
	{	/* npiv was not enable. */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - npiv not enable"); 
		logdata[0] = 0x01;
		logdata[1] = pp->npiv_mode;
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return VPORT_ERROR;
	}
	
	if( (test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)) && !HFC_FX_NPIV_SHAREABLE(pp) )
	{	/* This port was not shareable. */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - not shareable "); 
		logdata[0] = 0x08;
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return VPORT_ERROR;
	}
	
	if( (test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)) ||
		(test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status)) )
	{	/* This port is recovering. */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - mck recovery"); 
		return VPORT_ERROR;
	}
	
	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status) )
	{	/* This core's state was "isol stop". */
		HFC_DBGPRT( "hfcldd : hfc_vport_create - isol"); 
		return VPORT_ERROR;
	}
	
	HFC_DBGPRT("hfcldd%d: kmalloc_cnt_ap=%d\n",
			pp->instance, atomic_read(&hfc_manage_info.kmalloc_cnt_ap[pp->instance]));
	HFC_DBGPRT("hfcldd%d: dma_alloc_cnt_ap=%d\n",
			pp->instance, atomic_read(&hfc_manage_info.dma_alloc_cnt_ap[pp->instance]));
	
	vhost = scsi_host_alloc(host->hostt, sizeof(struct port_info));
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	if ( vhost == NULL ){
		HFC_DBGPRT( "hfcldd : hfc_vport_create - scsi_host_alloc() fail"); 
		logdata[0] = 0x02;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return VPORT_NORESOURCES;
	}
	
	for (i=0; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		if (memcmp((uchar*)&vpp->ww_name, (uchar*)&fc_vport->port_name, 8) == 0) {
			/* already assigned wwpn */
			HFC_DBGPRT( "hfcldd : hfc_vport_create -  already assigned wwpn"); 
			logdata[0] = 0x0b ;
			HFC_MEMCPY((uchar*)&logdata[8], (uchar*)&fc_vport->port_name, 8);
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			hfc_scsi_host_put(vhost);
			return VPORT_ERROR;
		}
	}
	
	vpp = (struct port_info *)vhost->hostdata;
	memset(vpp, 0, sizeof(struct port_info));
	
	/* Set structure character name */
	strcpy(vpp->name, "port_info");
	
	vpp->fc_vport = fc_vport;
	*(struct port_info **)fc_vport->dd_data = vpp;
	
	vpp->raslog_install = raslog_install;
	
	vpp->manage_info = pp->manage_info;
	
	vpp->hosts = vhost;
	vpp->host_no = vhost->host_no;
	vpp->pci_cfginf = pp->pci_cfginf;
	
	/* don't call hfc_fx_query_devid() */
	vpp->pkg.vender_id		= pp->pkg.vender_id;
	vpp->pkg.device_id		= pp->pkg.device_id;
	vpp->pkg.sub_system_id	= pp->pkg.sub_system_id;
	vpp->pkg.type			= pp->pkg.type;
	vpp->pkg.map			= pp->pkg.map;
	vpp->pkg.port			= pp->pkg.port;
	vpp->core_num			= pp->core_num;
	
	/* don't call hfc_fx_pci_conf() */
	vpp->mem_base_addr	= pp->mem_base_addr;
	
	vhost->transportt = hfc_vport_fc_attach_transport;
	
	/* don't call hfc_fx_query_pktype() */
	vpp->pkg.code		= pp->pkg.code;
	vpp->pkg.core_no	= pp->pkg.core_no;
	vpp->pkg.one_core	= pp->pkg.one_core;
	vpp->pkg.lsi_rev	= pp->pkg.lsi_rev;
	
	vpp->pport = pp;
	
	if (hfc_fx_rid_register(pp, vpp)) {
		HFC_DBGPRT( "hfcldd : hfc_vport_create - hfc_fx_rid_register() fail"); 
		logdata[0] = 0x03 ;
		logdata[1] = (uchar)pp->vport_num ;
		logdata[2] = (uchar)pp->max_vport_count ;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		hfc_scsi_host_put(vhost);
		return VPORT_NORESOURCES;
	}
	
	/* don't call hfc_fx_read_hfcbios() */
	vpp->port_no		= pp->port_no;
	vpp->automap		= pp->automap;
	vpp->narrowmap		= pp->narrowmap;
	vpp->defparam		= pp->defparam;
	vpp->linkspeed		= pp->linkspeed;
	vpp->topology		= pp->topology;
	vpp->isol_cmd		= pp->isol_cmd;
	vpp->spinup_delay	= pp->spinup_delay;
	for (i=0; i<8; i++) {
		vpp->boot_priority[i].ww_name = pp->boot_priority[i].ww_name;
		vpp->boot_priority[i].lun = pp->boot_priority[i].lun;
	}
	
	vpp->instance = pp->instance;
	
	/* don't call hfc_fx_search_adapter_number() */
	HFC_MEMCPY((uchar*)&vpp->ww_name, (uchar*)&fc_vport->port_name, 8);
	HFC_DBGPRT("hfc_vport_create() - new port_name=%llx\n", (unsigned long long)vpp->ww_name);
	HFC_MEMCPY((uchar*)&vpp->node_name, (uchar*)&fc_vport->node_name, 8);
	HFC_DBGPRT("hfc_vport_create() - new node_name=%llx\n", (unsigned long long)vpp->node_name);
	vpp->org_ww_name = pp->org_ww_name;
	vpp->org_node_name = pp->org_node_name;
	vpp->add_ww_name = pp->add_ww_name;
	vpp->add_node_name = pp->add_node_name;
	HFC_MEMCPY((uchar*)&vpp->vfc_ww_name, (uchar*)&fc_vport->port_name, 8);
	HFC_MEMCPY((uchar*)&vpp->vfc_node_name, (uchar*)&fc_vport->node_name, 8);
	
	vhost->unique_id	= host->unique_id;
	vpp->unique_id		= pp->unique_id;
	vpp->dev_minor		= pp->dev_minor;
	vpp->dev_minor		= (pp->dev_minor | 0x00000100);
	vpp->dev_major		= pp->dev_major;
	HFC_MEMCPY((uchar*)&vpp->ecid[0], (uchar*)&pp->ecid[0], 64);
	
	hfc_fx_conf_setup(vpp);
	hfc_fx_param_copy(pp, vpp);
	
	set_bit(HFC_PS_ENABLE, (ulong *)&vpp->status);
	
	memcpy(vpp->adap_id, pp->adap_id, 16);
	vpp->sys_rev = pp->sys_rev;
	memcpy(vpp->vpd_buf, pp->vpd_buf, 512);
	memcpy(vpp->model_name, pp->model_name, 16);
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	if (hfc_fx_attach(vpp)) {
		HFC_DBGPRT( "hfcldd : hfc_vport_create - hfc_fx_attach() fail"); 
		logdata[0] = 0x05 ;
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_rid_unregister(pp, vpp);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		hfc_scsi_host_put(vhost);
		return VPORT_NORESOURCES;
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	rid = vpp->rid;
	
	/* region_arg update */
	pp->region_arg[vpp->rid] = vpp->region_arg[vpp->rid];
	for (i=1; i<=pp->max_vport_count; i++) {
		wkpp = pp->vport_ptr[i].vport_arg;
		if (wkpp == NULL)
			continue;
		
		for (j=0;j<MAX_REGION_PROBE;j++) {
			wkpp->region_arg[j] = pp->region_arg[j];
		}
	}
	
	if (HFC_FX_MIN_PORT_IN_REGION(vpp)) {
		/* core status update */
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/vpp->core_num){
			if (vpp->region_arg[vpp->rid] != NULL) {
				vpp->region_arg[vpp->rid]->core_arg[i]->status =
					pp->region_arg[pp->rid]->core_arg[i]->status;
			}
		}
		
		set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&vpp->status);
		set_bit(HFC_PD_NEED_CORE_START, (ulong *)&vpp->status_detail1);
		set_bit(HFC_ATTACH, (ulong *)&vpp->attach_status);
	}
	else {
		vpp->host_alpa = pp->host_alpa;
		vpp->used_nmsrv = pp->used_nmsrv;
		vpp->connect_type = pp->connect_type;
		set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&vpp->status);
		set_bit(HFC_PD_NEED_FLOGI, (ulong *)&vpp->status_detail1);
		set_bit(HFC_ATTACH, (ulong *)&vpp->attach_status);
		if (test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status)) {
			set_bit(HFC_PS_CONNECTED, (ulong *)&vpp->status);
		}
		else {
			clear_bit(HFC_PS_CONNECTED, (ulong *)&vpp->status);
		}
	}
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	/* Set INIT_ADDR in CCA */
	if (HFC_FX_MIN_PORT_IN_REGION(vpp)) {
		init_addr  = 0x1000;
		init_addr += 0x80 * vpp->rid;
		
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/vpp->core_num){
			if (vpp->region_arg[vpp->rid] != NULL) {
				hfc_fx_write_reg_ext(vpp, (uint)init_addr + i*0x20 + 0x10, (char)0x04,
					((vpp->region_arg[vpp->rid]->core_arg[i]->padr_init) >> 32));
				hfc_fx_write_reg_ext(vpp, (uint)init_addr + i*0x20 + 0x14, (char)0x04,
					(vpp->region_arg[vpp->rid]->core_arg[i]->padr_init));
			}
		}
	}
	
	vpp->open_status = 0;
	vpp->msi_flag = pp->msi_flag;
	
	/* Determine master core */
	vpp->master_core_no = pp->master_core_no;
	vpp->available_pcore = pp->available_pcore;

#if !defined(HFC_STAR)
	rtn = scsi_add_host_with_dma(vhost, &fc_vport->dev, &pp->pci_cfginf->dev);
#else
	rtn = scsi_add_host(vhost, &fc_vport->dev);
#endif

	if(rtn) {
		logdata[0] = 0x06 ;
		logdata[1] = (uchar)rtn ;
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_rid_unregister(pp, vpp);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xd8, logdata, 16);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		hfc_scsi_host_put(vhost);
		return VPORT_NORESOURCES;
	}
	
	hfc_fx_vport_set_state(vpp->fc_vport, FC_VPORT_INITIALIZING);
	
	/* Set fixed fc host attributes */
	hfc_fx_fc_host_init(vhost, vpp);
	
	if (HFC_FX_MIN_PORT_IN_REGION(vpp)) {
		hfc_fx_set_fw_init_tbl(vpp);
		hfc_fx_core_start(vpp, 0);
		hfc_fx_initialize(vpp, 0);
	}
	else {
		if (test_bit(HFC_PS_CONNECTED, (ulong *)&vpp->status)) {
			hfc_fx_vport_initialize(vpp);
		}
	}
	
	hfc_fx_lu_scan_start(vpp);
	do {
		msleep(1);
	} while (atomic_read(&pp->rport_event_wait) != 0);
	
	HFC_DBGPRT("hfc_vport_create() success - rid=0x%02x\n", rid);
	
	HFC_EXIT("hfc_vport_create");
	
	return VPORT_OK;
}

int hfc_vport_delete(struct fc_vport *fc_vport)
{
	struct Scsi_Host	*host;
	struct adap_info	*ap;
	struct port_info	*pp;
	struct port_info	*ppp;
	struct region_info	*rp;
	uchar				rid;
	ulong				flags = 0;
	
	HFC_ENTRY("hfc_vport_delete");
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		/* HFC-PCM */
		HFC_DBGPRT( "hfcldd : hfc_vport_delete - HFC-PCM"); 
		return VPORT_ERROR;
	}
	
	ap = *(struct adap_info **)fc_vport->dd_data;
	pp = *(struct port_info **)fc_vport->dd_data;
	
	if (!ap || !pp) {
		HFC_DBGPRT( "hfcldd : hfc_vport_delete - ap, pp null"); 
		return VPORT_ERROR;
	}
	
	ppp = pp->pport;
	
	if (!strcmp(ap->name, "adap_info")) {
		/* FIVE-EX */
		HFC_DBGPRT( "hfcldd : hfc_vport_delete - FIVE-EX"); 
		return VPORT_ERROR;
	}
	
	host = pp->hosts;
	if (host == NULL) {
		HFC_DBGPRT( "hfcldd : hfc_vport_delete - host null"); 
		return VPORT_ERROR;
	}
	
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		HFC_DBGPRT( "hfcldd : hfc_vport_delete - physical port"); 
		return VPORT_ERROR;
	}
	
	rp = pp->region_arg[pp->rid];
	if (!rp) {
		HFC_DBGPRT( "hfcldd : hfc_vport_delete - rp null"); 
		return VPORT_ERROR;
	}
	rid = pp->rid;
	
	fc_remove_host(host);
	
	scsi_remove_host(host);
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	clear_bit(HFC_FX_VPORT_DISABLE, (ulong *)&pp->pport->vport_ptr[rid].status);
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	hfc_fx_release_adp(pp);
	
	/* check alloc_cnt */
	HFC_DBGPRT("hfcldd%d: kmalloc_cnt_ap=%d\n",
			ppp->instance, atomic_read(&hfc_manage_info.kmalloc_cnt_ap[ppp->instance]));
	HFC_DBGPRT("hfcldd%d: dma_alloc_cnt_ap=%d\n",
			ppp->instance, atomic_read(&hfc_manage_info.dma_alloc_cnt_ap[ppp->instance]));
	
	hfc_scsi_host_put(host);
	
	HFC_DBGPRT("hfc_vport_delete() success - rid=0x%02x\n", rid);
	
	HFC_EXIT("hfc_vport_delete");
	
	return VPORT_OK;
}

int hfc_fx_vport_disable(struct fc_vport *fc_vport, bool disable)
{
	struct Scsi_Host	*host;
	struct port_info	*pp;
	struct region_info	*rp;
	uchar				rid;
	int					i;
	ulong				flags = 0;
	
	HFC_ENTRY("hfc_fx_vport_disable");
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		/* HFC-PCM */
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - HFC-PCM"); 
		return VPORT_ERROR;
	}
	
	pp = *(struct port_info **)fc_vport->dd_data;
	
	rid = pp->rid;
	host = pp->hosts;
	
	if (host == NULL) {
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - host null"); 
		return VPORT_ERROR;
	}
	
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - physical port"); 
		return VPORT_ERROR;
	}
	
	rp = pp->region_arg[rid];
	if (!rp) {
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - rp null"); 
		return VPORT_ERROR;
	}
	
	if (disable)
	{	/* vport disable */
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - vport disable "); 
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		
		if (!HFC_FX_VPORT_ENABLE(pp)) {
			/* already vport disable */
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - already vport disable "); 
			return VPORT_OK;
		}
		
		set_bit(HFC_FX_VPORT_DISABLE, (ulong *)&pp->pport->vport_ptr[rid].status);
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		hfc_fx_release_adp(pp);
		
		hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_DISABLED);
	}
	else
	{
		/* vport enable */
		HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - vport enable "); 
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		
		if (HFC_FX_VPORT_ENABLE(pp)) {
			/* already vport enable */
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			HFC_DBGPRT( "hfcldd : hfc_fx_vport_disable - already vport disable "); 
			return VPORT_OK;
		}
		
		clear_bit(HFC_FX_VPORT_DISABLE, (ulong *)&pp->pport->vport_ptr[rid].status);
		clear_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2);
		set_bit(HFC_PS_ENABLE, (ulong *)&pp->status);
		
		pp->open_status = 0;
		pp->attach_status = 0;
		
		set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);
		set_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1);
		set_bit(HFC_ATTACH, (ulong *)&pp->attach_status);
		
		/* core status update */
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if (pp->region_arg[rid] != NULL) {
				pp->region_arg[rid]->core_arg[i]->status =
					pp->pport->region_arg[rid]->core_arg[i]->status;
			}
		}
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		hfc_fx_core_start(pp, 0);
		
		hfc_fx_initialize(pp, 0);
		
		hfc_fx_lu_scan_start(pp);
		do {
			msleep(1);
		} while (atomic_read(&pp->pport->rport_event_wait) != 0);
	}
	
	HFC_DBGPRT("hfc_vport_disable() success - rid=0x%02x\n", rid);
	
	HFC_EXIT("hfc_fx_vport_disable");
	
	return VPORT_OK;
}

void hfc_fx_vport_set_state(struct fc_vport *fc_vport, enum fc_vport_state new_state)
{
	if (!fc_vport)
		return;
	
	fc_vport->vport_last_state = fc_vport->vport_state;
	fc_vport->vport_state = new_state;
}

#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#endif /* SYSFS_SUPPORT */
