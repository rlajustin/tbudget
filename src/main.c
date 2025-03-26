#include "main.h"

// Main function
int main(int argc, char *argv[])
{
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

        mvprintw(5, 2, "Total Budget: $%.2f", total_budget);

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

    // Calculate window sizes
    int budget_height = max_y / 2;
    int trans_height = max_y - budget_height - 1; // -1 for title bar and bottom bar
    int action_width = max_x / 3;

    // Define active section (0 = budget, 1 = transactions, 2 = actions)
    int active_section = 2; // Start with actions menu

    // Create sub-windows
    BoundedWindow budget_win, trans_win, action_win;

    // Prepare action menu
    const char *action_menu[] = {
        "Add Category",
        "Remove Category",
        "Add Transaction",
        "Remove Transaction",
        "Set Total Budget",
        "Export to CSV",
        "Exit Dashboard"};
    int action_menu_size = sizeof(action_menu) / sizeof(action_menu[0]);
    int highlighted_action = 0;

    // Turn off cursor
    curs_set(0);
    clear();
    draw_title(win, "TBudget Dashboard - Budget Management Tool");

    bool needs_redraw = true;
    bool is_leaving = false;

    // Main dashboard loop
    while (1)
    {
        if (needs_redraw)
        {
            // Recalculate sizes in case of window resize
            getmaxyx(win, max_y, max_x);
            budget_height = max_y / 2;
            trans_height = max_y - budget_height - 2;
            action_width = max_x / 3;

            delete_bounded(budget_win);
            delete_bounded(trans_win);
            delete_bounded(action_win);

            budget_win = draw_bounded_with_title(budget_height - 4, max_x - action_width - 4, 3, 2, "Budget Summary", active_section == 0, ALIGN_LEFT);
            trans_win = draw_bounded_with_title(trans_height - 4, max_x - action_width - 4, budget_height + 3, 2, "Transaction History", active_section == 1, ALIGN_LEFT);
            action_win = draw_bounded_with_title(max_y - 6, action_width - 4, 3, max_x - action_width + 2, "Actions", active_section == 2, ALIGN_LEFT);

            // Key help line
            char *help_text = is_leaving ? "Exiting TBudget, press Q again to confirm" : "TAB to switch sections | ENTER to select | Q to quit";

            // Adjust window sizes and positions
            // bwresize(budget_win, budget_height, max_x - action_width);
            // bwresize(trans_win, trans_height, max_x - action_width);
            // bwresize(action_win, max_y - 1, action_width);
            // bwmove(trans_win, budget_height + 1, 0);
            // bwmove(action_win, 1, max_x - action_widtfh);

            // Display help line at the bottom
            mvwhline(win, max_y - 1, 0, ' ', max_x); // Clear the line first
            mvprintw(max_y - 1, (max_x - strlen(help_text)) / 2, "%s", help_text);

            // Display budget summary
            mvwprintw(budget_win.textbox, 1, 2, "Total Budget: $%.2f", total_budget);
            if (category_count > 0)
            {
                display_categories(budget_win.textbox, 3);
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
                if (active_section == 2 && i == highlighted_action)
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
            bwnoutrefresh(budget_win);
            bwnoutrefresh(trans_win);
            bwnoutrefresh(action_win);
            doupdate();
            needs_redraw = false;
        }

        // Get user input
        ch = getch();

        if (ch == 'q' || ch == 'Q')
        {
            if (is_leaving)
            {
                delete_bounded(budget_win);
                delete_bounded(trans_win);
                delete_bounded(action_win);
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

        // Handle navigation between sections
        switch (ch)
        {
        case KEY_RIGHT:
        case 'l':
        case '\t': // Tab key
            active_section = (active_section + 1) % 3;
            needs_redraw = true;
            break;

        case KEY_LEFT:
        case 'h':
        case KEY_BTAB: // Shift+Tab
            active_section = (active_section + 2) % 3;
            needs_redraw = true;
            break;

        case 'j':
        case KEY_DOWN:
            if (active_section == 2 && highlighted_action < action_menu_size - 1)
            {
                highlighted_action++;
                needs_redraw = true;
            }
            else
            {
                active_section = (active_section + 1) % 3;
                needs_redraw = true;
            }
            break;

        case 'k':
        case KEY_UP:
            if (active_section == 2 && highlighted_action > 0)
            {
                highlighted_action--;
                needs_redraw = true;
            }
            else
            {
                active_section = (active_section + 2) % 3;
                needs_redraw = true;
            }
            break;

        case '\n': // Enter key
            if (active_section == 2)
            {
                // Handle action menu selection
                switch (highlighted_action)
                {
                case 0:
                    add_category_dialog();
                    needs_redraw = true;
                    break;

                case 1: // Remove Category
                    remove_category_dialog();
                    needs_redraw = true;
                    break;

                case 2: // Add Transaction
                    add_transaction_dialog();
                    needs_redraw = true;
                    break;

                case 3: // Remove Transaction
                    if (transaction_count > 0)
                    {
                        remove_transaction_dialog();
                        needs_redraw = true;
                    }
                    else
                    {
                        draw_error(action_win, "No transactions to remove.");
                    }
                    break;

                case 4: // Set Total Budget
                    set_budget_dialog();
                    needs_redraw = true;
                    break;

                case 5: // Export to CSV
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

                case 6: // Exit Dashboard
                    delete_bounded(budget_win);
                    delete_bounded(trans_win);
                    delete_bounded(action_win);
                    return;
                }
            }
            break;
        case KEY_RESIZE:
            needs_redraw = true;
            break;
        }
    }
}