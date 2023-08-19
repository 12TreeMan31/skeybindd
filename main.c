#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
// #include <getopt.h> // Command line args
#include <unistd.h>
// #include <syslog.h> // Logging
#include <poll.h>

#include <sys/stat.h>

#include <libevdev/libevdev.h>
#include <libseat.h>

int daemonize()
{
	switch (fork())
	{
	case -1:
		return -1;
	case 0:
		break;
	default:
		exit(EXIT_SUCCESS);
	}

	if (setsid() == -1)
		return -1;

	switch (fork())
	{
	case -1:
		return -1;
	case 0:
		break;
	default:
		exit(EXIT_SUCCESS);
	}

	umask(0);
	chdir("/");

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

static void handle_enable(struct libseat *backend, void *data)
{
	(void)backend;
	int *active = (int *)data;
	(*active)++;
}

static void handle_disable(struct libseat *backend, void *data)
{
	(void)backend;
	int *active = (int *)data;
	(*active)--;

	libseat_disable_seat(backend);
}

/*void sort(int **arr, int n)
{
	int key, j;
	for (int i = 1; i < n - 1; i++)
	{
		key = arr[i];
		j = i - 1;

		while (j >= 0 && key < arr[j])
		{
			arr[j + 1] = arr[j];
			j--;
		}
		arr[j + 1] = key;
	}
}*/

// Just in case
/* static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"device", required_argument, NULL, 'd'},
	{0, 0, 0, 0}}; */

int main(int argc, char *argv[])
{
	/* Args */
	if (argc < 2)
	{
		printf("Did not specify device\n");
		exit(EXIT_FAILURE);
	}
	char *file = argv[1];

	if (daemonize() == -1)
	{
		exit(EXIT_FAILURE);
	}

	/* Creates the seat */
	libseat_set_log_level(LIBSEAT_LOG_LEVEL_DEBUG);

	int active = 0;
	struct libseat_seat_listener listener = {
		.enable_seat = handle_enable,
		.disable_seat = handle_disable,
	};

	struct libseat *backend = libseat_open_seat(&listener, &active);
	if (backend == NULL)
		exit(EXIT_FAILURE);

	// Wait untill the seat has started
	while (active == 0)
	{
		if (libseat_dispatch(backend, -1) == -1)
		{
			libseat_close_seat(backend);
			exit(EXIT_FAILURE);
		}
	}

	int fd, device;
	device = libseat_open_device(backend, file, &fd);
	if (device == -1)
	{
		libseat_close_seat(backend);
		exit(EXIT_FAILURE);
	}

	/* Starts getting events with evdev */
	struct libevdev *dev = NULL;
	int rc;
	/*fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		exit(EXIT_FAILURE);*/
	rc = libevdev_new_from_fd(fd, &dev);
	if (rc == -1)
		exit(EXIT_FAILURE);

	struct pollfd fds[1] = {
		{
			.events = POLLIN,
			.fd = fd,
		},
	};

	// Have this be determaned when reading the config file

	// unsigned short pressedKeys[8];
	// memset(pressedKeys, -1, 8);

	/* TODO: Make this set through the config file */
	int pressedKeys[8] = {0};

	int volumeUp[8] =
		{
			0,
			0,
			0,
			0,
			0,
			0,
			KEY_U,
			KEY_LEFTALT,
		};

	do
	{
		// Used by libevdev to store the event info
		struct input_event ev;

		// Poll for new events then get the event data
		rc = poll(fds, 1, -1);
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

		// Handles events (Ignores all EV_SYN events but we might want to check for SYN_DROPPED)
		if (rc == 0 && ev.code != EV_SYN && ev.code != EV_MSC)
		{
			/*printf("Event: %s %s %d\n",
				   libevdev_event_type_get_name(ev.type),
				   libevdev_event_code_get_name(ev.type, ev.code),
				   ev.value);*/

			// Key pressed
			if (ev.value == 0)
			{
				// Remove key from
				for (int i = 0; i < 8; i++)
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
					for (int i = 0; i < 8; i++)
					{
						if (pressedKeys[i] == 0)
						{
							pressedKeys[i] = ev.code;
							break;
						}
					}
				}

				// Sort
				int key, j;
				for (int i = 1; i < 8; i++)
				{
					key = pressedKeys[i];
					j = i - 1;

					while (j >= 0 && key < pressedKeys[j])
					{
						pressedKeys[j + 1] = pressedKeys[j];
						j--;
					}
					pressedKeys[j + 1] = key;
				}

				if (pressedKeys[7] == volumeUp[7] &&
					pressedKeys[6] == volumeUp[6])
				{
					system("pactl set-sink-volume @DEFAULT_SINK@ +10%");
				}
			}
		}
	} while (rc == 1 || rc == 0 || rc == -EAGAIN);

	/*
	//Might not even use and just have user point to the device they want
	int fd;
	char buffer[200000], bufferT[100000];

	fd = open("/proc/bus/input/devices", O_RDONLY, O_NONBLOCK);
	if (read(fd, buffer, 100000) == -1)
	{
		perror("read");
	}
	read(fd, bufferT, 100000);
	strcat(buffer, bufferT);

	puts(buffer);

	char *pEvent, *pHandlers, *pEV = buffer;
	// Reads /proc/
	do
	{
		pHandlers = strstr(pEV, "Handlers=");
		if (pHandlers == NULL)
		{
			break;
		}
		pEV = strstr(pHandlers, "EV=");
		// Fix this because keyboard can be higher than 120013 (https://unix.stackexchange.com/questions/74903/explain-ev-in-proc-bus-input-devices-data)

		if (strncmp(pEV + 3, "120013", 6) == 0)
		{
			pEvent = strstr(pHandlers, "event");
			printf("Keyboard found! %.7s\n", pEvent);
		}
	} while (1);*/

	// Start polling? for events on keyboards

	// Prossess input -> Run some command specified by user
	return 0;
}