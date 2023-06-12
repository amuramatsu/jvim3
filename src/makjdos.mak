# vi:ts=8:sw=8
# Makefile for VIM on MSDOS, using Turbo C
#

#>>>>> choose options:
### -DDIGRAPHS		digraph support (at the cost of 1.6 Kbyte code)
### -DCOMPATIBLE	start in vi-compatible mode
### -DNOBACKUP		default is no backup file
### -DDEBUG		output a lot of debugging garbage
### -DTERMCAP		include termcap file support
### -DNO_BUILTIN_TCAPS	do not include builtin termcap entries
###				(use only with -DTERMCAP)
### -DSOME_BUILTIN_TCAPS include most useful builtin termcap entries
###				(use only without -DNO_BUILTIN_TCAPS)
### -DALL_BUILTIN_TCAPS	include all builtin termcap entries
###				(use only without -DNO_BUILTIN_TCAPS)
### -DVIMRC_FILE	name of the .vimrc file in current dir
### -DEXRC_FILE		name of the .exrc file in current dir
### -DSYSVIMRC_FILE	name of the global .vimrc file
### -DSYSEXRC_FILE	name of the global .exrc file
### -DDEFVIMRC_FILE	name of the system-wide .vimrc file
### -DVIM_HLP		name of the help file
### -DWEBB_COMPLETE	include Webb's code for command line completion
### -DWEBB_KEYWORD_COMPL include Webb's code for keyword completion
### -DNOTITLE		'title' option off by default
DEFINES = -DKANJI -DTRACK -DFEPCTRL -DCRMARK -DFEXRC -DWEBB_COMPLETE -DWEBB_KEYWORD_COMPL -DTERMCAP -DXARGS -DUSE_GREP -DUSE_TAGEX -DUSE_OPT #-DCOMPATIBLE
TERMLIB = termlib.obj

#>>>>> name of the compiler and linker, name of lib directory
CC	= bcc +jvim.cfg
LINK	= tlink

#>>>>> end of choices
###########################################################################

DBG	= #-v -Ddebug
INCL	= vim.h globals.h param.h keymap.h macros.h ascii.h msdos.h structs.h
CFLAGS	= $(DBG) -ml -O1 -DMSDOS $(DEFINES)

OBJ =	alloc.obj dos_v.obj buffer.obj charset.obj cmdcmds.obj cmdline.obj \
	csearch.obj digraph.obj edit.obj fileio.obj getchar.obj help.obj \
	linefunc.obj main.obj mark.obj memfile.obj memline.obj message.obj \
	misccmds.obj normal.obj ops.obj param.obj quickfix.obj regexp.obj \
	regsub.obj screen.obj search.obj \
	tag.obj term.obj undo.obj window.obj $(TERMLIB) \
	kanji.obj fepctrl.obj track.obj xargs.obj

.c.obj:
	$(CC) -c $&.c

.c.exe:
	$(CC) @&&!
$(CFLAGS) $&.c
!

###########################################################################

vim.exe: jvim.cfg $(OBJ) version.obj
	$(CC) @&&!
-ml $(DBG) -evim.exe $(OBJ) version.obj
!

ctags:
	command /c ctags *.c *.h

clean:
	del *.obj
	del *.exe
	del *.res
	del *.tr
	del *.td
	del *.tr2
	del *.td2
	del *.o
	del vim
	del jptab
	del mkcmdtab
	del *.@@@
	del *.com
	del *.lzh

update:
	make -f makjdos.mak clean
	lha u -rx d:\usr\backup.src\vim3jp.lzh *.*

lzh:
	lha u -rx jv3pc12.lzh vim.exe doc.j\*.* doc.e\*.*
	lha d vim3j12.lzh doc.j\readme.doc
	lha u vim3j12.lzh doc.j\readme.doc

addcr:	addcr.c
	$(CC) addcr.c
	command /c addcr.bat

###########################################################################
GREPOBJ = grep.obj alloc.obj charset.obj kanji.obj xargs.obj regexp.obj regsub.obj

grep.exe: $(GREPOBJ)
	$(CC) @&&!
-ml $(DBG) -egrep.exe $(GREPOBJ)
!

grep.obj:	grep/grep.c $(INCL)
	$(CC) @&&!
$(CFLAGS) -c grep/grep.c
!

###########################################################################

alloc.obj:	alloc.c  $(INCL)
dos_v.obj:	dos_v.c  $(INCL) msdos.h
msdos.obj:	msdos.c  $(INCL) msdos.h
buffer.obj:	buffer.c  $(INCL)
charset.obj:	charset.c  $(INCL)
cmdcmds.obj:	cmdcmds.c  $(INCL)
cmdline.obj:	cmdline.c  $(INCL) cmdtab.h
csearch.obj:	csearch.c  $(INCL)
digraph.obj:	digraph.c  $(INCL)
edit.obj:	edit.c  $(INCL)
fileio.obj:	fileio.c  $(INCL) kanji.h
getchar.obj:	getchar.c  $(INCL)
help.obj:	help.c  $(INCL)
linefunc.obj:	linefunc.c  $(INCL)
main.obj:	main.c  $(INCL)
mark.obj:	mark.c  $(INCL)
memfile.obj:	memfile.c  $(INCL)
memline.obj:	memline.c  $(INCL)
message.obj:	message.c  $(INCL)
misccmds.obj:	misccmds.c  $(INCL)
normal.obj:	normal.c  $(INCL) ops.h
ops.obj:	ops.c  $(INCL) ops.h
param.obj:	param.c  $(INCL)
quickfix.obj:	quickfix.c  $(INCL)
regexp.obj:	regexp.c  $(INCL)
regsub.obj:	regsub.c  $(INCL)
screen.obj:	screen.c  $(INCL) kanji.h
search.obj:	search.c  $(INCL)
tag.obj:	tag.c  $(INCL)
term.obj:	term.c  $(INCL) term.h
undo.obj:	undo.c  $(INCL)
window.obj:	window.c  $(INCL)
kanji.obj:	kanji.c  $(INCL) jptab.h kanji.h
fepctrl.obj:	fepctrl.c  $(INCL)
track.obj:	track.c  $(INCL) jptab.h kanji.h
mkcmdtab.exe:	mkcmdtab.c
jptab.exe:	jptab.c

cmdtab.h: cmdtab.tab mkcmdtab.c
	$(CC) mkcmdtab.c
	mkcmdtab cmdtab.tab cmdtab.h

jptab.h: jptab.c
	$(CC) jptab.c
	jptab.exe > jptab.h

jvim.cfg: makjdos.mak
  copy &&|
$(CFLAGS) -I. -IE:\BC45\INCLUDE -LE:\BC45\LIB
| jvim.cfg
