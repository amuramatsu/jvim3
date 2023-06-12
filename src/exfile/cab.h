/*******************************************************************************
 *	ZIP file support
 ******************************************************************************/
#include <windows.h>

BOOL		cab_init(HWND);
VOID		cab_term(void);
char	*	cab_ver(void);
BOOL		is_cab(const char *, char);
BOOL		cab_sfx(char *);
BOOL		cab_get_aname(const char *, char, char *);
BOOL		cab_getfile(const char *, const char *);
BOOL		cab_putfile(const char *, const char *);
int			cab_remove(const char *);
int			cab_chmod(const char *, int);
char	*	cab_fullpath(char *, char *, int);
DWORD		cab_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			cab_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		cab_findfirst(const char *, WIN32_FIND_DATA *);
void		cab_findclose(HANDLE);
BOOL		cab_breakcheck(void);
