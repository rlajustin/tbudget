#include "ui.h"

int get_transaction_choice(WINDOW *win, const Transaction transactions[], int transaction_count, int max_visible_items) // optimized for case of many transactions
{
  int start_index = 0;
  int visible_items = max_visible_items;
  int current_highlighted = 0;
  bool redraw = true;

  keypad(win, TRUE); // Enable arrow keys

  mvwprintw(win, 3, 0, "%-10s %-24s %-9s %-24s",
            "Date", "Description", "Amount", "Category");

  while (1)
  {
    if (redraw)
    {
      // Add scroll indicators if needed
      if (start_index > 0)
      {
        mvwprintw(win, 3, getmaxx(win) - 3, "^");
      }
      else
      {
        mvwprintw(win, 3, getmaxx(win) - 3, " ");
      }

      if (start_index + visible_items < transaction_count)
      {
        mvwprintw(win, 4 + visible_items, getmaxx(win) - 3, "v");
      }
      else
      {
        mvwprintw(win, 4 + visible_items, getmaxx(win) - 3, " ");
      }

      char prev_date[11] = "";
      for (int i = start_index; i < start_index + visible_items && i < transaction_count; i++)
      {
        // Clear the entire line first
        wmove(win, 4 + i - start_index, 0);
        wclrtoeol(win);

        char row_item[MAX_NAME_LEN + 50] = "";

        char category_name[MAX_NAME_LEN] = "Uncategorized";
        strcpy(category_name, transactions[i].cat_name);

        // Format date for display
        char display_date[11] = "";

        // If this date is the same as the previous one, use blank space
        if (strcmp(transactions[i].date, prev_date) == 0 && i != current_highlighted)
        {
          strcpy(display_date, "          ");
        }
        else
        {
          // Format YYYY-MM-DD to MM-DD-YYYY for display
          if (strlen(transactions[i].date) == 10)
          {
            sprintf(display_date, "%c%c-%c%c-%c%c%c%c",
                    transactions[i].date[5], transactions[i].date[6], // Month
                    transactions[i].date[8], transactions[i].date[9], // Day
                    transactions[i].date[0], transactions[i].date[1], // Year
                    transactions[i].date[2], transactions[i].date[3]);
          }
          else
          {
            strcpy(display_date, transactions[i].date); // Use as is if format is unexpected
          }

          // Remember this date for the next iteration
          strcpy(prev_date, transactions[i].date);
        }

        // Create a descriptive menu item
        char desc[24] = "";
        if (strlen(transactions[i].desc) > 23)
        {
          strncpy(desc, transactions[i].desc, 20);
          desc[20] = '\0';
          strcat(desc, "...");
        }
        else
        {
          strcpy(desc, transactions[i].desc);
        }

        sprintf(row_item, "%-10s %-24s $%-8.2f %-24s",
                display_date,
                desc,
                transactions[i].amt,
                category_name);

        // Apply highlighting before printing if this is the current item
        if (i == current_highlighted)
        {
          wattron(win, COLOR_PAIR(5));
        }

        // Print the row
        wprintw(win, "%s", row_item);
        // Turn off highlighting after printing
        if (i == current_highlighted)
        {
          wattroff(win, COLOR_PAIR(4));
        }
      }

      wrefresh(win);
      redraw = false;
    }

    int ch = wgetch(win);

    switch (ch)
    {
    case KEY_UP:
      if (current_highlighted > 0)
      {
        current_highlighted--;
        if (current_highlighted < start_index)
        {
          start_index = current_highlighted;
        }
        redraw = true;
      }
      continue;

    case KEY_DOWN:
      if (current_highlighted < transaction_count - 1)
      {
        current_highlighted++;
        if (current_highlighted >= start_index + visible_items)
        {
          start_index = current_highlighted - visible_items + 1;
        }
        redraw = true;
      }
      continue;

    case '\n': // Enter key - proceed to confirmation
      return current_highlighted;

    case 27: // ASCII ESC key
      return -1;

    case KEY_BACKSPACE:
#if KEY_BACKSPACE_ALT != 127 // Only include this case if the constants are different
    case KEY_BACKSPACE_ALT:
#endif
    case 127: // Backspace alternative
      return -1;
    }
  }
  return -1; // @dev this should never happen
}

