/* vi:ts=4:sw=4
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Read the file "credits.txt" for a list of people who contributed.
 * Read the file "uganda.txt" for copying and usage conditions.
 */

/*
 * This file contains various definitions of structures that are used by Vim
 */

/*
 * file position
 */

typedef struct fpos		FPOS;
/*
 * there is something wrong in the SAS compiler that makes typedefs not
 * valid in include files
 */
#ifdef SASC
typedef long			linenr_t;
typedef unsigned		colnr_t;
typedef unsigned short	short_u;
#endif

struct fpos
{
	linenr_t		lnum;			/* line number */
	colnr_t 		col;			/* column number */
};

/*
 * marks: positions in a file
 * (a normal mark is a lnum/col pair, the same as a file position)
 */

#define NMARKS			26			/* max. # of named marks */
#define JUMPLISTSIZE	30			/* max. # of marks in jump list */
#define TAGSTACKSIZE	20			/* max. # of tags in tag stack */

struct filemark
{
	FPOS			mark;			/* cursor position */
	int				fnum;			/* file number */
};

/*
 * the taggy struct is used to store the information about a :tag command:
 *	the tag name and the cursor position BEFORE the :tag command
 */
struct taggy
{
	char_u			*tagname;			/* tag name */
	struct filemark fmark;				/* cursor position */
};

/*
 * line number list
 */

/*
 * Each window can have a different line number associated with a buffer.
 * The window-pointer/line-number pairs are kept in the line number list.
 * The list of line numbers is kept in most-recently-used order.
 */

typedef struct window		WIN;
typedef struct winlnum		WINLNUM;

struct winlnum
{
	WINLNUM		*wl_next;			/* next entry or NULL for last entry */
	WINLNUM		*wl_prev;			/* previous entry or NULL for first entry */
	WIN			*wl_win;			/* pointer to window that did set wl_lnum */
	linenr_t	 wl_lnum;			/* last cursor line in the file */
};

/*
 * stuctures used for undo
 */

struct u_entry
{
	struct u_entry	*ue_next;	/* pointer to next entry in list */
	linenr_t		ue_top;		/* number of line above undo block */
	linenr_t		ue_bot;		/* number of line below undo block */
	linenr_t		ue_lcount;	/* linecount when u_save called */
	char_u			**ue_array;	/* array of lines in undo block */
	long			ue_size;	/* number of lines in ue_array */
};

struct u_header
{
	struct u_header	*uh_next;	/* pointer to next header in list */
	struct u_header	*uh_prev;	/* pointer to previous header in list */
	struct u_entry	*uh_entry;	/* pointer to first entry */
	FPOS			 uh_cursor;	/* cursor position before saving */
	int				 uh_changed;/* b_changed flag before undo/after redo */
	FPOS			 uh_namedm[NMARKS];	/* marks before undo/after redo */
};

/*
 * stuctures used in undo.c
 */
#if defined(UNIX) || defined(NT)
# define ALIGN_LONG		/* longword alignment and use filler byte */
# define ALIGN_SIZE (sizeof(long))
#else
# define ALIGN_SIZE (sizeof(short))
#endif

#define ALIGN_MASK (ALIGN_SIZE - 1)

typedef struct m_info info_t;

/*
 * stucture used to link chunks in one of the free chunk lists.
 */
struct m_info
{
#ifdef ALIGN_LONG
	long_u	 m_size;	/* size of the chunk (including m_info) */
#else
	short_u  m_size;	/* size of the chunk (including m_info) */
#endif
	info_t	*m_next;	/* pointer to next free chunk in the list */
};

/*
 * structure used to link blocks in the list of allocated blocks.
 */
struct m_block
{
	struct m_block	*mb_next;	/* pointer to next allocated block */
	info_t			mb_info;	/* head of free chuck list for this block */
};

/*
 * things used in memfile.c
 */

typedef struct block_hdr	BHDR;
typedef struct memfile		MEMFILE;
typedef long				blocknr_t;

/*
 * for each (previously) used block in the memfile there is one block header.
 *
 * The block may be linked in the used list OR in the free list.
 * The used blocks are also kept in hash lists.
 *
 * The used list is a doubly linked list, most recently used block first.
 * 		The blocks in the used list have a block of memory allocated.
 *		mf_used_count is the number of pages in the used list.
 * The hash lists are used to quickly find a block in the used list.
 * The free list is a single linked list, not sorted.
 *		The blocks in the free list have no block of memory allocated and
 *		the contents of the block in the file (if any) is irrelevant.
 */

