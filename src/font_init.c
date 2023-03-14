//==============================================================================
//
// font_init.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-12-07 - Initial Version.
//==============================================================================
//
#include "font_P.h"
#include "wmgr.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>


typedef struct {
	char           fh_fmver[8];
	unsigned long  fh_fntsz;
	unsigned long  fh_fbfsz;
	unsigned short fh_cbfsz;
	unsigned short fh_hedsz;
	unsigned short fh_fntid;
	unsigned short fh_sfvnr;
	char           fh_fntnm[70];
	char           fh_mdate[10];
	char           fh_laynm[70];
	char           fh_cpyrt[78];
	unsigned short fh_nchrl;
	unsigned short fh_nchrf;
	unsigned short fh_fchrf;
	unsigned short fh_nktks;
	unsigned short fh_nkprs;
	char           fh_flags;
	char           fh_cflgs;
	char           fh_famcl;
	char           fh_frmcl;
	char           fh_sfntn[32];
	char           fh_sfacn[16];
	char           fh_fntfm[14];
	unsigned short fh_itang;
	unsigned short fh_orupm;
} VQT_FHDR;


static const short _FONT_Proto[] = {
	' ','!','"','#','$','%','&', 39, '(',')','*','+',',','-','.','/',
	'0','1','2','3','4','5','6','7', '8','9',':',';','<','=','>','?',
	'@','A','B','C','D','E','F','G', 'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W', 'X','Y','Z','[', 92,']','^','_',
	'`','a','b','c','d','e','f','g', 'h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w', 'x','y','z','{','|','}','~'
};


static const short _FONT_TosLatin[256] = {
#	define ___ '\0'
#	define UDF '\0' // UNDEFINED
#	define NBS ' '  // NO-BREAK-SPACE
	0  ,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___,
	___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___,
	' ','!','"','#','$','%','&', 39, '(',')','*','+',',','-','.','/',
	'0','1','2','3','4','5','6','7', '8','9',':',';','<','=','>','?',
	'@','A','B','C','D','E','F','G', 'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W', 'X','Y','Z','[', 92,']','^','_',
	'`','a','b','c','d','e','f','g', 'h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w', 'x','y','z','{','|','}','~','',
	'‡','·','‚','„','‰','Â','Ê','Á', 'Ë','È','Í','Î','Ï','Ì','Ó','Ô',
	'','©','Ú','Û','Ù','ı',UDF,'˜', '¥','µ','˙','˚','¸',UDF,'ü','ø',
	NBS,'≠','õ','ú','ü','ù','|','›', 'π','Ω','¶','Æ','™',___,'æ','ˇ',
	'¯','Ò','˝','˛','∫','Ê','º','˘', ___,___,'ß','Ø','¨','´',___,'®',
	'∂',___,___,'∑','é','è','í','Ä', ___,'ê',___,___,___,___,___,___,
	___,'•',___,___,___,'∏','ô',___, '≤',___,___,___,'ö',___,___,'û',
	'Ö','†','É','∞','Ñ','Ü','ë','á', 'ä','Ç','à','â','ç','°','å','ã',
	___,'§','ï','¢','ì','±','î','ˆ', '≥','ó','£','ñ','Å',___,___,'ò'
};

static const short _FONT_ExtLatin[256] = {
	0  ,___,'',___,___,___,___,'¯', 'Ò',___,___,___,___,___,___,___,
	___,___,___,___,___,___,___,___, ___,'|','Û','Ú','„',___,'ú','˘',
	' ','!','"','#','$','%','&', 39, '(',')','*','+',',','-','.','/',
	'0','1','2','3','4','5','6','7', '8','9',':',';','<','=','>','?',
	'@','A','B','C','D','E','F','G', 'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W', 'X','Y','Z','[', 92,']','^','_',
	'`','a','b','c','d','e','f','g', 'h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w', 'x','y','z','{','|','}','~','',
	'‡','·','‚','„','‰','Â','Ê','Á', 'Ë','È','Í','Î','Ï','Ì','Ó','Ô',
	'','©','Ú','Û','Ù','ı',UDF,'˜', '¥','µ','˙','˚','¸',UDF,'ü','ø',
	NBS,'≠','õ','ú','◊','ù','Ÿ','›', 'π','Ω','¶','Æ','™','-','æ','ˇ',
	'¯','Ò','˝','˛','∫','Ê','º','˘', '⁄','€','ß','Ø','¨','´','‹','®',
	'∂','¬','√','∑','é','è','í','Ä', 'ƒ','ê','≈','∆','«','»','…',' ',
	'À','•','Ã','Õ','Œ','∏','ô','ÿ', '≤','œ','–','—','ö','“','”','û',
	'Ö','†','É','∞','Ñ','Ü','ë','á', 'ä','Ç','à','â','ç','°','å','ã',
	'‘','§','ï','¢','ì','±','î','ˆ', '≥','ó','£','ñ','Å','’','÷','ò'
};


