/*
 * hfcl_mlpf.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char mlpf_rcsid[] = "$Id: hfcl_mlpf.c,v 1.3.2.10.2.1.6.2.6.13.4.5.2.22.2.3.2.1.2.1.2.4 2015/07/29 08:04:06 toyo Exp $";

#include "hfcldd.h"
#include "hfclddcom.h"
#include "hfcl_detect.h"
#include "hfcl_hand_timer_trace.h"
#include "hfcl_top.h"
#include "hfcl_tbol.h"
#include "hfcl_mlpf.h"
#include "hfcl_ioctl.h"
#include "hfcl_strategy.h"
#include "hfcl_timer_recovery.h"
#include "hfcldd_conf.h"
#include "hfcl_handler.h"											/* FCLNX-GPL-489 */

/* MMIO-HG space address map */										/* @MLPF STR */
#define HFC_IOHGSPC_PARTSNUM_LEN        0x10
#define HFC_IOHGSPC_SYSREV_LEN          0x4
#define HFC_IOHGSPC_ADAPID_LEN          0x10
#define HFC_IOHGSPC_VPDAREA_LEN         0x80
#define HFC_IOHGSPC_INDAREA_LIEN        0x80
#define HFC_IOHGSPC_EFI_OP_TBL_LEN      0x100
const struct hg_map hfc_hg_map[1] = {
	/* MMIO-HG Ver=0100 */
	{
		{
	      /* MMIO-HG */     /* no : name		 			   size */
	      { 0x0000,         /* 0  :  HFC_IOHGSPC_ZERO			-   */
	        0x0000,         /* 1  :  HFC_IOHGSPC_LENGTH			4B  */
	        0x0004,         /* 2  :  HFC_IOHGSPC_VERSION		4B  */
	        0x0008,         /* 3  :  HFC_IOHGSPC_HYPSTATUS		4B  */
	        0x000c,         /* 4  :  HFC_IOHGSPC_CMNDREG		4B  */
	        0x0010,         /* 5  :  HFC_IOHGSPC_VFCWWPN0		4B  */
	        0x0014,         /* 6  :  HFC_IOHGSPC_VFCWWPN1		4B  */
	        0x0018,         /* 7  :  HFC_IOHGSPC_VFCWWNN0		4B  */
	        0x001c,         /* 8  :  HFC_IOHGSPC_VFCWWNN1		4B  */
	        0x0020,         /* 9  :  HFC_IOHGSPC_RID			4B  */
	        0x0024,         /* 10 :  HFC_IOHGSPC_MLPFDDVER		4B  */
	        0x0028,         /* 11 :  HFC_IOHGSPC_LPARSTATUS		4B  */
	        0x002C,         /* 12 :  HFC_IOHGSPC_HYPINTDETAIL	4B	*//* FCLNX-GPL-427 */
	        0x0000,         /* 13 :  */
	        0x0000,         /* 14 :  */
	        0x0000,         /* 15 :  */
	        0x0000,         /* 16 :  */
	        0x0000,         /* 17 :  */
	        0x0000,         /* 18 :  */
	        0x0000,         /* 19 :  */
	        0x0030,         /* 20 :  HFC_IOHGSPC_INDACC0		4B  */
	        0x0034,         /* 21 :  HFC_IOHGSPC_INDACC1		4B  */
	        0x0038,         /* 22 :  HFC_IOHGSPC_INDACC2		4B  */
	        0x003c,         /* 23 :  HFC_IOHGSPC_INDACC3		4B  */
	        0x0040,         /* 24 :  HFC_IOHGSPC_PARTSNUM0		4B  */
	        0x0044,         /* 25 :  HFC_IOHGSPC_PARTSNUM1		4B  */
	        0x0048,         /* 26 :  HFC_IOHGSPC_PARTSNUM2		4B  */
	        0x004c,         /* 27 :  HFC_IOHGSPC_PARTSNUM3		4B  */
	        0x0050,         /* 28 :  HFC_IOHGSPC_SYSREV0		4B  */
	        0x0054,         /* 29 :  HFC_IOHGSPC_SYSREV1		4B  */
	        0x0058,         /* 30 :  HFC_IOHGSPC_SYSREV2		4B  */
	        0x005c,         /* 31 :  HFC_IOHGSPC_SYSREV3		4B  */
	        0x0060,         /* 32 :  HFC_IOHGSPC_ADAPID0		4B  */
	        0x0064,         /* 33 :  HFC_IOHGSPC_ADAPID1		4B  */
	        0x0068,         /* 34 :  HFC_IOHGSPC_ADAPID2		4B  */
	        0x006c,         /* 35 :  HFC_IOHGSPC_ADAPID3		4B  */
	        0x0070,         /* 36 :  HFC_IOHGSPC_HVM_SUPPORT        4B  */ /* FCLNX-GPL-140 */
	        0x0074,         /* 37 :  HFC_IOHGSPC_DRV_SUPPORT        4B  */ /* FCLNX-GPL-140 */
	        0x0000,         /* 38 :  */
	        0x0000,         /* 39 :  */
	        0x0080,         /* 40 :  HFC_IOHGSPC_VPDAREA		128B */
	        0x0100,         /* 41 :  HFC_IOHGSPC_INDAREA		128B */
	        0x0180,         /* 42 :  HFC_IOHGSPC_LDS_LIMIT		2B	 */
	        0x0182,         /* 43 :  HFC_IOHGSPC_LDS_COUNT		2B	 */
	        0x0184,         /* 44 :  HFC_IOHGSPC_FCIF_LIMIT		2B	 */
	        0x0186,         /* 45 :  HFC_IOHGSPC_FCIF_COUNT		2B	 */
	        0x0188,         /* 46 :  HFC_IOHGSPC_TO_LIMIT		2B	 */
	        0x018a,         /* 47 :  HFC_IOHGSPC_TO_COUNT		2B	 */
	        0x018c,         /* 48 :  HFC_IOHGSPC_RST_LIMIT		2B	 */
	        0x018e,         /* 49 :  HFC_IOHGSPC_RST_COUNT		2B	 */
	        0x01f0,         /* 50 :  HFC_IOHGSPC_EFI_INFO0		4B  */
	        0x01f4,         /* 51 :  HFC_IOHGSPC_EFI_INFO1		4B  */
	        0x01f8,         /* 52 :  HFC_IOHGSPC_EFI_INFO2		4B  */
	        0x01fc,         /* 53 :  HFC_IOHGSPC_EFI_INFO3		4B  */
	        0x0200,         /* 54 :  HFC_IOHGSPC_EFI_OP_TBL0	4B  */
	        0x0204,         /* 55 :  HFC_IOHGSPC_EFI_OP_TBL1	4B  */
	        0x0190,         /* 56 :  HFC_IOHGSPC_HGCCA_ADDR0	4B  */
	        0x0194,         /* 57 :  HFC_IOHGSPC_HGCCA_ADDR1	4B  */
	        0x0198,         /* 58 :  HFC_IOHGSPC_HGCCA_FLAG		1B  */
	        0x0000         /* 59 :  */
	        }
	    }
    }
};																	/* @MLPF END */

/*--------------------------------------------------------------------------*/
/* name : hfc_version()                                                     */
/*--------------------------------------------------------------------------*/

/*
 * Function:    HFC_MMODE_CHECK_BASIC
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_MMODE_CHECK_BASIC(
	struct adap_info            *ap)
{
	if ( !(ap->mlpf_mode & HFC_MMODE_MLPF ) )
		return HFC_MMODE_CHECK_OK;
	else
		return HFC_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_MMODE_CHECK_MLPF
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_MMODE_CHECK_MLPF(
	struct adap_info            *ap)
{
	if (ap->mlpf_mode & HFC_MMODE_MLPF )
		return HFC_MMODE_CHECK_OK;
	else
		return HFC_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_MMODE_CHECK_SHADOW
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_MMODE_CHECK_SHADOW(
	struct adap_info            *ap)
{
	if (ap->mlpf_mode & HFC_MMODE_SHADOW )
		return HFC_MMODE_CHECK_OK;
	else
		return HFC_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_MMODE_CHECK_REBOOT
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_MMODE_CHECK_REBOOT(
	struct adap_info            *ap)
{
	if (ap->mlpf_mode & HFC_MMODE_REBOOT )
		return HFC_MMODE_CHECK_OK;
	else
		return HFC_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_MMODE_CHECK_SHARED
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_MMODE_CHECK_SHARED(
	struct adap_info            *ap)
{
	if ( (ap->mlpf_mode & HFC_MMODE_MLPF ) && ! (ap->mlpf_mode & HFC_MMODE_DEDICATE ) )
		return HFC_MMODE_CHECK_OK;
	else
		return HFC_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_MMODE_CHECK_DEDICATE
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_MMODE_CHECK_DEDICATE(
	struct adap_info            *ap)
{
	if ( (ap->mlpf_mode & HFC_MMODE_MLPF ) && (ap->mlpf_mode & HFC_MMODE_DEDICATE ) )
		return HFC_MMODE_CHECK_OK;
	else
		return HFC_MMODE_CHECK_ERROR;
}

#define MIN_IOBASE_LEN          0x100


/*
 * Function:    hfc_mlpf_setup_lparmode
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_mlpf_setup_lparmode(
	struct adap_info            *ap)
{
	struct pci_dev *pdev	= NULL;
	uint                      flag4, len4;
	uint                      base4;
	unsigned long             hyper_status;
	uint                        rid_int;
	uint64_t                    mmio_hg;
	unsigned long             lpar_status,status;	/* FCLNX-GPL-393 */
	
	if(HFC_MMODE_CHECK_SHADOW(ap)){
		HFC_DBGPRT("This LPAR is shadow LPAR\n");
	}
	else
		HFC_DBGPRT("This LPAR is Guest LPAR\n");
	
	/* PCI memory space mapping */
	pdev = ap->pci_cfginf;
	
	base4 = pci_resource_start(pdev, 4);
	len4 = pci_resource_len(pdev, 4);
	flag4 = pci_resource_flags(pdev, 4);
	
	if ( base4 == 0 )
	{
		ap->mlpf_mode &= ~( HFC_MMODE_MLPF | HFC_MMODE_DEDICATE );
		HFC_DBGPRT("hfc_mlpf_setup_lparmode: MMIO HG pointer NULL\n");
		if( HFC_MMODE_CHECK_SHADOW(ap)){
			HFC_DBGPRT("Shadow LPAR\n");
			HFC_ERRPRT("hfcldd : HFC_ERR9 FC Adapter Driver error (ErrNo:0xA3) \n"); 	/* FCLNX-0357 */
			HFC_ERRPRT("hfcldd : Invalid MMIO-HG Address.\n"); 							/* FCLNX-0357 */
			return HFC_MLPF_DISABLE;                                             /* FCLNX-0*** */
		}
		else
		{
			HFC_DBGPRT("Guest LPAR\n");
			return HFC_MLPF_ENABLE;  // When the setting is not invalid, it is ended with enable
		}
	}
	else
	{
		mmio_hg = (ulong)ioremap(base4, len4);  /* FCLNX-0352 */
		
		if (!(flag4 & IORESOURCE_MEM)) {
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xA3, NULL, 0) ;/* FCLNX-0357 */ /* FCLNX-GPL-161 */
			HFC_DBGPRT("  scsi(%ld): region #0 not a PIO resource, aborting\n",
				ap->host_no);
			HFC_DBGPRT("hfcldd :  Failed to pci_resource_flags \n");			 
			return HFC_MLPF_DISABLE;
		}
		
		if (len4 < MIN_IOBASE_LEN) {
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xA3, NULL, 0) ;/* FCLNX-0357 */ /* FCLNX-GPL-161 */
			HFC_DBGPRT("hfcldd : Invalid MMIO-HG Length.\n"); 							/* FCLNX-0357 */
			HFC_DBGPRT("  scsi(%d): Invalid PCI I/O region size, aborting\n",
					(uint)ap->host_no);
			HFC_DBGPRT("hfcldd :  Failed to pci_resource_flags \n");			 
			return HFC_MLPF_DISABLE;
		}
	
		HFC_DBGPRT("mmio_hg NOT NULL\n");
		ap->hg_mem_base_addr = mmio_hg;
		HFC_DBGPRT("set ap->hg_mem_base_addr mmio_hg\n");
		
		ap->mlpf_mode |= HFC_MMODE_MLPF;