void display_categories(WINDOW *win, int start_y)
{
  int y = start_y;

  mvwprintw(win, y++, 2, "%-30s %-15s %-15s", "Category", "Spent", "Budget");
  mvwprintw(win, y++, 2, "-------------------------------------------------------------------");

  double total_allocated = 0.0;
  double total_spent = 0.0;

  for (int i = 0; i < category_count; i++)
  {
    total_allocated += categories[i].budget;
    total_spent += categories[i].spent;

    char name[MAX_NAME_LEN];
    if (strlen(categories[i].name) > 29)
    {
      strncpy(name, categories[i].name, 26);
      name[26] = '\0';
      strcat(name, "...");
    }
    else
    {
      strcpy(name, categories[i].name);
    }

    // Display color block for pie chart legend
    if (categories[i].budget > 0)
    {
      int color_pair = PIE_COLOR_START + (i >= NUM_PIE_COLORS - 1 ? NUM_PIE_COLORS - 2 : i);
      wattron(win, COLOR_PAIR(color_pair));
      mvwprintw(win, y, 2, "  ");
      wattroff(win, COLOR_PAIR(color_pair));
      mvwprintw(win, y, 4, " %-27s", name);
      if (categories[i].spent > categories[i].budget)
      {
        wattron(win, COLOR_PAIR(3));
      }
      else if (categories[i].spent < 0.5 * categories[i].budget)
      {
        wattron(win, COLOR_PAIR(2));
      }
      else
      {
        wattron(win, COLOR_PAIR(1));
      }
      mvwprintw(win, y, 32, " $%-14.2f", categories[i].spent);
      wattroff(win, COLOR_PAIR(3) | COLOR_PAIR(2) | COLOR_PAIR(1));
      mvwprintw(win, y++, 48, " $%-14.2f", categories[i].budget);
    }
    else
    {
      mvwprintw(win, y++, 2, "%-30s $%-14.2f $%-14.2f",
                name, categories[i].spent, categories[i].budget);
    }
  }
  mvwprintw(win, y++, 2, "-------------------------------------------------------------------");

  if (total_allocated < total_budget)
  {
    double savings = total_budget - total_allocated;
    double savings_percent = (savings / total_budget) * 100.0;
    mvwprintw(win, y, 2, "[]");
    mvwprintw(win, y++, 4, " %-27s %-15s $%.2f (%.2f%%)",
              "Savings", " ", savings, savings_percent);
  }
  else
  {
    wattron(win, COLOR_PAIR(3));
    double overspent = total_spent - total_budget;
    double overspent_percent = (overspent / total_budget) * 100.0;
    mvwprintw(win, y++, 2, "%-30s $%.2f (%.2f%%)",
              "Overspent", overspent, overspent_percent);
    wattroff(win, COLOR_PAIR(3));
  }
  mvwprintw(win, y++, 2, "%-30s $%-14.2f $%-14.2f",
            "Total", total_spent, total_allocated);
}

