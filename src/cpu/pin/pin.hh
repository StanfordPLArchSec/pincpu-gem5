#pragma once

#include <sys/types.h>
#include <unistd.h>

namespace gem5
{

namespace pin
{

class Pin
{
  public:
    pid_t pinPid = -1;
    int reqFd = -1;
    int respFd = -1;
    
};

}

}
