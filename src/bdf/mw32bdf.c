/*****************************************
  Meadow BDF font manager
         Written by H.Miyashita.
******************************************/

/*****************************************

	This module provides these functions.

		bdffont *mw32_init_bdf_font(char *filename);

		void mw32_free_bdf_font(bdffont *fontp);

		int mw32_get_bdf_glyph(bdffont *fontp, int index, int size,
													glyph_struct *glyph);

		int mw32_BDF_TextOut(HDC hdc, int left,
						int top, unsigned char *text, int dim, int bytelen,
						int fixed_pitch_size, int char_space, int line_space);

******************************************/
#include <windows.h>
#include "mw32bdf.h"

static char		*g_bdffile = NULL;
static char		*g_jbdffile = NULL;
static bdffont *g_bdffontp = NULL;
static bdffont *g_jbdffontp = NULL;
static int		g_xsize;
static int		g_ysize;
static int		g_fix;
static BOOL		bJpn;

void mw32_free_bdf_font(bdffont *fontp);
bdffont *mw32_init_bdf_font(char *filename);
static int compchr(char *text, int len, char ch);

#define GLYPH_CACHE		1
#define BITMAP_CACHE	1

#if GLYPH_CACHE
static struct gcache {
	int					index;
	unsigned int		hit;
	glyph_struct		glyph;
} glyph_cache[0x500];

static VOID
init_cached_glyph()
{
	int				i;

	for (i = 0; i < sizeof(glyph_cache) / sizeof(struct gcache); i++)
	{
		if (glyph_cache[i].glyph.bitmap != NULL)
			free(glyph_cache[i].glyph.bitmap);
	}
	memset(glyph_cache, 0, sizeof(glyph_cache));
}

static int
get_cached_glyph(bdffont *fontp, int index, int size, glyph_struct *glyph)
{
	int				i;
	unsigned int	min = 0xffffffff;
	int				array = -1;

	if (0x20 <= index && index <= 0x7f)				/* base 0x0000 */
	{
		array = index - 0x20;
		if (glyph_cache[array].hit)
			goto glyph_copy;
	}
	else if (0xa0 <= index && index <= 0xdf)		/* base 0x0060 */
	{
		array = index - 0xa0 + 0x60;
		if (glyph_cache[array].hit)
			goto glyph_copy;
	}
	else if (0x2120 <= index && index <= 0x217f)	/* base 0x00a0 */
	{
		array = index - 0x2120 + 0xa0;
		if (glyph_cache[array].hit)
			goto glyph_copy;
	}
	else if (0x2330 <= index && index <= 0x237f)	/* base 0x0100 */
	{
		array = index - 0x2330 + 0x100;
		if (glyph_cache[array].hit)
			goto glyph_copy;
	}
	else if (0x2420 <= index && index <= 0x247f)	/* base 0x0150 */
	{
		array = index - 0x2420 + 0x150;
		if (glyph_cache[array].hit)
			goto glyph_copy;
	}
	else if (0x2520 <= index && index <= 0x257f)	/* base 0x01b0 */
	{
		array = index - 0x2520 + 0x1b0;
		if (glyph_cache[array].hit)
			goto glyph_copy;
	}
	else											/* base 0x0210 */
	{
		for (i = 0x210; i < sizeof(glyph_cache) / sizeof(struct gcache); i++)
		{
			if (glyph_cache[i].index == index)
			{
				array = i;
				goto glyph_copy;
			}
			if (glyph_cache[i].index == 0)
			{
				array = i;
				break;
			}
			if (min > glyph_cache[i].hit)
			{
				min = glyph_cache[i].hit;
				array = i;
			}
		}
	}
	if (array < 0)
		array = 0x210;
	glyph_cache[array].index = 0;
	glyph_cache[array].hit   = 0;
	if (glyph_cache[array].glyph.bitmap == NULL)
	{
		glyph_cache[array].glyph.bitmap
					= (unsigned char*) malloc(sizeof(unsigned char) * size);
		if (glyph_cache[array].glyph.bitmap == NULL)
			return(0);
	}
	if (mw32_get_bdf_glyph(fontp, index, size, &glyph_cache[array].glyph))
	{
glyph_copy:
		if (glyph_cache[array].hit < 0xf0000000)
			glyph_cache[array].hit++;
		glyph->dwidth		= glyph_cache[array].glyph.dwidth;
		glyph->bbw			= glyph_cache[array].glyph.bbw;
		glyph->bbh			= glyph_cache[array].glyph.bbh;
		glyph->bbox			= glyph_cache[array].glyph.bbox;
		glyph->bboy			= glyph_cache[array].glyph.bboy;
		glyph->bitmap_size	= glyph_cache[array].glyph.bitmap_size;
		memcpy(glyph->bitmap, glyph_cache[array].glyph.bitmap, size);
		return(1);
	}
	return(0);
}
#endif

