#include "core/header.hpp"
#include "algo/polyval.hpp"
#include "cpus/zen4.hpp"
#include "isa/zen4_isa.hpp"
#include "cpus/sappirerapids.hpp"
#include "isa/sappirerapids_isa.hpp"

#define BUFFER_SIZE (1024 * 1024)

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    Scheduler sched(SR);
    algo::polyval::polyvalx8(sched, isa::sr::instance(), num_blocks);

    std::printf("CPU: %s, total cycles = %d, cycles/byte = %.4f\n",
                sched.cpu().name.c_str(),
                sched.total_cycles(),
                (double)sched.total_cycles() / BUFFER_SIZE);
    
    std::printf("\n");
    sched.print_utilization();

                
    std::printf("\n--- Schedule (cycle 0..100) ---\n");
    sched.print_schedule(0, 100);

    return 0;
}