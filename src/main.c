#include "main.h"

// Main function
int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    // int mode = MODE_MENU; // Default mode

    initialize_data_directories();
    init_file_cache();     // Initialize the file cache
    cache_recent_months(); // Cache the 10 most recent months

    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    today_month = today->tm_mon + 1;
    today_year = today->tm_year + 1900;
    current_month = today_month;
    current_year = today_year;

    int res = load_budget_data();
    if (res < 0)
    {
        fprintf(stderr, "Failed to initialize data from file: %d\n", res);
        return -1;
    }

    if ((res = load_month(current_year, current_month)) < 0)
    {
        fprintf(stderr, "Failed to load budget data: %d\n", res);
        return -1;
    }

    // // Parse command line arguments
    // if (argc > 1)
    // {
    //     if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    //     {
    //         print_usage(argv[0]);
    //         return 0;
    //     }
    //     else if (strcmp(argv[1], "--dashboard") == 0 || strcmp(argv[1], "-d") == 0)
    //     {
    //         mode = MODE_DASHBOARD;
    //     }
    //     else if (strcmp(argv[1], "--export") == 0 || strcmp(argv[1], "-e") == 0)
    //     {
    //         // Export data to CSV and exit
    //         // load_data_from_file();
    //         // export_data_to_csv(0); // Not silent, show message
    //         printf("Data exported to %s\n", export_file_path);
    //         return 0;
    //     }
    //     else if (strcmp(argv[1], "--history") == 0 || strcmp(argv[1], "-l") == 0)
    //     {
    //         // List export history by directly using system command
    //         printf("Export history files in '%s' directory:\n\n", data_storage_dir);
    //         char mkdir_cmd[MAX_BUFFER];
    //         sprintf(mkdir_cmd, "mkdir -p %s", data_storage_dir);
    //         system(mkdir_cmd);

    //         char ls_cmd[MAX_BUFFER];
    //         sprintf(ls_cmd, "ls -lt %s", data_storage_dir);
    //         system(ls_cmd);

    //         printf("\nTo use a specific export file, import it with:\n");
    //         printf("  %s --import %s/filename.csv\n", argv[0], data_storage_dir);
    //         return 0;
    //     }
    //     else if (strcmp(argv[1], "--import") == 0 || strcmp(argv[1], "-i") == 0)
    //     {
    //         // Import data from CSV
    //         if (argc < 3)
    //         {
    //             fprintf(stderr, "Error: Import requires a filename\n");
    //             fprintf(stderr, "Usage: %s --import <filename>\n", argv[0]);
    //             return 1;
    //         }
    //         import_data_from_csv(argv[2]);
    //         // TODO: save to file
    //         cleanup_ncurses();
    //         printf("Data imported from %s\n", argv[2]);
    //         return 0;
    //     }
    //     else
    //     {
    //         // Try to parse as a number
    //         char *endptr;
    //         long val = strtol(argv[1], &endptr, 10);

    //         if (*endptr == '\0')
    //         { // Successful conversion
    //             if (val == MODE_MENU)
    //             {
    //                 mode = MODE_MENU;
    //             }
    //             else if (val == MODE_DASHBOARD)
    //             {
    //                 mode = MODE_DASHBOARD;
    //             }
    //             else
    //             {
    //                 fprintf(stderr, "Invalid mode: %ld\n", val);
    //                 print_usage(argv[0]);
    //                 return 1;
    //             }
    //         }
    //         else
    //         {
    //             fprintf(stderr, "Invalid argument: %s\n", argv[1]);
    //             print_usage(argv[0]);
    //             return 1;
    //         }
    //     }
    // }
    // else
    // {
    //     print_usage(argv[0]);
    //     return 1;
    // }

    res = run_dashboard_mode();
    return res;
}

