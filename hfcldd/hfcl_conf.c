/*
 * hfcl_conf.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 *
 */
/*
 * $Id: hfcl_conf.c,v 1.5.2.12.2.3.6.1.6.4.2.1.2.4.2.3.6.3.2.1.2.3.2.2 2016/01/11 07:43:55 mhayashi Exp $
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <scsi/scsi.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport_fc.h>

#include "hfcldd.h"
#include "hfcl_detect.h"
#include "hfcl_modulever.h"
#include "hfcl_npiv_fx.h"
#include "hfcl_ioctl.h"


#ifndef _HFC_NO_RASLOG
#include "hraslog.h"
#endif

extern struct manage_info hfc_manage_info;
extern int hfc_major; /* FCLNX-GPL-FX-492 */

#ifndef _HFC_NO_RASLOG
extern struct hraslogopt_st hraslogopt;
extern	uint	raslog_install;
#endif

static struct pci_device_id hfcldd_pci_tbl[] = {
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_3009,
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_300A,
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_300B,
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_300C,
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_300D,
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_3020, /* FCLNX-GPL-188,211 */
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0}, /* FCLNX-GPL-188,211 */
	{HFC_PCI_VENDOR_ID, HFC_PCI_DEVICE_ID_3070,
	PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, hfcldd_pci_tbl);

struct file_operations hfc_fops = {
	owner: THIS_MODULE,
#ifdef CONFIG_COMPAT
	unlocked_ioctl:hfc_ioctl64,
	compat_ioctl:hfc_ioctl32,
#else
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	ioctl:hfc_ioctl_common,
#else
	ioctl:hfc_ioctl,
#endif
#endif
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	open:hfc_open_common,
	release:hfc_close_common
#else
	open:hfc_open,
	release:hfc_close
#endif
};

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
struct miscdevice hfc_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hfcldd",
	.fops = &hfc_fops,
};
#endif


static struct pci_driver hfcldd_pci_driver = {
    .name           = "hfcldd",
    .id_table       = hfcldd_pci_tbl,
    .probe          = hfc_probe_one,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    .remove         = hfc_remove_one,
#else
    .remove         = __devexit_p(hfc_remove_one),
#endif
//    .suspend        = hfc_suspend_one,  /* suspend() entry point */ /* FCLNX-GPL-306 */ /* FCLNX-GPL-429 */
//    .resume         = hfc_resume_one,   /* suspend() entry point */ /* FCLNX-GPL-306 */ /* FCLNX-GPL-429 */
};

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern struct scsi_transport_template *hfc_fc_attach_transport;
extern struct scsi_transport_template *hfc_vport_fc_attach_transport;
extern struct fc_function_template hfcldd_fc_function_template;
extern struct fc_function_template hfcldd_vport_fc_function_template;
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */


/*
 * Function:    hfcldd_init
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Lock status: 
 *
 * Notes:       
 */
static int __init
hfcldd_init(void)
{
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
//	if (misc_register(&hfc_miscdev) == -1) {
//			HFC_DBGPRT(" hfcldd : hfcldd_init - misc_register failed\n");
//	}
#endif
	
	hfcldd_fc_function_template.vport_create = hfc_vport_create;
	hfcldd_fc_function_template.vport_delete = hfc_vport_delete;
	
    hfc_fc_attach_transport = fc_attach_transport(&hfcldd_fc_function_template);
    if (hfc_fc_attach_transport == NULL)
		return -ENOMEM;
	
	hfc_vport_fc_attach_transport = fc_attach_transport(&hfcldd_vport_fc_function_template);
	if (hfc_vport_fc_attach_transport == NULL)
		return -ENOMEM;
    
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
    return pci_register_driver(&hfcldd_pci_driver);
}

/*
 * Function:    hfcldd_exit
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Lock status: 
 *
 * Notes:       
 */
static void __exit
hfcldd_exit(void)
{
#ifndef _HFC_NO_RASLOG
	if( !raslog_install ){
		hraslogopt.func_code = CLOSE_RASLOG;
		raslog_install = _hraslogserv( &hraslogopt );
		if( raslog_install == 1 ){
			HFC_WRNPRT("hfcldd : HFC_ERR9 FC Adapter Driver error (ErrNo:0xC7)  Failed to close hraslog \n");
		}
		else if( raslog_install == 2 ){
			HFC_WRNPRT("hfcldd : HFC_ERR9 FC Adapter Driver error (ErrNo:0xC8) \n");
		}
		else{
			HFC_INFPRT("hfcldd : Closed hraslog\n");
		}
	}
#endif

	if(hfc_manage_info.hfcldd_mp_mod) {
		module_put(hfc_manage_info.mp_manage_info->hfcldd_mp);
	}

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
	/* FCLNX-GPL-FX-492 start */
	if (hfc_major == MISC_MAJOR) {
		misc_deregister(&hfc_miscdev);
	}
	/* FCLNX-GPL-FX-492 end */
#endif

    pci_unregister_driver(&hfcldd_pci_driver);
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    fc_release_transport(hfc_fc_attach_transport);
    fc_release_transport(hfc_vport_fc_attach_transport);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
}

module_init(hfcldd_init);
module_exit(hfcldd_exit);

MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("Hitachi Fibre Channel Host Bus Adapter driver module ");
MODULE_AUTHOR("Hitachi, Ltd.");
MODULE_VERSION(HFC_MODULEVER);

