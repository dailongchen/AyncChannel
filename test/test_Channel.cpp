#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <sstream>

#include "../Channel.h"

using namespace Async;

static int logLineCount = 0;
void logLine(const std::string& line) {
    static std::mutex logMutex;

    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << line << std::endl;
    logLineCount++;
}

void producerFunc(Channel<std::string>::ptr channel) {
    auto threadId = std::this_thread::get_id();
    for (int i = 0; i < 10; i++) {
        std::stringstream ss;
        ss << "send by " << threadId << ": " << i;
        channel->Push(ss.str());
    }
}

void consumerFunc(Channel<std::string>::ptr channel) {
    auto threadId = std::this_thread::get_id();
    while(true) {
        auto value = channel->Pop(100);
        if (value.IsOk()) {
            std::stringstream ss;
            ss << "received in " << threadId << ": " << value.Get();
            logLine(ss.str());
        } else {
            if (value.IsTimeout()) {
                logLine("timeout");
            }
            if (value.IsClosed()) {
                logLine("channel is closed");
            }
            break;
        }
    }
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    auto channel = std::make_shared<Channel<std::string>>(5);
    std::vector<std::shared_ptr<std::thread>> producerThreads;
    std::vector<std::shared_ptr<std::thread>> consumerThreads;

    for (int i = 0; i < 10; i++) {
        auto producerThread_1 = std::make_shared<std::thread>(producerFunc, channel);
        auto producerThread_2 = std::make_shared<std::thread>(producerFunc, channel);

        auto consumerThread = std::make_shared<std::thread>(consumerFunc, channel);

        producerThreads.push_back(producerThread_1);
        producerThreads.push_back(producerThread_2);
        consumerThreads.push_back(consumerThread);
    }

    for (auto& thread : producerThreads) {
        if (thread->joinable()) {
            thread->join();
        }
    }

    channel->Close();

    for (auto& thread : consumerThreads) {
        if (thread->joinable()) {
            thread->join();
        }
    }

    std::cout << "total " << logLineCount << " lines" << std::endl;
    REQUIRE(logLineCount == 210);
}