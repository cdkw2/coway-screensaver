#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <linux/limits.h>
#include <locale.h>
#include <math.h>
#include "conway-screensaver.h"

#define CONFIG_FILE "game_of_life.conf"
#define MAX_COLORS 8

int WIDTH, HEIGHT;
time_t last_glider_time;
int last_glider_spawn[2] = {0, 0};

typedef struct {
	int alive;
	int age;
} Cell;

Config config;

char config_path[PATH_MAX];

void get_config_path() {
	char exe_path[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (len != -1) {
		exe_path[len] = '\0';
		char *dir = dirname(exe_path);
		(void)dir;
		snprintf(config_path, sizeof(config_path), "%s/.config/conway-screensaver/%s", getenv("HOME"), CONFIG_FILE);
	} else {
		strcpy(config_path, CONFIG_FILE);
	}
}

void load_default_config() {
	config.infinite_mode = DEFAULT_INFINITE_MODE;
	config.update_interval = DEFAULT_UPDATE_INTERVAL;
	wcsncpy(config.cell_char, DEFAULT_CELL_CHAR, 8);
	config.max_age = DEFAULT_MAX_AGE;
	config.color_mode = DEFAULT_COLOR_MODE;
	config.glider_interval = DEFAULT_GLIDER_INTERVAL;
	config.initial_density = DEFAULT_INITIAL_DENSITY;
	config.wrap_edges = DEFAULT_WRAP_EDGES;
	config.char_background = DEFAULT_CHAR_BACKGROUND;
	config.char_foreground = DEFAULT_CHAR_FOREGROUND;
	config.background = DEFAULT_BACKGROUND;
	config.debug = DEFAULT_DEBUG;
}

void load_config() {
	get_config_path();
	FILE *file = fopen(config_path, "r");
	if (file == NULL) {
		load_default_config();
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), file)) {
		char *key = strtok(line, "=");
		char *value = strtok(NULL, "\n");
		if (key && value) {
			if (strcmp(key, "infinite_mode") == 0) config.infinite_mode = atoi(value);
			else if (strcmp(key, "update_interval") == 0) config.update_interval = atoi(value);
			else if (strcmp(key, "cell_char") == 0) mbstowcs(config.cell_char, value, 7);
			else if (strcmp(key, "max_age") == 0) config.max_age = atoi(value);
			else if (strcmp(key, "color_mode") == 0) config.color_mode = atoi(value);
			else if (strcmp(key, "glider_interval") == 0) config.glider_interval = atoi(value);
			else if (strcmp(key, "initial_density") == 0) config.initial_density = atof(value);
			else if (strcmp(key, "wrap_edges") == 0) config.wrap_edges = atoi(value);
			else if (strcmp(key, "char_background") == 0) config.char_background = atoi(value);
			else if (strcmp(key, "char_foreground") == 0) config.char_foreground = atoi(value);
			else if (strcmp(key, "background") == 0) config.background = atoi(value);
			else if (strcmp(key, "debug") == 0) config.debug = atoi(value);
		}
	}

	fclose(file);
}

void init_grid(Cell *grid) {
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			grid[y*WIDTH + x].alive = (rand() / (float)RAND_MAX < config.initial_density);
			grid[y*WIDTH + x].age = 0;
		}
	}
}

void print_grid(Cell *grid) {
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			if (grid[y*WIDTH + x].alive) {
				int color = config.max_age + 1;
				if (config.color_mode)
					color = (grid[y*WIDTH + x].age % config.max_age) + 1;

				attron(COLOR_PAIR(color));
				mvaddwstr(y, x, config.cell_char);
				attroff(COLOR_PAIR(color));
			} else {
				attron(COLOR_PAIR(config.max_age + 2));
				mvaddch(y, x, ' ');
				attroff(COLOR_PAIR(config.max_age + 2));
			}
		}
	}
}

int count_neighbors(Cell *grid, int x, int y) {
	int count = 0;
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			if (dx == 0 && dy == 0) continue;
			int nx = x + dx;
			int ny = y + dy;
			if (config.wrap_edges) {
				nx = (nx + WIDTH) % WIDTH;
				ny = (ny + HEIGHT) % HEIGHT;
			} else {
				if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
			}
			if (grid[ny*WIDTH + nx].alive) count++;
		}
	}
	return count;
}

void spawn_glider(Cell *grid, int x, int y) {
	int glider[3][3] = {
		{0, 1, 0},
		{0, 0, 1},
		{1, 1, 1}
	};

	for (int dy = 0; dy < 3; dy++) {
		for (int dx = 0; dx < 3; dx++) {
			int nx = (x + dx) % WIDTH;
			int ny = (y + dy) % HEIGHT;
			grid[ny*WIDTH + nx].alive = glider[dy][dx];
			grid[ny*WIDTH + nx].age = 0;
		}
	}
}

