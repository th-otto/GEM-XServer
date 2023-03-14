//==============================================================================
//
// selection.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-08-31 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "clnt.h"
#include "tools.h"
#include "selection.h"
#include "Atom.h"
#include "window.h"
#include "event.h"


//==============================================================================
void
SlctRemove (CLIENT * clnt)
{
	// Remove all selections of that client
	//...........................................................................
	
	int i;
	for (i = 0; i <= ATOM_Count; ++i) {
		if (ATOM_Table[i]->SelOwner == clnt) {
			ATOM_Table[i]->SelWind->nSelections--;
			ATOM_Table[i]->SelWind  = NULL;
			ATOM_Table[i]->SelOwner = NULL;
		}
	}
}

//==============================================================================
void
SlctClear (WINDOW * wind)
{
	// Remove all selections with that window
	//...........................................................................
	
	int i;
	for (i = 0; i <= ATOM_Count; ++i) {
		if (ATOM_Table[i]->SelWind == wind) {
			EvntSelectionClear (ATOM_Table[i]->SelOwner, wind->Id, i);
			ATOM_Table[i]->SelOwner = NULL;
			ATOM_Table[i]->SelWind  = NULL;
			if (!--wind->nSelections) break;
		}
	}
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//------------------------------------------------------------------------------
void
RQ_SetSelectionOwner (CLIENT * clnt, xSetSelectionOwnerReq * q)
{
	// Set or clear a selection.  A probably previous owner will be notified.
	//
	// Atom   selection: to be set or cleared
	// Window window:    owner or None for clear
	// Time   time:      change time or CurrentTime
	//...........................................................................
	
	WINDOW * wind = NULL;
	
	if (!AtomValid(q->selection)) {
		Bad(Atom, q->selection, SetSelectionOwner,"(W:%lX)", q->window);
	
	} else if (q->window != None  &&  !(wind = WindFind(q->window))) {
		Bad(Window, q->window, SetSelectionOwner,"():\n          not %s.", 
		                       (DBG_XRSC_TypeError ? "a window" : "found"));
		
	} else { //..................................................................
		
		Time   time = (q->time == CurrentTime ? MAIN_TimeStamp : q->time);
		ATOM * atom = ATOM_Table[q->selection];
		
		if (time < atom->SelTime  || time > MAIN_TimeStamp) {
			PRINT (SetSelectionOwner," of '%s' to W:%lX\n"
			       "          ignored :T:%lu %s T:%lu.",
			       atom->Name, q->window, time,
			       (time > MAIN_TimeStamp ? "> server time" : "< last change"),
			       (time > MAIN_TimeStamp ? MAIN_TimeStamp  : atom->SelTime));
			
		} else {
			DEBUG (SetSelectionOwner," '%s' to W:%lX (T:%lu)",
			       atom->Name, q->window, time);
			
			if (atom->SelOwner) {
				if (atom->SelOwner != clnt) {
					EvntSelectionClear (atom->SelOwner,
					                    atom->SelWind->Id, q->selection);
				}
				atom->SelWind->nSelections--;
			}
			
			if ((atom->SelWind = wind)) {
				atom->SelOwner = clnt;
				wind->nSelections++;
			} else {
				atom->SelOwner = NULL;
			}
			atom->SelTime = time;
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_GetSelectionOwner (CLIENT * clnt, xGetSelectionOwnerReq * q)
{
	// Get the owner window of a selection
	//
	// CARD32 id: requested selection
	//
	// Reply:
	// Window owner: owner or None
	//...........................................................................
	
	if (!AtomValid(q->id)) {
		Bad(Atom, q->id, GetSelectionOwner,);
	
	} else { //..................................................................
		
		ClntReplyPtr (GetSelectionOwner, r,);
		ATOM * atom = ATOM_Table[q->id];
		
		DEBUG (GetSelectionOwner," of '%s' (W:%lX)",
		       atom->Name, (atom->SelWind ? atom->SelWind->Id : None));
		
		r->owner = (atom->SelWind ? atom->SelWind->Id : None);
		
		ClntReply (GetSelectionOwner,, "w");
	}
}

//------------------------------------------------------------------------------
void
RQ_ConvertSelection (CLIENT * clnt, xConvertSelectionReq * q)//, WINDOW * wind)
{
	// Request the conversion of a selection to a target
	//
	// Atom   selection: to be converted
	// Atom   target:    result type
	// Atom   property:  value to be converted or None
	// Window requestor: where the conversion result should go to
	// Time   time:      timestamp or CurrentTime
	//...........................................................................
	
	WINDOW * wind = WindFind (q->requestor);
	
	if (!wind) {
		Bad(Window, q->requestor, ConvertSelection,"():\n          not %s.",
		                          (DBG_XRSC_TypeError ? "a window" : "found"));
		
	} else if (!AtomValid(q->selection)) {
		Bad(Atom, q->selection, ConvertSelection,"(W:%lX):\n"
		                        "          invalid selection.", q->requestor);
		
	} else if (!AtomValid(q->target)) {
		Bad(Atom, q->target, ConvertSelection,"(W:%lX,'%s'):\n"
		                     "          invalid target.",
		                     q->requestor, ATOM_Table[q->selection]->Name);
		
	} else if (q->property != None  && !AtomValid(q->property)) {
		Bad(Atom, q->property, ConvertSelection,"(W:%lX,'%s','%s'):\n"
		                       "          invalid property.", q->requestor,
		                       ATOM_Table[q->selection]->Name,
		                       ATOM_Table[q->target]->Name);
	
	} else { //..................................................................
		
		ATOM * slct = ATOM_Table[q->selection];
		
		DEBUG (ConvertSelection," '%s' with '%s' for W:%lX(%s)", slct->Name,
		       (q->property == None
		                       ? "<none>" : (char*)ATOM_Table[q->property]->Name),
		       q->requestor, ATOM_Table[q->target]->Name);
		
		if (slct->SelOwner) {
			EvntSelectionRequest (slct->SelOwner, q->time,
			                      slct->SelWind->Id, q->requestor,
			                      q->selection, q->target, q->property);
		} else {
			EvntSelectionNotify (clnt, q->time, q->requestor,
			                     q->selection, q->target, None);
		}
	}
}