#if BITMAP_CACHE
static struct bcache {
	int					index;
	unsigned int		hit;
	HBITMAP				hBitmap;
} bitmap_cache[0x500];

static VOID
init_cached_bitmap()
{
	int				i;

	for (i = 0; i < sizeof(bitmap_cache) / sizeof(struct bcache); i++)
	{
		if (bitmap_cache[i].hBitmap != NULL)
			DeleteObject(bitmap_cache[i].hBitmap);
	}
	memset(bitmap_cache, 0, sizeof(bitmap_cache));
}

static HBITMAP
get_cached_bitmap(int index, glyph_struct *glyph)
{
	HBITMAP			hBMP;
	int				i;
	unsigned int	min = 0xffffffff;
	int				array = -1;

	if (0x20 <= index && index <= 0x7f)				/* base 0x0000 */
	{
		array = index - 0x20;
		if (bitmap_cache[array].hBitmap)
			return(bitmap_cache[array].hBitmap);
	}
	else if (0xa0 <= index && index <= 0xdf)		/* base 0x0060 */
	{
		array = index - 0xa0 + 0x60;
		if (bitmap_cache[array].hBitmap)
			return(bitmap_cache[array].hBitmap);
	}
	else if (0x2120 <= index && index <= 0x217f)	/* base 0x00a0 */
	{
		array = index - 0x2120 + 0xa0;
		if (bitmap_cache[array].hBitmap)
			return(bitmap_cache[array].hBitmap);
	}
	else if (0x2330 <= index && index <= 0x237f)	/* base 0x0100 */
	{
		array = index - 0x2330 + 0x100;
		if (bitmap_cache[array].hBitmap)
			return(bitmap_cache[array].hBitmap);
	}
	else if (0x2420 <= index && index <= 0x247f)	/* base 0x0150 */
	{
		array = index - 0x2420 + 0x150;
		if (bitmap_cache[array].hBitmap)
			return(bitmap_cache[array].hBitmap);
	}
	else if (0x2520 <= index && index <= 0x257f)	/* base 0x01b0 */
	{
		array = index - 0x2520 + 0x1b0;
		if (bitmap_cache[array].hBitmap)
			return(bitmap_cache[array].hBitmap);
	}
	else											/* base 0x0210 */
	{
		for (i = 0x210; i < sizeof(bitmap_cache) / sizeof(struct bcache); i++)
		{
			if (bitmap_cache[i].index == index)
			{
				if (bitmap_cache[i].hit < 0xf0000000)
					bitmap_cache[i].hit++;
				return(bitmap_cache[i].hBitmap);
			}
			if (bitmap_cache[i].index == 0)
			{
				array = i;
				break;
			}
			if (min > bitmap_cache[i].hit)
			{
				min = bitmap_cache[i].hit;
				array = i;
			}
		}
	}
	if (array < 0)
		array = 0x210;
	if (bitmap_cache[array].hBitmap)
		DeleteObject(bitmap_cache[array].hBitmap);
	hBMP = CreateBitmap(glyph->bbw, glyph->bbh, 1, 1, glyph->bitmap);
	bitmap_cache[array].index = index;
	bitmap_cache[array].hit   = 1;
	bitmap_cache[array].hBitmap = hBMP;
	return(hBMP);
}
#endif

static int 
search_file_line(char *key, char *start, int len, char **val, char **next)
{
	int linelen;
	unsigned char *p;

	p = memchr(start, '\n', len);
	if (!p)
		return -1;
	for (;start < p;start++)
	{
		if ((*start != ' ') || (*start != '\t'))
			break;
	}
	linelen = p - start + 1;
	*next = p + 1;
	if (strncmp(start, key, min(strlen(key), linelen)) == 0)
	{
		*val = start + strlen(key);
		return 1;
	}
	return 0;
}

static int
proceed_file_line(char *key, char *start, int *len, char **val, char **next)
{
	int flag = 0;

	do {
		flag = search_file_line(key, start, *len, val, next);
		*len -= (int)(*next - start);
		start = *next;
	} while(flag == 0);

	if (flag == -1)
		return 0;
	return 1;
}
   
