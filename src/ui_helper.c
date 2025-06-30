#include "ui_helper.h"

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
  use_default_colors();

  // Set ESC delay programmatically (preferred method)
  set_escdelay(10); // Set to 10 milliseconds

  cbreak();             // Line buffering disabled
  noecho();             // Don't echo keystrokes
  keypad(stdscr, TRUE); // Enable special keys
  curs_set(0);          // Hide cursor during menu navigation
  start_color();        // Enable colors

  // Define color pairs
  init_pair(1, OVERRIDE_COLOR_BLUE, OVERRIDE_COLOR_WHITE);   // Title
  init_pair(2, OVERRIDE_COLOR_GREEN, OVERRIDE_COLOR_WHITE);  // Positive numbers
  init_pair(3, OVERRIDE_COLOR_RED, OVERRIDE_COLOR_WHITE);    // Negative numbers
  init_pair(4, OVERRIDE_COLOR_YELLOW, OVERRIDE_COLOR_WHITE); // Yellow numbers
  init_pair(5, OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_CYAN);   // Highlighted menu item

  // Enable blocking input (not non-blocking)
  nodelay(stdscr, FALSE);
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
    wattron(boundary, A_BOLD | COLOR_PAIR(5));
  }
  box(boundary, 0, 0);
  wattroff(boundary, A_BOLD | COLOR_PAIR(5));

  BoundedWindow result = {dialog, boundary, NULL, 0};

  // Allocate memory for children array
  result.children = (BoundedWindow **)malloc(sizeof(BoundedWindow *) * MAX_CHILD_WINDOWS);
  if (result.children == NULL)
  {
    // Handle memory allocation failure
    delwin(dialog);
    delwin(boundary);
    result.textbox = NULL;
    result.boundary = NULL;
  }

  return result;
}

BoundedWindow draw_bounded_with_title(int height, int width, int start_y, int start_x, const char *title, bool highlight, int alignment)
{
  WINDOW *dialog = newwin(height, width, start_y, start_x);
  WINDOW *boundary = newwin(height + 4, width + 4, start_y - 2, start_x - 2);
  if (highlight)
  {
    wattron(boundary, A_BOLD | COLOR_PAIR(5));
  }
  box(boundary, 0, 0);
  wattroff(boundary, A_BOLD | COLOR_PAIR(5));

  if (title != NULL && strlen(title) > 0)
  {
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
  }

  BoundedWindow result = {dialog, boundary, NULL, 0};

  // Allocate memory for children array
  result.children = (BoundedWindow **)malloc(sizeof(BoundedWindow *) * MAX_CHILD_WINDOWS);
  if (result.children == NULL)
  {
    // Handle memory allocation failure
    delwin(dialog);
    delwin(boundary);
    result.textbox = NULL;
    result.boundary = NULL;
  }

  return result;
}

void draw_title(WINDOW *win, const char *title)
{
  int width = getmaxx(win);
  wattron(win, A_BOLD | COLOR_PAIR(1));

  for (int i = 0; i < width; i++)
  {
    mvwaddch(win, 0, i, ' ');
  }

  mvwprintw(win, 0, (width - strlen(title)) / 2, "%s", title);
  wattroff(win, A_BOLD | COLOR_PAIR(1));
}