static struct {
	FONTFACE Font;
	char     Name[6];
} _FONT_Cursor = {
	{	NULL, NULL, 1, 0x00, 0, xTrue, xTrue, 10,
		16,16,  0,16, 16, 16,0, 0, 8, 0l,  0,16, 16, 16,0, 0,
		0, 153, 16,0,  NULL,  6,{'c'} },
	{	"ursor" }
};
FONTFACE  * _FONT_List  = &_FONT_Cursor.Font;
FONTALIAS * _FONT_Subst = NULL;
FONTALIAS * _FONT_Alias = NULL;


//------------------------------------------------------------------------------
static void
_save_face (FILE * file, FONTFACE * face)
{
	fputs (face->Name, file);
	fputs ("\n",       file);
	fprintf (file, "   %i, %i,%i, %i   %i,%i   %i,%i"
	         "   %i, %i,%i, %i,%i   %i, %i,%i, %i,%i\n",
	         face->Points, face->Width, face->Height, face->HalfLine,
	         face->MinChr, face->MaxChr, face->Ascent, face->Descent,
	         face->MinWidth, face->MinAsc, face->MinDesc,
	         face->MinLftBr, face->MinRgtBr,
	         face->MaxWidth, face->MaxAsc, face->MaxDesc,
	         face->MaxLftBr, face->MaxRgtBr);
}

//------------------------------------------------------------------------------
FONTFACE *
_Font_Create (const char * name, size_t len, unsigned type, BOOL sym, BOOL mono)
{
	FONTFACE * face = malloc (sizeof(FONTFACE) + len);
	if (face) {
		face->Next      = NULL;
		face->Type      = type;
		face->isSymbol  = sym;
		face->isMono    = mono;
		face->CharInfos = NULL;
		face->Length    = len;
		memcpy (face->Name, name, len +1);
	}
	return face;
}

