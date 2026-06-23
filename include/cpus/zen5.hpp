#pragma once
#include "core/cpu_model.hpp"

namespace cpu_strategy {
inline std::vector<int> zen5_port_priority(uint8_t allowed) {
    if (allowed == 0b1111) return {3, 0, 1, 2};
    if (allowed == 0b1110) return {3, 2, 1};
    std::vector<int> r;
    for (int p = 0; p < 4; ++p) {
        if ((allowed >> p) & 1) r.push_back(p);
    }
    return r;
}
}

inline const CpuModel ZEN5 = {
    "Zen5", 4, 8, 448, 96, cpu_strategy::zen5_port_priority,
};