void draw_menu(WINDOW *win, int highlighted_item, const char *menu_items[], int item_count, int start_y)
{
  for (int i = 0; i < item_count; i++)
  {
    if (i == highlighted_item)
    {
      wattron(win, COLOR_PAIR(5));
      mvwprintw(win, start_y + i, 2, "%s", menu_items[i]);
      wattroff(win, COLOR_PAIR(5));
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
  wclear(win.textbox);
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
  // First, delete all child windows
  for (int i = 0; i < win.child_count; i++)
  {
    if (win.children[i] != NULL)
    {
      delete_bounded(*win.children[i]);
    }
  }

  // Free the children array
  if (win.children != NULL)
  {
    free(win.children);
  }

  // Delete the windows
  if (win.boundary != NULL)
  {
    delwin(win.boundary);
  }
  if (win.textbox != NULL)
  {
    delwin(win.textbox);
  }
  touchwin(stdscr);
  wnoutrefresh(stdscr);
}

void delete_bounded_array(BoundedWindow *windows[], int count)
{
  for (int i = 0; i < count; i++)
  {
    if (windows[i]->boundary != NULL && windows[i]->textbox != NULL)
    {
      delete_bounded(*windows[i]);
    }
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

  // Then refresh this window
  if (win.boundary != NULL)
  {
    wnoutrefresh(win.boundary);
  }
  if (win.textbox != NULL)
  {
    wnoutrefresh(win.textbox);
  }

  // First refresh all child windows (bottom-up)
  for (int i = 0; i < win.child_count; i++)
  {
    if (win.children[i] != NULL)
    {
      bwnoutrefresh(*win.children[i]);
    }
  }
}

void bwarrnoutrefresh(BoundedWindow *windows[], int count)
{
  for (int i = 0; i < count; i++)
  {
    if (windows[i]->textbox != NULL && windows[i]->boundary != NULL)
    {
      bwnoutrefresh(*windows[i]);
    }
  }
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

    int ch = getch();

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

int get_scrollable_menu_choice(WINDOW *win, char *prompt, const char *menu_items[], int item_count, int max_visible_items, int start_y, bool show_numbers)
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
          wattron(win, COLOR_PAIR(5));
        }

        // Print the row
        wmove(win, start_y + 2 + i - start_index, 0);
        wclrtoeol(win);
        if (show_numbers)
        {
          wprintw(win, "%d. ", i + 1);
        }
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
    case KEY_BACKSPACE_ALT:
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

// New function to create a bounded window with initialization
BoundedWindow *create_bounded_window()
{
  BoundedWindow *win = (BoundedWindow *)malloc(sizeof(BoundedWindow));
  if (win == NULL)
  {
    return NULL; // Memory allocation failed
  }

  win->textbox = NULL;
  win->boundary = NULL;
  win->children = (BoundedWindow **)malloc(sizeof(BoundedWindow *) * MAX_CHILD_WINDOWS);
  win->child_count = 0;

  if (win->children == NULL)
  {
    free(win);
    return NULL; // Memory allocation failed
  }

  return win;
}

// Add a child window to a parent window
void add_child_window(BoundedWindow *parent, BoundedWindow *child)
{
  if (parent == NULL || child == NULL || parent->child_count >= MAX_CHILD_WINDOWS)
  {
    return;
  }

  // Allocate memory for the child and copy it
  BoundedWindow *new_child = (BoundedWindow *)malloc(sizeof(BoundedWindow));
  if (new_child == NULL)
  {
    return; // Memory allocation failed
  }

  // Copy the child window
  *new_child = *child;

  // Add it to the parent's children
  parent->children[parent->child_count++] = new_child;
}

// Recursively delete a bounded window and all its children
void delete_bounded_with_children(BoundedWindow *win)
{
  if (win == NULL)
  {
    return;
  }

  // First delete all child windows (bottom-up)
  for (int i = 0; i < win->child_count; i++)
  {
    delete_bounded_with_children(win->children[i]);
  }

  // Free the children array
  if (win->children != NULL)
  {
    free(win->children);
  }

  // Delete the ncurses windows
  if (win->boundary != NULL)
  {
    delwin(win->boundary);
  }
  if (win->textbox != NULL)
  {
    delwin(win->textbox);
  }

  // Free the window structure itself
  free(win);
}

// Refresh a window and all its children (bottom-up)
void bw_hierarchy_refresh(BoundedWindow win)
{
  // First refresh all child windows (bottom-up)
  for (int i = 0; i < win.child_count; i++)
  {
    if (win.children[i] != NULL)
    {
      bw_hierarchy_refresh(*win.children[i]);
    }
  }

  // Then refresh this window
  if (win.textbox != NULL && win.boundary != NULL)
  {
    wnoutrefresh(win.textbox);
    wnoutrefresh(win.boundary);
  }
}