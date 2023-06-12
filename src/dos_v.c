/* vi:set ts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Read the file "credits.txt" for a list of people who contributed.
 * Read the file "uganda.txt" for copying and usage conditions.
 */

/*
 * msdos.c
 *
 * MSDOS system-dependent routines.
 * A cheap plastic imitation of the amiga dependent code.
 * A lot in this file was made by Juergen Weigert (jw).
 */

#include <io.h>
#include <fcntl.h>
#ifndef __GO32__
#include <alloc.h>
#endif
#include <signal.h>
#include "vim.h"
#include "globals.h"
#include "param.h"
#include "proto.h"
#ifdef KANJI
#include "kanji.h"
#endif

static int kbhit __ARGS((void));
static int getch __ARGS((void));
static int WaitForChar __ARGS((int));
static int change_drive __ARGS((int));
static int get_row __ARGS((void));

typedef struct filelist
{
	char_u	**file;
	int		nfiles;
	int		maxfiles;
} FileList;

static void		addfile __ARGS((FileList *, char_u *, int));
static int		pstrcmp();	/* __ARGS((char **, char **)); BCC does not like this */
static void		strlowcpy __ARGS((char_u *, char_u *));
static int		expandpath __ARGS((FileList *, char_u *, int, int, int));

static int cbrk_pressed = FALSE;	/* set by ctrl-break interrupt */
static int ctrlc_pressed = FALSE;	/* set when ctrl-C or ctrl-break detected */
static int delayed_redraw = FALSE;	/* set when ctrl-C detected */

	long
mch_avail_mem(special)
	int special;
{
#ifdef __GO32__
	return 0x7fffffff;		/* virual memory eh */
#else
	return coreleft();
#endif
}

	void
vim_delay()
{
#if !defined(__GO32__) && defined(__BORLANDC__)
	if (term_console)
	{
		int		x = 500;
		while (x > 0)
		{
			if (kbhit())
				break;
			delay(100);
			x -= 100;
		}
	}
	else
#endif
	sleep(1);
}

/*
 * this version of remove is not scared by a readonly (backup) file
 *
 * returns -1 on error, 0 otherwise (just like remove())
 */
	int
vim_remove(name)
	char_u *name;
{
	setperm(name, S_IREAD|S_IWRITE);    /* default permissions */
	return unlink(name);
}

static int v_col, v_row;	/* current cursor position */
static int v_csize;			/* cusor size */
static int v_attr = 0x1f;
static int int29h = FALSE;
static int v_max_row;
#define v_refresh()			/* refresh real cursor position */	\
{union REGS regs; regs.h.ah=2; regs.h.bh=0; regs.h.dh=v_row; regs.h.dl=v_col; int86(0x10, &regs, &regs);}

/*
 * mch_write(): write the output buffer to the screen
 */
	void
