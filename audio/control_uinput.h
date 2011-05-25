#ifndef __CONTROL_UINPUT_H
/* operands in passthrough commands */
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

void uinput_connect(struct control *control);
void uinput_handle_panel_passthrough(struct control *control,
					const unsigned char *operands,
					int operand_count);
void uinput_disconnect(struct control *control, struct audio_device *dev);
#endif	/* __CONTROL_UINPUT_H */
