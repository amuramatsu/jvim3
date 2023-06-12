/*=============================================================== vim:ts=4:sw=4:
 *
 *	clip --- input clip board
 *
 *============================================================================*/
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>

/*==============================================================================
 *
 *============================================================================*/
#define	BUF_SIZE			128					/* allocate buffer size		  */

/*==============================================================================
 *
 *============================================================================*/
#if !defined(__DATE__) || !defined(__TIME__)
static char		version[] = "clip by Ken'ichi Tsuchida (1999 May 30)";
#else
static char		version[] = "clip by Ken'ichi Tsuchida (1999 May 30, compiled " __DATE__ " " __TIME__ ")";
#endif
static	BOOL				o_disp	= FALSE;	/* display input			  */
static	int					o_tabs	= 0;		/* expand tabstop			  */
static	BOOL				o_exec	= FALSE;	/* execute command			  */
static	HANDLE				hReadPipe;

/*==============================================================================
 *
 *============================================================================*/
/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
usage()
{
	fprintf(stderr, "usage: clip [-pV] [-e <num>]\n");
	fprintf(stderr, "	-p          print display.\n");
	fprintf(stderr, "	-e <num>    expand tabs.\n");
	fprintf(stderr, "	-V          print version.\n");
	exit(2);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static BOOL
popen(args)
char		*	args;
{
	STARTUPINFO				si;			/* for CreateProcess call */
	PROCESS_INFORMATION		pi;			/* for CreateProcess call */
	SECURITY_ATTRIBUTES		saPipe;		/* security for anonymous pipe */
	HANDLE					hWritePipe;
	HANDLE					hWritePipe2;

	/* set up the security attributes for the anonymous pipe */
	saPipe.nLength = sizeof(SECURITY_ATTRIBUTES);
	saPipe.lpSecurityDescriptor = NULL;
	saPipe.bInheritHandle = TRUE;

	/* create the anonymous pipe */
	if (!CreatePipe(&hReadPipe, &hWritePipe, &saPipe, 0))
		return(FALSE);

	if (!DuplicateHandle(GetCurrentProcess(), hReadPipe, GetCurrentProcess(),
							NULL, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return(FALSE);

	if (!DuplicateHandle(GetCurrentProcess(), hWritePipe, GetCurrentProcess(),
							&hWritePipe2, 0, TRUE, DUPLICATE_SAME_ACCESS))
		return(FALSE);

	/* Set up the STARTUPINFO structure for the CreateProcess() call */
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.hStdInput = hWritePipe2;
	si.hStdOutput = hWritePipe;
	si.hStdError = hWritePipe2;
	si.dwFlags = STARTF_USESTDHANDLES;

	if (!CreateProcess(NULL, args, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		return(FALSE);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(hWritePipe);
	CloseHandle(hWritePipe2);
	return(TRUE);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static int
pread()
{
	static char				chReadBuffer[16];
	static int				cchReadBuffer = -1;
	int						ch;

	/* read from the pipe until we get an ERROR_BROKEN_PIPE */
	for (;;)
	{
		if (cchReadBuffer <= 0)
		{
			if (!ReadFile(hReadPipe, chReadBuffer, sizeof(chReadBuffer),
														&cchReadBuffer, NULL))
			{
				if (GetLastError() == ERROR_BROKEN_PIPE)
					break;
				return(-1);
			}
			if (cchReadBuffer == 0)
				break;
		}
		ch = chReadBuffer[sizeof(chReadBuffer) - cchReadBuffer];
		cchReadBuffer--;
		return(ch);
	}
	CloseHandle(hReadPipe);
	return(EOF);
}

static int
paste()
{
	HANDLE				hClipData;
	unsigned char	*	lpClipData;
	unsigned char	*	buf = NULL;

	if (OpenClipboard(NULL) == FALSE)
		return(1);
	if ((hClipData = GetClipboardData(CF_TEXT)) != NULL
			&& (lpClipData = (unsigned char *)GlobalLock(hClipData)) != NULL)
	{
		int len = strlen(lpClipData) + 1;
		if ((buf = (unsigned char *)malloc(len)) != NULL)
			CopyMemory(buf, lpClipData, len);
		else
			fputs(lpClipData, stdout);
		GlobalUnlock(hClipData);
	}
	CloseClipboard();

	if (buf != NULL)
	{
		fputs(buf, stdout);
		free(buf);
	}
	return(0);
}

/*------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
void
main(argc, argv)
int				argc;
char		**	argv;
{
	char		*	optarg;
	int				skip;
	HANDLE			hClipData;
	char		*	lpClipData;
	char		*	p;
	int				c;
	int				num	= 0;
	int				size;
	int				col = 0;
	char			command[256];

#ifdef XARGS
	xargs(&argc, &argv);
#endif
	++argv;
	--argc;
	while (argc >= 1 && argv[0][0] == '-')
	{
		skip = FALSE;
		for (p = argv[0] + 1; *p && skip == FALSE; p++)
		{
			switch (*p) {
			case 'V':
				fprintf(stderr, "%s.\n", version);
				exit(0);
			case 'p':
				o_disp = TRUE;
				break;
			case 'e':						/* expand tabstop	  */
			case 'x':						/* execute command	  */
				if (p[1] != '\0')
					optarg = &p[1];
				else
				{
					++ argv;
					-- argc;
					if (argc >= 1)
						optarg = argv[0];
					else
					{
						fprintf(stderr, "option unmatch.\n");
						usage();
					}
				}
				skip = TRUE;
				switch (*p) {
				case 'e':					/* expand tabstop	  */
					o_tabs = atol((char *)optarg);
					if (o_tabs >= BUF_SIZE)
						exit(1);
					break;
				case 'x':
					o_exec = TRUE;			/* execute command */
					strcpy(command, optarg);
					while (argc > 1)
					{
						strcat(command, " ");
						++ argv;
						-- argc;
						strcat(command, *argv);
					}
					break;
				}
				break;
			case 'v':	/* paste */
				exit(paste());
				break;
			default:
				usage();
				break;
			}
		}
		++ argv;
		-- argc;
	}
	if (o_exec && !popen(command))
		exit(1);
	if ((p = malloc(BUF_SIZE + 1)) == NULL)
		exit(1);
	size = BUF_SIZE;
	while ((c = (o_exec ? pread() : fgetc(stdin))) != EOF)
	{
		if (o_tabs && c == '\t')
		{
			int				n_spaces;
			int				i;

			n_spaces = o_tabs - col % o_tabs;
			if ((num + n_spaces + 1) >= size)
			{
				if ((p = realloc(p, size + BUF_SIZE)) == NULL)
					exit(1);
				size += BUF_SIZE;
			}
			for (i = 0; i < n_spaces; i++)
			{
				p[num++] = ' ';
				if (o_disp)
					fputc(' ', stdout);
				col++;
			}
		}
		else
		{
			if (num >= size)
			{
				if ((p = realloc(p, size + BUF_SIZE)) == NULL)
					exit(1);
				size += BUF_SIZE;
			}
			p[num++] = c;
			if (o_disp)
				fputc(c, stdout);
			col++;
			if (c == '\r' || c == '\n')
				col = 0;
		}
	}
	p[num] = '\0';
	if ((hClipData = GlobalAlloc(GMEM_MOVEABLE, num)) == NULL)
		exit(1);
	if ((lpClipData = GlobalLock(hClipData)) == NULL)
	{
		GlobalFree(hClipData);
		exit(1);
	}
	CopyMemory(lpClipData, p, num);
	GlobalUnlock(hClipData);
	if (OpenClipboard(NULL) == FALSE)
	{
		GlobalFree(hClipData);
		exit(1);
	}
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hClipData);
	CloseClipboard();
	exit(0);
}
