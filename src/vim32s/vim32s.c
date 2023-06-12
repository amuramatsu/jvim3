/*=============================================================== vim:ts=4:sw=4:
 *
 * GREP	- Japanized GREP
 *
 *============================================================================*/
#include <windows.h>
#define EXTERN
#include "vim.h"
#include "globals.h"
#include "proto.h"
#include "kanji.h"
#include "regexp.h"
#include "fcntl.h"

/*==============================================================================
 *	I/O parameter
 *============================================================================*/
static unsigned char	*	IObuffer;
static long					IOsize;
static unsigned char	*	IOstart;
static long					IOrest;

/*==============================================================================
 *	line parameter
 *============================================================================*/
typedef struct {
	long					num;			/* line number					  */
	long					size;			/* line size					  */
	long					rest;			/* line rest size				  */
	unsigned char		*	lp;				/* line pointer					  */
} LINE;

/*==============================================================================
 *	command parameter
 *============================================================================*/
typedef struct {
	char					cmd[MAX_PATH + 32];
	int						dir;
	int						val;
	int						wait;
} SEL;
static SEL					SelBox[32];
static int					SelCnt = 0;
static int					SelVal = 0;
static int					o_ubig;

/*==============================================================================
 *	global parameter
 *============================================================================*/
int							reg_ic;
int							reg_jic;
int							p_magic = TRUE;
int							p_jkc	= FALSE;
int							p_opt;
char_u						p_jp	= 'S';
int							p_cpage = CP_ACP;

int
emsg(msg)
char_u		*	msg;
{
	return(TRUE);
}

char_u	*
ml_get_buf(buf, lnum, will_change)
BUF			*buf;
linenr_t	lnum;
int			will_change;		/* line will be changed */
{
	return(NULL);
}

int
mf_release_all()
{
	return(0);
}

long
mch_avail_mem(spec)
int spec;
{
	return 0x7fffffff;		/* virual memory eh */
}

/*==============================================================================
 *	prototype define
 *============================================================================*/
static void
mbox(char_u *s, ...)
{
	char		buf[1024];
	va_list		ap;

	va_start(ap, s);
	wvsprintf(buf, s, ap);
	va_end(ap);
#if 0
	if (getenv("VIM32DEBUG") != NULL)
		MessageBox(NULL, buf, "Debug", MB_OK);
#endif
}

/*==============================================================================
 *	prototype define
 *============================================================================*/
static int			fileopen __ARGS((const char *));
static int			fileread __ARGS((int, LINE *));

/*==============================================================================
 *	line parameter
 *============================================================================*/
static int
fileopen(fname)
const char		*	fname;
{
	int					fd;

	if (IObuffer == NULL)
	{
		IOsize = 0x10000L;					/* read 64K at a time */
		while ((IObuffer = malloc(IOsize)) == NULL)
		{
			IOsize >>= 1;
			if (IOsize < 1024)
				ExitProcess(2);
		}
	}
	IOstart= IObuffer;
	IOrest = 0;
	fd = open(fname, O_RDONLY);
	if (fd < 0)
		return(-1);
	if ((IOrest = read(fd, (char *)IObuffer, (size_t)IOsize)) <= 0)
		return(fd);
	p_jp = judge_jcode(&p_jp, &o_ubig, IObuffer, IOrest);
	if (toupper(p_jp) == JP_WIDE)	/* UNICODE */
		IOrest = wide2multi(IObuffer, IOrest, o_ubig, TRUE);
	return(fd);
}

