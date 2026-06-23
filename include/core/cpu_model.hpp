#pragma once
#include "core/header.hpp"

struct CpuModel {
    std::string name;
    int num_ports;
    int dispatch_width;
    int rob_size;
    int schedq_size;
};