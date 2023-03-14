//==============================================================================
//
// request.h
//
// Copyright (C) 2000 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-07-23 - Initial Version.
//==============================================================================
//
#ifndef __REQUEST_H__
# define __REQUEST_H__

# ifndef XPROTO_H
#  include <X11/Xproto.h>
# endif

typedef xReq xGetInputFocusReq;
typedef xReq xGetKeyboardControlReq;
typedef xReq xGetModifierMappingReq;
typedef xReq xGetPointerControlReq;
typedef xReq xGetPointerMappingReq;
typedef xReq xGetScreenSaverReq;
typedef xReq xGrabServerReq;
typedef xReq xListExtensionsReq;
typedef xReq xNoOperationReq;
typedef xReq xQueryKeymapReq;
typedef xReq xUngrabServerReq;

typedef xResourceReq xCloseFontReq;
typedef xResourceReq xDestroySubwindowsReq;
typedef xResourceReq xDestroyWindowReq;
typedef xResourceReq xFreeColormapReq;
typedef xResourceReq xFreeCursorReq;
typedef xResourceReq xFreeGCReq;
typedef xResourceReq xFreePixmapReq;
typedef xResourceReq xGetAtomNameReq;
typedef xResourceReq xGetFontPathReq;
typedef xResourceReq xGetGeometryReq;
typedef xResourceReq xGetSelectionOwnerReq;
typedef xResourceReq xGetWindowAttributesReq;
typedef xResourceReq xInstallColormapReq;
typedef xResourceReq xKillClientReq;
typedef xResourceReq xListInstalledColormapsReq;
typedef xResourceReq xListPropertiesReq;
typedef xResourceReq xMapSubwindowsReq;
typedef xResourceReq xMapWindowReq;
typedef xResourceReq xQueryFontReq;
typedef xResourceReq xQueryPointerReq;
typedef xResourceReq xQueryTreeReq;
typedef xResourceReq xUngrabKeyboardReq;
typedef xResourceReq xUngrabPointerReq;
typedef xResourceReq xUninstallColormapReq;
typedef xResourceReq xUnmapSubwindowsReq;
typedef xResourceReq xUnmapWindowReq;


#define REQUEST_DEF(T,P) void RQ_##T (p_CLIENT , x##T##Req *);

#endif __REQUEST_H__


