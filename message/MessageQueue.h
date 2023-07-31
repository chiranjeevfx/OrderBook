//
// Created by Chiranjeev Kumar on 7/30/23.
//

#ifndef MYPROJECT_MESSAGEQUEUE_H
#define MYPROJECT_MESSAGEQUEUE_H



#include <queue>
#include <mutex>
#include <condition_variable>

class MessageQueue {
public:
    void send(const std::string& message);
    std::string receive();

private:
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

#endif //MYPROJECT_MESSAGEQUEUE_H

