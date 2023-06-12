/*******************************************************************************
 *	LZH file support
 ******************************************************************************/
#include <windows.h>

BOOL		lha_init(HWND);
VOID		lha_term(void);
char	*	lha_ver(void);
BOOL		is_lha(const char *, char);
BOOL		lha_sfx(char *);
BOOL		lha_get_aname(const char *, char, char *);
BOOL		lha_getfile(const char *, const char *);
BOOL		lha_putfile(const char *, const char *);
int			lha_remove(const char *);
int			lha_chmod(const char *, int);
char	*	lha_fullpath(char *, char *, int);
DWORD		lha_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			lha_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		lha_findfirst(const char *, WIN32_FIND_DATA *);
void		lha_findclose(HANDLE);
BOOL		lha_breakcheck(void);
