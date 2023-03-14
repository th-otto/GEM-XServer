/*
 *==============================================================================
 *
 * clnt.c
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-03 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "clnt_P.h"
#include "server.h"
#include "selection.h"
#include "event.h"
#include "window.h"
#include "fontable.h"
#include "Cursor.h"
#include "grph.h"
#include "wmgr.h"
#include "xrsc.h"
#include "x_gem.h"
#include "x_mint.h"
#include "x_printf.h"
#include "Request.h"


#define O_BLOCK 8192


CARD32 _CLNT_RidBase = RID_MASK + 1;
CARD32 _CLNT_RidCounter = 0;

CLIENT *CLNT_Base = NULL;
CARD16 CLNT_BaseNum = 0;

CLIENT *CLNT_Requestor = NULL;
jmp_buf CLNT_Error;
CLNT_POOL CLNT_Pool;


static void FT_Clnt_reply_MSB(p_CLIENT, CARD32 size, const char *form);
static void FT_Clnt_reply_LSB(p_CLIENT, CARD32 size, const char *form);

static void FT_Clnt_error_MSB(p_CLIENT, CARD8 code, CARD8 majOp, CARD16 minOp, CARD32 val);
static void FT_Clnt_error_LSB(p_CLIENT, CARD8 code, CARD8 majOp, CARD16 minOp, CARD32 val);

static FUNCTABL _CLNT_Functabl_MSB = {
	FT_Clnt_reply_MSB,
	FT_Clnt_error_MSB,
	FT_Evnt_send_MSB,
	FT_Grph_ShiftArc_MSB,
	FT_Grph_ShiftPnt_MSB,
	FT_Grph_ShiftR2P_MSB
};

static FUNCTABL _CLNT_Functabl_LSB = {
	FT_Clnt_reply_LSB,
	FT_Clnt_error_LSB,
	FT_Evnt_send_LSB,
	FT_Grph_ShiftArc_LSB,
	FT_Grph_ShiftPnt_LSB,
	FT_Grph_ShiftR2P_LSB
};


/* ------------------------------------------------------------------------------ */
static void _Clnt_EvalAuth(CLIENT *clnt, xConnClientPrefix *q)
{
	PRINT(0, "ByteOrder = %s, Version = X%i.%i",
		  (clnt->DoSwap ? "IntelCrap" : "MSB"), q->majorVersion, q->minorVersion);

	if (q->nbytesAuthProto || q->nbytesAuthString)
	{
		int i;
		char *p = (char *) &q[1];
		CARD8 *s = (CARD8 *) p + Align(q->nbytesAuthProto);

		x_printf("[%s:%i] Authorization: '%*s' = ", clnt->Addr, clnt->Port, q->nbytesAuthProto, p);
		for (i = 0; i < q->nbytesAuthString; x_printf("%02x", s[i++]))
			;
		x_printf("\n");
	} else
	{
		PRINT(0, "Authorization: <none>");
	}
	/* do some auth stuff */
	if (0)
	{
		xConnSetupPrefix *r = (xConnSetupPrefix *) clnt->oBuf.Mem;

		r->success = BadRequest;
		r->lengthReason = 0;
		r->majorVersion = X_PROTOCOL;
		r->minorVersion = X_PROTOCOL_REVISION;
		r->length = 0;
		clnt->oBuf.Left = sizeof(xConnSetupPrefix);
		clnt->oBuf.Done = 0;

	}
	clnt->oBuf.Left = SrvrSetup(clnt->oBuf.Mem, CNFG_MaxReqLength, clnt->DoSwap, clnt->Id << RID_MASK_BITS);
	clnt->oBuf.Done = 0;
	MAIN_FDSET_wr |= 1uL << clnt->Fd;

	clnt->Eval = (clnt->DoSwap ? Clnt_EvalSelect_LSB : Clnt_EvalSelect_MSB);
	clnt->iBuf.Left = sizeof(xReq);
	clnt->iBuf.Done = 0;

	WmgrClntUpdate(clnt, NULL);
}

