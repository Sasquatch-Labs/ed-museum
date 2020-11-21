/*
 * Editor
 */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#define	FNSIZE	64
#define	LBSIZE	512
#define	ESIZE	128
#define	GBSIZE	256
#define	NBRA	5
#define	EOF	-1

#define	CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	NCCL	8
#define	CDOL	10
#define	CEOF	11
#define	CKET	12

#define	STAR	01

#define	error	errfunc()
#define	READ	0
#define	WRITE	1

int	peekc;
int	lastc;
char	savedfile[FNSIZE];
char	file[FNSIZE];
char	linebuf[LBSIZE];
char	rhsbuf[LBSIZE/2];
char	expbuf[ESIZE+4];
int	circfl;
int	*zero;
int	*dot;
int	*dol;
unsigned int nlall = 128;
int	*addr1;
int	*addr2;
char	genbuf[LBSIZE];
int	count;
char	*nextip;
char	*linebp;
int	ninbuf;
int	io;
int	pflag;
void (*onhup)(int);
void (*onquit)(int);
int	vflag = 1;
int	listf;
int	col;
char	*globp;
int	tfile = -1;
int	tline;
char	tfname[] = "/tmp/eXXXXXX";
char	*loc1;
char	*loc2;
char	*locs;
char	ibuff[512];
int	iblock = -1;
char	obuff[512];
int	oblock = -1;
int	ichanged;
int	nleft;
void errfunc();
char	TMPERR[] = "TMP";
int	names[26];
char	*braslist[NBRA];
char	*braelist[NBRA];
jmp_buf savej;

int *address(void);
void commands(void);

char	line[70];
char	*linp = line;

void
edputchar(int ac)
{
	char *lp;
	int c;

	lp = linp;
	c = ac;
	if (listf) {
		col++;
		if (col >= 72) {
			col = 0;
			*lp++ = '\\';
			*lp++ = '\n';
		}
		if (c=='\t') {
			c = '>';
			goto esc;
		}
		if (c=='\b') {
			c = '<';
		esc:
			*lp++ = '-';
			*lp++ = '\b';
			*lp++ = c;
			goto out;
		}
		if (c<' ' && c!= '\n') {
			*lp++ = '\\';
			*lp++ = (c>>3)+'0';
			*lp++ = (c&07)+'0';
			col =+ 2;
			goto out;
		}
	}
	*lp++ = c;
out:
	if(c == '\n' || lp >= &line[64]) {
		linp = line;
		write(1, line, lp-line);
		return;
	}
	linp = lp;
}

void
putd(void)
{
	int r;

	r = count % 10;
	count = count/10;
	if (count)
		putd();
	edputchar(r + '0');
}

void
edputs(char *as)
{
	register char *sp;

	sp = as;
	col = 0;
	while (*sp)
		edputchar(*sp++);
	edputchar('\n');
}

void
blkio(int b, char *buf, ssize_t (*iofcn)(int, void *, size_t))
{
	lseek(tfile, b<<9, 0);
	if ((*iofcn)(tfile, buf, 512) != 512) {
		edputs(TMPERR);
		error;
	}
}

ssize_t
edwrite(int fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

char *
getblock(int atl, int iof)
{
	/*extern read(), write();*/
	int bno, off;
	
	bno = (atl>>8)&0377;
	off = (atl<<1)&0774;
	if (bno >= 255) {
		edputs(TMPERR);
		error;
	}
	nleft = 512 - off;
	if (bno==iblock) {
		ichanged |= iof;
		return(ibuff+off);
	}
	if (bno==oblock)
		return(obuff+off);
	if (iof==READ) {
		if (ichanged)
			blkio(iblock, ibuff, edwrite);
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);
		return(ibuff+off);
	}
	if (oblock>=0)
		blkio(oblock, obuff, edwrite);
	oblock = bno;
	return(obuff+off);
}

char *
getline(int tl)
{
	char *bp, *lp;
	int nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~0377;
	while (*lp++ = *bp++)
		if (--nl == 0) {
			bp = getblock(tl+=0400, READ);
			nl = nleft;
		}
	return(linebuf);
}