//------------------------------------------------------------------------------
void
_Font_Bounds (FONTFACE * face, BOOL mono)
{
	short dist[5], width, minADE, maxADE, dmy[3];
	
	vqt_fontinfo (GRPH_Vdi, &minADE, &maxADE, dist, &width, dmy);
	face->HalfLine =  dist[2];
	face->Ascent   =  dist[4];
	face->MaxAsc   =  dist[3];
	face->MinAsc   =  dist[2];
	face->Descent  = (dist[0] ? dist[0] +1 : 0);
	face->MaxDesc  = (dist[1] ? dist[1] +1 : 0);
	face->MinDesc  =  0;
	face->MinChr   =  minADE;
	face->MaxChr   =  maxADE;
	vqt_width (GRPH_Vdi, '.', &dist[0], &dist[1], &dist[2]);
	face->MinWidth = dist[0] - dist[1] - dist[2];
	face->MinLftBr = dist[1];
	face->MinRgtBr = dist[0] - dist[2];
	if (mono) {
		face->MaxWidth = face->MinWidth;
		face->MaxLftBr = face->MinLftBr;
		face->MaxRgtBr = face->MinRgtBr;
	} else {
		vqt_width (GRPH_Vdi, 'W', &dist[0], &dist[1], &dist[2]);
		face->MaxWidth = dist[0] - dist[1] - dist[2];
	//	if (face->MaxWidth < width) face->MaxWidth = width;
		face->MaxLftBr = dist[1];
		face->MaxRgtBr = dist[0] - dist[2];
	}
	face->MinAttr = face->MaxAttr = 0;
	
	if ((!face->Type || face->Type >= 2) &&
	    (face->HalfLine >= face->Ascent || face->MaxDesc > face->Ascent /2)) {
		short hgt      = face->Ascent + face->Descent;
		short size     = hgt * GRPH_Depth;
		long  buf[size < 29 ? 29 : size];
		MFDB  mfdb     = { buf, 32, hgt, 2, 0, 1, 0,0,0 };
		short hdl      = GRPH_Handle;
		short w_in[20] = { 1, 0,0, 0,0, face->Index,G_BLACK, 0,0,0, 2,
		                   31,hgt -1, GRPH_muWidth, GRPH_muHeight, 0,0,0,0,0 };
		
		v_opnbm (w_in, &mfdb, &hdl, (short*)buf);
		if (hdl > 0) {
			short pxy[4] = { 0, 0, 31, hgt -1 };
			PXY   p      = { 14, 0 };
			
			vs_clip       (hdl, 1, pxy);
			vswr_mode     (hdl, MD_TRANS);
			vst_alignment (hdl, 1, 5, pxy, pxy);
			if (!face->Type) {
				vst_height (hdl, face->Height, pxy, pxy, pxy, pxy);
				vst_width  (hdl, face->Width,  pxy, pxy, pxy, pxy);
			} else {
				vst_point  (hdl, face->Points, pxy, pxy, pxy, pxy);
			}
			memset (buf, 0, sizeof(long) * hgt);
			
			if (face->MaxDesc > face->Ascent /2) {
				short txt[] = {'g','p','q','y'};
				short i     = sizeof(txt) /2;
				while (--i) v_gtext16n (hdl, p, &txt[i], 1);
				i = hgt;
				while (--i && !buf[i]);
				face->MaxDesc = i - face->Ascent +1;
			}
			if (face->HalfLine >= face->Ascent) {
				short txt[] = {'a','c','e','m','n','o','r','s','z'};
				short i     = sizeof(txt) /2;
				while (--i) v_gtext16n (hdl, p, &txt[i], 1);
				i = -1;
				while (++i < hgt && !buf[i]);
				face->HalfLine = face->MinAsc = face->Ascent - i;
			}
			if (face->MaxAsc >= face->Ascent) {
				short txt[] = {'A','O','T'};
				short i     = sizeof(txt) /2;
				while (--i) v_gtext16n (hdl, p, &txt[0], 1);
				i = -1;
				while (++i < hgt && !buf[i]);
				face->MaxAsc = face->Ascent - i;
			}
			v_clsbm (hdl);
		}
	}
}

