//==============================================================================
//
// keyboard.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-15 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "clnt.h"
#include "tools.h"
#include "keyboard.h"
#include "window_P.h"
#include "event.h"
#include "x_gem.h"
#include "x_mint.h"
#include "wmgr.h" // for debugging only

#include <stdio.h>
#include <mint/ssystem.h>

#include <X11/Xproto.h>

#define XK_PUBLISHING
#define XK_TECHNICAL
#include <X11/keysym.h>


#define KEYSYM_OFFS   8
#define KEYCODE(s)    (s + KEYSYM_OFFS)

#define KC_SHFT_R   0x36 // 54
#define KC_SHFT_L   0x2A // 42
#define KC_CTRL     0x1D // 29
#define KC_ALT      0x38 // 56
#define KC_LOCK     0x3A // 58
#define KC_ALTGR    0x00 //

static CARD8 KYBD_Static[][2] = {      // assignment of keycodes to the bits
	{ KEYCODE(KC_SHFT_R),ShiftMask   }, // of the mask returned from Kbshift()
	{ KEYCODE(KC_SHFT_L),ShiftMask   }, // and VDI.  AltGr is remapped from bit
	{ KEYCODE(KC_CTRL),  ControlMask }, // 7 to bit 5.
	{ KEYCODE(KC_ALT),   Mod1Mask    },
	{ KEYCODE(KC_LOCK),  LockMask    },
	{ KEYCODE(KC_ALTGR), Mod2Mask    },
	{ 0, }                      // <-- this one is used to hold a pending keycode
};                             //     to generate a KeyRelease event
CARD8 * KYBD_Pending = KYBD_Static[numberof(KYBD_Static)-1];

static KeyCode KYBD_ModMap[8][2];
static KeySym  KYBD_Symbol[140][4];
static CARD8   KYBD_Set[256];
static CARD8   KYBD_Rep[32];

const CARD8 KYBD_CodeMin = KEYSYM_OFFS;
CARD8       KYBD_CodeMax;
CARD8       KYBD_PrvMeta = 0;
CARD16      KYBD_Repeat;

#define K_ENGLISH   AESLANG_ENGLISH
#define K_GERMAN    AESLANG_GERMAN
#define K_FRENCH    AESLANG_FRENCH
#define K_SWEDISH   AESLANG_SWEDISH
static char KYBD_Lang  = K_ENGLISH;
static BOOL KYBD_Atari = xFalse;

typedef struct {
	CARD8 scan, asc;
} KEYPAIR;

static struct { 
	CARD8   * unShift, * Shift,   * CapsLck;
	KEYPAIR * Alt,     * AltShft, * AltCaps, * AltGr;
} * KYBD_Table = NULL;
	

typedef struct {
	CARD16 scan;
	CARD8  col;
	CARD8  chr;
	KeySym sym;
} SCANTAB;


