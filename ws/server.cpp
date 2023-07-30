//
// Created by Chiranjeev Kumar on 7/30/23.
//

#define BOOST_NO_CXX17_HDR_STRING_VIEW // Add this line to use std::string_view

#include <iostream>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main() {
    net::io_context ioc;
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 9002));

    while (true) {
        tcp::socket socket(ioc);

        acceptor.accept(socket);

        websocket::stream <tcp::socket> ws(std::move(socket));

        ws.accept();

        beast::flat_buffer buffer;
        beast::error_code ec;

        while (true) {
            // Receive message from the client
            ws.read(buffer, ec);

            if (ec == websocket::error::closed) {
                // Client closed the connection
                break;
            } else if (ec) {
                // Some error occurred
                std::cerr << "Error reading message: " << ec.message() << std::endl;
                break;
            }

            // Send the received message back to the client
            ws.text(ws.got_text());
            ws.write(buffer.data(), ec);
            if (ec) {
                std::cerr << "Error sending message: " << ec.message() << std::endl;
                break;
            }
        }
    }

    return 0;
}
