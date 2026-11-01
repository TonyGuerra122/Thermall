#include "../include/terminal.h"

int main(int argc, char** argv) {
	const char* filename = NULL;

	if (argc > 1) {
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-H") == 0) {
			printf("Usage: %s [filename]\n", argv[0]);
			printf("If no filename is provided, an empty unnamed buffer is opened.\n");
			return 0;
		}
		filename = argv[1];
	}

	Terminal* terminal = createTerminal(filename);

	if (!terminal) {
		fprintf(stderr, "Failed to create terminal\n");
		return 1;
	}

	handleTerminalInput(terminal);

	// freeTerminal também encerra o curses (endwin)
	freeTerminal(terminal);

	return 0;
}
