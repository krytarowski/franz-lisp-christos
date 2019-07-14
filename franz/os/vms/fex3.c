/*-					-[Sat Apr  9 17:03:02 1983 by layer]-
 * 	fex3.c
 * nlambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: fex3.c,v 1.15 85/03/13 17:18:29 sklower Exp";
#else
__RCSID("$Id: fex3.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
static int      pagsiz, pagrnd;


/*
 *Ndumplisp -- create executable version of current state of this lisp.
 */
/*** VMS version of Ndumplisp ***/
#include "aout.h"
#undef	protect
#include <vms/vmsexe.h>

extern lispval  reborn;
extern          etext;
extern int      dmpmode;

lispval
Ndumplisp(void)
{
	struct exec    *workp;
	lispval         argptr, temp;
	char           *fname;
	ISD            *Isd;
	int             i;
	struct exec     work, old;
	int             extra_cref_page = 0;
	char           *start_of_data;
	int             descrip, des2, count, ax, mode;
	char            buf[5000], stabname[100], tbuf[BUFSIZ];
	int             fp, fp1;
	union {
		char            Buffer[512];
		struct {
			IHD             Ihd;
			IHA             Iha;
			IHS             Ihs;
			IHI             Ihi;
		}               Header;
	}               Buffer;	/* VMS Header */

	/*
	 *	Dumpmode is always 413!!
	 */
	mode = 0413;
	pagsiz = Igtpgsz();
	pagrnd = pagsiz - 1;

	workp = &work;
	workp->a_magic = mode;
	workp->a_text = ((((unsigned) (&etext)) - 1) & (~pagrnd)) + pagsiz;
	start_of_data = (char *) workp->a_text;
	workp->a_data =
		(unsigned) sbrk(0) - (unsigned) start_of_data;
	workp->a_bss = 0;
	workp->a_syms = 0;
	workp->a_entry = (unsigned) gstart();
	workp->a_trsize = 0;
	workp->a_drsize = 0;

	fname = "savedlisp";	/* set defaults */
	reborn = CNIL;
	argptr = lbot->val;
	if (argptr != nil) {
		temp = argptr->d.car;
		if ((TYPE(temp)) == ATOM)
			fname = temp->a.pname;
	}
	/*
	 *	Open the new executable file
	 */
	strcpy(buf, fname);
	if (index(buf, '.') == 0)
		strcat(buf, ".exe");
	if ((descrip = creat(buf, 0777)) < 0)
		error("Dumplisp failed", FALSE);
	/*
	 *	Create the VMS header
	 */
	for (i = 0; i < 512; i++)
		Buffer.Buffer[i] = 0;	/* Clear Header */
	Buffer.Header.Ihd.size = sizeof(Buffer.Header);
	Buffer.Header.Ihd.activoff = sizeof(IHD);
	Buffer.Header.Ihd.symdbgoff = sizeof(IHD) + sizeof(IHA);
	Buffer.Header.Ihd.imgidoff = sizeof(IHD) + sizeof(IHA) + sizeof(IHS);
	Buffer.Header.Ihd.majorid[0] = '0';
	Buffer.Header.Ihd.majorid[1] = '2';
	Buffer.Header.Ihd.minorid[0] = '0';
	Buffer.Header.Ihd.minorid[1] = '2';
	Buffer.Header.Ihd.imgtype = IHD_EXECUTABLE;
	Buffer.Header.Ihd.privreqs[0] = -1;
	Buffer.Header.Ihd.privreqs[1] = -1;
	Buffer.Header.Ihd.lnkflags.nopobufs = 1;
	Buffer.Header.Ihd.imgiocnt = 250;

	Buffer.Header.Iha.tfradr1 = SYS$IMGSTA;
	Buffer.Header.Iha.tfradr2 = workp->a_entry;

	strcpy(Buffer.Header.Ihi.imgnam + 1, "SAVEDLISP");
	Buffer.Header.Ihi.imgnam[0] = 9;
	Buffer.Header.Ihi.imgid[0] = 0;
	Buffer.Header.Ihi.imgid[1] = '0';
	sys$gettim(Buffer.Header.Ihi.linktime);
	strcpy(Buffer.Header.Ihi.linkid + 1, " Opus 38");
	Buffer.Header.Ihi.linkid[0] = 8;

	Isd = (ISD *) & Buffer.Buffer[sizeof(Buffer.Header)];
	/* Text ISD */
	Isd->size = ISDSIZE_TEXT;
	Isd->pagcnt = workp->a_text >> LSHIFT;
	Isd->vpnpfc.vpn = 0;
	Isd->flags.type = ISD_NORMAL;
	Isd->vbn = 3;
	Isd = (ISD *) ((char *) Isd + Isd->size);
	/* Data ISD */
	Isd->size = ISDSIZE_TEXT;
	Isd->pagcnt = workp->a_data >> LSHIFT;
	Isd->vpnpfc.vpn = ((unsigned) start_of_data) >> LSHIFT;
	Isd->flags.type = ISD_NORMAL;
	Isd->flags.crf = 1;
	Isd->flags.wrt = 1;
	Isd->vbn = (workp->a_text >> LSHIFT) + 3;
	Isd = (ISD *) ((char *) Isd + Isd->size);
	/* Stack ISD */
	Isd->size = ISDSIZE_DZRO;
	Isd->pagcnt = ISDSTACK_SIZE;
	Isd->vpnpfc.vpn = ISDSTACK_BASE;
	Isd->flags.type = ISD_USERSTACK;
	Isd->flags.dzro = 1;
	Isd->flags.wrt = 1;
	Isd = (ISD *) ((char *) Isd + Isd->size);
	/* End of ISD List */
	Isd->size = 0;
	Isd = (ISD *) ((char *) Isd + 2);
	/*
	 *	Make the rest of the header -1s
	 */
	for (i = ((char *) Isd - Buffer.Buffer); i < 512; i++)
		Buffer.Buffer[i] = -1;
	/*
	 *	Write the VMS Header
	 */
	if (write(descrip, Buffer.Buffer, 512) == -1)
		error("Dumplisp failed", FALSE);
#if	EUNICE_UNIX_OBJECT_FILE_CFASL
	/*
	 *	Get the UNIX symbol table file header
	 */
	des2 = open(gstab(), 0);
	if (des2 >= 0) {
		old.a_magic = 0;
		if (read(des2, (char *) &old, sizeof(old)) >= 0) {
			if (N_BADMAG(old)) {
				lseek(des2, 512, 0);	/* Try block #1 */
				read(des2, (char *) &old, sizeof(old));
			}
			if (!N_BADMAG(old))
				work.a_syms = old.a_syms;
		}
	}
#endif	/* EUNICE_UNIX_OBJECT_FILE_CFASL */
	/*
	 *	Update the UNIX header so that the extra cref page is
	 *	considered part of data space.
	 */
	if (extra_cref_page)
		work.a_data += 512;
	/*
	 *	Write the UNIX header
	 */
	if (write(descrip, &work, sizeof(work)) == -1)
		error("Dumplisp failed", FALSE);
	/*
	 *	seek to 1024 (end of headers)
	 */
	if (lseek(descrip, 1024, 0) == -1)
		error("Dumplisp failed", FALSE);
	/*
	 *	write the world
	 */
	if (write(descrip, 0, workp->a_text) == -1)
		error("Dumplisp failed", FALSE);
	if (write(descrip, start_of_data, workp->a_data) == -1)
		error("Dumplisp failed", FALSE);

#if	!EUNICE_UNIX_OBJECT_FILE_CFASL
	/*
	 *	VMS OBJECT files: We are done with the executable file
	 */
	close(descrip);
	/*
	 *	Now try to write the symbol table file!
	 */
	strcpy(buf, gstab());

	strcpy(stabname, fname);
	if (index(stabname, '.') == 0)
		strcat(stabname, ".stb");
	else
		strcpy(index(stabname, '.'), ".stb");

	/* Use Link/Unlink to rename the symbol table */
	if (!strncmp(gstab(), "tmp:", 4))
		if (link(buf, stabname) >= 0)
			if (unlink(buf) >= 0)
				return (nil);

	/* Copy the symbol table */
	if ((fp = open(buf, 0)) < 0)
		error("Symbol table file not there\n", FALSE);
	fp1 = creat(stabname, 0666, "var");
	while ((i = read(fp, buf, 5000)) > 0)
		if (write(fp1, buf, i) == -1) {
			close(fp);
			close(fp1);
			error("Error writing symbol table\n", FALSE);
		}
	close(fp);
	close(fp1);
	if (i < 0)
		error("Error reading symbol table\n", FALSE);
	if (!strncmp(gstab(), "tmp:", 4))
		unlink(gstab);
	/*
	 *	Done
	 */
	reborn = 0;
	return (nil);
#else	/* EUNICE_UNIX_OBJECT_FILE_CFASL */
	/*
	 *	UNIX OBJECT files: append the new symbol table
	 */
	if (des2 > 0 && work.a_syms) {
		count = old.a_text + old.a_data + (old.a_magic == 0413 ? 1024
						   : sizeof(old));
		if (-1 == lseek(des2, count, 0))
			error("Could not seek to stab", FALSE);
		for (count = old.a_syms; count > 0; count -= BUFSIZ) {
			ax = read(des2, tbuf, (int) (count < BUFSIZ ? count : BUFSIZ));
			if (ax == 0) {
				printf("Unexpected end of syms", count);
				fflush(stdout);
				break;
			} else if (ax > 0)
				write(descrip, tbuf, ax);
			else
				error("Failure to write dumplisp stab", FALSE);
		}
		if (-1 == lseek(des2, (long)
				((old.a_magic == 0413 ? 1024 : sizeof(old))
				 + old.a_text + old.a_data
				 + old.a_trsize + old.a_drsize + old.a_syms),
				0))
			error(" Could not seek to string table ", FALSE);
		for (ax = 1; ax > 0;) {
			ax = read(des2, tbuf, BUFSIZ);
			if (ax > 0)
				write(descrip, tbuf, ax);
			else if (ax < 0)
				error("Error in string table read ", FALSE);
		}
	}
	close(descrip);
	if (des2 > 0)
		close(des2);
	reborn = 0;

	return (nil);
#endif	/* EUNICE_UNIX_OBJECT_FILE_CFASL */
}
