/*					-[Sat Jan 29 13:54:30 1983 by jkf]-
 * 	dfuncs.h
 * external function declaration
 *
 * Header: dfuncs.h,v 1.2 84/02/29 17:09:10 sklower Exp
 *
 * (c) copyright 1982, Regents of the University of California
 */

struct types;
struct ftab;
struct s_dot;
struct frame;
struct bindspec;

/* alloc.c */
int get_more_space(struct types *, int);
lispval next_one(struct types *);
void space_warn(int);
lispval next_pure_one(struct types *);
lispval newint(void);
lispval pnewint(void);
lispval newdot(void);
lispval pnewdot(void);
lispval newdoub(void);
lispval pnewdb(void);
lispval newsdot(void);
lispval pnewsdot(void);
struct atom *newatom(int);
char *newstr(int);
char *inewstr(char *);
char *pinewstr(char *);
lispval newarray(void);
lispval newfunct(void);
lispval newval(void);
lispval pnewval(void);
lispval newhunk(int);
lispval pnewhunk(int);
lispval inewval(lispval);
lispval newvec(int);
lispval pnewvec(int);
lispval nveci(int);
lispval pnveci(int);
lispval getvec(int, struct types *, int);
lispval Ipurep(lispval);
void pruneb(lispval);
lispval Badcall(void);
lispval Ngc(void);
lispval gc(struct types *);
void gc1(void);
lispval alloc(lispval, intptr_t);
lispval csegment(int, int, int);
char *atomtoolong(char *);
int csizeof(lispval);
int typenum(lispval);
char *gethspace(int, int);
void gcrebear(void);
void markit(lispval *);
void markdp(lispval);
char *xsbrk(int);
char *ysbrk(int, int);
lispval getatom(int);
lispval inewatom(char *);
int hashfcn(char *);
lispval LImemory(void);
void myhook(void);

/* data.c */

/* divbig.c */
void divbig(lispval, lispval, lispval *, lispval *);
lispval export(intptr_t *, intptr_t *);
int Ihau(int);
lispval Lhau(void);
lispval Lhaipar(void);
lispval Ibiglsh(lispval, lispval, int);
lispval Lsbiglsh(void);
lispval Lbiglsh(void);
lispval HackHex(void);

/* error.c */
lispval error(char *, int);
lispval errorh(lispval, char *, lispval, int, int);
lispval errorh1(lispval, char *, lispval, int, int, lispval);
lispval errorh2(lispval, char *, lispval, int, int, lispval, lispval);
lispval calhan(int, lispval *, lispval, int, lispval, char *, lispval);
void lispend(char[]);
void namerr(void);
void binderr(void);
void rtaberr(void);
void xserr(void);
void badmem(int);
lispval argerr(char *);
void wnaerr(lispval, lispval);

/* eval.c */
lispval eval(lispval);
lispval popnames(struct nament *);
void dumpnamestack(void);
lispval Lapply(void);
void rebind(lispval, struct argent *);
lispval Ifuncal(lispval);
lispval Lfuncal(void);
void fchack(void);
lispval Llexfun(void);
#ifndef protect
lispval protect(lispval);
#endif
lispval unprot(void);
lispval linterp(void);
lispval Undeff(lispval);
void bindfix(lispval, ...);

/* eval2.c */
int dumpmydata(int);
lispval Ifcall(lispval);
lispval ftolsp_(lispval);
lispval ftlspn_(lispval, intptr_t *);
lispval Ifclosure(lispval, int);
lispval Iarray(lispval, lispval, int);
void xpopnames(struct nament *);
struct nament *locatevar(lispval, int *, struct nament *);
lispval LIfss(void);

/* evalf.c */
lispval Levalf(void);
struct frame *searchforpdl(struct frame *);
void vfypdlp(struct frame *);
lispval Lfretn(void);

/* fasl.c */
lispval Lfasl(void);
int compar(int *, int *);
struct trent *gettran(int);
void clrtt(int);
lispval chktt(void);
void add_offset(int *, int);

/* fex1.c */
lispval Nprog(void);
lispval Ncatch(void);
lispval Nerrset(void);
lispval Nthrow(void);
lispval Ngo(void);
lispval Nreset(void);
lispval Nbreak(void);
void Nexit(void);
lispval Nsys(void);
lispval Ndef(void);
lispval Nquote(void);
lispval Nsetq(void);
lispval Ncond(void);
lispval Nand(void);
lispval Nor(void);

