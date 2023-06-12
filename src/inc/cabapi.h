////////////////////////////////////////////////////////////
// CABAPI.H      Kuniaki Miyauchi

#if !defined(CABAPI_H)
#define CABAPI_H

#define CABAPI_VERSION	84

#ifndef FNAME_MAX32
#define FNAME_MAX32		512
#endif

#if defined(__BORLANDC__)
#pragma option -a-
#else
#pragma pack(1)
#endif

#ifndef ARC_DECSTRACT
#define ARC_DECSTRACT
typedef	HGLOBAL	HARC;

typedef struct {
	DWORD 			dwOriginalSize;		/* �t�@�C���̃T�C�Y */
 	DWORD 			dwCompressedSize;	/* ���k��̃T�C�Y */
	DWORD			dwCRC;				/* �i�[�t�@�C���̃`�F�b�N�T�� */
	UINT			uFlag;				/* �������� */
	UINT			uOSType;			/* ���ɍ쐬�Ɏg��ꂽ�n�r */
	WORD			wRatio;				/* ���k�� */
	WORD			wDate;				/* �i�[�t�@�C���̓��t(DOS �`��) */
	WORD 			wTime;				/* �i�[�t�@�C���̎���(�V) */
	char			szFileName[FNAME_MAX32 + 1];	/* ���ɖ� */
	char			dummy1[3];
	char			szAttribute[8];		/* �i�[�t�@�C���̑���(���ɌŗL) */
	char			szMode[8];			/* �i�[�t�@�C���̊i�[���[�h(�V) */
}	INDIVIDUALINFO, *LPINDIVIDUALINFO;

typedef struct {
	DWORD 			dwFileSize;		/* �i�[�t�@�C���̃T�C�Y */
	DWORD			dwWriteSize;	/* �������݃T�C�Y */
	char			szSourceFileName[FNAME_MAX32 + 1];	/* �i�[�t�@�C���� */
	char			dummy1[3];
	char			szDestFileName[FNAME_MAX32 + 1];
									/* �𓀐�܂��͈��k���p�X�� */
	char			dummy[3];
}	EXTRACTINGINFO, *LPEXTRACTINGINFO;

typedef struct {
	EXTRACTINGINFO exinfo;
	DWORD dwCompressedSize;
	DWORD dwCRC;
	UINT  uOSType;
	WORD  wRatio;
	WORD  wDate;
	WORD  wTime;
	char  szAttribute[8];
	char  szMode[8];
} EXTRACTINGINFOEX, *LPEXTRACTINGINFOEX;

#endif

typedef struct {
	UINT			uTotalFiles;
	UINT			uCountInfoFiles;
	UINT			uErrorCount;
	LONG			lDiskSpace;
	HGLOBAL			hSubInfo;
}	LHALOCALINFO;

#if !defined(__BORLANDC__)
#pragma pack()
#else
#pragma option -a.
#endif

#if !defined(__BORLANDC__)
#define	_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ### LHA.DLL Ver 1.1 �ƌ݊����̂��� API �ł��B### */
WORD WINAPI CabGetVersion(VOID);

BOOL WINAPI CabGetRunning(VOID);

BOOL WINAPI CabGetBackGroundMode(VOID);
BOOL WINAPI CabSetBackGroundMode(const BOOL bBkgndMode);
BOOL WINAPI CabGetCursorMode(VOID);
BOOL WINAPI CabSetCursorMode(const BOOL bCursorMode);
WORD WINAPI CabGetCursorInterval(VOID);
BOOL WINAPI CabSetCursorInterval(const WORD wInterval);

int WINAPI Cab(const HWND hwnd, LPCSTR pszCmdLine,
						LPSTR pszOutput, const DWORD dwSize);

/* ###�w�����A�[�J�C�o API�x���ʂ� API �ł��B### */
#if !defined(CHECKARCHIVE_RAPID)
#define	CHECKARCHIVE_RAPID		0	/* �Ȉ�(�ŏ��̂R�w�b�_�܂�) */
#define	CHECKARCHIVE_BASIC		1	/* �W��(�S�Ẵw�b�_) */
#define	CHECKARCHIVE_FULLCRC	2	/* ���S(�i�[�t�@�C���� CRC �`�F�b�N) */

	/* �ȉ��̃t���O�͏�L�Ƒg�ݍ��킹�Ďg�p�B*/
#define CHECKARCHIVE_RECOVERY	4   /* �j���w�b�_��ǂݔ�΂��ď��� */
#define CHECKARCHIVE_SFX		8	/* SFX ���ǂ�����Ԃ� */
#define CHECKARCHIVE_ALL		16	/* �t�@�C���̍Ō�܂Ō������� */
#endif

