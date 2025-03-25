#include "ui.h"

void setup_ncurses()
{
  // Set ESC key delay to 10ms through environment variable
  // This must be done BEFORE initscr() is called
  char *esc_delay = getenv("ESCDELAY");
  if (esc_delay == NULL)
  {
    // If ESCDELAY is not already set in the environment, set it to 10ms
    setenv("ESCDELAY", "10", 0);
  }

  initscr(); // Initialize ncurses

  // Set ESC delay programmatically (preferred method)
  set_escdelay(10); // Set to 10 milliseconds

  cbreak();             // Line buffering disabled
  noecho();             // Don't echo keystrokes
  keypad(stdscr, TRUE); // Enable special keys
  curs_set(0);          // Hide cursor during menu navigation
  start_color();        // Enable colors

  // Define color pairs
  init_pair(1, COLOR_WHITE, COLOR_BLUE);  // Title
  init_pair(2, COLOR_GREEN, COLOR_BLACK); // Positive numbers
  init_pair(3, COLOR_RED, COLOR_BLACK);   // Negative numbers
  init_pair(4, COLOR_BLACK, COLOR_CYAN);  // Highlighted menu item

  // Enable blocking input (not non-blocking)
  nodelay(stdscr, FALSE);

  // To reduce flicker, set these options
  intrflush(stdscr, FALSE);
}

void cleanup_ncurses()
{
  endwin(); // End ncurses mode
}

void draw_title(WINDOW *win, const char *title)
{
  int width = getmaxx(win);
  wattron(win, COLOR_PAIR(1));

  for (int i = 0; i < width; i++)
  {
    mvwaddch(win, 0, i, ' ');
  }

  mvwprintw(win, 0, (width - strlen(title)) / 2, "%s", title);
  wattroff(win, COLOR_PAIR(1));
}

void draw_menu(WINDOW *win, int highlighted_item, const char *menu_items[], int item_count, int start_y)
{
  for (int i = 0; i < item_count; i++)
  {
    if (i == highlighted_item)
    {
      wattron(win, COLOR_PAIR(4));
      mvwprintw(win, start_y + i, 2, "%s", menu_items[i]);
      wattroff(win, COLOR_PAIR(4));
    }
    else
    {
      mvwprintw(win, start_y + i, 2, "%s", menu_items[i]);
    }
  }
  wnoutrefresh(win);
}

int get_menu_choice(WINDOW *win, const char *menu_items[], int item_count, int start_y, int allow_back)
{
  int highlighted_item = 0;
  int ch;

  // Enable keypad mode for this window to properly detect arrow keys
  keypad(win, TRUE);

  draw_menu(win, highlighted_item, menu_items, item_count, start_y);
  wrefresh(win);

  while (1)
  {
    ch = wgetch(win);

    // Handle numeric key presses (1-9)
    if (ch >= '1' && ch <= '9')
    {
      int choice = ch - '1';
      if (choice < item_count)
      {
        return choice;
      }
    }

    // Handle arrow keys and enter
    switch (ch)
    {
    case KEY_UP:
      if (highlighted_item > 0)
      {
        highlighted_item--;
      }
      break;
    case KEY_DOWN:
      if (highlighted_item < item_count - 1)
      {
        highlighted_item++;
      }
      break;
    case KEY_LEFT:
    case KEY_RIGHT:
      // Ignore left/right arrow keys in menus
      break;
    case '\n': // Enter key
      return highlighted_item;
    case KEY_BACKSPACE:
    case KEY_BACKSPACE_ALT: // This is already 127 from the main.h define
      if (allow_back)
      {
        return -1; // Signal to go back
      }
      break;
    case 27: // ESC key (ASCII value)
      if (allow_back)
      {
        return -1; // Signal to go back
      }
      break;
    }

    draw_menu(win, highlighted_item, menu_items, item_count, start_y);
    wrefresh(win);
  }
}