/* fex2.c */
lispval Ndo(void);
lispval Nprogv(void);
lispval Nprogn(void);
lispval Nprog2(void);
lispval typred(int, lispval);
lispval Nfunction(void);

/* fex3.c */
lispval Ndumplisp(void);
void pagerand(void);
void pageseql(void);
void pagenorm(void);
lispval Lgetaddress(void);
int Igtpgsz(void);

/* fex4.c */
lispval Lsyscall(void);
lispval Nevwhen(void);
lispval Nstatus(void);
lispval Nsstatus(void);
lispval Isstatus(lispval, lispval);
lispval Istsrch(lispval);
lispval Iaddstat(lispval, int, int, lispval);

/* fexr.c */
lispval Ngcafter(void);
lispval Nopval(void);
lispval copval(lispval, lispval);

/* ffasl.c */
lispval dispget(lispval, char *, lispval);
lispval Lcfasl(void);
char *gstab(void);
char *mytemp(void);
void ungstab(void);
char *verify(lispval, char *);
lispval verifyl(lispval, char *);
void copyblock(FILE *, FILE *, intptr_t);
lispval Lrmadd(void);
char *Ilibdir(void);

/* fpipe.c */
FILE *fpipe(FILE *[2 ]);
lispval Nioreset(void);
lispval P(FILE *);
FILE *fstopen(char *, int, char *);

/* frame.c */
void Inonlocalgo(int, lispval, lispval);
void Iretfromfr(struct frame *);
int matchtags(lispval, lispval);
lispval Lframedump(void);

/* inits.c */
void initial(void);
void sginth(int);
void sigcall(int);
void delayoff(void);
void dosig(void);
void badmr(int);
void re_enable(int, void (*)(int));

/* io.c */
lispval readr(FILE *);
lispval readrx(int);
void macrox(void);
void imacrox(lispval, int);
lispval ratomr(FILE *);
int Iratom(void);
lispval getnum(char *);
lispval dopow(char *, int);
lispval calcnum(char *, char *, int);
lispval finatom(char *);
char *atomtoointptr_t(char *);
void printr(lispval, FILE *);
int vectorpr(lispval, FILE *);
void lfltpr(char *, double);
void dmpport(FILE *);

/* lam1.c */
lispval Leval(void);
lispval Lxcar(void);
lispval Lxcdr(void);
lispval cxxr(int, int);
lispval Lcar(void);
lispval Lcdr(void);
lispval Lcadr(void);
lispval Lcaar(void);
lispval Lc02r(void);
lispval Lc12r(void);
lispval Lc03r(void);
lispval Lc13r(void);
lispval Lc04r(void);
lispval Lc14r(void);
lispval Lnthelem(void);
lispval Lscons(void);
lispval Lbigtol(void);
lispval Lcons(void);
lispval rpla(int);
lispval Lrplca(void);
lispval Lrplcd(void);
lispval Leq(void);
lispval Lnull(void);
lispval Lreturn(void);
lispval Linfile(void);
lispval Loutfile(void);
lispval Lterpr(void);
lispval Lclose(void);
lispval Ltruename(void);
lispval Lnwritn(void);
lispval Ldrain(void);
lispval Llist(void);
lispval Lnumberp(void);
lispval Latom(void);
lispval Ltype(void);
lispval Ldtpr(void);
lispval Lbcdp(void);
lispval Lportp(void);
lispval Larrayp(void);
lispval Lhunkp(void);
lispval Lset(void);
lispval Lequal(void);
lispval oLequal(void);
int Iequal(lispval, lispval);
lispval Zequal(void);
lispval Lprint(void);
lispval Lpatom(void);
lispval Lpntlen(void);
int Ipntlen(void);
#ifndef okport
FILE *okport(lispval, FILE *);
#endif

/* lam2.c */
lispval Lflatsi(void);
void Iflatsi(lispval);
lispval Lread(void);
lispval Lratom(void);
lispval Lreadc(void);
lispval Lr(int);
lispval Lload(void);
lispval Iconcat(int);
lispval Lconcat(void);
lispval Luconcat(void);
lispval Lputprop(void);
lispval Iputprop(lispval, lispval, lispval);
lispval Lget(void);
lispval Iget(lispval, lispval);
lispval Igetplist(lispval, lispval);
lispval Lgetd(void);
lispval Lputd(void);
lispval Lmapcrx(int, int);
lispval Lmpcar(void);
lispval Lmaplist(void);
lispval Lmapcx(int);
lispval Lmapc(void);
lispval Lmap(void);
lispval Lmapcan(void);
lispval Lmapcon(void);

