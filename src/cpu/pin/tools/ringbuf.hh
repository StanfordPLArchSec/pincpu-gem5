#pragma once

#include <cstddef>
#include <array>

template <typename T, size_t N>
class RingBuffer {
    static_assert((N & (N - 1)) == 0, "template argument N must be a power of 2");
  public:
    RingBuffer(const T &value)
    {
        for (T &x : buf)
            x = value;
    }

    RingBuffer()
        : RingBuffer(T())
    {
    }

    void push(const T &value)
    {
        buf[next] = value;
        next = (next + 1) % N;
    }

    template <class OutputIt>
    OutputIt get(OutputIt out) {
        for (size_t i = 0; i < N; ++i)
            *out++ = buf[(next + i) % N];
        return out;
    }
    
  private:
    std::array<T, N> buf;
    size_t next;
};