int get_string_input(WINDOW *win, char *buffer, int max_len, int y, int x)
{
  int ch;
  int pos = 0;
  int cursor_x = x;

  // Enable keypad mode for this window to properly detect arrow keys
  keypad(win, TRUE);

  // Turn on cursor
  curs_set(1);

  // Calculate absolute positions within the dialog window
  wmove(win, y, x);

  // Get current cursor position in window
  int cursor_y, temp_x;
  getyx(win, cursor_y, temp_x);

  // Clear the input area
  for (int i = 0; i < max_len; i++)
  {
    mvwaddch(win, cursor_y, x + i, ' ');
  }

  // If buffer already has content, display it
  if (buffer[0] != '\0')
  {
    mvwprintw(win, cursor_y, x, "%s", buffer);
    pos = strlen(buffer);
    cursor_x = x + pos;
  }

  wmove(win, cursor_y, cursor_x);
  wrefresh(win);

  while (1)
  {
    ch = wgetch(win);

    if (ch == '\n')
    {
      buffer[pos] = '\0';
      break;
    }
    else if (ch == 27)
    { // ASCII value for ESC key
      // Always treat as a true escape when in string input
      curs_set(0);
      return 0; // Cancel
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT)
    { // Handle backspace or delete
      if (pos > 0)
      {
        pos--;
        cursor_x--;

        // Shift characters to the left
        for (int i = pos; i < (int)strlen(buffer); i++)
        {
          buffer[i] = buffer[i + 1];
        }

        // Redraw the input field
        wmove(win, cursor_y, x);
        for (int i = 0; i < max_len; i++)
        {
          waddch(win, ' ');
        }
        mvwprintw(win, cursor_y, x, "%s", buffer);
        wmove(win, cursor_y, cursor_x);
      }
    }
    else if (ch == KEY_LEFT)
    {
      if (pos > 0)
      {
        pos--;
        cursor_x--;
        wmove(win, cursor_y, cursor_x);
      }
    }
    else if (ch == KEY_RIGHT)
    {
      if (pos < (int)strlen(buffer))
      {
        pos++;
        cursor_x++;
        wmove(win, cursor_y, cursor_x);
      }
    }
    else if (ch == KEY_UP || ch == KEY_DOWN)
    {
      // Ignore up/down arrow keys in text input
    }
    else if (isprint(ch) && pos < max_len - 1)
    {
      // Shift characters to the right
      for (int i = strlen(buffer); i >= pos; i--)
      {
        buffer[i + 1] = buffer[i];
      }

      buffer[pos] = ch;
      pos++;
      cursor_x++;

      // Redraw the input field
      wmove(win, cursor_y, x);
      for (int i = 0; i < max_len; i++)
      {
        waddch(win, ' ');
      }
      mvwprintw(win, cursor_y, x, "%s", buffer);
      wmove(win, cursor_y, cursor_x);
    }

    wrefresh(win);
  }

  curs_set(0);
  return 1;
}

