/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define AUDIO_CONTROL_INTERFACE "org.bluez.Control"

/* operands in passthrough commands */
#if 0
	SELECT_OP,
	UP_OP,
	DOWN_OP,
	LEFT_OP,
	RIGHT_OP,
	RIGHT_UP_OP,
	RIGHT_DOWN_OP,
	LEFT_UP_OP,
	LEFT_DOWN_OP,
	ROOT_MENU_OP,
	SETUP_MENU_OP,
	CONTENTS_MENU_OP,
	FAVORITE_MENU_OP,
	EXIT_OP,
	N0_OP,
	N1_OP,
	N2_OP,
	N3_OP,
	N4_OP,
	N5_OP,
	N6_OP,
	N7_OP,
	N8_OP,
	N9_OP,
	NDOT_OP,
	ENTER_OP,
	CLEAR_OP,
	CHANNEL_UP_OP,
	CHANNEL_DOWN_OP,
	PREV_CHANNEL_OP,
	SOUND_SELECT_OP,
	INPUT_SELECT_OP,
	DISPLAY_INFO_OP,
	HELP_OP,
	PAGE_UP_OP,
	PAGE_DOWN_OP,
	POWER_OP,
	VOL_UP_OP,
	VOL_DOWN_OP,
	MUTE_OP,
	PLAY_OP,
	STOP_OP,
	PAUSE_OP,
	RECORD_OP,
	REWIND_OP,
	FAST_FORWARD_OP,
	EJECT_OP,
	FORWARD_OP,
	BACKWARD_OP,
	ANGLE_OP,
	SUBPICTURE_OP,
	F1_OP,
	F2_OP,
	F3_OP,
	F4_OP,
	F5_OP,
#endif
#define VOL_UP_OP		0x41
#define VOL_DOWN_OP		0x42
#define MUTE_OP			0x43
#define PLAY_OP			0x44
#define STOP_OP			0x45
#define PAUSE_OP		0x46
#define RECORD_OP		0x47
#define REWIND_OP		0x48
#define FAST_FORWARD_OP		0x49
#define EJECT_OP		0x4a
#define FORWARD_OP		0x4b
#define BACKWARD_OP		0x4c

#define QUIRK_NO_RELEASE	1 << 0

typedef enum {
	AVCTP_STATE_DISCONNECTED = 0,
	AVCTP_STATE_CONNECTING,
	AVCTP_STATE_CONNECTED
} avctp_state_t;

struct control_plugin;
struct control {
	struct audio_device *dev;

	avctp_state_t state;

	GIOChannel *io;
	guint io_id;

	uint16_t mtu;

	gboolean target;

	uint8_t key_quirks[256];

	struct control_plugin *plugin;
	void *plugin_priv;
};

struct control_plugin
{
	char *name;

	void (*connect)(struct control *control);
	void (*handle_panel_passthrough)(struct control *control,
					const unsigned char *operands,
					int operand_count);
	void (*disconnect)(struct control *control, struct audio_device *dev);
};

typedef void (*avctp_state_cb) (struct audio_device *dev,
				avctp_state_t old_state,
				avctp_state_t new_state,
				void *user_data);

unsigned int avctp_add_state_cb(avctp_state_cb cb, void *user_data);
gboolean avctp_remove_state_cb(unsigned int id);

int avrcp_register(DBusConnection *conn, const bdaddr_t *src, GKeyFile *config);
void avrcp_unregister(const bdaddr_t *src);

gboolean avrcp_connect(struct audio_device *dev);
void avrcp_disconnect(struct audio_device *dev);

struct control *control_init(struct audio_device *dev, uint16_t uuid16);
void control_update(struct audio_device *dev, uint16_t uuid16);
void control_unregister(struct audio_device *dev);
gboolean control_is_active(struct audio_device *dev);
