# vi:ts=8:sw=8
# Makefile for Vim on Unix
#

# Note: You MUST uncomment three hardware dependend lines!

# There are three types of defines:
#
# 1. configuration dependend
#	Used for "make install". Adjust the path names and protections
#	to your desire. Also defines the root for the X11 files (not required).
#
# 2. various choices
#	Can be changed to match your compiler or your preferences (not
#	required).
#
# 3. hardware dependend
#	If you machine is in the list, remove one '#' in front of the defines
#	following it. Otherwise: Find a machine that is similar and change the
#	defines to make it work. Normally you can just try and see what error
#	messages you get. (REQUIRED).

# The following systems have entries below. They have been tested and should
# work without modification. But later code changes may cause small problems.
# There are entries for other systems, but these have not been tested recently.

#system:		tested configurations:		tested by:

#Sun 4.1.x		cc	gcc	X11	no X11	(jw) (mool)
#FreeBSD		cc	gcc	X11	no X11	(mool)
#linux 1.0		cc		X11
#Linux 1.0.9			gcc		no X11	(jw)
#ULTRIX 4.2A on MIPS	cc	gcc		no X11	(mool)
#HPUX			cc	gcc	X11	no X11	(jw) (mool)
#irix 4.0.5H		cc		X11
#IRIX 4.0  SGI		cc		X11		(jw)
#SINIX-L 5.41		cc			no X11
#MOT188			cc			no X11
#Sequent/ptx 1.3	cc			no X11	(jw)
#osf1			cc			no X11	(jw)
#Unisys 6035		cc			no X11
#SCO 3.2		cc	gcc		no X11	jos@oce.nl
#Solaris		cc		X11
#Solaris/Sun OS 5.3	cc			no X11	(jw)
#AIX (rs6000)		cc			no X11	(jw)
#RISCos on MIPS		cc		X11	no X11	(jw)

# configurations marked by (jw) have been tested by Juergen Weigert:
#    jnweiger@uni-erlangen.de

#
# PART 1: configuration dependend
#

### root directory for X11 files (unless overruled in hardware-dependend part)
### Unfortunately there is no standard for these, everybody puts them
### somewhere else
#X11LIBDIR = /usr/openwin/lib
#X11INCDIR = /usr/openwin/include
### for some hpux systems:
#X11LIBDIR = /usr/lib/X11R5
#X11INCDIR = /usr/include/X11R5
###
X11LIBDIR = /usr/X11R6/lib
X11INCDIR = /usr/X11R6/include
###
#X11LIBDIR = /usr/X386/lib
#X11INCDIR = /usr/X386/include
###
#X11LIBDIR = /usr/X11/lib
#X11INCDIR = /usr/X11/include


### Prefix for location of files
PREFIX?= /usr/local

### Location of binary
BINLOC = $(PREFIX)/bin

### Name of target
TARGET = jvim3

### Location of man page
MANLOC = $(PREFIX)/man/man1

### Location of help file
HELPLOC = $(PREFIX)/lib

### Program to run on installed binary
STRIP = strip

### Permissions for vim binary
BINMOD = 755

### Permissions for man page
MANMOD = 644

### Permissions for help file
HELPMOD = 644

MANFILE = ../doc/vim.1

HELPFILE = ../doc.j/vim.hlp

#
# PART 2: various choices
#

