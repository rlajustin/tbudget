#include "globals.h"

// Current month data
int category_count = 0;
Category categories[MAX_CATEGORIES] = {0};
int sorted_categories_indices[MAX_CATEGORIES] = {0};
double current_month_total_budget = 0.0;
double uncategorized_spent = 0.0;
TransactionNode *transaction_head = NULL;
TransactionNode *transaction_tail = NULL;
int current_month_transaction_count = 0;
TransactionNode **sorted_transactions = NULL;

// data file
int subscription_count = 0;
Subscription subscriptions[MAX_SUBSCRIPTIONS] = {0};
int default_category_count = 0;
Category default_categories[MAX_CATEGORIES] = {0};
double default_monthly_budget = 0.0;

FlexContainer *main_layout = NULL;

// Month management
int today_month = 0;
int today_year = 0;
int current_month = 0;
int current_year = 0;
int loaded_month = 0;
int loaded_year = 0;

// Global file paths
char app_data_dir[MAX_BUFFER];
char data_storage_dir[MAX_BUFFER];
char export_file_path[MAX_BUFFER];
char data_file_path[MAX_BUFFER];

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
