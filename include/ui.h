#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "globals.h"
#include "utils.h"
#include "piechart.h"
#include "ui_helper.h"
#include "subscriptions.h"
int get_transaction_choice(WINDOW *win, const Transaction transactions[], int transaction_count, int max_visible_items);

// dashboard display
void display_categories(WINDOW *win, int start_y);
void display_transactions(WINDOW *win, int selected_transaction, int* first_display_transaction, int start_y, bool highlight_selected);
void display_subscriptions(WINDOW *win, int start_y);
BoundedWindow draw_bar_chart(WINDOW *parent);
BoundedWindow* create_bar_chart(BoundedWindow *parent);

#endif // UI_H

