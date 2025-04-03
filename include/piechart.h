#ifndef PIECHART_H
#define PIECHART_H

#include <ncurses.h>
#include <math.h>
#include <string.h>
#include "globals.h"
#include "ui.h"

// Define additional color pairs for the pie chart
#define PIE_COLOR_START 10  // Start from 10 to avoid conflicts with existing colors
#define NUM_PIE_COLORS 8    // Number of distinct colors used for pie chart - must match size of pie_colors array
#define PIE_DARKER_COLOR_START (PIE_COLOR_START + NUM_PIE_COLORS)

// Structure to hold a slice of the pie
typedef struct {
    double percentage;  // Percentage of the total
    int color_pair;     // Color pair to use
    char label[MAX_NAME_LEN];  // Label for the slice
} PieSlice;

// Function to initialize pie chart colors (call this before drawing)
void init_pie_chart_colors();

// Function to draw a pie chart of categories
void draw_pie_chart(WINDOW *win, int center_y, int center_x, double height, double width, PieSlice slices[], int slice_count);

// Function that displays both pie chart and legend
void display_budget_pie_chart(WINDOW *win, double width, double height);

#endif // PIECHART_H 