/*
 * hfcl_ioctl.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_ioctl.h,v 1.11.2.10.2.2.14.1.6.4.2.4.6.4.2.1.2.3.2.2 2015/03/29 09:53:31 toyo Exp $
 */

#ifndef _H_HFCL_IOCTL
#define _H_HFCL_IOCTL

extern struct manage_info hfc_manage_info;
/*extern char hfc_ver[8][64] ;*/
extern void structdump( int loc, uchar *p, int size );
extern void memorydump( uchar *dump_txt, uchar *p, int size ) ;
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
extern int hfc_open_common(struct inode *inode, struct file *file )  ;
#endif
extern int hfc_open(  struct inode *inode, struct file *file );
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
extern int hfc_close_common( struct inode *inode, struct file *file );
#endif
extern int hfc_close( struct inode *inode, struct file *file );
#ifdef CONFIG_COMPAT
extern long hfc_ioctl32(struct file *file, unsigned int cmd, unsigned long arg);
extern long hfc_ioctl64(struct file *file, unsigned int cmd, unsigned long arg);
extern int hfc_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg, int ioctl32 );
#else
//extern int hfc_ioctl_common(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg ); 
extern int hfc_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg ); 
#endif

#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
#ifdef CONFIG_COMPAT
extern long hfc_ioctl_common(struct inode *inode, struct file *file, int cmd, unsigned long arg, int ioctl32);
#else
extern int hfc_ioctl_common(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg ); 
#endif
#endif

extern int hfc_inquiry(   struct adap_info *ap, void *arg );
extern int hfc_sciocmd(   struct adap_info *ap, void *arg, int internal, int tiomeout ); /* FCLNX-0223 */
extern int hfc_get_all_port_ids( struct adap_info *ap, void *arg );
extern int hfc_get_sid_from_wwpn( struct adap_info *ap, void *arg );
extern int hfc_get_wwpn_from_sid( struct adap_info *ap, void *arg );
extern int hfc_payload(   struct adap_info *ap, void *arg );
extern int hfc_get_rnid(  struct adap_info *ap, void *arg );
extern int hfc_set_rnid(  struct adap_info *ap, void *arg );
extern int hfc_adap_stat( struct adap_info *ap, void *arg );
extern int hfc_adap_stat_new( struct adap_info *ap, void *arg );	/* FCLNX-GPL-261 */
extern int hfc_hba_api(   struct adap_info *ap, void *arg );
extern int hfc_port_attr( struct adap_info *ap, struct hfc_ioctl_api *api );
extern int hfc_adap_attr( struct adap_info *ap, struct hfc_ioctl_api *api ) ;
extern int hfc_support(   struct adap_info *ap, void *arg );
extern int hfc_target_mapping( struct adap_info *ap, void *arg );
extern int hfc_fcp_binding(struct adap_info *ap, void *arg );
extern int hfc_procinfo(   struct adap_info *ap, void *arg );
extern int hfc_fc4stat(   struct adap_info *ap, void *arg );					/* FCLNX-0404 */
extern int hfc_wwn_info(   struct adap_info *ap, void *arg );   //FCLNX-0178
extern int hfc_mp_target_map(struct adap_info *ap, void *arg );

extern int hfc_adapter_enable(struct adap_info *ap, void *arg );				/* hotplug */
extern int hfc_adapter_disable(struct adap_info *ap, void *arg );				/* hotplug */
extern int hfc_scsi_scan( struct adap_info *ap, void *arg );					/* hotplug */
extern int hfc_read_apparam( struct adap_info *ap, void *arg );        //FCLNX-0488
extern void hfc_ioctl_iodone(struct scsi_cmnd *cmnd);
extern int hfc_isolate_operation( struct adap_info *ap, void *arg );			/* FCLNX-GPL-147 */
extern void hfc_start_isolate(struct adap_info *ap);

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_change_dev_loss_tmo(struct adap_info *ap);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
#endif

//#ifdef HAVE_UNLOCKED_IOCTL
#ifdef CONFIG_COMPAT
extern long hfc_ioctl32(struct file *file, unsigned int cmd, unsigned long arg);
extern long hfc_ioctl64(struct file *file, unsigned int cmd, unsigned long arg);
#endif

/*--------------------------------------------------------------------------*/
/* Macro */
/*--------------------------------------------------------------------------*/
#define COPYIN(s, d, n) 	copy_from_user((d), (s), (n))
#define COPYOUT(s, d, n)	copy_to_user((d), (s), (n))
#define BCOPY(s, d, n)		memcpy((d), (s), (n))
#define BZERO(d, n)			memset((d), 0, (n))

#define HFC_TRC_IOCTL						0x00
#define HFC_TRC_IOCTL_DIAG					0x10

#define HFC_TRC_IOCTL_SC_OPEN				0x20
#define HFC_TRC_IOCTL_SC_CLOSE				0x21

#define HFC_TRC_IOCTL_SC_ENABLE				0x24
#define HFC_TRC_IOCTL_SC_DISABLE			0x25
#define HFC_TRC_IOCTL_SC_DKSCAN				0x26								/* hotplug */
#define HFC_TRC_IOCTL_SC_APPATH				0x27

