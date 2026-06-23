/*マイクロオペレーションを管理*/
#pragma once
#include "header.hpp"

class uops {
public:
    uops(int cycles, uint8_t allowed_ports, std::string name)
        : cycles(cycles), allowed_ports(allowed_ports), name(std::move(name)) {}

    int         get_cycles()        const { return cycles; }
    uint8_t     get_allowed_ports() const { return allowed_ports; }
    const std::string& get_name()   const { return name; }

private:
    int cycles;
    uint8_t allowed_ports = 0b0000;
    std::string name = "*********";
};
