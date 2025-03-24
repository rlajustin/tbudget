#ifndef UTILS_H
#define UTILS_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

// Function prototypes
void setup_ncurses();
void cleanup_ncurses();
void draw_title(WINDOW *win, const char *title);
void draw_menu(WINDOW *win, int highlighted_item, const char *menu_items[], int item_count, int start_y);
int get_menu_choice(WINDOW *win, const char *menu_items[], int item_count, int start_y, int allow_back);
int get_string_input(WINDOW *win, char *buffer, int max_len, int y, int x);
double get_double_input(WINDOW *win, int y, int x);
int get_int_input(WINDOW *win, int y, int x);
int get_date_input(WINDOW *win, char *date_buffer, int y, int x);
void display_categories(WINDOW *win, int start_y);
void display_transactions(WINDOW *win, int start_y);
void write_export_content(FILE *export_file);
void export_data_to_csv(int silent);
void import_data_from_csv(const char *filename);
void save_data_to_file();
void load_data_from_file();
char *get_home_directory();
int create_directory_if_not_exists(const char *path);
void initialize_data_directories();

#endif // UTILS_H 