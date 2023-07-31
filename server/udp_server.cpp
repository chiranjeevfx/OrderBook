//
// Created by Chiranjeev Kumar on 7/30/23.
//


#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main() {
    boost::asio::io_context io_context;

    udp::socket socket(io_context, udp::endpoint(udp::v4(), 9002));

    while (true) {
        char data[1024];
        udp::endpoint remote_endpoint;

        boost::system::error_code error;
        size_t length = socket.receive_from(boost::asio::buffer(data), remote_endpoint, 0, error);

        if (error && error != boost::asio::error::message_size) {
            std::cerr << "Error receiving data: " << error.message() << std::endl;
            continue;
        }

        std::cout << "Received message from " << remote_endpoint.address().to_string() << ":" << remote_endpoint.port() << std::endl;
        std::cout << "Message: " << data << std::endl;

        socket.send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
        if (error) {
            std::cerr << "Error sending data: " << error.message() << std::endl;
        }
    }

    return 0;
}
