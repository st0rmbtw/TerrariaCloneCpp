#pragma once

#ifndef WORLD_LIGHTMAP_HPP_
#define WORLD_LIGHTMAP_HPP_

#include <cstdint>
#include <SGE/math/rect.hpp>

#include "../types/tile_pos.hpp"

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

using LightMask = bool;

struct LightMap {
    Color* colors = nullptr;
    LightMask* masks = nullptr;
    int width = 0;
    int height = 0;

    LightMap() noexcept = default;
    
    LightMap(glm::ivec2 size) : LightMap(size.x, size.y) {}
    
    LightMap(int width, int height) : width(width), height(height) {
        colors = new Color[width * height]();
        masks = new LightMask[width * height]();
    }

    LightMap(const LightMap& other) = delete;
    LightMap& operator=(const LightMap &other) noexcept = delete;

    LightMap(LightMap&& other) noexcept {
        move(other);
    }

    LightMap& operator=(LightMap&& other) noexcept {
        move(other);
        return *this;
    }

    ~LightMap() {
        if (colors != nullptr) delete[] colors;
        if (masks != nullptr) delete[] masks;
        
        colors = nullptr;
        masks = nullptr;
    }

    [[nodiscard]]
    inline glm::vec3 get_color(int index) const noexcept {
        if (!(index >= 0 && index < width * height)) {
            return glm::vec3(0.0f);
        }

        return colors[index].as_vec3();
    }

    [[nodiscard]]
    inline glm::vec3 get_color(TilePos pos) const noexcept {
        return get_color(pos.y * width + pos.x);
    }

    inline void set_color(size_t index, const glm::vec3& color) noexcept {
        colors[index] = Color(color);
    }

    inline void set_color(TilePos pos, const glm::vec3& color) noexcept {
        set_color(pos.y * width + pos.x, color);
    }

    [[nodiscard]]
    inline LightMask get_mask(int index) const noexcept {
        if (!(index >= 0 && index < width * height)) {
            return false;
        }

        return masks[index];
    }

    [[nodiscard]]
    inline LightMask get_mask(TilePos pos) const noexcept {
        return get_mask(pos.y * width + pos.x);
    }

    inline void set_mask(size_t index, LightMask mask) noexcept {
        masks[index] = mask;
    }

    inline void set_mask(TilePos pos, LightMask mask) noexcept {
        set_mask(pos.y * width + pos.x, mask);
    }

    void init_area(const WorldData& world, const sge::IRect& area, glm::ivec2 tile_offset = {0, 0});

    void blur(int index, glm::vec3& prev_light, float& prev_decay) {
        using Constants::LIGHT_EPSILON;

        glm::vec3 this_light = get_color(index);

        prev_light.r = prev_light.r < LIGHT_EPSILON ? 0.0f : prev_light.r;
        prev_light.g = prev_light.g < LIGHT_EPSILON ? 0.0f : prev_light.g;
        prev_light.b = prev_light.b < LIGHT_EPSILON ? 0.0f : prev_light.b;

        if (prev_light.r < this_light.r) {
            prev_light.r = this_light.r;
        } else {
            this_light.r = prev_light.r;
        }

        if (prev_light.g < this_light.g) {
            prev_light.g = this_light.g;
        } else {
            this_light.g = prev_light.g;
        }

        if (prev_light.b < this_light.b) {
            prev_light.b = this_light.b;
        } else {
            this_light.b = prev_light.b;
        }

        set_color(index, this_light);

        prev_light = prev_light * prev_decay;
        prev_decay = Constants::LightDecay(get_mask(index));
    }

    void blur_line(int start, int end, int stride, glm::vec3& prev_light, float& prev_decay, glm::vec3& prev_light2, float& prev_decay2) {
        using Constants::LIGHT_EPSILON;

        int length = end - start;
        for (int index = 0; index < length; index += stride) {
            blur(start + index, prev_light, prev_decay);
            blur(end - index, prev_light2, prev_decay2);
        }
    }

    uint32_t blur_until_black(int start, int stride, glm::vec3& prev_light, float& prev_decay);

    void blur_horizontal(const sge::IRect& area, const LightMap& reference, glm::ivec2 reference_offset);

    void blur_horizontal(const sge::IRect& area) {
        blur_horizontal(area, *this, glm::ivec2(0));
    }

    void blur_vertical(const sge::IRect& area, const LightMap& reference, glm::ivec2 reference_offset);

    void blur_vertical(const sge::IRect& area) {
        blur_vertical(area, *this, glm::ivec2(0));
    }

private:
    inline void move(LightMap& from) {
        this->colors = from.colors;
        this->masks = from.masks;
        this->width = from.width;
        this->height = from.height;

        from.colors = nullptr;
        from.masks = nullptr;
    }
};

#endif