static int
set_bdf_font_info(bdffont *fontp)
{
	unsigned char *start, *p, *q;
	int len, flag;
	int bbw, bbh, bbx, bby;
	int val1;

	len = fontp->size;
	start = fontp->font;

	fontp->yoffset = 0;
	fontp->relative_compose = 0;
	fontp->default_ascent = 0;

	flag = proceed_file_line("FONTBOUNDINGBOX", start, &len, &p, &q);
	if (!flag)
		return 0;
	bbw = strtol(p, &start, 10);
	p = start;
	bbh = strtol(p, &start, 10);
	p = start;
	bbx = strtol(p, &start, 10);
	p = start;
	bby = strtol(p, &start, 10);

	fontp->llx = bbx;
	fontp->lly = bby;
	fontp->urx = bbw + bbx;
	fontp->ury = bbh + bby;

	start = q;
	flag = proceed_file_line("STARTPROPERTIES", start, &len, &p, &q);
	if (!flag)
		return 1;

	do {
		start = q;
		if (search_file_line("PIXEL_SIZE", start, len, &p, &q) == 1)
		{
			val1 = atoi(p);
		}
		else if (search_file_line("FONT_ASCENT", start, len, &p, &q) == 1)
		{
			val1 = atoi(p);
			fontp->ury = val1;
		}
		else if (search_file_line("FONT_DESCENT", start, len, &p, &q) == 1)
		{
			val1 = atoi(p);
			fontp->lly = -val1;
		}
		else if (search_file_line("_MULE_BASELINE_OFFSET", start, len, &p, &q) == 1)
		{
			val1 = atoi(p);
			fontp->yoffset = -val1;
		}
		else if (search_file_line("_MULE_RELATIVE_COMPOSE", start, len, &p, &q) == 1)
		{
			val1 = atoi(p);
			fontp->relative_compose = val1;
		}
		else if (search_file_line("_MULE_DEFAULT_ASCENT", start, len, &p, &q) == 1)
		{
			val1 = atoi(p);
			fontp->default_ascent = val1;
		}
		else
		{
			flag = search_file_line("ENDPROPERTIES", start, len, &p, &q);
		}
		if (flag == -1)
			return 0;
		len -= (q - start);
	} while (flag == 0);
	start = q;
	flag = proceed_file_line("CHARS", start, &len, &p, &q);
	if (!flag)
		return 0;
	fontp->seeked = q;
	return 1;
}

bdffont*
mw32_init_bdf_font(char *filename)
{
	HANDLE hfile, hfilemap;
	bdffont *bdffontp;
	unsigned char *font;
	BY_HANDLE_FILE_INFORMATION fileinfo;
	int i;

	hfile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
									OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		return NULL;
	if (!GetFileInformationByHandle(hfile, &fileinfo) ||
							(fileinfo.nFileSizeHigh != 0) ||
							(fileinfo.nFileSizeLow > BDF_FILE_SIZE_MAX))
	{
		CloseHandle(hfile);
		return NULL;
	}
	hfilemap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hfilemap == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hfile);
		return NULL;
	}

	font = MapViewOfFile(hfilemap, FILE_MAP_READ, 0, 0, 0);
	if (!font)
	{
		CloseHandle(hfilemap);
		CloseHandle(hfile);
		return NULL;
	}

	bdffontp = (bdffont *) malloc(sizeof(bdffont));
	for(i = 0;i < BDF_FIRST_OFFSET_TABLE;i++)
		bdffontp->offset[i] = NULL;
	bdffontp->size = fileinfo.nFileSizeLow;
	bdffontp->font = font;
	bdffontp->hfile = hfile;
	bdffontp->hfilemap = hfilemap;
	bdffontp->filename = (char*) malloc(strlen(filename) + 1);
	strcpy(bdffontp->filename, filename);

	if (!set_bdf_font_info(bdffontp))
	{
		mw32_free_bdf_font(bdffontp);
		return NULL;
	}
	return bdffontp;
}

void
mw32_free_bdf_font(bdffont *fontp)
{
	int i;

	UnmapViewOfFile(fontp->font);
	CloseHandle(fontp->hfilemap);
	CloseHandle(fontp->hfile);
	free(fontp->filename);
	for(i = 0;i < BDF_FIRST_OFFSET_TABLE;i++)
	{
		if (fontp->offset[i])
			free(fontp->offset[i]);
	}
	free(fontp);
}

static unsigned char*
get_cached_char_offset(bdffont *fontp, int index)
{
	unsigned char **offset1;
	unsigned char *offset2;

	if (index > 0xffff)
		return NULL;

	offset1 = fontp->offset[BDF_FIRST_OFFSET(index)];
	if (!offset1)
		return NULL;
	offset2 = offset1[BDF_SECOND_OFFSET(index)];

	if (offset2)
		return offset2;

	return NULL;
}

static void
cache_char_offset(bdffont *fontp, int index, unsigned char *offset)
{
	unsigned char **offset1;

	if (index > 0xffff)
		return;

	offset1 = fontp->offset[BDF_FIRST_OFFSET(index)];
	if (!offset1)
	{
		offset1 = fontp->offset[BDF_FIRST_OFFSET(index)] =
				(unsigned char **) malloc(sizeof(unsigned char*) *
												BDF_SECOND_OFFSET_TABLE);
		memset(offset1, 0, sizeof(unsigned char*) * BDF_SECOND_OFFSET_TABLE);
	}
	offset1[BDF_SECOND_OFFSET(index)] = offset;

	return;
}