static KeySym Tos2Iso[256] = {

#	define NO_SYM(_) NoSymbol
	
	// 0 .. 31
	NoSymbol, XK_Up,        XK_Down,  XK_Right,    XK_Left,  NoSymbol, NoSymbol,
	NoSymbol, XK_BackSpace, XK_Tab,   XK_Linefeed, XK_Clear, NoSymbol, XK_Return,
	NoSymbol, NoSymbol,     NoSymbol, NoSymbol,    NoSymbol, NoSymbol, NoSymbol,
	NoSymbol, NoSymbol,     NoSymbol, NoSymbol,    NoSymbol, NoSymbol, XK_Escape,
	NoSymbol, NoSymbol,     NoSymbol, NoSymbol,
	
	// 32 .. 47
	XK_space,    XK_exclam,    XK_quotedbl,   XK_numbersign, XK_dollar,
	XK_percent,  XK_ampersand, XK_apostrophe, XK_parenleft,  XK_parenright,
	XK_asterisk, XK_plus,      XK_comma,      XK_minus,      XK_period,
	XK_slash,
	// 48 .. 57
	XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9,
	// 58 .. 64
	XK_colon, XK_semicolon, XK_less, XK_equal, XK_greater, XK_question, XK_at,
	// 65 .. 90
	XK_A, XK_B, XK_C, XK_D, XK_E, XK_F, XK_G, XK_H, XK_I, XK_J, XK_K, XK_L, XK_M,
	XK_N, XK_O, XK_P, XK_Q, XK_R, XK_S, XK_T, XK_U, XK_V, XK_W, XK_X, XK_Y, XK_Z,
	// 91 .. 96
	XK_bracketleft, XK_backslash, XK_bracketright, XK_asciicircum, XK_underscore,
	XK_grave,
	// 97 .. 122
	XK_a, XK_b, XK_c, XK_d, XK_e, XK_f, XK_g, XK_h, XK_i, XK_j, XK_k, XK_l, XK_m,
	XK_n, XK_o, XK_p, XK_q, XK_r, XK_s, XK_t, XK_u, XK_v, XK_w, XK_x, XK_y, XK_z,
	// 123 .. 127
	XK_braceleft, XK_bar, XK_braceright, XK_asciitilde, XK_Delete,
	
	// 128 .. 147 -- Non-ASCII
	XK_Ccedilla,   XK_udiaeresis, XK_eacute,      XK_acircumflex, XK_adiaeresis,
	XK_agrave,     XK_aring,      XK_ccedilla,    XK_ecircumflex, XK_ediaeresis,
	XK_egrave,     XK_idiaeresis, XK_icircumflex, XK_igrave,      XK_Adiaeresis,
	XK_Aring,      XK_Eacute,     XK_ae,          XK_AE,          XK_ocircumflex,
	// 148 .. 165
	XK_odiaeresis, XK_ograve,     XK_ucircumflex, XK_ugrave,      XK_ydiaeresis,
	XK_Odiaeresis, XK_Udiaeresis, XK_cent,        XK_sterling,    XK_yen,
	XK_ssharp,     XK_FFrancSign, XK_aacute,      XK_iacute,      XK_oacute,
	XK_uacute,     XK_ntilde,     XK_Ntilde,
	// 166 .. 184
	XK_ordfeminine,   XK_masculine,      XK_questiondown, NO_SYM('©'),
	NO_SYM('ª'),      XK_onehalf,        XK_onequarter,   XK_exclamdown,
	XK_guillemotleft, XK_guillemotright, XK_atilde,       XK_otilde,
	XK_Ooblique,      XK_oslash,         XK_oe,           XK_OE,
	XK_Agrave,        XK_Atilde,         XK_Otilde,
	// 185 .. 193
	XK_diaeresis, XK_acute,                                  NO_SYM('»'),
	XK_paragraph, XK_copyright, XK_registered, XK_trademark,
	NO_SYM('À'),  NO_SYM('Á'),
	// 194 .. 220
	NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol,
	NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol,
	NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol,
	NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol, NoSymbol,
	// 221 .. 223
	XK_section, XK_logicaland, XK_infinity,
	// 224 .. 235
	XK_Greek_alpha,   XK_Greek_beta,  XK_Greek_GAMMA, XK_Greek_pi,
	XK_Greek_EPSILON, XK_Greek_sigma, XK_Greek_mu,    XK_Greek_rho,
	XK_Greek_PHI,     XK_Greek_THETA, XK_Greek_OMEGA, XK_Greek_delta,
	// 235 .. 
	NO_SYM('ì'),    XK_Greek_phi,      XK_includedin,       XK_intersection,
	NO_SYM('ð'),    XK_plusminus,      XK_greaterthanequal, XK_lessthanequal,
	XK_topintegral, XK_botintegral,    XK_division,         XK_similarequal,
	XK_degree,      XK_periodcentered, NO_SYM('ú'),         NO_SYM('û'),
	NO_SYM('ü'),    XK_twosuperior,    XK_threesuperior,    XK_macron
	
#	undef NO_SYM
};


//------------------------------------------------------------------------------
static void
set_sym (KeySym line[], int col, KeySym sym)
{
	line += col;
	*line = sym;
	if (!(col & 1)) {
		col = 4 - col;
		while (--col > 0) *(++line) = NoSymbol;
	}
}

//------------------------------------------------------------------------------
static void
set_map (SCANTAB * tab, size_t num)
{
	while (num--) {
		KeySym * k = &KYBD_Symbol[tab->scan][tab->col];
		if (*k  &&  *k != XK_VoidSymbol) {
			printf ("    * can't map code %i:%i to %04lX '%.1s', is %04lX \n",
			        tab->scan, tab->col, tab->sym,
			        (tab->chr >= ' ' ? (char*)&tab->chr : ""), *k);
		} else {
			set_sym (KYBD_Symbol[tab->scan], tab->col, tab->sym);
			if (tab->chr) {
				KYBD_Set[tab->chr] = tab->scan;
			}
 		}
		tab++;
	}
}

