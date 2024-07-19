#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include "includes/skeybindd.h"
#include "config.h"

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

void spawn_program(char *command[])
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
	if (libevdev_new_from_fd(fd, dev) < 0)
	{
		return -1;
	}
	if (libevdev_grab(*dev, LIBEVDEV_GRAB) < 0)
	{
		libevdev_free(*dev);
		return -1;
	}
	if (libevdev_uinput_create_from_device(*dev, LIBEVDEV_UINPUT_OPEN_MANAGED, udev) < 0)
	{
		libevdev_grab(*dev, LIBEVDEV_UNGRAB);
		libevdev_free(*dev);
		return -1;
	}

	return 0;
}

void sort(uint16_t *arr, int n)
{
	uint16_t key;
	int j;
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

// Returns -1 on error, 0 if no keybinding, 1 if keybinding
int handle_event(struct input_event *ev, uint16_t *keyState)
{
	uint64_t keylist = 0;

	switch (ev->value)
	{
	case 0: // Key released
		for (int i = 0; i < KEY_BUFFER; i++)
		{
			if (ev->code == keyState[i])
			{
				keyState[i] = 0;
			}
		}
		return 0;
	case 1: // Key pressed
		for (int i = 0; i <= KEY_BUFFER - 1; i++)
		{
			// Out of sync
			if (keyState[i] == ev->code)
			{
				memset(keyState, 0, sizeof(uint16_t) * KEY_BUFFER);
				return 1;
			}
			if (keyState[i] == 0)
			{
				keyState[i] = ev->code;
				break;
			}
		}

		// Formats keyState so its easier to compare
		sort(keyState, KEY_BUFFER);

	case 2: // Key held
		// Sees if the currently pressed keys match
		for (int i = 0; i < KEY_BUFFER; i++)
		{
			keylist = keylist << 16 | keyState[i];
		}

		for (int i = 0; i <= KEYBINDING_LEN; i++)
		{
			if (keylist == keybindings[i].binding.seed)
			{
				spawn_program((char **)keybindings[i].command);
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
	{"file", required_argument, NULL, 'f'},
	{0, 0, 0, 0}};

// Returns the keyboards fd
int handle_arguments(int argc, char *argv[])
{
	int fd;
	int opt = 0, optIndex = 0;
	while (opt != -1)
	{
		opt = getopt_long(argc, argv, "vdf:", long_options, &optIndex);
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
		case 'v':
			printf("skeybindd " VERSION "\n");
			exit(EXIT_SUCCESS);
			break;
		}
	}
	return fd;
}

int main(int argc, char *argv[])
{
	int keyboardfd = handle_arguments(argc, argv);
	struct libevdev *dev = NULL;
	struct libevdev_uinput *udev = NULL;

	if (create_udevice(&dev, &udev, keyboardfd) < 0)
	{
		perror("Could not create device");
		close(keyboardfd);
		return -1;
	}

	// Sorts keybinds in an expected format
	for (int i = 0; i < KEYBINDING_LEN; i++)
	{
		sort(keybindings[i].binding.keycodes, KEY_BUFFER);
		uint64_t tempSeed = 0;
		for (int j = 0; j < KEY_BUFFER; j++)
		{
			// This is done becuase it simplifies comparisons with the active keyboard state
			tempSeed = tempSeed << 16 | keybindings[i].binding.keycodes[j];
		}
		keybindings[i].binding.seed = tempSeed;
	}

	uint16_t keyState[KEY_BUFFER] = {0};
	struct input_event ev;
	while (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0)
	{
		// Event dropped
		if (ev.code == EV_SYN && ev.type == SYN_DROPPED)
		{
			libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
			fprintf(stderr, "SYN_DROPPED\n");
		}

		if (handle_event(&ev, keyState) == 1)
			continue;

		// MCS_SCAN is not dropped properly?
		libevdev_uinput_write_event(udev, ev.type, ev.code, ev.value);
	}

	return 0;
}