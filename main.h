#ifndef MAIN_H
#define MAIN_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

// Constants
#define MAX_CATEGORIES 20
#define MAX_NAME_LEN 50
#define MAX_TRANSACTIONS 10000000 // you are probably not extremely wealthy so this should be enough
#define MAX_BUFFER 1024

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
    double amount;
} Category;

typedef struct
{
    char description[MAX_NAME_LEN];
    double amount;
    int category_id;
    char date[11]; // Format: YYYY-MM-DD
} Transaction;

// Global variables
extern Category categories[MAX_CATEGORIES];
extern int category_count;
extern Transaction transactions[MAX_TRANSACTIONS];
extern int transaction_count;
extern double total_budget;

// Global file paths
extern char app_data_dir[MAX_BUFFER];
extern char data_storage_dir[MAX_BUFFER];
extern char data_file_path[MAX_BUFFER];
extern char export_file_path[MAX_BUFFER];

// Function prototypes
void budget_setup();
void manage_transactions();
void run_menu_mode();
void run_dashboard_mode();
void print_usage(const char *program_name);

// Dashboard mode helper functions
void add_category_dialog();
void remove_category_dialog();
void set_budget_dialog();
void add_transaction_dialog();
void remove_transaction_dialog();

#endif // MAIN_H 