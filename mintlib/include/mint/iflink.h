#ifndef _IFLINK_H
#define _IFLINK_H

#ifndef _FEATURES_H
# include _FEATURES_H
#endif

__EXTERN int	if_link		__PROTO ((char*, char*));
__EXTERN int	if_getlnkflags	__PROTO ((char*, short*));
__EXTERN int	if_setlnkflags	__PROTO ((char*, short));
__EXTERN int	if_getifflags	__PROTO ((char*, short*));
__EXTERN int	if_setifflags	__PROTO ((char*, short));

#endif