BOOL WINAPI CabCheckArchive(LPCSTR pszArchive, const int iMode);

int WINAPI CabGetFileCount(LPCSTR pszArchive);

#if !defined(UNPACK_CONFIG_MODE)
#define	UNPACK_CONFIG_MODE		1	/* �𓀌n�̐ݒ� */
#define PACK_CONFIG_MODE		2	/* ���k�n�̐ݒ� */
#endif

BOOL WINAPI CabConfigDialog(const HWND hwnd, LPSTR pszComBuffer,
						const int iMode);

#if !defined(ISARC_FUNCTION_START)
#define ISARC_FUNCTION_START			0
#define ISARC							0	/* Cab */
#define ISARC_GET_VERSION				1	/* CabGetVersion */
#define ISARC_GET_CURSOR_INTERVAL		2	/* CabGetCursorInterval */
#define ISARC_SET_CURSOR_INTERVAL		3	/* CabSetCursorInterval */
#define ISARC_GET_BACK_GROUND_MODE		4	/* CabGetBackGroundMode */
#define ISARC_SET_BACK_GROUND_MODE		5	/* CabSetBackGroundMode */
#define ISARC_GET_CURSOR_MODE			6	/* CabGetCursorMode */
#define ISARC_SET_CURSOR_MODE			7	/* CabSetCursorMode */
#define ISARC_GET_RUNNING				8	/* CabGetRunning */

#define ISARC_CHECK_ARCHIVE				16	/* CabCheckArchive */
#define ISARC_CONFIG_DIALOG				17	/* CabConfigDialog */
#define ISARC_GET_FILE_COUNT			18	/* CabGetFileCount */
#define ISARC_QUERY_FUNCTION_LIST		19	/* CabQueryFunctionList */
#define ISARC_HOUT						20	/* (CabHOut) */
#define ISARC_STRUCTOUT					21	/* (CabStructOut) */
#define ISARC_GET_ARC_FILE_INFO			22	/* CabGetArcFileInfo */

#define ISARC_OPEN_ARCHIVE				23	/* CabOpenArchive */
#define ISARC_CLOSE_ARCHIVE				24	/* CabCloseArchive */
#define ISARC_FIND_FIRST				25	/* CabFindFirst */
#define ISARC_FIND_NEXT					26	/* CabFindNext */
#define ISARC_EXTRACT					27	/* (CabExtract) */
#define ISARC_ADD						28	/* (CabAdd) */
#define ISARC_MOVE						29	/* (CabMove) */
#define ISARC_DELETE					30	/* (CabDelete) */

#define ISARC_GET_ARC_FILE_NAME			40	/* CabGetArcFileName */
#define ISARC_GET_ARC_FILE_SIZE			41	/* CabGetArcFileSize */
#define ISARC_GET_ARC_ORIGINAL_SIZE		42	/* CabArcOriginalSize */
#define ISARC_GET_ARC_COMPRESSED_SIZE	43	/* CabGetArcCompressedSize */
#define ISARC_GET_ARC_RATIO				44	/* CabGetArcRatio */
#define ISARC_GET_ARC_DATE				45	/* CabGetArcDate */
#define ISARC_GET_ARC_TIME				46	/* CabGetArcTime */
#define ISARC_GET_ARC_OS_TYPE			47	/* CabGetArcOSType */
#define ISARC_GET_ARC_IS_SFX_FILE		48	/* CabGetArcIsSFXFile */
#define ISARC_GET_FILE_NAME				57	/* CabGetFileName */
#define ISARC_GET_ORIGINAL_SIZE			58	/* CabGetOriginalSize */
#define ISARC_GET_COMPRESSED_SIZE		59	/* CabGetCompressedSize */
#define ISARC_GET_RATIO					60	/* CabGetRatio */
#define ISARC_GET_DATE					61	/* CabGetDate */
#define ISARC_GET_TIME					62	/* CabGetTime */
#define ISARC_GET_CRC					63	/* CabGetCRC */
#define ISARC_GET_ATTRIBUTE				64	/* CabGetAttribute */
#define ISARC_GET_OS_TYPE				65	/* CabGetOSType */
#define ISARC_GET_METHOD				66	/* CabGetMethod */
#define ISARC_GET_WRITE_TIME			67	/* CabGetWriteTime */
#define ISARC_GET_CREATE_TIME			68	/* CabGetCreateTime */
#define ISARC_GET_ACCESS_TIME			69	/* CabGetAccessTime */

#define ISARC_FUNCTION_END				69
#endif	/* ISARC_FUNCTION_START */