//		ap->hg_map  = (struct hg_map *) &hfc_hg_map[0]; /* FCLNX-0326 *//* FCLNX-0359 */

		ap->lparmode.hg_map = (struct hg_map *) &hfc_hg_map[0];         /* FCLNX-0374 */
		
		if(HFC_MMODE_CHECK_SHADOW(ap))
		{
			if ( hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_REBOOT, HFC_CHECK_HYPER_STATE) )
			{
				HFC_DBGPRT("Shadow LPAR is rebooted\n");
				ap->mlpf_mode |= HFC_MMODE_REBOOT;
				hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_REBOOT, HFC_DISABLE_HYPER_STATE);	/* FCLNX-GPL-411 */
			}
		}
		/* Refer MMIO-HG HyperStatus */
		HFC_DBGPRT("Refer MMIO-HG HyperStatus\n");
		hyper_status = (unsigned long) hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4 );

		if (hyper_status & HFC_HG_HYPSTATUS_MODE)
		{
			/* dedicate mode */
			if(HFC_MMODE_CHECK_SHADOW(ap))
			{
				HFC_DBGPRT("hfc_mlpf_setup_lparmode: This Shadow LPAR is dedicate %x\n", ap->mlpf_mode);
				HFC_ERRPRT("hfcldd : HFC_ERR9 FC Adapter Driver error (ErrNo:0xA2) \n"); 	/* FCLNX-0357 */
				HFC_ERRPRT("hfcldd : Invalid Hyper Status Mode.\n"); 						/* FCLNX-0357 */
				hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_ISVALID, HFC_ENABLE_LPAR_STATE);
				hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_SPACE, HFC_DISABLE_LPAR_STATE);
				hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_WWN_INVALID, HFC_ENABLE_LPAR_STATE);
				return HFC_MLPF_DISABLE;
			}
			
			ap->mlpf_mode |= HFC_MMODE_DEDICATE;
			
			HFC_DBGPRT("hfc_mlpf_setup_lparmode: This LPAR is dedicate %x\n", ap->mlpf_mode);
		} else 
		{
			/* shared mode */
			if (hyper_status & HFC_HG_HYPSTATUS_FDISABLE)
			{
				HFC_DBGPRT("hfc_mlpf_setup_lparmode: This LPAR is force disable %x\n", ap->mlpf_mode);
				return HFC_MLPF_DISABLE;
			}
			HFC_DBGPRT("hfc_mlpf_setup_lparmode: This LPAR is shared %x\n", ap->mlpf_mode);
		}
	}
	
	ap->hg_cca_p = NULL;		/* FCLNX-GPL-494 */
	
	if(HFC_MMODE_CHECK_SHADOW(ap))
	{
		uchar                   adap_id[16];
		uchar                   sys_buf[4];
		uint					wk;						/* FCLNX-GPL-319 */
		uchar					addr[4];				/* FCLNX-GPL-319 */

		
		HFC_DBGPRT("hfc_mlpf_setup_lparmode: Shadow LPAR set adap_id, sysrev %x\n", ap->mlpf_mode);
		
		// Sets adap_id and sys_buf to mmio-hg, but parts_number is set after.
		if( hfc_read_flash(ap, 0x54, 4, addr)){			/* FCLNX-GPL-319 */
			return(HFC_MLPF_DISABLE);
		}
		HFC_4B_TO_4L(wk, (*(uint*)(&addr[0])));
		if(hfc_read_flash(ap, wk, 16, adap_id)){
			return(HFC_MLPF_DISABLE);
		}												/* FCLNX-GPL-319 */
		hfc_mlpf_set_mmio_hg(ap, adap_id, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
		
		if( hfc_read_flash(ap, 0, 4, sys_buf) ){
			return(HFC_MLPF_DISABLE);
		}
		hfc_mlpf_set_mmio_hg(ap, sys_buf, HFC_IOHGSPC_SYSREV0, HFC_IOHGSPC_SYSREV_LEN);
	}
	
	//Sets RID.
	if ( HFC_MMODE_CHECK_SHARED(ap) )
	{
		HFC_DBGPRT("hfc_mlpf_setup_lparmode: Shared LPAR set RID %x\n", ap->mlpf_mode);
		rid_int = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_RID, 0x4 );
		ap->rid = rid_int;
		HFC_DBGPRT("hfc_mlpf_setup_lparmode: read RID from MMIOHG%x\n", ap->rid);
		
		/* FCLNX-GPL-393 */
		lpar_status = (unsigned long) hfc_read_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4 );
		
		if(!(HFC_MMODE_CHECK_SHADOW(ap) )){
			status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 4);

			if(status & HFC_HG_LPAR_ISOLATION_SUPPORT)
				set_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support);
			if (lpar_status & HFC_HG_LPRSTATUS_ISOLSUPPRT)
				set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support);
		}
		/* FCLNX-GPL-393 */
	}
	
	/* Set Support bits for MMIO-HG Version 2 */ /* FCLNX-GPL-140 */ /* FCLNX-GPL-306 */ /* FCLNX-GPL-393 *//* FCLNX-GPL-481 *//* FCLNX-GPL-494 */
	hfc_write_hg_reg(ap,  HFC_IOHGSPC_DRV_SUPPORT, 4, 
		(
			HFC_HG_FIVE_EX_SUPPORT |
		    HFC_HG_MULTI_ALPA_SUPPORT |
		    HFC_HG_GUEST_FWUPDATE_SUPPORT |
		    HFC_HG_LPAR_MIGRATION_SUPPORT |
		    HFC_HG_LPAR_ISOLATION_SUPPORT |
		    HFC_HG_LPAR_STATISTICS_SUPPORT
		) );
	HFC_DBGPRT("hfc_mlpf_setup_lparmode: Exit HFC_MLPF_ENABLE %x\n", ap->mlpf_mode);
	return HFC_MLPF_ENABLE;
}


/*
 * Function:    hfc_mlpf_setup_wwn
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_mlpf_setup_wwn(
	struct adap_info            *ap)
{

	uint                        wk;
	uchar                       adap_id[16];


	wk = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_VFCWWPN0, 0x4 );
	ap->vfc_ww_name = wk;
	ap->vfc_ww_name <<= 32;
	HFC_DBGPRT("get vfc_wwpn form MMIOHG %llx\n", (unsigned long long)ap->vfc_ww_name);
	
	wk = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_VFCWWPN1, 0x4 );
	ap->vfc_ww_name |= wk;

	wk = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_VFCWWNN0, 0x4 );
	ap->vfc_node_name = wk;
	ap->vfc_node_name <<= 32;
	HFC_DBGPRT("get vfc_wwnn form MMIOHG %llx\n", (unsigned long long)ap->vfc_node_name);

	wk = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_VFCWWNN1, 0x4 );
	ap->vfc_node_name |= wk;

	ap->ww_name = ap->vfc_ww_name;
	ap->node_name = ap->vfc_node_name;
	HFC_DBGPRT("set adap_info : wwpn %llx\n", (unsigned long long)ap->ww_name);
	HFC_DBGPRT("set adap_info : wwnn %llx\n", (unsigned long long)ap->node_name);
	
	hfc_mlpf_get_mmio_hg(ap, adap_id, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
	
	HFC_4B_TO_4L(wk, (*(uint*)(&adap_id[0])));
	ap->org_ww_name = wk;
	ap->org_ww_name <<= 32;
	HFC_4B_TO_4L(wk, (*(uint*)(&adap_id[4])));
	ap->org_ww_name |= wk;
	HFC_4B_TO_4L(wk, (*(uint*)(&adap_id[8])));
	ap->org_node_name = wk;
	ap->org_node_name <<= 32;
	HFC_4B_TO_4L(wk, (*(uint*)(&adap_id[12])));
	ap->org_node_name |= wk;
	
	ap->org_ww_name += ap->port_no * 2;
	ap->org_node_name += ap->port_no * 2;
	
	return HFC_MLPF_VFC_VALID;
}


/*
 * Function:    hfc_mlpf_set_mmio_hg
 *
 * Purpose:     This routine sets hfcldd_data from mp_adap_info or adap-info to MMIOHG.
 *              This routine is considered for Shadow LPAR.
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_set_mmio_hg(
	struct adap_info            *ap,
	uchar                       *buf,
	uint                        regno,
	uint                        length)
{
	uchar                       *ptr;
	uint                        value;							/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	uint                        i;
	uint                        offset;
	
	offset = ap->lparmode.hg_map->iosp.reg[regno];                  /* FCLNX-0330 *//* FCLNX-0374 */
	HFC_DBGPRT("hfc_mlpf_set_mmio_hg: offset %x\n", offset);
	HFC_DBGPRT("hfc_mlpf_set_mmio_hg: length %x\n", length);
	
	ptr = (uchar *)buf;
	
	for ( i = 0 ; i < (length/4) ; i++)                         /* FCLNX-0331 */
	{
		value = 0;
		value += 0xff000000 & ((uint)ptr[(i*4)] << 24);      /* FCLNX-0333 */
		value += 0x00ff0000 & ((uint)ptr[((i*4)+1)] << 16 );
		value += 0x0000ff00 & ((uint)ptr[((i*4)+2)] << 8 );
		value += 0x000000ff & (uint)ptr[((i*4)+3)];
		
		HFC_DBGPRT("hfc_mlpf_set_mmio_hg: value[%d] : %x\n", i, value);
		hfc_write_reg_hg_ext(ap, offset + (i*4), 4, value);
	}	
	
	return;
}


/*
 * Function:    hfc_mlpf_set_mmio_hg
 *
 * Purpose:     This routine gets VPDDATA from MMIOHG, and sets it mp_adap_info.
 *              This routine is considered for Guest LPAR.
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  buf        -
 *  regno      -
 *  length     -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_get_mmio_hg(
	struct adap_info            *ap,
	uchar                       *buf,
	uint                        regno,
	uint                        length)
{
	uchar                       *ptr;
	uint                        value;
	uint                        i;
	uint                        offset;
	
	offset = ap->lparmode.hg_map->iosp.reg[regno];                  /* FCLNX-0374 */
	ptr = (uchar *)buf;
	
	for ( i = 0 ; i < (length / 4) ; i ++)          /* FCLNX-0332 */
	{
		value = 0;
		value = (uint)hfc_read_reg_hg_ext(ap, offset + (i*4), 4);
		
		/* store data with reverse order */
		*ptr++ = (uchar)((value >> 24) & 0xFF);
		*ptr++ = (uchar)((value >> 16) & 0xFF);
		*ptr++ = (uchar)((value >> 8) & 0xFF);
		*ptr++ = (uchar)(value & 0xFF);
	}	
	
	return;
}


/*
 * Function:    hfc_mlpf_config_check
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_mlpf_config_check(
	struct adap_info            *ap)
{
	uchar                       wk1;
	uchar                       wk2;
	uchar                       wk3;
	uint                        wk4;
	
	// This case is considered for Guest LPAR.
	// Guest LPAR doesn't have to check status about Link Initialize resoponse.
	// So, This routine is finished for Guest LPAR at success .
	if (!HFC_MMODE_CHECK_SHADOW(ap))
		return 0;
	
	wk1 = hfc_read_val( ap->fw_init_p->fw_iocinfo.flag);
	wk2 = hfc_read_val( ap->fw_init_p->fw_iocinfo.connect_type);
	wk3 = hfc_read_val( ap->fw_init_p->fw_iocinfo.npiv_valid);
	
	if( ( ap->pkg.type == HFC_PKTYPE_FPP ) || ( ap->pkg.type == HFC_PKTYPE_FIVE ) ) {
		if ( !(wk1 & HFC_FABRIC_VALID) )
		{
			hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_UNSHARABLE, HFC_ENABLE_LPAR_STATE);
			hfc_mlpf_change_state(ap, (HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRSTATUS_LINKDOWN), HFC_DISABLE_LPAR_STATE);	/* FCLNX-0396 */
			hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_PRIVATE, HFC_ENABLE_LPAR_STATE);
			
			return HFC_MLPF_CONFIG_CHECK_ERROR;
		} else if ( wk2 == HFC_AL)
		{
			hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_UNSHARABLE, HFC_ENABLE_LPAR_STATE);
			hfc_mlpf_change_state(ap, (HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRSTATUS_LINKDOWN), HFC_DISABLE_LPAR_STATE);	/* FCLNX-0396 */
			hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_FCAL, HFC_ENABLE_LPAR_STATE);
			
			return HFC_MLPF_CONFIG_CHECK_ERROR;
		} else if ( !(wk3 & HFC_NPIV_VALID) )
		{
			hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_UNSHARABLE, HFC_ENABLE_LPAR_STATE);
			hfc_mlpf_change_state(ap, (HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRSTATUS_LINKDOWN), HFC_DISABLE_LPAR_STATE);	/* FCLNX-0396 */
			hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_NO_NPIV, HFC_ENABLE_LPAR_STATE);
			
			return HFC_MLPF_CONFIG_CHECK_ERROR;
		}
		
		hfc_mlpf_change_state(
			ap, 
			HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_SPACE, 
			HFC_DISABLE_LPAR_STATE);
		
		return HFC_MLPF_CONFIG_CHECK_OK;
	}
	else { /* FIVE-EX */
		wk4 = hfc_read_val( ap->fw_init_p->fw_iocinfo.p2p_port_id);
		wk4 = (wk4 >> 24) & 0xff;	/* ALPA count */
		
		if ( !(wk1 & HFC_FABRIC_VALID) ) 
		{
			if ( wk2 == HFC_AL) 	/* Multi ALPA */
			{
				hfc_mlpf_change_state(
					ap,
					(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE),
					 HFC_DISABLE_LPAR_STATE);
				hfc_mlpf_change_state(
					ap,
					(((wk2 << 12) & 0x00003000) | wk4),
					HFC_ENABLE_LPAR_STATE);
				
				return HFC_MLPF_CONFIG_CHECK_OK;
			}
			else 
			{
				hfc_mlpf_change_state(
					ap,
					HFC_HG_LPRSTATUS_UNSHARABLE,
					HFC_ENABLE_LPAR_STATE);
				hfc_mlpf_change_state(
					ap,
					(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE),
					HFC_DISABLE_LPAR_STATE);
				hfc_mlpf_change_state(
					ap,
					((wk2 << 12) & 0x00003000),
					HFC_ENABLE_LPAR_STATE);
				
				return HFC_MLPF_CONFIG_CHECK_ERROR;
			}
		} 
		else if ( wk2 == HFC_AL)	/* Multi ALPA */
		{
			hfc_mlpf_change_state(
				ap, 
				(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE),
				HFC_DISABLE_LPAR_STATE);
			hfc_mlpf_change_state(
				ap,
				(HFC_HG_LPRDETAIL_FCSW | ((wk2 << 12) & 0x00003000) | wk4),
				HFC_ENABLE_LPAR_STATE);
			
			return HFC_MLPF_CONFIG_CHECK_OK;
		} 
		else if ( !(wk3 & HFC_NPIV_VALID) ) 
		{
			hfc_mlpf_change_state(
				ap, 
				HFC_HG_LPRSTATUS_UNSHARABLE,
				HFC_ENABLE_LPAR_STATE);
			hfc_mlpf_change_state(
				ap,
				(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE),
				HFC_DISABLE_LPAR_STATE);
			hfc_mlpf_change_state(
				ap,
				(HFC_HG_LPRDETAIL_FCSW | ((wk2 << 12) & 0x00003000)),
				HFC_ENABLE_LPAR_STATE);
			
			return HFC_MLPF_CONFIG_CHECK_ERROR;
		}
		else						/* NPIV */
		{
			hfc_mlpf_change_state(
				ap,
				(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE),
				HFC_DISABLE_LPAR_STATE);
			hfc_mlpf_change_state(
				ap,
				(HFC_HG_LPRDETAIL_FCSW | HFC_HG_LPRDETAIL_NPIV | ((wk2 << 12) & 0x00003000)),
				HFC_ENABLE_LPAR_STATE);
			
			return HFC_MLPF_CONFIG_CHECK_OK;
		}
	}
}


