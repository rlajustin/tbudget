#include "actions.h"

// Helper function for adding a category in dashboard mode
void add_category_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Create a dialog box centered on screen
    int dialog_height = DEFAULT_DIALOG_HEIGHT;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
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

    Category cat_to_add = {0};

    if (!get_input(dialog.textbox, cat_to_add.name, "Enter category name: ", MAX_NAME_LEN, INPUT_STRING))
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

    cat_to_add.budget = amount;
    cat_to_add.spent = 0.0;
    cat_to_add.extra = 0.0;
    int res = add_category(&cat_to_add, current_year, current_month);
    if (res < 0)
    {
        char error_message[MAX_BUFFER];
        sprintf(error_message, "Failed to add category: Error %d", res);
        draw_error(dialog, error_message);
    }
    delete_bounded(dialog);
}

// Helper function for removing a category in dashboard mode
void remove_category_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Calculate dialog size based on number of categories
    int dialog_height = DEFAULT_DIALOG_HEIGHT;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
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
        sprintf(category_menu[i], "%s ($%.2f)", categories[sorted_categories_indices[i]].name, categories[sorted_categories_indices[i]].budget);
    }

    int cat_choice = get_scrollable_menu_choice(dialog.textbox, "Select a category to remove:", (const char **)category_menu, category_count, 6, 1, true);

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

    const char *confirm_message[1];
    char message_buffer[100];
    sprintf(message_buffer, "Are you sure you want to remove \"%s\"?", categories[sorted_categories_indices[cat_choice]].name);
    confirm_message[0] = message_buffer;
    int confirm = get_confirmation(dialog.textbox, confirm_message, 1);

    if (confirm == 0)
    {
        int result = remove_category(cat_choice, current_year, current_month);

        if (result < 1)
        {
            if (result == -4)
            {
                draw_error(dialog, "Cannot remove category with existing transactions.");
            }
            else if (result == -2)
            {
                draw_error(dialog, "Invalid category index.");
            }
            else
            {
                draw_error(dialog, "Failed to remove category.");
            }
        }
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
    int dialog_height = DEFAULT_DIALOG_HEIGHT;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Set Total Budget", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    // Current budget
    mvwprintw(dialog.textbox, 2, 0, "Current Budget: $%.2f", current_month_total_budget);
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

    current_month_total_budget = new_budget;
    int res = set_budget(new_budget, current_year, current_month);
    if (res == 0)
    {
        char error_message[MAX_BUFFER];
        sprintf(error_message, "Failed to set budget: Error %d", res);
        draw_error(dialog, error_message);
    }
    delete_bounded(dialog);
}

// Helper function for adding a transaction in dashboard mode
void add_expense_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    int dialog_height = DEFAULT_DIALOG_HEIGHT;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Add Expense", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

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
    if (!get_date_input(dialog.textbox, date_buffer, "Enter date (YYYY-MM-DD): "))
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
        sprintf(category_menu[i], "%s", categories[sorted_categories_indices[i]].name);
    }

    int cat_choice = get_scrollable_menu_choice(dialog.textbox, "Select a category for this expense:", (const char **)category_menu, category_count, 6, 1, true);

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
    new_transaction.cat_index = cat_choice;

    int year, month, day;
    sscanf(new_transaction.date, "%d-%d-%d", &year, &month, &day);
    int result = add_transaction(&new_transaction, year, month);
    if (result < 0)
    {
        char error_message[MAX_BUFFER];
        sprintf(error_message, "Failed to add transaction: Error %d", result);
        draw_error(dialog, error_message);
    }

    delete_bounded(dialog);
}

