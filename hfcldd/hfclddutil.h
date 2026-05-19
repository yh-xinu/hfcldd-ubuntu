/*
 * hfclddutil.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfclddutil.h,v 1.1.2.5.20.1.6.2.2.2.6.1.2.1.2.1 2015/02/04 08:32:56 toyo Exp $
 */

/* Return Code */
// #define HFCUT_NORMALEND		0	/* normal end */
#define HFCUT_IOCFAIL		1	/* ioctl() fail. */
#define HFCUT_TIMEDOUT		2	/* ioctl() timeout. */
#define HFCUT_BUSY			3	/* ioctl() or resource busy. */
#define HFCUT_SPFILE		4	/* special file open error. */
#define HFCUT_INVAL			5	/* Invalid argument. */
#define HFCUT_NOMEM			6	/* Not enough space. */
#define HFCUT_DHFCPCM		7	/* Disable HFC-PCM. */
#define HFCUT_NODEV			8	/* No such device. */
#define HFCUT_PFAIL			9	/* path fail. */
#define HFCUT_STATUS		10	/* bad path status. */

#define HFCUT_OPEN			50	/* file oepn error. */
#define HFCUT_CLOSE			51	/* file close error. */

#define HFCUT_BADF			60	/* Bad file descriptor. */
#define HFCUT_INTERNAL		61	/* internal error. */
#define HFCUT_CONFMODF		62	/* conf module access error. */
#define HFCUT_TMPMODF		63	/* temp module access error. */

#define HFCUT_CONFF			70	/* conf file access error. */
#define HFCUT_CONFFBIG		71	/* conf file too large. */
#define HFCUT_CONFFSEEK		72	/* conf file seek error. */
#define HFCUT_CONFFWR		73	/* conf file write error. */
#define HFCUT_KEYF			74	/* key file access error. */
#define HFCUT_KEYFBIG		75	/* key file too large. */
#define HFCUT_KEYFSEEK		76	/* key file seek error. */
#define HFCUT_KEYFWR		77	/* key file write error. */

#define HFCUT_INVCONF		78	/* invalid hfcldd.conf format. */
#define HFCUT_INVCONFMOD	79	/* invalid hfcldd_conf format. */
#define HFCUT_UNAME			80	/* uname() access fail. */
#define HFCUT_NOBUFS		81	/* No buffer space available.*/
#define HFCUT_AGAIN			82	/* Resource unavailable, try again */
#define HFCUT_NOTSUP 		83	/* Not supported. */

#define HFCUT_CONFFRD		84	/* conf file read error. */
#define HFCUT_FWNOTSUP		100	/* isolate sfp not support. */	/* FCLNX-GPL-146 */