int is_area_free(Cell *grid, int x, int y) {
	int glider[3][3] = {
		{0, 1, 0},
		{0, 0, 1},
		{1, 1, 1}
	};

	for (int dy = 0; dy < 3; dy++) {
		for (int dx = 0; dx < 3; dx++) {
			int nx = (x + dx) % WIDTH;
			int ny = (y + dy) % HEIGHT;
			if (grid[ny * WIDTH + nx].alive && glider[dy][dx] != 0) {
				return 0;
			}
		}
	}
	return 1;
}

void update_grid(Cell *grid, Cell *new_grid) {
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			int neighbors = count_neighbors(grid, x, y);
			if (grid[y*WIDTH + x].alive) {
				new_grid[y*WIDTH + x].alive = (neighbors == 2 || neighbors == 3);
				new_grid[y*WIDTH + x].age = new_grid[y*WIDTH + x].alive ? grid[y*WIDTH + x].age + 1 : 0;
			} else {
				new_grid[y*WIDTH + x].alive = (neighbors == 3);
				new_grid[y*WIDTH + x].age = 0;
			}
		}
	}

	if (config.infinite_mode && difftime(time(NULL), last_glider_time) >= config.glider_interval) {
		int rx = rand() % (WIDTH - 3);
		int ry = rand() % (HEIGHT - 3);

		if (is_area_free(new_grid, rx, ry)) {
			spawn_glider(new_grid, rx, ry);
			last_glider_time = time(NULL);
			last_glider_spawn[0] = rx;
			last_glider_spawn[1] = ry;
		}
	}
}

void resize_grid(Cell **grid, Cell **new_grid) {
	int new_height, new_width;
	getmaxyx(stdscr, new_height, new_width);
	if (config.debug)
		new_height--;

	if (new_height != HEIGHT || new_width != WIDTH) {
		HEIGHT = new_height;
		WIDTH = new_width;

		*grid = realloc(*grid, HEIGHT * WIDTH * sizeof(Cell));
		if (*grid == NULL)
			exit(1);

		*new_grid = realloc(*new_grid, HEIGHT * WIDTH * sizeof(Cell));
		if (*new_grid == NULL)
			exit(1);

		init_grid(*grid);
	}
}

// void init_pattern(Cell **grid, const char *pattern) {
// 	if (strcmp(pattern, "glider") == 0) {
// 		spawn_glider(grid, WIDTH / 2, HEIGHT / 2);
// 	} else if (strcmp(pattern, "blinker") == 0) {
// 		grid[HEIGHT / 2][WIDTH / 2 - 1].alive = 1;
// 		grid[HEIGHT / 2][WIDTH / 2].alive = 1;
// 		grid[HEIGHT / 2][WIDTH / 2 + 1].alive = 1;
// 	}
// }

int main() {
	setlocale(LC_ALL, "");
	load_config();

	srand(time(NULL));
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	timeout(0);
	start_color();
	use_default_colors();

	for (int i = 1; i <= config.max_age; i++) {
		init_pair(i, i, config.char_background);
	}
	init_pair(config.max_age+1, config.char_foreground, config.char_background);
	init_pair(config.max_age+2, -1, config.background);

	getmaxyx(stdscr, HEIGHT, WIDTH);
	if (config.debug)
		HEIGHT--;

	Cell *grid = malloc(HEIGHT * WIDTH * sizeof(Cell));
	if (grid == NULL)
		exit(1);
	Cell *new_grid = malloc(HEIGHT * WIDTH * sizeof(Cell));
	if (new_grid == NULL)
		exit(1);

	init_grid(grid);
	last_glider_time = time(NULL);

	// char pattern[20];
	// mvprintw(0, 0, "Choose a pattern (glider/blinker): ");
	// getnstr(pattern, sizeof(pattern) - 1);
	// init_pattern(grid, pattern);

	int generation = 0;
	while (1) {
		resize_grid(&grid, &new_grid);
		print_grid(grid);
		if (config.debug)
			mvprintw(HEIGHT, 0, "Generation: %d | Glider spawned at (%d, %d) | Press '%c' to quit, '%c' to reset, '%c' to speed up, '%c' to slow down",
					generation, last_glider_spawn[0], last_glider_spawn[1], MY_KEY_QUIT, MY_KEY_RESET, MY_KEY_SPEED_UP, MY_KEY_SLOW_DOWN);
		refresh();

		update_grid(grid, new_grid);

		Cell *temp = grid;
		grid = new_grid;
		new_grid = temp;

		generation++;

		int ch = getch();
		if (ch == MY_KEY_QUIT) break;
		else if (ch == MY_KEY_RESET) {
			init_grid(grid);
			generation = 0;
		} else if (ch == MY_KEY_SPEED_UP) {
			config.update_interval = (int)fmax(1, config.update_interval - 10000);
		} else if (ch == MY_KEY_SLOW_DOWN) {
			config.update_interval = (int)fmin(1000000, config.update_interval + 10000);
		}
		usleep(config.update_interval);
	}

	free(grid);
	free(new_grid);
	endwin();
	return 0;
}
