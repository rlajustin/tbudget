#include "piechart.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

// Array of possible slice colors - now foreground and background are the same for solid color blocks
static const short pie_colors[][2] = {
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_YELLOW},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_GREEN},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_MAGENTA},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_CYAN},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_RED},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_BLUE},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_BLACK},
    {OVERRIDE_COLOR_WHITE, OVERRIDE_COLOR_WHITE},
};

static const short pie_colors_darker[][2] = {
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_YELLOW},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_GREEN},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_MAGENTA},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_CYAN},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_RED},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_BLUE},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_BLACK},
    {OVERRIDE_COLOR_BLACK, OVERRIDE_COLOR_WHITE},
};

// These are now defined in piechart.h
// #define NUM_PIE_COLORS (sizeof(pie_colors) / sizeof(pie_colors[0]))
// #define PIE_DARKER_COLOR_START (PIE_COLOR_START + NUM_PIE_COLORS)

void init_pie_chart_colors()
{
    // Initialize color pairs for the pie chart - normal colors
    for (int i = 0; i < (int)NUM_PIE_COLORS; i++)
    {
        init_pair(PIE_COLOR_START + i, pie_colors[i][0], pie_colors[i][1]);
    }
    for (int i = 0; i < (int)NUM_PIE_COLORS; i++)
    {
        init_pair(PIE_DARKER_COLOR_START + i, pie_colors_darker[i][0], pie_colors_darker[i][1]);
    }
}

// Function to determine if a point is inside an angle range
static bool is_point_in_slice(double angle, double start_angle, double end_angle)
{
    // Handle slice that crosses 0/360 degree
    if (start_angle > end_angle)
    {
        return (angle >= start_angle && angle <= 360.0) ||
               (angle >= 0.0 && angle <= end_angle);
    }
    return (angle >= start_angle && angle <= end_angle);
}

// Get the appropriate character and determine which color pair to use based on distance from center
static int get_pie_char(bool darker)
{
    if (darker)
    {
        return '#';
    }
    else
    {
        return ' ';
    }
}

void draw_pie_chart(WINDOW *win, int center_y, int center_x, double height, double width, PieSlice slices[], int slice_count)
{

    // 3D effect parameters
    double x_radius = width / 2;  // Horizontal radius (wider)
    double y_radius = height / 2; // Vertical radius (shorter for perspective)
    double tilt = atan(y_radius / x_radius);

    // For each position in a rectangular area around the center
    for (int y = center_y - y_radius; y <= center_y + y_radius; y++)
    {
        for (int x = center_x - x_radius; x <= center_x + x_radius; x++)
        {
            // Calculate normalized coordinates relative to center
            double dx = (double)(x - center_x);
            double dy = (double)(y - center_y);

            // Normalized coordinates for quadrant determination (-1 to 1 range)
            double norm_x = dx / x_radius;
            double norm_y = dy / y_radius;

            // Calculate if point is within the ellipse
            // Formula: (x/a)² + (y/b)² <= 1
            if ((dx * dx) / (x_radius * x_radius) + (dy * dy) / (y_radius * y_radius) <= 1.0)
            {
                // Convert 2D ellipse coordinates to 3D circle coordinates with perspective tilt
                // First, calculate the angle on the ellipse
                double ellipse_angle = atan2(dy, dx);

                // Calculate the radial distance from center (0-1 range)
                double distance = sqrt(norm_x * norm_x + norm_y * norm_y);

                // Adjust the angle based on the 3D perspective effect
                // The adjustment is stronger at the edges and diminishes toward the center
                double perspective_factor = sin(ellipse_angle) * sin(tilt) * (distance * 0.5);

                // Calculate the final 3D-adjusted angle
                double angle = atan2(dy / y_radius, (dx / x_radius) - perspective_factor) * 180.0 / PI;
                if (angle < 0)
                    angle += 360.0;

                // Find which slice this point belongs to
                double slice_start = 0.0;
                bool found = false;

                for (int i = 0; i < slice_count; i++)
                {
                    double slice_angle = (slices[i].percentage / 100.0) * 360.0;
                    double slice_end = slice_start + slice_angle;

                    if (is_point_in_slice(angle, slice_start, slice_end))
                    {
                        // Determine if we should use the darker character variant
                        // for bottom portion of the pie chart (shadow effect)
                        bool use_darker = sqrt(norm_x * norm_x * 0.6 + norm_y * norm_y) > 0.78 && dy > 0;

                        int ch = get_pie_char(use_darker);

                        int color = slices[i].color_pair;

                        wattron(win, COLOR_PAIR(color));
                        mvwaddch(win, y, x, ch);
                        wattroff(win, COLOR_PAIR(color));
                        found = true;
                        break;
                    }

                    slice_start = slice_end;
                }

                // If not in any slice (rounding errors), use the last slice color
                if (!found && slice_count > 0)
                {
                    int ch = get_pie_char(false);

                    int color = slices[slice_count - 1].color_pair;

                    wattron(win, COLOR_PAIR(color));
                    mvwaddch(win, y, x, ch);
                    wattroff(win, COLOR_PAIR(color));
                }
            }
        }
    }
}

void display_budget_pie_chart(WINDOW *win, double width, double height)
{
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    // Shift the pie chart center down by adjusting center_y
    int center_y = max_y / 2 - 3;
    int center_x = max_x / 2;

    // Draw the pie chart
    init_pie_chart_colors();

    if (category_count == 0)
    {
        mvwprintw(win, center_y, center_x - 10, "No budget categories defined");
        return;
    }

    PieSlice slices[NUM_PIE_COLORS] = {0};
    int slice_count = 0;
    double used_budget = 0.0;

    for (int i = 0; i < category_count; i++)
    {
        if (categories[i].budget > 0)
        {
            used_budget += categories[i].budget;
            if (i >= NUM_PIE_COLORS - 1)
            {
                strcpy(slices[slice_count - 1].label, "Other");
                slices[slice_count - 1].percentage += (categories[i].budget / total_budget) * 100.0;
            }
            else
            {
                slices[slice_count].percentage = (categories[i].budget / total_budget) * 100.0;
                slices[slice_count].color_pair = PIE_DARKER_COLOR_START + i;
                strncpy(slices[slice_count].label, categories[i].name, MAX_NAME_LEN - 1);
                slices[slice_count].label[MAX_NAME_LEN - 1] = '\0';
                slice_count++;
            }
        }
    }
    slices[slice_count].percentage = 100.0 - used_budget;
    slices[slice_count].color_pair = PIE_DARKER_COLOR_START + NUM_PIE_COLORS - 1;
    strncpy(slices[slice_count].label, "Savings", MAX_NAME_LEN - 1);
    slices[slice_count].label[MAX_NAME_LEN - 1] = '\0';
    slice_count++;

    draw_pie_chart(win, center_y, center_x, height, width, slices, slice_count);
    mvwprintw(win, 0, center_x - 8, "Budget Allocation");
}