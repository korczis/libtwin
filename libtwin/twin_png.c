/*
 * Libpng interface to twin
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

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <png.h>

#include "twin_png.h"

#if 0
#include <stdio.h>
#define DEBUG(fmt...)	printf(fmt)
#else
#define DEBUG(fmt...)
#endif

typedef struct  _twin_png_priv {
	int	fd;
} twin_png_priv_t;

static void twin_png_read_fn(png_structp png, png_bytep data, png_size_t size)
{
	twin_png_priv_t	*priv = png_get_io_ptr(png);
	int n;

	n = read(priv->fd, data, size);
	DEBUG(" twin_png_read_fn size=%d, got=%d\n", size, n);
	if (n < size)
		png_error(png, "end of file !\n");	
}

twin_pixmap_t *twin_png_to_pixmap(const char *filepath, twin_format_t fmt)
{
	uint8_t		signature[8];
	int		fd, i, rb = 0;
	size_t		n;
	png_structp	png = NULL;
	png_infop	info = NULL;
	twin_pixmap_t	*pix = NULL;
	twin_png_priv_t priv;
	png_uint_32	width, height;
	int		depth, ctype, interlace;
	png_bytep	*rowp = NULL;

	DEBUG("png read for %s, format=%d\n", filepath, fmt);

	fd = open(filepath, O_RDONLY);
	if (fd < 0)
		goto fail;

	DEBUG("checking signature...\n");

	n = read(fd, signature, 8);
	if (png_sig_cmp(signature, 0, n) != 0)
		goto fail_close;

	DEBUG("creating libpng structures...\n");

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
				     NULL, NULL, NULL);
	if (png == NULL)
		goto fail_close;

	info = png_create_info_struct(png);
	if (info == NULL)
		goto fail_free;
	    
	if (setjmp(png_jmpbuf(png))) {
		DEBUG("* error callback *\n");
		if (pix)
			twin_pixmap_destroy(pix);
		pix = NULL;
		goto fail_free;
	}
	priv.fd = fd;
	png_set_read_fn(png, &priv, twin_png_read_fn);

	png_set_sig_bytes(png, n);

	DEBUG("reading picture infos ...\n");
	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &depth, &ctype, &interlace,
		     int_p_NULL, int_p_NULL);
	
	DEBUG(" 1- size/depth/ctype/int = %ldx%ld/%d/%d/%d\n",
	      width, height, depth, ctype, interlace);

	if (depth == 16)
		png_set_strip_16(png);
	if (ctype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);
	if (ctype == PNG_COLOR_TYPE_GRAY && depth < 8)
		png_set_gray_1_2_4_to_8(png);
	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	png_get_IHDR(png, info, &width, &height, &depth, &ctype, &interlace,
		     int_p_NULL, int_p_NULL);

	DEBUG(" 2- size/depth/ctype/int = %ldx%ld/%d/%d/%d\n",
	      width, height, depth, ctype, interlace);

	switch(fmt) {
	case TWIN_A8:
		if (ctype != PNG_COLOR_TYPE_GRAY ||
		    depth != 8)
			goto fail_free;
		rb = width;
		break;
	case TWIN_RGB16:
		/* unsupported for now */
		goto fail_free;
	case TWIN_ARGB32:
		if (ctype == PNG_COLOR_TYPE_RGB)
			png_set_filler(png, 0xff, PNG_FILLER_BEFORE);
		if (ctype == PNG_COLOR_TYPE_RGB_ALPHA)
			png_set_swap_alpha(png);
		if (ctype == PNG_COLOR_TYPE_GRAY ||
		    ctype == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);

		png_get_IHDR(png, info, &width, &height, &depth, &ctype,
			     &interlace, int_p_NULL, int_p_NULL);

		DEBUG(" 3- size/depth/ctype/int = %ldx%ld/%d/%d/%d\n",
		      width, height, depth, ctype, interlace);

		if (depth != 8)
			goto fail_free;
		rb = width * 4;
		break;
	}
	DEBUG(" rowbytes = %d\n", rb);

	DEBUG("preparing pixmap & row pointer array...\n");

	rowp = malloc(height * sizeof(png_bytep));
	if (rowp == NULL)
		goto fail_free;
	pix = twin_pixmap_create(fmt, width, height);
	if (pix == NULL)
		goto fail_free;
	for (i = 0; i < height; i++)
		rowp[i] = pix->p.b + rb * i;

	DEBUG("reading image...\n");

	png_read_image(png, rowp);

	png_read_end(png, NULL);

	if (fmt == TWIN_ARGB32)
		twin_premultiply_alpha(pix);

 fail_free:
	if (rowp)
		free(rowp);
	png_destroy_read_struct(&png, &info, png_infopp_NULL);
 fail_close:
	close(fd);
 fail:
	return pix;
}