/* lam3.c */
lispval Lalfalp(void);
lispval Lncons(void);
lispval Lzerop(void);
lispval Lonep(void);
lispval cmpx(int);
lispval Lgreaterp(void);
lispval Llessp(void);
lispval Ldiff(void);
lispval Lmod(void);
lispval Ladd1(void);
lispval Lsub1(void);
lispval Lminus(void);
lispval Lnegp(void);
lispval Labsval(void);
lispval Loblist(void);
lispval Lsetsyn(void);
void rpltab(char, unsigned char *);
lispval Lgetsyntax(void);
lispval Lzapline(void);

/* lam4.c */
lispval Ladd(void);
lispval Lsub(void);
lispval Ltimes(void);
lispval Lquo(void);
lispval Lfp(void);
lispval Lfm(void);
lispval Lft(void);
lispval Lflessp(void);
lispval Lfd(void);
lispval Lfadd1(void);
lispval Lfexpt(void);
lispval Lfsub1(void);
lispval Ldbtofl(void);
lispval Lfltodb(void);

/* lam5.c */
lispval Lexpldx(int, int);
lispval Lxpldc(void);
lispval Lxpldn(void);
lispval Lxplda(void);
lispval Largv(void);
lispval Lchdir(void);
lispval Lascii(void);
lispval Lboole(void);
lispval Lfact(void);
lispval Lfix(void);
lispval Lfrexp(void);
lispval Lfloat(void);
double Ifloat(lispval);
lispval Lbreak(void);
lispval LDivide(void);
lispval LEmuldiv(void);

/* lam6.c */
lispval Lreadli(void);
lispval Lgetenv(void);
lispval Lboundp(void);
lispval Lplist(void);
lispval Lsetpli(void);
lispval Lsignal(void);
lispval Lassq(void);
lispval Lkilcopy(void);
lispval Larg(void);
lispval Lsetarg(void);
lispval Lptime(void);
lispval Lerr(void);
lispval Ltyi(void);
lispval Luntyi(void);
lispval Ltyipeek(void);
lispval Ltyo(void);
lispval Imkrtab(int);
lispval Lmakertbl(void);
lispval Lcpy1(void);
lispval Lcopyint(void);

/* lam7.c */
lispval Lfork(void);
lispval Lwait(void);
lispval Lpipe(void);
lispval Lfdopen(void);
lispval Lexece(void);
lispval Lprocess(void);
lispval Lgensym(void);
lispval Lremprop(void);
lispval Lbcdad(void);
lispval Lstringp(void);
lispval Lsymbolp(void);
lispval Lrematom(void);
lispval Lprname(void);
lispval Lexit(void);
lispval Iimplode(int);
lispval Lmaknam(void);
lispval Limplode(void);
lispval Lntern(void);
lispval Ibindvars(void);
lispval Iunbindvars(void);
lispval Ltymestr(void);

/* lam8.c */
lispval Imath(double (*func)(double));
lispval Lsin(void);
lispval Lcos(void);
lispval Lasin(void);
lispval Lacos(void);
lispval Lsqrt(void);
lispval Lexp(void);
lispval Llog(void);
lispval Latan(void);
lispval Lrandom(void);
lispval Lmakunb(void);
lispval Lfseek(void);
lispval Lhashst(void);
lispval Lctcherr(void);
lispval LMakhunk(void);
lispval Lcxr(void);
lispval Lrplcx(void);
lispval Lstarrpx(void);
lispval Lhunksize(void);
lispval Lhtol(void);
lispval Lfileopen(void);
intptr_t invmod(intptr_t, intptr_t);
lispval Lstarinvmod(void);
lispval LstarMod(void);
lispval Llsh(void);
void bndchk(void);
lispval Lcprintf(void);
lispval Lsprintf(void);
lispval Lprobef(void);
lispval Lsubstring(void);
lispval Lsstrn(void);
lispval Lcharindex(void);
lispval Lpurcopy(void);
lispval Ipurcopy(lispval);
lispval Lpurep(void);
lispval Lnvec(void);
lispval Lnvecb(void);
lispval Lnvecw(void);
lispval Lnvecl(void);
lispval Inewvector(int);
lispval Lvectorp(void);
lispval Lpvp(void);
lispval LIvref(void);
lispval LIvset(void);
lispval LIvsize(void);
lispval Lvprop(void);
lispval Lvsp(void);
int vecequal(lispval, lispval);
int veciequal(lispval, lispval);

