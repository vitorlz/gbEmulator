#include "app.h"

int main(int argc, char* argv[])
{
    App app;
    std::string rom = argc > 1 ? argv[1] : "";
    app.run(rom);
}
