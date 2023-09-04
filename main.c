#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
// #include <syslog.h> // Logging
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include "config.h"

#define STR_LENGTH(x) sizeof(x) / sizeof(*x)
#define LOG(str) fprintf(stdout, str)

#ifndef SYSTEMD
int daemonize(void)
{
	if (fork() != 0)
		exit(EXIT_SUCCESS);

	if (setsid() == -1)
		return -1;

	if (fork() != 0)
		exit(EXIT_SUCCESS);

	int maxfd, fd;
	maxfd = sysconf(_SC_OPEN_MAX);
	for (fd = 0; fd < maxfd; fd++)
		close(fd);

	close(STDIN_FILENO);
	fd = open("/dev/null", O_RDWR);
	if (fd != STDIN_FILENO)
		return -1;
	if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
		return -1;
	if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
		return -1;
	return 0;
}
#endif

void program_spawn(char *command[])
{
	if (fork() == 0)
	{
		if (fork() == 0)
		{
			setsid();
			execvp(command[0], command);
		}
		exit(EXIT_SUCCESS);
	}
	wait(NULL);
}

int create_udevice(struct libevdev **dev, struct libevdev_uinput **udev, char *path)
{
	int fd, rc;
	fd = open(path, O_RDWR, O_NOCTTY);
	if (fd < 0)
	{
		LOG(strerror(fd));
		return fd;
	}

	/* Sets up evdev */
	rc = libevdev_new_from_fd(fd, dev);
	if (rc < 0)
	{
		LOG(strerror(rc));
		close(fd);
		return rc;
	}

	/* Grabs device so it cannot pass stray inputs */
	rc = libevdev_grab(*dev, LIBEVDEV_GRAB);
	if (rc < 0)
	{
		LOG(strerror(rc));
		close(fd);
		libevdev_free(*dev);
		return rc;
	}

	/* uinput */
	rc = libevdev_uinput_create_from_device(*dev, LIBEVDEV_UINPUT_OPEN_MANAGED, udev);
	if (rc < 0)
	{
		LOG(strerror(rc));
		close(fd);
		libevdev_free(*dev);
		libevdev_grab(*dev, LIBEVDEV_UNGRAB);
		return rc;
	}

	return 0;
}

void sort(int *arr, int n)
{
	int key, j;
	for (int i = 1; i < n - 1; i++)
	{
		key = arr[i];
		j = i - 1;

		while (j >= 0 && key > arr[j])
		{
			arr[j + 1] = arr[j];
			j--;
		}
		arr[j + 1] = key;
	}
}

/* Looks to see if anything interesting is happening
   Returns -1 on error, 0 if no keybinding, 1 if keybinding*/
int handle_event(struct input_event *ev, int *pressedKeys)
{
	switch (ev->value)
	{
	case 0: // Key released
		for (int i = 0; i <= KEY_BUFFER - 1; i++)
		{
			if (ev->code == pressedKeys[i])
			{
				pressedKeys[i] = 0;
			}
		}
		return 0;
	case 1: // Key pressed
		for (int i = 0; i <= KEY_BUFFER - 1; i++)
		{
			if (pressedKeys[i] == 0)
			{
				pressedKeys[i] = ev->code;
				break;
			}
		}
		// Formats pressedKeys so its easier to compare
		sort(pressedKeys, KEY_BUFFER);

		// Sees if the currently pressed keys match
		for (int i = 0; i <= STR_LENGTH(keybindings); i++)
		{
			// Picks one keybind list from config.h
			struct keybinding *curKeybind = &keybindings[i];
			bool isMatch = true;

			for (int j = 0; j <= KEY_BUFFER - 1; j++)
			{
				if (pressedKeys[j] != curKeybind->keycodes[j])
				{
					isMatch = false;
					break;
				}
			}
			if (isMatch)
			{
				program_spawn((char **)curKeybind->command);
				return 1;
			}
		}
		return 0;
	case 2: // Key held
		return 0;
	default: // Not expected
		return -1;
	}
}

int main(int argc, char *argv[])
{
	int rc;

	/* Args */
	if (argc < 2)
	{
		printf("Did not specify device\n");
		exit(EXIT_FAILURE);
	}
	char *file = argv[1];

#ifndef SYSTEMD
	rc = daemonize();
	if (rc == -1)
		exit(rc);
#endif

	struct libevdev *dev = NULL;
	struct libevdev_uinput *udev = NULL;
	rc = create_udevice(&dev, &udev, file);
	if (rc < 0)
		exit(rc);

	/* Sorts keybinds in an expected format */
	for (int i = 0; i < STR_LENGTH(keybindings); i++)
	{
		sort(keybindings[i].keycodes, KEY_BUFFER);
	}

	// Will contain all active key events
	int pressedKeys[KEY_BUFFER] = {0};

	rc = 0;
	while (rc == 1 || rc == 0 || rc == -EAGAIN)
	{
		struct input_event ev;
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);

		// Filters out unused events
		if (rc != 0 || (ev.code == EV_SYN && ev.type == SYN_DROPPED))
			continue;

		if (handle_event(&ev, pressedKeys) == 1)
		{
			continue;
		}

		/* MCS_SCAN is not dropped properly */
		libevdev_uinput_write_event(udev, ev.type, ev.code, ev.value);
	}

	return 0;
}