#include "core/header.hpp"
#include "algo/polyval.hpp"
#include "cpus/zen4.hpp"
#include "isa/zen4_isa.hpp"

#define BUFFER_SIZE (1024 * 1024)

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    Scheduler sched(ZEN4);
    algo::polyval::polyvalx8(sched, isa::zen4::instance(), num_blocks);

    std::printf("CPU: %s, total cycles = %d, cycles/byte = %.4f\n",
                sched.cpu().name.c_str(),
                sched.total_cycles(),
                (double)sched.total_cycles() / BUFFER_SIZE);
    return 0;
}