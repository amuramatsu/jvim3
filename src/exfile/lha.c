/*==============================================================================
 *	LHA file support file
 *============================================================================*/
#include <windows.h>
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
#include "lha.h"

/*==============================================================================
 *	LHA file support
 *============================================================================*/
typedef	WORD	(WINAPI *pUnlhaGetVersion)(VOID);
typedef	BOOL	(WINAPI *pUnlhaCheckArchive)(LPCSTR _szFileName,
						const int _iMode);
typedef	HARC	(WINAPI *pUnlhaOpenArchive)(const HWND _hwnd,
						LPCSTR _szFileName, const DWORD _dwMode);
typedef	int		(WINAPI _export *pUnlhaCloseArchive)(HARC _harc);
typedef	int		(WINAPI *pUnlha)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPSTR _szOutput, const DWORD _dwSize);
typedef	int		(WINAPI *pUnlhaFindFirst)(HARC _harc, LPCSTR _szWildName,
						INDIVIDUALINFO FAR *lpSubInfo);
typedef	int		(WINAPI *pUnlhaFindNext)(HARC _harc,
						INDIVIDUALINFO FAR *_lpSubInfo);
typedef	int		(WINAPI *pUnlhaExtractMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPBYTE _szBuffer, const DWORD _dwSize, time_t *_lpTime,
						LPWORD _lpwAttr, LPDWORD _lpdwWriteSize);
typedef	int		(WINAPI *pUnlhaCompressMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						const LPBYTE _szBuffer, const DWORD _dwSize,
						const time_t *_lpTime, const LPWORD _lpwAttr,
						LPDWORD _lpdwWriteSize);
typedef struct readbuf {
	struct readbuf		*	next;
	char				*	fname;
} READBUF;


static	HANDLE				hLhaLib				= NULL;
static	pUnlhaGetVersion	pLhaGetVersion		= NULL;
static	pUnlhaCheckArchive	pLhaCheckArchive	= NULL;
static	pUnlhaOpenArchive	pLhaOpenArchive		= NULL;
static	pUnlhaCloseArchive	pLhaCloseArchive	= NULL;
static	pUnlha				pLha				= NULL;
static	pUnlhaFindFirst		pLhaFindFirst		= NULL;
static	pUnlhaFindNext		pLhaFindNext		= NULL;
static	pUnlhaExtractMem	pLhaExtractMem		= NULL;
static	pUnlhaCompressMem	pLhaCompressMem		= NULL;

static	INDIVIDUALINFO		SubInfo;
static	char				find_fname[MAX_PATH];
static	char				find_aname[MAX_PATH];
static	READBUF			*	lha_rbuf			= NULL;
static	BOOL				lha_stop			= FALSE;
extern	BOOL				ef_dir;

static	HWND				lha_disp;
static	char			*	ext[] = {".lzh", ".exe", NULL};

#define LIBNAME				"unlha32.dll"
#define	EDISP				(128*1024)
#define	CDISP				(16*1024)

/*==============================================================================
 *	LHA support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
static BOOL
lha_getname(name, sep, aname, fname)
const char	*	name;
char			sep;
char		*	aname;
char		*	fname;
{
	const char	*	p;
	char		**	ep		= ext;
	char			lha_fname[MAX_PATH];
	char			lha_aname[MAX_PATH];

	if (hLhaLib == NULL)
		return(FALSE);
	if (strlen(name) <= 5)
		return(FALSE);
	if (fname == NULL)
		fname = lha_fname;
	if (aname == NULL)
		aname = lha_aname;
	while (*ep)
	{
		p = name + 1;
		while (*p)
		{
			if ((strlen(p) >= strlen(*ep))
					&& strnicmp(p, *ep, strlen(*ep)) == 0
					&& p[strlen(*ep)] == sep)
			{
				strncpy(aname, name, (p + strlen(*ep)) - name);
				aname[(p + strlen(*ep)) - name] = '\0';
				if (sep != '\0')
				{
					strcpy(fname, p + strlen(*ep) + 1);
					if (strlen(fname) == 0)
						return(FALSE);
					if (ef_getperm(aname) != (-1)
									&& (*pLhaCheckArchive)(aname, 1) == FALSE)
						return(FALSE);
				}
				else if ((*pLhaCheckArchive)(aname, 1) == FALSE)
					return(FALSE);
				return(TRUE);
			}
			p++;
		}
		ep++;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	LHA init
 *----------------------------------------------------------------------------*/
