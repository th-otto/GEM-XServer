/*
 *==============================================================================
 *
 * config.c -- Globals and constants.
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-21 - Initial Version.
 *==============================================================================
 */
#include "main.h"


/* maximum allowed size of a client request in longs, taken as a 16bit unsigned */

CARD32 CNFG_MaxReqLength = 65535uL;		/* maximum size */


/* pathes and file locations */

#define X_BIN_U "U:\\usr\\X11\\bin"

const char PATH_DEBUG_OUT[] = "U:\\usr\\bin\\xcat";
const char PATH_XBIN_RSRC[] = X_BIN_U "\\Xapp.rsc";
const char *PATH_RSRC = PATH_XBIN_RSRC + sizeof(X_BIN_U);

#define X_ETC "/etc/X11"
#define X_BIN "/usr/X11/bin"
#define V_LIB "/var/lib/Xapp"

const char PATH_Xconsole[] = X_BIN "/xconsole";

const char PATH_Xmodmap[] = X_BIN "/xmodmap";
const char PATH_XmodmapRc[] = X_ETC "/Xmodmap";

const char PATH_FontsAlias[] = X_ETC "/fonts.alias";
const char PATH_LibDir[] = V_LIB;
const char PATH_FontsDb[] = V_LIB "/fonts.db";
