#include "../include/terminal.h"

#include <errno.h>

typedef enum {
	LOAD_OK,
	LOAD_NOT_FOUND,
	LOAD_ERROR
} LoadResult;

// Uma tecla lida: 'special' indica tecla de função (KEY_*); senão 'code' é um ponto de código Unicode
typedef struct {
	bool special;
	int code;
} Key;

// Mensagem mostrada na linha de status (substitui as saídas em stderr, que bagunçam o curses)
static char statusMsg[160];

static void setMessage(const char* msg) {
	snprintf(statusMsg, sizeof(statusMsg), "%s", msg);
}

static char* duplicateString(const char* s) {
	size_t len = strlen(s) + 1;
	char* copy = (char*)malloc(len);
	if (copy) memcpy(copy, s, len);
	return copy;
}

// --- Gerenciamento das linhas ---

static LineText* newLine(const char* text, size_t length) {
	LineText* line = (LineText*)malloc(sizeof(LineText));
	if (!line) return NULL;

	if (!create_line_text(line, text, length)) {
		free(line);
		return NULL;
	}
	return line;
}

static void destroyLine(LineText* line) {
	if (!line) return;
	destroy_line_text(line);
	free(line);
}

static bool initEmptyBuffer(Terminal* terminal) {
	terminal->lines = (LineText**)malloc(sizeof(LineText*));
	if (!terminal->lines) return false;

	terminal->lines[0] = newLine("", 0);
	if (!terminal->lines[0]) {
		free(terminal->lines);
		terminal->lines = NULL;
		return false;
	}

	terminal->lineCount = 1;
	terminal->isSaved = true;
	return true;
}

static void freeLines(Terminal* terminal) {
	if (terminal->lines) {
		for (size_t i = 0; i < terminal->lineCount; i++) {
			destroyLine(terminal->lines[i]);
		}
		free(terminal->lines);
	}
	terminal->lines = NULL;
	terminal->lineCount = 0;
}

static void removeLine(Terminal* terminal, size_t index) {
	destroyLine(terminal->lines[index]);
	memmove(&terminal->lines[index], &terminal->lines[index + 1],
		sizeof(LineText*) * (terminal->lineCount - index - 1));
	terminal->lineCount--;
}

// --- Arquivo ---

static LoadResult loadFileLines(Terminal* terminal, const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) {
		return errno == ENOENT ? LOAD_NOT_FOUND : LOAD_ERROR;
	}

	const char* error = "Could not read file";
	char* buf = NULL;
	size_t cap = 16;

	if (fseek(f, 0, SEEK_END) != 0) goto fail;
	long size = ftell(f);
	if (size < 0) goto fail;
	rewind(f);

	buf = (char*)malloc((size_t)size + 1);
	if (!buf) goto fail;

	size_t readn = fread(buf, 1, (size_t)size, f);
	fclose(f);
	f = NULL;
	buf[readn] = '\0';

	terminal->lines = (LineText**)malloc(cap * sizeof(LineText*));
	if (!terminal->lines) goto fail;
	terminal->lineCount = 0;

	char* start = buf;
	for (size_t i = 0; i <= readn; i++) {
		if (i != readn && buf[i] != '\n') continue;

		size_t len = (size_t)(buf + i - start);
		while (len > 0 && start[len - 1] == '\r') len--;

		// Arquivo terminado em '\n' não gera uma linha vazia extra
		if (i == readn && len == 0 && readn > 0 && buf[readn - 1] == '\n') break;

		if (terminal->lineCount == cap) {
			LineText** grown = (LineText**)realloc(terminal->lines, cap * 2 * sizeof(LineText*));
			if (!grown) goto fail;
			terminal->lines = grown;
			cap *= 2;
		}

		LineText* line = newLine(start, len);
		if (!line) goto fail;
		terminal->lines[terminal->lineCount++] = line;

		start = buf + i + 1;
	}

	free(buf);

	if (terminal->lineCount == 0) {
		// Arquivo vazio: começa com uma linha em branco
		freeLines(terminal);
		if (!initEmptyBuffer(terminal)) return LOAD_ERROR;
	}

	terminal->isSaved = true;
	return LOAD_OK;

fail:
	if (f) fclose(f);
	free(buf);
	freeLines(terminal);
	fprintf(stderr, "%s: %s\n", path, error);
	return LOAD_ERROR;
}