static unsigned char*
seek_char_offset(bdffont *fontp, int index)
{
	int len, flag, font_index;
	unsigned char *start, *p, *q;

	if (!fontp->seeked)
		return NULL;

	start = fontp->seeked;
	len = fontp->size - (start - fontp->font);

	do {
		flag = proceed_file_line("ENCODING", start, &len, &p, &q);
		if (!flag)
		{
			fontp->seeked = NULL;
			return NULL;
		}
		font_index = atoi(p);
		cache_char_offset(fontp, font_index, q);
		start = q;
	} while (font_index != index);
	fontp->seeked = q;

	return q;
}

#define GET_HEX_VAL(x) ((isdigit(x)) ? ((x) - '0') : \
			(((x) >= 'A') && ((x) <= 'Z')) ? ((x) - 'A' + 10) : \
			(((x) >= 'a') && ((x) <= 'z')) ? ((x) - 'a' + 10) : \
			(-1))

int
mw32_get_bdf_glyph(bdffont *fontp, int index, int size, glyph_struct *glyph)
{
	unsigned char *start, *p, *q, *bitmapp;
	unsigned char val1, val2;
	int i, j, len, flag;

	start = get_cached_char_offset(fontp, index);
	if (!start)
		start = seek_char_offset(fontp, index);
	if (!start)
		return 0;

	len = fontp->size - (start - fontp->font);

	flag = proceed_file_line("DWIDTH", start, &len, &p, &q);
	if (!flag)
		return 0;
	glyph->dwidth = atoi(p);

	start = q;
	flag = proceed_file_line("BBX", start, &len, &p, &q);
	if (!flag)
		return 0;
	glyph->bbw = strtol(p, &start, 10);
	p = start;
	glyph->bbh = strtol(p, &start, 10);
	p = start;
	glyph->bbox = strtol(p, &start, 10);
	p = start;
	glyph->bboy = strtol(p, &start, 10);

	if (size == 0)
		return 1;

	start = q;
	flag = proceed_file_line("BITMAP", start, &len, &p, &q);
	if (!flag)
		return 0;

	p = q;
	bitmapp = glyph->bitmap;
	for(i = 0;i < glyph->bbh;i++)
	{
		q = memchr(p, '\n', len);
		if (!q)
			return 0;
		for(j = 0;((q > p) && (j < ((glyph->bbw + 7) / 8 )));j++)
		{
			val1 = GET_HEX_VAL(*p);
			if (val1 == -1)
				return 0;
			p++;
			val2 = GET_HEX_VAL(*p);
			if (val2 == -1)
				return 0;
			p++;
			size--;
			if (size <= 0)
				return 0;
			/* NAND Operation.  */
			*bitmapp++ = (unsigned char)~((val1 << 4) | val2);
		}
		/* CreateBitmap requires WORD alignment.  */
		if (j % 2)
		{
			*bitmapp++ = 0xff;
		}
		p = q + 1;
	}

	return 1;
}

