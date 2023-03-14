//==============================================================================
//
// server.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-03 - Initial Version.
//==============================================================================
//
#ifndef __SERVER_H__
#	define __SERVER_H__

#include "xrsc.h"


typedef struct s_CONNECTION * p_ACCEPT;

typedef union {
	struct s_CONNECTION * p;
	p_ACCEPT              Accept;
	p_CLIENT              Client;
} p_CONNECTION;

typedef BOOL (CONN_RW) (p_CONNECTION, BOOL rd, BOOL wr);

struct s_CONNECTION {
	char      _resrvd[sizeof(struct s_XRSC)];
	int       Fd;
	int       Port;
	long      FdSet;
	CONN_RW * ConnRW;
};


int    SrvrInit   (int port);
void   SrvrReset  (void);
BOOL   SrvrSelect (short exclusive);
void   SrvrGrab       (p_CLIENT);
void   SrvrUngrab     (p_CLIENT);
void   SrvrConnInsert (p_CONNECTION);
void   SrvrConnRemove (p_CONNECTION);
size_t SrvrSetup      (void * buf, CARD16 maxreqlen, int DoSwap, long rid);


#endif __SERVER_H__
