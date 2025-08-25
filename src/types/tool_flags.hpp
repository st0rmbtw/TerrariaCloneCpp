#pragma once

#ifndef TYPES_TOOL_FLAGS_HPP_
#define TYPES_TOOL_FLAGS_HPP_

#include <cstdint>

namespace ToolFlags {
    enum : uint8_t {
        None = 0,
        Axe = 1 << 0,
        Pickaxe = 1 << 1,
        Hammer = 1 << 2,
        Hand = 1 << 3
    };

    constexpr uint8_t Any = 0xFF;
}

#endif