#pragma once
#include "core/scheduler.hpp"
#include "core/isa.hpp"
#include "core/argument.hpp"

namespace algo::xctr_polyval {

// ============================================================
// schoolbook 補助関数（polyval.hpp と同等）
// ここでは独立コピーとして持つ
// ============================================================
namespace detail {

inline void schoolbook_add128(Scheduler& s, Isa& i,
        argument& reg, argument& htbl,
        argument& t0, argument& t1, argument& t2, argument& t3) {
    i.pclmulqdq().exe(s, t3, reg, htbl);
    i.pxor()     .exe(s, t2, t2, t3);
    i.pclmulqdq().exe(s, t3, reg, htbl);
    i.pxor()     .exe(s, t0, t0, t3);
    i.pclmulqdq().exe(s, t3, reg, htbl);
    i.pxor()     .exe(s, t1, t1, t3);
    i.pclmulqdq().exe(s, t3, reg, htbl);
    i.pxor()     .exe(s, t2, t2, t3);
}

inline void schoolbook_initialadd128(Scheduler& s, Isa& i,
        argument& reg, argument& htbl,
        argument& t0, argument& t1, argument& t2, argument& t3) {
    i.pclmulqdq().exe(s, t2, reg, htbl);
    i.pclmulqdq().exe(s, t0, reg, htbl);
    i.pclmulqdq().exe(s, t3, reg, htbl);
    i.pclmulqdq().exe(s, t1, reg, htbl);
    i.pxor()     .exe(s, t2, t2, t3);
}

inline void polyreduce128(Scheduler& s, Isa& i,
                          argument& out, argument& ctx, argument& x) {
    argument x0, x1, y0, y1, y2;
    i.pclmulqdq().exe(s, x0, x, ctx);
    i.pshufd()   .exe(s, y0, x);
    i.pxor()     .exe(s, y1, y0, x0);
    i.pclmulqdq().exe(s, x1, y1, ctx);
    i.pshufd()   .exe(s, y2, y1);
    i.pxor()     .exe(s, out, y2, x1);
}

// AES-256 1ブロック（汎用、ラウンド数指定可能）
inline void aes_encrypt(Scheduler& s, Isa& i, int num_rounds,
                        argument& out, argument& in, argument& key) {
    i.pxor().exe(s, out, in, key);
    for (int r = 0; r < num_rounds - 1; ++r) {
        i.aesenc().exe(s, out, out, key);
    }
    i.aesenclast().exe(s, out, out, key);
}


inline int xctr_polyvalx4_core(Scheduler& s, Isa& i,
                                int num_rounds, int num_blocks) {
    argument S, key, inc;
    argument ctr0, ctr1, ctr2, ctr3;
    argument data0, data1, data2, data3;
    argument tmps0, tmps1, tmps2, tmps3;
    argument htbl0, htbl1, htbl2, htbl3;
    argument X, Y, Z;

    for (int b = 0; b < num_blocks / 4; ++b) {
        // 1) xorx4
        i.pxor().exe(s, tmps0, ctr0, S);
        i.pxor().exe(s, tmps1, ctr1, S);
        i.pxor().exe(s, tmps2, ctr2, S);
        i.pxor().exe(s, tmps3, ctr3, S);

        // 2) AES x4 (num_rounds round)
        i.pxor().exe(s, tmps0, tmps0, key);
        i.pxor().exe(s, tmps1, tmps1, key);
        i.pxor().exe(s, tmps2, tmps2, key);
        i.pxor().exe(s, tmps3, tmps3, key);
        for (int r = 0; r < num_rounds - 1; ++r) {
            i.aesenc().exe(s, tmps0, tmps0, key);
            i.aesenc().exe(s, tmps1, tmps1, key);
            i.aesenc().exe(s, tmps2, tmps2, key);
            i.aesenc().exe(s, tmps3, tmps3, key);
        }
        i.aesenclast().exe(s, tmps0, tmps0, key);
        i.aesenclast().exe(s, tmps1, tmps1, key);
        i.aesenclast().exe(s, tmps2, tmps2, key);
        i.aesenclast().exe(s, tmps3, tmps3, key);

        // 3) data ^= tmps
        i.pxor().exe(s, data0, tmps0, data0);
        i.pxor().exe(s, data1, tmps1, data1);
        i.pxor().exe(s, data2, tmps2, data2);
        i.pxor().exe(s, data3, tmps3, data3);

        // 4) polyval 部
        polyreduce128(s, i, Y, htbl0, Y);
        i.pxor().exe(s, Z, X, Y);
        i.pxor().exe(s, Z, Z, data0);

        schoolbook_initialadd128(s, i, data3, htbl0,
                                  tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data2, htbl1,
                                  tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data1, htbl2,
                                  tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, Z,     htbl3,
                                  tmps0, tmps1, tmps2, tmps3);

        i.psrldq().exe(s, tmps3, tmps2);
        i.pslldq().exe(s, tmps2, tmps2);
        i.pxor().exe(s, X, tmps3, tmps1);
        i.pxor().exe(s, Y, tmps0, tmps2);

        // 5) ctr 増分
        i.paddq().exe(s, ctr0, ctr0, inc);
        i.paddq().exe(s, ctr1, ctr1, inc);
        i.paddq().exe(s, ctr2, ctr2, inc);
        i.paddq().exe(s, ctr3, ctr3, inc);
    }
    return 0;
}


}  // namespace detail


inline int xctr_polyvalx4_aes128(Scheduler& s, Isa& i, int num_blocks) {
    return detail::xctr_polyvalx4_core(s, i, 10, num_blocks);
}
inline int xctr_polyvalx4_aes256(Scheduler& s, Isa& i, int num_blocks) {
    return detail::xctr_polyvalx4_core(s, i, 14, num_blocks);
}


}  // namespace algo::xctr_polyval