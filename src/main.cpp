#include "cpus/zen4.hpp"
#include "cpus/zen5.hpp"
#include "scheduler.hpp"
#include "instruction.hpp"
#include "isa.hpp"
#include "isa/zen4_isa.hpp"
#include "isa/zen5_isa.hpp"

#define BUFFER_SIZE (1024 * 1024)

// ---- アルゴリズム実装：CPU非依存 ----

void polyreduce128(Scheduler& s, Isa& i,
                   argument& ppl_reduced, argument& ctx, argument& x){
    argument x0, x1, y0, y1, y2;
    i.pclmulqdq().exe(s, x0, x, ctx);
    i.pshufd()   .exe(s, y0, x);
    i.pxor()     .exe(s, y1, y0, x0);
    i.pclmulqdq().exe(s, x1, y1, ctx);
    i.pshufd()   .exe(s, y2, y1);
    i.pxor()     .exe(s, ppl_reduced, y2, x1);
}

void polydot128(Scheduler& s, Isa& i,
                argument& result, argument& ctx, argument& a, argument& b){
    argument pp00, pp11, pp10, pp01;
    i.pclmulqdq().exe(s, pp00, a, b);
    i.pclmulqdq().exe(s, pp11, a, b);
    i.pclmulqdq().exe(s, pp10, a, b);
    i.pclmulqdq().exe(s, pp01, a, b);

    argument ppmid, ppmid_ls, ppmid_rs, ppupper, pplower, ppl_reduced;
    i.pxor()  .exe(s, ppmid, pp10, pp01);
    i.pslldq().exe(s, ppmid_ls, ppmid);
    i.psrldq().exe(s, ppmid_rs, ppmid);
    i.pxor()  .exe(s, ppupper, ppmid_rs, pp11);
    i.pxor()  .exe(s, pplower, ppmid_ls, pp00);
    polyreduce128(s, i, ppl_reduced, ctx, pplower);
    i.pxor()  .exe(s, result, ppupper, ppl_reduced);
}

void schoolbook_add128(Scheduler& s, Isa& i,
        argument& reg, argument& htbl_reg,
        argument& tmps0, argument& tmps1, argument& tmps2, argument& tmps3){
    i.pclmulqdq().exe(s, tmps3, reg, htbl_reg);
    i.pxor()     .exe(s, tmps2, tmps2, tmps3);
    i.pclmulqdq().exe(s, tmps3, reg, htbl_reg);
    i.pxor()     .exe(s, tmps0, tmps0, tmps3);
    i.pclmulqdq().exe(s, tmps3, reg, htbl_reg);
    i.pxor()     .exe(s, tmps1, tmps1, tmps3);
    i.pclmulqdq().exe(s, tmps3, reg, htbl_reg);
    i.pxor()     .exe(s, tmps2, tmps2, tmps3);
}

void schoolbook_initialadd128(Scheduler& s, Isa& i,
        argument& reg, argument& htbl_reg,
        argument& tmps0, argument& tmps1, argument& tmps2, argument& tmps3){
    i.pclmulqdq().exe(s, tmps2, reg, htbl_reg);
    i.pclmulqdq().exe(s, tmps0, reg, htbl_reg);
    i.pclmulqdq().exe(s, tmps3, reg, htbl_reg);
    i.pclmulqdq().exe(s, tmps1, reg, htbl_reg);
    i.pxor()     .exe(s, tmps2, tmps2, tmps3);
}

int polyvalx8_func(Scheduler& s, Isa& i, int num_blocks){
    argument ctx, X, Y, Z;
    argument data0, data1, data2, data3, data4, data5, data6, data7;
    argument ctx_htbl0, ctx_htbl1, ctx_htbl2, ctx_htbl3;
    argument ctx_htbl4, ctx_htbl5, ctx_htbl6, ctx_htbl7;
    argument tmps0, tmps1, tmps2, tmps3;

    for (int b = 0; b < num_blocks/8; b++){
        polyreduce128(s, i, Y, ctx, Y);
        i.pxor().exe(s, Z, X, Y);
        i.pxor().exe(s, Z, Z, data0);

        schoolbook_initialadd128(s, i, data7, ctx_htbl0, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data6, ctx_htbl1, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data5, ctx_htbl2, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data4, ctx_htbl3, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data3, ctx_htbl4, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data2, ctx_htbl5, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, data1, ctx_htbl6, tmps0, tmps1, tmps2, tmps3);
        schoolbook_add128       (s, i, Z,     ctx_htbl7, tmps0, tmps1, tmps2, tmps3);

        i.psrldq().exe(s, tmps3, tmps2);
        i.pslldq().exe(s, tmps2, tmps2);
        i.pxor()  .exe(s, X, tmps3, tmps1);
        i.pxor()  .exe(s, Y, tmps0, tmps2);
    }
    polyreduce128(s, i, Y, ctx, Y);
    i.pxor().exe(s, Z, X, Y);
    return 0;
}

// ---- main ----

int main() {
    int num_blocks = BUFFER_SIZE / 16;

    // Zen4
    {
        Scheduler sched(ZEN4);
        polyvalx8_func(sched, isa::zen4::instance(), num_blocks);
        std::printf("%-6s : %8d cycles, %.4f c/B\n",
                    sched.cpu().name.c_str(),
                    sched.total_cycles(),
                    (double)sched.total_cycles() / BUFFER_SIZE);
    }

    // Zen5
    {
        Scheduler sched(ZEN5);
        polyvalx8_func(sched, isa::zen5::instance(), num_blocks);
        std::printf("%-6s : %8d cycles, %.4f c/B\n",
                    sched.cpu().name.c_str(),
                    sched.total_cycles(),
                    (double)sched.total_cycles() / BUFFER_SIZE);
    }
    return 0;
}