#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "globals.h"
#include "utils.h"
#include "piechart.h"

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

typedef enum InputType {
  INPUT_STRING,
  INPUT_DOUBLE,
  INPUT_INT,
} InputType;

typedef struct {
  WINDOW *textbox;
  WINDOW *boundary;
} BoundedWindow;

void setup_ncurses();
void cleanup_ncurses();

// bounded window methods
void delete_bounded(BoundedWindow win);
void bwresize(BoundedWindow win, int height, int width);  
void bwmove(BoundedWindow win, int start_y, int start_x);
void bwnoutrefresh(BoundedWindow win);

// just drawing
BoundedWindow draw_bounded(int height, int width, int start_y, int start_x, bool highlight);
BoundedWindow draw_bounded_with_title(int height, int width, int start_y, int start_x, const char* title, bool highlight, int alignment);
BoundedWindow draw_alert(const char *title, const char *message[], int message_count);
BoundedWindow draw_alert_persistent(const char *title, const char *message[], int message_count);
void draw_title(WINDOW *win, const char *title);
void draw_error(BoundedWindow win, const char *message);
void draw_menu(WINDOW *win, int highlighted_item, const char *menu_items[], int item_count, int start_y);

// getting input
int get_confirmation(WINDOW *win, const char *message[], int item_count);
int get_menu_choice(WINDOW *win, const char *menu_items[], int item_count, int start_y, int allow_back);
int get_scrollable_menu_choice(WINDOW *win, char* prompt, const char *menu_items[], int item_count, int max_visible_items, int start_y);
int get_transaction_choice(WINDOW *win, const Transaction transactions[], int transaction_count, int max_visible_items);
int get_input(WINDOW *win, void *return_value, char* prompt, int max_len, InputType type);
int get_date_input(WINDOW *win, char *date_buffer, char* prompt);

// dashboard display
void display_categories(WINDOW *win, int start_y);
void display_transactions(WINDOW *win, int start_y);
BoundedWindow draw_bar_chart(WINDOW *win);

// other helper
void format_date(WINDOW *win, int y, int x, int day, int month, int year, int highlighted_field, int cursor_positions[]);

#endif // UI_H

