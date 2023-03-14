//==============================================================================
//
// ICCC.h -- Inter Client Communication Convention.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-05-09 - Initial Version.
//==============================================================================
//
#ifndef __ICCC_H__
# define __ICCC_H__


// Property XA_WM_NORMAL_HINTS, type XA_WM_SIZE_HINTS

typedef struct {
	long flags;       // marks which fields in this structure are defined
	long _x,_y,_w,_h; // obsolete
	long min_width, min_height;
	long max_width, max_height;
	long inc_width, inc_height;
	struct {
		long x;   // numerator
		long y;   // denominator
	}    min_aspect, max_aspect;
	long base_width, base_height;   // added by ICCCM version 1
	long win_gravity;               // added by ICCCM version 1
} SizeHints;

// flags argument in size hints
#define USPosition  (1L << 0) // user specified x, y
#define USSize      (1L << 1) // user specified width, height
#define PPosition   (1L << 2) // program specified position
#define PSize       (1L << 3) // program specified size
#define PMinSize    (1L << 4) // program specified minimum size
#define PMaxSize    (1L << 5) // program specified maximum size
#define PResizeInc  (1L << 6) // program specified resize increments
#define PAspect     (1L << 7) // program specified min and max aspect ratios
#define PBaseSize   (1L << 8) // program specified base for incrementing
#define PWinGravity (1L << 9) // program specified window gravity


// Property XA_WM_HINTS, type XA_WM_HINTS

typedef struct {
	long flags;   // marks which fields in this structure are defined
	long input;   // does this application rely on the window manager to
	              // get keyboard input?
	int    initial_state;  // see below
	Pixmap icon_pixmap;    // pixmap to be used as icon
	Window icon_window;    // window to be used as icon
	int    icon_x, icon_y; // initial position of icon
	Pixmap icon_mask;      // icon mask bitmap
	XID    window_group;   // id of related window group
	/* this structure may be extended in the future */
} WmHints;

// definition for flags of WmHints
#define InputHint        (1L << 0)
#define StateHint        (1L << 1)
#define IconPixmapHint   (1L << 2)
#define IconWindowHint   (1L << 3)
#define IconPositionHint (1L << 4)
#define IconMaskHint     (1L << 5)
#define WindowGroupHint  (1L << 6)
#define XUrgencyHint     (1L << 8)

// definitions for initial window state
#define WithdrawnState 0   // for windows that are not mapped
#define NormalState    1   // most applications want to start this way
#define IconicState    3   // application wants to start as an icon


#endif __ICCC_H__
