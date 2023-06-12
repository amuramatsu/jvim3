/* vi:ts=4:sw=4
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Read the file "credits.txt" for a list of people who contributed.
 * Read the file "uganda.txt" for copying and usage conditions.
 */

/* prototypes from syntax.c */
int		syn_user_color __ARGS((char_u));
void	syn_clr __ARGS((BUF *));
int		syn_add __ARGS((BUF *, char_u *));
void	syn_inschar __ARGS((char_u *, colnr_t));
void	syn_delchar __ARGS((char_u *, colnr_t));
int		is_crsyntax __ARGS((WIN *));
int		is_syntax __ARGS((WIN *, linenr_t, char_u **, char_u **));
