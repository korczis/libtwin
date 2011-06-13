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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdarg.h>
#include <byteswap.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/input.h>

#include "twin_fbdev.h"
#include "twinint.h"

#ifdef HAVE_ALTIVEC
#include <altivec.h>
#endif

#define _IMMEDIATE_REFRESH

/* We might want to have more error logging options */
#define SERROR(fmt...)	do { fprintf(stderr, fmt); \
			     fprintf(stderr, " : %s\n", strerror(errno)); \
			} while(0)
#define IERROR(fmt...)	fprintf(stderr, fmt)

#define DEBUG(fmt...)	printf(fmt)
//#define DEBUG(fmt...)

/* Only one instance can exist */
static twin_fbdev_t *twin_fb;
static int vt_switch_pending;

static void _twin_fbdev_put_span (twin_coord_t    left,
				  twin_coord_t    top,
				  twin_coord_t    right,
				  twin_argb32_t   *pixels,
				  void	  	  *closure)
{
	twin_fbdev_t    *tf = closure;
	twin_coord_t    width = right - left;
	unsigned int	*dest;

	if (!tf->active || tf->fb_base == MAP_FAILED)
		return;

	dest = (unsigned int *)(tf->fb_ptr + top * tf->fb_fix.line_length);
	dest += left;
	while(width--)
		*(dest++) = *(pixels++);
}

#ifdef HAVE_ALTIVEC
static void _twin_fbdev_vec_put_span (twin_coord_t    left,
				      twin_coord_t    top,
				      twin_coord_t    right,
				      twin_argb32_t   *pixels,
				      void     	      *closure)
{
	twin_fbdev_t    	*tf = closure;
	twin_coord_t    	width = right - left;
	unsigned int		*dest;
	vector unsigned char 	edgeperm;
	vector unsigned char	src0v, src1v, srcv;

	if (!tf->active || tf->fb_base == MAP_FAILED)
		return;

	dest = (unsigned int *)(tf->fb_ptr + top * tf->fb_fix.line_length);
	dest += left;

	while((((unsigned long)dest) & 0xf) && width--)
		*(dest++) = *(pixels++);

	edgeperm = vec_lvsl (0, pixels);
	src0v = vec_ld (0, pixels);
	while(width >= 4) {
		src1v = vec_ld (16, pixels);
		srcv = vec_perm (src0v, src1v, edgeperm);
		vec_st ((vector unsigned int)srcv, 0, dest);
		src0v = src1v;
		dest += 4;
		pixels += 4;
		width -= 4;
	}
	while(width--)
		*(dest++) = *(pixels++);
}
#endif /* HAVE_ALTIVEC */

static twin_bool_t twin_fbdev_apply_config(twin_fbdev_t *tf)
{
	off_t off, pgsize = getpagesize();
	struct fb_cmap cmap;
	size_t len;

	/* Tweak fields to default to 32 bpp argb and virtual == phys */
	tf->fb_var.xres_virtual = tf->fb_var.xres;
	tf->fb_var.yres_virtual = tf->fb_var.yres;
	tf->fb_var.bits_per_pixel = 32;
	tf->fb_var.red.length = 8;
	tf->fb_var.green.length = 8;
	tf->fb_var.blue.length = 8;
	tf->fb_var.transp.length = 8;
	tf->fb_var.red.offset = 0;
	tf->fb_var.green.offset = 0;
	tf->fb_var.blue.offset = 0;
	tf->fb_var.transp.offset = 0;

	/* Apply fbdev settings */
	if (ioctl(tf->fb_fd, FBIOPUT_VSCREENINFO, &tf->fb_var) < 0) {
		SERROR("can't set fb mode");
		return 0;
	}

	/* Get new fbdev configuration */
	if (ioctl(tf->fb_fd, FBIOGET_VSCREENINFO, tf->fb_var) < 0) {
		SERROR("can't get framebuffer config");
		return 0;
	}

	DEBUG("fbdev set config set to:\n");
	DEBUG(" xres          = %d\n", tf->fb_var.xres);
	DEBUG(" yres          = %d\n", tf->fb_var.yres);
	DEBUG(" xres_virtual  = %d\n", tf->fb_var.xres_virtual);
	DEBUG(" yres_virtual  = %d\n", tf->fb_var.yres_virtual);
	DEBUG(" bits_per_pix  = %d\n", tf->fb_var.bits_per_pixel);
	DEBUG(" red.len/off   = %d/%d\n",
	      tf->fb_var.red.length, tf->fb_var.red.offset);
	DEBUG(" green.len/off = %d/%d\n",
	      tf->fb_var.green.length, tf->fb_var.green.offset);
	DEBUG(" blue.len/off  = %d/%d\n",
	      tf->fb_var.blue.length, tf->fb_var.blue.offset);
	DEBUG(" trans.len/off = %d/%d\n",
	      tf->fb_var.transp.length, tf->fb_var.transp.offset);

	/* Check bits per pixel */
	if (tf->fb_var.bits_per_pixel != 32) {
		SERROR("can't set fb bpp to 32");
		return 0;
	}

	/* Set colormap */
	cmap.start = 0;
	cmap.len = 256;
	cmap.red = tf->cmap[0];
	cmap.green = tf->cmap[1];
	cmap.blue = tf->cmap[2];
	cmap.transp = NULL;
	ioctl(tf->fb_fd, FBIOPUTCMAP, &cmap);

	/* Get remaining settings */
	ioctl(tf->fb_fd, FBIOGET_FSCREENINFO, &tf->fb_fix);

	DEBUG(" line_lenght   = %d\n", tf->fb_fix.line_length);

	/* Map the fb */
	off = (off_t)tf->fb_fix.smem_start & (pgsize - 1);
	len = (size_t)tf->fb_fix.smem_len + off + (pgsize - 1);
	len &= ~(pgsize - 1);
	tf->fb_len = len;

	tf->fb_base = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,
			   tf->fb_fd, 0);
	if (tf->fb_base == MAP_FAILED) {
		SERROR("can't mmap framebuffer");
		return 0;
	}
	tf->fb_ptr = tf->fb_base + off;

	return 1;
}