void display_transactions(WINDOW *win, int start_y, int selected_transaction, int *first_display_transaction, bool highlight_selected)
{
  if (current_month_transaction_count == 0)
  {
    mvwprintw(win, 1, 2, "No transactions recorded yet.");
    mvwprintw(win, 2, 2, "Select 'Add Transaction' from the Actions menu.");
  }
  int y = start_y;
  int max_y, max_x;
  getmaxyx(win, max_y, max_x);

  // Calculate how many transactions we can display
  int displayable_rows = max_y - start_y - 3; // Account for header and separator
  if (displayable_rows < 1)
    displayable_rows = 1;

  // Ensure selected transaction is within valid range
  if (selected_transaction < 0)
    selected_transaction = 0;
  if (selected_transaction >= current_month_transaction_count)
    selected_transaction = current_month_transaction_count - 1;

  // Adjust first_display_transaction if necessary to keep selected transaction visible
  if (selected_transaction < *first_display_transaction)
    *first_display_transaction = selected_transaction;
  else if (selected_transaction >= *first_display_transaction + displayable_rows)
    *first_display_transaction = selected_transaction - displayable_rows + 1;

  // Ensure first_display_transaction is within valid range
  if (*first_display_transaction < 0)
    *first_display_transaction = 0;
  if (current_month_transaction_count > 0 && *first_display_transaction >= current_month_transaction_count)
    *first_display_transaction = current_month_transaction_count - 1;

  // Display headers
  mvwprintw(win, y++, 2, "%-10s %-24s %-10s %-24s",
            "Date", "Description", "Amount", "Category");
  mvwprintw(win, y++, 2, "-----------------------------------------------------------------------");

  char prev_date[11] = "";

  // Display scroll indicators if needed
  if (*first_display_transaction > 0)
    mvwprintw(win, y - 1, max_x - 3, "^");

  if (*first_display_transaction + displayable_rows < current_month_transaction_count)
    mvwprintw(win, y + displayable_rows, max_x - 3, "v");

  // Sort transactions before display
  sort_current_month_transactions();

  // Display visible transactions
  int last_display = *first_display_transaction + displayable_rows;
  if (last_display > current_month_transaction_count)
    last_display = current_month_transaction_count;

  for (int i = *first_display_transaction; i < last_display; i++)
  {
    char category_name[MAX_NAME_LEN] = "Uncategorized";
    strcpy(category_name, current_month_transactions[i].cat_name);

    // Format date for display
    char display_date[11];

    // If this date is the same as the previous one, use blank space
    // But always show date for selected transaction
    if (strcmp(current_month_transactions[i].date, prev_date) == 0 && i != selected_transaction)
    {
      strcpy(display_date, "          ");
    }
    else
    {
      // Format YYYY-MM-DD to MM-DD-YYYY for display
      if (strlen(current_month_transactions[i].date) == 10)
      {
        sprintf(display_date, "%c%c-%c%c-%c%c%c%c",
                current_month_transactions[i].date[5], current_month_transactions[i].date[6], // Month
                current_month_transactions[i].date[8], current_month_transactions[i].date[9], // Day
                current_month_transactions[i].date[0], current_month_transactions[i].date[1], // Year
                current_month_transactions[i].date[2], current_month_transactions[i].date[3]);
      }
      else
      {
        strcpy(display_date, current_month_transactions[i].date); // Use as is if format is unexpected
      }

      // Remember this date for the next iteration
      strcpy(prev_date, current_month_transactions[i].date);
    }

    // Highlight selected transaction
    if (i == selected_transaction && highlight_selected)
      wattron(win, COLOR_PAIR(5));

    mvwprintw(win, y++, 2, "%-10s %-24s $%-9.2f %-24s",
              display_date,
              current_month_transactions[i].desc,
              current_month_transactions[i].amt,
              category_name);

    if (i == selected_transaction && highlight_selected)
      wattroff(win, COLOR_PAIR(5));
  }
}

