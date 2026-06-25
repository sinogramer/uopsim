#include <cstdio>
#include "algo/xctr_polyval.hpp"
#include "algo/xctr.hpp"
#include "algo/polyval.hpp"
#include "cpus/zen4.hpp"
#include "cpus/sappirerapids.hpp"
#include "isa/zen4_isa.hpp"
#include "isa/sappirerapids_isa.hpp"

#define BUFFER_SIZE (1024 * 1024)

struct Target {
    const char* label;
    const CpuModel& cpu;
    Isa& isa;
};

struct AesVariant {
    const char* label;
    int (*xctr_fn)(Scheduler&, Isa&, int);
    int (*fused_fn)(Scheduler&, Isa&, int);
};

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    Target targets[] = {
        { "Zen4", ZEN4, isa::zen4::instance() },
        { "SR",   SR,   isa::sr::instance()   },
    };

    AesVariant variants[] = {
        { "AES-128", algo::xctr::xctr_aes128x4,
                     algo::xctr_polyval::xctr_polyvalx4_aes128 },
        { "AES-256", algo::xctr::xctr_aes256x4,
                     algo::xctr_polyval::xctr_polyvalx4_aes256 },
    };

    std::printf("=== AEAD (xctr + polyval), buffer = %d KB ===\n\n",
                BUFFER_SIZE / 1024);

    for (auto& t : targets) {
        std::printf("---- %s ----\n", t.label);
        // polyval 単体は AES と独立
        Scheduler s_poly(t.cpu);
        algo::polyval::polyvalx4(s_poly, t.isa, num_blocks);
        double poly_cb = (double)s_poly.total_cycles() / BUFFER_SIZE;
        std::printf("  polyvalx4              : %10d cycles, %.4f c/B\n",
                    s_poly.total_cycles(), poly_cb);

        for (auto& v : variants) {
            Scheduler s_xctr(t.cpu);
            v.xctr_fn(s_xctr, t.isa, num_blocks);
            double xctr_cb = (double)s_xctr.total_cycles() / BUFFER_SIZE;

            Scheduler s_seq(t.cpu);
            v.xctr_fn(s_seq, t.isa, num_blocks);
            algo::polyval::polyvalx4(s_seq, t.isa, num_blocks);
            double seq_cb = (double)s_seq.total_cycles() / BUFFER_SIZE;

            Scheduler s_fused(t.cpu);
            v.fused_fn(s_fused, t.isa, num_blocks);
            double fused_cb = (double)s_fused.total_cycles() / BUFFER_SIZE;

            std::printf("  [%s]\n", v.label);
            std::printf("    xctr_only            : %10d cycles, %.4f c/B\n",
                        s_xctr.total_cycles(), xctr_cb);
            std::printf("    serial (xctr+poly)   : %10d cycles, %.4f c/B\n",
                        s_seq.total_cycles(),  seq_cb);
            std::printf("    fused                : %10d cycles, %.4f c/B\n",
                        s_fused.total_cycles(), fused_cb);
            std::printf("    speedup (serial/fused): %.2fx\n",
                        seq_cb / fused_cb);
        }
        std::printf("\n");
    }
    return 0;
}