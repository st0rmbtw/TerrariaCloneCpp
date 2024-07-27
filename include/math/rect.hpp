#pragma once

#ifndef TERRARIA_MATH_RECT_HPP
#define TERRARIA_MATH_RECT_HPP

#include <glm/glm.hpp>

namespace math {

template <class _Type>
struct _Rect {
private:
    using _Glm = glm::vec<2, _Type>;

public:
    _Glm min;
    _Glm max;

    _Rect() :
        min(0.0f),
        max(0.0f) {}

    _Rect(_Glm min, _Glm max) :
        min(min),
        max(max) {}
    
    inline constexpr static _Rect from_corners(_Glm p1, _Glm p2) noexcept {
        return _Rect(glm::min(p1, p2), glm::max(p1, p2));
    }

    inline constexpr static _Rect from_top_left(_Glm origin, _Glm size) noexcept {
        return _Rect(origin, origin + size);
    }

    inline constexpr static _Rect from_center_size(_Glm origin, _Glm size) noexcept {
        const _Glm half_size = size * static_cast<_Type>(2);
        return _Rect::from_center_half_size(origin, half_size);
    }

    inline constexpr static _Rect from_center_half_size(_Glm origin, _Glm half_size) noexcept {
        return _Rect(origin - half_size, origin + half_size);
    }

    inline constexpr _Type width() const noexcept { return this->max.x - this->min.x; }
    inline constexpr _Type height() const noexcept { return this->max.y - this->min.y; }
    inline constexpr _Type half_width() const noexcept { return this->width() / static_cast<_Type>(2); }
    inline constexpr _Type half_height() const noexcept { return this->height() / static_cast<_Type>(2); }

    inline constexpr _Glm center() const noexcept { return (this->min + this->max) / static_cast<_Type>(2); }
    inline constexpr _Glm size() const noexcept { return _Glm(this->width(), this->height()); }
    inline constexpr _Glm half_size() const noexcept { return _Glm(this->half_width(), this->half_height()); }

    inline constexpr _Type left(void) const noexcept { return this->min.x; }
    inline constexpr _Type right(void) const noexcept { return this->max.x; }
    inline constexpr _Type bottom(void) const noexcept { return this->min.y; }
    inline constexpr _Type top(void) const noexcept { return this->max.y; }

    inline constexpr _Rect<_Type> clamp(_Glm min, _Glm max) const {
        return _Rect::from_corners(glm::max(this->min, min), glm::min(this->max, max));
    }

    inline constexpr bool contains(const _Glm& point) const noexcept {
        return (
            point.x >= this->min.x &&
            point.y >= this->min.y &&
            point.x <= this->max.x &&
            point.y <= this->max.y
        );
    }

    inline bool intersects(const _Rect& other) const noexcept {
        return (
            this->left() < other.right() &&
            this->right() > other.left() &&
            this->top() > other.bottom() &&
            this->bottom() < other.top()
        );
    }

    _Rect<_Type> operator/(const _Rect<_Type> &rhs) const noexcept {
        return from_corners(this->min * rhs.min, this->max * rhs.max);
    }

    _Rect<_Type> operator/(const _Type rhs) const noexcept {
        return from_corners(this->min / rhs, this->max / rhs);
    }

    _Rect<_Type> operator*(const _Rect<_Type> &rhs) const noexcept {
        return from_corners(this->min * rhs, this->max * rhs);
    }

    _Rect<_Type> operator*(const _Type rhs) const noexcept {
        return from_corners(this->min * rhs, this->max * rhs);
    }

    _Rect<_Type> operator+(const _Rect<_Type> &rhs) const noexcept {
        return from_corners(this->min + rhs.min, this->max + rhs.max);
    }

    _Rect<_Type> operator+(const _Type rhs) const noexcept {
        return from_corners(this->min + rhs, this->max + rhs);
    }

    _Rect<_Type> operator-(const _Rect<_Type> &rhs) const noexcept {
        return from_corners(this->min - rhs.min, this->max - rhs.max);
    }

    _Rect<_Type> operator-(const _Type rhs) const noexcept {
        return from_corners(this->min - rhs, this->max - rhs);
    }
};

using Rect = _Rect<glm::float32>;
using URect = _Rect<glm::uint32>;
using IRect = _Rect<glm::int32>;

}

#endif