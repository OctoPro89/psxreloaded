#pragma once

#include <unordered_map>
#include "Dynarec.h"

namespace dynarec
{
    class BlockCache
    {
    public:
        CompiledBlock* Lookup(u32 pc);
        CompiledBlock* Insert(CompiledBlock&& block);
        void Clear();

    private:
        std::unordered_map<u32, CompiledBlock> m_blocks;
    };
}