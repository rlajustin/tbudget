#include "utils.h"
#include "main.h" // For global variables and constants
#include <sys/time.h>
#include <stdlib.h> // For getenv, setenv

void setup_ncurses()
{
    // Set ESC key delay to 10ms through environment variable
    // This must be done BEFORE initscr() is called
    char *esc_delay = getenv("ESCDELAY");
    if (esc_delay == NULL) {
        // If ESCDELAY is not already set in the environment, set it to 10ms
        setenv("ESCDELAY", "10", 0);
    }
    
    initscr();            // Initialize ncurses
    
    // Set ESC delay programmatically (preferred method)
    set_escdelay(10);     // Set to 10 milliseconds
    
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
    for (int i = 0; i < max_len; i++) {
        mvwaddch(win, cursor_y, x + i, ' ');
    }
    
    // If buffer already has content, display it
    if (buffer[0] != '\0') {
        mvwprintw(win, cursor_y, x, "%s", buffer);
        pos = strlen(buffer);
        cursor_x = x + pos;
    }
    
    wmove(win, cursor_y, cursor_x);
    wrefresh(win);
    
    while (1) {
        ch = wgetch(win);
        
        if (ch == '\n') {
            buffer[pos] = '\0';
            break;
        } else if (ch == 27) { // ASCII value for ESC key
            // Always treat as a true escape when in string input
            curs_set(0);
            return 0; // Cancel
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT) { // Handle backspace or delete
            if (pos > 0) {
                pos--;
                cursor_x--;
                
                // Shift characters to the left
                for (int i = pos; i < (int)strlen(buffer); i++) {
                    buffer[i] = buffer[i + 1];
                }
                
                // Redraw the input field
                wmove(win, cursor_y, x);
                for (int i = 0; i < max_len; i++) {
                    waddch(win, ' ');
                }
                mvwprintw(win, cursor_y, x, "%s", buffer);
                wmove(win, cursor_y, cursor_x);
            }
        } else if (ch == KEY_LEFT) {
            if (pos > 0) {
                pos--;
                cursor_x--;
                wmove(win, cursor_y, cursor_x);
            }
        } else if (ch == KEY_RIGHT) {
            if (pos < (int)strlen(buffer)) {
                pos++;
                cursor_x++;
                wmove(win, cursor_y, cursor_x);
            }
        } else if (ch == KEY_UP || ch == KEY_DOWN) {
            // Ignore up/down arrow keys in text input
        } else if (isprint(ch) && pos < max_len - 1) {
            // Shift characters to the right
            for (int i = strlen(buffer); i >= pos; i--) {
                buffer[i + 1] = buffer[i];
            }
            
            buffer[pos] = ch;
            pos++;
            cursor_x++;
            
            // Redraw the input field
            wmove(win, cursor_y, x);
            for (int i = 0; i < max_len; i++) {
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
    int pos = 0;           // Current cursor position in buffer
    
    // Turn on cursor
    curs_set(1);
    
    // If we already have a value, display it
    wmove(win, y, x);
    wrefresh(win);
    
    noecho(); // Don't automatically echo characters
    
    while (1) {
        int ch = wgetch(win);
        
        if (ch == '\n') {
            buffer[pos] = '\0';
            break;
        } else if (ch == 27) { // Direct ESC key detection
            curs_set(0);
            return -1.0; // Signal cancellation
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT) {
            if (pos > 0) {
                pos--;
                mvwaddch(win, y, x + pos, ' ');
                wmove(win, y, x + pos);
                wrefresh(win);
                
                // Shift characters to the left
                for (int i = pos; i < (int)strlen(buffer); i++) {
                    buffer[i] = buffer[i + 1];
                }
                
                // Redraw the input
                wmove(win, y, x);
                wclrtoeol(win);
                mvwprintw(win, y, x, "%s", buffer);
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if (ch == KEY_LEFT) {
            if (pos > 0) {
                pos--;
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if (ch == KEY_RIGHT) {
            if (pos < (int)strlen(buffer)) {
                pos++;
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if ((isdigit(ch) || ch == '.' || ch == '-') && pos < MAX_BUFFER - 1) {
            // Validate input (only allow one decimal point and ensure minus sign is at start)
            bool valid = true;
            
            if (ch == '.' && strchr(buffer, '.') != NULL) {
                valid = false; // Don't allow multiple decimal points
            }
            
            if (ch == '-' && pos != 0) {
                valid = false; // Minus sign only at beginning
            }
            
            if (valid) {
                // Shift characters to the right
                for (int i = strlen(buffer); i >= pos; i--) {
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
    if (buffer[0] != '\0') {
        value = atof(buffer);
    }
    
    curs_set(0);
    return value;
}

// Function to get integer input
int get_int_input(WINDOW *win, int y, int x)
{
    char buffer[MAX_BUFFER] = {0}; // Buffer for user input
    int pos = 0;           // Current cursor position in buffer
    
    // Turn on cursor
    curs_set(1);
    
    // If we already have a value, display it
    wmove(win, y, x);
    wrefresh(win);
    
    noecho(); // Don't automatically echo characters
    
    while (1) {
        int ch = wgetch(win);
        
        if (ch == '\n') {
            buffer[pos] = '\0';
            break;
        } else if (ch == 27) { // Direct ESC key detection
            curs_set(0);
            return -1; // Signal cancellation
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT) {
            if (pos > 0) {
                pos--;
                mvwaddch(win, y, x + pos, ' ');
                wmove(win, y, x + pos);
                wrefresh(win);
                
                // Shift characters to the left
                for (int i = pos; i < (int)strlen(buffer); i++) {
                    buffer[i] = buffer[i + 1];
                }
                
                // Redraw the input
                wmove(win, y, x);
                wclrtoeol(win);
                mvwprintw(win, y, x, "%s", buffer);
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if (ch == KEY_LEFT) {
            if (pos > 0) {
                pos--;
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if (ch == KEY_RIGHT) {
            if (pos < (int)strlen(buffer)) {
                pos++;
                wmove(win, y, x + pos);
                wrefresh(win);
            }
        } else if ((isdigit(ch) || ch == '-') && pos < MAX_BUFFER - 1) {
            // Validate input (only allow minus sign at start)
            bool valid = true;
            
            if (ch == '-' && pos != 0) {
                valid = false; // Minus sign only at beginning
            }
            
            if (valid) {
                // Shift characters to the right
                for (int i = strlen(buffer); i >= pos; i--) {
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
    if (buffer[0] != '\0') {
        value = atoi(buffer);
    }
    
    curs_set(0);
    return value;
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
    for (int i = 0; i < transaction_count - 1; i++) {
        for (int j = 0; j < transaction_count - i - 1; j++) {
            // Compare dates (format is YYYY-MM-DD so string comparison works)
            if (strcmp(transactions[j].date, transactions[j + 1].date) < 0) {
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
        if (strcmp(transactions[i].date, prev_date) == 0) {
            strcpy(display_date, "          ");
        } else {
            // Format YYYY-MM-DD to MM-DD-YYYY for display
            if (strlen(transactions[i].date) == 10) {
                sprintf(display_date, "%c%c-%c%c-%c%c%c%c",
                        transactions[i].date[5], transactions[i].date[6],   // Month
                        transactions[i].date[8], transactions[i].date[9],   // Day
                        transactions[i].date[0], transactions[i].date[1],   // Year
                        transactions[i].date[2], transactions[i].date[3]);
            } else {
                strcpy(display_date, transactions[i].date); // Use as is if format is unexpected
            }
            
            // Remember this date for the next iteration
            strcpy(prev_date, transactions[i].date);
        }

        mvwprintw(win, y++, 2, "%-10s %-24s $%-9.2f %-24s",
                  display_date, transactions[i].description, transactions[i].amount, category_name);
    }
}

void write_export_content(FILE *export_file) {
    // Write CSV headers
    fprintf(export_file, "TBudget Export\n\n");
    
    // Total budget
    fprintf(export_file, "Total Budget:,%.2f\n\n", total_budget);
    
    // Categories
    fprintf(export_file, "CATEGORIES\n");
    fprintf(export_file, "Name,Amount,Percentage\n");
    
    double total_allocated = 0.0;
    for (int i = 0; i < category_count; i++) {
        total_allocated += categories[i].amount;
    }
    
    for (int i = 0; i < category_count; i++) {
        double percentage = 0.0;
        if (total_budget > 0) {
            percentage = (categories[i].amount / total_budget) * 100.0;
        }
        
        // Escape category names with quotes if they contain commas
        if (strchr(categories[i].name, ',') != NULL) {
            fprintf(export_file, "\"%s\",%.2f,%.2f%%\n", 
                    categories[i].name, categories[i].amount, percentage);
        } else {
            fprintf(export_file, "%s,%.2f,%.2f%%\n", 
                    categories[i].name, categories[i].amount, percentage);
        }
    }
    
    fprintf(export_file, "Total Allocated,%.2f,%.2f%%\n", 
            total_allocated, 
            total_budget > 0 ? (total_allocated / total_budget) * 100.0 : 0.0);
    
    double remaining = total_budget - total_allocated;
    fprintf(export_file, "Remaining Unallocated,%.2f,%.2f%%\n\n", 
            remaining, 
            total_budget > 0 ? (remaining / total_budget) * 100.0 : 0.0);
    
    // Transactions
    fprintf(export_file, "TRANSACTIONS\n");
    fprintf(export_file, "ID,Description,Amount,Category,Date\n");
    
    for (int i = 0; i < transaction_count; i++) {
        char category_name[MAX_NAME_LEN] = "Uncategorized";
        
        if (transactions[i].category_id >= 0 && transactions[i].category_id < category_count) {
            strcpy(category_name, categories[transactions[i].category_id].name);
        }
        
        // Escape descriptions with quotes if they contain commas
        if (strchr(transactions[i].description, ',') != NULL) {
            fprintf(export_file, "%d,\"%s\",%.2f,\"%s\",%s\n", 
                    i + 1, transactions[i].description, transactions[i].amount, 
                    category_name, transactions[i].date);
        } else if (strchr(category_name, ',') != NULL) {
            // If category name has a comma, we need to quote it
            fprintf(export_file, "%d,%s,%.2f,\"%s\",%s\n", 
                    i + 1, transactions[i].description, transactions[i].amount, 
                    category_name, transactions[i].date);
        } else {
            fprintf(export_file, "%d,%s,%.2f,%s,%s\n", 
                    i + 1, transactions[i].description, transactions[i].amount, 
                    category_name, transactions[i].date);
        }
    }
}

void export_data_to_csv(int silent) {
    // Create the main export file
    FILE *file = fopen(export_file_path, "w");
    if (!file) {
        if (!silent) {
            fprintf(stderr, "Error: Unable to open export file\n");
        }
        return;
    }
    
    // Also create a timestamped version in the exports directory
    char timestamp_filename[MAX_BUFFER];
    char timestamp[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    
    // Create exports directory if it doesn't exist
    char mkdir_cmd[MAX_BUFFER];
    sprintf(mkdir_cmd, "mkdir -p %s", data_storage_dir);
    system(mkdir_cmd);
    
    sprintf(timestamp_filename, "%s/tbudget_export_%s.csv", data_storage_dir, timestamp);
    FILE *history_file = fopen(timestamp_filename, "w");
    
    // Write content to the main export file
    write_export_content(file);
    fclose(file);
    
    // Write to the history file if it was opened successfully
    if (history_file) {
        write_export_content(history_file);
        fclose(history_file);
        
        if (!silent) {
            fprintf(stderr, "Created timestamped backup: %s\n", timestamp_filename);
        }
    }
    
    // Return success message to stderr for visibility, but only if not silent
    if (!silent) {
        fprintf(stderr, "Successfully exported data to %s\n", export_file_path);
    }
}

void import_data_from_csv(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Unable to open import file: %s\n", filename);
        return;
    }

    // Reset data
    category_count = 0;
    transaction_count = 0;
    total_budget = 0.0;

    char line[MAX_BUFFER];
    int section = 0;     // 0 = header, 1 = budget, 2 = categories, 3 = transactions
    int header_line = 0; // Used to skip header lines

    // Read file line by line
    while (fgets(line, sizeof(line), file))
    {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines
        if (strlen(line) == 0)
        {
            continue;
        }

        // Check for section headers
        if (strncmp(line, "TBudget Export", 14) == 0)
        {
            section = 0;
            continue;
        }
        else if (strncmp(line, "Total Budget:", 13) == 0)
        {
            section = 1;
            char *comma = strchr(line, ',');
            if (comma)
            {
                total_budget = atof(comma + 1);
            }
            continue;
        }
        else if (strcmp(line, "CATEGORIES") == 0)
        {
            section = 2;
            header_line = 1; // Next line is header
            continue;
        }
        else if (strcmp(line, "TRANSACTIONS") == 0)
        {
            section = 3;
            header_line = 1; // Next line is header
            continue;
        }

        // Skip header lines
        if (header_line)
        {
            header_line = 0;
            continue;
        }

        // Skip summary lines in categories section
        if (section == 2 && (strncmp(line, "Total Allocated,", 16) == 0 ||
                             strncmp(line, "Remaining Unallocated,", 22) == 0))
        {
            continue;
        }

        // Parse data based on section
        if (section == 2 && category_count < MAX_CATEGORIES)
        {
            // Categories section
            char name[MAX_NAME_LEN] = {0};
            double amount = 0.0;

            // Handle quoted fields (for names with commas)
            if (line[0] == '"')
            {
                int i = 1;
                int j = 0;
                // Extract text until the closing quote
                while (line[i] != '"' && line[i] != '\0' && j < MAX_NAME_LEN - 1)
                {
                    name[j++] = line[i++];
                }
                name[j] = '\0';

                // Skip to the comma after the closing quote
                while (line[i] != ',' && line[i] != '\0')
                {
                    i++;
                }

                // Skip the comma
                if (line[i] == ',')
                {
                    i++;
                }

                // Parse amount
                amount = atof(line + i);
            }
            else
            {
                // Simple case: no quotes
                char *comma = strchr(line, ',');
                if (comma)
                {
                    int name_len = comma - line;
                    if (name_len > MAX_NAME_LEN - 1)
                    {
                        name_len = MAX_NAME_LEN - 1;
                    }
                    strncpy(name, line, name_len);
                    name[name_len] = '\0';

                    amount = atof(comma + 1);
                }
            }

            // Store in category array
            if (strlen(name) > 0)
            {
                strncpy(categories[category_count].name, name, MAX_NAME_LEN - 1);
                categories[category_count].name[MAX_NAME_LEN - 1] = '\0';
                categories[category_count].amount = amount;
                category_count++;
            }
        }
        else if (section == 3 && transaction_count < MAX_TRANSACTIONS)
        {
            // Transactions section
            // Skip ID and extract description, amount, category name, date
            char *ptr = line;
            char *comma = strchr(ptr, ',');
            if (!comma)
                continue;

            // Skip ID
            ptr = comma + 1;

            // Parse description (handle quotes)
            char description[MAX_NAME_LEN] = {0};
            if (*ptr == '"')
            {
                ptr++; // Skip opening quote
                int i = 0;
                while (*ptr != '"' && *ptr != '\0' && i < MAX_NAME_LEN - 1)
                {
                    description[i++] = *ptr++;
                }
                description[i] = '\0';

                // Skip to next comma
                while (*ptr != ',' && *ptr != '\0')
                    ptr++;
                if (*ptr == ',')
                    ptr++;
            }
            else
            {
                comma = strchr(ptr, ',');
                if (!comma)
                    continue;

                int desc_len = comma - ptr;
                if (desc_len > MAX_NAME_LEN - 1)
                {
                    desc_len = MAX_NAME_LEN - 1;
                }
                strncpy(description, ptr, desc_len);
                description[desc_len] = '\0';

                ptr = comma + 1;
            }

            // Parse amount
            double amount = 0.0;
            comma = strchr(ptr, ',');
            if (!comma)
                continue;

            char amount_str[32] = {0};
            int amount_len = comma - ptr;
            if (amount_len < 32)
            {
                strncpy(amount_str, ptr, amount_len);
                amount_str[amount_len] = '\0';
                amount = atof(amount_str);
            }

            ptr = comma + 1;

            // Parse category name (lookup ID)
            char category_name[MAX_NAME_LEN] = {0};
            int category_id = -1;

            if (*ptr == '"')
            {
                ptr++; // Skip opening quote
                int i = 0;
                while (*ptr != '"' && *ptr != '\0' && i < MAX_NAME_LEN - 1)
                {
                    category_name[i++] = *ptr++;
                }
                category_name[i] = '\0';

                // Skip to next comma
                while (*ptr != ',' && *ptr != '\0')
                    ptr++;
                if (*ptr == ',')
                    ptr++;
            }
            else
            {
                comma = strchr(ptr, ',');
                if (!comma)
                    continue;

                int cat_len = comma - ptr;
                if (cat_len > MAX_NAME_LEN - 1)
                {
                    cat_len = MAX_NAME_LEN - 1;
                }
                strncpy(category_name, ptr, cat_len);
                category_name[cat_len] = '\0';

                ptr = comma + 1;
            }

            // Look up category ID
            for (int i = 0; i < category_count; i++)
            {
                if (strcmp(categories[i].name, category_name) == 0)
                {
                    category_id = i;
                    break;
                }
            }

            // If category not found and it's not "Uncategorized"
            if (category_id == -1 && strcmp(category_name, "Uncategorized") != 0)
            {
                // Add as a new category with zero amount (user can adjust later)
                if (category_count < MAX_CATEGORIES)
                {
                    strncpy(categories[category_count].name, category_name, MAX_NAME_LEN - 1);
                    categories[category_count].name[MAX_NAME_LEN - 1] = '\0';
                    categories[category_count].amount = 0.0;
                    category_id = category_count;
                    category_count++;
                }
            }

            // Parse date
            char date[11] = {0};
            strncpy(date, ptr, 10);
            date[10] = '\0';

            // Store in transaction array
            if (strlen(description) > 0)
            {
                strncpy(transactions[transaction_count].description, description, MAX_NAME_LEN - 1);
                transactions[transaction_count].description[MAX_NAME_LEN - 1] = '\0';
                transactions[transaction_count].amount = amount;
                transactions[transaction_count].category_id = category_id;
                strncpy(transactions[transaction_count].date, date, 10);
                transactions[transaction_count].date[10] = '\0';
                transaction_count++;
            }
        }
    }

    fclose(file);

    // Return success message to stderr for visibility
    fprintf(stderr, "Successfully imported data from %s\n", filename);
}

void save_data_to_file()
{
    FILE *file = fopen(data_file_path, "w");
    if (!file)
    {
        // If unable to open file, just return silently
        // In a production app, we'd want to notify the user
        return;
    }

    // Write CSV-compatible format
    // First line: header for budget
    fprintf(file, "BUDGET,%.2f\n", total_budget);

    // Second section: categories
    fprintf(file, "CATEGORIES\n");
    fprintf(file, "Name,Amount\n");
    for (int i = 0; i < category_count; i++)
    {
        // Escape any commas in the name with quotes
        if (strchr(categories[i].name, ',') != NULL)
        {
            fprintf(file, "\"%s\",%.2f\n", categories[i].name, categories[i].amount);
        }
        else
        {
            fprintf(file, "%s,%.2f\n", categories[i].name, categories[i].amount);
        }
    }

    // Third section: transactions
    fprintf(file, "TRANSACTIONS\n");
    fprintf(file, "Description,Amount,CategoryID,Date\n");
    for (int i = 0; i < transaction_count; i++)
    {
        // Escape any commas in the description with quotes
        if (strchr(transactions[i].description, ',') != NULL)
        {
            fprintf(file, "\"%s\",%.2f,%d,%s\n",
                    transactions[i].description,
                    transactions[i].amount,
                    transactions[i].category_id,
                    transactions[i].date);
        }
        else
        {
            fprintf(file, "%s,%.2f,%d,%s\n",
                    transactions[i].description,
                    transactions[i].amount,
                    transactions[i].category_id,
                    transactions[i].date);
        }
    }

    fclose(file);

    // Automatically export to CSV whenever data is saved
    export_data_to_csv(1); // Silent, don't show message when automatic
}

void load_data_from_file()
{
    FILE *file = fopen(data_file_path, "r");
    if (!file)
    {
        // If file doesn't exist, that's okay - we'll start with empty data
        return;
    }

    char line[MAX_BUFFER];
    int section = 0; // 0 = budget, 1 = categories, 2 = transactions

    // Reset data
    category_count = 0;
    transaction_count = 0;

    // Read file line by line
    while (fgets(line, sizeof(line), file))
    {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines
        if (strlen(line) == 0)
        {
            continue;
        }

        // Check for section headers
        if (strncmp(line, "BUDGET,", 7) == 0)
        {
            total_budget = atof(line + 7);
            continue;
        }
        else if (strcmp(line, "CATEGORIES") == 0)
        {
            section = 1;
            continue;
        }
        else if (strcmp(line, "TRANSACTIONS") == 0)
        {
            section = 2;
            continue;
        }

        // Skip header lines
        if (strncmp(line, "Name,", 5) == 0 || strncmp(line, "Description,", 12) == 0)
        {
            continue;
        }

        // Parse data based on section
        if (section == 1 && category_count < MAX_CATEGORIES)
        {
            // Categories section
            char name[MAX_NAME_LEN] = {0};
            double amount = 0.0;

            // Handle quoted fields (for names with commas)
            if (line[0] == '"')
            {
                int i = 1;
                int j = 0;
                // Extract text until the closing quote
                while (line[i] != '"' && line[i] != '\0' && j < MAX_NAME_LEN - 1)
                {
                    name[j++] = line[i++];
                }
                name[j] = '\0';

                // Skip to the comma after the closing quote
                while (line[i] != ',' && line[i] != '\0')
                {
                    i++;
                }

                // Skip the comma
                if (line[i] == ',')
                {
                    i++;
                }

                // Parse amount
                amount = atof(line + i);
            }
            else
            {
                // Simple case: no quotes
                char *comma = strchr(line, ',');
                if (comma)
                {
                    int name_len = comma - line;
                    if (name_len > MAX_NAME_LEN - 1)
                    {
                        name_len = MAX_NAME_LEN - 1;
                    }
                    strncpy(name, line, name_len);
                    name[name_len] = '\0';

                    amount = atof(comma + 1);
                }
            }

            // Store in category array
            if (strlen(name) > 0)
            {
                strncpy(categories[category_count].name, name, MAX_NAME_LEN - 1);
                categories[category_count].name[MAX_NAME_LEN - 1] = '\0';
                categories[category_count].amount = amount;
                category_count++;
            }
        }
        else if (section == 2 && transaction_count < MAX_TRANSACTIONS)
        {
            // Transactions section
            char description[MAX_NAME_LEN] = {0};
            double amount = 0.0;
            int category_id = 0;
            char date[11] = {0};

            // Parse the transaction line
            char *ptr = line;

            // Handle quoted fields (for descriptions with commas)
            if (line[0] == '"')
            {
                int i = 1;
                int j = 0;
                // Extract text until the closing quote
                while (line[i] != '"' && line[i] != '\0' && j < MAX_NAME_LEN - 1)
                {
                    description[j++] = line[i++];
                }
                description[j] = '\0';

                // Skip to the comma after the closing quote
                while (line[i] != ',' && line[i] != '\0')
                {
                    i++;
                }

                // Skip the comma
                if (line[i] == ',')
                {
                    i++;
                }

                // Parse remaining fields
                ptr = line + i;
            }
            else
            {
                // Simple case: no quotes
                char *comma = strchr(ptr, ',');
                if (comma)
                {
                    int desc_len = comma - ptr;
                    if (desc_len > MAX_NAME_LEN - 1)
                    {
                        desc_len = MAX_NAME_LEN - 1;
                    }
                    strncpy(description, ptr, desc_len);
                    description[desc_len] = '\0';

                    ptr = comma + 1;
                }
            }

            // Parse amount
            char *comma = strchr(ptr, ',');
            if (comma)
            {
                char amount_str[32] = {0};
                int amount_len = comma - ptr;
                if (amount_len < 32)
                {
                    strncpy(amount_str, ptr, amount_len);
                    amount_str[amount_len] = '\0';
                    amount = atof(amount_str);
                }

                ptr = comma + 1;
            }

            // Parse category_id
            comma = strchr(ptr, ',');
            if (comma)
            {
                char cat_id_str[16] = {0};
                int cat_id_len = comma - ptr;
                if (cat_id_len < 16)
                {
                    strncpy(cat_id_str, ptr, cat_id_len);
                    cat_id_str[cat_id_len] = '\0';
                    category_id = atoi(cat_id_str);
                }

                ptr = comma + 1;
            }

            // Parse date
            if (strlen(ptr) > 0)
            {
                strncpy(date, ptr, 10);
                date[10] = '\0';
            }

            // Store in transaction array
            if (strlen(description) > 0)
            {
                strncpy(transactions[transaction_count].description, description, MAX_NAME_LEN - 1);
                transactions[transaction_count].description[MAX_NAME_LEN - 1] = '\0';
                transactions[transaction_count].amount = amount;
                transactions[transaction_count].category_id = category_id;
                strncpy(transactions[transaction_count].date, date, 10);
                transactions[transaction_count].date[10] = '\0';
                transaction_count++;
            }
        }
    }

    fclose(file);
}

char *get_home_directory() {
    char *home = getenv("HOME");
    
    if (home != NULL) {
        return home;
    }
    
    // If HOME is not available, try to get it from passwd
    struct passwd *pw = getpwuid(getuid());
    if (pw != NULL) {
        return pw->pw_dir;
    }
    
    // If all fails, use current directory
    return ".";
}

int create_directory_if_not_exists(const char *path) {
    struct stat st;
    
    if (stat(path, &st) == 0) {
        // Already exists
        if (S_ISDIR(st.st_mode)) {
            return 0; // Directory exists
        }
        return -1; // Exists but is not a directory
    }
    
    // Create the directory
    #ifdef _WIN32
    int result = mkdir(path);
    #else
    int result = mkdir(path, 0755);
    #endif
    
    if (result != 0) {
        fprintf(stderr, "Error creating directory %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    return 0;
}

void initialize_data_directories() {
    // Get the user's home directory
    char *home_dir = get_home_directory();
    
    // Create the app directory path
    #ifdef __APPLE__
    sprintf(app_data_dir, "%s/Library/Application Support/%s", home_dir, APP_DIR_NAME);
    #elif defined(_WIN32)
    sprintf(app_data_dir, "%s\\AppData\\Local\\%s", home_dir, APP_DIR_NAME);
    #else // Linux and other Unix-like
    sprintf(app_data_dir, "%s/.config/%s", home_dir, APP_DIR_NAME);
    #endif
    
    // Create the data directory path
    sprintf(data_storage_dir, "%s/%s", app_data_dir, DATA_DIR_NAME);
    
    // Create file paths
    sprintf(data_file_path, "%s/%s", app_data_dir, DATA_FILE_NAME);
    sprintf(export_file_path, "%s/%s", app_data_dir, EXPORT_FILE_NAME);
    
    // Create directories
    create_directory_if_not_exists(app_data_dir);
    create_directory_if_not_exists(data_storage_dir);
}

// Helper function to get days in a month
static int get_days_in_month(int m, int y) {
    // Array of days in each month
    static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
        return 29; // Leap year
    return days_in_month[m];
}

// Helper function to validate day value
static int validate_day(int day, int month, int year) {
    int max_days = get_days_in_month(month, year);
    if (day > max_days) {
        return 1; // Wrap around to 1
    } else if (day < 1) {
        return max_days; // Wrap to max days
    }
    return day;
}

// Helper function to format and display the date
static void format_date(WINDOW *win, int y, int x, int day, int month, int year, 
                        int highlighted_field, const int cursor_positions[]) {
    // Ensure day is valid for current month/year
    day = validate_day(day, month, year);
    
    // Create date string in DD-MM-YYYY format for display
    char date_str[11];
    sprintf(date_str, "%02d-%02d-%04d", day, month, year);
    
    // Display the date string with the current field highlighted
    for (int i = 0; i < 10; i++) {
        if ((i < 2 && highlighted_field == 0) ||
            (i > 2 && i < 5 && highlighted_field == 1) ||
            (i > 5 && highlighted_field == 2)) {
            wattron(win, COLOR_PAIR(4)); // Highlight current field
        }
        
        mvwaddch(win, y, x + i, date_str[i]);
        
        if ((i < 2 && highlighted_field == 0) ||
            (i > 2 && i < 5 && highlighted_field == 1) ||
            (i > 5 && highlighted_field == 2)) {
            wattroff(win, COLOR_PAIR(4));
        }
    }
    
    // Draw "Today" button
    if (highlighted_field == 3) {
        wattron(win, COLOR_PAIR(4));
    }
    mvwprintw(win, y, x + 12, "[Today]");
    if (highlighted_field == 3) {
        wattroff(win, COLOR_PAIR(4));
    }
    
    // Position cursor at the current field
    if (highlighted_field < 3) {
        wmove(win, y, x + cursor_positions[highlighted_field]);
    } else {
        wmove(win, y, x + 12); // "Today" button
    }
}

// Function to get date input with improved UX
int get_date_input(WINDOW *win, char *date_buffer, int y, int x)
{
    int ch;
    int highlighted_field = 0; // 0 = day, 1 = month, 2 = year, 3 = "Today" button
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
    if (strlen(date_buffer) >= 10) {
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
    } else {
        // If we have a last transaction, use its date
        if (transaction_count > 0) {
            char temp[5];
            strncpy(temp, transactions[transaction_count-1].date, 4);
            temp[4] = '\0';
            year = atoi(temp);
            
            strncpy(temp, transactions[transaction_count-1].date + 5, 2);
            temp[2] = '\0';
            month = atoi(temp);
            
            strncpy(temp, transactions[transaction_count-1].date + 8, 2);
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
    while (1) {
        ch = wgetch(win);
        
        if (ch == '\n') {
            // If "Today" button is selected, set to today's date
            if (highlighted_field == 3) {
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
            } else {
                // User is done editing, convert to YYYY-MM-DD format
                sprintf(date_buffer, "%04d-%02d-%02d", year, month, day);
                
                // Restore original cursor state
                curs_set(original_cursor_state);
                return 1;
            }
        } 
        else if (ch == 27) { // ASCII value for ESC key
            // Always treat as a true escape in date input
            curs_set(original_cursor_state);
            return 0;
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_BACKSPACE_ALT) {
            // User wants to cancel input
            curs_set(original_cursor_state);
            return 0;
        }
        else if (ch == KEY_RIGHT) {
            // Move to next field
            highlighted_field = (highlighted_field + 1) % 4;
            format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
            wnoutrefresh(win);
            doupdate();
        } 
        else if (ch == KEY_LEFT) {
            // Move to previous field
            highlighted_field = (highlighted_field + 3) % 4;
            format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
            wnoutrefresh(win);
            doupdate();
        } 
        else if (ch == KEY_UP) {
            // Increase current field value
            if (highlighted_field == 0) {
                day++;
                if (day > get_days_in_month(month, year)) {
                    day = 1;
                }
            } 
            else if (highlighted_field == 1) {
                month++;
                if (month > 12) {
                    month = 1;
                }
                day = validate_day(day, month, year);
            } 
            else if (highlighted_field == 2) {
                year++;
                day = validate_day(day, month, year);
            }
            // No action for "Today" button
            
            format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
            wnoutrefresh(win);
            doupdate();
        } 
        else if (ch == KEY_DOWN) {
            // Decrease current field value
            if (highlighted_field == 0) {
                day--;
                if (day < 1) {
                    day = get_days_in_month(month, year);
                }
            } 
            else if (highlighted_field == 1) {
                month--;
                if (month < 1) {
                    month = 12;
                }
                day = validate_day(day, month, year);
            } 
            else if (highlighted_field == 2) {
                year--;
                day = validate_day(day, month, year);
            }
            // No action for "Today" button
            
            format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
            wnoutrefresh(win);
            doupdate();
        } 
        else if (isdigit(ch) && highlighted_field < 3) {
            // Handle direct digit input for each field
            int digit = ch - '0';
            
            if (highlighted_field == 0) { // Day field
                day = day % 10 * 10 + digit;
                if (day > get_days_in_month(month, year)) {
                    day = digit;
                }
                if (day == 0) {
                    day = digit;
                }
            } 
            else if (highlighted_field == 1) { // Month field
                month = month % 10 * 10 + digit;
                if (month > 12) {
                    month = digit;
                }
                if (month == 0) {
                    month = digit;
                }
                day = validate_day(day, month, year);
            } 
            else if (highlighted_field == 2) { // Year field
                year = year % 1000 * 10 + digit;
                day = validate_day(day, month, year);
            }
            
            format_date(win, y, x, day, month, year, highlighted_field, cursor_positions);
            wnoutrefresh(win);
            doupdate();
        }
    }
} 