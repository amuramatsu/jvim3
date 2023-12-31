/* term.c */
void set_term __PARMS((unsigned char *term));
char *tgoto __PARMS((char *cm, int x, int y));
void termcapinit __PARMS((unsigned char *term));
void flushbuf __PARMS((void));
void outchar __PARMS((unsigned int c));
#ifdef KANJI
void outchar2 __PARMS((unsigned int c1, unsigned int c2));
#endif
void outstrn __PARMS((unsigned char *s));
void outstr __PARMS((unsigned char *s));
void windgoto __PARMS((int row, int col));
void setcursor __PARMS((void));
void ttest __PARMS((int pairs));
int inchar __PARMS((unsigned char *buf, int maxlen, int time));
int check_termcode __PARMS((unsigned char *buf));
void outnum __PARMS((long n));
void check_winsize __PARMS((void));
void set_winsize __PARMS((int width, int height, int mustset));
void settmode __PARMS((int raw));
void starttermcap __PARMS((void));
void stoptermcap __PARMS((void));
void cursor_on __PARMS((void));
void cursor_off __PARMS((void));
void scroll_region_set __PARMS((struct window *wp));
void scroll_region_reset __PARMS((void));
