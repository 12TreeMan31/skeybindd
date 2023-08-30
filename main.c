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
		exit(EXIT_FAILURE);
#endif

	int fd;

	/* Sets up evdev */
	struct libevdev *dev = NULL;
	fd = open(file, O_RDWR, O_NOCTTY);
	rc = libevdev_new_from_fd(fd, &dev);
	if (rc == -1)
		exit(EXIT_FAILURE);

	/* Grabs device so it cannot pass stray inputs */
	rc = libevdev_grab(dev, LIBEVDEV_GRAB);
	if (rc == -1)
		exit(EXIT_FAILURE);

	/* uinput */
	struct libevdev_uinput *udev;
	rc = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &udev);
	if (rc == -1)
		exit(EXIT_FAILURE);

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
		if (rc != 0 || (ev.code == EV_SYN && ev.type == SYN_DROPPED) || ev.code == EV_MSC)
			continue;

		/* Handles the events */
		// Key pressed
		if (ev.value == 0)
		{
			// Remove key from
			for (int i = 0; i < KEY_BUFFER; i++)
			{
				if (pressedKeys[i] == ev.code)
				{
					pressedKeys[i] = 0;
				}
			}
			libevdev_uinput_write_event(udev, ev.type, ev.code, ev.value);
		}
		// Key released
		else if (ev.value == 1 || ev.value == 2)
		{
			if (ev.value == 1)
			{
				// Add key to the queue
				for (int i = 0; i < KEY_BUFFER; i++)
				{
					if (pressedKeys[i] == 0)
					{
						pressedKeys[i] = ev.code;
						break;
					}
				}
			}

			sort(pressedKeys, KEY_BUFFER);

			bool isMatch;
			for (int i = 0; i < STR_LENGTH(keybindings); i++)
			{
				struct keybinding *curKeybind = &keybindings[i];
				// int *curKeybind = keybindings[i].keycodes;

				isMatch = true;
				for (int j = 0; j < KEY_BUFFER - 1; j++)
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
					break;
				}
			}
			if (!isMatch)
			{
				libevdev_uinput_write_event(udev, ev.type, ev.code, ev.value);
				// libevdev_uinput_write_event(udev, EV_SYN, SYN_REPORT, 0);
			}
		}
	}

	return 0;
}