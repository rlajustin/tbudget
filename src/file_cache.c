#include "file_cache.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h> // for access()

static FileCache file_cache = {0};

void init_file_cache(void)
{
    file_cache.count = 0;
    memset(file_cache.files, 0, sizeof(file_cache.files));
}

int cleanup_file_cache(void)
{
    for (int i = 0; i < file_cache.count; i++)
    {
        if (file_cache.files[i].fp)
        {
            if (fclose(file_cache.files[i].fp) < 0)
            {
                return -1;
            }
        }
    }
    file_cache.count = 0;
    return 0;
}

FILE *open_file(const char *relative_file_path, bool create_if_not_exists)
{
    // Check if file is already cached
    for (int i = 0; i < file_cache.count; i++)
    {
        if (strcmp(file_cache.files[i].relative_file_path, relative_file_path) == 0)
        {
            file_cache.files[i].last_accessed = time(NULL);
            return file_cache.files[i].fp;
        }
    }

    // If cache is full, remove oldest entry
    if (file_cache.count >= MAX_CACHED_FILES)
    {
        remove_oldest_cached_file();
    }

    // Add new cache entry
    char *path = malloc(MAX_BUFFER);
    sprintf(path, "%s/%s", data_storage_dir, relative_file_path);
    strncpy(file_cache.files[file_cache.count].relative_file_path, relative_file_path, MAX_BUFFER - 1);
    FILE *fp = NULL;

    if (access(path, F_OK) == 0)
    {
        // File exists
        fp = fopen(path, "r+b");
    }
    else if (create_if_not_exists)
    {
        // File does not exist, create it
        fp = fopen(path, "w+b");
    }
    else
    {
        // File does not exist and we don't want to create it
        free(path);
        return NULL;
    }

    if (fp == NULL)
    {
        free(path);
        return NULL;
    }
    file_cache.files[file_cache.count].fp = fp;
    file_cache.files[file_cache.count].last_accessed = time(NULL);
    file_cache.count++;
    free(path);
    return fp;
}

void remove_oldest_cached_file(void)
{
    if (file_cache.count == 0)
        return;

    int oldest_idx = 0;
    time_t oldest_time = file_cache.files[0].last_accessed;

    // Find oldest entry
    for (int i = 1; i < file_cache.count; i++)
    {
        if (file_cache.files[i].last_accessed < oldest_time)
        {
            oldest_time = file_cache.files[i].last_accessed;
            oldest_idx = i;
        }
    }

    // Free the data
    fclose(file_cache.files[oldest_idx].fp);

    // Shift remaining entries
    for (int i = oldest_idx; i < file_cache.count - 1; i++)
    {
        file_cache.files[i] = file_cache.files[i + 1];
    }

    file_cache.count--;
}

void cache_recent_months(void)
{
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    int current_year = current_time->tm_year + 1900;
    int current_month = current_time->tm_mon + 1;

    // Cache the last 10 months
    for (int i = 0; i < 12; i++)
    {
        int year = current_year;
        int month = current_month - i;

        // Handle year rollover
        if (month <= 0)
        {
            month += 12;
            year--;
        }

        char relative_file_path[MAX_BUFFER];
        sprintf(relative_file_path, "%d-%d.dat", year, month);

        // Try to load the month's data
        open_file(relative_file_path, false);
    }
}

FILE *open_month_file(int year, int month)
{
    char relative_file_path[MAX_BUFFER];
    sprintf(relative_file_path, "%d-%d.dat", year, month);
    return open_file(relative_file_path, true);
}