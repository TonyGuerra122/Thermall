#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 8191

typedef struct {
	char text[MAX_LINE_LENGTH];
	size_t length;
	size_t capacity;
}LineText;

// Fun��o para criar um LineText a partir de uma string
void create_line_text(LineText* line, const char* text);

// Fun��o para adicionar um caractere ao final de um LineText
void insert_char(LineText* line, size_t pos, char ch);

// Fun��o para excluir um caractere de um LineText
void delete_char(LineText* line, size_t pos);