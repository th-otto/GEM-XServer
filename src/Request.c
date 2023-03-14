//==============================================================================
//
// request.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-09-09 - Initial Version.
//==============================================================================
//
#include "clnt_P.h"
#include "x_mint.h"

#include <stdio.h> // printf

#include <X11/Xproto.h>


//------------------------------------------------------------------------------
static void
_Clnt_EvalFunction_MSB (CLIENT * clnt, xReq * q)
{
	(*RequestTable[q->reqType].func) (clnt, q);
	
	clnt->Eval      = Clnt_EvalSelect_MSB;
	clnt->iBuf.Left = sizeof(xReq);
	clnt->iBuf.Done = 0;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void
_Clnt_EvalFunction_LSB (CLIENT * clnt, xReq * q)
{
	if (RequestTable[q->reqType].Form) {
		ClntSwap (q +1, RequestTable[q->reqType].Form);
	}
	(*RequestTable[q->reqType].func) (clnt, q);
	
	clnt->Eval      = Clnt_EvalSelect_LSB;
	clnt->iBuf.Left = sizeof(xReq);
	clnt->iBuf.Done = 0;
}

//------------------------------------------------------------------------------
static BOOL
_Clnt__EvalSelect (CLIENT * clnt, xReq * q)
{
	clnt->SeqNum++;
	
	if (q->reqType >= FirstExtensionError) {
		PRINT (,"Invalid extension request '%0x'.", q->reqType);
	printf("!!! %p %p \n", q, clnt->iBuf.Mem);
	exit(1);
		longjmp (CLNT_Error, 2);
	}
	if (q->length > CNFG_MaxReqLength || !q->length) {
		PRINT (,"\33pError\33q Bad request length %u/%lu in '%s'.",
		       q->length, CNFG_MaxReqLength, RequestTable[q->reqType].Name);
		longjmp (CLNT_Error, 2);
	}

#if 0
if (q->reqType != X_GetInputFocus) {
	printf("==> %s \n", RequestTable[q->reqType].Name);
}
# if 0
if (!ClntFind(clnt->Id << RID_MASK_BITS)) {
	printf("\n CLNT not found !!! \n\n");
	exit(1);
} else {
	Xrsc(void, 11, clnt->Drawables);
	Xrsc(void, 12, clnt->Fontables);
	Xrsc(void, 13, clnt->Cursors);
}
# endif
#endif
	
	if (q->length == 1) {
		// no swapping neccessary, function pointer of clnt is still EvalSelect
		(*RequestTable[q->reqType].func) (clnt, q);
		clnt->iBuf.Left = sizeof(xReq);
		clnt->iBuf.Done = 0;
		
		return xFalse;
	
	} else {
		long n = Finstat (clnt->Fd);
		clnt->iBuf.Left = (q->length -1) *4;
		if (n > 0) {
			if (n > clnt->iBuf.Left) n = clnt->iBuf.Left;
			if ((n = Fread (clnt->Fd, n, q +1)) > 0) {
				clnt->iBuf.Done += n;
				clnt->iBuf.Left -= n;
			}
		}
	}
	return xTrue;
}

//==============================================================================
void
Clnt_EvalSelect_MSB (CLIENT * clnt, xReq * q)
{
	if (_Clnt__EvalSelect (clnt, q)) {
		if (clnt->iBuf.Left) {
			clnt->Eval = _Clnt_EvalFunction_MSB;
		} else {
			_Clnt_EvalFunction_MSB (clnt, q);
		}
	}
}

//==============================================================================
void
Clnt_EvalSelect_LSB (CLIENT * clnt, xReq * q)
{
	q->length = Swap16(q->length);
	
	if (_Clnt__EvalSelect (clnt, q)) {
		if (clnt->iBuf.Left) {
			clnt->Eval = _Clnt_EvalFunction_LSB;
		} else {
			_Clnt_EvalFunction_LSB (clnt, q);
		}
	}
}


//==============================================================================
#include "Request.h"

const REQUEST RequestTable[FirstExtensionError] = {
	{NULL, "Failure", NULL},
#	define REQUEST_DEF(f,s) { (RQSTCB)RQ_##f, #f, s },
#	include "Request.h"
};
