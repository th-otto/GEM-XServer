/*
 *==============================================================================
 *
 * clnt.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-03 - Initial Version.
 *==============================================================================
 */
#ifndef __CLNT_H__
#define __CLNT_H__

#include "main.h"
#include "xrsc.h"
#include <stdarg.h>

#include <X11/Xproto.h>

#ifndef FirstExtensionError
#define FirstExtensionError 128
#endif


#define CLNTENTLEN        32


typedef struct
{
	size_t Left;
	size_t Done;
	char *Mem;
} NETBUF;

typedef struct
{
	size_t Left;						/* bytes to be read */
	size_t Done;						/* number of already written, empty space at beginning */
	size_t Size;
	char *Mem;
} O_BUFF;


/* --- Request Callback Functions --- */
typedef void (*RQSTCB)(p_CLIENT, xReq *xReq);

typedef struct
{
	void (*clnt_reply)(p_CLIENT, CARD32 size, const char *form);
	void (*clnt_error)(p_CLIENT, CARD8 code, CARD8 majOp, CARD16 minOp, CARD32 val);
	void (*event_send)(p_CLIENT, p_WINDOW, CARD16 evnt, va_list);
	void (*grph_shift_arc)(const PXY *origin, xArc *arc, size_t num, short mode);
	void (*grph_shift_pnt)(const PXY *origin, PXY *pxy, size_t num, short mode);
	void (*grph_shift_r2p)(const PXY *origin, GRECT *rct, size_t num);
} FUNCTABL;


typedef struct s_CLIENT
{
	XRSC(CLIENT, _unused);
	int Fd;
	int Port;
	long FdSet;
	void *ConnRW;

	p_CLIENT Next;
	CARD16 SeqNum;
	BOOL DoSwap;
	BYTE CloseDown;
	char *Name;
	char *Addr;
	O_BUFF oBuf;
	NETBUF iBuf;
	RQSTCB Eval;
	const FUNCTABL *Fnct;
	CARD32 EventReffs;
	XRSCPOOL(DRAWABLE, Drawables, 6);
	XRSCPOOL(FONTABLE, Fontables, 5);
	XRSCPOOL(CURSOR, Cursors, 4);
	char Entry[CLNTENTLEN + 2];
} CLIENT;


typedef struct
{
	RQSTCB func;
	const char *Name;
	const char *Form;
} REQUEST;
extern const REQUEST RequestTable[ /*FirstExtensionError */ ];

extern CLIENT *CLNT_Base;
extern CARD16 CLNT_BaseNum;
extern CLIENT *CLNT_Requestor;

typedef XRSCPOOL(CLIENT, CLNT_POOL, 4);
extern CLNT_POOL CLNT_Pool;

void Clnt_EvalSelect_MSB(CLIENT *, xReq *xReq);
void Clnt_EvalSelect_LSB(CLIENT *, xReq *xReq);


static inline CLIENT *ClntFind(CARD32 id)
{
	return Xrsc(CLIENT, RID_Base(id), CLNT_Pool);
}


void ClntInit(BOOL initNreset);
int ClntCreate(int fd, const char *name, const char *addr, int port);
int ClntDelete(CLIENT *);
void *ClntSwap(void *buf, const char *form);
void ClntPrint(CLIENT *, int req, const char *form, ...) __attribute__((format(printf, 3, 4)));
void ClntError(CLIENT *, int err, CARD32 val, int req, const char *form, ...) __attribute__((format(printf, 5, 6)));
void ClntAbort(int code) __attribute__((__noreturn__));

void *ClntOutBuffer(O_BUFF *buf, size_t need, size_t copy_n, BOOL refuse);

#define ClntReplyPtr(T,r,s)  x##T##Reply *r = _clnt_r_ptr (&clnt->oBuf, sz_x##T##Reply + (s))
#define ClntReply(T,s,f)    clnt->Fnct->clnt_reply (clnt, sz_x##T##Reply + (s), f)


#define PRINT( req, frm, args...)   ClntPrint (clnt, req, frm, ## args)

#ifndef NODEBUG
#define Bad( err, val, req, frm, args...) \
	ClntError (clnt, err, val, req, frm, ## args)
#else
#define Bad( err, val, req, frm, args...) \
	clnt->Fnct->clnt_error (clnt, err, req,0, val)
#endif

#ifdef TRACE
#define DEBUG( req, frm, args...)   PRINT (req, frm, ## args)
#else
#define DEBUG( req, frm, args...)
#endif


static inline void *_clnt_r_ptr(O_BUFF *buf, size_t need)
{
	void *r;
	size_t n = buf->Done + buf->Left;

	if (n + need <= buf->Size)
		r = buf->Mem + n;
	else
		r = ClntOutBuffer(buf, need, 0, 1);
	return r;
}

#endif /* __CLNT_H__ */
