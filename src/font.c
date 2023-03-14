//==============================================================================
//
// font.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-15 - Initial Version.
//==============================================================================
//
#include "font_P.h"
#include "Atom.h"

#include <stdlib.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <string.h>
#include <fnmatch.h>
#include <ctype.h>

#include <X11/X.h>


static CARD8 _FONT_AscDesc[256] = { // ISO-coded
#	define SET(a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p) \
				0x##a, 0x##b, 0x##c, 0x##d, 0x##e, 0x##f, 0x##g, 0x##h, \
				0x##i, 0x##j, 0x##k, 0x##l, 0x##m, 0x##n, 0x##o, 0x##p
	SET ( 1, 1, 1, 1, 1, 1, 1,34, 32, 1, 1, 1, 1, 1, 1, 1),
	SET ( 1, 1, 1, 1, 1, 1, 1, 1,  1,31,31,31,21, 1,31,23),
	SET (31,31,34,32,31,31,31,34, 31,31,22,22,10,23,11,31),
	//    0  1  2  3  4  5  6  7   8  9  :  ;  <  =  >  ?
	SET (31,31,31,31,31,31,31,31, 31,31,21,20,31,22,31,31),
	//    @  A  B  C  D  E  F  G   H  I  J  K  L  M  N  O
	SET (31,31,31,31,31,31,31,31, 31,31,31,31,31,31,31,31),
	//    P  Q  R  S  T  U  V  W   X  Y  Z  [  \  ]  ^  _
	SET (31,31,31,31,31,31,31,31, 31,31,31,31,31,31,34,00),
	//    `  a  b  c  d  e  f  g   h  i  j  k  l  m  n  o
	SET (34,21,31,21,31,21,31,20, 31,31,30,31,31,21,21,21),
	//    p  q  r  s  t  u  v  w   x  y  z  {  |  }  ~  
	SET (20,20,21,21,31,21,21,21, 21,20,21,31,31,31,34,21),
	SET (21,30,31,21,31,31,20,21, 31,31,31,31,31,31,31,31),
	SET (32,23,31,31,30,40, 1,22, 21,31,11,31,32, 1,30,33),
	SET (31,31,21,31,32,31,31,31, 44,31,32,31,23,23,31,34),
	SET (34,32,33,33,34,20,31,23, 00,33,33,31,31,31,31,31),
	SET (41,41,41,41,41,41,31,30, 41,41,41,41,41,41,41,41),
	SET (31,41,41,41,41,41,41,32, 31,41,41,41,41,41,31,30),
	SET (41,41,41,41,41,41,21,20, 41,41,41,41,41,41,41,41),
	SET (31,41,41,41,41,41,41,21, 21,41,41,41,41,40,30,40)
#	undef SET
};


//==============================================================================
void
FontDelete (p_FONT font, p_CLIENT clnt)
{
	XrscDelete (clnt->Fontables, font);
}

//------------------------------------------------------------------------------
static inline
p_FONT FontFind (CARD32 id) {
	p_FONT font = FablFind(id).Font;
	if ((DBG_XRSC_TypeError = (font && !font->isFont))) font = NULL;
	return font;
}


//==============================================================================
BOOL
FontValues (p_FONTABLE fabl, CARD32 id)
{
	p_FONTABLE font;
	BOOL       ok = xTrue;
	
	if (!id || !(ok = ((font.Font = FontFind (id))) != NULL)) {
		fabl.p->FontFace    = NULL;
		fabl.p->FontIndex   = 1;
		fabl.p->FontEffects = 0;
		fabl.p->FontPoints  = 10;
	} else {
		FontCopy (fabl, font);
	}
	return ok;
}


//==============================================================================
short *
FontTrans_C (short * arr, const char * str, int len,
             const struct s_FONTFACE * face)
{
	const short * t = face->CharSet;
	short       * a = arr;
	if (!t) {
		while (len-- > 0) {
			*(arr++) = *(str++);
		}
	} else {
		while (len-- > 0) {
			*(arr++) = t[(int)*(str++)];
		}
	}
	return a;
}

