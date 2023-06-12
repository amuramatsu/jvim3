/* vi:ts=4:sw=4
 *
 * VIM - Vi IMproved
 *
 */

/*
 * winnt.c
 *
 * Windows NT system-dependent routines.
 * A reasonable approximation of the amiga dependent code.
 * Portions lifted from SDK samples, from the MSDOS dependent code,
 * and from NetHack 3.1.3.
 *
 * rogerk@wonderware.com
 */

#include <io.h>
#include <direct.h>
#include "vim.h"
#include "globals.h"
#include "param.h"
#include "ops.h"
#include "proto.h"
#include <fcntl.h>
#ifdef KANJI
#include "kanji.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#undef DELETE
#include <windows.h>
#include <windowsx.h>
#include <wincon.h>
#include <commctrl.h>
#include <dlgs.h>
#ifndef NO_WHEEL
# include <zmouse.h>
# ifndef SPI_GETWHEELSCROLLLINES
#  define SPI_GETWHEELSCROLLLINES   104
# endif
#endif
#ifndef WS_EX_LAYERED
# define	WS_EX_LAYERED			0x80000
#endif
#ifndef LWA_ALPHA
# define	LWA_ALPHA				2
#endif
#ifndef LSFW_LOCK
# define	LSFW_LOCK				1
#endif
#ifndef LSFW_UNLOCK
# define	LSFW_UNLOCK				2
#endif
#include "winjmenu.h"
#include <ocidl.h>
#include <olectl.h>
#include <crtdbg.h>
#define HIMETRIC_INCH	2540

#define CUST_MENU					0

#define MAX_HISTORY					30
#define TITLE_LEN					48

static int WaitForChar __ARGS((int));
static int cbrk_handler __ARGS(());
static void delay __ARGS((int));
static void gotoxy __ARGS((int, int));
static void scroll __ARGS((void));
static void vbell __ARGS((void));
static void cursor_visible __ARGS((int));
static void clrscr __ARGS((void));
static void clreol __ARGS((void));
static void insline __ARGS((int));
static void delline __ARGS((int));
static void normvideo __ARGS((void));
static void textattr __ARGS((WORD));
static void putch __ARGS((char));
static int kbhit __ARGS((void));
static int tgetch __ARGS((void));
static void resizeConBufAndWindow __ARGS((HANDLE, long, long));
#ifndef notdef
static int isctlkey __ARGS((void));
#endif

/* Win32 Console handles for input and output */
HANDLE          hConIn;
HANDLE          hConOut;
static int		maxRows;
#ifndef notdef
static HANDLE	h_mainthread;
#endif
static BOOL		v_nt;
static BOOL		IsTelnet = FALSE;

/* Win32 Screen buffer,coordinate,console I/O information */
CONSOLE_SCREEN_BUFFER_INFO csbi;
COORD           ntcoord;
INPUT_RECORD    ir;

/* The attribute of the screen when the editor was started */
WORD            DefaultAttribute;

#define KEY_TIME		1
#define MOUSE_TIME		3
#define TRIPLE_TIME		4
#define SHOW_TIME		5
#define TAIL_TIME		6
#define WM_TASKTRAY		(WM_APP + 100)

#define KEY_REP			10		/* key repeat count */
#define KEY_REDRAW		7		/* redraw use count */

#if defined(KANJI) && defined(SYNTAX)
#define	CMODE			'*'
#else
#define	CMODE			0x80
#endif

static INT				iScrollLines			= 3;
#ifndef NO_WHEEL
static UINT				uiMsh_MsgMouseWheel		= 0;
static UINT				uiMsh_Msg3DSupport		= 0;
static UINT				uiMsh_MsgScrollLines	= 0;
static BOOL				f3DSupport				= 0;
#endif
static BOOL				bIClose					= FALSE;
static BOOL				bWClose					= FALSE;
static int				nowRows = 25;
static int				nowCols = 80;
static char				keybuf[128];
static char_u		*	cbuf					= keybuf;
static char				szAppName[16]			= "JVim";
static int				c_size					= sizeof(keybuf);
static int				c_end					= 0;
static int				c_next					= 0;
static int				c_ind					= 0;
static int				w_p_tw;
static int				w_p_wm;
static int				w_p_ai;
static int				w_p_si;
static int				w_p_et;
static int				w_p_uc;
static int				w_p_sm;
static int				w_p_ru;
static int				w_p_ri;
static int				w_p_paste;
	   HWND				hVimWnd	= NULL;
static HACCEL			hAcc = NULL;
static DWORD			config_x;
static DWORD			config_y;
static DWORD			config_w;
static DWORD			config_h;
static DWORD			config_sbar		= TRUE;
static DWORD			config_save		= FALSE;
static DWORD			config_comb		= FALSE;
static DWORD			config_unicode	= FALSE;
static DWORD			config_tray		= FALSE;
static DWORD			config_mouse	= FALSE;
#ifdef NT106KEY
static DWORD			config_nt106	= FALSE;
#endif
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
static DWORD			config_share	= TRUE;
static DWORD			config_common	= FALSE;
static HMENU			hCFile;
#endif
static DWORD			config_menu		= TRUE;
static LOGFONT			config_font;
#ifdef KANJI
static LOGFONT			config_jfont;
static BOOL				v_difffont		= FALSE;
static BOOL CALLBACK	FontDialogProc(HWND, UINT, WPARAM, LPARAM);
#endif
static DWORD			config_fgcolor	= RGB(  0,   0,   0);
static DWORD			config_bgcolor	= RGB(255, 255, 255);
static DWORD			config_tbcolor	= (-1);	/* bold */
static DWORD			config_socolor	= (-1);	/* standout */
static DWORD			config_ticolor	= (-1);	/* invert/reverse */
static DWORD			config_tbbitmap	= (-1);	/* bold */
static DWORD			config_sobitmap	= (-1);	/* standout */
static DWORD			config_tibitmap	= (-1);	/* invert/reverse */
static DWORD			config_color[16];
static char				config_printer[MAXPATHL];
static DWORD			config_bitmap	= FALSE;
static char				config_bitmapfile[MAXPATHL];
static DWORD			config_bitsize	= 100;
static DWORD			config_bitcenter= TRUE;
static DWORD			config_wave		= FALSE;
static char				config_wavefile[MAXPATHL];
#ifdef USE_BDF
extern void				GetBDFfont(HINSTANCE, int, char *, char *, int *, int *, DWORD *);
extern int				bdfTextOut(HDC, int, int, UINT, CONST RECT *, char *, UINT, CONST INT *, int, int, DWORD, DWORD, int, int);
static BOOL				config_bdf		= FALSE;
static char				config_bdffile[MAXPATHL];
static char				config_jbdffile[MAXPATHL];
static DWORD			config_fgbdf	= RGB(  0,   0,   0);
static DWORD			config_bgbdf	= RGB(255, 255, 255);
static int				v_bxchar	= 0;
static int				v_bychar	= 0;
#endif
static int				config_overflow = 3; 	/* larger than 2 */
static DWORD			config_show = 500;
static DWORD			config_fadeout = TRUE;
static BOOL				config_grepwin = TRUE;
#ifdef USE_HISTORY
static DWORD			config_history = TRUE;
static DWORD			config_hauto = TRUE;
static HMENU			hHist;
#endif
static char				config_load[CMDBUFFSIZE];
static char				config_unload[CMDBUFFSIZE];
static BOOL				config_ini	= FALSE;
static int				do_resize	= FALSE;
static BOOL				do_time		= FALSE;
static BOOL				do_vb		= FALSE;
static BOOL				do_trip		= FALSE;
static BOOL				do_drag		= FALSE;
static int				v_row		= 0;
static int				v_col		= 0;
static int				v_region	= 0;
static BOOL				v_cursor	= FALSE;
static BOOL				v_focus		= FALSE;
static int				v_caret		= 0;
static int				v_xchar		= 0;
static int				v_ychar		= 0;
static int				v_lspace	= 0;
static int				v_cspace	= 0;
static int				v_trans		= 0;
static DWORD		*	v_fgcolor	= &config_fgcolor;
static DWORD		*	v_bgcolor	= &config_bgcolor;
static DWORD		*	v_tbcolor	= &config_tbcolor;
static DWORD		*	v_socolor	= &config_socolor;
static DWORD		*	v_ticolor	= &config_ticolor;
static BOOL				v_ttfont;
static HFONT			v_font;
static INT			*	v_space		= NULL;
static short		*	v_char		= NULL;
static INT				v_ssize		= 0;
static HANDLE			hInst;
static HMENU			v_menu		= NULL;
static BOOL				v_extend	= FALSE;
static BOOL				v_macro		= FALSE;
static OSVERSIONINFO	ver_info;
static BOOL				do_msg		= FALSE;
static BOOL				bSyncPaint	= FALSE;
static HCURSOR			hIbeamCurs	= NULL;
static HCURSOR			hArrowCurs	= NULL;
static HCURSOR			hWaitCurs	= NULL;
static LPSTR			lpCurrCurs	= NULL;
static NOTIFYICONDATA	nIcon;
static BOOL CALLBACK	PrinterDialog(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK	BitmapDialog(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK	WaveDialog(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK	CommandDialog(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK	LoadDialog(HWND, UINT, WPARAM, LPARAM);
static void				LoadCommand();
static void				UnloadCommand();
static char *			DisplayPathName(char *, unsigned int);
static char 		*	HistoryGetMenu(int);
static char			*	HistoryGetCommand(int);
static void				HistoryRename(int, int);
static BOOL CALLBACK	LineSpaceDialog(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK	LineSpaceDialogEx(HWND, UINT, WPARAM, LPARAM);
static void				ScrollBar();
static int				isbitmap(char *, HWND);
static int				iswave(char *);
static BOOL				CopyScreenToBitmap(HDC hDC, BOOL force);
static BOOL				LoadBitmapFromBMPFile(HDC hDC, LPTSTR szFileName);
static void				SetLayerd(void);

typedef HWND			(WINAPI *tCreateUpDownControl)(DWORD, int, int, int, int, HWND, int, HINSTANCE, HWND, int, int, int);
static	tCreateUpDownControl		pCreateUpDownControl		= NULL;

typedef DWORD			(WINAPI *tSetLayeredWindowAttributes)(HWND, DWORD, BYTE, DWORD);
static	tSetLayeredWindowAttributes	pSetLayeredWindowAttributes	= NULL;

typedef BOOL			(WINAPI *tAllowSetForegroundWindow)(DWORD);
static	tAllowSetForegroundWindow	pAllowSetForegroundWindow	= NULL;

typedef	BOOL			(WINAPI *tLockSetForegroundWindow)(UINT);
static	tLockSetForegroundWindow	pLockSetForegroundWindow	= NULL;

typedef struct filelist
{
	char_u        **file;
	int             nfiles;
	int             maxfiles;
} FileList;

static void		addfile __ARGS((FileList *, char *, int));
#ifdef __BORLANDC__
static int      pstrcmp();      /* __ARGS((char **, char **)); BCC does not
								 * like this */
#else
static int      pstrcmp __ARGS((const void *, const void *));
#endif
static void		strlowcpy __ARGS((char *, char *));
static int		expandpath __ARGS((FileList *, char *, int, int, int));

static int      cbrk_pressed = FALSE;   /* set by ctrl-break interrupt */
static int      ctrlc_pressed = FALSE;  /* set when ctrl-C or ctrl-break
										 * detected */
static BOOL		syntax_on();

#if defined(KANJI) && defined(SYNTAX)
# define istrans()		((config_bitmap || (syntax_on() && !v_ttfont)) ? TRUE : FALSE)
# ifdef USE_BDF
#  define italicplus()	((config_bitmap || (syntax_on() && !v_ttfont && !config_bdf)) ? 1 : 0)
#  define issynpaint()	(syntax_on() && !v_ttfont && !config_bdf && !config_bitmap)
# else
#  define italicplus()	((config_bitmap || (syntax_on() && !v_ttfont)) ? 1 : 0)
#  define issynpaint()	(syntax_on() && !v_ttfont && !config_bitmap)
# endif
#else
# define istrans()		config_bitmap
# define italicplus()	(0)
#endif
#ifdef KANJI
# define iskanakan(c)	(ISkanji(c) ? 1 : (ISkana(c) ? 2 : 0))
#endif

#if defined(KANJI) && defined(SYNTAX)
/*
 *
 */
static BOOL
syntax_on()
{
	WIN		*	wp;

	wp = firstwin;
	while (wp != NULL)
	{
		if (wp->w_p_syt)
			return(TRUE);
		wp = wp->w_next;
	}
	return(FALSE);
}
#endif

/*
 *
 */
static VOID
LoadConfig(BOOL init)
{
	HKEY		hKey;
	DWORD		size;
	DWORD		type;
	int			openkey = FALSE;
	char		name[_MAX_PATH];

	if (BenchTime)
		goto error;
	while (init)		/* ini file */
	{
		char			szIniFile[_MAX_PATH];	/* private profile file name  */
		char			szSecName[32];		/* private profile section name  */
		char			color[128];
		char		*	p;
		char		*	last;
		HWND			hWnd;
		DWORD			rgb[3];

		if (strcmp(GuiIni, "reg") == 0)
			break;
		if ((p = STRCHR(GuiIni, ':')) != NULL && getperm(p + 1) != (-1))
		{
			ZeroMemory(szSecName, sizeof(szSecName));
			strncpy(szSecName, GuiIni, p - GuiIni);
			strcpy(szIniFile, p + 1);
		}
		else
		{
			strcpy(szSecName, GuiIni);
			if (GetModuleFileName(NULL, szIniFile, _MAX_PATH) == 0)
				break;
			last = p = szIniFile + 3;	/* drive + : + \ */
			while (*p)
			{
				if (*p == '.')
					last = p + 1;
				p++;
			}
			*last = '\0';
			lstrcpy(last, "ini");
		}
		if (getperm(szIniFile) == (-1))
			break;
		config_ini = TRUE;
		hWnd = CreateDialog(hInst, "LOAD", NULL, LoadDialog);
		ShowWindow(hWnd, SW_NORMAL);
		Sleep(1000);
		/* get parameter */
		GetPrivateProfileString(szSecName, "printer", "",
							config_printer, sizeof(config_printer), szIniFile);
		config_unicode = GetPrivateProfileInt(szSecName, "unicode", FALSE, szIniFile);
		Columns = GetPrivateProfileInt(szSecName, "cols", 80, szIniFile);
		Rows = GetPrivateProfileInt(szSecName, "rows", 25, szIniFile);
		config_sbar = GetPrivateProfileInt(szSecName, "scrollbar", TRUE, szIniFile);
		config_menu = GetPrivateProfileInt(szSecName, "menu", TRUE, szIniFile);
		GetPrivateProfileString(szSecName, "bitmap", "",
					config_bitmapfile, sizeof(config_bitmapfile), szIniFile);
		if (!isbitmap(config_bitmapfile, NULL))
			config_bitmapfile[0] = '\0';
		else
			config_bitmap = TRUE;
		GetPrivateProfileString(szSecName, "wave", "",
						config_wavefile, sizeof(config_wavefile), szIniFile);
		if (!iswave(config_wavefile))
			config_wavefile[0] = '\0';
		else
			config_wave = TRUE;
		config_fadeout
				= GetPrivateProfileInt(szSecName, "fadeout", 1, szIniFile);
		config_grepwin
				= GetPrivateProfileInt(szSecName, "grepwin", 1, szIniFile);
#ifdef USE_HISTORY
		config_history	= FALSE;
		config_hauto	= FALSE;
		GetPrivateProfileString(szSecName, "history", "auto",
										color, sizeof(color), szIniFile);
		if (strcmp("auto", color) == 0)
		{
			config_history	= TRUE;
			config_hauto	= TRUE;
		}
		else if (strcmp("on", color) == 0)
			config_history	= TRUE;
#endif
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
		config_share	= FALSE;
		config_common	= FALSE;
		GetPrivateProfileString(szSecName, "share", "on",
										color, sizeof(color), szIniFile);
		if (strcmp("compatible", color) == 0 || strcmp("common", color) == 0)
		{
			config_share	= TRUE;
			config_common	= TRUE;
		}
		else if (strcmp("on", color) == 0)
			config_share	= TRUE;
#endif
#ifdef USE_BDF
		GetPrivateProfileString(szSecName, "bdffile", "",
						config_bdffile, sizeof(config_bdffile), szIniFile);
		GetPrivateProfileString(szSecName, "jbdffile", "",
						config_jbdffile, sizeof(config_jbdffile), szIniFile);
		config_bdf = GetPrivateProfileInt(szSecName, "bdf", FALSE, szIniFile);
#endif
		config_fgcolor = RGB(0, 0, 0);
		GetPrivateProfileString(szSecName, "textcolor", "",
										color, sizeof(color), szIniFile);
		if (strcmp("white", color) == 0)
			config_fgcolor = RGB(255, 255, 255);
		else if (strcmp("black", color) == 0)
			config_fgcolor = RGB(0, 0, 0);
		else if (strcmp("blue", color) == 0)
			config_fgcolor = RGB(0, 0, 128);
		else
		{
			sscanf(color, "%d,%d,%d", &rgb[0], &rgb[1], &rgb[2]);
			config_fgcolor = RGB(rgb[0], rgb[1], rgb[2]);
		}
		config_bgcolor = RGB(255, 255, 255);
		GetPrivateProfileString(szSecName, "backcolor", "",
										color, sizeof(color), szIniFile);
		if (strcmp("white", color) == 0)
			config_bgcolor = RGB(255, 255, 255);
		else if (strcmp("black", color) == 0)
			config_bgcolor = RGB(0, 0, 0);
		else if (strcmp("blue", color) == 0)
			config_bgcolor = RGB(0, 0, 128);
		else
		{
			sscanf(color, "%d,%d,%d", &rgb[0], &rgb[1], &rgb[2]);
			config_bgcolor = RGB(rgb[0], rgb[1], rgb[2]);
		}
		config_font.lfHeight
				= GetPrivateProfileInt(szSecName, "fontsize", 14, szIniFile);
		{
			HDC         hDC;
			INT			PointSize = GetPrivateProfileInt(szSecName, "fontsize", 14, szIniFile);

			hDC = GetDC(hWnd);
			config_font.lfHeight =
						-MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			ReleaseDC(hWnd, hDC);
		}
		config_font.lfWidth			= 0;
		config_font.lfEscapement	= 0;
		config_font.lfOrientation	= 0;
		config_font.lfWeight		= 0;
		config_font.lfItalic		= 0;
		config_font.lfUnderline		= 0;
		config_font.lfStrikeOut		= 0;
		/* config_font.lfCharSet	= OEM_CHARSET; */
		config_font.lfCharSet		= 0;
		config_font.lfOutPrecision	= OUT_DEFAULT_PRECIS;
		config_font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		config_font.lfQuality		= DEFAULT_QUALITY;
		config_font.lfPitchAndFamily= FIXED_PITCH | FF_MODERN;
		GetPrivateProfileString(szSecName, "fontname", "FixedSys",
			config_font.lfFaceName, sizeof(config_font.lfFaceName), szIniFile);
#ifdef KANJI
		GetPrivateProfileString(szSecName, "jfontname", "",
			config_jfont.lfFaceName, sizeof(config_jfont.lfFaceName), szIniFile);
		if (config_jfont.lfFaceName[0] == '\0')
			memcpy(&config_jfont, &config_font, sizeof(config_jfont));
		else
		{
			config_jfont.lfHeight
					= GetPrivateProfileInt(szSecName, "jfontsize", 14, szIniFile);
			{
				HDC         hDC;
				INT			PointSize = GetPrivateProfileInt(szSecName, "jfontsize", 14, szIniFile);

				hDC = GetDC(hWnd);
				config_jfont.lfHeight =
							-MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
				ReleaseDC(hWnd, hDC);
			}
			config_jfont.lfWidth		= 0;
			config_jfont.lfEscapement	= 0;
			config_jfont.lfOrientation	= 0;
			config_jfont.lfWeight		= 0;
			config_jfont.lfItalic		= 0;
			config_jfont.lfUnderline	= 0;
			config_jfont.lfStrikeOut	= 0;
			config_jfont.lfOutPrecision	= OUT_DEFAULT_PRECIS;
			config_jfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			config_jfont.lfQuality		= DEFAULT_QUALITY;
			config_jfont.lfPitchAndFamily= FIXED_PITCH | FF_MODERN;
		}
		/* config_jfont.lfCharSet	= OEM_CHARSET; */
		config_jfont.lfCharSet		= SHIFTJIS_CHARSET;
#endif
		v_lspace = GetPrivateProfileInt(szSecName, "linespace", 0, szIniFile);
		if (v_lspace > 10)
			v_lspace = 10;
		v_cspace = GetPrivateProfileInt(szSecName, "charspace", 0, szIniFile);
		if (v_cspace > 10)
			v_cspace = 10;
		config_x = CW_USEDEFAULT;
		config_y = CW_USEDEFAULT;
		config_w = CW_USEDEFAULT;
		config_h = CW_USEDEFAULT;
		DestroyWindow(hWnd);
		return;
	}
	/*
	 *	Common Registory
	 */
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Vim", 0,
										KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		goto error;
	openkey = TRUE;
	size = sizeof(config_printer);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "printer", NULL, &type, (BYTE *)&config_printer, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_unicode);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "unicode", NULL, &type, (BYTE *)&config_unicode, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tray);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "tray", NULL, &type, (BYTE *)&config_tray, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_mouse);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "mouse", NULL, &type, (BYTE *)&config_mouse, &size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef NT106KEY
	size = sizeof(config_nt106);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "nt106", NULL, &type, (BYTE *)&config_nt106, &size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_menu);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "menu", NULL, &type, (BYTE *)&config_menu, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_sbar);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "scrollbar", NULL, &type, (BYTE *)&config_sbar, &size)
															!= ERROR_SUCCESS)
		goto error;
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
	size = sizeof(config_share);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "share", NULL, &type, (BYTE *)&config_share, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_common);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "common", NULL, &type, (BYTE *)&config_common, &size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_fadeout);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "fadeout", NULL, &type, (BYTE *)&config_fadeout, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_grepwin);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "grepwin", NULL, &type, (BYTE *)&config_grepwin, &size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef USE_HISTORY
	size = sizeof(config_history);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "history", NULL, &type, (BYTE *)&config_history, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_hauto);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "hauto", NULL, &type, (BYTE *)&config_hauto, &size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_color);
	type = REG_BINARY;
	if (RegQueryValueEx(hKey, "custcolor", NULL, &type, (BYTE *)config_color, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_overflow);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "overflow", NULL, &type, (BYTE *)&config_overflow, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_show);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "show", NULL, &type, (BYTE *)&config_show, &size)
															!= ERROR_SUCCESS)
		goto error;
	/*
	 *	Original Registory
	 */
	RegCloseKey(hKey);
	openkey = FALSE;
	sprintf(name, "Software\\Vim\\%d", GuiConfig);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
									KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		goto error;
	openkey = TRUE;
	size = sizeof(config_w);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "width", NULL, &type, (BYTE *)&config_w, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_h);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "height", NULL, &type, (BYTE *)&config_h, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(Columns);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "cols", NULL, &type, (BYTE *)&Columns, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(Rows);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "rows", NULL, &type, (BYTE *)&Rows, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_fgcolor);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "color-fg", NULL, &type, (BYTE *)&config_fgcolor, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bgcolor);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "color-bg", NULL, &type, (BYTE *)&config_bgcolor, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tbcolor);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "color-tb", NULL, &type, (BYTE *)&config_tbcolor, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_socolor);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "color-so", NULL, &type, (BYTE *)&config_socolor, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_ticolor);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "color-ti", NULL, &type, (BYTE *)&config_ticolor, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tbbitmap);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bitmap-tb", NULL, &type, (BYTE *)&config_tbbitmap, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_sobitmap);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bitmap-so", NULL, &type, (BYTE *)&config_sobitmap, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tibitmap);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bitmap-ti", NULL, &type, (BYTE *)&config_tibitmap, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_font);
	type = REG_BINARY;
	if (RegQueryValueEx(hKey, "font", NULL, &type, (BYTE *)&config_font, &size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef KANJI
	size = sizeof(config_jfont);
	type = REG_BINARY;
	if (RegQueryValueEx(hKey, "jfont", NULL, &type, (BYTE *)&config_jfont, &size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(v_lspace);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "lspace", NULL, &type, (BYTE *)&v_lspace, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(v_cspace);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "cspace", NULL, &type, (BYTE *)&v_cspace, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(v_trans);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "trans", NULL, &type, (BYTE *)&v_trans, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitmap);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bitmap", NULL, &type, (BYTE *)&config_bitmap, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitsize);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bitsize", NULL, &type, (BYTE *)&config_bitsize, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitcenter);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bitcenter", NULL, &type, (BYTE *)&config_bitcenter, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitmapfile);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "bitmapfile", NULL, &type, (BYTE *)config_bitmapfile, &size)
															!= ERROR_SUCCESS)
		goto error;
	if (!isbitmap(config_bitmapfile, NULL))
	{
		config_bitmap = FALSE;
		config_bitmapfile[0] = '\0';
	}
	size = sizeof(config_wave);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "wave", NULL, &type, (BYTE *)&config_wave, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_wavefile);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "wavefile", NULL, &type, (BYTE *)&config_wavefile, &size)
															!= ERROR_SUCCESS)
		goto error;
	if (!iswave(config_wavefile))
	{
		config_wave = FALSE;
		config_wavefile[0] = '\0';
	}
#ifdef USE_BDF
	size = sizeof(config_bdf);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bdf", NULL, &type, (BYTE *)&config_bdf, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bdffile);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "bdffile", NULL, &type, (BYTE *)&config_bdffile, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_jbdffile);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "bdffilej", NULL, &type, (BYTE *)&config_jbdffile, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_fgbdf);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bdf-fg", NULL, &type, (BYTE *)&config_fgbdf, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bgbdf);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "bdf-bg", NULL, &type, (BYTE *)&config_bgbdf, &size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_comb);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "comb", NULL, &type, (BYTE *)&config_comb, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_load);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "load", NULL, &type, (BYTE *)&config_load, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_unload);
	type = REG_SZ;
	if (RegQueryValueEx(hKey, "unload", NULL, &type, (BYTE *)&config_unload, &size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_save);
	type = REG_DWORD;
	if (RegQueryValueEx(hKey, "save", NULL, &type, (BYTE *)&config_save, &size)
															!= ERROR_SUCCESS)
		goto error;
	if (config_save)
	{
		size = sizeof(config_x);
		type = REG_DWORD;
		if (RegQueryValueEx(hKey, "posx", NULL, &type, (BYTE *)&config_x, &size)
															!= ERROR_SUCCESS)
			goto error;
		size = sizeof(config_y);
		type = REG_DWORD;
		if (RegQueryValueEx(hKey, "posy", NULL, &type, (BYTE *)&config_y, &size)
															!= ERROR_SUCCESS)
			goto error;
		if (config_w > (DWORD)GetSystemMetrics(SM_CXSCREEN))
			config_x = 1;
		if (config_h > (DWORD)GetSystemMetrics(SM_CYSCREEN))
			config_y = 1;
		if (config_x > 0x7fffffff)
			config_x = 1;
		if ((config_x & 0x7fffffff) > (DWORD)GetSystemMetrics(SM_CXSCREEN))
			config_x = 1;
		if (config_y > 0x7fffffff)
			config_y = 1;
		if ((config_y & 0x7fffffff) > (DWORD)GetSystemMetrics(SM_CYSCREEN))
			config_y = 1;
	}
	else
	{
		config_x = CW_USEDEFAULT;
		config_y = CW_USEDEFAULT;
	}
	RegCloseKey(hKey);
	return;
error:
	if (openkey)
		RegCloseKey(hKey);
	if (!init && !BenchTime)
		return;
	Columns	= 80;
	Rows	= 25;
	config_x = CW_USEDEFAULT;
	config_y = CW_USEDEFAULT;
	if (BenchTime)
		config_x = config_y = 1;
	config_w = CW_USEDEFAULT;
	config_h = CW_USEDEFAULT;

	v_cspace		= 0;
	v_lspace		= 0;
	v_trans			= 0;
	config_sbar		= TRUE;
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
	config_share	= TRUE;
	config_common	= FALSE;
#endif
	config_fadeout	= TRUE;
	config_grepwin	= TRUE;
#ifdef USE_HISTORY
	config_history	= TRUE;
	config_hauto	= TRUE;
#endif
	config_save		= FALSE;
	config_comb		= FALSE;
	config_load[0]	= '\0';
	config_unload[0]= '\0';
	config_unicode	= FALSE;
	config_tray		= FALSE;
	config_mouse	= FALSE;
#ifdef NT106KEY
	config_nt106	= TRUE;
#endif
	config_menu		= TRUE;
	config_font.lfHeight		= 14;
	config_font.lfWidth			= 0;
	config_font.lfEscapement	= 0;
	config_font.lfOrientation	= 0;
	config_font.lfWeight		= 0;
	config_font.lfItalic		= 0;
	config_font.lfUnderline		= 0;
	config_font.lfStrikeOut		= 0;
	/* config_font.lfCharSet	= OEM_CHARSET; */
	config_font.lfCharSet		= SHIFTJIS_CHARSET;
	config_font.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	config_font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	config_font.lfQuality		= DEFAULT_QUALITY;
	config_font.lfPitchAndFamily= FIXED_PITCH | FF_MODERN;
	strcpy(config_font.lfFaceName, "FixedSys");
#ifdef KANJI
	config_jfont.lfHeight		= 14;
	config_jfont.lfWidth		= 0;
	config_jfont.lfEscapement	= 0;
	config_jfont.lfOrientation	= 0;
	config_jfont.lfWeight		= 0;
	config_jfont.lfItalic		= 0;
	config_jfont.lfUnderline	= 0;
	config_jfont.lfStrikeOut	= 0;
	/* config_jfont.lfCharSet	= OEM_CHARSET; */
	config_jfont.lfCharSet		= SHIFTJIS_CHARSET;
	config_jfont.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	config_jfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	config_jfont.lfQuality		= DEFAULT_QUALITY;
	config_jfont.lfPitchAndFamily= FIXED_PITCH | FF_MODERN;
	strcpy(config_jfont.lfFaceName, "FixedSys");
#endif
	config_fgcolor	= RGB(  0,   0,   0);
	config_bgcolor	= RGB(255, 255, 255);
	config_tbcolor	= (-1);
	config_socolor	= (-1);
	config_ticolor	= (-1);
	config_tbbitmap	= (-1);
	config_sobitmap	= (-1);
	config_tibitmap	= (-1);
	config_printer[0] = '\0';
	config_bitmap	= FALSE;
	config_bitsize	= 100;
	config_bitcenter= TRUE;
	config_bitmapfile[0] = '\0';
	if (BenchTime && GuiConfig)
	{
		sprintf(name, "Software\\Vim\\%d", GuiConfig);
		if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
		{
			size = sizeof(config_bitmap);
			type = REG_DWORD;
			RegQueryValueEx(hKey, "bitmap", NULL, &type, (BYTE *)&config_bitmap, &size);
			size = sizeof(config_bitsize);
			type = REG_DWORD;
			RegQueryValueEx(hKey, "bitsize", NULL, &type, (BYTE *)&config_bitsize, &size);
			size = sizeof(config_bitcenter);
			type = REG_DWORD;
			RegQueryValueEx(hKey, "bitcenter", NULL, &type, (BYTE *)&config_bitcenter, &size);
			size = sizeof(config_bitmapfile);
			type = REG_SZ;
			RegQueryValueEx(hKey, "bitmapfile", NULL, &type, (BYTE *)config_bitmapfile, &size);
			if (!isbitmap(config_bitmapfile, NULL))
			{
				config_bitmap = FALSE;
				config_bitmapfile[0] = '\0';
			}
			RegCloseKey(hKey);
		}
	}
	config_wave		= FALSE;
	config_wavefile[0] = '\0';
#ifdef USE_BDF
	config_bdf		= FALSE;
	config_bdffile[0]	= '\0';
	config_jbdffile[0]	= '\0';
	config_fgbdf	= RGB(  0,   0,   0);
	config_bgbdf	= RGB(255, 255, 255);
	if (BenchTime && GuiConfig)
	{
		sprintf(name, "Software\\Vim\\%d", GuiConfig);
		if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
		{
			size = sizeof(config_bdf);
			type = REG_DWORD;
			RegQueryValueEx(hKey, "bdf", NULL, &type, (BYTE *)&config_bdf, &size);
			size = sizeof(config_bdffile);
			type = REG_SZ;
			RegQueryValueEx(hKey, "bdffile", NULL, &type, (BYTE *)&config_bdffile, &size);
			size = sizeof(config_jbdffile);
			type = REG_SZ;
			RegQueryValueEx(hKey, "bdffilej", NULL, &type, (BYTE *)&config_jbdffile, &size);
			RegCloseKey(hKey);
		}
	}
#endif
	config_overflow	= 3;
	config_show		= 500;
	if (BenchTime)
	{
		char_u	font[] = {	0xf5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
							0x03, 0x02, 0x01, 0x31, 0x82, 0x6c, 0x82, 0x72,
							0x20, 0x83, 0x53, 0x83, 0x56, 0x83, 0x62, 0x83,
							0x4e, 0x00, 0x00, 0x00, 0x54, 0x67, 0x02, 0x00,
							0xdb, 0x0f, 0x78, 0x7b, 0x2f, 0x13, 0x97, 0x27,
							0x00, 0xa0, 0x00, 0x00};
		memcpy(&config_font, font, sizeof(config_font));
#ifdef KANJI
		memcpy(&config_jfont, font, sizeof(config_jfont));
#endif
	}
}

static VOID
SaveConfig()
{
	HKEY		hKey;
	DWORD		size;
	int			openkey = FALSE;
	RECT		rcWindow;
	char		name[_MAX_PATH];

	if (BenchTime || config_ini)
		return;
	if (GetWindowRect(hVimWnd, &rcWindow))
	{
		config_x = rcWindow.left;
		config_y = rcWindow.top;
	}
	else
	{
		config_x = CW_USEDEFAULT;
		config_y = CW_USEDEFAULT;
	}
	/*
	 *	Common Registory
	 */
	if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Vim", 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &size)
															!= ERROR_SUCCESS)
		goto error;
	openkey = TRUE;
#ifdef KANJI
	{
		size = strlen(longJpVersion) + 1;
		if (RegSetValueEx(hKey, NULL, 0, REG_SZ, longJpVersion, size)
															!= ERROR_SUCCESS)
			goto error;
	}
