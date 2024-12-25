#pragma once
#include <chrono>

class target_clock
{
  public:
    void set_target_time_from_now(std::chrono::steady_clock::duration period)
    {
        _end_time = std::chrono::steady_clock::now() + period;
    }

    bool target_time_reached() const
    {
        return std::chrono::steady_clock::now() >= _end_time;
    }

    void adjust_target_time(std::chrono::steady_clock::duration additional)
    {
        _end_time += additional;
    }

    std::chrono::steady_clock::duration rest_to_target_time() const
    {
        return _end_time - std::chrono::steady_clock::now();
    }

  private:
    std::chrono::steady_clock::time_point _end_time;
};
