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

#include "twin_hello.h"
#include <time.h>
#include <string.h>

static twin_time_t
_twin_hello_timeout (twin_time_t now, void *closure)
{
    twin_label_t    *labelb = closure;
    time_t	    secs = time (0);
    char	    *t = ctime(&secs);

    *strchr(t, '\n') = '\0';
    twin_label_set (labelb, t,
		    0xff008000,
		    twin_int_to_fixed (12),
		    TWIN_TEXT_OBLIQUE);
    return 1000;
}

void
twin_hello_start (twin_screen_t *screen, const char *name, int x, int y, int w, int h)
{
    twin_toplevel_t *top = twin_toplevel_create (screen,
						 TWIN_ARGB32,
						 TwinWindowApplication,
						 x, y, w, h, name);
    twin_label_t    *labela = twin_label_create (&top->box,
						name,
						0xff000080,
						twin_int_to_fixed (12),
						TWIN_TEXT_ROMAN);
    twin_widget_t   *widget = twin_widget_create (&top->box,
						  0xff800000,
						  1, 2, 0, 0);
    twin_label_t    *labelb = twin_label_create (&top->box,
						 name,
						 0xff008000,
						 twin_int_to_fixed (12),
						 TWIN_TEXT_OBLIQUE);
    twin_button_t   *button = twin_button_create (&top->box,
						  "Button",
						  0xff800000,
						  twin_int_to_fixed (18),
						  TWIN_TEXT_BOLD|TWIN_TEXT_OBLIQUE);
    twin_widget_set (&labela->widget, 0xc0c0c0c0);
    (void) widget;
    twin_widget_set (&labelb->widget, 0xc0c0c0c0);
    twin_widget_set (&button->label.widget, 0xc0808080);
    twin_toplevel_show (top);
    twin_set_timeout (_twin_hello_timeout, 1000, labelb);
}
