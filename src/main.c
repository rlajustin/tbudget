#include "main.h"
#include <locale.h>

// Main function
int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    int mode = MODE_MENU; // Default mode

    // Parse command line arguments
    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[1], "--menu") == 0 || strcmp(argv[1], "-m") == 0)
        {
            mode = MODE_MENU;
        }
        else if (strcmp(argv[1], "--dashboard") == 0 || strcmp(argv[1], "-d") == 0)
        {
            mode = MODE_DASHBOARD;
        }
        else if (strcmp(argv[1], "--export") == 0 || strcmp(argv[1], "-e") == 0)
        {
            // Export data to CSV and exit
            setup_ncurses(); // Need to initialize for proper cleanup
            load_data_from_file();
            cleanup_ncurses();
            export_data_to_csv(0); // Not silent, show message
            printf("Data exported to %s\n", export_file_path);
            return 0;
        }
        else if (strcmp(argv[1], "--history") == 0 || strcmp(argv[1], "-l") == 0)
        {
            // List export history by directly using system command
            printf("Export history files in '%s' directory:\n\n", data_storage_dir);
            char mkdir_cmd[MAX_BUFFER];
            sprintf(mkdir_cmd, "mkdir -p %s", data_storage_dir);
            system(mkdir_cmd);

            char ls_cmd[MAX_BUFFER];
            sprintf(ls_cmd, "ls -lt %s", data_storage_dir);
            system(ls_cmd);

            printf("\nTo use a specific export file, import it with:\n");
            printf("  %s --import %s/filename.csv\n", argv[0], data_storage_dir);
            return 0;
        }
        else if (strcmp(argv[1], "--import") == 0 || strcmp(argv[1], "-i") == 0)
        {
            // Import data from CSV
            if (argc < 3)
            {
                fprintf(stderr, "Error: Import requires a filename\n");
                fprintf(stderr, "Usage: %s --import <filename>\n", argv[0]);
                return 1;
            }
            setup_ncurses(); // Need to initialize for proper cleanup
            import_data_from_csv(argv[2]);
            save_data_to_file();
            cleanup_ncurses();
            printf("Data imported from %s\n", argv[2]);
            return 0;
        }
        else
        {
            // Try to parse as a number
            char *endptr;
            long val = strtol(argv[1], &endptr, 10);

            if (*endptr == '\0')
            { // Successful conversion
                if (val == MODE_MENU)
                {
                    mode = MODE_MENU;
                }
                else if (val == MODE_DASHBOARD)
                {
                    mode = MODE_DASHBOARD;
                }
                else
                {
                    fprintf(stderr, "Invalid mode: %ld\n", val);
                    print_usage(argv[0]);
                    return 1;
                }
            }
            else
            {
                fprintf(stderr, "Invalid argument: %s\n", argv[1]);
                print_usage(argv[0]);
                return 1;
            }
        }
    }
    else
    {
        print_usage(argv[0]);
        return 1;
    }

    initialize_data_directories();
    int load_result = load_data_from_file();
    if (load_result < 0)
    {
        fprintf(stderr, "Error: Failed to load data from file: %d\n", load_result);
        return 1;
    }

    sort_categories_by_budget();
    compute_monthly();

    // Initialize data directories
    setup_ncurses();

    // Load data from file at startup

    // Run the appropriate mode
    if (mode == MODE_MENU)
    {
        run_menu_mode();
    }
    else
    { // mode == MODE_DASHBOARD
        run_dashboard_mode();
    }

    curs_set(1);
    cleanup_ncurses();
    save_data_to_file();

    return 0;
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

// The original menu-based mode
void run_menu_mode()
{
    const char *main_menu[] = {
        "1. Do nothing",
        "2. Go dashboard mode",
        "3. Exit"};
    int menu_size = sizeof(main_menu) / sizeof(main_menu[0]);

    while (1)
    {
        clear();
        draw_title(stdscr, "TBudget - Budget Management Tool");
        char *month = get_month_name();
        mvprintw(5, 2, "%s Budget: $%.2f", month, total_budget);

        int choice = get_menu_choice(stdscr, main_menu, menu_size, 10, 0);

        if (choice == -1)
        {
            return; // Escape or unexpected input
        }

        switch (choice)
        {
        case 0: // Budget Setup (index 0 corresponds to option 1)
            break;
        case 1: // Manage Transactions
            run_dashboard_mode();
            break;
        case 2: // Exit
            return;
        }
    }
}

