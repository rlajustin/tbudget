#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h> // For timestamps in export filenames
#include <sys/stat.h> // For mkdir
#include <errno.h>    // For errno
#include <unistd.h>   // For getenv, access
#include <pwd.h>      // For getpwuid
#include "main.h"
#include "utils.h"

// Global variable definitions
Category categories[MAX_CATEGORIES];
int category_count = 0;
Transaction transactions[MAX_TRANSACTIONS];
int transaction_count = 0;
double total_budget = 0.0;

// Global file paths
char app_data_dir[MAX_BUFFER];
char data_storage_dir[MAX_BUFFER];
char data_file_path[MAX_BUFFER];
char export_file_path[MAX_BUFFER];

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

    // Initialize data directories
    initialize_data_directories();

    setup_ncurses();

    // Load data from file at startup
    load_data_from_file();

    // Run the appropriate mode
    if (mode == MODE_MENU)
    {
        run_menu_mode();
    }
    else
    { // mode == MODE_DASHBOARD
        run_dashboard_mode();
    }

    cleanup_ncurses();

    // Save data to file before exit
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
        "1. Budget Setup",
        "2. Manage Transactions",
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
            budget_setup();
            break;
        case 1: // Manage Transactions
            manage_transactions();
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
    int trans_height = max_y - budget_height - 1; // -1 for title bar
    int action_width = max_x / 3;

    // Create sub-windows
    WINDOW *budget_win = newwin(budget_height, max_x - action_width, 1, 0);
    WINDOW *trans_win = newwin(trans_height, max_x - action_width, budget_height + 1, 0);
    WINDOW *action_win = newwin(max_y - 1, action_width, 1, max_x - action_width);

    // Define active section (0 = budget, 1 = transactions, 2 = actions)
    int active_section = 2; // Start with actions menu
    
    // Prepare action menu
    const char *action_menu[] = {
        "Add Category",
        "Remove Category",
        "Add Transaction",
        "Remove Transaction",
        "Set Total Budget",
        "Export to CSV",
        "Exit Dashboard"
    };
    int action_menu_size = sizeof(action_menu) / sizeof(action_menu[0]);
    int highlighted_action = 0;

    // Key help line
    const char *help_text = "TAB to switch sections | ENTER to select | Q to quit";
    
    // Turn off cursor
    curs_set(0);
    
    // Initial draw of the screen
        clear();
        draw_title(win, "TBudget Dashboard - Budget Management Tool");
    
    // Calculate sizes in case of window resize
    getmaxyx(win, max_y, max_x);
    budget_height = max_y / 2;
    trans_height = max_y - budget_height - 1;
    action_width = max_x / 3;
    
    // Adjust window sizes and positions
    wresize(budget_win, budget_height, max_x - action_width);
    wresize(trans_win, trans_height, max_x - action_width);
    wresize(action_win, max_y - 1, action_width);
    mvwin(trans_win, budget_height + 1, 0);
    mvwin(action_win, 1, max_x - action_width);

        // Display help line at the bottom
        mvprintw(max_y - 1, (max_x - strlen(help_text)) / 2, "%s", help_text);

        // Clear sub-windows
        wclear(budget_win);
        wclear(trans_win);
    wclear(action_win);

    // Draw borders with highlighted active section
    if (active_section == 0) {
        wattron(budget_win, A_BOLD | COLOR_PAIR(4));
    }
        box(budget_win, 0, 0);
    if (active_section == 0) {
        wattroff(budget_win, A_BOLD | COLOR_PAIR(4));
    }
    
    if (active_section == 1) {
        wattron(trans_win, A_BOLD | COLOR_PAIR(4));
    }
        box(trans_win, 0, 0);
    if (active_section == 1) {
        wattroff(trans_win, A_BOLD | COLOR_PAIR(4));
    }
    
    if (active_section == 2) {
        wattron(action_win, A_BOLD | COLOR_PAIR(4));
    }
    box(action_win, 0, 0);
    if (active_section == 2) {
        wattroff(action_win, A_BOLD | COLOR_PAIR(4));
    }

        // Draw window titles
        mvwprintw(budget_win, 0, 2, " Budget Summary ");
        mvwprintw(trans_win, 0, 2, " Transaction History ");
    mvwprintw(action_win, 0, 2, " Actions ");

        // Display budget summary
        mvwprintw(budget_win, 1, 2, "Total Budget: $%.2f", total_budget);
    if (category_count > 0) {
            display_categories(budget_win, 3);
    } else {
            mvwprintw(budget_win, 3, 2, "No budget categories defined yet.");
        mvwprintw(budget_win, 4, 2, "Select 'Add Category' from the Actions menu.");
        }

        // Display transaction history
    if (transaction_count > 0) {
            display_transactions(trans_win, 1);
    } else {
            mvwprintw(trans_win, 1, 2, "No transactions recorded yet.");
        mvwprintw(trans_win, 2, 2, "Select 'Add Transaction' from the Actions menu.");
    }

    // Display action menu
    for (int i = 0; i < action_menu_size; i++) {
        if (active_section == 2 && i == highlighted_action) {
            wattron(action_win, COLOR_PAIR(4));
            mvwprintw(action_win, 2 + i, 2, "%s", action_menu[i]);
            wattroff(action_win, COLOR_PAIR(4));
        } else {
            mvwprintw(action_win, 2 + i, 2, "%s", action_menu[i]);
        }
    }

    // Initial refresh of windows
    wnoutrefresh(budget_win);
    wnoutrefresh(trans_win);
    wnoutrefresh(action_win);
    wnoutrefresh(win);
    // Do a single update of the physical screen - reduces flicker
    doupdate();

    // Main dashboard loop
    bool needs_redraw = false;
    
    while (1) {
        // Get user input
        ch = getch();
        needs_redraw = false;

        // Handle navigation between sections
        switch (ch) {
            case '\t':  // Tab key
                active_section = (active_section + 1) % 3;
                needs_redraw = true;
                break;
                
            case KEY_BTAB:  // Shift+Tab
                active_section = (active_section + 2) % 3;
                needs_redraw = true;
                break;
                
            case 'j':
            case 'J':
            case KEY_DOWN:
                if (active_section == 2 && highlighted_action < action_menu_size - 1) {
                    highlighted_action++;
                    needs_redraw = true;
                }
                break;
                
            case 'k':
            case 'K':
            case KEY_UP:
                if (active_section == 2 && highlighted_action > 0) {
                    highlighted_action--;
                    needs_redraw = true;
                }
                break;
                
            case '\n':  // Enter key
                if (active_section == 2) {
                    // Handle action menu selection
                    switch (highlighted_action) {
                        case 0:  // Add Category
            delwin(budget_win);
            delwin(trans_win);
                            delwin(action_win);
                            
                            // Call a modified version of the category add functionality
                            add_category_dialog();
                            
                            // Recreate windows
                            budget_win = newwin(budget_height, max_x - action_width, 1, 0);
                            trans_win = newwin(trans_height, max_x - action_width, budget_height + 1, 0);
                            action_win = newwin(max_y - 1, action_width, 1, max_x - action_width);
                            needs_redraw = true;
                            break;
                            
                        case 1:  // Remove Category
                            if (category_count > 0) {
            delwin(budget_win);
            delwin(trans_win);
                                delwin(action_win);
                                
                                // Call a modified version of the category remove functionality
                                remove_category_dialog();
                                
                                // Recreate windows
                                budget_win = newwin(budget_height, max_x - action_width, 1, 0);
                                trans_win = newwin(trans_height, max_x - action_width, budget_height + 1, 0);
                                action_win = newwin(max_y - 1, action_width, 1, max_x - action_width);
                                needs_redraw = true;
                            }
            break;

                        case 2:  // Add Transaction
                            if (category_count > 0) {
            delwin(budget_win);
            delwin(trans_win);
                                delwin(action_win);
                                
                                // Call a modified version of the transaction add functionality
                                add_transaction_dialog();
                                
                                // Recreate windows
                                budget_win = newwin(budget_height, max_x - action_width, 1, 0);
                                trans_win = newwin(trans_height, max_x - action_width, budget_height + 1, 0);
                                action_win = newwin(max_y - 1, action_width, 1, max_x - action_width);
                                needs_redraw = true;
                            } else {
                                // Show error if no categories defined
                                wclear(action_win);
                                box(action_win, 0, 0);
                                mvwprintw(action_win, 0, 2, " Error ");
                                mvwprintw(action_win, 2, 2, "Please create categories first.");
                                mvwprintw(action_win, 3, 2, "Press any key to continue...");
                                wnoutrefresh(action_win);
                                doupdate();
                getch();
                                needs_redraw = true;
                            }
                            break;
                            
                        case 3:  // Remove Transaction
                            if (transaction_count > 0) {
                                delwin(budget_win);
                                delwin(trans_win);
                                delwin(action_win);
                                
                                // Call the transaction remove functionality
                                remove_transaction_dialog();
                                
                                // Recreate windows
                                budget_win = newwin(budget_height, max_x - action_width, 1, 0);
                                trans_win = newwin(trans_height, max_x - action_width, budget_height + 1, 0);
                                action_win = newwin(max_y - 1, action_width, 1, max_x - action_width);
                                needs_redraw = true;
                            } else {
                                // Show error if no transactions defined
                                wclear(action_win);
                                box(action_win, 0, 0);
                                mvwprintw(action_win, 0, 2, " Error ");
                                mvwprintw(action_win, 2, 2, "No transactions to remove.");
                                mvwprintw(action_win, 3, 2, "Press any key to continue...");
                                wnoutrefresh(action_win);
                                doupdate();
                                getch();
                                needs_redraw = true;
                            }
                            break;
                            
                        case 4:  // Set Total Budget
                            delwin(budget_win);
                            delwin(trans_win);
                            delwin(action_win);
                            
                            // Call a modified version of setting the total budget
                            set_budget_dialog();

            // Recreate windows
                            budget_win = newwin(budget_height, max_x - action_width, 1, 0);
                            trans_win = newwin(trans_height, max_x - action_width, budget_height + 1, 0);
                            action_win = newwin(max_y - 1, action_width, 1, max_x - action_width);
                            needs_redraw = true;
                            break;
                            
                        case 5:  // Export to CSV
                            wclear(action_win);
                            box(action_win, 0, 0);
                            mvwprintw(action_win, 0, 2, " Export ");
                            mvwprintw(action_win, 2, 2, "Exporting data...");
                            wnoutrefresh(action_win);
                            doupdate();
                            
                            // Export the data
                            export_data_to_csv(0);
                            
                            mvwprintw(action_win, 3, 2, "Data exported to:");
                            mvwprintw(action_win, 4, 2, "%.30s", export_file_path);
                            mvwprintw(action_win, 6, 2, "Press any key to continue...");
                            wnoutrefresh(action_win);
                            doupdate();
                            getch();
                            needs_redraw = true;
                            break;
                            
                        case 6:  // Exit Dashboard
                            delwin(budget_win);
                            delwin(trans_win);
                            delwin(action_win);
                            return;
                    }
                }
                break;
                
            case 'q':
            case 'Q':
                // Exit dashboard
                delwin(budget_win);
                delwin(trans_win);
                delwin(action_win);
                return;
                
            case KEY_RESIZE:
                // Handle terminal resize
            getmaxyx(win, max_y, max_x);
            budget_height = max_y / 2;
            trans_height = max_y - budget_height - 1;
                action_width = max_x / 3;
                needs_redraw = true;
            break;
        }
        
        // Only redraw if something has changed
        if (needs_redraw) {
            // Recalculate sizes in case of window resize
            getmaxyx(win, max_y, max_x);
            budget_height = max_y / 2;
            trans_height = max_y - budget_height - 1;
            action_width = max_x / 3;
            
            // Adjust window sizes and positions
            wresize(budget_win, budget_height, max_x - action_width);
            wresize(trans_win, trans_height, max_x - action_width);
            wresize(action_win, max_y - 1, action_width);
            mvwin(trans_win, budget_height + 1, 0);
            mvwin(action_win, 1, max_x - action_width);
            
            // Display help line at the bottom
            mvwhline(win, max_y - 1, 0, ' ', max_x);  // Clear the line first
            mvprintw(max_y - 1, (max_x - strlen(help_text)) / 2, "%s", help_text);
            
            // Clear sub-windows
            wclear(budget_win);
            wclear(trans_win);
            wclear(action_win);
            
            // Draw borders with highlighted active section
            if (active_section == 0) {
                wattron(budget_win, A_BOLD | COLOR_PAIR(4));
            }
            box(budget_win, 0, 0);
            if (active_section == 0) {
                wattroff(budget_win, A_BOLD | COLOR_PAIR(4));
            }
            
            if (active_section == 1) {
                wattron(trans_win, A_BOLD | COLOR_PAIR(4));
            }
            box(trans_win, 0, 0);
            if (active_section == 1) {
                wattroff(trans_win, A_BOLD | COLOR_PAIR(4));
            }
            
            if (active_section == 2) {
                wattron(action_win, A_BOLD | COLOR_PAIR(4));
            }
            box(action_win, 0, 0);
            if (active_section == 2) {
                wattroff(action_win, A_BOLD | COLOR_PAIR(4));
            }
            
            // Draw window titles
            mvwprintw(budget_win, 0, 2, " Budget Summary ");
            mvwprintw(trans_win, 0, 2, " Transaction History ");
            mvwprintw(action_win, 0, 2, " Actions ");
            
            // Display budget summary
            mvwprintw(budget_win, 1, 2, "Total Budget: $%.2f", total_budget);
            if (category_count > 0) {
                display_categories(budget_win, 3);
            } else {
                mvwprintw(budget_win, 3, 2, "No budget categories defined yet.");
                mvwprintw(budget_win, 4, 2, "Select 'Add Category' from the Actions menu.");
            }
            
            // Display transaction history
            if (transaction_count > 0) {
                display_transactions(trans_win, 1);
            } else {
                mvwprintw(trans_win, 1, 2, "No transactions recorded yet.");
                mvwprintw(trans_win, 2, 2, "Select 'Add Transaction' from the Actions menu.");
            }
            
            // Display action menu
            for (int i = 0; i < action_menu_size; i++) {
                if (active_section == 2 && i == highlighted_action) {
                    wattron(action_win, COLOR_PAIR(4));
                    mvwprintw(action_win, 2 + i, 2, "%s", action_menu[i]);
                    wattroff(action_win, COLOR_PAIR(4));
                } else {
                    mvwprintw(action_win, 2 + i, 2, "%s", action_menu[i]);
                }
            }
            
            // Refresh windows
            wnoutrefresh(budget_win);
            wnoutrefresh(trans_win);
            wnoutrefresh(action_win);
            wnoutrefresh(win);
            // Do a single update of the physical screen - reduces flicker
            doupdate();
        }
    }
}

