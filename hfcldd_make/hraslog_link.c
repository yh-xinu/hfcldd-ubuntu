/*******************************************************************************
 *
 *      hraslog_link.c - RASLOG linkage module
 *
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hraslog_link.c,v 1.1.2.4.28.3.2.3.6.1.2.1.2.1.2.3 2015/04/23 01:19:36 yyamada Exp $
 */

#include <linux/kernel.h>
#include <linux/slab.h>	
#include <linux/fs.h>	
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/dcache.h>
#include <linux/smp.h>
#include <asm/atomic.h>

#include "hraslog.h"

#define HRASLOG_VERSION 1
#define HRASLOG_REVISION 1
#define HRASLOG_RVER 0
#define HRASLOG_WVER 0



struct module *_hraslog_mod;
EXPORT_SYMBOL(_hraslog_mod);

int (*_hraslog_symbol)(const char *, const char *, unsigned int, const char *, void *);
EXPORT_SYMBOL(_hraslog_symbol);

int _hraslogserv(struct hraslogopt_st *hraslogoptp) {
  
	return 2;
}
EXPORT_SYMBOL(_hraslogserv);

int _hraslog(const char* error_id, 
			 const char* resource_name,
			 unsigned int length,
			 const char* detail_data)
{
    return 2;
}
EXPORT_SYMBOL(_hraslog);


int _hraslogcheck(void)
{
    return -1;
}
EXPORT_SYMBOL(_hraslogcheck);

int _hraslog_system_state;
EXPORT_SYMBOL(_hraslog_system_state);

int (*_hraslog_save_symbol)(int);
EXPORT_SYMBOL(_hraslog_save_symbol);


static int hrasloglink_init(void)
{
	_hraslog_symbol = 0;
	_hraslog_mod = 0;
	return 0;
}


static void hrasloglink_exit(void)
{
  
}

module_init(hrasloglink_init);
module_exit(hrasloglink_exit);
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1.0-0 E0004");
MODULE_AUTHOR("Hitachi, Ltd.");