/*
 * Function:    hfc_mlpf_check_state
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_mlpf_check_state(
	struct adap_info            *ap,
	uint                        status,
	uint                        type)
{
	uint                        wk4 = 0;
	uint                        check;
	
	HFC_DBGPRT("hfc_mlpf_check_state Enter\n");
	if ( type == HFC_CHECK_HYPER_STATE )
	{
		wk4 = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
	} else if ( type == HFC_CHECK_LPAR_STATE )
	{
		wk4 = hfc_read_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4);
	} else if ( type == HFC_CHECK_HVM_SUPPORT )	/* FCLNX-GPL-489 */
	{
		wk4 = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
	}											/* FCLNX-GPL-489 */

	
	check = wk4 & status;
	
	if ( check == status )
	{
		HFC_DBGPRT("hfc_mlpf_check_state End return 1\n");
		return 1;
	}
	else
	{
		HFC_DBGPRT("hfc_mlpf_check_state End return 0\n");
		return 0;
	}
}


/* FCLNX-GPL-393 */
/*
 * Function:    hfc_mlpf_check_isol_support
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_check_isol_support(
	struct adap_info            *ap)
{
	uint                        status,hyp_status;
	
	status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 4);
	
	hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);

	if(status & HFC_HG_LPAR_ISOLATION_SUPPORT){
		set_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support);

		if(((ap->pkg.port <= 1)||(ap->fw_init_p->func2 & HFC_FWF_ISOLHVM))	||
		(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOL)			||
		(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOLRCV)		||
		(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_ISOL)){	/* FCLNX-GPL-427 */
			hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_ISOLSUPPRT, HFC_ENABLE_LPAR_STATE);
			set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support);
		}
	}
	
	return;
}



/*
 * Function:    hfc_mlpf_check_isol_psycalport
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       This function must be called before calling hfc_start_adapter.
 */
void hfc_mlpf_check_isol_psycalport(
	struct adap_info            *ap)
{
	uint					hyp_status;
	uchar					logdata[16];

	ap->isol_force = HFC_NO_FRC_ISOL;

	if(!test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support)) return;

	hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
	
	if((hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOL)||
	(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOLRCV)||
	(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_ISOL))
	{	/* FCLNX-GPL-427 */
		ap->isol_force = HFC_SHARED_PRT_FRC_ISOL;

		memset(logdata, 0, 16);
		ap->c_err = 0x00;
		logdata[0]=ap->c_err;
		
		if( hyp_status & HFC_HG_HYPSTATUS_ISOLCMD){	/* FCLNX-GPL-427 */
			ap->isol_detail = HFC_ISOLATE_PORT_C;
			
			logdata[1]=ap->isol_detail;
			
			if( HFC_MMODE_CHECK_SHADOW(ap) ){
				if (ap->pkg.port <= 1) {
					hfc_errlog(
						ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0x8E, logdata, 16);
				}
				else{
					hfc_errlog(
						ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0xD4, logdata, 16);
				}
			}
			else{
				hfc_errlog(
					ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT2, 0xD4, logdata, 16);
			}

			HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
				ap->pci_cfginf->bus->number,
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn));
		}
		else{
			ap->isol_detail = HFC_ISOLATE_PORT_E;
			
			logdata[1]=ap->isol_detail;
			
			if( HFC_MMODE_CHECK_SHADOW(ap) ){
				if (ap->pkg.port <= 1) {
					hfc_errlog(
						ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0x8F, logdata, 16);
				}
				else{
					hfc_errlog(
						ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
						ERRID_HFCP_EVNT2, 0xD5, logdata, 16);
				}
			}
			else{
				hfc_errlog(
					ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT2, 0xD5, logdata, 16);
			}
			
			HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by error \n",
				ap->pci_cfginf->bus->number,
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn));
		}
	}
	return;
}
/* FCLNX-GPL-393 */


/*
 * Function:    hfc_mlpf_change_state
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_change_state(
	struct adap_info            *ap,
	uint                        status,
	uint                        type)
{
	uint                        wk4;
	
	HFC_DBGPRT("hfc_mlpf_change_state - type=0x%08x, status=0x%08x\n", type, status);
	
	if( type == HFC_ENABLE_HYPER_STATE )
	{
		wk4 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
		wk4 |= status;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4, wk4 );
	} else if ( type == HFC_DISABLE_HYPER_STATE )
	{
		wk4 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
		wk4 &= ~status;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4, wk4 );
	} else if ( type == HFC_ENABLE_LPAR_STATE )
	{
		wk4 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4);
		wk4 |= status;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4, wk4 );
	} else if ( type == HFC_DISABLE_LPAR_STATE )
	{
		wk4 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4);
		wk4 &= ~status;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4, wk4 );
	} else if( type == HFC_ENABLE_DRV_SUPPORT )	/* FCLNX-GPL-489 */
	{
		wk4 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_DRV_SUPPORT, 0x4);
		wk4 |= status;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_DRV_SUPPORT, 0x4, wk4 );
	} else if ( type == HFC_DISABLE_DRV_SUPPORT )
	{
		wk4 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_DRV_SUPPORT, 0x4);
		wk4 &= ~status;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_DRV_SUPPORT, 0x4, wk4 );
	}											/* FCLNX-GPL-489 */
	
	return;
}


/*
 * Function:    hfc_read_reg_hg_ext
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  offset     - 
 *  reg_size   - 
 *
 * Returns:     
 *
 * Notes:       
 */
uint64_t hfc_read_reg_hg_ext(
	struct adap_info            *ap,
	uint                        offset,
	char                        reg_size)
{
	uchar *ptr;
	ushort data16=0,rtn16=0;	/* FCLNX-0659 */
	uint   data32=0,rtn=0;
	
	if ( offset % reg_size ) {
		return (0);
	}

	ptr = (uchar *)(ap->hg_mem_base_addr + (ulong)offset);

	switch (reg_size) {
		case 1: 
				rtn = readb( (uchar  *) ptr );
				break;

		case 2: 
				data16 = readw( (ushort *) ptr );
				rtn16 = data16;	 					/* FCLNX-0659 *//* Endian Convert Error */
				HFC_2B_TO_2L(rtn16, data16);		/* FCLNX-0659 */
				rtn = (uint)rtn16;					/* FCLNX-0659 */
				break;

		case 4: 
				data32 = readl( (uint   *) ptr );
				rtn = data32;
				HFC_4B_TO_4L(rtn, data32);
				break;

		default: 
				break;
	}

	
	HFC_DBGPRT("hfc_read_reg_hg_ext: rtn %x\n", rtn);
	return(rtn);
}


/*
 * Function:    hfc_write_reg_hg_ext
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  offset     - 
 *  reg_size   - 
 *  data       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_write_reg_hg_ext(
	struct adap_info            *ap,
	uint                        offset,
	char                        reg_size,
	uint64_t                    data)
{
	uchar   *ptr;
	ushort  data16_1,data16_2=0;
	uint    data32_1,data32_2=0;

	if ( offset % reg_size ) {
		return;
	}
	
	ptr = (uchar *)( ap->hg_mem_base_addr + (ulong)offset);
	
	switch (reg_size) {
		case 1: 
				writeb( (unsigned)data, (uchar  *) ptr ); 
				break;
		case 2: 
				data16_1 = (ushort) data;
				data16_2 = data16_1;
				HFC_2B_TO_2L(data16_2, data16_1);
				writew( (unsigned)data16_2, (uchar  *) ptr );
				break;
		case 4: 
				data32_1 = (uint) data;
				data32_2 = data32_1;
				HFC_4B_TO_4L(data32_2, data32_1);

				writel( (unsigned)data32_2, (uchar  *) ptr );
				break;

		default: 
				break;
	}
	
	HFC_DBGPRT("hfc_write_reg_hg_ext end \n");
	return;
}


/*
 * Function:    hfc_mlpf_pci_error
 *
 * Purpose:     It happens as follows, the following functions are called with 
 *              fc_abend() for Guest LPAR, and it comes off
 *              case HFC_ABEND_SERR:
 *              case HFC_ABEND_PERR:
 *              case HFC_ABEND_SPERR:
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_pci_error(
	struct adap_info            *ap,
	uchar                       type)
{
	uint                    err_no=0 ;
	uchar                   data[16];
	
	memset(data, 0, 16);
	
	if( type == HFC_ABEND_SERR )
		err_no = 0x00000032 ;
	
	if( type == HFC_ABEND_PERR )
		err_no = 0x00000033 ;
	
	if( type == HFC_ABEND_SPERR )
		err_no = 0x00000034 ;
	
	hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERRD,err_no,data,16) ;
	
	return;
}


/*
 * Function:    hfc_mlpf_mck_recovery
 *
 * Purpose:     It is not in hfc_mck_recovery_five at mck and calls it as follows for shadow
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_mck_recovery(
	struct adap_info            *ap,
	uchar                        type)
{
	hfc_mck_recovery_five(ap,type);
	
	hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, HFC_MLPF_REC_END );
	
	set_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status);
	
}


/*
 * Function:    hfc_mlpf_intr
 *
 * Purpose:     The following functions are called at the time of sharing after int_a bit[0 to 3] is checked
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  int_a_reg  - 
 *
 * Returns:     
 *
 * Notes:       
 */
uchar hfc_mlpf_intr(
	struct adap_info            *ap,
	uint                        int_a_reg)
{
	uint                        int_a_rst;
	uint                        hyp_status;
	uint                        hyp_int_detail=0; /* FCLNX-GPL-427 */
	uint                        hyp_support=0;    /* FCLNX-GPL-427 */
	
	if( int_a_reg & HFC_MLPF_HWERR )
	{
		HFC_DBGPRT("hfcldd : hfcl_intr - HFC_MLPF_HWERR bit 23 occured\n");
		
		/* read HYPER status before reset int_a_reg.     */
		hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
		
		/* FCLNX-GPL-427 */
		hyp_support = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 4);
		if ( hyp_support & HFC_HG_LPAR_ISOLATION_SUPPORT ) { /* read and write(clear) HYPER Interrupt Detail */
			hyp_int_detail = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPINTDETAIL, 0x4);
			hfc_write_hg_reg(ap, HFC_IOHGSPC_HYPINTDETAIL, 0x4, hyp_int_detail);
		}
		
		/* reset PCI space for HW_MLPF_HWERR			 */
		int_a_rst = int_a_reg & HFC_MLPF_HWERR;
		hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);
		
		/* FCLNX-GPL-427 */
		if ( hyp_support & HFC_HG_LPAR_ISOLATION_SUPPORT ) {
			if( ( int_a_reg & HFC_MLPF_REC_END ) && (HFC_MMODE_CHECK_SHADOW(ap) ) ){
				if(hyp_int_detail & (HFC_HG_HYPINTDET_FMCK | HFC_HG_HYPINTDET_FCSTP | HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD | HFC_HG_HYPINTDET_RCV_ISOL)){
					int_a_rst = int_a_reg & HFC_MLPF_REC_END;
					hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);
				}
				else if(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL){
					int_a_rst = int_a_reg & HFC_MLPF_REC_END;
					hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);

					hfc_mlpf_mckend_int(ap);
				}
			}
			hfc_mlpf_hwerr_int_detail(ap, hyp_status, hyp_int_detail);
		} else {
			hfc_mlpf_hwerr_int(ap, hyp_status);
		}
		
		hfc_hand2_trace(HFC_TRC_MLPF_INT, 0x10, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
		
		return(1);
	} else if( ( int_a_reg & HFC_MLPF_REC_END ) && (HFC_MMODE_CHECK_SHADOW(ap) ) )
	{                                              // Guest LPAR is not set this bit
		HFC_DBGPRT("hfcldd : hfcl_intr - HFC_MLPF_HWERR bit 22 occured\n");
		
		/* reset PCI space for HW_MLPF_HWERR			 */
		int_a_rst = int_a_reg & HFC_MLPF_REC_END;
		hfc_issue_int_a_rst(ap, int_a_rst, int_a_reg);

		hfc_mlpf_mckend_int(ap);
		
		hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);	/* FCLNX-GPL-403 */

		hfc_hand2_trace(HFC_TRC_MLPF_INT, 0x11, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */

		return(1);
	}
	
	return(0);
}


/*
 * Function:    hfc_mlpf_hwerr_int
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  hyp_status - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_hwerr_int(
	struct adap_info     *ap,
	unsigned int         hyp_status)
{
	struct mp_adap_info *mpap;	/* FCLNX-GPL-393 */
	
	mpap = ap->mp_adap_info;	/* FCLNX-GPL-393 */
	
	if( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		// check Force CSTP INT
		if( hyp_status & ( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML ) )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x01, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
			hfc_mlpf_change_state(
				ap, 
				( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML ),
				HFC_DISABLE_HYPER_STATE );
			hfc_w_stop(ap, HFC_MLPF_FCSTP_TMR) ;
			clear_bit(HFC_MLPF_WAIT_FCSTP, (ulong *)&ap->status );
			
			if(  hyp_status &  HFC_HG_HYPSTATUS_FCSTP )
				hfc_mlpf_fcstp_int(ap, HFC_ABEND_FCSTP);
			else if ( hyp_status & HFC_HG_HYPSTATUS_FCSTP_IML )
				hfc_mlpf_fcstp_int(ap, HFC_ABEND_FCSTP_IML);
			
			return;
		}
		// check FMCK INT
		else if( hyp_status & HFC_HG_HYPSTATUS_FMCK )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x05, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_FMCK, HFC_DISABLE_HYPER_STATE );
			hfc_w_stop(ap, HFC_MLPF_FMCK_TMR) ;
			clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
				
			hfc_issue_forced_mck(ap, HFC_ABEND_T3);
			
			return;
		}
	}
	if( HFC_MMODE_CHECK_SHARED(ap) && !( HFC_MMODE_CHECK_SHADOW(ap) ) )
	{
		// check CSTP END
		if( hyp_status & HFC_HG_HYPSTATUS_CSTPEND )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x11, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_CSTPEND, HFC_DISABLE_HYPER_STATE );
			hfc_w_stop(ap, HFC_MLPF_MCKEND_TMR) ;
			clear_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status );
			ap->wait_isol = 0x00;	/* FCLNX-GPL-413 */
			
			hfc_mlpf_cstpend_int(ap);
			
			return;
		}
		// check MCK INT(Hyper)
		else if(hyp_status & HFC_HG_HYPSTATUS_MCK )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x15, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
