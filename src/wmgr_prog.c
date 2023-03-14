//==============================================================================
//
// wmgr_prog.c -- Execution of client programs.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-07-10 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "wmgr.h"
#include "x_mint.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <dirent.h>
#include <sys/stat.h>


//------------------------------------------------------------------------------
static char *
tos2unix (char * path)
{
	char * unx = path;
	
	if (path[1] == ':') {
		char drv = toupper (path[0]);
		int  len = -1;
		char buf[NAME_MAX+1] = "/";
		
		if (drv == 'U') {
			len = 0;
		
		} else if (drv >= 'A'  &&  drv <= 'Z') {
			int         dev = drv - 'A';
			struct stat s;
			buf[1] = dev + 'a';
			buf[2] = '\0';
		
			if (!stat (buf, &s) &&  s.st_dev == dev) {
				len = 2;
		
			} else {
				DIR * dir = opendir ("/");
				if (dir) {
					struct dirent * ent;
					while ((ent = readdir (dir))) {
						int  l = ent->d_namlen +1;
						memcpy (buf +1, ent->d_name, l);
						if (!stat (buf, &s) &&  s.st_dev == dev) {
							len = l;
							break;
						}
					}
					closedir (dir);
				}
			}
		}
		if (len >= 0  && (unx = malloc (len + strlen (path += 2)))) {
			char * u, * p;
			if (len) u = (char*)memcpy (unx, buf, len) + len;
			else     u = unx;
			*(u++) = '/';
			while ((p = strchr (++path, '\\'))) {
				int l = p - path;
				u      = (char*)memcpy (u, path, l) + l;
				*(u++) = '/';
				path   = p;
			}
			strcpy (u, path);
		}
	}
	return unx;
}

//==============================================================================
short
WmgrLaunch (const char * prg, int argc, const char * arg_v[])
{
	extern char ** environ;
	char        ** src = environ;
	short          pid = -1;
	const char   * cmd = "\x7F\0";
	char         * env, * unx;
	size_t         len,   u_l;
	int            i;
	
	char * argv[argc];
	
	if (!(unx = tos2unix ((char*)prg))) {
		return -1;
	}
	
	u_l = strlen (unx) +1;
	len = u_l +6 +1;
	for (i = 0; src[i]; i++) {
		if (!strncmp (src[i], "ARGV=", 5)) break;
		len += strlen (src[i]) +1;
	}
	for (i = 0; i < argc; i++) {
		if (!(argv[i] = tos2unix ((char*)arg_v[i]))) {
			argc = i;
			break;
		}
		len += strlen (argv[i]) +1;
	}
	if ((env = malloc (len))) {
		char * e = env;
		for (i = 0; src[i]; i++) {
			size_t l = strlen (src[i]) +1;
			memcpy (e, src[i], l);
			e += l;
		}
		memcpy (e, "ARGV=", 6);
		e += 6;
		memcpy (e, unx, u_l);
		e += u_l;
		for (i = 0; i < argc; i++) {
			size_t l = strlen (argv[i]) +1;
			memcpy (e, argv[i], l);
			e += l;
		}
		*e = '\0';
		pid = Pexec (100, prg, cmd, env);
		
		free (env);
	}
	
	for (i = 0; i < argc; i++) {
		if (argv[i] != arg_v[i]) free (argv[i]);
	}
	if (unx != prg) free (unx);
	
	return pid;
}
