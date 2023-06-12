/*****************************************
  Meadow BDF font manager
         Written by H.Miyashita.
******************************************/
#define BDF_FIRST_OFFSET_TABLE 0x200
#define BDF_SECOND_OFFSET_TABLE 0x80
#define BDF_SECOND_OFFSET(x) ((x) & 0x7f)
#define BDF_FIRST_OFFSET(x) (((x) >> 8) | (((x) & 0x80) << 1))

/* Structure of glyph information of one character.  */
typedef struct
{
  int dwidth;			/* width in pixels */
  int bbw, bbh, bbox, bboy;	/* bounding box in pixels */
  int bitmap_size;		/* byte lengh of the following slots */
  unsigned char *bitmap;	/*  */
} glyph_struct;

typedef struct
{
  char *filename;
  HANDLE hfile;
  HANDLE hfilemap;
  unsigned char *font;
  unsigned char *seeked;
  DWORD size;
  unsigned char **offset[BDF_FIRST_OFFSET_TABLE];
  int llx, lly, urx, ury;	/* Font bounding box */

  int yoffset;
  int relative_compose;
  int default_ascent;

}bdffont;

#define BDF_FILE_SIZE_MAX 256*1024*1024 /* 256Mb */
#define BDF_FONT_FILE(font) (((bdffont*)(font))->filename)
#define MAKELENDSHORT(c1, c2) (unsigned short)((c1) | ((c2) << 8))

extern bdffont *mw32_init_bdf_font(char *filename);
extern void mw32_free_bdf_font(bdffont *fontp);
extern int mw32_get_bdf_glyph(bdffont *fontp, int index, int size,
			      glyph_struct *glyph);
extern int mw32_BDF_TextOut(HDC hdc, int left,
			    int top, unsigned char *text, int bytelen, int fixed_pitch_size, int bitmap, int reverse, DWORD fg, DWORD bg, int cspace, int lspace);