#define HFC_TRC_IOCTL_SC_INQ				0x30
#define HFC_TRC_IOCTL_SC_SCICMD				0x31
#define HFC_TRC_IOCTL_SC_GIDFT				0x32
#define HFC_TRC_IOCTL_SC_GWWN				0x33
#define HFC_TRC_IOCTL_SC_PAYLD				0x34
#define HFC_TRC_IOCTL_SC_GRNID				0x35
#define HFC_TRC_IOCTL_SC_SRNID				0x36
#define HFC_TRC_IOCTL_SC_APSTAT				0x37
#define HFC_TRC_IOCTL_SC_API				0x38
#define HFC_TRC_IOCTL_SC_APATTR				0x39
#define HFC_TRC_IOCTL_SC_PTATTR				0x3A
#define HFC_TRC_IOCTL_SC_SPT				0x3B
#define HFC_TRC_IOCTL_SC_TGTMAP				0x3C
#define HFC_TRC_IOCTL_SC_PBIND				0x3D
#define HFC_TRC_IOCTL_SC_PRINFO				0x3E
#define HFC_TRC_IOCTL_SC_FC4STAT			0x3F								/* FCLNX-0404 */

#define HFC_TRC_IOCTL_FWTRC					0x40
#define HFC_TRC_IOCTL_MIHLOG				0x41
#define HFC_TRC_IOCTL_LDCHTRC				0x42
#define HFC_TRC_IOCTL_MEINTLOG				0x43
#define HFC_TRC_IOCTL_FRCLOG				0x44
#define HFC_TRC_IOCTL_LOOP					0x45
#define HFC_TRC_IOCTL_FWPOST				0x46
#define HFC_TRC_IOCTL_FWSTART				0x47
#define HFC_TRC_IOCTL_FSTOP					0x48
#define HFC_TRC_IOCTL_INITMDSET				0x49
#define HFC_TRC_IOCTL_FCPMDSET				0x4A
#define HFC_TRC_IOCTL_PCIACC				0x4B
#define HFC_TRC_IOCTL_PCICNFACC				0x4C

#define HFC_TRC_IOCTL_FWINIT				0x50
#define HFC_TRC_IOCTL_XOB					0x51
#define HFC_TRC_IOCTL_XRB					0x52
#define HFC_TRC_IOCTL_SEG					0x53
#define HFC_TRC_IOCTL_MPADAP				0x54
#define HFC_TRC_IOCTL_APINFO				0x55
#define HFC_TRC_IOCTL_TGINFO				0x56
#define HFC_TRC_IOCTL_MNG					0x57
#define HFC_TRC_IOCTL_HFCTRC				0x58
#define HFC_TRC_IOCTL_MB					0x59
#define HFC_TRC_IOCTL_VER					0x5A
#define HFC_TRC_IOCTL_HWLOG					0x5B
#define HFC_TRC_IOCTL_TREE					0x5C
#define HFC_TRC_IOCTL_HFCPKT                0x5D
#define HFC_TRC_IOCTL_CORE					0x5E

#define HFC_TRC_IOCTL_WWN_INFO              0x60    //FCLNX-0178

#define HFC_TRC_IOCTL_DEV		            0x60
#define HFC_TRC_IOCTL_LGTGT		            0x61
#define HFC_TRC_IOCTL_LGDEV		            0x62
#define HFC_TRC_IOCTL_PATHINFO		        0x63
#define HFC_TRC_IOCTL_PATHINFO1		        0x64
#define HFC_TRC_IOCTL_PATHINFO2		        0x65
#define HFC_TRC_IOCTL_FOINFO		        0x66

#define HFC_TRC_IOCTL_MP_RDPARM				0x70
#define HFC_TRC_IOCTL_MP_WRPARM				0x71
#define HFC_TRC_IOCTL_MP_TGTMAP				0x72
#define HFC_TRC_IOCTL_MP_LUMAP				0x73
#define HFC_TRC_IOCTL_MP_SETPATH			0x74
#define HFC_TRC_IOCTL_MP_PTHEALTH			0x75
#define HFC_TRC_IOCTL_MP_LGTGTMAP			0x76
#define HFC_TRC_IOCTL_MP_LGPATH1			0x77

#define HFC_TRC_IOCTL_MP_LGDEVPARM			0x78		/* FCLNX-0677 *//* FCLNX-710 */

#define HFC_TRC_IOCTL_HFC_HBA_ISOLATION			0x80	/* FCLNX-0488 */
#define HFC_TRC_IOCTL_HFC_READ_APPARAM			0x81	/* FCLNX-0488 */
#define HFC_TRC_IOCTL_SC_APSTAT_NEW				0x82	/* FCLNX-GPL-261 */
#define HFC_TRC_IOCTL_HFCLDD_CONF_STORE			0x93

#define HFC_GS_RJT 0x8001	/* Response code indicates an */
#define HFC_GS_ACC 0x8002	/* Successful response code.  */

/* ------------------------------------*/
struct  hfc_linux_ioctl_header{
        uchar   minor;
        uchar   rsv[3];
};
/* ------------------------------------*/


#endif /* _H_HFCL_IOCTL */