// Helper function for adding a category in dashboard mode
void add_category_dialog() {
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    
    // Create a dialog box centered on screen
    int dialog_height = 9;
    int dialog_width = 60;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;
    
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 13) / 2, " Add Category ");
    
    if (category_count >= MAX_CATEGORIES) {
        mvwprintw(dialog, 2, 2, "Maximum number of categories reached.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wnoutrefresh(dialog);
        doupdate();
        getch();
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }
    
    // Get category name
    mvwprintw(dialog, 2, 2, "Enter category name: ");
    mvwprintw(dialog, 3, 2, " ");
    wnoutrefresh(dialog);
    doupdate();
    
    if (!get_string_input(dialog, categories[category_count].name, MAX_NAME_LEN, 2, 22)) {
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return; // User canceled
    }
    
    // Get amount
    mvwprintw(dialog, 5, 2, "Enter allocated amount: ");
    mvwprintw(dialog, 6, 2, "$");
    wnoutrefresh(dialog);
    doupdate();
    
    double amount = get_double_input(dialog, 6, 4);
    
    // Check if input was canceled
    if (amount == -1.0) {
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }
    
    categories[category_count].amount = amount;
    category_count++;
    save_data_to_file(); // Save after adding category
    
    mvwprintw(dialog, 7, 2, "Category added successfully. Press any key to continue...");
    wnoutrefresh(dialog);
    doupdate();
    getch();
    
    delwin(dialog);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
}

