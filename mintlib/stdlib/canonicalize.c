/* Return the canonical absolute name of a given file.
   Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
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
 
/* Modified by Guido Flohr <gufl0000@stud.uni-sb.de> for MiNTLib.  */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#ifdef __TURBOC__
# include <sys\param.h>
# include <sys\stat.h>
#else
# include <sys/param.h>
# include <sys/stat.h>
#endif
#include <errno.h>

#ifdef __MINT__
# define __lxstat(version, file, buf) __lstat(file, buf)
# define _STAT_VER

__EXTERN int __lstat __PROTO ((const char*, struct stat*));
__EXTERN char* __getcwd __PROTO ((char*, int));
__EXTERN void* __mempcpy __PROTO ((void*, const void*, size_t));
#endif

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated path
   separators ('/') or symlinks.  All path components must exist.  If
   RESOLVED is null, the result is malloc'd; otherwise, if the
   canonical name is PATH_MAX chars or more, returns null with `errno'
   set to ENAMETOOLONG; if the name fits in fewer than PATH_MAX chars,
   returns the name in RESOLVED.  If the name cannot be resolved and
   RESOLVED is non-NULL, it contains the path of the first component
   that cannot be resolved.  If the path can be resolved, RESOLVED
   holds the same value as the value returned.  */

static char *
canonicalize (const char *name, char *resolved)
{
  char *rpath, *dest, *extra_buf = NULL;
  const char *start, *end, *rpath_limit;
  long int path_max;
  int num_links = 0;

  if (name == NULL)
    {
      /* As per Single Unix Specification V2 we must return an error if
	 either parameter is a null pointer.  We extend this to allow
	 the RESOLVED parameter be NULL in case the we are expected to
	 allocate the room for the return value.  */
      __set_errno (EINVAL);
      return NULL;
    }

  if (name[0] == '\0')
    {
      /* As per Single Unix Specification V2 we must return an error if
	 the name argument points to an empty string.  */
      __set_errno (ENOENT);
      return NULL;
    }

#ifdef PATH_MAX
  path_max = PATH_MAX;
#else
  path_max = pathconf (name, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 1024;
#endif

  rpath = resolved ? alloca (path_max) : malloc (path_max);
  rpath_limit = rpath + path_max;

  if (name[0] != '/' || name[0] == '\\')
    {
      if (!__getcwd (rpath, path_max))
	goto error;
      dest = strchr (rpath, '\0');
    }
  else
    {
      rpath[0] = '/';
      dest = rpath + 1;
    }

  for (start = end = name; *start; start = end)
    {
      struct stat st;
      int n;

      /* Skip sequence of multiple path-separators.  */
      while (*start == '/' || *start == '\\')
	++start;

      /* Find end of path component.  */
      for (end = start; *end && *end != '/' && *end != '\\'; ++end)
	/* Nothing.  */;

      if (end - start == 0)
	break;
      else if (end - start == 1 && start[0] == '.')
	/* nothing */;
      else if (end - start == 2 && start[0] == '.' && start[1] == '.')
	{
	  /* Back up to previous component, ignore if at root already.  */
	  if (dest > rpath + 1)
     	    while ((--dest)[-1] != '/' && dest[-1] != '\\');
	}
      else
	{
	  size_t new_size;

	  if (dest[-1] != '/' && dest[-1] != '\\')
	    *dest++ = '/';

	  if (dest + (end - start) >= rpath_limit)
	    {
	      long dest_offset = dest - rpath;

	      if (resolved)
		{
		  __set_errno (ENAMETOOLONG);
		  goto error;
		}
	      new_size = rpath_limit - rpath;
	      if (end - start + 1 > path_max)
		new_size += end - start + 1;
	      else
		new_size += path_max;
	      rpath = realloc (rpath, new_size);
	      rpath_limit = rpath + new_size;
	      if (rpath == NULL)
		return NULL;

	      dest = rpath + dest_offset;
	    }

	  dest = __mempcpy (dest, start, end - start);
	  *dest = '\0';

	  if (__lxstat (_STAT_VER, rpath, &st) < 0)
	    goto error;

	  if (S_ISLNK (st.st_mode))
	    {
	      char *buf = alloca (path_max);
	      size_t len;

#ifdef __MINT__
# ifndef MAXSYMLINKS
#  define MAXSYMLINKS 4
# endif
#endif
	      if (++num_links > MAXSYMLINKS)
		{
		  __set_errno (ELOOP);
		  goto error;
		}

	      n = __readlink (rpath, buf, path_max);
	      if (n < 0)
		goto error;
	      buf[n] = '\0';

	      if (!extra_buf)
		extra_buf = alloca (path_max);

	      len = strlen (end);
	      if ((long int) (n + len) >= path_max)
		{
		  __set_errno (ENAMETOOLONG);
		  goto error;
		}

	      /* Careful here, end may be a pointer into extra_buf... */
	      memmove (&extra_buf[n], end, len + 1);
	      name = end = memcpy (extra_buf, buf, n);

	      if (buf[0] == '/' || buf[0] == '\\')
		dest = rpath + 1;	/* It's an absolute symlink */
	      else
		/* Back up to previous component, ignore if at root already: */
		if (dest > rpath + 1)
		  while ((--dest)[-1] != '/' && dest[-1] != '\\');
	    }
	}
    }
  if (dest > rpath + 1 && (dest[-1] == '/' || dest[-1] == '\\'))
    --dest;
  *dest = '\0';

  return resolved ? memcpy (resolved, rpath, dest - rpath + 1) : rpath;

error:
  if (resolved)
    strcpy (resolved, rpath);
  else
    free (rpath);
  return NULL;
}


char *
__realpath (const char *name, char *resolved)
{
  if (resolved == NULL)
    {
      __set_errno (EINVAL);
      return NULL;
    }

  return canonicalize (name, resolved);
}
weak_alias (__realpath, realpath)

char *
__canonicalize_file_name (const char *name)
{
  return canonicalize (name, NULL);
}
weak_alias (__canonicalize_file_name, canonicalize_file_name)
