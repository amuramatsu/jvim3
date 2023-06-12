/*******************************************************************************
 *	LZH file support
 ******************************************************************************/
#include <windows.h>

BOOL		ftp_init(HWND);
VOID		ftp_term(void);
char	*	ftp_ver(void);
BOOL		is_ftp(const char *, char);
BOOL		ftp_sfx(char *);
BOOL		ftp_getfile(const char *, const char *);
BOOL		ftp_putfile(const char *, const char *);
int			ftp_remove(const char *);
int			ftp_chmod(const char *, int);
char	*	ftp_fullpath(char *, char *, int);
DWORD		ftp_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			ftp_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		ftp_findfirst(const char *, WIN32_FIND_DATA *);
void		ftp_findclose(HANDLE);
BOOL		ftp_breakcheck(void);
