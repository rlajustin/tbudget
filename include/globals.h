#ifndef GLOBALS_H
#define GLOBALS_H

#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
#include "flex_layout.h"

// Constants
#define NUM_CONSTANTS 2
#define MAX_BUFFER 1024
#define DEFAULT_DIALOG_HEIGHT 10
#define DEFAULT_DIALOG_WIDTH 70
#define MAX_SUBSCRIPTIONS 1024

// must be constant for savefiles
#define MAX_CATEGORIES 32
#define MAX_NAME_LEN 32

// Period types for subscriptions
#define PERIOD_WEEKLY 0
#define PERIOD_MONTHLY 1
#define PERIOD_YEARLY 2
#define PERIOD_CUSTOM_DAYS 3

// Window definitions
#define ACTIONS_MENU_WINDOW 0
#define BUDGET_SUMMARY_WINDOW 1
#define BUDGET_BREAKDOWN_WINDOW 2
#define TRANSACTION_HISTORY_WINDOW 3
#define SUBSCRIPTIONS_WINDOW 4
#define TODO_WINDOW 5

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static inline char *trunc_str(const char *str, size_t len)
{
    static char tempbuf[MAX_BUFFER];
    strncpy(tempbuf, str, len - 3);
    tempbuf[len - 3] = '.';
    tempbuf[len - 2] = '.';
    tempbuf[len - 1] = '.';
    tempbuf[len] = '\0';
    return tempbuf;
}

// Key definitions
// #define KEY_ESCAPE 27

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
    double budget;
    double spent;
    double extra;
    char name[MAX_NAME_LEN];
} Category;

typedef struct
{
    bool expense;
    double amt;
    int cat_index;
    char desc[MAX_NAME_LEN];
    char date[11]; // Format: YYYY-MM-DD
} Transaction;

typedef struct
{
    char name[MAX_NAME_LEN];
    bool expense;         // true if it's an expense, false if income
    double amount;        // amount of money
    int period_type;      // PERIOD_WEEKLY, PERIOD_MONTHLY, PERIOD_YEARLY
    int period_day;       // 0-6 for weekly (Sun-Sat), 1-31 for monthly, 1-12 for yearly (month)
    int period_month_day; // Only used for yearly (1-31 for day of month)
    char start_date[11];  // Format: YYYY-MM-DD
    char end_date[11];    // Format: YYYY-MM-DD
    struct tm last_updated;
    char cat_name[MAX_NAME_LEN]; // Category for the subscription
} Subscription;

typedef struct TransactionNode
{
    Transaction data;
    struct TransactionNode *next;
    struct TransactionNode *prev;
    int index; // index of the transaction in the LL
} TransactionNode;

// Global variables from data file (loaded by init)
extern Subscription subscriptions[MAX_SUBSCRIPTIONS];
extern int subscription_count;
extern double default_monthly_budget;
extern Category default_categories[MAX_CATEGORIES];
extern int default_category_count;

// Global variables dependent on current month (loaded by load_month)
extern int category_count;
extern Category categories[MAX_CATEGORIES];
extern int sorted_categories_indices[MAX_CATEGORIES];
extern double uncategorized_spent;
extern double current_month_total_budget;

// Month management
extern int today_month;
extern int today_year;
extern int current_month;
extern int current_year;
extern int loaded_month;
extern int loaded_year;

// Global file paths
extern char app_data_dir[MAX_BUFFER];
extern char data_storage_dir[MAX_BUFFER];
extern char export_file_path[MAX_BUFFER];
extern char data_file_path[MAX_BUFFER];

extern const short pie_colors[][2];
extern const short pie_colors_darker[][2];

extern TransactionNode *transaction_head;
extern TransactionNode *transaction_tail;
extern TransactionNode **sorted_transactions; // array of pointers to transactions, sorted (this is cursed)
extern int current_month_transaction_count;

static const char days_in_week[][10] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char month_names[][10] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"};

extern FlexContainer *main_layout;

#endif