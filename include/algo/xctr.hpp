#pragma once
#include "core/scheduler.hpp"
#include "core/isa.hpp"
#include "core/argument.hpp"

namespace algo::xctr {

// ============================================================
// 1 回の AES 暗号化（汎用）
//   num_rounds = AES-128 なら 10, AES-256 なら 14
// ============================================================
inline void aes_encrypt(Scheduler& s, Isa& i, int num_rounds,
                        argument& out, argument& in, argument& key) {
    // 初回 AddRoundKey
    i.pxor().exe(s, out, in, key);
    // (num_rounds - 1) 回の aesenc
    for (int r = 0; r < num_rounds - 1; ++r) {
        i.aesenc().exe(s, out, out, key);
    }
    // 最終ラウンド
    i.aesenclast().exe(s, out, out, key);
}

// 利便のためのラッパー
inline void aes128_encrypt(Scheduler& s, Isa& i,
                           argument& out, argument& in, argument& key) {
    aes_encrypt(s, i, 10, out, in, key);
}

inline void aes256_encrypt(Scheduler& s, Isa& i,
                           argument& out, argument& in, argument& key) {
    aes_encrypt(s, i, 14, out, in, key);
}

// ============================================================
// xctr 直列版 (1ブロック / ループ) - AES-128 / AES-256
// ============================================================
inline int xctr_aes_x1(Scheduler& s, Isa& i, int num_rounds, int num_blocks) {
    argument S, ctr, inc, key, data;

    for (int b = 0; b < num_blocks; ++b) {
        argument tmp0, tmp1;
        i.pxor() .exe(s, tmp0, ctr, S);
        aes_encrypt(s, i, num_rounds, tmp0, tmp0, key);
        i.pxor() .exe(s, tmp1, tmp0, data);
        i.paddq().exe(s, ctr, ctr, inc);
    }
    return 0;
}

inline int xctr_aes128(Scheduler& s, Isa& i, int num_blocks) {
    return xctr_aes_x1(s, i, 10, num_blocks);
}
inline int xctr_aes256(Scheduler& s, Isa& i, int num_blocks) {
    return xctr_aes_x1(s, i, 14, num_blocks);
}

// ============================================================
// xctr 4並列版
// ============================================================
inline int xctr_aes_x4(Scheduler& s, Isa& i, int num_rounds, int num_blocks) {
    argument S, inc4, key;
    argument ctr0, ctr1, ctr2, ctr3;
    argument data0, data1, data2, data3;
    argument t0, t1, t2, t3;

    for (int b = 0; b < num_blocks / 4; ++b) {
        // --- xorx4_bfix ---
        i.pxor().exe(s, t0, ctr0, S);
        i.pxor().exe(s, t1, ctr1, S);
        i.pxor().exe(s, t2, ctr2, S);
        i.pxor().exe(s, t3, ctr3, S);

        // --- aesx4: 4並列 AES ---
        // 初回 AddRoundKey
        i.pxor().exe(s, t0, t0, key);
        i.pxor().exe(s, t1, t1, key);
        i.pxor().exe(s, t2, t2, key);
        i.pxor().exe(s, t3, t3, key);
        // (num_rounds - 1) ラウンド
        for (int r = 0; r < num_rounds - 1; ++r) {
            i.aesenc().exe(s, t0, t0, key);
            i.aesenc().exe(s, t1, t1, key);
            i.aesenc().exe(s, t2, t2, key);
            i.aesenc().exe(s, t3, t3, key);
        }
        // 最終ラウンド
        i.aesenclast().exe(s, t0, t0, key);
        i.aesenclast().exe(s, t1, t1, key);
        i.aesenclast().exe(s, t2, t2, key);
        i.aesenclast().exe(s, t3, t3, key);

        // --- xorx4_1wise ---
        i.pxor().exe(s, t0, t0, data0);
        i.pxor().exe(s, t1, t1, data1);
        i.pxor().exe(s, t2, t2, data2);
        i.pxor().exe(s, t3, t3, data3);

        // --- addx4_bfix ---
        i.paddq().exe(s, ctr0, ctr0, inc4);
        i.paddq().exe(s, ctr1, ctr1, inc4);
        i.paddq().exe(s, ctr2, ctr2, inc4);
        i.paddq().exe(s, ctr3, ctr3, inc4);
    }
    return 0;
}

inline int xctr_aes128x4(Scheduler& s, Isa& i, int num_blocks) {
    return xctr_aes_x4(s, i, 10, num_blocks);
}
inline int xctr_aes256x4(Scheduler& s, Isa& i, int num_blocks) {
    return xctr_aes_x4(s, i, 14, num_blocks);
}

// ============================================================
// xctr 8並列版
// ============================================================
inline int xctr_aes_x8(Scheduler& s, Isa& i, int num_rounds, int num_blocks) {
    argument S, inc8, key;
    argument ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    argument data0, data1, data2, data3, data4, data5, data6, data7;
    argument t0, t1, t2, t3, t4, t5, t6, t7;

    for (int b = 0; b < num_blocks / 8; ++b) {
        // --- xorx8_bfix ---
        i.pxor().exe(s, t0, ctr0, S);
        i.pxor().exe(s, t1, ctr1, S);
        i.pxor().exe(s, t2, ctr2, S);
        i.pxor().exe(s, t3, ctr3, S);
        i.pxor().exe(s, t4, ctr4, S);
        i.pxor().exe(s, t5, ctr5, S);
        i.pxor().exe(s, t6, ctr6, S);
        i.pxor().exe(s, t7, ctr7, S);

        // --- aesx8 ---
        // 初回 AddRoundKey
        i.pxor().exe(s, t0, t0, key);
        i.pxor().exe(s, t1, t1, key);
        i.pxor().exe(s, t2, t2, key);
        i.pxor().exe(s, t3, t3, key);
        i.pxor().exe(s, t4, t4, key);
        i.pxor().exe(s, t5, t5, key);
        i.pxor().exe(s, t6, t6, key);
        i.pxor().exe(s, t7, t7, key);
        // (num_rounds - 1) ラウンド
        for (int r = 0; r < num_rounds - 1; ++r) {
            i.aesenc().exe(s, t0, t0, key);
            i.aesenc().exe(s, t1, t1, key);
            i.aesenc().exe(s, t2, t2, key);
            i.aesenc().exe(s, t3, t3, key);
            i.aesenc().exe(s, t4, t4, key);
            i.aesenc().exe(s, t5, t5, key);
            i.aesenc().exe(s, t6, t6, key);
            i.aesenc().exe(s, t7, t7, key);
        }
        // 最終ラウンド
        i.aesenclast().exe(s, t0, t0, key);
        i.aesenclast().exe(s, t1, t1, key);
        i.aesenclast().exe(s, t2, t2, key);
        i.aesenclast().exe(s, t3, t3, key);
        i.aesenclast().exe(s, t4, t4, key);
        i.aesenclast().exe(s, t5, t5, key);
        i.aesenclast().exe(s, t6, t6, key);
        i.aesenclast().exe(s, t7, t7, key);

        // --- xorx8_1wise ---
        i.pxor().exe(s, t0, t0, data0);
        i.pxor().exe(s, t1, t1, data1);
        i.pxor().exe(s, t2, t2, data2);
        i.pxor().exe(s, t3, t3, data3);
        i.pxor().exe(s, t4, t4, data4);
        i.pxor().exe(s, t5, t5, data5);
        i.pxor().exe(s, t6, t6, data6);
        i.pxor().exe(s, t7, t7, data7);

        // --- addx8_bfix ---
        i.paddq().exe(s, ctr0, ctr0, inc8);
        i.paddq().exe(s, ctr1, ctr1, inc8);
        i.paddq().exe(s, ctr2, ctr2, inc8);
        i.paddq().exe(s, ctr3, ctr3, inc8);
        i.paddq().exe(s, ctr4, ctr4, inc8);
        i.paddq().exe(s, ctr5, ctr5, inc8);
        i.paddq().exe(s, ctr6, ctr6, inc8);
        i.paddq().exe(s, ctr7, ctr7, inc8);
    }
    return 0;
}

inline int xctr_aes128x8(Scheduler& s, Isa& i, int num_blocks) {
    return xctr_aes_x8(s, i, 10, num_blocks);
}
inline int xctr_aes256x8(Scheduler& s, Isa& i, int num_blocks) {
    return xctr_aes_x8(s, i, 14, num_blocks);
}

}  // namespace algo::xctr