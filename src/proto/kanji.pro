/* kanji.c */
int		ISkanji			__ARGS((int));
int		ISkana			__ARGS((int));
int		ISdisp			__ARGS((int));
int		ISkanjiPosition	__ARGS((unsigned char *, int));
int		ISkanjiPointer	__ARGS((unsigned char *, unsigned char *));
int		ISkanjiCol		__ARGS((linenr_t, colnr_t));
int		ISkanjiCur		__ARGS((void));
int		ISkanjiFpos		__ARGS((FPOS *));
int		vcol2col		__ARGS((WIN *, linenr_t, colnr_t, int *, int, int));
int_u	sjistojis		__ARGS((char_u, char_u));
int_u	jistosjis		__ARGS((char_u, char_u));
void	kanjito			__ARGS((char_u *, char_u *, int));
void	kanato			__ARGS((char_u *, char_u *, int));
int		jpcls			__ARGS((char_u, char_u));
int		isjppunc		__ARGS((char_u, char_u, int));
int		isaspunc		__ARGS((char_u, int));
int		isjsend			__ARGS((char_u *));
void	jptocase		__ARGS((char_u *, char_u *, int));
int		isjpspace		__ARGS((char_u, char_u));
int		judge_jcode		__ARGS((char_u *, int *, char_u *, long));
int		kanjiconvsfrom	__ARGS((char_u*, int, char_u*, int, char*, char, int*));
char_u *kanjiconvsto	__ARGS((char_u *, int, int));
char   *fileconvsfrom	__ARGS((char_u *));
char   *fileconvsto		__ARGS((char_u *));
void	binaryconvsfrom	__ARGS((linenr_t, char, int *, char_u *, int, char_u *));
char_u *binaryconvsto	__ARGS((char, char_u *, int *, int));
int		jp_strnicmp		__ARGS((char_u *, char_u *, size_t));
#ifdef UCODE
int		wide2multi		__ARGS((char_u *, int, int, int));
void	multi2wide		__ARGS((char_u *, char_u *, int, int));
#endif
