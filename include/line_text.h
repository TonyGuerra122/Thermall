#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Largura de uma tabulação na tela (em colunas)
#define TAB_WIDTH 4

// Uma linha de texto em UTF-8. 'length' e 'capacity' são em bytes.
// 'text' sempre termina com '\0' e capacity >= length + 1.
typedef struct {
	char* text;
	size_t length;
	size_t capacity;
} LineText;

// Inicializa uma linha com uma cópia de 'length' bytes de 'text'
bool create_line_text(LineText* line, const char* text, size_t length);

// Libera o buffer interno da linha (não libera a struct em si)
void destroy_line_text(LineText* line);

// Insere 'length' bytes de 'text' na posição (em bytes) 'pos'
bool insert_text(LineText* line, size_t pos, const char* text, size_t length);

// Remove 'length' bytes a partir da posição 'pos'
void delete_range(LineText* line, size_t pos, size_t length);

// Corta a linha em 'length' bytes
void truncate_line(LineText* line, size_t length);

// --- UTF-8 ---

// Tamanho (1 a 4) da sequência UTF-8 válida no início de 's', ou 0 se for inválida
size_t utf8_seq_len(const char* s, size_t avail);

// Posição do início do caractere anterior a 'pos'
size_t utf8_prev(const char* s, size_t pos);

// Posição do início do caractere seguinte ao de 'pos'
size_t utf8_next(const char* s, size_t length, size_t pos);

// Codifica um ponto de código em UTF-8; retorna o número de bytes escritos
size_t utf8_encode(uint32_t codepoint, char out[4]);

// --- Colunas da tela ---

// Largura (em colunas) do caractere em 'pos' quando ele começa na coluna 'col'
size_t line_cell_width(const LineText* line, size_t pos, size_t col);

// Coluna da tela correspondente à posição 'pos' (em bytes)
size_t line_display_col(const LineText* line, size_t pos);

// Posição (em bytes) mais próxima da coluna 'col', sem passar dela
size_t line_col_to_byte(const LineText* line, size_t col);
