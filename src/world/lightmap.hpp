#pragma once

#include "world_data.hpp"
#ifndef WORLD_LIGHTMAP_HPP_
#define WORLD_LIGHTMAP_HPP_

#include <cstdint>
#include <thread>
#include <atomic>
#include <memory>
#include <SGE/math/rect.hpp>

#include "../types/tile_pos.hpp"

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

    void init_area(const WorldData& world, const sge::IRect& area, glm::ivec2 tile_offset = {0, 0}) {
        ZoneScoped;

        using Constants::SUBDIVISION;

    #ifndef SGE_DEBUG
        #pragma omp parallel for collapse(2)
    #endif
        for (int y = area.min.y; y < area.max.y; ++y) {
            for (int x = area.min.x; x < area.max.x; ++x) {
                const TilePos color_pos = TilePos(x, y);
                const TilePos tile_pos = tile_offset + color_pos / SUBDIVISION;

                set_mask(color_pos, world.solid_block_exists(tile_pos));

                std::optional<glm::vec3> light = block_light(world.get_block_type(tile_pos));
                if (light.has_value()) {
                    set_color(color_pos, light.value());
                    continue;
                }

                if (tile_offset.y * SUBDIVISION + y >= world.layers.underground * SUBDIVISION) {
                    set_color(color_pos, glm::vec3(0.0f));
                    continue;
                }

                if (tile_pos.x < world.playable_area.min.x || tile_pos.x > world.playable_area.max.x - 1 || world.solid_block_exists(tile_pos) || world.wall_exists(tile_pos)) {
                    set_color(color_pos, glm::vec3(0.0f));
                } else {
                    set_color(color_pos, glm::vec3(1.0f));
                }
            }
        }
    }

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

    uint32_t blur_until_black(int start, int stride, glm::vec3& prev_light, float& prev_decay) {
        int index = start;
        uint32_t i = 0;
        while (i < Constants::LIGHT_AIR_DECAY_STEPS) {
            const glm::vec3 this_light = get_color(index);

            const bool x_end = prev_light.x <= this_light.x;
            const bool y_end = prev_light.y <= this_light.y;
            const bool z_end = prev_light.z <= this_light.z;

            if (x_end && y_end && z_end) {
                break;
            }

            blur(index, prev_light, prev_decay);

            index += stride;
            ++i;
        }

        return i;
    }

    void blur_horizontal(const sge::IRect& area) {
        for (int y = area.min.y; y < area.max.y; ++y) {
            glm::vec3 prev_light = get_color({area.min.x, y});
            float prev_decay = Constants::LightDecay(get_mask({area.min.x - 1, y}));

            glm::vec3 prev_light2 = get_color({area.max.x - 1, y});
            float prev_decay2 = Constants::LightDecay(get_mask({area.max.x, y}));

            blur_line(y * width + area.min.x, y * width + (area.max.x - 1), 1, prev_light, prev_decay, prev_light2, prev_decay2);
        }
    }

    void blur_vertical(const sge::IRect& area) {
        for (int x = area.min.x; x < area.max.x; ++x) {
            glm::vec3 prev_light = get_color({x, area.min.y});
            float prev_decay = Constants::LightDecay(get_mask({x, area.min.y - 1}));

            glm::vec3 prev_light2 = get_color({x, area.max.y - 1});
            float prev_decay2 = Constants::LightDecay(get_mask({x, area.max.y}));

            blur_line(area.min.y * width + x, (area.max.y - 1) * width + x, width, prev_light, prev_decay, prev_light2, prev_decay2);
        }
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

struct LightMapTaskResult {
    Color* data;
    LightMask* mask;
    int width;
    int height;
    int offset_x = 0;
    int offset_y = 0;
    bool is_complete = false;
};

struct LightMapTask {
    std::thread t;
    std::shared_ptr<std::atomic<LightMapTaskResult>> result;

    LightMapTask(std::thread t, std::shared_ptr<std::atomic<LightMapTaskResult>> is_complete) :
        t(std::move(t)),
        result(std::move(is_complete)) {}

    LightMapTask(const LightMapTask&) = delete;
    LightMapTask& operator=(const LightMapTask&) = delete;

    LightMapTask(LightMapTask&& other) noexcept {
        result = std::move(other.result);
        t.swap(other.t);
    }

    LightMapTask& operator=(LightMapTask&& other) noexcept {
        result = std::move(other.result);
        t.swap(other.t);
        return *this;
    }

    ~LightMapTask() {
        if (t.joinable()) t.join();
    }
};

#endif