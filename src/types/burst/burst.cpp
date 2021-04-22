#include "types/burst/burst.hpp"

#include <cassert>
#include <stdexcept>

Burst::Burst(BurstType type, int length)
{
    burst_type = type;
    length = length;
}

void Burst::update_time(int delta_t)
{
    length -= delta_t;
}
