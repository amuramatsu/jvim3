/*******************************************************************************
 *	LZH file support
 ******************************************************************************/
#include <windows.h>

BOOL		tar_init(HWND);
VOID		tar_term(void);
char	*	tar_ver(void);
BOOL		is_tar(const char *, char);
BOOL		tar_sfx(char *);
BOOL		tar_get_aname(const char *, char, char *);
BOOL		tar_getfile(const char *, const char *);
BOOL		tar_putfile(const char *, const char *);
int			tar_remove(const char *);
int			tar_chmod(const char *, int);
char	*	tar_fullpath(char *, char *, int);
DWORD		tar_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			tar_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		tar_findfirst(const char *, WIN32_FIND_DATA *);
void		tar_findclose(HANDLE);
BOOL		tar_breakcheck(void);

BOOL		gzip_init(HWND);
VOID		gzip_term(void);
char	*	gzip_ver(void);
char	*	gzip_version(void);
BOOL		is_gzip(const char *, char);
BOOL		gzip_sfx(char *);
BOOL		gzip_get_aname(const char *, char, char *);
BOOL		gzip_getfile(const char *, const char *);
BOOL		gzip_putfile(const char *, const char *);
int			gzip_remove(const char *);
int			gzip_chmod(const char *, int);
char	*	gzip_fullpath(char *, char *, int);
DWORD		gzip_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			gzip_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		gzip_findfirst(const char *, WIN32_FIND_DATA *);
void		gzip_findclose(HANDLE);
BOOL		gzip_breakcheck(void);