### -DDIGRAPHS		digraph support
### -DNO_FREE_NULL	do not call free() with a null pointer
### -DCOMPATIBLE	start in vi-compatible mode
### -DNOBACKUP		default is no backup file
### -DDEBUG		output a lot of debugging garbage
### -DSTRNCASECMP	use strncasecmp() instead of internal function
### -DUSE_LOCALE	use setlocale() to change ctype() and others
### -DTERMCAP		full termcap/terminfo file support
### -DTERMINFO		use terminfo instead of termcap entries for builtin terms
### -DNO_BUILTIN_TCAPS	do not include builtin termcap entries
###				(use only with -DTERMCAP)
### -DSOME_BUILTIN_TCAPS include most useful builtin termcap entries
###				(use only without -DNO_BUILTIN_TCAPS)
### -DALL_BUILTIN_TCAPS	include all builtin termcap entries
###				(use only without -DNO_BUILTIN_TCAPS)
### -DMAXNAMLEN 31	maximum length of a file name (if not defined in sys/dir.h)
### -Dconst=		for compilers that don't have type const
### -DVIMRC_FILE=name		name of the .vimrc file in current dir
### -DEXRC_FILE=name		name of the .exrc file in current dir
### -DSYSVIMRC_FILE=name	name of the global .vimrc file
### -DSYSEXRC_FILE=name		name of the global .exrc file
### -DDEFVIMRC_FILE=name	name of the system-wide .vimrc file
### -DVIM_HLP=name		name of the help file
### -DUSE_SYSTEM	use system() instead of fork/exec for starting a shell
### -DVIM_ISSPACE	use when isspace() can't handle meta chars
### -DNOLIMITS		limits.h does not exist
### -DNOSTDLIB		stdlib.h does not exist
### -DUSE_X11		include code for xterm title saving
### -DWEBB_COMPLETE	include Webb's code for command line completion
### -DWEBB_KEYWORD_COMPL include Webb's code for keyword completion
### -DNOTITLE		'title' option off by default

#
# PART 2-2: Select Japanese Input System dependend
#

# generic for FEP Sequence
#
#FEPOPT  = -DFEPCTRL
#FEPLIBS =
#FEPOBJS = fepseq.o

# generic for BOW IME control Sequence
#
#FEPOPT  = -DFEPCTRL -DJP_DEF=\"SSS\"
#FEPLIBS =
#FEPOBJS = fepbow.o

# generic for CANNA input system
#
#FEPOPT  = -DFEPCTRL -DCANNA
#FEPLIBS = -lcanna
#FEPOBJS = fepcanna.o

# generic for ONEW input system
#
#FEPOPT  = -DFEPCTRL -DONEW
#FEPLIBS = -lonew -lcanna
#FEPOBJS = feponew.o

DEFS = -DDIGRAPHS -DTERMCAP -DSOME_BUILTIN_TCAPS -DNO_FREE_NULL -DVIM_ISSPACE \
		-DWEBB_COMPLETE -DWEBB_KEYWORD_COMPL \
		-DVIM_HLP=\"$(HELPLOC)/jvim3.hlp\" \
		-DDEFVIMRC_FILE=\"$(PREFIX)/etc/jvim3rc\" \
		-DKANJI -DUCODE -DTRACK -DCRMARK -DFEXRC -DUSE_GREP -DUSE_TAGEX -DUSE_OPT $(FEPOPT)

#
# PART 3: hardware dependend
#

### CC entry:      name and arguments for the compiler (also for linking)
### MACHINE entry: defines used for compiling (not for linking)
### LIBS:          defines used for linking

# generic for Sun, NeXT, POSIX and SYSV R4 (?) (TESTED for Sun 4.1.x)
# standard cc with optimizer
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE -DUSE_X11
#CC=cc -O -I$(X11INCDIR)
#LIBS = -ltermlib -L$(X11LIBDIR) -lX11

# generic for Sun, FreeBSD, NetBSD, NeXT, POSIX and SYSV R4 (?) without x11 code
#	(TESTED for Sun 4.1.x and FreeBSD)
# standard cc with optimizer
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE
#CC=cc -O
#LIBS = -ltermlib

# FreeBSD and NetBSD with Xfree (TESTED for FreeBSD)
# standard cc with optimizer
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE -DUSE_X11
#CC=cc -O -I$(X11INCDIR)
#LIBS = -ltermlib -L$(X11LIBDIR) -lX11