void saveFile(Terminal* terminal) {
	if (!terminal) return;
	if (!terminal->filename) {
		setMessage("No file name. Start the program with a file name to be able to save.");
		return;
	}

	FILE* file = fopen(terminal->filename, "w");
	if (!file) {
		setMessage("Failed to open file for writing");
		return;
	}

	bool ok = true;
	for (size_t i = 0; i < terminal->lineCount && ok; i++) {
		const LineText* line = terminal->lines[i];
		ok = fwrite(line->text, 1, line->length, file) == line->length && fputc('\n', file) != EOF;
	}

	if (fclose(file) != 0) ok = false;

	if (ok) {
		terminal->isSaved = true;
		setMessage("File saved");
	}
	else {
		setMessage("Failed to write file");
	}
}

// --- Ciclo de vida ---

Terminal* createTerminal(const char* filename) {
	Terminal* terminal = (Terminal*)calloc(1, sizeof(Terminal));
	if (!terminal) return NULL;

	terminal->isSaved = true;
	bool ok = false;

	if (filename) {
		terminal->filename = duplicateString(filename);
		if (!terminal->filename) {
			free(terminal);
			return NULL;
		}

		LoadResult result = loadFileLines(terminal, terminal->filename);
		if (result == LOAD_ERROR) {
			free(terminal->filename);
			free(terminal);
			return NULL;
		}
		ok = (result == LOAD_OK);
	}

	// Arquivo inexistente ou sem nome: começa com um buffer vazio
	if (!ok && !initEmptyBuffer(terminal)) {
		free(terminal->filename);
		free(terminal);
		return NULL;
	}

	// Inicializa curses
	initscr();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(1);

	return terminal;
}

void freeTerminal(Terminal* terminal) {
	if (terminal) {
		free(terminal->filename);
		freeLines(terminal);
		free(terminal);
		endwin();
	}
}

double estimateMemoryCost(const Terminal* terminal) {
	if (!terminal) return 0.0;

	double totalSize = sizeof(Terminal);
	if (terminal->filename)
		totalSize += strlen(terminal->filename) + 1;

	totalSize += terminal->lineCount * sizeof(LineText*);

	for (size_t i = 0; i < terminal->lineCount; i++) {
		if (terminal->lines[i])
			totalSize += sizeof(LineText) + terminal->lines[i]->capacity;
	}

	return totalSize / (1024.0 * 1024.0);
}

// --- Desenho ---

// Corta 's' (UTF-8) para caber em 'cols' colunas, sem partir um caractere no meio
static void fitToWidth(char* s, int cols) {
	size_t len = strlen(s);
	size_t pos = 0;

	for (int col = 0; pos < len && col < cols; col++) {
		pos = utf8_next(s, len, pos);
	}
	s[pos] = '\0';
}

static void showStatusLine(const char* text) {
	char buf[512];
	snprintf(buf, sizeof(buf), "%s", text);
	fitToWidth(buf, COLS);

	move(LINES - 1, 0);
	clrtoeol();
	addstr(buf);
}

// Monta em 'out' a parte da linha visível entre as colunas [viewLeft, viewLeft + width).
// 'out' precisa ter pelo menos width * 4 + 1 bytes.
static void renderRow(const LineText* line, size_t viewLeft, size_t width, char* out) {
	size_t col = 0, pos = 0;
	size_t viewRight = viewLeft + width;

	while (pos < line->length && col < viewRight) {
		unsigned char c = (unsigned char)line->text[pos];
		size_t w = line_cell_width(line, pos, col);
		size_t seq = utf8_seq_len(line->text + pos, line->length - pos);

		if (c == '\t') {
			for (size_t k = 0; k < w; k++) {
				if (col + k >= viewLeft && col + k < viewRight) *out++ = ' ';
			}
		}
		else if (col >= viewLeft) {
			if (seq == 0 || c < 32 || c == 127) {
				*out++ = '?'; // byte inválido ou caractere de controle
			}
			else {
				memcpy(out, line->text + pos, seq);
				out += seq;
			}
		}

		col += w;
		pos += seq ? seq : 1;
	}

	*out = '\0';
}

