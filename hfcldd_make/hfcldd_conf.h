/*
 * hfcldd_conf.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcldd_conf.h,v 1.1.2.7.2.1.22.2.2.2.6.1.2.1.2.1 2015/02/04 08:32:56 toyo Exp $
 */


#ifndef _H_HFCLDD_CONF             /* Double definition prevention */
#define _H_HFCLDD_CONF

extern struct nonpub_symbol_list *hfc_get_nonpub_symbol_list(void);
extern struct pub_symbol_list *hfc_get_pub_symbol_list(void);
extern char *hfcldd_cnf(void);
extern char *hfcldd_key(void);
extern char *hfcldd_key_hop(void);	/* FCLNX-0429 */

#endif /* _H_HFCLDD_CONF */
