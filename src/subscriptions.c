#include "globals.h"
#include "subscriptions.h"
#include <time.h>
#include <string.h>

// Helper function to check if a date string is before another date string
bool is_date_before(const char *date1, const char *date2)
{
  return strcmp(date1, date2) < 0;
}

// Helper function to check if a date string is after another date string
bool is_date_after(const char *date1, const char *date2)
{
  return strcmp(date1, date2) > 0;
}

// Helper function to get today's date as a string (YYYY-MM-DD)
static void get_today_date(char *date_str)
{
  time_t now = time(NULL);
  struct tm *today = localtime(&now);
  sprintf(date_str, "%04d-%02d-%02d",
          today->tm_year + 1900,
          today->tm_mon + 1,
          today->tm_mday);
}

// Helper function to add days to a date string
static void add_days_to_date(const char *start_date, int days, char *result_date)
{
  struct tm date = {0};
  sscanf(start_date, "%d-%d-%d", &date.tm_year, &date.tm_mon, &date.tm_mday);
  date.tm_year -= 1900; // Adjust year
  date.tm_mon -= 1;     // Adjust month (0-11)

  time_t time_stamp = mktime(&date);
  time_stamp += days * 24 * 60 * 60; // Add days in seconds

  struct tm *new_date = localtime(&time_stamp);
  sprintf(result_date, "%04d-%02d-%02d",
          new_date->tm_year + 1900,
          new_date->tm_mon + 1,
          new_date->tm_mday);
}

// Helper function to get the next occurrence of a weekday after a given date
static void get_next_weekday(const char *start_date, int target_weekday, char *result_date)
{
  struct tm date = {0};
  sscanf(start_date, "%d-%d-%d", &date.tm_year, &date.tm_mon, &date.tm_mday);
  date.tm_year -= 1900;
  date.tm_mon -= 1;

  time_t time_stamp = mktime(&date);
  struct tm *curr_date = localtime(&time_stamp);

  int days_to_add = (target_weekday - curr_date->tm_wday + 7) % 7;
  if (days_to_add == 0)
    days_to_add = 7; // If today is the target day, go to next week

  time_stamp += days_to_add * 24 * 60 * 60;
  curr_date = localtime(&time_stamp);

  sprintf(result_date, "%04d-%02d-%02d",
          curr_date->tm_year + 1900,
          curr_date->tm_mon + 1,
          curr_date->tm_mday);
}

// Helper function to get the next occurrence of a monthly date
static void get_next_monthly(const char *start_date, int target_day, char *result_date)
{
  struct tm date = {0};
  sscanf(start_date, "%d-%d-%d", &date.tm_year, &date.tm_mon, &date.tm_mday);
  date.tm_year -= 1900;
  date.tm_mon -= 1;

  // Move to next month if we're past the target day
  if (date.tm_mday > target_day)
  {
    date.tm_mon++;
    if (date.tm_mon > 11)
    {
      date.tm_mon = 0;
      date.tm_year++;
    }
  }

  // Set the target day, adjusting for month length
  date.tm_mday = target_day;
  mktime(&date); // This will normalize the date if target_day is too large

  sprintf(result_date, "%04d-%02d-%02d",
          date.tm_year + 1900,
          date.tm_mon + 1,
          date.tm_mday);
}

// Helper function to get the next occurrence of a yearly date
static void get_next_yearly(const char *start_date, int target_month, int target_day, char *result_date)
{
  struct tm date = {0};
  sscanf(start_date, "%d-%d-%d", &date.tm_year, &date.tm_mon, &date.tm_mday);
  date.tm_year -= 1900;
  date.tm_mon = target_month - 1;
  date.tm_mday = target_day;

  // If we're past this year's date, move to next year
  time_t now = time(NULL);
  struct tm *today = localtime(&now);
  if (today->tm_mon > target_month - 1 ||
      (today->tm_mon == target_month - 1 && today->tm_mday > target_day))
  {
    date.tm_year = today->tm_year + 1;
  }
  else
  {
    date.tm_year = today->tm_year;
  }

  mktime(&date); // Normalize the date

  sprintf(result_date, "%04d-%02d-%02d",
          date.tm_year + 1900,
          date.tm_mon + 1,
          date.tm_mday);
}

void update_subscription(int index)
{
  time_t now = time(NULL);
  struct tm *today = localtime(&now);
  char today_date[11];
  get_today_date(today_date);
  // Skip if subscription hasn't started yet
  if (is_date_after(subscriptions[index].start_date, today_date))
    return;

  // Skip if subscription has ended
  if (is_date_before(subscriptions[index].end_date, today_date))
    return;

  // Check if we need to create new transactions
  char next_date[11];
  bool should_create = false;

  switch (subscriptions[index].period_type)
  {
    case PERIOD_WEEKLY:
      get_next_weekday(subscriptions[index].start_date, subscriptions[index].period_day, next_date);
      if (is_date_before(next_date, today_date))
        should_create = true;
      break;

    case PERIOD_MONTHLY:
      get_next_monthly(subscriptions[index].start_date, subscriptions[index].period_day, next_date);
      if (is_date_before(next_date, today_date))
        should_create = true;
      break;

    case PERIOD_YEARLY:
      get_next_yearly(subscriptions[index].start_date, subscriptions[index].period_day,
                    subscriptions[index].period_month_day, next_date);
      if (is_date_before(next_date, today_date))
        should_create = true;
      break;

    case PERIOD_CUSTOM_DAYS:
      add_days_to_date(subscriptions[index].start_date, subscriptions[index].period_day, next_date);
      if (is_date_before(next_date, today_date))
        should_create = true;
      break;
  }

  if (should_create && transaction_count < MAX_TRANSACTIONS)
  {
    // Create new transaction
    Transaction new_trans;
    new_trans.expense = subscriptions[index].expense;
    new_trans.amt = subscriptions[index].amount;
    strcpy(new_trans.desc, subscriptions[index].name);
    strcpy(new_trans.cat_name, subscriptions[index].cat_name);
    strcpy(new_trans.date, next_date);

    // Insert transaction in correct position (most recent first)
    int insert_pos = 0;
    while (insert_pos < transaction_count &&
           strcmp(transactions[insert_pos].date, new_trans.date) > 0)
    {
      insert_pos++;
    }

    // Shift existing transactions
    if (insert_pos < transaction_count)
    {
      for (int j = transaction_count; j > insert_pos; j--)
      {
        transactions[j] = transactions[j - 1];
      }
    }

    // Insert new transaction
    transactions[insert_pos] = new_trans;
    transaction_count++;

    // Update subscription's last_updated
    subscriptions[index].last_updated = today->tm_yday + 1;

    // Update start_date to next occurrence
    strcpy(subscriptions[index].start_date, next_date);
  }
}

// Function to update subscriptions and create transactions
void update_subscriptions()
{
  for (int i = 0; i < subscription_count; i++)
  {
    update_subscription(i);
  }
}
