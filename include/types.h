/*
 *==============================================================================
 *
 * types.h
 *
 * Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2001-02-22 - Initial Version.
 *==============================================================================
 */
#ifndef __TYPES_H__
#define __TYPES_H__


typedef struct s_PRECT
{
	PXY lu;								/* upper left corner */
	PXY rd;								/* bottom right corner */
} PRECT;
typedef PRECT *p_PRECT;


#endif /* __TYPES_H__ */
