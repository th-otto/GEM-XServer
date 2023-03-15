/* Machine-dependent SIGPROF signal handler.  "Generic" version w/ sigcontext
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

/* In many Unix systems signal handlers are called like this
   and the interrupted PC is easily findable in the `struct sigcontext'.  */

/* FIXME !!!
 * this is the real sigcontext under MINT
 */
struct mint_sigcontext
{
  ulong sc_pc;
  ulong sc_usp;
  ushort sc_sr;
};

static void
profil_counter (int signr, int code, struct mint_sigcontext *scp)
{
  profil_count ((void *) scp->sc_pc);
}
