/* fepctrl.c */
#ifdef CANNA
#define fep_init		canna_init
#define fep_term		canna_term
#define fep_on			canna_on
#define fep_off			canna_off
#define fep_force_on	canna_force_on
#define fep_force_off	canna_force_off
#define fep_get_mode	canna_get_mode
#define fep_visualmode	canna_visualmode
#define fep_resize		canna_resize
#define fep_isready		canna_isready
#define fep_getkey		canna_getkey
#define fep_inject		canna_inject
#endif
#ifdef ONEW
#define fep_init		onew_init
#define fep_term		onew_term
#define fep_on			onew_on
#define fep_off			onew_off
#define fep_force_on	onew_force_on
#define fep_force_off	onew_force_off
#define fep_get_mode	onew_get_mode
#define fep_visualmode	onew_visualmode
#define fep_resize		onew_resize
#define fep_isready		onew_isready
#define fep_getkey		onew_getkey
#define fep_inject		onew_inject
#define isdir			vim_isdir
int		vim_isdir __PARMS((char_u *));
#endif

int		fep_init __PARMS((void));
void	fep_term __PARMS((void));
void	fep_on __PARMS((void));
void	fep_off __PARMS((void));
void	fep_force_on __PARMS((void));
void	fep_force_off __PARMS((void));
int		fep_get_mode __PARMS((void));

#ifdef NT
void	fep_win_sync __PARMS((HWND));
void	fep_win_font __PARMS((HWND, LOGFONT	*));
#endif

#if defined(CANNA) || defined(ONEW)
void	fep_visualmode __PARMS((int));
void	fep_resize __PARMS((void));
int		fep_isready __PARMS((void));
int		fep_getkey __PARMS((void));
void	fep_inject __PARMS((int));
#endif
#ifdef CANNA
extern	int		canna_empty;
void	canna_echoback __PARMS((void));
#endif
#ifdef ONEW
void	onew_freqsave __PARMS((void));
void	Onew_curmsg __PARMS((char *));
#endif
