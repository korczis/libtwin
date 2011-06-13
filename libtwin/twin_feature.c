/*
 * Linux fbdev driver for Twin
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

#include <signal.h>
#include <setjmp.h>

#include "twinint.h"

static unsigned int _twin_features;

#ifdef HAVE_ALTIVEC

static jmp_buf _twin_feature_jmpbuf;

static void illegal_instruction(int sig)
{
        longjmp(_twin_feature_jmpbuf, 1);
}


static int _twin_have_altivec(void)
{
        volatile int altivec = 0;
        void (*handler)(int sig);

        handler = signal(SIGILL, illegal_instruction);
        if ( setjmp(_twin_feature_jmpbuf) == 0 ) {
                asm volatile ("mtspr 256, %0\n\t"
                              "vand %%v0, %%v0, %%v0"
                              :
                              : "r" (-1));
                altivec = 1;
        }
        signal(SIGILL, handler);

	return altivec;
}

#else
#define _twin_have_altivec()
#endif /* HAVE_ALTIVEC */

int twin_has_feature(unsigned int feature)
{
	return (_twin_features & feature) != 0;
}

void twin_feature_init(void)
{
	if (_twin_have_altivec())
		_twin_features |= TWIN_FEATURE_ALTIVEC;

	_twin_draw_set_features();
}
