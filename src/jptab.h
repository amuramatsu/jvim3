/*
   This table is generated by jptab.c. 
*/

#ifndef JP_TRACKTAB
#define JUPS 1
#define JLOS 2

#define JPCS TRUE
#define JPOS TRUE
#define JPCK TRUE
#define JPCA TRUE

#define JPOA TRUE

static struct _jptab
{
	char	cls1, cls2;
	unsigned int	punccsym:1, puncckana:1, punccasc:1;
	unsigned int	puncosym:1, puncoasc:1, stcend:1;
	unsigned int	scase:2;
	char	swap;
}		jptab[128] = {
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\000' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\001' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\002' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\003' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\004' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\005' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\006' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\007' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\010' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\011' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\012' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\013' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\014' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\015' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\016' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\017' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\020' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\021' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\022' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\023' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\024' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\025' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\026' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\027' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\030' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\031' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\032' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\033' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\034' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\035' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\036' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\037' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\040' },
{JPC_KIGOU,/*Spc*/ 0,   0,JPCK,JPCA,   0,   0,1,   0,'\041' },
{JPC_KIGOU2,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JUPS,'\044' },
{JPC_ALNUM,JPC_KIGOU,JPCS,JPCK,   0,   0,   0,1,JUPS,'\045' },
{JPC_HIRA ,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\042' },
{JPC_KATA ,JPC_KIGOU,JPCS,JPCK,   0,   0,   0,1,JLOS,'\043' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\076' },
{JPC_KANJI,JPC_KIGOU,JPCS,JPCK,   0,   0,   0,0,   0,'\047' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,JPOA,0,   0,'\050' },
{JPC_KANJI,JPC_KIGOU,JPCS,JPCK,JPCA,   0,   0,1,   0,'\051' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,1,   0,'\052' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,   0,'\053' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,JPCA,   0,   0,0,   0,'\054' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,JPCA,   0,   0,0,JUPS,'\056' },
{JPC_KANJI,JPC_KIGOU,   0,   0,JPCA,JPOS,   0,0,JLOS,'\055' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\057' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\060' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\062' },
{JPC_KANJI,/*Spc*/ 0,   0,   0,   0,   0,   0,0,JLOS,'\061' },
{JPC_KANJI,JPC_HIRA ,JPCS,   0,   0,   0,   0,0,JUPS,'\065' },
{JPC_KANJI,JPC_HIRA ,JPCS,   0,   0,   0,   0,0,JUPS,'\066' },
{JPC_KANJI,JPC_HIRA ,JPCS,   0,   0,   0,   0,0,JLOS,'\063' },
{JPC_KANJI,JPC_HIRA ,JPCS,   0,   0,   0,   0,0,JLOS,'\064' },
{JPC_KANJI,JPC_KANJI,JPCS,   0,   0,   0,   0,0,JUPS,'\071' },
{JPC_KANJI,JPC_KANJI,JPCS,   0,   0,   0,   0,0,   0,'\070' },
{JPC_KANJI,JPC_KANJI,JPCS,   0,   0,   0,   0,0,JLOS,'\067' },
{JPC_KANJI,JPC_KANJI,JPCS,   0,JPCA,   0,   0,0,   0,'\072' },
{JPC_KANJI,JPC_KIGOU,   0,   0,JPCA,   0,   0,0,   0,'\073' },
{JPC_KANJI,JPC_KATA ,JPCS,   0,   0,   0,JPOA,0,JUPS,'\135' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\075' },
{JPC_KANJI,JPC_KIGOU,   0,   0,JPCA,   0,   0,0,JLOS,'\046' },
{JPC_KANJI,JPC_KIGOU,   0,   0,JPCA,   0,   0,0,JUPS,'\100' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,JPOA,0,JLOS,'\077' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\101' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\103' },
{JPC_KANJI,JPC_KIGOU,   0,JPCK,   0,   0,   0,0,JUPS,'\102' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JUPS,'\105' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\104' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JUPS,'\107' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\106' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JUPS,'\111' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\110' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JUPS,'\122' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JUPS,'\123' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JLOS,'\120' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\121' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,   0,'\116' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,   0,'\117' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JUPS,'\114' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JUPS,'\115' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JLOS,'\112' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\113' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,   0,'\124' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,   0,'\125' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JUPS,'\130' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JUPS,'\131' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JLOS,'\126' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JLOS,'\127' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,   0,'\132' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,JPOA,0,   0,'\133' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\136' },
{JPC_KANJI,JPC_KATA ,JPCS,   0,JPCA,   0,   0,0,JLOS,'\074' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\134' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\166' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,JPOA,0,JLOS,'\163' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\142' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\141' },
{JPC_KANJI,JPC_KIGOU,   0,JPCK,   0,   0,   0,0,JUPS,'\145' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\146' },
{JPC_KANJI,JPC_KIGOU,   0,JPCK,   0,   0,   0,0,JLOS,'\143' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\144' },
{JPC_KANJI,JPC_KIGOU,   0,JPCK,   0,   0,   0,0,   0,'\147' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\150' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\152' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\151' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,JUPS,'\156' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,   0,'\154' },
{JPC_KANJI,JPC_KIGOU,JPCS,   0,   0,   0,   0,0,   0,'\155' },
{JPC_KANJI,JPC_KIGOU,JPCS,JPCK,   0,   0,   0,0,JLOS,'\153' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JUPS,'\160' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,JLOS,'\157' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,   0,'\161' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,   0,'\162' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\140' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\164' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\165' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\137' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\167' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,JPOS,   0,0,   0,'\170' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JUPS,'\172' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\171' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,JPOA,0,JUPS,'\174' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,JLOS,'\173' },
{JPC_KANJI,JPC_KIGOU,   0,   0,JPCA,   0,   0,0,   0,'\175' },
{JPC_KANJI,JPC_KIGOU,   0,   0,   0,   0,   0,0,   0,'\176' },
{       -1,       -1,   0,   0,   0,   0,   0,0,   0,'\177' }
};

char jisx0201r[] ={
	 '\040', '\040',
	 '\201', '\102',
	 '\201', '\165',
	 '\201', '\166',
	 '\201', '\101',
	 '\201', '\105',
	 '\203', '\222',
	 '\203', '\100',
	 '\203', '\102',
	 '\203', '\104',
	 '\203', '\106',
	 '\203', '\110',
	 '\203', '\203',
	 '\203', '\205',
	 '\203', '\207',
	 '\203', '\142',
	 '\201', '\174',
	 '\203', '\101',
	 '\203', '\103',
	 '\203', '\105',
	 '\203', '\107',
	 '\203', '\111',
	 '\203', '\112',
	 '\203', '\114',
	 '\203', '\116',
	 '\203', '\120',
	 '\203', '\122',
	 '\203', '\124',
	 '\203', '\126',
	 '\203', '\130',
	 '\203', '\132',
	 '\203', '\134',
	 '\203', '\136',
	 '\203', '\140',
	 '\203', '\143',
	 '\203', '\145',
	 '\203', '\147',
	 '\203', '\151',
	 '\203', '\152',
	 '\203', '\153',
	 '\203', '\154',
	 '\203', '\155',
	 '\203', '\156',
	 '\203', '\161',
	 '\203', '\164',
	 '\203', '\167',
	 '\203', '\172',
	 '\203', '\175',
	 '\203', '\176',
	 '\203', '\200',
	 '\203', '\201',
	 '\203', '\202',
	 '\203', '\204',
	 '\203', '\206',
	 '\203', '\210',
	 '\203', '\211',
	 '\203', '\212',
	 '\203', '\213',
	 '\203', '\214',
	 '\203', '\215',
	 '\203', '\217',
	 '\203', '\223',
	 '\201', '\112',
	 '\201', '\113',
};

char *Asconv[] =
{
 "\041\201\111"/* ! */,  "\042\201\150"/* " */,  "\043\201\224"/* # */,
 "\044\201\220"/* $ */,  "\045\201\223"/* % */,  "\046\201\225"/* & */,
 "\047\201\146"/* ' */,  "\050\201\151"/* ( */,  "\051\201\152"/* ) */,
 "\055\201\174"/* - */,  "\075\201\201"/* = */,  "\136\201\117"/* ^ */,
 "\176\201\120"/* ~ */,  "\134\201\217"/* \ */,  "\174\201\142"/* | */,
 "\100\201\227"/* @ */,  "\140\201\145"/* ` */,  "\133\201\155"/* [ */,
 "\173\201\157"/* { */,  "\073\201\107"/* ; */,  "\053\201\173"/* + */,
 "\072\201\106"/* : */,  "\052\201\226"/* * */,  "\135\201\156"/* ] */,
 "\175\201\160"/* } */,  "\054\201\103"/* , */,  "\074\201\203"/* < */,
 "\056\201\104"/* . */,  "\076\201\204"/* > */,  "\057\201\136"/* / */,
 "\077\201\110"/* ? */,  "\040\201\100"/*   */,  "\011\201\100"/* 	 */,
 "\000\000\000"/* @ */,
};

#else /* JP_TRACKTAB */
char *tracktab_jp[] =
{
 "\040\040", "\201\253", "\201\252", "\204\240",
 "\201\250", "\204\243", "\204\242", "\204\247",
 "\201\251", "\204\244", "\204\241", "\204\245",
 "\204\237", "\204\250", "\204\246", "\204\251"
};
char *tracktab_bj[] =
{
 "\040\040", "\201\253", "\201\252", "\204\253",
 "\201\250", "\204\256", "\204\255", "\204\262",
 "\201\251", "\204\257", "\204\254", "\204\260",
 "\204\252", "\204\263", "\204\261", "\204\264"
};
char *tracktab_hj[] =
{
 "\040\040", "\201\253", "\201\252", "\204\240",
 "\201\250", "\204\243", "\204\242", "\204\274",
 "\201\251", "\204\257", "\204\254", "\204\272",
 "\204\252", "\204\270", "\204\266", "\204\271"
};
#endif /* JP_TRACKTAB */