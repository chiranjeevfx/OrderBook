//
// Created by Chiranjeev Kumar on 7/30/23.
//

#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using error_code = boost::system::error_code; // Explicitly specify the error_code type to avoid ambiguity

int main() {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        auto const results = resolver.resolve("localhost", "9002");
        net::connect(ws.next_layer(), results.begin(), results.end());

        // WebSocket handshake
        ws.handshake("localhost", "/");

        for (int i = 1; i <= 20; ++i) {
            // Send a message to the server
            std::string msg = "Message " + std::to_string(i);
            ws.write(net::buffer(msg));

            // Receive the server's response
            beast::flat_buffer buffer;
            ws.read(buffer);

            // Print the response
            std::cout << "Received: " << beast::buffers_to_string(buffer.data()) << std::endl;
        }

        // Gracefully close the WebSocket connection
//        ws.close(websocket::close_code::normal);

    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
