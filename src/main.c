/*
 *==============================================================================
 *
 * main.c
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-05-30 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mint/ssystem.h>

#include "main.h"
#include "tools.h"
#include "server.h"
#include "Pointer.h"
#include "keyboard.h"
#include "wmgr.h"
#include "x_gem.h"
#include "x_mint.h"
#include "x_printf.h"
#include "window_P.h"
#include "version.h"

#include <X11/X.h>
#include <X11/Xproto.h>

static void sig_child(int sig);
static void shutdown(void);


static EVMULT_IN ev_i = {
	MU_MESAG | MU_TIMER | MU_BUTTON | MU_KEYBD,
	0x0102, 0x03, 0x00,
	MO_ENTER, { 0, 0, 0, 0 },
	MO_LEAVE, { 0, 0, 1, 1 },
};

static EVMULT_OUT ev_o;


long MAIN_FDSET_wr = 0L;
long MAIN_FDSET_rd = 0L;

PXY *MAIN_PointerPos = &ev_o.emo_mouse;
CARD32 MAIN_TimeStamp = 0;
CARD16 MAIN_KeyButMask = 0;

short _MAIN_Mctrl = 0;
short _MAIN_Wupdt = 0;

static short _MAIN_Xcons = 0;			/* pid of xconsole, if started by server */


/* ============================================================================== */
int main(int argc, char *argv[])
{
	int port = 6000;
	int rtn = 1;

	if (appl_init() < 0)
	{
		x_printf("ERROR: Can't initialize AES.\n");
	} else
	{
		short xcon;
		short redir = _app;

		xcon = Fopen("/dev/xconout2", 0x80);
		signal(SIGCHLD, sig_child);

		if (xcon >= 0)
		{
			const char *args[] = {
				"-g", "-0-0", "-fn", "6x10",
				"-exitOnFail", "-notify", "-file", "/dev/xconout2"
			};
			_MAIN_Xcons = WmgrLaunch(PATH_Xconsole, numberof(args), args);
			if (_MAIN_Xcons > 0)
			{
				set_printf(xTrue);
				redir = 0;
			} else
			{
				_MAIN_Xcons = 0;
			}
			Fclose(xcon);
		}
		if (redir)
		{
			redir = shel_write(1, 0, 0, (char *) PATH_DEBUG_OUT, (char *) "");
			if (redir)
				sleep(1);
		}
		atexit(shutdown);
		WmgrIntro(xTrue);

		if (redir)
		{
			short typ;
			short pid;
			char path[33] = "/pipe\0ttyp\0\0";
			char *file = strchr(path, '\0') + 1;
			char *ttyp = strchr(file, '\0');
			size_t len = ttyp - file;
			DIR *dir = opendir(path);

			if (dir)
			{
				struct dirent *ent;
				int fd;

				file[-1] = '/';
				while ((ent = readdir(dir)))
				{
					if (!strncmp(ent->d_name, file, len) && *ttyp < ent->d_name[len])
					{
						*ttyp = ent->d_name[len];
					}
				}
				closedir(dir);
				if (*ttyp && (fd = open(path, O_WRONLY)) > 0)
				{
					dup2(fd, STDOUT_FILENO);
				}
			}
			if (appl_search(-redir, path, &typ, &pid))
			{
				(void) Pkill(pid, 1);
				(void) Pkill(pid + 1, 1);
			}
		}

		x_printf("X Server %s [%s] starting ...\n", GLBL_Version, GLBL_Build);

		if (!_app || _AESnumapps != 1)
			menu_register(gl_apid, "  X");

		if (SrvrInit(port) >= 0)
		{
			CARD32 t_start = clock() * (1000 / CLOCKS_PER_SEC);
			BOOL run = xTrue;
			clock_t kb_tmout = 0x7FFFFFFF;

			WmgrActivate(xTrue);		/*(_app == 0); */

			if (argc > 1 && argv[1] && *argv[1])
			{
				int i;

				if (access(argv[1], X_OK) == 0)
				{
					WmgrLaunch(argv[1], argc - 2, (const char **) (argv + 2));
				}
				for (i = 2; i < argc; i++)
					x_printf("  |%s|\n", argv[i]);
			}

			rtn = 0;

			while (run)
			{
				short msg[8];
				BOOL reset = xFalse;
				short event = evnt_multi_fast(&ev_i, msg, &ev_o);
				CARD8 meta = (ev_o.emo_kmeta & (K_RSHIFT | K_LSHIFT | K_CAPSLOCK | K_CTRL | K_ALT))
					| (ev_o.emo_kmeta & K_ALTGR ? K_XALTGR : 0);
				short chng;

				MAIN_TimeStamp = (clock() * (1000 / CLOCKS_PER_SEC) - t_start);

				if (event & MU_KEYBD)
				{
					if (meta == (K_CTRL | K_ALT) && ev_o.emo_kreturn == 0x0E08)
					{
						if (_app)
						{
							fputs("\033p   X Server Shutdown forced   \033q\n", stderr);
							return 1;
						} else
						{
							x_printf("\033p   X Server Reset forced   \033q\n");
							reset = xTrue;
						}
					}
					chng = KybdEvent(ev_o.emo_kreturn, meta);
					if (*KYBD_Pending)
						kb_tmout = MAIN_TimeStamp + KYBD_Repeat;
				} else if (meta != KYBD_PrvMeta || MAIN_TimeStamp > kb_tmout)
				{
					chng = KybdEvent(0, meta);
					kb_tmout = 0x7FFFFFFF;
				} else
				{
					chng = 0;
				}
				if (WMGR_Cursor && (chng &= K_ALT | K_CTRL))
				{
					WmgrKeybd(chng);
				}

				if (event & MU_BUTTON)
				{
					CARD16 prev_mask = MAIN_KeyButMask & 0xFF00;

					MAIN_But_Mask = PntrMap(ev_o.emo_mbutton) >> 8;
					if (ev_o.emo_mbutton)
					{
						if (!prev_mask)
							WindMctrl(xTrue);
						ev_i.emi_bclicks = 0x0101;
					} else
					{
						WindMctrl(xFalse);
						ev_i.emi_bclicks = 0x0102;
					}
					if (WindButton(prev_mask, ev_o.emo_mclicks) && _MAIN_Mctrl)
					{
						graf_mkstate(&ev_o.emo_mouse.p_x, &ev_o.emo_mouse.p_y, &ev_o.emo_mbutton, &ev_o.emo_kmeta);
						if (!ev_o.emo_mbutton)
						{
							WindMctrl(xFalse);
						}
						MAIN_But_Mask = PntrMap(ev_o.emo_mbutton) >> 8;
						meta = (ev_o.emo_kmeta & (K_RSHIFT | K_LSHIFT | K_CAPSLOCK | K_CTRL | K_ALT))
							| (ev_o.emo_kmeta & K_ALTGR ? K_XALTGR : 0);
						if (meta != KYBD_PrvMeta && (chng = KybdEvent(0, meta) & (K_ALT | K_CTRL)) && WMGR_Cursor)
						{
							WmgrKeybd(chng);
						}
						ev_i.emi_bclicks = 0x0102;
					}
					ev_i.emi_bstate = ev_o.emo_mbutton;
				}
				ev_i.emi_m2.g_x = ev_o.emo_mouse.p_x;
				ev_i.emi_m2.g_y = ev_o.emo_mouse.p_y;

				if (event & MU_MESAG)
				{
					if (msg[0] == MN_SELECTED)
					{
						if (WmgrMenu(msg[3], msg[4], ev_o.emo_kmeta))
						{
							if (WMGR_ExitFlag)
							{
								run = xFalse;
								break;
							} else if (!WMGR_Active)
							{
								reset = xTrue;
							}
						}
					} else if (msg[0] == AP_TERM)
					{
						run = xFalse;
						break;
					} else
					{
						reset = WmgrMessage(msg);
					}
				}

				if (event & MU_M1)
					WindPointerWatch(xTrue);
				else if (event & MU_M2)
					WindPointerMove(NULL);

				if (reset || SrvrSelect(_MAIN_Xcons))
				{
					x_printf("\nLast client left, server reset ...\n");
					if (_MAIN_Mctrl)
					{
						puts("*BOING*");
						while (_MAIN_Mctrl > 0)
						{
							WindMctrl(xFalse);
						}
					}
					while (_MAIN_Wupdt > 0)
					{
						WindUpdate(xFalse);
					}
					SrvrReset();

				} else if (_MAIN_Xcons && WIND_ChngTrigger)
				{
					SrvrUngrab(NULL);
					set_printf(xFalse);
					_MAIN_Xcons = 0;
				}
			}
		}
	}
	return rtn;
}