static void twin_fbdev_switch(twin_fbdev_t *tf, int activate)
{
	tf->vt_active = activate;

	DEBUG("console switch: %sactivating\n", activate ? "" : "de");

	/* Upon activation */
	if (activate) {
		/* Switch complete */
		ioctl(tf->vt_fd, VT_RELDISP, VT_ACKACQ);		

		/* Restore fbdev settings */
		if (twin_fbdev_apply_config(tf)) {
			tf->active = 1;

			/* Mark entire screen for refresh */
			if (tf->screen)
				twin_screen_damage (tf->screen, 0, 0,
						    tf->screen->width,
						    tf->screen->height);
		}
	} else {
		/* Allow switch. Maybe we want to expose some option
		 * to disallow them ?
		 */
		ioctl(tf->vt_fd, VT_RELDISP, 1);

		tf->active = 0;

		if (tf->fb_base != MAP_FAILED)
			munmap(tf->fb_base, tf->fb_len);
		tf->fb_base = MAP_FAILED;
	}
}

static twin_bool_t twin_fbdev_work(void *closure)
{
	twin_fbdev_t *tf = closure;

	if (vt_switch_pending) {
		twin_fbdev_switch(tf, !tf->vt_active);
		vt_switch_pending = 0;
	}

#ifndef _IMMEDIATE_REFRESH
	if (tf->screen && tf->active &&
	    twin_screen_damaged (tf->screen))
		twin_screen_update(tf->screen);
#endif /* _IMMEDIATE_REFRESH */

	return TWIN_TRUE;
}

#ifdef _IMMEDIATE_REFRESH
static void twin_fbdev_damaged(void *closure)
{
	twin_fbdev_t *tf = closure;

#if 0
	DEBUG("fbdev damaged %d,%d,%d,%d, active=%d\n",
	      tf->screen->damage.left, tf->screen->damage.top,
	      tf->screen->damage.right, tf->screen->damage.bottom,
	      tf->active);
#endif

	if (tf->active && twin_screen_damaged (tf->screen))
		twin_screen_update(tf->screen);
}
#endif /* _IMMEDIATE_REFRESH */

static void twin_fbdev_vtswitch(int sig)
{
	signal(sig, twin_fbdev_vtswitch);
	vt_switch_pending = 1;
}

static twin_bool_t twin_fbdev_read_events(int file, twin_file_op_t ops,
					  void *closure)
{
	twin_fbdev_t *tf = closure;
	unsigned char events[16];
	twin_event_t evt;
	int i, count, down;

	count = read(file, events, 16);

	for (i = 0; i < count; i++) {
		unsigned char e = events[i];

		down = !(e & 0x80);
		e &= 0x7f;

		/* XXX Handle special keys (make more configurable) */
		switch(e) {
		case KEY_F1...KEY_F10:
			if (down)
				ioctl(tf->vt_fd, VT_ACTIVATE, e - KEY_F1 + 1);
			break;
		case KEY_ESC:
			if (down)
				kill(0, SIGINT);
			break;
		default:
			evt.kind = down ? TwinEventKeyDown : TwinEventKeyUp;
			evt.u.key.key = e;
			twin_screen_dispatch(tf->screen, &evt);
			break;
		}
	}

	return 1;
}

