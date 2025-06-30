#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <time.h>
#include "globals.h"

// hopefully the rest is just handled by the OS caching system . . . fopen is slow right

#define MAX_CACHED_FILES 12 // cache the past 12 months

typedef struct {
    char relative_file_path[MAX_BUFFER];
    time_t last_accessed;
    FILE* fp;
} CachedFile;

typedef struct {
    CachedFile files[MAX_CACHED_FILES];
    int count;
} FileCache;

// Function prototypes
void init_file_cache(void);
int cleanup_file_cache(void);
FILE *open_file(const char *relative_file_path, bool create_if_not_exists);
void remove_oldest_cached_file(void);
void cache_recent_months(void);
FILE* open_month_file(int year, int month);

#endif // FILE_CACHE_H 