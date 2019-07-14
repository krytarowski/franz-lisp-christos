/* Modified by RMT 11 May 1992 for 386bsd */

/*					-[Thu Mar  3 15:57:51 1983 by jkf]-
 * 	config.h
 * configuration dependent info
 *
 * Header: config.h,v 1.16 87/12/14 18:33:31 sklower Exp
 *
 * (c) copyright 1982, Regents of the University of California
 */
 
/* 
 * this file contains parameters which each site is likely to modify
 * in order to personalize the configuration of Lisp at their site.
 * The typical things to modifiy are:
 *    [optionally] turn on GCSTRINGS
 *    [optionally] provide a value for SITE 
 */

/* GCSTRINGS - define this if you want the garbage collector to reclaim
 *  strings.  It is not normally set because in typical applications the
 *  expense of collecting strings is not worth the amount of space
 *  retrieved
 */
 
/* #define GCSTRINGS */

/*
** NILIS0 -- for any UNIX implementation in which the users
**	address space starts at 0 (like m_vax, above). 
**
** NPINREG -- for the verison if lisp that keeps np and lbot in global
**	registers.  On the 68000, there is a special `hacked' version
**	of the C compiler that is needed to do this.
**
** #define NILIS0		1
** #define NPINREG		1
*/
#define NILIS0		1

#define OS      "unix"

/* DOMAIN - this is put on the (status features) list and
 * 	is the value of (status domain)
 */
#define LISP_DOMAIN  "ucb"

/*  TTSIZE is the absolute limit, in pages (both text and data), of the
 * size to which the lisp system may grow.
 * If you change this, you must recompile alloc.c and data.c.
 */
#define TTSIZE 1021600

#define STACKSIZE (16 * 1024)
