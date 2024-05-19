#include "thermall.h"

int main(int argc, char *argv[])
{
    const char *filename = (argc > 1) ? argv[1] : "";

    init_thermall();
    run_thermall(filename);
    exit_thermall();

    return 0;
}