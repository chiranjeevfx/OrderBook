//
// Created by Chiranjeev Kumar on 7/30/23.
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <list>
#include <algorithm>
using namespace std;

using boost::asio::ip::udp;

int main() {
    string inputFileName = "input.csv";
    ifstream file(inputFileName);
    string line;
    boost::asio::io_context io_context;

    udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_context);
    udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), "localhost", "9003");

    std::string message = "Hello, UDP Server!";
    boost::system::error_code error;
    while (getline(file, line)) {
        socket.send_to(boost::asio::buffer(line), *endpoints.begin(), 0, error);
        if (error) {
            std::cerr << "Error sending data: " << error.message() << std::endl;
            return 1;
        }
    }


    char reply[1024];
    udp::endpoint sender_endpoint;

    size_t reply_length = socket.receive_from(boost::asio::buffer(reply), sender_endpoint, 0, error);
    if (error && error != boost::asio::error::message_size) {
        std::cerr << "Error receiving data: " << error.message() << std::endl;
        return 1;
    }

    std::cout << "Received reply from " << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
    std::cout << "Reply: " << reply << std::endl;

    return 0;
}