// Add function to remove transactions
// Helper function for removing a transaction in dashboard mode
void remove_transaction_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Keep the variable height calculation for transactions
    int max_display = 10;
    int display_count = current_month_transaction_count < max_display ? current_month_transaction_count : max_display;
    int dialog_height = display_count > 1 ? display_count + 8 : 10;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Remove Transaction", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    if (current_month_transaction_count == 0)
    {
        draw_error(dialog, "No transactions to remove.");
        return;
    }

    // Ensure transactions are sorted with most recent first
    // This sort has already been done in display_transactions
    // for (int i = 0; i < transaction_count - 1; i++)
    // {
    //     for (int j = 0; j < transaction_count - i - 1; j++)
    //     {
    //         // Compare dates (format is YYYY-MM-DD so string comparison works)
    //         if (strcmp(transactions[j].date, transactions[j + 1].date) < 0)
    //         {
    //             // Swap transactions
    //             Transaction temp = transactions[j];
    //             transactions[j] = transactions[j + 1];
    //             transactions[j + 1] = temp;
    //         }
    //     }
    // }

    mvwprintw(dialog.textbox, 1, 0, "Select a transaction to remove:");
    wrefresh(dialog.textbox);

    int trans_choice = get_transaction_choice(dialog.textbox, sorted_transactions, current_month_transaction_count, 10);

    if (trans_choice == -1)
    {
        delete_bounded(dialog);
        return;
    }

    char category_name[MAX_NAME_LEN] = {0};
    strcpy(category_name, categories[sorted_transactions[trans_choice]->data.cat_index].name);

    // Format date for display
    char display_date[11];
    if (strlen(sorted_transactions[trans_choice]->data.date) == 10)
    {
        strcpy(display_date, sorted_transactions[trans_choice]->data.date);
    }
    else
    {
        strcpy(display_date, sorted_transactions[trans_choice]->data.date);
    }

    const char *confirm_message[5];

    char message_buffer[4][100];
    sprintf(message_buffer[0], "Date: %s", display_date);
    sprintf(message_buffer[1], "Description: %s", sorted_transactions[trans_choice]->data.desc);
    sprintf(message_buffer[2], "Amount: $%.2f", sorted_transactions[trans_choice]->data.amt);
    sprintf(message_buffer[3], "Category: %s", category_name);
    confirm_message[0] = "Are you sure you want to remove this transaction?";
    confirm_message[1] = message_buffer[0];
    confirm_message[2] = message_buffer[1];
    confirm_message[3] = message_buffer[2];
    confirm_message[4] = message_buffer[3];
    int confirm = get_confirmation(dialog.textbox, confirm_message, 5);

    if (confirm == 0)
    { // Yes, remove it
        int result = remove_transaction(trans_choice);
        if (result == 0)
        {
            draw_error(dialog, "Failed to remove transaction.");
        }
    }

    delete_bounded(dialog);
}

void budget_summary_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    int dialog_height = DEFAULT_DIALOG_HEIGHT;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Budget Summary", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    const char *options_names[5] = {0};
    int options_values[5] = {0};
    int num_options = 0;

    if (category_count < MAX_CATEGORIES)
    {
        options_names[num_options] = "Add Category";
        options_values[num_options] = 0;
        num_options++;
    }
    if (category_count > 0)
    {
        options_names[num_options] = "Remove Category";
        options_values[num_options] = 1;
        num_options++;
    }
    if (current_month_transaction_count > 0)
    {
        options_names[num_options] = "View Transactions";
        options_values[num_options] = 2;
        num_options++;
    }
    options_names[num_options] = "Set Total Budget";
    options_values[num_options] = 3;
    num_options++;
    options_names[num_options] = "Exit";
    options_values[num_options] = 4;
    num_options++;

    int option_choice = get_scrollable_menu_choice(dialog.textbox, "Select an option:", options_names, num_options, 5, 1, true);
    if (option_choice == -1)
    {
        delete_bounded(dialog);
        return;
    }

    switch (options_values[option_choice])
    {
    case 0:
        add_category_dialog();
        sort_categories_by_budget();
        break;
    case 1:
        remove_category_dialog();
        // sorted inside of this
        break;
    case 2:
        draw_error(dialog, "Not implemented yet");
        break;
    case 3:
        set_budget_dialog();
        break;
    case 4:
        break;
    }

    delete_bounded(dialog);
}