static twin_bool_t twin_fbdev_get_vt(twin_fbdev_t *tf, int wanted_vt)
{
	struct vt_stat vts;
	int ttyfd;
	char vtname[16];

	/* Open tty0 and use it to look for a free vt */
	ttyfd = open("/dev/tty0", O_WRONLY);
	if (ttyfd < 0) {
		SERROR("can't open /dev/tty0");
		ttyfd = open("/dev/vc/0", O_WRONLY);
		if (ttyfd < 0) {
			SERROR("can't open /dev/vc/0");
			return 0;
		}
	}
	/* Get previous VT */
	if (ioctl(ttyfd, VT_GETSTATE, &vts) < 0) {
		SERROR("can't get current VT");
		return 0;
	}
	tf->vt_prev = vts.v_active;

	DEBUG("previous vt: %d\n", tf->vt_prev);

	/* Sanity check wanted_vt and try to obtain a free VT. This is
	 * all somewhat racy but that's how everybody does it.
	 */
	if (wanted_vt > 31)
		wanted_vt = -1;
	if (wanted_vt > 0 && (vts.v_state & (1 << wanted_vt))) {
		IERROR("vt%d is busy\n", wanted_vt);
		wanted_vt = -1;
	}
	if (wanted_vt < 0)
		if (ioctl(ttyfd, VT_OPENQRY, &wanted_vt) < 0)
			wanted_vt = -1;
	if (wanted_vt < 0) {
		IERROR("can't find a free VT");
		return 0;
	}

	tf->vt_no = wanted_vt;

	DEBUG("new vt: %d\n", tf->vt_no);


	/* we don't need tty0 anymore, close it and open the target VT. */
	close(ttyfd);

	/* lose tty */
	setpgrp();
       	if ((ttyfd = open("/dev/tty", O_RDWR)) >= 0) {
		ioctl(ttyfd, TIOCNOTTY, 0);
		close(ttyfd);
	}

	sprintf(vtname, "/dev/tty%d", tf->vt_no);
	tf->vt_fd = open(vtname, O_RDWR | O_NONBLOCK);
	if (tf->vt_fd < 0) {
		sprintf(vtname, "/dev/vc/%d", tf->vt_no);
		tf->vt_fd = open(vtname, O_RDWR | O_NONBLOCK);
	}
	if (tf->vt_fd < 0) {
		SERROR("can't open tty %d", tf->vt_no);
		return 0;
	}

	/* set new controlling terminal */
	ioctl(tf->vt_fd, TIOCSCTTY, 1);

	/* set keyboard mode */
	ioctl(tf->vt_fd, KDSKBMODE, K_XLATE);

	tf->vt_active = tf->active = 0;
	
	return 1;
}

static twin_bool_t twin_fbdev_setup_vt(twin_fbdev_t *tf, int switch_sig)
{
	struct vt_mode vtm;
	struct termios tio;

	if (ioctl(tf->vt_fd, VT_GETMODE, &vtm) < 0) {
		SERROR("can't get VT mode");
		return 0;
	}
	vtm.mode = VT_PROCESS;
	vtm.relsig = switch_sig;
	vtm.acqsig = switch_sig;
	
	signal(switch_sig, twin_fbdev_vtswitch);
	tf->vt_swsig = switch_sig;

	if (ioctl(tf->vt_fd, VT_SETMODE, &vtm) < 0) {
		SERROR("can't set VT mode");
		signal(switch_sig, SIG_IGN);
		return 0;
	}

	tcgetattr(tf->vt_fd, &tf->old_tio);

	ioctl(tf->vt_fd, KDGKBMODE, &tf->old_kbmode);
	ioctl(tf->vt_fd, KDSKBMODE, K_MEDIUMRAW);

	tio = tf->old_tio;
	tio.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
	tio.c_oflag = 0;
	tio.c_cflag = CREAD | CS8;
	tio.c_lflag = 0;
	tio.c_cc[VTIME]=0;
	tio.c_cc[VMIN]=1;
	cfsetispeed(&tio, 9600);
	cfsetospeed(&tio, 9600);
	tcsetattr(tf->vt_fd, TCSANOW, &tio);

	ioctl(tf->vt_fd, KDSETMODE, KD_GRAPHICS);

	return 1;
}

