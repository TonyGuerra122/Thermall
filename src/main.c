#include "../include/terminal.h"

int main(int argc, char** argv) {
	const char* filename = argc > 1 ? argv[1] : "untitled";

	if (filename == "--help" || filename == "-H") {
		printf("Usage: %s [filename]\n", argv[0]);
		printf("If no filename is provided, it will create an untitled terminal.\n");
		return 0;
	}

	Terminal* terminal = createTerminal(filename);

	if (!terminal) {
		fprintf(stderr, "Failed to create terminal\n");
		return 1;
	}

	handleTerminalInput(terminal);

	endwin();

	freeTerminal(terminal);

	return 0;
}