#if 0 /* FCLNX-GPL-427 */
			if( test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) )
			{
				clear_bit( HFC_WAIT_T3, (ulong *)&ap->status );
			}
			clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_MCK, HFC_DISABLE_HYPER_STATE );
			set_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
			hfc_mlpf_mck_recovery_glpar(ap);
			hfc_w_stop(ap, HFC_MLPF_MCKEND_TMR) ;
			hfc_w_start(ap, HFC_MLPF_MCKEND_TMR) ;
#else
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_MCK, HFC_DISABLE_HYPER_STATE );
			hfc_mlpf_mck_recovery_glpar(ap);
#endif
			return;
		}
		// mck rec INT
		else if( !(hyp_status & HFC_HG_HYPSTATUS_MCK ) && 
				!(hyp_status & HFC_HG_HYPSTATUS_FCSTP) &&
				!(hyp_status & HFC_HG_HYPSTATUS_FMCK ) )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x16, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
#if 0 /* FCLNX-GPL-427 */
			hfc_w_stop(ap, HFC_MLPF_MCKEND_TMR) ;
			clear_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status );
#endif
			hfc_mlpf_mckend_int_glpar(ap);
			
			return;
		}
		// check Force CSTP INT
		else if( hyp_status & HFC_HG_HYPSTATUS_FCSTP)
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x17, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_FCSTP, HFC_DISABLE_HYPER_STATE );
			
			//hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, HFC_MLPF_HWERR );
			
			return;
		}
		// check FMCK INT
		else if( hyp_status & HFC_HG_HYPSTATUS_FMCK )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT, 0x18, ap, NULL, NULL, hyp_status, 0, 0);	/* FCLNX-GPL-393 */
			
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_FMCK, HFC_DISABLE_HYPER_STATE );
			clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
			set_bit( HFC_WAIT_T3, (ulong *)&ap->status );		/* FCLNX-GPL-0320 */
			return;
		}
	}
	
	/* other factor should not happen because int mask is disabled */
	hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,
		0xb1,(uchar*)&hyp_status,4);	/* FCLNX-GPL-423 */
	
	HFC_DBGPRT(" hfcldd : hfc_hwerr_int - end\n");
	return;
}


/*
 * Function:    hfc_mlpf_hwerr_int_detail
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  hyp_status - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_hwerr_int_detail(
	struct adap_info     *ap,
	unsigned int         hyp_status,
	unsigned int         hyp_int_detail)
{
	struct mp_adap_info *mpap;	/* FCLNX-GPL-393 */
	int skip = 0, fw_start = 0, cnt = 0;
	uint hyp_condition = 0;
	uint *hypint_priority = NULL;
	
	uint hypint_priority_normal[16] = { /* HyperCondition(Normal) and HypIntDetail(RecvIsol=0) */
		HFC_HG_HYPINTDET_FMCK, HFC_HG_HYPINTDET_FCSTP, HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_MCK_END,
		(HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), HFC_HG_HYPINTDET_F_ISOL_END,
		HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END, 0, 0, 0, 0, 0, 0, 0, 0
	};
	uint hypint_priority_normal_isolrcv[16] = { /* HyperCondition(Normal) and HypIntDetail(RecvIsol=1) */
		HFC_HG_HYPINTDET_F_ISOL_END, HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END,
		HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_MCK_END,
		(HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	uint hypint_priority_mck[16] = { /* HyperCondition(MCK) */
		HFC_HG_HYPINTDET_MCK_END, (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), 
		HFC_HG_HYPINTDET_F_ISOL_END, HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END,
		HFC_HG_HYPINTDET_MCK, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	uint hypint_priority_isol[16] = { /* HyperCondition(ISOL/WaitISOL/WaitRcvISOL) */
		HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_MCK_END, 
		(HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), 
		HFC_HG_HYPINTDET_F_ISOL_END, HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	mpap = ap->mp_adap_info;	/* FCLNX-GPL-393 */
	
	if( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		// check Force CSTP INT
		if( hyp_int_detail & HFC_HG_HYPINTDET_FCSTP)
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x01, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
			
			hfc_mlpf_change_state(
				ap, 
				( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML ),
				HFC_DISABLE_HYPER_STATE );
			hfc_w_stop(ap, HFC_MLPF_FCSTP_TMR) ;
			clear_bit(HFC_MLPF_WAIT_FCSTP, (ulong *)&ap->status );
			
			if(  hyp_status &  HFC_HG_HYPSTATUS_FCSTP )
				hfc_mlpf_fcstp_int(ap, HFC_ABEND_FCSTP);
			else if ( hyp_status & HFC_HG_HYPSTATUS_FCSTP_IML )
				hfc_mlpf_fcstp_int(ap, HFC_ABEND_FCSTP_IML);
			
			return;
		}
		// check FMCK INT
		else if( hyp_int_detail &  HFC_HG_HYPINTDET_FMCK )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x05, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
			
			if((test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status)) ||
			(test_bit(HFC_ISOL, (ulong *)&ap->status)) ||
			(test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status))){
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,
				0xb1,(uchar*)&hyp_status,4);	/* FCLNX-GPL-423 */
				return;
			}
			
			if(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL){
				ap->c_err = HFC_ISOLATE_SHADOW;
				set_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol);
			}
			if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLCMD ){
				set_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol);
			}
			if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLERR ){
				set_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol);
			}
			
			hfc_w_stop(ap, HFC_MLPF_FMCK_TMR) ;
			clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
				
			hfc_issue_forced_mck(ap, HFC_ABEND_T3);
			
			return;
		}
		// check Force Isolate INT	/* FCLNX-GPL-393 */
		else if ( hyp_int_detail & ( HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD ) )
		{
			
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x04, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
			
			if((hyp_int_detail &  HFC_HG_HYPINTDET_F_ISOLERR) && 
			  ((test_bit(HFC_ISOL, (ulong *)&ap->status))||(test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status)))){	/* FCLNX-GPL-433 */
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,
				0xb1,(uchar*)&hyp_status,4);	/* FCLNX-GPL-423 */
				return;
			}
			
			if((test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol))||(test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol))){
				/* Force Isolate Interrupt is not executed in running force isolate process. */
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x06, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
				return;
			}
			
			if( hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL ){
				hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
			}
			
			ap->c_err = HFC_ISOLATE_SHADOW;		/* FCLNX-GPL-426 */
			if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLCMD ){
				set_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol);
			}
			else if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLERR ){
				set_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol);
			}
			
			if((!test_bit(HFC_WAIT_T3, (ulong *)&ap->status)) &&
			(!test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status)) &&
			(!test_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol))){	/* FCLNX-GPL-432 */
				hfc_mlpf_isol_start_slpar(ap, hyp_status);	/* FCLNX-GPL-426 */
			}
			
			return;
		}							/* FCLNX-GPL-393 */
		//check Recovery Isolate port	/* FCLNX-GPL-393 */
		else if ( hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x03, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
			
			if(test_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol)){
				/* Recovry Isolate Interrupt is not executed in running recovery isolate process. */
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x07, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
				return;
			}
			
			if(!test_bit(HFC_ISOL, (ulong *)&ap->status)){
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x08, ap, NULL, NULL, hyp_status, hyp_int_detail, ap->wait_isol);
				hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
				return;
			}
			
			ap->c_err = HFC_ISOLATE_SHADOW;
			set_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol);
			
			if((!test_bit(HFC_WAIT_T3, (ulong *)&ap->status)) &&
			(!test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status)) &&
			(!test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol)) &&
			(!test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol))){
				hfc_mlpf_isol_recovery_start_slpar(ap, hyp_status);	/* FCLNX-GPL-426 */
			}
			return;
		}	/* FCLNX-GPL-393*/

	}
	
	if ( HFC_MMODE_CHECK_SHARED(ap) && !( HFC_MMODE_CHECK_SHADOW(ap) ) )
	{
		// Check HyperIntDetail bit
		if ( hyp_int_detail & (~HFC_HG_HYPINTDET_GUEST_SUPPORT) )	/* unsupported interrupt */
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x30, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
			if (ap->debug_func & HFC_DEBUG_HYP_INT_CHK) {
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xb1,(uchar*)&hyp_int_detail,4);	/* FCLNX-GPL-423 */
			}
			hyp_int_detail &= HFC_HG_HYPINTDET_GUEST_SUPPORT; /* exclude unsupport interrupt */
		}
		else if ( hyp_int_detail == 0 ) {
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x31, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
			if (ap->debug_func & HFC_DEBUG_HYP_INT_CHK) {
				hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xb1,(uchar*)&hyp_int_detail,4);	/* FCLNX-GPL-423 */
			}
		}
		
		// CSTP END
		if ( hyp_int_detail & HFC_HG_HYPINTDET_CSTP_END )
		{
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x10, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
			
			hfc_w_stop(ap, HFC_MLPF_MCKEND_TMR) ;
			clear_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status );
			hfc_mlpf_cstpend_int(ap);
			return;
		}
		
		// FCLNX-GPL-489 Live Migration
		if ( hyp_int_detail & (HFC_HG_HYPINTDET_MIG_END | HFC_HG_HYPINTDET_MIG_RCV) )
		{
			if ( hyp_int_detail & HFC_HG_HYPINTDET_MIG_END) {
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x21, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				hfc_mlpf_migration_end(ap);
			}
			if ( hyp_int_detail & HFC_HG_HYPINTDET_MIG_RCV) {
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x22, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				hfc_mlpf_migration_recovery(ap, hyp_status);
			}
			return;
		}
		
		/* HyperCondition from HyperStatus */
		hyp_condition = hfc_mlpf_check_hypcondition(hyp_status);
		
		/* The interrupt processing priority level is decided by HyperCondition */
		switch ( hyp_condition )
		{
		case HFC_HYPCONDITION_MCK :
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x11, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
			hypint_priority = hypint_priority_mck;
			break;
		case HFC_HYPCONDITION_WAIT_ISOL : 
		case HFC_HYPCONDITION_WAIT_ISOLRCV :
		case HFC_HYPCONDITION_ISOL:
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x12, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
			hypint_priority = hypint_priority_isol;
			break;
		case HFC_HYPCONDITION_NORMAL :
			if ( !(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL_END) ) {
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x13, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				hypint_priority = hypint_priority_normal;
			} else {
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x14, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				hypint_priority = hypint_priority_normal_isolrcv;
			}
			break;
		case HFC_HYPCONDITION_CSTP:
		default:
			hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x15, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
			return;
		}
		
		for (cnt = 0; (cnt < 16) && (hypint_priority[cnt] != 0) ; cnt++)
		{
			if ( !(hyp_int_detail & hypint_priority[cnt]) ) {
				continue;
			}
			
			switch ( hypint_priority[cnt] )
			{
			case (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD) : // Isolate Start
				if ( !(hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) ) {
					hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x16, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
					hfc_mlpf_isol_start_glpar(ap, hyp_status);
				}
				break;
			case HFC_HG_HYPINTDET_F_ISOL_END : // Isolate End
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x17, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				hfc_mlpf_isol_end_glpar(ap, hyp_status);
				break;
			case HFC_HG_HYPINTDET_RCV_ISOL : // Recovery Isolate Start
				skip = 0;
				if ( hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL_END ) {
					skip = 1;
				}
				if ( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END ) {
					skip = 1;
				}
				if ( (hyp_int_detail & (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD)) &&
				    !(hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) ) 
				{
					skip = 1;
				}
				if (!skip) {
					hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x18, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
					hfc_mlpf_isol_recovery_start_glpar(ap, hyp_status);
				}
				break;
			case HFC_HG_HYPINTDET_RCV_ISOL_END : // Recovery Isolate End
				skip = 0;
				fw_start = 1;
				if ( (hyp_int_detail & (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD)) &&
				    !(hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) ) 
				{
					skip = 1;
				}
				if ( (hypint_priority == hypint_priority_isol) &&       /* HyperStatus(Isol) */
				     (hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) )
				{
					skip = 1;
				}
				if (!skip) {
					hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x19, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
					if (hypint_priority == hypint_priority_normal_isolrcv) {
						if (hyp_int_detail & HFC_HG_HYPINTDET_MCK_END) {
							fw_start = 0;
						}
					}
					hfc_mlpf_isol_recovery_end_glpar(ap, hyp_status, fw_start);
				}
				break;
			case HFC_HG_HYPINTDET_MCK : // MCK rec Start
				skip = 0;
				if ( hypint_priority == hypint_priority_mck )
				{
					if ( (hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) &&
					     !(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL_END) )
					{
							skip =1;
					}
				}
				if (!skip) {
					hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x1a, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
					hfc_mlpf_mck_recovery_glpar(ap);	
				}
				
				break;
			case HFC_HG_HYPINTDET_MCK_END : // MCK rec End
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x1d, ap, NULL, NULL, hyp_status, 0, 0);
				hfc_mlpf_mckend_int_glpar(ap);
				break;
			case HFC_HG_HYPINTDET_FMCK : // FMCK INT
				if ( (hyp_int_detail & ~HFC_HG_HYPINTDET_FCSTP ) == HFC_HG_HYPINTDET_FMCK ) { /* only FMCK bit (exclude F-CSTP) */
					hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x1e, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
					clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
					set_bit( HFC_WAIT_T3, (ulong *)&ap->status );		/* FCLNX-GPL-0320 */
				}
				break;
			case HFC_HG_HYPINTDET_FCSTP : // Force CSTP INT
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x1f, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				break;
			default : 
				hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x20, ap, NULL, NULL, hyp_status, hyp_int_detail, 0);
				break;
			} /* switch */
		} /* for */
	}
	
	HFC_DBGPRT(" hfcldd : hfc_hwerr_int_detail - end\n");
	return;
}


