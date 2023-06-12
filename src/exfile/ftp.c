/*==============================================================================
 *	LHA file support file
 *============================================================================*/
#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <unlha32.h>

#define  __EXFILE__C
#include "exfile.h"
#include "ftp.h"

/*==============================================================================
 *	LHA file support
 *============================================================================*/
#define MAX_LENGTH		256
typedef struct {
	DWORD				access;
	BOOL				proxy;
	char				host[MAX_LENGTH];
	char				user[MAX_LENGTH];
	char				pass[MAX_LENGTH];
} cache;
static cache			Cache[8];

static HINTERNET		hFindOpen;
static HINTERNET		hFindConnect; 
static HWND				hFtpWnd;

static char				Tempbase[MAX_PATH];

typedef	INTERNETAPI HINTERNET	(WINAPI *pInternetOpenA)(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
typedef	INTERNETAPI BOOL		(WINAPI	*pInternetCloseHandle)(HINTERNET);
typedef	INTERNETAPI BOOL		(WINAPI *pInternetSetOptionExA)(HINTERNET, DWORD, LPVOID, DWORD, DWORD);
typedef	INTERNETAPI HINTERNET	(WINAPI *pInternetConnectA)(HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD);
typedef	INTERNETAPI BOOL		(WINAPI *pInternetFindNextFileA)(HINTERNET, LPVOID);
typedef	INTERNETAPI BOOL		(WINAPI *pFtpGetFileA)(HINTERNET, LPCSTR, LPCSTR, BOOL, DWORD, DWORD, DWORD);
typedef	INTERNETAPI BOOL		(WINAPI *pFtpPutFileA)(HINTERNET, LPCSTR, LPCSTR, DWORD, DWORD);
typedef	INTERNETAPI BOOL		(WINAPI *pFtpDeleteFileA)(HINTERNET, LPCSTR);
typedef	INTERNETAPI HINTERNET	(WINAPI *pFtpFindFirstFileA)(HINTERNET, LPCSTR, LPWIN32_FIND_DATA, DWORD, DWORD);
typedef	URLCACHEAPI BOOL		(WINAPI *pDeleteUrlCacheEntry)(LPCSTR);

static	HANDLE					hWininetLib				= NULL;
static	pInternetOpenA			pInternetOpen			= NULL;
static	pInternetCloseHandle	pInternetClose			= NULL;
static	pInternetSetOptionExA	pInternetSetOption		= NULL;
static	pInternetConnectA		pInternetConnect		= NULL;
static	pInternetFindNextFileA	pInternetFindNextFile	= NULL;
static	pFtpGetFileA			pFtpGetFile				= NULL;
static	pFtpPutFileA			pFtpPutFile				= NULL;
static	pFtpDeleteFileA			pFtpDeleteFile			= NULL;
static	pFtpFindFirstFileA		pFtpFindFirstFile		= NULL;
static	pDeleteUrlCacheEntry	pDeleteCache			= NULL;

static	BOOL					ftp_stop				= FALSE;

/*==============================================================================
 *	LHA support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
static BOOL
ftp_getname(name, hname, fname)
char		*	name;
char		*	hname;
char		*	fname;
{
	char		*	p;
	int				pos;

	if (hWininetLib == NULL)
		return(FALSE);
	hname[0] = fname[0] = '\0';
	if (strncmp(name, "ftp://", 6) != 0)
		return(FALSE);
	/* get hostname */
	pos	= 0;
	p	= &name[6];
	while (*p)
	{
		if (*p == '/')
		{
			hname[pos] = '\0';
			p++;
			break;
		}
		hname[pos++] = *p++;
	}
	if (hname[0] == '\0')
		return(FALSE);
	pos = 0;
	while (*p)
		fname[pos++] = *p++;
	fname[pos] = '\0';
	if (fname[0] == '\0')
		return(FALSE);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int					wmId;
	HWND				hwndOwner;
	RECT				rc;
	RECT				rcDlg;
	RECT				rcOwner;
	static cache	*	cp = NULL;

	switch (uMsg) {
	case WM_INITDIALOG:
		hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hWnd, &rcDlg);
		CopyRect(&rc, &rcOwner);
		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rc, -rc.left, -rc.top);
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
		SetWindowPos(hWnd, HWND_TOP,
				rcOwner.left + (rc.right / 2), rcOwner.top + (rc.bottom / 2),
				0, 0, SWP_NOSIZE);
		cp = (cache *)lParam;
		SetForegroundWindow(hWnd);
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			if (cp != NULL)
			{
				GetDlgItemText(hWnd, 1000, cp->user,  MAX_LENGTH);
				GetDlgItemText(hWnd, 1001, cp->pass,  MAX_LENGTH);
				cp->proxy = SendDlgItemMessage(hWnd, 1004, BM_GETCHECK, 0, 0);
			}
			cp = NULL;
			EndDialog(hWnd, 0);
			return(TRUE);
		case IDCANCEL:
			cp = NULL;
			EndDialog(hWnd, 1);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
