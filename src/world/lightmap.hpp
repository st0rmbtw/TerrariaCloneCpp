#ifndef WORLD_LIGHTMAP_HPP
#define WORLD_LIGHTMAP_HPP

#include <cstdint>
#include <thread>
#include <atomic>
#include <memory>

#include "../types/tile_pos.hpp"
#include "../constants.hpp"

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    Color() noexcept = default;

    explicit Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept :
        r(r), g(g), b(b), a(a) {}

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

    LightMap(int tiles_width, int tiles_height) {
        using Constants::SUBDIVISION;
        width = tiles_width * SUBDIVISION;
        height = tiles_height * SUBDIVISION;
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