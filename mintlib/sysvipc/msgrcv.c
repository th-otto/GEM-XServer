/* Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* Modified for MiNTLib by Frank Naumann <fnaumann@freemint.de>.  */

#include <sys/msg.h>
#include <errno.h>
#include <mintbind.h>

/* Read a message from the queue associated with the message queue
   descriptor MSQID.  At most MSGSZ bytes of the message are placed
   in the buffer specified by the MSGP parameter.  The MSGTYP parameter
   describes which message is returned in MSGFLG describes the behaviour
   in buffer overflow or queue underflow.  */

int
__msgrcv (msqid, msgp, msgsz, msgtyp, msgflg)
     int msqid;
     void *msgp;
     size_t msgsz;
     long msgtyp;
     int msgflg;
{
	int ret;
	
	ret = Pmsgrcv (msqid, msgp, msgsz, msgtyp, msgflg);
	if (ret < 0) {
		__set_errno (-ret);
		return -1;
	}
	return ret;
}

weak_alias (__msgrcv, msgrcv)
