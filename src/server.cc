#include "../libs/json.hh"
using json = nlohmann::json;

#include "http.hh"

#include <iostream>
#include <stdio.h>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) { // error if user did not pass prefrences.json as cli argument
        std::cout << 
            "Error run the program with ./server <path to prefrences.json>" 
        << std::endl;
        return 1;
    }

    std::thread http (startServer, argv[1]);

    std::this_thread::sleep_for (std::chrono::seconds(60));

    shouldExit = true;

    http.join();

    return 0;
}