/*
 * libjpeg interface to twin
 *
 * Copyright 2007 Benjamin Herrenschmidt <benh@kernel.crashing.org>
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
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "twin_jpeg.h"

#if BITS_IN_JSAMPLE != 8
#error supports only libjpeg with 8 bits per sample
#endif

#if 0
#define DEBUG(fmt...)	printf(fmt)
#else
#define DEBUG(fmt...)
#endif

struct twin_jpeg_err_mgr {
	struct jpeg_error_mgr	mgr;
	jmp_buf			jbuf;
};

static void twin_jpeg_error_exit(j_common_ptr cinfo)
{
	struct twin_jpeg_err_mgr *jerr =
		(struct twin_jpeg_err_mgr *)cinfo->err;

	/* Do we want to display crap ? Maybe make that an option... */
	(*cinfo->err->output_message)(cinfo);

	longjmp(jerr->jbuf, 1);
}


twin_pixmap_t *twin_jpeg_to_pixmap(const char *filepath, twin_format_t fmt)
{
	struct jpeg_decompress_struct	cinfo;
	struct twin_jpeg_err_mgr	jerr;
	FILE				*infile;
	JSAMPARRAY			rowbuf;
	int				rowstride;
	twin_pixmap_t			*pix = NULL;
	twin_coord_t			width, height;

	/* Current implementation only produces TWIN_ARGB32
	 * and TWIN_A8
	 */
	if (fmt != TWIN_ARGB32 && fmt != TWIN_A8)
		return NULL;

	/* Open file first, as example */
	infile = fopen(filepath, "rb");
	if (infile == NULL) {
		fprintf(stderr, "can't open %s\n", filepath);
		return NULL;
	}

	/* Error handling crap */
	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.err = jpeg_std_error(&jerr.mgr);
	jerr.mgr.error_exit = twin_jpeg_error_exit;
	if (setjmp(jerr.jbuf)) {
		fprintf(stderr, "failure decoding %s\n", filepath);
		if (pix)
			twin_pixmap_destroy(pix);
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return NULL;
	}

	/* Init libjpeg, hook it up to file and read header */
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void)jpeg_read_header(&cinfo, TRUE);

	/* Settings */
	width = cinfo.image_width;
	height = cinfo.image_height;
	if (fmt == TWIN_ARGB32)
		cinfo.out_color_space = JCS_RGB;
	else
		cinfo.out_color_space = JCS_GRAYSCALE;
	DEBUG("cinfo.image_width        = %d\n", cinfo.image_width);
	DEBUG("cinfo.image_height       = %d\n", cinfo.image_height);

	/* Allocate pixmap */
	pix = twin_pixmap_create(fmt, width, height);
	if (pix == NULL)
		longjmp(jerr.jbuf, 1);

	/* Start decompression engine */
	(void)jpeg_start_decompress(&cinfo);

	DEBUG("cinfo.output_width       = %d\n", cinfo.output_width);
	DEBUG("cinfo.output_height      = %d\n", cinfo.output_height);
	DEBUG("cinfo.output_components  = %d\n", cinfo.output_components);
	DEBUG("cinfo.output_color_comp  = %d\n", cinfo.out_color_components);
	DEBUG("cinfo.rec_outbuf_height  = %d\n", cinfo.rec_outbuf_height);
	DEBUG("cinfo.out_color_space    = %d\n", cinfo.out_color_space);

	if ((fmt == TWIN_A8 && cinfo.output_components != 1) ||
	    (fmt == TWIN_ARGB32 && (cinfo.output_components != 3 &&
				    cinfo.output_components != 4)))
		longjmp(jerr.jbuf, 1);

	rowstride = cinfo.output_width * cinfo.output_components;
	DEBUG("rowstride                = %d\n", rowstride);

	rowbuf = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, rowstride, 1);
	
	/* Process rows */
	while (cinfo.output_scanline < cinfo.output_height) {
		twin_pointer_t  p =
			twin_pixmap_pointer(pix, 0, cinfo.output_scanline);
		(void)jpeg_read_scanlines(&cinfo, rowbuf, 1);
		if (fmt == TWIN_A8 || cinfo.output_components == 4)
			memcpy(p.a8, rowbuf, rowstride);
		else {
			JSAMPLE		*s = *rowbuf;
			unsigned int	i;

			for (i = 0; i < width; i++) {
				unsigned int r, g, b;
				r = *(s++);
				g = *(s++);
				b = *(s++);
				*(p.argb32++) = 0xff000000u |
					(r << 16) | (g << 8) | b;
			}
		}
	}

	/* Cleanup */
	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	return pix;
}


twin_bool_t twin_jpeg_query(const char		*filepath,
			    twin_coord_t	*out_width,
			    twin_coord_t	*out_height,
			    int			*out_components,
			    twin_jpeg_cspace_t	*out_colorspace)
{
	struct jpeg_decompress_struct	cinfo;
	struct twin_jpeg_err_mgr	jerr;
	FILE				*infile;

	/* Open file first, as example */
	infile = fopen(filepath, "rb");
	if (infile == NULL) {
		fprintf(stderr, "can't open %s\n", filepath);
		return TWIN_FALSE;
	}

	/* Error handling crap */
	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.err = jpeg_std_error(&jerr.mgr);
	jerr.mgr.error_exit = twin_jpeg_error_exit;
	if (setjmp(jerr.jbuf)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return TWIN_FALSE;
	}

	/* Init libjpeg, hook it up to file and read header */
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void)jpeg_read_header(&cinfo, TRUE);

	/* Gather infos */
	if (out_width)
		*out_width	= cinfo.image_width;
	if (out_height)
		*out_height	= cinfo.image_height;
	if (out_components)
		*out_components	= cinfo.num_components;
	if (out_colorspace)
		*out_colorspace	= (twin_jpeg_cspace_t)cinfo.jpeg_color_space;

	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	return TWIN_TRUE;
}
