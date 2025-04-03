#ifndef SUBSCRIPTIONS_H
#define SUBSCRIPTIONS_H

#include <time.h>
#include <string.h>

#include "globals.h"
#include "transaction_manager.h"

// Function to update subscriptions and create transactions
bool is_date_before(const char *date1, const char *date2);
bool is_date_after(const char *date1, const char *date2);
void update_subscription(int index); 
void update_subscriptions();

#endif 