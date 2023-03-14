//==============================================================================
//
// Pointer.c -- Handling of Mouse Pointer Behavior.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-02 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "clnt.h"
#include "event.h"
#include "Pointer.h"

#include <X11/X.h>
#include <X11/Xproto.h>


#define PNTR_MAP 3
CARD16 PNTR_Mapping[PNTR_MAP+1] = { 0, Button1Mask, Button2Mask, Button3Mask };
CARD8  PNTR_Table  [PNTR_MAP+1] = { 0, Button1,     Button2,     Button3     };


//==============================================================================
void
PntrInit (BOOL initNreset)
{
	if (!initNreset) {
		PNTR_Mapping[1] = 1 << (7 + (PNTR_Table[1] = Button1));
		PNTR_Mapping[2] = 1 << (7 + (PNTR_Table[2] = Button2));
		PNTR_Mapping[3] = 1 << (7 + (PNTR_Table[3] = Button3));
	}
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_SetPointerMapping (CLIENT * clnt, xSetPointerMappingReq * q)
{
	// Change mapping of physical to logical pointer button
	//
	// CARD8 nElts:               Number of buttons
	CARD8 * but = (CARD8*)(q +1); // list of buttons
	//
	// Reply:
	// CARD8 success: MappingSuccess or MappingBusy
	//...........................................................................
	
	if (q->nElts != PNTR_MAP) {
		Bad(Value, q->nElts, SetPointerMapping,"(): wrong button count.");
	
	} else if ((but[0] && (but[0] == but[1] || but[0] == but[2])) ||
	           (but[1] &&  but[1] == but[2])) {
		Bad(Value, (but[1] &&  but[1] == but[2] ? but[1] : but[0]),
		           SetPointerMapping,"():\n          duplicate button %i,%i,%i.",
		           but[0], but[1], but[2]);
	
	} else { //..................................................................
	
		ClntReplyPtr (SetPointerMapping, r,);
		
		if (MAIN_But_Mask) {
			DEBUG (SetPointerMapping," ignored %i,%i,%i.", but[0], but[1], but[2]);
			r->success = MappingBusy;
			ClntReply (SetPointerMapping,,NULL);
			
		} else {
			int i;
			DEBUG (SetPointerMapping,"() %i,%i,%i -> %i,%i,%i.",
			       PNTR_Table[1], PNTR_Table[2], PNTR_Table[3],
			       but[0], but[1], but[2]);
			for (i = 0; i < PNTR_MAP; ++i) {
				if (but[i]) {
					PNTR_Mapping[i+1] = 1 << (7 + (PNTR_Table[i+1] = but[i]));
				} else {
					PNTR_Mapping[i+1] = PNTR_Table[i+1] = 0;
				}
			}
			r->success = MappingSuccess;
			ClntReply (SetPointerMapping,,NULL);
			
			EvntMappingNotify (MappingPointer, 0,0);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetPointerMapping (CLIENT * clnt, xGetPointerMappingReq * _unused_)
{
	// Returns mapping of physical to logical pointer button
	//
	// Reply:
	// CARD8 nElts:    Number of buttons
	// (CARD8*)(r +1): List of buttons
	//...........................................................................
	
	ClntReplyPtr (GetPointerMapping, r, PNTR_MAP * sizeof(CARD8));
	int i;
	
	DEBUG (GetPointerMapping," ");
	
	for (i = 0; i < PNTR_MAP; ++i) {
		((CARD8*)(r +1))[i] = PNTR_Table[i+1];
	}
	r->nElts = PNTR_MAP;
	
	ClntReply (GetPointerMapping, r->nElts, NULL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ChangePointerControl (CLIENT * clnt, xChangePointerControlReq * q)
{
	// Change pointer accelaration - not supported
	//
	// INT16 accelNum, accelDenum:
	// INT16 threshold:
	// BOOL doAccel, doThresh:
	//...........................................................................
	
	if (q->accelNum < -1  ||  q->accelDenum <= 0) {
		Bad(Value, (q->accelNum < -1 ? q->accelNum : q->accelDenum),
		           ChangePointerControl,"(): %s.",
		           (q->accelNum < -1 ? "numerator" : "denominator"));
		
	} else {
		PRINT (- X_ChangePointerControl,": %s = %i/%i, %s = %i",
		       (q->doAccel ? "accel" : "(accel)"), q->accelNum, q->accelDenum,
		       (q->doThresh ? "treshold" : "(treshold)"), q->threshold);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetPointerControl (CLIENT * clnt, xGetPointerControlReq * q)
{
	// Returns current pointer acceleration and treshold - always default values
	//
	// Reply:
	// CARD16 accelNumerator, accelDenominator:
	// CARD16 threshold:
	//...........................................................................
	
	ClntReplyPtr (GetPointerControl, r,);
	
	DEBUG (GetPointerControl," ");
	
	r->accelNumerator   = 1;
	r->accelDenominator = 1;
	r->threshold        = 1;
	ClntReply (GetPointerControl,, ":.");
}
