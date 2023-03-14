//==============================================================================
//
// clnt_P.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-10-03 - Initial Version.
//==============================================================================
//
#ifndef __CLNT_P_H__
# define __CLNT_P_H__

#include "main.h"
#include "clnt.h"
#include "tools.h"

#include <setjmp.h>

#include <X11/X.h>


extern jmp_buf CLNT_Error;


void Clnt_EvalSelect_MSB (CLIENT *, p_xReq);
void Clnt_EvalSelect_LSB (CLIENT *, p_xReq);


#endif __CLNT_P_H__
