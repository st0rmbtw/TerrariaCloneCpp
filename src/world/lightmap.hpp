#pragma once

#ifndef WORLD_LIGHTMAP_HPP_
#define WORLD_LIGHTMAP_HPP_

#include <cstdint>
#include <SGE/math/rect.hpp>
#include <SGE/utils/containers/heaparray.hpp>

#include "../constants.hpp"

struct WorldData;

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Color() noexcept = default;

    explicit Color(uint8_t r, uint8_t g, uint8_t b) noexcept :
        r(r), g(g), b(b) {}

    explicit Color(const glm::vec3& c) noexcept :
        r(c.r * 255.0f), g(c.g * 255.0f), b(c.b * 255.0f) {}

    [[nodiscard]]
    inline glm::vec3 as_vec3() const noexcept {
        return glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
    }
};

using LightPos = glm::i64vec2;

using LightMask = bool;

struct LightMap {
    sge::HeapArray<Color> colors;
    sge::HeapArray<LightMask> masks;
    int64_t width = 0;
    int64_t height = 0;

    LightMap() noexcept = default;
    
    LightMap(glm::ivec2 size) : LightMap(size.x, size.y) {}
    
    LightMap(uint32_t width, uint32_t height) : width(width), height(height) {
        colors = sge::HeapArray<Color>(width * height);
        masks = sge::HeapArray<LightMask>(width * height);
    }

    LightMap(const LightMap& other) = delete;
    LightMap& operator=(const LightMap &other) noexcept = delete;

    LightMap(LightMap&& other) noexcept {
        operator=(std::move(other));
    }

    LightMap& operator=(LightMap&& other) noexcept {
        colors = std::move(other.colors);
        masks = std::move(other.masks);
        width = other.width;
        height = other.height;
        return *this;
    }

    [[nodiscard]]
    inline glm::vec3 get_color(int64_t index) const noexcept {
        if (!(index >= 0 && index < width * height)) {
            return glm::vec3(0.0f);
        }

        return colors[index].as_vec3();
    }

    [[nodiscard]]
    inline glm::vec3 get_color(LightPos pos) const noexcept {
        return get_color(pos.y * width + pos.x);
    }

    inline void set_color(size_t index, const glm::vec3& color) noexcept {
        colors[index] = Color(color);
    }

    inline void set_color(LightPos pos, const glm::vec3& color) noexcept {
        set_color(pos.y * width + pos.x, color);
    }

    [[nodiscard]]
    inline LightMask get_mask(int64_t index) const noexcept {
        if (!(index >= 0 && index < width * height)) {
            return false;
        }

        return masks[index];
    }

    [[nodiscard]]
    inline LightMask get_mask(LightPos pos) const noexcept {
        return get_mask(pos.y * width + pos.x);
    }

    inline void set_mask(size_t index, LightMask mask) noexcept {
        masks[index] = mask;
    }

    inline void set_mask(LightPos pos, LightMask mask) noexcept {
        set_mask(pos.y * width + pos.x, mask);
    }

    void init_area(const WorldData& world, const sge::rect<int64_t>& area, glm::ivec2 tile_offset = {0, 0});

    void blur(int64_t index, glm::vec3& prev_light, float& prev_decay) {
        using Constants::LIGHT_EPSILON;

        glm::vec3 this_light = get_color(index);

        prev_light.r = prev_light.r < LIGHT_EPSILON ? 0.0f : prev_light.r;
        prev_light.g = prev_light.g < LIGHT_EPSILON ? 0.0f : prev_light.g;
        prev_light.b = prev_light.b < LIGHT_EPSILON ? 0.0f : prev_light.b;

        bool update = false;

        if (prev_light.r < this_light.r) {
            prev_light.r = this_light.r;
        } else {
            this_light.r = prev_light.r;
            update = true;
        }

        if (prev_light.g < this_light.g) {
            prev_light.g = this_light.g;
        } else {
            this_light.g = prev_light.g;
            update = true;
        }

        if (prev_light.b < this_light.b) {
            prev_light.b = this_light.b;
        } else {
            this_light.b = prev_light.b;
            update = true;
        }

        if (update) {
            set_color(index, this_light);
        }

        prev_light = prev_light * prev_decay;
        prev_decay = Constants::LightDecay(get_mask(index));
    }

    void blur_line(int64_t start, int64_t end, int64_t stride, glm::vec3& prev_light, float& prev_decay, glm::vec3& prev_light2, float& prev_decay2) {
        using Constants::LIGHT_EPSILON;

        int64_t length = end - start;
        for (int64_t index = 0; index < length; index += stride) {
            blur(start + index, prev_light, prev_decay);
            blur(end - index, prev_light2, prev_decay2);
        }
    }

    uint32_t blur_until_black(int64_t start, int64_t stride, glm::vec3& prev_light, float& prev_decay);

    void blur_horizontal(const sge::rect<int64_t>& area, const LightMap& reference, glm::ivec2 reference_offset = {});

    void blur_horizontal(const sge::rect<int64_t>& area) {
        blur_horizontal(area, *this, glm::ivec2(0));
    }

    void blur_vertical(const sge::rect<int64_t>& area, const LightMap& reference, glm::ivec2 reference_offset = {});

    void blur_vertical(const sge::rect<int64_t>& area) {
        blur_vertical(area, *this, glm::ivec2(0));
    }
};

#endif