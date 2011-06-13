/*
 * Manipulating twin cursor images
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

#include <twin_def.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <byteswap.h>
#include <endian.h>

#ifdef HAVE_ZLIB
#include <zlib.h>

#define fdtype gzFile
#define cursor_open(file) gzopen(file, "rb")
#define cursor_read gzread
#define cursor_seek gzseek
#define cursor_close gzclose

#else
#define fdtype int
#define cursor_open(file) open(file, O_RDONLY)
#define cursor_read read
#define cursor_seek lseek
#define cursor_close close
#endif

#include "twin.h"

/* Make something better here ! */

static unsigned int twin_def_cursor_image[] = {
	0x00000000, 0x88ffffff, 0x88ffffff, 0x00000000,
	0x88ffffff, 0xff000000, 0xff000000, 0x88ffffff,
	0x88ffffff, 0xff000000, 0xff000000, 0x88ffffff,
	0x00000000, 0x88ffffff, 0x88ffffff, 0x00000000,
};

twin_pixmap_t *twin_get_default_cursor(int *hx, int *hy)
{
    twin_pixmap_t *cur;
    twin_pointer_t data;

    data.argb32 = twin_def_cursor_image;
    cur = twin_pixmap_create_const(TWIN_ARGB32, 4, 4, 16, data);
    if (cur == NULL)
	return NULL;
    *hx = *hy = 2;
    return cur;
}

/*
 * Bits extracted from Xcursor
 */

#define XCURSOR_MAGIC   0x72756358  /* "Xcur" LSBFirst */

#define XCURSOR_FILE_MAJOR      1
#define XCURSOR_FILE_MINOR      0
#define XCURSOR_FILE_VERSION    ((XCURSOR_FILE_MAJOR << 16) | (XCURSOR_FILE_MINOR))
#define XCURSOR_FILE_HEADER_LEN (4 * 4)
#define XCURSOR_FILE_TOC_LEN    (3 * 4)

/*
 * Cursor files start with a header.  The header
 * contains a magic number, a version number and a
 * table of contents which has type and offset information
 * for the remaining tables in the file.
 *
 * File minor versions increment for compatible changes
 * File major versions increment for incompatible changes (never, we hope)
 *
 * Chunks of the same type are always upward compatible.  Incompatible
 * changes are made with new chunk types; the old data can remain under
 * the old type.  Upward compatible changes can add header data as the
 * header lengths are specified in the file.
 *
 *  File:
 *      FileHeader
 *      LISTofChunk
 *
 *  FileHeader:
 *      0  CARD32          magic       magic number
 *      1  CARD32          header      bytes in file header
 *      2  CARD32          version     file version
 *      3  CARD32          ntoc        number of toc entries
 *         LISTofFileToc   toc         table of contents
 *
 *  FileToc:
 *      0  CARD32          type        entry type
 *      1  CARD32          subtype     entry subtype (size for images)
 *      2  CARD32          position    absolute file position
 */


/*
 * The rest of the file is a list of chunks, each tagged by type
 * and version.
 *
 *  Chunk:
 *      ChunkHeader
 *      <extra type-specific header fields>
 *      <type-specific data>
 *
 *  ChunkHeader:
 *      0  CARD32      header      bytes in chunk header + type header
 *      1  CARD32      type        chunk type
 *      2  CARD32      subtype     chunk subtype
 *      3  CARD32      version     chunk type version
 */

#define XCURSOR_CHUNK_HEADER_LEN    (4 * 4)

/*
 * Each cursor image occupies a separate image chunk.
 * The length of the image header follows the chunk header
 * so that future versions can extend the header without
 * breaking older applications
 *
 *  Image:
 *      0  ChunkHeader     header  chunk header
 *      4  CARD32          width   actual width
 *      5  CARD32          height  actual height
 *      6  CARD32          xhot    hot spot x
 *      7  CARD32          yhot    hot spot y
 *      8  CARD32          delay   animation delay
 *         LISTofCARD32    pixels  ARGB pixels
 */

#define XCURSOR_IMAGE_TYPE          0xfffd0002
#define XCURSOR_IMAGE_VERSION       1
#define XCURSOR_IMAGE_HEADER_LEN    (XCURSOR_CHUNK_HEADER_LEN + (5*4))
#define XCURSOR_IMAGE_MAX_SIZE      0x7fff      /* 32767x32767 max cursor size */

static inline int twin_read_header(fdtype fd, uint32_t *buf, int size)
{
	int i, len;

	len = cursor_read(fd, buf, size);
	if (len != size)
		return 0;

#if __BYTE_ORDER == __BIG_ENDIAN
	for (i = 0; i < (len / 4); i++)
		buf[i] = bswap_32(buf[i]);
#endif
	return 1;
}

twin_pixmap_t *twin_load_X_cursor(const char *file, int index,
				  int *hx, int *hy)
{
	fdtype		fd;
	int		img, i, toccnt;
	uint32_t	buffer[32], filepos, size;
	twin_pixmap_t	*cur = NULL;	

	fd = cursor_open(file);
	if (fd < 0)
		return NULL;
	if (!twin_read_header(fd, buffer, XCURSOR_FILE_HEADER_LEN))
		goto bail;

	/* check magic */
	if (buffer[0] != XCURSOR_MAGIC)
		goto bail;

	/* check version. assume we support all minor versions */
	if ((buffer[2] >> 16) != XCURSOR_FILE_MAJOR)
		goto bail;

	/* get number of TOC entries */
	toccnt = buffer[3];

	/* seek to first toc entry (header len) */
	cursor_seek(fd, buffer[1] , SEEK_SET);

	/* look for the index'th image in TOC */
	img = 0;
	filepos = 0;
	for (i = 0; filepos == 0 && i < toccnt; i++) {
		if (!twin_read_header(fd, buffer, XCURSOR_FILE_TOC_LEN))
			goto bail;
		if (buffer[0] == XCURSOR_IMAGE_TYPE) {
			if (img == index)
				filepos = buffer[2];
			img++;
		}
	}
	/* check if found */
	if (filepos == 0)
		goto bail;

	/* seek to image header and read it */
	cursor_seek(fd, filepos, SEEK_SET);
	if (!twin_read_header(fd, buffer, XCURSOR_IMAGE_HEADER_LEN))
		goto bail;

	/* check chunk type */
	if (buffer[1] != XCURSOR_IMAGE_TYPE)
		goto bail;

	/* check image version */
	if (buffer[3] != XCURSOR_IMAGE_VERSION)
		goto bail;

	/* get hotspot */
	*hx = buffer[6];
	*hy = buffer[7];

	/* create pixmap */
	cur = twin_pixmap_create(TWIN_ARGB32, buffer[4], buffer[5]);
	if (cur == NULL)
		goto bail;

	/* load pixels */
	size = buffer[4] * buffer[5] * 4;
	cursor_seek(fd, filepos + buffer[0], SEEK_SET);
	if (cursor_read(fd, cur->p.v, size) != size) {
		twin_pixmap_destroy(cur);
		goto bail;
	}

#if __BYTE_ORDER == __BIG_ENDIAN
	for (i = 0; i < (size / 4); i++)
		cur->p.argb32[i] = bswap_32(cur->p.argb32[i]);
#endif

 bail:
	cursor_close(fd);
	return cur;
}

