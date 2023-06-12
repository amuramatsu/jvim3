# vi:ts=8:sw=8
# Makefile for VIM on WINNT, using MS SDK
#
NODEBUG=1
TARGETOS=WIN95
!include <ntwin32.mak>
ccommon= /O2 #/G5 /GA

#>>>>> choose options:
DEFINES	= -DKANJI -DWIN32 -DUCODE

#>>>>> name of the compiler and linker, name of lib directory
CC	= cl -nologo
LINK	= cl
DBG	= #-v
CFLAGS	= -DMSDOS -DNT $(DEFINES) $(DBG) $(cflags) $(cdebug) $(cvarsdll)

#>>>>> end of choices

###########################################################################
INCL =	vim.h globals.h param.h keymap.h macros.h ascii.h msdos.h structs.h
OBJ = vim32s.obj alloc.obj charset.obj kanji.obj regexp.obj regsub.obj

###########################################################################
vim32s.exe: $(OBJ) vim32s.res
	$(link) $(linkdebug) $(guilflags) -out:vim32s.exe $(OBJ) vim32s.res $(guilibsdll)
	del vim32s.obj

vim32s.obj: vim32s/vim32s.c $(INCL)
	$(CC) -I. $(CFLAGS) -c vim32s\vim32s.c

vim32s.res: vim32s/vim32s.rc
	rc /fo vim32s.res vim32s/vim32s.rc

###########################################################################
alloc.obj:	alloc.c  $(INCL)
charset.obj:	charset.c  $(INCL)
regexp.obj:	regexp.c  $(INCL)
regsub.obj:	regsub.c  $(INCL)
kanji.obj:	kanji.c  $(INCL) jptab.h