int
putline(void)
{
	char *bp, *lp;
	int nl, tl;

	lp = linebuf;
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~0377;
	while (*bp = *lp++) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl+=0400, WRITE);
			nl = nleft;
		}
	}
	nl = tline;
	tline += (((lp-linebuf)+03)>>1)&077776;
	return(nl);
}

void
setdot(void)
{
	if (addr2 == 0)
		addr1 = addr2 = dot;
	if (addr1 > addr2)
		error;
}

void
setall(void)
{
	if (addr2==0) {
		addr1 = zero+1;
		addr2 = dol;
		if (dol==zero)
			addr1 = zero;
	}
	setdot();
}

void
setnoaddr(void)
{
	if (addr2)
		error;
}

void
nonzero(void)
{
	if (addr1<=zero || addr2>dol)
		error;
}

void
newline(void)
{
	register c;

	if ((c = getchar()) == '\n')
		return;
	if (c=='p' || c=='l') {
		pflag++;
		if (c=='l')
			listf++;
		if (getchar() == '\n')
			return;
	}
	error;
}

void
filename(void)
{
	register char *p1, *p2;
	register c;

	count = 0;
	c = getchar();
	if (c=='\n' || c==EOF) {
		p1 = savedfile;
		if (*p1==0)
			error;
		p2 = file;
		while (*p2++ = *p1++);
		return;
	}
	if (c!=' ')
		error;
	while ((c = getchar()) == ' ');
	if (c=='\n')
		error;
	p1 = file;
	do {
		*p1++ = c;
	} while ((c = getchar()) != '\n');
	*p1++ = 0;
	if (savedfile[0]==0) {
		p1 = savedfile;
		p2 = file;
		while (*p1++ = *p2++);
	}
}

void
exfile(void)
{
	close(io);
	io = -1;
	if (vflag) {
		putd();
		edputchar('\n');
	}
}

void
onintr(int ign)
{
	signal(SIGINT, onintr);
	edputchar('\n');
	lastc = '\n';
	error;
}

void
errfunc(void)
{
	int c;

	listf = 0;
	edputs("?");
	count = 0;
	lseek(0, 0, 2);
	pflag = 0;
	if (globp)
		lastc = '\n';
	globp = 0;
	peekc = lastc;
	while ((c = getchar()) != '\n' && c != EOF);
	if (io > 0) {
		close(io);
		io = -1;
	}
	longjmp(savej, 1);
}

int
getchar(void)
{
	if (lastc=peekc) {
		peekc = 0;
		return(lastc);
	}
	if (globp) {
		if ((lastc = *globp++) != 0)
			return(lastc);
		globp = 0;
		return(EOF);
	}
	if (read(0, &lastc, 1) <= 0)
		return(lastc = EOF);
	lastc &= 0177;
	return(lastc);
}

int
gettty(void)
{
	int c;
	char *gf;
	char *p;

	p = linebuf;
	gf = globp;
	while ((c = getchar()) != '\n') {
		if (c==EOF) {
			if (gf)
				peekc = c;
			return(c);
		}
		if ((c &= 0177) == 0)
			continue;
		*p++ = c;
		if (p >= &linebuf[LBSIZE-2])
			error;
	}
	*p++ = 0;
	if (linebuf[0]=='.' && linebuf[1]==0)
		return(EOF);
	return(0);
}

int
getfile(void)
{
	int c;
	char *lp, *fp;

	lp = linebuf;
	fp = nextip;
	do {
		if (--ninbuf < 0) {
			if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0)
				return(EOF);
			fp = genbuf;
		}
		if (lp >= &linebuf[LBSIZE])
			error;
		if ((*lp++ = c = *fp++ & 0177) == 0) {
			lp--;
			continue;
		}
		++count;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	return(0);
}

