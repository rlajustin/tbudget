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

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Add Category", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    if (category_count >= MAX_CATEGORIES)
    {
        draw_error(dialog, "Maximum number of categories reached.");
        return;
    }

    // Get category name
    wnoutrefresh(dialog.textbox);

    if (!get_input(dialog.textbox, categories[category_count].name, "Enter category name: ", MAX_NAME_LEN, INPUT_STRING))
    {
        delete_bounded(dialog);
        return; // User canceled
    }

    // Get amount
    wnoutrefresh(dialog.textbox);

    double amount = -1.0;
    if (!get_input(dialog.textbox, &amount, "Enter allocated amount: $", MAX_BUFFER, INPUT_DOUBLE))
    {
        amount = -1.0; // Ensure -1.0 is returned if user cancels
    }

    // Check if input was canceled
    if (amount == -1.0)
    {
        delete_bounded(dialog);
        return;
    }

    categories[category_count].budget = amount;
    categories[category_count].spent = 0.0;
    categories[category_count].extra = 0.0;
    category_count++;
    save_data_to_file(); // Save after adding category

    delete_bounded(dialog);
}

// Helper function for removing a category in dashboard mode
void remove_category_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Calculate dialog size based on number of categories
    int dialog_height = 10;
    int dialog_width = 60;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Remove Category", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    if (category_count == 0)
    {
        draw_error(dialog, "No categories to remove.");
        return;
    }

    // Create dynamic menu for categories
    char **category_menu = malloc(category_count * sizeof(char *));
    if (category_menu == NULL)
    {
        draw_error(dialog, "Memory allocation error.");
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

            draw_error(dialog, "Memory allocation error.");
            return;
        }
        sprintf(category_menu[i], "%d. %s ($%.2f)", i + 1, categories[i].name, categories[i].budget);
    }

    int cat_choice = get_scrollable_menu_choice(dialog.textbox, "Select a category to remove:", (const char **)category_menu, category_count, 6, 1);

    // Free allocated memory
    for (int i = 0; i < category_count; i++)
    {
        free(category_menu[i]);
    }
    free(category_menu);

    if (cat_choice == -1)
    { // User pressed backspace or escape
        delete_bounded(dialog);
        return;
    }

    const char *confirm_message[2];
    char message_buffer[100];
    sprintf(message_buffer, "Are you sure you want to remove \"%s\"?", categories[cat_choice].name);
    confirm_message[0] = message_buffer;
    confirm_message[1] = "Any transactions with this category will be uncategorized.";
    int confirm = get_confirmation(dialog.textbox, confirm_message, 2);

    if (confirm == 0)
    { // Yes, remove it
        // Check if there are any transactions using this category
        for (int i = 0; i < transaction_count; i++)
        {
            if (strcmp(transactions[i].cat_name, categories[cat_choice].name) == 0)
            {
                strcpy(transactions[i].cat_name, "Uncategorized");
            }
            else if (strcmp(transactions[i].cat_name, categories[cat_choice].name) > 0)
            {
                strcpy(transactions[i].cat_name, categories[cat_choice - 1].name);
            }
        }

        // Remove the category by shifting all categories after it one position back
        for (int i = cat_choice; i < category_count - 1; i++)
        {
            categories[i] = categories[i + 1];
        }

        category_count--;
        save_data_to_file(); // Save after removing category
    }

    delete_bounded(dialog);
}

// Helper function for setting budget in dashboard mode
void set_budget_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Create a dialog box centered on screen
    int dialog_height = 9;
    int dialog_width = 50;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Set Total Budget", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    // Current budget
    mvwprintw(dialog.textbox, 2, 0, "Current Budget: $%.2f", total_budget);
    wmove(dialog.textbox, 3, 0);
    wnoutrefresh(dialog.textbox);

    double new_budget = -1.0;
    if (!get_input(dialog.textbox, &new_budget, "Enter new total budget amount: $", MAX_BUFFER, INPUT_DOUBLE))
    {
        new_budget = -1.0; // Ensure -1.0 is returned if user cancels
    }

    // Check if input was canceled
    if (new_budget == -1.0)
    {
        delete_bounded(dialog);
        return;
    }
    const char *confirm_message[2];
    char message_buffer[100];
    sprintf(message_buffer, "Are you sure you want to set the total budget to $%.2f?", new_budget);
    confirm_message[0] = message_buffer;
    int confirm = get_confirmation(dialog.textbox, confirm_message, 1);
    if (confirm == 1) // no selected
    {
        delete_bounded(dialog);
        return;
    }

    total_budget = new_budget;
    save_data_to_file(); // Save after budget update
    delete_bounded(dialog);
}

