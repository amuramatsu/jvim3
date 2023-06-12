/*==============================================================================
 *	zip file support file
 *============================================================================*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <unzip32.h>

#define  __EXFILE__C
#include "exfile.h"
#include "zip.h"

/*==============================================================================
 *	Zip file support
 *============================================================================*/
typedef	WORD	(WINAPI *pUnZipGetVersion)(VOID);
typedef	BOOL	(WINAPI *pUnZipCheckArchive)(LPCSTR _szFileName,
						const int _iMode);
typedef	HARC	(WINAPI *pUnZipOpenArchive)(const HWND _hwnd,
						LPCSTR _szFileName, const DWORD _dwMode);
typedef	int		(WINAPI _export *pUnZipCloseArchive)(HARC _harc);
typedef	int		(WINAPI *pUnZip)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPSTR _szOutput, const DWORD _dwSize);
typedef	int		(WINAPI *pUnZipFindFirst)(HARC _harc, LPCSTR _szWildName,
						INDIVIDUALINFO FAR *lpSubInfo);
typedef	int		(WINAPI *pUnZipFindNext)(HARC _harc,
						INDIVIDUALINFO FAR *_lpSubInfo);
typedef	int		(WINAPI *pUnZipExtractMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPBYTE _szBuffer, const DWORD _dwSize, time_t *_lpTime,
						LPWORD _lpwAttr, LPDWORD _lpdwWriteSize);
typedef	int		(WINAPI *pUnZipCompressMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						const LPBYTE _szBuffer, const DWORD _dwSize,
						const time_t *_lpTime, const LPWORD _lpwAttr,
						LPDWORD _lpdwWriteSize);
typedef struct readbuf {
	struct readbuf		*	next;
	char				*	fname;
} READBUF;


static	HANDLE				hZipLib				= NULL;
static	pUnZipGetVersion	pZipGetVersion		= NULL;
static	pUnZipCheckArchive	pZipCheckArchive	= NULL;
static	pUnZipOpenArchive	pZipOpenArchive		= NULL;
static	pUnZipCloseArchive	pZipCloseArchive	= NULL;
static	pUnZip				pZip				= NULL;
static	pUnZipFindFirst		pZipFindFirst		= NULL;
static	pUnZipFindNext		pZipFindNext		= NULL;
static	pUnZipExtractMem	pZipExtractMem		= NULL;
static	pUnZipCompressMem	pZipCompressMem		= NULL;

static	INDIVIDUALINFO		SubInfo;
static	char				find_fname[MAX_PATH];
static	char				find_aname[MAX_PATH];
static	READBUF			*	zip_rbuf			= NULL;
static	char			*	ext[] = {".zip", ".exe", NULL};

extern	BOOL				ef_dir;

static	HWND				zip_disp;
#define LIBNAME				"unzip32.dll"
#define	EDISP				(128*1024)
#define	CDISP				(16*1024)

/*==============================================================================
 *	Zip support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	CHECK Zip file
 *----------------------------------------------------------------------------*/
static BOOL
zip_getname(name, sep, aname, fname)
const char	*	name;
char			sep;
char		*	aname;
char		*	fname;
{
	const char	*	p;
	char		**	ep		= ext;
	char			zip_fname[MAX_PATH];
	char			zip_aname[MAX_PATH];

	if (hZipLib == NULL)
		return(FALSE);
	if (strlen(name) <= 4)
		return(FALSE);
	if (fname == NULL)
		fname = zip_fname;
	if (aname == NULL)
		aname = zip_aname;
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
									&& (*pZipCheckArchive)(aname, 1) == FALSE)
						return(FALSE);
				}
				else if ((*pZipCheckArchive)(aname, 1) == FALSE)
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
 *	Zip init
 *----------------------------------------------------------------------------*/
