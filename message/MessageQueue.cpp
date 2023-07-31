//
// Created by Chiranjeev Kumar on 7/30/23.
//

#include "MessageQueue.h"

void MessageQueue::send(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(message);
    condition_.notify_one();
}

std::string MessageQueue::receive() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]{ return !queue_.empty(); });

    std::string message = queue_.front();
    queue_.pop();

    return message;
}
