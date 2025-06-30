#include "saveload.h"

char *get_home_directory()
{
    char *home = getenv("HOME");

    if (home != NULL)
    {
        return home;
    }

    // If HOME is not available, try to get it from passwd
    struct passwd *pw = getpwuid(getuid());
    if (pw != NULL)
    {
        return pw->pw_dir;
    }

    // If all fails, use current directory
    return ".";
}

int create_directory_if_not_exists(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
    {
        // Already exists
        if (S_ISDIR(st.st_mode))
        {
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

    if (result != 0)
    {
        fprintf(stderr, "Error creating directory %s: %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

void initialize_data_directories()
{
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
    // sprintf(export_file_path, "%s/%s", app_data_dir, EXPORT_FILE_NAME);

    sprintf(data_file_path, "%s/%s", data_storage_dir, DATA_FILE_NAME);

    // Create directories
    create_directory_if_not_exists(app_data_dir);
    create_directory_if_not_exists(data_storage_dir);
}

/*
 * Initialize data from the data file
 *
 * Returns:
 *   1     - Success
 *   -1    - I/O error occurred
 *   -2    - Data corrupted
 */
int load_budget_data()
{
    FILE *file;
    if (access(data_file_path, F_OK) == 0)
    {
        // File exists
        file = fopen(data_file_path, "r+b");
    }
    else
    {
        file = fopen(data_file_path, "w+b");
        FileHeader header = {
            .name = "tbudget_1.0",
            .last_modified = time(NULL)};
        fwrite(&header, sizeof(FileHeader), 1, file);
        const int default_constants[NUM_CONSTANTS] = {
            MAX_CATEGORIES,
            MAX_NAME_LEN};
        fwrite(default_constants, sizeof(int), NUM_CONSTANTS, file);
        fwrite(&(double){0}, sizeof(double), 1, file); // no default monthly budget
        fwrite(&(int){0}, sizeof(int), 1, file);       // no default categories
        fwrite(&(Category){0}, sizeof(Category), MAX_CATEGORIES, file);
        fwrite(&(int){0}, sizeof(int), 1, file); // no default subscriptions
    }
    fseek(file, 0, SEEK_SET);

    // Read and validate header
    FileHeader header;
    if (fread(&header, sizeof(FileHeader), 1, file) != 1)
    {
        fclose(file);
        return -1;
    }

    int constants[NUM_CONSTANTS];
    if (fread(constants, sizeof(int), NUM_CONSTANTS, file) != NUM_CONSTANTS)
    {
        fclose(file);
        return -1;
    }

    const int prev_constants[NUM_CONSTANTS] = {
        MAX_CATEGORIES,
        MAX_NAME_LEN};

    if (memcmp(constants, prev_constants, NUM_CONSTANTS) != 0)
    {
        fprintf(stderr, "Error: set in globals.h MAX_CATEGORIES=%d, MAX_NAME_LEN=%d\n", constants[0], constants[1]);
        fclose(file);
        return -1;
    }

    // Read default monthly budget
    if (fread(&default_monthly_budget, sizeof(double), 1, file) != 1)
    {
        fclose(file);
        return -1;
    }

    // Read default categories
    if (fread(&default_category_count, sizeof(int), 1, file) != 1 ||
        default_category_count > MAX_CATEGORIES || default_category_count < 0)
    {
        default_category_count = 0;
    }
    else
    {
        if (fread(default_categories, sizeof(Category), MAX_CATEGORIES, file) != (size_t)MAX_CATEGORIES)
        {
            fclose(file);
            return -1;
        }
    }

    // Read subscription count and validate
    if (fread(&subscription_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    if (
        subscription_count < 0)
    {
        return -2;
    }
    else
    {
        subscriptions = (Subscription *)malloc(subscription_count * sizeof(Subscription));
        // Read subscriptions
        if (fread(subscriptions, sizeof(Subscription), subscription_count, file) != (size_t)subscription_count)
        {
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    return 1;
}

/*
 * Initialize data from the data file
 *
 * Returns:
 *   1     - Success
 *   -1    - I/O error occurred
 */
int save_budget_data()
{
    FILE *file;
    if (access(data_file_path, F_OK) == 0)
    {
        // File exists
        file = fopen(data_file_path, "r+b");
    }
    else
    {
        return -1;
    }

    fseek(file, sizeof(FileHeader) + sizeof(int) * NUM_CONSTANTS, SEEK_SET);
    if (fwrite(&default_monthly_budget, sizeof(double), 1, file) != 1)
    {
        return -1;
    }
    if (fwrite(&default_category_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    if (fwrite(&default_categories, sizeof(Category), MAX_CATEGORIES, file) != 1)
    {
        return -1;
    }
    if (fwrite(&subscription_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    if (fwrite(&subscriptions, sizeof(Subscription), subscription_count, file) != 1)
    {
        return -1;
    }
    fclose(file);

    return 1;
}

/*
 * Load the month's data from the data file
 *
 * Returns:
 *   1     - Success
 *   -1    - I/O error occurred
 */
int load_month(int year, int month)
{
    // Free all existing transactions before loading new ones
    if (transaction_head)
    {
        // Free the linked list of transactions
        TransactionNode *current = transaction_head;
        while (current)
        {
            TransactionNode *next = current->next;
            free(current);
            current = next;
        }
        transaction_head = NULL;
        transaction_tail = NULL;
        current_month_transaction_count = 0;
    }

    FILE *file = open_month_file(year, month);
    if (!file)
    {
        return -1;
    }
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0)
    {
        fwrite(&default_monthly_budget, sizeof(double), 1, file);
        fwrite(&default_category_count, sizeof(int), 1, file);
        // fills with empty category data
        fwrite(&(Category){0}, sizeof(Category), MAX_CATEGORIES, file);
        // fills with default categories if there are any
        fseek(file, sizeof(double) + sizeof(int), SEEK_SET);
        fwrite(default_categories, sizeof(Category), default_category_count, file);
        fseek(file, 0, SEEK_END);
        fwrite(&(double){0}, sizeof(double), 1, file); // uncategorized spent
        fwrite(&(int){0}, sizeof(int), 1, file);       // number of transactions (0)
    }
    fseek(file, 0, SEEK_SET);

    // Read monthly budget
    if (fread(&current_month_total_budget, sizeof(double), 1, file) != 1)
    {
        return -1;
    }

    // Read the categories
    // TODO default to default categories if empty
    if (fread(&category_count, sizeof(int), 1, file) == 1)
    {
        if (fread(categories, sizeof(Category), category_count, file) != (size_t)category_count)
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
    sort_categories_by_budget();
    fseek(file, sizeof(double) + sizeof(int) + (sizeof(Category) * MAX_CATEGORIES), SEEK_SET);
    // Read uncategorized spent
    if (fread(&uncategorized_spent, sizeof(double), 1, file) != 1)
    {
        return -1;
    }

    // Read the number of transactions
    int transaction_count;
    if (fread(&transaction_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }

    // Read the transactions into a linked list
    current_month_transaction_count = transaction_count;
    if (transaction_head)
    {
        cleanup_transactions();
    }
    if (sorted_transactions)
    {
        free(sorted_transactions);
    }
    sorted_transactions = (TransactionNode **)malloc(sizeof(TransactionNode *) * current_month_transaction_count);
    for (int i = 0; i < current_month_transaction_count; i++)
    {
        Transaction temp_transaction;
        if (fread(&temp_transaction, sizeof(Transaction), 1, file) != 1)
        {
            return -1;
        }

        // Create a new node
        TransactionNode *new_node = (TransactionNode *)malloc(sizeof(TransactionNode));
        sorted_transactions[i] = new_node;
        if (!new_node)
        {
            return -1;
        }

        // Copy transaction data to the new node
        new_node->data = temp_transaction;
        new_node->index = i;
        new_node->next = NULL;

        // Add to the linked list
        if (transaction_tail)
        {
            transaction_tail->next = new_node;
            new_node->prev = transaction_tail;
            transaction_tail = new_node;
        }
        else
        {
            transaction_head = transaction_tail = new_node;
            new_node->prev = NULL;
        }
    }

    qsort(sorted_transactions, current_month_transaction_count, sizeof(TransactionNode *), compare_transactions_by_date);
    loaded_month = month;
    loaded_year = year;
    if (year == today_year && month == today_month)
    {
        default_monthly_budget = current_month_total_budget;
        default_category_count = category_count;
        memcpy(default_categories, categories, category_count * sizeof(Category));
    }
    return 1;
}

/*
 * Add a transaction to the current month's data file
 *
 * Returns:
 *   1     - Success
 *   -1    - I/O error occurred
 *   -2    - Malloc error
 */
int add_transaction(Transaction *transaction, int year, int month)
{
    FILE *file = open_month_file(year, month);
    if (!file)
    {
        return -1;
    }
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) // pointer starts at the end of the file, so this checks if the file is empty
    {
        fwrite(&default_monthly_budget, sizeof(double), 1, file);
        fwrite(&default_category_count, sizeof(int), 1, file);
        fwrite(&(Category){0}, sizeof(Category), MAX_CATEGORIES, file);
        fseek(file, sizeof(double) + sizeof(int), SEEK_SET);
        fwrite(default_categories, sizeof(Category), default_category_count, file);
        fseek(file, 0, SEEK_END);
        fwrite(&(double){0}, sizeof(double), 1, file); // uncategorized spent
        fwrite(&(int){0}, sizeof(int), 1, file);       // number of transactions
    }

    // update categories
    fseek(file, sizeof(double), SEEK_SET);
    int cat_count = 0;
    Category cat;
    if (fread(&cat_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    fseek(file, sizeof(double) + sizeof(int) + (sizeof(Category) * transaction->cat_index), SEEK_SET);
    if (fread(&cat, sizeof(Category), 1, file) != 1)
    {
        return -1;
    }

    cat.spent += transaction->amt;
    fwrite(&cat, sizeof(Category), 1, file);

    // add transaction
    fseek(file, sizeof(double) + sizeof(int) + (sizeof(Category) * MAX_CATEGORIES) + sizeof(double), SEEK_SET);
    int tmp_count;
    if (fread(&tmp_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    tmp_count++;
    fseek(file, -sizeof(int), SEEK_CUR);
    fwrite(&tmp_count, sizeof(int), 1, file);

    fseek(file, 0, SEEK_END);
    fwrite(transaction, sizeof(Transaction), 1, file);
    if (year != current_year || month != current_month) // don't need to store it in memory
    {
        return 1;
    }

    // Add the transaction to the linked list
    TransactionNode *new_node = (TransactionNode *)malloc(sizeof(TransactionNode));
    if (!new_node)
    {
        return -2;
    }

    // Copy transaction data to the new node
    new_node->data = *transaction;
    new_node->next = NULL;
    new_node->prev = transaction_tail;
    new_node->index = current_month_transaction_count++;

    // Add to the linked list
    if (transaction_tail)
    {
        transaction_tail->next = new_node;
        transaction_tail = new_node;
    }
    else
    {
        transaction_head = transaction_tail = new_node;
    }

    // Binary search to insert into sorted transactions array
    int left = 0;
    int right = current_month_transaction_count - 2;
    int insert_pos = 0;

    while (left <= right)
    {
        int mid = (left + right) / 2;
        if (compare_transactions_by_date(&sorted_transactions[mid], &new_node) > 0)
        {
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }
    insert_pos = left;

    // Reallocate the sorted_transactions array to make room for the new element
    TransactionNode **new_sorted = (TransactionNode **)realloc(sorted_transactions, (current_month_transaction_count + 1) * sizeof(TransactionNode *));
    if (new_sorted)
        sorted_transactions = new_sorted;
    else
    {
        free(new_node); // Don't leak the transaction node we created earlier
        return -2;
    }

    memmove(&sorted_transactions[insert_pos + 1],
            &sorted_transactions[insert_pos],
            (current_month_transaction_count - insert_pos) * sizeof(TransactionNode *));
    sorted_transactions[insert_pos] = new_node;

    current_month_transaction_count++;

    return 1;
}

/*
 * Add a subscription to the budget data file
 * Returns:
 *   1      - Success
 *   -1     - I/O error occurred
 *   -2     - Memory error occurred
 */
int add_subscription(Subscription *subscription)
{
    FILE *file;
    if (access(data_file_path, F_OK) == 0)
    {
        // File exists
        file = fopen(data_file_path, "r+b");
    }
    else
        return -1;
    subscription_count++;
    fseek(file, sizeof(FileHeader) + sizeof(int) * NUM_CONSTANTS + sizeof(double) + sizeof(int) + sizeof(Category) * MAX_CATEGORIES, SEEK_SET);
    if (fwrite(&subscription_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    fseek(file, sizeof(Subscription) * (subscription_count - 1), SEEK_CUR);

    if (fwrite(subscription, sizeof(Subscription), 1, file) != 1)
    {
        return -1;
    }

    Subscription *new_subscriptions = realloc(subscriptions, subscription_count * sizeof(Subscription));
    if (new_subscriptions)
    {
        subscriptions = new_subscriptions;
        subscriptions[subscription_count - 1] = *subscription;
    }
    else
        return -2;
    return 1;
}

/*
 * Removes a subscription from the budget data file
 * Returns:
 *   1      - Success
 *   -1     - I/O error occurred
 *   -2     - Memory error occurred
 */
int remove_subscription(int index)
{
    FILE *file;
    if (access(data_file_path, F_OK) == 0)
    {
        // File exists
        file = fopen(data_file_path, "r+b");
    }
    else
        return -1;
    subscription_count--;
    fseek(file, sizeof(FileHeader) + sizeof(int) * NUM_CONSTANTS + sizeof(double) + sizeof(int) + sizeof(Category) * MAX_CATEGORIES, SEEK_SET);
    if (fwrite(&subscription_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    Subscription to_replace = subscriptions[subscription_count];

    fseek(file, sizeof(FileHeader) + sizeof(int) * NUM_CONSTANTS + sizeof(double) + sizeof(int) + sizeof(Category) * MAX_CATEGORIES + sizeof(int) + sizeof(Subscription) * index, SEEK_CUR);
    if (fwrite(&to_replace, sizeof(Subscription), 1, file) != 1)
    {
        return -1;
    }
    Subscription *new_subscriptions = realloc(subscriptions, subscription_count * sizeof(Subscription));
    if (new_subscriptions)
    {
        subscriptions = new_subscriptions;
        subscriptions[index] = to_replace;
    }
    else
    {
        return -2;
    }

    int new_size = sizeof(double) + sizeof(int) * NUM_CONSTANTS + sizeof(double) + sizeof(int) + (sizeof(Category) * MAX_CATEGORIES) + sizeof(int) + subscription_count * sizeof(Transaction);
    ftruncate(fileno(file), new_size);

    return 1;
}

/*
 * Add a category to the current month's data file
 *
 * Returns:
 *   1     - Success
 *   -1    - I/O error occurred
 *   -2    - Category already exists
 *   -3    - Not in current month (shouldn't be possible with current app structure)
 *   -1XX  - Category count mismatch (XX = expected count)
 */
int add_category(Category *category, int year, int month)
{
    if (year != current_year || month != current_month)
    {
        return -3;
    }
    FILE *file = open_month_file(year, month);
    int write_index = category_count;
    if (!file)
    {
        return -1;
    }
    for (int i = 0; i < MAX_CATEGORIES; i++)
    {
        if (categories[i].budget > 0.0)
        {
            if (strcmp(categories[i].name, category->name) == 0)
            {
                return -2;
            }
        }
        else if (write_index > i)
        {
            write_index = i;
        }
    }
    fseek(file, sizeof(double) + sizeof(int) + write_index * sizeof(Category), SEEK_SET);
    fwrite(category, sizeof(Category), 1, file);
    categories[write_index] = *category;
    category_count++;

    fseek(file, sizeof(double), SEEK_SET);
    fwrite(&category_count, sizeof(int), 1, file);

    // if it's the most recent month, make this a default category
    if (year == today_year && month == today_month)
    {
        default_category_count++;
        memcpy(default_categories, categories, sizeof(categories));
    }
    return 1;
}

/*
 * Remove a category from the current month's data file
 *
 * Returns:
 *   1     - Success
 *   -1    - I/O error occurred
 *   -2    - Category index out of bounds
 *   -3    - Not in current month (shouldn't be possible with current app structure)
 *   -4    - Category already doesn't exist
 *   -1XX  - Category count mismatch (XX = expected count)
 */
int remove_category(int category_index, int year, int month)
{
    if (year != current_year || month != current_month)
    {
        return -3;
    }

    if (category_index < 0 || category_index >= MAX_CATEGORIES)
    {
        return -2; // Category index out of bounds
    }

    FILE *file = open_month_file(year, month);
    if (!file)
    {
        return -1;
    }

    categories[category_index].budget = 0.0; // effectively deletes it, but lets us use other data later
    category_count--;
    sort_categories_by_budget();
    WINDOW *win = stdscr;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    int new_index = -1;
    if (category_count > 0)
    {
        // Keep the variable height calculation for transactions
        int max_display = 10;
        int display_count = current_month_transaction_count < max_display ? current_month_transaction_count : max_display;
        int dialog_height = display_count > 1 ? display_count + 8 : 10;
        int dialog_width = DEFAULT_DIALOG_WIDTH;
        int start_y = (max_y - dialog_height) / 2;
        int start_x = (max_x - dialog_width) / 2;
        BoundedWindow dialog = draw_bounded_with_title(dialog_height, dialog_width, start_y, start_x, "Removing Category", false, ALIGN_CENTER);
        wnoutrefresh(dialog.boundary);
        int choice = get_category_choice_recategorize(dialog.textbox, categories[category_index].name);
        if (choice > -1)
            new_index = sorted_categories_indices[choice];
    }
    // update transactions
    TransactionNode *iter = transaction_head;
    while (iter != NULL)
    {
        if (iter->data.cat_index == category_index)
        {
            iter->data.cat_index = new_index;
            fseek(file, sizeof(double) + sizeof(int) + (sizeof(Category) * MAX_CATEGORIES) + sizeof(double) + iter->index * sizeof(Transaction), SEEK_SET);
            fwrite(&iter->data, sizeof(Transaction), 1, file);
        }
        iter = iter->next;
    }

    if (new_index != -1)
    {
        categories[new_index].spent += categories[category_index].spent;
    }
    else
    {
        uncategorized_spent += categories[category_index].spent;
    }

    // Rewrite the file with the updated categories
    fseek(file, sizeof(double), SEEK_SET);
    fwrite(&category_count, sizeof(int), 1, file);
    fwrite(&categories, sizeof(Category), MAX_CATEGORIES, file);
    fwrite(&uncategorized_spent, sizeof(double), 1, file);

    // Update default categories if it's the current month
    if (year == today_year && month == today_month)
    {
        memcpy(default_categories, categories, sizeof(categories));
    }

    fclose(file);
    return 1;
}

/*
 * Set the budget for the current month
 *
 * Returns:
 *   1     - Success
 *   -1    - Not in current month (shouldn't be possible with current app structure)
 *   -2    - I/O error occurred
 */
int set_budget(double budget, int year, int month)
{
    if (year != current_year || month != current_month)
    {
        return -1;
    }
    FILE *file = open_month_file(year, month);
    if (!file)
    {
        return 0;
    }
    fseek(file, 0, SEEK_SET);
    fwrite(&budget, sizeof(double), 1, file);
    if (year == today_year && month == today_month)
    {
        default_monthly_budget = budget;
    }
    return 1;
}

// note this must be in the current month/year
int remove_transaction(int index)
{
    FILE *file = open_month_file(current_year, current_month);
    if (!file)
    {
        return -1;
    }
    TransactionNode *tx = sorted_transactions[index];
    int cat_index = tx->data.cat_index;
    if (cat_index == -1)
    {
        fseek(file, sizeof(double) + sizeof(int) + sizeof(Category) * MAX_CATEGORIES, SEEK_SET);
        uncategorized_spent -= (-1 * (1 - tx->data.expense)) * tx->data.amt;
        fwrite(&uncategorized_spent, sizeof(double), 1, file);
    }
    else
    {
        categories[cat_index].spent -= (-1 * (1 - tx->data.expense)) * tx->data.amt;
        fseek(file, sizeof(double) + sizeof(int) + sizeof(Category) * cat_index, SEEK_SET);
        fwrite(&categories[cat_index], sizeof(Category), 1, file);
        fseek(file, sizeof(double) + sizeof(int) + sizeof(Category) * MAX_CATEGORIES + sizeof(double), SEEK_SET);
    }
    int tmp_count;
    if (fread(&tmp_count, sizeof(int), 1, file) != 1)
    {
        return 0;
    }
    tmp_count--;
    fseek(file, -sizeof(int), SEEK_CUR);
    fwrite(&tmp_count, sizeof(int), 1, file);

    TransactionNode *to_remove = sorted_transactions[index]; // parent -> transaction in LL

    // Remove from UI by shifting all transactions after it one position back
    for (int i = index; i < current_month_transaction_count - 1; i++)
    {
        sorted_transactions[i] = sorted_transactions[i + 1];
    } // we should implement the changes buffer thing here instead

    int remove_id = to_remove->index;

    if (to_remove == transaction_tail)
    {
        transaction_tail = to_remove->prev;
        transaction_tail->next = NULL;
        free(to_remove);
    }
    else
    {
        TransactionNode *last = transaction_tail;
        last->index = to_remove->index;
        transaction_tail = last->prev;
        last->next = to_remove->next;
        last->prev = to_remove->prev;
        free(to_remove);
        fseek(file, sizeof(double) + sizeof(int) + sizeof(Category) * MAX_CATEGORIES + sizeof(double) + sizeof(int) + sizeof(Transaction) * remove_id, SEEK_SET);
        fwrite(&last->data, sizeof(Transaction), 1, file);
    }

    int new_size = sizeof(double) + sizeof(int) + (sizeof(Category) * MAX_CATEGORIES) + sizeof(double) + sizeof(int) + (tmp_count * sizeof(Transaction));
    ftruncate(fileno(file), new_size);
    return 1;
}

int get_category_index(int year, int month, char *name)
{
    FILE *file = open_month_file(year, month);
    if (!file)
    {
        return -1;
    }
    // Read category count
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0)
    {
        fwrite(&default_monthly_budget, sizeof(double), 1, file);
        fwrite(&default_category_count, sizeof(int), 1, file);
        // fills with empty category data
        fwrite(&(Category){0}, sizeof(Category), MAX_CATEGORIES, file);
        // fills with default categories if there are any
        fseek(file, sizeof(double) + sizeof(int), SEEK_SET);
        fwrite(default_categories, sizeof(Category), default_category_count, file);
        fseek(file, 0, SEEK_END);
        fwrite(&(double){0}, sizeof(double), 1, file); // uncategorized spent
        fwrite(&(int){0}, sizeof(int), 1, file);       // number of transactions (0)
    }
    fseek(file, sizeof(double), SEEK_SET);
    int file_category_count = 0;
    if (fread(&file_category_count, sizeof(int), 1, file) != 1)
    {
        fclose(file);
        return -1;
    }
    // Read categories
    Category file_categories[MAX_CATEGORIES];
    if (fread(file_categories, sizeof(Category), file_category_count, file) != (size_t)file_category_count)
    {
        fclose(file);
        return -1;
    }
    fclose(file);
    for (int i = 0; i < file_category_count; i++)
    {
        if (strcmp(file_categories[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1; // Not found
}

// Reads categories for a given month from the savefile into out_categories and out_count, without modifying global state
int read_month_categories(int year, int month, Category *out_categories, int *out_count)
{
    FILE *file = open_month_file(year, month);
    if (!file)
        return -1;
    fseek(file, sizeof(double), SEEK_SET);
    if (fread(out_count, sizeof(int), 1, file) != 1)
    {
        return -1;
    }
    if (fread(out_categories, sizeof(Category), MAX_CATEGORIES, file) != (size_t)MAX_CATEGORIES)
    {
        return -1;
    }
    fclose(file);
    return 1;
}