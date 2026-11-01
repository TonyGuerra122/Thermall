#include "../include/terminal.h"

#include <errno.h>

typedef enum {
	LOAD_OK,
	LOAD_NOT_FOUND,
	LOAD_ERROR
} LoadResult;

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

static bool initEmptyBuffer(Terminal* terminal) {
	terminal->lines = (LineText**)malloc(sizeof(LineText*));
	if (!terminal->lines) return false;

	LineText* line = (LineText*)malloc(sizeof(LineText));
	if (!line) {
		free(terminal->lines);
		terminal->lines = NULL;
		return false;
	}

	create_line_text(line, "");
	terminal->lines[0] = line;
	terminal->lineCount = 1;
	terminal->isSaved = true;

	return true;
}

static void freeLines(Terminal* terminal) {
	if (terminal->lines) {
		for (size_t i = 0; i < terminal->lineCount; i++) {
			free(terminal->lines[i]);
		}
		free(terminal->lines);
	}
	terminal->lines = NULL;
	terminal->lineCount = 0;
}

static void removeLine(Terminal* terminal, size_t index) {
	free(terminal->lines[index]);
	memmove(&terminal->lines[index], &terminal->lines[index + 1],
		sizeof(LineText*) * (terminal->lineCount - index - 1));
	terminal->lineCount--;
}

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

		if (len >= MAX_LINE_LENGTH) {
			error = "File has a line that is too long";
			goto fail;
		}
		if (terminal->lineCount >= MAX_LINES) {
			error = "File has too many lines";
			goto fail;
		}

		if (terminal->lineCount == cap) {
			LineText** grown = (LineText**)realloc(terminal->lines, cap * 2 * sizeof(LineText*));
			if (!grown) goto fail;
			terminal->lines = grown;
			cap *= 2;
		}

		LineText* line = (LineText*)malloc(sizeof(LineText));
		if (!line) goto fail;

		create_line_text(line, "");
		memcpy(line->text, start, len);
		line->text[len] = '\0';
		line->length = len;
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

static void clampCursorX(const Terminal* terminal, int cursorY, int* cursorX) {
	if ((size_t)*cursorX > terminal->lines[cursorY]->length) {
		*cursorX = (int)terminal->lines[cursorY]->length;
	}
}

static void draw(const Terminal* terminal, size_t* viewTop, int cursorY, int cursorX) {
	size_t rows = LINES > 1 ? (size_t)LINES - 1 : 1;

	// Faz a rolagem acompanhar o cursor
	if ((size_t)cursorY < *viewTop) {
		*viewTop = (size_t)cursorY;
	}
	else if ((size_t)cursorY >= *viewTop + rows) {
		*viewTop = (size_t)cursorY - rows + 1;
	}

	erase();

	for (size_t i = 0; i < rows; i++) {
		size_t lineIndex = *viewTop + i;
		if (lineIndex >= terminal->lineCount) break;

		mvaddnstr((int)i, 0, terminal->lines[lineIndex]->text, COLS);
	}

	char status[256];
	if (statusMsg[0]) {
		snprintf(status, sizeof(status), "%s", statusMsg);
	}
	else {
		snprintf(status, sizeof(status), "File: (%s) Line: %d Column: %d Total Lines: %zu Mem: %.2fMB Saved: %s",
			terminal->filename ? terminal->filename : "[No Name]",
			cursorY + 1, cursorX + 1, terminal->lineCount,
			estimateMemoryCost(terminal),
			terminal->isSaved ? "(Saved)" : "(Unsaved)");
	}
	mvaddnstr(LINES - 1, 0, status, COLS);

	move((int)((size_t)cursorY - *viewTop), cursorX);
	refresh();
}