/*
 * Function:    hfc_mlpf_mck_recovery_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_mck_recovery_glpar(
	struct adap_info            *ap)
{
	struct target_info          *target;
	uint                        lp;
	struct mp_adap_info			*mpap; /* FCLNX-GPL-177 */
	struct dev_info				*dev=NULL;	/* FCLNX-GPL-0343 */
	
/* FCLNX-GPL-427 */
	if( test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) )
	{
		clear_bit( HFC_WAIT_T3, (ulong *)&ap->status );
	}
	clear_bit(HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );
	hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_MCK, HFC_DISABLE_HYPER_STATE );
	set_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
/* FCLNX-GPL-427 */

	HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY); 
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)						
	{
		target = hfc_hash_target_info(ap, lp);
		if (target != NULL)
		{
			hfc_cancel_scsi_cmd(
				ap, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,		/* FCLNX-0429 */
				TRUE, TRUE, HFC_FLASH_TARGET);
			target->status = HFC_NON_STATUS ; 
			
			dev = target->dev;										/* FCLNX-GPL-0343 */
			while( dev != NULL ){
				dev->lustat = 0x00;
				dev = dev->next;
			}														/* FCLNX-GPL-0343 */
//			for (lun=0;lun<MAX_DEV_CNT;lun++)
//				target->lustat[lun] = 0x00;
			
			if ( hfc_manage_info.hfcldd_mp_mod ) {
				hfc_manage_info.npubp->hfc_forced_offline_e(target, TRUE);
			}
		}
	}
	
	mpap = ap->mp_adap_info;  /* FCLNX-GPL-177 start */
	
	if( !test_and_set_bit(HFC_HMCK_RECOVRTY, (ulong *)&mpap->lock) )
	{	/* for Master Port only */
		mpap->mck_err_cnt++ ;	/* FCLNX-GPL-057 */
	}  /* FCLNX-GPL-177 end */
	
	hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )HFC_MLPF_HWERR );
	hfc_reset_watchdog(ap);
	
	return;
}


/*
 * Function:    hfc_mlpf_mckend_int
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_mckend_int(
	struct adap_info            *ap)
{
	hfc_w_stop(ap, HFC_MLPF_MCKEND_TMR) ;
	clear_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status );
	
	set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
	hfc_w_stop( ap, HFC_LINKUP2_TMR );
	hfc_w_start( ap, HFC_LINKUP2_TMR );
}


/*
 * Function:    hfc_mlpf_mckend_int_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_mckend_int_glpar(
	struct adap_info            *ap)
{
/* FCLNX-GPL-427 */
	hfc_w_stop(ap, HFC_MLPF_MCKEND_TMR) ;
	clear_bit(HFC_MLPF_WAIT_MCKEND, (ulong *)&ap->status );
/* FCLNX-GPL-427 */

	hfc_mlpf_errlog_glpar(ap);
	
	hfc_reset_adap_info(ap);
	
	if ( hfc_mlpf_check_normal_hypsts(ap) ) { /* FCLNX-GPL-427 HyperStatus Normal, execute F/W start */

		/* WS Comu Area(0x318) */
		hfc_reset_start(ap, HFC_SET_WS80);
		/* INITTBL ADR(0x310)  */
		/* ALPA(0x319)         */
		hfc_reset_start(ap, HFC_SET_INIADR);
		
		hfc_reset_start(ap, HFC_FW_START); 
		
		ap->mck_linkup = HFC_LINKUP_MCK;			/* FCLNX-GPL-393 */
		ap->isol_err_mck_cnt = (uchar)ap->mp_adap_info->mck_err_cnt;		/* FCLNX-GPL-393 */	

		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		test_and_clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&ap->mp_adap_info->lock); /* FCLNX-GPL-177 */
	
		if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
			if(ap->mck_on_sleep) {
				hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);
			}
		} 
	
		/* Interruption mask setting */
		hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask_mlpf[ap->pkg.type]);
	
		/* Link Up Timer Set */
		//@MLPF
		set_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
	
		hfc_w_stop( ap, HFC_LINKUP2_TMR );
		hfc_w_start( ap, HFC_LINKUP2_TMR );
	}
	else { /* FCLNX-GPL-427 HyperStatus NOT Normal */
		ap->mck_linkup = HFC_LINKUP_MCK;			/* FCLNX-GPL-393 */
		ap->isol_err_mck_cnt = (uchar)ap->mp_adap_info->mck_err_cnt;		/* FCLNX-GPL-393 */
		
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		test_and_clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&ap->mp_adap_info->lock); /* FCLNX-GPL-177 */
	}
	
	if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
	{
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
	}
	
	return;
}


/*
 * Function:    hfc_mlpf_isol_start_slpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_isol_start_slpar(
	struct adap_info     *ap,
	unsigned int         hyp_status)
{
	if(hyp_status & HFC_HG_HYPSTATUS_ISOLCMD){
		ap->isol_detail = HFC_ISOLATE_PORT_C;
	}
	else{
		ap->isol_detail = HFC_ISOLATE_PORT_E;
	}

	ap->c_err = 0x00;
	
	if(hfc_force_linkdown(ap, FALSE, FALSE)){
		hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_END);     //@MLPF
		clear_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol);
		clear_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol);
		return;
	}
	
	if(ap->pkg.port <= 1){
		hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_END);     //@MLPF
		clear_bit(HFC_WAIT_ISOL_ERR, (ulong *)&ap->wait_isol);
		clear_bit(HFC_WAIT_ISOL_CMD, (ulong *)&ap->wait_isol);
	}
			
	hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_ENABLE, HFC_DISABLE_HYPER_STATE );   /* FCLNX-0393 */
	hfc_mlpf_indacc(ap);

	return;
}
 
 

/*
 * Function:    hfc_mlpf_isol_start_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_isol_start_glpar(
	struct adap_info     *ap,
	unsigned int         hyp_status)
{
	hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )(HFC_MLPF_REC_END | HFC_MLPF_HWERR) );
	
	if (hyp_status & HFC_HG_HYPSTATUS_ISOLCMD){
		ap->isol_detail = HFC_ISOLATE_PORT_C;
	}
	else {
		ap->isol_detail = HFC_ISOLATE_PORT_E;
	}
	hfc_reset_all_timer(ap);
	set_bit(HFC_ISOL, (ulong *)&ap->status);
	
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_MLPF_ISOLEND_TMR, 0, TRUE);
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_MLPF_ISOLEND_TMR, 0, FALSE);
}

/*
 * Function:    hfc_mlpf_isol_end_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_isol_end_glpar(
	struct adap_info            *ap,
	unsigned int		hyp_status)
{
	// Guest LPAR only
	uint                        lp;
	struct target_info			*target;
	uchar					logdata[16];
	
	hfc_mlpf_isol_start_glpar(ap, hyp_status); /* FCLNX-GPL-427 */
	
	hfc_watchdog_enter(ap, NULL, NULL, 0, HFC_MLPF_ISOLEND_TMR, 0, TRUE);
	
	memset(logdata, 0, 16);
	logdata[0]=ap->c_err;
	logdata[1]=ap->isol_detail;
	
	if (ap->isol_detail == HFC_ISOLATE_PORT_C){
		hfc_errlog(
				ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
				ERRID_HFCP_EVNT2, 0xD4, logdata, 16);

		HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
				ap->pci_cfginf->bus->number,
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn));
	}
	else{
		hfc_errlog(
				ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
				ERRID_HFCP_EVNT2, 0xD5, logdata, 16);
		
		HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by error \n",
				ap->pci_cfginf->bus->number,
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn));
	}
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target = hfc_hash_target_valid(ap, lp);		/* FCLNX-703 *//* FCLNX-GPL-433 */
		if (target != NULL)
		{
			if ( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) ){	/* FCLNX-703 */
				hfc_notify_tout(ap, target);	/* FCLNX-GPL-573 output SCSI time-out log. */
				hfc_cancel_scsi_cmd(
					ap, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,
							TRUE, TRUE, HFC_FLASH_TARGET);
				target->status = HFC_NON_STATUS ;
			}
			
			if ( hfc_manage_info.hfcldd_mp_mod ) {
				if ((ap->isol_detail == HFC_ISOLATE_PORT_C)
				 || (ap->isol_detail == HFC_ISOLATE_CHKSTP_C)) {
					hfc_manage_info.npubp->hfc_forced_offline_c(target, TRUE);
				}
				hfc_manage_info.npubp->hfc_forced_offline_e(target, TRUE);	/* FCLNX-704 */
			}
		}														/* FCLNX-GPL-433 */
	}
	hfc_wwnverify_linkup_timeout(ap, NULL, 0);		/* FCLNX-GPL-433 */
	
	HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY );
	
	ap->status = 0;          /* FCLNX-GPL-427 *//* FCLNX-GPL-572 */
	set_bit(HFC_ENABLE,  (ulong *)&ap->status);	/* FCLNX-GPL-572 */
	set_bit(HFC_ISOL, (ulong *)&ap->status);	/* FCLNX-GPL-572 */
	
	if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
		if(ap->mck_on_sleep) {
			hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);	
		}
	}
//	if(ap->isol_detail == HFC_ISOLATE_PORT_C){
//		hfc_wake_up(&ap->ioctl_event, &ap->ioctl_event_wait);
//	}
	if(ap->initialize == 1) {                             /* FCLNX-GPL-431 */
		hfc_wake_up(&ap->init_event,&ap->int_a_poll); /* FCLNX-GPL-431 */
	}
	
	return;
}


/*
 * Function:    hfc_mlpf_isol_recovery_start_slpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_isol_recovery_start_slpar(
	struct adap_info	*ap,
	unsigned int		hyp_status)
{
	ap->c_err = 0x00;

	if ( test_bit(HFC_HWISOL, (ulong *)&ap->mp_adap_info->status) ) { /* FCLNX-556 */
		hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x02, ap, NULL, NULL, hyp_status, 0, 0);
		hfc_force_linkdown_recovery(ap);
	}
	else if(test_bit(HFC_ISOL, (ulong *)&ap->status) ) {
	
		hfc_hand2_trace(HFC_TRC_MLPF_HWERR_INT_DET, 0x03, ap, NULL, NULL, hyp_status, 0, 0);
		/* port recovery FRAME_A is not supported */
		if ( !(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))){
			hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
			clear_bit(HFC_WAIT_ISOL_REC, (ulong *)&ap->wait_isol);
			return;
		}
		/* call hfc_force_linkdown_port_recovery */
		hfc_force_linkdown_recovery_port(ap);	/* FCLNX-GPL-402 */
				
	}
	
	hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_LINKDOWN, HFC_ENABLE_LPAR_STATE);	/* FCLNX-GPL-393 */
	hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_UNSHARABLE, HFC_DISABLE_LPAR_STATE);	/* FCLNX-GPL-393 */
	hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_SPACE, HFC_DISABLE_LPAR_STATE);		/* FCLNX-GPL-489 */
	
	return;
}

/*
 * Function:    hfc_mlpf_isol_recovery_start_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_isol_recovery_start_glpar(
	struct adap_info	*ap,
	unsigned int		hyp_status)
{
	if(hyp_status & HFC_HG_HYPSTATUS_MCK){
		hfc_reset_adap_info(ap);
	}
			
	if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
		hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
	}														/* FCLNX-GPL-331 */
	else{
		hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
	}

	set_bit ( HFC_ISOL_RECOVERY, (ulong *)&ap->status);
	
	return;
}

/*
 * Function:    hfc_mlpf_isol_recovery_end_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
 
#define HFC_ISSUE_FW_START		1
#define HFC_SKIP_FW_START		2
 
void hfc_mlpf_isol_recovery_end_glpar(
	struct adap_info            *ap,
	unsigned int		hyp_status,
	int					fw_start)
{
	struct mp_adap_info		*mpap;
	uchar					fwstart_log = HFC_ISSUE_FW_START;
	unsigned long long		wkp;		/* FCLNX-GPL-490 */
	int						dma_size;	/* FCLNX-GPL-490 */
	
/* FCLNX-GPL-427 */
	hfc_mlpf_isol_recovery_start_glpar(ap, hyp_status);

	clear_bit ( HFC_ISOL_RECOVERY, (ulong *)&ap->status);
	ap->isol_detail = HFC_NO_ISOLATE;
	ap->c_err = 0x00;
		
	if(!test_bit ( HFC_ISOL, (ulong *)&ap->status)){
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x06, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */
		fwstart_log = HFC_SKIP_FW_START;
		goto hfc_skip_isol_recovery;
	}
	
	hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x05, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */

	
	clear_bit ( HFC_ISOL, (ulong *)&ap->status);

/* FCLNX-GPL-427 */

	mpap = ap->mp_adap_info;

#if 0 /* FCLNX-GPL-427 */
	if ( (ap->isol_err_mck_cnt != (uchar)mpap->mck_err_cnt) ||	/* FCLNX-GPL-357 */
	     (ap->isol_cmd_mck_cnt != (uchar)mpap->mck_err_cnt) )
	{
		hfc_reset_adap_info(ap);
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL_REC_P, 0x04, ap, NULL, NULL, 0, 0, 0);	/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */
	}																		/* FCLNX-GPL-357 */
