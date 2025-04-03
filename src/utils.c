#include "utils.h"

void write_export_content(FILE *export_file)
{
    // Write CSV headers
    fprintf(export_file, "TBudget Export\n\n");

    // Total budget
    fprintf(export_file, "%s %d Budget:,%.2f\n\n", month_names[current_month], current_year, total_budget);

    // Categories
    fprintf(export_file, "CATEGORIES\n");
    fprintf(export_file, "Name,Amount,Percentage\n");

    double total_allocated = 0.0;
    for (int i = 0; i < category_count; i++)
    {
        total_allocated += categories[i].budget;
    }

    for (int i = 0; i < category_count; i++)
    {
        double percentage = 0.0;
        if (total_budget > 0)
        {
            percentage = (categories[i].budget / total_budget) * 100.0;
        }

        // Escape category names with quotes if they contain commas
        if (strchr(categories[i].name, ',') != NULL)
        {
            fprintf(export_file, "\"%s\",%.2f,%.2f%%\n",
                    categories[i].name, categories[i].budget, percentage);
        }
        else
        {
            fprintf(export_file, "%s,%.2f,%.2f%%\n",
                    categories[i].name, categories[i].budget, percentage);
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

    // for (int i = 0; i < transaction_count; i++)
    // {
    //     char category_name[MAX_NAME_LEN] = "Uncategorized";
    //     strcpy(category_name, transactions[i].cat_name);

    //     // Escape descriptions with quotes if they contain commas
    //     if (strchr(transactions[i].desc, ',') != NULL)
    //     {
    //         fprintf(export_file, "%d,\"%s\",%.2f,\"%s\",%s\n",
    //                 i + 1, transactions[i].desc, transactions[i].amt,
    //                 category_name, transactions[i].date);
    //     }
    //     else if (strchr(category_name, ',') != NULL)
    //     {
    //         // If category name has a comma, we need to quote it
    //         fprintf(export_file, "%d,%s,%.2f,\"%s\",%s\n",
    //                 i + 1, transactions[i].desc, transactions[i].amt,
    //                 category_name, transactions[i].date);
    //     }
    //     else
    //     {
    //         fprintf(export_file, "%d,%s,%.2f,%s,%s\n",
    //                 i + 1, transactions[i].desc, transactions[i].amt,
    //                 category_name, transactions[i].date);
    //     }
    // }
}

void export_data_to_csv(int silent)
{
    // Create the main export file
    FILE *file = fopen(export_file_path, "w");
    if (!file)
    {
        if (!silent)
        {
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
    if (history_file)
    {
        write_export_content(history_file);
        fclose(history_file);

        if (!silent)
        {
            fprintf(stderr, "Created timestamped backup: %s\n", timestamp_filename);
        }
    }

    // Return success message to stderr for visibility, but only if not silent
    if (!silent)
    {
        fprintf(stderr, "Successfully exported data to %s\n", export_file_path);
    }
}

void import_data_from_csv(const char *filename) // broken
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Unable to open import file: %s\n", filename);
        return;
    }

    // Reset data
    category_count = 0;
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
                categories[category_count].budget = amount;
                category_count++;
            }
        }
        else if (section == 3 && current_month_transaction_count < MAX_TRANSACTIONS)
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
                    categories[category_count].budget = 0.0;
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
                strncpy(current_month_transactions[current_month_transaction_count].desc, description, MAX_NAME_LEN - 1);
                current_month_transactions[current_month_transaction_count].desc[MAX_NAME_LEN - 1] = '\0';
                current_month_transactions[current_month_transaction_count].amt = amount;
                strncpy(current_month_transactions[current_month_transaction_count].cat_name, category_name, MAX_NAME_LEN - 1);
                current_month_transactions[current_month_transaction_count].cat_name[MAX_NAME_LEN - 1] = '\0';
                strncpy(current_month_transactions[current_month_transaction_count].date, date, 10);
                current_month_transactions[current_month_transaction_count].date[10] = '\0';
                current_month_transaction_count++;
            }
        }
    }

    fclose(file);

    // Return success message to stderr for visibility
    fprintf(stderr, "Successfully imported data from %s\n", filename);
}

void save_data_to_file()
{
    FILE *file = fopen(data_file_path, "wb");
    if (!file)
    {
        // If unable to open file, just return silently
        // In a production app, we'd want to notify the user
        return;
    }

    // Write a simple header for file format validation
    const char header[] = "TBUDGET_DATA";
    fwrite(header, sizeof(char), strlen(header), file);

    // Write version for future compatibility
    const int version = 2;  // Increment version for new format
    fwrite(&version, sizeof(int), 1, file);

    // Write total budget
    fwrite(&total_budget, sizeof(double), 1, file);

    // Write category count and categories
    fwrite(&category_count, sizeof(int), 1, file);
    fwrite(categories, sizeof(Category), category_count, file);

    // Write month index information
    fwrite(&month_index_manager.count, sizeof(int), 1, file);
    fwrite(month_index_manager.indexes, sizeof(MonthIndex), month_index_manager.count, file);

    // Write subscription count and subscriptions
    fwrite(&subscription_count, sizeof(int), 1, file);
    fwrite(subscriptions, sizeof(Subscription), subscription_count, file);

    fclose(file);
}

