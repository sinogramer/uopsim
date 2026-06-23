#include "cpus/zen4.hpp"
#include "scheduler.hpp"
#include "instruction.hpp"

#define BUFFER_SIZE (1024 * 1024)

instruction pxor   ({{ uops(1, 0b1111, "  pxor   ") }}); 
instruction pshufd ({{ uops(1, 0b1110, "  pshufd ") }});
instruction pslldq ({{ uops(2, 0b0110, "  pslldq ") }});
instruction psrldq ({{ uops(2, 0b0110, "  psrldq ") }});
instruction pclmulqdq({
    { uops(1, 0b0011, "pclmulqd1") },
    { uops(2, 0b1110, "pclmulqd2") },
    { uops(1, 0b0110, "pclmulqd3") }
}, 1, "", 2);

void polyreduce128(Scheduler& s,
                   argument& ppl_reduced, argument& ctx, argument& x){
    argument x0, x1, y0, y1, y2;
    pclmulqdq.exe(s, x0, x, ctx);
    pshufd   .exe(s, y0, x);
    pxor     .exe(s, y1, y0, x0);
    pclmulqdq.exe(s, x1, y1, ctx);
    pshufd   .exe(s, y2, y1);
    pxor     .exe(s, ppl_reduced, y2, x1);
}

void polydot128(Scheduler& s,
                argument& result, argument& ctx, argument& a, argument& b){
    argument pp00, pp11, pp10, pp01;
    pclmulqdq.exe(s, pp00, a, b);
    pclmulqdq.exe(s, pp11, a, b);
    pclmulqdq.exe(s, pp10, a, b);
    pclmulqdq.exe(s, pp01, a, b);

    argument ppmid, ppmid_ls, ppmid_rs, ppupper, pplower, ppl_reduced;
    pxor  .exe(s, ppmid, pp10, pp01);
    pslldq.exe(s, ppmid_ls, ppmid);
    psrldq.exe(s, ppmid_rs, ppmid);
    pxor  .exe(s, ppupper, ppmid_rs, pp11);
    pxor  .exe(s, pplower, ppmid_ls, pp00);
    polyreduce128(s, ppl_reduced, ctx, pplower);
    pxor  .exe(s, result, ppupper, ppl_reduced);
}

void schoolbook_add128(Scheduler& s,
        argument& reg, argument& htbl_reg,
        argument& tmps0, argument& tmps1, argument& tmps2, argument& tmps3){
    pclmulqdq.exe(s, tmps3, reg, htbl_reg);
    pxor     .exe(s, tmps2, tmps2, tmps3);
    pclmulqdq.exe(s, tmps3, reg, htbl_reg);
    pxor     .exe(s, tmps0, tmps0, tmps3);
    pclmulqdq.exe(s, tmps3, reg, htbl_reg);
    pxor     .exe(s, tmps1, tmps1, tmps3);
    pclmulqdq.exe(s, tmps3, reg, htbl_reg);
    pxor     .exe(s, tmps2, tmps2, tmps3);
}

void schoolbook_initialadd128(Scheduler& s,
        argument& reg, argument& htbl_reg,
        argument& tmps0, argument& tmps1, argument& tmps2, argument& tmps3){
    pclmulqdq.exe(s, tmps2, reg, htbl_reg);
    pclmulqdq.exe(s, tmps0, reg, htbl_reg);
    pclmulqdq.exe(s, tmps3, reg, htbl_reg);
    pclmulqdq.exe(s, tmps1, reg, htbl_reg);
    pxor     .exe(s, tmps2, tmps2, tmps3);
}

int polyvalx8_func(Scheduler& s, int num_blocks){
    argument ctx, X, Y, Z;
    argument data0, data1, data2, data3, data4, data5, data6, data7;
    argument ctx_htbl0, ctx_htbl1, ctx_htbl2, ctx_htbl3;
    argument ctx_htbl4, ctx_htbl5, ctx_htbl6, ctx_htbl7;
    argument tmps0, tmps1, tmps2, tmps3;

    for (int i = 0; i < num_blocks/8; i++){
        polyreduce128(s, Y, ctx, Y);
        pxor.exe(s, Z, X, Y);
        pxor.exe(s, Z, Z, data0);

        schoolbook_initialadd128(s, data7, ctx_htbl0, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, data6, ctx_htbl1, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, data5, ctx_htbl2, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, data4, ctx_htbl3, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, data3, ctx_htbl4, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, data2, ctx_htbl5, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, data1, ctx_htbl6, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, Z,     ctx_htbl7, tmps0, tmps1, tmps2, tmps3);

        psrldq.exe(s, tmps3, tmps2);
        pslldq.exe(s, tmps2, tmps2);
        pxor  .exe(s, X, tmps3, tmps1);
        pxor  .exe(s, Y, tmps0, tmps2);
    }
    polyreduce128(s, Y, ctx, Y);
    pxor.exe(s, Z, X, Y);
    return 0;
}

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    // Zen4 で測定
    {
        Scheduler sched(ZEN4);
        polyvalx8_func(sched, num_blocks);
        std::printf("CPU: %s, total cycles = %d, cycles/byte = %.3f\n",
                    sched.cpu().name.c_str(),
                    sched.total_cycles(),
                    (double)sched.total_cycles() / BUFFER_SIZE);
    }

    // Zen5 で測定（同じ関数を再利用！）
    // {
    //     Scheduler sched(ZEN5);
    //     polyvalx8_func(sched, num_blocks);
    //     std::printf("CPU: %s, total cycles = %d, cycles/byte = %.3f\n",
    //                 sched.cpu().name.c_str(),
    //                 sched.total_cycles(),
    //                 (double)sched.total_cycles() / BUFFER_SIZE);
    // }
    return 0;
}