#ifndef SAVELOAD_H
#define SAVELOAD_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include "utils.h"
#include "ui.h"
#include "globals.h"
#include "file_cache.h"

typedef struct
{
    char name[16];        // "tbudget_" + version
    time_t last_modified; // Last modification time
} FileHeader;

// Function prototypes for utils.c
char *get_home_directory();
int create_directory_if_not_exists(const char *path);
void initialize_data_directories();
int load_budget_data();
int load_month(int year, int month);
int add_transaction(Transaction *transaction, int year, int month);
int add_subscription(Subscription *subscription, int year, int month);
int add_category(Category *category, int year, int month);
int remove_category(int category_index, int year, int month);
int set_budget(double budget, int year, int month);
int remove_transaction(int index);

int get_category_index(int year, int month, char *name);
int read_month_categories(int year, int month, Category *out_categories, int *out_count);
// int save_data_to_file();
// void write_export_content(FILE *export_file);
// void export_data_to_csv(int silent);
// void import_data_from_csv(const char *filename);
#endif // SAVELOAD_H