void add_subscription_dialog()
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    int dialog_height = 14;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Add Subscription", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    // Create a new subscription
    Subscription new_sub;
    memset(&new_sub, 0, sizeof(Subscription));

    // Get subscription name
    if (!get_input(dialog.textbox, new_sub.name, "Enter subscription name: ", MAX_NAME_LEN, INPUT_STRING))
    {
        delete_bounded(dialog);
        return; // User canceled
    }

    if (!get_input(dialog.textbox, &new_sub.amount, "Enter amount: $", MAX_BUFFER, INPUT_DOUBLE))
    {
        delete_bounded(dialog);
        return; // User canceled
    }

    const char *type_menu[] = {
        "Expense",
        "Income"};
    int y, x;
    getyx(dialog.textbox, y, x);
    int type_choice = get_scrollable_menu_choice(dialog.textbox, "Select type:", type_menu, 2, 5, y, false);
    if (type_choice == -1)
    {
        delete_bounded(dialog);
        return; // User canceled
    }
    new_sub.expense = (type_choice == 0);

    // Get period type
    const char *period_menu[] = {
        "Weekly",
        "Monthly",
        "Yearly",
        "Custom Days"};
    wclear(dialog.textbox);
    mvwprintw(dialog.textbox, 1, 0, "Select period:");
    int period_choice = get_scrollable_menu_choice(dialog.textbox, "", period_menu, 4, 5, 1, false);
    if (period_choice == -1)
    {
        delete_bounded(dialog);
        return; // User canceled
    }
    new_sub.period_type = period_choice;

    // Get period day based on type
    wclear(dialog.textbox);
    switch (period_choice)
    {
    case PERIOD_WEEKLY:
    {
        getyx(dialog.textbox, y, x);
        const char *day_menu[] = {
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday"};
        mvwprintw(dialog.textbox, 1, 0, "Select day of the week:");
        int day_choice = get_menu_choice(dialog.textbox, day_menu, 7, 2, true);
        if (day_choice == -1)
        {
            delete_bounded(dialog);
            return; // User canceled
        }
        new_sub.period_day = day_choice;

        mvwprintw(dialog.textbox, 1, 0, "Select day of the week: %s", day_menu[day_choice]);
        wclrtobot(dialog.textbox);
        wmove(dialog.textbox, 2, 0);
        break;
    }
    case PERIOD_MONTHLY:
    {
        int day;
        wclear(dialog.textbox);
        if (!get_input(dialog.textbox, &day, "Enter day of month (1-31): ", MAX_BUFFER, INPUT_INT))
        {
            delete_bounded(dialog);
            return; // User canceled
        }
        new_sub.period_day = (day < 1) ? 1 : (day > 31) ? 31
                                                        : day;
        break;
    }
    case PERIOD_YEARLY:
    {
        int month;
        wclear(dialog.textbox);
        if (!get_input(dialog.textbox, &month, "Enter month (1-12): ", MAX_BUFFER, INPUT_INT))
        {
            delete_bounded(dialog);
            return; // User canceled
        }
        new_sub.period_day = (month < 1) ? 1 : (month > 12) ? 12
                                                            : month;

        int day;
        wclear(dialog.textbox);
        if (!get_input(dialog.textbox, &day, "Enter day of month (1-31): ", MAX_BUFFER, INPUT_INT))
        {
            delete_bounded(dialog);
            return; // User canceled
        }
        new_sub.period_month_day = (day < 1) ? 1 : (day > 31) ? 31
                                                              : day;
        break;
    }
    case PERIOD_CUSTOM_DAYS:
    {
        int days;
        wclear(dialog.textbox);
        if (!get_input(dialog.textbox, &days, "Enter number of days between recurrences (1-365): ", MAX_BUFFER, INPUT_INT))
        {
            delete_bounded(dialog);
            return; // User canceled
        }
        new_sub.period_day = (days < 1) ? 1 : (days > 365) ? 365
                                                           : days;
        break;
    }
    }

    // Get start date
    char date_buffer[11];
    if (!get_date_input(dialog.textbox, date_buffer, "Enter start date (YYYY-MM-DD): "))
    {
        delete_bounded(dialog);
        return; // User canceled
    }
    getyx(dialog.textbox, y, x);
    wmove(dialog.textbox, y + 1, 0);
    strncpy(new_sub.start_date, date_buffer, 10);
    new_sub.start_date[10] = '\0';

    if (!get_date_input(dialog.textbox, date_buffer, "Enter end date (YYYY-MM-DD), or use same as start date for indefinite:"))
    {
        delete_bounded(dialog);
        return; // User canceled
    }
    else
    {
        if (strcmp(date_buffer, new_sub.start_date) == 0)
        {
            strcpy(new_sub.end_date, "9999-12-31"); // Far future date
        }
        else
        {
            strcpy(new_sub.end_date, date_buffer);
        }
    }
    new_sub.end_date[10] = '\0';

    // Get category if it's an expense
    if (new_sub.expense && category_count > 0)
    {
        char **category_menu = malloc(category_count * sizeof(char *));
        if (category_menu == NULL)
        {
            draw_error(dialog, "Memory allocation error.");
            delete_bounded(dialog);
            return;
        }

        for (int i = 0; i < category_count; i++)
        {
            category_menu[i] = malloc(MAX_NAME_LEN);
            if (category_menu[i] == NULL)
            {
                for (int j = 0; j < i; j++)
                {
                    free(category_menu[j]);
                }
                free(category_menu);
                draw_error(dialog, "Memory allocation error.");
                delete_bounded(dialog);
                return;
            }
            strcpy(category_menu[i], categories[i].name);
        }

        wclear(dialog.textbox);
        int cat_choice = get_scrollable_menu_choice(dialog.textbox, "Select category:", (const char **)category_menu, category_count, 5, 1, true);

        // Free allocated memory
        for (int i = 0; i < category_count; i++)
        {
            free(category_menu[i]);
        }
        free(category_menu);

        if (cat_choice == -1)
        {
            delete_bounded(dialog);
            return; // User canceled
        }

        strcpy(new_sub.cat_name, categories[cat_choice].name);
    }
    else
    {
        strcpy(new_sub.cat_name, "Uncategorized");
    }

    // Set initial last_updated to start_date
    struct tm start_tm = {0};
    sscanf(new_sub.start_date, "%d-%d-%d",
           &start_tm.tm_year, &start_tm.tm_mon, &start_tm.tm_mday);
    start_tm.tm_year -= 1900; // Adjust year (tm_year is years since 1900)
    start_tm.tm_mon -= 1;     // Adjust month (tm_mon is 0-11)
    mktime(&start_tm);        // Normalize the time
    new_sub.last_updated = start_tm;

    int res = add_subscription(&new_sub);
    if (res == -1)
    {
        draw_error(dialog, "Error adding subscription");
    }
    update_subscriptions();

    delete_bounded(dialog);
}

void remove_subscription_dialog(int selected_subscription)
{
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    int dialog_height = DEFAULT_DIALOG_HEIGHT;
    int dialog_width = DEFAULT_DIALOG_WIDTH;
    int start_y = (max_y - dialog_height) / 2;
    int start_x = (max_x - dialog_width) / 2;

    BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Remove Subscription", false, ALIGN_CENTER);
    wnoutrefresh(dialog.boundary);

    if (selected_subscription == -1 || selected_subscription >= subscription_count)
    {
        draw_error(dialog, "Subscription not found.");
        return;
    }

    // Create dynamic menu for subscriptions
    char **sub_menu = malloc(subscription_count * sizeof(char *));
    if (sub_menu == NULL)
    {
        draw_error(dialog, "Memory allocation error.");
        return;
    }

    const char *confirm_message[2];
    char message_buffer[100];
    sprintf(message_buffer, "Are you sure you want to remove \"%s\"?", subscriptions[selected_subscription].name);
    confirm_message[0] = message_buffer;
    confirm_message[1] = "This will not remove any transactions already created.";
    int confirm = get_confirmation(dialog.textbox, confirm_message, 2);

    if (confirm == 0)
    {
        remove_subscription(selected_subscription);
    }

    delete_bounded(dialog);
}