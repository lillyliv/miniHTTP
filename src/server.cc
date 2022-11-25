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

    FILE *fp;

    fp = fopen(argv[1], "r");
    json j = json::parse(fp);
    const auto contents = j.dump();
    std::cout << contents << std::endl;

    std::thread http (startServer, 8080);

    std::this_thread::sleep_for (std::chrono::seconds(60));

    shouldExit = true;

    http.join();

    return 0;
}