//==============================================================================
//
// x_printf.c -- override printf for buffered output.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-07-11 - Initial Version.
//==============================================================================
//
#define _GNU_SOURCE
# include <stdio.h>
#undef _GNU_SOURCE
#include <stdlib.h>
#include <stdarg.h>

#include <X11/Xproto.h>


void set_printf (BOOL buffer);
int  print_f (const char * format, ...);

struct PR_ENT {
	struct PR_ENT * next;
	char          * text;
};

static struct PR_ENT  * pr_ent_beg = NULL,
                     ** pr_ent_end = &pr_ent_beg;

/*static*/ int (*pr_out)(const char * , va_list ) = vprintf;


//------------------------------------------------------------------------------
static int
pr_flush (const char * format, va_list args)
{
	pr_out = vprintf;
	
	while (pr_ent_beg) {
		struct PR_ENT * next = pr_ent_beg->next;
		printf ("%s", pr_ent_beg->text);
		free (pr_ent_beg);
		pr_ent_beg = next;
	}
	pr_ent_end = &pr_ent_beg;
	
	return vprintf (format, args);
}

//------------------------------------------------------------------------------
static int
pr_buffer (const char * format, va_list args)
{
	BOOL            ok = xFalse;
	struct PR_ENT * pr = malloc (sizeof (struct PR_ENT));
	int             n  = -1;
	
	if (pr) {
		pr->next = NULL;
		n = vasprintf (&pr->text, format, args);
		if (pr->text) ok = xTrue;
		else          free (pr);
	}
	if (ok) {
		*pr_ent_end = pr;
		pr_ent_end  = &pr->next;
		pr->next    = NULL;
	} else {
		n = pr_flush (format, args);
	}
	return n;
}


//==============================================================================
void
set_printf (BOOL buffer)
{
	pr_out = (buffer ? pr_buffer : pr_flush);
}

//==============================================================================
int
printf (const char * format, ...)
{
	int     n;
	va_list args;
	va_start      (args, format);
	n = (*pr_out) (format, args);
	va_end        (args);
	
	return n;
}
