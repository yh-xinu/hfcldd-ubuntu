/*
 * hfcl_version.c
 * Copyright (C) 2007, 2016, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char verc_rcsid[] = "$Id: hfcl_version.c,v 1.86.2.20.28.2.2.2.6.2.2.4.2.2.4.1 2016/02/18 05:15:41 mhayashi Exp $";

#include "hfcldd.h"
#include "hfcl_modulever.h"

extern void hfc_version(void);

const char package_ver[16] = HFC_MODULEVER; /* package version              */

extern char det_rcsid[];                    /* hfcl_detect.c                */
extern char diag_rcsid[];                   /* hfcl_diag.c                  */
extern char intr_rcsid[];                   /* hfcl_handler.c               */
extern char ioctl_rcsid[];                  /* hfcl_ioctl.c                 */
extern char stra_rcsid[];                   /* hfcl_strategy.c              */
extern char timer_rcsid[];                  /* hfcl_timer_recovery.c        */
extern char top_rcsid[];                    /* hfcl_top.c                   */
extern char mlpf_rcsid[];                   /* hfcl_mlpf.c                  */

extern char det_fx_rcsid[];                 /* hfcl_detect_fx.c             */
extern char diag_fx_rcsid[];                /* hfcl_diag_fx.c               */
extern char intr_fx_rcsid[];                /* hfcl_handler_fx.c            */
extern char ioctl_fx_rcsid[];               /* hfcl_ioctl_fx.c              */
extern char stra_fx_rcsid[];                /* hfcl_strategy_fx.c           */
extern char timer_fx_rcsid[];               /* hfcl_timer_recovery_fx.c     */
extern char top_fx_rcsid[];                 /* hfcl_top_fx.c                */
extern char mlpf_fx_rcsid[];                /* hfcl_mlpf_fx.c               */
extern char npiv_rcsid[];                   /* hfcl_npiv_fx.c               */

extern struct manage_info hfc_manage_info;

char hfc_ver[24][96];

/*
 * Function:    hfc_version
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_version(void)
{
    int len ;
    
    memset(hfc_ver,0,sizeof(hfc_ver)) ;

	sprintf(hfc_ver[0],"Packege No : %s", package_ver );
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0) /* FCLNX-GPL-FX-496 start */
	sprintf(hfc_ver[1],"Build Date : %s %s",__DATE__,__TIME__);
#endif /* FCLNX-GPL-FX-496 end */

	len = strlen(det_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[2],det_rcsid,len) ;

	len = strlen(diag_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[3],diag_rcsid,len) ;

	len = strlen(intr_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[4],intr_rcsid,len) ;

	len = strlen(ioctl_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[5],ioctl_rcsid,len) ;

	len = strlen(stra_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[6],stra_rcsid,len) ;

	len = strlen(timer_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[7],timer_rcsid,len) ;

	len = strlen(top_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[8],top_rcsid,len) ;

	len = strlen(mlpf_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[9],mlpf_rcsid,len) ;

	/* FIVE-FX Start */
	len = strlen(det_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[10],det_fx_rcsid,len) ;

	len = strlen(diag_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[11],diag_fx_rcsid,len) ;

	len = strlen(intr_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[12],intr_fx_rcsid,len) ;

	len = strlen(ioctl_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[13],ioctl_fx_rcsid,len) ;

	len = strlen(stra_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[14],stra_fx_rcsid,len) ;

	len = strlen(timer_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[15],timer_fx_rcsid,len) ;

	len = strlen(top_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[16],top_fx_rcsid,len) ;

	len = strlen(mlpf_fx_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[17],mlpf_fx_rcsid,len) ;

	len = strlen(npiv_rcsid) ;
	if( len > 95 )
		len = 96 ;
	HFC_MEMCPY(hfc_ver[18],npiv_rcsid,len) ;
	/* FIVE-FX End */
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		len = strlen(hfc_manage_info.npubp->hfc_get_pcm_rcsid()) ;
		if( len > 95 )
			len = 96 ;
		HFC_MEMCPY(hfc_ver[19],hfc_manage_info.npubp->hfc_get_pcm_rcsid(),len) ;
		
		/* FIVE-FX Start */
		len = strlen(hfc_manage_info.npubp->hfc_fx_get_pcm_rcsid()) ;
		if( len > 95 )
			len = 96 ;
		HFC_MEMCPY(hfc_ver[20],hfc_manage_info.npubp->hfc_fx_get_pcm_rcsid(),len);
		/* FIVE-FX End */
		
	}
}