static twin_bool_t twin_fbdev_init_fb(twin_fbdev_t *tf)
{
	int i;

	/* We always open /dev/fb0 for now. Might want fixing */
	tf->fb_fd = open("/dev/fb0", O_RDWR);
	if (tf->fb_fd < 0) {
		SERROR("can't open /dev/fb0");
		return 0;
	}

	/* Get initial fbdev configuration */
	if (ioctl(tf->fb_fd, FBIOGET_VSCREENINFO, &tf->fb_var) < 0) {
		SERROR("can't get framebuffer config");
		return 0;
	}

	DEBUG("initial screen size: %dx%d\n",
	      tf->fb_var.xres, tf->fb_var.yres);

	tf->fb_base = MAP_FAILED;

	for (i = 0; i < 256; i++) {
		unsigned short c = (i << 8) | i;
		tf->cmap[0][i] = tf->cmap[1][i] = tf->cmap[2][i] = c;
	}

	return 1;	
}

static twin_bool_t twin_fbdev_init_screen(twin_fbdev_t *tf)
{
	twin_put_span_t span;

	span = _twin_fbdev_put_span;
#ifdef HAVE_ALTIVEC
	if (twin_has_feature(TWIN_FEATURE_ALTIVEC))
		span = _twin_fbdev_vec_put_span;
#endif

	tf->screen = twin_screen_create(tf->fb_var.xres,
					tf->fb_var.yres,
					NULL, span, tf);
	if (tf->screen == NULL) {
		IERROR("can't create twin screen");
		return 0;
	}

	return 1;
}

static void twin_fbdev_cleanup_vt(twin_fbdev_t *tf)
{
	struct vt_mode vtm;

	ioctl(tf->vt_fd, VT_GETMODE, &vtm);
	vtm.mode = VT_AUTO;
	vtm.relsig = 0;
	vtm.acqsig = 0;
	ioctl(tf->vt_fd, VT_SETMODE, &vtm);

	signal(tf->vt_swsig, SIG_DFL);

	tcsetattr(tf->vt_fd, TCSANOW, &tf->old_tio);
	ioctl(tf->vt_fd, KDSKBMODE, tf->old_kbmode);

	ioctl(tf->vt_fd, KDSETMODE, KD_TEXT);
	ioctl(tf->vt_fd, VT_ACTIVATE, tf->vt_prev);
	ioctl(tf->vt_fd, VT_WAITACTIVE, tf->vt_prev);
}

twin_fbdev_t *twin_fbdev_create(int wanted_vt, int switch_sig)
{
	twin_fbdev_t *tf;

	if (twin_fb != NULL) {
		IERROR("twin_fbdev supports only one instance");
		return NULL;
	}

	tf = calloc(1, sizeof(twin_fbdev_t));
	if (tf == NULL)
		return NULL;

	if (!twin_fbdev_get_vt(tf, wanted_vt))
		goto err_free;
	
	if (!twin_fbdev_setup_vt(tf, switch_sig))
		goto err_release;

	if (!twin_fbdev_init_fb(tf))
		goto err_reset_vt;

	if (!twin_fbdev_init_screen(tf))
		goto err_close_fb;

	twin_set_work(twin_fbdev_work, TWIN_WORK_REDISPLAY, tf);

	twin_set_file(twin_fbdev_read_events, tf->vt_fd, TWIN_READ, tf);

#ifdef _IMMEDIATE_REFRESH
	twin_screen_register_damaged(tf->screen, twin_fbdev_damaged, tf);
#endif
	twin_fb = tf;
	return tf;

 err_close_fb:
	close(tf->fb_fd);
 err_reset_vt:
	twin_fbdev_cleanup_vt(tf);
	signal(switch_sig, SIG_DFL);
 err_release:
	close(tf->vt_fd);
 err_free:
	free(tf);
	return NULL;
}

void twin_fbdev_destroy(twin_fbdev_t *tf)
{
	tf->active = 0;
	twin_fbdev_cleanup_vt(tf);
	close(tf->fb_fd);
	close(tf->vt_fd);
	free(tf);
	twin_fb = NULL;
}

twin_bool_t twin_fbdev_activate(twin_fbdev_t *tf)
{
	/* If VT is not active, try to activate it. We don't deadlock
	 * here thanks to linux not waiting for VT_RELDISP on the target
	 * but we might want to be more careful
	 */
	if (!tf->vt_active) {
		if (ioctl(tf->vt_fd, VT_ACTIVATE, tf->vt_no) < 0)
			return 0;
		if (ioctl(tf->vt_fd, VT_WAITACTIVE, tf->vt_no) < 0)
			return 0;
	}

	/* Run work to process the VT switch */
	twin_fbdev_work(tf);

	/* If the screen is not active, then we failed
	 * the fbdev configuration
	 */
	return tf->active;
}

