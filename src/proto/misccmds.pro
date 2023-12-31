/* misccmds.c */
int get_indent __PARMS((void));
void set_indent __PARMS((int size, int delete));
int Opencmd __PARMS((int dir, int redraw, int delspaces));
int plines __PARMS((long p));
int plines_win __PARMS((struct window *wp, long p));
int plines_m __PARMS((long first, long last));
int plines_m_win __PARMS((struct window *wp, long first, long last));
#ifdef KANJI
void inschar __PARMS((int c, int k));
#else
void inschar __PARMS((int c));
#endif
void insstr __PARMS((unsigned char *s));
int delchar __PARMS((int fixpos));
void dellines __PARMS((long nlines, int dowindow, int undo));
int gchar __PARMS((struct fpos *pos));
int gchar_cursor __PARMS((void));
void pchar_cursor __PARMS((int c));
int inindent __PARMS((void));
void skipspace __PARMS((unsigned char **pp));
void skiptospace __PARMS((unsigned char **pp));
void skiptodigit __PARMS((unsigned char **pp));
long getdigits __PARMS((unsigned char **pp));
unsigned char *plural __PARMS((long n));
void set_Changed __PARMS((void));
void unset_Changed __PARMS((struct buffer *buf));
void change_warning __PARMS((void));
int ask_yesno __PARMS((unsigned char *str));
void msgmore __PARMS((long n));
void beep __PARMS((void));
void expand_env __PARMS((unsigned char *src, unsigned char *dst, int dstlen));
void home_replace __PARMS((unsigned char *src, unsigned char *dst, int dstlen));
int fullpathcmp __PARMS((unsigned char *s1, unsigned char *s2));
unsigned char *gettail __PARMS((unsigned char *fname));
int ispathsep __PARMS((int c));