/* ------------------------------------------------------------------------------ */
static void _Clnt_EvalInit(CLIENT *clnt, xConnClientPrefix *q)
{
	switch (q->byteOrder)
	{
	case 0x42:
		/* that's nice :-) */
		clnt->Fnct = &_CLNT_Functabl_MSB;
		clnt->DoSwap = xFalse;
		break;
	case 0x6C:
		clnt->Fnct = &_CLNT_Functabl_LSB;
		clnt->DoSwap = xTrue;
		q->majorVersion = Swap16(q->majorVersion);
		q->minorVersion = Swap16(q->minorVersion);
		q->nbytesAuthProto = Swap16(q->nbytesAuthProto);
		q->nbytesAuthString = Swap16(q->nbytesAuthString);
		break;
	default:
		PRINT(0, "Invalid byte order value '%02x'.\n", q->byteOrder);
		longjmp(CLNT_Error, 2);
	}
	if (q->majorVersion != X_PROTOCOL)
	{
		PRINT(0, "Invalid version %i.%i.\n", q->majorVersion, q->minorVersion);
		longjmp(CLNT_Error, 2);
	}
	clnt->iBuf.Left = Align(q->nbytesAuthProto) + Align(q->nbytesAuthString);
	if (clnt->iBuf.Left)
	{
		clnt->Eval = (RQSTCB) _Clnt_EvalAuth;

	} else
	{
		_Clnt_EvalAuth(clnt, q);
	}
}


#if defined(__GNUC__) && (__GNUC__ <= 2)
/*
 * gcc 2.95.3 complains about __retvalue being clobbered by longjmp
 */
#undef trap_1_ww /* used for Finstat & Foutstat */
#define trap_1_ww(n, a)							\
__extension__								\
({									\
	long __retvalue;				\
	short _a = (short)(a);						\
	    								\
	__asm__ volatile						\
	(\
		"\tmovw	%2,sp@-\n" \
		"\tmovw    %1,sp@-\n" \
		"\ttrap    #1\n" \
		"\taddqw   #4,sp\n"						\
		"\tmove.l  %%d0,%0\n"						\
	: "=r"(__retvalue)			/* outputs */		\
	: "g"(n), "r"(_a)			/* inputs  */		\
	: "d0", "d1", "d2", "a0", "a1", "a2"    /* clobbered regs */	\
	  AND_MEMORY							\
	);								\
	__retvalue;							\
})
#undef trap_1_wwll /* used by Fread/Fwrite */
#define trap_1_wwll(n, a, b, c)						\
__extension__								\
({									\
	long __retvalue;				\
	short _a = (short)(a);						\
	long  _b = (long) (b);						\
	long  _c = (long) (c);						\
	    								\
	__asm__ volatile						\
	(\
		"\tmovl    %4,sp@-\n" \
		"\tmovl    %3,sp@-\n" \
		"\tmovw    %2,sp@-\n" \
		"\tmovw    %1,sp@-\n" \
		"\ttrap    #1\n"	\
		"\tlea	sp@(12),sp\n"					\
		"\tmove.l  %%d0,%0\n"						\
	: "=r"(__retvalue)			/* outputs */		\
	: "g"(n), "r"(_a), "r"(_b), "r"(_c)     /* inputs  */		\
	: "d0", "d1", "d2", "a0", "a1", "a2"    /* clobbered regs */	\
	  AND_MEMORY							\
	);								\
	__retvalue;							\
})
#endif