// Function to get numeric input for a double value
double get_double_input(WINDOW *win, int y, int x)
{
  char buffer[MAX_BUFFER] = {0}; // Buffer for user input
  int pos = 0;                   // Current cursor position in buffer

  // Turn on cursor
  curs_set(1);

  // If we already have a value, display it
  wmove(win, y, x);
  wrefresh(win);

  noecho(); // Don't automatically echo characters

  while (1)
  {
    int ch = wgetch(win);

    if (ch == '\n')
    {
      buffer[pos] = '\0';
      break;
    }
    else if (ch == 27)
    { // Direct ESC key detection
      curs_set(0);
      return -1.0; // Signal cancellation
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT)
    {
      if (pos > 0)
      {
        pos--;
        mvwaddch(win, y, x + pos, ' ');
        wmove(win, y, x + pos);
        wrefresh(win);

        // Shift characters to the left
        for (int i = pos; i < (int)strlen(buffer); i++)
        {
          buffer[i] = buffer[i + 1];
        }

        // Redraw the input
        wmove(win, y, x);
        wclrtoeol(win);
        mvwprintw(win, y, x, "%s", buffer);
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
    else if (ch == KEY_LEFT)
    {
      if (pos > 0)
      {
        pos--;
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
    else if (ch == KEY_RIGHT)
    {
      if (pos < (int)strlen(buffer))
      {
        pos++;
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
    else if ((isdigit(ch) || ch == '.' || ch == '-') && pos < MAX_BUFFER - 1)
    {
      // Validate input (only allow one decimal point and ensure minus sign is at start)
      bool valid = true;

      if (ch == '.' && strchr(buffer, '.') != NULL)
      {
        valid = false; // Don't allow multiple decimal points
      }

      if (ch == '-' && pos != 0)
      {
        valid = false; // Minus sign only at beginning
      }

      if (valid)
      {
        // Shift characters to the right
        for (int i = strlen(buffer); i >= pos; i--)
        {
          buffer[i + 1] = buffer[i];
        }

        buffer[pos] = ch;
        pos++;

        // Redraw the input
        wmove(win, y, x);
        wclrtoeol(win);
        mvwprintw(win, y, x, "%s", buffer);
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
  }

  // Convert to double
  double value = -1.0; // Default to -1 if conversion fails
  if (buffer[0] != '\0')
  {
    value = atof(buffer);
  }

  curs_set(0);
  return value;
}

// Function to get integer input
int get_int_input(WINDOW *win, int y, int x)
{
  char buffer[MAX_BUFFER] = {0}; // Buffer for user input
  int pos = 0;                   // Current cursor position in buffer

  // Turn on cursor
  curs_set(1);

  // If we already have a value, display it
  wmove(win, y, x);
  wrefresh(win);

  noecho(); // Don't automatically echo characters

  while (1)
  {
    int ch = wgetch(win);

    if (ch == '\n')
    {
      buffer[pos] = '\0';
      break;
    }
    else if (ch == 27)
    { // Direct ESC key detection
      curs_set(0);
      return -1; // Signal cancellation
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT)
    {
      if (pos > 0)
      {
        pos--;
        mvwaddch(win, y, x + pos, ' ');
        wmove(win, y, x + pos);
        wrefresh(win);

        // Shift characters to the left
        for (int i = pos; i < (int)strlen(buffer); i++)
        {
          buffer[i] = buffer[i + 1];
        }

        // Redraw the input
        wmove(win, y, x);
        wclrtoeol(win);
        mvwprintw(win, y, x, "%s", buffer);
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
    else if (ch == KEY_LEFT)
    {
      if (pos > 0)
      {
        pos--;
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
    else if (ch == KEY_RIGHT)
    {
      if (pos < (int)strlen(buffer))
      {
        pos++;
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
    else if ((isdigit(ch) || ch == '-') && pos < MAX_BUFFER - 1)
    {
      // Validate input (only allow minus sign at start)
      bool valid = true;

      if (ch == '-' && pos != 0)
      {
        valid = false; // Minus sign only at beginning
      }

      if (valid)
      {
        // Shift characters to the right
        for (int i = strlen(buffer); i >= pos; i--)
        {
          buffer[i + 1] = buffer[i];
        }

        buffer[pos] = ch;
        pos++;

        // Redraw the input
        wmove(win, y, x);
        wclrtoeol(win);
        mvwprintw(win, y, x, "%s", buffer);
        wmove(win, y, x + pos);
        wrefresh(win);
      }
    }
  }

  // Convert to int
  int value = -1; // Default to -1 if conversion fails
  if (buffer[0] != '\0')
  {
    value = atoi(buffer);
  }

  curs_set(0);
  return value;
}

// Function to get date input with improved UX
int get_date_input(WINDOW *win, char *date_buffer, int y, int x)
{
  int ch;
  int highlighted_field = 0;           // 0 = day, 1 = month, 2 = year, 3 = "Today" button
  int cursor_positions[3] = {0, 3, 6}; // Positions for DD-MM-YYYY

  // Enable keypad mode for this window to properly detect arrow keys
  keypad(win, TRUE);

  // Date components
  int day = 1;
  int month = 1;
  int year = 2025;

  // Pre-fill with today's date
  time_t now = time(NULL);
  struct tm *today = localtime(&now);
  day = today->tm_mday;
  month = today->tm_mon + 1; // tm_mon is 0-11
  year = today->tm_year + 1900;

  // If date_buffer contains a valid date, use it instead
  if (strlen(date_buffer) >= 10)
  {
    // Convert from YYYY-MM-DD to day, month, year
    char temp[5];
    strncpy(temp, date_buffer, 4);
    temp[4] = '\0';
    year = atoi(temp);

    strncpy(temp, date_buffer + 5, 2);
    temp[2] = '\0';
    month = atoi(temp);

    strncpy(temp, date_buffer + 8, 2);
    temp[2] = '\0';
    day = atoi(temp);
  }
  else
  {
    // If we have a last transaction, use its date
    if (transaction_count > 0)
    {
      char temp[5];
      strncpy(temp, transactions[transaction_count - 1].date, 4);
      temp[4] = '\0';
      year = atoi(temp);

      strncpy(temp, transactions[transaction_count - 1].date + 5, 2);
      temp[2] = '\0';
      month = atoi(temp);

      strncpy(temp, transactions[transaction_count - 1].date + 8, 2);
      temp[2] = '\0';
      day = atoi(temp);
    }
  }

  // Save original cursor state to restore later
  int original_cursor_state = curs_set(1); // Show cursor and save original state

  // Initial display
  format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
  wnoutrefresh(win);
  doupdate();

  // Process input
  while (1)
  {
    ch = wgetch(win);

    if (ch == '\n')
    {
      // If "Today" button is selected, set to today's date
      if (highlighted_field == 3)
      {
        time_t now = time(NULL);
        struct tm *today = localtime(&now);
        day = today->tm_mday;
        month = today->tm_mon + 1;
        year = today->tm_year + 1900;

        // Show updated date
        format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
        wnoutrefresh(win);
        doupdate();

        // Short delay to show the change
        napms(300);

        // Set the date buffer in YYYY-MM-DD format
        sprintf(date_buffer, "%04d-%02d-%02d", year, month, day);

        // Restore original cursor state
        curs_set(original_cursor_state);
        return 1;
      }
      else
      {
        // User is done editing, convert to YYYY-MM-DD format
        sprintf(date_buffer, "%04d-%02d-%02d", year, month, day);

        // Restore original cursor state
        curs_set(original_cursor_state);
        return 1;
      }
    }
    else if (ch == 27)
    { // ASCII value for ESC key
      // Always treat as a true escape in date input
      curs_set(original_cursor_state);
      return 0;
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT)
    {
      // User wants to cancel input
      curs_set(original_cursor_state);
      return 0;
    }
    else if (ch == KEY_RIGHT)
    {
      // Move to next field
      highlighted_field = (highlighted_field + 1) % 4;
      format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
      wnoutrefresh(win);
      doupdate();
    }
    else if (ch == KEY_LEFT)
    {
      // Move to previous field
      highlighted_field = (highlighted_field + 3) % 4;
      format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
      wnoutrefresh(win);
      doupdate();
    }
    else if (ch == KEY_UP)
    {
      // Increase current field value
      if (highlighted_field == 0)
      {
        day++;
        if (day > get_days_in_month(month, year))
        {
          day = 1;
        }
      }
      else if (highlighted_field == 1)
      {
        month++;
        if (month > 12)
        {
          month = 1;
        }
        day = validate_day(day, month, year);
      }
      else if (highlighted_field == 2)
      {
        year++;
        day = validate_day(day, month, year);
      }
      // No action for "Today" button

      format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
      wnoutrefresh(win);
      doupdate();
    }
    else if (ch == KEY_DOWN)
    {
      // Decrease current field value
      if (highlighted_field == 0)
      {
        day--;
        if (day < 1)
        {
          day = get_days_in_month(month, year);
        }
      }
      else if (highlighted_field == 1)
      {
        month--;
        if (month < 1)
        {
          month = 12;
        }
        day = validate_day(day, month, year);
      }
      else if (highlighted_field == 2)
      {
        year--;
        day = validate_day(day, month, year);
      }
      // No action for "Today" button

      format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
      wnoutrefresh(win);
      doupdate();
    }
    else if (isdigit(ch) && highlighted_field < 3)
    {
      // Handle direct digit input for each field
      int digit = ch - '0';

      if (highlighted_field == 0)
      { // Day field
        day = day % 10 * 10 + digit;
        if (day > get_days_in_month(month, year))
        {
          day = digit;
        }
        if (day == 0)
        {
          day = digit;
        }
      }
      else if (highlighted_field == 1)
      { // Month field
        month = month % 10 * 10 + digit;
        if (month > 12)
        {
          month = digit;
        }
        if (month == 0)
        {
          month = digit;
        }
        day = validate_day(day, month, year);
      }
      else if (highlighted_field == 2)
      { // Year field
        year = year % 1000 * 10 + digit;
        day = validate_day(day, month, year);
      }

      format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
      wnoutrefresh(win);
      doupdate();
    }
  }
}

void display_categories(WINDOW *win, int start_y)
{
  int y = start_y;

  mvwprintw(win, y++, 2, "%-30s %-15s %-15s", "Category", "Amount", "Percentage");
  mvwprintw(win, y++, 2, "-------------------------------------------------------");

  double total_allocated = 0.0;
  for (int i = 0; i < category_count; i++)
  {
    total_allocated += categories[i].amount;
  }

  for (int i = 0; i < category_count; i++)
  {
    double percentage = 0.0;
    if (total_budget > 0)
    {
      percentage = (categories[i].amount / total_budget) * 100.0;
    }

    mvwprintw(win, y++, 2, "%-30s $%-14.2f %.2f%%",
              categories[i].name, categories[i].amount, percentage);
  }

  mvwprintw(win, y++, 2, "-------------------------------------------------------");
  mvwprintw(win, y++, 2, "%-30s $%-14.2f %.2f%%",
            "Total Allocated", total_allocated,
            total_budget > 0 ? (total_allocated / total_budget) * 100.0 : 0.0);

  double remaining = total_budget - total_allocated;
  if (remaining >= 0)
  {
    wattron(win, COLOR_PAIR(2));
  }
  else
  {
    wattron(win, COLOR_PAIR(3));
  }

  mvwprintw(win, y++, 2, "%-30s $%-14.2f %.2f%%",
            "Remaining Unallocated", remaining,
            total_budget > 0 ? (remaining / total_budget) * 100.0 : 0.0);

  if (remaining >= 0)
  {
    wattroff(win, COLOR_PAIR(2));
  }
  else
  {
    wattroff(win, COLOR_PAIR(3));
  }
}

void display_transactions(WINDOW *win, int start_y)
{
  int y = start_y;

  // First sort transactions by date (most recent first)
  // Simple bubble sort since we don't expect a huge number of transactions
  for (int i = 0; i < transaction_count - 1; i++)
  {
    for (int j = 0; j < transaction_count - i - 1; j++)
    {
      // Compare dates (format is YYYY-MM-DD so string comparison works)
      if (strcmp(transactions[j].date, transactions[j + 1].date) < 0)
      {
        // Swap transactions
        Transaction temp = transactions[j];
        transactions[j] = transactions[j + 1];
        transactions[j + 1] = temp;
      }
    }
  }

  mvwprintw(win, y++, 2, "%-10s %-24s %-10s %-24s",
            "Date", "Description", "Amount", "Category");
  mvwprintw(win, y++, 2, "-----------------------------------------------------------------------");

  char prev_date[11] = "";

  for (int i = 0; i < transaction_count; i++)
  {
    char category_name[MAX_NAME_LEN] = "Uncategorized";

    if (transactions[i].category_id >= 0 && transactions[i].category_id < category_count)
    {
      strcpy(category_name, categories[transactions[i].category_id].name);
    }

    // Format date for display
    char display_date[11];

    // If this date is the same as the previous one, use blank space
    if (strcmp(transactions[i].date, prev_date) == 0)
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

    mvwprintw(win, y++, 2, "%-10s %-24s $%-9.2f %-24s",
              display_date, transactions[i].description, transactions[i].amount, category_name);
  }
}

// Helper function to format and display the date
void format_date(WINDOW *win, int y, int x, int day, int month, int year,
                        int highlighted_field, int cursor_positions[])
{
  // Ensure day is valid for current month/year
  day = validate_day(day, month, year);

  // Create date string in DD-MM-YYYY format for display
  char date_str[11];
  sprintf(date_str, "%02d-%02d-%04d", day, month, year);

  // Display the date string with the current field highlighted
  for (int i = 0; i < 10; i++)
  {
    if ((i < 2 && highlighted_field == 0) ||
        (i > 2 && i < 5 && highlighted_field == 1) ||
        (i > 5 && highlighted_field == 2))
    {
      wattron(win, COLOR_PAIR(4)); // Highlight current field
    }

    mvwaddch(win, y, x + i, date_str[i]);

    if ((i < 2 && highlighted_field == 0) ||
        (i > 2 && i < 5 && highlighted_field == 1) ||
        (i > 5 && highlighted_field == 2))
    {
      wattroff(win, COLOR_PAIR(4));
    }
  }

  // Draw "Today" button
  if (highlighted_field == 3)
  {
    wattron(win, COLOR_PAIR(4));
  }
  mvwprintw(win, y, x + 12, "[Today]");
  if (highlighted_field == 3)
  {
    wattroff(win, COLOR_PAIR(4));
  }

  // Position cursor at the current field
  if (highlighted_field < 3)
  {
    wmove(win, y, x + cursor_positions[highlighted_field]);
  }
  else
  {
    wmove(win, y, x + 12); // "Today" button
  }
}