BOOL WINAPI CabQueryFunctionList(const int iFunction);

#ifndef WM_ARCEXTRACT
#define	WM_ARCEXTRACT	"wm_arcextract"

#define	ARCEXTRACT_BEGIN		0	/* �Y���t�@�C���̏����̊J�n */
#define	ARCEXTRACT_INPROCESS	1	/* �Y���t�@�C���̓W�J�� */
#define	ARCEXTRACT_END			2	/* �����I���A�֘A���������J�� */
#define ARCEXTRACT_OPEN			3	/* �Y�����ɂ̏����̊J�n */
#define ARCEXTRACT_COPY			4	/* ���[�N�t�@�C���̏����߂� */

WINAPI CabSetOwnerWindow(const HWND hwnd);
BOOL WINAPI CabClearOwnerWindow(VOID);

typedef BOOL CALLBACK ARCHIVERPROC(HWND hwnd, UINT uMsg,
						UINT nState, LPEXTRACTINGINFOEX lpEis);
typedef ARCHIVERPROC *LPARCHIVERPROC;

BOOL WINAPI CabSetOwnerWindowEx(HWND hwnd, LPARCHIVERPROC lpArcProc);
BOOL WINAPI CabKillOwnerWindowEx(HWND hwnd);
#endif

/* OpenArchive �n API �ł��B */

#if !defined(EXTRACT_FOUND_FILE)
/* MODE (for UnarjOpenArchive) */
#define M_INIT_FILE_USE			0x00000001L	/* ���W�X�g���̐ݒ���g�p */
#define M_REGARDLESS_INIT_FILE	0x00000002L	/* �V ���g�p���Ȃ� */
#define M_CHECK_ALL_PATH		0x00000100L	/* ���i�ȃt�@�C�����T�[�` */
#define M_CHECK_FILENAME_ONLY	0x00000200L	/* �V���s��Ȃ� */
#define M_USE_DRIVE_LETTER		0x00001000L	/* �h���C�u������i�[ */
#define M_NOT_USE_DRIVE_LETTER	0x00002000L	/* �V ���i�[���Ȃ� */
#define M_ERROR_MESSAGE_ON		0x00400000L	/* �G���[���b�Z�[�W��\�� */
#define M_ERROR_MESSAGE_OFF		0x00800000L	/* �V��\�����Ȃ� */
#define M_RECOVERY_ON			0x08000000L /* �j���w�b�_�̓ǂݔ�΂� */

#define EXTRACT_FOUND_FILE		0x40000000L	/* �������ꂽ�t�@�C������ */
#define EXTRACT_NAMED_FILE		0x80000000L	/* �w�肵���t�@�C������ */
#endif /* EXTRACT_FOUND_FILE */

#if !defined(SFX_NOT)
#define SFX_NOT					0
#define SFX_DOS_213S			1
#define SFX_DOS_250S			2
#define SFX_DOS_265S			3
#define SFX_DOS_213L			51
#define SFX_DOS_250L			52
#define SFX_DOS_265L			53
#define SFX_WIN16_213			1001
#define SFX_WIN16_213_1			1001
#define SFX_WIN16_213_2			1002
#define SFX_WIN16_250_1			1011
#define SFX_WIN32_213			2001
#define SFX_WIN32_250			2011
#define SFX_WIN32_250_1			2011
#define SFX_WIN32_250_6			2012
#define SFX_LZHSFX_1002			2051
#define SFX_LZHSFX_1100			2052
#define SFX_LZHAUTO_0002		2101
#define SFX_LZHAUTO_1002		2102
#define SFX_LZHAUTO_1100		2103
#endif

HARC WINAPI CabOpenArchive(const HWND hwnd, LPCSTR pszArchive,
							const DWORD dwMode);
int WINAPI CabCloseArchive(HARC harc);
int WINAPI CabFindFirst(HARC harc, LPCSTR pszWildName,
							INDIVIDUALINFO *lpSubInfo);
