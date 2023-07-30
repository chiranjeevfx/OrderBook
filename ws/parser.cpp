//
// Created by Chiranjeev Kumar on 7/30/23.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::istringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        result.push_back(field);
    }
    return result;
}

int main() {
    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));

    std::ifstream file("../client/input.csv");
    if (!file.is_open()) {
        std::cerr << "Failed to open input.csv" << std::endl;
        return 1;
    }

    std::string server_ip = "127.0.0.1";
    unsigned short server_port = 9002;

    std::string line;
    while (std::getline(file, line)) {
        std::cout<<"while loop\n";
        std::cout<<line<<"\n";
        // Parse CSV line
        std::vector<std::string> fields = parseCSVLine(line);

        // Convert CSV line to a single string
        std::string message;
        for (const std::string& field : fields) {
            message += field + ",";
        }
        // Remove the trailing comma
        if (!message.empty()) {
            message.pop_back();
        }

        // Send message to the server
        socket.send_to(boost::asio::buffer(message), udp::endpoint(boost::asio::ip::address::from_string(server_ip), server_port));

        // Receive and print the server's response
        char recv_buffer[1024];
        udp::endpoint sender_endpoint;
        size_t length = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);
        std::cout<< "receive_from" <<"\n";
        if (length > 0) {
            std::string response(recv_buffer, length);
            std::cout << "Received from server: " << response << std::endl;
        }
    }

    return 0;
}