// Print usage information
void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [OPTION]\n", program_name);
    fprintf(stderr, "Budget management application for graduate students.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -m, --menu        Run in menu-based mode (default)\n");
    fprintf(stderr, "  -d, --dashboard   Run in dashboard mode (full-screen overview)\n");
    fprintf(stderr, "  -e, --export      Export data to CSV (exports to %s)\n", export_file_path);
    fprintf(stderr, "  -i, --import      Import data from CSV file (requires filename)\n");
    fprintf(stderr, "                    Example: %s --import path/to/data.csv\n", program_name);
    fprintf(stderr, "  -l, --history     List export history files (stored in %s)\n", data_storage_dir);
    fprintf(stderr, "  -h, --help        Display this help and exit\n");
    fprintf(stderr, "  1                 Run in menu-based mode\n");
    fprintf(stderr, "  2                 Run in dashboard mode\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Data is stored in %s\n", app_data_dir);
}

// // The original menu-based mode
// void run_menu_mode()
// {
//     const char *main_menu[] = {
//         "1. Do nothing",
//         "2. Go dashboard mode",
//         "3. Exit"};
//     int menu_size = sizeof(main_menu) / sizeof(main_menu[0]);

//     while (1)
//     {
//         clear();
//         draw_title(stdscr, "TBudget - Budget Management Tool");

//         int choice = get_menu_choice(stdscr, main_menu, menu_size, 10, 0);

//         if (choice == -1)
//         {
//             return; // Escape or unexpected input
//         }

//         switch (choice)
//         {
//         case 0: // Budget Setup (index 0 corresponds to option 1)
//             break;
//         case 1: // Manage Transactions
//             run_dashboard_mode();
//             break;
//         case 2: // Exit
//             return;
//         }
//     }
// }