BOOL
lha_init(window)
HWND			window;
{
	if (hLhaLib == NULL)
	{
		lha_disp = window;
		hLhaLib = LoadLibrary(LIBNAME);
		if (hLhaLib == NULL)
			return(FALSE);
		pLhaGetVersion
			= (pUnlhaGetVersion)GetProcAddress(hLhaLib, "UnlhaGetVersion");
		pLhaCheckArchive
			= (pUnlhaCheckArchive)GetProcAddress(hLhaLib, "UnlhaCheckArchive");
		pLhaOpenArchive
			= (pUnlhaOpenArchive)GetProcAddress(hLhaLib, "UnlhaOpenArchive");
		pLhaCloseArchive
			= (pUnlhaCloseArchive)GetProcAddress(hLhaLib, "UnlhaCloseArchive");
		pLha	= (pUnlha)GetProcAddress(hLhaLib, "Unlha");
		pLhaFindFirst
			= (pUnlhaFindFirst)GetProcAddress(hLhaLib, "UnlhaFindFirst");
		pLhaFindNext
			= (pUnlhaFindNext)GetProcAddress(hLhaLib, "UnlhaFindNext");
		pLhaExtractMem
			= (pUnlhaExtractMem)GetProcAddress(hLhaLib, "UnlhaExtractMem");
		pLhaCompressMem
			= (pUnlhaCompressMem)GetProcAddress(hLhaLib, "UnlhaCompressMem");
		if (pLhaGetVersion && pLhaCheckArchive && pLhaOpenArchive
						&& pLhaCloseArchive && pLha && pLhaFindFirst
						&& pLhaFindNext && pLhaExtractMem && pLhaCompressMem)
			;
		else
		{
			FreeLibrary(hLhaLib);
			hLhaLib = NULL;
			return(FALSE);
		}
		if ((*pLhaGetVersion)() < 89 || (*pLhaGetVersion)() == 91)
		{
			FreeLibrary(hLhaLib);
			hLhaLib = NULL;
			return(FALSE);
		}
	}
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	LHA term
 *----------------------------------------------------------------------------*/
VOID
lha_term()
{
	FreeLibrary(hLhaLib);
	hLhaLib = NULL;
}

/*------------------------------------------------------------------------------
 *	LHA term
 *----------------------------------------------------------------------------*/
char *
lha_ver()
{
	static char		ver[1024];
	int				i;

	sprintf(ver, "%s : %d.%d (",
			LIBNAME, (*pLhaGetVersion)() / 100, (*pLhaGetVersion)() % 100);
	for (i = 0; ext[i] != NULL; i++)
	{
		if (i != 0)
			strcat(ver, "/");
		strcat(ver, ext[i]);
	}
	strcat(ver, ")");
	return(ver);
}

/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
BOOL
is_lha(fname, sep)
const char	*	fname;
char			sep;
{
	return(lha_getname(fname, sep, NULL, NULL));
}

/*------------------------------------------------------------------------------
 *	CHECK LHA file
 *----------------------------------------------------------------------------*/
BOOL
lha_sfx(fname)
char		*	fname;
{
	DWORD			type;
	char			lha_aname[MAX_PATH];

	if (lha_getname(fname, ':', lha_aname, NULL))
	{
		type = (*pLhaCheckArchive)(lha_aname, 9);
		if (type > 0x8000)
			return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
lha_get_aname(fname, sep, aname)
const char	*	fname;
char			sep;
char		*	aname;
{
	return(lha_getname(fname, sep, aname, NULL));
}

/*------------------------------------------------------------------------------
 *	open() emulation
 *----------------------------------------------------------------------------*/
BOOL
lha_getfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				lha_fname[MAX_PATH];
	char				lha_aname[MAX_PATH];
	char				lha_cline[MAX_PATH + 32];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	int					fd;
	char			*	p		= NULL;

	if (!lha_getname(fname, ':', lha_aname, lha_fname))
		return(FALSE);
	if ((hFile = lha_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(FALSE);
	lha_findclose(hFile);
	if ((p = malloc(data.nFileSizeLow + 10)) == NULL)
		return(FALSE);
	sprintf(lha_cline, "-d1 -gm %s \"%s\" \"%s\"",
		(data.nFileSizeLow < EDISP || !lha_disp) ? "-n -gp0" : "", lha_aname, lha_fname);
	if ((*pLhaExtractMem)(lha_disp, lha_cline, p,
						data.nFileSizeLow + 1, NULL, NULL, NULL) != 0)
	{
		lha_stop = TRUE;
		sprintf(lha_cline, "-d1 -gm -n -gp0 %s %s", lha_aname, lha_fname);
		if ((*pLhaExtractMem)(lha_disp, lha_cline, p,
						data.nFileSizeLow + 1, NULL, NULL, NULL) != 0)
		{
			free(p);
			return(FALSE);
		}
	}
	if ((fd = open(tmp, O_WRONLY | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) < 0)
	{
		free(p);
		return(FALSE);
	}
	write(fd, p, data.nFileSizeLow);
	close(fd);
	free(p);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	close emulation
 *----------------------------------------------------------------------------*/
BOOL
lha_putfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				lha_fname[MAX_PATH];
	char				lha_aname[MAX_PATH];
	char				lha_cline[MAX_PATH + 32];
	struct stat			st;
	char			*	p		= NULL;
	long 				perm	= -1;
	int					fd;

	if (!lha_getname(fname, ':', lha_aname, lha_fname))
		return(FALSE);
	if (stat(tmp, &st) != 0)
		goto error;
	if ((p = malloc(st.st_size + 1)) == NULL)
		goto error;
	if ((fd = open(tmp, O_RDONLY, S_IREAD | S_IWRITE)) < 0)
		goto error;
	if (st.st_size)
		read(fd, p, st.st_size);
	close(fd);
	sprintf(lha_cline, "-d1 -gm %s \"%s\" \"%s\"",
				st.st_size < CDISP || !lha_disp ? "-n -gp0" : "", lha_aname, lha_fname);
	perm = ef_getperm(lha_aname);
	if (perm > 0)
		chmod(lha_aname, S_IREAD|S_IWRITE);
	if ((*pLhaCompressMem)(lha_disp, lha_cline, p, st.st_size,
												NULL, NULL, NULL) != 0)
		goto error;
	if (perm > 0)
		chmod(lha_aname, perm);
	return(TRUE);
error:
	if (p != NULL)
		free(p);
	if (perm > 0)
		chmod(lha_aname, perm);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	unlink()/remove() emulation
 *----------------------------------------------------------------------------*/
int
lha_remove(fname)
const char	*	fname;
{
	char				lha_fname[MAX_PATH];
	char				lha_aname[MAX_PATH];
	char				lha_cline[MAX_PATH + 32];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	long 				perm;

	if ((hFile = lha_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(-1);
	lha_findclose(hFile);
	lha_getname(fname, ':', lha_aname, lha_fname);
	if ((perm = ef_getperm(lha_aname)) == (-1))
		return(-1);
	chmod(lha_aname, S_IREAD|S_IWRITE);
	sprintf(lha_cline, "d -d1 -n -gp0 -gm \"%s\" \"%s\"", lha_aname, lha_fname);
	(*pLha)(lha_disp, lha_cline, NULL, 0);
	chmod(lha_aname, perm);
	return(0);
}

/*------------------------------------------------------------------------------
 *	chmod() emulation
 *----------------------------------------------------------------------------*/
int
lha_chmod(name, perm)
const char	*	name;
int				perm;
{
	return(0);
}

/*------------------------------------------------------------------------------
 *	fullpath() emulation
 *----------------------------------------------------------------------------*/
char *
lha_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	char				lha_fname[MAX_PATH];
	char				lha_aname[MAX_PATH];

	if (!lha_getname(fname, ':', lha_aname, lha_fname))
		return(NULL);
	if (_fullpath(buf, lha_aname, len - strlen(lha_fname)) == NULL)
		return(NULL);
	strcat(buf, ":");
	strcat(buf, lha_fname);
	return(buf);
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
DWORD
lha_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	char				lha_fname[MAX_PATH];
	char				lha_aname[MAX_PATH];
	char			*	p;

	if (!lha_getname(lpszFile, ':', lha_aname, lha_fname))
		return(0);
	if (GetFullPathName(lha_aname, cchPath, lpszPath, ppszFilePart) == 0)
		return(0);
	if ((strlen(lha_fname) + strlen(lpszPath) + 1) > cchPath)
		return(0);
	strcat(lpszPath, ":");
	*ppszFilePart = lpszPath + strlen(lpszPath);
	if (ef_dir)
	{
		if ((p = strrchr(lha_fname, '/')) != NULL)
			*ppszFilePart += p - lha_fname + 1;
	}
	strcat(lpszPath, lha_fname);
	return(strlen(lpszPath));
}

/*------------------------------------------------------------------------------
 *	findnext() emulation
 *----------------------------------------------------------------------------*/
int
lha_findnext(harc, data)
HANDLE			harc;
WIN32_FIND_DATA	* 	data;
{
	int				pos;
	int				n;
	char		*	p;
	READBUF		*	rbuf;
	READBUF		**	brbuf;

retry:
	if ((*pLhaFindNext)((HARC)harc, &SubInfo) == 0)
	{
		if (!ef_dir)
		{
			memset(data->cFileName, '\0', sizeof(data->cFileName));
			strcpy(data->cFileName, SubInfo.szFileName);
			data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			if (strchr(SubInfo.szAttribute, 'W') == NULL)
				data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
			data->nFileSizeHigh = 0;
			data->nFileSizeLow  = SubInfo.dwOriginalSize;
			return(TRUE);
		}
		if ((p = strrchr(find_fname, '/')) != NULL)
		{
			if (strnicmp(find_fname, SubInfo.szFileName, p - find_fname + 1) != 0)
				goto retry;
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
					goto retry;
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
					goto retry;
			}
		}
		else
		{
			if (find_fname[0] != '/' && SubInfo.szFileName[0] == '/')
				goto retry;
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
					goto retry;
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
					goto retry;
			}
		}
		pos = 0;
		data->nFileSizeHigh = 0;
		data->nFileSizeLow  = SubInfo.dwOriginalSize;
		data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		if (strchr(SubInfo.szAttribute, 'W') == NULL)
			data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		if (p != NULL)
			pos = p - find_fname + 1;
		n = strlen(&SubInfo.szFileName[pos]);
		if ((p = strchr(&SubInfo.szFileName[pos], '/')) != NULL)
		{
			if (pos == 0 && p == SubInfo.szFileName)
				n = 1;
			else
				n = p - &SubInfo.szFileName[pos];
			data->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		}
		memset(data->cFileName, '\0', sizeof(data->cFileName));
		strncpy(data->cFileName, &SubInfo.szFileName[pos], n);
		rbuf = lha_rbuf;
		while (rbuf)
		{
			if (stricmp(rbuf->fname, data->cFileName) == 0)
				goto retry;
			if (rbuf->next == NULL)
				break;
			rbuf = rbuf->next;
		}
		if (rbuf == NULL)
		{
			if ((lha_rbuf = malloc(sizeof(READBUF))) == NULL)
				return(FALSE);
			brbuf = &lha_rbuf;
			rbuf = lha_rbuf;
		}
		else
		{
			if ((rbuf->next = malloc(sizeof(READBUF))) == NULL)
				return(FALSE);
			brbuf = &rbuf->next;
			rbuf = rbuf->next;
		}
		rbuf->next = NULL;
		if ((rbuf->fname = malloc(strlen(data->cFileName) + 1)) == NULL)
		{
			*brbuf = NULL;
			free(rbuf);
			return(FALSE);
		}
		strcpy(rbuf->fname, data->cFileName);
		return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	findfirst() emulation
 *----------------------------------------------------------------------------*/
HANDLE
lha_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	HARC			hArc;
	int				pos;
	int				n;
	char		*	p;

	if (!lha_getname(name, ':', find_aname, find_fname))
		return(INVALID_HANDLE_VALUE);
	lha_rbuf = NULL;
	if ((*pLhaCheckArchive)(find_aname, 1) == FALSE)
		return(INVALID_HANDLE_VALUE);
	if ((hArc = (*pLhaOpenArchive)(lha_disp, find_aname, M_INIT_FILE_USE)) == NULL)
		return(INVALID_HANDLE_VALUE);
	if ((*pLhaFindFirst)(hArc, "*.*", &SubInfo) == 0)
	{
		if (!ef_dir)
		{
			memset(data->cFileName, '\0', sizeof(data->cFileName));
			strcpy(data->cFileName, SubInfo.szFileName);
			data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			if (strchr(SubInfo.szAttribute, 'W') == NULL)
				data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
			data->nFileSizeHigh = 0;
			data->nFileSizeLow  = SubInfo.dwOriginalSize;
			return((HANDLE)hArc);
		}
		if ((p = strrchr(find_fname, '/')) != NULL)
		{
			if (strnicmp(find_fname, SubInfo.szFileName, p - find_fname + 1) != 0)
			{
				if (lha_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pLhaCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (lha_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pLhaCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (lha_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pLhaCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
			}
		}
		else
		{
			if (find_fname[0] != '/' && SubInfo.szFileName[0] == '/')
			{
				if (lha_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pLhaCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (lha_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pLhaCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (lha_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pLhaCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
			}
		}
		pos = 0;
		data->nFileSizeHigh = 0;
		data->nFileSizeLow  = SubInfo.dwOriginalSize;
		data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		if (strchr(SubInfo.szAttribute, 'W') == NULL)
			data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		if (p != NULL)
			pos = p - find_fname + 1;
		n = strlen(&SubInfo.szFileName[pos]);
		if ((p = strchr(&SubInfo.szFileName[pos], '/')) != NULL)
		{
			if (pos == 0 && p == SubInfo.szFileName)
				n = 1;
			else
				n = p - &SubInfo.szFileName[pos];
			data->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		}
		memset(data->cFileName, '\0', sizeof(data->cFileName));
		strncpy(data->cFileName, &SubInfo.szFileName[pos], n);
		if ((lha_rbuf = malloc(sizeof(READBUF))) == NULL)
		{
			(*pLhaCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		lha_rbuf->next = NULL;
		if ((lha_rbuf->fname = malloc(strlen(data->cFileName) + 1)) == NULL)
		{
			free(lha_rbuf);
			lha_rbuf = NULL;
			(*pLhaCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		strcpy(lha_rbuf->fname, data->cFileName);
		return((HANDLE)hArc);
	}
	(*pLhaCloseArchive)(hArc);
	return(INVALID_HANDLE_VALUE);
}

/*------------------------------------------------------------------------------
 *	findclose() emulation
 *----------------------------------------------------------------------------*/
void
lha_findclose(harc)
HANDLE			harc;
{
	READBUF		*	rbuf;
	READBUF		*	nrbuf;

	(*pLhaCloseArchive)((HARC)harc);
	rbuf = lha_rbuf;
	while (rbuf)
	{
		nrbuf = rbuf->next;
		free(rbuf->fname);
		free(rbuf);
		rbuf = nrbuf;
	}
	lha_rbuf = NULL;
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
BOOL
lha_breakcheck()
{
	BOOL			rc = lha_stop;

	lha_stop = FALSE;
	return(rc);
}