#endif
	size = strlen(config_printer) + 1;
	if (RegSetValueEx(hKey, "printer", 0, REG_SZ, (BYTE *)&config_printer, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_unicode);
	if (RegSetValueEx(hKey, "unicode", 0, REG_DWORD, (BYTE *)&config_unicode, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tray);
	if (RegSetValueEx(hKey, "tray", 0, REG_DWORD, (BYTE *)&config_tray, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_mouse);
	if (RegSetValueEx(hKey, "mouse", 0, REG_DWORD, (BYTE *)&config_mouse, size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef NT106KEY
	size = sizeof(config_nt106);
	if (RegSetValueEx(hKey, "nt106", 0, REG_DWORD, (BYTE *)&config_nt106, size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_menu);
	if (RegSetValueEx(hKey, "menu", 0, REG_DWORD, (BYTE *)&config_menu, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_sbar);
	if (RegSetValueEx(hKey, "scrollbar", 0, REG_DWORD, (BYTE *)&config_sbar, size)
															!= ERROR_SUCCESS)
		goto error;
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
	size = sizeof(config_share);
	if (RegSetValueEx(hKey, "share", 0, REG_DWORD, (BYTE *)&config_share, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_common);
	if (RegSetValueEx(hKey, "common", 0, REG_DWORD, (BYTE *)&config_common, size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_fadeout);
	if (RegSetValueEx(hKey, "fadeout", 0, REG_DWORD, (BYTE *)&config_fadeout, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_grepwin);
	if (RegSetValueEx(hKey, "grepwin", 0, REG_DWORD, (BYTE *)&config_grepwin, size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef USE_HISTORY
	size = sizeof(config_history);
	if (RegSetValueEx(hKey, "history", 0, REG_DWORD, (BYTE *)&config_history, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_hauto);
	if (RegSetValueEx(hKey, "hauto", 0, REG_DWORD, (BYTE *)&config_hauto, size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_color);
	if (RegSetValueEx(hKey, "custcolor", 0, REG_BINARY, (BYTE *)config_color, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_overflow);
	if (RegSetValueEx(hKey, "overflow", 0, REG_DWORD, (BYTE *)&config_overflow, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_show);
	if (RegSetValueEx(hKey, "show", 0, REG_DWORD, (BYTE *)&config_show, size)
															!= ERROR_SUCCESS)
		goto error;
	/*
	 *	Original Registory
	 */
	RegCloseKey(hKey);
	openkey = FALSE;
	sprintf(name, "Software\\Vim\\%d", GuiConfig);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, name, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &size)
														!= ERROR_SUCCESS)
		goto error;
	openkey = TRUE;
	{
		GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, name, sizeof(name));
		strcat(name, " ");
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, &name[strlen(name)], sizeof(name) - strlen(name));
		size = strlen(name) + 1;
		if (RegSetValueEx(hKey, NULL, 0, REG_SZ, name, size) != ERROR_SUCCESS)
			goto error;
	}
	size = sizeof(config_w);
	if (RegSetValueEx(hKey, "width", 0, REG_DWORD, (BYTE *)&config_w, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_h);
	if (RegSetValueEx(hKey, "height", 0, REG_DWORD, (BYTE *)&config_h, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(Columns);
	if (RegSetValueEx(hKey, "cols", 0, REG_DWORD, (BYTE *)&Columns, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(Rows);
	if (RegSetValueEx(hKey, "rows", 0, REG_DWORD, (BYTE *)&Rows, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_fgcolor);
	if (RegSetValueEx(hKey, "color-fg", 0, REG_DWORD, (BYTE *)&config_fgcolor, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bgcolor);
	if (RegSetValueEx(hKey, "color-bg", 0, REG_DWORD, (BYTE *)&config_bgcolor, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tbcolor);
	if (RegSetValueEx(hKey, "color-tb", 0, REG_DWORD, (BYTE *)&config_tbcolor, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_socolor);
	if (RegSetValueEx(hKey, "color-so", 0, REG_DWORD, (BYTE *)&config_socolor, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_ticolor);
	if (RegSetValueEx(hKey, "color-ti", 0, REG_DWORD, (BYTE *)&config_ticolor, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tbbitmap);
	if (RegSetValueEx(hKey, "bitmap-tb", 0, REG_DWORD, (BYTE *)&config_tbbitmap, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_sobitmap);
	if (RegSetValueEx(hKey, "bitmap-so", 0, REG_DWORD, (BYTE *)&config_sobitmap, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_tibitmap);
	if (RegSetValueEx(hKey, "bitmap-ti", 0, REG_DWORD, (BYTE *)&config_tibitmap, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_font);
	if (RegSetValueEx(hKey, "font", 0, REG_BINARY, (BYTE *)&config_font, size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef KANJI
	size = sizeof(config_jfont);
	if (RegSetValueEx(hKey, "jfont", 0, REG_BINARY, (BYTE *)&config_jfont, size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(v_lspace);
	if (RegSetValueEx(hKey, "lspace", 0, REG_DWORD, (BYTE *)&v_lspace, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(v_cspace);
	if (RegSetValueEx(hKey, "cspace", 0, REG_DWORD, (BYTE *)&v_cspace, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(v_trans);
	if (RegSetValueEx(hKey, "trans", 0, REG_DWORD, (BYTE *)&v_trans, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitmap);
	if (RegSetValueEx(hKey, "bitmap", 0, REG_DWORD, (BYTE *)&config_bitmap, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitsize);
	if (RegSetValueEx(hKey, "bitsize", 0, REG_DWORD, (BYTE *)&config_bitsize, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bitcenter);
	if (RegSetValueEx(hKey, "bitcenter", 0, REG_DWORD, (BYTE *)&config_bitcenter, size)
															!= ERROR_SUCCESS)
		goto error;
	size = strlen(config_bitmapfile) + 1;
	if (RegSetValueEx(hKey, "bitmapfile", 0, REG_SZ, (BYTE *)config_bitmapfile, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_wave);
	if (RegSetValueEx(hKey, "wave", 0, REG_DWORD, (BYTE *)&config_wave, size)
															!= ERROR_SUCCESS)
		goto error;
	size = strlen(config_wavefile) + 1;
	if (RegSetValueEx(hKey, "wavefile", 0, REG_SZ, (BYTE *)&config_wavefile, size)
															!= ERROR_SUCCESS)
		goto error;
#ifdef USE_BDF
	size = sizeof(config_bdf);
	if (RegSetValueEx(hKey, "bdf", 0, REG_DWORD, (BYTE *)&config_bdf, size)
															!= ERROR_SUCCESS)
		goto error;
	size = strlen(config_bdffile) + 1;
	if (RegSetValueEx(hKey, "bdffile", 0, REG_SZ, (BYTE *)&config_bdffile, size)
															!= ERROR_SUCCESS)
		goto error;
	size = strlen(config_jbdffile) + 1;
	if (RegSetValueEx(hKey, "bdffilej", 0, REG_SZ, (BYTE *)&config_jbdffile, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_fgbdf);
	if (RegSetValueEx(hKey, "bdf-fg", 0, REG_DWORD, (BYTE *)&config_fgbdf, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_bgbdf);
	if (RegSetValueEx(hKey, "bdf-bg", 0, REG_DWORD, (BYTE *)&config_bgbdf, size)
															!= ERROR_SUCCESS)
		goto error;
#endif
	size = sizeof(config_comb);
	if (RegSetValueEx(hKey, "comb", 0, REG_DWORD, (BYTE *)&config_comb, size)
															!= ERROR_SUCCESS)
		goto error;
	size = strlen(config_load) + 1;
	if (RegSetValueEx(hKey, "load", 0, REG_SZ, (BYTE *)&config_load, size)
															!= ERROR_SUCCESS)
		goto error;
	size = strlen(config_unload) + 1;
	if (RegSetValueEx(hKey, "unload", 0, REG_SZ, (BYTE *)&config_unload, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_save);
	if (RegSetValueEx(hKey, "save", 0, REG_DWORD, (BYTE *)&config_save, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_x);
	if (RegSetValueEx(hKey, "posx", 0, REG_DWORD, (BYTE *)&config_x, size)
															!= ERROR_SUCCESS)
		goto error;
	size = sizeof(config_y);
	if (RegSetValueEx(hKey, "posy", 0, REG_DWORD, (BYTE *)&config_y, size)
															!= ERROR_SUCCESS)
		goto error;
	RegCloseKey(hKey);
	return;
error:
	if (openkey)
		RegCloseKey(hKey);
}

static BOOL
ResetScreen(HWND hWnd)
{
	HDC         hDC;
	TEXTMETRIC  tm;

	if (NULL != v_font)
		DeleteObject(v_font);

	v_font = CreateFontIndirect(&config_font);

	hDC = GetDC(hWnd);
	SelectObject(hDC, v_font);
	GetTextMetrics(hDC, &tm);
	ReleaseDC(hWnd, hDC);

	if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
		v_ttfont = TRUE;
	else
		v_ttfont = FALSE;
	if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
		v_xchar = (tm.tmMaxCharWidth * 1 + tm.tmAveCharWidth * 2) / 3;
	else
		v_xchar = tm.tmAveCharWidth;
	v_ychar = tm.tmHeight + tm.tmExternalLeading;
#ifdef KANJI
	{
		DeleteObject(v_font);
		v_font = CreateFontIndirect(&config_jfont);

		hDC = GetDC(hWnd);
		SelectObject(hDC, v_font);
		GetTextMetrics(hDC, &tm);
		ReleaseDC(hWnd, hDC);

		v_difffont = memcmp(&config_font, &config_jfont, sizeof(config_font));

		if (tm.tmPitchAndFamily & TMPF_TRUETYPE)
			v_ttfont = TRUE;
		if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
			v_xchar = (tm.tmMaxCharWidth * 1 + tm.tmAveCharWidth * 2) / 3 > v_xchar ? (tm.tmMaxCharWidth * 1 + tm.tmAveCharWidth * 2) / 3 : v_xchar;
		else
			v_xchar = tm.tmAveCharWidth > v_xchar ? tm.tmAveCharWidth : v_xchar;
		v_ychar = tm.tmHeight + tm.tmExternalLeading > v_ychar ? tm.tmHeight + tm.tmExternalLeading : v_ychar;

		DeleteObject(v_font);
		v_font = CreateFontIndirect(&config_font);
	}
#endif
#ifdef USE_BDF
	if (config_bdf)
	{
		v_xchar = v_bxchar;
		v_ychar = v_bychar;
	}
#endif
	v_xchar += v_cspace;
	v_ychar += v_lspace;

	do_resize = TRUE;
	if (v_cursor && v_focus)
		HideCaret(hWnd);
	v_caret = 0;
	return(TRUE);
}

static VOID
MoveCursor(HWND hWnd)
{
	int			width;

	if (v_cursor && v_focus)
	{
#ifdef KANJI
		if (WinScreen == NULL || v_row >= Rows)
			width = 1;
		else
			width = ISkanji(*(WinScreen[v_row] + v_col)) ? 2 : 1;
#else
		width = 1;
#endif
		if (width != v_caret)
		{
			CreateCaret(hWnd, NULL, width * (v_xchar - v_cspace), v_ychar - v_lspace);
			ShowCaret(hWnd);
			v_caret = width;
		}
	}
	SetCaretPos(v_col * v_xchar, v_row * v_ychar);
}

static INT
WindowSize(HWND hWnd, WORD wVertSize, WORD wHorzSize)
{
	RECT		rcClient;
	int			i;

	if (!IsIconic(hWnd) && GetClientRect(hWnd, &rcClient))
	{
		do_resize = TRUE;
		nowRows = (rcClient.bottom - rcClient.top) / v_ychar;
		nowCols = (rcClient.right - rcClient.left) / v_xchar;
	}
	else
	{
		nowRows = Rows;
		nowCols = Columns;
	}
	if (nowCols > v_ssize)
	{
		v_ssize = nowCols;
		free(v_space);
		v_space = malloc(sizeof(INT) * nowCols);
		free(v_char);
		v_char = malloc(sizeof(short) * nowCols);
	}
	for (i = 0; i < v_ssize; i++)
		v_space[i] = v_xchar;
	return(0);
}

#ifdef KANJI
static void
SetFontType(char_u *c, char_u mode, HDC hDC, HFONT *phOldFont)
{
	LOGFONT			logfont;

# ifdef USE_BDF
	if (config_bdf)
		return;
# endif
	if (c != NULL && iskanakan(*c))
		memcpy(&logfont, &config_jfont, sizeof(logfont));
	else
		memcpy(&logfont, &config_font, sizeof(logfont));
	SelectObject(hDC, *phOldFont);
	if (mode >= 0x80)
	{
			 if (0x80 <= mode && mode <= 0x9f)	mode = 1;
		else if (0xa0 <= mode && mode <= 0xbf)	mode = 2;
		else if (0xc0 <= mode && mode <= 0xdf)	mode = 3;
		else if (0xe0 <= mode && mode <= 0xff)	mode = 4;
		else									mode = 0;
	}
	if (NULL != v_font)
		DeleteObject(v_font);
	switch (mode) {
	case 1:
		logfont.lfItalic		= 0;
		logfont.lfUnderline		= 0;
		logfont.lfWeight		= FW_BOLD;
		break;
	case 2:
		logfont.lfItalic		= 1;
		logfont.lfUnderline		= 0;
		logfont.lfWeight		= FW_NORMAL;
		break;
	case 3:
		logfont.lfItalic		= 0;
		logfont.lfUnderline		= 1;
		logfont.lfWeight		= FW_NORMAL;
		break;
	case 4:
		logfont.lfItalic		= 1;
		logfont.lfUnderline		= 0;
		logfont.lfWeight		= FW_BOLD;
		break;
	default:
		logfont.lfItalic		= 0;
		logfont.lfUnderline		= 0;
		logfont.lfWeight		= FW_NORMAL;
		break;
	}
	v_font = CreateFontIndirect(&logfont);
	*phOldFont = SelectObject(hDC, v_font);
}

static DWORD
GetColor(char_u mode, int tb)
{
	int			color;
	DWORD		red;
	DWORD		green;
	DWORD		blue;
	DWORD		rgb;
	char_u	*	p = NULL;

	if (tb == 't')
		rgb = *v_fgcolor;
	else
		rgb = *v_bgcolor;
	if (mode >= 0x80)
	{
			 if (0x80 <= mode && mode <= 0x9f) mode -= 0x40;
		else if (0xa0 <= mode && mode <= 0xbf) mode -= 0x60;
		else if (0xc0 <= mode && mode <= 0xdf) mode -= 0x80;
		else if (0xe0 <= mode && mode <= 0xff) mode -= 0xa0;
	}
	switch (mode & 0x7f) {
	case 'b':	/* bold */
		if (*v_tbcolor != (-1))
		{
			if (tb == 't')
				rgb = *v_tbcolor;
			else
				rgb = *v_socolor;
		}
		else
			p = T_TB;
		break;
	case 's':	/* standout */
		if (*v_socolor != (-1))
		{
			if (tb == 't')
				rgb = *v_socolor;
			else
				rgb = *v_tbcolor;
		}
		else
			p = T_SO;
		break;
#if defined(KANJI) && defined(SYNTAX)
	case 'A':	/* text */
		break;
	case 'B':	/* white */
		if (tb == 't')
			rgb = RGB(0xff, 0xff, 0xff);
		break;
	case 'C':	/* black */
		if (tb == 't')
			rgb = RGB(0x00, 0x00, 0x00);
		break;
	case 'D':	/* red */
		if (tb == 't')
			rgb = RGB(0xff, 0x00, 0x00);
		break;
	case 'E':	/* green */
		if (tb == 't')
			rgb = RGB(0x00, 0x80, 0x00);
		break;
	case 'F':	/* blue */
		if (tb == 't')
			rgb = RGB(0x00, 0x00, 0xff);
		break;
	case 'G':	/* yellow */
		if (tb == 't')
			rgb = RGB(0xff, 0xff, 0x00);
		break;
	case 'H':	/* fuchsia */
		if (tb == 't')
			rgb = RGB(0xff, 0x00, 0xff);
		break;
	case 'I':	/* silver */
		if (tb == 't')
			rgb = RGB(0xc0, 0xc0, 0xc0);
		break;
	case 'J':	/* gold */
		if (tb == 't')
			rgb = RGB(0x80, 0x80, 0x00);
		break;
	case 'K':	/* lime green */
		if (tb == 't')
			rgb = RGB(0x00, 0xff, 0x00);
		break;
	case 'L':	/* navy */
		if (tb == 't')
			rgb = RGB(0x00, 0x00, 0x80);
		break;
	case 'M':	/* aqua */
		if (tb == 't')
			rgb = RGB(0x00, 0xff, 0xff);
		break;
	case 'N':	/* gray */
		if (tb == 't')
			rgb = RGB(0x80, 0x80, 0x80);
		break;
	case 'O':	/* maroon */
		if (tb == 't')
			rgb = RGB(0x80, 0x00, 0x00);
		break;
	case 'P':	/* olive */
		if (tb == 't')
			rgb = RGB(0x80, 0x80, 0x00);
		break;
	case 'Q':	/* purple */
		if (tb == 't')
			rgb = RGB(0x80, 0x00, 0x00);
		break;
	case 'R':	/* teal */
		if (tb == 't')
			rgb = RGB(0x00, 0x80, 0x80);
		break;
	case '[': case '\\': case ']': case '^': case '_':
	case 'V': case 'W': case 'X': case 'Y': case 'Z':
		if (tb == 't')
			rgb = syn_user_color(mode);
		break;
	case '@':	/* reverse */
		if (!config_bitmap)
		{
			if (tb == 't')
				rgb = *v_bgcolor;
			else
				rgb = *v_fgcolor;
			break;
		}
		/* no break */
#endif
	default:	/* invert/reverse */
		if (*v_ticolor != (-1))
		{
			if (tb == 't')
				rgb = *v_ticolor;
			else
				rgb = *v_tbcolor;
		}
		else
			p = T_TI;
		break;
	}
	if (p != NULL && *p != NUL)
	{
		p += 2;
		color = getdigits(&p);
		if (tb != 't')
			color = (color & 0xf0) >> 4;
		red = green = blue = 0;
		if (color & FOREGROUND_BLUE)
			blue = 255;
		if (color & FOREGROUND_GREEN)
			green = 255;
		if (color & FOREGROUND_RED)
			red = 255;
		if ((color & FOREGROUND_INTENSITY) == 0)
		{
			blue	= (blue  == 0 ? 0 : 160);
			green	= (green == 0 ? 0 : 160);
			red		= (red   == 0 ? 0 : 160);
		}
		rgb = RGB(red, green, blue);
	}
	return(rgb);
}
#endif

static void
PrintChar(HDC hdc, RECT *rt, HFONT *phOldFont, char_u *p, int size, char_u mode)
{
#if defined(KANJI) && defined(SYNTAX)
	HBRUSH		hbrush;
	HBRUSH		holdbrush;
	DWORD		color;

	if (issynpaint())
	{
		if (mode)
		{
			if (do_vb)
				color = GetColor(mode, 't');
			else
				color = GetColor(mode, 'b');
		}
		else
		{
			if (do_vb)
				color = *v_fgcolor;
			else
				color = *v_bgcolor;
		}
		if ((!do_vb && *v_bgcolor != color) || (do_vb && *v_fgcolor != color))
		{
			hbrush	= CreateSolidBrush(color);
			holdbrush = SelectObject(hdc, hbrush);
			FillRect(hdc, rt, hbrush);
			SelectObject(hdc, holdbrush);
			DeleteObject(hbrush);
		}
	}
	if (syntax_on())
		SetFontType(p, mode, hdc, phOldFont);
	else if (v_difffont)
	{
		SelectObject(hdc, *phOldFont);
		if (NULL != v_font)
			DeleteObject(v_font);
		if (p != NULL && iskanakan(*p))
			v_font = CreateFontIndirect(&config_jfont);
		else
			v_font = CreateFontIndirect(&config_font);
		*phOldFont = SelectObject(hdc, v_font);
	}
#endif
#ifdef USE_BDF
	if (config_bdf)
		bdfTextOut(hdc, rt->left, rt->top,
					0, rt, p, size, v_space,
					config_bitmap, GetBkColor(hdc) == *v_bgcolor ? FALSE : TRUE,
					GetTextColor(hdc), GetBkColor(hdc), v_cspace, v_lspace);
	else
#endif
	if (config_unicode)
	{
		if (ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			int				i;
			int			*	w = v_space;

			for (i = 0; i < size; i++, w++)
			{
				if (ISkanji(p[i]))
				{
					*w = v_xchar * 2;
					i++;
				}
				else
					*w = v_xchar;
			}
			size = MultiByteToWideChar(p_cpage, MB_PRECOMPOSED,
												p, size, v_char, v_ssize);
			ExtTextOutW(hdc, rt->left, rt->top,
					istrans() ? 0 : ETO_OPAQUE, rt, v_char, size, v_space);
		}
		else if (v_ttfont)
		{
			int				i;
			int			*	w = v_space;
			LONG			left;

			for (i = 0; i < size; i++, w++)
			{
				if (ISkanji(p[i]))
				{
					*w = v_xchar * 2;
					i++;
				}
				else
					*w = v_xchar;
			}
			w = v_space;
			size = MultiByteToWideChar(p_cpage, MB_PRECOMPOSED,
												p, size, v_char, v_ssize);
			for (i = 0, left = rt->left; i < size; left += *w, i++, w++)
			{
				rt->left  = left;
				rt->right = left + *w;
				ExtTextOutW(hdc, left, rt->top,
						istrans() ? 0 : ETO_OPAQUE, rt, &v_char[i], 1, w);
			}
		}
		else
		{
			size = MultiByteToWideChar(p_cpage, MB_PRECOMPOSED,
												p, size, v_char, v_ssize);
			ExtTextOutW(hdc, rt->left, rt->top,
						istrans() ? 0 : ETO_OPAQUE, rt, v_char, size, NULL);
		}
	}
	else
		ExtTextOutA(hdc, rt->left, rt->top,
					istrans() ? 0 : ETO_OPAQUE, rt, p, size, v_space);
}

static BOOL
PaintWindow(HWND hWnd)
{
	int				nRow, nCol, nEndRow, nEndCol;
	HDC				hDC;
	HFONT			hOldFont;
	PAINTSTRUCT		ps;
	RECT			rect;
	char_u		*	p;
	int				caret = FALSE;

	if (GetUpdateRect(hWnd, &rect, FALSE) != TRUE)
		return(0);

	hDC		= BeginPaint(hWnd, &ps);
	rect	= ps.rcPaint;
	hOldFont= SelectObject(hDC, v_font);

	if (config_bitmap && LoadBitmapFromBMPFile(hDC, config_bitmapfile))
		SetBkMode(hDC, TRANSPARENT);
#if defined(KANJI) && defined(SYNTAX)
	else if (issynpaint())
		SetBkMode(hDC, TRANSPARENT);
#endif
	else
		SetBkMode(hDC, OPAQUE);

	if (!screen_valid())
		goto no_draw;

	nRow	= min(Rows - 1, max(0, rect.top / v_ychar));
	nEndRow	= min(Rows - 1, ((rect.bottom - 1) / v_ychar));
#ifdef KANJI
	nCol	= min(Columns - 1, max(0, (rect.left - v_xchar) / v_xchar));
#else
	nCol	= min(Columns - 1, max(0, rect.left / v_xchar));
#endif
	nEndCol	= min(Columns - 1, ((rect.right - 1) / v_xchar));
	if (v_cursor && v_focus)
	{
		HideCaret(hWnd);
		caret = TRUE;
	}
#if defined(KANJI) && defined(SYNTAX)
	if (syntax_on() && !v_ttfont
# ifdef USE_BDF
			&& !config_bdf
# endif
			&& !config_bitmap)
	{
		HBRUSH		hbrush;
		HBRUSH		holdbrush;

		rect.top	= nRow * v_ychar;
		rect.bottom	= nEndRow * v_ychar + v_ychar;
		rect.left	= nCol * v_xchar;
		rect.right	= nEndCol * v_xchar + v_xchar;
		if (do_vb)
			hbrush	= CreateSolidBrush(*v_fgcolor);
		else
			hbrush	= CreateSolidBrush(*v_bgcolor);
		holdbrush = SelectObject(hDC, hbrush);
		FillRect(hDC, &rect, hbrush);
		SelectObject(hDC, holdbrush);
		DeleteObject(hbrush);
	}
#endif
	for (; nRow <= nEndRow; nRow++)
	{
		int			i;
		int			i0;
		char_u		attr;
#ifdef KANJI
		int			kanji;
#endif

		rect.top	= nRow * v_ychar;
		rect.bottom	= nRow * v_ychar + v_ychar;
		rect.left	= nCol * v_xchar;

		p = WinScreen[nRow];
		attr = 0xff;
#ifdef KANJI
		kanji = iskanakan(p[nCol]);
#endif
		for (i = i0 = nCol; i <= nEndCol; i++)
		{
#ifdef KANJI
			if (attr != p[Columns + i] || kanji != iskanakan(p[i]))
#else
			if (attr != (p[i] & 0x80))
#endif
			{
#ifdef KANJI
				if (p[Columns + i0])
#else
				if (p[i0] & 0x80)
#endif
				{
#ifdef KANJI
					if (do_vb)
					{
						SetTextColor(hDC, GetColor(p[Columns + i0], 'b'));
						SetBkColor(hDC, GetColor(p[Columns + i0], 't'));
					}
					else
					{
						SetTextColor(hDC, GetColor(p[Columns + i0], 't'));
						SetBkColor(hDC, GetColor(p[Columns + i0], 'b'));
					}
#else
					if (do_vb)
					{
						SetTextColor(hDC, *v_fgcolor);
						SetBkColor(hDC, *v_bgcolor);
					}
					else
					{
						SetTextColor(hDC, *v_bgcolor);
						SetBkColor(hDC, *v_fgcolor);
					}
#endif
				}
				else
				{
					if (do_vb)
					{
						SetTextColor(hDC, *v_bgcolor);
						SetBkColor(hDC, *v_fgcolor);
					}
					else
					{
						SetTextColor(hDC, *v_fgcolor);
						SetBkColor(hDC, *v_bgcolor);
					}
				}
				if ((i - i0) > 0)
				{
					rect.right	= i * v_xchar;
#ifdef KANJI
					if ((ISkanjiPosition(p, i0 + 1) == 2)
									&& (ISkanjiPosition(p, i) == 1))
					{
						rect.left  = (i0 + 1) * v_xchar;
						rect.right = (i + 1) * v_xchar;
						PrintChar(hDC, &rect, &hOldFont, p + i0 + 1, i - i0, p[Columns + i0]);
					}
					else if (ISkanjiPosition(p, i0 + 1) == 2)
					{
						rect.left  = (i0 + 1) * v_xchar;
						if ((i - (i0 + 1)) > 0)
							PrintChar(hDC, &rect, &hOldFont, p + i0 + 1, i - (i0 + 1), p[Columns + i0]);
					}
					else if (ISkanjiPosition(p, i) == 1)
					{
						rect.right = (i + 1) * v_xchar;
						PrintChar(hDC, &rect, &hOldFont, p + i0, (i - i0) + 1, p[Columns + i0]);
					}
					else
#endif
					PrintChar(hDC, &rect, &hOldFont, p + i0, i - i0, p[Columns + i0]);
					rect.left	= i * v_xchar;
					i0 = i;
				}
#ifdef KANJI
				attr = p[Columns + i];
				kanji = iskanakan(p[i]);
#else
				attr = (p[i] & 0x80);
#endif
			}
		}
		if ((i - i0) > 0)
		{
#ifdef KANJI
			if (p[Columns + i0])
#else
			if (p[i0] & 0x80)
#endif
			{
#ifdef KANJI
				if (do_vb)
				{
					SetTextColor(hDC, GetColor(p[Columns + i0], 'b'));
					SetBkColor(hDC, GetColor(p[Columns + i0], 't'));
				}
				else
				{
					SetTextColor(hDC, GetColor(p[Columns + i0], 't'));
					SetBkColor(hDC, GetColor(p[Columns + i0], 'b'));
				}
#else
				if (do_vb)
				{
					SetTextColor(hDC, *v_fgcolor);
					SetBkColor(hDC, *v_bgcolor);
				}
				else
				{
					SetTextColor(hDC, *v_bgcolor);
					SetBkColor(hDC, *v_fgcolor);
				}
#endif
			}
			else
			{
				if (do_vb)
				{
					SetTextColor(hDC, *v_bgcolor);
					SetBkColor(hDC, *v_fgcolor);
				}
				else
				{
					SetTextColor(hDC, *v_fgcolor);
					SetBkColor(hDC, *v_bgcolor);
				}
			}
			rect.right	= i * v_xchar;
#ifdef KANJI
			if ((ISkanjiPosition(p, i0 + 1) == 2)
							&& (ISkanjiPosition(p, i) == 1))
			{
				rect.left  = (i0 + 1) * v_xchar;
				rect.right = (i + 1) * v_xchar;
				PrintChar(hDC, &rect, &hOldFont, p + i0 + 1, i - i0, p[Columns + i0]);
			}
			else if (ISkanjiPosition(p, i0 + 1) == 2)
			{
				rect.left  = (i0 + 1) * v_xchar;
				if ((i - (i0 + 1)) > 0)
					PrintChar(hDC, &rect, &hOldFont, p + i0 + 1, i - (i0 + 1), p[Columns + i0]);
			}
			else if (ISkanjiPosition(p, i) == 1)
			{
				rect.right = (i + 1) * v_xchar;
				PrintChar(hDC, &rect, &hOldFont, p + i0, (i - i0) + 1, p[Columns + i0]);
			}
			else
#endif
			PrintChar(hDC, &rect, &hOldFont, p + i0, i - i0, p[Columns + i0]);
		}
	}
#if defined(KANJI) && defined(SYNTAX)
	if (syntax_on())
		SetFontType(NULL, 0, hDC, &hOldFont);
#endif
no_draw:
	SelectObject(hDC, hOldFont);
	EndPaint(hWnd, &ps);
	if (caret == TRUE)
		ShowCaret(hWnd);
	return(0);
}

static BOOL
keybuf_chk(area)
int			area;
{
	char		*	p;

	if ((area + c_end) >= c_size)
	{
		if ((p = alloc(c_end + ((area / sizeof(keybuf)) + 1) * sizeof(keybuf))) != NULL)
		{
			memcpy(p, cbuf, c_size);
			if (cbuf != keybuf)
				free(cbuf);
			cbuf = p;
			c_size = c_end + ((area / sizeof(keybuf)) + 1) * sizeof(keybuf);
			return(TRUE);
		}
		return(FALSE);
	}
	return(TRUE);
}

static WIN *
get_linecol(LPARAM lParam, FPOS *pos, int *row, int *col)
{
	WIN		*	wp;
	int			x;
	int			y;
	int			i;
	int			j;
	linenr_t	lnum;

	*col = x = min(Columns - 1, max(0, LOWORD(lParam) / v_xchar));
	*row = y = min(Rows - 1, (HIWORD(lParam) - 1) / v_ychar);
#ifdef KANJI
	if (ISkanjiPosition(WinScreen[y], x + 1) == 2)
		x--;
#endif
	wp = firstwin;
	pos->lnum = 0;
	pos->col = MAXCOL;
	while (wp != NULL)
	{
		if (wp->w_winpos <= y && y < (wp->w_winpos + wp->w_height))
			break;
		wp = wp->w_next;
	}
	if (wp == NULL)
		return(NULL);
	if (wp->w_p_wrap)			/* long line wrapping, adjust curwin->w_row */
	{
		lnum = wp->w_topline;
		for (i = wp->w_winpos; i < (wp->w_winpos + wp->w_height); )
		{
			if (wp->w_buffer->b_ml.ml_line_count < lnum)
				break;
			j = plines_win(wp, lnum);
			if (i <= y && y < (i + j))
			{
				j = Columns * (y - i) + x - (wp->w_p_nu ? 8 : 0);
				if (j < 0)
					j = 0;
				pos->col = vcol2col(wp, lnum, j, NULL, 0, 0);
				pos->lnum = lnum;
				return(wp);
			}
			i += j;
			lnum ++;
		}
	}
	else
	{
		lnum = wp->w_topline + (y - wp->w_winpos);
		j = x - (wp->w_p_nu ? 8 : 0) + curwin->w_leftcol;
		if (j < 0)
			j = 0;
		pos->col = vcol2col(wp, lnum, j, NULL, 0, 0);
		pos->lnum = lnum;
	}
	return(wp);
}

static WIN *
get_statusline(LPARAM lParam, int *row)
{
	WIN		*	wp;
	int			y;

	*row = y = min(Rows - 1, (HIWORD(lParam) - 1) / v_ychar);
	wp = firstwin;
	while (wp != NULL)
	{
		if (y == (wp->w_winpos + wp->w_height))
			break;
		wp = wp->w_next;
	}
	if (wp == NULL || wp == lastwin)
		return(NULL);
	return(wp);
}

static VOID
cursor_refresh(HWND hWnd)
{
	adjust_cursor();
	cursupdate();
	if (VIsual.lnum)
		updateScreen(INVERTED);
	if (must_redraw)
		updateScreen(must_redraw);
	showruler(FALSE);
	setcursor();
	cursor_on();
	flushbuf();
	MoveCursor(hWnd);
}

static VOID
clear_visual(HWND hWnd)
{
	if (VIsual.lnum != 0)
	{
		VIsual.lnum = 0;
		Visual_block = FALSE;
		updateScreen(NOT_VALID);
		cursor_refresh(hWnd);
	}
}

static VOID
clear_cmode(HWND hWnd)
{
	int				i;
	int				j;
	char_u		*	p;
	RECT			rect;

	for (i = 0; i < Rows; i++)
	{
		p = WinScreen[i];
		for (j = 0; j < Columns; j++)
#if defined(KANJI) && defined(SYNTAX)
			p[Columns + j]  =  0;
#else
			p[Columns + j] &= ~CMODE;
#endif
	}
	rect.left	= 0;
	rect.right	= Columns * v_xchar;
	rect.top	= 0;
	rect.bottom	= Rows * v_ychar;
	InvalidateRect(hWnd, &rect, FALSE);
}

static VOID
draw_cmode(HWND hWnd, int cs_row, int cs_col, int ce_row, int ce_col)
{
	int				nRow, nCol, nEndRow, nEndCol;
	int				i;
	int				j;
	char_u		*	p;
	RECT			rect;

	clear_cmode(hWnd);
	nRow	= min(cs_row, ce_row);
	nEndRow	= max(cs_row, ce_row);
	nCol	= min(cs_col, ce_col);
	nEndCol	= max(cs_col, ce_col);
	for (i = nRow; i <= nEndRow; i++)
	{
		p = WinScreen[i];
		for (j = nCol; j <= nEndCol; j++)
		{
#ifdef KANJI
			if (j == nCol)
			{
				if (ISkanjiPosition(p, j + 1) == 2)
# ifdef SYNTAX
					p[Columns + j - 1]  = CMODE;
# else
					p[Columns + j - 1] |= CMODE;
# endif
			}
			else if (j == nEndCol)
			{
				if (ISkanjiPosition(p, j + 1) == 1)
# ifdef SYNTAX
					p[Columns + j - 1]  = CMODE;
# else
					p[Columns + j + 1] |= CMODE;
# endif
			}
#endif
#if defined(KANJI) && defined(SYNTAX)
			p[Columns + j]  = CMODE;
#else
			p[Columns + j] |= CMODE;
#endif
		}
	}
	rect.left	= nCol * v_xchar;
	rect.right	= (nEndCol + 1) * v_xchar - 1;
	rect.top	= nRow * v_ychar;
	rect.bottom	= (nEndRow + 1) * v_ychar - 1;
	InvalidateRect(hWnd, &rect, FALSE);
}

static VOID
yank_cmode(HWND hWnd, BOOL clip)
{
	HANDLE			hClipData;
	char		*	lpClipData;
	int				i;
	int				j;
	char_u		*	p;
	char_u		*	ptr;
	int				num = 0;
	int				line = 0;
	BOOL			flg;

	for (i = 0; i < Rows; i++)
	{
		flg = FALSE;
		p = WinScreen[i];
		for (j = 0; j < Columns; j++)
		{
#if defined(KANJI) && defined(SYNTAX)
			if (p[Columns + j] == CMODE)
#else
			if (p[Columns + j] & CMODE)
#endif
			{
				num++;
				flg = TRUE;
			}
		}
		if (flg)
			line++;
	}
	if (num == 0)
		return;
	if (clip)
	{
		hClipData = GlobalAlloc(GMEM_MOVEABLE, num + (line * 2));
		if (hClipData == NULL)
			return;
		if ((lpClipData = GlobalLock(hClipData)) == NULL)
		{
			GlobalFree(hClipData);
			return;
		}
		ptr = lpClipData;
		for (i = 0; i < Rows; i++)
		{
			flg = FALSE;
			p = WinScreen[i];
			for (j = 0; j < Columns; j++)
			{
#if defined(KANJI) && defined(SYNTAX)
				if (p[Columns + j] == CMODE)
#else
				if (p[Columns + j] & CMODE)
#endif
				{
					*ptr++ = p[j];
					flg = TRUE;
				}
			}
			if (flg)
			{
				*ptr++ = '\r';
				*ptr++ = '\n';
			}
		}
		ptr[-2] = '\0';
		GlobalUnlock(hClipData);
		if (OpenClipboard(hWnd) == FALSE)
		{
			GlobalFree(hClipData);
			return;
		}
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hClipData);
		CloseClipboard();
	}
	else
	{
		if (!keybuf_chk(num + line))
			return;
		for (i = 0; i < Rows; i++)
		{
			flg = FALSE;
			p = WinScreen[i];
			for (j = 0; j < Columns; j++)
			{
#if defined(KANJI) && defined(SYNTAX)
				if (p[Columns + j] == CMODE)
#else
				if (p[Columns + j] & CMODE)
#endif
				{
					cbuf[c_end++] = p[j];
					flg = TRUE;
				}
			}
			if (flg)
				cbuf[c_end++] = '\n';
		}
		cbuf[--c_end] = '\0';
	}
}

static void
TopWindow(HWND hWnd)
{
	if ((ver_info.dwPlatformId != VER_PLATFORM_WIN32_NT
				&& ver_info.dwMajorVersion == 4
				&& ver_info.dwMinorVersion >= 10)
		|| (ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
				&& ver_info.dwMajorVersion >= 5))
	{		/* Windows 98 later or Windows 2000 later */
		SetForegroundWindow(hWnd);
		if (GetForegroundWindow() != hWnd)
		{
			ShowWindow(hWnd, SW_MINIMIZE);
			OpenIcon(hWnd);
		}
	}
	else
		SetForegroundWindow(hWnd);
}

LRESULT FAR PASCAL
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	extern void			start_arrow __ARGS((void));
	static DWORD		repeat = 0;
	static BOOL			state_shift	= FALSE;
	static BOOL			state_ctrl	= FALSE;
	static WIN		*	selwin		= NULL;
	static WIN		*	selstatus	= NULL;
	static FPOS			selpos;
	static int			updown	= 0;
	static int			leftright = 0;
	static BOOL			vmode	= FALSE;
	static BOOL			cmode	= FALSE;
	static int			cs_row	= 0;
	static int			cs_col	= 0;
	static int			ce_row	= 0;
	static int			ce_col	= 0;
	static LPARAM		mouse_pos;
	static BOOL			s_cursor	= TRUE;
	static DWORD		oldmix	= 0;
	static FILETIME		byFile;
	static char			filter[] = "ALL\0*.*\0C\0*.cpp;*.h;*.def;*.rc\0DOC\0*.txt;*.doc\0";
	OPENFILENAME		ofn;
	CHOOSECOLOR			cfColor;
	COPYDATASTRUCT	*	cds;
	DWORD			*	pColor;
	HANDLE				hClipData;
	char			*	lpClipData;
	char_u			*	p;
	INT					i;
	INT					files;
	WIN				*	wp;
	BUF				*	buf;
	FPOS				pos;
	int					row;
	int					col;
	int					more;
	BOOL				redraw	= FALSE;
	RECT				rcWindow;
	HMENU				hEdit;
	HMENU				hMenu;
	POINT				point;
	BOOL				bClear;
	int					rc;
	LOGFONT				logfont;

	switch (uMsg) {
	case WM_CREATE:
		nIcon.cbSize	= sizeof(NOTIFYICONDATA);
		nIcon.uID		= 1;
		nIcon.hWnd		= hWnd;
		nIcon.uFlags	= NIF_MESSAGE|NIF_ICON|NIF_TIP;
		nIcon.hIcon		= LoadIcon(hInst, "vim");
		nIcon.uCallbackMessage = WM_TASKTRAY;
		DragAcceptFiles(hWnd, TRUE);
		ResetScreen(hWnd);
		MoveCursor(hWnd);
		return(0);
	case WM_TASKTRAY:
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
			Shell_NotifyIcon(NIM_DELETE, &nIcon);
			ShowWindow(hWnd, SW_SHOW);
			OpenIcon(hWnd);
			SetForegroundWindow(hWnd);
			break;
		case WM_RBUTTONUP:
			SetForegroundWindow(hWnd);
			GetCursorPos(&point);
			hEdit = CreatePopupMenu();
			AppendMenu(hEdit,  MF_STRING,   IDM_OPEN,  "&Open");
			AppendMenu(hEdit,  MF_STRING,   IDM_EXIT,  "&Exit");
			AppendMenu(hEdit,  MF_SEPARATOR,0,			NULL);
			AppendMenu(hEdit,  MF_STRING,   IDM_CANCEL, "&Cancel");
			TrackPopupMenu(hEdit, TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
								point.x, point.y, 0, hWnd, NULL);
			DestroyMenu(hEdit);
			break;
		}
		return(0);
	case WM_ERASEBKGND:
#ifdef USE_BDF
		if (config_bdf)
		{
			HBRUSH		hbrush;
			HBRUSH		holdbrush;
			RECT		rcWindow;

			GetWindowRect(hWnd, &rcWindow);
			hbrush	= CreateSolidBrush(*v_bgcolor);
			holdbrush = SelectObject((HDC)wParam, hbrush);
			FillRect((HDC)wParam, &rcWindow, hbrush);
			SelectObject((HDC)wParam, holdbrush);
			DeleteObject(hbrush);
		}
#endif
		return(1);
	case WM_PAINT:
		return(PaintWindow(hWnd));
	case WM_SIZE:
		return(WindowSize(hWnd, HIWORD(lParam), LOWORD(lParam)));
	case WM_SETCURSOR:
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
		if (ef_share_busy())
		{
			SetCursor(hWaitCurs);
			lpCurrCurs = IDC_WAIT;
		}
		else
#endif
		if (LOWORD(lParam) == HTCLIENT && lpCurrCurs != IDC_IBEAM)
		{
			SetCursor(hIbeamCurs);
			lpCurrCurs = IDC_IBEAM;
		}
		else if (LOWORD(lParam) != HTCLIENT && lpCurrCurs != IDC_ARROW)
		{
			SetCursor(hArrowCurs);
			lpCurrCurs = IDC_ARROW;
		}
		break;
	case WM_CHAR:
		if (cmode)
		{
			if (LOBYTE(wParam) == Ctrl('C'))
				yank_cmode(hWnd, TRUE);
			clear_cmode(hWnd);
#if defined(KANJI) && defined(SYNTAX)
			if (syntax_on())
				updateScreen(CLEAR);
#endif
			cmode = FALSE;
			if (LOBYTE(wParam) == Ctrl('C'))
			{
				ctrlc_pressed = FALSE;
				return(0);
			}
		}
		if (s_cursor && config_mouse
			&& (((State & NORMAL) && strchr("aAiIoOR", LOBYTE(wParam)) != NULL)
								|| ((State & INSERT) && LOBYTE(wParam) != ESC)))
		{
			s_cursor = FALSE;
			ShowCursor(FALSE);
		}
		else if (!s_cursor && (State & INSERT) && LOBYTE(wParam) == ESC)
		{
			s_cursor = TRUE;
			ShowCursor(TRUE);
		}
		if (!keybuf_chk(2))
			return(0);
		switch (LOBYTE(wParam)) {
		case Ctrl('^'):
		case Ctrl('@'):
			/* already processed on WM_KEYDOWN */
			break;
		default:
#ifdef FEPCTRL
			if (!(State & NORMAL)
					&& (curbuf->b_p_fc && (p_fk != NULL && STRRCHR(p_fk, ESC+'@') != NULL)))
			/* shift + space key special routine */
			{
				static int		kanji = 0;

				if (kanji)
				{
					if (LOBYTE(wParam) == 0x40)
					{
						if (fep_get_mode())
							fep_force_off();
						else
							fep_force_on();
					}
					else
					{
						cbuf[c_end++] = kanji;
						cbuf[c_end++] = LOBYTE(wParam);
					}
					kanji = 0;
				}
				else if (LOBYTE(wParam) == ' ' && state_shift)
				{
					if (fep_get_mode())
						fep_force_off();
					else
						fep_force_on();
				}
				else if (LOBYTE(wParam) == 0x81 && state_shift)
					kanji = 0x81;
				else
					cbuf[c_end++] = LOBYTE(wParam);
			}
			else
#endif
			if (repeat && (config_overflow < 3))
				cbuf[c_end++] = LOBYTE(wParam);
			else if (repeat && (LOBYTE(wParam) == 'j'
										|| LOBYTE(wParam) == 'k'
										|| LOBYTE(wParam) == Ctrl('N')
										|| LOBYTE(wParam) == Ctrl('P')
										|| LOBYTE(wParam) == Ctrl('E')
										|| LOBYTE(wParam) == Ctrl('Y')))
			{
				i = 1;
				if ((p_sj > 1)
						&& ((curwin->w_winpos + curwin->w_height - 1 <= v_row)
												|| (curwin->w_winpos == v_row)))
					;
				else if (curwin->w_height <= 3)
					;
				else if ((curwin->w_winpos + curwin->w_height - 1 <= v_row)
												|| (curwin->w_winpos == v_row))
				{
					repeat++;
					i = repeat / KEY_REP + (repeat > 3 ? 2 : 1);
					if (i > curwin->w_p_scroll)
						i = curwin->w_p_scroll;
					if (i > config_overflow)
						i = config_overflow;
				}
				if (i == 0)
					;
				else if (vpeekc() != NUL)
				{
					if (c_next == 0)
						;
					else if ((c_end - c_next) <= config_overflow)
						i = 1;
					else
					{
						if (i > 3)
							config_overflow = --i;
						i = 0;		/* key buffer overflow */
					}
				}
				else
				{
					if (i == config_overflow && curwin->w_height > 10 && c_next == 0)
						config_overflow = ++i;
					else if ((c_end - c_next) <= config_overflow)
					{
						col = config_overflow - (c_end - c_next);
						i = i > col ? col : i;
					}
					else
						i = 0;		/* key buffer overflow */
				}
				while (i--)
				{
					if (!keybuf_chk(1))
						break;
					cbuf[c_end++] = LOBYTE(wParam);
				}
			}
			else if (repeat && (!curwin->w_p_wrap || (p_ww & 4))
							&& (LOBYTE(wParam) == 'h' || LOBYTE(wParam) == 'l'))
			{
				repeat++;
				if (c_next != 0 && vpeekc() != NUL)
					;
				else if (repeat > KEY_REP)
					cbuf[c_end++] = LOBYTE(wParam);
				cbuf[c_end++] = LOBYTE(wParam);
			}
			else if (repeat && isalpha(LOBYTE(wParam))
					&& (c_end - c_next) && (cbuf[c_end - 1] == LOBYTE(wParam)))
				/* key repeat cancel */;
			else
				cbuf[c_end++] = LOBYTE(wParam);
			break;
		}
		return(0);
	case WM_SYSKEYDOWN:
		if (lParam & (1 << 29))		/* ALT (Menu) Key Down */
			break;
		/* no break */
	case WM_KEYDOWN:
		if ((State & NORMAL) && (lParam & 0x40000000))
		{
			if (repeat == 0)
				repeat = 1;
		}
		else
			repeat = 0;
		if (wParam == VK_SHIFT)
			state_shift	= TRUE;
		else if (wParam == VK_CONTROL)
			state_ctrl	= TRUE;
		else if (wParam == VK_NUMLOCK || wParam == VK_CAPITAL)
			;
		else if (state_ctrl)
		{
			if (GetKeyState(VK_CONTROL) & 0x8000)
			{
				if (!keybuf_chk(2))
					return(0);
				switch (wParam) {
				case '6':
				case 0xde: /* '^' key */
					cbuf[c_end++] = '^' & 0x1f;
					break;
				case 'c':
				case 'C':
					ctrlc_pressed = TRUE;
					break;
				case '@':
				case 0xc0: /* '@' key */
					cbuf[c_end++] = K_ZERO;
					break;
				case VK_LEFT:
					cbuf[c_end++] = K_NUL;
					cbuf[c_end++] = 's';
					break;
				case VK_RIGHT:
					cbuf[c_end++] = K_NUL;
					cbuf[c_end++] = 't';
					break;
				}
			}
		}
		else if (state_shift)
		{
			if (!keybuf_chk(2))
				return(0);
			switch (wParam) {
			case VK_LEFT:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 's'; break;
			case VK_RIGHT:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 't'; break;
			case VK_F1:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'T'; break;
			case VK_F2:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'U'; break;
			case VK_F3:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'V'; break;
			case VK_F4:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'W'; break;
			case VK_F5:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'X'; break;
			case VK_F6:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'Y'; break;
			case VK_F7:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'Z'; break;
			case VK_F8:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '['; break;
			case VK_F9:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '\\'; break;
			case VK_F10:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = ']'; break;
			}
		}
		else
		{
			if (repeat && (config_overflow < 3))
				;
			else if (repeat && (wParam == VK_UP || wParam == VK_DOWN))
			{
				if ((p_sj > 1)
						&& ((curwin->w_winpos + curwin->w_height - 1 <= v_row)
												|| (curwin->w_winpos == v_row)))
					;
				else if (curwin->w_height <= 3)
					;
				else
				{
					repeat++;
					i = repeat / KEY_REP + 1;
					if (i > curwin->w_p_scroll)
						i = curwin->w_p_scroll;
					if (i > config_overflow)
						i = config_overflow;
					if (vpeekc() != NUL)
					{
						if (c_next == 0)
							;
						else if ((c_end - c_next) <= (2 * config_overflow))
							i = 1;
						else
						{
							if (i > 3)
								config_overflow = --i;
							i = 0;		/* key buffer overflow */
						}
					}
					else
					{
						if (i == config_overflow && curwin->w_height > 10 && c_next == 0)
							config_overflow = ++i;
						else if ((c_end - c_next) <= (config_overflow * 2))
						{
							col = ((config_overflow * 2) - (c_end - c_next)) / 2;
							i = i > col ? col : i;
						}
						else
							i = 0;		/* key buffer overflow */
					}
					while (i-- > 1)
					{
						if (!keybuf_chk(2))
							break;
						cbuf[c_end++] = K_NUL;
						if (wParam == VK_UP)
							cbuf[c_end++] = 'H';
						else
							cbuf[c_end++] = 'P';
					}
				}
			}
			else if (repeat && (!curwin->w_p_wrap || (p_ww & 8))
					&& (wParam == VK_LEFT || wParam == VK_RIGHT))
			{
				repeat++;
				if (c_next != 0 && vpeekc() != NUL)
					;
				else if (repeat > KEY_REP && keybuf_chk(2))
				{
					cbuf[c_end++] = K_NUL;
					if (wParam == VK_LEFT)
						cbuf[c_end++] = 'K';
					else if (wParam == VK_RIGHT)
						cbuf[c_end++] = 'M';
				}
			}
			if (!keybuf_chk(2))
				return(0);
			switch (wParam) {
			case VK_PRIOR:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'I'; break;
			case VK_NEXT:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'Q'; break;
			case VK_UP:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'H'; break;
			case VK_DOWN:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'P'; break;
			case VK_LEFT:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'K'; break;
			case VK_RIGHT:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'M'; break;
			case VK_DELETE:	cbuf[c_end++] = '\177';	break;
			case VK_F1:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = ';'; break;
			case VK_F2:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '<'; break;
			case VK_F3:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '='; break;
			case VK_F4:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '>'; break;
			case VK_F5:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '?'; break;
			case VK_F6:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = '@'; break;
			case VK_F7:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'A'; break;
			case VK_F8:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'B'; break;
			case VK_F9:		cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'C'; break;
			case VK_F10:	cbuf[c_end++] = K_NUL; cbuf[c_end++] = 'D'; break;
#ifdef NT106KEY
			/* ZENKAKU / HANKAKU KEY */
			case 0xf3: case 0xf4:
				if (config_nt106)
					cbuf[c_end++] = '[' & 0x1f;		/* ESC key !! */
				break;
#endif
			}
		}
		return(0);
	case WM_KEYUP:
		if (repeat && c_ind == 0)
		{
			for (i = c_next; i < c_end; i++)
			{
				if (cbuf[i] == cbuf[c_end - 1])
					c_end = i + 1;
			}
		}
		repeat = 0;
		switch (wParam) {
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			state_shift = FALSE;
			break;
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
			state_ctrl	= FALSE;
			break;
		case VK_NUMLOCK:
		case VK_CAPITAL:
			break;
		}
		return(0);
	case WM_SYSKEYUP:
		return(0);
	case WM_SETFOCUS:
		v_focus = TRUE;
		MoveCursor(hWnd);
		return(0);
	case WM_KILLFOCUS:
		if (v_cursor && v_focus)
			HideCaret(hWnd);
		v_focus = FALSE;
		v_caret = 0;
		state_shift	= FALSE;
		state_ctrl	= FALSE;
		repeat = 0;
		return(0);
	case WM_DESTROY:
		DragAcceptFiles(hWnd, FALSE);
		DeleteObject(v_font);
		PostQuitMessage(0);
		return(0);
	case WM_COPYDATA:
		cds = (COPYDATASTRUCT *)lParam;
		if (do_msg != TRUE && GuiWin == '1' && NameBuff != NULL
				&& cds->dwData != 0 && (State & NORMAL) && cmode == FALSE
				&& selwin == NULL && VIsual.lnum == 0
				&& !(!p_hid && curbuf->b_changed && (autowrite(curbuf) == FAIL)))
		{
			COPYDATASTRUCT	*	cds = (COPYDATASTRUCT *)lParam;

			do_msg = TRUE;
			more = p_more;
			p_more = FALSE;
			++no_wait_return;
			if (!did_cd)
			{
				BUF		*buf;

				for (buf = firstbuf; buf != NULL; buf = buf->b_next)
				{
					buf->b_xfilename = buf->b_filename;
					mf_fullname(buf->b_ml.ml_mfp);
				}
				status_redraw_all();
			}
			did_cd = TRUE;
			p = cds->lpData;
			SetCurrentDirectory(p);
			strncpy(IObuff, &p[cds->dwData], IOSIZE);
			docmdline(IObuff);
			--no_wait_return;
			p_more = more;
			cursor_refresh(hWnd);
			if (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
										&& ver_info.dwMajorVersion == 3))
				Shell_NotifyIcon(NIM_DELETE, &nIcon);
			ShowWindow(hWnd, SW_SHOW);
			OpenIcon(hWnd);
			SetForegroundWindow(hWnd);
			SetTimer(hWnd, SHOW_TIME, config_show, NULL);
			do_msg = FALSE;
			return(TRUE);
		}
		break;
	case WM_DROPFILES:
		if (RedrawingDisabled || no_wait_return)
			return(0);
		selwin = NULL;
		if (cmode)
			clear_cmode(hWnd);
		clear_visual(hWnd);
#if defined(KANJI) && defined(SYNTAX)
		if (syntax_on() && cmode)
			updateScreen(CLEAR);
#endif
		cmode = FALSE;
		more = p_more;
		p_more = FALSE;
		++no_wait_return;
		if (State & NORMAL)
		{
			int			change = FALSE;
			int			bitmap = FALSE;

			if (!p_hid && curbuf->b_changed && (autowrite(curbuf) == FAIL))
				change = TRUE;
			if (change && win_split(0L, FALSE) == FAIL)
			{
				WIN 	*	nextwp;
				WIN		*	now = curwin;

				for (wp = firstwin; wp != NULL; wp = nextwp)
				{
					nextwp = wp->w_next;
					if (wp->w_buffer->b_changed && (autowrite(wp->w_buffer) == FAIL))
						;
					else if (lastwin != firstwin)
					{
						win_enter(wp, TRUE);
						close_window(TRUE);
					}
				}
				win_enter(now, TRUE);
			}
			else
				change = FALSE;
			if (change && win_split(0L, FALSE) == FAIL)
				;
			else
			{
				files = DragQueryFile((HANDLE)wParam, 0xffffffff, NULL, 0);
				memset(IObuff, '\0', IOSIZE);
				strcpy(IObuff, ":args");
				for (i = 0; i < files; i++)
				{
					if (DragQueryFile((HANDLE)wParam, i, NameBuff, MAXPATHL) != 0)
					{
						if (isbitmap(NameBuff, NULL))
						{
							if (bitmap)
								Sleep(1000);
							strcpy(config_bitmapfile, NameBuff);
							config_bitmap = TRUE;
							InvalidateRect(hWnd, NULL, TRUE);
							UpdateWindow(hWnd);
							bitmap = TRUE;
						}
						else if (iswave(NameBuff))
						{
							int			j = 0;

							if (bitmap)
								Sleep(1000);
							strcpy(config_wavefile, NameBuff);
							config_wave = TRUE;
							sndPlaySound(config_wavefile, SND_SYNC);
						}
						else
						{
							if ((strlen(IObuff) + strlen(NameBuff) + 3) > IOSIZE)
								break;
							strcat(IObuff, " \"");
							strcat(IObuff, NameBuff);
							strcat(IObuff, "\"");
						}
					}
				}
				do_drag = TRUE;
				if (strlen(IObuff) > 5)
					docmdline(IObuff);
				do_drag = FALSE;
			}
		}
		else if (State & INSERT)
		{
			files = DragQueryFile((HANDLE)wParam, 0xffffffff, NULL, 0);
			memset(IObuff, '\0', IOSIZE);
			for (i = 0; i < files; i++)
			{
				if (DragQueryFile((HANDLE)wParam, i, NameBuff, MAXPATHL) != 0)
				{
					if ((strlen(IObuff) + strlen(NameBuff) + 3) > IOSIZE)
						break;
					strcat(IObuff, "\"");
					strcat(IObuff, NameBuff);
					strcat(IObuff, "\"");
				}
				if (i != (files - 1))
					strcat(IObuff, "\n");
			}
			if (keybuf_chk(strlen(IObuff)))
			{
				memcpy(&cbuf[c_end], IObuff, strlen(IObuff));
				c_end += strlen(IObuff);
			}
		}
		else
			emsg("No command mode");
		--no_wait_return;
		p_more = more;
		cursor_refresh(hWnd);
		DragFinish((HANDLE)wParam);
		if (IsIconic(hWnd))
			OpenIcon(hWnd);
		TopWindow(hVimWnd);
		return(0);
	case WM_QUERYOPEN:
		return(TRUE);
	case WM_INITMENU:
		hMenu = GetSystemMenu(hVimWnd, FALSE);
		for (i = 0; i < 2; i++)
		{
			int				j;

#ifdef USE_BDF
			if (config_bdf)
			{
				CheckMenuItem(hMenu, IDM_FONT, MF_BYCOMMAND | MF_UNCHECKED);
				CheckMenuItem(hMenu, IDM_BDF, MF_BYCOMMAND | MF_CHECKED);
			}
			else
			{
				CheckMenuItem(hMenu, IDM_FONT, MF_BYCOMMAND | MF_CHECKED);
				CheckMenuItem(hMenu, IDM_BDF, MF_BYCOMMAND | MF_UNCHECKED);
			}
#endif
			for (j = IDM_CONF0; j <= IDM_CONF3; j++)
				CheckMenuItem(hMenu, j, MF_BYCOMMAND | MF_UNCHECKED);
			if (GuiConfig <= IDM_CONF3)
				CheckMenuItem(hMenu, IDM_CONF0 + GuiConfig, MF_BYCOMMAND | MF_CHECKED);
#ifdef USE_HISTORY
			if (i == 0)
				DeleteMenu(hMenu, 12, MF_BYPOSITION);
			for (j = IDM_HIST1; j <= IDM_HIST9; j++)
				DeleteMenu(hMenu, j, MF_BYCOMMAND);
			if (!config_ini)
			{
				char		*	p;
				HMENU			hHMenu;

				if (i == 0)
				{
					hHMenu = CreatePopupMenu();
					AppendMenu(hHMenu, MF_STRING, IDM_HSAVE, "&Save History");
					AppendMenu(hMenu,  MF_POPUP,  (UINT)hHMenu, "&History");
				}
				else
					hHMenu = GetSubMenu(hMenu, 5);
				for (j = IDM_HIST1; j <= IDM_HIST9; j++)
				{
					if ((p = HistoryGetMenu(j - IDM_HISTM)) != NULL)
					{
						AppendMenu(hHMenu, MF_STRING, j,  p);
					}
				}
			}
#endif
			if (State & NORMAL)
			{
				if (i == 0)
				{
					EnableMenuItem(hMenu, 7, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(hMenu, 8, MF_BYPOSITION | MF_ENABLED);
					if (config_ini)
						EnableMenuItem(hMenu, 9, MF_BYPOSITION | MF_GRAYED);
					else
						EnableMenuItem(hMenu, 9, MF_BYPOSITION | MF_ENABLED);
				}
				else
				{
					EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_ENABLED);
					if (config_ini)
						EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_GRAYED);
					else
						EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
#ifdef USE_HISTORY
					if (config_ini)
						EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_GRAYED);
					else
						EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_ENABLED);
#endif
					EnableMenuItem(hMenu, IDM_HELP,  MF_BYCOMMAND | MF_ENABLED);
				}
				if (VIsual.lnum != 0 || cmode)
					EnableMenuItem(hMenu, IDM_YANK,  MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(hMenu, IDM_YANK,  MF_BYCOMMAND | MF_GRAYED);
				if (VIsual.lnum != 0)
					EnableMenuItem(hMenu, IDM_DELETE,MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(hMenu, IDM_DELETE,MF_BYCOMMAND | MF_GRAYED);
				if (cmode)
					EnableMenuItem(hMenu, IDM_PASTE, MF_BYCOMMAND | MF_GRAYED);
				else
					EnableMenuItem(hMenu, IDM_PASTE, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu, IDM_CLICK, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu, IDM_FILE,  MF_BYCOMMAND | MF_ENABLED);
				if (curbuf != NULL && curbuf->b_filename != NULL)
					EnableMenuItem(hMenu, IDM_SFILE, MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(hMenu, IDM_SFILE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_AFILE, MF_BYCOMMAND | MF_ENABLED);
				if (strlen(config_printer) == 0 ||
						(curbuf->b_ml.ml_line_count == 1 && strlen(ml_get(1)) == 0))
					EnableMenuItem(hMenu, IDM_PRINT, MF_BYCOMMAND | MF_GRAYED);
				else
					EnableMenuItem(hMenu, IDM_PRINT, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu, IDM_EXIT,  MF_BYCOMMAND | MF_ENABLED);
				if (!curbuf->b_changed && (lastwin == firstwin)
											&& (curbuf->b_filename != NULL))
					EnableMenuItem(hMenu, IDM_TAIL,  MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(hMenu, IDM_TAIL,  MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				if (i == 0)
				{
					EnableMenuItem(hMenu, 7, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(hMenu, 8, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(hMenu, 9, MF_BYPOSITION | MF_GRAYED);
#ifdef USE_HISTORY
					if (config_ini)
						EnableMenuItem(hMenu, 12, MF_BYPOSITION | MF_GRAYED);
#endif
				}
				else
				{
					EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_DISABLED);
					EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_DISABLED);
					if (config_ini)
						EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_GRAYED);
					else
						EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_DISABLED);
#ifdef USE_HISTORY
					EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_GRAYED);
#endif
					EnableMenuItem(hMenu, IDM_HELP,  MF_BYCOMMAND|MF_DISABLED);
				}
				if (cmode)
					EnableMenuItem(hMenu, IDM_YANK,  MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(hMenu, IDM_YANK,  MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_DELETE,MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_PASTE, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu, IDM_CLICK, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_FILE,  MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_SFILE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_AFILE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_PRINT, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_EXIT,  MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_TAIL,  MF_BYCOMMAND | MF_GRAYED);
			}
			if ((curwin->w_arg_idx + 1) < arg_count)
				EnableMenuItem(hMenu, IDM_NFILE, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hMenu, IDM_NFILE, MF_BYCOMMAND | MF_GRAYED);
			if (curwin->w_arg_idx >= 1)
				EnableMenuItem(hMenu, IDM_PFILE, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hMenu, IDM_PFILE, MF_BYCOMMAND | MF_GRAYED);
			if (config_bitmap)
				CheckMenuItem(hMenu, IDM_BITMAP, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_BITMAP, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_wave)
				CheckMenuItem(hMenu, IDM_WAVE, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_WAVE, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_save)
				CheckMenuItem(hMenu, IDM_SAVE, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_SAVE, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_comb)
				CheckMenuItem(hMenu, IDM_COMB, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_COMB, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_sbar)
				CheckMenuItem(hMenu, IDM_SBAR, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_SBAR, MF_BYCOMMAND | MF_UNCHECKED);
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
			if (config_share || config_common)
				CheckMenuItem(hMenu, (UINT)hCFile, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, (UINT)hCFile, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_share)
				CheckMenuItem(hMenu, IDM_SHARE, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_SHARE, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_common)
				CheckMenuItem(hMenu, IDM_COMMON,MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_COMMON,MF_BYCOMMAND | MF_UNCHECKED);
#endif
			if (pSetLayeredWindowAttributes)
			{
				if (config_fadeout)
					CheckMenuItem(hMenu, IDM_FADEOUT, MF_BYCOMMAND | MF_CHECKED);
				else
					CheckMenuItem(hMenu, IDM_FADEOUT, MF_BYCOMMAND | MF_UNCHECKED);
			}
			if (config_grepwin)
				CheckMenuItem(hMenu, IDM_GREPWIN, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_GREPWIN, MF_BYCOMMAND | MF_UNCHECKED);
#ifdef USE_HISTORY
			CheckMenuItem(hMenu, (UINT)hHist, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_HISTORY, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_HAUTO,   MF_BYCOMMAND | MF_UNCHECKED);
			if (!config_ini)
			{
				if (config_history || config_hauto)
					CheckMenuItem(hMenu, (UINT)hHist, MF_BYCOMMAND | MF_CHECKED);
				if (config_history)
					CheckMenuItem(hMenu, IDM_HISTORY, MF_BYCOMMAND | MF_CHECKED);
				if (config_hauto)
					CheckMenuItem(hMenu, IDM_HAUTO, MF_BYCOMMAND | MF_CHECKED);
			}
#endif
			if (config_unicode)
				CheckMenuItem(hMenu, IDM_UNICODE, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_UNICODE, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_tray)
				CheckMenuItem(hMenu, IDM_TRAY, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_TRAY, MF_BYCOMMAND | MF_UNCHECKED);
			if (GuiWin == '1')
				CheckMenuItem(hMenu, IDM_ONEWIN, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_ONEWIN, MF_BYCOMMAND | MF_UNCHECKED);
			if (config_mouse)
				CheckMenuItem(hMenu, IDM_MOUSE, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_MOUSE, MF_BYCOMMAND | MF_UNCHECKED);
#ifdef NT106KEY
			if (config_nt106)
				CheckMenuItem(hMenu, IDM_NT106, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_NT106, MF_BYCOMMAND | MF_UNCHECKED);
#endif
			if (config_menu)
				CheckMenuItem(hMenu, IDM_MENU, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_MENU, MF_BYCOMMAND | MF_UNCHECKED);
			if (v_macro)
				CheckMenuItem(hMenu, IDM_TAIL, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_TAIL, MF_BYCOMMAND | MF_UNCHECKED);
			if (GuiConfig && !config_ini)
				EnableMenuItem(hMenu, IDM_COMB, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hMenu, IDM_COMB, MF_BYCOMMAND | MF_GRAYED);
			if (GuiConfig && !config_ini)
				EnableMenuItem(hMenu, IDM_COMS, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hMenu, IDM_COMS, MF_BYCOMMAND | MF_GRAYED);
			hMenu = v_menu;
		}
		DrawMenuBar(hWnd);
		return(0);
	case WM_COMMAND:
		wParam = LOWORD(wParam);
	case WM_SYSCOMMAND:
		selwin = NULL;
		bClear = TRUE;
		if (!s_cursor)
		{
			s_cursor = TRUE;
			ShowCursor(TRUE);
		}
		switch (wParam) {
		case IDM_PASTE:
		case IDM_YANK:
		case IDM_DELETE:
		case IDM_CLICK:
		case IDM_PRINT:
			break;
		default:
			if ((wParam & 0xf000) != 0xf000)
			{
				if (cmode)
					clear_cmode(hWnd);
				clear_visual(hWnd);
#if defined(KANJI) && defined(SYNTAX)
				if (syntax_on() && cmode)
					updateScreen(CLEAR);
#endif
				cmode = FALSE;
			}
			break;
		}
		switch (wParam) {
		case IDM_EXTEND:
			if (!v_extend)
			{
				HMENU			hMenu;
				HMENU			hFile;
				HMENU			hSetup;
				HMENU			hColor;

				hMenu = GetSystemMenu(hVimWnd, FALSE);
#if CUST_MENU
				hFile = GetSubMenu(hMenu, 5);
				AppendMenu(hFile, MF_STRING, IDM_TAIL,  "&Tail");
				hSetup = GetSubMenu(hMenu, 3);
#else
				hFile = GetSubMenu(hMenu, 10);
				AppendMenu(hFile, MF_STRING, IDM_TAIL,  "&Tail");
				hSetup = GetSubMenu(hMenu, 8);
#endif
				hColor = CreatePopupMenu();
				AppendMenu(hColor, MF_STRING, IDM_TBCOLOR,  "&TB");
				AppendMenu(hColor, MF_STRING, IDM_SOCOLOR,  "&SO");
				AppendMenu(hColor, MF_STRING, IDM_TICOLOR,  "T&I");
				AppendMenu(hColor, MF_STRING, IDM_DELCOLOR, "&DEL");
				InsertMenu(hSetup, IDM_BITMAP, MF_POPUP, (UINT)hColor,"&Extend Color");
				InsertMenu(hSetup, IDM_SAVE, MF_UNCHECKED, IDM_COMB, "Combi&nation");
				InsertMenu(hSetup, IDM_SAVE, MF_UNCHECKED, IDM_COMS, "C&ombination Command");
				hSetup = GetSubMenu(v_menu, 3);
				InsertMenu(hSetup, IDM_BITMAP, MF_POPUP, (UINT)hColor,"&Extend Color");
				InsertMenu(hSetup, IDM_SAVE, MF_UNCHECKED, IDM_COMB, "Combi&nation");
				InsertMenu(hSetup, IDM_SAVE, MF_UNCHECKED, IDM_COMS, "C&ombination Command");
			}
			v_extend = TRUE;
			break;
		case IDM_VER:
			{
				char		msg[1024];
				sprintf(msg,
#ifdef KANJI
					"    %s\r\nPorted to W32-GUI and Japanized Ver. %s",
					longVersion,
					longJpVersion
#else
					"%s",
					longVersion
#endif
				);
				MessageBox(hWnd, msg,
#ifdef KANJI
					JpVersion,
#else
					Version,
#endif
				MB_OK);
			}
			break;
		case IDM_VER2:
			{
				char		msg[4096];
				sprintf(msg,
#ifdef KANJI
					"    %s\r\nPorted to W32-GUI and Japanized Ver. %s",
					longVersion,
					longJpVersion
#else
					"%s",
					longVersion
#endif
				);
				if (vimgetenv("VIM") != NULL)
					sprintf(&msg[strlen(msg)],
							"\r\n    System Dir   %s", vimgetenv("VIM"));
				if (vimgetenv("HOME") != NULL)
					sprintf(&msg[strlen(msg)],
							"\r\n    Home Dir     %s", vimgetenv("HOME"));
#ifdef USE_EXFILE
				sprintf(&msg[strlen(msg)],
						"\r\n    Extend File System Support:\r\n%s", ef_ver());
# ifdef USE_MATOME
				sprintf(&msg[strlen(msg)],
						"\r\n    MIME Decode Support:\r\n%s", decode_ver());
# endif
#endif
				MessageBox(hWnd, msg,
#ifdef KANJI
					JpVersion,
#else
					Version,
#endif
				MB_OK);
			}
			break;
		case IDM_HELP:
			if ((State & NORMAL) && VIsual.lnum == 0 && !cmode)
			{
				docmdline(":help");
				cursor_refresh(hWnd);
			}
			break;
		case IDM_NFILE:
			if ((State & NORMAL) && ((curwin->w_arg_idx + 1) < arg_count)
												&& VIsual.lnum == 0 && !cmode)
				docmdline(":next");
			SendMessage(hVimWnd, WM_INITMENU, 0, 0);
			cursor_refresh(hWnd);
			break;
		case IDM_PFILE:
			if ((State & NORMAL) && (curwin->w_arg_idx >= 1)
												&& VIsual.lnum == 0 && !cmode)
				docmdline(":Next");
			SendMessage(hVimWnd, WM_INITMENU, 0, 0);
			cursor_refresh(hWnd);
			break;
		case IDM_TAIL:
			v_macro = !v_macro;
			if (v_macro)
			{
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
				if (config_share || config_common)
					ef_share_close(curbuf->b_filename);
#endif
				SetTimer(hWnd, TAIL_TIME, config_show * 5, NULL);
				ZeroMemory(&byFile, sizeof(byFile));
			}
			else
				KillTimer(hWnd, TAIL_TIME);
			break;
		case IDM_MENU:
			config_menu = !config_menu;
			if (config_menu)
				SetMenu(hVimWnd, v_menu);
			else
				SetMenu(hVimWnd, NULL);
			do_resize = TRUE;
			{
				RECT		rcWindow;
				if (GetWindowRect(hWnd, &rcWindow))
				{
					config_x = rcWindow.left;
					config_y = rcWindow.top;
				}
			}
			if ((config_x & 0x7fffffff) > (DWORD)GetSystemMetrics(SM_CXSCREEN))
				config_x = 1;
			if ((config_y & 0x7fffffff) > (DWORD)GetSystemMetrics(SM_CYSCREEN))
				config_y = 1;
			nowCols = Columns;
			nowRows = Rows;
			MoveWindow(hWnd, config_x, config_y, config_w, config_h, TRUE);
			mch_get_winsize();
			break;
		case IDM_FONT:
#ifdef KANJI
			DialogBoxParam(hInst, "JFONT", hWnd, FontDialogProc, (LPARAM)NULL);
#else
			{
				CHOOSEFONT			cfFont;

				memset(&cfFont, 0, sizeof(cfFont));
				memcpy(&logfont, &config_font, sizeof(logfont));
				cfFont.lStructSize	= sizeof(cfFont);
				cfFont.hwndOwner	= hWnd;
				cfFont.hDC			= NULL;
				cfFont.rgbColors	= *v_fgcolor;
				cfFont.lpLogFont	= &logfont;
				cfFont.Flags		= CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT
										| CF_ANSIONLY | CF_NOVERTFONTS
										/* | CF_FIXEDPITCHONLY | CF_EFFECTS */ ;
				cfFont.lCustData	= 0;
				cfFont.lpfnHook		= NULL;
				cfFont.lpTemplateName= NULL;
				cfFont.hInstance	= hInst;
				if (ChooseFont(&cfFont))
					memcpy(&config_font, &logfont, sizeof(logfont));
			}
#endif
			ResetScreen(hWnd);
			break;
		case IDM_LSPACE:
			if (pSetLayeredWindowAttributes != NULL)
				DialogBoxParam(hInst, "LINESPACEEX", hWnd, LineSpaceDialogEx, (LPARAM)NULL);
			else
				DialogBoxParam(hInst, "LINESPACE", hWnd, LineSpaceDialog, (LPARAM)NULL);
			ResetScreen(hWnd);
			break;
#ifdef USE_BDF
		case IDM_BDF:
			GetBDFfont(hInst, 1, config_bdffile, config_jbdffile, &v_bxchar, &v_bychar, &config_bdf);
			if (!config_ini && config_bdf && config_bitmap)
			{
				v_fgcolor = &config_fgbdf;
				v_bgcolor = &config_bgbdf;
			}
			else
			{
				v_fgcolor = &config_fgcolor;
				v_bgcolor = &config_bgcolor;
			}
			ResetScreen(hWnd);
			break;
		case IDM_BDFONOFF:
			config_bdf = !config_bdf;
			GetBDFfont(hInst, 0, config_bdffile, config_jbdffile, &v_bxchar, &v_bychar, &config_bdf);
			if (!config_ini && config_bdf && config_bitmap)
			{
				v_fgcolor = &config_fgbdf;
				v_bgcolor = &config_bgbdf;
			}
			else
			{
				v_fgcolor = &config_fgcolor;
				v_bgcolor = &config_bgcolor;
			}
			ResetScreen(hWnd);
# if 0
			if (GuiConfig && GetClientRect(hWnd, &rcWindow))
			{
				Columns = (rcWindow.right - rcWindow.left + v_xchar / 2) / v_xchar;
				Rows = (rcWindow.bottom - rcWindow.top + v_ychar / 2) / v_ychar;
				nowCols = Columns;
				nowRows = Rows;
			}
# endif
			mch_set_winsize();
			break;
#endif
		case IDM_FILE:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			IObuff[0] = '\0';
			if (curbuf->b_filename != NULL)
			{
				strcpy(IObuff, curbuf->b_filename);
				*gettail(IObuff) = NUL;
			}
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= hInst;
			ofn.lpstrFilter		= "ALL(*.*)\0*.*\0EFS(*.lzh;*.gz;*.Z;*.tgz;*.taz)\0*.lzh;*.gz;*.Z;*.tgz;*.taz;*.tar;*.tar.gz;*.tar.Z\0C(*.c;*.cpp;*.h;*.def;*.rc)\0*.c;*.cpp;*.h;*.def;*.rc\0DOC(*.txt;*.doc)\0*.txt;*.doc\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAXPATHL;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= NULL;
			ofn.Flags			= OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_ALLOWMULTISELECT
									| OFN_EXPLORER | OFN_NOCHANGEDIR /*| OFN_NOVALIDATE */;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			i = GetOpenFileName(&ofn);
			if (i)
			{
				int			change = FALSE;

				selwin = NULL;
				more = p_more;
				p_more = FALSE;
				++no_wait_return;
				if (!p_hid && curbuf->b_changed && (autowrite(curbuf) == FAIL))
					change = TRUE;
				if (change && win_split(0L, FALSE) == FAIL)
				{
					WIN 	*	nextwp;
					WIN		*	now = curwin;

					for (wp = firstwin; wp != NULL; wp = nextwp)
					{
						nextwp = wp->w_next;
						if (wp->w_buffer->b_changed && (autowrite(wp->w_buffer) == FAIL))
							;
						else if (lastwin != firstwin)
						{
							win_enter(wp, TRUE);
							close_window(TRUE);
						}
					}
					win_enter(now, TRUE);
				}
				else
					change = FALSE;
				if (change && win_split(0L, FALSE) == FAIL)
					;
				else
				{
					memset(IObuff, '\0', IOSIZE);
					strcpy(IObuff, ":args");
					if (NameBuff[ofn.nFileOffset - 1] != '\0')
					{
						strcat(IObuff, " \"");
						strcat(IObuff, NameBuff);
						strcat(IObuff, "\"");
					}
					else
					{
						col = strlen(NameBuff);
						for (;;)
						{
							col++;
							if (NameBuff[col] == '\0')
								break;
							if ((IOSIZE - strlen(IObuff))
									< (strlen(NameBuff) + strlen(&NameBuff[col]) + 5))
								break;
							strcat(IObuff, " \"");
							strcat(IObuff, NameBuff);
							strcat(IObuff, "\\");
							strcat(IObuff, &NameBuff[col]);
							strcat(IObuff, "\"");
							col += strlen(&NameBuff[col]);
						}
					}
					do_drag = TRUE;
					docmdline(IObuff);
					do_drag = FALSE;
				}
				--no_wait_return;
				p_more = more;
				cursor_refresh(hWnd);
			}
			break;
		case IDM_SFILE:
			selwin = NULL;
			more = p_more;
			p_more = FALSE;
			++no_wait_return;
			docmdline(":w!");
			--no_wait_return;
			p_more = more;
			cursor_refresh(hWnd);
			break;
		case IDM_AFILE:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			IObuff[0] = '\0';
			if (curbuf->b_filename != NULL)
			{
				strcpy(IObuff, curbuf->b_filename);
				*gettail(IObuff) = NUL;
				strcpy(NameBuff, gettail(curbuf->b_filename));
			}
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= hInst;
			ofn.lpstrFilter		= NULL;
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 0;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAXPATHL;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= NULL;
			ofn.Flags			= OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT
									| OFN_EXPLORER | OFN_NOCHANGEDIR /*| OFN_NOVALIDATE */;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			if (GetSaveFileName(&ofn))
			{
				selwin = NULL;
				more = p_more;
				p_more = FALSE;
				++no_wait_return;
				memset(IObuff, '\0', IOSIZE);
				strcpy(IObuff, ":w! ");
				strcat(IObuff, NameBuff);
				docmdline(IObuff);
				--no_wait_return;
				p_more = more;
				cursor_refresh(hWnd);
			}
			break;
		case IDM_BWHITE:
			*v_bgcolor = RGB(255, 255, 255);
			ResetScreen(hWnd);
			break;
		case IDM_BBLACK:
			*v_bgcolor = RGB(0, 0, 0);
			ResetScreen(hWnd);
			break;
		case IDM_FWHITE:
			*v_fgcolor = RGB(255, 255, 255);
			ResetScreen(hWnd);
			break;
		case IDM_FBLACK:
			*v_fgcolor = RGB(0, 0, 0);
			ResetScreen(hWnd);
			break;
		case IDM_FBLUE:
			*v_fgcolor = RGB(0, 0, 128);
			ResetScreen(hWnd);
			break;
		case IDM_BCOLOR:
			pColor	= v_bgcolor;
			goto SetColor;
		case IDM_FCOLOR:
			pColor	= v_fgcolor;
			goto SetColor;
		case IDM_TBCOLOR:
			pColor = v_tbcolor;
			goto SetColor;
		case IDM_SOCOLOR:
			pColor = v_socolor;
			goto SetColor;
		case IDM_TICOLOR:
			pColor = v_ticolor;
SetColor:
			memset(&cfColor, 0, sizeof(cfColor));
			cfColor.lStructSize	= sizeof(cfColor);
			cfColor.hwndOwner	= hWnd;
			cfColor.hInstance	= hInst;
			cfColor.rgbResult	= *pColor;
			cfColor.lpCustColors= config_color;
			cfColor.Flags		= 0;
			cfColor.lCustData	= 0;
			cfColor.lpfnHook	= NULL;
			cfColor.lpTemplateName	= NULL;
			if (ChooseColor(&cfColor))
				*pColor = cfColor.rgbResult;
			ResetScreen(hWnd);
			break;
		case IDM_DELCOLOR:
			*v_tbcolor = (-1);
			*v_socolor = (-1);
			*v_ticolor = (-1);
			ResetScreen(hWnd);
			break;
		case IDM_CANCEL:
			break;
		case IDM_PASTE:
			if (cmode)
				yank_cmode(hWnd, FALSE);
			else
			{
				long			size = 0;
				extern int		restart_edit;	/* this is in edit.c */

				if (((State & NORMAL) || State == INSERT) && OpenClipboard(hWnd))
				{
					if ((hClipData = GetClipboardData(CF_TEXT)) != NULL)
					{
						if ((lpClipData = GlobalLock(hClipData)) != NULL)
							size = strlen(lpClipData);
						GlobalUnlock(hClipData);
					}
					CloseClipboard();
				}
				if ((size > 2048) && !VIsual.lnum)
				{
					if (State & NORMAL)
					{
						yankbuffer = '@';
						if (restart_edit)
							doput(BACKWARD, 1, FALSE);
						else
							doput(FORWARD, 1, FALSE);
						yankbuffer = 0;
						curwin->w_cursor = curbuf->b_endop;
						cursor_refresh(hWnd);
						if (restart_edit && !lineempty(curwin->w_cursor.lnum))
						{
							inc_cursor();
							if (gchar_cursor() != NUL)
								dec_cursor();
						}
					}
					else if (keybuf_chk(3))
					{
						cbuf[c_end++] = Ctrl('O');
						cbuf[c_end++] = 'g';
						cbuf[c_end++] = 'V';
						NoMap = TRUE;
					}
					break;	/* no use keyboard buffer */
				}
				else if (OpenClipboard(hWnd))
				{
					int				imode = 0;

					if ((State & NORMAL) && !restart_edit)
						imode = 2;
					if ((hClipData = GetClipboardData(CF_TEXT)) != NULL)
					{
						if ((lpClipData = GlobalLock(hClipData)) != NULL)
						{
							if (!keybuf_chk(strlen(lpClipData) + 1 + imode))
							{
								GlobalUnlock(hClipData);
								CloseClipboard();
								break;
							}
							if (imode)
							{
								if (VIsual.lnum)
								{
									cbuf[c_end++] = 's';
									bClear = FALSE;
								}
								else
									cbuf[c_end++] = 'a';
							}
							for (p = lpClipData; *p; )
							{
#ifdef KANJI
								if (ISkanji(*p))
								{
									cbuf[c_end++] = *p++;
									cbuf[c_end++] = *p++;
								}
								else if (*p == 0xa0)	/* hannkaku kana space */
								{
									cbuf[c_end++] = ' ';
									*p++;
								}
								else
#endif
								{
									if (*p == '\r' && *(p+1) == '\n')
										++p;
									cbuf[c_end++] = *p++;
								}
							}
							if (imode)
								cbuf[c_end++] = ESC;
							GlobalUnlock(hClipData);
						}
					}
					CloseClipboard();
				}
			}
			c_ind = c_end;
			if (c_ind > 0)
			{
				w_p_tw = curbuf->b_p_tw;
				w_p_wm = curbuf->b_p_wm;
				w_p_ai = curbuf->b_p_ai;
				w_p_si = curbuf->b_p_si;
				w_p_et = curbuf->b_p_et;
				w_p_uc = p_uc;
				w_p_sm = p_sm;
				w_p_ru = p_ru;
				w_p_ri = p_ri;
				w_p_paste = p_paste;
				p_uc = c_ind;
				curbuf->b_p_tw = 0;
				curbuf->b_p_wm = 0;
				curbuf->b_p_ai = FALSE;
				curbuf->b_p_si = FALSE;
				curbuf->b_p_et = FALSE;
				p_sm = 0;
				p_ru = 0;
				p_ri = 0;
				p_paste = TRUE;
			}
			break;
		case IDM_YANK:
			if (cmode)
			{
				yank_cmode(hWnd, TRUE);
				break;
			}
		case IDM_DELETE:
		case IDM_CLICK:
			if (VIsual.lnum)
			{
				curbuf->b_startop = VIsual;
				if (lt(curbuf->b_startop, curwin->w_cursor))
				{
					curbuf->b_endop = curwin->w_cursor;
					if (wParam != IDM_YANK)		/* notepad compatible */
						curwin->w_cursor = curbuf->b_startop;
				}
				else
				{
					curbuf->b_endop = curbuf->b_startop;
					curbuf->b_startop = curwin->w_cursor;
				}
				nlines = curbuf->b_endop.lnum - curbuf->b_startop.lnum + 1;
				mincl = TRUE;
				if (VIsual.col == VISUALLINE)
					mtype = MLINE;
				else
				{
					mtype = MCHAR;
					if (*ml_get_pos(&(curbuf->b_endop)) == NUL)
						mincl = FALSE;
				}
				curwin->w_set_curswant = 1;
				if (mtype == MCHAR && !mincl &&
									equal(curbuf->b_startop, curbuf->b_endop))
					break;
#ifdef KANJI
				if (mincl && curbuf->b_endop.col != VISUALLINE
										&& ISkanji(gchar(&curbuf->b_endop)))
				{
					mincl = FALSE;
					curbuf->b_endop.col += 2;
				}
#endif
				if (mtype == MCHAR && mincl == FALSE && curbuf->b_endop.col == 0 && nlines > 1)
				{
					--nlines;
					--curbuf->b_endop.lnum;
					if (inindent())
						mtype = MLINE;
					else
					{
						curbuf->b_endop.col = STRLEN(ml_get(curbuf->b_endop.lnum));
						if (curbuf->b_endop.col)
						{
							--curbuf->b_endop.col;
							mincl = TRUE;
						}
					}
				}
				if (Visual_block)				/* block mode */
				{
					colnr_t			n;
					startvcol = getvcol(curwin, &(curbuf->b_startop), 2);
					n = getvcol(curwin, &(curbuf->b_endop), 2);
					if (n < startvcol)
						startvcol = (colnr_t)n;
					endvcol = getvcol(curwin, &(curbuf->b_startop), 3);
					n = getvcol(curwin, &(curbuf->b_endop), 3);
					if (n > endvcol)
						endvcol = n;
					coladvance(startvcol);
				}
			}
			if ((wParam == IDM_YANK) && (VIsual.lnum))
			{
				operator = STRCHR(opchars, 'y') - opchars + 1;
				yankbuffer = '@';
				(void)doyank(FALSE);
				yankbuffer = 0;
				operator = NOP;
				goto get_clipdata;
			}
			else if (wParam == IDM_YANK)
				goto get_clipdata;
			else if ((wParam == IDM_DELETE) && (VIsual.lnum))
			{
				operator = STRCHR(opchars, 'd') - opchars + 1;
				yankbuffer = '@';
				dodelete();
				yankbuffer = 0;
				operator = NOP;
				goto get_clipdata;
			}
			else if (wParam == IDM_CLICK && VIsual.lnum
							&& curbuf->b_startop.lnum == curbuf->b_endop.lnum)
			{
				if (mtype == MLINE)
				{
					i = strlen(ml_get(curbuf->b_startop.lnum));
					curbuf->b_startop.col = 0;
				}
				else
				{
					i = curbuf->b_endop.col - curbuf->b_startop.col + 1 - !mincl;
#ifdef KANJI
					if (ISkanjiFpos(&curbuf->b_endop) == 2)
						i++;
#endif
				}
				if (i == 0 || i >= MAXPATHL || i >= IOSIZE)
					break;
				strncpy(IObuff, ml_get(curbuf->b_startop.lnum) + curbuf->b_startop.col, (int)i);
				IObuff[i] = NUL;
				rc = (int)ShellExecute(NULL, NULL, IObuff, NULL, ".", SW_SHOW);
				if (!(rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND
						|| rc == SE_ERR_NOASSOC || rc == SE_ERR_ASSOCINCOMPLETE))
					break;
				if (FullName(IObuff, NameBuff, MAXPATHL) == OK
											&& strcmp(IObuff, NameBuff) != 0)
					ShellExecute(NULL, NULL, NameBuff, NULL, ".", SW_SHOW);
				break;
			}
			else
				break;
get_clipdata:
			if ((i = yank_to_clipboard(NULL)) != 0)
			{
				hClipData = GlobalAlloc(GMEM_MOVEABLE, i);
				if (hClipData == NULL)
					break;
				if ((lpClipData = GlobalLock(hClipData)) == NULL)
				{
					GlobalFree(hClipData);
					break;
				}
				yank_to_clipboard(lpClipData);
				GlobalUnlock(hClipData);
				if (OpenClipboard(hWnd) == FALSE)
				{
					GlobalFree(hClipData);
					break;
				}
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hClipData);
				CloseClipboard();
			}
			break;
		case IDM_COMB:
			config_comb = !config_comb;
			break;
		case IDM_SAVE:
			config_save = !config_save;
			break;
		case IDM_TRAY:
			config_tray = !config_tray;
			break;
		case IDM_ONEWIN:
			if (GuiWin == '1')
				GuiWin = 'w';
			else
				GuiWin = '1';
			break;
		case IDM_MOUSE:
			config_mouse = !config_mouse;
			break;
#ifdef NT106KEY
		case IDM_NT106:
			config_nt106 = !config_nt106;
			break;
#endif
		case IDM_UNICODE:
			config_unicode = !config_unicode;
			for (i = 0; i < v_ssize; i++)
				v_space[i] = v_xchar;
			break;
		case IDM_OPEN:
			if (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
										&& ver_info.dwMajorVersion == 3))
			{
				Shell_NotifyIcon(NIM_DELETE, &nIcon);
				ShowWindow(hWnd, SW_SHOW);
				OpenIcon(hWnd);
				TopWindow(hVimWnd);
			}
			break;
		case IDM_EXIT:
			bIClose = TRUE;
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case IDM_NEW:
			{
				char					name[MAXPATHL];
				char					command[MAXPATHL + 16];
				STARTUPINFO				si;
				PROCESS_INFORMATION		pi;

				GetModuleFileName(NULL, name, sizeof(name));
				memset(&pi, 0, sizeof(pi));
				memset(&si, 0, sizeof(si));
				si.cb = sizeof(si);
				if (config_ini)
					sprintf(command, "%s -I %s", name, GuiIni);
				else
					sprintf(command, "%s -n%d", name, GuiConfig);
				if (CreateProcess(NULL, command, NULL, NULL, FALSE,
						CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) == TRUE)
				{
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
			}
			break;
		case IDM_PRINT:
			{
				char					filename[MAXPATHL];
				char				*	fn;
				int						textmode = curbuf->b_p_tx;
#ifdef USE_OPT
				int						opt = p_opt;
#endif
#ifdef KANJI
				char_u					code = *curbuf->b_p_jc;
#endif
				DWORD					hThreadID;
				extern WINAPI			PrinterThread(PVOID filename);

				fn = curbuf->b_sfilename != NULL ? gettail(curbuf->b_sfilename) : "Untitled";
				for (i = 0; i < 1000; i++)
				{
					filename[0] = '\0';
					if (p_dir != NULL && *p_dir != '\0')
					{
						if (*p_dir == '>')	/* skip '>' in front of dir name */
							STRCPY(filename, p_dir + 1);
						else
							STRCPY(filename, p_dir);
						if (!ispathsep(*(filename + STRLEN(filename) - 1)))
							STRCAT(filename, PATHSEPSTR);
					}
					if (i == 0)
						sprintf(&filename[STRLEN(filename)], "%s", fn);
					else
						sprintf(&filename[STRLEN(filename)], "%s(%d)", fn, i);
					if (getperm(filename) < 0)
						break;		/* for loop */
				}
				curbuf->b_p_tx = TRUE;
#ifdef USE_OPT
				p_opt = 0;
#endif
#ifdef KANJI
				*curbuf->b_p_jc = tolower(JP_SYS);
#endif
				++no_wait_return;
				if (VIsual.lnum)
				{
					curbuf->b_startop = VIsual;
					if (lt(curbuf->b_startop, curwin->w_cursor))
					{
						curbuf->b_endop = curwin->w_cursor;
						curwin->w_cursor = curbuf->b_startop;
					}
					else
					{
						curbuf->b_endop = curbuf->b_startop;
						curbuf->b_startop = curwin->w_cursor;
					}
#if 0
					if (1 < curbuf->b_startop.lnum
							|| curbuf->b_endop.lnum < curbuf->b_ml.ml_line_count)
						STRCAT(filename, "[digest]");
#endif
					buf_write(curbuf, filename, NULL,
							curbuf->b_startop.lnum, curbuf->b_endop.lnum,
							FALSE, 0, FALSE);
				}
				else
					buf_write(curbuf, filename, NULL,
							(linenr_t)1, curbuf->b_ml.ml_line_count,
							FALSE, 0, FALSE);
				--no_wait_return;
				updateScreen(CLEAR);
				cursor_refresh(hWnd);
#ifdef KANJI
				*curbuf->b_p_jc = code;
#endif
#ifdef USE_OPT
				p_opt = opt;
#endif
				curbuf->b_p_tx = textmode;
				fn = malloc(strlen(filename) + 1);
				strcpy(fn, filename);
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PrinterThread, fn, 0, &hThreadID);
			}
			break;
		case IDM_PRINTSET:
			if (DialogBoxParam(hInst, "PRINTER", hWnd, PrinterDialog, (LPARAM)NULL) != 0)
				return(FALSE);
			break;
		case IDM_BITMAP:
			DialogBoxParam(hInst, "BITMAP", hWnd, BitmapDialog, (LPARAM)NULL);
			if (!config_ini && config_bitmap)
			{
				v_tbcolor = &config_tbbitmap;
				v_socolor = &config_sobitmap;
				v_ticolor = &config_tibitmap;
			}
			else
			{
				v_tbcolor = &config_tbcolor;
				v_socolor = &config_socolor;
				v_ticolor = &config_ticolor;
			}
			bSyncPaint = TRUE;
			break;
		case IDM_BITMAPONOFF:
			config_bitmap = !config_bitmap;
			if (!isbitmap(config_bitmapfile, NULL))
				config_bitmap = FALSE;
#ifdef USE_BDF
			if (!config_ini && config_bdf && config_bitmap)
			{
				v_fgcolor = &config_fgbdf;
				v_bgcolor = &config_bgbdf;
			}
			else
			{
				v_fgcolor = &config_fgcolor;
				v_bgcolor = &config_bgcolor;
			}
#endif
			if (!config_ini && config_bitmap)
			{
				v_tbcolor = &config_tbbitmap;
				v_socolor = &config_sobitmap;
				v_ticolor = &config_tibitmap;
			}
			else
			{
				v_tbcolor = &config_tbcolor;
				v_socolor = &config_socolor;
				v_ticolor = &config_ticolor;
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case IDM_BITMAPUP:
		case IDM_BITMAPDOWN:
			if (config_bitmap)
			{
				char_u		*	q;
				WIN32_FIND_DATA fb;
				HANDLE          hFind;
				char			buf[MAXPATHL];
				char			first[MAXPATHL];
				char			before[MAXPATHL];
				BOOL			bFlg = TRUE;
				BOOL			bFind = FALSE;
				int				cnt = 0;

				STRCPY(buf, config_bitmapfile);
				q = buf;
				q = gettail(buf);
				*q = NUL;
				if (buf[0] && ispathsep(*(buf + STRLEN(buf) - 1)))
					q = buf + STRLEN(buf) - 1;
				*q = NUL;
				strcat(buf, "\\*.*");
				if ((hFind = FindFirstFile(buf, &fb)) != INVALID_HANDLE_VALUE)
				{
					while (bFlg)
					{
						*q = NUL;
						strcat(buf, "\\");
						strcat(buf, fb.cFileName);
						if (isbitmap(buf, NULL))
						{
							if (bFind && wParam == IDM_BITMAPUP)
							{
								strcpy(config_bitmapfile, buf);
								bFind = FALSE;
								break;
							}
							if (cnt == 0)
								strcpy(first, buf);
							if (stricmp(buf, config_bitmapfile) == 0)
							{
								if (wParam == IDM_BITMAPDOWN && cnt)
								{
									strcpy(config_bitmapfile, before);
									break;
								}
								bFind = TRUE;
							}
							else
								strcpy(before, buf);
							cnt++;
						}
						bFlg = FindNextFile(hFind, &fb);
					}
					FindClose(hFind);
					if (bFind)
					{
						if (wParam == IDM_BITMAPUP)
							strcpy(config_bitmapfile, first);
						else if (cnt > 1)
							strcpy(config_bitmapfile, before);
					}
				}
				if (!isbitmap(config_bitmapfile, NULL))
					config_bitmap = FALSE;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		case IDM_TRANSUP:
			if (v_trans <= 95)
			{
				v_trans += 5;
				SetLayerd();
			}
			break;
		case IDM_TRANSDOWN:
			if (v_trans >= 5)
				v_trans -= 5;
			else
				v_trans = 0;
			SetLayerd();
			break;
		case IDM_BITSIZEUP:
			if (config_bitmap)
			{
				config_bitsize += 5;
				if (config_bitsize > 100)
					config_bitsize = 100;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		case IDM_BITSIZEDOWN:
			if (config_bitmap)
			{
				config_bitsize -= 5;
				if (config_bitsize < 10)
					config_bitsize = 10;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		case IDM_WAVE:
			DialogBoxParam(hInst, "WAVE", hWnd, WaveDialog, (LPARAM)NULL);
			break;
		case IDM_WAVEONOFF:
			config_wave = !config_wave;
			if (!iswave(config_wavefile))
				config_wave = FALSE;
			break;
		case IDM_SBAR:
			config_sbar = !config_sbar;
			ScrollBar();
			mch_set_winsize();
			break;
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
		case IDM_SHARE:
			if (config_share)
				config_share = FALSE;
			else
			{
				config_share = TRUE;
				config_common= FALSE;
			}
			goto share;
		case IDM_COMMON:
			if (config_common)
				config_common= FALSE;
			else
			{
				config_share = FALSE;
				config_common= TRUE;
			}
share:
			if (config_share || config_common)
			{
				ef_share_term();
				ef_share_init(config_common);
				for (buf = firstbuf; buf != NULL; buf = buf->b_next)
				{
					if (buf->b_ml.ml_mfp != NULL)
						ef_share_open(buf->b_filename);
				}
			}
			else
				ef_share_term();
			break;
#endif
		case IDM_FADEOUT:
			config_fadeout = !config_fadeout;
			break;
		case IDM_GREPWIN:
			config_grepwin = !config_grepwin;
			break;
#ifdef USE_HISTORY
		case IDM_HISTORY:
			config_history = !config_history;
			break;
		case IDM_HAUTO:
			config_hauto = !config_hauto;
			break;
		case IDM_HSAVE:
			{
				DWORD	hwork = config_hauto;

				config_hauto = TRUE;
				win_history_append(curbuf);
				config_hauto = hwork;
			}
			break;
#endif
		case IDM_COMS:
			if (v_extend && GuiConfig)
				DialogBoxParam(hInst, "COMMAND", hWnd, CommandDialog, (LPARAM)NULL);
			break;
		case IDM_CONFS:
			SaveConfig();
			break;
		case IDM_CONFUP:
			if (!config_ini)
			{
				int			find = 0;
				int			max = v_extend ? IDM_CONF9 : IDM_CONF3;
				HKEY		hKey;
				char		name[8];

				for (i = GuiConfig + 1; i <= (max - IDM_CONF0); i++)
				{
					sprintf(name, "Software\\Vim\\%d", i);
					if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
					{
						find = IDM_CONF0 + i;
						RegCloseKey(hKey);
						break;
					}
				}
				if (!find)
				{
					for (i = IDM_CONF0 - IDM_CONF0; i < GuiConfig; i++)
					{
						sprintf(name, "Software\\Vim\\%d", i);
						if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
						{
							find = IDM_CONF0 + i;
							RegCloseKey(hKey);
							break;
						}
					}
				}
				if (find)
					return(SendMessage(hWnd, WM_COMMAND, find, 0));
			}
			break;
		case IDM_CONFDOWN:
			if (!config_ini)
			{
				int			find = 0;
				int			max = v_extend ? IDM_CONF9 : IDM_CONF3;
				HKEY		hKey;
				char		name[8];

				for (i = GuiConfig - 1; i >= IDM_CONF0 - IDM_CONF0; i--)
				{
					sprintf(name, "Software\\Vim\\%d", i);
					if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
					{
						find = IDM_CONF0 + i;
						RegCloseKey(hKey);
						break;
					}
				}
				if (!find)
				{
					for (i = max ; i >= GuiConfig; i--)
					{
						sprintf(name, "Software\\Vim\\%d", i);
						if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
						{
							find = IDM_CONF0 + i;
							RegCloseKey(hKey);
							break;
						}
					}
				}
				if (find)
					return(SendMessage(hWnd, WM_COMMAND, find, 0));
			}
			break;
		default:
			if (IDM_CONF0 <= wParam && wParam <= IDM_CONF9)
			{
				if (RedrawingDisabled)
					break;
				if (!v_extend && IDM_CONF3 < wParam)
					break;
				flushbuf();
				SendMessage(hVimWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
				if (config_comb)
					UnloadCommand();
				GuiConfig = wParam - IDM_CONF0;
				LoadConfig(FALSE);
				oldmix = 0;
#ifdef USE_BDF
				if (config_bdf)
					GetBDFfont(hInst, 0, config_bdffile, config_jbdffile, &v_bxchar, &v_bychar, &config_bdf);
				if (!config_ini && config_bdf && config_bitmap)
				{
					v_fgcolor = &config_fgbdf;
					v_bgcolor = &config_bgbdf;
				}
				else
				{
					v_fgcolor = &config_fgcolor;
					v_bgcolor = &config_bgcolor;
				}
#endif
				if (!config_ini && config_bitmap)
				{
					v_tbcolor = &config_tbbitmap;
					v_socolor = &config_sobitmap;
					v_ticolor = &config_tibitmap;
				}
				else
				{
					v_tbcolor = &config_tbcolor;
					v_socolor = &config_socolor;
					v_ticolor = &config_ticolor;
				}
				ResetScreen(hWnd);
				ScrollBar();
				do_resize = TRUE;
				if (!config_save)
				{
					RECT		rcWindow;
					if (GetWindowRect(hWnd, &rcWindow))
					{
						config_x = rcWindow.left;
						config_y = rcWindow.top;
					}
				}
				if ((config_x & 0x7fffffff) > (DWORD)GetSystemMetrics(SM_CXSCREEN))
					config_x = 1;
				if ((config_y & 0x7fffffff) > (DWORD)GetSystemMetrics(SM_CYSCREEN))
					config_y = 1;
				nowCols = Columns;
				nowRows = Rows;
				MoveWindow(hWnd, config_x, config_y, config_w, config_h, TRUE);
				mch_get_winsize();
				comp_Botline_all();
				cursor_refresh(hWnd);
				if (config_comb)
					LoadCommand();
				SetLayerd();
				break;
			}
#ifdef USE_HISTORY
			else if (IDM_HIST1 <= wParam && wParam <= IDM_HIST9)
			{
				char	*	cl;

				++no_wait_return;
				if ((cl = HistoryGetCommand(wParam - IDM_HISTM)) != NULL)
				{
					if (keybuf_chk(strlen(cl)))
					{
						memcpy(&cbuf[c_end], cl, strlen(cl));
						c_end += strlen(cl);
					}
				}
				--no_wait_return;
				break;
			}
#endif
			return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		}
		if (cmode)
			clear_cmode(hWnd);
		if (bClear)
			clear_visual(hWnd);
#if defined(KANJI) && defined(SYNTAX)
		if (syntax_on() && cmode)
			updateScreen(CLEAR);
#endif
		cmode = FALSE;
		return(0);
	case WM_TIMER:
		if (wParam == KEY_TIME)
			do_time = TRUE;
		else if (wParam == SHOW_TIME)
		{
			KillTimer(hWnd, SHOW_TIME);
			TopWindow(hVimWnd);
		}
		else if (wParam == TAIL_TIME)
		{
			HANDLE			hFile;
			FILETIME		nowFile;

			KillTimer(hWnd, TAIL_TIME);
			v_macro = FALSE;
			if (curbuf->b_changed || !(State & NORMAL)
					|| (lastwin != firstwin) || (curbuf->b_filename == NULL))
				return(0);
			hFile = CreateFile(curbuf->b_filename, GENERIC_READ,
										FILE_SHARE_READ | FILE_SHARE_WRITE,
										NULL, OPEN_EXISTING, 0, 0);
			if (hFile == INVALID_HANDLE_VALUE)
				return(0);
			v_macro = TRUE;
			while (GetFileTime(hFile, NULL, &nowFile, NULL)
								&& (CompareFileTime(&byFile, &nowFile) < 0))
			{
				if (curwin->w_cursor.lnum != curbuf->b_ml.ml_line_count)
					break;
				CopyMemory(&byFile, &nowFile, sizeof(byFile));
				more = p_more;
				p_more = FALSE;
				++no_wait_return;
				docmdline(":e!");
				--no_wait_return;
				p_more = more;
				curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
				if (keybuf_chk(2))
				{
					cbuf[c_end++] = 'z';
					cbuf[c_end++] = '-';
				}
				beginline(TRUE);
				cursor_refresh(hWnd);
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
				if (config_share || config_common)
					ef_share_close(curbuf->b_filename);
#endif
				break;
			}
			CloseHandle(hFile);
			SetTimer(hWnd, TAIL_TIME, config_show * 5, NULL);
		}
		else if ((wParam == MOUSE_TIME) && (selwin != NULL) && VIsual.lnum)
		{
			if (updown == 0 && leftright == 0)
				return(0);
			else if (updown > 0)
				oneup(updown);
			else if (updown < 0)
				onedown(-updown);
			else if (leftright < 0 && selwin->w_leftcol)
				oneleft();
			else if (leftright > 0 && selwin->w_p_wrap != TRUE)
			{
				if (p_ss == 0)
					i = Columns / 2;
				else
					i = p_ss;
				while (i--)
					oneright();
			}
			cursor_refresh(hWnd);
		}
		else if (wParam == TRIPLE_TIME)
			do_trip = FALSE;
		return(0);
	case WM_MOUSEACTIVATE:
		if (LOWORD(lParam) == HTCLIENT || LOWORD(lParam) == HTVSCROLL)
			return(MA_ACTIVATEANDEAT);
		return(MA_ACTIVATE);
	case WM_LBUTTONDOWN:
		if (cmode)
		{
			clear_cmode(hWnd);
#if defined(KANJI) && defined(SYNTAX)
			if (syntax_on())
				updateScreen(CLEAR);
#endif
			cmode = FALSE;
		}
		if (VIsual.lnum != 0)
		{
			updateScreen(NOT_VALID);
			cursor_refresh(hWnd);
			clear_visual(hWnd);
		}
		wp = get_linecol(lParam, &pos, &row, &col);
		if (GetKeyState(VK_MENU) & 0x8000)
		{
			if (do_trip)
			{
				int			start	= Columns;
				int			end		= 0;

				p = WinScreen[row];
				for (col = 0; col < Columns; col++)
				{
					if (p[col] != ' ')
					{
						start = col;
						break;
					}
				}
				for (col = Columns - 1; col >= 0; col--)
				{
					if (p[col] != ' ')
					{
						end = col;
						break;
					}
				}
				for (col = start; col <= end; col++)
				{
#if defined(KANJI) && defined(SYNTAX)
					p[Columns + col]  = CMODE;
#else
					p[Columns + col] |= CMODE;
#endif
					cmode = TRUE;
				}
				KillTimer(hWnd, TRIPLE_TIME);
				do_trip = FALSE;
				if (cmode)
				{
					rcWindow.left	= 0;
					rcWindow.right	= Columns * v_xchar - 1;
					rcWindow.top	= row * v_ychar;
					rcWindow.bottom	= (row + 1) * v_ychar - 1;
					InvalidateRect(hWnd, &rcWindow, FALSE);
				}
			}
			else
			{
				if (cmode)
					clear_cmode(hWnd);
				cs_row = row;
				cs_col = col;
				cmode = TRUE;
			}
		}
		else if (wp == NULL)
		{
			if (((wp = get_statusline(lParam, &row)) != NULL) && (State & NORMAL))
			{
				if (curwin != wp)
					win_enter(wp, TRUE);
				curwin->w_set_curswant = TRUE;
				updateScreen(CLEAR);
				cursor_refresh(hWnd);
				if (VIsual.lnum != 0)
					clear_visual(hWnd);
				selstatus = wp;
				cs_row = row;
			}
		}
		else if (wp == NULL || pos.lnum == 0)
			;
		else if (State & NORMAL)
		{
			wp->w_cursor = pos;
			if (curwin != wp)
				win_enter(wp, TRUE);
			curwin->w_set_curswant = TRUE;
			updateScreen(NOT_VALID);
			cursor_refresh(hWnd);
			if (VIsual.lnum != 0)
				clear_visual(hWnd);
			if (pos.lnum != 0 && SetTimer(hWnd, MOUSE_TIME, 60, NULL) != 0)
			{
				vmode = FALSE;
				selwin = wp;
				selpos = pos;
				if (wParam & MK_SHIFT)
					selpos.col = MAXCOL;
				else if (wParam & MK_CONTROL)
					vmode = TRUE;
				SetCapture(hWnd);
			}
		}
		else if (State & INSERT)
		{
			start_arrow();
			wp->w_cursor = pos;
			win_enter(wp, TRUE);
			curwin->w_set_curswant = TRUE;
			cursor_refresh(hWnd);
#ifdef FEPCTRL
			if (curbuf->b_p_fc && fep_get_mode())
				fep_win_sync(hVimWnd);
#endif
		}
		return(0);
	case WM_MOUSEMOVE:
		if (mouse_pos == lParam)
			return(0);
		mouse_pos = lParam;
		if (!s_cursor)
		{
			s_cursor = TRUE;
			ShowCursor(TRUE);
		}
		updown = 0;
		leftright = 0;
		if (cmode && (wParam & MK_LBUTTON))
		{
			get_linecol(lParam, &pos, &ce_row, &ce_col);
			draw_cmode(hVimWnd, cs_row, cs_col, ce_row, ce_col);
		}
		else if ((selwin != NULL) && (wParam & MK_LBUTTON))
		{
			GetClientRect(hVimWnd, &rcWindow);
			if ((short)HIWORD(lParam) < 0)
			{
				if ((short)HIWORD(lParam) < -(v_ychar * 5))
					updown = 5;
				else
					updown = 1;
				return(0);
			}
			else if (rcWindow.bottom < HIWORD(lParam))
			{
				if ((HIWORD(lParam) - rcWindow.bottom) > (v_ychar * 5))
					updown = -5;
				else
					updown = -1;
				return(0);
			}
			if ((short)LOWORD(lParam) < 0)
			{
				leftright = -1;
				lParam = MAKELONG(1, HIWORD(lParam));
			}
			else if (rcWindow.right < LOWORD(lParam))
			{
				leftright = 1;
				lParam = MAKELONG(rcWindow.right - 1, HIWORD(lParam));
			}
			if (((wp = get_linecol(lParam, &pos, &row, &col)) != NULL)
														&& (wp == selwin))
			{
				if (selpos.col == pos.col && selpos.lnum == pos.lnum)
					return(0);
				if (VIsual.lnum == 0)
				{
					VIsual = selpos;
					Visual_block = vmode;
					if (selpos.col == MAXCOL)
						wp->w_cursor.col = 0;
					else
						wp->w_cursor.col = selpos.col;
					wp->w_cursor.lnum = selpos.lnum;
					cursor_refresh(hWnd);
				}
				if (pos.lnum != 0)
					wp->w_cursor = pos;
				cursor_refresh(hWnd);
			}
			if (row < selwin->w_winpos)
			{
				updown = 1;
				leftright = 0;
			}
			else if ((selwin->w_winpos + selwin->w_height) <= row)
			{
				updown = -1;
				leftright = 0;
			}
			else
				updown = 0;
		}
		else if ((selstatus != NULL) && (wParam & MK_LBUTTON))
		{
			i = min(Rows - 1, (HIWORD(lParam) - 1) / v_ychar);
			if (i > cs_row)
				win_setheight(curwin->w_height + (i - cs_row));
			else if (i < cs_row)
				win_setheight(curwin->w_height - (cs_row - i));
			if (i == (curwin->w_winpos + curwin->w_height))
				cs_row = i;
			cursor_refresh(hWnd);
		}
		return(0);
	case WM_LBUTTONUP:
		if (selwin != NULL)
		{
			wp = get_linecol(lParam, &pos, &row, &col);
			if (wp == selwin
					&& selpos.col == pos.col && selpos.lnum == selpos.lnum)
				;
			else if (VIsual.lnum)
			{
				if ((wp != NULL) && (wp == selwin) && (pos.lnum != 0))
					wp->w_cursor = pos;
				updateScreen(VALID);
				cursor_refresh(hWnd);
			}
			selwin = NULL;
			KillTimer(hWnd, MOUSE_TIME);
			ReleaseCapture();
		}
		selstatus = NULL;
		return(0);
	case WM_LBUTTONDBLCLK:
		selwin = NULL;
		if (cmode)
			clear_cmode(hWnd);
		clear_visual(hWnd);
#if defined(KANJI) && defined(SYNTAX)
		if (syntax_on() && cmode)
			updateScreen(CLEAR);
#endif
		cmode = FALSE;
		wp = get_linecol(lParam, &pos, &row, &col);
		if (GetKeyState(VK_MENU) & 0x8000)
		{
			p = WinScreen[row];
#ifdef KANJI
			if (col > 0 && ISkanjiPosition(p, col + 1) == 2)
				col--;
			if (ISkanji(p[col]))
			{
				int class;
				class = jpcls(p[col], p[col+1]);
				while (col > 0 && class == jpcls(p[col-2],p[col-1]))
					col -= 2;
				while (col < Columns && class == jpcls(p[col],p[col+1]))
				{
# ifdef SYNTAX
					p[Columns + col + 0]  = CMODE;
					p[Columns + col + 1]  = CMODE;
# else
					p[Columns + col + 0] |= CMODE;
					p[Columns + col + 1] |= CMODE;
# endif
					col += 2;
					cmode = TRUE;
				}
			}
			else
			{
				while (col > 0 && ISkanjiPosition(p, col) == 0 && isidchar(p[col - 1]))
					--col;
				while (col < Columns && !ISkanji(p[col]) && isidchar(p[col]))
				{
# ifdef SYNTAX
					p[Columns + col]  = CMODE;
# else
					p[Columns + col] |= CMODE;
# endif
					col++;
					cmode = TRUE;
				}
			}
#else
			while (col > 0 && isidchar(ptr[col - 1]))
				--col;
			while (col < Columns && isidchar(p[col]))
			{
				p[Columns + col] |= CMODE;
				++col;
				cmode = TRUE;
			}
#endif
			if (cmode)
			{
				rcWindow.left	= 0;
				rcWindow.right	= Columns * v_xchar - 1;
				rcWindow.top	= row * v_ychar;
				rcWindow.bottom	= (row + 1) * v_ychar - 1;
				InvalidateRect(hWnd, &rcWindow, FALSE);
				if (SetTimer(hWnd, TRIPLE_TIME, GetDoubleClickTime(), NULL) != 0)
					do_trip = TRUE;
			}
		}
		else if (wp == NULL || pos.lnum == 0)
			;
		else if (State & NORMAL)
		{
			if (Visual_block)
				return(0);
			if (!(GetKeyState(VK_CONTROL) & 0x8000))
			{
				cbuf[c_end++] = 'g';
				cbuf[c_end++] = 'g';
				return(0);
			}
			p = ml_get_buf(wp->w_buffer, pos.lnum, FALSE);
			i = pos.col;
			while (i > 0 && !iswhite(p[i - 1]))
#ifdef KANJI
				if (ISkanjiPosition(p, i) == 2 && isjpspace(p[i-2], p[i-1]))
					break;
				else
#endif
				--i;
			p = &p[i];
			i = 0;
			while (p[i] != NUL && !iswhite(p[i]))
#ifdef KANJI
				if (isjpspace(p[i], p[i + 1]))
					break;
				else
#endif
				++i;
			if (i == 0 || i >= MAXPATHL || i >= IOSIZE)
				return(0);
			strncpy(IObuff, p, i);
			IObuff[i] = NUL;
			rc = (int)ShellExecute(NULL, NULL, IObuff, NULL, ".", SW_SHOW);
			if (!(rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND
					|| rc == SE_ERR_NOASSOC || rc == SE_ERR_ASSOCINCOMPLETE))
				return(0);
			if (FullName(IObuff, NameBuff, MAXPATHL) == OK
									&& strcmp(IObuff, NameBuff) != 0)
			{
				rc = (int)ShellExecute(NULL, NULL, NameBuff, NULL, ".", SW_SHOW);
				if (!(rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND
						|| rc == SE_ERR_NOASSOC || rc == SE_ERR_ASSOCINCOMPLETE))
					return(0);
			}
			p = ml_get_buf(wp->w_buffer, pos.lnum, FALSE);
			i = pos.col;
			while (i > 0 && p[i - 1] != '"')
				--i;
			if (i == 0 && p[0] != '"')
				return(0);
			p = &p[i];
			i = 0;
			while (p[i] != NUL && p[i] != '"')
				++i;
			if (i == 0 || i >= MAXPATHL || i >= IOSIZE || p[i] == NUL)
				return(0);
			strncpy(IObuff, p, i);
			IObuff[i] = NUL;
			ShellExecute(NULL, NULL, IObuff, NULL, ".", SW_SHOW);
		}
		return(0);
	case WM_RBUTTONUP:
		selwin = NULL;
		redraw = TRUE;
		if (!s_cursor)
		{
			s_cursor = TRUE;
			ShowCursor(TRUE);
		}
		GetWindowRect(hWnd, &rcWindow);
		hEdit = CreatePopupMenu();
		if (cmode)
		{
			AppendMenu(hEdit,  MF_STRING,   IDM_YANK,   "&Yank");
			if (State == CMDLINE || State == INSERT || State == REPLACE)
			{
				AppendMenu(hEdit,  MF_STRING,   IDM_PASTE,  "&Paste");
				if (GetKeyState(VK_MENU) & 0x8000)
				{
					redraw = FALSE;
					yank_cmode(hWnd, FALSE);
					clear_cmode(hWnd);
#if defined(KANJI) && defined(SYNTAX)
					if (syntax_on())
						updateScreen(CLEAR);
#endif
					cmode = FALSE;
				}
			}
		}
		else if (VIsual.lnum == 0)
		{
			AppendMenu(hEdit,  MF_STRING,   IDM_PASTE,  "&Put");
			if (State & NORMAL)
			{
				if ((curwin->w_arg_idx + 1) < arg_count)
					AppendMenu(hEdit,  MF_STRING,   IDM_NFILE,  "&Next");
				if (curwin->w_arg_idx >= 1)
					AppendMenu(hEdit,  MF_STRING,   IDM_PFILE,  "P&rev");
			}
		}
		else if (State & NORMAL)
		{
			AppendMenu(hEdit,  MF_STRING,   IDM_DELETE, "&Delete");
			AppendMenu(hEdit,  MF_STRING,   IDM_YANK,   "&Yank");
			AppendMenu(hEdit,  MF_STRING,   IDM_PASTE,  "P&ut");
			AppendMenu(hEdit,  MF_SEPARATOR,0,			NULL);
			AppendMenu(hEdit,  MF_STRING,   IDM_CLICK,  "&Run");
			if (strlen(config_printer))
			{
				AppendMenu(hEdit,  MF_SEPARATOR,0,			NULL);
				AppendMenu(hEdit,  MF_STRING,   IDM_PRINT,  "&Print");
			}
		}
		if (redraw)
		{
			AppendMenu(hEdit,  MF_SEPARATOR,0,			NULL);
			AppendMenu(hEdit,  MF_STRING,   IDM_CANCEL, "&Cancel");
			TrackPopupMenu(hEdit, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
					rcWindow.left + LOWORD(lParam),
					rcWindow.top + HIWORD(lParam) + (cmode || VIsual.lnum == 0 ? v_ychar - 2 : -2),
					0, hWnd, NULL);
		}
		DestroyMenu(hEdit);
		return(0);
#ifdef WM_MOUSEWHEEL
	case WM_MOUSEWHEEL:
		{
			WPARAM			UpDown;

			if ((signed)wParam < 0)
				UpDown = SB_LINEDOWN;
			else
				UpDown = SB_LINEUP;
			for (i = 0; i < iScrollLines; i++)
				PostMessage(hWnd, WM_VSCROLL, UpDown, 0);
		}
		return(0);
#endif
	case WM_VSCROLL:
		selwin = NULL;
		if (cmode)
		{
			clear_cmode(hWnd);
#if defined(KANJI) && defined(SYNTAX)
			if (syntax_on())
				updateScreen(CLEAR);
#endif
			cmode = FALSE;
		}
		clear_visual(hWnd);
		if (!((State & NORMAL) || (State & INSERT)))
			return(0);
		if (State & INSERT)
			start_arrow();
		switch (LOWORD(wParam)) {
		case SB_LINEDOWN:
			if (curwin->w_empty_rows
							&& curwin->w_botline >= curbuf->b_ml.ml_line_count)
				;
			else
			{
				scrollup(1);
				coladvance(curwin->w_curswant);
				updateScreen(VALID);
			}
			break;
		case SB_LINEUP:
			scrolldown(1);
			coladvance(curwin->w_curswant);
			updateScreen(VALID);
			break;
		case SB_PAGEDOWN:
			if (curwin->w_empty_rows
							&& curwin->w_botline >= curbuf->b_ml.ml_line_count)
				;
			else
				(void)onepage(FORWARD, 1);
			break;
		case SB_PAGEUP:
			if (curwin->w_cursor.lnum <= (Rows - 1))
			{
				curwin->w_cursor.lnum = 1;
				beginline(TRUE);
			}
			else
				(void)onepage(BACKWARD, 1);
			break;
		case SB_THUMBTRACK:
			{
				linenr_t	old = curbuf->b_ml.ml_line_count;
				linenr_t	lnum = curbuf->b_ml.ml_line_count;
				INT			high = curwin->w_height - curwin->w_status_height;
				linenr_t	disp = (high * lnum) / curbuf->b_ml.ml_line_count;
				INT			param;

				while (lnum > 0x7ff)
					lnum >>= 1;
				if (HIWORD(wParam) < 1)
					param = 0;
				else
					param = ((HIWORD(wParam) + 1) * lnum) / (lnum - disp);
				lnum = (param * curbuf->b_ml.ml_line_count) / lnum;
				if (lnum <= 0)
					lnum = 1;
				else if (lnum >= curbuf->b_ml.ml_line_count)
					lnum = curbuf->b_ml.ml_line_count;
				curwin->w_cursor.lnum = lnum;
				beginline(TRUE);
			}
			break;
		case SB_ENDSCROLL:
			break;
		}
		cursor_refresh(hWnd);
		return(0);
	case WM_QUERYENDSESSION:
		hWnd = CreateDialog(hInst, "TERM", NULL, LoadDialog);
		ShowWindow(hWnd, SW_NORMAL);
		Sleep(1000);
		ml_sync_all(FALSE);
		ctrlc_pressed = TRUE;
		for (buf = firstbuf; buf != NULL; buf = buf->b_next)
		{
			if (buf->b_changed && (autowrite(buf) == FAIL))
			{
				DestroyWindow(hWnd);
				return(TRUE);
			}
		}
		getout(0);
		return(TRUE);
	case WM_ENDSESSION:
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_CLOSE:
		if (config_tray && BenchTime == 0 && !bIClose
				&& (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
										&& ver_info.dwMajorVersion == 3)))
		{
			ShowWindow(hWnd, SW_HIDE);
			GetWindowText(hWnd, nIcon.szTip, sizeof(nIcon.szTip));
			Shell_NotifyIcon(NIM_ADD, &nIcon);
			return(0);
		}
		if (bIClose && !(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
											&& ver_info.dwMajorVersion == 3))
		{
			Shell_NotifyIcon(NIM_DELETE, &nIcon);
			ShowWindow(hWnd, SW_SHOW);
			OpenIcon(hWnd);
			TopWindow(hVimWnd);
		}
		bIClose = FALSE;
		ctrlc_pressed = TRUE;
		for (buf = firstbuf; BenchTime == 0 && buf != NULL; buf = buf->b_next)
		{
			if (buf->b_changed && (autowrite(buf) == FAIL))
			{
				i = MessageBox(hWnd, "No write since last change. Write quit ?", szAppName, MB_YESNOCANCEL|MB_DEFBUTTON2);
				switch (i) {
				case IDYES:
					i = 0;
					for (buf = firstbuf; buf != NULL; buf = buf->b_next)
					{
						if (buf->b_changed && buf->b_filename == NULL)
						{
							char		batbuf[MAXPATHL];

							for (i++; i <= 0xfffff; i++)
							{
								batbuf[0] = '\0';
								if (p_dir != NULL && *p_dir != NUL)
								{
									if (*p_dir == '>')
										STRCPY(batbuf, p_dir + 1);
									else
										STRCPY(batbuf, p_dir);
									if (!ispathsep(*(batbuf + STRLEN(batbuf) - 1)))
										STRCAT(batbuf, PATHSEPSTR);
								}
								sprintf(&batbuf[STRLEN(batbuf)], "bak%05x.txt", i);
								if (getperm(batbuf) < 0)
								{
									buf->b_filename = strsave(batbuf);
									break;		/* for loop */
								}
							}
						}
					}
					docmdline(":wqall!");
					break;
				case IDNO:
					docmdline(":qall!");
					break;
				case IDCANCEL:
					return(0);
				}
				break;
			}
		}
		bWClose = TRUE;
		docmdline(":qall!");
		/* no break */
	default:
#ifndef NO_WHEEL
		if (uiMsh_MsgMouseWheel != 0 && uMsg == uiMsh_MsgMouseWheel)
			return(SendMessage(hWnd, WM_MOUSEWHEEL, wParam, lParam));
#endif
		break;
	}
	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

/*
 *
 */
void
wincmd_paste()
{
	if (GuiWin)
		SendMessage(hVimWnd, WM_COMMAND, IDM_PASTE, 0);
	NoMap = FALSE;
}

/*
 *
 */
void
wincmd_cut()
{
	if (GuiWin)
		SendMessage(hVimWnd, WM_COMMAND, IDM_YANK, 0);
}

/*
 *
 */
void
wincmd_delete()
{
	if (GuiWin)
		SendMessage(hVimWnd, WM_COMMAND, IDM_DELETE, 0);
}

/*
 *
 */
void
wincmd_active()
{
	if (GuiWin)
	{
		SetForegroundWindow(hVimWnd);
		SetTimer(hVimWnd, SHOW_TIME, config_show, NULL);
	}
}

/*
 *
 */
void
wincmd_redraw()
{
	if (GuiWin)
		bSyncPaint = TRUE;
}

/*
 *
 */
int
wincmd_grep(linep, filep)
char	*	linep;
char	*	filep;
{
	char					name[MAXPATHL];
	char					command[1024];
	STARTUPINFO				si;
	PROCESS_INFORMATION		pi;

	if (GuiWin && config_grepwin)
	{
		GetModuleFileName(NULL, name, sizeof(name));
		memset(&pi, 0, sizeof(pi));
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		if (config_ini)
			sprintf(command, "%s -I %s -no -v +%s %s", name, GuiIni,    linep, filep);
		else
			sprintf(command, "%s -n%d  -no -v +%s %s", name, GuiConfig, linep, filep);
		if (CreateProcess(NULL, command, NULL, NULL, FALSE,
				CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) == TRUE)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return(TRUE);
		}
	}
	return(FALSE);
}

/*
 *
 */
int
wincmd_togle()
{
	HWND			hWnd;
	char			buf[128];

	if (GuiWin)
	{
		hWnd = GetFirstSibling(hVimWnd);
		while (IsWindow(hWnd))
		{
			GetClassName(hWnd, buf, sizeof(buf));
			if (strcmp(buf, szAppName) == 0 && hVimWnd != hWnd)
			{
				TopWindow(hWnd);
				return(TRUE);
			}
			hWnd = GetNextSibling(hWnd);
		}
	}
	return(FALSE);
}

/*
 *
 */
int
wincmd_rotate()
{
	HWND			hWnd;
	char			buf[128];

	if (GuiWin)
	{
		hWnd = GetLastSibling(hVimWnd);
		while (IsWindow(hWnd))
		{
			GetClassName(hWnd, buf, sizeof(buf));
			if (strcmp(buf, szAppName) == 0 && hVimWnd != hWnd)
			{
				TopWindow(hWnd);
				return(TRUE);
			}
			hWnd = GetPrevSibling(hWnd);
		}
	}
	return(FALSE);
}

/*
 *
 */
	void
vim_delay()
{
	delay(500);
}

/*
 * this version of remove is not scared by a readonly (backup) file
 */
	int
vim_remove(name)
	char_u         *name;
{
	setperm(name, S_IREAD|S_IWRITE);    /* default permissions */
	return unlink(name);
}

	static void
ScrollBar()
{
	SCROLLINFO		si;
	static UINT		sbar = (-1);
	static SCROLLINFO		oSi;

	if (curbuf == NULL || curwin == NULL)
		return;
	if (config_sbar)
	{
		INT			nPos, nPage;
		INT			high = curwin->w_height - curwin->w_status_height;
		linenr_t	lnum;
		linenr_t	elnum = curwin->w_empty_rows ? curwin->w_empty_rows - 1 : 0;
		INT			shift = 0;

		if (high < 1)
			high = 1;

		lnum = curbuf->b_ml.ml_line_count + elnum;
		while (lnum > 0x7ff)
		{
			shift++;
			lnum >>= 1;
		}
		high >>= shift;
		if (high <= 0)
			high = 1;
		nPage = ((curwin->w_botline - curwin->w_topline - 1) * lnum)
										/ (curbuf->b_ml.ml_line_count + elnum);
		if (nPage < 1)
			nPage = 1;
		nPos  = (curwin->w_topline >> shift) - 1;
		if (nPos < 0)
			nPos = 0;

		memset(&si, 0, sizeof(si));
		si.cbSize = sizeof(si);

		if (config_sbar != sbar)
		{
			si.fMask = SIF_ALL;
			si.nMin  = 0;
			si.nMax  = 1;
			si.nPos  = 0;
			si.nPage = 1;
			SetScrollInfo(hVimWnd, SB_VERT, &si, TRUE);
		}

		si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		si.nMin  = 0;
		si.nMax  = lnum - 1;
		si.nPos  = nPos;
		si.nPage = nPage;
		if ((curwin->w_botline - 1) >= curbuf->b_ml.ml_line_count)
			si.nPage = lnum - nPos;
		if (curwin->w_topline <= 1
					&& (curwin->w_botline - 1) >= curbuf->b_ml.ml_line_count)
			si.nMax = 0;
		if (memcmp(&oSi, &si, sizeof(si)) != 0)
		{
			SetScrollInfo(hVimWnd, SB_VERT, &si, TRUE);
			memcpy(&oSi, &si, sizeof(si));
		}
		sbar = config_sbar;
	}
	else if (config_sbar != sbar)
	{
		sbar = config_sbar;
		memset(&oSi, 0, sizeof(oSi));
		oSi.cbSize = sizeof(oSi);
		oSi.fMask  = SIF_ALL;
		SetScrollInfo(hVimWnd, SB_VERT, &oSi, TRUE);
	}
}

/*
 * mch_write(): write the output buffer to the screen
 */
	void
mch_write(s, len)
	char_u         *s;
	int             len;
{
	char_u         *p;
	WORD			row,
					col;
	static DWORD	btime = 0;

	s[len] = '\0';
	if (GuiWin)
	{
		RECT			rect;

		ScrollBar();
		while (len--)
		{
			if (s[0] == '\n')
			{
				v_col = 0;
				v_row++;
				if (v_row >= (Rows - 1))
				{
					v_row = Rows - 1;
					if (!config_bitmap)
						ScrollWindow(hVimWnd, 0, v_ychar, NULL, NULL);
					else
						InvalidateRect(hVimWnd, NULL, FALSE);
				}
				s++;
				continue;
			}
			else if (s[0] == '\r')
			{
				v_col = 0;
				s++;
				continue;
			}
			else if (s[0] == '\b')		/* backspace */
			{
				if (--v_col < 0)
					v_col = 0;
				s++;
				continue;
			}
			else if (s[0] == '\a' || s[0] == '\007')
			{
				if (p_vb)
				{
					do_vb = TRUE;
					InvalidateRect(hVimWnd, NULL, FALSE);
					UpdateWindow(hVimWnd);
					delay(50);
					do_vb = FALSE;
					InvalidateRect(hVimWnd, NULL, FALSE);
					UpdateWindow(hVimWnd);
					Sleep(50);
				}
				else if (config_wave)
					sndPlaySound(config_wavefile, SND_ASYNC|SND_NOSTOP);
				else if ((btime + 50) < GetTickCount())
				{
					MessageBeep(0);
					btime = GetTickCount();
					if (BenchTime)
						ctrlc_pressed = TRUE;
				}
				s++;
				continue;
			} else if (s[0] == ESC && len > 1 && s[1] == '|') {
				switch (s[2]) {
				case 'v':
					if (v_cursor && v_focus)
						HideCaret(hVimWnd);
					v_cursor = FALSE;
					goto wgot3;

				case 'V':
					if (v_cursor != TRUE && v_focus)
						ShowCaret(hVimWnd);
					v_cursor = TRUE;
					MoveCursor(hVimWnd);
					goto wgot3;

				case 'J':	/* clear screen */
					rect.left	= 0;
					rect.right	= Columns * v_xchar;
					rect.top	= 0;
					rect.bottom	= Rows * v_ychar;
					if (!config_bitmap)
					{
						HDC			hDC;
						HBRUSH		hbrush;
						HBRUSH		holdbrush;
						BOOL		hide = FALSE;

						if (v_cursor && v_focus)
						{
							HideCaret(hVimWnd);
							hide = TRUE;
						}
						UpdateWindow(hVimWnd);
						hDC = GetDC(hVimWnd);
						hbrush	= CreateSolidBrush(*v_bgcolor);
						holdbrush = SelectObject(hDC, hbrush);
						FillRect(hDC, &rect, hbrush);
						SelectObject(hDC, holdbrush);
						DeleteObject(hbrush);
						ReleaseDC(hVimWnd, hDC);
						if (hide)
							ShowCaret(hVimWnd);
					}
					else
						InvalidateRect(hVimWnd, &rect, FALSE);
					goto wgot3;

				case 'K':	/* clreol */
					rect.left	= v_col * v_xchar;
					rect.right	= Columns * v_xchar;
					rect.top	= v_row * v_ychar;
					rect.bottom	= rect.top + v_ychar;
					if (!config_bitmap)
					{
						HDC			hDC;
						HBRUSH		hbrush;
						HBRUSH		holdbrush;
						BOOL		hide = FALSE;

						if (v_cursor && v_focus)
						{
							HideCaret(hVimWnd);
							hide = TRUE;
						}
						UpdateWindow(hVimWnd);
						hDC = GetDC(hVimWnd);
						hbrush	= CreateSolidBrush(*v_bgcolor);
						holdbrush = SelectObject(hDC, hbrush);
						FillRect(hDC, &rect, hbrush);
						SelectObject(hDC, holdbrush);
						DeleteObject(hbrush);
						ReleaseDC(hVimWnd, hDC);
						if (hide)
							ShowCaret(hVimWnd);
					}
					else
						InvalidateRect(hVimWnd, &rect, FALSE);
					goto wgot3;

				case 'L':	/* insline */
					rect.left	= 0;
					rect.top	= v_row * v_ychar;
					rect.right	= Columns * v_xchar;
					rect.bottom	= v_region * v_ychar;
					if (!config_bitmap)
						ScrollWindow(hVimWnd, 0, v_ychar, NULL, &rect);
					else
						InvalidateRect(hVimWnd, &rect, FALSE);
					goto wgot3;

				case 'M':	/* delline */
					rect.left	= 0;
					rect.top	= v_row * v_ychar;
					rect.right	= Columns * v_xchar;
					rect.bottom	= v_region * v_ychar;
					if (!config_bitmap)
						ScrollWindow(hVimWnd, 0, -v_ychar, NULL, &rect);
					else
						InvalidateRect(hVimWnd, &rect, FALSE);
			wgot3:  s += 3;
					len -= 2;
					continue;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					p = s + 2;
					row = getdigits(&p);        /* no check for length! */
					if (p > s + len)
						break;
					if (*p == ';')
					{
						++p;
						col = getdigits(&p);    /* no check for length! */
						if (p > s + len)
							break;
						if (*p == 'H')
						{
							if (!RedrawingDisabled && !((row - 2) <= v_row && v_row <= row))
								UpdateWindow(hVimWnd);
							v_col = col - 1;
							v_row = row - 1;
							len -= p - s;
							s = p + 1;
							continue;
						}
						else if (*p == 'S')
						{
							v_region = col + 1;
							len -= p - s;
							s = p + 1;
							continue;
						}
					}
					else if (*p == 'm')
					{
						/* video color */
						len -= p - s;
						s = p + 1;
						continue;
					}
					else if (*p == 'L')
					{
						/* insline(row) */
						rect.left	= 0;
						rect.top	= v_row * v_ychar;
						rect.right	= Columns * v_xchar;
						rect.bottom	= v_region * v_ychar;
						if (!config_bitmap)
							ScrollWindow(hVimWnd, 0, v_ychar * row, NULL, &rect);
						else
							InvalidateRect(hVimWnd, &rect, FALSE);
						len -= p - s;
						s = p + 1;
						continue;
					}
					else if (*p == 'M')
					{
						/* delline(row) */
						rect.left	= 0;
						rect.top	= v_row * v_ychar;
						rect.right	= Columns * v_xchar;
						rect.bottom	= v_region * v_ychar;
						if (!config_bitmap)
							ScrollWindow(hVimWnd, 0, -(v_ychar * row), NULL, &rect);
						else
							InvalidateRect(hVimWnd, &rect, FALSE);
						len -= p - s;
						s = p + 1;
						continue;
					}
				}
			}
			else
			{
				int           prefix = 1;

				if (len >= 2)
				{
					prefix = strcspn(s, "\n\r\a\b\033\007");
					if (prefix > (Columns - v_col))
						prefix = Columns - v_col;
					else if (prefix == 0)
						prefix = 1;
				}
				rect.left	= v_col * v_xchar;
				rect.right	= rect.left + v_xchar * prefix;
				rect.right	= rect.left + v_xchar * (prefix + italicplus());
				rect.top	= v_row * v_ychar;
				rect.bottom	= rect.top + v_ychar;
				InvalidateRect(hVimWnd, &rect, FALSE);
				v_col += prefix;
				s += prefix;
				len -= prefix - 1;
				if (v_col >= Columns)
				{
					v_col = 0;
					v_row++;
					if (v_row >= Rows)
					{
						v_row = Rows - 1;
						if (!config_bitmap)
							ScrollWindow(hVimWnd, 0, v_ychar, NULL, NULL);
						else
							InvalidateRect(hVimWnd, NULL, FALSE);
					}
				}
				continue;
			}
			s++;
		}
	}
	else if (IsTelnet || !term_console)
		write(1, s, (unsigned)len);
	else
	{
		while (len--) {

			/* optimization: use one single WriteConsole for runs of text,
			   rather than calling putch() multiple times.  It ain't curses,
			   but it helps. */

			DWORD           prefix = strcspn(s, "\n\r\a\033");

			if (prefix) {
				DWORD           nwritten;

				if (WriteConsole(hConOut, s, prefix, &nwritten, 0)) {

					len -= (nwritten - 1);
					s += nwritten;
				}
				continue;
			}

			if (s[0] == '\n') {
				if (ntcoord.Y == (Rows - 1)) {
					gotoxy(1, ntcoord.Y + 1);
					scroll();
				} else {
					gotoxy(1, ntcoord.Y + 2);
				}
				s++;
				continue;
			} else if (s[0] == '\r') {
				gotoxy(1, ntcoord.Y + 1);
				s++;
				continue;
			} else if (s[0] == '\a') {
				vbell();
				s++;
				continue;
			} else if (s[0] == ESC && len > 1 && s[1] == '|') {
				switch (s[2]) {

				case 'v':
					cursor_visible(0);
					goto got3;

				case 'V':
					cursor_visible(1);
					goto got3;

				case 'J':
					clrscr();
					goto got3;

				case 'K':
					clreol();
					goto got3;

				case 'L':
					insline(1);
					goto got3;

				case 'M':
					delline(1);
			got3:   s += 3;
					len -= 2;
					continue;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					p = s + 2;
					row = getdigits(&p);        /* no check for length! */
					if (p > s + len)
						break;
					if (*p == ';') {
						++p;
						col = getdigits(&p);    /* no check for length! */
						if (p > s + len)
							break;
						if (*p == 'H') {
							gotoxy(col, row);
							len -= p - s;
							s = p + 1;
							continue;
						}
					} else if (*p == 'm') {
						if (row == 0)
							normvideo();
						else
							textattr(row);
						len -= p - s;
						s = p + 1;
						continue;
					} else if (*p == 'L') {
						insline(row);
						len -= p - s;
						s = p + 1;
						continue;
					} else if (*p == 'M') {
						delline(row);
						len -= p - s;
						s = p + 1;
						continue;
					}
				}
			}
			putch(*s++);
		}
	}
}

/*
 *  Keyboard translation tables.
 *  (Adopted from the MSDOS port)
 */

#define KEYPADLO    0x47
#define KEYPADHI    0x53

#define PADKEYS     (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x)    (KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Wait until console input is available
 */

	static int
WaitForChar(msec)
	int             msec;
{
	if (GuiWin)
	{
		MSG				msg;
		int				settime = FALSE;
#ifdef FEPCTRL
		int				fepsync = FALSE;
#endif

		if (msec == 0)
			do_time = TRUE;
		else if (msec > 0)
		{
			do_time = FALSE;
			if (SetTimer(hVimWnd, KEY_TIME, msec, NULL) != 0)
				settime = TRUE;
		}
		else
			do_time = FALSE;
		for (;;)
		{
			if (do_resize)
			{
				if (settime)
					KillTimer(hVimWnd, KEY_TIME);
				return FALSE;
			}
			if (kbhit())
			{
				if (settime)
					KillTimer(hVimWnd, KEY_TIME);
				return TRUE;
			}
			if (do_time)
				break;
#ifdef FEPCTRL
			if (curbuf->b_p_fc && fep_get_mode())
			{
				if (fepsync == FALSE)
					fep_win_sync(hVimWnd);
				fepsync = TRUE;
			}
			else
				fepsync = FALSE;
#endif
			if (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
			{
				if (!TranslateAccelerator(hVimWnd, hAcc, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				continue;
			}
			if (GetMessage(&msg, NULL, 0, 0))
			{
				if (!TranslateAccelerator(hVimWnd, hAcc, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		if (settime)
			KillTimer(hVimWnd, KEY_TIME);
		return FALSE;
	}
	else
	{
		DWORD           count;
		int             ch;
		int             scan;
		int             retval = 0;
		DWORD			time = GetTickCount() + msec;

retry:
		if (WaitForSingleObject(hConIn, msec) == WAIT_OBJECT_0) {
			count = 0;
			(void)PeekConsoleInput(hConIn, &ir, 1, &count);
			if (count > 0) {
				ch = ir.Event.KeyEvent.uChar.AsciiChar;
				scan = ir.Event.KeyEvent.wVirtualScanCode;
#ifndef notdef
				if (!ch)
					ch = isctlkey();
#endif
				if (((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown) &&
					(ch || (iskeypad(scan)))) {
					retval = 1;     /* Found what we sought */
				}
			} else {                /* There are no events in console event queue */
				retval = 0;
			}
		}
		if (retval == 0
#ifdef FEPCTRL
					&& curbuf->b_p_fc && fep_get_mode() == 0
#endif
								&& time > GetTickCount()) {
			if (count)
				(void)ReadConsoleInput(hConIn, &ir, 1, &count);
			goto retry;
		}
		return retval;
	}
}

static int pending = 0;

	static int
tgetch()
{
	int             valid = 0;
	DWORD           count;
	unsigned short int scan;
	unsigned char   ch;

	if (pending)
	{
		ch = pending;
		pending = 0;
	}
	else
	{

		valid = 0;
		while (!valid)
		{
			(void)ReadConsoleInput(hConIn, &ir, 1, &count);
			if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT)
			{
				set_winsize(Rows, Columns, FALSE);
			}
			else
			{
				if ((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown)
				{
					ch = ir.Event.KeyEvent.uChar.AsciiChar;
					scan = ir.Event.KeyEvent.wVirtualScanCode;
#ifndef notdef
					if (!ch)
						ch = isctlkey();
#endif
					if (ch || (iskeypad(scan)))
						valid = 1;
				}
			}
		}
		if (!ch)
		{
			pending = scan;
			ch = 0;
		}
	}
	return ch;
}


	static int
kbhit()
{
	if (GuiWin)
		return(c_next < c_end);
	else
	{
		int             done = 0;   /* true =  "stop searching"        */
		int             retval;     /* true =  "we had a match"        */
		DWORD           count;
		unsigned short int scan;
		unsigned char   ch;

		if (pending)
			return 1;

		done = 0;
		retval = 0;
		while (!done)
		{
			count = 0;
			PeekConsoleInput(hConIn, &ir, 1, &count);
			if (count > 0) {
				ch = ir.Event.KeyEvent.uChar.AsciiChar;
				scan = ir.Event.KeyEvent.wVirtualScanCode;
#ifndef notdef
				if (!ch)
					ch = isctlkey();
#endif
				if (((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown) &&
						(ch || (iskeypad(scan)) ))
				{
					done = 1;       /* Stop looking         */
					retval = 1;     /* Found what we sought */
				} else              /* Discard it, its an insignificant event */
					ReadConsoleInput(hConIn, &ir, 1, &count);
			}
			else                	/* There are no events in console event queue */
			{
				done = 1;           /* Stop looking               */
				retval = 0;
			}
		}
		return retval;
	}
}

	static int
getch()
{
	int				c;

	c = cbuf[c_next++];
	if (c_next == c_end)
		c_next = c_end = 0;
	if (GuiWin && c_ind > 0 && (c_next == c_ind || c_end == 0))
		c_ind = -1;
	return c;
}

/*
 * GetChars(): low level input funcion.
 * Get a characters from the keyboard.
 * If time == 0 do not wait for characters.
 * If time == n wait a short time for characters.
 * If time == -1 wait forever for characters.
 */
	int
GetChars(buf, maxlen, time)
	char_u         *buf;
	int             maxlen;
	int             time;
{
	int             len = 0;
	int             c;
	static int		disp = 0;
	static int		oldState = (-1);

	if (GuiWin)
	{
		if (oldState != (State & NORMAL))
		{
			oldState = (State & NORMAL);
			SendMessage(hVimWnd, WM_INITMENU, 0, 0);
		}
		if (c_ind < 0)
		{
			c_ind = 0;
			curbuf->b_p_tw = w_p_tw;
			curbuf->b_p_wm = w_p_wm;
			curbuf->b_p_ai = w_p_ai;
			curbuf->b_p_si = w_p_si;
			curbuf->b_p_et = w_p_et;
			p_sm = w_p_sm;
			p_ru = w_p_ru;
			p_ri = w_p_ri;
			p_uc = w_p_uc;
			p_paste = w_p_paste;
			disp = 0;
		}
		else if (c_ind > (c_next + 32))
		{
			if (config_overflow < KEY_REDRAW)
			{
				RedrawingDisabled = TRUE;
				if ((disp % 41) == 0)
					RedrawingDisabled = FALSE;
				disp++;
			}
		}
		flushbuf();
		MoveCursor(hVimWnd);
		if (time >= 0)
		{
			while (WaitForChar(time) == 0)		/* no character available */
			{
				if (!do_resize)		/* return if not interrupted by resize */
					return 0;
				set_winsize(0, 0, FALSE);
				do_resize = FALSE;
				cursor_refresh(hVimWnd);
			}
		}
		else	/* time == -1 */
		{
		/*
		 * If there is no character available within 2 seconds (default)
		 * write the autoscript file to disk
		 */
			if (WaitForChar((int)p_ut) == 0)
			{
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
				if ((State & NORMAL) && !v_macro && ef_share_replace(curbuf->b_filename))
				{
					if (keybuf_chk(5))
					{
						cbuf[c_end++] = ':';
						cbuf[c_end++] = 'e';
						cbuf[c_end++] = '!';
						cbuf[c_end++] = '\n';
					}
				}
#endif
				updatescript(0);
			}
		}

	/*
	 * Try to read as many characters as there are.
	 * Works for the controlling tty only.
	 */
		--maxlen;		/* may get two chars at once */
		/*
		 * we will get at least one key. Get more if they are available
		 * After a ctrl-break we have to read a 0 (!) from the buffer.
		 * bioskey(1) will return 0 if no key is available and when a
		 * ctrl-break was typed. When ctrl-break is hit, this does not always
		 * implies a key hit.
		 */
		for (;;)	/* repeat until we got a character */
		{
			if (do_resize)		/* window changed size */
			{
				set_winsize(0, 0, FALSE);
				do_resize = FALSE;
				cursor_refresh(hVimWnd);
			}
			WaitForChar(-1);
			if (do_resize)
				continue;
			c = 0;
			while (kbhit() && len < maxlen)
			{
				switch (c = getch()) {
				case 0:
					*buf++ = K_NUL;
					break;
				default:
					*buf++ = c;
					break;
				}
				len++;
				if (c_ind < 0)
				{
					RedrawingDisabled = FALSE;
					break;
				}
			}
			if (c == K_NUL && WaitForChar((int)p_tm))
			{
				*buf++ = getch();
				len++;
			}
			if (len > 0)
				return len;
		}
	}
	else if (IsTelnet)
	{
		DWORD		count;

		flushbuf();
		if (time >= 0)
		{
			while (time > 0)
			{
				if (PeekNamedPipe(hConIn, NULL, 0, NULL, NULL, &count) == 0)
					return 0;
				if (count == 0)
				{
					Sleep(20);
					time -= 20;
				}
				else
					break;
			}
			if (count == 0)
				return 0;
		}
		else
		{
			time = p_ut;
			while (time > 0)
			{
				if (PeekNamedPipe(hConIn, NULL, 0, NULL, NULL, &count) == 0)
					return 0;
				if (count == 0)
				{
					Sleep(20);
					time -= 20;
				}
				else
					break;
			}
			if (count == 0)
			{
				updatescript(0);
				count = maxlen;
			}
		}
		if (count > (DWORD)maxlen)
			count = maxlen;
		if (ReadFile(hConIn, buf, count, &count, NULL) != 0 && count)
			return count;
		return 0;
	}
	else
	{
		if (time >= 0) {
			if (time == 0)          /* don't know if time == 0 is allowed */
				time = 1;
			if (WaitForChar(time) == 0)     /* no character available */
				return 0;
		} else {                    /* time == -1 */
			/* If there is no character available within 2 seconds (default)
			 * write the autoscript file to disk */
			if (WaitForChar((int) p_ut) == 0)
				updatescript(0);
		}
		if (!v_nt) {
			DWORD	count = 0;
			for (;;) {
#ifdef FEPCTRL
				if (curbuf->b_p_fc && fep_get_mode() != 0) {
						/* IME enable mode... try special IME handling */
					if (ReadConsole(hConIn, buf, maxlen, &count, NULL) && count)
						return count;
				}
#endif
				if (WaitForSingleObject(hConIn, INFINITE) != WAIT_OBJECT_0) {
					return 0;			/* no KEY data */
				}
				ir.Event.KeyEvent.uChar.AsciiChar = 0;
				if (!PeekConsoleInput(hConIn, &ir, 1, &count) || count == 0) {
					continue;
				}
				if (ir.EventType != KEY_EVENT) {	/* MOUSE/WINDOWS EVENT */
					(void)ReadConsoleInput(hConIn, &ir, 1, &count);
					continue;
				}
				if (ir.Event.KeyEvent.bKeyDown
					&& (ir.Event.KeyEvent.dwControlKeyState
						& (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED|ENHANCED_KEY)) == 0
					&& ir.Event.KeyEvent.uChar.AsciiChar != 0) {
										/* but Bata Release : ZEN/HAN key is '@' */
					if (ReadConsole(hConIn, buf, maxlen, &count, NULL) && count) {
						return count;
					}
					continue;
				}
				if (!ReadConsoleInput(hConIn, &ir, 1, &count) || count == 0) {
					continue;
				}
				if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
					if ((buf[0] = isctlkey()) != 0) {
						return 1;
					} else if (iskeypad(ir.Event.KeyEvent.wVirtualScanCode)) {
						buf[0] = K_NUL;
						buf[1] = ir.Event.KeyEvent.wVirtualScanCode;
						return 2;
					} else if ((ir.Event.KeyEvent.wVirtualScanCode == '}')
								&& (ir.Event.KeyEvent.dwControlKeyState
									& (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))) {
						ReadConsoleInput(hConIn, &ir, 1, &count);
						buf[0] = Ctrl('\\');
						return 1;
					}
				}
			}
		}

	/*
	 * Try to read as many characters as there are.
	 * Works for the controlling tty only.
	 */
		--maxlen;                   /* may get two chars at once */
		/* we will get at least one key. Get more if they are available After a
		 * ctrl-break we have to read a 0 (!) from the buffer. bioskey(1) will
		 * return 0 if no key is available and when a ctrl-break was typed. When
		 * ctrl-break is hit, this does not always implies a key hit. */
		cbrk_pressed = FALSE;
		while ((len == 0 || kbhit()) && len < maxlen) {
			switch (c = tgetch()) {
			case 0:
				*buf++ = K_NUL;
				break;
			case 3:
				cbrk_pressed = TRUE;
				/* FALLTHROUGH */
			default:
				*buf++ = c;
			}
			len++;
		}
		return len;
	}
}

/*
 * We have no job control, fake it by starting a new shell.
 */
	void
mch_suspend()
{
	outstr("new shell started\n");
	call_shell(NULL, 0, TRUE);
}

#if 0	/* ken */
extern int      _fmode;
#endif
char            OrigTitle[256];

/*
 *
 */
int APIENTRY
WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HINSTANCE hInstance;
HINSTANCE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;
{
	char			**	argv;
	int					argc	= 1;
	int					num		= 4;
	char			*	p;
	int					c;
	BOOL				cygnus;
	static char			name[MAXPATHL];
	static char		*	dmy[2]	= {name, NULL};

	hInst = hInstance;
	SubSysCon = FALSE;
	GetModuleFileName(NULL, name, sizeof(name));
	if (lpCmdLine == NULL || (lpCmdLine != NULL && *lpCmdLine == '\0')
				|| (argv = malloc(num * sizeof(char *))) == NULL
				|| (p = malloc(strlen(lpCmdLine) + 1)) == NULL)
		argv = dmy;
	else
	{
		argv[0] = name;
		strcpy(p, lpCmdLine);
		while (*p != '\0')
		{
			if (argc >= num)
			{
				num += 4;
				if ((argv = realloc(argv, num * sizeof(char *))) == NULL)
					goto end;
			}
			if (*p == '"')
			{
				argv[argc++] = p + 1;
				while (*++p != '"')
					if (*p == '\0')
						goto end;
			}
			else if (getperm(p) != (-1))
			{
				argv[argc++] = p;
				break;
			}
			else
			{
				argv[argc++] = p;
				c = *p;
				cygnus = FALSE;
				if (p[0] == '/' && p[1] == '/' && isalpha(p[2]) && p[3] == '/')
					cygnus = TRUE;
				else if (strnicmp("/cygdrive/", p, 10) == 0
											&& isalpha(p[10]) && p[11] == '/')
					cygnus = TRUE;
				while (c != ' ' && c != '\t')
				{
#ifdef KANJI
					if (ISkanji(c))
						++p;
#endif
					c = *++p;
					if (cygnus && c == '\\')
					{
						memmove(&p[0], &p[1], strlen(p));
						if (*p == ' ' || *p == '\t')
							;
						else
							c = *p;
					}
					if (c == '\0')
						goto end;
				}
			}
			*p = '\0';
			c = *++p;
			while (c == ' ' || c == '\t')
				c = *++p;
		}
end:
		;
	}
	if (vimgetenv("VIM32DEBUG") == NULL)
	{
		__try {
			main(argc, argv);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION) {
			;
		}
	}
	else
		main(argc, argv);
	return(0);
}

/*
 *
 */
	void
mch_windinit(argc, argv, command)
	int				argc;
	char		  **argv;
	char		   *command;
{
	CONSOLE_SCREEN_BUFFER_INFO	csbi;

	_fmode = O_BINARY;          /* we do our own CR-LF translation */
	flushbuf();

	v_nt = FALSE;

	ver_info.dwOSVersionInfoSize = sizeof(ver_info);
	if (!GetVersionEx(&ver_info))
	{
		FatalAppExit(0, "Win32 API error.");
		ExitProcess(99);
	}
	if (ver_info.dwPlatformId == VER_PLATFORM_WIN32s)
	{
		FatalAppExit(0, "Win32s is not support");
		ExitProcess(99);
	}
	if (ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
		v_nt = TRUE;
	if (GuiWin)
	{
		HMENU		hMenu;
		HMENU		hTColor;
		HMENU		hBColor;
		HMENU		hEdit;
		HMENU		hSetup;
		HMENU		hGSetup;
		HMENU		hFile;
		HMENU		hConf;
		WNDCLASS	wndclass;
		UINT		w;
		HANDLE		hLibrary;

		if (SubSysCon)
			hInst = GetModuleHandle(NULL);
		v_region = Rows = nowRows;
		Columns = nowCols;
		if (SubSysCon)
			FreeConsole();

		w = SetErrorMode(SEM_NOOPENFILEERRORBOX);
		hLibrary = LoadLibrary("comctl32.dll");
		if (hLibrary != NULL)
		{
			pCreateUpDownControl
					= (tCreateUpDownControl)GetProcAddress(hLibrary, "CreateUpDownControl");
		}
		hLibrary = LoadLibrary("user32.dll");
		if (hLibrary != NULL)
		{
			pSetLayeredWindowAttributes
					= (tSetLayeredWindowAttributes)GetProcAddress(hLibrary, "SetLayeredWindowAttributes");
			pAllowSetForegroundWindow
					= (tAllowSetForegroundWindow)GetProcAddress(hLibrary, "AllowSetForegroundWindow");
			pLockSetForegroundWindow
					= (tLockSetForegroundWindow)GetProcAddress(hLibrary, "LockSetForegroundWindow");
		}
		SetErrorMode(w);

		v_cursor				= FALSE;
		v_font					= NULL;

		v_ssize = 256;
		if ((v_space = malloc(sizeof(INT) * v_ssize)) == NULL)
			ExitProcess(99);
		if ((v_char = malloc(sizeof(short) * v_ssize)) == NULL)
			ExitProcess(99);
		wndclass.style			= CS_DBLCLKS;
		wndclass.lpfnWndProc	= WndProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= sizeof(LONG);
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
		wndclass.hCursor		= NULL; /* LoadCursor(NULL, IDC_IBEAM); */
		wndclass.hbrBackground	= NULL;	/* GetStockObject(WHITE_BRUSH); */
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= szAppName;
		if (RegisterClass(&wndclass) == 0)
			ExitProcess(99);
		LoadConfig(TRUE);
#ifdef USE_BDF
		if (config_bdf)
			GetBDFfont(hInst, 0, config_bdffile, config_jbdffile, &v_bxchar, &v_bychar, &config_bdf);
		if (!config_ini && config_bdf && config_bitmap)
		{
			v_fgcolor = &config_fgbdf;
			v_bgcolor = &config_bgbdf;
		}
		else
		{
			v_fgcolor = &config_fgcolor;
			v_bgcolor = &config_bgcolor;
		}
#endif
		if (!config_ini && config_bitmap)
		{
			v_tbcolor = &config_tbbitmap;
			v_socolor = &config_sobitmap;
			v_ticolor = &config_tibitmap;
		}
		else
		{
			v_tbcolor = &config_tbcolor;
			v_socolor = &config_socolor;
			v_ticolor = &config_ticolor;
		}
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
		if (config_share || config_common)
			ef_share_init(config_common);
#endif
		v_menu = LoadMenu(hInst, "VIMMENU");
		if (pSetLayeredWindowAttributes == NULL)
			DeleteMenu(v_menu, IDM_FADEOUT, MF_BYCOMMAND);
		hVimWnd = CreateWindow(szAppName, szAppName,
				   WS_OVERLAPPEDWINDOW|(config_sbar ? WS_VSCROLL : 0),
				   config_x, config_y,
				   config_w, config_h, NULL, config_menu ? v_menu : NULL, hInst, NULL);
		if (NULL == hVimWnd)
			ExitProcess(99);
		hIbeamCurs	= LoadCursor(NULL, IDC_IBEAM);
		hArrowCurs	= LoadCursor(NULL, IDC_ARROW);
		hWaitCurs	= LoadCursor(NULL, IDC_WAIT);
		lpCurrCurs	= IDC_IBEAM;
		SetCursor(hIbeamCurs);
		hAcc = LoadAccelerators(hInst, "ACCKEYS");
		SetClassLong(hVimWnd, GCL_HICON, (LONG)LoadIcon(hInst, "vim"));
		hTColor= CreatePopupMenu();
		AppendMenu(hTColor,MF_STRING,    IDM_FWHITE,   "&White");
		AppendMenu(hTColor,MF_STRING,    IDM_FBLACK,   "&Black");
		AppendMenu(hTColor,MF_STRING,    IDM_FBLUE,    "&NavyBlue");
		AppendMenu(hTColor,MF_STRING,    IDM_FCOLOR,   "&Choice");
		hBColor= CreatePopupMenu();
		AppendMenu(hBColor,MF_STRING,    IDM_BWHITE,   "&White");
		AppendMenu(hBColor,MF_STRING,    IDM_BBLACK,   "&Black");
		AppendMenu(hBColor,MF_STRING,    IDM_BCOLOR,   "&Choice");
		hSetup= CreatePopupMenu();
		AppendMenu(hSetup, MF_STRING,    IDM_FONT,     "&Font");
#ifdef USE_BDF
		AppendMenu(hSetup, MF_STRING,    IDM_BDF,      "B&DF FONT");
#endif
		AppendMenu(hSetup, MF_STRING,    IDM_LSPACE,   "&Line Space");
		AppendMenu(hSetup, MF_POPUP,     (UINT)hTColor,"&Text Color");
		AppendMenu(hSetup, MF_POPUP,     (UINT)hBColor,"Back &Color");
		AppendMenu(hSetup, MF_STRING,    IDM_BITMAP,   "&Bitmap File");
		AppendMenu(hSetup, MF_STRING,    IDM_WAVE,     "&Wave File");
		AppendMenu(hSetup, MF_UNCHECKED, IDM_SAVE,     "&Save Window Position");
		hGSetup= CreatePopupMenu();
		AppendMenu(hGSetup,MF_CHECKED,   IDM_SBAR,     "&Scrollbar\tAlt+S");
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_MENU,     "&Menu\tAlt+M");
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_UNICODE,  "&Unicode Font");
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_TRAY,     "&Task Tray");
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_ONEWIN,   "One &Window");
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_MOUSE,    "&Erase Mouse");
#ifdef NT106KEY
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_NT106,    "&ZEN/HAN to ESC");
#endif
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
		{
			hCFile = CreatePopupMenu();
			AppendMenu(hCFile, MF_UNCHECKED, IDM_SHARE,    "&Share Files");
			AppendMenu(hCFile, MF_UNCHECKED, IDM_COMMON,   "&MS Compatible");
			AppendMenu(hGSetup,MF_POPUP,     (UINT)hCFile, "Share &Files");
		}
#endif
		if (pSetLayeredWindowAttributes)
			AppendMenu(hGSetup,MF_UNCHECKED, IDM_FADEOUT,  "Fade&out");
		AppendMenu(hGSetup,MF_UNCHECKED, IDM_GREPWIN,  "&Grep Window");
#ifdef USE_HISTORY
		{
			hHist = CreatePopupMenu();
			AppendMenu(hHist,  MF_UNCHECKED, IDM_HISTORY,  "&History");
			AppendMenu(hHist,  MF_UNCHECKED, IDM_HAUTO,    "&Auto History");
			AppendMenu(hGSetup,MF_POPUP,     (UINT)hHist,  "&History");
		}
#endif
		AppendMenu(hGSetup,MF_STRING,    IDM_PRINTSET, "&Print Command");
		hConf = CreatePopupMenu();
		AppendMenu(hConf,  MF_STRING,    IDM_CONFS,    "&Save Config");
		AppendMenu(hConf,  MF_UNCHECKED, IDM_CONF0,    "Default(&0)\tAlt+0");
		AppendMenu(hConf,  MF_UNCHECKED, IDM_CONF1,    "Config (&1)\tAlt+1");
		AppendMenu(hConf,  MF_UNCHECKED, IDM_CONF2,    "Config (&2)\tAlt+2");
		AppendMenu(hConf,  MF_UNCHECKED, IDM_CONF3,    "Config (&3)\tAlt+3");
		hEdit = CreatePopupMenu();
		AppendMenu(hEdit,  MF_STRING,    IDM_DELETE,   "&Delete\tAlt+X");
		AppendMenu(hEdit,  MF_STRING,    IDM_YANK,     "&Yank\tAlt+C");
		AppendMenu(hEdit,  MF_STRING,    IDM_PASTE,    "&Paste\tAlt+V");
		hFile = CreatePopupMenu();
		AppendMenu(hFile,  MF_STRING,    IDM_NEW,      "&New Window\tAlt+N");
		AppendMenu(hFile,  MF_STRING,    IDM_CLICK,    "&Run");
		AppendMenu(hFile,  MF_STRING,    IDM_FILE,     "&Open File\tAlt+O");
		AppendMenu(hFile,  MF_STRING,    IDM_SFILE,    "&Save File");
		AppendMenu(hFile,  MF_STRING,    IDM_AFILE,    "Save &As...");
		AppendMenu(hFile,  MF_STRING,    IDM_PRINT,    "&Print\tAlt+P");
		hMenu = GetSystemMenu(hVimWnd, FALSE);
		DeleteMenu(hMenu,  5, MF_BYPOSITION);
#if CUST_MENU
		DeleteMenu(hMenu, SC_SIZE,     MF_BYCOMMAND);
		DeleteMenu(hMenu, SC_MOVE,     MF_BYCOMMAND);
		DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
		DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
		DeleteMenu(hMenu, SC_CLOSE,    MF_BYCOMMAND);
		DeleteMenu(hMenu, SC_RESTORE,  MF_BYCOMMAND);
		hWin = CreatePopupMenu();
		AppendMenu(hWin,  MF_STRING, SC_RESTORE,  "Restore Window(&R)");
		AppendMenu(hWin,  MF_STRING, SC_MOVE,     "Move Window(&M)");
		AppendMenu(hWin,  MF_STRING, SC_SIZE,     "Change Window Size(&S)");
		AppendMenu(hWin,  MF_STRING, SC_MINIMIZE, "Min Window(&N)");
		AppendMenu(hWin,  MF_STRING, SC_MAXIMIZE, "Max Window(&X)");
		AppendMenu(hWin,  MF_STRING, SC_CLOSE,    "Close Window(&C)");
		AppendMenu(hMenu, MF_POPUP,  (UINT)hWin,  "&Window");
#else
		{
			char		buf[128];
			char	*	p;
			UINT		item[] = {SC_RESTORE,SC_MINIMIZE,SC_MAXIMIZE,SC_CLOSE};
			int			i;

			for (i = 0; i < sizeof(item) / sizeof(UINT); i++)
			{
				GetMenuString(hMenu, item[i],  buf, sizeof(buf), MF_BYCOMMAND);
				if ((p = strchr(buf, '\t')) != NULL)
					*p = NUL;
				ModifyMenu(hMenu, item[i], MF_BYCOMMAND|MF_STRING, item[i], buf);
			}
		}
#endif
		AppendMenu(hMenu,  MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu,  MF_POPUP,     (UINT)hGSetup,"G&lobal Setup");
		AppendMenu(hMenu,  MF_POPUP,     (UINT)hSetup, "Set&up");
		AppendMenu(hMenu,  MF_POPUP,     (UINT)hConf,  "Confi&g");
		AppendMenu(hMenu,  MF_POPUP,     (UINT)hFile,  "&File");
		AppendMenu(hMenu,  MF_POPUP,     (UINT)hEdit,  "&Edit");
		SetMenuItemBitmaps(hMenu, IDM_BITMAP, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_WAVE, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_SAVE, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_MENU, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_UNICODE, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_TRAY, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_ONEWIN, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_MOUSE, MF_BYCOMMAND, NULL, NULL);
#ifdef NT106KEY
		SetMenuItemBitmaps(hMenu, IDM_NT106, MF_BYCOMMAND, NULL, NULL);
#endif
#if defined(USE_EXFILE) && defined(USE_SHARE_CHECK)
		SetMenuItemBitmaps(hMenu, (UINT)hCFile, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_SHARE, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_COMMON,MF_BYCOMMAND, NULL, NULL);
#endif
		if (pSetLayeredWindowAttributes)
			SetMenuItemBitmaps(hMenu, IDM_FADEOUT, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_GREPWIN, MF_BYCOMMAND, NULL, NULL);
#ifdef USE_HISTORY
		SetMenuItemBitmaps(hMenu, (UINT)hHist, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_HISTORY, MF_BYCOMMAND, NULL, NULL);
		SetMenuItemBitmaps(hMenu, IDM_HAUTO,   MF_BYCOMMAND, NULL, NULL);
#endif
		if (GuiWin == '1')
		{
			HWND			hWnd;
			COPYDATASTRUCT	cds;
			char			buf[MAXPATHL];
			int				size = 8;
			int				j;
			char		*	p;

			if (argc > 1)
			{
				GetCurrentDirectory(MAXPATHL, buf);
				for (j = 0; j < (argc - 1); j++)
					size += strlen(argv[j]) + 3;
				size += strlen(buf) + 1
							+ (command == NULL ? 0 : strlen(command) + 1);
			}
			cds.dwData = 0;
			cds.cbData = size;
			if ((cds.lpData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cds.cbData)) != NULL)
			{
				if (argc > 1)
				{
					p = cds.lpData;
					strcpy(p, buf);
					size = strlen(buf) + 1;
					if (argc == 2)
					{
						strcpy(&p[size], ":e ");
						if (command
								&& (command[0] == '$' || ('0' <= command[0] && command[0] <= '9')))
						{
							strcat(&p[size], "+");
							strcat(&p[size], command);
							strcat(&p[size], " ");
						}
						strcat(&p[size], argv[0]);
					}
					else
					{
						strcpy(&p[size], ":args");
						for (j = 0; j < (argc - 1); j++)
						{
							strcat(&p[size], " \"");
							strcat(&p[size], argv[j]);
							strcat(&p[size], "\"");
						}
					}
					cds.dwData = size;
				}
				do_msg	= TRUE;
				hWnd = GetFirstSibling(hVimWnd);
				while (IsWindow(hWnd))
				{
					GetClassName(hWnd, buf, sizeof(buf));
					if (strcmp(buf, szAppName) == 0 && hVimWnd != hWnd)
					{
						if (SendMessage(hWnd, WM_COPYDATA,
									(WPARAM)hVimWnd, (LPARAM)&cds) == TRUE)
							ExitProcess(0);
					}
					hWnd = GetNextSibling(hWnd);
				}
				HeapFree(GetProcessHeap(), 0, cds.lpData);
				do_msg	= FALSE;
			}
		}
		ShowWindow(hVimWnd, SW_SHOWDEFAULT);
		UpdateWindow(hVimWnd);
		TopWindow(hVimWnd);
		GetWindowText(hVimWnd, OrigTitle, sizeof(OrigTitle));

#ifndef NO_WHEEL
		/* NT 4 and later supports WM_MOUSEWHEEL */
		/* Future Win32 versions ( >= 5.0 ) should support WM_MOUSEWHEEL */
		if ((ver_info.dwMajorVersion >= 5)
					|| (VER_PLATFORM_WIN32_NT == ver_info.dwPlatformId
											&& ver_info.dwMajorVersion >= 4))
		{
			if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
											&iScrollLines, SPIF_UPDATEINIFILE))
				iScrollLines = 3;
		}
		else
		{
			HwndMSWheel(&uiMsh_MsgMouseWheel, &uiMsh_Msg3DSupport,
						  &uiMsh_MsgScrollLines, &f3DSupport, &iScrollLines);
			if (iScrollLines == 0)
				iScrollLines = 3;
		}
#endif

		mch_set_winsize();
		do_resize = FALSE;

		SetLayerd();
	}
	else if (IsTelnet)
	{
		BenchTime = 0;
		hConIn = GetStdHandle(STD_INPUT_HANDLE);
		hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else
	{
		BenchTime = 0;
		/* Obtain handles for the standard Console I/O devices */
		hConIn = CreateFile("CONIN$",
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL, OPEN_EXISTING, 0, NULL);

		hConOut = CreateFile("CONOUT$",
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL, OPEN_EXISTING, 0, NULL);
#ifndef notdef
		if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
							GetCurrentProcess(), &h_mainthread,
						0, FALSE, DUPLICATE_SAME_ACCESS))
		{
			FatalAppExit(0, "initialize error\n");
			ExitProcess(99);
		}
#endif
		GetConsoleScreenBufferInfo(hConOut, &csbi);
		DefaultAttribute = csbi.wAttributes;
#ifndef notdef
		if (!v_nt)
			DefaultAttribute = csbi.wAttributes | FOREGROUND_INTENSITY;
#endif
		mch_get_winsize();
		GetConsoleTitle(OrigTitle, sizeof(OrigTitle));
	}
	if (vimgetenv("HOME") == NULL)
	{
		char	home[MAXPATHL+8];
		BOOL	bEnv = FALSE;

		if (v_nt && vimgetenv("HOMEDRIVE") != NULL
											&& vimgetenv("HOMEPATH") != NULL)
		{
			bEnv = TRUE;
			if (vimgetenv("HOMESHARE") != NULL)
				;
			else if (strcmp(vimgetenv("HOMEPATH"), "\\") == 0)
			{
				if (vimgetenv("SystemDrive") != NULL)
				{
					if (stricmp(vimgetenv("HOMEDRIVE"), vimgetenv("SystemDrive")) == 0)
						bEnv = FALSE;
				}
			}
		}
		strcpy(home, "HOME=");
		if (bEnv)
		{
			strcat(home, vimgetenv("HOMEDRIVE"));
			strcat(home, vimgetenv("HOMEPATH"));
		}
		else
		{
			char *	p;
			char *	last;

			GetModuleFileName(NULL, &home[5], MAXPATHL);
			last = p = home + 8;	/* drive + : + \ */
			while (*p)
			{
				if (*p == '\\')
					last = p;
				p++;
			}
			*last = '\0';
		}
		putenv(home);
	}
	if (vimgetenv("VIM") == NULL)
	{
		char	vim[MAXPATHL+8];
		char *	p;
		char *	last;

		strcpy(vim, "VIM=");
		GetModuleFileName(NULL, &vim[4], MAXPATHL);
		last = p = vim + 7;	/* drive + : + \ */
		while (*p)
		{
			if (*p == '\\')
				last = p;
			p++;
		}
		*last = '\0';
		putenv(vim);
	}
	if (vimgetenv("TMP") == NULL)
	{
		char	tmp[MAXPATHL+4];
		strcpy(tmp, "TMP=");
		if (vimgetenv("TEMP") != NULL)
			strcat(tmp, vimgetenv("TEMP"));
		else if (GetTempPath(MAXPATHL, &tmp[4]) != 0)
			;
		else
			GetCurrentDirectory(MAXPATHL, &tmp[4]);
		if (tmp[strlen(tmp) - 1] == '\\')
			tmp[strlen(tmp) - 1] = '\0';
		putenv(tmp);
	}
	if (BenchTime)
	{
		putenv("VIMINIT=");	putenv("EXINIT=");	putenv("HOME=_");
		putenv("TEMP=.");	putenv("TMP=.");
		BenchTime = GetTickCount();
	}
#ifdef USE_EXFILE
	else if (!NoEFS)
		ef_init(hVimWnd);
#endif
}

	void
check_win(argc, argv)
	int             argc;
	char          **argv;
{
	if (!isatty(0) || !isatty(1))
	{
#ifdef notdef		/* Windows NT telnetd support */
		fprintf(stderr, "VIM: no controlling terminal\n");
		exit(2);
#else
		IsTelnet = TRUE;
#endif
	}
	/* In some cases with DOS 6.0 on a NEC notebook there is a 12 seconds
	 * delay when starting up that can be avoided by the next two lines.
	 * Don't ask me why! This could be fixed by removing setver.sys from
	 * config.sys. Forget it. gotoxy(1,1); cputs(" "); */
}

/*
 * fname_case(): Set the case of the filename, if it already exists.
 *                 msdos filesystem is far to primitive for that. do nothing.
 */
	void
fname_case(name)
	char_u         *name;
{
#ifndef notdef
	WIN32_FIND_DATA fb;
	HANDLE          hFind;
	char_u		*	tname;
	char_u			buf[MAXPATHL];

	if (GetFullPathName(name, sizeof(buf), buf, &tname) == 0)
		return;
	if ((hFind = FindFirstFile(buf, &fb)) != INVALID_HANDLE_VALUE)
	{
		if (strlen(name) == strlen(buf))
		{
			strcpy(tname, fb.cFileName);
			strcpy(name, buf);
		}
		FindClose(hFind);
	}
#endif
}


/*
 * mch_settitle(): set titlebar of our window
 * Can the icon also be set?
 */
	void
mch_settitle(title, icon)
	char_u         *title;
	char_u         *icon;
{
	if (title != NULL && !p_icon)
	{
		if (GuiWin)
		{
			if (icon != NULL && strlen(title) > (TITLE_LEN + 6))
				SetWindowText(hVimWnd, icon);
			else
				SetWindowText(hVimWnd, DisplayPathName(title, (TITLE_LEN + 6) > sizeof(nIcon.szTip) ? sizeof(nIcon.szTip) : TITLE_LEN + 6));
		}
		else if (IsTelnet)
			;
		else
		{
			if (icon != NULL && strlen(title) > sizeof(nIcon.szTip))
				SetConsoleTitle(icon);
			else
				SetConsoleTitle(title);
		}
	}
	else if (icon != NULL)
	{
		if (GuiWin)
			SetWindowText(hVimWnd, icon);
		else if (IsTelnet)
			;
		else
			SetConsoleTitle(icon);
	}
}

/*
 * Restore the window/icon title.
 * which is one of:
 *    1  Just restore title
 *  2  Just restore icon (which we don't have)
 *    3  Restore title and icon (which we don't have)
 */
	void
mch_restore_title(which)
	int which;
{
	mch_settitle((which & 1) ? OrigTitle : NULL, NULL);
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return non-zero for success.
 */
	int
vim_dirname(buf, len)
	char_u         *buf;
	int             len;
{
#ifdef __BORLANDC__
	return (getcwd(buf, len) != NULL ? OK : FAIL);
#else
	return (_getcwd(buf, len) != NULL ? OK : FAIL);
#endif
}

/*
 * get absolute filename into buffer 'buf' of length 'len' bytes
 */
	int
FullName(fname, buf, len)
	char_u         *fname,
				   *buf;
	int             len;
{
	if (fname == NULL)          /* always fail */
		return FAIL;

	if (_fullpath(buf, fname, len) == NULL) {
		strncpy(buf, fname, len);       /* failed, use the relative path name */
		return FAIL;
	}
	return OK;
}

/*
 * return TRUE is fname is an absolute path name
 */
	int
isFullName(fname)
	char_u        *fname;
{
#ifdef notdef
	return (STRCHR(fname, ':') != NULL);
#else
	if (strlen(fname) > 3 && isalpha(fname[0]) && fname[1] == ':' && fname[2] == '\\')
		return(TRUE);
	if (strlen(fname) >= 2 && fname[0] == '\\' && fname[1] == '\\')
		return(TRUE);
	return(FALSE);
#endif
}

/*
 * get file permissions for 'name'
 * -1 : error
 * else FA_attributes defined in dos.h
 */
	long
getperm(name)
	char_u         *name;
{
	struct stat statb;
	long        r;

#ifdef USE_EXFILE
	if (ef_stat(name, &statb))
#else
	if (stat(name, &statb))
#endif
		return -1;
	r = statb.st_mode & 0x7fffffff;
	return r;
}

/*
 * set file permission for 'name' to 'perm'
 */
	int
setperm(name, perm)
	char_u         *name;
	long            perm;
{
	return chmod(name, perm);
}

/*
 * check if "name" is a directory
 */
int             isdir(name)
	char_u         *name;
{
	int f;

	f = getperm(name);
	if (f == -1)
		return -1;                    /* file does not exist at all */
	if ((f & S_IFDIR) == 0)
		return FAIL;                /* not a directory */
	return OK;
}

/*
 * Careful: mch_windexit() may be called before mch_windinit()!
 */
	void
mch_windexit(r)
	int             r;
{
	if (GuiWin && NameBuff != NULL)
	{
		if (GuiConfig == 0)
			SaveConfig();
	}
	else
		GuiWin = FALSE;
	if (GuiWin && !BenchTime)
	{
		if (config_fadeout && (bWClose || v_trans) && pSetLayeredWindowAttributes != NULL)
		{
			BYTE			gbAlpha = 0xff;
			DWORD			dwTime  = 30;

			gbAlpha = (BYTE)(((230 * (100 - v_trans)) / 100) + 25);
			if (gbAlpha > 180)
				gbAlpha = 180;
			else if (gbAlpha > 50)
				gbAlpha -= 20;
			SetWindowLong(hVimWnd, GWL_EXSTYLE,
						GetWindowLong(hVimWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			pSetLayeredWindowAttributes(hVimWnd, 0, gbAlpha, LWA_ALPHA);
			UpdateWindow(hVimWnd);
			while (gbAlpha > 15)
			{
				pSetLayeredWindowAttributes(hVimWnd, 0, gbAlpha, LWA_ALPHA);
				gbAlpha -= 12;
				if (gbAlpha > 200)
					dwTime = 10;
				else if (gbAlpha > 100)
					dwTime = 15;
				else if (gbAlpha > 10)
					dwTime = 20;
				Sleep(dwTime);
			}
		}
		else
			delay(100);
	}
#ifdef FEPCTRL
	if (FepInit)
		fep_term();
#endif
	settmode(0);
	stoptermcap();
	flushbuf();
	ml_close_all();                 /* remove all memfiles */
	mch_restore_title(3);
	if (scriptout)
		fclose(scriptout);
	if (GuiWin)
	{
		MSG				msg;

		DestroyWindow(hVimWnd);
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(hVimWnd, hAcc, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
#ifdef USE_EXFILE
	ef_term();
#endif
	if (BenchTime && !ctrlc_pressed)
	{
		DWORD		tm = GetTickCount() - BenchTime;
		char		buf[256];

		sprintf(buf, "%dm %ds %d",
				tm / (60 * 1000), (tm % (60 * 1000)) / 1000, tm % 1000);
		MessageBox(NULL, buf, "Bench Mark Time", MB_OK);
	}
	ExitProcess(r);
}

/*
 * function for ctrl-break interrupt
 */
#ifndef notdef
	static void
v_hangup(void *arg)
{
	TerminateThread(h_mainthread, 0);    /* forced terminate main processing */
	docmdline(":qall!");
}
#endif

	BOOL WINAPI
handler_routine(DWORD dwCtrlType)
{
#ifdef notdef
	cbrk_pressed = TRUE;
	ctrlc_pressed = TRUE;
#else
# ifdef __BORLANDC__
	DWORD		IdThread;
	HANDLE		hang_thread;
# else
	ULONG		hang_thread;
# endif
	static int firsttime = TRUE;

	switch (dwCtrlType) {
	case CTRL_BREAK_EVENT:
	case CTRL_C_EVENT:
		cbrk_pressed = TRUE;
		ctrlc_pressed = TRUE;
		return(TRUE);
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		if (!firsttime) {
			return (TRUE);                /* show default dialog box */
		}
		firsttime = FALSE;
		SuspendThread(h_mainthread);		/* suspend main processing */
# ifdef __BORLANDC__
#  if 1
		TerminateThread(h_mainthread, 0);	/*forced terminate main processing*/
		docmdline(":qall!");
#  else
		hang_thread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE)v_hangup, NULL, 0, &IdThread);
		if (hang_thread != NULL) {
			WaitForSingleObject(hang_thread, 10000);
										/* wait for finish thread */
		}
#  endif
# else    /* MICROSOFT */
		hang_thread = _beginthread(v_hangup, 0x2000, NULL);
						/* process exception by multi-thread C Library manner */
		if (hang_thread != (ULONG)-1) {
			WaitForSingleObject((HANDLE)hang_thread, 10000);
										/* wait for finish thread */
		}
# endif
		return(TRUE);
	}
	return(FALSE);
#endif
}

/*
 * set the tty in (raw) ? "raw" : "cooked" mode
 *
 */

	void
mch_settmode(raw)
	int             raw;
{
	DWORD           cmodein;
	DWORD           cmodeout;

	if (GuiWin || IsTelnet)
		return;
	if (term_console)
		scroll_region = FALSE;
	GetConsoleMode(hConIn, &cmodein);
	GetConsoleMode(hConOut, &cmodeout);

	if (raw) {
		if (term_console)
			outstr(T_TP);       /* set colors */

		cmodein &= ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
#ifdef KANJI
					 ENABLE_MOUSE_INPUT |
#endif
					 ENABLE_ECHO_INPUT);
#ifdef notdef
		cmodein |= ENABLE_WINDOW_INPUT;
#endif

		SetConsoleMode(hConIn, cmodein);

#ifndef KANJI
		cmodeout &= ~(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
#endif

		SetConsoleMode(hConOut, cmodeout);
		SetConsoleCtrlHandler(handler_routine, TRUE);
	} else {

		if (term_console)
			normvideo();        /* restore screen colors */

		cmodein |= (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
#ifdef KANJI
					ENABLE_MOUSE_INPUT |
#endif
					ENABLE_ECHO_INPUT);
		cmodein &= ~(ENABLE_WINDOW_INPUT);

		SetConsoleMode(hConIn, cmodein);

#ifndef KANJI
		cmodeout |= (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
#endif

		SetConsoleMode(hConOut, cmodeout);

		SetConsoleCtrlHandler(handler_routine, FALSE);
	}
}

	int
mch_get_winsize()
{
	if (GuiWin)
	{
		Columns = nowCols;
		v_region = Rows = nowRows;
		mch_set_winsize();
#ifdef FEPCTRL
		if (FepInit)
		{
			LOGFONT			logfont;

			memcpy(&logfont, &config_jfont, sizeof(logfont));
			logfont.lfHeight		= -v_ychar;
			logfont.lfWidth			= v_xchar;
			logfont.lfItalic		= 0;
			logfont.lfUnderline		= 0;
			logfont.lfWeight		= FW_NORMAL;
			fep_win_font(hVimWnd, &logfont);
		}
#endif
	}
	else if (IsTelnet)
	{
		extern void getlinecol();

		getlinecol();	/* get "co" and "li" entries from termcap */
		if (Columns <= 0 || Rows <= 0)
		{
			Columns = 80;
			maxRows = Rows = 25;
			return OK;
		}
	}
	else
	{
		/*
		 * Use the console mode API
		 */
		if (GetConsoleScreenBufferInfo(hConOut, &csbi)) {
			maxRows = Rows = csbi.dwSize.Y;
			maxRows = csbi.dwSize.Y;
			Rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			Columns = csbi.dwSize.X;
			DefaultAttribute = csbi.wAttributes;
#ifndef notdef
			if (!v_nt)
				DefaultAttribute = csbi.wAttributes | FOREGROUND_INTENSITY;
#endif
#ifdef KANJI
			if (term_console) {
				if (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
						&& ver_info.dwMajorVersion == 3
						&& ver_info.dwMinorVersion == 50)) {
					Rows--;
				} else {
#  ifdef FEPCTRL
					if (FepInit)
					{
						if (fep_init())
							Rows--;
					}
#  endif
				}
			}
#endif
		} else {
			maxRows = Rows = 25;
			Columns = 80;
		}

		if (Columns < 5 || Columns > MAX_COLUMNS ||
			Rows < 2 || Rows > MAX_COLUMNS) {
			/* these values are overwritten by termcap size or default */
			Columns = 80;
			maxRows = Rows = 25;
			return OK;
		}
	}
	check_winsize();
	/*script_winsize();*/

	return OK;
}

/*********************************************************************
* FUNCTION: perr(PCHAR szFileName, int line, PCHAR szApiName,        *
*                DWORD dwError)                                      *
*                                                                    *
* PURPOSE: report API errors. Allocate a new console buffer, display *
*          error number and error text, restore previous console     *
*          buffer                                                    *
*                                                                    *
* INPUT: current source file name, current line number, name of the  *
*        API that failed, and the error number                       *
*                                                                    *
* RETURNS: none                                                      *
*********************************************************************/

/* maximum size of the buffer to be returned from FormatMessage */
#define MAX_MSG_BUF_SIZE 512

	void
perr(PCHAR szFileName, int line, PCHAR szApiName, DWORD dwError)
{
	CHAR            szTemp[1024];
	DWORD           cMsgLen;
	CHAR           *msgBuf;     /* buffer for message text from system */
	int             iButtonPressed;     /* receives button pressed in the
										 * error box */

	/* format our error message */
	sprintf(szTemp, "%s: Error %d from %s on line %d:\n", szFileName,
			dwError, szApiName, line);
	/* get the text description for that error number from the system */
	cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
						 FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL, dwError,
	 MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR) & msgBuf, MAX_MSG_BUF_SIZE,
							NULL);
	if (!cMsgLen)
		sprintf(szTemp + strlen(szTemp), "Unable to obtain error message text! \n"
				"%s: Error %d from %s on line %d", __FILE__,
				GetLastError(), "FormatMessage", __LINE__);
	else
		strcat(szTemp, msgBuf);
	strcat(szTemp, "\n\nContinue execution?");
	MessageBeep(MB_ICONEXCLAMATION);
	iButtonPressed = MessageBox(NULL, szTemp, "Console API Error",
						  MB_ICONEXCLAMATION | MB_YESNO | MB_SETFOREGROUND);
	/* free the message buffer returned to us by the system */
	if (cMsgLen)
		LocalFree((HLOCAL) msgBuf);
	if (iButtonPressed == IDNO)
		exit(1);
	return;
}
#define PERR(bSuccess, api) {if (!(bSuccess)) perr(__FILE__, __LINE__, \
	api, GetLastError());}


	static void
resizeConBufAndWindow(HANDLE hConsole, long xSize, long ySize)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;    /* hold current console buffer info */
	BOOL            bSuccess;
	SMALL_RECT      srWindowRect;       /* hold the new console size */
	COORD           coordScreen;

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");
	/* get the largest size we can size the console window to */
	coordScreen = GetLargestConsoleWindowSize(hConsole);
	PERR(coordScreen.X | coordScreen.Y, "GetLargestConsoleWindowSize");
	/* define the new console window size and scroll position */
	srWindowRect.Right = (SHORT) (min(xSize, coordScreen.X) - 1);
	srWindowRect.Bottom = (SHORT) (min(ySize, coordScreen.Y) - 1);
	srWindowRect.Left = srWindowRect.Top = (SHORT) 0;
	/* define the new console buffer size */
	coordScreen.X = xSize;
	coordScreen.Y = ySize;
	/* if the current buffer is larger than what we want, resize the */
	/* console window first, then the buffer */
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y > (DWORD) xSize * ySize) {
		bSuccess = SetConsoleWindowInfo(hConsole, TRUE, &srWindowRect);
		PERR(bSuccess, "SetConsoleWindowInfo");
		bSuccess = SetConsoleScreenBufferSize(hConsole, coordScreen);
		PERR(bSuccess, "SetConsoleScreenBufferSize");
	}
	/* if the current buffer is smaller than what we want, resize the */
	/* buffer first, then the console window */
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y < (DWORD) xSize * ySize) {
		bSuccess = SetConsoleScreenBufferSize(hConsole, coordScreen);
		PERR(bSuccess, "SetConsoleScreenBufferSize");
		bSuccess = SetConsoleWindowInfo(hConsole, TRUE, &srWindowRect);
		PERR(bSuccess, "SetConsoleWindowInfo");
	}
	/* if the current buffer *is* the size we want, don't do anything! */
	return;
}

	void
mch_set_winsize()
{
	if (GuiWin)
	{
		RECT		rcClient;
		RECT		rcWindow;

		GetClientRect(hVimWnd, &rcClient);
		if (GetWindowRect(hVimWnd, &rcWindow))
		{
			config_w = ((rcWindow.right - rcWindow.left)
							- (rcClient.right - rcClient.left))
							+ v_xchar * Columns;
			config_h = ((rcWindow.bottom - rcWindow.top)
							- (rcClient.bottom - rcClient.top))
							+ v_ychar * Rows;
			MoveWindow(hVimWnd, rcWindow.left, rcWindow.top, config_w, config_h, TRUE);
		}
	}
	else if (IsTelnet)
		;
	else
	{
		resizeConBufAndWindow(hConOut, Columns, Rows);
		mch_get_winsize();
	}
}

	int
call_shell(cmd, filter, cooked)
	char_u         *cmd;
	int             filter;     /* if != 0: called by dofilter() */
	int             cooked;
{
	int             x = 0;
	char            newcmd[MAXPATHL];

	flushbuf();

#ifdef FEPCTRL
	if (FepInit && !GuiWin)
		fep_term();
#endif

	if (cooked)
		settmode(0);            /* set to cooked mode */

	if (GuiWin)
	{
		STARTUPINFO				si;
		PROCESS_INFORMATION		pi;

		memset(&pi, 0, sizeof(pi));
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		if (cmd == NULL)
		{
			if (CreateProcess(p_sh, NULL, NULL, NULL, FALSE,
					CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) == TRUE)
			{
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			else
				x = GetLastError();
		}
		else if (filter || DoMake || BenchTime)
		{
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = /*SW_HIDE*/SW_MINIMIZE;
			sprintf(newcmd, "%s /c %s", p_sh, cmd);
			if (CreateProcess(NULL, newcmd, NULL, NULL, TRUE,
					CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) == TRUE)
			{
				for (;;)
				{
					if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0)
					{
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
						break;
					}
					breakcheck();
				}
				TopWindow(hVimWnd);
			}
			else
				x = GetLastError();
		}
		else
		{
			static	char	*	ext[]	= {".com", ".exe", ".bat", NULL};
			char			**	ep		= ext;
			char				exe[MAXPATHL];
			char				arg[MAXPATHL];
			char			*	p		= exe;
			char			*	last	= NULL;
			BOOL				bShell	= FALSE;

			exe[0] = arg[0] = NUL;
			strcpy(exe, cmd);
			if (getperm(p) == (-1))
			{
				while (*p)
				{
					if (*p == ' ' || *p == '\t')
					{
						*p = NUL;
						p++;
						strcpy(arg, p);
						break;
					}
					p++;
				}
			}
			p = exe;
			while (*p)
			{
				if (*p == '.')
					last = p;
				p++;
			}
			if (last)
			{
				bShell = TRUE;
				while (*ep)
				{
					if (strnicmp(last, *ep, strlen(*ep)) == 0)
					{
						bShell = FALSE;
						break;
					}
					ep++;
				}
			}
			if (last == NULL || bShell == FALSE)
			{
				char		exebuf[MAXPATHL];
				SHFILEINFO	shinfo;
				DWORD		dwRtn;

				if ((int)FindExecutable(exe, ".", exebuf) <= 32)
					;
				else
				{
					dwRtn = SHGetFileInfo(exebuf, SHGFI_USEFILEATTRIBUTES,
										&shinfo, sizeof(shinfo), SHGFI_EXETYPE);
					if ((LOWORD(dwRtn) == 0x4550/*PE*/ && HIWORD(dwRtn) == 0)
							|| (LOWORD(dwRtn) == 0x5a4d/*MZ*/ && HIWORD(dwRtn) == 0))
						;
					else
						bShell = TRUE;
				}
			}
			if (bShell)
			{
				x = (int)ShellExecute(NULL, NULL, exe, arg, ".", SW_SHOW);
				if (x > 32)
					x = 0;
			}
			else
			{
				if (v_nt)
					sprintf(newcmd, "%s /c %s && pause", p_sh, cmd);
				else
				{
					FILE	*	fp;
					int			i;
					char		batbuf[MAXPATHL];

					for (i = 0; i < 1000; i++)
					{
						batbuf[0] = '\0';
						if (p_dir != NULL && *p_dir != NUL)
						{
							if (*p_dir == '>')	/* skip '>' in front of dir */
								STRCPY(batbuf, p_dir + 1);
							else
								STRCPY(batbuf, p_dir);
							if (!ispathsep(*(batbuf + STRLEN(batbuf) - 1)))
								STRCAT(batbuf, PATHSEPSTR);
						}
						sprintf(&batbuf[STRLEN(batbuf)], "vim%05d.bat", i);
						if (getperm(batbuf) < 0)
						{
							if ((fp = fopen(batbuf, "w")) != NULL)
							{
								fprintf(fp, "%s\r\n", cmd);
								fprintf(fp, "pause\r\n");
								fprintf(fp, "del \"%s\"\r\n", batbuf);
								fclose(fp);
								sprintf(newcmd, "%s /c %s", p_sh, batbuf);
								break;		/* for loop */
							}
						}
					}
				}
#if 0
				if (pLockSetForegroundWindow)
					pLockSetForegroundWindow(LSFW_UNLOCK);
#endif
				if (CreateProcess(NULL, newcmd, NULL, NULL, FALSE,
						CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) == TRUE)
				{
#if 0
					if (pAllowSetForegroundWindow)
						pAllowSetForegroundWindow(pi.dwProcessId);
#endif
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
				else
					x = GetLastError();
#if 0
				if (pLockSetForegroundWindow)
					pLockSetForegroundWindow(LSFW_LOCK);
#endif
			}
		}
		if (!filter && x == 0 && c_end == 0)
			cbuf[c_end++] = '\n';
	}
	else
	{
		if (cmd == NULL)
			x = system(p_sh);
		else
		{
			sprintf(newcmd, "%s /c %s", p_sh, cmd);
			x = system(newcmd);
		}
		outchar('\n');
	}
	if (cooked)
		settmode(1);            /* set to raw mode */

#ifdef WEBB_COMPLETE
	if (x && !expand_interactively)
#else
	if (x)
#endif
	{
		smsg("%d returned", x);
		outchar('\n');
	}

#ifdef FEPCTRL
	if (FepInit && !GuiWin)
		fep_init();
#endif

	resettitle();
	return x;
}

#define FL_CHUNK 32

	static void
addfile(fl, f, isdir)
	FileList       *fl;
	char           *f;
	int             isdir;
{
	char           *p;

	if (!fl->file) {
		fl->file = (char_u **) alloc(sizeof(char *) * FL_CHUNK);
		if (!fl->file)
			return;
		fl->nfiles = 0;
		fl->maxfiles = FL_CHUNK;
	}
	if (fl->nfiles >= fl->maxfiles) {
		char_u        **t;
		int             i;

		t = (char_u **)lalloc(sizeof(char *) * (fl->maxfiles + FL_CHUNK), TRUE);
		if (!t)
			return;
		for (i = fl->nfiles - 1; i >= 0; i--)
			t[i] = fl->file[i];
		free(fl->file);
		fl->file = t;
		fl->maxfiles += FL_CHUNK;
	}
	p = alloc((unsigned) (strlen(f) + 1 + isdir));
	if (p) {
		strcpy(p, f);
		if (isdir)
			strcat(p, "/");
	}
	fl->file[fl->nfiles++] = p;
}

#ifdef __BORLANDC__
	static int
pstrcmp(a, b)
	char_u        **a,
				  **b;
{
	return (strcmp(*a, *b));
}
#else
	static int
pstrcmp(const void *s1, const void *s2)
{
	unsigned char * p1 = *(unsigned char **)s1;
	unsigned char * p2 = *(unsigned char **)s2;

#ifdef notdef
	return (strcmp(p1, p2));
#else
	return (stricmp(p1, p2));
#endif
}
#endif

	int
has_wildcard(s)
	char_u         *s;
{
	int			pos = 1;

	if (do_drag)
		return 0;
	if (s)
#ifndef notdef
		if (*s == '~' || *s == '$')
			return 1;
		else
#endif
		for (; *s; ++s, ++pos)
		{
#ifdef XARGS
			if (*s == '[' || *s == '{')
				return pos;
			else
#endif
			if (*s == '?' || *s == '*')
				return pos;
#ifdef KANJI
			if (ISkanji(*s))
			{
				++s;
				++pos;
			}
#endif
		}
	return 0;
}

static int
has_wildcards(s)
	char_u         *s;
{
	int			pos = 1;
	if (s)
		for (; *s; ++s, ++pos)
		{
			if (*s == '?' || *s == '*')
				return pos;
#ifdef KANJI
			if (ISkanji(*s))
			{
				++s;
				++pos;
			}
#endif
		}
	return 0;
}

	static void
strlowcpy(d, s)
	char           *d,
				   *s;
{
	while (*s)
		*d++ = tolower(*s++);
	*d = '\0';
}

	static int
expandpath(fl, path, fonly, donly, notf)
	FileList       *fl;
	char           *path;
	int             fonly,
					donly,
					notf;
{
	char            buf[MAX_PATH];
	char           *p,
				   *s,
				   *e;
	int             lastn,
					c = 1,
					r;
	WIN32_FIND_DATA fb;
	HANDLE          hFind;

	lastn = fl->nfiles;

/*
 * Find the first part in the path name that contains a wildcard.
 * Copy it into buf, including the preceding characters.
 */
	p = buf;
	s = NULL;
	e = NULL;
#ifndef notdef
	memset(buf, NUL, sizeof(buf));
#endif
	while (*path) {
		if (*path == '\\' || *path == ':' || *path == '/') {
			if (e)
				break;
			else
				s = p;
		}
		if (*path == '*' || *path == '?')
			e = p;
#ifdef KANJI
		if (ISkanji(*path))
			*p++ = *path++;
#endif
		*p++ = *path++;
	}
	e = p;
	if (s)
		s++;
	else
		s = buf;

	/* now we have one wildcard component between s and e */
	*e = '\0';
	r = 0;
	/* If we are expanding wildcards we try both files and directories */
	if ((hFind = FindFirstFile(buf, &fb)) == INVALID_HANDLE_VALUE) {
		/* not found */
#ifndef notdef
		if (has_wildcard(buf))
			return 0;
#endif
		strcpy(e, path);
		if (notf)
			addfile(fl, buf, FALSE);
		return 1;               /* unexpanded or empty */
	}
	while (c) {
#ifdef notdef
		strlowcpy(s, fb.cFileName);
#else
		strcpy(s, fb.cFileName);
#endif
		if (*s != '.' || (s[1] != '\0' && (s[1] != '.' || s[2] != '\0'))) {
			strcat(buf, path);
			if (!has_wildcard(path))
				addfile(fl, buf, (isdir(buf) > 0));
			else
				r |= expandpath(fl, buf, fonly, donly, notf);
		}
		c = FindNextFile(hFind, &fb);
	}
	qsort(fl->file + lastn, fl->nfiles - lastn, sizeof(char *), pstrcmp);
	FindClose(hFind);
	return r;
}

/*
 * MSDOS rebuilt of Scott Ballantynes ExpandWildCard for amiga/arp.
 * jw
 */

	int
ExpandWildCards(num_pat, pat, num_file, file, files_only, list_notfound)
	int             num_pat;
	char_u        **pat;
	int            *num_file;
	char_u       ***file;
	int             files_only,
					list_notfound;
{
	int             i,
					r = 0;
	FileList        f;

	f.file = NULL;
	f.nfiles = 0;
	for (i = 0; i < num_pat; i++)
	{
#if !defined(notdef) && defined(XARGS)
		char_u		buf[MAXPATHL+4];
		int			j;
		char	**	result;

		memset(buf, 0, sizeof(buf));
		expand_env(pat[i], buf, MAXPATHL);
		if (strcspn(buf, " \t{") < strlen(buf))
		{
			if (do_drag)
				j = strlen(buf) + 1;
			else if ((j = has_wildcards(buf)) == 0)
				j = strlen(buf) + 1;
			memmove(&buf[1], buf, strlen(buf) + 1);
			memmove(&buf[j+1], &buf[j], strlen(buf) + 1 - j);
			buf[0] = '\"';
			buf[j] = '\"';
		}
# ifdef USE_EXFILE
		if (is_efarc(buf, '\0'))
		{
			strcat(buf, ":*");
			result = ef_globfilename(buf, FALSE);
		}
		else if (buf[strlen(buf) - 1] == '\"')
		{
			buf[strlen(buf) - 1] = '\0';
			if (is_efarc(&buf[1], '\0'))
			{
				strcat(buf, "\":*");
				result = ef_globfilename(buf, FALSE);
			}
			else
			{
				strcat(buf, "\"");
				result = ef_globfilename(buf, TRUE);
			}
		}
		else
			result = ef_globfilename(buf, TRUE);
# else
		result = glob_filename(buf);
# endif
		for (j = 0; result[j] != NULL; j++)
		{
			if (!has_wildcard(result[j]))
				addfile(&f, result[j], files_only ? FALSE : (isdir(result[j]) == TRUE));
			else
				r |= expandpath(&f, result[j], files_only, 0, list_notfound);
			free(result[j]);
		}
		free(result);
#else
		if (!has_wildcard(pat[i]))
			addfile(&f, pat[i], files_only ? FALSE : (isdir(pat[i]) > 0));
		else
			r |= expandpath(&f, pat[i], files_only, 0, list_notfound);
#endif
	}
	if (r == 0)
	{
		*num_file = f.nfiles;
		*file = f.file;
	}
	else
	{
		*num_file = 0;
		*file = NULL;
	}
	return (r ? FAIL : OK);
}

	void
FreeWild(num, file)
	int             num;
	char_u        **file;
{
	if (file == NULL || num <= 0)
		return;
	while (num--)
		free(file[num]);
	free(file);
}

/*
 * The normal chdir() does not change the default drive.
 * This one does.
 */
#undef chdir
int             vim_chdir(path)
	char_u         *path;
{
	if (path[0] == NUL)         /* just checking... */
		return FAIL;
	if (path[1] == ':') {       /* has a drive name */
		if (_chdrive(toupper(path[0]) - 'A' + 1))
			return -1;          /* invalid drive name */
		path += 2;
	}
	if (*path == NUL)           /* drive name only */
		return OK;
#ifdef __BORLANDC__
	return chdir(path);         /* let the normal chdir() do the rest */
#else
	return _chdir(path);        /* let the normal chdir() do the rest */
#endif
}

	static void
clrscr()
{
	DWORD           count;

	ntcoord.X = 0;
	ntcoord.Y = 0;
	FillConsoleOutputCharacter(hConOut, ' ', Columns * maxRows,
							   ntcoord, &count);
	FillConsoleOutputAttribute(hConOut, DefaultAttribute, maxRows * Columns,
							   ntcoord, &count);
}

	static void
clreol()
{
	DWORD           count;
	FillConsoleOutputCharacter(hConOut, ' ',
								Columns - ntcoord.X,
								ntcoord, &count);
	FillConsoleOutputAttribute(hConOut, DefaultAttribute,
								Columns - ntcoord.X,
								ntcoord, &count);
}

	static void
insline(int count)
{
	SMALL_RECT      source;
	COORD           dest;
	CHAR_INFO       fill;

	dest.X = 0;
	dest.Y = ntcoord.Y + count;

	source.Left = 0;
	source.Top = ntcoord.Y;
	source.Right = Columns;
	source.Bottom = Rows - 1;

	fill.Char.AsciiChar = ' ';
	fill.Attributes = DefaultAttribute;

	ScrollConsoleScreenBuffer(hConOut, &source, (PSMALL_RECT) 0, dest, &fill);
#ifndef notdef
	if (maxRows != Rows)
	{
		DWORD           w;
		dest.X = 0;
		dest.Y = maxRows - 1;
		FillConsoleOutputCharacter(hConOut, ' ', Columns, dest, &w);
		FillConsoleOutputAttribute(hConOut, DefaultAttribute, Columns, dest, &w);
	}
#endif
}

	static void
delline(int count)
{
	SMALL_RECT      source;
	COORD           dest;
	CHAR_INFO       fill;

	dest.X = 0;
	dest.Y = ntcoord.Y;

	source.Left = 0;
	source.Top = ntcoord.Y + count;
	source.Right = Columns;
	source.Bottom = maxRows - 1;

	/* get current attributes and fill out CHAR_INFO structure for fill char */
	fill.Char.AsciiChar = ' ';
	fill.Attributes = DefaultAttribute;

	ScrollConsoleScreenBuffer(hConOut, &source, (PSMALL_RECT) 0, dest, &fill);
#ifndef notdef
	if (count > 2)
	{
		DWORD           w;
		dest.X = 0;
		dest.Y = maxRows - 1 - count;
		FillConsoleOutputCharacter(hConOut, ' ', Columns * count, dest, &w);
		FillConsoleOutputAttribute(hConOut, DefaultAttribute,
												Columns * count, dest, &w);
	}
#endif
}


	static void
scroll()
{
	SMALL_RECT      source;
	COORD           dest;
	CHAR_INFO       fill;

	dest.X = 0;
	dest.Y = 0;

	source.Left = 0;
	source.Top = 1;
	source.Right = Columns;
	source.Bottom = Rows - 1;

	/* get current attributes and fill out CHAR_INFO structure for fill char */
	fill.Char.AsciiChar = ' ';
	fill.Attributes = DefaultAttribute;

	ScrollConsoleScreenBuffer(hConOut, &source, (PSMALL_RECT) 0, dest, &fill);
}

	static void
gotoxy(x, y)
	register int    x,
					y;
{
	ntcoord.X = x - 1;
	ntcoord.Y = y - 1;
	SetConsoleCursorPosition(hConOut, ntcoord);
}

	static void
normvideo()
{
	WORD            attr = DefaultAttribute;

	SetConsoleTextAttribute(hConOut, attr);
}

	static void
textattr(WORD attr)
{
	SetConsoleTextAttribute(hConOut, attr);
}

	static void
putch(char c)
{
	DWORD           count;

	WriteConsole(hConOut, &c, 1, &count, 0);
	ntcoord.X += count;
}

	static void
delay(x)
{
	if (GuiWin)
	{
		if (BenchTime == 0)
			WaitForChar(x);
	}
	else if (IsTelnet)
		Sleep(x);
	else
	{
#ifdef notdef
		Sleep(x);
#else
		while (x > 0)
		{
			if (kbhit())
				break;
			Sleep(100);
			x -= 100;
		}
#endif
	}
}

#ifdef __BORLANDC__
	void
#endif
sleep(x)
{
#ifdef notdef
	Sleep(x * 1000);
#else
	delay(x * 1000);
#endif
#ifndef __BORLANDC__
	return 0;
#endif
}

	static void
vbell()
{
	COORD           origin = {0, 0};
	WORD            flash = ~DefaultAttribute & 0xff;
	DWORD           count;
	LPWORD          oldattrs;

	if (p_vb) {
		oldattrs = (LPWORD) alloc(Rows * Columns * sizeof(WORD));
		if (oldattrs) {
			ReadConsoleOutputAttribute(hConOut, oldattrs, Rows * Columns,
											origin, &count);
			FillConsoleOutputAttribute(hConOut, flash, Rows * Columns,
											origin, &count);
			WriteConsoleOutputAttribute(hConOut, oldattrs, Rows * Columns,
											origin, &count);
			free(oldattrs);
		}
	} else {
		WriteConsole(hConOut, "\a", 1, &count, 0);
	}
}

	static void
cursor_visible(int visible)
{
#ifndef FEPCTRL
	CONSOLE_CURSOR_INFO cci;

	cci.bVisible = visible ? TRUE : FALSE;
#ifdef notdef
	cci.dwSize = 100;           /* 100 percent cursor */
#else
	cci.dwSize = 30;            /*  30 percent cursor */
#endif
	SetConsoleCursorInfo(hConOut, &cci);
#endif
}

	void
set_window(void)
{
}

/*
 * check for an "interrupt signal": CTRL-break or CTRL-C
 */
	void
breakcheck()
{
	if (GuiWin)
	{
		MSG				msg;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE)
		{
			if (!TranslateAccelerator(hVimWnd, hAcc, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
#ifdef USE_EXFILE
	if (ef_breakcheck() || ctrlc_pressed)
#else
	if (ctrlc_pressed)
#endif
	{
		ctrlc_pressed = FALSE;
		got_int = TRUE;
	}
}

	long
mch_avail_mem(spec)
	int spec;
{
	return 0x7fffffff;        /* virual memory eh */
}

/*
 * return non-zero if a character is available
 */
	int
mch_char_avail()
{
	return WaitForChar(0);
}

/*
 * set screen mode, always fails.
 */
	int
mch_screenmode(arg)
	char_u     *arg;
{
	EMSG("Screen mode setting not supported");
	return FAIL;
}

#ifndef notdef
	static int
isctlkey()
{
	switch (ir.Event.KeyEvent.wVirtualKeyCode) {
	case VK_DELETE:
		return '\177';
#ifdef NT106KEY
	case 0xf3:  case 0xf4:        /* ZENKAKU / HANKAKU KEY */
		if (config_nt106)
			return '[' & 0x1f;        /* ESC key !! */
		break;
#endif
	case '6':                    /* jkeyb.sys */ /* ken add */
	case 0xde:                    /* ^ key */
		if (ir.Event.KeyEvent.dwControlKeyState
				& (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
			return '^' & 0x1f;
		break;
	case '@':
	case 0xc0: /* '@' key */
		if (ir.Event.KeyEvent.dwControlKeyState
				& (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
			return K_ZERO;
		break;
	}
	return 0;
}
#endif

	void
chk_ctlkey(c, k)
	int		*c;
	int		*k;
{
	int		w;

	if (term_console)
		return;
	while (1)
	{
		if (*c == Ctrl('Q') || *c == Ctrl(']'))
		{
			w = vgetc();
			switch (w) {
			case 'h':
				*c = K_LARROW;
				break;
			case 'j':
				*c = K_DARROW;
				break;
			case 'k':
				*c = K_UARROW;
				break;
			case 'l':
				*c = K_RARROW;
				break;
			case 'H':
			case 'b':
				*c = K_SLARROW;
				break;
			case 'J':
			case Ctrl('F'):
				*c = K_SDARROW;
				break;
			case 'K':
			case Ctrl('B'):
				*c = K_SUARROW;
				break;
			case 'L':
			case 'w':
				*c = K_SRARROW;
				break;
			default:
				beep();
#ifdef KANJI
				if (ISkanji(w))
					vgetc();
#endif
				*c = vgetc();
#ifdef KANJI
				if (ISkanji(*c))
					*k = vgetc();
#endif
				continue;
			}
		}
		break;
	}
}

static int
iswave(fname)
char		*	fname;
{
	int				fd;
	char			magic[15];
	int				len;

	if ((strlen(fname) >= strlen(".wav"))
			&& strnicmp(&fname[strlen(fname) - 4], ".wav", strlen(".wav")) == 0)
	{
		if ((fd = open(fname, O_RDONLY, S_IREAD | S_IWRITE)) < 0)
			return(FALSE);
		len = read(fd, magic, 15);
		close(fd);
		if (len < 15)
			return(FALSE);
		if (memcmp(magic, "RIFF", 4) == 0
								&& memcmp(&magic[8], "WAVEfmt", 4) == 0)
			return(TRUE);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
PrinterDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;
	OPENFILENAME		ofn;

	switch (uMsg) {
	case WM_INITDIALOG:
		if (strlen(config_printer))
			SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)config_printer);
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			GetDlgItemText(hWnd, 1000, config_printer, sizeof(config_printer));
			EndDialog(hWnd, 0);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, 1);
			return(TRUE);
		case 1001:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			GetDlgItemText(hWnd, 1000, IObuff, IOSIZE);
			*gettail(IObuff) = NUL;
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= hInst;
			ofn.lpstrFilter		= "Program File(*.exe;*.com;*.bat)\0*.exe;*.com;*.bat\0ALL(*.*)\0*.*\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAXPATHL;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= "Print Command Select";
			ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			if (GetOpenFileName(&ofn))
			{
				strcpy(IObuff, "\"");
				strcat(IObuff, NameBuff);
				strcat(IObuff, "\"");
				SendDlgItemMessage(hWnd, 1000, EM_SETSEL, 0, (LPARAM)-2);
				SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)IObuff);
			}
			break;
		}
		break;
	}
	return(FALSE);
}

WINAPI
PrinterThread(PVOID filename)
{
	STARTUPINFO				si;
	PROCESS_INFORMATION		pi;
	char					command[MAXPATHL * 2];

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = /*SW_HIDE*/SW_MINIMIZE;
	sprintf(command, "%s \"%s\"", config_printer, filename);
	if (CreateProcess(NULL, command, NULL, NULL, FALSE,
						CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi) == TRUE)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		TopWindow(hVimWnd);
	}
	remove(filename);
	free(filename);
	ExitThread(0);
	return(0);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
LRESULT PASCAL
BitmapHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND		hwndBrush;
	LPOFNOTIFY		pofn;

	switch (uMsg) {
	case WM_INITDIALOG:
		hwndBrush = GetDlgItem(hwnd, 1010);
		return(TRUE);
	case WM_NOTIFY:
		pofn = (LPOFNOTIFY)lParam;
		if (pofn->hdr.code == CDN_SELCHANGE)
		{
			if (CommDlg_OpenSave_GetSpec(GetParent(hwnd), NameBuff, MAXPATHL) <= MAXPATHL)
				isbitmap(NameBuff, hwndBrush);
		}
		break;
	case WM_COMMAND:
		if (wParam == 1013)
			GetDlgItemText(hwnd, edt1, NameBuff, MAXPATHL);
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
BitmapDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;
	OPENFILENAME		ofn;
	static DWORD		bUse;
	static HWND			udWnd;
	static DWORD		bitsize;
	static DWORD		bitcenter;

	switch (uMsg) {
	case WM_INITDIALOG:
		if (strlen(config_bitmapfile))
			SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)config_bitmapfile);
		bUse = config_bitmap;
		if (bUse)
			CheckDlgButton(hWnd, 1003, MF_CHECKED);
		else
			CheckDlgButton(hWnd, 1003, MF_UNCHECKED);
		bitsize = config_bitsize;
		bitcenter  = config_bitcenter;
		if (config_bitcenter)
			CheckDlgButton(hWnd, 1004, MF_CHECKED);
		else
			CheckDlgButton(hWnd, 1004, MF_UNCHECKED);
		if (pCreateUpDownControl != NULL)
		{
			udWnd = pCreateUpDownControl(
					WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
					132, 39, 8, 12, hWnd, 1009, hInst,
					GetDlgItem(hWnd, 1002), 100, 10, config_bitsize);
		}
		else
		{
			udWnd = NULL;
			wsprintf(NameBuff, "%d", config_bitsize);
			SendDlgItemMessage(hWnd, 1002, WM_SETTEXT, 0, (LPARAM)NameBuff);
		}
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_VSCROLL:
		if (udWnd == (HWND)lParam)
		{
			config_bitsize = GetDlgItemInt(hWnd, 1002, NULL, FALSE);
			updateScreen(CLEAR);
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			GetDlgItemText(hWnd, 1000, config_bitmapfile, sizeof(config_bitmapfile));
			config_bitmap = bUse;
			if (isbitmap(config_bitmapfile, NULL))
			{
				config_bitsize = GetDlgItemInt(hWnd, 1002, NULL, FALSE);
				if (config_bitsize > 100)
					config_bitsize = 100;
				if (config_bitsize < 10)
					config_bitsize = 100;
				EndDialog(hWnd, 0);
			}
			else
			{
				config_bitmap = FALSE;
				config_bitmapfile[0] = '\0';
				config_bitsize   = bitsize;
				config_bitcenter = bitcenter;
				EndDialog(hWnd, 1);
			}
			return(TRUE);
		case IDCANCEL:
			config_bitcenter = bitcenter;
			EndDialog(hWnd, 1);
			return(TRUE);
		case 1001:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			GetDlgItemText(hWnd, 1000, IObuff, IOSIZE);
			*gettail(IObuff) = NUL;
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= hInst;
			ofn.lpstrFilter		= "Graphic Files(*.bmp;*.gif;*.jpg;*.ico;*.emf;*.wmf)\0*.bmp;*.gif;*.jpg;*.ico;*.emf;*.wmf\0Bitmaps(*.bmp)\0*.bmp\0GIF Files(*.gif)\0*.gif\0JPEG Files(*.jpg)\0*.jpg\0Icons(*.ico)\0*.ico\0Enhanced Metafiles(*.emf)\0*.emf\0Windows Metafiles(*.wmf)\0*.wmf\0ALL(*.*)\0*.*\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAXPATHL;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= "Bitmap File Select";
			ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			if (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
											&& ver_info.dwMajorVersion == 3))
			{
				ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
				ofn.lpTemplateName	= "BITMAPDLGEXP";
				ofn.lpfnHook		= BitmapHookProc;
			}
			if (GetOpenFileName(&ofn))
			{
				SendDlgItemMessage(hWnd, 1000, EM_SETSEL, 0, (LPARAM)-2);
				SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)NameBuff);
				if (strcmp(config_bitmapfile, NameBuff) != 0)
				{
					config_bitsize = 100;
					wsprintf(NameBuff, "%d", config_bitsize);
					SendDlgItemMessage(hWnd, 1002, WM_SETTEXT, 0, (LPARAM)NameBuff);
				}
			}
			break;
		case 1003:
			if (bUse)
			{
				bUse = FALSE;
				CheckDlgButton(hWnd, 1003, MF_UNCHECKED);
			}
			else
			{
				bUse = TRUE;
				CheckDlgButton(hWnd, 1003, MF_CHECKED);
			}
			break;
		case 1004:
			if (config_bitcenter)
			{
				config_bitcenter = FALSE;
				CheckDlgButton(hWnd, 1004, MF_UNCHECKED);
			}
			else
			{
				config_bitcenter = TRUE;
				CheckDlgButton(hWnd, 1004, MF_CHECKED);
			}
			updateScreen(CLEAR);
			break;
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
LRESULT PASCAL
WaveHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPOFNOTIFY pofn;

	switch (uMsg) {
	case WM_INITDIALOG:
		return(TRUE);
	case WM_NOTIFY:
		pofn = (LPOFNOTIFY)lParam;
		if (pofn->hdr.code == CDN_SELCHANGE)
			CommDlg_OpenSave_GetSpec(GetParent(hwnd), NameBuff, MAXPATHL);
		break;
	case WM_COMMAND:
		if (wParam == 1013)
		{
			if (ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
											&& ver_info.dwMajorVersion == 3)
				GetDlgItemText(hwnd, edt1, NameBuff, MAXPATHL);
			PlaySound(NameBuff, NULL, SND_FILENAME);
		}
		break;
	}
	return FALSE;
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
WaveDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;
	OPENFILENAME		ofn;
	static DWORD		bUse;

	switch (uMsg) {
	case WM_INITDIALOG:
		if (strlen(config_wavefile))
			SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)config_wavefile);
		bUse = config_wave;
		if (bUse)
			CheckDlgButton(hWnd, 1003, MF_CHECKED);
		else
			CheckDlgButton(hWnd, 1003, MF_UNCHECKED);
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			GetDlgItemText(hWnd, 1000, config_wavefile, sizeof(config_wavefile));
			config_wave = bUse;
			if (iswave(config_wavefile))
				EndDialog(hWnd, 0);
			else
			{
				config_wavefile[0] = '\0';
				config_wave = FALSE;
				EndDialog(hWnd, 1);
			}
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, 1);
			return(TRUE);
		case 1001:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			GetDlgItemText(hWnd, 1000, IObuff, IOSIZE);
			*gettail(IObuff) = NUL;
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= hInst;
			ofn.lpstrFilter		= "Wave File(*.wav)\0*.wav\0ALL(*.*)\0*.*\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAXPATHL;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= "Wave File Select";
			ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			if (ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
											&& ver_info.dwMajorVersion == 3)
			{
				ofn.Flags			= OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
				ofn.lpTemplateName	= "WAVEFILEOPENDIALOG";
			}
			else
			{
				ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
				ofn.lpTemplateName	= "WAVEDLGEXP";
			}
			ofn.lpfnHook		= WaveHookProc;
			if (GetOpenFileName(&ofn))
			{
				SendDlgItemMessage(hWnd, 1000, EM_SETSEL, 0, (LPARAM)-2);
				SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)NameBuff);
			}
			break;
		case 1003:
			if (bUse)
			{
				bUse = FALSE;
				CheckDlgButton(hWnd, 1003, MF_UNCHECKED);
			}
			else
			{
				bUse = TRUE;
				CheckDlgButton(hWnd, 1003, MF_CHECKED);
			}
			break;
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
CommandDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;

	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd, 1001, EM_SETLIMITTEXT, 0, (LPARAM)sizeof(config_load));
		SendDlgItemMessage(hWnd, 1002, EM_SETLIMITTEXT, 0, (LPARAM)sizeof(config_unload));
		SendDlgItemMessage(hWnd, 1001, WM_SETTEXT, 0, (LPARAM)"");
		SendDlgItemMessage(hWnd, 1002, WM_SETTEXT, 0, (LPARAM)"");
		if (GuiConfig == 0)
		{
			SendDlgItemMessage(hWnd, 1001, EM_SETREADONLY, TRUE, 0);
			SendDlgItemMessage(hWnd, 1002, EM_SETREADONLY, TRUE, 0);
		}
		else
		{
			SendDlgItemMessage(hWnd, 1001, WM_SETTEXT, 0, (LPARAM)config_load);
			SendDlgItemMessage(hWnd, 1002, WM_SETTEXT, 0, (LPARAM)config_unload);
		}
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			GetDlgItemText(hWnd, 1001, config_load, sizeof(config_load));
			GetDlgItemText(hWnd, 1002, config_unload, sizeof(config_unload));
			EndDialog(hWnd, 0);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, 1);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static void
UnloadCommand()
{
	++no_wait_return;
	if (GuiConfig != 0)
		docmdline(config_unload);
	--no_wait_return;
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static void
LoadCommand()
{
	char			*	p;
	char				load[CMDBUFFSIZE];

	++no_wait_return;
	if (GuiConfig != 0)
	{
		STRCPY(load, config_load);
		p = load;
		while (*p)
		{
			if (*p == '\\' && p[1])
			{
				if (isalpha(p[1]))
					*p = toupper(p[1]) - 0x40;
				else
					*p = p[1];
				STRCPY(p + 1, p + 2);
			}
			p++;
		}
		docmdline(load);
	}
	--no_wait_return;
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
void
InitCommand()
{
	if (config_comb && GuiConfig)
		LoadCommand();
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
LoadDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE	| SWP_NOSIZE);
		return(TRUE);
	default:
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static char *
DisplayPathName(char *fname, unsigned int max)
{
	char			*	file;
	char			*	top;
	char			*	bot;
	char			*	p;
	static char			dot[] = "...";
	static char			disp[_MAX_PATH];

	memset(disp, '\0', sizeof(disp));
	if (strlen(fname) < max)
	{
		strcpy(disp, fname);
		return(disp);
	}
	file = gettail(fname);
	if (strlen(file) >= sizeof(disp))
		return(file);
	if ((strlen(file) + sizeof(dot) + 3) > max)
		strcpy(disp, file);
	else
	{
		--file;		/* path separater include */
		bot = p = fname + 3;
		while (*p && p < file)
		{
#ifdef KANJI
			if (ISkanji(*p))
				p++;
			else
#endif
			if (*p == '\\' || *p == '/' || *p == ':')
				bot =p;
			p++;
		}
		if ((strlen(bot) + sizeof(dot)) < max)
			file = bot;
		top = p = fname + 3;
		while (*p)
		{
#ifdef KANJI
			if (ISkanji(*p))
				p++;
			else
#endif
			if (*p == '\\' || *p == '/' || *p == ':')
				top = p + 1;
			p++;
			if ((strlen(file) + sizeof(dot) + (p - fname)) > max)
				break;
		}
		strncpy(disp, fname, top - fname);
		strcat(disp, dot);
		strcat(disp, file);
	}
	return(disp);
}

#ifdef USE_HISTORY
/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static int
HistoryCount()
{
	HKEY				hKey;
	char				name[CMDBUFFSIZE];
	int					i;

	for (i = 1; i <= MAX_HISTORY; i++)
	{
		sprintf(name, "Software\\Vim\\History\\%03d", i);
		if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
			RegCloseKey(hKey);
		else
			return(i - 1);
	}
	return(MAX_HISTORY);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static BOOL
HistoryGetNo(int no, char *fname, int *line)
{
	HKEY				hKey;
	DWORD				size;
	DWORD				type;
	char				name[CMDBUFFSIZE];

	sprintf(name, "Software\\Vim\\History\\%03d", no);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
									KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		size = MAXPATHL;
		type = REG_SZ;
		if (RegQueryValueEx(hKey, "name", NULL, &type,
									(BYTE *)fname, &size) == ERROR_SUCCESS)
		{
			size = sizeof(*line);
			type = REG_DWORD;
			if (RegQueryValueEx(hKey, "line", NULL, &type,
									(BYTE *)line, &size) == ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				return(TRUE);
			}
		}
		RegCloseKey(hKey);
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static char *
HistoryGetMenu(int no)
{
	int					line;
	char				name[MAXPATHL];
	static char			buff[CMDBUFFSIZE];

	if (HistoryGetNo(no, name, &line))
	{
		sprintf(buff, "&%d %s", no, DisplayPathName(name, TITLE_LEN));
		return(buff);
	}
	return(NULL);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static char *
HistoryGetCommand(int no)
{
	int					line;
	char				name[MAXPATHL];
	static char			buff[CMDBUFFSIZE];
	int					max;
	int					i;

	if (HistoryGetNo(no, name, &line))
	{
		while (getperm(name) == (-1))
		{
			max = HistoryCount();
			if (max != no)
			{
				for (i = no; i < max; i++)
					HistoryRename(i + 1, i);
			}
			sprintf(buff, "File \"%s\" not found.", name);
			MessageBox(hVimWnd, buff,
#ifdef KANJI
					JpVersion,
#else
					Version,
#endif
					MB_OK);
			return(NULL);
		}
		sprintf(buff, ":e +%d %s\n", line, name);
		return(buff);
	}
	return(NULL);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
HistoryRename(int old, int new)
{
	HKEY				hoKey;
	HKEY				hnKey;
	DWORD				size;
	DWORD				type;
	DWORD				line;
	char				oname[CMDBUFFSIZE];
	char				nname[CMDBUFFSIZE];
	char				name[MAXPATHL];

	sprintf(oname, "Software\\Vim\\History\\%03d", old);
	sprintf(nname, "Software\\Vim\\History\\%03d", new);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, nname, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hnKey, &size)
															!= ERROR_SUCCESS)
		return;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, oname, 0,
									KEY_ALL_ACCESS, &hoKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hnKey);
		return;
	}
	size = sizeof(name);
	type = REG_SZ;
	if (RegQueryValueEx(hoKey, NULL, NULL, &type,
								(BYTE *)name, &size) == ERROR_SUCCESS)
		RegSetValueEx(hnKey, NULL, 0, REG_SZ, (BYTE *)name, size);
	size = sizeof(name);
	type = REG_SZ;
	if (RegQueryValueEx(hoKey, "name", NULL, &type,
								(BYTE *)name, &size) == ERROR_SUCCESS)
		RegSetValueEx(hnKey, "name", 0, REG_SZ, (BYTE *)name, size);
	size = sizeof(line);
	type = REG_DWORD;
	if (RegQueryValueEx(hoKey, "line", NULL, &type,
								(BYTE *)&line, &size) == ERROR_SUCCESS)
		RegSetValueEx(hnKey, "line", 0, REG_DWORD, (BYTE *)&line, size);
	RegDeleteKey(hoKey, "name");
	RegDeleteKey(hoKey, "line");
	RegCloseKey(hoKey);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Vim\\History", 0,
									KEY_ALL_ACCESS, &hoKey) == ERROR_SUCCESS)
	{
		sprintf(oname, "%03d", old);
		RegDeleteKey(hoKey, oname);
		RegCloseKey(hoKey);
	}
	RegCloseKey(hnKey);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static int
HistoryDuplicate(char *fname)
{
	HKEY				hKey;
	DWORD				size;
	DWORD				type;
	char				name[CMDBUFFSIZE];
	int					i;

	for (i = 1; i <= MAX_HISTORY; i++)
	{
		sprintf(name, "Software\\Vim\\History\\%03d", i);
		if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
		{
			size = sizeof(name);
			type = REG_SZ;
			if (RegQueryValueEx(hKey, "name", NULL, &type,
										(BYTE *)name, &size) == ERROR_SUCCESS)
			{
				if (fnamecmp(fname, name) == 0)
				{
					RegCloseKey(hKey);
					return(i);
				}
			}
			RegCloseKey(hKey);
		}
		else
			break;
	}
	return(0);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
HistoryAppend(char *name, int line)
{
	HKEY				hKey;
	DWORD				size;
	int					max;
	int					i;
	char				date[_MAX_PATH];

	if (name == NULL)
		return;
	if ((max = HistoryDuplicate(name)) == 0)
	{
		max = HistoryCount();
		if (max < MAX_HISTORY)
			max++;
	}
	for (i = max; i > 1; i--)
		HistoryRename(i - 1, i);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Vim\\History\\001", 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &size)
															!= ERROR_SUCCESS)
		return;
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, date, sizeof(date));
	strcat(date, " ");
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, &date[strlen(date)], sizeof(date) - strlen(date));
	size = strlen(date) + 1;
	RegSetValueEx(hKey, NULL, 0, REG_SZ, date, size);
	size = strlen(name) + 1;
	RegSetValueEx(hKey, "name", 0, REG_SZ, (BYTE *)name, size);
	size = sizeof(line);
	RegSetValueEx(hKey, "line", 0, REG_DWORD, (BYTE *)&line, size);
	RegCloseKey(hKey);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static int
HistoryGetLine(char *fname)
{
	HKEY				hKey;
	DWORD				size;
	DWORD				type;
	DWORD				line;
	char				name[CMDBUFFSIZE];
	int					i;

	if (fname == NULL)
		return(0);
	for (i = 1; i <= MAX_HISTORY; i++)
	{
		sprintf(name, "Software\\Vim\\History\\%03d", i);
		if (RegOpenKeyEx(HKEY_CURRENT_USER, name, 0,
										KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
		{
			size = sizeof(name);
			type = REG_SZ;
			if (RegQueryValueEx(hKey, "name", NULL, &type,
										(BYTE *)name, &size) == ERROR_SUCCESS)
			{
				if (fnamecmp(fname, name) == 0)
				{
					size = sizeof(line);
					type = REG_DWORD;
					RegQueryValueEx(hKey, "line", NULL, &type, (BYTE *)&line, &size);
					RegCloseKey(hKey);
					return(line);
				}
			}
			RegCloseKey(hKey);
		}
		else
			break;
	}
	return(0);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
void
win_history_append(BUF *buf)
{
	WIN 	*	wp;

	if (!GuiWin || !config_hauto || config_ini)
		return;
	if (buf->b_filename == NULL || buf->b_mtime == 0)
		return;
	if (getperm(buf->b_filename) == (-1))
		return;
	for (wp = firstwin; wp != NULL; wp = wp->w_next)
	{
		if (wp->w_buffer == buf)
		{
			HistoryAppend(buf->b_filename, wp->w_cursor.lnum);
			break;
		}
	}
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
char *
win_history_line(BUF *buf)
{
	int				line;
	static char		cmdline[128];

	if (!GuiWin || !config_history || config_ini)
		return(NULL);
	if (buf->b_filename == NULL)
		return(NULL);
	line = HistoryGetLine(buf->b_filename);
	if (line <= 1)
		return(NULL);
	sprintf(cmdline, "+%d", line - 1);
	sprintf(cmdline, ":%d", line);
	return(cmdline);
}
#endif

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
LineSpaceDialogEx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;
	static HWND			udCWnd;
	static HWND			udLWnd;
	static HWND			udTWnd;
	static DWORD		cspace;
	static DWORD		lspace;
	static DWORD		tspace;

	switch (uMsg) {
	case WM_INITDIALOG:
		cspace = v_cspace;
		lspace = v_lspace;
		tspace = v_trans;
		if (pCreateUpDownControl != NULL)
		{
			udCWnd = pCreateUpDownControl(
					WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
					132, 39, 8, 12, hWnd, 1009, hInst,
					GetDlgItem(hWnd, 1001), 10, 0, v_cspace);
			udLWnd = pCreateUpDownControl(
					WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
					132, 39, 8, 12, hWnd, 1009, hInst,
					GetDlgItem(hWnd, 1002), 10, 0, v_lspace);
			udTWnd = pCreateUpDownControl(
					WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
					132, 39, 8, 12, hWnd, 1009, hInst,
					GetDlgItem(hWnd, 1003), 100, 0, v_trans);
		}
		else
		{
			udCWnd = NULL;
			udLWnd = NULL;
			udTWnd = NULL;
			wsprintf(NameBuff, "%d", v_cspace);
			SendDlgItemMessage(hWnd, 1001, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", v_lspace);
			SendDlgItemMessage(hWnd, 1002, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", v_trans);
			SendDlgItemMessage(hWnd, 1003, WM_SETTEXT, 0, (LPARAM)NameBuff);
		}
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_VSCROLL:
		if (udCWnd == (HWND)lParam)
		{
			v_cspace = GetDlgItemInt(hWnd, 1001, NULL, FALSE);
			ResetScreen(hVimWnd);
			mch_set_winsize();
			updateScreen(CLEAR);
			return 1;
		}
		if (udLWnd == (HWND)lParam)
		{
			v_lspace = GetDlgItemInt(hWnd, 1002, NULL, FALSE);
			ResetScreen(hVimWnd);
			mch_set_winsize();
			updateScreen(CLEAR);
			return 1;
		}
		if (udTWnd == (HWND)lParam)
		{
			v_trans = GetDlgItemInt(hWnd, 1003, NULL, FALSE);
			ResetScreen(hVimWnd);
			mch_set_winsize();
			updateScreen(CLEAR);
			SetLayerd();
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			v_cspace = GetDlgItemInt(hWnd, 1001, NULL, FALSE);
			if (v_cspace > 10)
				v_cspace = 10;
			v_lspace = GetDlgItemInt(hWnd, 1002, NULL, FALSE);
			if (v_lspace > 10)
				v_lspace = 10;
			v_trans = GetDlgItemInt(hWnd, 1003, NULL, FALSE);
			if (v_trans > 100)
				v_trans = 100;
			SetLayerd();
			EndDialog(hWnd, 0);
			return(TRUE);
		case IDCANCEL:
			v_cspace = cspace;
			v_lspace = lspace;
			v_trans  = tspace;
			SetLayerd();
			EndDialog(hWnd, 1);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
LineSpaceDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;
	static HWND			udCWnd;
	static HWND			udLWnd;
	static DWORD		cspace;
	static DWORD		lspace;

	switch (uMsg) {
	case WM_INITDIALOG:
		cspace = v_cspace;
		lspace = v_lspace;
		if (pCreateUpDownControl != NULL)
		{
			udCWnd = pCreateUpDownControl(
					WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
					132, 39, 8, 12, hWnd, 1009, hInst,
					GetDlgItem(hWnd, 1001), 10, 0, v_cspace);
			udLWnd = pCreateUpDownControl(
					WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT,
					132, 39, 8, 12, hWnd, 1009, hInst,
					GetDlgItem(hWnd, 1002), 10, 0, v_lspace);
		}
		else
		{
			udCWnd = NULL;
			udLWnd = NULL;
			wsprintf(NameBuff, "%d", v_cspace);
			SendDlgItemMessage(hWnd, 1001, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", v_lspace);
			SendDlgItemMessage(hWnd, 1002, WM_SETTEXT, 0, (LPARAM)NameBuff);
		}
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_VSCROLL:
		if (udCWnd == (HWND)lParam)
		{
			v_cspace = GetDlgItemInt(hWnd, 1001, NULL, FALSE);
			ResetScreen(hVimWnd);
			mch_set_winsize();
			updateScreen(CLEAR);
			return 1;
		}
		if (udLWnd == (HWND)lParam)
		{
			v_lspace = GetDlgItemInt(hWnd, 1002, NULL, FALSE);
			ResetScreen(hVimWnd);
			mch_set_winsize();
			updateScreen(CLEAR);
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			v_cspace = GetDlgItemInt(hWnd, 1001, NULL, FALSE);
			if (v_cspace > 10)
				v_cspace = 10;
			v_lspace = GetDlgItemInt(hWnd, 1002, NULL, FALSE);
			if (v_lspace > 10)
				v_lspace = 10;
			EndDialog(hWnd, 0);
			return(TRUE);
		case IDCANCEL:
			v_cspace = cspace;
			v_lspace = lspace;
			EndDialog(hWnd, 1);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

static void
SetLayerd()
{
	static int			vtrans		= 0;

	if (pSetLayeredWindowAttributes == NULL)
		v_trans = 0;
	else
	{
		if (vtrans == v_trans)
			return;
		if (v_trans == 0)
		{
			pSetLayeredWindowAttributes(hVimWnd, 0, (BYTE)255, LWA_ALPHA);
			SetWindowLong(hVimWnd, GWL_EXSTYLE,
					GetWindowLong(hVimWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
		}
		else
		{
			if (vtrans == 0)
			{
				SetWindowLong(hVimWnd, GWL_EXSTYLE,
						GetWindowLong(hVimWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			}
			pSetLayeredWindowAttributes(hVimWnd, 0,
					(BYTE)(((230 * (100 - v_trans)) / 100) + 25), LWA_ALPHA);
		}
		vtrans = v_trans;
		ResetScreen(hVimWnd);
	}
}

static BOOL
LoadBitmapFromBMPFile(HDC hdc, LPTSTR szFileName)
{
	HANDLE				hFile;
	DWORD				dwFileSize;
	LPVOID				pvData		= NULL;
	HGLOBAL				hGlobal;
	DWORD				dwBytesRead = 0;
	LPSTREAM			pstm		= NULL;
	LPPICTURE			gpPicture	= NULL;
	static HBITMAP		hBitmap		= NULL;
	static HDC			hDCMem		= NULL;
	static int			cxDesired	= 0;
	static int			cyDesired	= 0;
	static DWORD		bitsize		= 0;
	static DWORD		bitcenter	= -1;
	static BOOL			bh			= FALSE;
	static char			bitmap[MAXPATHL];
	static DWORD		bgcolor;
	static long			hmWidth;
	static long			hmHeight;
	static int			cx;
	static int			cy;
	double				per = 1;
	int					nWidth;
	int					nHeight;
	HBRUSH				hbrush, holdbrush;
	RECT				rcWindow;
	HBITMAP				hOldBitmap;
	HDC					hMemDC;
	RGBQUAD				rgb[256];
	LPLOGPALETTE		pLogPal;
	HPALETTE			hPalette;
	HPALETTE			hOldPalette;
	int					i;

	if ((hBitmap != NULL
				&& (cxDesired != (Columns * v_xchar) || cyDesired != (Rows * v_ychar) || config_bitsize != bitsize || config_bitcenter != bitcenter))
			|| strcmp(bitmap, szFileName) != 0
			|| bSyncPaint
			|| bgcolor != *v_bgcolor)
	{
		if (hBitmap)
			DeleteObject(hBitmap);
		hBitmap = NULL;
		if (hDCMem)
			DeleteDC(hDCMem);
		hDCMem = NULL;
		strcpy(bitmap, szFileName);
		bgcolor = *v_bgcolor;
		bitsize   = config_bitsize;
		bitcenter = config_bitcenter;
		bSyncPaint = FALSE;
	}
	if (hBitmap == NULL)
	{
		if ((hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			return(FALSE);
		if ((dwFileSize = GetFileSize(hFile, NULL)) == -1)
		{
			CloseHandle(hFile);
			return(FALSE);
		}
		if ((hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize)) == NULL)
		{
			CloseHandle(hFile);
			return(FALSE);
		}
		if ((pvData = GlobalLock(hGlobal)) == NULL)
		{
			GlobalUnlock(hGlobal);
			GlobalFree(hGlobal);
			CloseHandle(hFile);
			return(FALSE);
		}
		if (!ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL))
		{
			GlobalUnlock(hGlobal);
			GlobalFree(hGlobal);
			CloseHandle(hFile);
			return(FALSE);
		}
		GlobalUnlock(hGlobal);
		CloseHandle(hFile);
		if (CreateStreamOnHGlobal(hGlobal, TRUE, &pstm))
		{
			GlobalFree(hGlobal);
			return(FALSE);
		}
		if (OleLoadPicture(pstm, dwFileSize, FALSE, (REFIID)&IID_IPicture, (LPVOID *)&gpPicture))
		{
			GlobalFree(hGlobal);
			pstm->lpVtbl->Release(pstm);
			return(FALSE);
		}
		pstm->lpVtbl->Release(pstm);
		gpPicture->lpVtbl->get_Width(gpPicture, &hmWidth);
		gpPicture->lpVtbl->get_Height(gpPicture, &hmHeight);

		cxDesired = Columns * v_xchar;
		cyDesired = Rows * v_ychar;

		if (hmWidth > cxDesired || hmHeight > cyDesired)
		{
			if (((double)hmWidth / (double)cxDesired)
								> ((double)hmHeight / (double)cyDesired))
			{
				per = 1.0 / ((double)hmWidth / (double)cxDesired);
				bh = FALSE;
			}
			else
			{
				per = 1.0 / ((double)hmHeight / (double)cyDesired);
				bh = TRUE;
			}
		}
		else
		{
			if (((double)cxDesired / (double)hmWidth)
								< ((double)cyDesired / (double)hmHeight))
			{
				per = (double)cxDesired / (double)hmWidth;
				bh = FALSE;
			}
			else
			{
				per = (double)cyDesired / (double)hmHeight;
				bh = TRUE;
			}
		}
		cx = cxDesired;
		cy = cyDesired;
		if (bh)
			cx = (int)(hmWidth * per);
		else
			cy = (int)(hmHeight * per);

		hMemDC = CreateCompatibleDC(NULL);
		GetDIBColorTable(hMemDC, 0, 256, rgb);
		pLogPal = malloc(sizeof(LOGPALETTE) + (256 * sizeof(PALETTEENTRY)));
		pLogPal->palVersion = 0x300;
		pLogPal->palNumEntries = 256;
		for (i = 0; i < 256; i++)
		{
			pLogPal->palPalEntry[i].peRed	= rgb[i].rgbRed;
			pLogPal->palPalEntry[i].peGreen	= rgb[i].rgbGreen;
			pLogPal->palPalEntry[i].peBlue	= rgb[i].rgbBlue;
			pLogPal->palPalEntry[i].peFlags	= 0;
		}
		hPalette = CreatePalette(pLogPal);
		free(pLogPal);
		DeleteDC(hMemDC);

		GetWindowRect(hVimWnd, &rcWindow);
		hbrush	= CreateSolidBrush(*v_bgcolor);
		holdbrush = SelectObject(hdc, hbrush);
		rcWindow.left = 0;
		rcWindow.top = 0;
		rcWindow.right = cxDesired;
		rcWindow.bottom = cyDesired;
		FillRect(hdc, &rcWindow, hbrush);
		SelectObject(hdc, holdbrush);
		DeleteObject(hbrush);
		// convert himetric to pixels
		nWidth	= MulDiv(hmWidth, GetDeviceCaps(hdc, LOGPIXELSX), HIMETRIC_INCH);
		nHeight	= MulDiv(hmHeight, GetDeviceCaps(hdc, LOGPIXELSY), HIMETRIC_INCH);
		if (config_bitsize != 100)
		{
			cx = (cx * config_bitsize) / 100;
			cy = (cy * config_bitsize) / 100;
		}
		gpPicture->lpVtbl->Render(gpPicture, hdc,
				config_bitcenter ? (cxDesired - cx) / 2 : cxDesired - cx,
				config_bitcenter ? (cyDesired - cy) / 2 : 0,
				cx, cy, 0, hmHeight, hmWidth, -hmHeight, &rcWindow);
		hBitmap		= CreateCompatibleBitmap(hdc, cxDesired, cyDesired);
		hDCMem		= CreateCompatibleDC(hdc);
		hOldBitmap	= SelectObject(hDCMem, hBitmap);
		hOldPalette	= SelectPalette(hdc, hPalette, FALSE);
		RealizePalette(hdc);
		BitBlt(hDCMem, 0, 0, cxDesired, cyDesired, hdc, 0, 0, SRCCOPY);
		SelectObject(hDCMem, hOldBitmap);
		SelectPalette(hdc, hOldPalette, FALSE);
		DeleteObject(hPalette);
		gpPicture->lpVtbl->Release(gpPicture);
	}
	else if (!hBitmap)
		return(FALSE);
	hMemDC		= CreateCompatibleDC(hdc);
	hOldBitmap	= SelectObject(hMemDC, hBitmap);
	BitBlt(hdc, 0, 0, cxDesired, cyDesired, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
	return(TRUE);
}

static BOOL
isbitmap(LPTSTR szFileName, HWND hwnd)
{
	HANDLE				hFile;
	DWORD				dwFileSize;
	LPVOID				pvData		= NULL;
	HGLOBAL				hGlobal;
	DWORD				dwBytesRead = 0;
	LPSTREAM			pstm		= NULL;
	static LPPICTURE	gpPicture	= NULL;

	if ((hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		return(FALSE);
	if ((dwFileSize = GetFileSize(hFile, NULL)) == -1)
	{
		CloseHandle(hFile);
		return(FALSE);
	}
	if ((hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize)) == NULL)
	{
		CloseHandle(hFile);
		return(FALSE);
	}
	if ((pvData = GlobalLock(hGlobal)) == NULL)
	{
		GlobalUnlock(hGlobal);
		GlobalFree(hGlobal);
		CloseHandle(hFile);
		return(FALSE);
	}
	if (!ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL))
	{
		GlobalUnlock(hGlobal);
		GlobalFree(hGlobal);
		CloseHandle(hFile);
		return(FALSE);
	}
	GlobalUnlock(hGlobal);
	CloseHandle(hFile);
	if (CreateStreamOnHGlobal(hGlobal, TRUE, &pstm))
	{
		GlobalFree(hGlobal);
		return(FALSE);
	}
	if (OleLoadPicture(pstm, dwFileSize, FALSE, (REFIID)&IID_IPicture, (LPVOID *)&gpPicture))
	{
		GlobalFree(hGlobal);
		pstm->lpVtbl->Release(pstm);
		return(FALSE);
	}
	if (hwnd != NULL)
	{
		long				hmWidth;
		long				hmHeight;
		RECT				rcWindow;
		HDC					hdc;

		GetClientRect(hwnd, &rcWindow);
		hdc = GetDC(hwnd);
		gpPicture->lpVtbl->get_Width(gpPicture, &hmWidth);
		gpPicture->lpVtbl->get_Height(gpPicture, &hmHeight);
		gpPicture->lpVtbl->Render(gpPicture, hdc, 0, 0, rcWindow.right, rcWindow.bottom, 0, hmHeight, hmWidth, -hmHeight, &rcWindow);
		ReleaseDC(hwnd, hdc);
	}
	gpPicture->lpVtbl->Release(gpPicture);
	pstm->lpVtbl->Release(pstm);
	return(TRUE);
}

#ifdef KANJI
/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static BOOL CALLBACK
FontDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int					wmId;
	CHOOSEFONT			cfFont;
	static LOGFONT		logfont;
	static LOGFONT		jlogfont;
	LOGFONT				work;

	switch (uMsg) {
	case WM_INITDIALOG:
		memcpy(&logfont, &config_font, sizeof(logfont));
		memcpy(&jlogfont, &config_jfont, sizeof(jlogfont));
		SendDlgItemMessage(hWnd, 2000, WM_SETTEXT, 0, (LPARAM)config_font.lfFaceName);
		SendDlgItemMessage(hWnd, 4000, WM_SETTEXT, 0, (LPARAM)config_jfont.lfFaceName);
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			memcpy(&config_font, &logfont, sizeof(config_font));
			memcpy(&config_jfont, &jlogfont, sizeof(config_jfont));
			EndDialog(hWnd, 0);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, 1);
			return(TRUE);
		case 1001:
		case 3001:
			memset(&cfFont, 0, sizeof(cfFont));
			if (wmId == 1001)
				memcpy(&work, &logfont, sizeof(work));
			else
				memcpy(&work, &jlogfont, sizeof(work));
			cfFont.lStructSize	= sizeof(cfFont);
			cfFont.hwndOwner	= hWnd;
			cfFont.hDC			= NULL;
			cfFont.rgbColors	= *v_fgcolor;
			cfFont.lpLogFont	= &work;
			cfFont.Flags		= CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT
									| CF_ANSIONLY | CF_NOVERTFONTS
									/* | CF_FIXEDPITCHONLY | CF_EFFECTS */ ;
			cfFont.lCustData	= 0;
			cfFont.lpfnHook		= NULL;
			cfFont.lpTemplateName= NULL;
			cfFont.hInstance	= hInst;
			if (ChooseFont(&cfFont))
			{
				if (wmId == 1001)
					memcpy(&logfont, &work, sizeof(work));
				else
					memcpy(&jlogfont, &work, sizeof(work));
				SendDlgItemMessage(hWnd, 2000, WM_SETTEXT, 0, (LPARAM)logfont.lfFaceName);
				SendDlgItemMessage(hWnd, 4000, WM_SETTEXT, 0, (LPARAM)jlogfont.lfFaceName);
			}
			break;
		}
		break;
	}
	return(FALSE);
}
#endif

	int
mbox(char_u *s, ...)
{
	char		buf[256];
	va_list		ap;

	va_start(ap, s);
	wvsprintf(buf, s, ap);
	va_end(ap);
	MessageBox(NULL, buf, "Debug", MB_OK);
	return(0);
}

	int
debugf(char_u *s, ...)
{
	char		buf[512];
	va_list		ap;
	static FILE *fp = NULL;

	if (fp == NULL)
	{
		if ((fp = fopen("d:\\tmp\\vim.log", "w")) == NULL)
			return 0;
	}
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, buf, sizeof(buf));
	wsprintf(&buf[strlen(buf)], ":%08x: ", GetTickCount());
	va_start(ap, s);
	wvsprintf(&buf[strlen(buf)], s, ap);
	va_end(ap);
	fprintf(fp, buf);
	fflush(fp);
	return(0);
}