/* ------------------------------------------------------------------------------ */
static void sig_child(int sig)
{
	long rusage;

	if (_MAIN_Xcons && Pkill(0, _MAIN_Xcons))
	{
		SrvrUngrab(NULL);
		set_printf(xFalse);
		_MAIN_Xcons = 0;
	}
	Pwaitpid(-1, 0, &rusage);
}

/* ------------------------------------------------------------------------------ */
static void shutdown(void)
{
	while (_MAIN_Wupdt > 0)
	{
		WindUpdate(xFalse);
	}
	while (_MAIN_Mctrl > 0)
	{
		WindMctrl(xFalse);
	}
	WmgrExit();
	appl_exit();
	fputs("\nbye ...\n", stdout);
}


/* ============================================================================== */
void MainSetMove(BOOL onNoff)
{
	if (onNoff)
		ev_i.emi_flags |= MU_M2;
	else
		ev_i.emi_flags &= ~MU_M2;
}


/* ============================================================================== */
void MainSetWatch(const GRECT *area, BOOL leaveNenter)
{
	if (area)
	{
		ev_i.emi_flags |= MU_M1;
		ev_i.emi_m1leave = leaveNenter;
		ev_i.emi_m1 = *area;
	} else
	{
		ev_i.emi_flags &= ~MU_M1;
	}
}
