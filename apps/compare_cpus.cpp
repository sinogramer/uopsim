#include "core/header.hpp"
#include "algo/polyval.hpp"
#include "cpus/zen4.hpp"
#include "isa/zen4_isa.hpp"
#include "cpus/sappirerapids.hpp"
#include "isa/sappirerapids_isa.hpp"

#define BUFFER_SIZE (1024 * 1024)

struct Target {
    const char* label;
    const CpuModel& cpu;
    Isa& isa;
};

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    Target targets[] = {
        { "Zen4", ZEN4, isa::zen4::instance() },
        { "SappireRapids", SR, isa::sr::instance() },
    };

    std::printf("=== polyvalx8 ===\n");
    for (auto& t : targets) {
        Scheduler s(t.cpu);
        algo::polyval::polyvalx8(s, t.isa, num_blocks);
        std::printf("%-6s : %10d cycles, %.4f c/B\n",
                    t.label, s.total_cycles(),
                    (double)s.total_cycles() / BUFFER_SIZE);
    }
    return 0;
}