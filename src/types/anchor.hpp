#pragma once

#ifndef TYPES_ANCHOR_HPP_
#define TYPES_ANCHOR_HPP_

#include <cstdint>

namespace AnchorType {
    using Type = uint8_t;
    enum : Type { 
        None = 0,
        SolidSide = 1 << 0,
        SolidTile = 1 << 1,
        Tree = 1 << 2
    };

    static constexpr Type Default = SolidTile | SolidSide;
}

struct AnchorData {
    AnchorType::Type left = AnchorType::Default;
    AnchorType::Type right = AnchorType::Default;
    AnchorType::Type bottom = AnchorType::Default;
    AnchorType::Type top = AnchorType::Default;
    bool wall = true;
};

#endif