#ifdef REQUEST_DEF

	REQUEST_DEF (CreateWindow,            "wwR:4*")  // window.c
	REQUEST_DEF (ChangeWindowAttributes,  "w*")      // window.c
	REQUEST_DEF (GetWindowAttributes,     "w")       // window.c
	REQUEST_DEF (DestroyWindow,           "w")       // window.c
	REQUEST_DEF (DestroySubwindows,       "w")       // window.c
	REQUEST_DEF (ChangeSaveSet,           "w")       // window.c
	REQUEST_DEF (ReparentWindow,          "ww")      // window.c
	REQUEST_DEF (MapWindow,               "w")       // window.c
	REQUEST_DEF (MapSubwindows,           "w")       // window.c
	REQUEST_DEF (UnmapWindow,             "w")       // window.c
	REQUEST_DEF (UnmapSubwindows,         "w")       // window.c
	REQUEST_DEF (ConfigureWindow,         "w.")      // window.c
	REQUEST_DEF (CirculateWindow,         "w")       // window.c
	REQUEST_DEF (GetGeometry,             "d")       // drawable.c
	REQUEST_DEF (QueryTree,               "w")       // window.c
	REQUEST_DEF (InternAtom,              ".")       // Atom.c
	REQUEST_DEF (GetAtomName,             "a")       // Atom.c
	REQUEST_DEF (ChangeProperty,          "waa4l")   // Property.c
	REQUEST_DEF (DeleteProperty,          "wa")      // Property.c
	REQUEST_DEF (GetProperty,             "waall")   // Property.c
	REQUEST_DEF (ListProperties,          "w")       // Property.c
	REQUEST_DEF (SetSelectionOwner,       "wat")     // selection.c
	REQUEST_DEF (GetSelectionOwner,       "a")       // selection.c
	REQUEST_DEF (ConvertSelection,        "waaat")   // selection.c
	REQUEST_DEF (SendEvent,               "ws")      // event.c
	REQUEST_DEF (GrabPointer,             "w.2wlt")  // wind_grab.c
	REQUEST_DEF (UngrabPointer,           "t")       // wind_grab.c
	REQUEST_DEF (GrabButton,              "w.2wl2.") // wind_grab.c
	REQUEST_DEF (UngrabButton,            "w.")      // wind_grab.c
	REQUEST_DEF (ChangeActivePointerGrab, "lt.")     // wind_grab.c
	REQUEST_DEF (GrabKeyboard,            "wt")      // wind_grab.c
	REQUEST_DEF (UngrabKeyboard,          "t")       // wind_grab.c
	REQUEST_DEF (GrabKey,                 "w.")      // wind_grab.c
	REQUEST_DEF (UngrabKey,               "w.")      // wind_grab.c
	REQUEST_DEF (AllowEvents,             "t")       // clnt.c
	REQUEST_DEF (GrabServer,                NULL)    // clnt.c
	REQUEST_DEF (UngrabServer,              NULL)    // clnt.c
	REQUEST_DEF (QueryPointer,            "w")       // window.c
	REQUEST_DEF (GetMotionEvents,         "wtt")     // clnt.c
	REQUEST_DEF (TranslateCoords,         "wwP")     // wind_pntr.c
	REQUEST_DEF (WarpPointer,             "wwRP")    // window.c
	REQUEST_DEF (SetInputFocus,           "wt")      // wind_grab.c
	REQUEST_DEF (GetInputFocus,             NULL)    // wind_grab.c
	REQUEST_DEF (QueryKeymap,               NULL)    // clnt.c
	REQUEST_DEF (OpenFont,                "f.")      // font.c
	REQUEST_DEF (CloseFont,               "f")       // font.c
	REQUEST_DEF (QueryFont,               "f")       // font.c
	REQUEST_DEF (QueryTextExtents,        "f")       // font.c
	REQUEST_DEF (ListFonts,               ":")       // font.c
	REQUEST_DEF (ListFontsWithInfo,       ":")       // font.c
	REQUEST_DEF (SetFontPath,             ".")       // font.c
	REQUEST_DEF (GetFontPath,               NULL)    // font.c
	REQUEST_DEF (CreatePixmap,            "pd:")     // pixmap.c
	REQUEST_DEF (FreePixmap,              "p")       // pixmap.c
	REQUEST_DEF (CreateGC,                "gd*")     // gcontext.c
	REQUEST_DEF (ChangeGC,                "g*")      // gcontext.c
	REQUEST_DEF (CopyGC,                  "gg*")     // gcontext.c
	REQUEST_DEF (SetDashes,               "g:")      // gcontext.c
	REQUEST_DEF (SetClipRectangles,       "gP")      // gcontext.c
	REQUEST_DEF (FreeGC,                  "g")       // gcontext.c
	REQUEST_DEF (ClearArea,               "wR")      // wind_draw.c
	REQUEST_DEF (CopyArea,                "ddgPR")   // drawable.c
	REQUEST_DEF (CopyPlane,               "ddgPRl")  // drawable.c
	REQUEST_DEF (PolyPoint,               "dg")      // drawable.c
	REQUEST_DEF (PolyLine,                "dg")      // grph.c
	REQUEST_DEF (PolySegment,             "dg")      // grph.c
	REQUEST_DEF (PolyRectangle,           "dg")      // grph.c
	REQUEST_DEF (PolyArc,                 "dg")      // grph.c
	REQUEST_DEF (FillPoly,                "dg")      // grph.c
	REQUEST_DEF (PolyFillRectangle,       "dg")      // drawable.c
	REQUEST_DEF (PolyFillArc,             "dg")      // grph.c
	REQUEST_DEF (PutImage,                "dg:P")    // drawable.c
	REQUEST_DEF (GetImage,                "dRl")     // drawable.c
	REQUEST_DEF (PolyText8,               "dgP")     // grph.c
	REQUEST_DEF (PolyText16,              "dgP")     // grph.c
	REQUEST_DEF (ImageText8,              "dgP")     // grph.c
	REQUEST_DEF (ImageText16,             "dgP")     // grph.c
	REQUEST_DEF (CreateColormap,          "mwl")     // colormap.c
	REQUEST_DEF (FreeColormap,            "m")       // colormap.c
	REQUEST_DEF (CopyColormapAndFree,     "mm")      // colormap.c
	REQUEST_DEF (InstallColormap,         "m")       // colormap.c
	REQUEST_DEF (UninstallColormap,       "m")       // colormap.c
	REQUEST_DEF (ListInstalledColormaps,  "w")     // colormap.c
	REQUEST_DEF (AllocColor,              "m:.")     // colormap.c
	REQUEST_DEF (AllocNamedColor,         "m.")      // colormap.c
	REQUEST_DEF (AllocColorCells,         "m:")      // colormap.c
	REQUEST_DEF (AllocColorPlanes,        "m::")     // colormap.c
	REQUEST_DEF (FreeColors,              "ml")      // colormap.c
	REQUEST_DEF (StoreColors,             "m")       // colormap.c
	REQUEST_DEF (StoreNamedColor,         "ml.")     // colormap.c
	REQUEST_DEF (QueryColors,             "m")       // colormap.c
	REQUEST_DEF (LookupColor,             "m.")      // colormap.c
	REQUEST_DEF (CreateCursor,            "lpp:::P") // Cursor.c
	REQUEST_DEF (CreateGlyphCursor,       "lff::::") // Cursor.c
	REQUEST_DEF (FreeCursor,              "l")       // Cursor.c
	REQUEST_DEF (RecolorCursor,           "l:::")    // Cursor.c
	REQUEST_DEF (QueryBestSize,           "d:")      // grph.c
	REQUEST_DEF (QueryExtension,          ".")       // srvr.c
	REQUEST_DEF (ListExtensions,            NULL)    // srvr.c
	REQUEST_DEF (ChangeKeyboardMapping,     NULL)    // Keyboard.c
	REQUEST_DEF (GetKeyboardMapping,        NULL)    // Keyboard.c
	REQUEST_DEF (ChangeKeyboardControl,   "*")       // srvr.c
	REQUEST_DEF (GetKeyboardControl,        NULL)    // srvr.c
	REQUEST_DEF (Bell,                      NULL)    // srvr.c
	REQUEST_DEF (ChangePointerControl,    ":.")      // srvr.c
	REQUEST_DEF (GetPointerControl,         NULL)    // srvr.c
	REQUEST_DEF (SetScreenSaver,          ":")       // srvr.c
	REQUEST_DEF (GetScreenSaver,            NULL)    // srvr.c
	REQUEST_DEF (ChangeHosts,             "2.")      // srvr.c
	REQUEST_DEF (ListHosts,                 NULL)    // srvr.c
	REQUEST_DEF (SetAccessControl,          NULL)    // srvr.c
	REQUEST_DEF (SetCloseDownMode,          NULL)    // clnt.c
	REQUEST_DEF (KillClient,              "l")       // clnt.c
	REQUEST_DEF (RotateProperties,        "w:")      // Property.c
	REQUEST_DEF (ForceScreenSaver,          NULL)    // srvr.c
	REQUEST_DEF (SetPointerMapping,         NULL)    // srvr.c
	REQUEST_DEF (GetPointerMapping,         NULL)    // srvr.c
	REQUEST_DEF (SetModifierMapping,        NULL)    // Keyboard.c
	REQUEST_DEF (GetModifierMapping,        NULL)    // Keyboard.c
	REQUEST_DEF (NoOperation,               NULL)    // srvr.c
	
#	undef REQUEST_DEF

#endif REQUEST_DEF
