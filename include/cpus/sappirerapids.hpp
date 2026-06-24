#pragma once
#include "core/cpu_model.hpp"

namespace cpu_strategy {
inline std::vector<int> sr_port_priority(uint8_t allowed) {
    if (allowed == 0b0100011) return {0, 1, 5};
    if (allowed == 0b0100010) return {1, 5};
    if (allowed == 0b0000011) return {0, 1};
    std::vector<int> r;
    for (int p = 0; p < 7; ++p) {
        if ((allowed >> p) & 1) r.push_back(p);
    }
    return r;
}
}

inline const CpuModel SR = {
    /* name           */ "Sapphire Rapids",
    /* num_ports      */ 7,
    /* dispatch_width */ 6,
    /* rob_size       */ 512,
    /* schedq_size    */ 205,
    cpu_strategy::sr_port_priority,
};