int
mw32_BDF_TextOut(HDC hdc, int left,
		 int top, unsigned char *text, int bytelen, int fixed_pitch_size,
		 int bitmap, int rev, DWORD fg, DWORD bg, int cspace, int lspace)
{
	int bitmap_size, index, btop;
	unsigned char *textp;
	glyph_struct glyph;
	HDC hCompatDC = 0;
	HBITMAP hBMP;
	HBRUSH hFgBrush, hOrgBrush;
	HANDLE holdobj, horgobj = 0;
	UINT textalign;
	int flag = 0;
	int kanji = 0;
	bdffont *fontp;
	int abitmap_size = 0, jbitmap_size = 0;

	if (g_jbdffontp != NULL)
	{
		fontp = g_jbdffontp;
		jbitmap_size = ((fontp->urx - fontp->llx) / 8 + 2) * (fontp->ury - fontp->lly) + 256;
	}
	if (g_bdffontp != NULL)
	{
		fontp = g_bdffontp;
		abitmap_size = ((fontp->urx - fontp->llx) / 8 + 2) * (fontp->ury - fontp->lly) + 256;
	}
	if (abitmap_size > jbitmap_size)
		bitmap_size = abitmap_size;
	else
		bitmap_size = jbitmap_size;
	glyph.bitmap = (unsigned char*) malloc(sizeof(unsigned char) * bitmap_size);

	hCompatDC = CreateCompatibleDC(hdc);

	textalign = GetTextAlign(hdc);

	SaveDC(hdc);

	hFgBrush = CreateSolidBrush(GetTextColor(hdc));
	hOrgBrush = SelectObject(hdc, hFgBrush);
	SetTextColor(hdc, bitmap && rev ? RGB(0, 0, 0) : fg);
	SetBkColor(hdc, bitmap ? RGB(255, 255, 255) : bg);

	textp = text;
	while(bytelen > 0)
	{
		if ((0x81 <= (unsigned char)(*textp) && (unsigned char)(*textp) <= 0x9F)
				|| (0xE0 <= (unsigned char)(*textp) && (unsigned char)(*textp) <= 0xFC))
		{
			bytelen -= 2;
			if (bytelen < 0)
				break;
			index = MAKELENDSHORT(textp[1], textp[0]);
			{
				unsigned char		high = textp[0];
				unsigned char		low = textp[1];
				if (high <= 0x9f)
					high -= 0x71;
				else
					high -= 0xb1;
				high = high * 2 + 1;
				if (low > 0x7f)
					low--;
				if (low >= 0x9e)
				{
					low -= 0x7d;
					high++;
				}
				else
				{
					low -= 0x1f;
				}
				index = (((unsigned int)high << 8) | (low & 0xff));
			}
			textp += 2;
		}
		else
		{
			index = *textp++;
			bytelen--;
		}
		if (index > 0xff)
		{
			fontp = g_jbdffontp;
			kanji = 1;
		}
		else
		{
			fontp = g_bdffontp;
			kanji = 0;
		}
		if (fontp == NULL)
		{
			if (fixed_pitch_size)
				left += fixed_pitch_size * (index > 0xff ? 2 : 1);
			else
				left += glyph.dwidth;
			continue;
		}
#if GLYPH_CACHE
		if (!get_cached_glyph(fontp, index, bitmap_size, &glyph))
#else
		if (!mw32_get_bdf_glyph(fontp, index, bitmap_size, &glyph))
#endif
		{
#if 1
			if (kanji)
				index = 0x2126;
			else
				index = 0x40;
			if (!mw32_get_bdf_glyph(fontp, index, bitmap_size, &glyph))
			{
				if (fixed_pitch_size)
					left += fixed_pitch_size * (index > 0xff ? 2 : 1);
				else
					left += glyph.dwidth;
				continue;
			}
#else
			if (horgobj)
			{
				SelectObject(hCompatDC, horgobj);
				DeleteObject(hBMP);
			}
			DeleteDC(hCompatDC);
			free(glyph.bitmap);
			return 0;
#endif
		}
#if BITMAP_CACHE
		hBMP = get_cached_bitmap(index, &glyph);
#else
		hBMP = CreateBitmap(glyph.bbw, glyph.bbh, 1, 1, glyph.bitmap);
#endif
		if (textalign & TA_BASELINE)
		{
			btop = top - (glyph.bbh + glyph.bboy);
		}
		else if (textalign & TA_BOTTOM)
		{
			btop = top - glyph.bbh;
		}
		else
		{
			btop = top;
		}

#if BITMAP_CACHE
		if (!horgobj)
		{
			horgobj = SelectObject(hCompatDC, hBMP);
		}
		else
		{
			SelectObject(hCompatDC, horgobj);
			SelectObject(hCompatDC, hBMP);
		}
#else
		if (horgobj)
		{
			SelectObject(hCompatDC, horgobj);
			DeleteObject(holdobj);
			SelectObject(hCompatDC, hBMP);
			holdobj = hBMP;
		}
		else
		{
			horgobj = SelectObject(hCompatDC, hBMP);
			holdobj = hBMP;
		}
#endif
		if (!bitmap)
		{
			if (g_fix || cspace || lspace)
			{
				HBRUSH		hbrush, holdbrush;
				RECT		rcWindow;

				hbrush	= CreateSolidBrush(bg);
				holdbrush = SelectObject(hdc, hbrush);
				rcWindow.left = left;
				rcWindow.top = btop;
				rcWindow.right = left + (g_xsize + cspace) * (index > 0xff ? 2 : 1);
				rcWindow.bottom = btop + g_ysize + lspace;
				FillRect(hdc, &rcWindow, hbrush);
				SelectObject(hdc, holdbrush);
				DeleteObject(hbrush);
			}
			BitBlt(hdc, left, btop, glyph.bbw, glyph.bbh, hCompatDC, 0, 0, SRCCOPY);
		}
#if 0
		else if (index == ' ' && compchr(textp, bytelen, ' '))
			BitBlt(hdc, left, btop, glyph.bbw, glyph.bbh, hCompatDC, 0, 0, SRCAND);
		else if (rev)
			BitBlt(hdc, left, btop, glyph.bbw, glyph.bbh, hCompatDC, 0, 0, SRCAND);
		else
			BitBlt(hdc, left, btop, glyph.bbw, glyph.bbh, hCompatDC, 0, 0, 0xB8074A);
#else
		else if (rev)
			BitBlt(hdc, left, btop, glyph.bbw, glyph.bbh, hCompatDC, 0, 0, 0xE20746);
		else
			BitBlt(hdc, left, btop, glyph.bbw, glyph.bbh, hCompatDC, 0, 0, 0x990066);
#endif
		if (fixed_pitch_size)
			left += fixed_pitch_size * (index > 0xff ? 2 : 1);
		else
			left += glyph.dwidth;
	}
	SelectObject(hCompatDC, horgobj);
	SelectObject(hdc, hOrgBrush);
	DeleteObject(hFgBrush);
#if !BITMAP_CACHE
	DeleteObject(hBMP);
#endif
	DeleteDC(hCompatDC);
	RestoreDC(hdc, -1);
	free(glyph.bitmap);

	return 1;
}

