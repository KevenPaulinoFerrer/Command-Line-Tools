#include <stdlib.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

std::string help()
{
    return "";
}

int main(int argc, char *argv[])
{
    if (argc < 0)
    {
        std::cout << help();
    }

    return 0;
}
