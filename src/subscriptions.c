#include "subscriptions.h"

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
  int cat_index = -1, cat_index_month = -1, cat_index_year = -1;

  // Skip if subscription hasn't started yet
  if (is_date_after(subscriptions[index].start_date, today_date))
    return;

  // Skip if subscription has ended
  if (is_date_before(subscriptions[index].end_date, today_date))
    return;

  // Convert last_updated to string for comparison
  char last_updated_date[11];
  sprintf(last_updated_date, "%04d-%02d-%02d",
          subscriptions[index].last_updated.tm_year + 1900,
          subscriptions[index].last_updated.tm_mon + 1,
          subscriptions[index].last_updated.tm_mday);

  // Use last_updated as our iterator
  char date_iterator[11];
  strcpy(date_iterator, last_updated_date);

  // Keep track of next occurrence date
  char next_date[11];
  WINDOW *win = stdscr;
  int max_y, max_x;
  getmaxyx(win, max_y, max_x);

  // Keep the variable height calculation for transactions
  int max_display = 10;
  int display_count = current_month_transaction_count < max_display ? current_month_transaction_count : max_display;
  int dialog_height = display_count > 1 ? display_count + 8 : 10;
  int dialog_width = DEFAULT_DIALOG_WIDTH;
  int start_y = (max_y - dialog_height) / 2;
  int start_x = (max_x - dialog_width) / 2;

  // Iterate until we catch up to today
  while (is_date_before(date_iterator, today_date))
  {
    // Calculate next occurrence based on period type
    switch (subscriptions[index].period_type)
    {
    case PERIOD_WEEKLY:
      get_next_weekday(date_iterator, subscriptions[index].period_day, next_date);
      break;

    case PERIOD_MONTHLY:
      get_next_monthly(date_iterator, subscriptions[index].period_day, next_date);
      break;

    case PERIOD_YEARLY:
      get_next_yearly(date_iterator, subscriptions[index].period_month_day,
                      subscriptions[index].period_day, next_date);
      break;

    case PERIOD_CUSTOM_DAYS:
      add_days_to_date(date_iterator, subscriptions[index].period_day, next_date);
      break;
    }

    // If next occurrence is after today, we're done
    if (is_date_after(next_date, today_date))
      break;

    Transaction new_trans = {
        .expense = subscriptions[index].expense,
        .amt = subscriptions[index].amount,
        .cat_index = cat_index};
    strcpy(new_trans.desc, subscriptions[index].name);
    strcpy(new_trans.date, next_date);
    new_trans.date[sizeof(new_trans.date) - 1] = '\0';
    int month = get_month_from_date(new_trans.date);
    int year = get_year_from_date(new_trans.date);
    if (cat_index_month != month || cat_index_year != year)
    {
      cat_index_month = month;
      cat_index_year = year;
      cat_index = get_category_index(year, month, subscriptions[index].cat_name);
    }
    if (cat_index == -1)
    {
      BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Updating Subscriptions", false, ALIGN_CENTER);
      wnoutrefresh(dialog.boundary);
      cat_index = get_category_choice_subscription(dialog.textbox, year, month, subscriptions[index].name, subscriptions[index].cat_name);
    }

    add_transaction(&new_trans, year, month);

    // Update date_iterator to next occurrence for next iteration
    strcpy(date_iterator, next_date);
  }

  // Update subscription's last_updated to today
  subscriptions[index].last_updated = *today;
}

// Function to update subscriptions and create transactions
void update_subscriptions()
{
  for (int i = 0; i < subscription_count; i++)
  {
    update_subscription(i);
  }
}
