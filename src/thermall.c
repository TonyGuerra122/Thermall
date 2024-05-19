#include "thermall.h"
#include "text_buffer.h"

void init_thermall()
{
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);
}

void run_thermall(const char *filename)
{
    const char *file = strlen(filename) > 0 ? filename : "untitled";

    TextBuffer *buffer = create_text_buffer(file);

    clear();
    printw("File: %s\n", buffer->filename);
    printw("%s", buffer->content);
    move(1, 0); // Move cursor below the file name

    handle_input(buffer);

    free_text_buffer(buffer);
}

void handle_input(TextBuffer *buffer)
{
    int ch;
    size_t cursor = buffer->size;
    int x = 0, y = 1;

    while ((ch = getch()) != 27)
    { // Press 'ESC' to quit
        switch (ch)
        {
        case '\b': // Backspace key in Windows
        case 127:  // Handle both '\b' and ASCII 127 for backspace
            if (cursor > 0)
            {
                if (buffer->content[cursor - 1] == '\n')
                {
                    // Move cursor to the end of the previous line
                    cursor--;
                    buffer->size--;
                    if (y > 1)
                    {
                        y--;
                        x = 0;
                        while (cursor > 0 && buffer->content[cursor - 1] != '\n')
                        {
                            cursor--;
                            x++;
                        }
                    }
                }
                else
                {
                    memmove(&buffer->content[cursor - 1], &buffer->content[cursor], buffer->size - cursor + 1);
                    buffer->size--;
                    cursor--;
                    x--;
                }
            }
            break;
        case KEY_DC: // Delete key
            if (cursor < buffer->size)
            {
                memmove(&buffer->content[cursor], &buffer->content[cursor + 1], buffer->size - cursor);
                buffer->size--;
            }
            break;
        case '\n': // Enter key
            buffer->content = (char *)realloc(buffer->content, buffer->size + 2);
            memmove(&buffer->content[cursor + 1], &buffer->content[cursor], buffer->size - cursor + 1);
            buffer->content[cursor] = '\n';
            buffer->size++;
            cursor++;
            x = 0;
            y++;
            break;
        case KEY_LEFT: // Left arrow key
            if (cursor > 0)
            {
                cursor--;
                if (buffer->content[cursor] == '\n')
                {
                    y--;
                    x = 0;
                    size_t temp_cursor = cursor;
                    while (temp_cursor > 0 && buffer->content[temp_cursor - 1] != '\n')
                    {
                        temp_cursor--;
                        x++;
                    }
                }
                else
                {
                    x--;
                }
            }
            break;
        case KEY_RIGHT: // Right arrow key
            if (cursor < buffer->size)
            {
                if (buffer->content[cursor] == '\n')
                {
                    y++;
                    x = 0;
                }
                else
                {
                    x++;
                }
                cursor++;
            }
            break;
        case KEY_UP: // Up arrow key
            if (y > 1)
            {
                y--;
                x = 0;
                size_t temp_cursor = cursor;
                while (temp_cursor > 0 && buffer->content[temp_cursor - 1] != '\n')
                {
                    temp_cursor--;
                }
                if (temp_cursor > 0)
                {
                    temp_cursor--; // Skip the newline character
                }
                while (temp_cursor > 0 && buffer->content[temp_cursor - 1] != '\n')
                {
                    temp_cursor--;
                    x++;
                }
                cursor = temp_cursor;
            }
            break;
        case KEY_DOWN: // Down arrow key
            if (y < getmaxy(stdscr) - 1)
            {
                y++;
                x = 0;
                while (buffer->content[cursor] != '\n' && cursor < buffer->size)
                {
                    cursor++;
                }
                if (buffer->content[cursor] == '\n')
                {
                    cursor++;
                }
                while (buffer->content[cursor] != '\n' && cursor < buffer->size)
                {
                    cursor++;
                    x++;
                }
            }
            break;
        case 19: // CTRL + S to save
            save_text_buffer(buffer);
            break;
        default: // Handle other characters
            buffer->content = (char *)realloc(buffer->content, buffer->size + 2);
            memmove(&buffer->content[cursor + 1], &buffer->content[cursor], buffer->size - cursor + 1);
            buffer->content[cursor] = ch;
            buffer->size++;
            cursor++;
            if (x < getmaxx(stdscr) - 1)
            {
                x++;
            }
            else if (y < getmaxy(stdscr) - 1)
            {
                y++;
                x = 0;
            }
            break;
        }

        buffer->content[buffer->size] = '\0'; // Null-terminate the content

        clear();
        printw("File: %s\n", buffer->filename);
        printw("%s", buffer->content);
        move(y, x);
        refresh();
    }
}

void exit_thermall()
{
    refresh();
    endwin();
}