mch_write(s, len)
	char_u	*s;
	int		len;
{
	char_u	*p;
	int		row, col;
	union REGS regs;

	if (term_console)		/* translate ESC | sequences into bios calls */
	{
		while (len--)
		{
			if (s[0] == '\n')
			{
				v_col = 0;
				v_row++;
				v_refresh();
				if (v_row >= (Rows - 1))
				{
					regs.x.ax = 0x601;
					regs.h.bh = v_attr;
					regs.x.cx = 0;
					regs.x.dx = ((Rows - 1) << 8) + Columns - 1;
					int86(0x10, &regs, &regs);
					v_row = Rows - 1;
				}
				v_refresh();
				s++;
				continue;
			}
			else if (s[0] == '\r')
			{
				v_col = 0;
				v_refresh();
				s++;
				continue;
			}
			else if (s[0] == '\b')		/* backspace */
			{
				if (--v_col < 0)
					v_col = 0;
				v_refresh();
				s++;
				continue;
			}
			else if (s[0] == '\a' || s[0] == '\007')
			{
				regs.x.ax = 0xe00 | s[0];
				regs.h.bh = 0;
				regs.h.bl = v_attr;
				int86(0x10, &regs, &regs);
				v_refresh();
				s++;
				continue;
			}
			else if (s[0] == ESC && len > 1 && s[1] == '|')
			{
				switch (s[2]) {
				case 'v':
							regs.x.ax = 0x200;
							regs.h.ch = 0x20;
							regs.h.cl = 0;
							int86(0x10, &regs, &regs);
							goto got3;

				case 'V':
							regs.x.ax = 0x200;
							regs.h.ch = 6;
							regs.h.cl = 7;
							int86(0x10, &regs, &regs);
							goto got3;

				case 'W':
							regs.x.ax = 0x200;
							regs.h.ch = 0;
							regs.h.cl = 7;
							int86(0x10, &regs, &regs);
							goto got3;

				case 'J':	/*clrscr();*/
							regs.x.ax = 0x600;
							regs.h.bh = v_attr;
							regs.x.cx = 0;
							regs.x.dx = ((v_max_row - 1) << 8) + Columns - 1;
							int86(0x10, &regs, &regs);
							v_col = v_row = 0;
							goto got3;

				case 'K':	/*clreol();*/
							regs.h.ah = 0x6;
							regs.h.al = 0;
							regs.h.bh = v_attr;
							regs.h.ch = v_row;
							regs.h.cl = v_col;
							regs.h.dh = v_row;
							regs.h.dl = Columns - 1;
							int86(0x10, &regs, &regs);
							goto got3;

				case 'L':	/*insline();*/
							regs.x.ax = 0x701;
							regs.h.bh = v_attr;
							regs.x.cx = v_row << 8;
							regs.x.dx = ((Rows - 1) << 8) + Columns - 1;
							int86(0x10, &regs, &regs);
							goto got3;

				case 'M':	/*delline();*/
							regs.x.ax = 0x601;
							regs.h.bh = v_attr;
							regs.x.cx = v_row << 8;
							regs.x.dx = ((Rows - 1) << 8) + Columns - 1;
							int86(0x10, &regs, &regs);
got3:						v_refresh();
							s += 3;
							len -= 2;
							continue;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':	p = s + 2;
							row = getdigits(&p);		/* no check for length! */
							if (p > s + len)
								break;
							if (*p == ';')
							{
								++p;
								col = getdigits(&p);/* no check for length! */
								if (p > s + len)
									break;
								if (*p == 'H' || *p == 'r')
								{
									if (*p == 'H')	/* set cursor position */
									{
										v_col = col - 1;
										v_row = row - 1;
									}
									else				/* set scroll region  */
									{
										;/*window(1, row, Columns, col);*/
									}
									len -= p - s;
									s = p + 1;
									v_refresh();
									continue;
								}
							}
							else if (*p == 'm')
							{
								if (row == 0)
									v_attr = 0x0f;
								else
									v_attr = row/*0x3f*/;
								len -= p - s;
								s = p + 1;
								continue;
							}
				}
			}
			else if (s[0] >= ' ')
			{
				regs.h.ah = 0x09;
				regs.h.al = s[0];
				regs.h.bh = 0;
				regs.h.bl = v_attr;
				regs.x.cx = 1;
				int86(0x10, &regs, &regs);
				v_col ++;
				if (v_col >= Columns)
				{
					v_col = 0;
					if (++v_row >= Rows)
					{
						regs.x.ax = 0x601;
						regs.h.bh = v_attr;
						regs.x.cx = 0;
						regs.x.dx = ((Rows - 1) << 8) + Columns - 1;
						int86(0x10, &regs, &regs);
						v_row = Rows - 1;
					}
				}
				s++;
				v_refresh();
				continue;
			}
			s++;
		}
	}
	else
	{
#ifdef notdef
		write(1, s, (unsigned)len);
#else
		if (int29h)
		{
			while (len--)
			{
				regs.x.ax = *s++;
				int86(0x29, &regs, &regs);
			}
		}
		else
		{
			while (len--)
				bdos(6, *s++, 0);
		}
#endif
	}
}

	static int
kbhit()
{
	return(bdos(0xb, 0, 0) & 0xff);
}

	static int
getch()
{
	return(bdos(7, 0, 0) & 0xff);
}

#define POLL_SPEED 10	/* milliseconds between polls */

/*
 * Simulate WaitForChar() by slowly polling with kbhit().
 *
 * If Vim should work over the serial line after a 'ctty com1' we must use
 * kbhit() and getch(). (jw)
 *
 * return TRUE if a character is available, FALSE otherwise
 */
	static int
