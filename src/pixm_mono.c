//==============================================================================
//
// pixm_mono.c
//
// Copyright (C) 2000 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-10-15 - Initial Version.
//==============================================================================
//
#include "pixmap_P.h"

//#include <X11/X.h>


//------------------------------------------------------------------------------
// Monochrome operations on a (16bit) column
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __col_clr (CARD16 * mem, CARD16 inc, CARD16 num, CARD16 pat)
{
	while (num--) { *mem &= pat; mem += inc; }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __col_set (CARD16 * mem, CARD16 inc, CARD16 num, CARD16 pat)
{
	while (num--) { *mem |= pat; mem += inc; }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __col_neg (CARD16 * mem, CARD16 inc, CARD16 num, CARD16 pat)
{
	while (num--) {
		*mem = ((*mem ^ pat) & pat) | (*mem & ~pat);
		mem += inc;
	}
}

//------------------------------------------------------------------------------
// Monochrome operations on a rectangle
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __rec_clr (CARD16 * mem, CARD16 inc, CARD16 num, CARD16 len)
{
	while (num--) {
		int i;
		for (i = 0; i <= len; mem[i++] = 0x0000);
		mem += inc;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __rec_set (CARD16 * mem, CARD16 inc, CARD16 num, CARD16 len)
{
	while (num--) {
		int i;
		for (i = 0; i <= len; mem[i++] = 0xFFFF);
		mem += inc;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void __rec_neg (CARD16 * mem, CARD16 inc, CARD16 num, CARD16 len)
{
	while (num--) {
		int i;
		for (i = 0; i <= len; mem[i++] ^= 0xFFFF);
		mem += inc;
	}
}

//------------------------------------------------------------------------------
CARD32
_Pmap_MonoFuncts (p_GC gc,
                  void (**f_col)(CARD16 *, CARD16 inc, CARD16 num, CARD16 pat),
                  void (**f_rec)(CARD16 *, CARD16 inc, CARD16 num, CARD16 len))
{
	//_Table_of_Operation_Modes.__________________________________
	// The enum values of operation names are identical to the
	// binary result of src/dst combination.
	//------------------------------------------------------------
	//______________combinations:__src = 00.11 |Function if src is
	// Name          | Operation | dst = 01.01 | cleared | set
	//---------------|-----------|-------------|---------|--------
	// GXclear       |   0             | 00.00 |        CLR
	// GXand         |  src AND  dst   | 00.01 |     CLR | NOP
	// GXandReverse  |  src AND !dst   | 00.10 |     CLR | NEG
	// GXcopy        |  src            | 00.11 |     CLR | SET
	// GXandInverted | !src AND  dst   | 01.00 |     NOP | CLR
	// GXnoop        |           dst   | 01.01 |        NOP
	// GXxor         |  src XOR  dst   | 01.10 |     NOP | NEG
	// GXor          |  src OR   dst   | 01.11 |     NOP | SET
	// GXnor         | !src AND !dst   | 10.00 |     NEG | CLR
	// GXequiv       | !src XOR  dst   | 10.01 |     NEG | NOP
	// GXinvert      |          !dst   | 10.10 |        NEG
	// GXorReverse   |  src OR  !dst   | 10.11 |     NEG | SET
	// GXcopyInverted| !src            | 11.00 |     SET | CLR
	// GXorInverted  | !src OR   dst   | 11.01 |     SET | NOP
	// GXnand        | !src OR  !dst   | 11.10 |     SET | NEG
	// GXset         |   1             | 11.11 |        SET
	
	switch ((gc->Foreground & 1 ? gc->Function : (gc->Function >> 2)) & 0x03) {
		case 0x00: *f_col = __col_clr; *f_rec = __rec_clr; return 0xFFFF0000;
		case 0x02: *f_col = __col_neg; *f_rec = __rec_neg; return 0x0000FFFF;
		case 0x03: *f_col = __col_set; *f_rec = __rec_set; return 0x0000FFFF;
	}
	// else 0x01 -> NOP
	
	return 0;
}
