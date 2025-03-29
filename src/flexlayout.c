#include "flexlayout.h"
#include <stdlib.h>
#include <string.h>

// Create a flex window item
FlexItem flex_window(int flex_grow, int flex_basis, 
                    const char *title, bool is_active, int align, 
                    BoundedWindow *window_ptr) {
    FlexItem item = {
        .type = FLEX_ITEM_WINDOW,
        .flex_grow = flex_grow,
        .flex_basis = flex_basis,
        .min_size = 0,
        .max_size = 0,
        .is_active = is_active,
        .title = (char*)title,
        .align = align,
        .window_ptr = window_ptr,
        .container = NULL
    };
    return item;
}

// Create a flex window item with min/max constraints
FlexItem flex_window_constrained(int flex_grow, int flex_basis, 
                                int min_size, int max_size,
                                const char *title, bool is_active, int align, 
                                BoundedWindow *window_ptr) {
    FlexItem item = {
        .type = FLEX_ITEM_WINDOW,
        .flex_grow = flex_grow,
        .flex_basis = flex_basis,
        .min_size = min_size,
        .max_size = max_size,
        .is_active = is_active,
        .title = (char*)title,
        .align = align,
        .window_ptr = window_ptr,
        .container = NULL
    };
    return item;
}

// Create a flex container item
FlexItem flex_container(int flex_grow, int flex_basis, 
                       int min_size, int max_size,
                       FlexContainer *container) {
    FlexItem item = {
        .type = FLEX_ITEM_CONTAINER,
        .flex_grow = flex_grow,
        .flex_basis = flex_basis,
        .min_size = min_size,
        .max_size = max_size,
        .is_active = false,
        .title = NULL,
        .align = 0,
        .window_ptr = NULL,
        .container = container
    };
    return item;
}

// Create a new flex container
FlexContainer *create_flex_container(FlexDirection direction, 
                                    FlexJustify justify, 
                                    FlexAlign align,
                                    int gap, int padding, 
                                    int item_count) {
    FlexContainer *container = (FlexContainer*)malloc(sizeof(FlexContainer));
    if (container == NULL) {
        return NULL;  // Memory allocation failed
    }
    
    container->direction = direction;
    container->justify = justify;
    container->align = align;
    container->gap = gap;
    container->padding = padding;
    container->item_count = 0;
    
    // Allocate memory for items
    container->items = (FlexItem*)malloc(sizeof(FlexItem) * item_count);
    if (container->items == NULL) {
        free(container);
        return NULL;  // Memory allocation failed
    }
    
    return container;
}

// Add an item to a container
void flex_container_add_item(FlexContainer *container, FlexItem item) {
    if (container != NULL) {
        container->items[container->item_count++] = item;
    }
}