WaitForChar(msec)
	int msec;
{
#if !defined(__GO32__) && defined(__BORLANDC__)
	if (term_console)
	{
		for (;;)
		{
			if (kbhit() || cbrk_pressed)
				return TRUE;
			if (msec <= 0)
				break;
			delay(POLL_SPEED);		/* not DOSGEN function */
			msec -= POLL_SPEED;
		}
	}
#endif
	return FALSE;
}

/*
 * GetChars(): low level input funcion.
 * Get a characters from the keyboard.
 * If time == 0 do not wait for characters.
 * If time == n wait a short time for characters.
 * If time == -1 wait forever for characters.
 *
 * return the number of characters obtained
 */
	int
GetChars(buf, maxlen, time)
	char_u		*buf;
	int 		maxlen;
	int 		time;
{
	int 		len = 0;
	int			c;

	if (term_console)
		v_refresh();
/*
 * if we got a ctrl-C when we were busy, there will be a "^C" somewhere
 * on the sceen, so we need to redisplay it.
 */
	if (delayed_redraw)
	{
		delayed_redraw = FALSE;
		updateScreen(CLEAR);
		setcursor();
		flushbuf();
	}

	if (time >= 0)
	{
		if (WaitForChar(time) == 0) 	/* no character available */
			return 0;
	}
	else	/* time == -1 */
	{
	/*
	 * If there is no character available within 2 seconds (default)
	 * write the autoscript file to disk
	 */
		if (WaitForChar((int)p_ut) == 0)
			updatescript(0);
	}

/*
 * Try to read as many characters as there are.
 * Works for the controlling tty only.
 */
	--maxlen;		/* may get two chars at once */
	/*
	 * we will get at least one key. Get more if they are available
	 * After a ctrl-break we have to read a 0 (!) from the buffer.
	 * bioskey(1) will return 0 if no key is available and when a
	 * ctrl-break was typed. When ctrl-break is hit, this does not always
	 * implies a key hit.
	 */
	cbrk_pressed = FALSE;
	while ((len == 0 || kbhit()) && len < maxlen)
	{
		switch (c = getch())
		{
		case 0:
				*buf++ = K_NUL;
				break;
		case 3:
				cbrk_pressed = TRUE;
				/*FALLTHROUGH*/
		default:
				*buf++ = c;
		}
		len++;
	}
	return len;
}

/*
 * return non-zero if a character is available
 */
	int
mch_char_avail()
{
	return WaitForChar(0);
}

/*
 * We have no job control, fake it by starting a new shell.
 */
	void
mch_suspend()
{
	OUTSTR("new shell started\n");
	(void)call_shell(NULL, 0, TRUE);
}

extern int _fmode;
/*
 * we do not use windows, there is not much to do here
 */
	void
mch_windinit()
{
	_fmode = O_BINARY;		/* we do our own CR-LF translation */
	flushbuf();
#ifndef __GO32__
	{
		int	v_stat;
		v_stat = ioctl(1, 0);
		ioctl(1, 1, (void *)(v_stat | 0x0020));
		if ((ioctl(1, 0) & 0x0090) == 0x0090)
			int29h = TRUE;
		ioctl(1, 1, (void *)(v_stat));
	}
#endif
	(void)mch_get_winsize();
}

	void
check_win(argc, argv)
	int		argc;
	char	**argv;
{
	if (!isatty(0) || !isatty(1))
	{
		fprintf(stderr, "VIM: no controlling terminal\n");
		exit(2);
	}
}

/*
 * fname_case(): Set the case of the filename, if it already exists.
 *				 msdos filesystem is far to primitive for that. do nothing.
 */
	void
fname_case(name)
	char_u *name;
{
}

/*
 * mch_settitle(): set titlebar of our window.
 * Dos console has no title.
 */
	void
mch_settitle(title, icon)
	char_u *title;
	char_u *icon;
{
}

/*
 * Restore the window/icon title. (which we don't have)
 */
	void
