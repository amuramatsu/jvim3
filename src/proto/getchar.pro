/* getchar.c */
unsigned char *get_recorded __PARMS((void));
unsigned char *get_inserted __PARMS((void));
int stuff_empty __PARMS((void));
void flush_buffers __PARMS((int typeahead));
void ResetRedobuff __PARMS((void));
void AppendToRedobuff __PARMS((unsigned char *s));
void AppendCharToRedobuff __PARMS((int c));
void AppendNumberToRedobuff __PARMS((long n));
void stuffReadbuff __PARMS((unsigned char *s));
void stuffcharReadbuff __PARMS((int c));
void stuffnumReadbuff __PARMS((long n));
void copy_redo __PARMS((void));
int start_redo __PARMS((long count));
int start_redo_ins __PARMS((void));
void set_redo_ins __PARMS((void));
void stop_redo_ins __PARMS((void));
int ins_typestr __PARMS((unsigned char *str, int noremap));
void del_typestr __PARMS((int len));
int openscript __PARMS((unsigned char *name));
void updatescript __PARMS((int c));
int vgetc __PARMS((void));
int vpeekc __PARMS((void));
int domap __PARMS((int maptype, unsigned char *keys, int mode));
int check_abbr __PARMS((int c, unsigned char *ptr, int col, int mincol));
int makemap __PARMS((FILE *fd));
int putescstr __PARMS((FILE *fd, unsigned char *str, int set));
