#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include "minui/minui.h"

#define CRYPTSETUP		"/system/bin/cryptsetup"

#define SDCARD_DEVICE		"/dev/block/mmcblk0p2"
#define DATA_DEVICE		"/dev/block/loop0"

#define SDCARD_MAPPER_NAME	"encrypted-sdcard"
#define DATA_MAPPER_NAME	"encrypted-data"

#define CHAR_WIDTH		23
#define CHAR_HEIGHT		41

#define CHAR_START		0x20
#define CHAR_END		0x7E

// should have been defined in linux/input.h ?!
#define ABS_MT_TRACKING_ID	0x39	/* Unique ID of initiated contact */
#define ABS_MT_POSITION_X	0x35	/* Center X touch position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y touch position */
#define ABS_MT_WIDTH_MAJOR	0x32	/* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* Minor axis (omit if circular) */
#define SYN_MT_REPORT		2
#define ABS_MT_TOUCH_MAJOR	0x30	/* Major axis of touching ellipse */
#define ABS_MT_WIDTH_MAJOR	0x32	/* Major axis of approaching ellipse */

struct keymap {
	unsigned char key;
	int xpos;
	int ypos;
	int selected;
};

struct keymap keys[CHAR_END - CHAR_START];
struct input_event keyqueue[2048];

char passphrase[1024];

pthread_mutex_t keymutex;
unsigned int sp = 0;

gr_surface background;
int res, current = 0;

void wipe_passphrase() {
	memset(passphrase, 0, 1024);
}

void draw_keymap() {
	size_t i;
	char keybuf[2];

	for(i = 0; i < (CHAR_END - CHAR_START); i++) {
		sprintf(keybuf, "%c", keys[i].key);

		if(keys[i].selected == 1) {
			gr_color(255, 0, 0, 255);
			gr_fill(keys[i].xpos, keys[i].ypos - CHAR_HEIGHT, keys[i].xpos + CHAR_WIDTH, keys[i].ypos);
			gr_color(255, 255, 255, 255);
		}
		else
			gr_color(0, 0, 0, 255);

		gr_text(keys[i].xpos, keys[i].ypos, keybuf);
	}
}

static void *input_thread() {
	int rel_sum = 0;

	for(;;) {
		struct  input_event ev;

		do {
			ev_get(&ev, 0);

			switch(ev.type) {
				case EV_REL:
					rel_sum += ev.value;
					break;

				default:
					rel_sum = 0;
			}

			if(rel_sum > 4 || rel_sum < -4)
				break;

		} while((ev.type != EV_KEY || ev.code > KEY_MAX) && ev.type != EV_ABS && ev.type != EV_SYN);

		rel_sum = 0;

		// Add the key to the fifo
		pthread_mutex_lock(&keymutex);
		if(sp < (sizeof(keyqueue) / sizeof(struct input_event)))
			sp++;

		keyqueue[sp] = ev;
		pthread_mutex_unlock(&keymutex);
	}

	return 0;
}

void ui_init(void) {
	gr_init();
	ev_init();

	// Generate bitmap from /system/res/padlock.png ( you can change the path in minui/resources.c)
	res_create_surface("padlock", &background);
	wipe_passphrase();
}