mch_restore_title(which)
	int which;
{
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
	int
vim_dirname(buf, len)
	char_u	*buf;
	int		len;
{
	return (getcwd(buf, len) != NULL ? OK : FAIL);
}

/*
 * Change default drive (just like _chdrive of Borland C 3.1)
 */
	static int
change_drive(drive)
	int drive;
{
	unsigned dummy;
	union REGS regs;

	regs.h.ah = 0x0e;
	regs.h.dl = drive - 1;
	intdos(&regs, &regs);	/* set default drive */
	regs.h.ah = 0x19;
	intdos(&regs, &regs);	/* get default drive */
	if (regs.h.al == drive - 1)
		return 0;
	else
		return -1;
}

/*
 * get absolute filename into buffer 'buf' of length 'len' bytes
 *
 * return FAIL for failure, OK otherwise
 */
	int
FullName(fname, buf, len)
	char_u	*fname, *buf;
	int		len;
{
	if (fname == NULL)	/* always fail */
	{
		*buf = NUL;
		return FAIL;
	}

	if (isFullName(fname))		/* allready expanded */
	{
		STRNCPY(buf, fname, len);
		return OK;
	}

#ifdef __BORLANDC__		/* the old Turbo C does not have this */
	if (_fullpath(buf, fname, len) == NULL)
	{
		STRNCPY(buf, fname, len);	/* failed, use the relative path name */
		return FAIL;
	}
	return OK;
#else					/* almost the same as FullName in unix.c */
	{
		int		l;
		char_u	olddir[MAXPATHL];
		char_u	*p, *q;
		int		c;
		int		retval = OK;

		*buf = 0;
		/*
		 * change to the directory for a moment,
		 * and then do the getwd() (and get back to where we were).
		 * This will get the correct path name with "../" things.
		 */
		p = strrchr(fname, '/');
		q = strrchr(fname, '\\');
		if (q && (p == NULL || q > p))
			p = q;
		q = strrchr(fname, ':');
		if (q && (p == NULL || q > p))
			p = q;
		if (p != NULL)
		{
			if (getcwd(olddir, MAXPATHL) == NULL)
			{
				p = NULL;		/* can't get current dir: don't chdir */
				retval = FAIL;
			}
			else
			{
				if (*p == ':' || (p > fname && p[-1] == ':'))
					q = p + 1;
				else
					q = p;
				c = *q;
				*q = NUL;
				if (chdir(fname))
					retval = FAIL;
				else
					fname = p + 1;
				*q = c;
			}
		}
		if (getcwd(buf, len) == NULL)
		{
			retval = FAIL;
			*buf = NUL;
		}
		l = STRLEN(buf);
		if (l && buf[l - 1] != '/' && buf[l - 1] != '\\')
			strcat(buf, "\\");
		if (p)
			chdir(olddir);
		strcat(buf, fname);
		return retval;
	}
#endif
}

/*
 * return TRUE is fname is an absolute path name
 */
	int
isFullName(fname)
	char_u		*fname;
{
	return (STRCHR(fname, ':') != NULL);
}

/*
 * get file permissions for 'name'
 * -1 : error
 * else FA_attributes defined in dos.h
 */
	long
getperm(name)
	char_u *name;
{
	struct stat statb;
	long		r;

	if (stat(name, &statb))
		return -1;
	r = (unsigned)statb.st_mode;
	return r;
}

/*
 * set file permission for 'name' to 'perm'
 *
 * return FAIL for failure, OK otherwise
 */
	int
setperm(name, perm)
	char_u	*name;
	long	perm;
{
	return chmod(name, perm);
}

/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory
 * return -1 for error
 *
 * beware of a trailing backslash that may have been added by addfile()
 */
	int
isdir(name)
	char_u *name;
{
	int f;

	f = getperm(name);
	if (f == -1)
		return -1;					/* file does not exist at all */
    if ((f & S_IFDIR) == 0)
        return FAIL;				/* not a directory */
    return OK;
}

/*
 * Careful: mch_windexit() may be called before mch_windinit()!
 */
	void
mch_windexit(r)
	int r;
{
#ifdef FEPCTRL
	if (FepInit)
		fep_term();
#endif
	settmode(0);
	stoptermcap();
	flushbuf();
	ml_close_all(); 				/* remove all memfiles */
	exit(r);
}

/*
 * function for ctrl-break interrupt
 */
	void
catch_cbrk()
{
	cbrk_pressed = TRUE;
	ctrlc_pressed = TRUE;
	signal(SIGINT, catch_cbrk);
}

/*
 * ctrl-break handler for DOS. Never called when a ctrl-break is typed, because
 * we catch interrupt 1b. If you type ctrl-C while Vim is waiting for a
 * character this function is not called. When a ctrl-C is typed while Vim is
 * busy this function may be called. By that time a ^C has been displayed on
 * the screen, so we have to redisplay the screen. We can't do that here,
 * because we may be called by DOS. The redraw is in GetChars().
 */
	static int
cbrk_handler()
{
	delayed_redraw = TRUE;
	return 1; 				/* resume operation after ctrl-break */
}

/*
 * set the tty in (raw) ? "raw" : "cooked" mode
 *
 * Does not change the tty, as bioskey() and kbhit() work raw all the time.
 */

	void
mch_settmode(raw)
	int  raw;
{
	static int saved_cbrk;

	if (raw)
	{
		saved_cbrk = getcbrk();			/* save old ctrl-break setting */
		setcbrk(0);						/* do not check for ctrl-break */
#ifndef __GO32__
		ctrlbrk(cbrk_handler);			/* vim's ctrl-break handler */
#endif
		signal(SIGINT, catch_cbrk);
		if (term_console)
		{
			scroll_region = FALSE;		/* terminal supports scroll region */
			outstr(T_TP);				/* set colors */
		}
	}
	else
	{
		setcbrk(saved_cbrk);			/* restore ctrl-break setting */
		signal(SIGINT, SIG_DFL);
	}
}

/*
 * set screen mode
 * return FAIL for failure, OK otherwise
 */
	int
mch_screenmode(arg)
	char_u		*arg;
{
	EMSG("Screen mode setting not supported");
	return FAIL;
}

/*
 * try to get the real window size
 * return FAIL for failure, OK otherwise
 */
	static int
get_row()
{
	int		row;

#ifndef __GO32__
#  ifdef KANJI
	if ((row = *(unsigned char far *)(0x00400084L)) != 0
			&& *(unsigned short far *)(0x00400085L) != 0)
		row = row + 1;
	else
#  endif
	if (*(int far *)(0x0040004cl)<=4096)
		row = 25;
	else if ((((*(unsigned char far *)(0x0040004aL)*2*
			(*(unsigned char far *)(0x00400084L)+1))+0xfff)&(~0xfff))==
			*(unsigned int far *)(0x0040004cL))
		row = *(unsigned char far *)(0x00400084L)+1;
#else
	row = 25;
#endif
	return row;
}

	int
mch_get_winsize()
{
	if (term_console)
	{
		union REGS	regs;

		scroll_region = FALSE;		/* terminal supports scroll region */
		regs.h.ah = 0x0f;
		int86(0x10, &regs, &regs);
		Columns = regs.h.ah;
		Rows = v_max_row = get_row();
#ifdef FEPCTRL
		if (FepInit)
		{
			fep_force_on();
			Rows = get_row();
			fep_force_off();
			if (Rows == v_max_row)
				Rows--;
		}
#endif

	}
	else
	{
		char	*p;

		Columns = 0;
		Rows = 0;
		if ((p = (char *)vimgetenv("LINES")) != NULL)
			Rows = atoi(p);
		if ((p = (char *)vimgetenv("COLUMNS")) != NULL)
			Columns = atoi(p);
#ifdef TERMCAP
		if (Columns == 0 || Rows == 0)
		{
			extern void getlinecol __ARGS((void));
			getlinecol();
		}
#endif
	}
	if (Columns < 5 || Columns > MAX_COLUMNS ||
					Rows < 2 || Rows > MAX_COLUMNS)
	{
		/* these values are overwritten by termcap size or default */
		Columns = 80;
		Rows = 25;
		return FAIL;
	}
	check_winsize();

	return OK;
}

/*
 * Set the active window for delline/insline.
 */
	void
set_window()
{
}

	void
mch_set_winsize()
{
	/* should try to set the window size to Rows and Columns */
	/* may involve switching display mode.... */
}

/*
 * call shell, return FAIL for failure, OK otherwise
 */
	int
call_shell(cmd, filter, cooked)
	char_u	*cmd;
	int 	filter; 		/* if != 0: called by dofilter() */
	int		cooked;
{
	int		x;
	char_u	newcmd[200];

	flushbuf();

#ifdef FEPCTRL
	if (FepInit)
		fep_term();
#endif

	if (cooked)
		settmode(0);		/* set to cooked mode */

	if (cmd == NULL)
		x = system(p_sh);
	else
	{ 					/* we use "command" to start the shell, slow but easy */
		sprintf(newcmd, "%s /c %s", p_sh, cmd);
		x = system(newcmd);
	}
	outchar('\n');
	if (cooked)
		settmode(1);		/* set to raw mode */

#ifdef WEBB_COMPLETE
	if (x && !expand_interactively)
#else
	if (x)
#endif
	{
		outnum((long)x);
		outstrn((char_u *)" returned\n");
	}

#ifdef FEPCTRL
	if (FepInit)
		fep_init();
#endif

	resettitle();
	(void)mch_get_winsize();		/* display mode may have been changed */
	return (x ? FAIL : OK);
}

/*
 * check for an "interrupt signal": CTRL-break or CTRL-C
 */
	void
breakcheck()
{
	if (ctrlc_pressed)
	{
		ctrlc_pressed = FALSE;
		got_int = TRUE;
	}
}

#define FL_CHUNK 32

	static void
addfile(fl, f, isdir)
	FileList	*fl;
	char_u		*f;
	int			isdir;
{
	char_u		*p;

	if (!fl->file)
	{
		fl->file = (char_u **)alloc(sizeof(char_u *) * FL_CHUNK);
		if (!fl->file)
			return;
		fl->nfiles = 0;
		fl->maxfiles = FL_CHUNK;
	}
	if (fl->nfiles >= fl->maxfiles)
	{
		char_u	**t;
		int		i;

		t = (char_u **)lalloc((long_u)(sizeof(char_u *) * (fl->maxfiles + FL_CHUNK)), TRUE);
		if (!t)
			return;
		for (i = fl->nfiles - 1; i >= 0; i--)
			t[i] = fl->file[i];
		free(fl->file);
		fl->file = t;
		fl->maxfiles += FL_CHUNK;
	}
	p = alloc((unsigned)(STRLEN(f) + 1 + isdir));
	if (p)
	{
		STRCPY(p, f);
		if (isdir)
#ifndef notdef
			if (STRCHR(p, '/') != NULL)
				strcat(p, "/");
			else
#endif
			strcat(p, "\\");
	}
	fl->file[fl->nfiles++] = p;
}

	static int
pstrcmp(a, b)
	char_u **a, **b;
{
	return (strcmp(*a, *b));
}

	int
has_wildcard(s)
	char_u *s;
{
	if (s)
#ifndef notdef
		if (*s == '~' || *s == '$')
			return 1;
		else
#endif
		for ( ; *s; ++s)
#ifdef XARGS
			if (*s == '[' || *s == '`' || *s == '{' || *s == '$' || *s == '~')
				return TRUE;
			else
#endif
			if (*s == '?' || *s == '*')
				return TRUE;
	return FALSE;
}

	static void
strlowcpy(d, s)
	char_u *d, *s;
{
	while (*s)
		*d++ = tolower(*s++);
	*d = '\0';
}

	static int
expandpath(fl, path, fonly, donly, notf)
	FileList	*fl;
	char_u		*path;
	int			fonly, donly, notf;
{
	char_u	buf[MAXPATH];
	char_u	*p, *s, *e;
	int		lastn, c, r;
	struct	ffblk fb;

	lastn = fl->nfiles;

/*
 * Find the first part in the path name that contains a wildcard.
 * Copy it into buf, including the preceding characters.
 */
	p = buf;
	s = NULL;
	e = NULL;
#ifndef notdef
	memset(buf, NUL, sizeof(buf));
#endif
	while (*path)
	{
		if (*path == '\\' || *path == ':' || *path == '/')
		{
			if (e)
				break;
			else
				s = p;
		}
		if (*path == '*' || *path == '?')
			e = p;
		*p++ = *path++;
	}
	e = p;
	if (s)
		s++;
	else
		s = buf;

	/* if the file name ends in "*" and does not contain a ".", addd ".*" */
	if (e[-1] == '*' && STRCHR(s, '.') == NULL)
	{
		*e++ = '.';
		*e++ = '*';
	}
	/* now we have one wildcard component between s and e */
	*e = '\0';
	r = 0;
	/* If we are expanding wildcards we try both files and directories */
	if ((c = findfirst(buf, &fb, (*path || !notf) ? FA_DIREC : 0)) != 0)
	{
		/* not found */
#ifndef notdef
		if (has_wildcard(buf))
			return 0;
#endif
		STRCPY(e, path);
		if (notf)
			addfile(fl, buf, FALSE);
		return 1; /* unexpanded or empty */
	}
	while (!c)
	{
		strlowcpy(s, fb.ff_name);
			/* ignore "." and ".." */
		if (*s != '.' || (s[1] != '\0' && (s[1] != '.' || s[2] != '\0')))
		{
			strcat(buf, path);
			if (!has_wildcard(path))
				addfile(fl, buf, (isdir(buf) == TRUE));
			else
				r |= expandpath(fl, buf, fonly, donly, notf);
		}
		c = findnext(&fb);
	}
	qsort(fl->file + lastn, fl->nfiles - lastn, sizeof(char_u *), pstrcmp);
	return r;
}

/*
 * MSDOS rebuilt of Scott Ballantynes ExpandWildCard for amiga/arp.
 * jw
 */

	int
ExpandWildCards(num_pat, pat, num_file, file, files_only, list_notfound)
	int 	num_pat;
	char_u	**pat;
	int 	*num_file;
	char_u	***file;
	int 	files_only, list_notfound;
{
	int			i, r = 0;
	FileList	f;
#ifndef notdef
	char_u		buf[MAXPATHL];
# ifdef XARGS
	int			j;
	char	**	result;
# endif
#endif

	f.file = NULL;
	f.nfiles = 0;
	for (i = 0; i < num_pat; i++)
	{
#ifndef notdef
		expand_env(pat[i], buf, MAXPATHL);
# ifdef XARGS
		result = glob_filename(buf);
		for (j = 0; result[j] != NULL; j++)
		{
			if (!has_wildcard(result[j]))
				addfile(&f, result[j], files_only ? FALSE : (isdir(result[j]) == TRUE));
			else
				r |= expandpath(&f, result[j], files_only, 0, list_notfound);
			free(result[j]);
		}
		free(result);
# else
		if (!has_wildcard(buf))
			addfile(&f, buf, files_only ? FALSE : (isdir(pat[i]) == TRUE));
		else
			r |= expandpath(&f, buf, files_only, 0, list_notfound);
# endif
#else
		if (!has_wildcard(pat[i]))
			addfile(&f, pat[i], files_only ? FALSE : (isdir(pat[i]) == TRUE));
		else
			r |= expandpath(&f, pat[i], files_only, 0, list_notfound);
#endif
	}
	if (f.nfiles != 0)
	{
		*num_file = f.nfiles;
		*file = f.file;
	}
	else
	{
		*num_file = 0;
		*file = NULL;
	}
	return (r ? FAIL : OK);
}

	void
FreeWild(num, file)
	int		num;
	char_u	**file;
{
	if (file == NULL || num <= 0)
		return;
	while (num--)
		free(file[num]);
	free(file);
}

/*
 * The normal chdir() does not change the default drive.
 * This one does.
 */
#undef chdir
	int
vim_chdir(path)
	char_u *path;
{
	if (path[0] == NUL)				/* just checking... */
		return 0;
	if (path[1] == ':')				/* has a drive name */
	{
		if (change_drive(toupper(path[0]) - 'A' + 1))
			return -1;				/* invalid drive name */
		path += 2;
	}
	if (*path == NUL)				/* drive name only */
		return 0;
	return chdir(path);				/* let the normal chdir() do the rest */
}


	void
chk_ctlkey(c, k)
	int		*c;
	int		*k;
{
	int		w;

	if (term_console)
		return;
	while (1)
	{
		if (*c == Ctrl('Q') || *c == Ctrl(']'))
		{
			w = vgetc();
			switch (w) {
			case 'h':
				*c = K_LARROW;
				break;
			case 'j':
				*c = K_DARROW;
				break;
			case 'k':
				*c = K_UARROW;
				break;
			case 'l':
				*c = K_RARROW;
				break;
			case 'H':
			case 'b':
				*c = K_SLARROW;
				break;
			case 'J':
			case Ctrl('F'):
				*c = K_SDARROW;
				break;
			case 'K':
			case Ctrl('B'):
				*c = K_SUARROW;
				break;
			case 'L':
			case 'w':
				*c = K_SRARROW;
				break;
			default:
				beep();
				if (ISkanji(w))
					vgetc();
				*c = vgetc();
				if (ISkanji(*c))
					*k = vgetc();
				continue;
			}
		}
		break;
	}
}
