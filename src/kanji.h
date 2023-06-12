/*******************************************************************************
 * vi:ts=4:sw=4
 *	original Ogasawara Hiroyuki (COR.)
 *  original Atsushi Nakamura
 ******************************************************************************/
#ifdef KANJI

#define BRCHAR		'\\'		/* line break char */

#define	JP_EUC		'E'			/* EUC */
#define	JP_JIS		'J'			/* JIS */
#define	JP_SJIS		'S'			/* Shift-JIS */
#define JP_WIDE		'U'			/* UNICODE */
#define JP_UTF8		'T'			/* UTF-8 */
#define JP_NONE		'X'			/* No Kanji */
#define JP_FSTR		"EJSejs"
#define JP_STR		"EJS"
#define	JP_KEY		*p_jp		/* file/key in code */
#define	JP_DISP		*(p_jp + 1)	/* terminal display code */
#define	JP_FILE		*(p_jp + 2)	/* write code for new file */
#define	JP_SYS		*(p_jp + 2)	/* system code for pipe */

# ifndef JP_DEF
#  if defined(MSDOS) || defined(__CYGWIN__)
#   define	JP_DEF	"SSS"
#  else
#   define	JP_DEF	"EEE"
#  endif
# endif

# ifdef UCODE
#  undef JP_FSTR
#  define JP_FSTR       "EJSUTejsut"
#  undef JP_STR
#  define JP_STR		"EJST"
# endif

#define JP_ASCII	0
#define JP_KANJI	1
#define	JP_KANA		2

#define JPC_ALNUM	3
#define JPC_HIRA	4
#define JPC_KATA	5		/* 2byte kana */
#define JPC_KANJI	6
#define JPC_KIGOU	7
#define JPC_KIGOU2	8
#define JPC_KANA	9		/* 1byte kana */

#define JP1_ALNUM	'#'
#define JP1_HIRA	'$'
#define JP1_KATA	'%'
#define JP1_KIGOU	'!'
#define JP1_KIGOU2	'"'

#define HexChar(_c)	((_c) >= 10 ? 'A' + (_c) - 10 : '0' + (_c));

#endif /* KANJI */
