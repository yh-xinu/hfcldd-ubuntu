/*
 * hfcl_npiv.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_npiv_fx.h,v 1.1.2.6.2.1.2.1 2015/02/04 08:32:48 toyo Exp $
 */

#ifndef _H_HFCL_NPIV
#define _H_HFCL_NPIV

/* API function return codes */
#define VPORT_OK			0
#define VPORT_ERROR			-1
#define VPORT_INVAL			-2
#define VPORT_NOMEM			-3
#define VPORT_NORESOURCES	-4

extern struct manage_info hfc_manage_info;

extern int HFC_FX_NPIV_ENABLE(struct port_info *pp);
extern int HFC_FX_NPIV_EXT_MODE(struct port_info *pp);
extern int HFC_FX_NPIV_SHAREABLE(struct port_info *pp);
extern int HFC_FX_VPORT_EXIST(struct port_info *pp);
extern int HFC_FX_EXT_VPORT_EXIST(struct region_info *rp);
extern int HFC_FX_MIN_PORT_IN_REGION(struct port_info *pp);
extern int HFC_FX_PHYSICAL_PORT(struct port_info *pp);
extern int HFC_FX_VIRTUAL_PORT(struct port_info *pp);
extern int HFC_FX_MQ_VIRTUAL_PORT(struct port_info *pp);
extern int HFC_FX_VPORT_ENABLE(struct port_info *pp);
extern int HFC_FX_MQ_ENABLE(struct port_info *pp);
extern int HFC_FX_MQ_VALID(struct port_info *pp);
extern struct port_info *HFC_FX_GET_MIN_PORT_IN_REGION(struct port_info *pp);

extern void hfc_fx_npiv_config_check(struct port_info *pp, struct core_info *core);
extern int hfc_fx_rid_register(struct port_info *ppp, struct port_info *vpp);
extern int hfc_fx_rid_unregister(struct port_info *ppp, struct port_info *vpp);
extern void hfc_fx_serach_minimum_port_in_region(struct port_info *ppp, int rid);
extern void hfc_fx_vport_initialize(struct port_info *pp);
extern void hfc_fx_param_copy(struct port_info *pp, struct port_info *vpp);
extern void hfc_fx_start_vport(struct port_info *vpp);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern int hfc_vport_create(struct fc_vport *fc_vport, bool disable);
extern int hfc_vport_delete(struct fc_vport *fc_vport);
extern int hfc_fx_vport_disable(struct fc_vport *fc_vport, bool disable);
extern void hfc_fx_vport_set_state(struct fc_vport *fc_vport, enum fc_vport_state new_state);
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#endif /* SYSFS_SUPPORT */

#endif /* _H_HFCL_NPIV*/
