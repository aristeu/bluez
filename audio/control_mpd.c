#if 0
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif                                                                                                                                                        
#include <stdlib.h>                                                                                                                                           #include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>                                                                                                                                           #include <signal.h>
#include <sys/types.h>                                                                                                                                        #include <sys/stat.h>
#include <fcntl.h>                                                                                                                                            
#include <bluetooth/bluetooth.h>                                                                                                                              
#include <glib.h>

#include "log.h"
#include "error.h"
#include "adapter.h"
#include "../src/device.h"
#include "device.h"                                                                                                                                           #include "control.h"
#include "uinput.h"
#include "control_uinput.h"
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <glib.h>
#include <mpd/client.h>
#include <bluetooth/bluetooth.h>
#include "log.h"
#include "error.h"
#include "control.h"
#include "control_mpd.h"

#define MPD_DEFAULT_PORT 444

struct mpd_priv {
	int			socket;
	char			*server;
	int			port;
	char			*user;
	char			*password;
	bool			auth;
	struct mpd_connection	*conn;
};

static void mconnect(struct control *control)
{
	struct mpd_priv *priv = control->plugin_priv;

	priv->conn = mpd_connection_new(priv->server, priv->port, 0);
	if (priv->conn == NULL) {
		error("MPD: not enough memory");
		return;
	}

	if (mpd_connection_get_error(priv->conn) != MPD_ERROR_SUCCESS) {
		error("MPD: unable to connect to the server");
		return;
	}
}

static void handle_panel_passthrough(struct control *control,
				  const unsigned char *operands,
				  int operand_count)
{
	struct mpd_priv *priv = control->plugin_priv;
	bool rc;

	if (operand_count == 0)
		return;

	/*
	 * we don't care about repeat, just issue the command on the first
	 * press for now
	 */
	if (operands[0] & 0x80)
		return;

	switch (operands[0] & 0x7F) {
		case PLAY_OP:
			rc = mpd_send_play(priv->conn);
		break;
		case STOP_OP:
			rc = mpd_send_stop(priv->conn);
		break;
		case PAUSE_OP:
			rc = mpd_send_toggle_pause(priv->conn);
		break;
		case FORWARD_OP:
			rc = mpd_send_next(priv->conn);
		break;
		case BACKWARD_OP:
			rc = mpd_send_previous(priv->conn);
		break;
		default:
			DBG("AVRCP: unknown op: %#x", operands[0] & 0x7F);
			return;
	}
	if (rc == false)
		DBG("AVRCP: error sending command to mpd: %s\n",
		    mpd_connection_get_error_message(priv->conn)); 
}

static void mdisconnect(struct control *control, struct audio_device *dev)
{
	struct mpd_priv *priv = control->plugin_priv;

	mpd_connection_free(priv->conn);
}

static int minit(struct control *control, GKeyFile *config)
{
	struct mpd_priv *priv;
	GError *err = NULL;

	priv = g_new0(struct mpd_priv, 1);
	if (priv == NULL)
		return 1;

	priv->server = g_key_file_get_string(config, "MPD", "server", &err);
	if (err) {
		error("MPD: at least server must be specified in the config file");
		goto error;
	}

	priv->port = g_key_file_get_integer(config, "MPD", "port", &err);
	if (err) {
		g_clear_error(&err);
		priv->port = MPD_DEFAULT_PORT;
	}

	priv->auth = g_key_file_get_boolean(config, "MPD", "auth", &err);
	if (err) {
		g_clear_error(&err);
		priv->auth = false;
	} else if (priv->auth == true) {
		priv->user = g_key_file_get_string(config, "MPD", "user", &err);
		if (!err) {
			priv->password = g_key_file_get_string(config, "MPD", "password", &err);
		}
		if (err) {
			error("MPD: user and password must be specified if authentication is enabled");
			goto error;
		}
	}

	control->plugin_priv = priv;

	return 0;

error:
	if (priv->server)
		g_free(priv->server);
	if (priv->password)
		g_free(priv->password);
	g_free(priv);
	return 1;
}

struct control_plugin mpd_control_plugin = {
	.name = "mpd",
	.init = minit,
	.connect = mconnect,
	.handle_panel_passthrough = handle_panel_passthrough,
	.disconnect = mdisconnect,
};

