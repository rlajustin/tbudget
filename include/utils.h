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
#include <stdio.h>
#include "globals.h"
#include "transaction_manager.h"

// Function prototypes for utils.c
void write_export_content(FILE *export_file);
void export_data_to_csv(int silent);
void import_data_from_csv(const char *filename);
void save_data_to_file();
int load_data_from_file();
char *get_home_directory();
int create_directory_if_not_exists(const char *path);
void initialize_data_directories();
int get_days_in_month(int m, int y);
int validate_day(int day, int month, int year);
void compute_monthly(int month, int year);
void sort_categories_by_budget();
void sort_categories_by_spent();
#endif // UTILS_H 