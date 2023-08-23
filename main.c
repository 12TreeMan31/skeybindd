#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
// #include <syslog.h> // Logging
#include <libevdev/libevdev.h>
// #include <libevdev/libevdev-uinput.h>

#include "config.h"

#define STR_LENGTH(x) sizeof(x) / sizeof(x[0])

#ifndef SYSTEMD_DAEMON
int daemonize()
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

int program_spawn(char *command[])
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

int main(int argc, char *argv[])
{
	int rc;

	// Args
	if (argc < 2)
	{
		printf("Did not specify device\n");
		exit(EXIT_FAILURE);
	}
	char *file = argv[1];

#ifndef SYSTEMD_DAEMON
	rc = daemonize();
	if (rc == -1)
		exit(EXIT_FAILURE);
#endif

	/* Starts getting events with evdev */

	struct libevdev *dev = NULL;
	int fd;
	fd = open(file, O_RDWR, O_NOCTTY);
	rc = libevdev_new_from_fd(fd, &dev);
	if (rc == -1)
		exit(EXIT_FAILURE);

	struct pollfd fds[1] = {
		{
			.events = POLLIN,
			.fd = fd,
		},
	};

	// Sorts the keys in a way thats good for sorting
	for (int i = 0; i < STR_LENGTH(keybindings); i++)
	{
		sort(keybindings[i].keycodes, KEY_BUFFER);
	}

	// Whats currently being pressed
	int pressedKeys[KEY_BUFFER] = {0};

	rc = 1;
	while (rc == 1 || rc == 0 || rc == -EAGAIN)
	{
		// Used by libevdev to store the event info
		struct input_event ev;

		// Poll for new events then get the event data
		rc = poll(fds, 1, -1);
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

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

			// Compairs to two arrays
			bool isMatch;
			for (int i = 0; i < STR_LENGTH(keybindings); i++)
			{
				int *tempArr = keybindings[i].keycodes;

				isMatch = false;
				for (int j = 0; j < KEY_BUFFER - 1; j++)
				{
					if (pressedKeys[j] == tempArr[j])
					{
						isMatch = true;
					}
					else
					{
						isMatch = false;
						break;
					}
				}
				if (isMatch)
				{
					program_spawn((char **)keybindings[i].command);
					break;
				}
			}
		}
	}

	return 0;
}