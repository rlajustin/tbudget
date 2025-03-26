#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "globals.h"
#include "utils.h"

typedef enum InputType {
  INPUT_STRING,
  INPUT_DOUBLE,
  INPUT_INT,
} InputType;

// Function prototypes for ui.c
void setup_ncurses();
void cleanup_ncurses();
void draw_title(WINDOW *win, const char *title);
void draw_menu(WINDOW *win, int highlighted_item, const char *menu_items[], int item_count, int start_y);
int get_menu_choice(WINDOW *win, const char *menu_items[], int item_count, int start_y, int allow_back);
int get_input(WINDOW *win, void *return_value, int max_len, int y, int x, InputType type);
int get_date_input(WINDOW *win, char *date_buffer, int y, int x);
void display_categories(WINDOW *win, int start_y);
void display_transactions(WINDOW *win, int start_y);
void format_date(WINDOW *win, int y, int x, int day, int month, int year, int highlighted_field, int cursor_positions[]);

#endif // UI_H

