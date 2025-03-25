#include "globals.h"
#include "actions.h"

// Helper function for adding a category in dashboard mode
void add_category_dialog()
{
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

    if (category_count >= MAX_CATEGORIES)
    {
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

    if (!get_string_input(dialog, categories[category_count].name, MAX_NAME_LEN, 2, 22))
    {
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
    if (amount == -1.0)
    {
        delwin(dialog);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();
        return;
    }

    categories[category_count].amount = amount;
    category_count++;
    save_data_to_file(); // Save after adding category

    delwin(dialog);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
}

// Helper function for removing a category in dashboard mode
void remove_category_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    if (category_count == 0)
    {
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
    if (category_menu == NULL)
    {
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
    for (int i = 0; i < category_count; i++)
    {
        free(category_menu[i]);
    }
    free(category_menu);

    if (cat_choice == -1)
    { // User pressed backspace or escape
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
        "No, keep it"};

    wnoutrefresh(dialog);
    doupdate();

    int confirm = get_menu_choice(dialog, confirm_menu, 2, 5, 0);

    if (confirm == 0)
    { // Yes, remove it
        // Check if there are any transactions using this category
        for (int i = 0; i < transaction_count; i++)
        {
            if (transactions[i].category_id == cat_choice)
            {
                transactions[i].category_id = -1; // Set to uncategorized
            }
            else if (transactions[i].category_id > cat_choice)
            {
                transactions[i].category_id--; // Adjust category IDs for shifted categories
            }
        }

        // Remove the category by shifting all categories after it one position back
        for (int i = cat_choice; i < category_count - 1; i++)
        {
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
void set_budget_dialog()
{
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
    if (new_budget == -1.0)
    {
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
void add_transaction_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    refresh(); // Ensure base screen is up to date

    if (transaction_count >= MAX_TRANSACTIONS)
    {
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

    if (category_count == 0)
    {
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

    if (!get_string_input(dialog, new_transaction.description, MAX_NAME_LEN, 2, 33))
    {
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
    if (amount == -1.0)
    {
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

    if (!get_date_input(dialog, date_buffer, 8, 29))
    {
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
    if (dialog_height > max_y - 4)
    {
        dialog_height = max_y - 4; // Cap at reasonable size
    }

    dialog = newwin(dialog_height, dialog_width, (max_y - dialog_height) / 2, start_x);
    keypad(dialog, TRUE);
    box(dialog, 0, 0);

    mvwprintw(dialog, 0, (dialog_width - 18) / 2, " Select Category ");

    // Create dynamic menu for categories
    char **category_menu = malloc(category_count * sizeof(char *));
    if (category_menu == NULL)
    {
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
    for (int i = 0; i < category_count; i++)
    {
        free(category_menu[i]);
    }
    free(category_menu);

    if (cat_choice == -1)
    { // User pressed backspace or ESC
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
           strcmp(transactions[insert_pos].date, new_transaction.date) > 0)
    {
        insert_pos++;
    }

    // We want to insert at the beginning of the same-date group
    // We don't need to move further down for same dates

    // Make room for the new transaction
    if (insert_pos < transaction_count)
    {
        // Shift transactions down
        for (int i = transaction_count; i > insert_pos; i--)
        {
            transactions[i] = transactions[i - 1];
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
void remove_transaction_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    refresh(); // Ensure base screen is up to date

    if (transaction_count == 0)
    {
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

    // Create dynamic menu for transactions
    char **transaction_menu = malloc(transaction_count * sizeof(char *));
    if (transaction_menu == NULL)
    {
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

    for (int i = 0; i < transaction_count; i++)
    {
        transaction_menu[i] = malloc(MAX_NAME_LEN + 50); // Plenty of space for all info
        if (transaction_menu[i] == NULL)
        {
            // Free previously allocated memory
            for (int j = 0; j < i; j++)
            {
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

    while (!selection_made)
    {
        if (redraw)
        {
            // Display header
            mvwprintw(dialog, 4, 2, "%-10s %-24s %-10s %-24s",
                      "Date", "Description", "Amount", "Category");

            // Display visible transactions
            for (int i = 0; i < visible_items && (i + start_index) < transaction_count; i++)
            {
                if (i + start_index == current_highlighted)
                {
                    wattron(dialog, COLOR_PAIR(4));
                }

                mvwprintw(dialog, 5 + i, 2, "%s", transaction_menu[i + start_index]);

                if (i + start_index == current_highlighted)
                {
                    wattroff(dialog, COLOR_PAIR(4));
                }
            }

            // Add scroll indicators if needed
            if (start_index > 0)
            {
                mvwprintw(dialog, 4, dialog_width - 3, "↑");
            }
            else
            {
                mvwprintw(dialog, 4, dialog_width - 3, " ");
            }

            if (start_index + visible_items < transaction_count)
            {
                mvwprintw(dialog, 5 + visible_items - 1, dialog_width - 3, "↓");
            }
            else
            {
                mvwprintw(dialog, 5 + visible_items - 1, dialog_width - 3, " ");
            }

            wrefresh(dialog);
            redraw = false;
        }

        int ch = wgetch(dialog);

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
            break;

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
            break;

        case '\n': // Enter key - proceed to confirmation
            selection_made = true;
            trans_choice = current_highlighted;
            break;

        case 27: // ASCII ESC key
            // Free allocated memory
            for (int i = 0; i < transaction_count; i++)
            {
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
#if KEY_BACKSPACE_ALT != 127 // Only include this case if the constants are different
        case KEY_BACKSPACE_ALT:
#endif
        case 127: // Backspace alternative
            // Free allocated memory
            for (int i = 0; i < transaction_count; i++)
            {
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
    for (int i = 0; i < transaction_count; i++)
    {
        free(transaction_menu[i]);
    }
    free(transaction_menu);

    // Ask for confirmation
    werase(dialog);
    box(dialog, 0, 0);
    mvwprintw(dialog, 0, (dialog_width - 18) / 2, " Confirm Removal ");

    char category_name[MAX_NAME_LEN] = "Uncategorized";
    if (transactions[trans_choice].category_id >= 0 && transactions[trans_choice].category_id < category_count)
    {
        strcpy(category_name, categories[transactions[trans_choice].category_id].name);
    }

    // Format date for display
    char display_date[11];
    if (strlen(transactions[trans_choice].date) == 10)
    {
        sprintf(display_date, "%c%c-%c%c-%c%c%c%c",
                transactions[trans_choice].date[5], transactions[trans_choice].date[6], // Month
                transactions[trans_choice].date[8], transactions[trans_choice].date[9], // Day
                transactions[trans_choice].date[0], transactions[trans_choice].date[1], // Year
                transactions[trans_choice].date[2], transactions[trans_choice].date[3]);
    }
    else
    {
        strcpy(display_date, transactions[trans_choice].date);
    }

    mvwprintw(dialog, 2, 2, "Are you sure you want to remove this transaction?");
    mvwprintw(dialog, 4, 2, "Date: %s", display_date);
    mvwprintw(dialog, 5, 2, "Description: %s", transactions[trans_choice].description);
    mvwprintw(dialog, 6, 2, "Amount: $%.2f", transactions[trans_choice].amount);
    mvwprintw(dialog, 7, 2, "Category: %s", category_name);

    const char *confirm_menu[] = {
        "Yes, remove it",
        "No, keep it"};

    wrefresh(dialog);

    int confirm = get_menu_choice(dialog, confirm_menu, 2, 9, 0);

    if (confirm == 0)
    { // Yes, remove it
        // Remove the transaction by shifting all transactions after it one position back
        for (int i = trans_choice; i < transaction_count - 1; i++)
        {
            transactions[i] = transactions[i + 1];
        }

        transaction_count--;
        save_data_to_file(); // Save after removing transaction
    }

    werase(dialog);
    wrefresh(dialog);
    delwin(dialog);
    touchwin(stdscr);
    refresh();
}
