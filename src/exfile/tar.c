/*==============================================================================
 *	Tar/Gzip/Compress file support file
 *============================================================================*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <tar32.h>

#define  __EXFILE__C
#include "exfile.h"
#include "tar.h"

/*==============================================================================
 *	Tar file support
 *============================================================================*/
typedef	WORD	(WINAPI *pUntarGetVersion)(VOID);
typedef	BOOL	(WINAPI *pUntarCheckArchive)(LPCSTR _szFileName,
						const int _iMode);
typedef	HARC	(WINAPI *pUntarOpenArchive)(const HWND _hwnd,
						LPCSTR _szFileName, const DWORD _dwMode);
typedef	int		(WINAPI _export *pUntarCloseArchive)(HARC _harc);
typedef	int		(WINAPI *pUntar)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPSTR _szOutput, const DWORD _dwSize);
typedef	int		(WINAPI *pUntarFindFirst)(HARC _harc, LPCSTR _szWildName,
						INDIVIDUALINFO FAR *lpSubInfo);
typedef	int		(WINAPI *pUntarFindNext)(HARC _harc,
						INDIVIDUALINFO FAR *_lpSubInfo);
typedef	int		(WINAPI *pUntarExtractMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						LPBYTE _szBuffer, const DWORD _dwSize, time_t *_lpTime,
						LPWORD _lpwAttr, LPDWORD _lpdwWriteSize);
typedef	int		(WINAPI *pUntarCompressMem)(const HWND _hwnd, LPCSTR _szCmdLine,
						const LPBYTE _szBuffer, const DWORD _dwSize,
						const time_t *_lpTime, const LPWORD _lpwAttr,
						LPDWORD _lpdwWriteSize);
typedef struct readbuf {
	struct readbuf		*	next;
	char				*	fname;
} READBUF;


static	HANDLE				hTarLib				= NULL;
static	pUntarGetVersion	pTarGetVersion		= NULL;
static	pUntarCheckArchive	pTarCheckArchive	= NULL;
static	pUntarOpenArchive	pTarOpenArchive		= NULL;
static	pUntarCloseArchive	pTarCloseArchive	= NULL;
static	pUntar				pTar				= NULL;
static	pUntarFindFirst		pTarFindFirst		= NULL;
static	pUntarFindNext		pTarFindNext		= NULL;
static	pUntarExtractMem	pTarExtractMem		= NULL;
static	pUntarCompressMem	pTarCompressMem		= NULL;

static	INDIVIDUALINFO		SubInfo;
static	char				find_fname[MAX_PATH];
static	char				find_aname[MAX_PATH];
static	READBUF			*	tar_rbuf			= NULL;

static	BOOL				flg_tar_init		= FALSE;
static	BOOL				flg_gzip_init		= FALSE;
extern	BOOL				ef_dir;

static	char			*	tar_ext[]
		= {".tar", ".taZ", ".tgz", ".tar.gz", ".tar.Z", ".tar.bz2", NULL};
static	char			*	gzip_ext[]	= {".gz", ".Z", ".bz2", NULL};

static	HWND				tar_disp;
static	WORD				tar_version;
#define LIBNAME				"tar32.dll"
#define	CDISP				(16*1024)

/*==============================================================================
 *	Tar support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	Common init
 *----------------------------------------------------------------------------*/