static void draw(const Terminal* terminal, size_t* viewTop, size_t* viewLeft, size_t cursorY, size_t cursorX) {
	size_t rows = LINES > 1 ? (size_t)LINES - 1 : 1;
	size_t width = COLS > 0 ? (size_t)COLS : 1;
	size_t cursorCol = line_display_col(terminal->lines[cursorY], cursorX);

	// Faz a rolagem vertical e horizontal acompanhar o cursor
	if (cursorY < *viewTop) *viewTop = cursorY;
	else if (cursorY >= *viewTop + rows) *viewTop = cursorY - rows + 1;

	if (cursorCol < *viewLeft) *viewLeft = cursorCol;
	else if (cursorCol >= *viewLeft + width) *viewLeft = cursorCol - width + 1;

	erase();

	char* rowBuf = (char*)malloc(width * 4 + 1);
	if (rowBuf) {
		for (size_t i = 0; i < rows; i++) {
			size_t lineIndex = *viewTop + i;
			if (lineIndex >= terminal->lineCount) break;

			renderRow(terminal->lines[lineIndex], *viewLeft, width, rowBuf);
			mvaddstr((int)i, 0, rowBuf);
		}
		free(rowBuf);
	}

	char status[512];
	if (statusMsg[0]) {
		snprintf(status, sizeof(status), "%s", statusMsg);
	}
	else {
		snprintf(status, sizeof(status), "File: (%s) Line: %zu Column: %zu Total Lines: %zu Mem: %.2fMB Saved: %s",
			terminal->filename ? terminal->filename : "[No Name]",
			cursorY + 1, cursorCol + 1, terminal->lineCount,
			estimateMemoryCost(terminal),
			terminal->isSaved ? "(Saved)" : "(Unsaved)");
	}
	showStatusLine(status);

	move((int)(cursorY - *viewTop), (int)(cursorCol - *viewLeft));
	refresh();
}

// --- Entrada ---

static bool readKey(Key* key) {
	wint_t wch = 0;
	int result = get_wch(&wch);
	if (result == ERR) return false;

	key->special = (result == KEY_CODE_YES);
	key->code = (int)wch;
	return true;
}

// Caractere imprimível (ASCII ou Unicode), excluindo controles e surrogates
static bool isPrintable(const Key* key) {
	if (key->special) return false;
	int c = key->code;
	return (c >= 32 && c < 127) || (c >= 0xA0 && c < 0xD800) || (c > 0xDFFF && c <= 0x10FFFF);
}

// Junta a linha 'index + 1' no fim da linha 'index'. Devolve false (com mensagem) se faltar memória.
static bool joinWithNext(Terminal* terminal, size_t index) {
	LineText* line = terminal->lines[index];
	LineText* next = terminal->lines[index + 1];

	if (!insert_text(line, line->length, next->text, next->length)) {
		setMessage("Memory allocation failed");
		return false;
	}

	removeLine(terminal, index + 1);
	return true;
}