void handleTerminalInput(Terminal* terminal) {
	int ch, cursorX = 0, cursorY = 0;
	size_t viewTop = 0;
	bool mustExit = false;

	while (!mustExit) {
		draw(terminal, &viewTop, cursorY, cursorX);

		ch = getch();
		if (ch == ERR) break;

		statusMsg[0] = '\0';

		switch (ch) {
		case 27:
			if (!terminal->isSaved) {
				mvaddnstr(LINES - 1, 0, "Unsaved changes. Press 'q' to quit without saving or any other key to continue.", COLS);
				refresh();
				int quitCh = getch();
				if (quitCh == 'q' || quitCh == 'Q') {
					mustExit = true;
				}
				else if (quitCh != ERR && quitCh != 27) {
					// Devolve a tecla para ela não ser perdida
					ungetch(quitCh);
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
			if (cursorY > 0) cursorY--;
			clampCursorX(terminal, cursorY, &cursorX);
			break;
		case KEY_DOWN:
			if ((size_t)cursorY + 1 < terminal->lineCount) {
				cursorY++;
				clampCursorX(terminal, cursorY, &cursorX);
			}
			break;
		case KEY_LEFT:
			if (cursorX > 0) {
				cursorX--;
			}
			else if (cursorY > 0) {
				cursorY--;
				cursorX = (int)terminal->lines[cursorY]->length;
			}
			break;
		case KEY_RIGHT:
			if ((size_t)cursorX < terminal->lines[cursorY]->length) {
				cursorX++;
			}
			else if ((size_t)cursorY + 1 < terminal->lineCount) {
				cursorY++;
				cursorX = 0;
			}
			break;
		case '\n':
		case '\r':
		case KEY_ENTER: {
			if (terminal->lineCount >= MAX_LINES) {
				setMessage("Maximum number of lines reached");
				break;
			}

			LineText* current = terminal->lines[cursorY];

			LineText* newLine = (LineText*)malloc(sizeof(LineText));
			if (!newLine) {
				setMessage("Memory allocation failed");
				break;
			}

			LineText** newLines = (LineText**)realloc(terminal->lines, sizeof(LineText*) * (terminal->lineCount + 1));
			if (!newLines) {
				free(newLine);
				setMessage("Memory allocation failed");
				break;
			}
			terminal->lines = newLines;

			// Divide a linha atual na posição do cursor
			create_line_text(newLine, &current->text[cursorX]);
			current->text[cursorX] = '\0';
			current->length = (size_t)cursorX;

			memmove(&terminal->lines[cursorY + 2], &terminal->lines[cursorY + 1],
				sizeof(LineText*) * (terminal->lineCount - (size_t)cursorY - 1));
			terminal->lines[cursorY + 1] = newLine;
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
				delete_char(terminal->lines[cursorY], (size_t)cursorX - 1);
				cursorX--;
				terminal->isSaved = false;
			}
			else if (cursorY > 0) {
				LineText* currentLine = terminal->lines[cursorY];
				LineText* prevLine = terminal->lines[cursorY - 1];

				if (prevLine->length + currentLine->length >= MAX_LINE_LENGTH) {
					setMessage("Cannot join lines: result would be too long");
					break;
				}

				cursorX = (int)prevLine->length;
				memcpy(prevLine->text + prevLine->length, currentLine->text, currentLine->length + 1);
				prevLine->length += currentLine->length;

				removeLine(terminal, (size_t)cursorY);
				cursorY--;
				terminal->isSaved = false;
			}
			break;
		case KEY_DC:
			if ((size_t)cursorX < terminal->lines[cursorY]->length) {
				delete_char(terminal->lines[cursorY], (size_t)cursorX);
				terminal->isSaved = false;
			}
			else if ((size_t)cursorY + 1 < terminal->lineCount) {
				LineText* line = terminal->lines[cursorY];
				LineText* nextLine = terminal->lines[cursorY + 1];

				if (line->length + nextLine->length >= MAX_LINE_LENGTH) {
					setMessage("Cannot join lines: result would be too long");
					break;
				}

				memcpy(line->text + line->length, nextLine->text, nextLine->length + 1);
				line->length += nextLine->length;

				removeLine(terminal, (size_t)cursorY + 1);
				terminal->isSaved = false;
			}
			break;
		case KEY_PPAGE: {
			int pageSize = LINES > 2 ? LINES - 1 : 1;

			cursorY = cursorY > pageSize ? cursorY - pageSize : 0;
			clampCursorX(terminal, cursorY, &cursorX);
			break;
		}
		case KEY_NPAGE: {
			int pageSize = LINES > 2 ? LINES - 1 : 1;

			if ((size_t)cursorY + (size_t)pageSize < terminal->lineCount) {
				cursorY += pageSize;
			}
			else {
				cursorY = (int)terminal->lineCount - 1;
			}
			clampCursorX(terminal, cursorY, &cursorX);
			break;
		}
		default:
			if (ch >= 32 && ch <= 126 && cursorX < COLS - 1 &&
				terminal->lines[cursorY]->length + 1 < MAX_LINE_LENGTH) {
				insert_char(terminal->lines[cursorY], (size_t)cursorX, (char)ch);
				cursorX++;
				terminal->isSaved = false;
			}
			break;
		}
	}
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
			totalSize += sizeof(LineText);
	}

	return totalSize / (1024.0 * 1024.0);
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
		if (terminal->lines[i]) {
			ok = fprintf(file, "%s\n", terminal->lines[i]->text) >= 0;
		}
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