//------------------------------------------------------------------------------
static int
ins_map (const char * set)
{
	int   scan = 0, num = 0;
	CARD8 asc;
	
	while ((asc = *(set++))) {
		if (KYBD_Set[asc]) {
			printf ("    * '%c'(%04lX) already mapped to keycode %i \n",
			        asc, Tos2Iso[asc], KYBD_Set[asc]);
			if (*set) set++;
			continue;
		}
		do if (++scan > KYBD_CodeMax - KEYSYM_OFFS) {
			printf ("    * no space to map keycode%s for '%c%s'\n",
			        (*set ? "s" : ""), asc, set);
			return num;
		} while (KYBD_Symbol[scan][0] != XK_VoidSymbol);
		
		KYBD_Set[asc]        = scan;
		KYBD_Symbol[scan][0] = Tos2Iso[asc];
		num++;
	//	printf ("    mapped '%c'(%04lX) to keycode %i[0] \n",
	//	        asc, Tos2Iso[asc], scan);
		if (*set  &&  (asc = *(set++)) != ' ') {
			KYBD_Set[asc]        = scan;
			KYBD_Symbol[scan][1] = Tos2Iso[asc];
			num++;
		//	printf ("    mapped '%c'(%04lX) to keycode %i[1] \n",
		//	        asc, Tos2Iso[asc], scan);
		}
	}
	return num;
}

//------------------------------------------------------------------------------
static int
analyse_ktbl (const char * tbl, int shift)
{
	// Analyse a standard keytable returned by Keytbl() and store the results in
	// the KeyCode-to-KeySym translation table KYBD_Symbol[].
	//...........................................................................
	
	int    n   = 0;
	KeySym sym = NoSymbol;
	CARD8  asc;
	int    scan;
	
	for (scan = 1; scan < 128; scan++) {
		if ((asc = *(++tbl))) {
			if (!KYBD_Set[asc]) {
				KYBD_Set[asc] = scan;
				sym           = Tos2Iso[asc];
				
			} else if (KYBD_Set[asc] == scan) {
				asc = 0;
				
			} else switch (scan) {
				case 74: case 78: case 99 ... 114:
					// scancode already defined for this ascii, so it must be a
					// numblock key
					//
					if (KYBD_Symbol[scan][shift] == XK_VoidSymbol) {
						sym = XK_KP_Space + asc;
					} else {
						asc = 0;
					}
					break;
				
				default: if (shift) switch (asc) {
					// scancode is already defined by a numblock key, so replace it
					// by the shifted key's code
					//
					case '*'...'+': case '-'...'/': {
						// replace the numblock key's symbol
						//
						int scn = KYBD_Set[asc];
						KYBD_Symbol[scn][0] = XK_KP_Space + asc;
					}
					case '('...')':
						KYBD_Set[asc] = scan;
						sym           = Tos2Iso[asc];
						break;
					
					case '0'...'9':
						// there seems to be a bug in the falcon's shift-keytbl, it
						// defines shifted ins, home and arrows to numbers
						asc = 0;
						break;
				}
			}
			if (sym) {
				set_sym (KYBD_Symbol[scan], shift, sym);
				sym = NoSymbol;
				
			} else if (asc) {
				WmgrIntro (xFalse);
				printf ("    * duplicate scancode for 0x%02X '%.1s' (0x%02X):"
				               " 0x%02X %s \n",
				        asc, (asc >= ' ' ? (char*)&asc : ""), KYBD_Set[asc],
				        scan, (shift == 0 ? "unshift" : "shift"));
			}
			n++;
		}
	}
	return n;
}

//------------------------------------------------------------------------------
static int
analyse_xtbl (const KEYPAIR * tbl, int shift)
{
	// Analyse an extended keytable as returned by Keytbl() in TOS >= 4.0 and
	// store the results in the KeyCode-to-KeySym translation table KYBD_Symbol[].
	//...........................................................................
	
	int   n = 0;
	CARD8 scan;
	
	while ((scan = tbl->scan)) {
		CARD8 asc = tbl->asc;
		if (scan > numberof(KYBD_Symbol)) {
			WmgrIntro (xFalse);
			printf ("    * ignored scancode 0x%02X for 0x%02X '%.1s'\n",
			        scan, asc, (asc >= ' ' ? (char*)&asc : ""));
		
		} else if (!KYBD_Set[asc]) {
			KYBD_Set[asc] = scan;
			set_sym (KYBD_Symbol[scan], 2 + shift, Tos2Iso[asc]);
		
		} else {
			WmgrIntro (xFalse);
			printf ("    * duplicate scancode for 0x%02X '%.1s' (0x%02X):"
			               " 0x%02X alt/%s \n",
			        asc, (asc >= ' ' ? (char*)&asc : ""), KYBD_Set[asc],
			        scan, (shift == 0 ? "unshift" : "shift"));
		}
		tbl++;
		n++;
	}
	return n;
}