/* lam9.c */
lispval Ltci(void);
lispval Ltcx(void);
lispval LIfranzcall(void);

/* lamgc.c */
lispval Lgcstat(void);
void gcdump(void);

/* lamp.c */
lispval Lmonitor(void);

/* lamr.c */
lispval Lalloc(void);
lispval Lsizeof(void);
lispval Lsegment(void);
lispval Lforget(void);
lispval Lgetl(void);
lispval Lputl(void);
lispval Lgetdel(void);
lispval Lputdel(void);
lispval Lgetaux(void);
lispval Lputaux(void);
lispval Lgetdata(void);
lispval Lputdata(void);
lispval Lgeta(void);
lispval Lputa(void);
lispval Lmarray(void);
lispval Lgtentry(void);
lispval Lgetlang(void);
lispval Lputlang(void);
lispval Lgetparams(void);
lispval Lputparams(void);
lispval Lgetdisc(void);
lispval Lputdisc(void);
lispval Lgetloc(void);
lispval Lputloc(void);
lispval Lmfunction(void);
lispval Lreplace(void);
lispval Lvaluep(void);
void CNTTYP(void);
lispval Lod(void);
lispval Lfake(void);
lispval Lmaknum(void);
lispval Lderef(void);
lispval Lpname(void);
lispval Larayref(void);
lispval Lptr(void);
lispval Llctrace(void);
lispval Lslevel(void);
lispval Lsimpld(void);
lispval Lopval(void);

/* lisp.c */
int main(int, char **, char **);
lispval Ntpl(void);
void franzexit(int) __attribute__((__noreturn__));

/* low.c */

/* pbignum.c */
void pbignum(lispval, FILE *);

/* rlc.c */
int rlc(void);

/* subbig.c */
lispval subbig(lispval, lispval);

/* sysat.c */
void makevals(void);
lispval matom(char *);
lispval mstr(char *);
lispval mfun(char *, lispval (*start)(void), lispval);
lispval mftab(struct ftab *);

/* trace.c */
lispval Leval1(void);
lispval Levalhook(void);
lispval Lfunhook(void);
lispval Lrset(void);

/* adbig.c */
lispval adbig(lispval, lispval);

/* callg.c */
lispval callg(lispval (*fn)(void), intptr_t[]);

/* calqhat.c */
intptr_t calqhat(intptr_t *, intptr_t *);

/* clinkfns.c */
lispval set_redef(void);
void clink(char *[], int, lispval **, char *[], int, struct trent **, struct bindspec *);
lispval clinker(void);

/* dmlad.c */
void dmlad(lispval, int, int);

/* dodiv.c */
intptr_t dodiv(intptr_t *, intptr_t *);
void dsneg(intptr_t *, intptr_t *);

/* dsmult.c */
void dsmult(intptr_t *, intptr_t *, intptr_t);

/* ediv.c */
int ediv(int[2 ], int, char *);

/* emul.c */
int emul(int, int, int, int[2 ]);

/* exarith.c */
intptr_t exarith(int, int, intptr_t, intptr_t *, intptr_t *);

/* i386.c */
int mmuladd(intptr_t, intptr_t, intptr_t, intptr_t);
void Imuldiv(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t *, intptr_t *);
lispval Lpolyev(void);
lispval Lrot(void);
lispval Lshostk(void);
lispval Lbaktrace(void);
lispval LIshowstack(void);
void myfrexp(double, int *, int *, int *);
lispval Lmkcth(void);

/* inewint.c */
lispval inewint(int);

/* mlsb.c */
intptr_t mlsb(intptr_t *, intptr_t *, intptr_t *, int);
intptr_t adback(intptr_t *, intptr_t *, intptr_t *);
intptr_t dsdiv(intptr_t *, intptr_t *, intptr_t);
void dsadd1(intptr_t *, intptr_t *);
intptr_t dsrsh(intptr_t *, intptr_t *, intptr_t, intptr_t);

/* mulbig.c */
lispval mulbig(lispval, lispval);

/* nargs.c */
int nargs(intptr_t);

/* prunei.c */
void prunei(lispval);

/* qfuncl.c */
int qcons(void);
int qget(void);
int qlinker(void);
int qnewdoub(void);
int qnewint(void);
int qoneminus(void);
int qoneplus(void);
int qpushframe(void);
int mcount(void);
void *gstart(void);
void vlsub(int *, int *);
struct frame *Ipushf(int, lispval, lispval, struct frame *);
int qretfromfr(struct frame *);

/* debug */
void dump(const char *, lispval, int);
