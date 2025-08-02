#ifndef TYPES_NEIGHBORS
#define TYPES_NEIGHBORS

#pragma once

#include <optional>

template <class T>
struct Neighbors {
    std::optional<T> top;
    std::optional<T> bottom;
    std::optional<T> left;
    std::optional<T> right;
    std::optional<T> top_left;
    std::optional<T> top_right;
    std::optional<T> bottom_left;
    std::optional<T> bottom_right;

    [[nodiscard]]
    inline bool any_exists() const noexcept {
        return top.has_value() || bottom.has_value() || left.has_value() || 
            right.has_value() || top_left.has_value() || top_right.has_value() || 
            bottom_left.has_value() || bottom_right.has_value();
    }

    [[nodiscard]]
    inline bool any_not_exists() const noexcept {
        return !top.has_value() || !bottom.has_value() || !left.has_value() || 
            !right.has_value() || !top_left.has_value() || !top_right.has_value() || 
            !bottom_left.has_value() || !bottom_right.has_value();
    }
};

#endif