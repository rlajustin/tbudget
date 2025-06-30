#ifndef ACTIONS_H
#define ACTIONS_H

#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "ui.h"
#include "globals.h"
#include "saveload.h"

// Dashboard mode helper functions
void add_category_dialog();
void remove_category_dialog();
void set_budget_dialog();
void add_expense_dialog();
void remove_transaction_dialog();
void add_subscription_dialog();
void remove_subscription_dialog(int selected_subscription);

// Dashboard mode dialogs
void budget_summary_dialog();

#endif