/* ------------------------------------------------------------------------------ */
static size_t _CLNT_MaxObuf = 0;
static BOOL _Clnt_ConnRW(p_CONNECTION conn, BOOL rd, BOOL wr)
{
	int left;

	CLNT_Requestor = conn.Client;

	if ((left = setjmp(CLNT_Error)))
	{
		x_printf("aborted at #%i \n", left);
		ClntDelete(CLNT_Requestor);
	} else
	{
		int fd = CLNT_Requestor->Fd;

		if (rd)
		{
			long n = Finstat(fd);

			if (n < 0)
			{
				longjmp(CLNT_Error, 1);
			} else
			{
				NETBUF *buf = &CLNT_Requestor->iBuf;

				if (n > buf->Left)
					n = buf->Left;
				if ((n = Fread(fd, n, buf->Mem + buf->Done)) > 0)
				{
					buf->Done += n;
					if (!(buf->Left -= n))
					{
						CLNT_Requestor->Eval(CLNT_Requestor, (xReq *) buf->Mem);
					}
				}
			}
		}
		if (CLNT_Requestor->oBuf.Left + CLNT_Requestor->oBuf.Done > _CLNT_MaxObuf)
		{
			_CLNT_MaxObuf = CLNT_Requestor->oBuf.Left + CLNT_Requestor->oBuf.Done;
			x_printf("## oBuf 0x%X -> %lu \n", CLNT_Requestor->Id, _CLNT_MaxObuf);
		}
		if (wr)
		{
			long n = Foutstat(fd);

			if (n < 0)
			{
				longjmp(CLNT_Error, 1);
			} else
			{
				O_BUFF *buf = &CLNT_Requestor->oBuf;

				if (n > buf->Left)
					n = buf->Left;
				if ((n = Fwrite(fd, n, buf->Mem + buf->Done)) > 0)
				{
					if ((buf->Left -= n))
					{
						buf->Done += n;
					} else
					{
						buf->Done = 0;
						MAIN_FDSET_wr &= ~CLNT_Requestor->FdSet;
#if 0
						if (buf->Size > O_BLOCK)
						{
							char *m = malloc(O_BLOCK);

							if (m)
							{
								free(buf->Mem);
								buf->Mem = m;
								buf->Size = O_BLOCK;
							}
						}
#endif
					}
				}
			}
		}
	}
	CLNT_Requestor = NULL;

	return left != 0;
}


/* ============================================================================== */
void ClntInit(BOOL initNreset)
{
	if (initNreset)
	{
		XrscPoolInit(CLNT_Pool);

	} else
	{
		int i;

		for (i = 0; i < XrscPOOLSIZE(CLNT_Pool); ++i)
		{
			p_CLIENT c;

			while ((c = XrscPOOLITEM(CLNT_Pool, i)))
			{
				c->CloseDown = DestroyAll;
				ClntDelete(c);
			}
		}
	}
}

/* ============================================================================== */
int ClntCreate(int fd, const char *name, const char *addr, int port)
{
	CLIENT *clnt = XrscCreate(CLIENT, ++_CLNT_RidCounter, CLNT_Pool, 0);

	x_printf("[%s:%i] Request from %s (at %i)\n", addr, port, name, fd);

	if (clnt)
	{
		clnt->Next = CLNT_Base;
		CLNT_Base = clnt;
		CLNT_BaseNum++;
		clnt->Fd = fd;
		clnt->Port = port;
		clnt->FdSet = 0;
		clnt->ConnRW = _Clnt_ConnRW;
		clnt->SeqNum = 0;
		clnt->DoSwap = xFalse;
		clnt->CloseDown = DestroyAll;
		clnt->Name = strdup(name);
		clnt->Addr = strdup(addr);
		clnt->oBuf.Left = 0;
		clnt->oBuf.Done = 0;
		clnt->oBuf.Size = O_BLOCK;
		clnt->oBuf.Mem = malloc(O_BLOCK);
		clnt->iBuf.Mem = malloc(CNFG_MaxReqBytes);
		clnt->Fnct = NULL;
		clnt->EventReffs = 0;
		XrscPoolInit(clnt->Drawables);
		XrscPoolInit(clnt->Fontables);
		XrscPoolInit(clnt->Cursors);

		if (clnt->Name && clnt->Addr && clnt->oBuf.Mem && clnt->iBuf.Mem)
		{
			SrvrConnInsert((p_CONNECTION) clnt);
			clnt->Eval = (RQSTCB) _Clnt_EvalInit;
			clnt->iBuf.Left = sizeof(xConnClientPrefix);
			clnt->iBuf.Done = 0;
			WmgrClntInsert(clnt);

		} else
		{
			ClntDelete(clnt);
			fd = -1;
			clnt = NULL;
		}
	}
	if (!clnt)
	{
		x_printf("[%s:%i] \033pERROR\033q Memory exhausted.\n", addr, port);
		if (fd >= 0)
			close(fd);
		return xFalse;
	}
	return xTrue;
}