static BOOL
tgc_init(window)
HWND			window;
{
	if (hTarLib == NULL)
	{
		tar_disp = window;
		hTarLib = LoadLibrary(LIBNAME);
		if (hTarLib == NULL)
			return(FALSE);
		pTarGetVersion
			= (pUntarGetVersion)GetProcAddress(hTarLib, "TarGetVersion");
		pTarCheckArchive
			= (pUntarCheckArchive)GetProcAddress(hTarLib, "TarCheckArchive");
		pTarOpenArchive
			= (pUntarOpenArchive)GetProcAddress(hTarLib, "TarOpenArchive");
		pTarCloseArchive
			= (pUntarCloseArchive)GetProcAddress(hTarLib, "TarCloseArchive");
		pTar	= (pUntar)GetProcAddress(hTarLib, "Tar");
		pTarFindFirst
			= (pUntarFindFirst)GetProcAddress(hTarLib, "TarFindFirst");
		pTarFindNext
			= (pUntarFindNext)GetProcAddress(hTarLib, "TarFindNext");
		pTarExtractMem
			= (pUntarExtractMem)GetProcAddress(hTarLib, "TarExtractMem");
		pTarCompressMem
			= (pUntarCompressMem)GetProcAddress(hTarLib, "TarCompressMem");
		if (pTarGetVersion && pTarCheckArchive && pTarOpenArchive
						&& pTarCloseArchive && pTar && pTarFindFirst
						&& pTarFindNext && pTarExtractMem && pTarCompressMem)
			;
		else
		{
			FreeLibrary(hTarLib);
			hTarLib = NULL;
			return(FALSE);
		}
		tar_version = (*pTarGetVersion)();
		if (tar_version < 23)
		{
			FreeLibrary(hTarLib);
			hTarLib = NULL;
			return(FALSE);
		}
	}
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	CHECK Tar file
 *----------------------------------------------------------------------------*/
static BOOL
tar_getname(name, sep, aname, fname)
const char	*	name;
char			sep;
char		*	aname;
char		*	fname;
{
	const char	*	p;
	char		**	ep	= tar_ext;
	char			tar_fname[MAX_PATH];
	char			tar_aname[MAX_PATH];

	if (hTarLib == NULL)
		return(FALSE);
	if (strlen(name) <= 5)
		return(FALSE);
	if (fname == NULL)
		fname = tar_fname;
	if (aname == NULL)
		aname = tar_aname;
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
									&& (*pTarCheckArchive)(aname, 1) == FALSE)
						return(FALSE);
				}
				else if ((*pTarCheckArchive)(aname, 1) == FALSE)
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
 *	Tar init
 *----------------------------------------------------------------------------*/
