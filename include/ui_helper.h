#ifndef UI_HELPER_H
#define UI_HELPER_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "globals.h"
#include "utils.h"
#include "transaction_manager.h"

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2
#define MAX_CHILD_WINDOWS 5  // Maximum number of child windows

// Forward declaration for the structure
typedef struct BoundedWindow BoundedWindow;

typedef enum InputType {
  INPUT_STRING,
  INPUT_DOUBLE,
  INPUT_INT,
} InputType;

// Simplified BoundedWindow structure with children
struct BoundedWindow {
  WINDOW *textbox;
  WINDOW *boundary;
  BoundedWindow **children;     // Array of child windows
  int child_count;              // Number of child windows
};

void setup_ncurses();
void cleanup_ncurses();

// bounded window methods
void delete_bounded(BoundedWindow win);
void delete_bounded_array(BoundedWindow* windows[], int count);
void bwresize(BoundedWindow win, int height, int width);
void bwmove(BoundedWindow win, int start_y, int start_x);
void bwnoutrefresh(BoundedWindow win);
void bwarrnoutrefresh(BoundedWindow* windows[], int count);

// New functions for parent-child relationship
BoundedWindow *create_bounded_window();
void add_child_window(BoundedWindow *parent, BoundedWindow *child);
void delete_bounded_with_children(BoundedWindow *win);
void bw_hierarchy_refresh(BoundedWindow win);

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
int get_scrollable_menu_choice(WINDOW *win, char* prompt, const char *menu_items[], int item_count, int max_visible_items, int start_y, bool show_numbers);
int get_input(WINDOW *win, void *return_value, char* prompt, int max_len, InputType type);
int get_date_input(WINDOW *win, char *date_buffer, char* prompt);

// other helper
void format_date(WINDOW *win, int y, int x, int day, int month, int year, int highlighted_field, int cursor_positions[]);

#endif // UI_HELPER_H
