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

#ifndef _TWIN_DEMOLINE_H_
#define _TWIN_DEMOLINE_H_

#include <twin.h>

typedef struct _twin_demoline {
    twin_widget_t   widget;
    twin_point_t    points[2];
    int		    which;
    twin_fixed_t    line_width;
    twin_cap_t	    cap_style;
} twin_demoline_t;

void
_twin_demoline_paint (twin_demoline_t *demoline);

twin_dispatch_result_t
_twin_demoline_dispatch (twin_widget_t *widget, twin_event_t *event);
    
void
_twin_demoline_init (twin_demoline_t		*demoline, 
		  twin_box_t		*parent,
		  twin_dispatch_proc_t	dispatch);

twin_demoline_t *
twin_demoline_create (twin_box_t *parent);

void
twin_demoline_start (twin_screen_t *screen, const char *name, int x, int y, int w, int h);

#endif /* _TWIN_DEMOLINE_H_ */