//==============================================================================
short *
FontTrans_W (short * arr, const short * str, int len,
             const struct s_FONTFACE * face)
{
	const short * t = face->CharSet;
	short       * a;
	if (!t) {
		(const short*)a = str;
	
	} else {
		a = arr;
		while (len--) {
			*(arr++) = t[*(str++) & 0xFF];
		}
	}
	return a;
}


//------------------------------------------------------------------------------
static void
_Font_Alias (char * buf, const char * src, size_t len)
{
	FONTALIAS * alias = _FONT_Subst;
	int i;
	
	for (i = 0; i < len; i++) {
		buf[i] = tolower (src[i]);
	}
	buf[len] = '\0';
	
	while (alias) {
		char * ptr = strstr (buf, alias->Name);
		if (ptr) {
			size_t n_len = strlen (alias->Name);
			size_t p_len = strlen (alias->Pattern);
			int    diff  = p_len - n_len;
			if (diff > 0) {
				memmove (ptr + p_len, ptr + n_len, len - (ptr - buf) - n_len +1);
			} else if (diff < 1) {
				strcpy (ptr + p_len, ptr + n_len);
			}
			memcpy (ptr, alias->Pattern, p_len);
			if ((len += diff) > 255) {
				len      = 255;
				buf[len] = '\0';
			}
		}
		alias = alias->Next;
	}
	
	alias = _FONT_Alias;
	while (alias) {
		if (!strcasecmp (buf, alias->Name)) {
			strcpy (buf, alias->Pattern);
			break;
		}
		alias = alias->Next;
	}
}

