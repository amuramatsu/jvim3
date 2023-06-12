/*==============================================================================
 *	EXtended FILE support file
 *============================================================================*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <stdarg.h>

#define  __EXFILE__C
#include "exfile.h"
#include "lha.h"
#include "ftp.h"
#include "tar.h"
#include "zip.h"
#include "cab.h"

/*==============================================================================
 *	EXtended FILE support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
typedef struct {
	int				fd;						/* open dummy file descriptor	  */
	int				mode;					/* open mode					  */
	char			filename[MAX_PATH];		/* original file name			  */
	char			tempname[MAX_PATH];		/* dummy file name				  */
} TmpFile;
static TmpFile		Tempfile[16];
static char			Tempbase[MAX_PATH];

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
typedef struct {
	HANDLE			hFile;					/* open Share File handle		  */
	int				fd;						/* file descriptor				  */
	BOOL			permit;					/*								  */
	FILETIME		filetime;				/* open file time				  */
	FILETIME		checktime;				/* check file time				  */
	char			filename[MAX_PATH];		/* original file name			  */
} ShareFile;
static ShareFile	Sharefile[32];

typedef struct find_data {
	struct find_data *	next;
	WIN32_FIND_DATA		data;
} FindData;
typedef struct {
	HANDLE			hFile;					/*								  */
	FindData	*	data;					/*								  */
} TmpHandle;
static TmpHandle	TempFind[16];

static BOOL			ef_load = FALSE;
BOOL				ef_dir = TRUE;
static char			ef_version[4096];

static BOOL			ef_share = FALSE;
static BOOL			ef_common = FALSE;
static DWORD		ef_share_busy_count = 0;

static HWND			ef_window;

/*==============================================================================
 *	Shared FILE support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
ef_share_waitcursor(flg)
BOOL			flg;
{
	static HCURSOR	hWait = NULL;
	static HCURSOR	hOrig = NULL;

	if (flg)
	{
		if (hWait == NULL)
			hWait = LoadCursor(NULL, IDC_WAIT);
		if (ef_share_busy_count == 0 && hWait != NULL)
			hOrig = SetCursor(hWait);
		ef_share_busy_count++;
	}
	else if (ef_share_busy_count)
	{
		ef_share_busy_count--;
		if (ef_share_busy_count == 0)
			SetCursor(hOrig);
	}
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
ef_get_longpath(sname, lname)
char		*	sname;
char		*	lname;
{
	char		*	wk;

	_fullpath(lname, sname, MAX_PATH);
	if ((wk = malloc(strlen(lname) + 1)) != NULL)
	{
		strcpy(wk, lname);
		wk = ef_getlongname(wk);
		strcpy(lname, wk);
		free(wk);
	}
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
VOID
ef_share_init(BOOL bCommon)
{
	int						i;

	if (!ef_share)
	{
		for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
		{
			Sharefile[i].hFile			= INVALID_HANDLE_VALUE;
			Sharefile[i].fd				= -1;
			Sharefile[i].permit			= FALSE;
			Sharefile[i].filename[0]	= '\0';
		}
	}
	ef_share = TRUE;
	ef_common = bCommon;
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
VOID
ef_share_term()
{
	int						i;

	if (ef_share)
	{
		for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
		{
			if (Sharefile[i].hFile != INVALID_HANDLE_VALUE)
				CloseHandle(Sharefile[i].hFile);
			Sharefile[i].hFile = INVALID_HANDLE_VALUE;
		}
	}
	ef_share = FALSE;
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
ef_share_busy()
{
	if (!ef_share)
		return(FALSE);
	return(ef_share_busy_count);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
ef_share_check(const char *fname, int mode)
{
	SECURITY_ATTRIBUTES		security;
	HANDLE					handle;
	char					aname[MAX_PATH];
	char					target[MAX_PATH];

	if (!ef_share)
		return(FALSE);
	if (is_ftp(fname, ':'))
		return(FALSE);
	if (is_ef(fname, ':'))
	{
		if (ef_get_aname(fname, ':', aname))
			fname = aname;
	}
	ef_get_longpath(fname, target);
	security.nLength = sizeof(security);
	security.lpSecurityDescriptor = NULL;
	security.bInheritHandle = FALSE;	/* we don't hope inherit... */
	handle = CreateFile(target,
			mode & (O_WRONLY | O_RDWR) ?
									GENERIC_READ|GENERIC_WRITE : GENERIC_READ,
			mode & (O_WRONLY | O_RDWR) ? 0 : FILE_SHARE_READ,
			&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		switch (GetLastError()) {
		case ERROR_SHARING_VIOLATION:
			return(mode & (O_WRONLY | O_RDWR) ? TRUE : FALSE);
		case ERROR_ACCESS_DENIED:
			return(TRUE);
		case ERROR_FILE_NOT_FOUND:
			return(FALSE);
		}
	}
	else
		CloseHandle(handle);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
