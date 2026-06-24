#pragma once
#include "core/isa.hpp"

namespace isa::sr {

class SRIsa : public Isa {
public:
    instruction& pxor()      override { return pxor_; }
    instruction& pshufd()    override { return pshufd_; }
    instruction& pslldq()    override { return pslldq_; }
    instruction& psrldq()    override { return psrldq_; }
    instruction& pclmulqdq() override { return pclmulqdq_; }
    instruction& paddq() override { return paddq_; }
    instruction& aesenc() override { return aesenc_; }
    instruction& aesenclast() override { return aesenclast_; }

private:
    instruction pxor_   {{{ uops(1, 0b0100011, "  pxor   ") }}};
    instruction pshufd_ {{{ uops(1, 0b0100010, "  pshufd ") }}};
    instruction pslldq_ {{{ uops(1, 0b0100010, "  pslldq ") }}};
    instruction psrldq_ {{{ uops(1, 0b0100010, "  psrldq ") }}};
    instruction pclmulqdq_{{{ uops(3, 0b0100000, "pclmulqdq") }}};
    instruction paddq_ {{{ uops(1, 0b0100011, "  paddq  ") }}};
    instruction aesenc_ {{{ uops(3, 0b0000011, "  aesenc ") }}};
    instruction aesenclast_ {{{ uops(3, 0b0000011, "aesenlast") }}};
};

// シングルトン的にアクセスしたい場合
inline SRIsa& instance() {
    static SRIsa inst;
    return inst;
}

}  // namespace isa::sr