//==============================================================================
void
KybdInit (void)
{
	if (!KYBD_Table) {
		const char * type = "<unknown>";
		int   i;
		int   n_usf, n_sft, n_alt = 0, n_asf = 0, n_agr = 0, n_xtr = 0, n_gap = 0;
		BOOL  prnth  = xFalse;
		short os_ver = Ssystem (S_OSHEADER, 0x0000, 0);
		long  os_beg = Ssystem (S_OSHEADER, 0x0008, 0);
		long  os_end = Ssystem (S_OSHEADER, 0x000C, 0);
		if (os_end < os_beg) os_end += os_beg;
		
		i            = Kbrate (-1, -1);
		KYBD_Repeat  = ((unsigned)(i & 0xFF00) >>6) *5;
		KYBD_Table   = Keytbl ((void*)-1, (void*)-1, (void*)-1);
		KYBD_CodeMax = KYBD_CodeMin + numberof(KYBD_Symbol) -1;
		
		printf ("  Keyboard diagnostics: (TOS %i.%02i) \n",
		        os_ver >> 8, os_ver & 0x00FF);
/*
		printf ("    rom-tos ranges 0x%0lX..0x%0lX\n", os_beg, os_end);
		printf ("    unShift = %p   Shift   = %p   CapsLck = %p \n",
		        KYBD_Table->unShift, KYBD_Table->Shift, KYBD_Table->CapsLck);
		printf ("    Alt     = %p   AltShft = %p   AltCaps = %p \n",
		        KYBD_Table->Alt, KYBD_Table->AltShft, KYBD_Table->AltCaps);
		printf ("    AltGr   = %p \n", KYBD_Table->AltGr);
		
		//	Hades Keyboard diagnostics: TOS 3.06 (0x7fe00000..0x7fe08316)
		//	  unShift = 0x7fe3673c   Shift   = 0x7fe367bc   CapsLck = 0x7fe3683c
		//	  Alt     = 0x80b0101    AltShft = 0x10070000   AltCaps = (nil)
		//	  AltGr   = (nil)
*/		
		memset (KYBD_Set, 0, sizeof(KYBD_Set));
		for (i = 0; i < numberof(KYBD_Symbol); i++) {
			KYBD_Symbol[i][0] = KYBD_Symbol[i][1] = 
			KYBD_Symbol[i][2] = KYBD_Symbol[i][3] = XK_VoidSymbol;
		}
		
		// analyse the standard keytables
		
		n_usf = analyse_ktbl (KYBD_Table->unShift, 0);
		if (KYBD_Set[')']) {
			prnth = xTrue;
			if      (KYBD_Set['@']) KYBD_Lang = K_FRENCH;
			else if (KYBD_Set['#']) KYBD_Lang = K_GERMAN;
		}
		n_sft = analyse_ktbl (KYBD_Table->Shift,   1);
		
		if (os_ver < 0x0400) {
			
			if (prnth) {
				KYBD_Atari = xTrue;
				type       = "TT";
				if (KYBD_Lang == K_GERMAN) {
					SCANTAB tab[] = {
						{ KYBD_Set[''],2,  '@',XK_at },
						{ KYBD_Set['š'],3, '\\',XK_backslash },
						{ KYBD_Set['”'],2,  '[',XK_bracketleft },
						{ KYBD_Set['™'],3,  '{',XK_braceleft },
						{ KYBD_Set['„'],2,  ']',XK_bracketright },
						{ KYBD_Set['Ž'],3,  '}',XK_braceright } };
					set_map (tab, numberof(tab));
				}
			
			} else {
				KYBD_Atari = xFalse;
				type       = "Hades";
				if (!KYBD_Set['@']) {
					SCANTAB tab[] = {
						{ KYBD_Set['q'],2,  '@',XK_at },
						{ KYBD_Set['?'],3, '\\',XK_backslash },
						{ KYBD_Set['<'],2,  '|',XK_bar },
						{ KYBD_Set['+'],3,  '~',XK_asciitilde },
						{ KYBD_Set['8'],2,  '[',XK_bracketleft },
						{ KYBD_Set['7'],3,  '{',XK_braceleft },
						{ KYBD_Set['0'],2,  ']',XK_bracketright },
						{ KYBD_Set['9'],3,  '}',XK_braceright } };
					set_map (tab, numberof(tab));
				}
			}
		
		} else { // analyse extended keytables
			
			if ((long)KYBD_Table->AltGr >= os_beg  &&
			    (long)KYBD_Table->AltGr <  os_end) {
				KYBD_Atari = xFalse;
				type       = "Milan";
				n_agr = analyse_xtbl (KYBD_Table->AltGr, 0);
			
			} else {
				KYBD_Atari = xTrue;
				type       = "Falcon";
				n_alt = analyse_xtbl (KYBD_Table->Alt,     0);
				n_asf = analyse_xtbl (KYBD_Table->AltShft, 1);
			}
		}
		
		// apply the default modifier and function keycodes
		
		if (KYBD_Atari) {
			static SCANTAB tab[] = {   { 113,2, 0,XK_End },
			   { 99,2, 0,XK_Page_Up }, { 100,2, 0,XK_Page_Down } };
			set_map (tab, numberof(tab));
		
		} else { // pc keyboard
			static SCANTAB tab[] = {   { 79,0, 0,XK_End },
			   { 73,0, 0,XK_Page_Up }, { 81,0, 0,XK_Page_Down } };
			set_map (tab, numberof(tab));
			if      (KYBD_Set['†']) KYBD_Lang = K_SWEDISH;
			else if (KYBD_Set['‡']) KYBD_Lang = K_FRENCH;
			else if (KYBD_Set['ž']) KYBD_Lang = K_GERMAN;
		}
		{	static SCANTAB fnc_tab[] = {
				{ 59,0, 0,XK_F1  }, { 84,0, 0,XK_F11 },
				{ 60,0, 0,XK_F2  }, { 85,0, 0,XK_F12 },
				{ 61,0, 0,XK_F3  }, { 86,0, 0,XK_F13 },
				{ 62,0, 0,XK_F4  }, { 87,0, 0,XK_F14 },
				{ 63,0, 0,XK_F5  }, { 88,0, 0,XK_F15 },
				{ 64,0, 0,XK_F6  }, { 89,0, 0,XK_F16 },
				{ 65,0, 0,XK_F7  }, { 90,0, 0,XK_F17 },
				{ 66,0, 0,XK_F8  }, { 91,0, 0,XK_F18 },
				{ 67,0, 0,XK_F9  }, { 92,0, 0,XK_F19 },
				{ 68,0, 0,XK_F10 }, { 93,0, 0,XK_F20 },
				{ 71,0, 0,XK_Home },  { 82,0, 0,XK_Insert },
				{ 72,0, 0,XK_Up },    { 75,0, 0,XK_Left },
				{ 77,0, 0,XK_Right }, { 80,0, 0,XK_Down },
				{ 97,0, 0,XK_Undo },  { 98,0, 0,XK_Help } };
			static SCANTAB mod_tab[] = {
				{ KC_SHFT_L,0, 0,XK_Shift_L },   { KC_SHFT_R,0, 0,XK_Shift_R },
				{ KC_LOCK,0,   0,XK_Caps_Lock }, { KC_CTRL,0,   0,XK_Control_L },
				{ KC_ALT,0,    0,XK_Alt_L },     { KC_ALTGR,0,  0,XK_Mode_switch }};
			set_map (fnc_tab, numberof(fnc_tab));
			memset (KYBD_Rep, 0, sizeof(KYBD_Rep));
			i = KYBD_CodeMax - KEYSYM_OFFS +1;
			do if (KYBD_Symbol[i][0] != XK_VoidSymbol) {
				KYBD_Rep[i /8 +1] |= 1 << (i & 0x07);
			} while (--i);
			set_map (mod_tab, numberof(mod_tab));
		}
		
		// apply some additional keycodes
		
		if (!KYBD_Set['ý']) {
			SCANTAB tab = { KYBD_Set['2'],2,  'ý',XK_twosuperior };
			set_map (&tab, 1);
		}
		if (!KYBD_Set['þ']) {
			SCANTAB tab = { KYBD_Set['3'],2,  'þ',XK_threesuperior };
			set_map (&tab, 1);
		}
		if (!KYBD_Set['æ']) {
			SCANTAB tab[] = {{ KYBD_Set['m'],2,  'æ',XK_mu }};
			set_map (tab, 1);
		}
		
		if (!KYBD_Set['œ']) n_xtr += ins_map("œ");
		if (!KYBD_Set['Ý']) n_xtr += ins_map("Ý");
		if (!KYBD_Set['ž']) n_xtr += ins_map("ž");
		if (!KYBD_Set['„']) n_xtr += ins_map("„Ž”™š");
		if (!KYBD_Set['‡']) n_xtr += ins_map("‡€  ƒ …¶‚ˆ ŠÄ£ – —Ï");
		if (!KYBD_Set['†']) n_xtr += ins_map("†");
		n_xtr += ins_map("‘’´µ³²°·¤¥±¸‰ ‹ Œ  ˜");
		n_xtr += ins_map("®¯¨ ­ › Ÿ");
		
		i = KYBD_CodeMax - KEYSYM_OFFS +1;
		while (KYBD_Symbol[--i][0] == XK_VoidSymbol);
		KYBD_CodeMax = i + KEYSYM_OFFS;
		
		// build modifier mapping table (for GetModifierMapping request)
		
		memset (KYBD_ModMap, 0, sizeof(KYBD_ModMap));
		KYBD_ModMap[0][0] = KEYCODE(KC_SHFT_L);
		KYBD_ModMap[0][1] = KEYCODE(KC_SHFT_R);
		KYBD_ModMap[1][0] = KEYCODE(KC_LOCK);
		KYBD_ModMap[2][0] = KEYCODE(KC_CTRL);
		KYBD_ModMap[3][0] = KEYCODE(KC_ALT);
		if (KYBD_Atari) KYBD_ModMap[3][1] = KEYCODE(KC_ALTGR);
		else            KYBD_ModMap[4][0] = KEYCODE(KC_ALTGR);
		
		printf ("    got keycodes: %i unShift, %i Shift",   n_usf, n_sft);
		if (n_alt || n_asf) printf (", %i Alt, %i AltShft", n_alt, n_asf);
		if (n_agr)          printf (", %i AltGr",           n_agr);
		for (i = 1; i < sizeof(KYBD_Set); n_gap += (KYBD_Set[i++] ? 0 : 1));
		printf (", %i gap%s\n", n_gap, (n_gap == 1 ? "" : "s"));
		if (n_xtr) {
			printf ("    mapped %i extra keycode%s, ",
			        n_xtr, (n_xtr == 1 ? "" : "s"));
		} else {
			printf ("    ");
		}
		printf ("keycode range: [%i .. %i] \n", KYBD_CodeMin, KYBD_CodeMax);
		printf ("    assume layout '%s/%s' %s, repeat %ums\n",
		        (KYBD_Atari ? "Atari" : "PC"), type,
		        (KYBD_Lang == K_GERMAN ?  "german"  :
		         KYBD_Lang == K_FRENCH ?  "french"  :
		         KYBD_Lang == K_SWEDISH ? "swedish" : "us/english"),
		        KYBD_Repeat);
	}
}


