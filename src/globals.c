#include "globals.h"

// Define global variables
Category categories[MAX_CATEGORIES];
int category_count = 0;
Transaction *current_month_transactions = NULL;
Transaction *current_month_transactions_sorted = NULL;
int current_month_transaction_count = 0;
double total_budget = 0.0;
Subscription subscriptions[MAX_SUBSCRIPTIONS];
int subscription_count = 0;

// Month management
MonthIndexManager month_index_manager = {NULL, 0, 0};
int current_month = 0;
int current_year = 0;
int loaded_month = 0;
int loaded_year = 0;

// Global file paths
char app_data_dir[MAX_BUFFER];
char data_storage_dir[MAX_BUFFER];
char data_file_path[MAX_BUFFER];
char export_file_path[MAX_BUFFER];

const short pie_colors[][2] = {
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_YELLOW},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_GREEN},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_MAGENTA},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_CYAN},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_RED},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_BLUE},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_BLACK},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_WHITE},
};

const short pie_colors_darker[][2] = {
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_YELLOW},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_GREEN},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_MAGENTA},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_CYAN},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_RED},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_BLUE},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_BLACK},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_WHITE},
};