void
putfile(void)
{
	int *a1;
	char *fp, *lp;
	int nib;

	nib = 512;
	fp = genbuf;
	a1 = addr1;
	do {
		lp = getline(*a1++);
		for (;;) {
			if (--nib < 0) {
				write(io, genbuf, fp-genbuf);
				nib = 511;
				fp = genbuf;
			}
			++count;
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	write(io, genbuf, fp-genbuf);
}

int
append(int (*f)(), int *a)
{
	int *a1, *a2, *rdot;
	int nline, tl;

	nline = 0;
	dot = a;
	while ((*f)() == 0) {
		if ((dol-zero)+1 >= nlall) {
			int *ozero = zero;
			nlall += 512;
			if ((zero = (int *)realloc((char *)zero, nlall*sizeof(int)))==NULL) {
				lastc = '\n';
				zero = ozero;
				error;
			}
			dot += zero - ozero;
			dol += zero - ozero;
		}
		tl = putline();
		nline++;
		a1 = ++dol;
		a2 = a1+1;
		rdot = ++dot;
		while (a1 > rdot)
			*--a2 = *--a1;
		*rdot = tl;
	}
	return(nline);
}

void
unix(void)
{
	void (*savint)(int);
	int pid, rpid;
	int retcode;

	setnoaddr();
	if ((pid = fork()) == 0) {
		signal(SIGHUP, onhup);
		signal(SIGQUIT, onquit);
		execl("/bin/sh", "sh", "-t", 0);
		exit(0);
	}
	savint = signal(SIGINT, SIG_DFL);
	while ((rpid = wait(&retcode)) != pid && rpid != -1);
	signal(SIGINT, savint);
	edputs("!");
}

void
delete(void)
{
	int *a1, *a2, *a3;

	setdot();
	newline();
	nonzero();
	a1 = addr1;
	a2 = addr2+1;
	a3 = dol;
	dol -= a2 - a1;
	do
		*a1++ = *a2++;
	while (a2 <= a3);
	a1 = addr1;
	if (a1 > dol)
		a1 = dol;
	dot = a1;
}

void
init(void)
{
	char *p;

	tline = 0;
	iblock = -1;
	oblock = -1;
	ichanged = 0;
	tfile = mkstemp(tfname);
	dot = zero = dol = malloc(nlall*sizeof(int));
}

int
cclass(char *aset, char ac, int af)
{
	char *set, c;
	int n;

	set = aset;
	if ((c = ac) == 0)
		return(0);
	n = *set++;
	while (--n)
		if (*set++ == c)
			return(af);
	return(!af);
}

int
advance(char *alp, char *aep)
{
	char *lp, *ep, *curlp;
	char *nextep;

	lp = alp;
	ep = aep;
	for (;;) switch (*ep++) {

	case CCHR:
		if (*ep++ == *lp++)
			continue;
		return(0);

	case CDOT:
		if (*lp++)
			continue;
		return(0);

	case CDOL:
		if (*lp==0)
			continue;
		return(0);

	case CEOF:
		loc2 = lp;
		return(1);

	case CCL:
		if (cclass(ep, *lp++, 1)) {
			ep += *ep;
			continue;
		}
		return(0);

	case NCCL:
		if (cclass(ep, *lp++, 0)) {
			ep += *ep;
			continue;
		}
		return(0);

	case CBRA:
		braslist[*ep++] = lp;
		continue;

	case CKET:
		braelist[*ep++] = lp;
		continue;

	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
		goto star;

	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;

	case CCL|STAR:
	case NCCL|STAR:
		curlp = lp;
		while (cclass(ep, *lp++, ep[-1]==(CCL|STAR)));
		ep += *ep;
		goto star;

	star:
		do {
			lp--;
			if (lp==locs)
				break;
			if (advance(lp, ep))
				return(1);
		} while (lp > curlp);
		return(0);

	default:
		error;
	}
}

int
execute(int gf, int *addr)
{
	char *p1, *p2, c;

	if (gf) {
		if (circfl)
			return(0);
		p1 = linebuf;
		p2 = genbuf;
		while (*p1++ = *p2++);
		locs = p1 = loc2;
	} else {
		if (addr==zero)
			return(0);
		p1 = getline(*addr);
		locs = 0;
	}
	p2 = expbuf;
	if (circfl) {
		loc1 = p1;
		return(advance(p1, p2));
	}
	/* fast check for first character */
	if (*p2==CCHR) {
		c = p2[1];
		do {
			if (*p1!=c)
				continue;
			if (advance(p1, p2)) {
				loc1 = p1;
				return(1);
			}
		} while (*p1++);
		return(0);
	}
	/* regular algorithm */
	do {
		if (advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while (*p1++);
	return(0);
}

void
compile(int aeof)
{
	int eof, c;
	char *ep;
	char *lastep;
	char bracket[NBRA], *bracketp;
	int nbra;
	int cclcnt;

	ep = expbuf;
	eof = aeof;
	bracketp = bracket;
	nbra = 0;
	if ((c = getchar()) == eof) {
		if (*ep==0)
			error;
		return;
	}
	circfl = 0;
	if (c=='^') {
		c = getchar();
		circfl++;
	}
	if (c=='*')
		goto cerror;
	peekc = c;
	for (;;) {
		if (ep >= &expbuf[ESIZE])
			goto cerror;
		c = getchar();
		if (c==eof) {
			*ep++ = CEOF;
			return;
		}
		if (c!='*')
			lastep = ep;
		switch (c) {

		case '\\':
			if ((c = getchar())=='(') {
				if (nbra >= NBRA)
					goto cerror;
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;
			}
			if (c == ')') {
				if (bracketp <= bracket)
					goto cerror;
				*ep++ = CKET;
				*ep++ = *--bracketp;
				continue;
			}
			*ep++ = CCHR;
			if (c=='\n')
				goto cerror;
			*ep++ = c;
			continue;

		case '.':
			*ep++ = CDOT;
			continue;

		case '\n':
			goto cerror;

		case '*':
			if (*lastep==CBRA || *lastep==CKET)
				error;
			*lastep |= STAR;
			continue;

		case '$':
			if ((peekc=getchar()) != eof)
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			*ep++ = CCL;
			*ep++ = 0;
			cclcnt = 1;
			if ((c=getchar()) == '^') {
				c = getchar();
				ep[-2] = NCCL;
			}
			do {
				if (c=='\n')
					goto cerror;
				*ep++ = c;
				cclcnt++;
				if (ep >= &expbuf[ESIZE])
					goto cerror;
			} while ((c = getchar()) != ']');
			lastep[1] = cclcnt;
			continue;

		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
   cerror:
	expbuf[0] = 0;
	error;
}

void
global(int k)
{
	char *gp;
	int c;
	int *a1;
	char globuf[GBSIZE];

	if (globp)
		error;
	setall();
	nonzero();
	if ((c=getchar())=='\n')
		error;
	compile(c);
	gp = globuf;
	while ((c = getchar()) != '\n') {
		if (c==EOF)
			error;
		if (c=='\\') {
			c = getchar();
			if (c!='\n')
				*gp++ = '\\';
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE-2])
			error;
	}
	*gp++ = '\n';
	*gp++ = 0;
	for (a1=zero; a1<=dol; a1++) {
		*a1 &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(0, a1)==k)
			*a1 |= 01;
	}
	for (a1=zero; a1<=dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			globp = globuf;
			commands();
			a1 = zero;
		}
	}
}

int
compsub(void)
{
	int seof, c;
	char *p;
	int gsubf;

	if ((seof = getchar()) == '\n')
		error;
	compile(seof);
	p = rhsbuf;
	for (;;) {
		c = getchar();
		if (c=='\\')
			c = getchar() | 0200;
		if (c=='\n')
			error;
		if (c==seof)
			break;
		*p++ = c;
		if (p >= &rhsbuf[LBSIZE/2])
			error;
	}
	*p++ = 0;
	if ((peekc = getchar()) == 'g') {
		peekc = 0;
		newline();
		return(1);
	}
	newline();
	return(0);
}

int
getsub(void)
{
	char *p1, *p2;

	p1 = linebuf;
	if ((p2 = linebp) == 0)
		return(EOF);
	while (*p1++ = *p2++);
	linebp = 0;
	return(0);
}

char *
place(char *sp, char *l1, char *l2)
{
	while (l1 < l2) {
		*sp++ = *l1++;
		if (sp >= &genbuf[LBSIZE])
			error;
	}
	return(sp);
}

void
dosub(void)
{
	char *lp, *sp, *rp;
	int c;

	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	while (c = *rp++) {
		if (c=='&') {
			sp = place(sp, loc1, loc2);
			continue;
		} else if (c<0 && (c &= 0177) >='1' && c < NBRA+'1') {
			sp = place(sp, braslist[c-'1'], braelist[c-'1']);
			continue;
		}
		*sp++ = c&0177;
		if (sp >= &genbuf[LBSIZE])
			error;
	}
	lp = loc2;
	loc2 = sp + (linebuf - genbuf);
	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE])
			error;
	lp = linebuf;
	sp = genbuf;
	while (*lp++ = *sp++);
}

void
substitute(int inglob)
{
	register gsubf, *a1, nl;
	int getsub();

	gsubf = compsub();
	for (a1 = addr1; a1 <= addr2; a1++) {
		if (execute(0, a1)==0)
			continue;
		inglob |= 01;
		dosub();
		if (gsubf) {
			while (*loc2) {
				if (execute(1, NULL)==0)
					break;
				dosub();
			}
		}
		*a1 = putline();
		nl = append(getsub, a1);
		a1 += nl;
		addr2 += nl;
	}
	if (inglob==0)
		error;
}

void
reverse(int *aa1, int *aa2)
{
	int *a1, *a2, t;

	a1 = aa1;
	a2 = aa2;
	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

void
move(int cflag)
{
	int *adt, *ad1, *ad2;
	int getcopy();

	setdot();
	nonzero();
	if ((adt = address())==0)
		error;
	newline();
	ad1 = addr1;
	ad2 = addr2;
	if (cflag) {
		ad1 = dol;
		append(getcopy, ad1++);
		ad2 = dol;
	}
	ad2++;
	if (adt<ad1) {
		dot = adt + (ad2-ad1);
		if ((++adt)==ad1)
			return;
		reverse(adt, ad1);
		reverse(ad1, ad2);
		reverse(adt, ad2);
	} else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
	} else
		error;
}

int
getcopy(void)
{
	if (addr1 > addr2)
		return(EOF);
	getline(*addr1++);
	return(0);
}


int *
address(void)
{
	int *a1, minus, c;
	int n, relerr;

	minus = 0;
	a1 = 0;
	for (;;) {
		c = getchar();
		if ('0'<=c && c<='9') {
			n = 0;
			do {
				n *= 10;
				n += c - '0';
			} while ((c = getchar())>='0' && c<='9');
			peekc = c;
			if (a1==0)
				a1 = zero;
			if (minus<0)
				n = -n;
			a1 += n;
			minus = 0;
			continue;
		}
		relerr = 0;
		if (a1 || minus)
			relerr++;
		switch(c) {
		case ' ':
		case '\t':
			continue;
	
		case '+':
			minus++;
			if (a1==0)
				a1 = dot;
			continue;

		case '-':
		case '^':
			minus--;
			if (a1==0)
				a1 = dot;
			continue;
	
		case '?':
		case '/':
			compile(c);
			a1 = dot;
			for (;;) {
				if (c=='/') {
					a1++;
					if (a1 > dol)
						a1 = zero;
				} else {
					a1--;
					if (a1 < zero)
						a1 = dol;
				}
				if (execute(0, a1))
					break;
				if (a1==dot)
					error;
			}
			break;
	
		case '$':
			a1 = dol;
			break;
	
		case '.':
			a1 = dot;
			break;

		case '\'':
			if ((c = getchar()) < 'a' || c > 'z')
				error;
			for (a1=zero; a1<=dol; a1++)
				if (names[c-'a'] == (*a1|01))
					break;
			break;
	
		default:
			peekc = c;
			if (a1==0)
				return(0);
			a1 += minus;
			if (a1<zero || a1>dol)
				error;
			return(a1);
		}
		if (relerr)
			error;
	}
}

void
commands(void)
{
	int getfile(), gettty();
	register *a1, c;
	register char *p;
	int r;

	for (;;) {
	if (pflag) {
		pflag = 0;
		addr1 = addr2 = dot;
		goto print;
	}
	addr1 = 0;
	addr2 = 0;
	do {
		addr1 = addr2;
		if ((a1 = address())==0) {
			c = getchar();
			break;
		}
		addr2 = a1;
		if ((c=getchar()) == ';') {
			c = ',';
			dot = a1;
		}
	} while (c==',');
	if (addr1==0)
		addr1 = addr2;
	switch(c) {

	case 'a':
		setdot();
		newline();
		append(gettty, addr2);
		continue;

	case 'c':
		delete();
		append(gettty, addr1-1);
		continue;

	case 'd':
		delete();
		continue;

	case 'e':
		setnoaddr();
		if ((peekc = getchar()) != ' ')
			error;
		savedfile[0] = 0;
		init();
		addr2 = zero;
		goto caseread;

	case 'f':
		setnoaddr();
		if ((c = getchar()) != '\n') {
			peekc = c;
			savedfile[0] = 0;
			filename();
		}
		edputs(savedfile);
		continue;

	case 'g':
		global(1);
		continue;

	case 'i':
		setdot();
		nonzero();
		newline();
		append(gettty, addr2-1);
		continue;

	case 'k':
		if ((c = getchar()) < 'a' || c > 'z')
			error;
		newline();
		setdot();
		nonzero();
		names[c-'a'] = *addr2 | 01;
		continue;

	case 'm':
		move(0);
		continue;

	case '\n':
		if (addr2==0)
			addr2 = dot+1;
		addr1 = addr2;
		goto print;

	case 'l':
		listf++;
	case 'p':
		newline();
	print:
		setdot();
		nonzero();
		a1 = addr1;
		do
			edputs(getline(*a1++));
		while (a1 <= addr2);
		dot = addr2;
		listf = 0;
		continue;

	case 'q':
		setnoaddr();
		newline();
		unlink(tfname);
		exit(0);

	case 'r':
	caseread:
		filename();
		if ((io = open(file, 0)) < 0) {
			lastc = '\n';
			error;
		}
		setall();
		ninbuf = 0;
		append(getfile, addr2);
		exfile();
		continue;

	case 's':
		setdot();
		nonzero();
		substitute(globp!=0);
		continue;

	case 't':
		move(1);
		continue;

	case 'v':
		global(0);
		continue;

	case 'w':
		setall();
		nonzero();
		filename();
		if ((io = creat(file, 0666)) < 0)
			error;
		putfile();
		exfile();
		continue;

	case '=':
		setall();
		newline();
		count = (addr2-zero)&077777;
		putd();
		edputchar('\n');
		continue;

	case '!':
		unix();
		continue;

	case EOF:
		return;

	}
	error;
	}
}

int
main(int argc, char **argv)
{
	char *p1, *p2;

	onquit = signal(SIGQUIT, SIG_IGN);
	onhup = signal(SIGHUP, SIG_IGN);
	argv++;
	if (argc > 1 && **argv=='-') {
		vflag = 0;
		/* allow debugging quits? */
		if ((*argv)[1]=='q') {
			signal(SIGQUIT, SIG_DFL);
			vflag++;
		}
		argv++;
		argc--;
	}
	if (argc>1) {
		p1 = *argv;
		p2 = savedfile;
		while (*p2++ = *p1++);
		globp = "r";
	}
	init();
	if (signal(SIGINT, SIG_IGN) == SIG_DFL)
		signal(SIGINT, onintr);
	setjmp(savej);
	commands();
	unlink(tfname);
}
