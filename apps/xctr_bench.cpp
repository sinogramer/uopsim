#include <cstdio>
#include "algo/xctr.hpp"
#include "cpus/zen4.hpp"
#include "isa/zen4_isa.hpp"
#include "cpus/sappirerapids.hpp"
#include "isa/sappirerapids_isa.hpp"

#define BUFFER_SIZE (1024 * 1024)

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    struct Variant {
        const char* label;
        int (*func)(Scheduler&, Isa&, int);
    };

    Variant variants[] = {
        // AES-128
        { "xctr_aes128",    algo::xctr::xctr_aes128    },
        { "xctr_aes128x4",  algo::xctr::xctr_aes128x4  },
        { "xctr_aes128x8",  algo::xctr::xctr_aes128x8  },
        // AES-256
        { "xctr_aes256",    algo::xctr::xctr_aes256    },
        { "xctr_aes256x4",  algo::xctr::xctr_aes256x4  },
        { "xctr_aes256x8",  algo::xctr::xctr_aes256x8  },
    };

    std::printf("=== xctr on Zen4 (buffer = %d KB) ===\n", BUFFER_SIZE / 1024);
    for (auto& v : variants) {
        Scheduler s(ZEN4);
        v.func(s, isa::zen4::instance(), num_blocks);
        std::printf("%-16s : %10d cycles, %.4f c/B\n",
                    v.label, s.total_cycles(),
                    (double)s.total_cycles() / BUFFER_SIZE);
    }

    std::printf("=== xctr on SappireRapids (buffer = %d KB) ===\n", BUFFER_SIZE / 1024);
    for (auto& v : variants) {
        Scheduler s(SR);
        v.func(s, isa::zen4::instance(), num_blocks);
        std::printf("%-16s : %10d cycles, %.4f c/B\n",
                    v.label, s.total_cycles(),
                    (double)s.total_cycles() / BUFFER_SIZE);
    }
    return 0;
}