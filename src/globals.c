#include "globals.h"

// Define global variables
Category categories[MAX_CATEGORIES];
int category_count = 0;
Transaction transactions[MAX_TRANSACTIONS];
int transaction_count = 0;
double total_budget = 0.0;

// Global file paths
char app_data_dir[MAX_BUFFER];
char data_storage_dir[MAX_BUFFER];
char data_file_path[MAX_BUFFER];
char export_file_path[MAX_BUFFER]; 