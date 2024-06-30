#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include "config.h"

#define VERSION "1.0"

/*
	TODO
	- Properly handle dropping key events
	- Starting up without keeping the last regestered event
	- Stop program from crashing when device is unplugged
*/

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

int create_udevice(struct libevdev **dev, struct libevdev_uinput **udev, int fd)
{
	int rc;

	rc = libevdev_new_from_fd(fd, dev);
	if (rc < 0)
	{
		return rc;
	}

	// The reason why we grab the device is so that another application doesn't also try and regester key events
	rc = libevdev_grab(*dev, LIBEVDEV_GRAB);
	if (rc < 0)
	{
		libevdev_free(*dev);
		return rc;
	}

	rc = libevdev_uinput_create_from_device(*dev, LIBEVDEV_UINPUT_OPEN_MANAGED, udev);
	if (rc < 0)
	{
		libevdev_grab(*dev, LIBEVDEV_UNGRAB);
		libevdev_free(*dev);
		return rc;
	}

	return 0;
}

void sort(int *arr, int n)
{
	int key, j;
	for (int i = 1; i < n; i++)
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
		for (int i = 0; i < KEY_BUFFER; i++)
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
			if (pressedKeys[i] == ev->code)
			{
				return 1;
			}
			if (pressedKeys[i] == 0)
			{
				pressedKeys[i] = ev->code;
				break;
			}
		}

		// Formats pressedKeys so its easier to compare
		sort(pressedKeys, KEY_BUFFER);

	case 2: // Key held
		// Sees if the currently pressed keys match
		for (int i = 0; i <= KEYBINDING_LEN; i++)
		{
			// Picks one keybind list from config.h
			struct keybinding *curKeybind = &keybindings[i];
			bool isMatch = true;

			for (int j = 0; j < KEY_BUFFER; j++)
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
	default: // Not expected
		return -1;
	}
}

static const struct option long_options[] = {
	{"version", no_argument, NULL, 'v'},
	{"daemon", no_argument, NULL, 'd'},
	{"log", no_argument, NULL, 'l'},
	{"file", required_argument, NULL, 'f'},
	{0, 0, 0, 0}};

// Should return the fd of the keyboard
int handle_arguments(int argc, char *argv[])
{
	int fd;
	int opt = 0, optIndex = 0;
	while (opt != -1)
	{
		opt = getopt_long(argc, argv, "vdlf:", long_options, &optIndex);
		switch (opt)
		{
		case 'd':
			if (daemonize() == -1)
				exit(EXIT_FAILURE);
			break;
		case 'f':
			fd = open(optarg, O_RDONLY | O_NOCTTY);
			if (fd == -1)
			{
				perror("Could not open device");
				exit(EXIT_FAILURE);
			}
			break;
		case 'l':
			break;
		case 'v':
			printf("skeybindd " VERSION " 2024-06-29\n");
			exit(EXIT_SUCCESS);
			break;
		}
	}
	return fd;
}

int main(int argc, char *argv[])
{
	int fd = handle_arguments(argc, argv);
	struct libevdev *dev = NULL;
	struct libevdev_uinput *udev = NULL;

	if (create_udevice(&dev, &udev, fd) < 0)
	{
		perror("Could not create device");
		close(fd);
		return -1;
	}

	/* Sorts keybinds in an expected format */
	for (int i = 0; i < KEYBINDING_LEN; i++)
	{
		sort(keybindings[i].keycodes, KEY_BUFFER);
	}

	// Will contain all active key events
	int pressedKeys[KEY_BUFFER] = {0};

	struct input_event ev;

	// For now when grabbing device it might hold first key pressed
	libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

	while (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0)
	{
		// Event dropped
		if (ev.code == EV_SYN && ev.type == SYN_DROPPED)
		{
			libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
			fprintf(stderr, "SYN_DROPPED\n");
		}

		if (handle_event(&ev, pressedKeys) == 1)
			continue;

		/* MCS_SCAN is not dropped properly */
		libevdev_uinput_write_event(udev, ev.type, ev.code, ev.value);
	}

	return 0;
}