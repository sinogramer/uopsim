#pragma once
#include "isa.hpp"

namespace isa::zen4 {

class Zen4Isa : public Isa {
public:
    instruction& pxor()      override { return pxor_; }
    instruction& pshufd()    override { return pshufd_; }
    instruction& pslldq()    override { return pslldq_; }
    instruction& psrldq()    override { return psrldq_; }
    instruction& pclmulqdq() override { return pclmulqdq_; }

private:
    instruction pxor_   {{{ uops(1, 0b1111, "  pxor   ") }}};
    instruction pshufd_ {{{ uops(1, 0b1110, "  pshufd ") }}};
    instruction pslldq_ {{{ uops(2, 0b0110, "  pslldq ") }}};
    instruction psrldq_ {{{ uops(2, 0b0110, "  psrldq ") }}};
    instruction pclmulqdq_{
        {
            { uops(1, 0b0011, "pclmulqd1") },
            { uops(2, 0b1110, "pclmulqd2") },
            { uops(1, 0b0110, "pclmulqd3") }
        },
        1, "", 2
    };
};

// シングルトン的にアクセスしたい場合
inline Zen4Isa& instance() {
    static Zen4Isa inst;
    return inst;
}

}  // namespace isa::zen4