static int
fileread(fd, lp)
int					fd;
LINE			*	lp;
{
	long				size;
	unsigned char	*	ptr;
	unsigned char		c;
	int					n;
	int					found = FALSE;

	if (IOrest == 0)	/* end of file */
		return(-1);
	lp->rest += lp->size;
	lp->size  = 0;
	ptr  = IOstart;
	size = IOrest;
	ptr--;
	while (++ptr, --size >= 0)
	{
		if ((c = *ptr) != '\0' && c != '\n')	/* catch most common case */
			continue;
		if (c == '\0')
			*ptr = '\n';		/* NULs are replaced by newlines! */
		else if (ptr[-1] == '\r')	/* remove CR */
			ptr[-1] = '\0';
		found = TRUE;
retry:
		n = (ptr - IOstart) * 2 + 1;
		if (n > lp->rest)
		{
			char	*	p = lp->lp;

			if ((lp->lp = malloc(n)) == NULL)
				ExitProcess(2);
			if (p != NULL)
			{
				memmove(lp->lp, p, lp->size);
				free(p);
			}
			lp->rest = n;
			lp->lp[lp->size] = '\0';
		}
		n = kanjiconvsfrom(IOstart, ptr - IOstart,
					&lp->lp[lp->size], n, NULL, (char_u)toupper(p_jp), NULL);
		lp->size += n;
		lp->lp[lp->size] = '\0';	/* end of line */
		lp->rest = lp->rest - n;
		ptr++;
		IOrest -= ptr - IOstart;
		IOstart = ptr;	/* nothing left to write */
		if (IOrest == 0)
		{
			if ((IOrest = read(fd, (char *)IObuffer, (size_t)IOsize)) < 0)
				return(-1);
			if (toupper(p_jp) == JP_WIDE)	/* UNICODE */
				IOrest = wide2multi(IObuffer, IOrest, o_ubig, FALSE);
			if (IOrest == 0)		/* end of file */
				return(0);
			ptr = IOstart = IObuffer;	/* nothing left to write */
			size = IOrest;
			ptr--;
		}
		if (found == TRUE)
			return(0);
	}
	ptr--;
	goto retry;
}

/*==============================================================================
 *	Main function
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	select dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int					wmId;
	int					i;
	LRESULT				lResult;
	OPENFILENAME		ofn;
	char				NameBuff[MAX_PATH+2];

	switch (uMsg) {
	case WM_INITDIALOG:
		for (i = 0; i < SelCnt; i++)
		{
			lResult = SendDlgItemMessage(hWnd, 1000,
							CB_ADDSTRING, 0, (LPARAM)(LPSTR)SelBox[i].cmd);
			if (lResult == CB_ERR || lResult == CB_ERRSPACE)
				ExitProcess(8);
			lResult = SendDlgItemMessage(hWnd, 1000,
							CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)(DWORD)i); 
			if (lResult == CB_ERR ) 
				ExitProcess(8);
		}
		SendDlgItemMessage(hWnd, 1000, CB_SETCURSEL, 0, 0L);
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			lResult = SendDlgItemMessage(hWnd, 1000, CB_GETCURSEL, 0, 0);
			if (lResult == CB_ERR)
			{
				if (GetDlgItemText(hWnd, 1000, NameBuff, sizeof(NameBuff)) == 0)
					ExitProcess(8);
				SelBox[0].dir = TRUE;
				SelBox[0].wait= FALSE;
				strcpy(SelBox[0].cmd, NameBuff);
				lResult = 0;
			}
			else
			{
				lResult = SendDlgItemMessage(hWnd, 1000, CB_GETITEMDATA, lResult, 0);
				if (lResult == CB_ERR)
					ExitProcess(8);
				else if (GetDlgItemText(hWnd, 1000, NameBuff, sizeof(NameBuff)) == 0)
					ExitProcess(8);
				if (lResult >= SelCnt
						|| strcmp(SelBox[lResult].cmd, NameBuff) != 0)
				{
					SelBox[0].dir = TRUE;
					SelBox[0].wait= FALSE;
					strcpy(SelBox[0].cmd, NameBuff);
					lResult = 0;
				}
			}
			EndDialog(hWnd, lResult);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, -1);
			return(TRUE);
		case 1003:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
			ofn.lpstrFilter		= "Program File(*.exe;*.com;*.bat)\0*.exe;*.com;*.bat\0ALL(*.*)\0*.*\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= &NameBuff[1];
			ofn.nMaxFile		= MAX_PATH;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= NULL;
			ofn.lpstrTitle		= NULL;
			ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			if (GetOpenFileName(&ofn))
			{
				NameBuff[0] = '"';
				strcat(NameBuff, "\"");
				SendDlgItemMessage(hWnd, 1000, EM_SETSEL, 0, (LPARAM)-2);
				SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)NameBuff);
			}
			break;
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	select dialog
 *----------------------------------------------------------------------------*/
BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD		procid;

	if (GetWindowThreadProcessId(hwnd, &procid))
	{
		if (procid == lParam)
		{
			SetForegroundWindow(hwnd);
			SetFocus(hwnd);
			return(0);
		}
	}
	return(1);
}

/*==============================================================================
 *	Main function
 *============================================================================*/
/*
 * ini file argument
 *		extend   directory   search line   regular expression   command line
 *		.alm   ; +         ; 30          ; Subject: hello     ; "vim32.exe" -no
 *		.alm   ; -         ; 30          ; Subject: Re: hello ; "vim32.exe"
 *		.c     ; +         ;             ;                    ; "vim32.exe"
 *		       ;           ;             ;                    ; "vim32.exe"
 *
 */
int APIENTRY
WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HINSTANCE	hInstance;
HINSTANCE	hPrevInstance;
LPSTR		lpCmdLine;
int			nCmdShow;
{
	char					cline[4096];
	char				*	p;
	int						rc;
	char					ini[MAX_PATH];
	char					buf[4096];
	FILE			*		fdi;

	GetModuleFileName(NULL, ini, sizeof(ini));
	if ((p = strrchr(ini, '.')) == NULL)
		ExitProcess(1);
	strcpy(p, ".ini");
	if ((fdi = fopen(ini, "r")) == NULL)
		ExitProcess(3);
	while (fgets(buf, sizeof(buf), fdi) != NULL)
	{
		char				*	data[5];
		int						member = 0;

		if (buf[0] == '#')
			continue;
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		memset(data, 0, sizeof(data));
		p = buf;
		data[member++] = p;
		while (*p)
		{
			if (*p == ';')
			{
				data[member++] = &p[1];
				*p = '\0';
			}
			++p;
			if (member >= (sizeof(data) / sizeof(char *)))
			{
				int						i;
				int						line = 25;
				int						bin = FALSE;
				int						val = 0;
				int						chdir = TRUE;
				int						wait = FALSE;

				for (i = 0; i < (sizeof(data) / sizeof(char *)); i++)
				{
					p = data[i];
					while (*p && (*p == ' ' || *p == '\t'))
						*p++;
					data[i] = p;
					p = &p[strlen(p) - 1];
					while (p > data[i] && (*p == ' ' || *p == '\t'))
						*p--;
					p[1] = '\0';
				}
				if (strlen(data[0]))	/* extend */
				{
					WIN32_FIND_DATA fb;
					HANDLE          hFind;

					if ((hFind = FindFirstFile(lpCmdLine, &fb)) == INVALID_HANDLE_VALUE)
					{
						if ((p = strrchr(lpCmdLine, '.')) == NULL)
							break;
					}
					else
					{
						FindClose(hFind);
						if ((p = strrchr(fb.cFileName, '.')) == NULL)
							break;
					}
					if (stricmp(p, data[0]) != 0)
					{
						if ((strlen(p) - 1) == strlen(data[0])
								&& p[strlen(p) - 1] == '"'
								&& strnicmp(p, data[0], strlen(data[0])) == 0)
							;
						else
							break;
						mbox(".ext = \"%s\":\"%s\"", p, data[0]);
					}
					val += 100;
				}
				if (strlen(data[1]))	/* directory */
				{
					char		*	w = data[1];

					while (*w)
					{
						switch (*w) {
						case '-':
							chdir = FALSE;
							break;
						case '+':
							chdir = TRUE;
							break;
						case 's':
						case 'c': {
							BYTE	KeyState[256];

							if (GetKeyboardState(KeyState))
							{
								if (*w == 's' && KeyState[VK_SHIFT] & 0x80)
								{
									mbox("key = %x", KeyState[VK_SHIFT]);
									val += 10;
								}
								else if (*w == 'c' && KeyState[VK_CONTROL] & 0x80)
								{
									mbox("key = %x", KeyState[VK_CONTROL]);
									val += 10;
								}
								else
									val -= 10000;
							}
							else
								val -= 10000;
							}
							break;
						case 'w':
							wait = TRUE;
							break;
						}
						w++;
					}
				}
				if (strlen(data[2]))	/* search line number */
				{
					if (data[2][0] == 'b' || data[2][0] == 'B')
						bin = TRUE;
					else
						line = strtol(data[2], (char **)0, 0);
				}
				if (strlen(data[3]))	/* regular expression */
				{
					int						match = FALSE;
					char					fname[MAX_PATH];

					strcpy(fname, lpCmdLine);
					if (lpCmdLine[0] == '"'
								&& lpCmdLine[strlen(lpCmdLine) - 1] == '"')
					{

						strcpy(fname, &lpCmdLine[1]);
						fname[strlen(fname) - 1] = '\0';
					}
					if (bin)
					{
						FILE			*		fdb;
						char			*		top;

						if ((fdb = fopen(fname, "rb")) == NULL)
							break;
						top = p = data[3];
						for (;;)
						{
							if (*p == '\0' || *p == ',')
							{
								if (*top == 'x')
									(void)fgetc(fdb);	/* skip 1byte */
								else if (strtol(top, (char **)0, 16) != fgetc(fdb))
									break;
								top = p + 1;
							}
							if (*p == '\0')
							{
								match = TRUE;
								break;
							}
							p++;
						}
						fclose(fdb);
					}
					else
					{
						regexp				*	prog;
						int						fd;
						LINE					str;

						memset(&str, 0, sizeof(str));
						if ((prog = regcomp(data[3])) == NULL)
							break;
						if ((fd = fileopen(fname)) < 0)
							break;
						while (line--)
						{
							if (fileread(fd, &str) < 0)
								break;
							if (regexec(prog, str.lp, TRUE))
							{
								match = TRUE;
								break;
							}
						}
						close(fd);
						free(prog);
						free(str.lp);
					}
					if (!match)
						break;
					val += 50;
					mbox("match = \"%s\"", data[3]);
				}
				if (val > SelVal)
				{
					SelCnt = 0;
					SelVal = val;
				}
				if (val >= SelVal)
				{
					int						found = FALSE;

					for (i = 0; i < SelCnt; i++)
					{
						if (strcmp(SelBox[i].cmd, data[4]) == 0)
						{
							found = TRUE;
							break;
						}
					}
					if (!found && (sizeof(SelBox) / sizeof(SEL)) > SelCnt)
					{
						strcpy(SelBox[SelCnt].cmd, data[4]);
						SelBox[SelCnt].dir = chdir;
						SelBox[SelCnt].val = val;
						SelBox[SelCnt].wait= wait;
						SelCnt++;
					}
				}
				break;
			}
		}
	}
	fclose(fdi);
	rc = 0;
	if (SelCnt == 0
			|| (SelCnt > 1 && (rc = DialogBoxParam(hInstance, "SELECT", NULL, DialogProc, (LPARAM)0)) < 0))
		ExitProcess(9);
	wsprintf(cline, "%s %s", SelBox[rc].cmd, lpCmdLine);
	if (SelBox[rc].dir)
	{
		char					dir[MAX_PATH];
		char				*	tname;

		if (GetFullPathName(lpCmdLine, sizeof(dir), dir, &tname) != 0)
		{
			*tname = '\0';
			SetCurrentDirectory(dir);
		}
	}
	{
		STARTUPINFO				si;
		PROCESS_INFORMATION		pi;

		ZeroMemory(&pi, sizeof(pi));
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.wShowWindow = SW_SHOWNORMAL;
		if (CreateProcess(NULL, cline, NULL, NULL, FALSE,
							CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) != TRUE)
			ExitProcess(9);
		Sleep(300);
		EnumWindows(EnumWindowsProc, pi.dwProcessId);
		if (SelBox[rc].wait)
			WaitForSingleObject(pi.hProcess, INFINITE);
	}
	ExitProcess(0);
}