#endif

	if ( fw_start && hfc_mlpf_check_normal_hypsts(ap) ) { /* FCLNX-GPL-427 HyperStatus Normal, execute F/W start */

		/* FCLNX-GPL-490 */
		dma_size = sizeof(dma_addr_t);
		wkp = ap->padr_init;
		if(dma_size == 8){
			wkp >>=32;
		}
		else{
			wkp = 0;
		}
		
		hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR0, 0x4, wkp);				/* FCLNX-GPL-490 */
		hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR1, 0x4, ap->padr_init);	/* FCLNX-GPL-490 */
		
		hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_FW_START);	/* FCLNX-GPL-490 */
		
		ap->isol_err_mck_cnt = (uchar)ap->mp_adap_info->mck_err_cnt;		/* FCLNX-GPL-393 */	

		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		test_and_clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&ap->mp_adap_info->lock); /* FCLNX-GPL-177 */

		/* FCLNX-GPL-490 Start */
		set_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status);
		clear_bit(HFC_ISOL, (ulong *)&ap->status);
		ap->isol_detail = HFC_NO_ISOLATE;
		ap->c_err = 0x00;
		
		clear_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
			clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
#endif	/* FCLNX-GPL-FX-424 */
		
		if ( hfc_issue_linkini(ap) ) {
			fwstart_log = HFC_SKIP_FW_START;							/* FCLNX-GPL-521 */
			goto hfc_skip_isol_recovery;								/* FCLNX-GPL-521 */
		}
		/* FCLNX-GPL-490 End */
		
		/* Interruption mask setting */
		hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, hfc_inta_mask_mlpf[ap->pkg.type]);
		
	}
	else { /* FCLNX-GPL-427 HyperStatus NOT Normal */
		ap->isol_err_mck_cnt = (uchar)mpap->mck_err_cnt;		/* FCLNX-GPL-393 */
		
		clear_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status );
		test_and_clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&ap->mp_adap_info->lock); /* FCLNX-GPL-177 */
		fwstart_log = HFC_SKIP_FW_START;
	}

	if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
	{
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
	}

hfc_skip_isol_recovery:
/* FCLNX-GPL-427 */
	hfc_errlog(
		ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
		ERRID_HFCP_EVNT2, 0xD3, &fwstart_log, 1);
				
	HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is recovered \n",
		ap->pci_cfginf->bus->number,
		PCI_SLOT(ap->pci_cfginf->devfn),
		PCI_FUNC(ap->pci_cfginf->devfn));
/* FCLNX-GPL-427 */

	if( ( fwstart_log == HFC_SKIP_FW_START)&&(ap->initialize != 0) ){							/* FCLNX-GPL-521 */
		hfc_wake_up(&ap->init_event,&ap->int_a_poll);
	}													/* FCLNX-GPL-521 */


	return;
}

/*
 * Function:    hfc_mlpf_check_hypcondition
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   hyper status
 *
 * Returns:     HyperCondition
 *
 * Notes:       
 */
uint hfc_mlpf_check_hypcondition(unsigned int hyp_status)
{
	if (!(hyp_status & HFC_HG_HYPSTATUS_AVAILABLE)) {
		return HFC_HYPCONDITION_CSTP;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_WAIT_ISOL) {
		return HFC_HYPCONDITION_WAIT_ISOL;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_WAIT_RCV_ISOL) {
		return HFC_HYPCONDITION_WAIT_ISOLRCV;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_ISOL) {
		return HFC_HYPCONDITION_ISOL;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_MCK) {
		return HFC_HYPCONDITION_MCK;
	}
	else {
		return HFC_HYPCONDITION_NORMAL;
	}
}

/*
 * Function:    hfc_mlpf_check_normal_hypsts
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     HyperStatus  1:normal 0:not normal 
 *
 * Notes:       
 */
int hfc_mlpf_check_normal_hypsts(
	struct adap_info    *ap)
{
	unsigned int hyp_status;

	if ( !(HFC_MMODE_CHECK_SHARED(ap)) || HFC_MMODE_CHECK_SHADOW(ap) || 	
	     !(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support)) ) {
		return 1;
	}

	hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
	if (hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_NORMAL) {
		return 1;
	} else {
		return 0;
	}
}


/*
 * Function:    hfc_mlpf_fcstp_int
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_fcstp_int(
	struct adap_info            *ap,
	uchar                       type)
{
	
	hfc_chk_stop( ap, TRUE ) ;
	/* FCLNX-GPL-209 */
	
	return;
}


/*
 * Function:    hfc_mlpf_issue_ffcstp
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_issue_ffcstp(
	struct adap_info            *ap,
	uchar                       type)
{
	uint                        int_a_reg;
	
	int_a_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
	
	if( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		if( ( int_a_reg & HFC_MLPF_HWERR ) &&
			( hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML, HFC_CHECK_HYPER_STATE ) ) )
			return;
	}
	else
	{//Guest LPAR nothing is done and return is done
		return;
	}
	
	if ( !test_bit (HFC_CHK_STOP, (ulong *)&ap->status ) )
	{
		set_bit( HFC_MLPF_WAIT_FCSTP, (ulong *)&ap->status );
		
		hfc_w_stop(ap, HFC_MLPF_FCSTP_TMR) ;
		hfc_w_start(ap, HFC_MLPF_FCSTP_TMR) ;
		
		hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, HFC_MLPF_HWERR );
		
		if( type == HFC_ABEND_FCSTP )
			hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_FFCSTP);
		else if ( type == HFC_ABEND_FCSTP_IML )
			hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_FFCSTP_IML);
	}
	
	return;
}


/*
 * Function:    hfc_mlpf_issue_ffmck
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_issue_ffmck(
	struct adap_info            *ap,
	uchar                       type)
{
	struct mp_adap_info         *mpap = ap->mp_adap_info;
	uint                        int_a_reg;
	uint                        wk_reg;
	
	int_a_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
	
	if( HFC_MMODE_CHECK_SHADOW(ap) )
	{
		if( ( int_a_reg & HFC_HWERR_MCK ) ||
			( ( int_a_reg & HFC_MLPF_HWERR ) &&
			( hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FMCK, HFC_CHECK_HYPER_STATE ) ) ) )
			return;
	}
	else
	{   //Guest LPAR
		if( ( (int_a_reg & HFC_MLPF_HWERR ) &&
			( hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_ENABLE | HFC_HG_HYPSTATUS_MCK, HFC_CHECK_HYPER_STATE ) ) ) ||
			( ( int_a_reg & HFC_MLPF_HWERR ) &&
			( hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FMCK, HFC_CHECK_HYPER_STATE ) ) ) )
			return;
	}
	
	if( !test_bit( HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status ) &&
		!test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) &&
		!test_bit( HFC_HWCHKSTOP, (ulong *)&mpap->status ) )
	{
		set_bit( HFC_MLPF_WAIT_FMCK, (ulong *)&ap->status );

		
		if( HFC_MMODE_CHECK_SHADOW(ap) )
		{   // shadow LPAR only start FMCK TMR
			hfc_w_stop(ap, HFC_MLPF_FMCK_TMR) ;
			hfc_w_start(ap, HFC_MLPF_FMCK_TMR) ;
		}
		else
		{   // Guest LPAR set HFC_WAIT_T3
			set_bit( HFC_WAIT_T3, (ulong *)&ap->status );
		}
		
		wk_reg = 0;                                                             /* FCLNX-0389 */
		wk_reg |= type & 0xff;
		wk_reg <<= 16;
		wk_reg |= ap->mb->mb_init.command & 0xff;
		wk_reg <<= 8;
		wk_reg |= ap->mb->mb_init.sub_cmd & 0xff;
		hfc_write_reg(ap,( uint )HFC_IOSPACE_DRV_USED0,( char )0x4, wk_reg);    /* FCLNX-0389 */
		
		hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_FFMCK);
	}
	
	return;
}


/* FCLNX-GPL-393 */

/*
 * Function:    hfc_mlpf_issue_fisolate
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  proc       - 0:Isolate by error, 1:Isolate by command
 *
 * Returns:     
 *
 * Notes:       
 */

int hfc_mlpf_issue_fisolate(
	struct adap_info            *ap,
	uchar                       proc)
{
	struct mp_adap_info			*mpap;
	uint hyp_status;

	hyp_status = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);

	hfc_hand2_trace(HFC_TRC_MLPF_FORCE_ISOL, 0x00, ap, NULL, NULL, hyp_status, 0, 0);
	
	mpap = ap->mp_adap_info;
	
	if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has happen. */
		HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
		return EINVAL;
	}
	
	if (!(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support))) 
	{
		return EINVAL;
	}

	if (!(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))) { /* F/W support */
		return EINVAL;
	}
	
	if (test_bit(HFC_ISOL, (ulong *)&ap->status)) {	/* FCLNX-GPL-414 */
		if ((ap->isol_detail == HFC_ISOLATE_PORT_C)
		|| (ap->isol_detail == HFC_ISOLATE_CHKSTP_C)) {
		hfc_hand2_trace(HFC_TRC_FORCE_ISOL, 0x05, ap, NULL, NULL, 0, 0, 0);
			return 0;
		}
	}/* FCLNX-GPL-414 */
	
	if(	(ap->isol_detail == HFC_ISOLATE_SFPNOTSUPPORT) ||	/* FCLNX-GPL-415 */
		(ap->isol_detail == HFC_ISOLATE_SFPFAIL) ||
		(ap->isol_detail == HFC_ISOLATE_SFPDOWN)){
		return 0;
	}														/* FCLNX-GPL-415 */
	
//	if( ( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FMCK, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FCSTP, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FCSTP_IML, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_ISOLCMD, HFC_CHECK_HYPER_STATE ) ) ){
		
	hfc_hand2_trace(HFC_TRC_MLPF_FORCE_ISOL, 0x01, ap, NULL, NULL, hyp_status, 0, 0);
		
	if(proc == HFC_ISSUE_ISOLREQ_ERR){
		hfc_mlpf_set_errorlimit(ap);    /* FCLNX-GPL-431 */
		hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_ERR);
		//hfc_mlpf_set_errorlimit(ap);  /* FCLNX-GPL-431 */
	}
	else if(proc == HFC_ISSUE_ISOLREQ_CMD){
		hfc_mlpf_set_errorlimit(ap);   /* FCLNX-GPL-431 */
		hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_CMD);
		ap->isol_cmd_mck_cnt = (uchar)mpap->mck_err_cnt; /* FCWIN-GPL-393 */
		ap->isol_detail = HFC_ISOLATE_PORT_C;
		//hfc_mlpf_set_errorlimit(ap); /* FCLNX-GPL-431 */
	}
	return 0;
//	}
//	else{
//		return EINVAL;
//	}
}

/*
 * Function:    hfc_mlpf_issue_recov_isolate
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */

int hfc_mlpf_issue_recov_isolate(
	struct adap_info            *ap)
{
	uint hyp_status;

	hyp_status = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);

	hfc_hand2_trace(HFC_TRC_MLPF_RECV_ISOL, 0x00, ap, NULL, NULL, hyp_status, 0, 0);

	if( hfc_pcibus_chk(ap) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has happen. */
		HFC_ISSUE_CSTP_PCIERR(ap, FALSE);		/* FCLNX-GPL-400 */
		return EINVAL;
	}
	
	if (!(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support)))
	{
		return EINVAL;
	}

	if (!(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))) { /* HVM F/W support */
		return EINVAL;
	}
	
//	if (!(test_bit(HFC_ISOL, (ulong *)&ap->status))) {
//		return EINVAL;
//	}
	
	if (test_bit(HFC_CHK_STOP, (ulong *)&ap->status)) {
		return EINVAL;
	}
	
	if(	(ap->isol_detail == HFC_ISOLATE_SFPNOTSUPPORT) ||
		(ap->isol_detail == HFC_ISOLATE_SFPFAIL) ||
		(ap->isol_detail == HFC_ISOLATE_SFPDOWN)){
		return 0;											/* FCLNX-GPL-414 */
	}
	
//	if( ( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FMCK, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FCSTP, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_FCSTP_IML, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_CSTPEND, HFC_CHECK_HYPER_STATE ) ) &&
//		( !hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_WAIT_RCV_ISOL, HFC_CHECK_HYPER_STATE ) ) ){

		hfc_hand2_trace(HFC_TRC_MLPF_RECV_ISOL, 0x01, ap, NULL, NULL, hyp_status, 0, 0);
	
		hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOLATE);
		return 0;
//	}
//	else{
//		return 0;
//	}
}

/* FCLNX-GPL-393 */


/*
 * Function:    hfc_mlpf_cstpend_int
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_cstpend_int(
	struct adap_info            *ap)
{
	// Guest LPAR only
	uint                        lp;
	struct target_info			*target;
	
	hfc_mlpf_errlog_glpar(ap);
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target = hfc_hash_target_info(ap, lp);
		if( target != NULL )
		{
			hfc_cancel_scsi_cmd(ap,target,0, NULL, SCS_MCK, HFC_CSCSI_ERROR,	/* FCLNX-0429 */
					TRUE, TRUE, HFC_FLASH_TARGET);
			target->status = HFC_NON_STATUS;
			
			if ( hfc_manage_info.hfcldd_mp_mod ) {
				hfc_manage_info.npubp->hfc_forced_offline_e(target, TRUE);
			}
		}
	}
	
	clear_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status);
	if( test_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status)){
		if(ap->mck_on_sleep) {
			hfc_wake_up(&ap->mck_event, &ap->mck_event_wait);	
		}
	} 

	set_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status ) ;	/* FCLNX-GPL-419 */
	set_bit(HFC_CHK_STOP, (ulong *)&ap->status );
	
	if(hfc_mlpf_check_state(ap, HFC_HG_LPAR_LIVEMIG_SUPPORT, HFC_CHECK_HVM_SUPPORT )){	/* FCLNX-GPL-481*/
		hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )(HFC_MLPF_REC_END | HFC_MLPF_HWERR) );
	}
	else{
		hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000000 );
	}																					/* FCLNX-GPL-481*/
	
	clear_bit(HFC_ONLINE, (ulong *)&ap->status );
	
	if(ap->initialize == 1) {                             /* FCLNX-GPL-431 */
		hfc_wake_up(&ap->init_event,&ap->int_a_poll); /* FCLNX-GPL-431 */
	}
	
	if( test_bit(HFC_MB_PROL, (ulong *)&ap->mb_status) )
	{
		hfc_wake_up(&ap->mb_event, &ap->mb_event_wait);
	}
	
	return;
}



