/*
 * hraslog.h
 * HA Logger Kit for Linux
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 *
 * 2007/07/02 first version release
 * 2007/08/27 add _hraslogserv()
 * 2007/10/01 modify hraslogopt_st
 */

/*
 * $Id: hraslog.h,v 1.1.2.3.28.2.2.3.6.1.2.1.2.1 2015/02/04 08:32:56 toyo Exp $
 */


#ifndef _HRASLOG_H
#define _HRASLOG_H

#define OPEN_RASLOG  0
#define CLOSE_RASLOG 1
#define GET_INFO     2

struct hraslogopt_st {
  int func_code;
  int ver;
  int rev;
  int rver;
  int wver;
  char reserved[12];
};

extern int _hraslogserv(struct hraslogopt_st *hraslogoptp);

extern int _hraslog(const char* error_id,
		    const char* resource_name,
		    unsigned int length,
		    const char* detail_data);

extern int __hraslog(const char* error_id,
	      const char* resource_name,
	      unsigned int length,
	      const char* detail_data);

extern int _hraslogcheck(void);

extern int (*_hraslog_symbol)(const char *, 
		const char *, 
		unsigned int, 
		const char *, 
		void *);

extern struct module *_hraslog_mod;

#endif /* _HRASLOG_H */
