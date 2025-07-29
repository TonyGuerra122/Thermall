#include "../include/terminal.h"

int main(int argc, char** argv) {
	const char* filename = argc > 1 ? argv[1] : "untitled";

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
