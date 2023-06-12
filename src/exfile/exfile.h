/*******************************************************************************
 *	EXtended FILE support
 ******************************************************************************/
#include <windows.h>
VOID		ef_init(HWND);
VOID		ef_term(void);
char	*	ef_ver(void);
BOOL		is_ef(const char *, char);
BOOL		is_efarc(const char *, char);
BOOL		ef_get_aname(const char *, char, char *);
int			ef_open(const char *, int, ...);
int			ef_close(int);
int			ef_remove(const char *);
int			ef_rename(const char *, const char *);
long		ef_stat(char *, struct stat *);
int			ef_chmod(const char *, int);
char	*	ef_fullpath(char *, char *, int);
DWORD		ef_getfullpathname(LPCTSTR, DWORD, LPTSTR, LPTSTR *);
int			ef_findnext(HANDLE, WIN32_FIND_DATA *);
HANDLE		ef_findfirst(const char *, WIN32_FIND_DATA *);
void		ef_findclose(HANDLE);
BOOL		ef_breakcheck(void);
char **		ef_globfilename(char *, BOOL);

BOOL		ef_iskanji(unsigned char);
BOOL		ef_pathsep(unsigned char);
void		ef_slash(char *);
long		ef_getperm(char *);
char	*	ef_gettail(char *);
char	*	ef_getlongname(char *);
VOID		ef_loop(void);

VOID		ef_share_init(BOOL);
VOID		ef_share_term();
BOOL		ef_share_busy();
BOOL		ef_share_process(const char *);
BOOL		ef_share_replace(const char *);
BOOL		ef_share_check(const char *, int);
VOID		ef_share_open(const char *);
VOID		ef_share_close(const char *);

#ifdef USE_MATOME
void		decode(BOOL, long, long);
char	*	encode(char, char *, int, unsigned char *, long *, int);
char	*	decode_ver(void);
#endif

#ifndef  __EXFILE__C
# define	open				ef_open
# define	close				ef_close
# undef		remove
# define	remove				ef_remove
# define	rename				ef_rename
# define	chmod				ef_chmod
# undef		FindFirstFile
# define	FindFirstFile		ef_findfirst
# undef		FindNextFile
# define	FindNextFile		ef_findnext
# define	FindClose			ef_findclose
# define	_fullpath			ef_fullpath
# undef		GetFullPathName
# define	GetFullPathName		ef_getfullpathname
#endif