//==============================================================================
short
KybdEvent (CARD16 scan, CARD8 meta)
{
	CARD8   chng = (KYBD_PrvMeta ^ meta);
	KeyCode code = (CARD16)scan >> 8;
	
	if (scan) {
		if     (!code)        code  = KYBD_Set[scan];
		else if (code >= 120) code -= 118;
		
		if (KYBD_Symbol[code][0] == XK_VoidSymbol) {
			printf ("*** unknown scan code %02X:%02X ***\n", code, scan & 0xFF);
		}
	}
	
	if (_WIND_PointerRoot) {
		CARD32 c_id = _WIND_PointerRoot->Id;
		PXY    r_xy = WindPointerPos (NULL);
		PXY    e_xy = WindPointerPos (_WIND_PointerRoot);
		CARD8  shft = (meta & (K_LSHIFT|K_RSHIFT) ? ShiftMask : 0);
		int    i    = 0;
		
		if (*KYBD_Pending) {
			EvntPropagate (_WIND_PointerRoot, KeyReleaseMask, KeyRelease,
			               c_id, r_xy, e_xy, *KYBD_Pending);
			*KYBD_Pending = 0;
		}
		while (chng) {
			if (chng & 1) {
				if (meta & (1 << i)) {
					MAIN_Key_Mask |= KYBD_Static[i][1];
					EvntPropagate (_WIND_PointerRoot, KeyPressMask, KeyPress,
					               c_id, r_xy, e_xy, KYBD_Static[i][0]);
				} else {
					MAIN_Key_Mask &= ~KYBD_Static[i][1] | shft;
					EvntPropagate (_WIND_PointerRoot, KeyReleaseMask, KeyRelease,
					               c_id, r_xy, e_xy, KYBD_Static[i][0]);
				}
			}
			chng >>=1;
			i++;
		}
		if (code) {
			CARD8 save = MAIN_Key_Mask;
			if (scan < 0x0100) {
				MAIN_Key_Mask = (Tos2Iso[scan] == KYBD_Symbol[code][0]
				                 ? 0 : ShiftMask);
			}
			code = KEYCODE(code);
			EvntPropagate (_WIND_PointerRoot, KeyPressMask, KeyPress,
			               c_id, r_xy, e_xy, code);
			if (scan < 0x0100) {
				EvntPropagate (_WIND_PointerRoot, KeyReleaseMask, KeyRelease,
				               c_id, r_xy, e_xy, code);
				MAIN_Key_Mask = save;
			} else {
				*KYBD_Pending = code;
			}
		}
	
	} else {
		MAIN_Key_Mask = (meta & (K_LSHIFT|K_RSHIFT) ? ShiftMask   : 0)
		              | (meta &  K_LOCK             ? LockMask    : 0)
		              | (meta &  K_CTRL             ? ControlMask : 0)
		              | (meta &  K_ALT              ? Mod1Mask    : 0)
		              | (meta &  K_ALTGR            ? Mod2Mask    : 0);
		*KYBD_Pending = 0;
	}
	KYBD_PrvMeta = meta;
	
	return chng;
}


