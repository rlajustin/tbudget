#include "transaction_manager.h"

// Initialize the month index manager
void init_month_index_manager()
{
    month_index_manager.capacity = 10; // Start with space for 10 months
    month_index_manager.indexes = malloc(sizeof(MonthIndex) * month_index_manager.capacity);
    month_index_manager.count = 0;
}

// Free the month index manager
void free_month_index_manager()
{
    free(month_index_manager.indexes);
    month_index_manager.indexes = NULL;
    month_index_manager.count = 0;
    month_index_manager.capacity = 0;
}

// Compare function for qsort
int compare_transactions_by_date(const void *a, const void *b)
{
    const Transaction *ta = (const Transaction *)a;
    const Transaction *tb = (const Transaction *)b;
    return strcmp(tb->date, ta->date); // Reverse order for most recent first
}

// Sort current month's transactions by date
void sort_current_month_transactions()
{
    if (!current_month_transactions || current_month_transaction_count <= 1)
        return;

    qsort(current_month_transactions, current_month_transaction_count,
          sizeof(Transaction), compare_transactions_by_date);
}

// Add a new month index, expanding capacity if needed
void add_month_index(int year, int month, long file_position, int transaction_count)
{
    if (month_index_manager.count == month_index_manager.capacity)
    {
        month_index_manager.capacity *= 2;
        MonthIndex *new_indexes = realloc(month_index_manager.indexes,
                                          sizeof(MonthIndex) * month_index_manager.capacity);
        if (!new_indexes)
        {
            // Handle allocation failure
            return;
        }
        month_index_manager.indexes = new_indexes;
    }

    MonthIndex new_index = {
        .year = year,
        .month = month,
        .file_position = file_position,
        .transaction_count = transaction_count,
        .loaded = false};

    month_index_manager.indexes[month_index_manager.count++] = new_index;
}

// Find a month index in the manager
MonthIndex *find_month_index(int year, int month)
{
    for (int i = 0; i < month_index_manager.count; i++)
    {
        if (month_index_manager.indexes[i].year == year &&
            month_index_manager.indexes[i].month == month)
        {
            return &month_index_manager.indexes[i];
        }
    }
    return NULL;
}

// Load transactions for a specific month
bool load_month_transactions(int year, int month)
{
    // Free current month's transactions if they exist
    if (current_month_transactions)
    {
        free(current_month_transactions);
        current_month_transactions = NULL;
        current_month_transaction_count = 0;
    }

    MonthIndex *index = find_month_index(year, month);
    if (!index)
    {
        // No transactions for this month
        return true;
    }

    FILE *file = fopen(data_file_path, "rb");
    if (!file)
        return false;

    // Seek to the start of this month's transactions
    fseek(file, index->file_position, SEEK_SET);

    // Allocate memory for transactions
    current_month_transactions = malloc(sizeof(Transaction) * index->transaction_count);
    if (!current_month_transactions)
    {
        fclose(file);
        return false;
    }

    // Read all transactions for this month
    if (fread(current_month_transactions, sizeof(Transaction),
              index->transaction_count, file) != (size_t)index->transaction_count)
    {
        free(current_month_transactions);
        current_month_transactions = NULL;
        fclose(file);
        return false;
    }

    current_month_transaction_count = index->transaction_count;
    index->loaded = true;
    fclose(file);
    return true;
}

// Add a new transaction to the current month
bool add_transaction(Transaction *new_transaction)
{
    // Ensure we're working with the correct month's data
    char year_str[5], month_str[3];
    strncpy(year_str, new_transaction->date, 4);
    year_str[4] = '\0';
    strncpy(month_str, new_transaction->date + 5, 2);
    month_str[2] = '\0';

    int trans_year = atoi(year_str);
    int trans_month = atoi(month_str);

    // Find or create month index
    MonthIndex *index = find_month_index(trans_year, trans_month);
    if (!index)
    {
        // This is a new month - need to create index and update file
        FILE *file = fopen(data_file_path, "ab");
        if (!file)
            return false;

        // Get position for new month's transactions
        fseek(file, 0, SEEK_END);
        long position = ftell(file);

        // Add new month index
        add_month_index(trans_year, trans_month, position, 1);

        // Write the transaction
        fwrite(new_transaction, sizeof(Transaction), 1, file);
        fclose(file);

        // Load this month's transactions
        return load_month_transactions(trans_year, trans_month);
    }

    // Existing month - append transaction to file
    FILE *file = fopen(data_file_path, "r+b");
    if (!file)
        return false;

    // Seek to end of this month's transactions
    fseek(file, index->file_position + (index->transaction_count * sizeof(Transaction)), SEEK_SET);

    // Write the new transaction
    fwrite(new_transaction, sizeof(Transaction), 1, file);
    index->transaction_count++;

    // Update in-memory array
    if (trans_year == current_year && trans_month == current_month)
    {
        Transaction *new_array = realloc(current_month_transactions,
                                         sizeof(Transaction) * index->transaction_count);
        if (!new_array)
        {
            fclose(file);
            return false;
        }
        current_month_transactions = new_array;
        current_month_transactions[current_month_transaction_count++] = *new_transaction;
        fclose(file);
    }

    return true;
}

// Navigate to next/previous month
bool navigate_month(int direction)
{ // direction: 1 for next, -1 for previous
    int new_month = current_month + direction;
    int new_year = current_year;

    if (new_month > 12)
    {
        new_month = 1;
        new_year++;
    }
    else if (new_month < 1)
    {
        new_month = 12;
        new_year--;
    }

    // Try to load the new month
    if (load_month_transactions(new_year, new_month))
    {
        current_month = new_month;
        current_year = new_year;
        return true;
    }
    return false;
}