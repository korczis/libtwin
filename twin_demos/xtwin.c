/*
 * Twin - A Tiny Window System
 * Copyright © 2004 Keith Packard <keithp@keithp.com>
 * All rights reserved.
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

#include "twin_x11.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <twin_clock.h>
#include <twin_text.h>
#include <twin_demo.h>
#include <twin_hello.h>
#include <twin_calc.h>
#include <twin_demoline.h>
#include <twin_demospline.h>

#define WIDTH	512
#define HEIGHT	512

int
main (int argc, char **argv)
{
    Display	    *dpy = XOpenDisplay (0);
    twin_x11_t	    *x11;

    twin_feature_init();

    x11 = twin_x11_create (dpy, WIDTH, HEIGHT);

    twin_screen_set_background (x11->screen, twin_make_pattern ());
#if 0
    twin_demo_start (x11->screen, "Demo", 100, 100, 400, 400);
#endif
#if 0
    twin_text_start (x11->screen,  "Gettysburg Address", 0, 0, 300, 300);
#endif
#if 0
    twin_hello_start (x11->screen, "Hello, World", 0, 0, 200, 200);
#endif
#if 1
    twin_clock_start (x11->screen, "Clock", 10, 10, 200, 200);
#endif
#if 1
    twin_calc_start (x11->screen, "Calculator", 100, 100, 200, 200);
#endif
#if 1
    twin_demoline_start (x11->screen, "Demo Line", 0, 0, 400, 400);
#endif
#if 1
    twin_demospline_start (x11->screen, "Demo Spline", 20, 20, 400, 400);
#endif
    twin_dispatch ();
    return 0;
}
