/*==============================================================================
 *	cab file support file
 *============================================================================*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <cabapi.h>

#define  __EXFILE__C
#include "exfile.h"
#include "cab.h"

/*==============================================================================
 *	Cab file support
 *============================================================================*/
typedef	WORD	(WINAPI *pUnCabGetVersion)(VOID);
typedef	BOOL	(WINAPI *pUnCabCheckArchive)(LPCSTR _szFileName,
						const int _iMode);
typedef	HARC	(WINAPI *pUnCabOpenArchive)(const HWND _hwnd,
						LPCSTR _szFileName, const DWORD _dwMode);
typedef	int		(WINAPI _export *pUnCabCloseArchive)(HARC _harc);
typedef	int		(WINAPI *pUnCab)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPSTR _szOutput, const DWORD _dwSize);
typedef	int		(WINAPI *pUnCabFindFirst)(HARC _harc, LPCSTR _szWildName,
						INDIVIDUALINFO FAR *lpSubInfo);
typedef	int		(WINAPI *pUnCabFindNext)(HARC _harc,
						INDIVIDUALINFO FAR *_lpSubInfo);
typedef	int		(WINAPI *pUnCabExtractMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPBYTE _szBuffer, const DWORD _dwSize, time_t *_lpTime,
						LPWORD _lpwAttr, LPDWORD _lpdwWriteSize);
typedef	int		(WINAPI *pUnCabCompressMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						const LPBYTE _szBuffer, const DWORD _dwSize,
						const time_t *_lpTime, const LPWORD _lpwAttr,
						LPDWORD _lpdwWriteSize);
typedef struct readbuf {
	struct readbuf		*	next;
	char				*	fname;
} READBUF;


static	HANDLE				hCabLib				= NULL;
static	pUnCabGetVersion	pCabGetVersion		= NULL;
static	pUnCabCheckArchive	pCabCheckArchive	= NULL;
static	pUnCabOpenArchive	pCabOpenArchive		= NULL;
static	pUnCabCloseArchive	pCabCloseArchive	= NULL;
static	pUnCab				pCab				= NULL;
static	pUnCabFindFirst		pCabFindFirst		= NULL;
static	pUnCabFindNext		pCabFindNext		= NULL;
static	pUnCabExtractMem	pCabExtractMem		= NULL;
static	pUnCabCompressMem	pCabCompressMem		= NULL;

static	INDIVIDUALINFO		SubInfo;
static	char				find_fname[MAX_PATH];
static	char				find_aname[MAX_PATH];
static	READBUF			*	cab_rbuf			= NULL;
static	char				Tempdir[MAX_PATH];
static	char			*	ext[] = {".cab", ".exe", NULL};

extern	BOOL				ef_dir;

static	HWND				cab_disp;
#define LIBNAME				"cab32.dll"
#define PATHSEP				'/'
#define	EDISP				(128*1024)
#define	CDISP				(16*1024)

/*==============================================================================
 *	Cab support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	CHECK Cab file
 *----------------------------------------------------------------------------*/
static BOOL
cab_getname(name, sep, aname, fname)
const char	*	name;
char			sep;
char		*	aname;
char		*	fname;
{
	const char	*	p;
	char		**	ep		= ext;
	char			cab_fname[MAX_PATH];
	char			cab_aname[MAX_PATH];

	if (hCabLib == NULL)
		return(FALSE);
	if (strlen(name) <= 5)
		return(FALSE);
	if (fname == NULL)
		fname = cab_fname;
	if (aname == NULL)
		aname = cab_aname;
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
									&& (*pCabCheckArchive)(aname, 1) == FALSE)
						return(FALSE);
				}
				else if ((*pCabCheckArchive)(aname, 1) == FALSE)
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
 *	Cab init
 *----------------------------------------------------------------------------*/
