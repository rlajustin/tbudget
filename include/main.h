#ifndef MAIN_H
#define MAIN_H

#include "globals.h"
#include "actions.h"
#include "ui.h"
#include "utils.h"
#include "flex_layout.h"
#include "subscriptions.h"
#include "file_cache.h"
#include <locale.h>

void print_usage(const char *program_name);
void run_menu_mode();
int run_dashboard_mode();
int save_and_exit();

#endif