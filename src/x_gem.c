//==============================================================================
//
// x_gem.c -- extensions to the gem-lib.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#define __TYPES_H__
#include "x_gem.h"

#include <stddef.h>


//==============================================================================
short
wind_get_one (int WindowHandle, int What)
{
	short out;

	return wind_get_int(WindowHandle, What, &out) ? out : -1;
}

//==============================================================================
short
wind_set_proc (int WindowHandle, CICONBLK *icon)
{
	return wind_set_ptr(WindowHandle, 201, icon);
}