ef_share_process(const char *fname)
{
	SECURITY_ATTRIBUTES		security;
	HANDLE					handle;
	char					aname[MAX_PATH];
	char					target[MAX_PATH];
	int						i;

	if (!ef_share)
		return(FALSE);
	if (fname == NULL)
		return(FALSE);
	if (is_ftp(fname, ':'))
		return(FALSE);
	if (is_ef(fname, ':'))
	{
		if (ef_get_aname(fname, ':', aname))
			fname = aname;
	}
	ef_get_longpath(fname, target);
	for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
	{
		if (Sharefile[i].hFile != INVALID_HANDLE_VALUE
								&& stricmp(Sharefile[i].filename, target) == 0)
			return(FALSE);
	}
	security.nLength = sizeof(security);
	security.lpSecurityDescriptor = NULL;
	security.bInheritHandle = FALSE;	/* we don't hope inherit... */
	handle = CreateFile(target, GENERIC_READ, 0,
				&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		switch (GetLastError()) {
		case ERROR_SHARING_VIOLATION:
			return(TRUE);
		default:
			return(FALSE);
		}
	}
	CloseHandle(handle);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
VOID
ef_share_open(const char *fname)
{
	SECURITY_ATTRIBUTES		security;
	HANDLE					handle;
	char					aname[MAX_PATH];
	char					target[MAX_PATH];
	int						i;

	if (!ef_share)
		return;
	if (fname == NULL)
		return;
	if (is_ftp(fname, ':'))
		return;
	if (is_ef(fname, ':'))
	{
		if (ef_get_aname(fname, ':', aname))
			fname = aname;
	}
	ef_get_longpath(fname, target);
	for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
	{
		if (Sharefile[i].hFile != INVALID_HANDLE_VALUE
								&& stricmp(Sharefile[i].filename, target) == 0)
			return;
	}
	for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
	{
		if (Sharefile[i].hFile == INVALID_HANDLE_VALUE)
			break;
	}
	if (i >= (sizeof(Sharefile) / sizeof(ShareFile)))
		return;
	security.nLength = sizeof(security);
	security.lpSecurityDescriptor = NULL;
	security.bInheritHandle = FALSE;	/* we don't hope inherit... */
	handle = CreateFile(target, GENERIC_READ, 0,
				&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		switch (GetLastError()) {
		case ERROR_FILE_NOT_FOUND:
			handle = CreateFile(target, GENERIC_READ|GENERIC_WRITE, 0,
					&security, CREATE_NEW,
					FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,
					(HANDLE)NULL);
			break;
		}
	}
	else
	{
		CloseHandle(handle);
		handle = CreateFile(target, GENERIC_READ, FILE_SHARE_READ,
				&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	}
	if (ef_common)
	{
		Sharefile[i].hFile = INVALID_HANDLE_VALUE;
		ZeroMemory(&Sharefile[i].filetime, sizeof(Sharefile[i].filetime));
		ZeroMemory(&Sharefile[i].checktime, sizeof(Sharefile[i].checktime));
		if (handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
		handle = CreateFile(target,
					GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, 0, 0);
		if (handle != INVALID_HANDLE_VALUE)
		{
			GetFileTime(handle, NULL, NULL, &Sharefile[i].filetime);
			GetFileTime(handle, NULL, NULL, &Sharefile[i].checktime);
			CloseHandle(handle);
		}
	}
	else
		Sharefile[i].hFile = handle;
	strcpy(Sharefile[i].filename, target);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
VOID
ef_share_close(const char *fname)
{
	char					aname[MAX_PATH];
	char					target[MAX_PATH];
	int						i;

	if (!ef_share)
		return;
	if (fname == NULL)
		return;
	if (is_ftp(fname, ':'))
		return;
	if (is_ef(fname, ':'))
	{
		if (ef_get_aname(fname, ':', aname))
			fname = aname;
	}
	ef_get_longpath(fname, target);
	for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
	{
		if (stricmp(Sharefile[i].filename, target) == 0)
		{
			if (Sharefile[i].hFile != INVALID_HANDLE_VALUE)
				CloseHandle(Sharefile[i].hFile);
			Sharefile[i].hFile			= INVALID_HANDLE_VALUE;
			Sharefile[i].fd				= -1;
			Sharefile[i].permit			= FALSE;
			Sharefile[i].filename[0]	= '\0';
			return;
		}
	}
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static int
ef_share_opened(const char *fname)
{
	char					aname[MAX_PATH];
	char					target[MAX_PATH];
	int						i;

	if (!ef_share)
		return(-1);
	if (is_ftp(fname, ':'))
		return(-1);
	if (is_ef(fname, ':'))
	{
		if (ef_get_aname(fname, ':', aname))
			fname = aname;
	}
	ef_get_longpath(fname, target);
	for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
	{
		if (Sharefile[i].permit != TRUE
								&& stricmp(Sharefile[i].filename, target) == 0)
		{
			if (!ef_common && Sharefile[i].hFile != INVALID_HANDLE_VALUE)
				CloseHandle(Sharefile[i].hFile);
			Sharefile[i].permit	= TRUE;
			ef_share_waitcursor(TRUE);
			return(i);
		}
	}
	return(-1);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static VOID
ef_share_permit(int array, int fd)
{
	if (!ef_share)
		return;
	if (array < 0)
		return;
	if (fd < 0)
		return;
	Sharefile[array].fd = fd;
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static VOID
ef_share_prohibit(int array, int fd)
{
	SECURITY_ATTRIBUTES		security;
	HANDLE					handle;
	int						i;
	BOOL					bClose	= FALSE;

	if (!ef_share)
		return;
	if (array < 0 && fd < 0)
		return;
	if (array >= 0 && fd >= 0)
		return;
	if (array == (-1) && fd >= 0)
		bClose = TRUE;
	if (fd >= 0)
	{
		for (i = 0; i < (sizeof(Sharefile) / sizeof(ShareFile)); i++)
		{
			if (Sharefile[i].fd == fd)
			{
				array = i;
				break;
			}
		}
	}
	if (array < 0)
		return;
	if (!Sharefile[array].permit)
		return;
	security.nLength = sizeof(security);
	security.lpSecurityDescriptor = NULL;
	security.bInheritHandle = FALSE;
	handle = CreateFile(Sharefile[array].filename,
				GENERIC_READ, FILE_SHARE_READ, &security,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		switch (GetLastError()) {
		case ERROR_FILE_NOT_FOUND:
			handle = CreateFile(Sharefile[array].filename,
						GENERIC_READ|GENERIC_WRITE, 0,
						&security, CREATE_NEW,
						FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,
						(HANDLE)NULL);
			break;
		}
	}
	Sharefile[array].fd		= -1;
	Sharefile[array].permit	= FALSE;
	if (ef_common)
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			GetFileTime(handle, NULL, NULL, &Sharefile[array].checktime);
			if (bClose)
				GetFileTime(handle, NULL, NULL, &Sharefile[array].filetime);
			CloseHandle(handle);
		}
		Sharefile[array].hFile	= INVALID_HANDLE_VALUE;
	}
	else
		Sharefile[array].hFile	= handle;
	ef_share_waitcursor(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static BOOL
ef_share_change(int array)
{
	HANDLE			hFile;
	FILETIME		nowFile;

	if (!ef_common)
		return(FALSE);
	if (array < 0)
		return(FALSE);
	hFile = CreateFile(Sharefile[array].filename,
					GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return(FALSE);
	GetFileTime(hFile, NULL, NULL, &nowFile);
	CloseHandle(hFile);
	if (CompareFileTime(&Sharefile[array].filetime, &nowFile) < 0)
	{
		CopyMemory(&Sharefile[array].checktime, &nowFile, sizeof(nowFile));
		if (MessageBox(ef_window, "Write since last change. Write file ?",
										"", MB_YESNO|MB_DEFBUTTON1) == IDYES)
		{
			CopyMemory(&Sharefile[array].filetime, &nowFile, sizeof(nowFile));
			return(FALSE);
		}
		return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
ef_share_replace(const char * fname)
{
	HANDLE					hFile;
	FILETIME				nowFile;
	char					aname[MAX_PATH];
	char					target[MAX_PATH];
	int						array;

	if (!ef_share)
		return(FALSE);
	if (!ef_common)
		return(FALSE);
	if (fname == NULL)
		return(FALSE);
	if (is_ftp(fname, ':'))
		return(FALSE);
	if (is_ef(fname, ':'))
	{
		if (ef_get_aname(fname, ':', aname))
			fname = aname;
	}
	ef_get_longpath(fname, target);
	for (array = 0; array < (sizeof(Sharefile) / sizeof(ShareFile)); array++)
	{
		if (Sharefile[array].permit != TRUE
							&& stricmp(Sharefile[array].filename, target) == 0)
		{
			hFile = CreateFile(Sharefile[array].filename,
							GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING, 0, 0);
			if (hFile == INVALID_HANDLE_VALUE)
				return(FALSE);
			GetFileTime(hFile, NULL, NULL, &nowFile);
			CloseHandle(hFile);
			if (CompareFileTime(&Sharefile[array].checktime, &nowFile) < 0)
			{
				CopyMemory(&Sharefile[array].checktime, &nowFile, sizeof(nowFile));
				if (MessageBox(ef_window,
						"Write since last change. Reload file ?",
						"", MB_YESNO|MB_DEFBUTTON1) == IDYES)
					return(TRUE);
			}
			break;
		}
	}
	return(FALSE);
}

/*==============================================================================
 *	EXtended FILE support routine
 *============================================================================*/
/*------------------------------------------------------------------------------
 *	EXtended FILE get permission
 *----------------------------------------------------------------------------*/
BOOL
ef_iskanji(c)
unsigned char	c;
{
	return((0x81 <= c && c <=0x9F) || (0xE0 <= c && c <= 0xFC));
}

/*------------------------------------------------------------------------------
 *	EXtended FILE get permission
 *----------------------------------------------------------------------------*/
BOOL
ef_pathsep(c)
unsigned char	c;
{
	return(c == '\\' || c == '/');
}

/*------------------------------------------------------------------------------
 *	Get last file name
 *----------------------------------------------------------------------------*/
void
ef_slash(str)
char			*	str;
{
	while (*str)
	{
		if (ef_iskanji(*str))
			str += 2;
		else if (*str == '\\')
			*str++ = '/';
		else
			str++;
	}
}

/*------------------------------------------------------------------------------
 *	EXtended FILE get permission
 *----------------------------------------------------------------------------*/
long
ef_getperm(name)
char		*	name;
{
	struct stat		statb;
	long			r;

	if (stat(name, &statb))
		return -1;
	r = statb.st_mode & 0x7fffffff;
	return r;
}

/*------------------------------------------------------------------------------
 *	Get last file name
 *----------------------------------------------------------------------------*/
char *
ef_gettail(fname)
char			*	fname;
{
	char			*	p1;
	char			*	p2;

	for (p1 = p2 = fname; *p2; ++p2)	/* find last part of path */
	{
		if (ef_iskanji((unsigned char)(*p2)))
			++p2;
		else if (*p2 == ':' || *p2 == '\\' || *p2 == '/')
			p1 = p2 + 1;
	}
	return p1;
}

/*------------------------------------------------------------------------------
 *	EXtended FILE Expand Alias File Name to Long File Name
 *----------------------------------------------------------------------------*/
char *
ef_getlongname(fname)
char			*	fname;
{
	WIN32_FIND_DATA 	fb;
	HANDLE          	hFind;
	char			*	tname;
	char			*	wk;
	char				t;
	char				buf[MAX_PATH * 2];
	char				wbuf[MAX_PATH * 2];
	int					pos = 3;
	BOOL				alias;

	if (strlen(fname) < 5
			|| strchr(&fname[3], '~') == NULL
			|| !(isalpha(fname[0]) && fname[1] == ':' && fname[2] == '\\'))
		return(fname);
retry:
	alias = FALSE;
	tname = fname + pos;
	while (*tname)
	{
		if (tname[0] == '~' && ('0' <= tname[1] && tname[1] <= '9'))
		{
			tname += 2;
			alias = TRUE;
			while (*tname)
			{
				if (*tname == '\\' || *tname == ':')
					break;
				else if ('0' <= *tname && *tname <= '9')
					;
				else if (*tname == '.')
				{
					t = 0;
					tname++;
					while (*tname)
					{
						if (*tname == '\\' || *tname == ':')
							break;
						tname++;
						t++;
						if (t > 3)
						{
							alias = FALSE;
							break;
						}
					}
					break;
				}
				else
				{
					alias = FALSE;
					break;
				}
				tname++;
			}
			break;
		}
		tname++;
	}
	if (alias != TRUE)
		return(fname);
	strcpy(buf, fname);
	pos = tname - fname;
	t = buf[pos];
	buf[pos] = '\0';
	if (GetFullPathName(buf, sizeof(wbuf), wbuf, &tname) == 0)
		return(fname);
	if ((hFind = FindFirstFile(wbuf, &fb)) != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		strcpy(tname, fb.cFileName);
		if (t)
		{
			if (t == ':')
				strcat(wbuf, ":");
			else
				strcat(wbuf, "\\");
			strcat(wbuf, &buf[pos + 1]);
		}
		if ((wk = malloc(strlen(wbuf) + 1)) == NULL)
			return(fname);
		strcpy(wk, wbuf);
		free(fname);
		fname = wk;
		if (t != '\0' && t != ':')
			goto retry;
	}
	return(fname);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE is directory ?
 *----------------------------------------------------------------------------*/
static BOOL
ef_isdir(name)
char		*	name;
{
	struct stat		statb;

	if (name[strlen(name) - 1] == '/' || name[strlen(name) - 1] == '\\')
		return(TRUE);
	if (ef_stat(name, &statb) == 0 && (statb.st_mode & S_IFDIR))
		return(TRUE);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE use wild charactor ?
 *----------------------------------------------------------------------------*/
static BOOL
ef_has_wildcard(s)
char		*	s;
{
	if (*s == '~' || *s == '$')
		return(TRUE);
	for (; *s; ++s)
	{
		if ((0x81 <= (unsigned char)(*s) && (unsigned char)(*s) <= 0x9F)
				|| (0xE0 <= (unsigned char)(*s) && (unsigned char)(*s) <= 0xFC))
			++s;
		else if (*s == '[' || *s == '{')
			return(TRUE);
		else if (*s == '?' || *s == '*')
			return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE Message Loop Active
 *----------------------------------------------------------------------------*/
void
ef_loop()
{
	MSG				msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

/*------------------------------------------------------------------------------
 *	EXtended FILE check readonly archive
 *----------------------------------------------------------------------------*/
static BOOL
ef_sfx(fname)
char		*	fname;
{
	if (is_lha(fname, ':'))
		return(lha_sfx(fname));
	if (is_ftp(fname, ':'))
		return(ftp_sfx(fname));
	if (is_tar(fname, ':'))
		return(tar_sfx(fname));
	if (is_gzip(fname, ':'))
		return(gzip_sfx(fname));
	if (is_zip(fname, ':'))
		return(zip_sfx(fname));
	if (is_cab(fname, ':'))
		return(cab_sfx(fname));
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
BOOL
ef_get_aname(fname, sep, aname)
const char	*	fname;
char			sep;
char		*	aname;
{
	if (!ef_load)
		return(FALSE);
	if (lha_get_aname(fname, sep, aname))
		return(TRUE);
	if (tar_get_aname(fname, sep, aname))
		return(TRUE);
	if (gzip_get_aname(fname, sep, aname))
		return(TRUE);
	if (zip_get_aname(fname, sep, aname))
		return(TRUE);
	if (cab_get_aname(fname, sep, aname))
		return(TRUE);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE initialize
 *----------------------------------------------------------------------------*/
VOID
ef_init(window)
HWND			window;
{
	UINT			w;
	char		*	p;
	INT				i;
#define VER_LEFT	12

	if (ef_load == FALSE)
	{
		for (w = 0; w < (sizeof(Tempfile) / sizeof(TmpFile)); w++)
			Tempfile[w].fd = -1;
		for (w = 0; w < (sizeof(TempFind) / sizeof(TmpHandle)); w++)
			TempFind[w].hFile = INVALID_HANDLE_VALUE;
		Tempbase[0] = '\0';
		if ((p = getenv("TMP")) == NULL)
			p = getenv("TEMP");
		if (p != NULL)
		{
			strcpy(Tempbase, p);
			w = strlen(Tempbase) - 1;
			if (Tempbase[w] == '\\' || Tempbase[w] == '/')
				;
			else
				strcat(Tempbase, "\\");
		}
		strcat(Tempbase, "efXXXXXX");
		w = SetErrorMode(SEM_NOOPENFILEERRORBOX);
		ef_version[0] = '\0';
		ef_loop();
		if (lha_init(window))
		{
			for (i = 0; i < VER_LEFT; i++)
				strcat(ef_version, " ");
			sprintf(&ef_version[strlen(ef_version)], "%s\r\n", lha_ver());
		}
		ef_loop();
		if (ftp_init(window))
		{
			for (i = 0; i < VER_LEFT; i++)
				strcat(ef_version, " ");
			sprintf(&ef_version[strlen(ef_version)], "%s\r\n", ftp_ver());
		}
		ef_loop();
		if (tar_init(window))
		{
			for (i = 0; i < VER_LEFT; i++)
				strcat(ef_version, " ");
			sprintf(&ef_version[strlen(ef_version)], "%s\r\n", tar_ver());
		}
		ef_loop();
		if (gzip_init(window))
		{
			for (i = 0; i < VER_LEFT; i++)
				strcat(ef_version, " ");
			sprintf(&ef_version[strlen(ef_version)], "%s\r\n", gzip_ver());
		}
		ef_loop();
		if (zip_init(window))
		{
			for (i = 0; i < VER_LEFT; i++)
				strcat(ef_version, " ");
			sprintf(&ef_version[strlen(ef_version)], "%s\r\n", zip_ver());
		}
		ef_loop();
		if (cab_init(window))
		{
			for (i = 0; i < VER_LEFT; i++)
				strcat(ef_version, " ");
			sprintf(&ef_version[strlen(ef_version)], "%s", cab_ver());
		}
		ef_loop();
		SetErrorMode(w);
		ef_window = window;
		ef_load = TRUE;
	}
}

/*------------------------------------------------------------------------------
 *	EXtended FILE initialize
 *----------------------------------------------------------------------------*/
VOID
ef_term()
{
	int					i;

	if (ef_load)
	{
		lha_term();
		ftp_term();
		tar_term();
		gzip_term();
		zip_term();
		cab_term();
		for (i = 0; i < (sizeof(Tempfile) / sizeof(TmpFile)); i++)
		{
			if (Tempfile[i].fd != -1)
			{
				chmod(Tempfile[i].tempname, S_IREAD | S_IWRITE);
				unlink(Tempfile[i].tempname);
				Tempfile[i].fd = -1;
			}
		}
	}
	ef_load = FALSE;
}

/*------------------------------------------------------------------------------
 *	EXtended FILE initialize
 *----------------------------------------------------------------------------*/
char *
ef_ver()
{
	return(ef_version);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE check
 *----------------------------------------------------------------------------*/
BOOL
is_ef(fname, sep)
const char	*	fname;
char			sep;
{
	if (!ef_load)
		return(FALSE);
	if (is_lha(fname, sep))
		return(TRUE);
	if (is_ftp(fname, sep))
		return(TRUE);
	if (is_tar(fname, sep))
		return(TRUE);
	if (is_gzip(fname, sep))
		return(TRUE);
	if (is_zip(fname, sep))
		return(TRUE);
	if (is_cab(fname, sep))
		return(TRUE);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	EXtended Archive check
 *----------------------------------------------------------------------------*/
BOOL
is_efarc(fname, sep)
const char	*	fname;
char			sep;
{
	if (!ef_load)
		return(FALSE);
	if (is_lha(fname, sep))
		return(TRUE);
	if (is_tar(fname, sep))
		return(TRUE);
	if (is_gzip(fname, sep))
		return(TRUE);
	if (is_zip(fname, sep))
		return(TRUE);
	if (is_cab(fname, sep))
		return(TRUE);
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE open
 *----------------------------------------------------------------------------*/
int
ef_open(const char *fname, int mode, ...)
{
	va_list				ap;
	WIN32_FIND_DATA		data;
	BOOL				newfile	= TRUE;
	int					i;
	int					rtn = 0;
	TmpFile			*	tmp	= NULL;
	HANDLE				hFile;
	int					pmode;
	int					share;

	va_start(ap, mode);
	pmode = va_arg(ap, int);
	va_end(ap);
	share = ef_share_opened(fname);
	if (ef_share_check(fname, mode))
		rtn = (-1);
	else if (ef_share_change(share))
		rtn = (-1);
	else if (!ef_load || !is_ef(fname, ':'))
		rtn = open(fname, mode, pmode);
	else if (ef_isdir(fname))
		rtn = (-1);
	else if (strchr(fname, '*') != NULL || strchr(fname, '?') != NULL)
		rtn = (-1);
	else if ((hFile = ef_findfirst(fname, &data)) == INVALID_HANDLE_VALUE)
	{
		if ((mode & (O_WRONLY | O_CREAT)) && !ef_sfx(fname))
			goto newarch;
		rtn = (-1);
	}
	else
	{
		ef_findclose(hFile);
		if (((data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) || ef_sfx(fname))
												&& (mode & (O_WRONLY | O_RDWR)))
			rtn = (-1);
		else
		{
			newfile = FALSE;
newarch:
			for (i = 0; i < (sizeof(Tempfile) / sizeof(TmpFile)); i++)
			{
				if (Tempfile[i].fd == (-1))
				{
					strcpy(Tempfile[i].tempname, Tempbase);
					if (mktemp(Tempfile[i].tempname) == NULL)
						rtn = (-1);
					else
					{
						strcpy(Tempfile[i].filename, fname);
						tmp = &Tempfile[i];
					}
					break;
				}
			}
			if (tmp == NULL)
				rtn = (-1);
			else if (rtn != 0 || newfile)
				;
			else
			{
				if (is_lha(fname, ':'))
					rtn = lha_getfile(fname, tmp->tempname);
				else if (is_ftp(fname, ':'))
					rtn = ftp_getfile(fname, tmp->tempname);
				else if (is_tar(fname, ':'))
					rtn = tar_getfile(fname, tmp->tempname);
				else if (is_gzip(fname, ':'))
					rtn = gzip_getfile(fname, tmp->tempname);
				else if (is_zip(fname, ':'))
					rtn = zip_getfile(fname, tmp->tempname);
				else if (is_cab(fname, ':'))
					rtn = cab_getfile(fname, tmp->tempname);
				chmod(tmp->tempname, S_IREAD | S_IWRITE);
				if (!rtn)
				{
					unlink(tmp->tempname);
					rtn = (-1);
				}
				else
				{
					if ((rtn = open(tmp->tempname, mode, S_IREAD|S_IWRITE)) < 0)
					{
						unlink(tmp->tempname);
						rtn = (-1);
					}
					else
					{
						tmp->fd = rtn;
						tmp->mode = mode;
					}
				}
			}
		}
	}
	if (share >= 0)
	{
		if (rtn >= 0)
			ef_share_permit(share, rtn);
		else
			ef_share_prohibit(share, -1);
	}
	return(rtn);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE close
 *----------------------------------------------------------------------------*/
int
ef_close(fd)
int				fd;
{
	int					i;
	int					rtn;
	BOOL				change = FALSE;

	rtn = close(fd);
	if (!ef_load)
		;
	else if (rtn != 0)
		rtn = (-1);
	else
	{
		for (i = 0; i < (sizeof(Tempfile) / sizeof(TmpFile)); i++)
		{
			if (Tempfile[i].fd == fd)
			{
				if (Tempfile[i].mode & (O_WRONLY | O_CREAT))
					change = TRUE;
				if (change)
				{
					if (is_lha(Tempfile[i].filename, ':'))
					{
						if (!lha_putfile(Tempfile[i].filename,
														Tempfile[i].tempname))
							rtn = (-1);
					}
					else if (is_ftp(Tempfile[i].filename, ':'))
					{
						if (!ftp_putfile(Tempfile[i].filename,
														Tempfile[i].tempname))
							rtn = (-1);
					}
					else if (is_tar(Tempfile[i].filename, ':'))
					{
						if (!tar_putfile(Tempfile[i].filename,
														Tempfile[i].tempname))
							rtn = (-1);
					}
					else if (is_gzip(Tempfile[i].filename, ':'))
					{
						if (!gzip_putfile(Tempfile[i].filename,
														Tempfile[i].tempname))
							rtn = (-1);
					}
					else if (is_zip(Tempfile[i].filename, ':'))
					{
						if (!zip_putfile(Tempfile[i].filename,
														Tempfile[i].tempname))
							rtn = (-1);
					}
					else if (is_cab(Tempfile[i].filename, ':'))
					{
						if (!cab_putfile(Tempfile[i].filename,
														Tempfile[i].tempname))
							rtn = (-1);
					}
				}
				chmod(Tempfile[i].tempname, S_IREAD | S_IWRITE);
				unlink(Tempfile[i].tempname);
				Tempfile[i].fd = -1;
				break;
			}
		}
	}
	ef_share_prohibit(-1, fd);
	return(rtn);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE unlink()/remove()
 *----------------------------------------------------------------------------*/
int
ef_remove(fname)
const char	*	fname;
{
	int					rtn = -1;
	int					share;

	share = ef_share_opened(fname);
	if (!ef_load || !is_ef(fname, ':'))
	{
		chmod(fname, S_IREAD | S_IWRITE);
		rtn = unlink(fname);
	}
	else if (ef_sfx(fname))
		rtn = -1;
	else if (is_lha(fname, ':'))
		rtn = lha_remove(fname);
	else if (is_ftp(fname, ':'))
		rtn = ftp_remove(fname);
	else if (is_tar(fname, ':'))
		rtn = tar_remove(fname);
	else if (is_gzip(fname, ':'))
		rtn = gzip_remove(fname);
	else if (is_zip(fname, ':'))
		rtn = zip_remove(fname);
	else if (is_cab(fname, ':'))
		rtn = cab_remove(fname);
	ef_share_prohibit(share, -1);
	return(rtn);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE rename()
 *----------------------------------------------------------------------------*/
int
ef_rename(old, new)
const char	*	old;
const char	*	new;
{
	int					rtn = 0;
	char				tmpname[MAX_PATH];
	int					oshare;
	int					nshare;

	oshare = ef_share_opened(old);
	nshare = ef_share_opened(new);
	if (ef_share_change(oshare))
		rtn = (-1);
	else if (!ef_load || (!is_ef(old, ':') && !is_ef(new, ':')))
		rtn = rename(old, new);
	else if (ef_sfx(old) || ef_sfx(new))
		rtn = (-1);
	else
	{
		if (is_ef(old, ':'))
		{
			strcpy(tmpname, Tempbase);
			if (mktemp(tmpname) == NULL)
				rtn = (-1);
			else if (is_lha(old, ':'))
			{
				if (!lha_getfile(old, tmpname))
					rtn = (-1);
			}
			else if (is_ftp(old, ':'))
			{
				if (!ftp_getfile(old, tmpname))
					rtn = (-1);
			}
			else if (is_tar(old, ':'))
			{
				if (!tar_getfile(old, tmpname))
					rtn = (-1);
			}
			else if (is_gzip(old, ':'))
			{
				if (!gzip_getfile(old, tmpname))
					rtn = (-1);
			}
			else if (is_zip(old, ':'))
			{
				if (!zip_getfile(old, tmpname))
					rtn = (-1);
			}
			else if (is_cab(old, ':'))
			{
				if (!cab_getfile(old, tmpname))
					rtn = (-1);
			}
		}
		else
			strcpy(tmpname, old);
		if (rtn == 0)
		{
			if (is_ef(new, ':'))
			{
				if (is_lha(new, ':'))
					rtn = lha_putfile(new, tmpname);
				else if (is_ftp(new, ':'))
					rtn = ftp_putfile(new, tmpname);
				else if (is_tar(new, ':'))
					rtn = tar_putfile(new, tmpname);
				else if (is_gzip(new, ':'))
					rtn = gzip_putfile(new, tmpname);
				else if (is_zip(new, ':'))
					rtn = zip_putfile(new, tmpname);
				else if (is_cab(new, ':'))
					rtn = cab_putfile(new, tmpname);
				if (rtn)
					rtn = 0;
				else
					rtn = -1;
			}
			else
				rtn = rename(tmpname, new);
			if (rtn != 0)
			{
				if (is_ef(old, ':'))
					ef_remove(old);
				rtn = (-1);
			}
		}
	}
	ef_share_prohibit(nshare, -1);
	ef_share_prohibit(oshare, -1);
	return(rtn);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE stat()
 *----------------------------------------------------------------------------*/
long
ef_stat(name, st)
char		*	name;
struct stat	*	st;
{
	HANDLE				hFile;
	WIN32_FIND_DATA		data;
	unsigned short		r	= (-1);
	long				rtn = 0;
	int					share;
	char				aname[MAX_PATH];

	share = ef_share_opened(name);
	if (!ef_load || !is_ef(name, ':'))
		rtn = stat(name, st);
	else if (name[strlen(name) - 1] == '/' || name[strlen(name) - 1] == '\\')
	{
		st->st_mode = S_IREAD | S_IWRITE | S_IFDIR;
		rtn = (0);
	}
	else if ((hFile = ef_findfirst(name, &data)) == INVALID_HANDLE_VALUE)
		rtn = (-1);
	else
	{
		ef_findclose(hFile);
		if (ef_get_aname(name, ':', aname))
			stat(aname, st);
		r = S_IREAD|S_IWRITE;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			r &= ~S_IWRITE;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
			;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			r |= S_IFDIR;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
			;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)
			;
		st->st_mode = r;
	}
	ef_share_prohibit(share, -1);
	return(rtn);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE chmod()
 *----------------------------------------------------------------------------*/
int
ef_chmod(name, perm)
const char	*	name;
int				perm;
{
	int					rtn = -1;
	int					share;

	share = ef_share_opened(name);
	if (!ef_load || !is_ef(name, ':'))
		rtn = chmod(name, perm);
	else if (ef_sfx(name))
		rtn = -1;
	else if (is_lha(name, ':'))
		rtn = lha_chmod(name, perm);
	else if (is_ftp(name, ':'))
		rtn = ftp_chmod(name, perm);
	else if (is_tar(name, ':'))
		rtn = tar_chmod(name, perm);
	else if (is_gzip(name, ':'))
		rtn = gzip_chmod(name, perm);
	else if (is_zip(name, ':'))
		rtn = zip_chmod(name, perm);
	else if (is_cab(name, ':'))
		rtn = cab_chmod(name, perm);
	ef_share_prohibit(share, -1);
	return(rtn);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE fullpath()
 *----------------------------------------------------------------------------*/
char *
ef_fullpath(buf, fname, len)
char		*	buf;
char		*	fname;
int				len;
{
	if (fname == NULL)
		return(NULL);
	if (!ef_load || !is_ef(fname, ':'))
		return(_fullpath(buf, fname, len));
	if (ef_isdir(fname))
		return(NULL);
	if (is_lha(fname, ':'))
		return(lha_fullpath(buf, fname, len));
	if (is_ftp(fname, ':'))
		return(ftp_fullpath(buf, fname, len));
	if (is_tar(fname, ':'))
		return(tar_fullpath(buf, fname, len));
	if (is_gzip(fname, ':'))
		return(gzip_fullpath(buf, fname, len));
	if (is_zip(fname, ':'))
		return(zip_fullpath(buf, fname, len));
	if (is_cab(fname, ':'))
		return(cab_fullpath(buf, fname, len));
	return(NULL);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE getfullpathname()
 *----------------------------------------------------------------------------*/
DWORD
ef_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart)
LPCTSTR			lpszFile;
DWORD			cchPath;
LPTSTR			lpszPath;
LPTSTR		*	ppszFilePart;
{
	if (!ef_load || !is_ef(lpszFile, ':'))
		return(GetFullPathName(lpszFile, cchPath, lpszPath, ppszFilePart));
	if (ef_isdir(lpszFile))
		return(0);
	if (is_lha(lpszFile, ':'))
		return(lha_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart));
	if (is_ftp(lpszFile, ':'))
		return(ftp_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart));
	if (is_tar(lpszFile, ':'))
		return(tar_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart));
	if (is_gzip(lpszFile, ':'))
		return(gzip_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart));
	if (is_zip(lpszFile, ':'))
		return(zip_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart));
	if (is_cab(lpszFile, ':'))
		return(cab_getfullpathname(lpszFile, cchPath, lpszPath, ppszFilePart));
	return(0);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE findnext()
 *----------------------------------------------------------------------------*/
int
ef_findnext(hFile, data)
HANDLE				hFile;
WIN32_FIND_DATA	* 	data;
{
	FindData	*	p;
	int				i;

	for (i = 0; i < (sizeof(TempFind) / sizeof(TmpHandle)); i++)
	{
		if ((HANDLE)&TempFind[i] == hFile)
		{
			if (TempFind[i].data != NULL)
			{
				p = TempFind[i].data->next;
				memmove(data, &TempFind[i].data->data, sizeof(WIN32_FIND_DATA));
				free(TempFind[i].data);
				TempFind[i].data = p;
				return(TRUE);
			}
			return(FALSE);
		}
	}
	return(FindNextFile(hFile, data));
}

/*------------------------------------------------------------------------------
 *	EXtended FILE findfirst()
 *----------------------------------------------------------------------------*/
HANDLE
ef_findfirst(name, data)
const char		*	name;
WIN32_FIND_DATA	* 	data;
{
	int				i;
	TmpHandle	*	tmp = NULL;
	FindData	*	p;
	FindData	**	datap;

	if (!ef_load || !is_ef(name, ':'))
		return(FindFirstFile(name, data));
	for (i = 0; i < (sizeof(TempFind) / sizeof(TmpHandle)); i++)
	{
		if (TempFind[i].hFile == INVALID_HANDLE_VALUE)
		{
			tmp = &TempFind[i];
			break;
		}
	}
	if (tmp == NULL)
		return(INVALID_HANDLE_VALUE);
	tmp->data = NULL;
	datap = &tmp->data;
	if (is_lha(name, ':'))
	{
		if ((tmp->hFile = lha_findfirst(name, data)) != INVALID_HANDLE_VALUE)
		{
			while ((p = malloc(sizeof(FindData))) != NULL)
			{
				if (lha_findnext(tmp->hFile, &p->data))
				{
					*datap = p;
					p->next = NULL;
					datap = &p->next;
				}
				else
				{
					*datap = NULL;
					free(p);
					break;
				}
			}
			lha_findclose(tmp->hFile);
		}
		else
			return(INVALID_HANDLE_VALUE);
	}
	else if (is_ftp(name, ':'))
	{
		if ((tmp->hFile = ftp_findfirst(name, data)) != INVALID_HANDLE_VALUE)
		{
			while ((p = malloc(sizeof(FindData))) != NULL)
			{
				if (ftp_findnext(tmp->hFile, &p->data))
				{
					*datap = p;
					p->next = NULL;
					datap = &p->next;
				}
				else
				{
					*datap = NULL;
					free(p);
					break;
				}
			}
			ftp_findclose(tmp->hFile);
		}
		else
			return(INVALID_HANDLE_VALUE);
	}
	else if (is_tar(name, ':'))
	{
		if ((tmp->hFile = tar_findfirst(name, data)) != INVALID_HANDLE_VALUE)
		{
			while ((p = malloc(sizeof(FindData))) != NULL)
			{
				if (tar_findnext(tmp->hFile, &p->data))
				{
					*datap = p;
					p->next = NULL;
					datap = &p->next;
				}
				else
				{
					*datap = NULL;
					free(p);
					break;
				}
			}
			tar_findclose(tmp->hFile);
		}
		else
			return(INVALID_HANDLE_VALUE);
	}
	else if (is_gzip(name, ':'))
	{
		if ((tmp->hFile = gzip_findfirst(name, data)) != INVALID_HANDLE_VALUE)
		{
			while ((p = malloc(sizeof(FindData))) != NULL)
			{
				if (gzip_findnext(tmp->hFile, &p->data))
				{
					*datap = p;
					p->next = NULL;
					datap = &p->next;
				}
				else
				{
					*datap = NULL;
					free(p);
					break;
				}
			}
			gzip_findclose(tmp->hFile);
		}
		else
			return(INVALID_HANDLE_VALUE);
	}
	else if (is_zip(name, ':'))
	{
		if ((tmp->hFile = zip_findfirst(name, data)) != INVALID_HANDLE_VALUE)
		{
			while ((p = malloc(sizeof(FindData))) != NULL)
			{
				if (zip_findnext(tmp->hFile, &p->data))
				{
					*datap = p;
					p->next = NULL;
					datap = &p->next;
				}
				else
				{
					*datap = NULL;
					free(p);
					break;
				}
			}
			zip_findclose(tmp->hFile);
		}
		else
			return(INVALID_HANDLE_VALUE);
	}
	else if (is_cab(name, ':'))
	{
		if ((tmp->hFile = cab_findfirst(name, data)) != INVALID_HANDLE_VALUE)
		{
			while ((p = malloc(sizeof(FindData))) != NULL)
			{
				if (cab_findnext(tmp->hFile, &p->data))
				{
					*datap = p;
					p->next = NULL;
					datap = &p->next;
				}
				else
				{
					*datap = NULL;
					free(p);
					break;
				}
			}
			cab_findclose(tmp->hFile);
		}
		else
			return(INVALID_HANDLE_VALUE);
	}
	return((HANDLE)tmp);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE findclose()
 *----------------------------------------------------------------------------*/
void
ef_findclose(hFile)
HANDLE			hFile;
{
	FindData	*	p;
	int				i;

	for (i = 0; i < (sizeof(TempFind) / sizeof(TmpHandle)); i++)
	{
		if ((HANDLE)&TempFind[i] == hFile)
		{
			TempFind[i].hFile = INVALID_HANDLE_VALUE;
			while (TempFind[i].data != NULL)
			{
				p = TempFind[i].data->next;
				free(TempFind[i].data);
				TempFind[i].data = p;
			}
			return;
		}
	}
	FindClose(hFile);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE signal check
 *----------------------------------------------------------------------------*/
int
ef_breakcheck()
{
	BOOL			rc = FALSE;

	if (!ef_load)
		return(FALSE);
	if (lha_breakcheck())
		rc = TRUE;
	if (ftp_breakcheck())
		rc = TRUE;
	if (tar_breakcheck())
		rc = TRUE;
	if (gzip_breakcheck())
		rc = TRUE;
	if (zip_breakcheck())
		rc = TRUE;
	if (cab_breakcheck())
		rc = TRUE;
	return(rc);
}

/*------------------------------------------------------------------------------
 *	EXtended FILE global file name
 *----------------------------------------------------------------------------*/
char **
ef_globfilename(buf, dir)
char		*	buf;
BOOL			dir;
{
	int				j;
	char		**	result;
	char		*	p;
	char		*	wk;
	char		**	retp;
	int				cnt = 0;
	extern char	**	glob_filename(char *);

	if ((retp = (char **)malloc(sizeof(char *))) == NULL)
		return(NULL);
	ef_dir = dir;
	retp[cnt] = NULL;
	result = glob_filename(buf);
	for (j = 0; result[j] != NULL; j++)
	{
		if (ef_has_wildcard(result[j]) && (p = strrchr(&result[j][2], ':')) != NULL)
		{
			char	**	aresult;
			int			a;

			*p = '\0';
			aresult = glob_filename(result[j]);
			for (a = 0; aresult[a] != NULL; a++)
			{
				char		buf[MAX_PATH+2];
				char	**	lresult;
				int			k;

				strcpy(buf, aresult[a]);
				strcat(buf, ":");
				strcat(buf, p + 1);
				lresult = glob_filename(buf);
				for (k = 0; lresult[k] != NULL; k++)
				{
					if ((wk = malloc(sizeof(char *) * (cnt + 2))) == NULL)
					{
						ef_dir = TRUE;
						return(NULL);
					}
					memmove(wk, retp, sizeof(char *) * (cnt + 1));
					free(retp);
					retp = (char **)wk;
					retp[cnt] = lresult[k];
					retp[++cnt] = NULL;
				}
				free(lresult);
			}
			free(aresult);
			free(result[j]);
		}
		else
		{
			if ((wk = malloc(sizeof(char *) * (cnt + 2))) == NULL)
			{
				ef_dir = TRUE;
				return(NULL);
			}
			memmove(wk, retp, sizeof(char *) * (cnt + 1));
			free(retp);
			retp = (char **)wk;
			retp[cnt] = result[j];
			retp[++cnt] = NULL;
		}
	}
	free(result);
	ef_dir = TRUE;
	for (j = 0; retp[j] != NULL; j++)
		retp[j] = ef_getlongname(retp[j]);
	return(retp);
}
