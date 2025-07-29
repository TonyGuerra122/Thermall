#include "../include/line_text.h"

void create_line_text(LineText* line, const char* text) {
	size_t len = strlen(text);
	if (len >= MAX_LINE_LENGTH) len = MAX_LINE_LENGTH - 1;

	memcpy(line->text, text, len);
	line->text[len] = '\0';
	line->length = len;
	line->capacity = MAX_LINE_LENGTH;
}

void insert_char(LineText* line, size_t pos, char ch) {
	if (line->length + 1 >= line->capacity) return;
	if (pos > line->length) pos = line->length;

	memmove(&line->text[pos + 1], &line->text[pos], line->length - pos);
	line->text[pos] = ch;
	line->length++;
	line->text[line->length] = '\0';
}

void delete_char(LineText* line, size_t pos) {
	if (pos >= line->length) return;
	memmove(&line->text[pos], &line->text[pos + 1], line->length - pos - 1);
	line->length--;
	line->text[line->length] = '\0';
}