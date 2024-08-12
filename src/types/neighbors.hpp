#ifndef TERRARIA_TYPES_NEIGHBORS
#define TERRARIA_TYPES_NEIGHBORS

#pragma once

#include "../optional.hpp"

template <class T>
struct Neighbors {
    tl::optional<T> top;
    tl::optional<T> bottom;
    tl::optional<T> left;
    tl::optional<T> right;
    tl::optional<T> top_left;
    tl::optional<T> top_right;
    tl::optional<T> bottom_left;
    tl::optional<T> bottom_right;

    [[nodiscard]]
    inline bool any_not_exists() const {
        return top.is_none() || bottom.is_none() || left.is_none() || 
            right.is_none() || top_left.is_none() || top_right.is_none() || 
            bottom_left.is_none() || bottom_right.is_none();
    }

    [[nodiscard]]
    inline bool any_exists() const {
        return top.is_some() || bottom.is_some() || left.is_some() || 
            right.is_some() || top_left.is_some() || top_right.is_some() || 
            bottom_left.is_some() || bottom_right.is_some();
    }
};

#endif