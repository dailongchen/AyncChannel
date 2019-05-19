#pragma once

#include <stdexcept>

namespace Async {
    template<typename T>
    class ChannelValue {
    public:
        static ChannelValue<T> Timeout() {
            ChannelValue<T> value;
            value.m_timeout = true;

            return value;
        }

        static ChannelValue<T> Closed() {
            ChannelValue<T> value;
            value.m_closed = true;

            return value;
        }

    public:
        ChannelValue(T t)
        : m_value(t), m_timeout(false), m_closed(false) {}

        T Get() const {
            if (IsOk()) {
                return m_value;
            }

            if (IsClosed()) {
                throw new std::runtime_error("Channel is closed");
            }

            if (IsTimeout()) {
                throw new std::runtime_error("Timeout failure");
            }

            throw new std::runtime_error("Unknown failure");
        }

        bool IsOk() const { return !m_timeout && !m_closed; }
        bool IsClosed() const { return m_closed; }
        bool IsTimeout() const { return m_timeout; }

    private:
        ChannelValue() {}

    private:
        T m_value;
        bool m_timeout;
        bool m_closed;
    };
}