#include "../include/terminal.h"

int main() {
	Terminal* terminal = createTerminal("example.txt");

	if (!terminal) {
		fprintf(stderr, "Failed to create terminal\n");
		return 1;
	}

	handleTerminalInput(terminal);

	endwin();

	freeTerminal(terminal);

	return 0;
}