// Helper function for removing a category in dashboard mode
void remove_category_dialog() {
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    
    if (category_count == 0) {
        // Create a simple error dialog
        int dialog_height = 5;
        int dialog_width = 40;
        int start_y = (max_y - dialog_height) / 2;
        int start_x = (max_x - dialog_width) / 2;
        
        WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
        box(dialog, 0, 0);
        mvwprintw(dialog, 0, (dialog_width - 16) / 2, " Remove Category ");
        mvwprintw(dialog, 2, 2, "No categories to remove.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wnoutrefresh(dialog);
        doupdate();
        getch();
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }
    
    // Calculate dialog size based on number of categories
    int dialog_height = category_count + 8;
    int dialog_width = 60;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;
    
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 16) / 2, " Remove Category ");
    
    // Create dynamic menu for categories
    char **category_menu = malloc(category_count * sizeof(char *));
    if (category_menu == NULL) {
        mvwprintw(dialog, 2, 2, "Memory allocation error.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wnoutrefresh(dialog);
        doupdate();
        getch();
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }
    
    for (int i = 0; i < category_count; i++) {
        category_menu[i] = malloc(MAX_NAME_LEN + 20); // Extra space for number and amount
        if (category_menu[i] == NULL) {
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(category_menu[j]);
            }
            free(category_menu);
            
            mvwprintw(dialog, 2, 2, "Memory allocation error.");
            mvwprintw(dialog, 3, 2, "Press any key to continue...");
            wnoutrefresh(dialog);
            doupdate();
            getch();
            delwin(dialog);
            touchwin(stdscr);
            wnoutrefresh(stdscr);
            doupdate();
            return;
        }
        sprintf(category_menu[i], "%d. %s ($%.2f)", i + 1, categories[i].name, categories[i].amount);
    }
    
    mvwprintw(dialog, 2, 2, "Select a category to remove:");
    wnoutrefresh(dialog);
    doupdate();
    
    int cat_choice = get_menu_choice(dialog, (const char **)category_menu, category_count, 4, 1);
    
    // Free allocated memory
    for (int i = 0; i < category_count; i++) {
        free(category_menu[i]);
    }
    free(category_menu);
    
    if (cat_choice == -1) { // User pressed backspace or escape
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }
    
    // Ask for confirmation
    wclear(dialog);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 16) / 2, " Confirm Removal ");
    mvwprintw(dialog, 2, 2, "Are you sure you want to remove \"%s\"?", categories[cat_choice].name);
    mvwprintw(dialog, 3, 2, "Any transactions with this category will be uncategorized.");
    
    const char *confirm_menu[] = {
        "Yes, remove it",
        "No, keep it"
    };
    
    wnoutrefresh(dialog);
    doupdate();
    
    int confirm = get_menu_choice(dialog, confirm_menu, 2, 5, 0);
    
    if (confirm == 0) { // Yes, remove it
        // Check if there are any transactions using this category
        for (int i = 0; i < transaction_count; i++) {
            if (transactions[i].category_id == cat_choice) {
                transactions[i].category_id = -1; // Set to uncategorized
            } else if (transactions[i].category_id > cat_choice) {
                transactions[i].category_id--; // Adjust category IDs for shifted categories
            }
        }
        
        // Remove the category by shifting all categories after it one position back
        for (int i = cat_choice; i < category_count - 1; i++) {
            strcpy(categories[i].name, categories[i + 1].name);
            categories[i].amount = categories[i + 1].amount;
        }
        
        category_count--;
        save_data_to_file(); // Save after removing category
        
        // Show brief confirmation message
        mvwprintw(dialog, 8, 2, "Category removed successfully.");
        wnoutrefresh(dialog);
        doupdate();
        
        // Short delay instead of waiting for key press
        napms(500);
    }
    
    delwin(dialog);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
}