//------------------------------------------------------------------------------
static FONTFACE **
_Font_Match (const char * pattern, FONTFACE * start)
{
	FONTFACE ** face = (start ? &start->Next : &_FONT_List);
	
	while (*face) {
		if (!fnmatch (pattern, (*face)->Name, FNM_NOESCAPE|FNM_CASEFOLD)) {
			break;
		}
		face = &(*face)->Next;
	}
	return face;
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ListFonts (CLIENT * clnt, xListFontsReq * q)
{
	// Returns a list of available font names that matches the pattern.
	//
	// CARD16 maxNames: number of maximum names to return
	// CARD16 nbytes:   length of pattern
	char * patt = (char*)(q+1); // search pattern
	//
	// Reply:
	// CARD16 nFonts: number of names found
	//...........................................................................
   
	ClntReplyPtr (ListFonts, r,);
	FONTFACE  * face = _FONT_List->Next; // skip cursor font
	char      * list = (char*)(r +1);
	CARD16      num  = 0;
	size_t      size = 0;
	size_t      have = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
	                 - sz_xListFontsReply;
	char        buf[500] = "";
	
	PRINT (ListFonts," '%.*s' max=%u",
	       q->nbytes, (char*)(q +1), q->maxNames);
	
	_Font_Alias (buf, patt, q->nbytes);
	
	while (face  &&  num < q->maxNames) {
		if (!fnmatch (buf, face->Name, FNM_NOESCAPE|FNM_CASEFOLD)) {
			size_t len  = face->Length +1;
			size_t need = size + len;
			if (need > have) {
				void * rr = ClntOutBuffer (&clnt->oBuf,
				                           sz_xListFontsReply + need,
				                           sz_xListFontsReply + size, xFalse);
				if (!rr) break;
				r    = rr;
				list = (char*)(r +1);
				have = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
	              - sz_xListFontsReply;
			}
			memcpy (list + size, &face->Length, len);
			size = need;
			num++;
		}
		face = face->Next;
	}
	r->nFonts = num;
	ClntReply (ListFonts, size, ".");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_OpenFont (CLIENT * clnt, xOpenFontReq * q)
{
	// Associates identifier with a font.
	//
	// Font   fid:
	// CARD16 nbytes: length of pattern
	char * patt = (char*)(q+1); // search pattern
	//...........................................................................
   
   if (FablFind (q->fid).p) {
		Bad(IDChoice, q->fid, OpenFont,);
	
	} else { //..................................................................
		
		FONTFACE  * face;
		FONT      * font;
		unsigned    w, h;
		char        buf[500] = "";

		PRINT (OpenFont," F:%lX '%.*s'", q->fid, q->nbytes, patt);
		
		_Font_Alias (buf, patt, q->nbytes);
		face = *_Font_Match (buf, NULL);
		
		if (!face  &&  sscanf (patt, "%ux%u", &w, &h) == 2) {
			sprintf (buf, "*-%u-*-*-*-C-%u0-ISO8859-1", h, w);
			face = *_Font_Match (buf, NULL);
			
			if (!face) {
				FONTFACE ** prot = &_FONT_List;
				sprintf (buf, "*-Medium-*-*-*-%u-*-*-*-C-*-ISO8859-1", h);
				while (*(prot = _Font_Match (buf, *prot)) &&
				       ((*prot)->Type < 2 || (*prot)->isSymbol));
				if (!*prot) {
					prot = &_FONT_List;
					while (*prot && ((*prot)->Type < 2
					       || !(*prot)->isMono || (*prot)->isSymbol)) {
						prot = &(*prot)->Next;
					}
				}
				if (*prot) {
					short i, dmy[3];
					face = _Font_Create (patt, strlen(patt), 0, xFalse, xTrue);
					if (!face) {
						Bad(Alloc,, OpenFont," (generic)");
						return;
					}
					face->CharSet = (*prot)->CharSet;
					face->Index   = (*prot)->Index;
					face->Effects = 0;
					face->Points  = 0;
					vst_font (GRPH_Vdi, face->Index);
					i = 0;
					while (i < h) {
						short c_h;
						vst_height (GRPH_Vdi, h - i, dmy, dmy, dmy, &c_h);
						if (c_h <= h) {
							face->Height = h - i;
							break;
						}
						i++;
					}
					i = 0;
					while (1) {
						short c_w;
						vst_width  (GRPH_Vdi, w + i, dmy,dmy, &c_w, dmy);
						if (c_w >= w) {
							face->Width = w + i;
							break;
						}
						i++;
					}
					vst_width  (GRPH_Vdi, face->Width, dmy,dmy, dmy,dmy);
					_Font_Bounds (face, xTrue);
					face->Next = *prot;
					*prot      = face;
				}
			}
		}
		if (!face) {
			Bad(Name,, OpenFont,"('%.*s')\n          -> '%s'",
			           (int)q->nbytes, patt, buf);
		
		} else if (!(font = XrscCreate (FONT, q->fid, clnt->Fontables,))) {
			Bad(Alloc,, OpenFont,);
		
		} else {
			font->isFont = xTrue;
			
			if (!face) {
				FontValues ((p_FONTABLE)font, None);
			
			} else {
				font->FontFace    = face;
				font->FontIndex   = face->Index;
				font->FontEffects = face->Effects;
				if (face->Type) {
					font->FontPoints  = face->Points;
					font->FontWidth   = 0;
				} else {
					font->FontPoints  = face->Height;
					font->FontWidth   = face->Width;
				}
			}
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_CloseFont (CLIENT * clnt, xCloseFontReq * q)
{
	// Deletes association between resource ID ans font.
	//
	// CARD32 id: font
	//
	//...........................................................................
	
	FONT * font = FontFind (q->id);
	
	if (!font) {
		Bad(Font, q->id, CloseFont,);
	
	} else { //..................................................................
	
		DEBUG (CloseFont," F:%lX", q->id);
		
		FontDelete (font, ClntFind (q->id));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_QueryFont (CLIENT * clnt, xQueryFontReq * q)
{
	// Returns logical informations about a font
	//
	// CARD32 id: font
	//
	// Reply:
	// xCharInfo minBounds, maxBounds
	// CARD16    minCharOrByte2, maxCharOrByte2
	// CARD8     minByte1, maxByte1
	// INT16     fontAscent, fontDescent
	// CARD16    defaultChar
	// CARD8     drawDirection
	// BOOL      allCharsExist
	// CARD16    nFontProps:     followed by this many xFontProp structures
	// CARD32    nCharInfos:     followed by this many xCharInfo structures
	//...........................................................................
	
	FONT * font = FontFind (q->id);
	
	if (!font) {
		Bad(Font, q->id, QueryFont,);
	
	} else { //..................................................................
	
		FONTFACE *  face  = font->FontFace;
		size_t      size  = 0;
		CARD32      n_inf = face->MaxChr - face->MinChr +1;
		xCharInfo * info;
		short       asc[5], desc[5];
		ClntReplyPtr (QueryFont, r,
		              (sizeof(Atom) *2) + (n_inf * sizeof(xCharInfo)));
		
		PRINT (QueryFont," F:%lX", q->id);
		
		memcpy (&r->minBounds, &face->MinLftBr, (sizeof(xCharInfo) *2) +4);
		r->minCharOrByte2 = face->MinChr;
		r->maxCharOrByte2 = face->MaxChr;
		r->defaultChar    = ' ';
		r->drawDirection  = FontLeftToRight;
		r->minByte1       = 0;
		r->maxByte1       = 0;
		r->allCharsExist  = xTrue;
		r->fontAscent     = face->Ascent;
		r->fontDescent    = face->Descent;
		r->nFontProps     = 0;
		
		while (1) {
			Atom * atom = (Atom*)(r +1);
			if (!(*(atom++) = AtomGet ("FONT", 4, xFalse)) ||
			    !(*(atom++) = AtomGet (face->Name, face->Length, xFalse))) break;
			r->nFontProps++;
			size += sizeof(Atom) *2;
			break;
		}
		if (r->nFontProps && clnt->DoSwap) {
			Atom * a = (Atom*)(r +1);
			int    n = r->nFontProps;
			while (n--) {
				*a = Swap16(*a);
				a++;
			}
		}
		info          = (xCharInfo*)((char*)(r +1) + size);
		r->nCharInfos = n_inf;
		size         += r->nCharInfos * sizeof(xCharInfo);
		if (face->isSymbol) {
			asc[0]  = asc[1]  = asc[2]  = asc[3]  = asc[4]  = face->MaxAsc;
			desc[0] = desc[1] = desc[2] = desc[3] = desc[4] = face->MaxDesc;
		} else {
			asc[0] = 0;
			asc[1] = face->HalfLine /3;
			asc[2] = face->HalfLine;
			asc[3] = face->MaxAsc;
			asc[4] = face->Ascent;
			desc[0] = face->MaxDesc;
			desc[1] = 0;
			desc[2] = -(face->HalfLine /3);
			desc[3] = -((face->HalfLine +1) /2);
			desc[4] = -face->HalfLine;
		}
		if (face->isMono) {
			xCharInfo * p = info;
			short c;
			for (c = face->MinChr; c <= face->MaxChr; c++) {
				p->leftSideBearing  = face->MaxLftBr;
				p->rightSideBearing = face->MaxRgtBr;
				p->characterWidth   = face->MaxWidth;
				p->ascent           = asc [_FONT_AscDesc[c] >> 4];
				p->descent          = desc[_FONT_AscDesc[c] &  0x0F];
				p->attributes       = 0;
				p++;
			}
		
		} else if (face->CharInfos) {
			memcpy (info, face->CharInfos, size);
		
		} else {
			xCharInfo * p = info;
			short c, ld, rd, w;
			vst_font    (GRPH_Vdi, face->Index);
			vst_effects (GRPH_Vdi, face->Effects);
			vst_point   (GRPH_Vdi, face->Points, &c, &c, &c, &c);
			for (c = face->MinChr; c <= face->MaxChr; ++c) {
				vqt_width (GRPH_Vdi, (face->CharSet ? face->CharSet[c] : c),
				           &w, &ld, &rd);
				p->leftSideBearing  = ld;
				p->rightSideBearing = w - rd;
				p->characterWidth   = w;
				p->ascent           = asc [_FONT_AscDesc[c] >> 4];
				p->descent          = desc[_FONT_AscDesc[c] &  0x0F];
				p->attributes       = 0;
				p++;
			}
			if ((face->CharInfos = malloc (size))) {
				memcpy (face->CharInfos, info, size);
			}
		}
		if (clnt->DoSwap) {
			short * p = (short*)info;
			int     n = size /2;
			while (n--) {
				*p = Swap16(*p);
				p++;
			}
		}
		
		ClntReply (QueryFont, size, ":::4:::4::4:l");
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_QueryTextExtents (CLIENT * clnt, xQueryTextExtentsReq * q)
{
	// BOOL oddLength: True if last char2b is unused
	// Font fid:
	CARD8 * text = (CARD8*)(q +1);
	//
	// Reply:
	// CARD8  drawDirection:
	// INT16  fontAscent, fontDescent:
	// INT16  overallAscent, overallDescent:
	// INT32  overallWidth, overallLeft, overallRight:
	//...........................................................................
	
	p_FONTABLE fabl = FablFind (q->fid);
	
	if (!fabl.p) {
		Bad(Font, q->fid, QueryTextExtents,);
	
	} else { //..................................................................
	
		FONTFACE * face = fabl.p->FontFace;
		size_t     size = ((q->length *4) - sizeof(xQueryTextExtentsReq))
		                - (q->oddLength ? 1 : 0);
		short str[size];
		short ext[8];
		ClntReplyPtr (QueryTextExtents, r,);
		
		PRINT (QueryTextExtents,"('%*s') F:%lX", (int)size, text, q->fid);
		
		vst_font    (GRPH_Vdi, fabl.p->FontIndex);
		vst_effects (GRPH_Vdi, fabl.p->FontEffects);
		if (fabl.p->FontWidth) {
			vst_height (GRPH_Vdi, fabl.p->FontPoints, ext, ext, ext, ext);
			vst_width  (GRPH_Vdi, fabl.p->FontWidth,  ext, ext, ext, ext);
		} else {
			vst_point  (GRPH_Vdi, fabl.p->FontPoints, ext, ext, ext, ext);
		}
		vqt_extent16n (GRPH_Vdi, FontTrans_C (str, text, size, face), size, ext);
		r->drawDirection  = FontLeftToRight;
		r->fontAscent     = face->Ascent;
		r->fontDescent    = face->Descent;
		r->overallAscent  = face->MaxAsc;
		r->overallDescent = face->MaxDesc;
		r->overallWidth   = ext[4] - ext[0];
		r->overallLeft    = face->MinLftBr; 
		r->overallRight   = face->MaxRgtBr;
		
		ClntReply (QueryTextExtents,, "PPlll");
	}
}


//------------------------------------------------------------------------------
void
RQ_ListFontsWithInfo (CLIENT * clnt, xListFontsWithInfoReq * q)
{
	PRINT (- X_ListFontsWithInfo," '%.*s' max=%u",
	       q->nbytes, (char*)(q +1), q->maxNames);
}


//------------------------------------------------------------------------------
void
RQ_SetFontPath (CLIENT * clnt, xSetFontPathReq * q)
{
	PRINT (- X_SetFontPath," (%u)", q->nFonts);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetFontPath (CLIENT * clnt, xGetFontPathReq * q)
{
	// Returns the search path for fonts.  Due to the lack of own font
	// management, simply the VDI's default font directory is returned.
	//
	// Reply:
	// CARD16 nPaths: number of diretorys
	
	const char * src = "/c/gemsys";
	ClntReplyPtr (GetFontPath, r, sizeof(src) -1);
	char       * dst = (char*)(r +1);
	
	PRINT (- X_GetFontPath," ");
	
	r->nPaths = 1;
	*dst      = (char)(sizeof(src) -1);
	strncpy (dst +1, src, sizeof (src) -1);
	
	ClntReply (GetFontPath, sizeof(src) -1, ".");
}