struct block_hdr
{
	BHDR		*bh_next;			/* next block_hdr in free or used list */
	BHDR		*bh_prev;			/* previous block_hdr in used list */
	BHDR		*bh_hash_next;		/* next block_hdr in hash list */
	BHDR		*bh_hash_prev;		/* previous block_hdr in hash list */
	blocknr_t	bh_bnum;				/* block number */
	char_u		*bh_data;			/* pointer to memory (for used block) */
	int			bh_page_count;		/* number of pages in this block */

#define BH_DIRTY	1
#define BH_LOCKED	2
	char		bh_flags;			/* BH_DIRTY or BH_LOCKED */
};

/*
 * when a block with a negative number is flushed to the file, it gets
 * a positive number. Because the reference to the block is still the negative
 * number, we remember the translation to the new positive number in the
 * double linked trans lists. The structure is the same as the hash lists.
 */
typedef struct nr_trans NR_TRANS;

struct nr_trans
{
	NR_TRANS	*nt_next;			/* next nr_trans in hash list */
	NR_TRANS	*nt_prev;			/* previous nr_trans in hash list */
	blocknr_t	nt_old_bnum;			/* old, negative, number */
	blocknr_t	nt_new_bnum;			/* new, positive, number */
};

/*
 * Simplistic hashing scheme to quickly locate the blocks in the used list.
 * 64 blocks are found directly (64 * 4K = 256K, most files are smaller).
 */
#define MEMHASHSIZE		64
#define MEMHASH(nr)		((nr) & (MEMHASHSIZE - 1))

struct memfile
{
	char_u		*mf_fname;			/* name of the file */
	char_u		*mf_xfname;			/* idem, full path */
	int			mf_fd;				/* file descriptor */
	BHDR		*mf_free_first;		/* first block_hdr in free list */
	BHDR		*mf_used_first;		/* mru block_hdr in used list */
	BHDR		*mf_used_last;		/* lru block_hdr in used list */
	unsigned	mf_used_count;		/* number of pages in used list */
	unsigned	mf_used_count_max;	/* maximum number of pages in memory */
	BHDR		*mf_hash[MEMHASHSIZE];	/* array of hash lists */
	NR_TRANS	*mf_trans[MEMHASHSIZE];	/* array of trans lists */
	blocknr_t	mf_blocknr_max;		/* highest positive block number + 1*/
	blocknr_t	mf_blocknr_min;		/* lowest negative block number - 1 */
	blocknr_t	mf_neg_count;		/* number of negative blocks numbers */
	blocknr_t	mf_infile_count;	/* number of pages in the file */
	unsigned	mf_page_size;		/* number of bytes in a page */
	int			mf_dirty;			/* Set to TRUE if there are dirty blocks */
};

/*
 * things used in memline.c
 */
typedef struct info_pointer		IPTR;		/* block/index pair */

/*
 * When searching for a specific line, we remember what blocks in the tree
 * are the branches leading to that block. This is stored in ml_stack.
 * Each entry is a pointer to info in a block (may be data block or pointer block)
 */
struct info_pointer
{
	blocknr_t	ip_bnum;		/* block number */
	linenr_t	ip_low;			/* lowest lnum in this block */
	linenr_t	ip_high;		/* highest lnum in this block */
	int			ip_index;		/* index for block with current lnum */
};

typedef struct memline MEMLINE;

/*
 * the memline structure holds all the information about a memline
 */
struct memline
{
	linenr_t	ml_line_count;	/* number of lines in the buffer */

	MEMFILE		*ml_mfp;		/* pointer to associated memfile */

#define ML_EMPTY		1		/* empty buffer (one empty line */
#define ML_LINE_DIRTY	2		/* cached line was changed and allocated */
#define ML_LOCKED_DIRTY	4		/* ml_locked was changed */
#define ML_LOCKED_POS	8		/* ml_locked needs positive block number */
	int			ml_flags;

	IPTR		*ml_stack;		/* stack of pointer blocks (array of IPTRs) */
	int			ml_stack_top;	/* current top if ml_stack */
	int			ml_stack_size;	/* total number of entries in ml_stack */

	linenr_t	ml_line_lnum;	/* line number of cached line, 0 if not valid */
	char_u		*ml_line_ptr;	/* pointer to cached line */

	BHDR		*ml_locked;		/* block used by last ml_get */
	linenr_t	ml_locked_low;	/* first line in ml_locked */
	linenr_t	ml_locked_high;	/* last line in ml_locked */
	int			ml_locked_lineadd;	/* number of lines inserted in ml_locked */
};

