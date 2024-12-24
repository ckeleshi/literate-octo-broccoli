#pragma once
#include <chrono>

class timer
{
  public:
    void start_from_now()
    {
        _start_time = std::chrono::steady_clock::now();
    }

    long long get_elapsed_ms() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _start_time)
            .count();
    }

  private:
    std::chrono::steady_clock::time_point _start_time;
};