//==============================================================================
static void
Kybd_GetMapping (CARD8 * map)
{
	int   i, n = numberof(KYBD_Static) -1;
	CARD8 mask = KYBD_PrvMeta;
	
	memset (map, 0, 32);
	if (!*KYBD_Pending) n--;
	for (i = 0; i <= n && mask; i++) {
		if (mask & 1) {
			CARD8 k    = KYBD_Static[i][0];
			map[k /8] |= 1 << (k & 0x07);
		}
		mask >>= 1;
	}
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetKeyboardControl (CLIENT * clnt, xGetKeyboardControlReq * q)
{
	// Returns the current control values for the keyboard
	//
	// (no request parameters)
	//
	// BOOL   globalAutoRepeat: read from 'conterm' system variable bit 1
	// CARD32 ledMask:          if CapsLock is on, LED #1 is lit
	// CARD8  keyClickPercent:  read from 'conterm' system variable bit 0
	// CARD8  bellPercent:
	// CARD16 bellPitch:
	// CARD16 bellDuration:
	// BYTE   map[32]:         bit masks start here
	//...........................................................................
	
	ClntReplyPtr (GetKeyboardControl, r,);
	char conterm = Ssystem (S_GETBVAL, 0x484, 0);
	
	DEBUG (GetKeyboardControl," ");
	
	r->ledMask      = (MAIN_Key_Mask & LockMask ? 1 : 0);
	r->bellPercent  =  100;
	r->bellPitch    = 2000; //Hz
	r->bellDuration =  100; //ms
	r->keyClickPercent  = (conterm & 0x01 ? 100   : 0);
	r->globalAutoRepeat = (conterm & 0x02 ? xTrue : xFalse);
	
	if (r->globalAutoRepeat) memcpy (r->map, KYBD_Rep, sizeof(r->map));
	else                     memset (r->map, 0,        sizeof(r->map));
	
	ClntReply (GetKeyboardControl,, "l2:");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetModifierMapping (CLIENT * clnt, xGetModifierMappingReq * q)
{
	// Returns the keycodes of the modifiers Shift, Lock, Control, Mod1...Mod5
	//
	// Reply:
	// CARD8 numKeyPerModifier
	//...........................................................................
	
	ClntReplyPtr (GetModifierMapping, r, sizeof(KYBD_ModMap));
	
	DEBUG (GetModifierMapping," ");
	
	r->numKeyPerModifier = sizeof(*KYBD_ModMap);
	memcpy ((r +1), KYBD_ModMap, sizeof(KYBD_ModMap));
	
	ClntReply (GetModifierMapping, (8 *2), NULL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetKeyboardMapping (CLIENT * clnt, xGetKeyboardMappingReq * q)
{
	// Returns 'count' keysymbols starting from 'firstKeyCode'.
	//
	// KeyCode firstKeyCode:
	// CARD8   count:
	//
	// Reply:
	// CARD8 keySymsPerKeyCode:
	//...........................................................................
	
	if (q->firstKeyCode < KYBD_CodeMin) {
		Bad(Value, q->firstKeyCode, GetKeyboardMapping,"(): \n"
		    "          firstKeyCode %u < %u \n", q->firstKeyCode, KYBD_CodeMin);
	
	} else if (q->firstKeyCode + q->count -1 > KYBD_CodeMax) {
		Bad(Value, q->count, GetKeyboardMapping,"(): \n"
		    "          firstKeyCode + count -1 %u > %u \n",
		           q->firstKeyCode + q->count -1, KYBD_CodeMax);
	
	} else { //..................................................................
	
		KeySym * sym = KYBD_Symbol[q->firstKeyCode - KYBD_CodeMin];
		int      num = q->count * 4;
		size_t   len = num * sizeof(KeySym);
		ClntReplyPtr (GetKeyboardMapping, r, len);
		
		DEBUG (GetKeyboardMapping," %i (%i)", q->firstKeyCode, q->count);
		
		r->keySymsPerKeyCode = 4;
		if (clnt->DoSwap) {
			KeySym * s = (KeySym*)(r +1);
			int      n = num;
			while (n--) *(s++) = Swap32(*(sym++));
		} else {
			memcpy ((r +1), sym, len);
		}
		ClntReply (GetKeyboardMapping, len, NULL);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ChangeKeyboardMapping (CLIENT * clnt, xChangeKeyboardMappingReq * q)
{
	// Defines the first 'keySymsPerKeyCode' of 'count' KeySyms starting from
	// 'firstKeyCode'.  KeySyms outside this range remains unchanged.
	//
	// CARD8   keyCodes:
	// KeyCode firstKeyCode:
	// CARD8   keySymsPerKeyCode:
	//...........................................................................
	
	if (q->firstKeyCode < KYBD_CodeMin) {
		Bad(Value, q->firstKeyCode, ChangeKeyboardMapping,"(): \n"
		    "          firstKeyCode %u < %u \n", q->firstKeyCode, KYBD_CodeMin);
	
	} else if (q->firstKeyCode + q->keyCodes -1 > KYBD_CodeMax) {
		Bad(Value, q->keyCodes, ChangeKeyboardMapping,"(): \n"
		    "          firstKeyCode + keyCodes -1 %u > %u \n",
		           q->firstKeyCode + q->keyCodes -1, KYBD_CodeMax);
	
	} else if (q->keySymsPerKeyCode > numberof(*KYBD_Symbol)) {
		Bad(Alloc,, ChangeKeyboardMapping," %i (%i*%i)",
	       q->firstKeyCode, q->keyCodes, q->keySymsPerKeyCode);
		
		// no support for dynamical expansion of keySymsPerKeyCode for now!
	
	} else { //..................................................................
		
		KeySym * src = (KeySym*)(q +1);
		KeySym * dst = KYBD_Symbol[q->firstKeyCode - KYBD_CodeMin];
		int      n   = q->keyCodes;
		
		DEBUG (ChangeKeyboardMapping," %i (%i*%i)",
		       q->firstKeyCode, q->keyCodes, q->keySymsPerKeyCode);
		
		if (clnt->DoSwap) while (n--) {
			KeySym * sym = dst;
			int      i   = q->keySymsPerKeyCode;
			while (i--) *(sym++) = Swap32(*(src++));
			dst += numberof(*KYBD_Symbol);
		
		} else while (n--) {
			KeySym * sym = dst;
			int      i   = q->keySymsPerKeyCode;
			while (i--) *(sym++) = *(src++);
			dst += numberof(*KYBD_Symbol);
		}
		EvntMappingNotify (MappingKeyboard, 0,0);
	}
}


//------------------------------------------------------------------------------
void
RQ_ChangeKeyboardControl (CLIENT * clnt, xChangeKeyboardControlReq * q)
{
	PRINT (- X_ChangeKeyboardControl," ");
}

//------------------------------------------------------------------------------
void
RQ_SetModifierMapping (CLIENT * clnt, xSetModifierMappingReq * q)
{
	PRINT (- X_SetModifierMapping," ");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_QueryKeymap (CLIENT * clnt, xQueryKeymapReq * q)
{
	// Returns a bit vector for the logical state of the keyboard, where each
	// bit set to 1 indicates that the corresponding key is currently pressed.
	//...........................................................................
	
	ClntReplyPtr (QueryKeymap, r,);
	
	PRINT (QueryKeymap," ");
	
	Kybd_GetMapping (r->map);
	
	ClntReply (QueryKeymap,, NULL);
}
