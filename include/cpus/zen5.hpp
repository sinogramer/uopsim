#pragma once
#include "cpu_model.hpp"

inline const CpuModel ZEN5 = {
    /* name           */ "Zen5",
    /* num_ports      */ 4,
    /* dispatch_width */ 6,
    /* rob_size       */ 320,
    /* schedq_size    */ 64,
};