/*
 * Function:    hfc_mlpf_errlog_slpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_errlog_slpar(
	struct adap_info        *ap)
{
	struct mp_adap_info     *mpap;
	int                     rc;
	
	mpap = ap->mp_adap_info;
	
	rc = lock_try_mpap( mpap );	
	
	if( rc == 0 ){		/* fail to aquire lock */
		set_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}
	
	if( test_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock ) )
	{
		/* stop retry timer */
		clear_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
	}
	
	if ( ! hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_MCKLOG, HFC_CHECK_HYPER_STATE ) )
	{
		hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_MCKLOG,HFC_ENABLE_HYPER_STATE );
		
		hfc_mlpf_indacc(ap);
		
		hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_MCKLOG,HFC_DISABLE_HYPER_STATE );
	}
	
	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
}


/*
 * Function:    hfc_mlpf_errlog_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_errlog_glpar(
	struct adap_info        *ap)
{
	struct mp_adap_info     *mpap;
	struct hfc_err_rec		*err_rec;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	uchar                   data[16];
	int                     rc;
	int                     err_no=0;
	int                     read_reg = 0 ;	/* FCLNX_GPL-472 */
	
	mpap = ap->mp_adap_info;
	
	rc = lock_try_mpap( mpap );	
	
	if( rc == 0 ){		/* fail to aquire lock */
		set_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
		hfc_w_start( ap, HFC_MPAP_LOCK_TMR );
		return;
	}
	
	if( test_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock ) )
	{
		/* stop retry timer */
		clear_bit( HFC_LOCK_WAIT_6, (ulong *)&ap->mpap_lock );
		hfc_w_stop( ap, HFC_MPAP_LOCK_TMR );
	}
	
	hfc_mlpf_indacc(ap);
	
//	memset(&err_rec, 0, sizeof(struct hfc_err_rec));			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
//	HFC_MEMCPY( (uchar *)&err_rec, ap->mlpf_drv_log, 0x40);		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	err_rec = (struct hfc_err_rec *) ap->mlpf_drv_log;			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	
	HFC_4L_TO_4B(err_no, err_rec->log_area[0]);					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	HFC_MEMCPY( data, &err_rec->log_area[48], 16);				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	
	read_reg = hfc_read_reg_ext( ap,(uint)0, (char)0x4) ;		/* FCLNX_GPL-472 */

	if( ( err_no == 0x00000035 ) ||
		( err_no == 0x00000036 ) ||
		( err_no == 0x00000071 ) ) /* FCLNX-GPL-423 */
	{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, err_no, data,16) ;
	}
	else if ( ( err_no == 0x00000077 ) ||
		( err_no == 0x00000078 ) )
	{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRC, err_no, data,16) ;
	}
	else if ( err_no == 0x00000031 ) /* FCLNX-GPL-423 */
	{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_CHKSTP,ERRID_HFCP_ERR1, err_no,data,16) ;
	}
	else if( err_no == 0x0000002b )	/* FCLNX-GPL-423 */
	{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR2, err_no, data,16) ;
	}
	else if( err_no == 0x00000000 )	/* FCLNX-GPL-472 */
	{
		if( read_reg == 0xffffffff )
		{
			hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_ERR1,
													0x31,data,16) ;
		}
	}								/* FCLNX-GPL-472 */
	else{
		hfc_errlog(
			ap,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR4, err_no,data,16) ;
	}
	
	HFC_ADAP_UNLOCK(mpap, HFC_MP_ADAP_BUSY);
}


/*
 * Function:    hfc_mlpf_indacc
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_mlpf_indacc(
	struct adap_info        *ap)
{
	uint                    flag;
	uchar                   *log_area;
	struct mp_adap_info     *mpap;
	uint					reg_addr=0;		/* FCLNX-GPL-262 */
	uint					type=0;			/* FCLNX-GPL-262 */
	
	mpap = ap->mp_adap_info;
	
	log_area = (uchar *)mpap->hw_log;
	
	flag = hfc_read_hg_reg(ap, HFC_IOHGSPC_INDACC0, 4);
	if ( flag & HFC_MLPF_INDACC_FLG )
	{
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
		return HFC_MLPF_INDACC_DISABLE;
	}
	
	hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_SET_INDACC);
	
	/* It is repeated Read as for the right or wrong of INDACC use flag */
	flag = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_INDACC0, 0x4);
	if ( !(flag & HFC_MLPF_INDACC_FLG) )
	{
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
		return HFC_MLPF_INDACC_USED;
	}

	hfc_write_hg_reg(ap, HFC_IOHGSPC_INDACC3, 4, 0x100);
	type = hfc_read_hg_reg(ap, HFC_IOHGSPC_INDAREA, 4);
	if ( HFC_MMODE_CHECK_SHARED(ap) && ( !HFC_MMODE_CHECK_SHADOW(ap) ) )
		memset(mpap->hw_log, 0, HFC_HWLOG_SIZE);

	for ( reg_addr=0; reg_addr<HFC_HWLOG_SIZE; reg_addr+=0x80)		/* FCLNX-GPL-262 */
	{
		hfc_write_hg_reg(ap, HFC_IOHGSPC_INDACC3, 4, reg_addr);
		
		if ( HFC_MMODE_CHECK_SHADOW (ap) )
		{
			if ( reg_addr == 0 )
				hfc_mlpf_set_mmio_hg(ap, ap->mlpf_drv_log, HFC_IOHGSPC_INDAREA, 0x40 );
			else if ( reg_addr >= 0x400 )
				hfc_mlpf_set_mmio_hg(ap, (uchar *)&log_area[(reg_addr-0x400)], HFC_IOHGSPC_INDAREA, 0x80);
		}
		else if ( HFC_MMODE_CHECK_SHARED(ap) && ( !HFC_MMODE_CHECK_SHADOW(ap) ) )
		{
			if( type != 0x00000000 )
			{
				if ( reg_addr == 0 )
				{
					memset(ap->mlpf_drv_log, 0, 0x40);
					hfc_mlpf_get_mmio_hg(ap, ap->mlpf_drv_log, HFC_IOHGSPC_INDAREA, 0x40 );
				}

				hfc_mlpf_get_mmio_hg(ap, (uchar *)&log_area[reg_addr], HFC_IOHGSPC_INDAREA, 0x80);
			}
			else {
				if ( reg_addr == 0 )
				{
					memset(ap->mlpf_drv_log, 0, 0x40);
					hfc_mlpf_get_mmio_hg(ap, ap->mlpf_drv_log, HFC_IOHGSPC_INDAREA, 0x40 );
				}
				else if ( reg_addr >= 0x400 )
				{
					hfc_mlpf_get_mmio_hg(ap, (uchar *)&log_area[(reg_addr-0x400)], HFC_IOHGSPC_INDAREA, 0x80);
				}
			}
		}
	}																/* FCLNX-GPL-262 */
	
	hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RESET_INDACC);
	
	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);	
	return HFC_MLPF_INDACC_SUCCESS;
}


/* FCLNX-GPL-393 */
/*
 * Function:    hfc_mlpf_set_errorlimit
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_set_errorlimit(
	struct adap_info            *ap)
{
	// Guest LPAR only
	int	i;
	short	max_ld_err_count_s=0, min_ld_remain_count_s=0;
	struct	target_info		*target;
	
	/*  Write Error Limit for MMIO-HG */
	hfc_write_hg_reg(ap, HFC_IOHGSPC_LDS_LIMIT,  2, ap->ld_err_limit_s);
	hfc_write_hg_reg(ap, HFC_IOHGSPC_FCIF_LIMIT, 2, ap->if_err_limit);
	hfc_write_hg_reg(ap, HFC_IOHGSPC_TO_LIMIT,   2, ap->to_err_limit);
	hfc_write_hg_reg(ap, HFC_IOHGSPC_RST_LIMIT,  2, ap->rt_err_enable);
	
	/*  Write Error Count for MMIO-HG */
	if (!(hfc_manage_info.hfcldd_mp_mod)) {
		max_ld_err_count_s = ap->ld_err_count_s;
		for(i=0; i<(ap->max_target); i++){
			target = ap->target_arg[i];
			if( target != NULL){
				if( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)){
					if(max_ld_err_count_s < target->tgt_ld_err_count_s){
						max_ld_err_count_s = target->tgt_ld_err_count_s;
					}
				}
			}
		}
		hfc_write_hg_reg(ap, HFC_IOHGSPC_LDS_COUNT,  2, max_ld_err_count_s);
		hfc_write_hg_reg(ap, HFC_IOHGSPC_FCIF_COUNT, 2, ap->if_err_count);
		hfc_write_hg_reg(ap, HFC_IOHGSPC_TO_COUNT,   2, ap->to_err_count);
	}else{
		max_ld_err_count_s =	ap->ld_err_limit_s;
		if (ap->lds_errcnt_info != NULL ) {
			min_ld_remain_count_s = ap->lds_errcnt_info->remain_count;
		}
		for(i=0; i<(ap->max_target); i++){
			target = hfc_hash_target_valid(ap, i);
			if( target != NULL){
				if (target->tgt_lds_errcnt_info != NULL ) {
					if(target->tgt_lds_errcnt_info->remain_count < min_ld_remain_count_s){
						min_ld_remain_count_s = target->tgt_lds_errcnt_info->remain_count;
					}
				}
			}
		}
		max_ld_err_count_s -= min_ld_remain_count_s;
		hfc_write_hg_reg(ap, HFC_IOHGSPC_LDS_COUNT,  2, max_ld_err_count_s);
		hfc_write_hg_reg(ap, HFC_IOHGSPC_FCIF_COUNT, 2, ((ap->if_errcnt_info) ? (ap->if_err_limit - ap->if_errcnt_info->remain_count) : 0 ));
		hfc_write_hg_reg(ap, HFC_IOHGSPC_TO_COUNT, 2, ((ap->to_errcnt_info) ? (ap->to_err_limit - ap->to_errcnt_info->remain_count) : 0));
	}


	if(ap->c_err == HFC_ISOLATE_RT){
		hfc_write_hg_reg(ap, HFC_IOHGSPC_RST_COUNT,  2, 1);
	}
	else{
		hfc_write_hg_reg(ap, HFC_IOHGSPC_RST_COUNT,  2, 0);
	}
	
	return;
}


