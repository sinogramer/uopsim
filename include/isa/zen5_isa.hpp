#pragma once
#include "isa.hpp"

namespace isa::zen5 {

class Zen5Isa : public Isa {
public:
    instruction& pxor()      override { return pxor_; }
    instruction& pshufd()    override { return pshufd_; }
    instruction& pslldq()    override { return pslldq_; }
    instruction& psrldq()    override { return psrldq_; }
    instruction& pclmulqdq() override { return pclmulqdq_; }

private:
    // Zen5 の仕様に合わせて要調整
    instruction pxor_   {{{ uops(1, 0b1111, "  pxor   ") }}};
    instruction pshufd_ {{{ uops(1, 0b1110, "  pshufd ") }}};
    instruction pslldq_ {{{ uops(1, 0b0110, "  pslldq ") }}};  // Zen5は1cycle?
    instruction psrldq_ {{{ uops(1, 0b0110, "  psrldq ") }}};
    instruction pclmulqdq_{
        {
            { uops(1, 0b0011, "pclmulqd1") },
            { uops(1, 0b1110, "pclmulqd2") },  // 2 → 1 など、Zen5の改善を反映
            { uops(1, 0b0110, "pclmulqd3") }
        },
        1, "", 1
    };
};

inline Zen5Isa& instance() {
    static Zen5Isa inst;
    return inst;
}

}  // namespace isa::zen5