BoundedWindow draw_bar_chart(WINDOW *parent_win)
{
  double total_spent = 0.0;
  double total_budget_allocated = 0.0;

  int y, x, start_y, start_x;
  getmaxyx(parent_win, y, x);
  getbegyx(parent_win, start_y, start_x);

  // First calculate totals and prepare data
  for (int i = 0; i < category_count; i++)
  {
    total_spent += categories[i].spent;
    total_budget_allocated += categories[i].budget;
  }

  int bar_width = x - 20; // Leave some margin

  if (bar_width < 20)
  {
    bar_width = 20; // Minimum width for visibility
  }

  BoundedWindow bar_win = draw_bounded(1, bar_width, start_y + y - 2, start_x + 10, false);

  // Title for bar chart
  mvwprintw(parent_win, y - 5, 1, "Spending Distribution");

  // Sort categories by spent amount for the bar chart (highest to lowest)
  typedef struct
  {
    int index;
    double pct;
  } SpendingCategory;

  SpendingCategory sorted_cats[MAX_CATEGORIES];
  int num_active_cats = 0;

  // Fill array with categories that have spending
  for (int i = 0; i < category_count; i++)
  {
    if (categories[i].spent > 0)
    {
      sorted_cats[num_active_cats].index = i;
      sorted_cats[num_active_cats].pct = categories[i].spent / total_budget_allocated;
      num_active_cats++;
    }
  }

  // Sort by spent (bubble sort - simple but sufficient for small arrays)
  for (int i = 0; i < num_active_cats - 1; i++)
  {
    for (int j = 0; j < num_active_cats - i - 1; j++)
    {
      if (sorted_cats[j].pct < sorted_cats[j + 1].pct)
      {
        SpendingCategory temp = sorted_cats[j];
        sorted_cats[j] = sorted_cats[j + 1];
        sorted_cats[j + 1] = temp;
      }
    }
  }

  // Now draw the colored sections representing each category's portion
  int current_pos = 0;
  double drawn_pct = 0.0;
  char usage_str[50];
  sprintf(usage_str, " $%.2f/$%.2f (%.2f%%)", total_spent, total_budget_allocated, (total_spent / total_budget_allocated) * 100.0);
  int len = strlen(usage_str);

  for (int i = 0; i < num_active_cats; i++)
  {
    drawn_pct += sorted_cats[i].pct;
    int cat_index = sorted_cats[i].index;
    int color_pair = PIE_COLOR_START + (cat_index >= NUM_PIE_COLORS ? NUM_PIE_COLORS - 1 : cat_index);

    wattron(bar_win.textbox, COLOR_PAIR(color_pair));
    while (current_pos < bar_width * drawn_pct)
    {
      char ch = current_pos < len ? usage_str[current_pos] : ' ';
      mvwaddch(bar_win.textbox, 0, current_pos, ch);
      current_pos++;
    }
    wattroff(bar_win.textbox, COLOR_PAIR(color_pair));
  }
  while (current_pos < bar_width)
  {
    char ch = current_pos < len ? usage_str[current_pos] : ' ';
    mvwaddch(bar_win.textbox, 0, current_pos, ch);
    current_pos++;
  }
  return bar_win;
}

// New function to create a bar chart and add it as a child window
BoundedWindow *create_bar_chart(BoundedWindow *parent)
{
  if (parent == NULL || parent->textbox == NULL)
  {
    return NULL;
  }

  // Create the bar chart
  BoundedWindow bar_win = draw_bar_chart(parent->textbox);

  // Add it as a child window to the parent
  if (bar_win.textbox != NULL && bar_win.boundary != NULL)
  {
    add_child_window(parent, &bar_win);

    // Return a pointer to the child that was added
    return parent->children[parent->child_count - 1];
  }

  return NULL;
}

