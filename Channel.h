#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <deque>
#include <thread>

#include "ChannelValue.h"

namespace Async {
    template<typename T>
    class Channel {
    public:
        typedef std::shared_ptr<Channel> ptr;
        typedef std::weak_ptr<Channel> weakPtr;

    public:
        Channel(int limitSize = 0) // 0 means no limit
        : m_limitSize(limitSize), m_closed(false) {}

        ~Channel() {
            Close();
        }

        bool Push(T t);
        ChannelValue<T> Pop(int timeout_milliseconds = 0);

        void Close();
        bool IsClosed() const;

    private:
        int m_limitSize;
        std::deque<T> m_values;
        bool m_closed;

        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
    };

    template<typename T>
    bool Channel<T>::Push(T t) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_closed) {
            return false;
        }

        while (m_limitSize > 0 && m_values.size() >= m_limitSize) {
            m_cv.wait(lock);
        }

        m_values.push_back(t);
        m_cv.notify_all();

        return true;
    }

    template<typename T>
    ChannelValue<T> Channel<T>::Pop(int timeout_milliseconds) {
        auto now = std::chrono::system_clock::now();
        auto timeout = now + std::chrono::milliseconds(timeout_milliseconds);

        std::unique_lock<std::mutex> lock(m_mutex);

        while (m_values.empty()) {
            if (m_closed ) {
                return ChannelValue<T>::Closed();
            }

            if (timeout_milliseconds > 0) {
                auto cvStatus = m_cv.wait_until(lock, timeout);
                if (cvStatus == std::cv_status::timeout) {
                    return ChannelValue<T>::Timeout();
                }
            } else {
                m_cv.wait(lock);
            }
        }

        auto value = ChannelValue<T>(m_values.front());
        m_values.pop_front();
        m_cv.notify_all();

        return value;
    }

    template<typename T>
    void Channel<T>::Close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
        m_cv.notify_all();
    }

    template<typename T>
    bool Channel<T>::IsClosed() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_closed;
    }
}