BOOL
zip_init(window)
HWND			window;
{
	if (hZipLib == NULL)
	{
		zip_disp = window;
		hZipLib = LoadLibrary(LIBNAME);
		if (hZipLib == NULL)
			return(FALSE);
		pZipGetVersion
			= (pUnZipGetVersion)GetProcAddress(hZipLib, "UnZipGetVersion");
		pZipCheckArchive
			= (pUnZipCheckArchive)GetProcAddress(hZipLib, "UnZipCheckArchive");
		pZipOpenArchive
			= (pUnZipOpenArchive)GetProcAddress(hZipLib, "UnZipOpenArchive");
		pZipCloseArchive
			= (pUnZipCloseArchive)GetProcAddress(hZipLib, "UnZipCloseArchive");
		pZip	= (pUnZip)GetProcAddress(hZipLib, "UnZip");
		pZipFindFirst
			= (pUnZipFindFirst)GetProcAddress(hZipLib, "UnZipFindFirst");
		pZipFindNext
			= (pUnZipFindNext)GetProcAddress(hZipLib, "UnZipFindNext");
		pZipExtractMem
			= (pUnZipExtractMem)GetProcAddress(hZipLib, "UnZipExtractMem");
		pZipCompressMem
			= (pUnZipCompressMem)GetProcAddress(hZipLib, "UnZipCompressMem");
		if (pZipGetVersion && pZipCheckArchive && pZipOpenArchive
						&& pZipCloseArchive && pZip && pZipFindFirst
						&& pZipFindNext && pZipExtractMem /*pZipCompressMem */)
			;
		else
		{
			FreeLibrary(hZipLib);
			hZipLib = NULL;
			return(FALSE);
		}
		if ((*pZipGetVersion)() < 540)
		{
			FreeLibrary(hZipLib);
			hZipLib = NULL;
			return(FALSE);
		}
	}
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	Zip term
 *----------------------------------------------------------------------------*/
VOID
zip_term()
{
	if (hZipLib != NULL)
	{
		FreeLibrary(hZipLib);
		hZipLib = NULL;
	}
}

/*------------------------------------------------------------------------------
 *	Zip term
 *----------------------------------------------------------------------------*/
char *
zip_ver()
{
	static char		ver[1024];
	int				i;

	sprintf(ver, "%s : %d.%d (",
			LIBNAME, (*pZipGetVersion)() / 100, (*pZipGetVersion)() % 100);
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
 *	CHECK Zip file
 *----------------------------------------------------------------------------*/
BOOL
is_zip(fname, sep)
const char	*	fname;
char			sep;
{
	return(zip_getname(fname, sep, NULL, NULL));
}

/*------------------------------------------------------------------------------
 *	CHECK Zip file
 *----------------------------------------------------------------------------*/
BOOL
zip_sfx(fname)
char		*	fname;
{
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
zip_get_aname(fname, sep, aname)
const char	*	fname;
char			sep;
char		*	aname;
{
	return(zip_getname(fname, sep, aname, NULL));
}

/*------------------------------------------------------------------------------
 *	open() emulation
 *----------------------------------------------------------------------------*/
BOOL
zip_getfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				zip_fname[MAX_PATH];
	char				zip_aname[MAX_PATH];
	char				zip_cline[MAX_PATH + 32];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	int					fd;
	char			*	p		= NULL;

	if (!zip_getname(fname, ':', zip_aname, zip_fname))
		return(FALSE);
	if ((hFile = zip_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(FALSE);
	zip_findclose(hFile);
	if ((p = malloc(data.nFileSizeLow + 1000)) == NULL)
		return(FALSE);
	sprintf(zip_cline, "-p %s \"%s\" \"%s\"",
		(data.nFileSizeLow < EDISP || !zip_disp) ? "--i" : "", zip_aname, zip_fname);
#if 0
	if ((*pZip)(zip_disp, zip_cline, p, data.nFileSizeLow + 1000) != 0)
	{
		free(p);
		return(FALSE);
	}
#else
	if ((*pZipExtractMem)(zip_disp, zip_cline, p,
						data.nFileSizeLow + 1, NULL, NULL, NULL) != 0)
	{
		free(p);
		return(FALSE);
	}
#endif
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
zip_putfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				zip_fname[MAX_PATH];
	char				zip_aname[MAX_PATH];
	char				zip_cline[MAX_PATH + 32];
	struct stat			st;
	char			*	p		= NULL;
	long 				perm	= -1;
	int					fd;

	if (!zip_getname(fname, ':', zip_aname, zip_fname))
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
	sprintf(zip_cline, "-d1 -gm %s \"%s\" \"%s\"",
				st.st_size < CDISP || !zip_disp ? "-n -gp0" : "", zip_aname, zip_fname);
	perm = ef_getperm(zip_aname);
	if (perm > 0)
		chmod(zip_aname, S_IREAD|S_IWRITE);
	if ((*pZipCompressMem)(zip_disp, zip_cline, p, st.st_size,
												NULL, NULL, NULL) != 0)
		goto error;
	if (perm > 0)
		chmod(zip_aname, perm);
	return(TRUE);
error:
	if (p != NULL)
		free(p);
	if (perm > 0)
		chmod(zip_aname, perm);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	unlink()/remove() emulation
 *----------------------------------------------------------------------------*/
int
zip_remove(fname)
const char	*	fname;
{
	char				zip_fname[MAX_PATH];
	char				zip_aname[MAX_PATH];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	long 				perm;

	if ((hFile = zip_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(-1);
	zip_findclose(hFile);
	zip_getname(fname, ':', zip_aname, zip_fname);
	if ((perm = ef_getperm(zip_aname)) == (-1))
		return(-1);
	chmod(zip_aname, S_IREAD|S_IWRITE);
	chmod(zip_aname, perm);
	return(0);
}

/*------------------------------------------------------------------------------
 *	chmod() emulation
 *----------------------------------------------------------------------------*/
int
zip_chmod(name, perm)
const char	*	name;
int				perm;
{
	return(0);
}

/*------------------------------------------------------------------------------
 *	fullpath() emulation
 *----------------------------------------------------------------------------*/
char *
zip_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	char				zip_fname[MAX_PATH];
	char				zip_aname[MAX_PATH];

	if (!zip_getname(fname, ':', zip_aname, zip_fname))
		return(NULL);
	if (_fullpath(buf, zip_aname, len - strlen(zip_fname)) == NULL)
		return(NULL);
	strcat(buf, ":");
	strcat(buf, zip_fname);
	return(buf);
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
DWORD
zip_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	char				zip_fname[MAX_PATH];
	char				zip_aname[MAX_PATH];
	char			*	p;

	if (!zip_getname(lpszFile, ':', zip_aname, zip_fname))
		return(0);
	if (GetFullPathName(zip_aname, cchPath, lpszPath, ppszFilePart) == 0)
		return(0);
	if ((strlen(zip_fname) + strlen(lpszPath) + 1) > cchPath)
		return(0);
	strcat(lpszPath, ":");
	*ppszFilePart = lpszPath + strlen(lpszPath);
	if (ef_dir)
	{
		if ((p = strrchr(zip_fname, '/')) != NULL)
			*ppszFilePart += p - zip_fname + 1;
	}
	strcat(lpszPath, zip_fname);
	return(strlen(lpszPath));
}

/*------------------------------------------------------------------------------
 *	findnext() emulation
 *----------------------------------------------------------------------------*/
int
zip_findnext(harc, data)
HANDLE			harc;
WIN32_FIND_DATA	* 	data;
{
	int				pos;
	int				n;
	char		*	p;
	READBUF		*	rbuf;
	READBUF		**	brbuf;

retry:
	if ((*pZipFindNext)((HARC)harc, &SubInfo) == 0)
	{
		if (!ef_dir)
		{
			if (SubInfo.szFileName[strlen(SubInfo.szFileName) - 1] == '/')
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
		if (strchr(SubInfo.szAttribute, 'R') != NULL)
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
		rbuf = zip_rbuf;
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
			if ((zip_rbuf = malloc(sizeof(READBUF))) == NULL)
				return(FALSE);
			brbuf = &zip_rbuf;
			rbuf = zip_rbuf;
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
zip_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	HARC			hArc;
	int				pos;
	int				n;
	char		*	p;

	if (!zip_getname(name, ':', find_aname, find_fname))
		return(INVALID_HANDLE_VALUE);
	zip_rbuf = NULL;
	if ((*pZipCheckArchive)(find_aname, 1) == FALSE)
		return(INVALID_HANDLE_VALUE);
	if ((hArc = (*pZipOpenArchive)(zip_disp, find_aname, M_INIT_FILE_USE)) == NULL)
		return(INVALID_HANDLE_VALUE);
	if ((*pZipFindFirst)(hArc, "*.*", &SubInfo) == 0)
	{
		if (!ef_dir)
		{
			if (SubInfo.szFileName[strlen(SubInfo.szFileName) - 1] == '/')
			{
				if (zip_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pZipCloseArchive)(hArc);
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
		if ((p = strrchr(find_fname, '/')) != NULL)
		{
			if (SubInfo.szFileName[p - find_fname + 1] == '\0')
			{
				if (zip_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pZipCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if (strnicmp(find_fname, SubInfo.szFileName, p - find_fname + 1) != 0)
			{
				if (zip_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pZipCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (zip_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pZipCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (zip_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pZipCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
			}
		}
		else
		{
			if (find_fname[0] != '/' && SubInfo.szFileName[0] == '/')
			{
				if (zip_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pZipCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (zip_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pZipCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (zip_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pZipCloseArchive)(hArc);
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
		if ((zip_rbuf = malloc(sizeof(READBUF))) == NULL)
		{
			(*pZipCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		zip_rbuf->next = NULL;
		if ((zip_rbuf->fname = malloc(strlen(data->cFileName) + 1)) == NULL)
		{
			free(zip_rbuf);
			zip_rbuf = NULL;
			(*pZipCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		strcpy(zip_rbuf->fname, data->cFileName);
		return((HANDLE)hArc);
	}
	(*pZipCloseArchive)(hArc);
	return(INVALID_HANDLE_VALUE);
}

/*------------------------------------------------------------------------------
 *	findclose() emulation
 *----------------------------------------------------------------------------*/
void
zip_findclose(harc)
HANDLE			harc;
{
	READBUF		*	rbuf;
	READBUF		*	nrbuf;

	(*pZipCloseArchive)((HARC)harc);
	rbuf = zip_rbuf;
	while (rbuf)
	{
		nrbuf = rbuf->next;
		free(rbuf->fname);
		free(rbuf);
		rbuf = nrbuf;
	}
	zip_rbuf = NULL;
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
BOOL
zip_breakcheck()
{
	return(FALSE);
}
