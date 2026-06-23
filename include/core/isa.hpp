/*ISAインタフェース*/

#pragma once
#include "core/instruction.hpp"

class Isa {
public:
    virtual ~Isa() = default;

    // 命令ごとに「アクセサ」を持つ
    virtual instruction& pxor()      = 0;
    virtual instruction& pshufd()    = 0;
    virtual instruction& pslldq()    = 0;
    virtual instruction& psrldq()    = 0;
    virtual instruction& pclmulqdq() = 0;

};
