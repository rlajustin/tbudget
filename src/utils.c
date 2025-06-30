#include "utils.h"

int compare_transactions_by_date(const void *a, const void *b)
{
    const TransactionNode *node_a = *(const TransactionNode **)a;
    const TransactionNode *node_b = *(const TransactionNode **)b;
    return strcmp(node_b->data.date, node_a->data.date);
}

// Helper function to get days in a month
int get_days_in_month(int m, int y)
{
    // Array of days in each month
    static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
        return 29; // Leap year
    return days_in_month[m];
}

// Helper function to validate day value
int validate_day(int day, int month, int year)
{
    int max_days = get_days_in_month(month, year);
    if (day > max_days)
    {
        return 1; // Wrap around to 1
    }
    else if (day < 1)
    {
        return max_days; // Wrap to max days
    }
    return day;
}

void sort_categories_by_budget()
{
    double max_budget = 0.0;
    for (int i = 0; i < category_count; i++)
    {
        for (int j = 0; j < MAX_CATEGORIES; j++)
        {
            if (categories[j].budget > max_budget)
            {

                sorted_categories_indices[i] = j;
                max_budget = categories[j].budget;
            }
        }
        max_budget = 0.0;
    }
}

void cleanup_transactions()
{
    TransactionNode *current = transaction_head;
    while (current != NULL)
    {
        TransactionNode *next = current->next;
        free(current);
        current = next;
    }
    transaction_head = NULL;
    transaction_tail = NULL;
}

#ifdef __APPLE__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_macos_notification(const char *title, const char *message)
{
    // Build the AppleScript command to show a notification
    char script[2048];
    snprintf(script, sizeof(script),
             "osascript -e 'display notification \"%s\" with title \"%s\"'",
             message, title);

    // Execute the AppleScript command
    system(script);
}
#endif

int get_month_from_date(const char *date)
{
    // Expects date in format YYYY-MM-DD
    char month_str[3];
    month_str[0] = date[5];
    month_str[1] = date[6];
    month_str[2] = '\0';
    return atoi(month_str);
}

int get_year_from_date(const char *date)
{
    // Expects date in format YYYY-MM-DD
    char year_str[5];
    year_str[0] = date[0];
    year_str[1] = date[1];
    year_str[2] = date[2];
    year_str[3] = date[3];
    year_str[4] = '\0';
    return atoi(year_str);
}
