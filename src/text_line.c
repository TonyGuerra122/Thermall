#include "../include/line_text.h"

#define MIN_LINE_CAPACITY 32

// Garante espaço para pelo menos 'needed' bytes (incluindo o '\0')
static bool reserve(LineText* line, size_t needed) {
	if (needed <= line->capacity) return true;

	size_t newCapacity = line->capacity ? line->capacity : MIN_LINE_CAPACITY;
	while (newCapacity < needed) {
		if (newCapacity > SIZE_MAX / 2) return false;
		newCapacity *= 2;
	}

	char* grown = (char*)realloc(line->text, newCapacity);
	if (!grown) return false;

	line->text = grown;
	line->capacity = newCapacity;
	return true;
}

bool create_line_text(LineText* line, const char* text, size_t length) {
	line->text = NULL;
	line->length = 0;
	line->capacity = 0;

	if (!reserve(line, length + 1)) return false;

	if (length > 0) memcpy(line->text, text, length);
	line->text[length] = '\0';
	line->length = length;
	return true;
}

void destroy_line_text(LineText* line) {
	free(line->text);
	line->text = NULL;
	line->length = 0;
	line->capacity = 0;
}

bool insert_text(LineText* line, size_t pos, const char* text, size_t length) {
	if (pos > line->length) pos = line->length;
	if (length == 0) return true;
	if (!reserve(line, line->length + length + 1)) return false;

	memmove(line->text + pos + length, line->text + pos, line->length - pos);
	memcpy(line->text + pos, text, length);
	line->length += length;
	line->text[line->length] = '\0';
	return true;
}

void delete_range(LineText* line, size_t pos, size_t length) {
	if (pos >= line->length) return;
	if (length > line->length - pos) length = line->length - pos;

	memmove(line->text + pos, line->text + pos + length, line->length - pos - length);
	line->length -= length;
	line->text[line->length] = '\0';
}

void truncate_line(LineText* line, size_t length) {
	if (length >= line->length) return;
	line->length = length;
	line->text[length] = '\0';
}

size_t utf8_seq_len(const char* s, size_t avail) {
	if (avail == 0) return 0;

	unsigned char c = (unsigned char)s[0];
	size_t n;

	if (c < 0x80) return 1;
	else if (c >= 0xC2 && c <= 0xDF) n = 2;
	else if ((c & 0xF0) == 0xE0) n = 3;
	else if (c >= 0xF0 && c <= 0xF4) n = 4;
	else return 0;

	if (avail < n) return 0;
	for (size_t i = 1; i < n; i++) {
		if (((unsigned char)s[i] & 0xC0) != 0x80) return 0;
	}
	return n;
}

size_t utf8_prev(const char* s, size_t pos) {
	if (pos == 0) return 0;

	pos--;
	for (int steps = 0; pos > 0 && steps < 3 && ((unsigned char)s[pos] & 0xC0) == 0x80; steps++) {
		pos--;
	}
	return pos;
}

size_t utf8_next(const char* s, size_t length, size_t pos) {
	if (pos >= length) return length;

	size_t n = utf8_seq_len(s + pos, length - pos);
	return pos + (n ? n : 1);
}

size_t utf8_encode(uint32_t cp, char out[4]) {
	if (cp < 0x80) {
		out[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	out[0] = (char)(0xF0 | (cp >> 18));
	out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
	out[3] = (char)(0x80 | (cp & 0x3F));
	return 4;
}

size_t line_cell_width(const LineText* line, size_t pos, size_t col) {
	if (line->text[pos] == '\t') return TAB_WIDTH - (col % TAB_WIDTH);
	return 1;
}

size_t line_display_col(const LineText* line, size_t pos) {
	size_t col = 0, i = 0;

	while (i < pos && i < line->length) {
		col += line_cell_width(line, i, col);
		i = utf8_next(line->text, line->length, i);
	}
	return col;
}

size_t line_col_to_byte(const LineText* line, size_t col) {
	size_t c = 0, i = 0;

	while (i < line->length && c < col) {
		size_t w = line_cell_width(line, i, c);
		if (c + w > col) break; // 'col' cai no meio de uma tabulação
		c += w;
		i = utf8_next(line->text, line->length, i);
	}
	return i;
}
