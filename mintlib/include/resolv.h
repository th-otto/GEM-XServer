/*
 * Adopted to Mint-Net 1994, Kay Roemer.
 */

/*
 * Copyright (c) 1983, 1987, 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)resolv.h	5.15 (Berkeley) 4/3/91
 */

#ifndef _RESOLV_H_
#define	_RESOLV_H_

#ifndef _COMPILER_H
#include <compiler.h>
#endif

#include <sys/types.h>

/*
 * Resolver configuration file.
 * Normally not present, but may contain the address of the
 * inital name server(s) to query and the domain search list.
 */

#ifndef _PATH_RESCONF
#define _PATH_RESCONF	"/etc/resolv.conf"
#endif

/*
 * Global defines and variables for resolver stub.
 */
#define	MAXNS			3	/* max # name servers we'll track */
#define	MAXDFLSRCH		3	/* # default domain levels to try */
#define	MAXDNSRCH		6	/* max # domains in search path */
#define	LOCALDOMAINPARTS	2	/* min levels in name that is "local" */

#define	RES_TIMEOUT		5	/* min. seconds between retries */

__BEGIN_DECLS

struct state {
	int	retrans;	 	/* retransmition time interval */
	int	retry;			/* number of times to retransmit */
	long	options;		/* option flags - see below. */
	int	nscount;		/* number of name servers */
	struct	sockaddr_in nsaddr_list[MAXNS];	/* address of name server */
#define	nsaddr	nsaddr_list[0]		/* for backward compatibility */
	u_short	id;			/* current packet id */
	char	defdname[MAXDNAME];	/* default domain */
	char	*dnsrch[MAXDNSRCH+1];	/* components of domain to search */
};

/*
 * Resolver options
 */
#define RES_INIT	0x0001		/* address initialized */
#define RES_DEBUG	0x0002		/* print debug messages */
#define RES_AAONLY	0x0004		/* authoritative answers only */
#define RES_USEVC	0x0008		/* use virtual circuit */
#define RES_PRIMARY	0x0010		/* query primary server only */
#define RES_IGNTC	0x0020		/* ignore trucation errors */
#define RES_RECURSE	0x0040		/* recursion desired */
#define RES_DEFNAMES	0x0080		/* use default domain name */
#define RES_STAYOPEN	0x0100		/* Keep TCP socket open */
#define RES_DNSRCH	0x0200		/* search up local domain tree */

#define RES_DEFAULT	(RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

extern struct state _res;
extern int h_errno;

#include <stdio.h>

/* Private routines shared between libc/net, named, nslookup and others. */
#define	dn_skipname	__dn_skipname
#define	fp_query	__fp_query
#define	hostalias	__hostalias
#define	putlong		__putlong
#define	putshort	__putshort
#define p_class		__p_class
#define p_time		__p_time
#define p_type		__p_type

__EXTERN int	__dn_skipname	__PROTO((u_char *__comp_dn, u_char *__eom));
__EXTERN void	__fp_query	__PROTO((char *, FILE *));
__EXTERN char	*__hostalias	__PROTO((__const char *));
__EXTERN void	__PROTOutlong	__PROTO((u_long, u_char *));
__EXTERN void	__PROTOutshort	__PROTO((u_short, u_char *));
__EXTERN char	*__PROTO_class	__PROTO((int));
__EXTERN char	*__PROTO_time	__PROTO((u_long));
__EXTERN char	*__PROTO_type	__PROTO((int));

#if 1
__EXTERN void	__putshort	__PROTO((u_short s, u_char *));
__EXTERN void	__putlong	__PROTO((u_long l, u_char *));
#endif

__EXTERN int	dn_comp		__PROTO((u_char *__exp_dn, u_char *__comp_dn,
				int __length, u_char **__dnptrs,
				u_char **__lastdnptr));

__EXTERN int	dn_expand	__PROTO((u_char *__msg, u_char *__eomorig,
				u_char *__comp_dn, u_char *__exp_dn,
				int __length));
				
__EXTERN int	res_init 	__PROTO((void));

__EXTERN int	res_mkquery 	__PROTO((int __opval, __const char *__dname,
				int __class, int __type, char *__data,
				int __datalen, struct rrec *__newrr,
				char *__buf, int __buflen));
				
__EXTERN int	res_send 	__PROTO((__const char *__msg, int __msglen,
				char *__answer, int __anslen));

__EXTERN int	res_query	__PROTO((__const char *__dname, int __class,
				int __type, u_char *__answer, int __anslen));
				
__EXTERN int	res_search	__PROTO((char *__dname, int __class,
				int __type, u_char *__answer, int __anslen));

__EXTERN int	res_querydomain	__PROTO((char *__name, char *__domain,
				int __class, int __type, u_char *__answer,
				int __anslen));

__EXTERN void	herror __PROTO((__const char *__s));

__END_DECLS

#endif /* !_RESOLV_H_ */
