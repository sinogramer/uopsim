#include "core/header.hpp"
#include "algo/xctr.hpp"
#include "cpus/zen4.hpp"
#include "isa/zen4_isa.hpp"

int main() {
    Scheduler sched(ZEN4);
    algo::xctr::xctr(sched, isa::zen4::instance(), 65536);
    std::printf("xctr: %d cycles\n", sched.total_cycles());
    return 0;
}