/* FCLNX-GPL-399 */
/*
 * Function:    hfc_mlpf_set_led
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_set_led(
	struct adap_info			 *ap,
	uint64_t						data)
{	
	int i;
	
	for(i = 0; i < 4; i++){
		hfc_write_reg_ext(ap, (ap->pkg.map->iosp.reg[(uint)HFC_IOSPACE_CMDLED]+i), (char)0x1, (((uint)(data) >> (8*(3-i))) & 0x000000ff) );
	}
}

/* FCLNX-GPL-399 */
/*
 * Function:    hfc_mlpf_set_fcif
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_set_fcif(
	struct adap_info			 *ap,
	uint64_t						data)
{	
	int i;

	for(i = 0; i < 4; i++){
		hfc_write_reg_ext(ap, (ap->pkg.map->iosp.reg[(uint)HFC_IOSPACE_CMDFCIF]+i), (char)0x1, (((uint)(data) >> (8*(3-i))) & 0x000000ff) );
	}
}

/* FCLNX-GPL-489 */
/*
 * Function:    hfc_mlpf_migration_end
 *
 * Purpose:     Set Hardware Status after LPAR Migration
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_migration_end(
	struct adap_info    *ap)
{
	uint logdata[4];
	uchar wkc = 0;
	uchar buf[16];
	struct mp_adap_info     *mpap;
	uint wkint1 = 0, wkint2 = 0;
	
	if(ap == NULL)return;
	
	mpap = ap->mp_adap_info;
	
	if(mpap == NULL)return;
	
	memset(logdata, 0x00, sizeof(logdata));
	memset(buf, 0x00, sizeof(buf));
	
	hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x00, ap, NULL, NULL, 0, 0, 0);

	/* Set adap_id */
	hfc_mlpf_get_mmio_hg(ap, buf, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
	
	if (mpap->ap == ap) { /* FCLNX-GPL-504 */
	
		/* Copy adap_id */
		memcpy(mpap->adap_id, buf, sizeof(buf));
		
		/* Set sys_rev */
		mpap->sys_rev = hfc_get_sysrev(ap);
		
		mpap->mck_err_cnt = 0;
	} /* FCLNX-GPL-504 */
	
	/* Set WWPN */
	hfc_search_adapter_number(ap);

	/* Set HBA F/W, HVM F/W support bit */
	if(hfc_mlpf_check_state(ap, HFC_HG_LPAR_ISOLATION_SUPPORT, HFC_CHECK_HVM_SUPPORT )) /* FCLNX-GPL-551 */
		set_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support);
	if(hfc_mlpf_check_state(ap, HFC_HG_LPRSTATUS_ISOLSUPPRT, HFC_CHECK_LPAR_STATE )) /* FCLNX-GPL-551 */
		set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support);

	if(!( HFC_MMODE_CHECK_SHADOW(ap) ))
		hfc_mlpf_set_errorlimit(ap);
	
	/* Set LSI rev */
	if (ap->pkg.type == HFC_PKTYPE_FIVE_EX) {
		ap->pkg.lsi_rev = (uchar)hfc_read_reg(ap, HFC_IOSPACE_LSIREV, 0x01);
	}
	
	/* Set vpd support bit */
	hfc_copy_iocinfo(ap); 
	
	/* counter clear */
	ap->core_ce_cnt = 0;
	ap->pcie_sram_ce_cnt = 0;
	if(hfc_manage_info.hfcldd_mp_mod){
		hfc_manage_info.npubp->hfc_clear_errinfo(ap);
	}
	else{
		hfc_clear_errinfo_i(ap);
	}
	
	/* check for conflicting data*/
	/* check RID */
	wkint1 = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_RID, 0x4 );
	if (ap->rid != wkint1) {
		hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x01, ap, NULL, NULL, ap->rid, wkint1, 0);
		
		ap->rid = wkint1; /* FCLNX-GPL-549 */
	}
	
	/* check core no */
	wkc = (uchar)hfc_read_reg(ap, HFC_IOSPACE_CHNO, 0x1);
	if (ap->pkg.core_no != wkc) {
		hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x02, ap, NULL, NULL, ap->pkg.core_no, wkc, 0);
		
		memset(logdata, 0x00, sizeof(logdata));
		wkint1 = 0x00000002;
		HFC_4L_TO_4B(logdata[0], wkint1);
		logdata[1] = ap->pkg.core_no;
		logdata[2] = wkc;
	}
	
	/* check xob no */
	wkint1 = ap->fw_init_p->xob_inp; /* xob no */
	HFC_4B_TO_4L(wkint2, wkint1);
	wkint1 = ((wkint2 & 0x00ff0000)>>16) * HFC_XOB_PER_PAGE ;
	wkint1 += (wkint2 & 0x0000ffff) ;
	
	if (ap->xob_no != wkint1) {
		hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x03, ap, NULL, NULL, ap->xob_no, wkint2, 0);
		
		memset(logdata, 0x00, sizeof(logdata));
		wkint1 = 0x00000003;
		HFC_4L_TO_4B(logdata[0], wkint1);
		wkint2 = ap->xob_no;
		HFC_4L_TO_4B(logdata[1], wkint2);
		logdata[2] = ap->fw_init_p->xob_inp;
	}
	
	/* check xrb no */
	wkint1 = ap->fw_init_p->xrb_outp; /* xrb no */
	HFC_4B_TO_4L(wkint2, wkint1);
	wkint1 = ((wkint2 & 0x00ff0000)>>16) * HFC_XRB_PER_PAGE ;
	wkint1 += (wkint2 & 0x0000ffff) ;

	if (ap->xrb_no != wkint1) {
		hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x04, ap, NULL, NULL, ap->xrb_no, wkint2, 0);
		
		memset(logdata, 0x00, sizeof(logdata));
		wkint1 = 0x00000004;
		HFC_4L_TO_4B(logdata[0], wkint1);
		wkint2 = ap->xrb_no;
		HFC_4L_TO_4B(logdata[1], wkint2);
		logdata[2] = ap->fw_init_p->xrb_outp;
	}
	wkint1 = (mpap->ap == ap ? 1 : 0); /* FCLNX-GPL-504 */
	
	HFC_4L_TO_4B(logdata[3], wkint1); /* FCLNX-GPL-504 */
	
	HFC_INFPRT("hfcldd%d : Adapter was changed into the other one by Live Migration.(HBA WWPN=%llx result=0x%08x%08x%08x%08x)\n", ap->dev_minor, (unsigned long long)ap->org_ww_name, logdata[0],logdata[1],logdata[2],logdata[3]); /* FCLNX-GPL-520 */
	
#ifdef HFC_HVM_DEBUG /* FCLNX-GPL-520 */	
	/* Errlog 0xAF*/
	hfc_errlog(ap,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4, 0xAF, (uchar *)&logdata, sizeof(logdata)) ;
#endif /* FCLNX-GPL-520 */
	
	hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x10, ap, NULL, NULL, wkint1, 0, 0); /* FCLNX-GPL-504 */
}

/* FCLNX-GPL-489 */
/*
 * Function:    hfc_mlpf_migration_recovery
 *
 * Purpose:     Recovery process after LPAR Migration
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_mlpf_migration_recovery(
	struct adap_info    *ap,
	uint				hyp_status)
{
	uint lpar_sts = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4);
	uint sfp_err = (lpar_sts & 0x00000f00);
	uint hyp_condition = hfc_mlpf_check_hypcondition(hyp_status);
	
	hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x20, ap, NULL, NULL, hyp_status, lpar_sts, sfp_err);
	
	if ( hyp_condition == HFC_HYPCONDITION_NORMAL )
	{
		/* CHK-STOP or Isolate -> Normal(LinkUp or LinkDown) */
		if ( !sfp_err &&
			( test_bit(HFC_CHK_STOP, (ulong *)&ap->status) || test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status) ||
			  (ap->isol_detail != HFC_NO_ISOLATE) ) ) /* FCLNX-GPL-550 */
		{
			hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x21, ap, NULL, NULL, ap->isol_detail, 0, 0);
			if (ap->mp_adap_info->ap == ap) { /* FCLNX-GPL-504 */
				clear_bit(HFC_HWINIT_FAIL, (ulong *)&ap->mp_adap_info->status);
				clear_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status);
			} /* FCLNX-GPL-504 */
			clear_bit(HFC_CHK_STOP, (ulong *)&ap->status);
			set_bit(HFC_ATTACH, (ulong *)&ap->attach_status);
			set_bit( HFC_ISOL, (ulong *)&ap->status );	/* To recovery isolation */
			hfc_mlpf_isol_recovery_end_glpar(ap, hyp_status, 1);
		}
		/* LinkDown -> LinkDown */
		else if (lpar_sts & HFC_HG_LPRSTATUS_LINKDOWN) {
			hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x22, ap, NULL, NULL, ap->isol_detail, 0, 0);
			
			switch (sfp_err) {
			case HFC_HG_LPRSTATUS_SFP_FAIL:
				ap->status |= HFC_ISOL;
				ap->isol_detail = HFC_ISOLATE_SFPFAIL;
				break;
			case HFC_HG_LPRSTATUS_SFP_NOTSUPT:
				ap->status |= HFC_ISOL;
				ap->isol_detail = HFC_ISOLATE_SFPNOTSUPPORT;
				break;
			case HFC_HG_LPRSTATUS_SFP_DOWN:
				ap->status |= HFC_ISOL;
				ap->isol_detail = HFC_ISOLATE_SFPDOWN;
				break;
			default:
				ap->status &= ~HFC_ISOL;
				ap->isol_detail = HFC_NO_ISOLATE;
				break;
			}
			hfc_linkdown_intreq(ap, 1);
		}
		else { /* LinkDown -> LinkUp or LinkUp -> LinkUp */
			hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x23, ap, NULL, NULL, ap->isol_detail, 0, 0);
			
			set_bit(HFC_ONLINE, (ulong *)&ap->status);
			clear_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
				clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
				hfc_w_stop( ap, HFC_WLINKUP_CNT_TMR );
			}
#endif	/* FCLNX-GPL-FX-424 */
			clear_bit(HFC_ISOL, (ulong *)&ap->status);
			ap->isol_detail = HFC_NO_ISOLATE;
			hfc_w_stop( ap, HFC_LINKUP_TMR );
			
			/* Search Target. Issue GID_FT. */
			set_bit(HFC_NEED_NMSRV, (ulong *)&ap->status);
			start_next_mailbox( ap ); 
		}
	}
	else if ( hyp_condition == HFC_HYPCONDITION_CSTP ) /* -> CHK-STOP */
	{
		hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x24, ap, NULL, NULL, ap->isol_detail, 0, 0);
		
		/* Isolation -> CHK-STOP */
		if (ap->mp_adap_info->ap == ap) { /* FCLNX-GPL-504 */
			set_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status);
		} /* FCLNX-GPL-504 */
		set_bit(HFC_CHK_STOP, (ulong *)&ap->status);
		clear_bit(HFC_ISOL, (ulong *)&ap->status);
		ap->isol_detail = HFC_NO_ISOLATE;

		/* CHK-STOP ¡ö?CHK-STOP Nop */
		/* LinkDown ¡ö?CHK-STOP or Isolation is disallowed by Hyper. */
	}
	else if ( hyp_condition == HFC_HYPCONDITION_ISOL ) /* -> Isolation */
	{
		hfc_hand2_trace(HFC_TRC_MLPF_MIGRATION, 0x26, ap, NULL, NULL, ap->isol_detail, 0, 0);

		if (test_bit(HFC_CHK_STOP, (ulong *)&ap->status) || test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)) { /* CHK-STOP -> Isolation *//* FCLNX-GPL-550 */
			if (ap->mp_adap_info->ap == ap) { /* FCLNX-GPL-504 */
				clear_bit(HFC_HWINIT_FAIL, (ulong *)&ap->mp_adap_info->status);
				clear_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status);
			} /* FCLNX-GPL-504 */
			clear_bit(HFC_CHK_STOP, (ulong *)&ap->status);
			set_bit(HFC_ATTACH, (ulong *)&ap->attach_status);
			set_bit(HFC_ISOL, (ulong *)&ap->status);
		}
		if (hyp_status & HFC_HG_HYPSTATUS_ISOLCMD) {
			ap->isol_detail = HFC_ISOLATE_PORT_C;
		} else {
			ap->isol_detail = HFC_ISOLATE_PORT_E;
		}
	}
	else{
		if (ap->debug_func & HFC_DEBUG_HYP_INT_CHK) {
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4, 0xb1, NULL, 0) ;
		}
	}
}

/* FCLNX-GPL-494 */
/*
 * Function:    hfc_alloc_mlpf_cca
 *
 * Purpose:     Allcate uncached memory area shared hypervisor.
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */

int hfc_alloc_mlpf_cca(
	struct adap_info    *ap)
{
	dma_addr_t			bus_addr;
	void				*vir_addr;
	int					size;
	struct pci_dev		*pdev = NULL;
	
	HFC_DBGPRT( "  hfcldd : hfc_alloc_mlpf_cca - Start \n");
	
	pdev = ap->pci_cfginf;
	size = sizeof(struct hg_cca);

	vir_addr = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size, &bus_addr,GFP_ATOMIC);

	ap->hg_cca_p = (struct hg_cca *)vir_addr;
	if (ap->hg_cca_p == NULL) { 	/* failed? */
 		HFC_DBGPRT(" hfcldd : hfc_alloc_mlpf_cca  - exit with error (alloc_error_exit) \n");
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD0, NULL, 0) ;
		hfc_free_mlpf_cca(ap);
		HFC_EXIT("hfc_alloc_mlpf_cca  (error) ");
		return(1);
	}                                                                                         

	HFC_DBGPRT("  (hg_cca) logical=%lx, physical=%lx, size = %d \n",
			(ulong)ap->hg_cca_p, (ulong)bus_addr, size);
			
	memset((char *)ap->hg_cca_p, 0, (int)size);
	ap->padr_hg_cca = bus_addr;
	
	return (0);

}

/* FCLNX-GPL-494 */
/*
 * Function:    hfc_free_mlpf_cca
 *
 * Purpose:     free uncached memory area shared hypervisor.
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */

void hfc_free_mlpf_cca(
	struct adap_info    *ap)
{
	struct pci_dev		*pdev = NULL;
	
	pdev = ap->pci_cfginf;

	if ( ap->hg_cca_p != NULL ) {
		/* Release mlpf cca (128 Bytes) */
		HFC_DBGPRT(" hfcldd : hfc_free_mlpf_cca - free mlpf cca \n");
		memset((char *)ap->hg_cca_p, 0, (int)sizeof(struct hg_cca));

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)sizeof(struct hg_cca),(void *)ap->hg_cca_p, (dma_addr_t)ap->padr_hg_cca);

		ap->hg_cca_p = NULL;
	}

	return;
}

/* FCLNX-GPL-494 */
/*
 * Function:    hfc_mlpf_cca_setup
 *
 * Purpose:     Setup mlpf_cca
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */

#define HFC_HG_CCA_PHYADR_VALID		0x80

void hfc_mlpf_cca_setup(
	struct adap_info    *ap)
{
	uchar flag = 0;
	uint	wk = 0;
	unsigned long long		wkp;
	int						dma_size;
	
	if ( (HFC_MMODE_CHECK_BASIC(ap)) || (ap->hg_cca_p == NULL) ) {
		return;
	}
	
	wk = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
	if(!hfc_mlpf_check_state(ap, HFC_HG_LPAR_STATISTICS_SUPPORT, HFC_CHECK_HVM_SUPPORT )){
		ap->hg_cca_p = NULL;
		return;
	}

	ap->hg_cca_p->version = 0x01;
	if ( ap->fw_init_p != NULL ){	/* FCLNX-GPL-507 */
		if ( ap->fw_init_p->func2 & HFC_FWF_STATCCA ) {
			ap->hg_cca_p->valid |= HFC_FWSTATISTICS_VALID;
		}
	}	/* FCLNX-GPL-507 */
	ap->hg_cca_p->size = sizeof(struct hg_cca);

	flag = (uchar) hfc_read_hg_reg(ap, HFC_IOHGSPC_HGCCA_FLAG, 0x1);

	if (!(flag & HFC_HG_CCA_PHYADR_VALID)) { /* Not updating after setting pyscal address. */
		dma_size = sizeof(dma_addr_t);
		wkp = ap->padr_hg_cca;									/* FCLNX-GPL-524 */
		if(dma_size == 8){
			wkp >>=32;
		}
		else{
			wkp = 0;
		}
		
		hfc_write_hg_reg(ap, HFC_IOHGSPC_HGCCA_FLAG,  1, 0x00); /* Clear flag set by EFI driver. */
		hfc_write_hg_reg(ap, HFC_IOHGSPC_HGCCA_ADDR1, 4, ap->padr_hg_cca );
		hfc_write_hg_reg(ap, HFC_IOHGSPC_HGCCA_ADDR0, 4, wkp);
		/* Set flag after setting psycal address. */
		hfc_write_hg_reg(ap,  HFC_IOHGSPC_HGCCA_FLAG,  1, HFC_HG_CCA_PHYADR_VALID);
	}
}

