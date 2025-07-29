#pragma once

#include <curses.h>
#include <stdbool.h>
#include "line_text.h"

#define MAX_LINES 1000

typedef struct {
	char* filename;
	LineText** lines;
	size_t lineCount;
	bool isSaved;
}Terminal;

// Fun��o para criar um terminal
Terminal* createTerminal(const char* filename);

// Fun��o para lidar com a entrada do terminal
void handleTerminalInput(Terminal* terminal);

// Fun��o para liberar a mem�ria do terminal
void freeTerminal(Terminal* terminal);

// Fun��o para estimar o uso de mem�ria do terminal
double showHeader(const Terminal* terminal);

// Fun��o para salvar o conte�do do terminal em um arquivo
void saveFile(Terminal* terminal);