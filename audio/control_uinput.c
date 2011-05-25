#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <bluetooth/bluetooth.h>

#include <glib.h>

#include "log.h"
#include "error.h"
#include "adapter.h"
#include "../src/device.h"
#include "device.h"
#include "control.h"
#include "uinput.h"
#include "control_uinput.h"

struct uinput_priv {
	int fd;
};

static struct {
	const char *name;
	uint8_t avrcp;
	uint16_t uinput;
} key_map[] = {
	{ "PLAY",		PLAY_OP,		KEY_PLAYCD },
	{ "STOP",		STOP_OP,		KEY_STOPCD },
	{ "PAUSE",		PAUSE_OP,		KEY_PAUSECD },
	{ "FORWARD",		FORWARD_OP,		KEY_NEXTSONG },
	{ "BACKWARD",		BACKWARD_OP,		KEY_PREVIOUSSONG },
	{ "REWIND",		REWIND_OP,		KEY_REWIND },
	{ "FAST FORWARD",	FAST_FORWARD_OP,	KEY_FASTFORWARD },
	{ NULL }
};

static int send_event(int fd, uint16_t type, uint16_t code, int32_t value)
{
	struct uinput_event event;

	memset(&event, 0, sizeof(event));
	event.type	= type;
	event.code	= code;
	event.value	= value;

	return write(fd, &event, sizeof(event));
}

static void send_key(int fd, uint16_t key, int pressed)
{
	if (fd < 0)
		return;

	send_event(fd, EV_KEY, key, pressed);
	send_event(fd, EV_SYN, SYN_REPORT, 0);
}

static void uinput_handle_panel_passthrough(struct control *control,
					const unsigned char *operands,
					int operand_count)
{
	const char *status;
	struct uinput_priv *priv = control->plugin_priv;
	int pressed, i, fd = priv->fd;

	if (operand_count == 0)
		return;

	if (operands[0] & 0x80) {
		status = "released";
		pressed = 0;
	} else {
		status = "pressed";
		pressed = 1;
	}

	for (i = 0; key_map[i].name != NULL; i++) {
		uint8_t key_quirks;

		if ((operands[0] & 0x7F) != key_map[i].avrcp)
			continue;

		DBG("AVRCP: %s %s", key_map[i].name, status);

		key_quirks = control->key_quirks[key_map[i].avrcp];

		if (key_quirks & QUIRK_NO_RELEASE) {
			if (!pressed) {
				DBG("AVRCP: Ignoring release");
				break;
			}

			DBG("AVRCP: treating key press as press + release");
			send_key(fd, key_map[i].uinput, 1);
			send_key(fd, key_map[i].uinput, 0);
			break;
		}

		send_key(fd, key_map[i].uinput, pressed);
		break;
	}

	if (key_map[i].name == NULL)
		DBG("AVRCP: unknown button 0x%02X %s",
						operands[0] & 0x7F, status);
}

static void uinput_disconnect(struct control *control, struct audio_device *dev)
{
	struct uinput_priv *priv = control->plugin_priv;
	int fd = priv->fd;

	if (fd >= 0) {
		char address[18];

		ba2str(&dev->dst, address);
		DBG("AVRCP: closing uinput for %s", address);

		ioctl(fd, UI_DEV_DESTROY);
		close(fd);
	}
	free(control->plugin_priv);
	control->plugin_priv = NULL;
}

static int uinput_create(char *name)
{
	struct uinput_dev dev;
	int fd, err, i;

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0) {
		fd = open("/dev/input/uinput", O_RDWR);
		if (fd < 0) {
			fd = open("/dev/misc/uinput", O_RDWR);
			if (fd < 0) {
				err = errno;
				error("Can't open input device: %s (%d)",
							strerror(err), err);
				return -err;
			}
		}
	}

	memset(&dev, 0, sizeof(dev));
	if (name)
		strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE - 1);

	dev.id.bustype = BUS_BLUETOOTH;
	dev.id.vendor  = 0x0000;
	dev.id.product = 0x0000;
	dev.id.version = 0x0000;

	if (write(fd, &dev, sizeof(dev)) < 0) {
		err = errno;
		error("Can't write device information: %s (%d)",
						strerror(err), err);
		close(fd);
		errno = err;
		return -err;
	}

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_EVBIT, EV_REP);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	for (i = 0; key_map[i].name != NULL; i++)
		ioctl(fd, UI_SET_KEYBIT, key_map[i].uinput);

	if (ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
		err = errno;
		error("Can't create uinput device: %s (%d)",
						strerror(err), err);
		close(fd);
		errno = err;
		return -err;
	}

	return fd;
}

static void uinput_connect(struct control *control)
{
	struct audio_device *dev = control->dev;
	struct uinput_priv *priv;
	char address[18], name[248 + 1];
	int fd;

	priv = malloc(sizeof(*dev));
	if (priv == NULL)
		error("AVRCP: unable to allocate memory");
	memset(priv, 0, sizeof(*priv));

	device_get_name(dev->btd_dev, name, sizeof(name));
	if (g_str_equal(name, "Nokia CK-20W")) {
		control->key_quirks[FORWARD_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[BACKWARD_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[PLAY_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[PAUSE_OP] |= QUIRK_NO_RELEASE;
	}

	ba2str(&dev->dst, address);

	fd = uinput_create(address);
	if (fd < 0)
		error("AVRCP: failed to init uinput for %s", address);
	else
		DBG("AVRCP: uinput initialized for %s", address);
	control->plugin_priv = priv;
	priv->fd = fd;
}

struct control_plugin uinput_control_plugin = {
	.name = "uinput",
	.connect = uinput_connect,
	.handle_panel_passthrough = uinput_handle_panel_passthrough,
	.disconnect = uinput_disconnect,
};