// Helper function for adding a transaction in dashboard mode
void add_expense_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    int dialog_height = 11;
    int dialog_width = 60;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Add Expense", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    if (transaction_count >= MAX_TRANSACTIONS)
    {
        draw_error(dialog, "Maximum transactions reached.");
        return;
    }

    // Create dialog for transaction input
    if (category_count == 0)
    {
        draw_error(dialog, "Please set up categories first.");
        return;
    }

    // Temporary transaction data
    Transaction new_transaction;
    memset(&new_transaction, 0, sizeof(Transaction));
    new_transaction.expense = true;

    // Get transaction description
    wrefresh(dialog.textbox);

    if (!get_input(dialog.textbox, new_transaction.desc, "Enter transaction description: ", MAX_NAME_LEN, INPUT_STRING))
    {
        delete_bounded(dialog);
        return; // User canceled
    }

    // Get amount
    wrefresh(dialog.textbox);

    double amount = -1.0;
    if (!get_input(dialog.textbox, &amount, "Enter amount: $", MAX_BUFFER, INPUT_DOUBLE))
    {
        amount = -1.0; // Ensure -1.0 is returned if user cancels
    }

    // Check if input was canceled
    if (amount == -1.0)
    {
        delete_bounded(dialog);
        return;
    }

    new_transaction.amt = amount;

    char date_buffer[11] = "";
    if (!get_date_input(dialog.textbox, date_buffer, "Enter date (DD-MM-YYYY): "))
    {
        delete_bounded(dialog);
        return; // User canceled
    }

    // Copy the date string (already in YYYY-MM-DD format)
    strncpy(new_transaction.date, date_buffer, 10);
    new_transaction.date[10] = '\0';

    // Clean up previous dialog
    wclear(dialog.textbox);

    // Create a new dialog for category selection
    dialog_height = category_count + 6;
    if (dialog_height > max_y - 4)
    {
        dialog_height = max_y - 4; // Cap at reasonable size
    }

    // dialog = draw_bounded_with_title(dialog_height, dialog_width, (max_y - dialog_height) / 2, start_x, "Select Category", false, ALIGN_CENTER);

    // Create dynamic menu for categories
    char **category_menu = malloc(category_count * sizeof(char *));
    if (category_menu == NULL)
    {
        draw_error(dialog, "Memory allocation error.");
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

            draw_error(dialog, "Memory allocation error.");
            return;
        }
        sprintf(category_menu[i], "%d. %s", i + 1, categories[i].name);
    }

    int cat_choice = get_scrollable_menu_choice(dialog.textbox, "Select a category for this expense:", (const char **)category_menu, category_count, 6, 1);

    // Free allocated memory
    for (int i = 0; i < category_count; i++)
    {
        free(category_menu[i]);
    }
    free(category_menu);

    if (cat_choice == -1)
    { // User pressed backspace or ESC
        delete_bounded(dialog);
        return;
    }

    strcpy(new_transaction.cat_name, categories[cat_choice].name);

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

    delete_bounded(dialog);
}

// Add function to remove transactions
// Helper function for removing a transaction in dashboard mode
void remove_transaction_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Calculate dialog size based on number of transactions
    // Cap at a reasonable size for many transactions
    int max_display = 10; // Maximum number of transactions to display at once
    int display_count = transaction_count < max_display ? transaction_count : max_display;

    int dialog_height = display_count > 1 ? display_count + 8 : 10;
    int dialog_width = 70;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Remove Transaction", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    if (transaction_count == 0)
    {
        // Create a simple error dialog
        draw_error(dialog, "No transactions to remove.");
        return;
    }

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

    mvwprintw(dialog.textbox, 1, 0, "Select a transaction to remove:");
    wrefresh(dialog.textbox);

    int trans_choice = get_transaction_choice(dialog.textbox, transactions, transaction_count, 10);

    if (trans_choice == -1)
    {
        delete_bounded(dialog);
        return;
    }

    char category_name[MAX_NAME_LEN] = "Uncategorized";
    strcpy(category_name, transactions[trans_choice].cat_name);

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

    const char *confirm_message[5];

    char message_buffer[4][100];
    sprintf(message_buffer[0], "Date: %s", display_date);
    sprintf(message_buffer[1], "Description: %s", transactions[trans_choice].desc);
    sprintf(message_buffer[2], "Amount: $%.2f", transactions[trans_choice].amt);
    sprintf(message_buffer[3], "Category: %s", category_name);
    confirm_message[0] = "Are you sure you want to remove this transaction?";
    confirm_message[1] = message_buffer[0];
    confirm_message[2] = message_buffer[1];
    confirm_message[3] = message_buffer[2];
    confirm_message[4] = message_buffer[3];
    int confirm = get_confirmation(dialog.textbox, confirm_message, 5);

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

    delete_bounded(dialog);
}