# FreeBSD and NetBSD with Xfree (TESTED for FreeBSD)
# gcc with optimizer
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE -DUSE_X11
#CC=gcc -O -Wall -traditional -Dconst= -I$(X11INCDIR)
#LIBS = -ltermlib -L$(X11LIBDIR) -lX11

# like generic, but with termcap, for Linux, NeXT and others (NOT TESTED YET)
# standard cc with optimizer
#
#MACHINE = -DBSD_UNIX
#CC=cc -O
#LIBS = -ltermcap

# Linux 2.0.x/2.2.x (TESTED Debian GNU/Linux 2.0/2.1)
#MACHINE = -DBSD_UNIX
#CC=gcc -O
#LIBS = -lncurses

# Cygwin (TESTED 1.1-NetRelease)
#   Notes: Please check ../doc.j/cygwin.txt
#
#TARGET=vim.exe
#MACHINE = -DBSD_UNIX
#CC=gcc -O
#LIBS = -ltermcap

# linux 1.0 with X11 (TESTED)
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE -DUSE_X11
#CC=cc -O -I$(X11INCDIR)
#LIBS = -ltermcap -L$(X11LIBDIR) -lX11

# like generic, but with debugging (NOT TESTED YET)
#
#MACHINE = -DBSD_UNIX -g
#CC=cc
#LIBS = -ltermlib

# like generic, but with gcc and X11 (TESTED on Sun 4.1.x)
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE -DUSE_X11
#CC=gcc -O -Wall -traditional -Dconst= -L$(X11LIBDIR) -I$(X11INCDIR)
#LIBS = -ltermlib -lX11

# like generic, but with gcc, without X11 (TESTED on ULTRIX 4.2A on MIPS)
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE
#CC=gcc -O -Wall -traditional -Dconst=
#LIBS = -ltermlib

# like generic, but with gcc 2.5.8 (TESTED on Sun 4.1.3_U1)
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE
#CC=gcc -O1000
#LIBS = -ltermlib

# standard cc with optimizer for ULTRIX 4.2A on MIPS (ultrix defined) (TESTED)
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE
#CC=cc -O -Olimit 1500
#LIBS = -ltermlib

# GCC (2.2.2d) on Linux (1.0.9) (TESTED)
#
#MACHINE = -DBSD_UNIX
#CC=gcc -O6 -Wall
#LIBS = -ltermcap

# Apollo DOMAIN (with SYSTYPE = bsd4.3) (NOT TESTED YET)
#
#MACHINE = -DBSD_UNIX -DDOMAIN
#CC=cc -O -A systype,bsd4.3
#LIBS = -ltermlib

# HPUX with X11 (TESTED) (hpux is defined)
#
#MACHINE = -DBSD_UNIX -DTERMINFO -DUSE_X11
#CC=cc -O -I$(X11INCDIR)
#LIBS = -ltermcap -L$(X11LIBDIR) -lX11

# HPUX (TESTED) (hpux is defined)
#
#MACHINE = -DBSD_UNIX -DTERMINFO
#CC=cc -O
#LIBS = -ltermcap

# HPUX with gcc (TESTED) (hpux is defined)
#
#MACHINE = -DBSD_UNIX -DTERMINFO
#CC=gcc -O
#LIBS = -ltermcap

# hpux 9.01 (with termlib instead of termcap) (TESTED)
# irix 4.0.5H (TESTED)
#
#MACHINE = -DBSD_UNIX -DUSE_LOCALE -DUSE_X11
#CC=cc -O -I$(X11INCDIR)
#LIBS = -ltermlib -L$(X11LIBDIR) -lX11

# IRIX 4.0 (Silicon Graphics Indigo, __sgi will be defined) (TESTED)
#
#MACHINE = -DBSD_UNIX -DUSE_X11                         
#CC=cc -O -Olimit 1500
#LIBS = -ltermlib -lX11 -lmalloc -lc_s

