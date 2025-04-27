#pragma once

#include <iostream>

#define ENABLE_DEBUGGING 1

#if ENABLE_DEBUGGING
# define DEBUG(...) \
    do {            \
    __VA_ARGS__;    \
    } while (0)
#else
# define DEBUG(...) do { } while (0)
#endif

#if ENABLE_DEBUGGING
static inline std::ostream &
dbgs()
{
    return std::cerr;
}
#else
struct DummyPrinter
{
};

static inline DummyPrinter
dbgs()
{
    return DummyPrinter();
}

template <typename T>
DummyPrinter
operator<<(DummyPrinter, const T&)
{
    return DummyPrinter();
}
#endif