/* ============================================================================== */
int ClntDelete(CLIENT *clnt)
{
	int i;

	if (clnt->Fd >= 0)
	{
		if (clnt->Id)
			PRINT(0, "Connection %s:%i closed.", clnt->Addr, clnt->Port);
		else
			PRINT(0, "Connection closed.");
		SrvrConnRemove((p_CONNECTION) clnt);
		if (CLNT_Base)
		{
			CLIENT **base = &CLNT_Base;

			do
			{
				if (*base == clnt)
				{
					*base = clnt->Next;
					clnt->Next = NULL;
					CLNT_BaseNum--;
					break;
				}
			} while (*(base = &(*base)->Next));
		}
		close(clnt->Fd);
		clnt->Fd = -1;
	}
	SlctRemove(clnt);

	if (clnt->CloseDown == DestroyAll)
	{
		for (i = 0; i < XrscPOOLSIZE(clnt->Drawables); ++i)
		{
			p_DRAWABLE p;

			while ((p = XrscPOOLITEM(clnt->Drawables, i)).p)
				DrawDelete(p, clnt);
		}
		for (i = 0; i < XrscPOOLSIZE(clnt->Fontables); ++i)
		{
			p_FONTABLE p;

			while ((p = XrscPOOLITEM(clnt->Fontables, i)).p)
				FablDelete(p, clnt);
		}
		for (i = 0; i < XrscPOOLSIZE(clnt->Cursors); ++i)
		{
			p_CURSOR p;

			while ((p = XrscPOOLITEM(clnt->Cursors, i)))
				CrsrFree(p, clnt);
		}
	}
	if (clnt->EventReffs)
	{
		WindCleanup(clnt);
		if (clnt->EventReffs)
		{
			/*  x_printf ("\033pFATAL\033q clnt 0x%X holds still %lu event%s!\n", */
			x_printf("\033pERROR\033q clnt 0x%X holds still %lu event%s!\n",
				   clnt->Id, clnt->EventReffs, (clnt->EventReffs == 1 ? "" : "s"));
			/*  exit (1); */
		}
	}

	if (clnt == CLNT_Requestor)
	{
		CLNT_Requestor = NULL;
	}

	if (clnt->CloseDown == DestroyAll)
	{
		WmgrClntRemove(clnt);

		if (clnt->Name)
			free(clnt->Name);
		if (clnt->Addr)
			free(clnt->Addr);
		if (clnt->oBuf.Mem)
			free(clnt->oBuf.Mem);
		if (clnt->iBuf.Mem)
			free(clnt->iBuf.Mem);
		XrscDelete(CLNT_Pool, clnt);
	}

	return CLNT_BaseNum;
}


/* ------------------------------------------------------------------------------ */
void *ClntOutBuffer(O_BUFF *buf, size_t need, size_t copy_n, BOOL refuse)
{
	char *m = NULL;

	if (need <= buf->Size - buf->Left)
	{
		memmove(buf->Mem, buf->Mem + buf->Done, buf->Left + copy_n);
		buf->Done = 0;
		m = buf->Mem + buf->Left;

	} else
	{
		int i = (buf->Left + need + O_BLOCK - 1) / O_BLOCK;
		size_t s = i * O_BLOCK;

		if ((m = malloc(s)))
		{
			CLIENT *clnt = (CLIENT *) ((char *) buf - offsetof(CLIENT, oBuf));

			ClntPrint(clnt, 0, "### output buffer expanded to %li bytes. ###", s);
			if (copy_n += buf->Left)
			{
				memcpy(m, buf->Mem + buf->Done, copy_n);
			}
			free(buf->Mem);
			buf->Mem = m;
			buf->Size = s;
			buf->Done = 0;
			m += buf->Left;

		} else if (refuse)
		{
			CLIENT *clnt = (CLIENT *) ((char *) buf - offsetof(CLIENT, oBuf));

			ClntPrint(clnt, 0, "\033pERROR:\033q memory exhausted in output buffer (%li).", buf->Size + need);
			longjmp(CLNT_Error, 3);

		} else
		{
			CLIENT *clnt = (CLIENT *) ((char *) buf - offsetof(CLIENT, oBuf));

			ClntPrint(clnt, 0, "\033pWARNING:\033q memory exhausted in output buffer (%li).", buf->Size + need);
		}
	}
	return m;
}

