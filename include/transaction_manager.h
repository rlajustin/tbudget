#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "utils.h"

// Initialize/cleanup
void init_month_index_manager(void);
void free_month_index_manager(void);

// Month index management
void add_month_index(int year, int month, long file_position, int transaction_count);
MonthIndex* find_month_index(int year, int month);

// Transaction management
bool load_month_transactions(int year, int month);
bool add_transaction(Transaction *new_transaction);
void sort_current_month_transactions(void);
int compare_transactions_by_date(const void *a, const void *b);

// Navigation
bool navigate_month(int direction);  // direction: 1 for next, -1 for previous

#endif // TRANSACTION_MANAGER_H 