int
bdfTextOut(HDC hdc, int left, int top, UINT opt, CONST RECT *lprc, char *text, UINT size, CONST INT * lpdx, int bitmap, int rev, DWORD fg, DWORD bg, int cspace, int lspace)
{
	return mw32_BDF_TextOut(hdc, left, top, text, size, *lpdx, bitmap, rev, fg, bg, cspace, lspace);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static int
compchr(char *text, int len, char ch)
{
	int				i;

	for (i = 0; i < len; i++)
		if (text[i] != ch)
			return(FALSE);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
LRESULT PASCAL
BDFHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND			hwndBrush;
    static RECT			rt;
	char				NameBuff[MAX_PATH+2];
	LPOFNOTIFY			pofn;
    HDC					hdc;
	HBRUSH				hbrush;
	HBRUSH				holdbrush;
	bdffont			*	bdffontp;

	switch (uMsg) {
	case WM_INITDIALOG: 
		hwndBrush = GetDlgItem(hwnd, 1010); 
		GetClientRect(hwndBrush, &rt); 
		return(TRUE);
	case WM_NOTIFY:
		pofn = (LPOFNOTIFY)lParam;
		if (pofn->hdr.code == CDN_SELCHANGE)
		{
			if (CommDlg_OpenSave_GetSpec(GetParent(hwnd), NameBuff, MAX_PATH) <= MAX_PATH)
			{
				bdffontp = mw32_init_bdf_font(NameBuff);
				if (bdffontp != NULL)
				{
					hdc = GetDC(hwndBrush);
					hbrush	= CreateSolidBrush(RGB(185, 185, 185));
					holdbrush = SelectObject(hdc, hbrush);
					FillRect(hdc, &rt, hbrush);
					SelectObject(hdc, holdbrush);
					DeleteObject(hbrush);
					if (bJpn)
					{
						g_jbdffontp = bdffontp;
						mw32_BDF_TextOut(hdc, rt.left, rt.top,
								"‚ ‚ŸƒAƒ@ˆŸ‰F", 12, bdffontp->urx / 2,
								FALSE, FALSE, RGB(0, 0, 0), RGB(185, 185, 185), 0, 0);
						g_jbdffontp = NULL;
					}
					else
					{
						g_bdffontp = bdffontp;
						mw32_BDF_TextOut(hdc, rt.left, rt.top,
								"AaBbYyZz", 8, bdffontp->urx,
								FALSE, FALSE, RGB(0, 0, 0), RGB(185, 185, 185), 0, 0);
						g_bdffontp = NULL;
					}
					ReleaseDC(hwndBrush, hdc);
					mw32_free_bdf_font(bdffontp);
				}
			}
		}
		break;
	}
	return(FALSE);
}

/*------------------------------------------------------------------------------
 *	login dialog
 *----------------------------------------------------------------------------*/
static LRESULT CALLBACK
DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OSVERSIONINFO	ver_info;
    int					wmId;
	OPENFILENAME		ofn;
	char				NameBuff[MAX_PATH+2];
	char				IObuff[MAX_PATH+2];
	static DWORD		bUse;
	static DWORD	*	config;
	char			*	p;
	char			*	q;
	extern char		*	gettail();

	switch (uMsg) {
	case WM_INITDIALOG:
		ver_info.dwOSVersionInfoSize = sizeof(ver_info);
		GetVersionEx(&ver_info);
		if (strlen(g_bdffile))
			SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)g_bdffile);
		if (strlen(g_jbdffile))
			SendDlgItemMessage(hWnd, 3000, WM_SETTEXT, 0, (LPARAM)g_jbdffile);
		config = (DWORD *)lParam;
		bUse = *config;
		if (bUse)
			CheckDlgButton(hWnd, 1002, MF_CHECKED);
		else
			CheckDlgButton(hWnd, 1002, MF_UNCHECKED);
		g_bdffontp = mw32_init_bdf_font(g_bdffile);
		if (g_bdffontp == NULL)
		{
			SendDlgItemMessage(hWnd, 2000, WM_SETTEXT, 0, (LPARAM)"No font name");
			SendDlgItemMessage(hWnd, 2001, WM_SETTEXT, 0, (LPARAM)"0");
			SendDlgItemMessage(hWnd, 2002, WM_SETTEXT, 0, (LPARAM)"0");
		}
		else
		{
			p = g_bdffontp->font;
			q = NameBuff;
			while (*p)
			{
				if (*p == 0x0a)
				{
					*q = '\0';
					break;
				}
				else
					*q = *p;
				p++;
				q++;
			}
			SendDlgItemMessage(hWnd, 2000, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", g_bdffontp->urx);
			SendDlgItemMessage(hWnd, 2001, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", g_bdffontp->ury - g_bdffontp->lly);
			SendDlgItemMessage(hWnd, 2002, WM_SETTEXT, 0, (LPARAM)NameBuff);
			mw32_free_bdf_font(g_bdffontp);
			g_bdffontp = NULL;
		}
		g_jbdffontp = mw32_init_bdf_font(g_jbdffile);
		if (g_jbdffontp == NULL)
		{
			SendDlgItemMessage(hWnd, 4000, WM_SETTEXT, 0, (LPARAM)"No font name");
			SendDlgItemMessage(hWnd, 4001, WM_SETTEXT, 0, (LPARAM)"0");
			SendDlgItemMessage(hWnd, 4002, WM_SETTEXT, 0, (LPARAM)"0");
		}
		else
		{
			p = g_jbdffontp->font;
			q = NameBuff;
			while (*p)
			{
				if (*p == 0x0a)
				{
					*q = '\0';
					break;
				}
				else
					*q = *p;
				p++;
				q++;
			}
			SendDlgItemMessage(hWnd, 4000, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", g_jbdffontp->urx);
			SendDlgItemMessage(hWnd, 4001, WM_SETTEXT, 0, (LPARAM)NameBuff);
			wsprintf(NameBuff, "%d", g_jbdffontp->ury - g_jbdffontp->lly);
			SendDlgItemMessage(hWnd, 4002, WM_SETTEXT, 0, (LPARAM)NameBuff);
			mw32_free_bdf_font(g_jbdffontp);
			g_jbdffontp = NULL;
		}
		return(TRUE);
	case WM_DESTROY:
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId) {
		case IDOK:
			GetDlgItemText(hWnd, 1000, g_bdffile, MAX_PATH+2);
			GetDlgItemText(hWnd, 3000, g_jbdffile, MAX_PATH+2);
			*config = bUse;
			if (bUse)
				EndDialog(hWnd, 0);
			else
				EndDialog(hWnd, 1);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, 1);
			return(TRUE);
		case 1001:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			GetDlgItemText(hWnd, 1000, IObuff, MAX_PATH);
			*gettail(IObuff) = '\0';
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
			ofn.lpstrFilter		= "BDF File(*.bdf)\0*.bdf\0ALL(*.*)\0*.*\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAX_PATH;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= "ASCII Font Select";
			ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			ofn.lCustData		= FALSE;
			if (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
											&& ver_info.dwMajorVersion == 3))
			{
				ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
				ofn.lpfnHook		= BDFHookProc;
				ofn.lpTemplateName	= "BDFDLGEXP";
				bJpn				= FALSE;
			}
			if (GetOpenFileName(&ofn))
			{
				SendDlgItemMessage(hWnd, 1000, EM_SETSEL, 0, (LPARAM)-2);
				SendDlgItemMessage(hWnd, 1000, WM_SETTEXT, 0, (LPARAM)NameBuff);
				g_bdffontp = mw32_init_bdf_font(NameBuff);
				if (g_bdffontp != NULL)
				{
					p = g_bdffontp->font;
					q = NameBuff;
					while (*p)
					{
						if (*p == 0x0a)
						{
							*q = '\0';
							break;
						}
						else
							*q = *p;
						p++;
						q++;
					}
					SendDlgItemMessage(hWnd, 2000, WM_SETTEXT, 0, (LPARAM)NameBuff);
					wsprintf(NameBuff, "%d", g_bdffontp->urx);
					SendDlgItemMessage(hWnd, 2001, WM_SETTEXT, 0, (LPARAM)NameBuff);
					wsprintf(NameBuff, "%d", g_bdffontp->ury - g_bdffontp->lly);
					SendDlgItemMessage(hWnd, 2002, WM_SETTEXT, 0, (LPARAM)NameBuff);
					mw32_free_bdf_font(g_bdffontp);
					g_bdffontp = NULL;
				}
			}
			break;
		case 3001:
			memset(&ofn, 0, sizeof(ofn));
			NameBuff[0] = '\0';
			GetDlgItemText(hWnd, 3000, IObuff, MAX_PATH);
			*gettail(IObuff) = '\0';
			ofn.lStructSize		= sizeof(ofn);
			ofn.hwndOwner		= hWnd;
			ofn.hInstance		= (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
			ofn.lpstrFilter		= "BDF File(*.bdf)\0*.bdf\0ALL(*.*)\0*.*\0";
			ofn.lpstrCustomFilter = (LPSTR)NULL;
			ofn.nMaxCustFilter	= 0L;
			ofn.nFilterIndex	= 1;
			ofn.lpstrFile		= NameBuff;
			ofn.nMaxFile		= MAX_PATH;
			ofn.lpstrFileTitle	= NULL;
			ofn.nMaxFileTitle	= 0;
			ofn.lpstrInitialDir	= IObuff;
			ofn.lpstrTitle		= "Japanese Font Select";
			ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER;
			ofn.nFileOffset		= 0;
			ofn.nFileExtension	= 0;
			ofn.lpstrDefExt		= NULL;
			ofn.lCustData		= TRUE;
			if (!(ver_info.dwPlatformId == VER_PLATFORM_WIN32_NT
											&& ver_info.dwMajorVersion == 3))
			{
				ofn.Flags			= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
				ofn.lpfnHook		= BDFHookProc;
				ofn.lpTemplateName	= "BDFDLGEXP";
				bJpn				= TRUE;
			}
			if (GetOpenFileName(&ofn))
			{
				SendDlgItemMessage(hWnd, 3000, EM_SETSEL, 0, (LPARAM)-2);
				SendDlgItemMessage(hWnd, 3000, WM_SETTEXT, 0, (LPARAM)NameBuff);
				g_jbdffontp = mw32_init_bdf_font(NameBuff);
				if (g_jbdffontp != NULL)
				{
					p = g_jbdffontp->font;
					q = NameBuff;
					while (*p)
					{
						if (*p == 0x0a)
						{
							*q = '\0';
							break;
						}
						else
							*q = *p;
						p++;
						q++;
					}
					SendDlgItemMessage(hWnd, 4000, WM_SETTEXT, 0, (LPARAM)NameBuff);
					wsprintf(NameBuff, "%d", g_jbdffontp->urx);
					SendDlgItemMessage(hWnd, 4001, WM_SETTEXT, 0, (LPARAM)NameBuff);
					wsprintf(NameBuff, "%d", g_jbdffontp->ury - g_jbdffontp->lly);
					SendDlgItemMessage(hWnd, 4002, WM_SETTEXT, 0, (LPARAM)NameBuff);
					mw32_free_bdf_font(g_jbdffontp);
					g_jbdffontp = NULL;
				}
			}
			break;
		case 1002:
			if (bUse)
			{
				bUse = FALSE;
				CheckDlgButton(hWnd, 1002, MF_UNCHECKED);
			}
			else
			{
				bUse = TRUE;
				CheckDlgButton(hWnd, 1002, MF_CHECKED);
			}
			break;
		}
		break;
	}
	return(FALSE);
}