void display_subscriptions(WINDOW *win, int start_y, int selected_subscription, int *first_display_subscription, bool active)
{
  char today_date[11];
  time_t now = time(NULL);
  struct tm *today = localtime(&now);
  sprintf(today_date, "%04d-%02d-%02d",
          today->tm_year + 1900,
          today->tm_mon + 1,
          today->tm_mday);

  int row = 1;
  mvwprintw(win, row++, start_y, "Active Subscriptions:");
  row++;

  int max_y, max_x;
  getmaxyx(win, max_y, max_x);

  int num_rows = max_y - start_y;
  if (!active)
  {
    num_rows--;
  }

  if (selected_subscription > *first_display_subscription + num_rows)
  {
    *first_display_subscription = selected_subscription - num_rows + 1;
  }
  else if (*first_display_subscription > selected_subscription)
  {
    *first_display_subscription = selected_subscription;
  }

  if (*first_display_subscription < 0)
    *first_display_subscription = 0;
  if (subscription_count > 0 && *first_display_subscription >= subscription_count)
    *first_display_subscription = subscription_count - 1;

  if (*first_display_subscription > 0)
    mvwprintw(win, row - 1, max_x - 3, "^");

  if (*first_display_subscription + num_rows < subscription_count)
    mvwprintw(win, max_y - 1, max_x - 3, "v");

  int y = row;

  for (int i = *first_display_subscription; i < subscription_count && i < *first_display_subscription + num_rows; i++)
  {
    wmove(win, y, 0);
    int x = 0;
    // if (i != selected_subscription)
    // {
    //   int color_pair = PIE_COLOR_START + (i >= NUM_PIE_COLORS - 1 ? NUM_PIE_COLORS - 2 : i);
    //   wattron(win, COLOR_PAIR(color_pair));
    //   wprintw(win, "  ");
    //   wattroff(win, COLOR_PAIR(color_pair));
    //   wmove(win, y, x + 5);
    //   char desc[50] = {0};
    //   strcpy(desc, subscriptions[i].name);
    //   strcat(desc, ": $");
    //   char amt[100] = {0};
    //   sprintf(amt, "%.2f ", subscriptions[i].amount);
    //   strcat(desc, amt);
    //   char period[50] = {0};
    //   switch (subscriptions[i].period_type)
    //   {
    //   case PERIOD_WEEKLY:
    //     sprintf(period, "weekly");
    //     break;
    //   case PERIOD_MONTHLY:
    //     sprintf(period, "monthly");
    //     break;
    //   case PERIOD_YEARLY:
    //     sprintf(period, "yearly");
    //     break;
    //   case PERIOD_CUSTOM_DAYS:
    //     sprintf(period, "every %d days", subscriptions[i].period_day);
    //     break;
    //   }
    //   strcat(desc, period);
    //   wprintw(win, desc);
    // }
    char details[30] = {0};
    wmove(win, y, x);
    int color_pair = PIE_COLOR_START + (i >= NUM_PIE_COLORS - 1 ? NUM_PIE_COLORS - 2 : i);
    wattron(win, COLOR_PAIR(color_pair));
    wprintw(win, "  ");
    wattroff(win, COLOR_PAIR(color_pair));
    wmove(win, y, x + 2);
    if (i == selected_subscription)
    {
      wprintw(win, " > ");
    }
    char row1[100] = {0};
    char row2[100] = {0};

    strcpy(row1, trunc_str(subscriptions[i].name, 20));
    strcat(row1, " (");
    strcat(row1, trunc_str(subscriptions[i].cat_name, 20));
    strcat(row1, "): ");
    char date_str[11];
    sprintf(date_str, "%c%c-%c%c-%c%c%c%c", subscriptions[i].start_date[5], subscriptions[i].start_date[6], subscriptions[i].start_date[8], subscriptions[i].start_date[9], subscriptions[i].start_date[0], subscriptions[i].start_date[1], subscriptions[i].start_date[2], subscriptions[i].start_date[3]);
    strcat(row1, date_str);
    strcat(row1, " - ");
    if (strcmp(subscriptions[i].end_date, "9999-12-31") == 0)
    {
      strcat(row1, "N/A");
    }
    else
    {
      sprintf(date_str, "%c%c-%c%c-%c%c%c%c", subscriptions[i].end_date[5], subscriptions[i].end_date[6], subscriptions[i].end_date[8], subscriptions[i].end_date[9], subscriptions[i].end_date[0], subscriptions[i].end_date[1], subscriptions[i].end_date[2], subscriptions[i].end_date[3]);
      strcat(row1, date_str);
    }
    char amt[50] = {0};
    if (subscriptions[i].expense)
    {
      sprintf(amt, "$%.2f", subscriptions[i].amount);
    }
    else
    {
      sprintf(amt, "+$%.2f", subscriptions[i].amount);
    }
    strcat(row2, amt);
    switch (subscriptions[i].period_type)
    {
    case PERIOD_WEEKLY:
    {
      sprintf(details, ", weekly on %s", days_in_week[subscriptions[i].period_day]);
      break;
    }
    case PERIOD_MONTHLY:
      sprintf(details, ", monthly on the %d%s", subscriptions[i].period_day, subscriptions[i].period_day == 1 ? "st" : subscriptions[i].period_day == 2 ? "nd"
                                                                                                                   : subscriptions[i].period_day == 3   ? "rd"
                                                                                                                                                        : "th");
      break;
    case PERIOD_YEARLY:
      sprintf(details, ", yearly on %d/%d", subscriptions[i].period_month_day, subscriptions[i].period_day);
      break;
    case PERIOD_CUSTOM_DAYS:
      if (subscriptions[i].period_day == 1)
      {
        sprintf(details, ", daily");
      }
      else
      {
        sprintf(details, ", every %d days", subscriptions[i].period_day);
      }
      break;
    }
    strcat(row2, details);
    int row_len = getmaxx(win);
    wmove(win, y, 5);
    wprintw(win, "%s", trunc_str(row1, row_len - 5));
    wmove(win, y + 1, 5);
    wprintw(win, "%s", trunc_str(row2, row_len));
    y += 2;
  }
}