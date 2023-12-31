/* vi:ts=4:sw=4
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Read the file "credits.txt" for a list of people who contributed.
 * Read the file "uganda.txt" for copying and usage conditions.
 */

/*
 * linefunc.c: some functions to move to the next/previous line and
 *			   to the next/previous character
 */

#include "vim.h"
#include "globals.h"
#include "proto.h"
#ifdef KANJI
#include "kanji.h"
#endif

/*
 * coladvance(col)
 *
 * Try to advance the Cursor to the specified column.
 */

	void
coladvance(wcol)
	colnr_t 		wcol;
{
	int 				index;
	register char_u		*ptr;
	register colnr_t	col;

	ptr = ml_get(curwin->w_cursor.lnum);

	/* try to advance to the specified column */
	index = -1;
	col = 0;
	while (col <= wcol && *ptr)
	{
		++index;
		/* Count a tab for what it's worth (if list mode not on) */
#ifdef KANJI
		if (ISkanji(*ptr))
		{
			col += 2;
			++ptr;
			++index;
		}
		else
#endif
		col += chartabsize(*ptr, (long)col);
		++ptr;
	}
	/*
	 * in insert mode it is allowed to be one char beyond the end of the line
	 */
	if ((State & INSERT) && col <= wcol)
		++index;
	if (index < 0)
		curwin->w_cursor.col = 0;
	else
		curwin->w_cursor.col = index;
#ifdef KANJI
	if (ISkanjiCur() == 2 && curwin->w_cursor.col != 0)
		curwin->w_cursor.col--;
#endif
}

/*
 * inc(p)
 *
 * Increment the line pointer 'p' crossing line boundaries as necessary.
 * Return 1 when crossing a line, -1 when at end of file, 0 otherwise.
 */
	int
inc_cursor()
{
	return inc(&curwin->w_cursor);
}

	int
inc(lp)
	register FPOS  *lp;
{
	register char_u  *p = ml_get_pos(lp);

	if (*p != NUL)
	{			/* still within line */
#ifdef KANJI
		if (ISkanji(*p))
		{
			lp->col += 2;
			p++;
		}
		else
		{
			lp->col++;
		}
#else
		lp->col++;
#endif
		return ((p[1] != NUL) ? 0 : 1);
	}
	if (lp->lnum != curbuf->b_ml.ml_line_count)
	{			/* there is a next line */
		lp->col = 0;
		lp->lnum++;
		return 1;
	}
	return -1;
}

/*
 * incl(lp): same as inc(), but skip the NUL at the end of non-empty lines
 */
	int
incl(lp)
	register FPOS *lp;
{
	register int r;

	if ((r = inc(lp)) == 1 && lp->col)
		r = inc(lp);
	return r;
}

/*
 * dec(p)
 *
 * Decrement the line pointer 'p' crossing line boundaries as necessary.
 * Return 1 when crossing a line, -1 when at start of file, 0 otherwise.
 */
	int
dec_cursor()
{
	return dec(&curwin->w_cursor);
}

	int
dec(lp)
	register FPOS  *lp;
{
	if (lp->col > 0)
	{			/* still within line */
		lp->col--;
#ifdef KANJI
		if (ISkanjiFpos(lp) == 2 && lp->col > 0)
			lp->col--;
#endif
		return 0;
	}
	if (lp->lnum > 1)
	{			/* there is a prior line */
		lp->lnum--;
		lp->col = STRLEN(ml_get(lp->lnum));
		return 1;
	}
	return -1;					/* at start of file */
}

/*
 * decl(lp): same as dec(), but skip the NUL at the end of non-empty lines
 */
	int
decl(lp)
		register FPOS *lp;
{
		register int r;

		if ((r = dec(lp)) == 1 && lp->col)
				r = dec(lp);
		return r;
}

/*
 * make sure curwin->w_cursor in on a valid character
 */
	void
adjust_cursor()
{
	int len;

	if (curwin->w_cursor.lnum == 0)
		curwin->w_cursor.lnum = 1;
	if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;

	len = STRLEN(ml_get(curwin->w_cursor.lnum));
	if (len == 0)
		curwin->w_cursor.col = 0;
	else if (curwin->w_cursor.col >= len){
		curwin->w_cursor.col = len - 1;
#ifdef KANJI
		if (ISkanjiCur() == 2 && curwin->w_cursor.col > 0)
			curwin->w_cursor.col--;
#endif
	}
}
