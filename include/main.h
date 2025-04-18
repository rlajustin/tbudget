#ifndef MAIN_H
#define MAIN_H

#include "globals.h"
#include "actions.h"
#include "ui.h"
#include "utils.h"
#include "flex_layout.h"
#include "subscriptions.h"

#include <locale.h>

void print_usage(const char *program_name);
void run_menu_mode();
void run_dashboard_mode();

#endif