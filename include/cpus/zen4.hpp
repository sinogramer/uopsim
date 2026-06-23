#pragma once
#include "core/cpu_model.hpp"

inline const CpuModel ZEN4 = {
    /* name           */ "Zen4",
    /* num_ports      */ 4,
    /* dispatch_width */ 6,
    /* rob_size       */ 320,
    /* schedq_size    */ 64,
};
