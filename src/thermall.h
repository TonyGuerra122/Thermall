#pragma once

#include "text_buffer.h"

#include <curses.h>

void init_thermall();

void run_thermall(const char *);

void handle_input(TextBuffer *);

void exit_thermall();