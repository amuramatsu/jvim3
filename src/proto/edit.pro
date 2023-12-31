/* edit.c */
void edit __PARMS((long count));
#ifdef KANJI
int get_literal __PARMS((int *nextc, int *kp));
void insertchar __PARMS((unsigned int c, unsigned int k));
#else
int get_literal __PARMS((int *nextc));
void insertchar __PARMS((unsigned int c));
#endif
void beginline __PARMS((int flag));
int oneright __PARMS((void));
int oneleft __PARMS((void));
int oneup __PARMS((long n));
int onedown __PARMS((long n));
int onepage __PARMS((int dir, long count));
void stuff_inserted __PARMS((int c, long count, int no_esc));
unsigned char *get_last_insert __PARMS((void));
