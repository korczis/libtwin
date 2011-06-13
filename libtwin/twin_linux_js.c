/*
 * Linux joystick driver for Twin
 *
 * Copyright 2007 Jeremy Kerr <jk@ozlabs.org>
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

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include <linux/joystick.h>

#include "twin.h"
#include "twin_linux_js.h"

#define DEBUG(fmt...)	printf(fmt)

struct twin_js_dev {
	int fd;
	twin_screen_t *screen;
};

static twin_bool_t twin_linux_js_events(int file, twin_file_op_t ops,
					   void *closure)
{
	struct twin_js_dev *js = closure;
	struct js_event js_event;
	int rc;
	twin_event_t tev;

	for (;;) {

		rc = read(js->fd, &js_event, sizeof(js_event));
		if (rc < 0 && errno == EAGAIN)
			break;

		if (rc < 0) {
			DEBUG("Error reading from joystick device: %s\n",
					strerror(errno));
			return TWIN_FALSE;
		}

		if (rc != sizeof(js_event))
			break;

		if (js_event.type == JS_EVENT_BUTTON)
			tev.kind = TwinEventJoyButton;
		else if (js_event.type == JS_EVENT_AXIS)
			tev.kind = TwinEventJoyAxis;
		else
			continue;

		tev.u.js.control = js_event.number;
		tev.u.js.value = js_event.value;
		twin_screen_dispatch(js->screen, &tev);
	}

	return TWIN_TRUE;
}


static int nr_devs;
struct twin_js_dev *js_devs;

int twin_linux_js_create(twin_screen_t *screen)
{
	struct dirent *dirent;
	DIR *dir;

	dir = opendir("/dev/input");
	if (dir == NULL) {
		perror("opendir(/dev/input)");
		return -1;
	}

	while ((dirent = readdir(dir))) {
		/* buffer to hold dir.d_name, plus "/dev/input/" */
		char dev_name[sizeof(dirent->d_name) + 12];
		int dev_fd;
		struct twin_js_dev *js_dev, *tmp;


		if (strncmp(dirent->d_name, "js", 2))
			continue;

		strcpy(dev_name, "/dev/input/");
		strcat(dev_name, dirent->d_name);

		dev_fd = open(dev_name, O_RDONLY | O_NONBLOCK);
		if (dev_fd < 0) {
			perror("open");
			continue;
		}
		DEBUG("Adding joystick device %s\n", dev_name);

		tmp = realloc(js_devs, ++nr_devs * sizeof(*js_devs));
		if (!tmp) {
			close(dev_fd);
			continue;
		}
		js_devs = tmp;

		js_dev = js_devs + nr_devs - 1;
		js_dev->fd = dev_fd;
		js_dev->screen = screen;

		twin_set_file(twin_linux_js_events, dev_fd, TWIN_READ, js_dev);
	}
	closedir(dir);

	return 0;
}

void twin_linux_js_destroy()
{
	int i;

	for (i = 0; i < nr_devs; i++)
		close(js_devs[i].fd);

	free(js_devs);
	js_devs = NULL;
}

