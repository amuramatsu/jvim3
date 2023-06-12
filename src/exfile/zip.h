/*******************************************************************************
 *	ZIP file support
 ******************************************************************************/
#include <windows.h>

BOOL		zip_init(HWND);
VOID		zip_term(void);
char	*	zip_ver(void);
BOOL		is_zip(const char *, char);
BOOL		zip_sfx(char *);
BOOL		zip_get_aname(const char *, char, char *);
BOOL		zip_getfile(const char *, const char *);
BOOL		zip_putfile(const char *, const char *);
int			zip_remove(const char *);
int			zip_chmod(const char *, int);
char	*	zip_fullpath(char *, char *, int);
DWORD		zip_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			zip_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		zip_findfirst(const char *, WIN32_FIND_DATA *);
void		zip_findclose(HANDLE);
BOOL		zip_breakcheck(void);
