#ifndef TERRARIA_WORLD_LIGHTMAP_HPP
#define TERRARIA_WORLD_LIGHTMAP_HPP

#include <stdint.h>
#include <thread>

#include "../types/tile_pos.hpp"
#include "../constants.hpp"

struct Color {
    uint8_t r, g, b, a;

    Color() : r(0), g(0), b(0), a(0xFF) {}

    explicit Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b), a(0xFF) {}
};

struct LightMap {
    Color* data = nullptr;
    int width = 0;
    int height = 0;

    LightMap() = default;

    LightMap(int tiles_width, int tiles_height) {
        using Constants::SUBDIVISION;
        width = tiles_width * SUBDIVISION;
        height = tiles_height * SUBDIVISION;
        data = new Color[width * height];
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
        if (data) delete[] data;
    }

    [[nodiscard]]
    inline glm::vec3 get_color(TilePos pos) const {
        if (!(pos.x >= 0 && pos.y >= 0 && pos.x < width && pos.y < height)) {
            return glm::vec3(0.0f);
        }
        
        Color color = data[pos.y * width + pos.x];
        return glm::vec3(static_cast<float>(color.r) / 255.0f, static_cast<float>(color.g) / 255.0f, static_cast<float>(color.b / 255.0f));
    }

    inline void set_color(TilePos pos, glm::vec3 color) {
        data[pos.y * width + pos.x] = Color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f);
    }

private:
    inline void copy(LightMap& from) {
        this->data = from.data;
        this->width = from.width;
        this->height = from.height;

        from.data = nullptr;
    }
};

struct LightMapTaskResult {
    Color* data;
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

    LightMapTask(LightMapTask&& other) {
        result = std::move(other.result);
        t.swap(other.t);
    }

    LightMapTask& operator=(LightMapTask&& other) {
        result = std::move(other.result);
        t.swap(other.t);
        return *this;
    }

    ~LightMapTask() {
        if (t.joinable()) t.join();
    }
};

#endif