/*
 * buffer: structure that holds information about one file
 *
 * Several windows can share a single Buffer
 * A buffer is unallocated if there is no memfile for it.
 * A buffer is new if the associated file has never been loaded yet.
 */

typedef struct buffer BUF;

struct buffer
{
	MEMLINE			 b_ml;				/* associated memline (also contains
										 * line count) */

	BUF				*b_next;			/* links in list of buffers */
	BUF				*b_prev;

	int				 b_changed;			/* Set to 1 if something in the file has
								 		 * been changed and not written out. */

	int				 b_notedited;		/* Set to TRUE when file name is
										 * changed after starting to edit,
								 		 * reset when file is written out. */

	int              b_nwindows;		/* nr of windows open on this buffer */

	int				 b_neverloaded;		/* file has never been loaded into
										 * buffer, many variables still need
										 * to be set */

	/*
	 * b_filename has the full path of the file.
	 * b_sfilename is the name as the user typed it.
	 * b_xfilename is the same as b_sfilename, unless did_cd is set, then it
	 *    		   is the same as b_filename.
	 */
	char_u			*b_filename;
	char_u			*b_sfilename;
	char_u			*b_xfilename;

	int				 b_fnum;			/* file number for this file. */
	WINLNUM			*b_winlnum;			/* list of last used lnum for
										 * each window */

	long			 b_mtime;			/* last change time of original file */

	/*
	 * The following only used in mark.c.
	 */
	FPOS          	 b_namedm[NMARKS];	/* current marks */

	/*
	 * start and end of an operator, also used for '[ and ']
	 */
	FPOS			 b_startop;
	FPOS			 b_endop;

	/*
	 * The following only used in undo.c.
	 */
	struct u_header	*b_u_oldhead;		/* pointer to oldest header */
	struct u_header	*b_u_newhead;		/* pointer to newest header */
	struct u_header	*b_u_curhead;		/* pointer to current header */
	int				 b_u_numhead;		/* current number of headers */
	int				 b_u_synced;		/* entry lists are synced */

	/*
	 * variables for "U" command in undo.c
	 */
	char_u			*b_u_line_ptr;		/* saved line for "U" command */
	linenr_t		 b_u_line_lnum;		/* line number of line in u_line */
	colnr_t			 b_u_line_colnr;	/* optional column number */

	/*
	 * The following only used in undo.c
	 */
	struct m_block	 b_block_head;		/* head of allocated memory block list */
	info_t			*b_m_search;	 	/* pointer to chunk before previously
									   	 * allocated/freed chunk */
	struct m_block	*b_mb_current;		/* block where m_search points in */

	/*
	 * Variables "local" to a buffer.
	 * They are here because their value depends on the type of file
	 * or contents of the file being edited.
	 * The "save" options are for when the paste option is set.
	 */
	int				 b_p_ai, b_p_si, b_p_ro;
	int				 b_p_bin, b_p_eol, b_p_et, b_p_ml, b_p_sn, b_p_tx;
	long			 b_p_sw, b_p_ts, b_p_tw, b_p_wm;
	int				 b_p_ai_save, b_p_si_save;
	long			 b_p_tw_save, b_p_wm_save;

	char			 b_did_warn;		/* Set to 1 if user has been warned on
										 * first change of a read-only file */

#if !defined(MSDOS) || defined(NT)
	int				 b_shortname;		/* this file has an 8.3 filename */
#endif
#ifdef KANJI
	char			*b_p_jc;			/* kanji file code */
	int				 b_p_ubig;			/* unicode big endian */
#endif
#ifdef FEPCTRL
	int				 b_p_fc;			/* fep control */
	int				 fepmode;			/* fep current status */
	char_u			*b_p_ji;			/* keep ASC/JP insert mode */
	long			 knj_asc;			/* for automatic change of p_ji */
#endif
#ifdef TRACK
	char_u			*b_p_trs;			/* track character set */
	int				 b_p_tt;			/* transparent track mode */
#endif
#if defined(NT) && defined(USE_EXFILE) && defined(USE_MATOME)
	char			*b_p_ec;			/* encode type nothing */
#endif
#ifdef USE_OPT
	int				 b_p_opt;			/* option mode */
# define FOPT_MAC_FILE			0x0100		/* macintosh file */
# define FOPT_EXTEND			0x00ff		/* option mask */
# define FOPT_DEL_SPC			0x0001		/* delete line end spaces */
# define FOPT_REP_SPC			0x0002		/* replace kanji space */
# define FOPT_EXPAND_TAB		0x0004		/* expand tab */
# define FOPT_ENCODE_TAB		0x0008		/* line top space to tab */
# define FOPT_C_COMMENT			0x0040		/* C Comment Skip showmatch */
# define FOPT_GAIJI				0x0080		/* Gaiji */
#endif
#if defined(KANJI) && defined(NT) && defined(SYNTAX)
	char_u		   *b_syn_ptr;
	char_u		   *b_syn_tag;
	char_u		   *b_syn_match;
	char_u		   *b_syn_matchend;
	char_u		   *b_syn_curp;
	linenr_t		b_syn_line;
	linenr_t		b_syn_nline;
	char_u		   *b_syn_link;
#endif
};