// New dashboard mode that shows everything at once with flexible boxes
int run_dashboard_mode()
{
    setup_ncurses();
    WINDOW *win = stdscr;
    int ch;

    // Create windows for different sections
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Create sub-windows
    const int NUM_WINDOWS = 6, NUM_SELECTABLE_WINDOWS = 6; // Total number of interactive windows
    BoundedWindow action_win = {0}, budget_win = {0}, breakdown_win = {0}, trans_win = {0}, subscription_win = {0}, TODO_win = {0};
    BoundedWindow *all_windows[] = {&action_win, &budget_win, &breakdown_win, &trans_win, &subscription_win, &TODO_win};
    const char *window_titles[] = {
        "Actions",
        "Budget Summary",
        "Budget Breakdown",
        "Transaction History",
        "Repeating",
        "TODO"};

    // Prepare action menu
    const char *action_menu[] = {
        "Add Expense",
        "Remove Transaction",
        "Add Repeating",
        "Remove Repeating",
        "Export to CSV",
        "Previous Month",
        "Next Month",
        "Exit Dashboard"};
    int action_menu_size = sizeof(action_menu) / sizeof(action_menu[0]);
    int highlighted_action = 0;

    // Turn off cursor
    curs_set(0);
    clear();
    draw_title(win, "tbudget Dashboard");

    bool needs_redraw = true;
    bool is_leaving = false;
    bool show_pie_chart = true; // Flag to toggle between table and pie chart view

    // Create flex layout containers
    FlexContainer *top_row = NULL;
    FlexContainer *bottom_row = NULL;

    int active_window = 0; // Start with actions menu (window 0)

    int selected_transaction = 0;      // Index of the selected transaction
    int first_display_transaction = 0; // Index of the first transaction to display

    int selected_subscription = 0;      // Index of the selected subscription
    int first_display_subscription = 0; // Index of the first subscription to display

    // Vim-style navigation count buffer
    char count_buffer[16] = {0}; // Buffer to store the count prefix
    int count_buffer_pos = 0;    // Current position in the count buffer
    int res;

    update_subscriptions(); // Update subscriptions and create transactions

    // Main dashboard loop
    while (1)
    {
        if (current_month != loaded_month || current_year != loaded_year)
        {
            if ((res = load_month(current_year, current_month)) < 0)
            {
                fprintf(stderr, "Failed to load budget data: %d\n", res);
                return 0;
            }
        }
        if (needs_redraw)
        {
            // Recalculate sizes in case of window resize
            getmaxyx(win, max_y, max_x);

            // Clean up previous layout if any
            if (main_layout != NULL)
            {
                free_flex_layout(main_layout);
            }

            // Delete any existing windows
            delete_bounded_array(all_windows, NUM_WINDOWS);

            // Create new layout with current dimensions
            // Main layout - vertical column
            main_layout = create_flex_container(
                FLEX_COLUMN,        // Direction: vertically stacked
                FLEX_START,         // Justify: items at the start
                FLEX_ALIGN_STRETCH, // Align: stretch across container width
                1,                  // Gap between items
                0,                  // No padding
                2                   // Two items (top and bottom rows)
            );

            // Top row - horizontal row for action menu, budget summary, and breakdown
            top_row = create_flex_container(
                FLEX_ROW,           // Direction: horizontally arranged
                FLEX_START,         // Justify: items at the start
                FLEX_ALIGN_STRETCH, // Align: stretch height
                1,                  // Gap between items
                0,                  // No padding
                3                   // Three items
            );

            // Bottom row - horizontal row for transactions, subscriptions, and TODOs
            bottom_row = create_flex_container(
                FLEX_ROW,           // Direction: horizontally arranged
                FLEX_START,         // Justify: items at the start
                FLEX_ALIGN_STRETCH, // Align: stretch height
                1,                  // Gap between items
                0,                  // No padding
                3                   // Three items
            );

            // Add rows to main layout
            flex_container_add_item(main_layout, flex_container(1, 0, 0, 0, top_row));
            flex_container_add_item(main_layout, flex_container(1, 0, 0, 0, bottom_row));

            // Add items to top row
            flex_container_add_item(top_row, flex_window(1, 0, window_titles[0], active_window == 0, ALIGN_LEFT, &action_win));
            flex_container_add_item(top_row, flex_window(2, 0, window_titles[1], active_window == 1, ALIGN_LEFT, &budget_win));
            flex_container_add_item(top_row, flex_window(2, 0, window_titles[2], active_window == 2, ALIGN_LEFT, &breakdown_win));

            // Add items to bottom row
            flex_container_add_item(bottom_row, flex_window(2, 0, window_titles[3], active_window == 3, ALIGN_LEFT, &trans_win));
            flex_container_add_item(bottom_row, flex_window(1, 0, window_titles[4], active_window == 4, ALIGN_LEFT, &subscription_win));
            flex_container_add_item(bottom_row, flex_window(1, 0, window_titles[5], active_window == 5, ALIGN_LEFT, &TODO_win));

            // Apply the flex layout
            apply_flex_layout(main_layout, 0, 2, max_x, max_y - 3);

            // Key help line
            char *help_text = is_leaving ? "Exiting tbudget, press Q again to confirm" : "TAB/Shift+TAB or ARROW KEYS to navigate | ENTER to select | Q to quit";

            // Display help line at the bottom
            mvwhline(win, max_y - 1, 0, ' ', max_x); // Clear the line first
            mvwprintw(win, max_y - 1, (max_x - strlen(help_text)) / 2, "%s", help_text);

            const char *month = month_names[current_month];
            // Display budget summary
            mvwprintw(budget_win.textbox, 1, 2, "%s %d Budget: $%.2f", month, current_year, current_month_total_budget);
            if (category_count > 0)
            {
                // Show tabular view
                display_categories(budget_win.textbox, 3);
                if (show_pie_chart)
                {
                    int x, y;
                    getmaxyx(breakdown_win.textbox, y, x);
                    display_budget_pie_chart(breakdown_win.textbox, 0.8 * x, 0.65 * y);

                    // Create bar chart as a child of the breakdown window
                    create_bar_chart(&breakdown_win);
                }
            }
            else
            {
                mvwprintw(budget_win.textbox, 3, 2, "No budget categories defined yet.");
                mvwprintw(budget_win.textbox, 4, 2, "Select 'Add Category' from the Actions menu.");
            }

            // Display transaction history
            display_transactions(trans_win.textbox, 1, selected_transaction, &first_display_transaction, active_window == TRANSACTION_HISTORY_WINDOW);

            // Display subscriptions
            if (subscription_count > 0)
            {
                display_subscriptions(subscription_win.textbox, 1, selected_subscription, &first_display_subscription, active_window == SUBSCRIPTIONS_WINDOW);
            }
            else
            {
                mvwprintw(subscription_win.textbox, 1, 2, "No active subscriptions.");
                mvwprintw(subscription_win.textbox, 2, 2, "Select 'Add Subscription' from the Actions menu.");
            }

            // Display action menu
            for (int i = 0; i < action_menu_size; i++)
            {
                if (active_window == ACTIONS_MENU_WINDOW && i == highlighted_action)
                {
                    wattron(action_win.textbox, COLOR_PAIR(5));
                    mvwprintw(action_win.textbox, 2 + i, 2, "%d. %s", i + 1, action_menu[i]);
                    wattroff(action_win.textbox, COLOR_PAIR(5));
                }
                else
                {
                    mvwprintw(action_win.textbox, 2 + i, 2, "%d. %s", i + 1, action_menu[i]);
                }
            }

            // Refresh windows
            wnoutrefresh(win);
            bwarrnoutrefresh(all_windows, NUM_WINDOWS);
            doupdate();
            needs_redraw = false;
        }

        // Get user input
        ch = getch();

        if (ch == 'q' || ch == 'Q')
        {
            memset(count_buffer, 0, sizeof(count_buffer));
            count_buffer_pos = 0;
            if (is_leaving)
            {
                delete_bounded_array(all_windows, NUM_WINDOWS);
                save_and_exit();
                return 0;
            }
            else
            {
                is_leaving = true;
                needs_redraw = true;
                continue;
            }
        }
        else
        {
            if (is_leaving)
            {
                is_leaving = false;
                needs_redraw = true;
            }
        }

        // Check if character is a digit (0-9)
        if (ch >= '0' && ch <= '9')
        {
            // Add digit to the count buffer
            if (count_buffer_pos < (int)sizeof(count_buffer) - 1)
            { // Leave space for null terminator
                count_buffer[count_buffer_pos++] = ch;
                count_buffer[count_buffer_pos] = '\0';
            }
            needs_redraw = true; // Redraw to update the help text
            continue;
        }

        // Get count (default to 1 if buffer is empty)
        int count = 1;
        if (count_buffer_pos > 0)
        {
            count = atoi(count_buffer);
            if (count < 1)
                count = 1; // Ensure at least 1
        }

        // Handle window selection
        switch (ch)
        {
        case '\t': // Tab key
        case KEY_RIGHT:
            active_window = (active_window + 1) % NUM_SELECTABLE_WINDOWS;
            needs_redraw = true;
            break;
        case 'l':
            active_window = (active_window + count) % NUM_SELECTABLE_WINDOWS;
            needs_redraw = true;
            break;

        case KEY_BTAB: // Shift+Tab
        case KEY_LEFT:
            active_window = (active_window - 1 + NUM_SELECTABLE_WINDOWS) % NUM_SELECTABLE_WINDOWS;
            needs_redraw = true;
            break;
        case 'h':
            active_window = (active_window - (count % NUM_SELECTABLE_WINDOWS) + NUM_SELECTABLE_WINDOWS) % NUM_SELECTABLE_WINDOWS;
            needs_redraw = true;
            break;
        case 'j':
        case KEY_DOWN:
        {
            int amount = ch == 'j' ? count : 1;
            if (active_window == ACTIONS_MENU_WINDOW && highlighted_action < action_menu_size - 1)
            {
                highlighted_action += amount;
                highlighted_action = MIN(highlighted_action, action_menu_size - 1);
            }
            else if (active_window == TRANSACTION_HISTORY_WINDOW && selected_transaction < current_month_transaction_count - 1)
            {
                selected_transaction += amount;
                selected_transaction = MIN(selected_transaction, current_month_transaction_count - 1);
            }
            else if (active_window == SUBSCRIPTIONS_WINDOW && selected_subscription < subscription_count - 1)
            {
                selected_subscription += amount;
                selected_subscription = MIN(selected_subscription, subscription_count - 1);
            }
            else
            {
                active_window = (active_window + amount) % NUM_SELECTABLE_WINDOWS;
            }
            needs_redraw = true;
            break;
        }

        case 'k':
        case KEY_UP:
        {
            int amount = ch == 'k' ? count : 1;
            if (active_window == ACTIONS_MENU_WINDOW && highlighted_action > 0)
            {
                highlighted_action -= amount;
                highlighted_action = MAX(highlighted_action, 0);
            }
            else if (active_window == TRANSACTION_HISTORY_WINDOW && selected_transaction > 0)
            {
                selected_transaction -= amount;
                selected_transaction = MAX(selected_transaction, 0);
            }
            else if (active_window == SUBSCRIPTIONS_WINDOW && selected_subscription > 0)
            {
                selected_subscription -= amount;
                selected_subscription = MAX(selected_subscription, 0);
            }
            else
            {
                active_window = (active_window - (amount % NUM_SELECTABLE_WINDOWS) + NUM_SELECTABLE_WINDOWS) % NUM_SELECTABLE_WINDOWS;
            }
            needs_redraw = true;
            break;
        }

        case '\n': // Enter key
            switch (active_window)
            {
            case ACTIONS_MENU_WINDOW:
                // Handle action menu selection
                switch (highlighted_action)
                {
                case 0: // Add Expense
                    add_expense_dialog();
                    needs_redraw = true;
                    break;

                case 1: // Remove Transaction
                    remove_transaction_dialog();
                    needs_redraw = true;
                    break;

                case 2: // Add Subscription
                    add_subscription_dialog();
                    update_subscriptions();
                    needs_redraw = true;
                    break;

                case 4: // Export to CSV
                {
                    const char *export_msg[] = {"This may take a while..."};
                    BoundedWindow alert = draw_alert("Export to CSV", export_msg, 1);
                    doupdate();

                    // // Export the data
                    // export_data_to_csv(0);

                    // Show the alert
                    char export_path[MAX_BUFFER];
                    sprintf(export_path, "Export doesn't work but if it did, it would be exported to: %.30s", export_file_path);
                    const char *export_path_msg[] = {export_path};
                    draw_alert_persistent("Export to CSV", export_path_msg, 1);
                    delete_bounded(alert);
                    needs_redraw = true;
                    break;
                }
                case 5: // Previous Month
                    current_month--;
                    if (current_month < 1)
                    {
                        current_month = 12;
                        current_year--;
                    }
                    needs_redraw = true;
                    break;
                case 6: // Next Month
                    if (current_month == today_month && current_year == today_year)
                    {
                        break;
                    }
                    current_month++;
                    if (current_month > 12)
                    {
                        current_month = 1;
                        current_year++;
                    }
                    needs_redraw = true;
                    break;
                case 7: // Exit Dashboard
                    // Clean up layout
                    if (main_layout != NULL)
                    {
                        free_flex_layout(main_layout);
                    }
                    delete_bounded_array(all_windows, NUM_WINDOWS);
                    return 0;
                }
                break;
            case BUDGET_SUMMARY_WINDOW:
                budget_summary_dialog();
                needs_redraw = true;
                break;
            case SUBSCRIPTIONS_WINDOW:
                remove_subscription_dialog(selected_subscription);
                needs_redraw = true;
                break;
            }
            break;
        case KEY_RESIZE:
            needs_redraw = true;
            break;
        case '+':
            switch (active_window)
            {
            case SUBSCRIPTIONS_WINDOW:
                add_subscription_dialog();
                update_subscriptions();
                needs_redraw = true;
                break;
            }
            break;
        }
        memset(count_buffer, 0, sizeof(count_buffer));
        count_buffer_pos = 0;
    }
}

int save_and_exit()
{
    // bool success = true;
    // int res = save_data_to_file();
    // if (res < 0)
    // {
    //     fprintf(stderr, "Failed to save data to file: %d\n", res);
    //     success = false;
    // }
    int res = cleanup_file_cache();
    if (res < 0)
    {
        fprintf(stderr, "Failed to cleanup file cache: %d\n", res);
        // success = false;
    }
    if (main_layout != NULL)
    {
        free_flex_layout(main_layout);
    }
    cleanup_transactions();
    cleanup_ncurses();
    curs_set(1);
    return 0;
}