// Helper function for setting budget in dashboard mode
void set_budget_dialog() {
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    
    // Create a dialog box centered on screen
    int dialog_height = 7;
    int dialog_width = 50;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;
    
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 17) / 2, " Set Total Budget ");
    
    mvwprintw(dialog, 2, 2, "Current Budget: $%.2f", total_budget);
    mvwprintw(dialog, 3, 2, "Enter new total budget amount: ");
    mvwprintw(dialog, 4, 2, "$");
    wnoutrefresh(dialog);
    doupdate();
    
    double new_budget = get_double_input(dialog, 4, 4);
    
    // Check if input was canceled
    if (new_budget == -1.0) {
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }
    
    total_budget = new_budget;
    save_data_to_file(); // Save after budget update
    
    mvwprintw(dialog, 5, 2, "Budget updated successfully. Press any key to continue...");
    wnoutrefresh(dialog);
    doupdate();
    getch();
    
    delwin(dialog);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
}

// Helper function for adding a transaction in dashboard mode
void add_transaction_dialog() {
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    
    refresh(); // Ensure base screen is up to date
    
    if (transaction_count >= MAX_TRANSACTIONS) {
        // Create a simple error dialog
        int dialog_height = 5;
        int dialog_width = 40;
        int start_y = (max_y - dialog_height) / 2;
        int start_x = (max_x - dialog_width) / 2;
        
        WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
        box(dialog, 0, 0);
        mvwprintw(dialog, 0, (dialog_width - 17) / 2, " Add Transaction ");
        mvwprintw(dialog, 2, 2, "Maximum transactions reached.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wrefresh(dialog);
        getch();
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    if (category_count == 0) {
        // Create a simple error dialog
        int dialog_height = 5;
        int dialog_width = 40;
        int start_y = (max_y - dialog_height) / 2;
        int start_x = (max_x - dialog_width) / 2;
        
        WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
        box(dialog, 0, 0);
        mvwprintw(dialog, 0, (dialog_width - 17) / 2, " Add Transaction ");
        mvwprintw(dialog, 2, 2, "Please set up categories first.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wrefresh(dialog);
        getch();
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    // Create dialog for transaction input
    int dialog_height = 11;
    int dialog_width = 60;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;
    
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
    keypad(dialog, TRUE);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 17) / 2, " Add Transaction ");
    
    // Temporary transaction data
    Transaction new_transaction;
    memset(&new_transaction, 0, sizeof(Transaction));
    
    // Get transaction description
    mvwprintw(dialog, 2, 2, "Enter transaction description: ");
    mvwprintw(dialog, 3, 2, " ");
    wrefresh(dialog);
    
    if (!get_string_input(dialog, new_transaction.description, MAX_NAME_LEN, 2, 33)) {
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return; // User canceled
    }
    
    // Get amount
    mvwprintw(dialog, 5, 2, "Enter amount: ");
    mvwprintw(dialog, 6, 2, "$");
    wrefresh(dialog);
    
    double amount = get_double_input(dialog, 6, 4);
    
    // Check if input was canceled
    if (amount == -1.0) {
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    new_transaction.amount = amount;
    
    // Get date
    mvwprintw(dialog, 8, 2, "Enter date (DD-MM-YYYY): ");
    // Set default date to empty for now - our function will handle defaults
    char date_buffer[11] = "";
    mvwprintw(dialog, 9, 2, "(Use arrow keys or Type)");
    wrefresh(dialog);
    
    if (!get_date_input(dialog, date_buffer, 8, 29)) {
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return; // User canceled
    }
    
    // Copy the date string (already in YYYY-MM-DD format)
    strncpy(new_transaction.date, date_buffer, 10);
    new_transaction.date[10] = '\0';
    
    // Clean up previous dialog
    werase(dialog);
    wrefresh(dialog);
    delwin(dialog);
    
    // Create a new dialog for category selection
    dialog_height = category_count + 6;
    if (dialog_height > max_y - 4) {
        dialog_height = max_y - 4; // Cap at reasonable size
    }
    
    dialog = newwin(dialog_height, dialog_width, (max_y - dialog_height) / 2, start_x);
    keypad(dialog, TRUE);
    box(dialog, 0, 0);
    
    mvwprintw(dialog, 0, (dialog_width - 18) / 2, " Select Category ");
    
    // Create dynamic menu for categories
    char **category_menu = malloc(category_count * sizeof(char *));
    if (category_menu == NULL) {
        mvwprintw(dialog, 2, 2, "Memory allocation error.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wrefresh(dialog);
        getch();
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    for (int i = 0; i < category_count; i++) {
        category_menu[i] = malloc(MAX_NAME_LEN + 10); // Extra space for number and formatting
        if (category_menu[i] == NULL) {
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(category_menu[j]);
            }
            free(category_menu);
            
            mvwprintw(dialog, 2, 2, "Memory allocation error.");
            mvwprintw(dialog, 3, 2, "Press any key to continue...");
            wrefresh(dialog);
            getch();
            werase(dialog);
            wrefresh(dialog);
            delwin(dialog);
            touchwin(stdscr);
            refresh();
            return;
        }
        sprintf(category_menu[i], "%d. %s", i + 1, categories[i].name);
    }
    
    mvwprintw(dialog, 2, 2, "Select a category for this transaction:");
    wrefresh(dialog);
    
    int cat_choice = get_menu_choice(dialog, (const char **)category_menu, category_count, 4, 1);
    
    // Free allocated memory
    for (int i = 0; i < category_count; i++) {
        free(category_menu[i]);
    }
    free(category_menu);
    
    if (cat_choice == -1) { // User pressed backspace or ESC
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    new_transaction.category_id = cat_choice;
    
    // Find where to insert the new transaction by date (most recent first)
    int insert_pos = 0;
    
    // Since most recent dates are first (sorted in descending order),
    // we need to find the first transaction with a date less than or equal to the new one
    while (insert_pos < transaction_count && 
           strcmp(transactions[insert_pos].date, new_transaction.date) > 0) {
        insert_pos++;
    }
    
    // We want to insert at the beginning of the same-date group
    // We don't need to move further down for same dates
    
    // Make room for the new transaction
    if (insert_pos < transaction_count) {
        // Shift transactions down
        for (int i = transaction_count; i > insert_pos; i--) {
            transactions[i] = transactions[i-1];
        }
    }
    
    // Insert the new transaction
    transactions[insert_pos] = new_transaction;
    transaction_count++;
    save_data_to_file(); // Save after adding transaction
    
    // Show success message
    mvwprintw(dialog, dialog_height - 2, 2, "Transaction added successfully.");
    mvwprintw(dialog, dialog_height - 1, 2, "Press any key to continue...");
    wrefresh(dialog);
    getch();
    
    werase(dialog);
    wrefresh(dialog);
    delwin(dialog);
    touchwin(stdscr);
    refresh();
}

// Add function to remove transactions
// Helper function for removing a transaction in dashboard mode
void remove_transaction_dialog() {
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    
    refresh(); // Ensure base screen is up to date
    
    if (transaction_count == 0) {
        // Create a simple error dialog
        int dialog_height = 5;
        int dialog_width = 40;
        int start_y = (max_y - dialog_height) / 2;
        int start_x = (max_x - dialog_width) / 2;
        
        WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
        box(dialog, 0, 0);
        mvwprintw(dialog, 0, (dialog_width - 18) / 2, " Remove Transaction ");
        mvwprintw(dialog, 2, 2, "No transactions to remove.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wrefresh(dialog);
        getch();
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    // Calculate dialog size based on number of transactions
    // Cap at a reasonable size for many transactions
    int max_display = 15; // Maximum number of transactions to display at once
    int display_count = transaction_count < max_display ? transaction_count : max_display;
    
    int dialog_height = display_count + 8;
    int dialog_width = 70;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;
    
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 18) / 2, " Remove Transaction ");
    
    // Ensure transactions are sorted with most recent first
    // This sort has already been done in display_transactions
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
    
    // Create dynamic menu for transactions
    char **transaction_menu = malloc(transaction_count * sizeof(char *));
    if (transaction_menu == NULL) {
        mvwprintw(dialog, 2, 2, "Memory allocation error.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wrefresh(dialog);
        getch();
        werase(dialog);
        wrefresh(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return;
    }
    
    char prev_date[11] = "";
    
    for (int i = 0; i < transaction_count; i++) {
        transaction_menu[i] = malloc(MAX_NAME_LEN + 50); // Plenty of space for all info
        if (transaction_menu[i] == NULL) {
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(transaction_menu[j]);
            }
            free(transaction_menu);
            
            mvwprintw(dialog, 2, 2, "Memory allocation error.");
            mvwprintw(dialog, 3, 2, "Press any key to continue...");
            wrefresh(dialog);
            getch();
            werase(dialog);
            wrefresh(dialog);
            delwin(dialog);
            touchwin(stdscr);
            refresh();
            return;
        }
        
        char category_name[MAX_NAME_LEN] = "Uncategorized";
        if (transactions[i].category_id >= 0 && transactions[i].category_id < category_count) {
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
        
        // Create a descriptive menu item
        sprintf(transaction_menu[i], "%-10s %-24s $%-9.2f %-24s", 
                display_date, 
                transactions[i].description, 
                transactions[i].amount, 
                category_name);
    }
    
    mvwprintw(dialog, 2, 2, "Select a transaction to remove:");
    wrefresh(dialog);
    
    // Create scrollable menu if there are many transactions
    int start_index = 0;
    int visible_items = display_count;
    int current_highlighted = 0;
    bool redraw = true;
    bool selection_made = false;
    int trans_choice = 0;
    
    keypad(dialog, TRUE); // Enable arrow keys
    
    while (!selection_made) {
        if (redraw) {
            // Display header
            mvwprintw(dialog, 4, 2, "%-10s %-24s %-10s %-24s", 
                      "Date", "Description", "Amount", "Category");
            
            // Display visible transactions
            for (int i = 0; i < visible_items && (i + start_index) < transaction_count; i++) {
                if (i + start_index == current_highlighted) {
                    wattron(dialog, COLOR_PAIR(4));
                }
                
                mvwprintw(dialog, 5 + i, 2, "%s", transaction_menu[i + start_index]);
                
                if (i + start_index == current_highlighted) {
                    wattroff(dialog, COLOR_PAIR(4));
                }
            }
            
            // Add scroll indicators if needed
            if (start_index > 0) {
                mvwprintw(dialog, 4, dialog_width - 3, "↑");
            } else {
                mvwprintw(dialog, 4, dialog_width - 3, " ");
            }
            
            if (start_index + visible_items < transaction_count) {
                mvwprintw(dialog, 5 + visible_items - 1, dialog_width - 3, "↓");
            } else {
                mvwprintw(dialog, 5 + visible_items - 1, dialog_width - 3, " ");
            }
            
            wrefresh(dialog);
            redraw = false;
        }
        
        int ch = wgetch(dialog);
        
        switch (ch) {
            case KEY_UP:
                if (current_highlighted > 0) {
                    current_highlighted--;
                    if (current_highlighted < start_index) {
                        start_index = current_highlighted;
                    }
                    redraw = true;
                }
                break;
                
            case KEY_DOWN:
                if (current_highlighted < transaction_count - 1) {
                    current_highlighted++;
                    if (current_highlighted >= start_index + visible_items) {
                        start_index = current_highlighted - visible_items + 1;
                    }
                    redraw = true;
                }
                break;
                
            case '\n': // Enter key - proceed to confirmation
                selection_made = true;
                trans_choice = current_highlighted;
                break;
                
            case 27: // ASCII ESC key
                // Free allocated memory
                for (int i = 0; i < transaction_count; i++) {
                    free(transaction_menu[i]);
                }
                free(transaction_menu);
                werase(dialog);
                wrefresh(dialog);
                delwin(dialog);
                touchwin(stdscr);
                refresh();
                return;
                
            case KEY_BACKSPACE:
            #if KEY_BACKSPACE_ALT != 127  // Only include this case if the constants are different
            case KEY_BACKSPACE_ALT:
            #endif
            case 127: // Backspace alternative
                // Free allocated memory
                for (int i = 0; i < transaction_count; i++) {
                    free(transaction_menu[i]);
                }
                free(transaction_menu);
                werase(dialog);
                wrefresh(dialog);
                delwin(dialog);
                touchwin(stdscr);
                refresh();
                return;
        }
    }
    
    // We have a selection in current_highlighted
    // Free menu memory
    for (int i = 0; i < transaction_count; i++) {
        free(transaction_menu[i]);
    }
    free(transaction_menu);
    
    // Ask for confirmation
    werase(dialog);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 18) / 2, " Confirm Removal ");
    
    char category_name[MAX_NAME_LEN] = "Uncategorized";
    if (transactions[trans_choice].category_id >= 0 && transactions[trans_choice].category_id < category_count) {
        strcpy(category_name, categories[transactions[trans_choice].category_id].name);
    }
    
    // Format date for display
    char display_date[11];
    if (strlen(transactions[trans_choice].date) == 10) {
        sprintf(display_date, "%c%c-%c%c-%c%c%c%c",
                transactions[trans_choice].date[5], transactions[trans_choice].date[6],   // Month
                transactions[trans_choice].date[8], transactions[trans_choice].date[9],   // Day
                transactions[trans_choice].date[0], transactions[trans_choice].date[1],   // Year
                transactions[trans_choice].date[2], transactions[trans_choice].date[3]);
    } else {
        strcpy(display_date, transactions[trans_choice].date);
    }
    
    mvwprintw(dialog, 2, 2, "Are you sure you want to remove this transaction?");
    mvwprintw(dialog, 4, 2, "Date: %s", display_date);
    mvwprintw(dialog, 5, 2, "Description: %s", transactions[trans_choice].description);
    mvwprintw(dialog, 6, 2, "Amount: $%.2f", transactions[trans_choice].amount);
    mvwprintw(dialog, 7, 2, "Category: %s", category_name);
    
    const char *confirm_menu[] = {
        "Yes, remove it",
        "No, keep it"
    };
    
    wrefresh(dialog);
    
    int confirm = get_menu_choice(dialog, confirm_menu, 2, 9, 0);
    
    if (confirm == 0) { // Yes, remove it
        // Remove the transaction by shifting all transactions after it one position back
        for (int i = trans_choice; i < transaction_count - 1; i++) {
            transactions[i] = transactions[i + 1];
        }
        
        transaction_count--;
        save_data_to_file(); // Save after removing transaction
        
        // Show brief confirmation message
        werase(dialog);
        box(dialog, 0, 0);
        mvwprintw(dialog, 0, (dialog_width - 20) / 2, " Transaction Removed ");
        mvwprintw(dialog, 2, 2, "Transaction removed successfully.");
        mvwprintw(dialog, 3, 2, "Press any key to continue...");
        wrefresh(dialog);
        getch();
    }
    
    werase(dialog);
    wrefresh(dialog);
    delwin(dialog);
    touchwin(stdscr);
    refresh();
}

void budget_setup()
{
    WINDOW *win = stdscr;

    const char *budget_menu[] = {
        "1. Set Total Budget",
        "2. Add Category",
        "3. Remove Category",
        "4. Return to Main Menu"};
    int menu_size = sizeof(budget_menu) / sizeof(budget_menu[0]);

    while (1)
    {
        clear();
        draw_title(win, "Budget Setup");

        mvwprintw(win, 3, 2, "Current Total Budget: $%.2f", total_budget);

        // Display categories if we have any
        if (category_count > 0)
        {
            display_categories(win, 5);
        }

        int menu_start = 5 + (category_count > 0 ? category_count + 5 : 0);

        // Use custom menu navigation
        int choice = get_menu_choice(win, budget_menu, menu_size, menu_start, 1);

        if (choice == -1)
        { // Backspace or Escape was pressed
            return;
        }

        switch (choice)
        {
        case 0:
        { // Set Total Budget
            // Clear any previous messages
            for (int i = 0; i < 3; i++)
            {
                wmove(win, menu_start + 4 + i, 0);
                wclrtoeol(win);
            }
            wrefresh(win);

            // Split the prompt into multiple parts for better display
            mvwprintw(win, menu_start + 4, 2, "Enter new total budget amount: ");
            mvwprintw(win, menu_start + 5, 2, "$");
            wrefresh(win);

            double new_budget = get_double_input(win, menu_start + 5, 4);

            // Check if input was canceled
            if (new_budget == -1.0)
            {
                break;
            }

            total_budget = new_budget;
            save_data_to_file(); // Save after budget update
            break;
        }
        case 1:
        { // Add Category
            if (category_count >= MAX_CATEGORIES)
            {
                mvwprintw(win, menu_start + 4, 2, "Maximum number of categories reached.");
                mvwprintw(win, menu_start + 5, 2, "Press any key to continue...");
                getch();
                break;
            }

            // Clear any previous messages
            for (int i = 0; i < 4; i++)
            {
                wmove(win, menu_start + 4 + i, 0);
                wclrtoeol(win);
            }
            wrefresh(win);

            // Split the prompts into multiple parts for better display
            mvwprintw(win, menu_start + 4, 2, "Enter category name: ");
            mvwprintw(win, menu_start + 6, 2, "Enter allocated amount: ");
            mvwprintw(win, menu_start + 7, 2, "$");
            wrefresh(win);

            if (!get_string_input(win, categories[category_count].name, MAX_NAME_LEN, menu_start + 4, 23) ||
                !get_double_input(win, menu_start + 7, 4))
            {
                break; // User canceled
            }

            categories[category_count].amount = get_double_input(win, menu_start + 7, 4);
            category_count++;
            save_data_to_file(); // Save after adding category
            break;
        }
        case 2: // Remove Category
        {
            if (category_count == 0)
            {
                mvwprintw(win, menu_start + 4, 2, "No categories to remove.");
                mvwprintw(win, menu_start + 5, 2, "Press any key to continue...");
                getch();
                break;
            }

            // Clear the screen for the category selection menu
            clear();
            draw_title(win, "Remove Category");
            
            // Create dynamic menu for categories
            char **category_menu = malloc(category_count * sizeof(char *));
            if (category_menu == NULL)
            {
                mvwprintw(win, category_count + 5, 2, "Memory allocation error.");
                getch();
                break;
            }

            for (int i = 0; i < category_count; i++)
            {
                category_menu[i] = malloc(MAX_NAME_LEN + 20); // Extra space for number and amount
                if (category_menu[i] == NULL)
                {
                    // Free previously allocated memory
                    for (int j = 0; j < i; j++)
                    {
                        free(category_menu[j]);
                    }
                    free(category_menu);

                    mvwprintw(win, category_count + 5, 2, "Memory allocation error.");
                    getch();
            break;
        }
                sprintf(category_menu[i], "%d. %s ($%.2f)", i + 1, categories[i].name, categories[i].amount);
            }

            mvwprintw(win, 3, 2, "Select a category to remove:");
            int cat_choice = get_menu_choice(win, (const char **)category_menu, category_count, 5, 1);

            // Free allocated memory
            for (int i = 0; i < category_count; i++)
            {
                free(category_menu[i]);
            }
            free(category_menu);

            if (cat_choice == -1)
            { // User pressed backspace or escape
                break;
            }
            
            // Ask for confirmation
            clear();
            draw_title(win, "Confirm Removal");
            mvwprintw(win, 3, 2, "Are you sure you want to remove \"%s\"?", categories[cat_choice].name);
            mvwprintw(win, 4, 2, "Any transactions associated with this category will be set to uncategorized.");
            
            const char *confirm_menu[] = {
                "Yes, remove it",
                "No, keep it"
            };

            int confirm = get_menu_choice(win, confirm_menu, 2, 6, 0);
            
            if (confirm == 0) { // Yes, remove it
                // Check if there are any transactions using this category
                for (int i = 0; i < transaction_count; i++) {
                    if (transactions[i].category_id == cat_choice) {
                        transactions[i].category_id = -1; // Set to uncategorized
                    } else if (transactions[i].category_id > cat_choice) {
                        transactions[i].category_id--; // Adjust category IDs for shifted categories
                    }
                }
                
                // Remove the category by shifting all categories after it one position back
                for (int i = cat_choice; i < category_count - 1; i++) {
                    strcpy(categories[i].name, categories[i + 1].name);
                    categories[i].amount = categories[i + 1].amount;
                }
                
                category_count--;
                save_data_to_file(); // Save after removing category
                
                // Show a brief confirmation message
                clear();
                draw_title(win, "Category Removed");
                mvwprintw(win, 3, 2, "Category removed successfully.");
                wrefresh(win);
                
                // Short delay to show the confirmation message (500ms)
                napms(500);
            }
            break;
        }
        case 3: // Return to Main Menu (changed from case 2)
            return;
        }
    }
}

void manage_transactions()
{
    WINDOW *win = stdscr;

    const char *transaction_menu[] = {
        "1. Add Transaction",
        "2. Return to Main Menu"};
    int menu_size = sizeof(transaction_menu) / sizeof(transaction_menu[0]);

    while (1)
    {
        clear();
        draw_title(win, "Manage Transactions");

        // Display transactions if we have any
        if (transaction_count > 0)
        {
            display_transactions(win, 3);
        }
        else
        {
            mvwprintw(win, 3, 2, "No transactions recorded yet.");
        }

        int menu_start = 5 + (transaction_count > 0 ? transaction_count + 3 : 0);

        // Use custom menu navigation
        int choice = get_menu_choice(win, transaction_menu, menu_size, menu_start, 1);

        if (choice == -1)
        { // Backspace or Escape was pressed
            return;
        }

        switch (choice)
        {
        case 0:
        { // Add Transaction
            if (transaction_count >= MAX_TRANSACTIONS)
            {
                mvwprintw(win, menu_start + 4, 2, "Maximum number of transactions reached.");
                mvwprintw(win, menu_start + 5, 2, "Press any key to continue...");
                getch();
                break;
            }

            if (category_count == 0)
            {
                mvwprintw(win, menu_start + 4, 2, "Please set up budget categories first.");
                mvwprintw(win, menu_start + 5, 2, "Press any key to continue...");
                getch();
                break;
            }

            // Clear any previous messages
            for (int i = 0; i < 8; i++)
            {
                wmove(win, menu_start + 4 + i, 0);
                wclrtoeol(win);
            }
            wrefresh(win);

            // Split prompts into multiple parts for better display
            mvwprintw(win, menu_start + 4, 2, "Enter transaction description: ");
            mvwprintw(win, menu_start + 6, 2, "Enter amount: ");
            mvwprintw(win, menu_start + 7, 2, "$");
            wrefresh(win);

            if (!get_string_input(win, transactions[transaction_count].description,
                                  MAX_NAME_LEN, menu_start + 4, 33) ||
                !get_double_input(win, menu_start + 7, 4))
            {
                break; // User canceled
            }

            transactions[transaction_count].amount = get_double_input(win, menu_start + 7, 4);

            mvwprintw(win, menu_start + 8, 2, "Enter date (DD-MM-YYYY): ");
            mvwprintw(win, menu_start + 9, 2, "(Use arrow keys or Type)");
            wrefresh(win);

            if (!get_date_input(win, transactions[transaction_count].date,
                                  11, menu_start + 8))
            {
                break; // User canceled
            }

            // Display categories for selection
            clear();
            draw_title(win, "Select Category");

            // Create dynamic menu for categories
            char **category_menu = malloc(category_count * sizeof(char *));
            if (category_menu == NULL)
            {
                mvwprintw(win, category_count + 5, 2, "Memory allocation error.");
                getch();
                break;
            }

            for (int i = 0; i < category_count; i++)
            {
                category_menu[i] = malloc(MAX_NAME_LEN + 10); // Extra space for number and formatting
                if (category_menu[i] == NULL)
                {
                    // Free previously allocated memory
                    for (int j = 0; j < i; j++)
                    {
                        free(category_menu[j]);
                    }
                    free(category_menu);

                    mvwprintw(win, category_count + 5, 2, "Memory allocation error.");
                    getch();
                    break;
                }
                sprintf(category_menu[i], "%d. %s", i + 1, categories[i].name);
            }

            int cat_choice = get_menu_choice(win, (const char **)category_menu, category_count, 3, 1);

            // Free allocated memory
            for (int i = 0; i < category_count; i++)
            {
                free(category_menu[i]);
            }
            free(category_menu);

            if (cat_choice == -1)
            { // User pressed backspace
                break;
            }

            transactions[transaction_count].category_id = cat_choice;
            transaction_count++;
            save_data_to_file(); // Save after adding transaction
            break;
        }
        case 1: // Return to Main Menu
            return;
        }
    }
}