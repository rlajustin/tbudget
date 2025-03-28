#include "globals.h"
#include "ui.h"
#include "piechart.h" // Include piechart.h for pie chart color constants
#include <string.h>
#include <time.h>
#include <math.h>

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

BoundedWindow draw_bounded(int height, int width, int start_y, int start_x, bool highlight)
{
  WINDOW *dialog = newwin(height, width, start_y, start_x);
  WINDOW *boundary = newwin(height + 2, width + 2, start_y - 1, start_x - 1);
  if (highlight)
  {
    wattron(boundary, A_BOLD | COLOR_PAIR(4));
  }
  box(boundary, 0, 0);
  wattroff(boundary, A_BOLD | COLOR_PAIR(4));

  BoundedWindow result = {dialog, boundary};
  return result;
}

BoundedWindow draw_bounded_with_title(int height, int width, int start_y, int start_x, const char *title, bool highlight, int alignment)
{
  WINDOW *dialog = newwin(height, width, start_y, start_x);
  WINDOW *boundary = newwin(height + 4, width + 4, start_y - 2, start_x - 2);
  if (highlight)
  {
    wattron(boundary, A_BOLD | COLOR_PAIR(4));
  }
  box(boundary, 0, 0);
  wattroff(boundary, A_BOLD | COLOR_PAIR(4));

  if (alignment == ALIGN_CENTER)
  {
    mvwprintw(boundary, 0, (width - strlen(title)) / 2, " %s ", title);
  }
  else if (alignment == ALIGN_LEFT)
  {
    mvwprintw(boundary, 0, 5, " %s ", title);
  }
  else if (alignment == ALIGN_RIGHT)
  {
    mvwprintw(boundary, 0, width - strlen(title) - 5, " %s ", title);
  }

  BoundedWindow result = {dialog, boundary};
  return result;
}