# Convex (NOT TESTED YET)
#
#MACHINE = -DBSD_UNIX -DCONVEX
#CC=cc -O
#LIBS = -ltermcap

# generic SYSV_UNIX for Dynix/PTX and SYSV R3 (and R4?) (TESTED on SINIX-L 5.41)
# (TESTED on MOT188) (TESTED on Sequent/ptx 1.3) (TESTED on osf1)
# First try the line with locale. If this gives error messages try the other one.
#
#MACHINE = -DSYSV_UNIX -DUSE_LOCALE
#MACHINE = -DSYSV_UNIX
#CC=cc -O
#LIBS = -ltermlib

# generic SYSV_UNIX with LOCALE (TESTED on Unisys 6035)
#
#MACHINE = -DSYSV_UNIX -DUSE_LOCALE -DUNISYS
#CC=cc -O
#LIBS = -ltermlib

# SCO Xenix (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX -DSCO
#CC=cc -O
#LIBS = -ltermlib

# GCC on SCO 3.2 (TESTED by jos@oce.nl)
# cc works too.
#
#MACHINE = -DSYSV_UNIX -UM_XENIX -DSCO
#CC=gcc -O -Wall
#LIBS = -ltinfo

# GCC on another SCO Unix (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX -UM_XENIX -DSCO -g
#CC=gcc -O6 -fpcc-struct-return -fwritable-strings
#LIBS = -ltermlib -lmalloc

# Dynix with gcc (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX
#CC=gcc -O -Wall -traditional
#LIBS = -ltermlib

# SOLARIS with X11 anc cc (TESTED)
#
#MACHINE = -DSYSV_UNIX -DSOLARIS -DTERMINFO -DUSE_X11
#CC=cc -O -Xa -v -R$(X11LIBDIR) -L$(X11LIBDIR) -I$(X11INCDIR)
#LIBS = -ltermlib -lX11

# SOLARIS with X11 and gcc (TESTED with SOLARIS 2.3 and gcc 2.5.8)
#
#MACHINE = -DSYSV_UNIX -DSOLARIS -DTERMINFO -DUSE_X11
#CC=gcc -O -R$(X11LIBDIR) -L$(X11LIBDIR) -I$(X11INCDIR)
#LIBS = -ltermlib -lX11

# SOLARIS (also works for Esix 4.0.3, SYSV R4?) (TESTED on Sun OS 5.3)
#
#MACHINE = -DSYSV_UNIX -DSOLARIS -DTERMINFO
#CC=cc -O -Xa -v
#LIBS = -ltermlib

# UNICOS (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX -DUNICOS
#CC=cc -O
#LIBS = -ltermlib

# AIX (rs6000) (TESTED)
#
#MACHINE = -DSYSV_UNIX -DAIX
#CC=cc -O
#LIBS=-lcur

# UTS2 for Amdahl UTS 2.1.x (disable termcap below) (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX -DUTS2
#CC=cc -O
#LIBS = -ltermlib -lsocket

# UTS4 for Amdahl UTS 4.x (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX -DUTS4 -Xa
#CC=cc -O
#LIBS = -ltermlib

# USL for Unix Systems Laboratories (SYSV 4.2) (NOT TESTED YET)
#
#MACHINE = -DSYSV_UNIX -DUSL
#CC=cc -O
#LIBS = -ltermlib

# RISCos on MIPS without X11 (TESTED)
#
#MACHINE = -DSYSV_UNIX -DMIPS
#CC=cc -O
#LIBS = -ltermlib

# RISCos on MIPS with X11 (TESTED)
#
#MACHINE=-DSYSV_UNIX -DUSE_LOCALE -DUSE_X11
#CC=cc -O -I$(X11INCDIR)
#LIBS=-ltermlib -L$(X11LIBDIR) -lX11 -lsun