// New dashboard mode that shows everything at once with flexible boxes
void run_dashboard_mode()
{
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
        "Subscriptions",
        "TODO"};

    // Prepare action menu
    const char *action_menu[] = {
        "Add Expense",
        "Remove Transaction",
        "Export to CSV",
        "Exit Dashboard"};
    int action_menu_size = sizeof(action_menu) / sizeof(action_menu[0]);
    int highlighted_action = 0;

    // Turn off cursor
    curs_set(0);
    clear();
    draw_title(win, "tbudget Dashboard - Budget Management Tool");

    bool needs_redraw = true;
    bool is_leaving = false;
    bool show_pie_chart = true; // Flag to toggle between table and pie chart view

    // Create flex layout containers
    FlexContainer *main_layout = NULL;
    FlexContainer *top_row = NULL;
    FlexContainer *bottom_row = NULL;

    int active_window = 0; // Start with actions menu (window 0)

    // Main dashboard loop
    while (1)
    {
        if (needs_redraw)
        {
            // Recalculate sizes in case of window resize
            getmaxyx(win, max_y, max_x);

            // Clear the main window/background completely
            wclear(win);

            // Redraw the title
            draw_title(win, "tbudget Dashboard - Budget Management Tool");

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
            char *help_text = is_leaving ? "Exiting tbudget, press Q again to confirm" : "TAB/Shift+TAB or ARROW KEYS to switch windows | ENTER to select | Q to quit";

            // Display help line at the bottom
            mvwhline(win, max_y - 1, 0, ' ', max_x); // Clear the line first
            mvwprintw(win, max_y - 1, (max_x - strlen(help_text)) / 2, "%s", help_text);

            char *month = get_month_name();
            // Display budget summary
            mvwprintw(budget_win.textbox, 1, 2, "%s Budget: $%.2f", month, total_budget);
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
            if (transaction_count > 0)
            {
                display_transactions(trans_win.textbox, 1);
            }
            else
            {
                mvwprintw(trans_win.textbox, 1, 2, "No transactions recorded yet.");
                mvwprintw(trans_win.textbox, 2, 2, "Select 'Add Transaction' from the Actions menu.");
            }

            // Display action menu
            for (int i = 0; i < action_menu_size; i++)
            {
                if (active_window == ACTIONS_MENU_WINDOW && i == highlighted_action)
                {
                    wattron(action_win.textbox, COLOR_PAIR(4));
                    mvwprintw(action_win.textbox, 2 + i, 2, "%d. %s", i + 1, action_menu[i]);
                    wattroff(action_win.textbox, COLOR_PAIR(4));
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
            if (is_leaving)
            {
                // Clean up layout
                if (main_layout != NULL)
                {
                    free_flex_layout(main_layout);
                }
                delete_bounded_array(all_windows, NUM_WINDOWS);
                return;
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
                continue;
            }
        }

        // Handle toggle between pie chart and table view
        if (ch == 'p' || ch == 'P')
        {
            show_pie_chart = !show_pie_chart;
            needs_redraw = true;
            continue;
        }

        // Handle window selection
        switch (ch)
        {
        case '\t': // Tab key
        case KEY_RIGHT:
            active_window = (active_window + 1) % NUM_SELECTABLE_WINDOWS;
            needs_redraw = true;
            break;

        case KEY_BTAB: // Shift+Tab
        case KEY_LEFT:
            active_window = (active_window + NUM_SELECTABLE_WINDOWS - 1) % NUM_SELECTABLE_WINDOWS;
            needs_redraw = true;
            break;

        case 'j':
        case KEY_DOWN:
            if (active_window == ACTIONS_MENU_WINDOW && highlighted_action < action_menu_size - 1)
            {
                highlighted_action++;
                needs_redraw = true;
            }
            break;

        case 'k':
        case KEY_UP:
            if (active_window == ACTIONS_MENU_WINDOW && highlighted_action > 0)
            {
                highlighted_action--;
                needs_redraw = true;
            }
            break;

        case '\n': // Enter key
            switch (active_window)
            {
            case ACTIONS_MENU_WINDOW:
                // Handle action menu selection
                switch (highlighted_action)
                {
                case 0: // Add Expense
                    add_expense_dialog();
                    compute_monthly();
                    needs_redraw = true;
                    break;

                case 1: // Remove Transaction
                    remove_transaction_dialog();
                    compute_monthly();
                    needs_redraw = true;
                    break;

                case 2: // Export to CSV
                {
                    const char *export_msg[] = {"This may take a while..."};
                    BoundedWindow alert = draw_alert("Export to CSV", export_msg, 1);
                    doupdate();

                    // Export the data
                    export_data_to_csv(0);

                    // Show the alert
                    char export_path[MAX_BUFFER];
                    sprintf(export_path, "Export complete! Exported to: %.30s", export_file_path);
                    const char *export_path_msg[] = {export_path};
                    draw_alert_persistent("Export to CSV", export_path_msg, 1);
                    delete_bounded(alert);
                    needs_redraw = true;
                    break;
                }

                case 3: // Exit Dashboard
                    // Clean up layout
                    if (main_layout != NULL)
                    {
                        free_flex_layout(main_layout);
                    }
                    delete_bounded_array(all_windows, NUM_WINDOWS);
                    return;
                }
                break;
            case BUDGET_SUMMARY_WINDOW:
                budget_summary_dialog();
                needs_redraw = true;
                break;
            }
            break;
        case KEY_RESIZE:
            needs_redraw = true;
            break;
        }
    }
}