/*
 * Structure which contains all information that belongs to a window
 *
 * All row numbers are relative to the start of the window, except w_winpos.
 */

struct window
{
	BUF			*w_buffer; 			/* buffer we are a window into */

	WIN			*w_prev;			/* link to previous window (above) */
	WIN			*w_next;			/* link to next window (below) */

	FPOS		w_cursor;			/* cursor's position in buffer */

	/*
	 * These elements are related to the cursor's position in the window.
	 * This is related to character positions in the window, not in the file.
	 */
	int			w_row, w_col;		/* cursor's position in window */

	colnr_t		w_virtcol;			/* column number of the file's actual */
									/* line, as opposed to the column */
									/* number we're at on the screen. */
									/* This makes a difference on lines */
									/* which span more than one screen */
									/* line. */

	colnr_t		w_curswant;			/* The column we'd like to be at. */
									/* This is used to try to stay in */
									/* the same column through up/down */
									/* cursor motions. */

	int			w_set_curswant;		/* If set, then update w_curswant */
									/* the next time through cursupdate() */
									/* to the current virtual column */

	linenr_t	w_topline;			/* number of the line at the top of
									 * the screen */
	linenr_t	w_botline;			/* number of the line below the bottom
									 * of the screen */
	int			w_empty_rows;		/* number of ~ rows in window */

	int			w_winpos;			/* row of topline of window in screen */
	int			w_height;			/* number of rows in window, excluding
										status/command line */
	int			w_status_height;	/* number of status lines (0 or 1) */

	int			w_redr_status;		/* if TRUE status line must be redrawn */
	int			w_redr_type;		/* type of redraw to be performed on win */

	int			w_leftcol;			/* starting column of the screen */

/*
 * The height of the lines currently in the window is remembered
 * to avoid recomputing it every time. The number of entries is w_nrows.
 */
	int		 	w_lsize_valid;		/* nr. of valid LineSizes */
	linenr_t 	*w_lsize_lnum;		/* array of line numbers for w_lsize */
	char_u 	 	*w_lsize;			/* array of line heights */

	int			w_alt_fnum;			/* alternate file (for # and CTRL-^) */

	int			w_arg_idx;			/* current index in argument list */

	/*
	 * Variables "local" to a window.
	 * They are here because they influence the layout of the window or
	 * depend on the window layout.
	 */
	int			w_p_list,
				w_p_nu,
				w_p_wrap;
	long		w_p_scroll;
#ifdef CRMARK
	int			w_p_cr;
# define CRSTR			"$"			/* cr string */
#endif

	/*
	 * The w_prev_pcmark field is used to check whether we really did jump to
	 * a new line after setting the w_pcmark.  If not, then we revert to
	 * using the previous w_pcmark.
	 */
	FPOS		w_pcmark;			/* previous context mark */
	FPOS		w_prev_pcmark;		/* previous w_pcmark */

	/*
	 * the jumplist contains old cursor positions
	 */
	struct filemark w_jumplist[JUMPLISTSIZE];
	int 			w_jumplistlen;	/* number of active entries */
	int				w_jumplistidx;	/* current position */

	/*
	 * the tagstack grows from 0 upwards:
	 * entry 0: older
	 * entry 1: newer
	 * entry 2: newest
	 */
	struct taggy	w_tagstack[TAGSTACKSIZE];	/* the tag stack */
	int				w_tagstackidx;				/* index just below active entry */
	int				w_tagstacklen;				/* number of tags on the stack */

#if defined(KANJI) && defined(NT) && defined(SYNTAX)
	int				 w_p_syt;				/* syntax mode */
#endif
};
