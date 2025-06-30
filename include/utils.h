#ifndef UTILS_H
#define UTILS_H

#include "globals.h"
#include "saveload.h"

int get_days_in_month(int m, int y);
int validate_day(int day, int month, int year);
void sort_categories_by_budget();
int compare_transactions_by_date(const void *a, const void *b);
void cleanup_transactions();

// macOS notification utility for debugging
#ifdef __APPLE__
void show_macos_notification(const char *title, const char *message);
#endif

int get_month_from_date(const char *date);
int get_year_from_date(const char *date);

#endif // UTILS_H