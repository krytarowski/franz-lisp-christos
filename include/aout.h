/*
 * To be removed...
 */
#ifdef __APPLE__
struct exec {
	uintptr_t a_magic;
	uintptr_t a_text;
	uintptr_t a_trsize;
	uintptr_t a_data;
	uintptr_t a_drsize;
	uintptr_t a_syms;
	uintptr_t a_bss;
	uintptr_t a_entry;
};
#else
# include <a.out.h>
#endif

#include <nlist.h>

#ifdef __NetBSD__
# define a_magic a_midmag
#endif

#ifdef __linux__
# define a_magic a_info
#endif

#ifdef _AOUT_INCLUDE_
# define N_name n_un.n_name
# define STASSGN(p,q) (NTABLE[p].N_name = (q))
#else
# define N_name n_name
# define STASSGN(p,q) strncpy(NTABLE[(p)].n_name,(q),8)
#endif

#define OMAGIC          0407    /* old impure format */
#define NMAGIC          0410    /* read-only text */
#define ZMAGIC          0413    /* demand load format */
#define QMAGIC          0314    /* "compact" demand load format; deprecated */

#ifndef N_SYMOFF
#define	N_SYMOFF(x)	0
#define	N_STROFF(x)	0
#define	N_BADMAG(x)	0
#endif
