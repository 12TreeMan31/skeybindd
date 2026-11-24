#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "includes/skeybindd.h"

void sort(uint16_t *restrict arr, int n) {
    uint16_t key;
    int j;
    for (int i = 1; i < n; i++) {
        key = arr[i];
        j = i - 1;

        while (j >= 0 && key > arr[j]) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void spawn_program(char *command[]) {
    /*pid_t fork_pid = fork();
    if (fork_pid == 0)
    {
        setsid();
        execvp(command[0], command);
    }
    wait(NULL);*/
    if (fork() == 0) {
        if (fork() == 0) {
            setsid();
            execvp(command[0], command);
        }
        exit(EXIT_SUCCESS);
    }
    wait(NULL);
}

int create_udevice(struct libevdev **dev, struct libevdev_uinput **udev, int fd) {
    if (libevdev_new_from_fd(fd, dev) < 0) { return -1; }
    if (libevdev_grab(*dev, LIBEVDEV_GRAB) < 0) {
        libevdev_free(*dev);
        return -1;
    }
    if (libevdev_uinput_create_from_device(*dev, LIBEVDEV_UINPUT_OPEN_MANAGED, udev) < 0) {
        libevdev_grab(*dev, LIBEVDEV_UNGRAB);
        libevdev_free(*dev);
        return -1;
    }

    return 0;
}

// Returns -1 on error, 0 if no keybinding, 1 if keybinding
int handle_event(struct input_event *ev, uint16_t *keyState) {
    switch (ev->value) {
    case 0: // Key released
        for (int i = 0; i < KEY_BUFFER; i++) {
            if (ev->code == keyState[i]) { keyState[i] = 0; }
        }
        return 0;
    case 1: // Key pressed
        for (int i = 0; i < KEY_BUFFER; i++) {
            // Out of sync
            if (keyState[i] == ev->code) {
                // Do nothing for now...
            }
            if (keyState[i] == 0) {
                keyState[i] = ev->code;
                break;
            }
        }

        // Formats keyState so its easier to compare
        sort(keyState, KEY_BUFFER);

        // Fall through
    case 2: // Key held
        for (int i = 0; i < KEYBINDING_LEN; i++) {
            int n = memcmp(keyState, keybindings[i].keycodes, KEY_BUFFER);
            if (n == 0) {
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
    {"version", no_argument, NULL, 'v'}, {"file", required_argument, NULL, 'f'}, {0, 0, 0, 0}};

// Returns the keyboards fd
int handle_arguments(int argc, char *argv[]) {
    int fd = -1, opt = 0, optIndex = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "vf:", long_options, &optIndex);
        switch (opt) {
        case 'f':
            fd = open(optarg, O_RDONLY | O_NOCTTY);
            if (fd == -1) {
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

// https://stackoverflow.com/questions/41995349/why-does-ioctlfd-eviocgrab-1-cause-key-spam-sometimes
void waitReleaseAll(int fd) {

    struct input_event evt;
    unsigned char key_b[KEY_MAX / 8 + 1];
    int i, nothing;

    while (1) {
        memset(key_b, 0, sizeof(key_b));
        ioctl(fd, EVIOCGKEY(sizeof(key_b)), key_b);
        for (nothing = 1, i = 0; i < KEY_MAX / 8 + 1; i++) {
            if (key_b[i] != 0) {
                nothing = 0;
                break;
            }
        }
        if (nothing) break;
        read(fd, &evt, sizeof(evt));
    }
    printf("All keys are now released\n");
}

int main(int argc, char *argv[]) {
    int keyboardfd = handle_arguments(argc, argv);
    struct libevdev *dev = NULL;
    struct libevdev_uinput *udev = NULL;

    waitReleaseAll(keyboardfd);

    if (create_udevice(&dev, &udev, keyboardfd) < 0) {
        perror("Could not create device");
        close(keyboardfd);
        return -1;
    }

    // Sorts keybinds in an expected format
    for (int i = 0; i < KEYBINDING_LEN; i++) {
        sort(keybindings[i].keycodes, KEY_BUFFER);
    }

    struct input_event ev = {0};
    uint16_t keyState[KEY_BUFFER] = {0};
    int read_flag = LIBEVDEV_READ_FLAG_NORMAL;
    while (true) {
        int rc = libevdev_next_event(dev, read_flag, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SYNC && read_flag == LIBEVDEV_READ_FLAG_NORMAL) {
            // There are dropped events
            read_flag = LIBEVDEV_READ_FLAG_SYNC;
            continue;
        } else if (rc == -EAGAIN && read_flag == LIBEVDEV_READ_FLAG_SYNC) {
            // We have gotten all of the dropped events
            read_flag = LIBEVDEV_READ_FLAG_NORMAL;
            continue;
        }

        if (rc == -EBADF) {
            // FD was closed
        } else if (rc < 0) {
            perror("Error: ");
        }

        // We do not want other events leaking since we dont handle them (ex EV_SYN)
        if (ev.type != EV_KEY) { continue; }
        if (handle_event(&ev, keyState)) { continue; };

        // Writes the new event frame for unhandled events
        libevdev_uinput_write_event(udev, ev.type, ev.code, ev.value);
        libevdev_uinput_write_event(udev, EV_SYN, SYN_REPORT, 0);
    }

    ERR_LOG("%i: %s\n", errno, strerror(errno));
    return 1;
}