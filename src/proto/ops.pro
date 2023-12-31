/* ops.c */
void doshift __PARMS((int op, int curs_top, int amount));
void shift_line __PARMS((int left, int round, int amount));
int is_yank_buffer __PARMS((int c, int write));
int dorecord __PARMS((int c));
int doexecbuf __PARMS((int c));
int insertbuf __PARMS((int c));
void dodelete __PARMS((void));
void dotilde __PARMS((void));
void swapchar __PARMS((struct fpos *pos));
void dochange __PARMS((void));
void init_yank __PARMS((void));
int doyank __PARMS((int deleting));
void doput __PARMS((int dir, long count, int fix_indent));
void dodis __PARMS((void));
void dis_msg __PARMS((unsigned char *p, int skip_esc));
void dodojoin __PARMS((long count, int insert_space, int redraw));
int dojoin __PARMS((int insert_space, int redraw));
void doformat __PARMS((void));
void startinsert __PARMS((int initstr, int startln, long count));
int doaddsub __PARMS((int command, long Prenum1));
#ifdef NT
int yank_to_clipboard __PARMS((char_u *ptr));
#endif
