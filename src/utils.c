#include "utils.h"

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
int get_days_in_month(int m, int y) {
    // Array of days in each month
    static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
        return 29; // Leap year
    return days_in_month[m];
}

// Helper function to validate day value
int validate_day(int day, int month, int year) {
    int max_days = get_days_in_month(month, year);
    if (day > max_days) {
        return 1; // Wrap around to 1
    } else if (day < 1) {
        return max_days; // Wrap to max days
    }
    return day;
}