# NEC EWS4800 Series (EWS-UX/V R6.x)
#
#MACHINE = -DSYSV_UNIX -DSOLARIS -DTERMINFO
#CC=cc -O -KOlimit=3000
#LIBS = -ltermlib -lsocket -lnsl


################################################
##   no changes required below this line      ##
################################################

CFLAGS = -c $(MACHINE) $(DEFS)

INCL = vim.h globals.h param.h keymap.h macros.h ascii.h term.h unix.h structs.h proto.h

OBJ =	alloc.o unix.o buffer.o charset.o cmdcmds.o cmdline.o \
	csearch.o digraph.o edit.o fileio.o getchar.o help.o \
	linefunc.o main.o mark.o memfile.o memline.o message.o misccmds.o \
	normal.o ops.o param.o quickfix.o regexp.o \
	regsub.o screen.o search.o \
	tag.o term.o undo.o window.o $(TERMLIB) kanji.o track.o \
	u2s.o s2u.o $(FEPOBJS)

GOBJ = grep.o alloc.o charset.o kanji.o regexp.o regsub.o u2s.o s2u.o

all: $(TARGET) grep

$(TARGET): $(OBJ) version.c
	$(CC) $(CFLAGS) version.c
	$(CC) -o $(TARGET) $(OBJ) version.o $(LIBS) $(FEPLIBS)

grep: $(GOBJ)
	$(CC) -o grep/grep $(GOBJ) $(LIBS) $(FEPLIBS)

debug: $(OBJ) version.c
	$(CC) $(CFLAGS) version.c
	$(CC) -o $(TARGET) -g $(OBJ) version.o $(LIBS)

ctags:
	ctags *.c *.h

install: $(TARGET)
	-mkdir $(BINLOC)
	cp $(TARGET) $(BINLOC)
	chmod $(BINMOD) $(BINLOC)/$(TARGET)
	$(STRIP) $(BINLOC)/$(TARGET)
	-mkdir $(MANLOC)
	cp $(MANFILE) $(MANLOC)/jvim3.1
	chmod $(MANMOD) $(MANLOC)/jvim3.1
	-mkdir $(HELPLOC)
	cp $(HELPFILE) $(HELPLOC)/jvim3.hlp
	chmod $(HELPMOD) $(HELPLOC)/jvim3.hlp

clean:
	-rm -f $(OBJ) mkcmdtab.o version.o core $(TARGET) mkcmdtab cmdtab.h
	-rm -f *.bak
	-rm -f $(GOBJ)
#	-rm -f jptab.h jptab

#use this in case the files have been transported via an MSDOS system