BOOL
tar_init(window)
HWND			window;
{
	if (tgc_init(window))
	{
		flg_tar_init = TRUE;
		return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	Tar term
 *----------------------------------------------------------------------------*/
VOID
tar_term()
{
	flg_tar_init = FALSE;
	if (flg_tar_init == FALSE && flg_gzip_init == FALSE)
	{
		FreeLibrary(hTarLib);
		hTarLib = NULL;
	}
}

/*------------------------------------------------------------------------------
 *	Tar term
 *----------------------------------------------------------------------------*/
char *
tar_ver()
{
	static char		ver[1024];
	int				i;

	sprintf(ver, "%s : %d.%d (",
			LIBNAME, (*pTarGetVersion)() / 100, (*pTarGetVersion)() % 100);
	for (i = 0; tar_ext[i] != NULL; i++)
	{
		if (i != 0)
			strcat(ver, "/");
		strcat(ver, tar_ext[i]);
	}
	strcat(ver, ")");
	return(ver);
}

/*------------------------------------------------------------------------------
 *	CHECK Tar file
 *----------------------------------------------------------------------------*/
BOOL
is_tar(fname, sep)
const char	*	fname;
char			sep;
{
	return(tar_getname(fname, sep, NULL, NULL));
}

/*------------------------------------------------------------------------------
 *	CHECK Tar file
 *----------------------------------------------------------------------------*/
BOOL
tar_sfx(fname)
char		*	fname;
{
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
tar_get_aname(fname, sep, aname)
const char	*	fname;
char			sep;
char		*	aname;
{
	return(tar_getname(fname, sep, aname, NULL));
}

/*------------------------------------------------------------------------------
 *	open() emulation
 *----------------------------------------------------------------------------*/
BOOL
tar_getfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				tar_fname[MAX_PATH];
	char				tar_aname[MAX_PATH];
	char				tar_cline[MAX_PATH + 32];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	int					fd;
	char			*	p;
	char			*	w;
	int					i;

	if (!tar_getname(fname, ':', tar_aname, tar_fname))
		return(FALSE);
	if ((hFile = tar_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(FALSE);
	tar_findclose(hFile);
	if ((p = malloc(data.nFileSizeLow + 1000)) == NULL)
		return(FALSE);
	sprintf(tar_cline, "--display-dialog=0 -pxvf \"%s\" \"%s\"", tar_aname, tar_fname);
	if ((*pTar)(tar_disp, tar_cline, p, data.nFileSizeLow + 1000) != 0)
	{
		free(p);
		return(FALSE);
	}
	if ((fd = open(tmp, O_WRONLY | O_CREAT | O_BINARY, S_IREAD | S_IWRITE)) < 0)
	{
		free(p);
		return(FALSE);
	}
	w = p;
	for (i = 0; i < 1000; i++)
	{
		if (*w++ == '\n')
			break;
	}
	write(fd, w, data.nFileSizeLow);
	close(fd);
	free(p);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	close emulation
 *----------------------------------------------------------------------------*/
BOOL
tar_putfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				tar_fname[MAX_PATH];
	char				tar_aname[MAX_PATH];
	char				tar_cline[MAX_PATH + 32];
	struct stat			st;
	char			*	p;
	long 				perm	= -1;
	int					fd;

	if (!tar_getname(fname, ':', tar_aname, tar_fname))
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
	sprintf(tar_cline, "-d1 -gm %s \"%s\" \"%s\"",
				st.st_size < CDISP || !tar_disp ? "-n -gp0" : "", tar_aname, tar_fname);
	perm = ef_getperm(tar_aname);
	if (perm > 0)
		chmod(tar_aname, S_IREAD|S_IWRITE);
	if ((*pTarCompressMem)(tar_disp, tar_cline, p, st.st_size,
												NULL, NULL, NULL) != 0)
		goto error;
	if (perm > 0)
		chmod(tar_aname, perm);
	return(TRUE);
error:
	if (p != NULL)
		free(p);
	if (perm > 0)
		chmod(tar_aname, perm);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	unlink()/remove() emulation
 *----------------------------------------------------------------------------*/
int
tar_remove(fname)
const char	*	fname;
{
	char				tar_fname[MAX_PATH];
	char				tar_aname[MAX_PATH];
	char				tar_cline[MAX_PATH + 32];
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	long 				perm;

	if ((hFile = tar_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
		return(-1);
	tar_findclose(hFile);
	tar_getname(fname, ':', tar_aname, tar_fname);
	if ((perm = ef_getperm(tar_aname)) == (-1))
		return(-1);
	chmod(tar_aname, S_IREAD|S_IWRITE);
	sprintf(tar_cline, "d -d1 -n -gp0 -gm \"%s\" \"%s\"", tar_aname, tar_fname);
	(*pTar)(tar_disp, tar_cline, NULL, 0);
	chmod(tar_aname, perm);
	return(0);
}

/*------------------------------------------------------------------------------
 *	chmod() emulation
 *----------------------------------------------------------------------------*/
int
tar_chmod(name, perm)
const char	*	name;
int				perm;
{
	return(0);
}

/*------------------------------------------------------------------------------
 *	fullpath() emulation
 *----------------------------------------------------------------------------*/
char *
tar_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	char				tar_fname[MAX_PATH];
	char				tar_aname[MAX_PATH];

	if (!tar_getname(fname, ':', tar_aname, tar_fname))
		return(NULL);
	if (_fullpath(buf, tar_aname, len - strlen(tar_fname)) == NULL)
		return(NULL);
	strcat(buf, ":");
	strcat(buf, tar_fname);
	return(buf);
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
DWORD
tar_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	char				tar_fname[MAX_PATH];
	char				tar_aname[MAX_PATH];
	char			*	p;

	if (!tar_getname(lpszFile, ':', tar_aname, tar_fname))
		return(0);
	if (GetFullPathName(tar_aname, cchPath, lpszPath, ppszFilePart) == 0)
		return(0);
	if ((strlen(tar_fname) + strlen(lpszPath) + 1) > cchPath)
		return(0);
	strcat(lpszPath, ":");
	*ppszFilePart = lpszPath + strlen(lpszPath);
	if (ef_dir)
	{
		if ((p = strrchr(tar_fname, '/')) != NULL)
			*ppszFilePart += p - tar_fname + 1;
	}
	strcat(lpszPath, tar_fname);
	return(strlen(lpszPath));
}

/*------------------------------------------------------------------------------
 *	findnext() emulation
 *----------------------------------------------------------------------------*/
int
tar_findnext(harc, data)
HANDLE			harc;
WIN32_FIND_DATA	* 	data;
{
	int				pos;
	int				n;
	char		*	p;
	READBUF		*	rbuf;
	READBUF		**	brbuf;

retry:
	if ((*pTarFindNext)((HARC)harc, &SubInfo) == 0)
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
		rbuf = tar_rbuf;
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
			if ((tar_rbuf = malloc(sizeof(READBUF))) == NULL)
				return(FALSE);
			brbuf = &tar_rbuf;
			rbuf = tar_rbuf;
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
tar_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	HARC			hArc;
	int				pos;
	int				n;
	char		*	p;

	if (!tar_getname(name, ':', find_aname, find_fname))
		return(INVALID_HANDLE_VALUE);
	tar_rbuf = NULL;
	if ((*pTarCheckArchive)(find_aname, 1) == FALSE)
		return(INVALID_HANDLE_VALUE);
	if ((hArc = (*pTarOpenArchive)(tar_disp, find_aname, M_INIT_FILE_USE)) == NULL)
		return(INVALID_HANDLE_VALUE);
	if ((*pTarFindFirst)(hArc, "*.*", &SubInfo) == 0)
	{
		if (!ef_dir)
		{
			if (SubInfo.szFileName[strlen(SubInfo.szFileName) - 1] == '/')
			{
				if (tar_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pTarCloseArchive)(hArc);
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
				if (tar_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pTarCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if (strnicmp(find_fname, SubInfo.szFileName, p - find_fname + 1) != 0)
			{
				if (tar_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pTarCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (tar_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pTarCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (tar_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pTarCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
			}
		}
		else
		{
			if (find_fname[0] != '/' && SubInfo.szFileName[0] == '/')
			{
				if (tar_findnext((HANDLE)hArc, data))
					return((HANDLE)hArc);
				(*pTarCloseArchive)(hArc);
				return(INVALID_HANDLE_VALUE);
			}
			if ((strchr(find_fname, '*')) == NULL)
			{
				if (strnicmp(find_fname, SubInfo.szFileName, strlen(find_fname)) != 0)
				{
					if (tar_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pTarCloseArchive)(hArc);
					return(INVALID_HANDLE_VALUE);
				}
				if (SubInfo.szFileName[strlen(find_fname)] == '/'
						|| SubInfo.szFileName[strlen(find_fname)] == '\0')
					;
				else
				{
					if (tar_findnext((HANDLE)hArc, data))
						return((HANDLE)hArc);
					(*pTarCloseArchive)(hArc);
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
		if ((tar_rbuf = malloc(sizeof(READBUF))) == NULL)
		{
			(*pTarCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		tar_rbuf->next = NULL;
		if ((tar_rbuf->fname = malloc(strlen(data->cFileName) + 1)) == NULL)
		{
			free(tar_rbuf);
			tar_rbuf = NULL;
			(*pTarCloseArchive)(hArc);
			return(INVALID_HANDLE_VALUE);
		}
		strcpy(tar_rbuf->fname, data->cFileName);
		return((HANDLE)hArc);
	}
	(*pTarCloseArchive)(hArc);
	return(INVALID_HANDLE_VALUE);
}

/*------------------------------------------------------------------------------
 *	findclose() emulation
 *----------------------------------------------------------------------------*/
void
tar_findclose(harc)
HANDLE			harc;
{
	READBUF		*	rbuf;
	READBUF		*	nrbuf;

	(*pTarCloseArchive)((HARC)harc);
	rbuf = tar_rbuf;
	while (rbuf)
	{
		nrbuf = rbuf->next;
		free(rbuf->fname);
		free(rbuf);
		rbuf = nrbuf;
	}
	tar_rbuf = NULL;
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
BOOL
tar_breakcheck()
{
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	CHECK Gzip file
 *----------------------------------------------------------------------------*/
static BOOL
gzip_getname(name, sep, aname)
const char	*	name;
char			sep;
char		*	aname;
{
	const char	*	p;
	char		**	ep	= gzip_ext;
	char			gzip_aname[MAX_PATH];

	if (hTarLib == NULL)
		return(FALSE);
	if (strlen(name) <= 3)
		return(FALSE);
	if (aname == NULL)
		aname = gzip_aname;
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
					if (strlen(p + strlen(*ep) + 1) == 0)
						return(FALSE);
					if (ef_getperm(aname) != (-1)
									&& (*pTarCheckArchive)(aname, 1) == FALSE)
						return(FALSE);
				}
				else if ((*pTarCheckArchive)(aname, 1) == FALSE)
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
 *	CHECK Gzip file
 *----------------------------------------------------------------------------*/
static void
gzip_makname(name, fname)
char		*	name;
char		*	fname;
{
	char		*	p;
	char		*	wk;
	int				i;

	if ((wk = malloc(strlen(name) + 1)) != NULL)
	{
		strcpy(wk, name);
		wk = ef_getlongname(wk);
		p = ef_gettail(wk);
		strcpy(fname, p);
		free(wk);
	}
	else
	{
		p = ef_gettail(name);
		strcpy(fname, p);
	}
	for (i = strlen(fname); i >= 0; i--)
	{
		if (fname[i] == '.')		/* delete .gz or .Z */
		{
			fname[i] = '\0';
			break;
		}
	}
}

/*------------------------------------------------------------------------------
 *	Gzip init
 *----------------------------------------------------------------------------*/
BOOL
gzip_init(window)
HWND			window;
{
	if (tgc_init(window))
	{
		flg_gzip_init = TRUE;
		return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	Gzip term
 *----------------------------------------------------------------------------*/
VOID
gzip_term()
{
	flg_gzip_init = FALSE;
	if (flg_tar_init == FALSE && flg_gzip_init == FALSE)
	{
		FreeLibrary(hTarLib);
		hTarLib = NULL;
	}
}

/*------------------------------------------------------------------------------
 *	Gzip term
 *----------------------------------------------------------------------------*/
char *
gzip_ver()
{
	static char		ver[1024];
	int				i;

	sprintf(ver, "%s : %d.%d (",
			LIBNAME, (*pTarGetVersion)() / 100, (*pTarGetVersion)() % 100);
	for (i = 0; gzip_ext[i] != NULL; i++)
	{
		if (i != 0)
			strcat(ver, "/");
		strcat(ver, gzip_ext[i]);
	}
	strcat(ver, ")");
	return(ver);
}

/*------------------------------------------------------------------------------
 *	CHECK Gzip file
 *----------------------------------------------------------------------------*/
BOOL
is_gzip(fname, sep)
const char	*	fname;
char			sep;
{
	return(gzip_getname(fname, sep, NULL));
}

/*------------------------------------------------------------------------------
 *	CHECK Gzip file
 *----------------------------------------------------------------------------*/
BOOL
gzip_sfx(fname)
char		*	fname;
{
	char				gzip_aname[MAX_PATH];

	if (gzip_getname(fname, ':', gzip_aname)
			&& strlen(gzip_aname) > 3
			&& stricmp(&gzip_aname[strlen(gzip_aname) - 3], ".gz") == 0)
		return(FALSE);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
gzip_get_aname(fname, sep, aname)
const char	*	fname;
char			sep;
char		*	aname;
{
	return(gzip_getname(fname, sep, aname, NULL));
}

/*------------------------------------------------------------------------------
 *	open() emulation
 *----------------------------------------------------------------------------*/
BOOL
gzip_getfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				gzip_aname[MAX_PATH];
	char				gzip_cline[MAX_PATH + 32];

	if (!gzip_getname(fname, ':', gzip_aname))
		return(FALSE);
	if (tar_version < 200)
	{
		sprintf(gzip_cline, "--display-dialog=0 -xvfG \"%s\" \"%s\"", gzip_aname, tmp);
		if ((*pTar)(tar_disp, gzip_cline, NULL, 0) != 0)
			return(FALSE);
	}
	else
	{
		HANDLE				hFile;
		WIN32_FIND_DATA		data;
		int					fd;
		char			*	p;
		struct stat			st;
		DWORD				i;
		DWORD				lastchar = 0;

		if ((hFile = gzip_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
			return(FALSE);
		gzip_findclose(hFile);
		sprintf(gzip_cline, "--display-dialog=0 -pxvfG \"%s\"", gzip_aname);
		if (data.nFileSizeLow == 0)
		{
			if (stat(gzip_aname, &st) == 0)
				data.nFileSizeLow = st.st_size * 100;
			if (data.nFileSizeLow > (100 * 1024 * 1024))
				data.nFileSizeLow /= 2;
			if ((p = malloc(data.nFileSizeLow + 1000)) == NULL)
				return(FALSE);
			memset(p, 0, data.nFileSizeLow + 1000);
			if ((*pTar)(tar_disp, gzip_cline, p, data.nFileSizeLow + 1000) != 0)
			{
				free(p);
				return(FALSE);
			}
			for (i = 0; i < data.nFileSizeLow; i++)
			{
				if (p[i] != '\0')
					lastchar = i;
			}
			data.nFileSizeLow = lastchar + 1;
		}
		else
		{
			if ((p = malloc(data.nFileSizeLow + 1000)) == NULL)
				return(FALSE);
			if ((*pTar)(tar_disp, gzip_cline, p, data.nFileSizeLow + 1000) != 0)
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
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	close emulation
 *----------------------------------------------------------------------------*/
BOOL
gzip_putfile(fname, tmp)
const char	*	fname;
const char	*	tmp;
{
	char				gzip_aname[MAX_PATH];
	char				gzip_cline[MAX_PATH + 32];

	if (!gzip_getname(fname, ':', gzip_aname))
		return(FALSE);
	sprintf(gzip_cline, "--display-dialog=0 -cvfGz \"%s\" \"%s\"", gzip_aname, tmp);
	if ((*pTar)(tar_disp, gzip_cline, NULL, 0) != 0)
		return(FALSE);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	unlink()/remove() emulation
 *----------------------------------------------------------------------------*/
int
gzip_remove(fname)
const char	*	fname;
{
	char				gzip_aname[MAX_PATH];

	if (!gzip_getname(fname, ':', gzip_aname))
		return(-1);
	chmod(gzip_aname, S_IREAD | S_IWRITE);
	return(unlink(gzip_aname));
}

/*------------------------------------------------------------------------------
 *	chmod() emulation
 *----------------------------------------------------------------------------*/
int
gzip_chmod(name, perm)
const char	*	name;
int				perm;
{
	char				gzip_aname[MAX_PATH];

	if (!gzip_getname(name, ':', gzip_aname))
		return(-1);
	return(chmod(gzip_aname, perm));
}

/*------------------------------------------------------------------------------
 *	fullpath() emulation
 *----------------------------------------------------------------------------*/
char *
gzip_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	char				gzip_aname[MAX_PATH];
	char				gzip_fname[MAX_PATH];

	if (!gzip_getname(fname, ':', gzip_aname))
		return(NULL);
	gzip_makname(gzip_aname, gzip_fname);
	if (_fullpath(buf, gzip_aname, len - strlen(gzip_fname)) == NULL)
		return(NULL);
	strcat(buf, ":");
	strcat(buf, gzip_fname);
	return(buf);
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
DWORD
gzip_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	char				gzip_aname[MAX_PATH];
	char				gzip_fname[MAX_PATH];

	if (!gzip_getname(lpszFile, ':', gzip_aname))
		return(0);
	gzip_makname(gzip_aname, gzip_fname);
	if (GetFullPathName(gzip_aname, cchPath, lpszPath, ppszFilePart) == 0)
		return(0);
	if ((strlen(gzip_fname) + strlen(lpszPath) + 1) > cchPath)
		return(0);
	strcat(lpszPath, ":");
	*ppszFilePart = lpszPath + strlen(lpszPath);
	strcat(lpszPath, gzip_fname);
	return(strlen(lpszPath));
}

/*------------------------------------------------------------------------------
 *	findnext() emulation
 *----------------------------------------------------------------------------*/
int
gzip_findnext(harc, data)
HANDLE			harc;
WIN32_FIND_DATA	* 	data;
{
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	findfirst() emulation
 *----------------------------------------------------------------------------*/
HANDLE
gzip_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	char				gzip_aname[MAX_PATH];
	HANDLE				hFind;

	if (!gzip_getname(name, ':', gzip_aname))
		return(INVALID_HANDLE_VALUE);
	hFind = FindFirstFile(gzip_aname, data);
	if (hFind == INVALID_HANDLE_VALUE)
		return(INVALID_HANDLE_VALUE);
	FindClose(hFind);
	memset(data->cFileName, '\0', sizeof(data->cFileName));
	gzip_makname(gzip_aname, data->cFileName);
	if (tar_version >= 200)
	{
		HARC			hArc;

		if ((*pTarCheckArchive)(gzip_aname, 1) == FALSE)
			return(INVALID_HANDLE_VALUE);
		if ((hArc = (*pTarOpenArchive)(tar_disp, gzip_aname, M_INIT_FILE_USE)) == NULL)
			return(INVALID_HANDLE_VALUE);
		if ((*pTarFindFirst)(hArc, "*.*", &SubInfo) == 0)
		{
			data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			data->nFileSizeHigh = 0;
			data->nFileSizeLow  = SubInfo.dwOriginalSize;
		}
		(*pTarCloseArchive)(hArc);
	}
	return(NULL);
}

/*------------------------------------------------------------------------------
 *	findclose() emulation
 *----------------------------------------------------------------------------*/
void
gzip_findclose(harc)
HANDLE			harc;
{
}

/*------------------------------------------------------------------------------
 *	getfullpathname() emulation
 *----------------------------------------------------------------------------*/
BOOL
gzip_breakcheck()
{
	return(FALSE);
}
