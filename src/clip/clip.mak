# vi:ts=8:sw=8
# Makefile for VIM on WINNT, using MS SDK
#
NODEBUG=1
TARGETOS=WIN95
!include <ntwin32.mak>
ccommon= /O2 /Ox /GA
optlibs= user32.lib shell32.lib

#>>>>> name of the compiler and linker, name of lib directory
CC	= cl -nologo
LINK	= cl
DBG	= #-v
CFLAGS	= $(DEFINES) $(DBG) $(cflags) $(cdebug) $(cvarsdll)

###########################################################################
INCL =
OBJ =	clip.obj

###########################################################################
clip.exe: $(OBJ) clip.res
	$(link) $(linkdebug) $(conlflags) -out:clip.exe $(OBJ) clip.res $(guilibsdll)

clip.obj: clip/clip.c $(INCL)
	$(CC) -I. $(CFLAGS) -c clip\clip.c

clip.res: clip/clip.rc
	rc /fo clip.res clip/clip.rc

clean:
	del *.obj
	del *.exe