//==============================================================================
void
FontInit (short count)
{
	struct FONT_DB {
		struct FONT_DB * next;
		short            id;
		short            cnt;
		FONTFACE       * list;
		char             file[1];
	} * font_db = NULL;
	
	FONTFACE ** list = &_FONT_List;
	FILE      * f_db;
	char        buf[258];
	int         i, j;
	
	while (*list) list = &(*list)->Next;
	
	/*--- read font.alias --*/
	
	if ((f_db = fopen (PATH_FontsAlias, "r"))) {
		FONTALIAS ** subst = &_FONT_Subst;
		FONTALIAS ** alias = &_FONT_Alias;
		while (fgets (buf, sizeof(buf), f_db)) {
			FONTALIAS * elem;
			size_t len_a, len_b;
			char * a = buf, * b;
			BOOL   is_subst;
			
			while (isspace(*a)) a++;
			if (!*a  ||  *a == '!') continue;
			
			if (!(len_a = strcspn (a, " \t:\r\n"))) break;
			b = a + len_a;
			while (isspace(*(b))) b++;
			if ((is_subst = (*b == ':'))) {
				while (isspace(*++b));
			}
			if (!(len_b = strcspn (b, " \t\r\n"))) break;
			
			if ((elem = malloc (sizeof(FONTALIAS) + len_a + len_b))) {
				char * p = elem->Pattern = elem->Name + len_a +1;
				while (len_b--) *(p++) = tolower (*(b++)); *p = '\0';
				p = elem->Name;
				while (len_a--) *(p++) = tolower (*(a++)); *p = '\0';
				if (is_subst) {
					*subst = elem;
					subst  = &elem->Next;
				} else {
					*alias = elem;
					alias  = &elem->Next;
				}
				elem->Next = NULL;
			} else {
				break;
			}
		}
		fclose (f_db);
		f_db = NULL;
	}
	
	/*--- read fonts.db ---*/
	
	if (   (access (PATH_LibDir, R_OK|W_OK|X_OK) &&
	        mkdir  (PATH_LibDir, S_IRWXU|S_IRWXG|S_IRWXO))
	    || (!access (PATH_FontsDb, F_OK) &&
	        (   access (PATH_FontsDb, W_OK)
	         || !(f_db = fopen (PATH_FontsDb, "r"))))) {
		printf ("  \33pERROR\33q: Can't acess /var/lib/.\n");
		return;
		
	} else {
		int major = -1, minor = -1, tiny = 0;
		if (!fgets (buf, sizeof(buf), f_db)
		    || sscanf (buf, "# fonts.db; %d.%d.%d ", &major, &minor, &tiny) < 2
		    || minor < 6 || (minor == 6  &&  tiny < 3)) {
			fclose (f_db);
			f_db = NULL;
		}
	}
	if (f_db) {
		FONTFACE * face = NULL, ** fptr = NULL;
		unsigned   type, isMono, isSymbol;
		char       c;
		while (fgets (buf, sizeof(buf), f_db)) {
			int len = strlen (buf);
			if (buf[len-1] == '\n') buf[--len] = '\0';
			
			if (sscanf (buf, "%i: %u,%u,%u%c%n",
			            &i, &type, &isSymbol, &isMono, &c,&j) == 5  &&  c == ' ') {
				struct FONT_DB * db;
				if (face) {
					printf ("A\n");
					break;
				} else if (!(db = malloc (sizeof(struct FONT_DB) + len - j))) {
					printf ("a\n");
					break;
				}
				db->next = font_db;
				db->id   = i;
				db->cnt  = 0;
				db->list = NULL;
				if ((len -= j -1) > 1) {
					memcpy (db->file, buf + j, len);
				} else {
					db->file[0] = '\0';
				}
				fptr    = &db->list;
				font_db = db;
			
			} else if (!font_db) {
				printf ("B\n");
				break;
			
			} else if (buf[0] == '-') {
				if (face) {
					printf ("C\n");
					break;
				} else if (!(face = _Font_Create (buf, len,
				                                  type, isSymbol, isMono))) {
					printf ("c\n");
					break;
				}
			
			} else if (!face) {
				printf ("D\n");
				break;
			
			} else if (sscanf (buf, "%i, %hi,%hi, %hi  %hi,%hi %hi,%hi"
			                   "%hi, %hi,%hi, %hi,%hi %hi, %hi,%hi, %hi,%hi",
					             &i, &face->Width, &face->Height, &face->HalfLine,
					             &face->MinChr, &face->MaxChr,
					             &face->Ascent, &face->Descent,
					             &face->MinWidth, &face->MinAsc, &face->MinDesc,
					             &face->MinLftBr, &face->MinRgtBr,
					             &face->MaxWidth, &face->MaxAsc, &face->MaxDesc,
					             &face->MaxLftBr, &face->MaxRgtBr) == 18) {
				face->CharSet = NULL;
				face->Index   = font_db->id;
				face->Effects = 0;
				face->Points  = i;
				*fptr = face;
				fptr  = &face->Next;
				face  = NULL;
				font_db->cnt++;
			
			} else {
				if (face) free (face);
				face = NULL;
				printf ("d\n");
				break;
			}
		}
		
		fclose (f_db);
	}
	
	/*--- scan VDI fonts ---*/
	
	if ((f_db = fopen (PATH_FontsDb, "w"))) {
		fprintf (f_db, "# fonts.db; %s\n", GLBL_Version);
	}
	printf ("  loaded %i font%s\n", count, (count == 1 ? "" : "s"));
	for (i = 1; i <= count; i++) {
		struct FONT_DB * db = font_db;
		char          _tmp[1000];
		VQT_FHDR    * fhdr = (VQT_FHDR*)_tmp;
		XFNT_INFO     info;
		const char  * fmly = info.family_name,
		            * wght = info.style_name,
		            * creg = "ISO8859",
		            * cenc = "1",
		            * fndr = NULL, * setw = NULL, * astl = NULL;
		char          slnt[3] = "\0\0", spcg[2] = "\0";
		unsigned      resx = 72, resy = 72;
		BOOL          isMono, isSymbol;
		const short * cset = NULL;
		BOOL          latn = xFalse;
		char        * p;
		short         type, dmy;
		
		vqt_ext_name (GRPH_Vdi, i, info.font_name, &dmy, &type);
		isMono   = (type & 0x01 ? 1 : 0);
		isSymbol = (type & 0x10 ? 1 : 0);
		
		info.size           = sizeof(info);
		info.font_name[0]   = '\0';
		info.family_name[0] = '\0';
		info.style_name[0]  = '\0';
		info.pt_cnt         = 0;
		vqt_xfntinfo (GRPH_Vdi, 0x010F, 0, i, &info);
		if (info.pt_cnt > 1  &&
		    info.pt_sizes[info.pt_cnt -1] == info.pt_sizes[info.pt_cnt -2]) {
			info.pt_cnt--;
		}
		
		if (!isSymbol) {
			if (info.format == 1
			    &&  info.font_name[0] =='›'&&  info.font_name[1] =='-') {
				latn = xTrue;
				cset = _FONT_ExtLatin;
			} else {
				cset = _FONT_TosLatin;
			}
		}
		
		while (db) {
			if (db->id == info.id  &&  !strcmp (db->file, info.file_name1)) {
				if (db->cnt != info.pt_cnt) {
					db = NULL;
				} else for (j = 0; j < info.pt_cnt; ++j) {
					FONTFACE * face = db->list;
					while (face  &&  face->Points != info.pt_sizes[j]) {
						face = face->Next;
					}
					if (!face) {
						db = NULL;
						break;
					}
				}
				break;
			}
			db = db->next;
		}
		
		if (f_db) fprintf (f_db, "%i: %u,%u,%u %s\n", info.id,
		                   info.format, isSymbol, isMono, info.file_name1);
		
		if (db && db->list) {
			*list = db->list;
			do {
				if (f_db) _save_face (f_db, *list);
				(*list)->CharSet = cset;
			} while (*(list = &(*list)->Next));
			db->list = NULL;
			
			continue;
		}
		
		vst_font (GRPH_Vdi, info.id);
		
		if (info.format > 1) {
			vqt_fontheader (GRPH_Vdi, (char*)fhdr, info.file_name1);
			spcg[0] = (fhdr->fh_cflgs & 2 ? fhdr->fh_famcl == 3 ? 'C' : 'M' : 'P');
			slnt[0] = (fhdr->fh_cflgs & 1 ? 'I' : 'R');
			resx = 0;
			resy = 0;
		}
		if (info.format == 2) { //___Speedo___
			if (!strncasecmp (fmly, "Bits ", 5)) {
				fndr =  "Bitstream";
				fmly += 5;
			} else if ((p = strstr (fhdr->fh_cpyrt, "opyright"))) {
				if ((p = strstr (p, "by "))) {
					fndr = p +3;
					if ((p = strchr (fndr, ' '))) *p = '\0';
				}
			}
			if (fhdr->fh_cflgs & 0x04  ||  fhdr->fh_famcl == 1) {
				astl = "Serif";
			} else if (fhdr->fh_famcl == 2) {
				astl = "Sans";
			} else if (fhdr->fh_famcl == 4) {
				astl = "Script";
			} else if (fhdr->fh_famcl == 5) {
				astl = "Decorated";
			}
			switch (fhdr->fh_frmcl >> 4) {
				case  1: wght = "Thin";       break;
				case  2: wght = "UltraLight"; break;
				case  3: wght = "ExtraLight"; break;
				case  4: wght = "Light";      break;
				case  5: wght = "Book";       break;
				case  6: //wght = "Normal";     break;
				case  7: wght = "Medium";     break;
				case  8: wght = "Semibold";   break;
				case  9: wght = "Demibold";   break;
				case 10: wght = "Bold";       break;
				case 11: wght = "ExtraBold";  break;
				case 12: wght = "UltraBold";  break;
				case 13: wght = "Heavy";      break;
				case 14: wght = "Black";      break;
			}
			switch (fhdr->fh_frmcl & 0x0F) {
				case  4: setw = "Condensed";     break;
				case  6: setw = "SemiCondensed"; break;
				case  8: setw = "Normal";        break;
				case 10: setw = "SemiExpanded";  break;
				case 12: setw = "Expanded";      break;
			}
			
		} else if (latn) {
			do {
				fndr = info.font_name +2;
				if (!(p = strchr (fndr, '-'))) break;
				*(p++) = '\0';
				fmly   = p;
				if (!(p = strchr (fmly, '-'))) break;
				*p = '\0';
				if (!*(++p)) break;
				if (*p && *p != '-') switch (*(p++)) {
					case 'M': wght = "Medium"; break;
					case 'B': wght = "Bold";   break;
				}
				if (*(p++) != '-') break;
				switch (*p) {
					case 'R':
					case 'I': slnt[0] = *(p++); break;
				}
				if (*(p++) != '-') break;
				if (*p && *p != '-') switch (*(p++)) {
					case 'C': setw = "Condensed";     break;
					case 'c': setw = "SemiCondensed"; break;
					case 'N': setw = "Normal";        break;
					case 'e': setw = "SemiExpanded";  break;
					case 'E': setw = "Expanded";      break;
				}
				if (*(p++) != '-') break;
				if (*p != '-') {
					p++;   // AddStyle
				}
				if (*(p++) != '-') break;
				switch (*p) {
					case 'C':
					case 'P':
					case 'M': spcg[0] = *(p++); break;
				}
				if (*(p++) != '-') break;
				if (*p && *p != '-') switch (*(p++)) {
					case 'I': creg = "ISO8859";                   break;
					case '!': creg = fndr; cenc = "FONTSPECIFIC"; break;
				}
				if (*(p++) != '-') break;
				switch (*p) {
					case '1'...'9': cenc = p++; *p = '\0'; break;
					case 'F':       cenc = "FONTSPECIFIC"; break;
				}
			} while(0);
			resx = 75;
			resy = 75;
			
		} else {
			if (!*spcg) {
				spcg[0] = (isMono ? 'C' : 'P');
			}
			if (isSymbol) {
				creg = "Gdos";
				cenc = "FONTSPECIFIC";
			}
			if (!*fmly) {
				fmly = info.font_name;
			}
			if ((p = strstr (fmly, " sans")) || (p = strstr (fmly, " Sans"))) {
				strcpy (p, p +5);
				astl = "Sans";
			}
			if (!strcasecmp (wght, "Regular")) {
				wght = "Medium";
				if (!*slnt) slnt[0] = 'R';
			} else {
				if ((p = strstr(wght, "italic")) || (p = strstr(wght, "Italic"))) {
					strcpy ((p[-1] == ' ' ? p -1 : p), p +6);
					if (!*slnt) slnt[0] = 'I';
				}
			}
			if (info.format == 1) {
				if (!*slnt)          slnt[0] = '*';
				if (!wght || !*wght) wght    = "*";
			} else {
				if (!*slnt)          slnt[0] = 'R';
				if (!wght || !*wght) wght    = "Medium";
			}
			if (!setw) setw = "Normal";
		}
		
		if (fmly) {
			while ((p = strchr (fmly, ' '))) strcpy (p, p +1);
		}
		
		sprintf (info.file_name1,
		         "-%s-%s-%s-%s-%s-%s-%%u-%%u0-%u-%u-%s-%%u-%s-%s",
		         (fndr ? fndr : ""), (fmly ? fmly : ""), (wght ? wght : ""),
		         slnt, (setw ? setw : ""), (astl ? astl : ""),
		         resx, resy, spcg, creg, cenc);
		for (j = 0; j < info.pt_cnt; ++j) {
			int   len, avrg;
			short wdth, pxsz;
			vst_point (GRPH_Vdi, info.pt_sizes[j], &dmy,&dmy, &wdth, &pxsz);
			
			if (spcg[0] == 'P') {
				short ext[8];
				vqt_extent16n (GRPH_Vdi, _FONT_Proto, sizeof(_FONT_Proto) /2, ext);
				avrg = ((ext[4] - ext[0]) *10) / (sizeof(_FONT_Proto) /2);
			} else {
				avrg = wdth * 10;
			}
			len = sprintf (fhdr->fh_fmver, info.file_name1,
			               pxsz, info.pt_sizes[j], avrg);

			if ((*list = _Font_Create (fhdr->fh_fmver, len,
			                           info.format, isSymbol, isMono))) {
				FONTFACE * face = *list;
				face->CharSet = cset;
				face->Index   = info.id;
				face->Effects = 0;
				face->Points  = info.pt_sizes[j];
				face->Height  = pxsz;
				face->Width   = wdth;
				if (face->Type == 4) {
					// For determineing TrueType, finish the startup intro and
					// unlock the screen to avoid NVDI producing a deadlock while
					// possible reporting damaged fonts in console window.
					//
					WmgrIntro (xFalse);
					graf_mouse (BUSYBEE, NULL);
				}
				_Font_Bounds (face, (spcg[0] != 'P'));
				if (f_db) _save_face (f_db, face);
				list = &face->Next;
			}
		}
	}
	
	if (f_db) fclose (f_db);
	
	while (font_db) {
		struct FONT_DB * db = font_db;
		while (db->list) {
			FONTFACE * face = db->list;
			db->list = face->Next;
			free (face);
		}
		font_db = db->next;
		free (db);
	}
}
