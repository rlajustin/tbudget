#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "flex_layout.h"
#include "globals.h"
#include "utils.h"
#include "piechart.h"
#include "ui_helper.h"

int get_transaction_choice(WINDOW *win, TransactionNode **sorted_transactions, int transaction_count, int max_visible_items);
int get_category_choice_subscription(WINDOW *win, int year, int month, char *subscription_name, char *subscription_category);
int get_category_choice_recategorize(WINDOW *win, char *category_name);

// dashboard display
void display_categories(WINDOW *win, int start_y);
void display_transactions(WINDOW *win, int start_y, int selected_transaction, int *first_display_transaction, bool highlight_selected);
void display_subscriptions(WINDOW *win, int start_y, int selected_subscription, int *first_display_subscription, bool active);
BoundedWindow draw_bar_chart(WINDOW *parent);
BoundedWindow *create_bar_chart(BoundedWindow *parent);

// formatting
int get_date_input(WINDOW *win, char *date_buffer, char *prompt);
void format_date(WINDOW *win, int y, int x, int day, int month, int year, int highlighted_field, int cursor_positions[]);

#endif // UI_H