// Helper function for layout calculation - recursive
void calculate_layout_recursive(FlexContainer *container, 
                               int x, int y, int width, int height) {
    if (container == NULL || container->item_count == 0) {
        return;
    }
    
    // Account for container padding
    x += container->padding;
    y += container->padding;
    width -= container->padding * 2;
    height -= container->padding * 2;
    
    // Calculate total flex_grow and fixed sizes
    int total_flex_grow = 0;
    int total_fixed_size = 0;
    
    for (int i = 0; i < container->item_count; i++) {
        if (container->items[i].flex_grow > 0) {
            total_flex_grow += container->items[i].flex_grow;
        } else if (container->items[i].flex_basis > 0) {
            // For items with fixed size
            total_fixed_size += (container->direction == FLEX_ROW) ? 
                container->items[i].flex_basis : container->items[i].flex_basis;
        }
    }
    
    // Calculate available space after fixed items and gaps
    int available_space = (container->direction == FLEX_ROW) ? 
        width - total_fixed_size - (container->gap * (container->item_count - 1)) :
        height - total_fixed_size - (container->gap * (container->item_count - 1));
    
    // Calculate sizes for each item
    int sizes[container->item_count];
    for (int i = 0; i < container->item_count; i++) {
        if (container->items[i].flex_grow > 0) {
            // Calculate flexible size
            int flex_size = (available_space * container->items[i].flex_grow) / total_flex_grow;
            
            // Apply min/max constraints
            if (container->items[i].min_size > 0 && flex_size < container->items[i].min_size) {
                sizes[i] = container->items[i].min_size;
            } else if (container->items[i].max_size > 0 && flex_size > container->items[i].max_size) {
                sizes[i] = container->items[i].max_size;
            } else {
                sizes[i] = flex_size;
            }
        } else if (container->items[i].flex_basis > 0) {
            // Fixed size
            sizes[i] = container->items[i].flex_basis;
        } else {
            // Default minimum size
            sizes[i] = (container->direction == FLEX_ROW) ? 10 : 3;
        }
    }
    
    // Calculate positions based on justify
    int positions[container->item_count];
    
    switch (container->justify) {
        case FLEX_START:
            // Items at the start
            positions[0] = 0;
            for (int i = 1; i < container->item_count; i++) {
                positions[i] = positions[i-1] + sizes[i-1] + container->gap;
            }
            break;
            
        case FLEX_END:
            // Items at the end
            positions[container->item_count-1] = (container->direction == FLEX_ROW) ? 
                width - sizes[container->item_count-1] : height - sizes[container->item_count-1];
            for (int i = container->item_count-2; i >= 0; i--) {
                positions[i] = positions[i+1] - sizes[i] - container->gap;
            }
            break;
            
        case FLEX_CENTER: {
            // Calculate total used space
            int total_size = 0;
            for (int i = 0; i < container->item_count; i++) {
                total_size += sizes[i];
            }
            total_size += (container->item_count - 1) * container->gap;
            
            // Center the group
            int start_pos = ((container->direction == FLEX_ROW) ? width : height) / 2 - total_size / 2;
            if (start_pos < 0) start_pos = 0;
            
            positions[0] = start_pos;
            for (int i = 1; i < container->item_count; i++) {
                positions[i] = positions[i-1] + sizes[i-1] + container->gap;
            }
            break;
        }
            
        case FLEX_SPACE_BETWEEN:
            if (container->item_count > 1) {
                int total_item_size = 0;
                for (int i = 0; i < container->item_count; i++) {
                    total_item_size += sizes[i];
                }
                
                int total_space = (container->direction == FLEX_ROW) ? width : height;
                int total_gap = total_space - total_item_size;
                int gap_size = total_gap / (container->item_count - 1);
                
                positions[0] = 0;
                for (int i = 1; i < container->item_count; i++) {
                    positions[i] = positions[i-1] + sizes[i-1] + gap_size;
                }
            } else {
                positions[0] = 0;
            }
            break;
            
        case FLEX_SPACE_AROUND:
            if (container->item_count > 0) {
                int total_item_size = 0;
                for (int i = 0; i < container->item_count; i++) {
                    total_item_size += sizes[i];
                }
                
                int total_space = (container->direction == FLEX_ROW) ? width : height;
                int total_gap = total_space - total_item_size;
                int gap_size = total_gap / (container->item_count * 2);
                
                positions[0] = gap_size;
                for (int i = 1; i < container->item_count; i++) {
                    positions[i] = positions[i-1] + sizes[i-1] + (gap_size * 2);
                }
            }
            break;
            
        case FLEX_SPACE_EVENLY:
            if (container->item_count > 0) {
                int total_item_size = 0;
                for (int i = 0; i < container->item_count; i++) {
                    total_item_size += sizes[i];
                }
                
                int total_space = (container->direction == FLEX_ROW) ? width : height;
                int total_gap = total_space - total_item_size;
                int gap_size = total_gap / (container->item_count + 1);
                
                positions[0] = gap_size;
                for (int i = 1; i < container->item_count; i++) {
                    positions[i] = positions[i-1] + sizes[i-1] + gap_size;
                }
            }
            break;
    }
    
    // Process all items
    for (int i = 0; i < container->item_count; i++) {
        int item_x, item_y, item_width, item_height;
        
        if (container->direction == FLEX_ROW) {
            item_x = x + positions[i];
            item_width = sizes[i];
            
            // Handle vertical alignment
            if (container->align == FLEX_ALIGN_STRETCH) {
                item_height = height;
                item_y = y;
            } else {
                item_height = (container->items[i].flex_basis > 0 && 
                              container->items[i].type == FLEX_ITEM_WINDOW) ? 
                              container->items[i].flex_basis : height;
                
                switch (container->align) {
                    case FLEX_ALIGN_START:
                        item_y = y;
                        break;
                    case FLEX_ALIGN_END:
                        item_y = y + height - item_height;
                        break;
                    case FLEX_ALIGN_CENTER:
                        item_y = y + (height - item_height) / 2;
                        break;
                    default:
                        item_y = y;
                }
            }
        } else { // FLEX_COLUMN
            item_y = y + positions[i];
            item_height = sizes[i];
            
            // Handle horizontal alignment
            if (container->align == FLEX_ALIGN_STRETCH) {
                item_width = width;
                item_x = x;
            } else {
                item_width = (container->items[i].flex_basis > 0 && 
                             container->items[i].type == FLEX_ITEM_WINDOW) ?
                             container->items[i].flex_basis : width;
                
                switch (container->align) {
                    case FLEX_ALIGN_START:
                        item_x = x;
                        break;
                    case FLEX_ALIGN_END:
                        item_x = x + width - item_width;
                        break;
                    case FLEX_ALIGN_CENTER:
                        item_x = x + (width - item_width) / 2;
                        break;
                    default:
                        item_x = x;
                }
            }
        }
        
        // Process the item based on its type
        if (container->items[i].type == FLEX_ITEM_WINDOW) {
            // Calculate actual window size accounting for BoundedWindow borders
            int text_height = item_height - 4;  // Account for border and title
            int text_width = item_width - 4;    // Account for border padding
            
            // Ensure minimum size
            if (text_height < 1) text_height = 1;
            if (text_width < 1) text_width = 1;
            
            // Create window
            if (container->items[i].window_ptr != NULL) {
                // Account for the borders in the draw_bounded_with_title function
                // by adjusting the coordinates and sizes accordingly
                *container->items[i].window_ptr = draw_bounded_with_title(
                    text_height,
                    text_width,
                    item_y + 2,  // Adjusted for border
                    item_x + 2,  // Adjusted for border
                    container->items[i].title,
                    container->items[i].is_active,
                    container->items[i].align
                );
            }
        } else if (container->items[i].type == FLEX_ITEM_CONTAINER && 
                  container->items[i].container != NULL) {
            // Recursively layout nested container
            calculate_layout_recursive(
                container->items[i].container,
                item_x,
                item_y,
                item_width,
                item_height
            );
        }
    }
}

// Calculate and apply the layout
void apply_flex_layout(FlexContainer *root, int x, int y, int width, int height) {
    calculate_layout_recursive(root, x, y, width, height);
}

// Free memory allocated for containers and items - recursive
void free_flex_layout_recursive(FlexContainer *container) {
    if (container == NULL) {
        return;
    }
    
    // Free nested containers first
    for (int i = 0; i < container->item_count; i++) {
        if (container->items[i].type == FLEX_ITEM_CONTAINER && 
            container->items[i].container != NULL) {
            free_flex_layout_recursive(container->items[i].container);
        }
    }
    
    // Free the items array
    free(container->items);
    
    // Free the container itself
    free(container);
}

// Public interface to free layout memory
void free_flex_layout(FlexContainer *container) {
    free_flex_layout_recursive(container);
} 