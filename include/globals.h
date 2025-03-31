#ifndef GLOBALS_H
#define GLOBALS_H

#include <ncurses.h>

// Constants
#define MAX_CATEGORIES 20
#define MAX_NAME_LEN 50
#define MAX_TRANSACTIONS 10000000 // you are probably not extremely wealthy so this should be enough
#define MAX_BUFFER 1024
#define DEFAULT_DIALOG_HEIGHT 10
#define DEFAULT_DIALOG_WIDTH 70
#define MAX_SUBSCRIPTIONS 50

// Period types for subscriptions
#define PERIOD_WEEKLY 0
#define PERIOD_MONTHLY 1
#define PERIOD_YEARLY 2
#define PERIOD_CUSTOM_DAYS 3

// Color Overrides
#define OVERRIDE_COLOR_BLACK 7
#define OVERRIDE_COLOR_RED 1
#define OVERRIDE_COLOR_GREEN 2
#define OVERRIDE_COLOR_YELLOW 4
#define OVERRIDE_COLOR_BLUE 3
#define OVERRIDE_COLOR_MAGENTA 5
#define OVERRIDE_COLOR_CYAN 6
#define OVERRIDE_COLOR_WHITE 0

// Window definitions
#define ACTIONS_MENU_WINDOW 0
#define BUDGET_SUMMARY_WINDOW 1
#define BUDGET_BREAKDOWN_WINDOW 2
#define TRANSACTION_HISTORY_WINDOW 3
#define SUBSCRIPTIONS_WINDOW 4
#define TODO_WINDOW 5

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Key definitions
#define KEY_BACKSPACE_ALT 127
#define KEY_ESCAPE 27

// Mode definitions
#define MODE_MENU 1
#define MODE_DASHBOARD 2

// Define file paths for storage
#define APP_DIR_NAME "tbudget"
#define DATA_DIR_NAME "data_storage"
#define DATA_FILE_NAME "tbudget.dat"
#define EXPORT_FILE_NAME "tbudget_export.csv"

// Structures
typedef struct
{
    char name[MAX_NAME_LEN];
    double budget;
    double spent;
    double extra;
} Category;

typedef struct
{
    bool expense;
    double amt;
    char desc[MAX_NAME_LEN];
    char cat_name[MAX_NAME_LEN];
    char date[11]; // Format: YYYY-MM-DD
} Transaction;

typedef struct
{
    char name[MAX_NAME_LEN];
    bool expense;                // true if it's an expense, false if income
    double amount;               // amount of money
    int period_type;             // PERIOD_WEEKLY, PERIOD_MONTHLY, PERIOD_YEARLY
    int period_day;              // 0-6 for weekly (Sun-Sat), 1-31 for monthly, 1-12 for yearly (month)
    int period_month_day;        // Only used for yearly (1-31 for day of month)
    char start_date[11];         // Format: YYYY-MM-DD
    char end_date[11];           // Format: YYYY-MM-DD
    int last_updated;            // Day of year (1-366)
    char cat_name[MAX_NAME_LEN]; // Category for the subscription
} Subscription;

// Global variables
extern Category categories[MAX_CATEGORIES];
extern int category_count;
extern Transaction transactions[MAX_TRANSACTIONS];
extern int transaction_count;
extern double total_budget;
extern Subscription subscriptions[MAX_SUBSCRIPTIONS];
extern int subscription_count;

// Global file paths
extern char app_data_dir[MAX_BUFFER];
extern char data_storage_dir[MAX_BUFFER];
extern char data_file_path[MAX_BUFFER];
extern char export_file_path[MAX_BUFFER];

#endif