int WINAPI CabFindNext(HARC harc, INDIVIDUALINFO *lpSubInfo);
int WINAPI CabGetArcFileName(HARC harc, LPSTR lpBuffer, const int nSize);
DWORD WINAPI CabGetArcFileSize(HARC harc);
DWORD WINAPI CabGetArcOriginalSize(HARC harc);
DWORD WINAPI CabGetArcCompressedSize(HARC harc);
WORD WINAPI CabGetArcRatio(HARC harc);
WORD WINAPI CabGetArcDate(HARC harc);
WORD WINAPI CabGetArcTime(HARC harc);
UINT WINAPI CabGetArcOSType(HARC harc);
int WINAPI CabIsSFXFile(HARC harc);
int WINAPI CabGetFileName(HARC harc, LPSTR lpBuffer, const int nSize);
int WINAPI CabGetMethod(HARC harc, LPSTR lpBuffer, const int nSize);
DWORD WINAPI CabGetOriginalSize(HARC harc);
DWORD WINAPI CabGetCompressedSize(HARC harc);
WORD WINAPI CabGetRatio(HARC harc);
WORD WINAPI CabGetDate(HARC harc);
WORD WINAPI CabGetTime(HARC harc);
DWORD WINAPI CabGetWriteTime(HARC harc);
DWORD WINAPI CabGetAccessTime(HARC harc);
DWORD WINAPI CabGetCreateTime(HARC harc);
DWORD WINAPI CabGetCRC(HARC harc);
int WINAPI CabGetAttribute(HARC harc);
UINT WINAPI CabGetOSType(HARC harc);

/* ### CAB32.DLL �Ǝ��� API �ł��B### */
BOOL WINAPI CabGetCabinetInfo(LPCSTR pszCab, void* pv);
BOOL WINAPI CabGetNextCabinet(HARC harc, LPSTR pszNextCab, int nMaxLen);
BOOL WINAPI CabFdi(HWND hwnd, LPSTR pszCab, LPSTR pszPath, LPSTR pszFiles, DWORD dwFlags);
DWORD WINAPI CabGetUncompressedTotalSize(LPCSTR pszCab);
BOOL WINAPI CabGetCabinetFromSfx(LPCSTR pszSfx, LPCSTR pszPath);

#ifdef __cplusplus
}
#endif

#ifndef FA_RDONLY
/* Attribute */
#define FA_RDONLY       0x01            /* �������ݕی쑮�� */
#define FA_HIDDEN       0x02            /* �B������ */
#define FA_SYSTEM       0x04            /* �V�X�e������ */
#define FA_LABEL        0x08            /* �{�����[���E���x�� */
#define FA_DIREC        0x10            /* �f�B���N�g�� */
#define FA_ARCH         0x20            /* �A�[�J�C�u���� */
#endif

#ifndef ERROR_ARC_FILE_OPEN
/* WARNING */
#define ERROR_DISK_SPACE		0x8005
#define ERROR_READ_ONLY			0x8006
#define ERROR_USER_SKIP			0x8007
#define ERROR_UNKNOWN_TYPE		0x8008
#define ERROR_METHOD			0x8009
#define ERROR_PASSWORD_FILE		0x800A
#define ERROR_VERSION			0x800B
#define ERROR_FILE_CRC			0x800C
#define ERROR_FILE_OPEN			0x800D
#define ERROR_MORE_FRESH		0x800E
#define ERROR_NOT_EXIST			0x800F
#define ERROR_ALREADY_EXIST		0x8010

#define ERROR_TOO_MANY_FILES	0x8011

/* ERROR */
#define ERROR_MAKEDIRECTORY		0x8012
#define ERROR_CANNOT_WRITE		0x8013
#define ERROR_HUFFMAN_CODE		0x8014
#define ERROR_COMMENT_HEADER	0x8015
#define ERROR_HEADER_CRC		0x8016
#define ERROR_HEADER_BROKEN		0x8017
#define ERROR_ARC_FILE_OPEN		0x8018
#define ERROR_NOT_ARC_FILE		0x8019
#define ERROR_CANNOT_READ		0x801A
#define ERROR_FILE_STYLE		0x801B
#define ERROR_COMMAND_NAME		0x801C
#define ERROR_MORE_HEAP_MEMORY	0x801D
#define ERROR_ENOUGH_MEMORY		0x801E
#if !defined(ERROR_ALREADY_RUNNING)
#define ERROR_ALREADY_RUNNING	0x801F
#endif
#define ERROR_USER_CANCEL		0x8020
#define ERROR_HARC_ISNOT_OPENED	0x8021
#define ERROR_NOT_SEARCH_MODE	0x8022
#define ERROR_NOT_SUPPORT		0x8023
#define ERROR_TIME_STAMP		0x8024
#define ERROR_TMP_OPEN			0x8025
#define ERROR_LONG_FILE_NAME	0x8026
#define ERROR_ARC_READ_ONLY		0x8027
#define ERROR_SAME_NAME_FILE	0x8028
#define ERROR_NOT_FIND_ARC_FILE 0x8029
#define ERROR_RESPONSE_READ		0x802A
#define ERROR_NOT_FILENAME		0x802B
#define ERROR_TMP_COPY			0x802C

#define ERROR_END	ERROR_TMP_COPY
#endif /* ERROR_ARC_FILE_OPEN */


#endif	// CABAPI_H