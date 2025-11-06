#ifndef CONWAY_SCREENSAVER_H
#define CONWAY_SCREENSAVER_H

#include <wchar.h>

#define DEFAULT_INFINITE_MODE 0
#define DEFAULT_UPDATE_INTERVAL 100000
#define DEFAULT_CELL_CHAR L""
#define DEFAULT_MAX_AGE 5
#define DEFAULT_COLOR_MODE 1
#define DEFAULT_GLIDER_INTERVAL 5
#define DEFAULT_INITIAL_DENSITY 0.2
#define DEFAULT_WRAP_EDGES 1
#define DEFAULT_CHAR_BACKGROUND -1
#define DEFAULT_CHAR_FOREGROUND -1
#define DEFAULT_BACKGROUND -1
#define DEFAULT_DEBUG 0

#define MY_KEY_QUIT 'q'
#define MY_KEY_RESET 'r'
#define MY_KEY_SPEED_UP '+'
#define MY_KEY_SLOW_DOWN '-'

typedef struct {
	int infinite_mode;
	int update_interval;
	wchar_t cell_char[8];
	int max_age;
	int color_mode;
	int glider_interval;
	float initial_density;
	int wrap_edges;
	int char_background;
	int char_foreground;
	int background;
	int debug;
} Config;

extern Config config;

void load_default_config();

#endif