BOOL
cab_init(window)
HWND			window;
{
	char			*	p;
	UINT				w;

	if (hCabLib == NULL)
	{
		cab_disp = window;
		hCabLib = LoadLibrary(LIBNAME);
		if (hCabLib == NULL)
			return(FALSE);
		Tempdir[0] = '\0';
		if ((p = getenv("TMP")) == NULL)
			p = getenv("TEMP");
		if (p != NULL)
		{
			strcpy(Tempdir, p);
			w = strlen(Tempdir) - 1;
			if (Tempdir[w] == '\\' || Tempdir[w] == '/')
				;
			else
				strcat(Tempdir, "\\");
		}
		pCabGetVersion
			= (pUnCabGetVersion)GetProcAddress(hCabLib, "CabGetVersion");
		pCabCheckArchive
			= (pUnCabCheckArchive)GetProcAddress(hCabLib, "CabCheckArchive");
		pCabOpenArchive
			= (pUnCabOpenArchive)GetProcAddress(hCabLib, "CabOpenArchive");
		pCabCloseArchive
			= (pUnCabCloseArchive)GetProcAddress(hCabLib, "CabCloseArchive");
		pCab	= (pUnCab)GetProcAddress(hCabLib, "Cab");
		pCabFindFirst
			= (pUnCabFindFirst)GetProcAddress(hCabLib, "CabFindFirst");
		pCabFindNext
			= (pUnCabFindNext)GetProcAddress(hCabLib, "CabFindNext");
		pCabExtractMem
			= (pUnCabExtractMem)GetProcAddress(hCabLib, "CabExtractMem");
		pCabCompressMem
			= (pUnCabCompressMem)GetProcAddress(hCabLib, "CabCompressMem");
		if (pCabGetVersion && pCabCheckArchive && pCabOpenArchive
						&& pCabCloseArchive && pCab && pCabFindFirst
						&& pCabFindNext && pCabExtractMem/*&& pCabCompressMem*/)
			;
		else
		{
			FreeLibrary(hCabLib);
			hCabLib = NULL;
			return(FALSE);
		}
		if ((*pCabGetVersion)() < 84)
		{
			FreeLibrary(hCabLib);
			hCabLib = NULL;
			return(FALSE);
		}
	}
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	Cab term
 *----------------------------------------------------------------------------*/
VOID
cab_term()
{
	if (hCabLib != NULL)
	{
		FreeLibrary(hCabLib);
		hCabLib = NULL;
	}
}

/*------------------------------------------------------------------------------
 *	Cab term
 *----------------------------------------------------------------------------*/
char *
cab_ver()
{
	static char		ver[1024];
	int				i;

	sprintf(ver, "%s : %d.%d (",
			LIBNAME, (*pCabGetVersion)() / 100, (*pCabGetVersion)() % 100);
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
 *	CHECK Cab file
 *----------------------------------------------------------------------------*/
BOOL
is_cab(fname, sep)
const char	*	fname;
char			sep;
{
	return(cab_getname(fname, sep, NULL, NULL));
}

/*------------------------------------------------------------------------------
 *	CHECK Cab file
 *----------------------------------------------------------------------------*/
BOOL
cab_sfx(fname)
char		*	fname;
{
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	CHECK Cab file
 *----------------------------------------------------------------------------*/
BOOL
cab_get_aname(fname, sep, aname)
const char	*	fname;
char			sep;
char		*	aname;
{
	return(cab_getname(fname, sep, aname, NULL));
}

/*------------------------------------------------------------------------------
 *	open() emulation
 *----------------------------------------------------------------------------*/
BOOL
cab_getfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				cab_fname[MAX_PATH];
	char				cab_aname[MAX_PATH];
	char				cab_cline[MAX_PATH + 32];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;

	if (!cab_getname(fname, ':', cab_aname, cab_fname))
		return(FALSE);
	if ((hFile = cab_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(FALSE);
	cab_findclose(hFile);
#if 1
	{
		sprintf(cab_cline, "-x -j%c \"%s\" \"%s\" \"%s\"",
			(data.nFileSizeLow < EDISP || !cab_disp) ? 'i' : ' ', cab_aname, Tempdir, cab_fname);
		if ((*pCab)(cab_disp, cab_cline, NULL, 0) != 0)
			return(FALSE);
		strcpy(cab_aname, Tempdir);
		strcat(cab_aname, ef_gettail(cab_fname));
		rename(cab_aname, tmp);
	}
#else
	{
		int					fd;
		char			*	p		= NULL;

		if ((p = malloc(data.nFileSizeLow + 1000)) == NULL)
			return(FALSE);
		sprintf(cab_cline, "%s \"%s\" \"%s\"",
			(data.nFileSizeLow < EDISP || !cab_disp) ? "-i" : "", cab_aname, cab_fname);
		if ((*pCabExtractMem)(cab_disp, cab_cline, p,
							data.nFileSizeLow + 1, NULL, NULL, NULL) != 0)
		{
			sprintf(cab_cline, "-d1 -gm -n -gp0 %s %s", cab_aname, cab_fname);
			if ((*pCabExtractMem)(cab_disp, cab_cline, p,
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
	}
#endif
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	close emulation
 *----------------------------------------------------------------------------*/
BOOL
cab_putfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				cab_fname[MAX_PATH];
	char				cab_aname[MAX_PATH];
	char				cab_cline[MAX_PATH + 32];
	struct stat			st;
	char			*	p		= NULL;
	long 				perm	= -1;
	int					fd;

	if (!cab_getname(fname, ':', cab_aname, cab_fname))
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
	sprintf(cab_cline, "-d1 -gm %s \"%s\" \"%s\"",
				st.st_size < CDISP || !cab_disp ? "-n -gp0" : "", cab_aname, cab_fname);
	perm = ef_getperm(cab_aname);
	if (perm > 0)
		chmod(cab_aname, S_IREAD|S_IWRITE);
	if ((*pCabCompressMem)(cab_disp, cab_cline, p, st.st_size,
												NULL, NULL, NULL) != 0)
		goto error;
	if (perm > 0)
		chmod(cab_aname, perm);
	return(TRUE);
error:
	if (p != NULL)
		free(p);
	if (perm > 0)
		chmod(cab_aname, perm);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	unlink()/remove() emulation
 *----------------------------------------------------------------------------*/
int
cab_remove(fname)
const char	*	fname;
{
	char				cab_fname[MAX_PATH];
	char				cab_aname[MAX_PATH];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	long 				perm;

	if ((hFile = cab_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(-1);
	cab_findclose(hFile);
	cab_getname(fname, ':', cab_aname, cab_fname);
	if ((perm = ef_getperm(cab_aname)) == (-1))
		return(-1);
	chmod(cab_aname, S_IREAD|S_IWRITE);
	chmod(cab_aname, perm);
	return(0);
}

/*------------------------------------------------------------------------------
 *	chmod() emulation
 *----------------------------------------------------------------------------*/
int
cab_chmod(name, perm)
const char	*	name;
int				perm;
{
	return(0);
}

/*------------------------------------------------------------------------------
 *	fullpath() emulation
 *----------------------------------------------------------------------------*/
char *
cab_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	char				cab_fname[MAX_PATH];
	char				cab_aname[MAX_PATH];

	if (!cab_getname(fname, ':', cab_aname, cab_fname))
		return(NULL);
	if (_fullpath(buf, cab_aname, len - strlen(cab_fname)) == NULL)
		return(NULL);
	strcat(buf, ":");
	strcat(buf, cab_fname);
	return(buf);
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
DWORD
cab_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	char				cab_fname[MAX_PATH];
	char				cab_aname[MAX_PATH];
	char			*	p;

	if (!cab_getname(lpszFile, ':', cab_aname, cab_fname))
		return(0);
	if (GetFullPathName(cab_aname, cchPath, lpszPath, ppszFilePart) == 0)
		return(0);
	if ((strlen(cab_fname) + strlen(lpszPath) + 1) > cchPath)
		return(0);
	strcat(lpszPath, ":");
	*ppszFilePart = lpszPath + strlen(lpszPath);
	if (ef_dir)
	{
		if ((p = strrchr(cab_fname, PATHSEP)) != NULL)
			*ppszFilePart += p - cab_fname + 1;
	}
	strcat(lpszPath, cab_fname);
	return(strlen(lpszPath));
}

/*------------------------------------------------------------------------------
 *	findnext() emulation
 *----------------------------------------------------------------------------*/
int
cab_findnext(harc, data)
HANDLE			harc;
WIN32_FIND_DATA	* 	data;
{
	int				pos;
	int				n;
	char		*	p;
	READBUF		*	rbuf;
	READBUF		**	brbuf;

retry:
	if ((*pCabFindNext)((HARC)harc, &SubInfo) == 0)
	{
		ef_slash(SubInfo.szFileName);
		ef_slash(find_fname);
		if (!ef_dir)
		{
			if (SubInfo.szFileName[strlen(SubInfo.szFileName) - 1] == PATHSEP)
				goto retry;
			memset(data->cFileName, '\0', sizeof(data->cFileName));
			strcpy(data->cFileName, SubInfo.szFileName);
			data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			if (strchr(SubInfo.szAttribute, 'R') != NULL)
				data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
			data->nFileSizeHigh = 0;
			data->nFileSizeLow  = SubInfo.dwOriginalSize;
			return(TRUE);
		}
		if ((p = strrchr(find_fname, PATHSEP)) != NULL)
		{
			if (strnicmp(find_fname, SubInfo.szFileName, p - find_fname + 1) != 0)
				goto retry;
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
					goto retry;
				if (SubInfo.szFileName[strlen(find_fname)] == PATHSEP
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
					goto retry;
			}
		}
		else
		{
			if (find_fname[0] != PATHSEP && SubInfo.szFileName[0] == PATHSEP)
				goto retry;
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
					goto retry;
				if (SubInfo.szFileName[strlen(find_fname)] == PATHSEP
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
		if (strchr(SubInfo.szAttribute, 'R') != NULL)
			data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		if (p != NULL)
			pos = p - find_fname + 1;
		n = strlen(&SubInfo.szFileName[pos]);
		if ((p = strchr(&SubInfo.szFileName[pos], PATHSEP)) != NULL)
		{
			if (pos == 0 && p == SubInfo.szFileName)
				n = 1;
			else
				n = p - &SubInfo.szFileName[pos];
			data->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		}
		memset(data->cFileName, '\0', sizeof(data->cFileName));
		strncpy(data->cFileName, &SubInfo.szFileName[pos], n);
		rbuf = cab_rbuf;
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
			if ((cab_rbuf = malloc(sizeof(READBUF))) == NULL)
				return(FALSE);
			brbuf = &cab_rbuf;
			rbuf = cab_rbuf;
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
cab_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	HARC			hArc;
	int				pos;
	int				n;
	char		*	p;

	if (!cab_getname(name, ':', find_aname, find_fname))
		return(INVALID_HANDLE_VALUE);
	cab_rbuf = NULL;
	if ((*pCabCheckArchive)(find_aname, 1) == FALSE)
		return(INVALID_HANDLE_VALUE);
	if ((hArc = (*pCabOpenArchive)(cab_disp, find_aname, M_INIT_FILE_USE)) == NULL)
		return(INVALID_HANDLE_VALUE);
	if ((*pCabFindFirst)(hArc, "*.*", &SubInfo) == 0)
	{
		ef_slash(SubInfo.szFileName);
		ef_slash(find_fname);
		if (!ef_dir)
		{
			if (SubInfo.szFileName[strlen(SubInfo.szFileName) - 1] == PATHSEP)
			{
				if (cab_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pCabCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			memset(data->cFileName, '\0', sizeof(data->cFileName));
			strcpy(data->cFileName, SubInfo.szFileName);
			data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			if (strchr(SubInfo.szAttribute, 'R') != NULL)
				data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
			data->nFileSizeHigh = 0;
			data->nFileSizeLow  = SubInfo.dwOriginalSize;
			return((HANDLE)hArc);
		}
		if ((p = strrchr(find_fname, PATHSEP)) != NULL)
		{
			if (SubInfo.szFileName[p - find_fname + 1] == '\0')
			{
				if (cab_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pCabCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if (strnicmp(find_fname, SubInfo.szFileName, p - find_fname + 1) != 0)
			{
				if (cab_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pCabCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (cab_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pCabCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == PATHSEP
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (cab_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pCabCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
			}
		}
		else
		{
			if (find_fname[0] != PATHSEP && SubInfo.szFileName[0] == PATHSEP)
			{
				if (cab_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pCabCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (cab_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pCabCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == PATHSEP
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (cab_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pCabCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
			}
		}
		pos = 0;
		data->nFileSizeHigh = 0;
		data->nFileSizeLow  = SubInfo.dwOriginalSize;
		data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		if (strchr(SubInfo.szAttribute, 'R') != NULL)
			data->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		if (p != NULL)
			pos = p - find_fname + 1;
		n = strlen(&SubInfo.szFileName[pos]);
		if ((p = strchr(&SubInfo.szFileName[pos], PATHSEP)) != NULL)
		{
			if (pos == 0 && p == SubInfo.szFileName)
				n = 1;
			else
				n = p - &SubInfo.szFileName[pos];
			data->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		}
		memset(data->cFileName, '\0', sizeof(data->cFileName));
		strncpy(data->cFileName, &SubInfo.szFileName[pos], n);
		if ((cab_rbuf = malloc(sizeof(READBUF))) == NULL)
		{
			(*pCabCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		cab_rbuf->next = NULL;
		if ((cab_rbuf->fname = malloc(strlen(data->cFileName) + 1)) == NULL)
		{
			free(cab_rbuf);
			cab_rbuf = NULL;
			(*pCabCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		strcpy(cab_rbuf->fname, data->cFileName);
		return((HANDLE)hArc);
	}
	(*pCabCloseArchive)(hArc);
	return(INVALID_HANDLE_VALUE);
}

/*------------------------------------------------------------------------------
 *	findclose() emulation
 *----------------------------------------------------------------------------*/
void
cab_findclose(harc)
HANDLE			harc;
{
	READBUF		*	rbuf;
	READBUF		*	nrbuf;

	(*pCabCloseArchive)((HARC)harc);
	rbuf = cab_rbuf;
	while (rbuf)
	{
		nrbuf = rbuf->next;
		free(rbuf->fname);
		free(rbuf);
		rbuf = nrbuf;
	}
	cab_rbuf = NULL;
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
BOOL
cab_breakcheck()
{
	return(FALSE);
}