void
GetBDFfont(HINSTANCE hInst, int mode, char *fname, char *jfname, int *xsize, int *ysize, DWORD *config)
{
	int			ax = 0;
	int			ay = 0;
	int			jx = 0;
	int			jy = 0;

	g_bdffile  = fname;
	g_jbdffile = jfname;
	if (g_bdffontp != NULL)
		mw32_free_bdf_font(g_bdffontp);
	if (g_jbdffontp != NULL)
		mw32_free_bdf_font(g_jbdffontp);
	g_bdffontp  = NULL;
	g_jbdffontp = NULL;
	if (mode != 0)
	{
		if (DialogBoxParam(hInst, "BDF", NULL, (DLGPROC)DialogProc, (LPARAM)config) != 0 && !(*config))
			return; 
	}
#if GLYPH_CACHE
	init_cached_glyph();
#endif
#if BITMAP_CACHE
	init_cached_bitmap();
#endif
	/* check */
	g_bdffontp  = mw32_init_bdf_font(g_bdffile);
	g_jbdffontp = mw32_init_bdf_font(g_jbdffile);
	if (g_bdffontp == NULL && g_jbdffontp == NULL)
	{
		*config = FALSE;
		return; 
	}
	if (g_bdffontp != NULL)
	{
		ax = g_bdffontp->urx;
		ay = g_bdffontp->ury - g_bdffontp->lly;
	}
	if (g_jbdffontp != NULL)
	{
		jx = g_jbdffontp->urx;
		jy = g_jbdffontp->ury - g_jbdffontp->lly;
	}
	if ((ax * 2) >= jx)
		*xsize = ax;
	else
		*xsize = jx / 2;
	g_fix = (ax * 2) == jx ? 1 : 0;
	if (ay > jy)
		*ysize = ay;
	else
		*ysize = jy;
	if (!g_fix)
		g_fix = ay == jy ? 1 : 0;
	g_xsize = *xsize;
	g_ysize = *ysize;
	return;
}