int load_data_from_file()
{
    FILE *file = fopen(data_file_path, "rb");
    if (!file)
    {
        // If file doesn't exist, that's okay - we'll start with empty data
        init_month_index_manager();
        return 0;
    }

    // Read and validate header
    char header[13] = {0}; // "TBUDGET_DATA" is 12 chars + null terminator
    if (fread(header, sizeof(char), 12, file) != 12 || strcmp(header, "TBUDGET_DATA") != 0)
    {
        // Invalid file format
        fclose(file);
        init_month_index_manager();
        return -2;
    }

    // Read version
    int version;
    if (fread(&version, sizeof(int), 1, file) != 1)
    {
        fclose(file);
        init_month_index_manager();
        return -1;
    }

    // Read total budget
    if (fread(&total_budget, sizeof(double), 1, file) != 1)
    {
        fclose(file);
        init_month_index_manager();
        return -1;
    }

    // Read category count and validate
    if (fread(&category_count, sizeof(int), 1, file) != 1 || 
        category_count > MAX_CATEGORIES || category_count < 0)
    {
        fclose(file);
        init_month_index_manager();
        return -1;
    }

    // Read categories
    if (fread(categories, sizeof(Category), category_count, file) != (size_t)category_count)
    {
        fclose(file);
        init_month_index_manager();
        return -1;
    }

    if (version >= 2)
    {
        // New format with month indexes
        int index_count;
        if (fread(&index_count, sizeof(int), 1, file) != 1)
        {
            fclose(file);
            init_month_index_manager();
            return -1;
        }

        // Initialize month index manager with loaded count
        month_index_manager.capacity = index_count > 10 ? index_count : 10;
        month_index_manager.indexes = malloc(sizeof(MonthIndex) * month_index_manager.capacity);
        if (!month_index_manager.indexes)
        {
            fclose(file);
            return -1;
        }
        month_index_manager.count = index_count;

        // Read month indexes
        if (fread(month_index_manager.indexes, sizeof(MonthIndex), index_count, file) != (size_t)index_count)
        {
            free_month_index_manager();
            fclose(file);
            return -1;
        }
    }
    else
    {
        // Old format - need to convert existing transactions
        int old_transaction_count;
        if (fread(&old_transaction_count, sizeof(int), 1, file) != 1 || 
            old_transaction_count > MAX_TRANSACTIONS || old_transaction_count < 0)
        {
            fclose(file);
            init_month_index_manager();
            return -1;
        }

        // Initialize month index manager
        init_month_index_manager();

        // Read and convert old transactions
        Transaction old_trans;
        for (int i = 0; i < old_transaction_count; i++)
        {
            if (fread(&old_trans, sizeof(Transaction), 1, file) != 1)
            {
                free_month_index_manager();
                fclose(file);
                return -1;
            }
            add_transaction(&old_trans);
        }
    }

    // Read subscription count and validate
    if (fread(&subscription_count, sizeof(int), 1, file) != 1 || 
        subscription_count > MAX_SUBSCRIPTIONS || subscription_count < 0)
    {
        subscription_count = 0;
    }
    else
    {
        // Read subscriptions
        if (fread(subscriptions, sizeof(Subscription), subscription_count, file) != (size_t)subscription_count)
        {
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    // Load current month's transactions
    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    current_month = today->tm_mon + 1;
    current_year = today->tm_year + 1900;
    load_month_transactions(current_year, current_month);

    return 1;
}

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

    // Create file paths
    sprintf(data_file_path, "%s/%s", app_data_dir, DATA_FILE_NAME);
    sprintf(export_file_path, "%s/%s", app_data_dir, EXPORT_FILE_NAME);

    // Create directories
    create_directory_if_not_exists(app_data_dir);
    create_directory_if_not_exists(data_storage_dir);
}

// Helper function to get days in a month
int get_days_in_month(int m, int y)
{
    // Array of days in each month
    static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
        return 29; // Leap year
    return days_in_month[m];
}

// Helper function to validate day value
int validate_day(int day, int month, int year)
{
    int max_days = get_days_in_month(month, year);
    if (day > max_days)
    {
        return 1; // Wrap around to 1
    }
    else if (day < 1)
    {
        return max_days; // Wrap to max days
    }
    return day;
}

void compute_monthly(int month, int year)
{
    if (month == 0 || year == 0)
    {
        time_t now = time(NULL);
        struct tm *today = localtime(&now);
        month = today->tm_mon + 1;
        year = today->tm_year + 1900;
    }

    // Reset all category spending
    for (int i = 0; i < category_count; i++)
    {
        categories[i].spent = 0.0;
    }

    // Load the requested month's transactions if not already loaded
    if (month != loaded_month || year != loaded_year)
    {
        if (!load_month_transactions(year, month))
        {
            return;
        }
        loaded_month = month;
        loaded_year = year;
    }

    // Process all transactions for this month
    for (int i = 0; i < current_month_transaction_count; i++)
    {
        char *category_name = current_month_transactions[i].cat_name;
        for (int j = 0; j < category_count; j++)
        {
            if (strcmp(categories[j].name, category_name) == 0)
            {
                if (current_month_transactions[i].expense)
                {
                    categories[j].spent += current_month_transactions[i].amt;
                }
                else
                {
                    categories[j].spent -= current_month_transactions[i].amt;
                }
                break;
            }
        }
    }
}

void sort_categories_by_budget() {
    for (int i = 0; i < category_count; i++) {
        for (int j = i + 1; j < category_count; j++) {
            if (categories[i].budget < categories[j].budget) {
                Category temp = categories[i];
                categories[i] = categories[j];
                categories[j] = temp;
            }
        }
    }
}

void sort_categories_by_spent() {
    for (int i = 0; i < category_count; i++) {
        for (int j = i + 1; j < category_count; j++) {
            if (categories[i].spent < categories[j].spent) {
                Category temp = categories[i];
                categories[i] = categories[j];
                categories[j] = temp;
            }
        }
    }
}