FILES = *.c *.h makefile makefile.* cmdtab.tab proto/*.pro tags

dos2unix:
	-mv arp_prot.h arp_proto.h
	-mv ptx_stdl.h ptx_stdlib.h
	-mv sun_stdl.h sun_stdlib.h
	-mv makefile.dic makefile.dice
	-mv makefile.uni makefile.unix
	-mv makefile.man makefile.manx
	-mv makefile.6sa makefile.6sas
	-mv makefile.5sa makefile.5sas
	for i in $(FILES); do tr -d '\r\032' < $$i > ~tmp~; mv ~tmp~ $$i; echo $$i; done

###########################################################################

alloc.o:	alloc.c  $(INCL)
	$(CC) $(CFLAGS) alloc.c

unix.o:	unix.c  $(INCL)
	$(CC) $(CFLAGS) unix.c

buffer.o:	buffer.c  $(INCL)
	$(CC) $(CFLAGS) buffer.c

charset.o:	charset.c  $(INCL)
	$(CC) $(CFLAGS) charset.c

cmdcmds.o:	cmdcmds.c  $(INCL)
	$(CC) $(CFLAGS) cmdcmds.c

cmdline.o:	cmdline.c  $(INCL) cmdtab.h ops.h
	$(CC) $(CFLAGS) cmdline.c

csearch.o:	csearch.c  $(INCL)
	$(CC) $(CFLAGS) csearch.c

digraph.o:	digraph.c  $(INCL)
	$(CC) $(CFLAGS) digraph.c

edit.o:	edit.c  $(INCL) ops.h
	$(CC) $(CFLAGS) edit.c

fileio.o:	fileio.c  $(INCL)
	$(CC) $(CFLAGS) fileio.c

getchar.o:	getchar.c  $(INCL)
	$(CC) $(CFLAGS) getchar.c

help.o:	help.c  $(INCL)
	$(CC) $(CFLAGS) help.c

linefunc.o:	linefunc.c  $(INCL)
	$(CC) $(CFLAGS) linefunc.c

main.o:	main.c  $(INCL)
	$(CC) $(CFLAGS) main.c

mark.o:	mark.c  $(INCL)
	$(CC) $(CFLAGS) mark.c

memfile.o:	memfile.c  $(INCL)
	$(CC) $(CFLAGS) memfile.c

memline.o:	memline.c  $(INCL)
	$(CC) $(CFLAGS) memline.c

message.o:	message.c  $(INCL)
	$(CC) $(CFLAGS) message.c

misccmds.o:	misccmds.c  $(INCL)
	$(CC) $(CFLAGS) misccmds.c

normal.o:	normal.c  $(INCL) ops.h
	$(CC) $(CFLAGS) normal.c

ops.o:	ops.c  $(INCL) ops.h
	$(CC) $(CFLAGS) ops.c

param.o:	param.c  $(INCL)
	$(CC) $(CFLAGS) param.c

quickfix.o:	quickfix.c  $(INCL)
	$(CC) $(CFLAGS) quickfix.c

regexp.o:	regexp.c  $(INCL)
	$(CC) $(CFLAGS) regexp.c

regsub.o:	regsub.c  $(INCL)
	$(CC) $(CFLAGS) regsub.c

screen.o:	screen.c  $(INCL)
	$(CC) $(CFLAGS) screen.c

search.o:	search.c  $(INCL) ops.h
	$(CC) $(CFLAGS) search.c

tag.o:	tag.c  $(INCL)
	$(CC) $(CFLAGS) tag.c

term.o:	term.c  $(INCL)
	$(CC) $(CFLAGS) term.c

undo.o:	undo.c  $(INCL)
	$(CC) $(CFLAGS) undo.c

window.o:	window.c  $(INCL)
	$(CC) $(CFLAGS) window.c

cmdtab.h: cmdtab.tab mkcmdtab
	./mkcmdtab cmdtab.tab cmdtab.h

mkcmdtab: mkcmdtab.o
	$(CC) -o mkcmdtab mkcmdtab.o

mkcmdtab.o: mkcmdtab.c
	$(CC) $(CFLAGS) mkcmdtab.c

kanji.o: kanji.c $(INCL) jptab.h
	$(CC) $(CFLAGS) kanji.c

s2u.o: s2u.c
	$(CC) $(CFLAGS) s2u.c

u2s.o: u2s.c
	$(CC) $(CFLAGS) u2s.c

track.o: track.c $(INCL) jptab.h
	$(CC) $(CFLAGS) track.c

#jptab.h: jptab
#	./jptab > jptab.h

jptab: jptab.o
	$(CC) -o jptab jptab.o

jptab.o: jptab.c
	$(CC) $(CFLAGS) jptab.c

fepseq.o: fepseq.c
	$(CC) $(CFLAGS) fepseq.c

fepbow.o: fepbow.c
	$(CC) $(CFLAGS) fepbow.c

fepcanna.o: fepcanna.c
	$(CC) $(CFLAGS) -I${PREFIX}/include fepcanna.c

feponew.o: feponew.c
	$(CC) $(CFLAGS) feponew.c

grep.o: grep/grep.c
	$(CC) $(CFLAGS) -I. grep/grep.c
