#pragma once
#include <Windows.h>
#include <cstring>


class scoped_period
{
  public:
    scoped_period()
    {
        std::memset(&_caps, 0, sizeof(_caps));
        timeGetDevCaps(&_caps, sizeof(_caps));
        timeBeginPeriod(_caps.wPeriodMin);
    }

    ~scoped_period()
    {
        timeEndPeriod(_caps.wPeriodMin);
    }

  private:
    TIMECAPS _caps;
};
