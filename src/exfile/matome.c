/*==============================================================================
 *
 *============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "vim.h"
#include "globals.h"
#include "proto.h"
#include "param.h"
#include "fcntl.h"

#define	DEC(c)		(((c) - ' ') & 077)
#define BASE64		1
#define UUENCODE	2
#define QPRINT		3

static char *mime_type[]
		= {"=?ISO-8859-1?Q?", "=?ISO-2022-JP?B?", "=?shift_jis?B?",
														"=?shift-jis?B?", NULL};
static char boundary[80];

/*------------------------------------------------------------------------------
 *	uuencode decode routine
 *----------------------------------------------------------------------------*/
static BOOL
uudc(oflg, fp, line1, line2)
BOOL			oflg;
FILE		*	fp;
linenr_t	*	line1;
linenr_t	*	line2;
{
	linenr_t 		lnum;
	int				n;
	int				len;
	char		*	p;

	for (lnum = *line1; lnum <= *line2; ++lnum)
	{
		p = ml_get_buf(curbuf, lnum, FALSE);
		len = strlen(p);
		if (len == 0)
			break;
		n = DEC(*p);
		if (n <= 0)
			break;
		for (++p; n > 0 && len > 1; len -= 4, p += 4, n -= 3)
		{
			char		ch;

			if (n >= 3)
			{
				ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
				fputc(ch, fp);
				ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
				fputc(ch, fp);
				ch = DEC(p[2]) << 6 | DEC(p[3]);
				fputc(ch, fp);
			}
			else
			{
				if (n >= 1)
				{
					ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
					fputc(ch, fp);
				}
				if (n >= 2)
				{
					ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
					fputc(ch, fp);
				}
			}
		}
	}
	if (lnum >= curbuf->b_ml.ml_line_count)
	{
		if (!oflg)
		{
			emsg("No begin/end line");
			wait_return(FALSE);
		}
		return(FALSE);
	}
	p = ml_get_buf(curbuf, lnum + 1, FALSE);
	if (strcmp(p, "end"))
	{
		if (!strchr(p_dc, 'a'))
		{
			*line2 = lnum;
			return(TRUE);
		}
		if (!oflg)
		{
			emsg("No begin/end line");
			wait_return(FALSE);
		}
		return(FALSE);
	}
	*line2 = lnum;
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	base 64 encode routine
 *----------------------------------------------------------------------------*/
static char *
uuec(fname, crlf, ptr, len)
char		*	fname;
char		*	crlf;
unsigned char *	ptr;
long			len;
{
	static char	szUUencode[] =
		"`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`";
	DWORD			no;
	INT				i	= 0;
	INT				pos = 0;
	BYTE		*	encode;
	BYTE		*	p = ptr;
	int				size = 0;

	size = ((len + 2) / 3) * 4 + 1;
	size += ((size / 60) + 1) * (1 + strlen(crlf));
	size += strlen("Content-Type: application/octet-stream; name=\"\"");
	size += strlen(fname);
	size += strlen(crlf);
	size += strlen("Content-Transfer-Encoding: X-uuencode");
	size += strlen(crlf) * 2;
	size += strlen("begin 644 ");
	size += strlen(fname);
	size += strlen(crlf);
	size += strlen("end");
	if ((encode = (BYTE *)malloc(size + 16)) == NULL)
		return(NULL);
	pos += sprintf(&encode[pos],
			"Content-Type: application/octet-stream; name=\"%s\"", fname);
	pos += sprintf(&encode[pos], crlf);
	pos += sprintf(&encode[pos], "Content-Transfer-Encoding: X-uuencode");
	pos += sprintf(&encode[pos], "%s%s", crlf, crlf);
	pos += sprintf(&encode[pos], "begin 644 %s%s", fname, crlf);

	while (len > 0)
	{
		if ((i % 15) == 0)
		{
			if (len > 45)
				encode[pos++] = szUUencode[45];
			else
				encode[pos++] = szUUencode[len];
		}
		no  = ((DWORD)p[(i * 3) + 0] << 16) & 0x00ff0000;
		if (len > 1)
			no |= ((DWORD)p[(i * 3) + 1] <<  8) & 0x0000ff00;
		if (len > 2)
			no |= ((DWORD)p[(i * 3) + 2] <<  0) & 0x000000ff;
		encode[pos++] = szUUencode[((no & 0x00fc0000) >> 18)];
		encode[pos++] = szUUencode[((no & 0x0003f000) >> 12)];
		if (len > 1)
			encode[pos++] = szUUencode[((no & 0x00000fc0) >>  6)];
		if (len > 2)
			encode[pos++] = szUUencode[  no & 0x0000003f];
		if (((i != 0) && ((i % 15) == 14)) || len <= 3)
			pos += sprintf(&encode[pos], "%s", crlf);
		i++;
		len -= 3;
	}
	encode[pos++] = szUUencode[0];
	sprintf(&encode[pos], "%send", crlf);
	return(encode);
}

/*------------------------------------------------------------------------------
 *	base 64 decode routine
 *----------------------------------------------------------------------------*/
static BOOL
b64dc(oflg, fp, line1, line2, mime)
BOOL			oflg;
FILE		*	fp;
linenr_t	*	line1;
linenr_t	*	line2;
int				mime;
{
	static char	szB64decode[] = {
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 64, -1, -1,
	 -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	 -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, };
	linenr_t 			lnum;
	int					fill;
	int					len;
	int					i;
	int					j;
	int					bytes;
	unsigned char		decode[3];
	unsigned char		work[4];

	if (mime != (-1))
	{
		unsigned char	*p;
		unsigned char	*s;
		unsigned char	*e;

		p = ml_get_buf(curbuf, *line1, FALSE);
		s = strstr(p, "=?");
		e = strstr(p, "?=");
		for (; p < s; p++)
			fputc(*p, fp);
		for (p = s + strlen(mime_type[mime]); p < e; p += 4)
		{
			work[0] = szB64decode[p[0]];
			work[1] = szB64decode[p[1]];
			work[2] = szB64decode[p[2]];
			work[3] = szB64decode[p[3]];
			decode[0] = work[0] << 2 | work[1] >> 4;
			decode[1] = work[1] << 4 | work[2] >> 2;
			decode[2] = work[2] << 6 | work[3];
			bytes = (p[3] == '=') ? (p[2] == '=' ? 1 : 2) : 3;
			for (j = 0; j < bytes; j++)
				fputc(decode[j], fp);
			if (bytes != 3)
				break;
		}
		for (p = e + 2; *p; p++)
			fputc(*p, fp);
		return(TRUE);
	}
	for (lnum = *line1; lnum <= *line2; ++lnum)
	{
		int last_data = 0;
		unsigned char *p;

		if (lnum > curbuf->b_ml.ml_line_count)
		{
			if (!oflg)
			{
				emsg("Short file");
				wait_return(FALSE);
			}
			return(FALSE);
		}
		p = ml_get_buf(curbuf, lnum, FALSE);

		if (*p == '\0')
			break;
		if (boundary[0] != '\0' && strstr(p, boundary) != NULL)
		{
			lnum--;
			break;
		}

		p = ml_get_buf(curbuf, lnum, FALSE);
		len = strlen(p);
		if (lnum == *line1)
			fill = len;
		if (((len % 4) != 0) || (memcmp(p, "====", 4) == 0))
			break;

		for (i = 0; i < len; i += 4)
		{
			work[0] = szB64decode[p[i + 0]];
			work[1] = szB64decode[p[i + 1]];
			work[2] = szB64decode[p[i + 2]];
			work[3] = szB64decode[p[i + 3]];
			decode[0] = work[0] << 2 | work[1] >> 4;
			decode[1] = work[1] << 4 | work[2] >> 2;
			decode[2] = work[2] << 6 | work[3];
			bytes = (p[i + 3] == '=') ? (p[i + 2] == '=' ? 1 : 2) : 3;
			for (j = 0; j < bytes; j++)
				fputc(decode[j], fp);
			if (bytes != 3)
			{
				last_data = 1;
				break;
			}
		}
		if ((last_data != 0) || (len < fill))
			break;
	}
	if (lnum <= *line2)
		*line2 = lnum;
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	base 64 encode routine
 *----------------------------------------------------------------------------*/
static char *
b64ec(fname, crlf, ptr, len)
char		*	fname;
char		*	crlf;
unsigned char *	ptr;
long			len;
{
	static char	szB64encode[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	DWORD			no;
	INT				loop;
	INT				mod;
	INT				i;
	INT				pos = 0;
	BYTE		*	encode;
	BYTE		*	p = ptr;
	int				size = 0;

	size = ((len + 2) / 3) * 4 + 1;
	size += ((size / 72) + 1) * 2;
	size += strlen("Content-Type: application/octet-stream; name=\"\"");
	size += strlen(fname);
	size += strlen(crlf);
	size += strlen("Content-Transfer-Encoding: base64");
	size += strlen(crlf) * 2;
	if ((encode = (BYTE *)malloc(size + 16)) == NULL)
		return(NULL);
	pos += sprintf(&encode[pos],
			"Content-Type: application/octet-stream; name=\"%s\"", fname);
	pos += sprintf(&encode[pos], crlf);
	pos += sprintf(&encode[pos], "Content-Transfer-Encoding: base64");
	pos += sprintf(&encode[pos], "%s%s", crlf, crlf);

	loop = len / 3;
	mod  = len % 3;
	for (i = 0; i < loop; i++, pos += 4)
	{
		no  = ((DWORD)p[(i * 3) + 0] << 16) & 0x00ff0000;
		no |= ((DWORD)p[(i * 3) + 1] <<  8) & 0x0000ff00;
		no |= ((DWORD)p[(i * 3) + 2] <<  0) & 0x000000ff;
		encode[pos + 0] = szB64encode[((no & 0x00fc0000) >> 18)];
		encode[pos + 1] = szB64encode[((no & 0x0003f000) >> 12)];
		encode[pos + 2] = szB64encode[((no & 0x00000fc0) >>  6)];
		encode[pos + 3] = szB64encode[  no & 0x0000003f];
		if ((i != 0) && ((i % 18) == 17))
			pos += sprintf(&encode[pos + 4], "%s", crlf);
	}
	switch (mod) {
	case 1:
		no  = ((DWORD)p[(loop * 3) + 0] << 16) & 0x00ff0000;
		encode[pos + 0] = szB64encode[((no & 0x00fc0000) >> 18)];
		encode[pos + 1] = szB64encode[((no & 0x0003f000) >> 12)];
		encode[pos + 2] = '=';
		encode[pos + 3] = '=';
		sprintf(&encode[pos + 4], "%s", crlf);
		break;
	case 2:
		no  = ((DWORD)p[(loop * 3) + 0] << 16) & 0x00ff0000;
		no |= ((DWORD)p[(loop * 3) + 1] <<  8) & 0x0000ff00;
		encode[pos + 0] = szB64encode[((no & 0x00fc0000) >> 18)];
		encode[pos + 1] = szB64encode[((no & 0x0003f000) >> 12)];
		encode[pos + 2] = szB64encode[((no & 0x00000fc0) >>  6)];
		encode[pos + 3] = '=';
		sprintf(&encode[pos + 4], "%s", crlf);
		break;
	default:
		sprintf(&encode[pos], "%s", crlf);
		break;
	}
	return(encode);
}

/*------------------------------------------------------------------------------
 *	quoted printable decode routine
 *----------------------------------------------------------------------------*/
static BOOL
qprint(fp, line1, line2)
FILE		*	fp;
linenr_t	*	line1;
linenr_t	*	line2;
{
	linenr_t 		lnum;
	BOOL			crlf;

	for (lnum = *line1; lnum <= *line2; ++lnum)
	{
		unsigned char *p;

		if (lnum > curbuf->b_ml.ml_line_count)
		{
			emsg("Short file");
			wait_return(FALSE);
			return(FALSE);
		}
		p = ml_get_buf(curbuf, lnum, FALSE);

		if (boundary[0] != '\0' && strstr(p, boundary) != NULL)
		{
			lnum--;
			break;
		}

		crlf = TRUE;
		while (*p != '\0')
		{
			char c1, c2;

			if (*p == '=')
			{
				if (isxdigit(p[1]) && isxdigit(p[2]))
				{
					c1 = isdigit(p[1]) ? p[1] - '0' : tolower(p[1]) - 'a' + 10;
					c2 = isdigit(p[2]) ? p[2] - '0' : tolower(p[2]) - 'a' + 10;
					fputc(c1 << 4 | c2, fp);
					p += 3;
				}
				else if (p[1] == '\0')
				{
					crlf = FALSE;
					p++;
				}
				else
					fputc(*p++, fp);
			}
			else
				fputc(*p++, fp);
		}
		if (crlf)
		{
			fputc('\r', fp);
			fputc('\n', fp);
		}
	}
	if (lnum <= *line2)
		*line2 = lnum;
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	uuencode + gzip file decode routine
 *----------------------------------------------------------------------------*/
static BOOL
decode_part(oflg, line1, line2, linecount)
BOOL			oflg;
linenr_t	*	line1;
linenr_t	*	line2;
linenr_t 	*	linecount;
{
	linenr_t 		lnum;
	int				n;
	char		*	p;
	char		*	t;
	char			temp[MAX_PATH];
	int				encode = 0;
	FILE		*	fp;
	BOOL			output = FALSE;
	int				mime = -1;
#ifdef USE_EXFILE
	char			multi[MAX_PATH];
	BOOL			compress = FALSE;
#endif

	memset(IObuff, '\0', IOSIZE);
	temp[0] = '\0';
	if ((p = getenv("TMP")) == NULL)
		p = getenv("TEMP");
	if (p != NULL)
	{
		strcpy(temp, p);
		n = strlen(temp) - 1;
		if (temp[n] == '\\' || temp[n] == '/')
			;
		else
			strcat(temp, "\\");
	}
	strcat(temp, "efXXXXXX");
#ifdef USE_EXFILE
	strcpy(multi, temp);
	if (mktemp(multi) == NULL)
	{
		if (!oflg)
		{
			emsg("temporary name make error.");
			wait_return(FALSE);
		}
		return(FALSE);
	}
#endif
	if (mktemp(temp) == NULL)
	{
		if (!oflg)
		{
			emsg("temporary name make error.");
			wait_return(FALSE);
		}
		return(FALSE);
	}
	if (strchr(p_dc, 'a'))
	{
		for (lnum = *line1; lnum <= *line2; ++lnum)
		{
			p = ml_get_buf(curbuf, lnum, FALSE);
			if (strstr(p, "=?") && strstr(p, "?="))
			{
				int			i;

				for (i = 0; mime_type[i] != NULL; i++)
				{
					if (vim_strnicmp(mime_type[i], strstr(p, "=?"),
													strlen(mime_type[i])) == 0)
					{
						mime = i;
						encode = BASE64;
						*line1 = *line2 = lnum;
						break;
					}
				}
				if (mime != (-1))
					break;
			}
			else if (strncmp(p, "begin", 5) == 0)
			{
				int			mode;

				if (sscanf(p, "begin-base64 %o %s", &mode, IObuff) == 2)
				{
					encode = BASE64;
					*line1 = lnum + 1;
					break;
				}
				else if (sscanf(p, "begin %o %s", &mode, IObuff) == 2)
				{
					encode = UUENCODE;
					*line1 = lnum + 1;
					break;
				}
			}
			else if (strncmp(p, "Content-", 8) == 0)
			{
				for (; lnum <= *line2; ++lnum)
				{
					p = ml_get_buf(curbuf, lnum, FALSE);
					if (*p == '\0')
					{
						if (encode == 0 || encode == UUENCODE)
							;
						else
						{
							*line1 = lnum + 1;
							for (; lnum <= *line2; ++lnum)
							{
								p = ml_get_buf(curbuf, lnum, FALSE);
								if (*p != '\0')
								{
									*line1 = lnum;
									break;
								}
							}
							break;
						}
					}
					else if (encode == 0 && strstr(p, "=?") && strstr(p, "?="))
					{
						int			i;

						for (i = 0; mime_type[i] != NULL; i++)
						{
							if (vim_strnicmp(mime_type[i], strstr(p, "=?"),
													strlen(mime_type[i])) == 0)
							{
								mime = i;
								encode = BASE64;
								*line1 = *line2 = lnum;
								break;
							}
						}
						if (mime != (-1))
							break;
					}
					else if (encode == UUENCODE && strncmp(p, "begin", 5) == 0)
					{
						int			mode;

						if (sscanf(p, "begin %o %s", &mode, IObuff) == 2)
						{
							*line1 = lnum + 1;
							break;
						}
					}
					else if (strncmp(p, "Content-Transfer-Encoding:", 26) == 0)
					{
						if (stricmp(&p[strlen(p) - 6], "base64") == 0)
							encode = BASE64;
						else if (stricmp(&p[strlen(p) - 10], "X-uuencode") == 0)
							encode = UUENCODE;
						else if (stricmp(&p[strlen(p) - 16], "quoted-printable") == 0)
							encode = QPRINT;
					}
					else if (strstr(p, "multipart") && strstr(p, "boundary="))
						strncpy(boundary, strstr(p, "boundary=") + 10,
										strlen(strstr(p, "boundary=")) - 11);
					else if ((*p == ' ' || *p == '\t') && strstr(p, "boundary="))
						strncpy(boundary, strstr(p, "boundary=") + 10,
										strlen(strstr(p, "boundary=")) - 11);
					else if (IObuff[0] == '\0')
					{
						if (sscanf(p, "\tname=\"%s", IObuff) == 1)
							IObuff[strlen(IObuff) - 1] = '\0';
						else if (sscanf(p, "\tfilename=\"%s", IObuff) == 1)
							IObuff[strlen(IObuff) - 1] = '\0';
						else if (sscanf(p, "Content-Disposition: attachment; filename=\"%s", IObuff) == 1)
							IObuff[strlen(IObuff) - 1] = '\0';
						else if (sscanf(p, "Content-Type: application/octet-stream; name=\"%s", IObuff) == 1)
							IObuff[strlen(IObuff) - 1] = '\0';
					}
					if ((lnum + 1) <= *line2)
					{
						p = ml_get_buf(curbuf, lnum + 1, FALSE);
						if (!(*p == ' ' || *p == '\t' || *p == '\0'))
							goto BreakBreak;
					}
				}
				if (lnum > *line2)
					encode = 0;
				break;
			}
BreakBreak:
			;
		}
#ifdef USE_EXFILE
		{
			static char	*	ext[]	= {".lzh", ".gz", ".tar", ".taZ", ".taz", ".tar.gz", "tar.Z", ".tar.bz2", ".bz2", ".cab", ".zip", NULL};
			const char	*	p;
			char		**	ep		= ext;

			while (*ep && compress == FALSE)
			{
				p = IObuff + 1;
				while (*p)
				{
					if ((strlen(p) >= strlen(*ep))
							&& strnicmp(p, *ep, strlen(*ep)) == 0
							&& p[strlen(*ep)] == '\0')
					{
						compress = TRUE;
						strcat(temp, *ep);
						break;
					}
					p++;
				}
				ep++;
			}
		}
#endif
	}
	else
	{
		t = p_dc;
		while (*t)
		{
			switch (*t) {
			case 'b':
				encode = BASE64;
				break;
			case 'm':
				for (lnum = *line1; lnum <= *line2; ++lnum)
				{
					p = ml_get_buf(curbuf, lnum, FALSE);
					if (strstr(p, "=?") && strstr(p, "?="))
					{
						int			i;

						for (i = 0; mime_type[i] != NULL; i++)
						{
							if (vim_strnicmp(mime_type[i], strstr(p, "=?"),
															strlen(mime_type[i])) == 0)
							{
								mime = i;
								encode = BASE64;
								*line1 = *line2 = lnum;
								break;
							}
						}
						if (mime != (-1))
							break;
					}
				}
				break;
#ifdef USE_EXFILE
			case 'g':
				compress = TRUE;
				strcat(temp, ".gz");
				break;
			case 'l':
				compress = TRUE;
				strcat(temp, ".lzh");
				break;
			case 'c':
				compress = TRUE;
				strcat(temp, ".cab");
				break;
			case 'z':
				compress = TRUE;
				strcat(temp, ".zip");
				break;
#endif
			case 'u':
				encode = UUENCODE;
				break;
			case 'q':
				encode = QPRINT;
				break;
			case 'r':
				/* auto read */
				break;
			}
			t++;
		}
	}
	if (strchr(p_dc, '+'))
		output = TRUE;
	if (strchr(p_dc, '*'))
	{
		output = TRUE;
		strcat(temp, ".dmp");
	}
	if (encode == 0)
		return(FALSE);

	if ((fp = fopen(temp, "wb")) == NULL)
	{
		if (!oflg)
		{
			emsg("temporary file open error.");
			wait_return(FALSE);
		}
		return(FALSE);
	}
	switch (encode) {
	case BASE64:
		n = b64dc(oflg, fp, line1, line2, mime);
		break;
	case QPRINT:
		n = qprint(fp, line1, line2);
		break;
	default:
		n = uudc(oflg, fp, line1, line2);
		break;
	}
	fclose(fp);
	if (n != TRUE)
	{
		remove(temp);
		return(FALSE);
	}
	if (output)
	{
		if (!oflg)
			smsg((char_u *)"Output decoding file <%s>.",(long)temp);
		return(TRUE);
	}
#ifdef USE_EXFILE
	if (compress)
	{
		char		**	result;
		int				j;
		int				fd;
		int				bfd;
		int				size;
		int				files;
		char			gzip[MAX_PATH];

		if (is_efarc(temp, '\0'))
		{
			strcpy(gzip, temp);
			strcat(gzip, ":*");
			if ((bfd = open(multi, O_WRONLY | O_CREAT, 0666)) >= 0)
			{
				result = ef_globfilename(gzip, FALSE);
				for (j = 0; result[j] != NULL; j++)
					;
				files = j;
				for (j = 0; result[j] != NULL; j++)
				{
					if ((fd = ef_open(result[j], O_RDONLY)) >= 0)
					{
						if (files > 1)
						{
							sprintf(gzip, "[%d: %s]\r\n",
										j + 1, &result[j][strlen(temp) + 1]);
							write(bfd, gzip, (size_t)strlen(gzip));
						}
						while ((size = read(fd, (char *)IObuff, (size_t)IOSIZE)) > 0)
							write(bfd, (char *)IObuff, (size_t)size);
						ef_close(fd);
					}
					free(result[j]);
				}
				free(result);
				close(bfd);
			}
			else
			{
				remove(temp);
				if (!oflg)
				{
					emsg("temporary file open error.");
					wait_return(FALSE);
				}
				return(FALSE);
			}
			remove(temp);
			strcpy(temp, multi);
		}
	}
#endif

	curwin->w_cursor.lnum = *line1;
	curwin->w_cursor.col = 0;

	must_redraw = CLEAR;		/* screen has been shifted up one line */
	++no_wait_return;			/* don't call wait_return() while busy */
	windgoto((int)Rows - 1, 0);
	cursor_on();

	if (!u_save((linenr_t)(*line2), (linenr_t)(*line2 + 1)))
	{
		--no_wait_return;
		if (!oflg)
			wait_return(FALSE);
		return(FALSE);
	}
	n = readfile(temp, NULL, *line2, FALSE, (linenr_t)0, MAXLNUM);
	remove(temp);
	if (n == FAIL)
	{
		outchar('\n');
		if (!oflg)
			emsg2(e_notread, temp);
		--no_wait_return;
		if (!oflg)
			wait_return(FALSE);
		return(FALSE);
	}
	curwin->w_cursor.lnum = *line1;
	*linecount = *line2 - *line1 + 1;
	dellines(*linecount, TRUE, TRUE);
	--no_wait_return;
	updateScreen(CURSUPD);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *	uuencode + gzip file decode routine
 *----------------------------------------------------------------------------*/
void
decode(oflg, line1, line2)
BOOL			oflg;
linenr_t		line1;
linenr_t		line2;
{
	linenr_t		end;
	linenr_t 		total;
	linenr_t 		linecount;
	linenr_t		filtered = 0;
	linenr_t		already = 0;
	int				change;
	BOOL			complete = FALSE;

	DoMatome = TRUE;
	memset(boundary, '\0', sizeof(boundary));
	change = curbuf->b_changed;
	total = curbuf->b_ml.ml_line_count;
	end = line2;
	while (decode_part(oflg, &line1, &line2, &linecount))
	{
		complete = TRUE;
		if (!strchr(p_dc, 'r'))
			break;
		if (already < line2)
			filtered += linecount;
		already = line2;
#if 0
		line1 = line2 - (total - curbuf->b_ml.ml_line_count);
#endif
		line2 = end - (total - curbuf->b_ml.ml_line_count);
		if (line1 > line2)
			break;
		if (line1 > curbuf->b_ml.ml_line_count)
			break;
		end -= total - curbuf->b_ml.ml_line_count;
		total = curbuf->b_ml.ml_line_count;
	}
	curbuf->b_changed = change;

	if (complete && !oflg)
	{
		updateScreen(CLEAR);		/* do this before messages below */

		if (filtered > p_report)
			smsg((char_u *)"%ld lines filtered decoding file.",(long)filtered);
	}
	DoMatome = FALSE;
}

/*------------------------------------------------------------------------------
 *	base 64 encode routine
 *----------------------------------------------------------------------------*/
char *
encode(type, fname, crlf, ptr, len, last)
char			type;
char		*	fname;
int				crlf;
unsigned char *	ptr;
long		*	len;
int				last;
{
	static char		*	enc		= NULL;
	static long			enclen	= 0;
	char			*	tmp;
	char				buf[3];

	if (*len)
	{
		if ((tmp = realloc(enc, enclen + *len)) == NULL)
		{
			if (enc)
				free(enc);
			enc		= NULL;
			enclen	= 0;
			return(NULL);
		}
		enc = tmp;
		memcpy(&enc[enclen], ptr, *len);
		enclen += *len;
	}
	if (last)
	{
		if (crlf == 2)		/* mac */
		{
			buf[0] = CR;
			buf[1] = NUL;
		}
		else if (crlf == 1)	/* dos */
		{
			buf[0] = CR;
			buf[1] = NL;
			buf[2] = NUL;
		}
		else				/* unix */
		{
			buf[0] = NL;
			buf[1] = NUL;
		}
		if (type == 'b')
			tmp = b64ec(fname, buf, enc, enclen);
		else
			tmp = uuec(fname, buf, enc, enclen);
		free(enc);
		enc		= NULL;
		enclen	= 0;
		*len = strlen(tmp);
	}
	return(tmp);
}

/*------------------------------------------------------------------------------
 *	uuencode + gzip file decode routine
 *----------------------------------------------------------------------------*/
char *
decode_ver()
{
	static char		ver[1024];
	int				i;
	char		*	p;
#define VER_LEFT	12

	memset(ver, '\0', sizeof(ver));
	for (i = 0; i < VER_LEFT; i++)
		strcat(ver, " ");
	sprintf(&ver[strlen(ver)], "Support MIME Type : ");
	for (i = 0; mime_type[i] != NULL; i++)
	{
		if (i != 0)
			strcat(ver, " / ");
		p = strchr(&mime_type[i][2], '?');
		strncpy(&ver[strlen(ver)], &mime_type[i][2], p - &mime_type[i][2]);
	}
	strcat(ver, "\r\n");
	for (i = 0; i < VER_LEFT; i++)
		strcat(ver, " ");
	sprintf(&ver[strlen(ver)], "Content-Transfer-Encoding : base64\r\n");
	for (i = 0; i < VER_LEFT; i++)
		strcat(ver, " ");
	sprintf(&ver[strlen(ver)], "Content-Transfer-Encoding : X-uuencode\r\n");
	for (i = 0; i < VER_LEFT; i++)
		strcat(ver, " ");
	sprintf(&ver[strlen(ver)], "Content-Transfer-Encoding : quoted-printable\r\n");
	return(ver);
}
