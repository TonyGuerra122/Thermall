#include "../include/terminal.h"

Terminal* createTerminal(const char* filename) {
	// Inicializa curses
	initscr();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(1);

	// Aloca Terminal
	Terminal* terminal = malloc(sizeof(Terminal));
	if (!terminal) return NULL;

	terminal->filename = filename ? strdup(filename) : NULL;
	terminal->lines = malloc(sizeof(LineText*));
	if (!terminal->lines) {
		if (terminal->filename) free(terminal->filename);
		free(terminal);
		endwin();
		return NULL;
	}
	terminal->lineCount = 1;

	// Cria primeira linha
	LineText* line = malloc(sizeof(LineText));
	if (!line) {
		if (terminal->filename) free(terminal->filename);
		free(terminal->lines);
		free(terminal);
		endwin();
		return NULL;
	}
	create_line_text(line, "");
	terminal->lines[0] = line;
	terminal->isSaved = true;

	return terminal;
}

void handleTerminalInput(Terminal* terminal) {
	int ch, cursorX = 0, cursorY = 0;

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
			break;
		default:
			if (ch >= 32 && ch <= 126 && cursorX < COLS - 1) {
				insert_char(terminal->lines[cursorY], cursorX, (char)ch);
				cursorX++;
				terminal->isSaved = false;
			}

			break;
		}

		for (size_t i = 0; i < terminal->lineCount && i < (size_t)LINES - 1; i++) {
			mvprintw(i, 0, "%s", terminal->lines[i]->text);
		}

		char status[256];
		const char* saved = terminal->isSaved ? "(Saved)" : "(Unsaved)";
		double memUsage = showHeader(terminal);
		snprintf(status, sizeof(status), "File: (%s) Line: %d Column: %d Total Lines: %zu Mem: %.2fMB Saved: %s",
			terminal->filename, cursorY + 1, cursorX + 1, terminal->lineCount, memUsage, saved);

		mvprintw(LINES - 1, 0, "%s", status);

		move(cursorY, cursorX);
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

double showHeader(const Terminal* terminal) {
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