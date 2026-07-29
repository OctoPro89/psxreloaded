#include "BlockCache.h"

namespace dynarec
{
    CompiledBlock* BlockCache::Lookup(u32 pc)
    {
        auto it = m_blocks.find(pc);

        if (it == m_blocks.end())
            return nullptr;

        return &it->second;
    }

    CompiledBlock* BlockCache::Insert(CompiledBlock&& block)
    {
        auto [it, inserted] =
            m_blocks.emplace(block.startPC, std::move(block));

        return &it->second;
    }

    void BlockCache::Clear()
    {
        m_blocks.clear();
    }
}