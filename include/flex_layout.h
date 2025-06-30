#ifndef FLEX_LAYOUT_H
#define FLEX_LAYOUT_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>
#include "ui_helper.h"

// Flexbox-like layout system for ncurses windows
typedef enum
{
    FLEX_ROW,   // Arrange windows horizontally
    FLEX_COLUMN // Arrange windows vertically
} FlexDirection;

typedef enum
{
    FLEX_START,         // Pack items at the start of the container
    FLEX_END,           // Pack items at the end of the container
    FLEX_CENTER,        // Center items in the container
    FLEX_SPACE_BETWEEN, // Distribute items evenly with space between
    FLEX_SPACE_AROUND,  // Distribute items evenly with space around them
    FLEX_SPACE_EVENLY   // Distribute items with equal space between and at edges
} FlexJustify;

typedef enum
{
    FLEX_ALIGN_START,  // Align items at the start of the cross axis
    FLEX_ALIGN_END,    // Align items at the end of the cross axis
    FLEX_ALIGN_CENTER, // Center items along the cross axis
    FLEX_ALIGN_STRETCH // Stretch items to fill the cross axis
} FlexAlign;

typedef enum
{
    FLEX_ITEM_WINDOW,   // Item is a window that will be created
    FLEX_ITEM_CONTAINER // Item is a nested container
} FlexItemType;

// Forward declaration for nested structures
typedef struct FlexContainer FlexContainer;

// Properties for a flex item (window or container)
typedef struct
{
    FlexItemType type; // Type of item (window or nested container)

    // For both types:
    int flex_grow;  // Proportion to grow relative to siblings
    int flex_basis; // Base size (width or height depending on direction)
    int min_size;   // Minimum size
    int max_size;   // Maximum size (0 = no max)

    // For window items:
    bool is_active;            // Is this window active (for highlighting)
    char *title;               // Window title (NULL for no title)
    int align;                 // Title alignment
    BoundedWindow *window_ptr; // Pointer to store the created window

    // For container items:
    FlexContainer *container; // Nested container (if type is FLEX_ITEM_CONTAINER)
} FlexItem;

// Container for a group of flex items
struct FlexContainer
{
    FlexDirection direction; // Layout direction
    FlexJustify justify;     // Main axis distribution
    FlexAlign align;         // Cross axis alignment
    int gap;                 // Gap between items
    int padding;             // Padding inside container
    int item_count;          // Number of items in the container
    FlexItem *items;         // Array of items
};

// Create a flex window item
FlexItem flex_window(int flex_grow, int flex_basis,
                     const char *title, bool is_active, int align,
                     BoundedWindow *window_ptr);

// Create a flex window item with min/max constraints
FlexItem flex_window_constrained(int flex_grow, int flex_basis,
                                 int min_size, int max_size,
                                 const char *title, bool is_active, int align,
                                 BoundedWindow *window_ptr);

// Create a flex container item
FlexItem flex_container(int flex_grow, int flex_basis,
                        int min_size, int max_size,
                        FlexContainer *container);

// Create a new flex container
FlexContainer *create_flex_container(FlexDirection direction,
                                     FlexJustify justify,
                                     FlexAlign align,
                                     int gap, int padding,
                                     int item_count);

// Add an item to a container
void flex_container_add_item(FlexContainer *container, FlexItem item);

// Calculate and apply the layout to create windows
void apply_flex_layout(FlexContainer *root, int x, int y, int width, int height);

// Free memory allocated for containers and items
void free_flex_layout(FlexContainer *container);

#endif