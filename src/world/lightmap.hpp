#ifndef WORLD_LIGHTMAP_HPP
#define WORLD_LIGHTMAP_HPP

#include <stdint.h>
#include <thread>
#include <atomic>

#include "../types/tile_pos.hpp"
#include "../constants.hpp"
#include "../defines.hpp"

using Color = uint8_t;

using LightMask = bool;

struct LightMap {
    Color* colors = nullptr;
    LightMask* masks = nullptr;
    int width = 0;
    int height = 0;

    LightMap() = default;

    LightMap(int tiles_width, int tiles_height) {
        using Constants::SUBDIVISION;
        width = tiles_width * SUBDIVISION;
        height = tiles_height * SUBDIVISION;
        colors = new Color[width * height]();
        masks = new LightMask[width * height]();
    }

    LightMap(LightMap& other) {
        copy(other);
    }

    LightMap& operator=(LightMap&& other) noexcept {
        copy(other);
        return *this;
    }

    LightMap& operator=(LightMap &other) noexcept {
        copy(other);
        return *this;
    }

    ~LightMap() {
        if (colors) delete[] colors;
        if (masks) delete[] masks;
    }

    [[nodiscard]]
    inline float get_color(int index) const {
        if (!(index >= 0 && index < width * height)) {
            return 0.0f;
        }
        
        return colors[index] / 255.0f;
    }

    [[nodiscard]]
    inline float get_color(TilePos pos) const {
        return get_color(pos.y * width + pos.x);
    }

    inline void set_color(size_t index, float color) {
        colors[index] = static_cast<uint8_t>(color * 255.0f);
    }

    inline void set_color(TilePos pos, float color) {
        set_color(pos.y * width + pos.x, color);
    }

    [[nodiscard]]
    inline LightMask get_mask(int index) const {
        if (!(index >= 0 && index < width * height)) {
            return false;
        }

        return masks[index];
    }

    [[nodiscard]]
    inline LightMask get_mask(TilePos pos) const {
        return get_mask(pos.y * width + pos.x);
    }

    inline void set_mask(size_t index, LightMask mask) {
        masks[index] = mask;
    }

    inline void set_mask(TilePos pos, LightMask mask) {
        set_mask(pos.y * width + pos.x, mask);
    }

private:
    inline void copy(LightMap& from) {
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