void draw_title(WINDOW *win, const char *title)
{
  int width = getmaxx(win);
  wattron(win, A_BOLD | COLOR_PAIR(3));

  for (int i = 0; i < width; i++)
  {
    mvwaddch(win, 0, i, ' ');
  }

  mvwprintw(win, 0, (width - strlen(title)) / 2, "%s", title);
  wattroff(win, A_BOLD | COLOR_PAIR(3));
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

BoundedWindow draw_alert(const char *title, const char *message[], int message_count)
{
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  int size = message_count < 3 ? 5 : message_count + 2;
  BoundedWindow alert = draw_bounded_with_title(size, 70, max_y / 2 - (size / 2), max_x / 2 - 35, title, false, ALIGN_CENTER);
  for (int i = 0; i < message_count; i++)
  {
    mvwprintw(alert.textbox, 1 + i, 0, "%s", message[i]);
  }
  wnoutrefresh(alert.textbox);
  doupdate();
  return alert;
}

BoundedWindow draw_alert_persistent(const char *title, const char *message[], int message_count)
{
  const char *full_message[message_count + 1];
  memcpy(full_message, message, message_count * sizeof(char *));
  full_message[message_count] = "Press any key to continue...";
  BoundedWindow alert = draw_alert(title, full_message, message_count + 1);
  wnoutrefresh(alert.textbox);
  doupdate();
  getch();
  return alert;
}
void draw_error(BoundedWindow win, const char *message)
{
  mvwprintw(win.textbox, 1, 0, "%s", message);
  mvwprintw(win.textbox, 2, 0, "Press any key to continue...");
  wnoutrefresh(win.textbox);
  doupdate();
  getch();
  delwin(win.boundary);
  delwin(win.textbox);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();
}

// does not trigger refresh
void delete_bounded(BoundedWindow win)
{
  if (win.boundary != NULL)
  {
    delwin(win.boundary);
  }
  if (win.textbox != NULL)
  {
    delwin(win.textbox);
  }
}

void bwresize(BoundedWindow win, int height, int width)
{
  wresize(win.boundary, height, width);
  wresize(win.textbox, height - 2, width - 2);
}

void bwmove(BoundedWindow win, int start_y, int start_x)
{
  mvwin(win.boundary, start_y, start_x);
  mvwin(win.textbox, start_y + 1, start_x + 1);
}

void bwnoutrefresh(BoundedWindow win)
{
  wnoutrefresh(win.boundary);
  wnoutrefresh(win.textbox);
}

int get_confirmation(WINDOW *win, const char *message[], int item_count)
{

  const char *confirm_menu[] = {
      "Yes (y)",
      "No (n)"};

  // Ask for confirmation
  wclear(win);
  for (int i = 0; i < item_count; i++)
  {
    mvwprintw(win, i + 1, 0, "%s", message[i]);
  }
  int highlighted_item = 0;

  while (1)
  {
    draw_menu(win, highlighted_item, confirm_menu, 2, item_count + 2);
    wrefresh(win);

    int ch = wgetch(win);

    if (ch == 'y' || ch == 'Y')
    {
      return 0;
    }
    else if (ch == 'n' || ch == 'N' || ch == KEY_BACKSPACE || ch == KEY_BACKSPACE_ALT || ch == 27)
    {
      return 1;
    }
    else if (ch == KEY_DOWN || ch == KEY_UP || ch == '\t' || ch == KEY_BTAB)
    {
      highlighted_item = (highlighted_item + 1) % 2;
    }
    else if (ch == '\n')
    {
      return highlighted_item;
    }
  }
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

int get_scrollable_menu_choice(WINDOW *win, char *prompt, const char *menu_items[], int item_count, int max_visible_items, int start_y)
{
  int start_index = 0;
  int visible_items = max_visible_items;
  int current_highlighted = 0;
  bool redraw = true;

  keypad(win, TRUE); // Enable arrow keys

  mvwprintw(win, start_y, 0, "%s", prompt);

  char number_buffer[10] = {0};
  int buffer_pos = 0;
  time_t last_input_time = 0;

  while (1)
  {
    if (redraw)
    {
      // Add scroll indicators if needed
      if (start_index > 0)
      {
        mvwprintw(win, start_y + 1, getmaxx(win) - 3, "^");
      }
      else
      {
        mvwprintw(win, start_y + 1, getmaxx(win) - 3, " ");
      }
      if (start_index + visible_items < item_count)
      {
        mvwprintw(win, start_y + 2 + visible_items, getmaxx(win) - 3, "v");
      }
      else
      {
        mvwprintw(win, start_y + 2 + visible_items, getmaxx(win) - 3, " ");
      }

      for (int i = start_index; i < start_index + visible_items && i < item_count; i++)
      {
        // Apply highlighting before printing if this is the current item
        if (i == current_highlighted)
        {
          wattron(win, COLOR_PAIR(4));
        }

        // Print the row
        wmove(win, start_y + 2 + i - start_index, 0);
        wclrtoeol(win);
        wprintw(win, "%s", menu_items[i]);
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
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      time_t current_time = time(NULL);
      if (current_time - last_input_time > 1)
      { // 1 second timeout
        buffer_pos = 0;
        memset(number_buffer, 0, sizeof(number_buffer));
      }
      last_input_time = current_time;

      // Add digit to buffer
      if (buffer_pos < (int)sizeof(number_buffer) - 1)
      {
        number_buffer[buffer_pos++] = ch;
        number_buffer[buffer_pos] = '\0';
      }

      // Convert buffer to number and jump to that position
      int selected_index = atoi(number_buffer) - 1; // Convert to 0-based index
      if (selected_index >= item_count)
      {
        // flush buffer
        buffer_pos = 0;
        memset(number_buffer, 0, sizeof(number_buffer));
        number_buffer[buffer_pos++] = ch;
        number_buffer[buffer_pos] = '\0';
        selected_index = atoi(number_buffer) - 1;
      }

      // Validate the index
      if (selected_index >= 0 && selected_index < item_count)
      {
        current_highlighted = selected_index;

        // Adjust scroll position if needed
        if (current_highlighted < start_index)
        {
          start_index = current_highlighted;
        }
        else if (current_highlighted >= start_index + visible_items)
        {
          start_index = current_highlighted - visible_items + 1;
        }

        redraw = true;
      }
      continue;
    }
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
      else
      {
        current_highlighted = item_count - 1;
        start_index = current_highlighted - visible_items + 1;
        if (start_index < 0)
        {
          start_index = 0;
        }
        redraw = true;
      }
      continue;

    case KEY_DOWN:
      if (current_highlighted < item_count - 1)
      {
        current_highlighted++;
        if (current_highlighted >= start_index + visible_items)
        {
          start_index = current_highlighted - visible_items + 1;
        }
        redraw = true;
      }
      else
      {
        current_highlighted = 0;
        start_index = 0;
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
          wattron(win, COLOR_PAIR(4));
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

int get_input(WINDOW *win, void *value, char *prompt, int max_len, InputType type)
{
  int ch;
  int pos = 0;
  int y, x;
  getyx(win, y, x);
  wclrtoeol(win);
  mvwprintw(win, y, 0, "%s", prompt);
  x += strlen(prompt);
  char buffer[max_len];
  buffer[0] = '\0';

  // Enable keypad mode for this window to properly detect arrow keys
  keypad(win, TRUE);

  // Turn on cursor
  curs_set(1);

  mvwprintw(win, y, x, "%s", buffer);
  pos = strlen(buffer);
  int cursor_x = x + pos;

  wmove(win, y, cursor_x);

  while (1)
  {
    wrefresh(win);
    ch = wgetch(win);

    if (ch == '\n')
    {
      buffer[pos] = '\0';
      if (strlen(buffer) == 0)
      {
        curs_set(0);
        return 0;
      }
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
        wmove(win, y, x);
        wclrtoeol(win);
        mvwprintw(win, y, x, "%s", buffer);
        wmove(win, y, cursor_x);
      }
    }
    else if (ch == KEY_LEFT)
    {
      if (pos > 0)
      {
        pos--;
        cursor_x--;
        wmove(win, y, cursor_x);
      }
    }
    else if (ch == KEY_RIGHT)
    {
      if (pos < (int)strlen(buffer))
      {
        pos++;
        cursor_x++;
        wmove(win, y, cursor_x);
      }
    }
    else if (ch == KEY_UP || ch == KEY_DOWN)
    {
      // Ignore up/down arrow keys in text input
    }
    else if (isprint(ch) && pos < max_len - 1)
    {
      if (type == INPUT_DOUBLE)
      {
        if (isdigit(ch) || ch == '.')
        {
          if (ch == '.' && strchr(buffer, '.') != NULL)
          {
            continue;
          }

          if (ch == '-' && pos != 0)
          {
            continue; // Minus sign only at beginning
          }
        }
        else
        {
          continue;
        }
      }
      else if (type == INPUT_INT)
      {
        if (isdigit(ch))
        {
          if (ch == '-' && pos != 0)
          {
            continue; // Minus sign only at beginning
          }
        }
        else
        {
          continue;
        }
      }

      // Shift characters to the right
      for (int i = strlen(buffer); i >= pos; i--)
      {
        buffer[i + 1] = buffer[i];
      }

      buffer[pos] = ch;
      pos++;
      cursor_x++;

      // Redraw the input field
      wmove(win, y, x);
      wclrtoeol(win);
      mvwprintw(win, y, x, "%s", buffer);
      wmove(win, y, cursor_x);
    }
  }

  if (type == INPUT_DOUBLE)
  {
    double val = -1.0; // Default to -1 if conversion fails
    if (buffer[0] != '\0')
    {
      val = atof(buffer);
    }
    *(double *)value = val;
  }
  else if (type == INPUT_INT)
  {
    int val = -1; // Default to -1 if conversion fails
    if (buffer[0] != '\0')
    {
      val = atoi(buffer);
    }
    *(int *)value = val;
  }
  else
  {
    strcpy((char *)value, buffer);
  }

  wmove(win, y + 1, 0);
  wrefresh(win);

  curs_set(0);
  return 1;
}

// Function to get date input with improved UX
int get_date_input(WINDOW *win, char *date_buffer, char *prompt)
{
  int ch;
  int highlighted_field = 0;           // 0 = day, 1 = month, 2 = year, 3 = "Today" button
  int cursor_positions[3] = {0, 3, 6}; // Positions for DD-MM-YYYY
  int y, x;
  getyx(win, y, x);

  mvwprintw(win, y, x, "%s", prompt);
  x += strlen(prompt);

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
      mvwprintw(win, y++, 4, " %-27s $%-14.2f $%-14.2f",
                name, categories[i].spent, categories[i].budget);
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
    wattron(win, COLOR_PAIR(2));
    double savings = total_budget - total_allocated;
    double savings_percent = (savings / total_budget) * 100.0;
    wattron(win, COLOR_PAIR(PIE_COLOR_START + NUM_PIE_COLORS - 1));
    mvwprintw(win, y, 2, "[]");
    wattroff(win, COLOR_PAIR(PIE_COLOR_START + NUM_PIE_COLORS - 1));
    mvwprintw(win, y++, 4, " %-27s %-15s $%.2f (%.2f%%)",
              "Savings", " ", savings, savings_percent);
    wattroff(win, COLOR_PAIR(2));
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

    strcpy(category_name, transactions[i].cat_name);

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
              display_date, transactions[i].desc, transactions[i].amt, category_name);
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

BoundedWindow draw_bar_chart(WINDOW *win)
{
  double total_spent = 0.0;
  double total_budget_allocated = 0.0;

  int y, x, start_y, start_x;
  getmaxyx(win, y, x);
  getbegyx(win, start_y, start_x);

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

  // Skip if nothing spent
  if (total_spent <= 0)
  {
    mvwprintw(win, y - 5, 1, "No spending recorded yet");
    return bar_win;
  }

  // Title for bar chart
  mvwprintw(win, y - 5, 1, "Spending Distribution");

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
    int color_pair = PIE_DARKER_COLOR_START + (cat_index >= NUM_PIE_COLORS ? NUM_PIE_COLORS - 1 : cat_index);

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
