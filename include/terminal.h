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

// Função para criar um terminal
Terminal* createTerminal(const char* filename);

// Função para lidar com a entrada do terminal
void handleTerminalInput(Terminal* terminal);

// Função para liberar a memória do terminal
void freeTerminal(Terminal* terminal);

// Função para estimar o uso de memória do terminal
double showHeader(const Terminal* terminal);

// Função para salvar o conteúdo do terminal em um arquivo
void saveFile(Terminal* terminal);