void handleTerminalInput(Terminal* terminal) {
	size_t cursorX = 0, cursorY = 0; // cursorX é a posição em bytes dentro da linha
	size_t goalCol = 0;              // coluna desejada ao mover verticalmente
	size_t viewTop = 0, viewLeft = 0;
	bool mustExit = false;

	Key pending = { false, 0 };
	bool hasPending = false;

	while (!mustExit) {
		draw(terminal, &viewTop, &viewLeft, cursorY, cursorX);

		Key key;
		if (hasPending) {
			key = pending;
			hasPending = false;
		}
		else if (!readKey(&key)) {
			break;
		}

		statusMsg[0] = '\0';
		bool vertical = false;
		size_t pageSize = LINES > 2 ? (size_t)LINES - 1 : 1;

		if (isPrintable(&key) || (!key.special && key.code == '\t')) {
			char encoded[4];
			size_t n = key.code == '\t' ? (encoded[0] = '\t', 1) : utf8_encode((uint32_t)key.code, encoded);

			if (insert_text(terminal->lines[cursorY], cursorX, encoded, n)) {
				cursorX += n;
				terminal->isSaved = false;
			}
			else {
				setMessage("Memory allocation failed");
			}
		}
		else if (key.special || key.code < 0x100) {
			switch (key.code) {
			case 27: // Esc
				if (!terminal->isSaved) {
					showStatusLine("Unsaved changes. Press 'q' to quit without saving or any other key to continue.");
					refresh();

					Key answer;
					if (readKey(&answer)) {
						if (!answer.special && (answer.code == 'q' || answer.code == 'Q')) {
							mustExit = true;
						}
						else if (answer.special || answer.code != 27) {
							// Guarda a tecla para ela não ser perdida
							pending = answer;
							hasPending = true;
						}
					}
				}
				else {
					mustExit = true;
				}
				break;
			case KEY_RESIZE:
				resize_term(0, 0);
				break;
			case 19: // Ctrl+S
				saveFile(terminal);
				break;
			case KEY_UP:
				vertical = true;
				if (cursorY > 0) cursorY--;
				cursorX = line_col_to_byte(terminal->lines[cursorY], goalCol);
				break;
			case KEY_DOWN:
				vertical = true;
				if (cursorY + 1 < terminal->lineCount) cursorY++;
				cursorX = line_col_to_byte(terminal->lines[cursorY], goalCol);
				break;
			case KEY_PPAGE:
				vertical = true;
				cursorY = cursorY > pageSize ? cursorY - pageSize : 0;
				cursorX = line_col_to_byte(terminal->lines[cursorY], goalCol);
				break;
			case KEY_NPAGE:
				vertical = true;
				cursorY = cursorY + pageSize < terminal->lineCount ? cursorY + pageSize : terminal->lineCount - 1;
				cursorX = line_col_to_byte(terminal->lines[cursorY], goalCol);
				break;
			case KEY_LEFT:
				if (cursorX > 0) {
					cursorX = utf8_prev(terminal->lines[cursorY]->text, cursorX);
				}
				else if (cursorY > 0) {
					cursorY--;
					cursorX = terminal->lines[cursorY]->length;
				}
				break;
			case KEY_RIGHT: {
				LineText* line = terminal->lines[cursorY];
				if (cursorX < line->length) {
					cursorX = utf8_next(line->text, line->length, cursorX);
				}
				else if (cursorY + 1 < terminal->lineCount) {
					cursorY++;
					cursorX = 0;
				}
				break;
			}
			case KEY_HOME:
				cursorX = 0;
				break;
			case KEY_END:
				cursorX = terminal->lines[cursorY]->length;
				break;
			case '\n':
			case '\r':
			case KEY_ENTER: {
				LineText* current = terminal->lines[cursorY];

				LineText* tail = newLine(current->text + cursorX, current->length - cursorX);
				if (!tail) {
					setMessage("Memory allocation failed");
					break;
				}

				LineText** newLines = (LineText**)realloc(terminal->lines, sizeof(LineText*) * (terminal->lineCount + 1));
				if (!newLines) {
					destroyLine(tail);
					setMessage("Memory allocation failed");
					break;
				}
				terminal->lines = newLines;

				truncate_line(current, cursorX);

				memmove(&terminal->lines[cursorY + 2], &terminal->lines[cursorY + 1],
					sizeof(LineText*) * (terminal->lineCount - cursorY - 1));
				terminal->lines[cursorY + 1] = tail;
				terminal->lineCount++;

				cursorY++;
				cursorX = 0;
				terminal->isSaved = false;
				break;
			}
			case KEY_BACKSPACE:
			case 127:
			case 8:
				if (cursorX > 0) {
					LineText* line = terminal->lines[cursorY];
					size_t prev = utf8_prev(line->text, cursorX);
					delete_range(line, prev, cursorX - prev);
					cursorX = prev;
					terminal->isSaved = false;
				}
				else if (cursorY > 0) {
					size_t joinPos = terminal->lines[cursorY - 1]->length;
					if (joinWithNext(terminal, cursorY - 1)) {
						cursorY--;
						cursorX = joinPos;
						terminal->isSaved = false;
					}
				}
				break;
			case KEY_DC: {
				LineText* line = terminal->lines[cursorY];
				if (cursorX < line->length) {
					size_t next = utf8_next(line->text, line->length, cursorX);
					delete_range(line, cursorX, next - cursorX);
					terminal->isSaved = false;
				}
				else if (cursorY + 1 < terminal->lineCount) {
					if (joinWithNext(terminal, cursorY)) {
						terminal->isSaved = false;
					}
				}
				break;
			}
			default:
				break;
			}
		}

		if (!vertical) {
			goalCol = line_display_col(terminal->lines[cursorY], cursorX);
		}
	}
}