void draw_screen() {
	// This probably only looks good in HTC Wildfire resolution
	int bgwidth, bgheight, bgxpos, bgypos, i, cols;

	gr_color(0, 0, 0, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	bgwidth = gr_get_width(background);
	bgheight = gr_get_height(background);
	bgxpos = (gr_fb_width() - gr_get_width(background)) / 2;
	bgypos = (gr_fb_height() - gr_get_height(background)) / 2;

	gr_blit(background, 0, 0, bgwidth, bgheight, bgxpos, bgypos);

	gr_text(0, CHAR_HEIGHT, "Enter unlock phrase: ");

	cols = gr_fb_width() / CHAR_WIDTH;

	for(i = 0; i < (int) strlen(passphrase); i++) 
		gr_text(i * CHAR_WIDTH, CHAR_HEIGHT * 2, "*");

	for(; i < cols - 1; i++)
		gr_text(i * CHAR_WIDTH, CHAR_HEIGHT * 2, "_");

	gr_text(0, gr_fb_height() - CHAR_HEIGHT, "Press Volup to unlock");
	gr_text(0, gr_fb_height(), "Press Voldown to erase");

	draw_keymap();
	gr_flip();
}

void generate_keymap() {
	int xpos, ypos;
	char key;
	int i;

	xpos = 0;
	ypos = CHAR_HEIGHT * 4;

	for(i = 0, key = CHAR_START; key < CHAR_END; key++, i++, xpos += (CHAR_WIDTH * 2)) {
		if(xpos >= gr_fb_width() - CHAR_WIDTH) {
			ypos += CHAR_HEIGHT * 3 / 2;
			xpos = 0;
		}
		keys[i].key = key;
		keys[i].xpos = xpos;
		keys[i].ypos = ypos;
		keys[i].selected = 0;
	}

	keys[current].selected = 1;
}

int try_unlock(char* device, char* name) {
	// classic pipe+dup+exec
	int pipes[2];
	int pid;
	pipe(pipes);
	if ((pid = fork()) == -1) {
		perror("fork");
		_exit(1);
	} else if (pid == 0) { // child cryptsetup
		close(pipes[1]); // close the writing pipe
		dup2(pipes[0], 0); // make new stdin the reading pipe
		close(pipes[0]); // it's unneeded now
		execlp(CRYPTSETUP, "cryptsetup", "luksOpen", device, name, NULL);
		perror("exec"); // still here?!
		_exit(1); // no flush
	}
	// parent should write password now
	close(pipes[0]);
	write(pipes[1], passphrase, strlen(passphrase));
	close(pipes[1]);
	wait(NULL);
	// check /dev/mapper/whatever for readability
	char buffer[4096];
	snprintf(buffer, sizeof(buffer) - 1, "/dev/mapper/%s", name);
	int fd = open(buffer, 0);
	if(fd < 0)
		return 1;
	else {
		close(fd);
		return 0;
	}
}

	

void unlock() {
	char buffer[2048];
	int fd, failed = 0;

	gr_color(0, 0, 0, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
	gr_color(255, 255, 255, 255);

	gr_text((gr_fb_width() / 2) - ((strlen("Unlocking...") / 2) * CHAR_WIDTH), gr_fb_height() / 2, "Unlocking...");
	gr_flip();

	failed = try_unlock(SDCARD_DEVICE, SDCARD_MAPPER_NAME) + try_unlock(DATA_DEVICE, DATA_MAPPER_NAME);

	if(!failed) {
		gr_text((gr_fb_width() / 2) - ((strlen("Success!") / 2) * CHAR_WIDTH), gr_fb_height() / 2 + CHAR_HEIGHT, "Success!");
		gr_flip();
		exit(0);
	}

	gr_text((gr_fb_width() / 2) - ((strlen("Failed!") / 2) * CHAR_WIDTH), gr_fb_height() / 2 + CHAR_HEIGHT, "Failed!");
	gr_flip();

	sleep(2);
	wipe_passphrase();
}

void handle_key(struct input_event event) {
	int cols;

	cols = gr_fb_width() / (CHAR_WIDTH * 2);
	keys[current].selected = 0;

	// Joystick down or up
	if(event.type == EV_REL && event.code == 1) {
		if(event.value > 0) {
			if(current + cols < (CHAR_END - CHAR_START))
				current += cols;
		} else if(event.value < 0) {
			if(current - cols > 0)
				current -= cols;
		}
	}

	// Joystick left or right
	if(event.type == EV_REL && event.code == 0) {
		if(event.value > 0 && current < (CHAR_END - CHAR_START) - 1)
				current++;
		else if(event.value < 0 && current > 0)
				current--;
	}

	keys[current].selected = 1;

	// Pressed joystick
	if(event.type == EV_KEY && event.value == 0 && event.code == BTN_MOUSE) {
		passphrase[strlen(passphrase)] = keys[current].key;
	}

	// Pressed vol down
	if(event.type == EV_KEY && event.code == KEY_VOLUMEDOWN)
		wipe_passphrase();

	// Pressed vol up
	if(event.type == 1 && event.code == KEY_VOLUMEUP) {
		unlock();
	}

	draw_screen();
}

void handle_touch(struct input_event event) {
	static __s32 touch_x;
	static __s32 touch_y;
	static int touch_flag_ok;
	if (event.type == EV_ABS) {
		if (event.code == ABS_MT_TRACKING_ID) {
			touch_flag_ok = 0;
		} else if (event.code == ABS_MT_POSITION_X) {
			touch_x = event.value;
			touch_flag_ok++;
		} else if (event.code == ABS_MT_POSITION_Y) {
			touch_y = event.value;
			touch_flag_ok++;
		} else if ((event.code == ABS_MT_WIDTH_MAJOR || event.code == ABS_MT_TOUCH_MAJOR) && event.value == 0) {
			touch_flag_ok++;
		}
	} else if (event.type == EV_SYN && event.code == SYN_MT_REPORT && touch_flag_ok == 4) {
		int cols = gr_fb_width() / (CHAR_WIDTH * 2);
		int row = (touch_y - CHAR_HEIGHT * 4) / (CHAR_HEIGHT * 3 / 2);
		int col = touch_x / (CHAR_WIDTH * 2);
		int index = cols*row + col;
		keys[current].selected = 0;
		current = index;
		passphrase[strlen(passphrase)] = keys[current].key;
		keys[current].selected = 1;
		draw_screen();
	}
}

int main(int argc, char **argv, char **envp) {
	struct input_event event;
	pthread_t t;
	unsigned int i, key_up = 0;

	ui_init();
	generate_keymap();
	draw_screen();

	pthread_create(&t, NULL, input_thread, NULL);
	pthread_mutex_init(&keymutex, NULL);

	for(;;) {
		pthread_mutex_lock(&keymutex);

		if(sp > 0) {
			for(i = 0; i < sp; i++)
				keyqueue[i] = keyqueue[i + 1];

			event = keyqueue[0];
			sp--;

			pthread_mutex_unlock(&keymutex);
		} else {
			pthread_mutex_unlock(&keymutex);
			continue;
		}

		switch(event.type) {
			case(EV_KEY):
				if(key_up == 1) {
					key_up = 0;
					break;
				}
				key_up = 1;
			case(EV_REL):
				handle_key(event);
				break;
			case(EV_ABS):
			case(EV_SYN):
				handle_touch(event);
				break;
		}
	}

	return 0;
}
