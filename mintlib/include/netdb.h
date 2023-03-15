/*
 * Adopted to Mint-Net 1994, Kay Roemer.
 */

/*-
 * Copyright (c) 1980, 1983, 1988 Regents of the University of California.
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
 *	@(#)netdb.h	5.15 (Berkeley) 4/3/91
 */

#ifndef _NETDB_H_
#define _NETDB_H_

#ifndef _COMPILER_H
#include <compiler.h>
#endif

#define	_PATH_HEQUIV	"/etc/hosts.equiv"
#define	_PATH_HOSTS	"/etc/hosts"
#define	_PATH_NETWORKS	"/etc/networks"
#define	_PATH_PROTOCOLS	"/etc/protocols"
#define	_PATH_SERVICES	"/etc/services"
#define _PATH_HOSTCONF	"/etc/host.conf"

__BEGIN_DECLS

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	unsigned long	n_net;		/* network # */
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct	protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

/*
 * DO NOT USE -mshort with this !! */
struct rpcent {
	char	*r_name;	/* name of server for this rpc program */
	char	**r_aliases;	/* alias list */
	int	r_number;	/* rpc program number */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */
extern int h_errno;

#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */

__EXTERN void			endhostent	__PROTO((void));
__EXTERN void			endnetent	__PROTO((void));
__EXTERN void			endprotoent	__PROTO((void));
__EXTERN void			endservent	__PROTO((void));
__EXTERN struct hostent		*gethostbyaddr	__PROTO((const char *, int, int));
__EXTERN struct hostent		*gethostbyname	__PROTO((const char *));
__EXTERN struct hostent 	*gethostent	__PROTO((void));
__EXTERN struct netent		*getnetbyaddr	__PROTO((long, int));
__EXTERN struct netent		*getnetbyname	__PROTO((const char *));
__EXTERN struct netent		*getnetent	__PROTO((void));
__EXTERN struct protoent	*getprotobyname __PROTO((const char *));
__EXTERN struct protoent	*getprotobynumber __PROTO((int));
__EXTERN struct protoent	*getprotoent	__PROTO((void));
__EXTERN struct servent		*getservbyname	__PROTO((const char *, const char *));
__EXTERN struct servent		*getservbyport	__PROTO((int, const char *));
__EXTERN struct servent		*getservent	__PROTO((void));
__EXTERN void			herror		__PROTO((const char *));
__EXTERN void			sethostent	__PROTO((int));
__EXTERN void			setnetent	__PROTO((int));
__EXTERN void			setprotoent	__PROTO((int));
__EXTERN void			setservent	__PROTO((int));

__EXTERN struct rpcent		*getrpcbyname	__PROTO((char *));
__EXTERN struct rpcent		*getrpcbynumber	__PROTO((int));
__EXTERN struct rpcent		*getrpcent	__PROTO((void));
__EXTERN void			setrpcent	__PROTO((int));
__EXTERN void			endrpcent	__PROTO((void));

__END_DECLS

#endif /* !_NETDB_H_ */