static BOOL
ftp_connect(host, hOpen, hConnect)
char			*	host;
HINTERNET		*	hOpen;
HINTERNET		*	hConnect; 
{
	int					i;
	static cache		work;
	cache			*	cp = NULL;
	int					time_out = 60000;

	for (i = 0; i < (sizeof(Cache) / sizeof(cache)); i++)
	{
		if (strcmp(Cache[i].host, host) == 0)
		{
			cp = &Cache[i];
			break;
		}
	}
	if (cp == NULL)
	{
		if (strcmp(work.host, host) == 0
								&& (work.access + 10000) > GetTickCount())
			return(FALSE);
		memset(&work, 0, sizeof(work));
		cp = &work;
		strcpy(work.host, host);
		/* dialog */
		if (DialogBoxParam(GetModuleHandle(NULL),
								"LOGIN", hFtpWnd, DialogProc, (LPARAM)cp) != 0)
		{
			cp->access = GetTickCount();
			ftp_stop = TRUE;
			return(FALSE); 
		}
	}
	if (cp->proxy)
	{
		if ((*hOpen = (*pInternetOpen)("FTP", INTERNET_OPEN_TYPE_PRECONFIG,
												NULL, NULL, 0)) == NULL)
			return(FALSE); 
		(*pInternetSetOption)(*hOpen, INTERNET_OPTION_CONNECT_TIMEOUT, &time_out, sizeof(time_out), 0);
		*hConnect = (*pInternetConnect)(*hOpen, host,
					INTERNET_INVALID_PORT_NUMBER, cp->user, cp->pass,
					INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	}
	else
	{
		if ((*hOpen = (*pInternetOpen)("FTP", INTERNET_OPEN_TYPE_DIRECT,
												NULL, NULL, 0)) == NULL) 
			return(FALSE); 
		(*pInternetSetOption)(*hOpen, INTERNET_OPTION_CONNECT_TIMEOUT, &time_out, sizeof(time_out), 0);
		*hConnect = (*pInternetConnect)(*hOpen, host,
					INTERNET_INVALID_PORT_NUMBER, cp->user, cp->pass,
					INTERNET_SERVICE_FTP, 0, 0);
	}
    if (*hConnect == NULL)
	{
		(*pInternetClose)(*hOpen);
		if (cp != &work)
		{
			memmove(&work, cp, sizeof(work));
			cp->host[0] = '\0';
			cp->access = 0;
		}
		work.access = GetTickCount();
		return(FALSE);
	}
	cp->access = GetTickCount();
	if (cp == &work)
	{
		DWORD	access = Cache[0].access;
		int		no = 0;

		for (i = 1; i < (sizeof(Cache) / sizeof(cache)); i++)
		{
			if (Cache[i].access < access)
			{
				access = Cache[i].access;
				no = i;
			}
		}
		memmove(&Cache[no], cp, sizeof(cache));
	}
    return(TRUE);
}

/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
static void
ftp_disconnect(hOpen, hConnect)
HINTERNET			hOpen;
HINTERNET			hConnect; 
{
	(*pInternetClose)(hConnect);
	(*pInternetClose)(hOpen);
}

/*------------------------------------------------------------------------------
 *	LHA init
 *----------------------------------------------------------------------------*/
BOOL
ftp_init(window)
HWND			window;
{
	int				i;
	char		*	p;

	if (!isatty(0) || !isatty(1))
		return(FALSE);
	hFtpWnd = window;
	if (hWininetLib == NULL)
	{
		hWininetLib = LoadLibrary("wininet.dll");
		if (hWininetLib == NULL)
			return(FALSE);
		pInternetOpen
			= (pInternetOpenA)GetProcAddress(hWininetLib, "InternetOpenA");
		pInternetClose
			= (pInternetCloseHandle)GetProcAddress(hWininetLib, "InternetCloseHandle");
		pInternetSetOption
			= (pInternetSetOptionExA)GetProcAddress(hWininetLib, "InternetSetOptionExA");
		pInternetConnect
			= (pInternetConnectA)GetProcAddress(hWininetLib, "InternetConnectA");
		pInternetFindNextFile
			= (pInternetFindNextFileA)GetProcAddress(hWininetLib, "InternetFindNextFileA");
		pFtpGetFile
			= (pFtpGetFileA)GetProcAddress(hWininetLib, "FtpGetFileA");
		pFtpPutFile
			= (pFtpPutFileA)GetProcAddress(hWininetLib, "FtpPutFileA");
		pFtpDeleteFile
			= (pFtpDeleteFileA)GetProcAddress(hWininetLib, "FtpDeleteFileA");
		pFtpFindFirstFile
			= (pFtpFindFirstFileA)GetProcAddress(hWininetLib, "FtpFindFirstFileA");
		pDeleteCache
			= (pDeleteUrlCacheEntry)GetProcAddress(hWininetLib, "DeleteUrlCacheEntry");
		if (pInternetOpen && pInternetClose && pInternetSetOption
				&& pInternetConnect && pInternetFindNextFile
				&& pFtpGetFile && pFtpPutFile && pFtpDeleteFile
				&& pFtpFindFirstFile && pDeleteCache)
			;
		else
		{
			FreeLibrary(hWininetLib);
			hWininetLib = NULL;
			return(FALSE);
		}
		for (i = 0; i < (sizeof(Cache) / sizeof(cache)); i++)
		{
			Cache[i].host[0] = '\0';
			Cache[i].access = 0;
		}
		Tempbase[0] = '\0';
		if ((p = getenv("TMP")) == NULL)
			p = getenv("TEMP");
		if (p != NULL)
		{
			strcpy(Tempbase, p);
			i = strlen(Tempbase) - 1;
			if (Tempbase[i] == '\\' || Tempbase[i] == '/')
				;
			else
				strcat(Tempbase, "\\");
		}
		strcat(Tempbase, "efXXXXXX");
	}
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	LHA term
 *----------------------------------------------------------------------------*/
VOID
ftp_term()
{
	FreeLibrary(hWininetLib);
}

/*------------------------------------------------------------------------------
 *	LHA term
 *----------------------------------------------------------------------------*/
char *
ftp_ver()
{
	static char		ver[1024] = "ftp : 1.0 (ftp://host/xxxx)";
	return(ver);
}

/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
BOOL
is_ftp(fname, dummy)
const char	*	fname;
char			dummy;
{
	if (hWininetLib == NULL)
		return(FALSE);
	if (strncmp(fname, "ftp://", 6) == 0)
		return(TRUE);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
BOOL
ftp_sfx(fname)
char		*	fname;
{
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	open() emulation
 *----------------------------------------------------------------------------*/
BOOL
ftp_getfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				ftp_fname[MAX_PATH];
	char				ftp_hname[MAX_PATH];
	char				ftp_delete[MAX_PATH];
	int					i;
	HINTERNET			hOpen;
	HINTERNET			hConnect; 

	if (!ftp_getname(fname, ftp_hname, ftp_fname))
		return(FALSE);
	if (!ftp_connect(ftp_hname, &hOpen, &hConnect))
		return(FALSE);
	if (!(*pFtpGetFile)(hConnect, ftp_fname, tmp, FALSE,
							INTERNET_FLAG_RELOAD, FTP_TRANSFER_TYPE_BINARY, 0)) 
	{
		ftp_disconnect(hOpen, hConnect);
		return(FALSE);
	}
	ftp_disconnect(hOpen, hConnect);
	strcpy(ftp_delete, "ftp://");
	strcat(ftp_delete, ftp_hname);
	strcat(ftp_delete, "/");
	for (i = 0; i < sizeof(ftp_fname) && ftp_fname[i] != '\0'; i++)
	{
		if (ftp_fname[i] == '/')
			;
		else
		{
			strcat(ftp_delete, &ftp_fname[i]);
			(*pDeleteCache)(ftp_delete);
			break;
		}
	}
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	close emulation
 *----------------------------------------------------------------------------*/
BOOL
ftp_putfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				ftp_fname[MAX_PATH];
	char				ftp_hname[MAX_PATH];
	HINTERNET			hOpen;
	HINTERNET			hConnect; 

	if (!ftp_getname(fname, ftp_hname, ftp_fname))
		return(FALSE);
	if (!ftp_connect(ftp_hname, &hOpen, &hConnect))
		return(FALSE);
	if (!(*pFtpPutFile)(hConnect, tmp, ftp_fname, FTP_TRANSFER_TYPE_BINARY, 0))
	{
		ftp_disconnect(hOpen, hConnect);
		return(FALSE);
	}
	ftp_disconnect(hOpen, hConnect);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	unlink()/remove() emulation
 *----------------------------------------------------------------------------*/
int
ftp_remove(fname)
const char	*	fname;
{
	char				ftp_fname[MAX_PATH];
	char				ftp_hname[MAX_PATH];
	HINTERNET			hOpen;
	HINTERNET			hConnect; 

	if (!ftp_getname(fname, ftp_hname, ftp_fname))
		return(-1);
	if (!ftp_connect(ftp_hname, &hOpen, &hConnect))
		return(-1);
	if (!(*pFtpDeleteFile)(hConnect, ftp_fname)) 
	{
		ftp_disconnect(hOpen, hConnect);
		return(-1);
	}
	ftp_disconnect(hOpen, hConnect);
	return(0);
}

/*------------------------------------------------------------------------------
 *	chmod() emulation
 *----------------------------------------------------------------------------*/
int
ftp_chmod(name, perm)
const char	*	name;
int				perm;
{
	return(0);
}

/*------------------------------------------------------------------------------
 *	fullpath() emulation
 *----------------------------------------------------------------------------*/
char *
ftp_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	char				ftp_fname[MAX_PATH];
	char				ftp_hname[MAX_PATH];

	if (!ftp_getname(fname, ftp_hname, ftp_fname))
		return(NULL);
	strcpy(buf, fname);
	return(buf);
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
DWORD
ftp_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	return(0);
}

/*------------------------------------------------------------------------------
 *	findnext() emulation
 *----------------------------------------------------------------------------*/
int
ftp_findnext(hFind, data)
HANDLE				hFind;
WIN32_FIND_DATA	* 	data;
{
	return((*pInternetFindNextFile)(hFind, data));
}

/*------------------------------------------------------------------------------
 *	findfirst() emulation
 *----------------------------------------------------------------------------*/
HANDLE
ftp_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	HINTERNET				hFind; 
	char					ftp_fname[MAX_PATH];
	char					ftp_hname[MAX_PATH];
	char					ftp_tname[MAX_PATH];
	char				*	tail;
	BOOL					kanji = FALSE;
	int						i;

	if (!ftp_getname(name, ftp_hname, ftp_fname))
		return(INVALID_HANDLE_VALUE);
	if (!ftp_connect(ftp_hname, &hFindOpen, &hFindConnect))
		return(INVALID_HANDLE_VALUE);
	tail = ef_gettail(ftp_fname);
	memset(ftp_tname, '\0', sizeof(ftp_tname));
	if (tail != ftp_fname)
		strncpy(ftp_tname, ftp_fname, tail - ftp_fname);
	strcat(ftp_tname, "*.*");
	if ((hFind = (*pFtpFindFirstFile)(hFindConnect, ftp_tname, data, 0, 0)) == NULL)
	{
		ftp_disconnect(hFindOpen, hFindConnect);
		return(INVALID_HANDLE_VALUE);
	}
	if (strcmp(tail, "*.*") == 0 || strpbrk(tail, "*?") != NULL)
		return(hFind);
	do 
	{ 
		if (strcmp(data->cFileName, tail) == 0)
			return(hFind);
		for (i = 0; kanji == FALSE && i < sizeof(data->cFileName) && data->cFileName[i] != '\0'; i++)
		{
			if (data->cFileName[i] & 0x80)
			{
				kanji = TRUE;
				break;
			}
		}
		if (!ftp_findnext(hFind, data))
			break; 
	} while (TRUE); 
	if (kanji)
	{
		strcpy(ftp_fname, Tempbase);
		if (mktemp(ftp_fname) == NULL)
		{
			ftp_findclose(hFind);
			return(INVALID_HANDLE_VALUE);
		}
		if (ftp_getfile(name, ftp_fname))
		{
			data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			strcpy(data->cFileName, tail);
			chmod(ftp_fname, S_IREAD | S_IWRITE);
			unlink(ftp_fname);
			return(hFind);
		}
	}
	ftp_findclose(hFind);
	return(INVALID_HANDLE_VALUE);
}

/*------------------------------------------------------------------------------
 *	findclose() emulation
 *----------------------------------------------------------------------------*/
void
ftp_findclose(hFind)
HANDLE			hFind;
{
	(*pInternetClose)(hFind);
	ftp_disconnect(hFindOpen, hFindConnect);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
ftp_breakcheck()
{
	BOOL			rc = ftp_stop;

	ftp_stop = FALSE;
	return(rc);
}
