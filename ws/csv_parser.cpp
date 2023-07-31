//
// Created by Chiranjeev Kumar on 7/30/23.
//

#include "csv_parser.h"
#include <sstream>
#include <iostream>

std::vector<std::string> parseCSVLine(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    while (std::getline(tokenStream, token, delimiter)) {
//        std::cout<<token<<"\n"<<"\n";
        tokens.push_back(token);
    }
    return tokens;
}
