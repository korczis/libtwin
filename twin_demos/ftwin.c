/*
 * Test shell for twin fbdev & linux mouse driver
 *
 * Copyright 2006 Benjamin Herrenschmidt <benh@kernel.crashing.org>
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Twin Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <syscall.h>
#include <twin_clock.h>
#include <twin_text.h>
#include <twin_demo.h>
#include <twin_hello.h>
#include <twin_calc.h>
#include <twin_demoline.h>
#include <twin_demospline.h>

#include "twin_fbdev.h"
#include "twin_linux_mouse.h"

twin_fbdev_t *tf;

static void exitfunc(void)
{
	if (tf)
		twin_fbdev_destroy(tf);
	tf = NULL;
}

static void sigint(int sig)
{
	exitfunc();
	syscall(__NR_exit);
}

int main (int argc, char **argv)
{
	int hx, hy;
	twin_pixmap_t *cur;

	twin_feature_init();

	tf = twin_fbdev_create(-1, SIGUSR1);
	if (tf == NULL)
		return 1;

	atexit(exitfunc);
	signal(SIGINT, sigint);

	twin_linux_mouse_create(NULL, tf->screen);

	cur = twin_load_X_cursor("ftwin.cursor", 0, &hx, &hy);
	if (cur == NULL)
		cur = twin_get_default_cursor(&hx, &hy);
	if (cur != NULL)
		twin_screen_set_cursor(tf->screen, cur, hx, hy);
	twin_screen_set_background (tf->screen, twin_make_pattern ());
#if 0
	twin_demo_start (tf->screen, "Demo", 100, 100, 400, 400);
#endif
#if 0
	twin_text_start (tf->screen,  "Gettysburg Address", 0, 0, 300, 300);
#endif
#if 0
	twin_hello_start (tf->screen, "Hello, World", 0, 0, 200, 200);
#endif
#if 1
	twin_clock_start (tf->screen, "Clock", 10, 10, 200, 200);
#endif
#if 1
	twin_calc_start (tf->screen, "Calculator", 100, 100, 200, 200);
#endif
#if 1
	twin_demoline_start (tf->screen, "Demo Line", 0, 0, 400, 400);
#endif
#if 1
	twin_demospline_start (tf->screen, "Demo Spline", 20, 20, 400, 400);
#endif

	twin_fbdev_activate(tf);

	twin_dispatch ();

	return 0;
}
