//
// Created by Chiranjeev Kumar on 7/30/23.
//
#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <string>
#include <thread>

using boost::asio::ip::udp;

class MessageQueue {
public:
    void push(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back(msg);
        condition.notify_one();
    }

    std::string pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition.wait(lock, [&]{ return !messages_.empty(); });
        std::string msg = messages_.front();
        messages_.pop_front();
        return msg;
    }

private:
    std::mutex mutex_;
    std::condition_variable condition;
    std::deque<std::string> messages_;
};

void receiver(boost::asio::io_context& io_context, MessageQueue& message_queue, udp::socket& socket) {
    while (true) {
        std::cout<<"receiver\n";
        char recv_buffer[1024];
        udp::endpoint sender_endpoint;
        size_t length = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);
        if (length > 0) {
            std::string msg(recv_buffer, length);
            message_queue.push(msg);
        }
    }
}

void sender(boost::asio::io_context& io_context, MessageQueue& message_queue, udp::socket& socket, udp::endpoint& receiver_endpoint) {
    while (true) {
        std::cout<<"sender\n";
        std::string msg = message_queue.pop();
        socket.send_to(boost::asio::buffer(msg), receiver_endpoint);
    }
}

int main() {
    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));
    udp::resolver resolver(io_context);
    udp::resolver::query query(udp::v4(), "127.0.0.1", "9002");
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    MessageQueue message_queue;
    std::cout<<"main1\n";

    // Start the receiver thread
    std::thread receiver_thread(receiver, std::ref(io_context), std::ref(message_queue), std::ref(socket));

    // Start the sender thread
    std::thread sender_thread(sender, std::ref(io_context), std::ref(message_queue), std::ref(socket), std::ref(receiver_endpoint));

    // Run the io_context to start receiving and sending messages
    std::cout<<"main0\n";
    io_context.run();
    std::cout<<"main1\n";

    // Join the receiver and sender threads before exiting
    receiver_thread.join();
    std::cout<<"main2\n";
    sender_thread.join();
    std::cout<<"main3\n";

    return 0;
}
