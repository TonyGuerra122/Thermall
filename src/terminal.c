#include "../include/terminal.h"

Terminal* createTerminal(const char* filename) {
	// Alocando Terminal (antes de inicializar o ncurses)
	Terminal* terminal = (Terminal*)calloc(1, sizeof(Terminal));
	if (!terminal) return NULL;

	terminal->filename = filename ? strdup(filename) : NULL;
	terminal->lines = NULL;
	terminal->lineCount = 0;
	terminal->isSaved = true;
	bool ok = false;
	if (terminal->filename) {
		ok = loadFileLines(terminal, terminal->filename);
	}

	if (!ok) {
		if (!initEmptyBuffer(terminal)) {
			if (terminal->filename) free(terminal->filename);
			free(terminal);
			return NULL;
		}
	}

	// Inicializa curses
	initscr();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(1);

	return terminal;
}

void handleTerminalInput(Terminal* terminal) {
	int ch, cursorX = 0, cursorY = 0;
	size_t viewTop = 0;

	bool mustExit = false;

	while ((ch = getch()) && !mustExit) {
		clear();

		switch (ch) {
		case 27:
			if (!terminal->isSaved) {
				mvprintw(LINES - 1, 0, "Unsaved changes. Press 'q' to quit without saving or any other key to continue.");
				refresh();
				int quitCh = getch();
				if (quitCh == 'q' || quitCh == 'Q') {
					mustExit = true;
					return;
				}
			}
			else {
				mustExit = true;
			}

			break;
		case KEY_RESIZE:
			clear();
			mvprintw(0, 0, "Terminal resized. Press any key to continue.");
			break;
		case 19:
			saveFile(terminal);
			break;
		case KEY_UP:
			if (cursorY > 0) cursorY--;
			if ((size_t)cursorX > terminal->lines[cursorY]->length)
				cursorX = terminal->lines[cursorY]->length;
			break;
		case KEY_DOWN:
			if ((size_t)cursorY + 1 < terminal->lineCount) {
				cursorY++;
				if ((size_t)cursorX > terminal->lines[cursorY]->length)
					cursorX = terminal->lines[cursorY]->length;
			}
			break;
		case KEY_LEFT:
			if (cursorX > 0) {
				cursorX--;
			}
			else if (cursorY > 0) {
				cursorY--;
				cursorX = terminal->lines[cursorY]->length;
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
		case KEY_ENTER:
			if (terminal->lineCount >= MAX_LINES) break;

			LineText* current = NULL;
			if (cursorY >= 0 && (size_t)cursorY < terminal->lineCount) {
				current = terminal->lines[cursorY];
			}
			else {
				fprintf(stderr, "Cursor Y out of bounds: %d\n", cursorY);
				break;
			}

			LineText* newLine = malloc(sizeof(LineText));

			if (!newLine) {
				fprintf(stderr, "Memory allocation failed\n");
				break;
			}

			create_line_text(newLine, &current->text[cursorX]);

			current->text[cursorX] = '\0';
			current->length = strlen(current->text);

			LineText** newLines = realloc(terminal->lines, sizeof(LineText*) * (terminal->lineCount + 1));
			if (!newLines) {
				fprintf(stderr, "Memory allocation failed\n");
				free(newLine);
				break;
			}
			terminal->lines = newLines;
			memmove(&terminal->lines[cursorY + 2], &terminal->lines[cursorY + 1], sizeof(LineText*) * (terminal->lineCount - cursorY - 1));
			terminal->lines[cursorY + 1] = newLine;
			terminal->lineCount++;

			cursorY++;
			cursorX = 0;
			break;
		case KEY_BACKSPACE:
		case 127:
		case 8:
			if (cursorX > 0) {
				delete_char(terminal->lines[cursorY], (size_t)cursorX - 1);
				cursorX--;
			}
			else if (cursorY > 0) {
				LineText* currentLine = terminal->lines[cursorY];
				LineText* prevLine = terminal->lines[cursorY - 1];

				size_t prevLen = prevLine->length;
				strcat(prevLine->text, currentLine->text);
				prevLine->length = strlen(prevLine->text);
				cursorX = prevLine->length;

				free(currentLine);

				if ((size_t)cursorY + 1 < terminal->lineCount) {
					memmove(&terminal->lines[cursorY], &terminal->lines[cursorY + 1], sizeof(LineText*) * (terminal->lineCount - cursorY - 1));
				}

				terminal->lineCount--;
				cursorY--;
			}
			terminal->isSaved = false;
			break;
		case KEY_DC:
			if ((size_t)cursorX < terminal->lines[cursorY]->length) {
				delete_char(terminal->lines[cursorY], (size_t)cursorX);
			}
			else if ((size_t)cursorY + 1 < terminal->lineCount) {
				LineText* nextLine = terminal->lines[cursorY + 1];
				strcat(terminal->lines[cursorY]->text, nextLine->text);
				terminal->lines[cursorY]->length = strlen(terminal->lines[cursorY]->text);
				free(nextLine);
				memmove(&terminal->lines[cursorY + 1], &terminal->lines[cursorY + 2], sizeof(LineText*) * (terminal->lineCount - cursorY - 2));
				terminal->lineCount--;
			}
			terminal->isSaved = false;
			break;
		case KEY_PPAGE: {
			int pageSize = LINES - 1;

			if (cursorY > pageSize) {
				cursorY -= pageSize;
			}
			else {
				cursorY = 0;
			}

			if (viewTop >= pageSize) {
				viewTop -= pageSize;
			}
			else {
				viewTop = 0;
			}

			if ((size_t)cursorX > terminal->lines[cursorY]->length) {
				cursorX = terminal->lines[cursorY]->length;
			}

			break;
		}
		case KEY_NPAGE: {
			int pageSize = LINES - 1;

			if ((size_t)cursorY + pageSize < terminal->lineCount) {
				cursorY += pageSize;
			}
			else {
				cursorY = terminal->lineCount - 1;
			}

			size_t maxViewTop = terminal->lineCount > (size_t)pageSize ?
				terminal->lineCount - pageSize :
				0;

			if (viewTop + pageSize <= maxViewTop) {
				viewTop += pageSize;
			}
			else {
				viewTop = maxViewTop;
			}

			if ((size_t)cursorX > terminal->lines[cursorY]->length) {
				cursorX = terminal->lines[cursorY]->length;
			}

			break;
		}
		default:
			if (ch >= 32 && ch <= 126 && cursorX < COLS - 1) {
				insert_char(terminal->lines[cursorY], cursorX, (char)ch);
				cursorX++;
				terminal->isSaved = false;
			}

			break;
		}

		for (size_t i = 0; i < (size_t)LINES - 1; i++) {
			size_t lineIndex = viewTop + i;
			if (lineIndex >= terminal->lineCount) break;

			mvprintw(i, 0, "%s", terminal->lines[lineIndex]->text);
		}

		char status[256];
		const char* saved = terminal->isSaved ? "(Saved)" : "(Unsaved)";
		double memUsage = estimateMemoryCost(terminal);
		snprintf(status, sizeof(status), "File: (%s) Line: %d Column: %d Total Lines: %zu Mem: %.2fMB Saved: %s",
			terminal->filename, cursorY + 1, cursorX + 1, terminal->lineCount, memUsage, saved);

		mvprintw(LINES - 1, 0, "%s", status);

		move(cursorY - viewTop, cursorX);
		refresh();
	}
}

void freeTerminal(Terminal* terminal) {
	if (terminal) {
		if (terminal->filename) free(terminal->filename);
		if (terminal->lines) {
			for (size_t i = 0; i < terminal->lineCount; i++) {
				free(terminal->lines[i]);
			}
			free(terminal->lines);
		}
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
	if (!terminal || !terminal->filename) return;
	FILE* file = fopen(terminal->filename, "w");
	if (!file) {
		perror("Failed to open file for writing");
		return;
	}
	for (size_t i = 0; i < terminal->lineCount; i++) {
		if (terminal->lines[i]) {
			fprintf(file, "%s\n", terminal->lines[i]->text);
		}
	}
	fclose(file);
	terminal->isSaved = true;
}

bool initEmptyBuffer(Terminal* terminal) {
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

bool loadFileLines(Terminal* terminal, const char* path) {
	FILE* f = fopen(path, "rb");
	if (!f) {
		return false;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return false;
	}

	long size = ftell(f);
	if (size < 0) {
		fclose(f);
		return false;
	}

	rewind(f);

	char* buf = (char*)malloc((size_t)size + 1);
	if (!buf) {
		fclose(f);
		return false;
	}

	size_t readn = fread(buf, 1, (size_t)size, f);
	fclose(f);
	buf[readn] = '\0';

	size_t cap = 16;
	terminal->lines = (LineText**)malloc(cap * sizeof(LineText*));
	if (!terminal->lines) {
		free(buf);
		return false;
	}

	terminal->lineCount = 0;

	char* start = buf;
	for (size_t i = 0; i <= readn; i++) {
		bool isEol = (i == readn) || (buf[i] == '\n');
		if (isEol) {
			size_t len = (size_t)(buf + i - start);

			while (len > 0 && start[len - 1] == '\r') len--;

			char* tmp = (char*)malloc(len + 1);
			if (!tmp) {
				free(buf);
				free(terminal->lines);
				terminal->lines = NULL;
				return false;
			}
			memcpy(tmp, start, len);
			tmp[len] = '\0';

			LineText* line = malloc(sizeof(LineText));
			if (!line) {
				free(tmp);
				free(buf);
				free(terminal->lines);
				terminal->lines = NULL;
				return false;
			}

			create_line_text(line, tmp);
			terminal->lines[terminal->lineCount++] = line;

			free(tmp);
			start = buf + i + 1;
		}
	}

	if (terminal->lineCount == 0) {
		free(buf);
		initEmptyBuffer(terminal);
		return true;
	}

	free(buf);
	terminal->isSaved = true;

	return true;
}