/* ------------------------------------------------------------------------------ */
static void FT_Clnt_reply_MSB(CLIENT *clnt, CARD32 size, const char *_unused)
{
	O_BUFF *b = &clnt->oBuf;
	xGenericReply *r = (xGenericReply *) (b->Mem + b->Done + b->Left);

	r->type = X_Reply;
	r->sequenceNumber = clnt->SeqNum;
	r->length = Units(size - sizeof(xReply));

	b->Left += Align(size);
	MAIN_FDSET_wr |= clnt->FdSet;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void FT_Clnt_reply_LSB(CLIENT *clnt, CARD32 size, const char *form)
{
	O_BUFF *b = &clnt->oBuf;
	xGenericReply *r = (xGenericReply *) (b->Mem + b->Done + b->Left);

	r->type = X_Reply;
	r->sequenceNumber = Swap16(clnt->SeqNum);
	r->length = Swap32(Units(size - sizeof(xReply)));

	if (form)
		ClntSwap(&r->data00, form);

	b->Left += Align(size);
	MAIN_FDSET_wr |= clnt->FdSet;
}

/* ------------------------------------------------------------------------------ */
#define xErrorReply    xError
#define sz_xErrorReply sz_xError
static void FT_Clnt_error_MSB(p_CLIENT clnt, CARD8 code, CARD8 majOp, CARD16 minOp, CARD32 val)
{
	ClntReplyPtr(Error, e, 0);

	e->type = X_Error;
	e->errorCode = code;
	e->majorCode = majOp;
	e->minorCode = minOp;
	e->sequenceNumber = clnt->SeqNum;
	e->resourceID = val;

	clnt->oBuf.Left += sizeof(xError);
	MAIN_FDSET_wr |= clnt->FdSet;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void FT_Clnt_error_LSB(p_CLIENT clnt, CARD8 code, CARD8 majOp, CARD16 minOp, CARD32 val)
{
	ClntReplyPtr(Error, e, 0);

	e->type = X_Error;
	e->errorCode = code;
	e->majorCode = majOp;
	e->minorCode = Swap16(minOp);
	e->sequenceNumber = Swap16(clnt->SeqNum);
	e->resourceID = Swap32(val);

	clnt->oBuf.Left += sizeof(xError);
	MAIN_FDSET_wr |= clnt->FdSet;
}


/* ============================================================================== */
#include <stdarg.h>
void ClntPrint(CLIENT *clnt, int req, const char *form, ...)
{
	va_list vlst;
	BOOL head;
	BOOL crlf;

	if ((head = (*form == '+')))
		form++;
	if ((crlf = (*form == '-')))
		form++;

	if (!head)
	{
		if (!clnt)
		{
			clnt = CLNT_Requestor;
		}
		if (!clnt)
		{
			x_printf("---#----: ");
		} else if (clnt->Id > 0)
		{
			x_printf("%03X#%04X: ", clnt->Id << (RID_MASK_BITS & 3), clnt->SeqNum);
		} else if (clnt->Port > 0)
		{
			x_printf("[%s:%i] ", clnt->Addr, clnt->Port);
		} else
		{
			x_printf("[wm]%04X: ", clnt->SeqNum);
		}
		if (req > 0)
			x_printf(RequestTable[req].Name);
		else if (req < 0)
			x_printf("{%s}", RequestTable[-req].Name);
	}
	va_start(vlst, form);
	(*pr_out) (form, vlst);
	va_end(vlst);

	if (!crlf)
		x_printf("\n");
}

/* ============================================================================== */
void ClntError(CLIENT *clnt, int err, CARD32 val, int req, const char *add, ...)
{
	const char *text = "<invalid>";
	const char *v = NULL;

	switch (err)
	{
#	define CASE(e,f) case e: text = #e; v = f; break
		CASE(BadRequest, NULL);
		CASE(BadValue, "");
		CASE(BadWindow, "W");
		CASE(BadPixmap, "P");
		CASE(BadAtom, "A");
		CASE(BadCursor, "C");
		CASE(BadFont, "F");
		CASE(BadMatch, NULL);
		CASE(BadDrawable, "D");
		CASE(BadAccess, NULL);
		CASE(BadAlloc, NULL);
		CASE(BadColor, "H");
		CASE(BadGC, "G");
		CASE(BadIDChoice, " ");
		CASE(BadName, NULL);
		CASE(BadLength, NULL);
		CASE(BadImplementation, NULL);
#	undef CASE
	}
	PRINT(0, "-\033pERROR %s\033q", text);
	if (v)
	{
		if (!*v)
			x_printf(" %li", val);
		else if (*v == ' ')
			x_printf(" %lX", val);
		else
			x_printf(" %c:%lX", *v, val);
	}
	x_printf(" in %s", RequestTable[req].Name);
	if (add[1])
	{
		va_list vlst;

		va_start(vlst, add);
		(*pr_out) (add + 1, vlst);
		va_end(vlst);
	}
	x_printf("\n");

	clnt->Fnct->clnt_error(clnt, err, req, 0, val);
}


/* ============================================================================== */
/* */
/* Callback Functions */

/* ------------------------------------------------------------------------------ */
void RQ_KillClient(CLIENT *clnt, xKillClientReq *q)
{
	CARD32 c_id = q->id & ~RID_MASK;

	if (q->id == AllTemporary)
	{
		int i;

		PRINT(X_KillClient, "(AllTemporary)");

		for (i = 0; i < XrscPOOLSIZE(CLNT_Pool); ++i)
		{
			p_CLIENT c = XrscPOOLITEM(CLNT_Pool, i);

			while (c)
			{
				if (c->CloseDown == AllTemporary && c->Fd < 0)
				{
					c->CloseDown = DestroyAll;
					ClntDelete(c);
					c = XrscPOOLITEM(CLNT_Pool, i);
				} else
				{
					c = c->NextXRSC;
				}
			}
		}
	} else if (!c_id)
	{
		int owner = -1;

		if (!(q->id & 0x8000) || ((owner = wind_get_one(q->id & 0x7FFF, WF_OWNER)) < 0))
		{
			Bad(BadValue, q->id, X_KillClient, "_");
		} else
		{
			short msg[8] = { WM_CLOSED, gl_apid, 0, q->id & 0x7FFF, 0, 0, 0, 0 };
			DEBUG(X_KillClient, "(W:%lX)", q->id);
			appl_write(owner, 16, msg);
		}
	} else
	{
		CLIENT *kill = Xrsc(CLIENT, RID_Base(c_id), CLNT_Pool);

		if (kill)
		{
			DEBUG(X_KillClient, "(%lX)", q->id);
			ClntDelete(kill);
		} else
		{
			Bad(BadValue, q->id, X_KillClient, "_");
		}
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_SetCloseDownMode(CLIENT *clnt, xSetCloseDownModeReq *q)
{
	/*
	 * Define what will happen to the client's resources at connection close.
	 *
	 * BYTE mode: DestroyAll, RetainPermanent, RetainTemporary
	 *...........................................................................
	 */
	if (q->mode > 2)
	{
		Bad(BadValue, q->mode, X_SetCloseDownMode, "_");
	} else
	{
		PRINT(X_SetCloseDownMode, " '%s'",
			  (q->mode == DestroyAll ? "DestroyAll" :
			   q->mode == RetainPermanent ? "RetainPermanent" : "RetainTemporary"));
		clnt->CloseDown = q->mode;
	}
}


/* ------------------------------------------------------------------------------ */
void RQ_AllowEvents(CLIENT *clnt, xAllowEventsReq *q)
{
	PRINT(-X_AllowEvents, " T:%lX mode=%i", q->time, (int) q->mode);
}

/* ------------------------------------------------------------------------------ */
void RQ_GetMotionEvents(CLIENT *clnt, xGetMotionEventsReq *q)
{
	PRINT(-X_GetMotionEvents, " W:%lX T:%lX - T:%lX", q->window, q->start, q->stop);
}
