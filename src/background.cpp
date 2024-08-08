#include "background.hpp"

#include <vector>

#include "assets.hpp"
#include "constants.hpp"
#include "renderer/renderer.hpp"
#include "types/anchor.hpp"
#include "types/texture.hpp"

using Constants::TILE_SIZE;

class Layer {
public:
    Layer(TextureKey key, float scale, const Camera& camera) {
        const Texture& texture = Assets::GetTexture(key);

        m_texture = texture;
        m_scale = scale;

        calculate_count(camera);
    }

    void calculate_count(const Camera& camera) {
        m_texture_width = m_texture.size.x * m_scale;

        if (m_nonscale) {
            int count = static_cast<float>(camera.viewport().x) / (m_texture_width * 0.5f) + 1;
            m_count = glm::max(count, 1);
        } else {
            int count = static_cast<float>(camera.get_projection_area().width()) / (m_texture_width * 0.5f) + 1;
            m_count = glm::max(count, 1);
        }
    }

    inline Layer& set_position(const glm::vec2& position) { m_position = position; return *this; }
    inline Layer& set_speed(float x, float y) { m_speed = glm::vec2(x, y); return *this; }
    inline Layer& set_y(const float y) { m_y = y;  return *this; }
    inline Layer& set_anchor(const Anchor anchor) { m_anchor = anchor; return *this; }
    inline Layer& set_fill_screen_height() { m_fill_screen_height = true; return *this; }
    inline Layer& set_nonscale(bool nonscale) { m_nonscale = nonscale; return *this; }

    [[nodiscard]] inline const Texture& texture() const { return m_texture; }
    [[nodiscard]] inline const glm::vec2& position() const { return m_position; }
    [[nodiscard]] inline const glm::vec2& speed() const { return m_speed; }
    [[nodiscard]] inline float scale() const { return m_scale; }
    [[nodiscard]] inline float y() const { return m_y; }
    [[nodiscard]] inline float texture_width() const { return m_texture_width; }
    [[nodiscard]] inline int count() const { return m_count; }
    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }
    [[nodiscard]] inline uint32_t texture_id() const { return m_texture.id; }
    [[nodiscard]] inline bool nonscale() const { return m_nonscale; }

private:
    Texture m_texture;
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_speed = glm::vec2(0.0f);
    float m_scale = 0.0f;
    float m_y;
    float m_texture_width = 0.0f;
    int m_count = 0;
    Anchor m_anchor;
    bool m_fill_screen_height = false;
    bool m_nonscale = true;
};

static struct BackgroundState {
    std::vector<Layer> layers;
} state;

void spawn_cavern_background(const Camera& camera, const World& world);

void Background::Init(const Camera& camera, const World& world) {
    state.layers.clear();

    // const float pos = 150.0f;

    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background0),
    //     .speed = glm::vec2(1.f, 0.f),
    //     .scale = 1.f,
    //     .y = 0.f,
    //     .fill_screen_height = true
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background7),
    //     .speed = glm::vec2(1.f, 0.9f),
    //     .scale = 1.5f,
    //     .y = pos,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background90),
    //     .speed = glm::vec2(1.f, 0.8f),
    //     .scale = 1.5f,
    //     .y = pos - 200.0f,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background91),
    //     .speed = glm::vec2(1.f, 0.7f),
    //     .scale = 1.5f,
    //     .y = pos - 300.0f,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background92),
    //     .speed = glm::vec2(1.f, 0.6f),
    //     .scale = 1.5f,
    //     .y = 0.f,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background112),
    //     .speed = glm::vec2(1.f, 0.7f),
    //     .scale = 1.2f,
    //     .y = 0.f,
    // });

    state.layers.push_back(
        Layer(TextureKey::Background93, 2.0f, camera)
            .set_speed(0.2f, 0.4f)
            .set_y(world.layers().underground * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
    );

    state.layers.push_back(
        Layer(TextureKey::Background114, 2.0f, camera)
            .set_speed(0.4f, 0.5f)
            .set_y((world.layers().underground + world.layers().dirt_height * 0.5f) * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
    );

    state.layers.push_back(
        Layer(TextureKey::Background55, 2.0f, camera)
            .set_speed(0.8f, 0.6f)
            .set_y((world.layers().underground + world.layers().dirt_height * 0.5f) * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
    );

    spawn_cavern_background(camera, world);
}

void Background::Update(const Camera &camera) {
    for (Layer& layer : state.layers) {
        const float texture_width = layer.texture_width();

        glm::vec2 new_position = layer.position();

        new_position.y = camera.position().y + (layer.y() - camera.position().y) * layer.speed().y;
        new_position.x = camera.position().x - camera.position().x * (1.0f - layer.speed().x);
        new_position.x = camera.position().x - fmod(new_position.x, texture_width);

        layer.set_position(new_position);
    }
}

void Background::Render(const Camera& camera) {
    if (state.layers.empty()) return;

    int depth = Renderer::GetGlobalDepthIndex();
    uint32_t prev_texture_id = state.layers[0].texture_id();

    Sprite sprite;
    for (const Layer& layer : state.layers) {
        sprite.set_texture(layer.texture());
        sprite.set_anchor(layer.anchor());
        sprite.set_scale(layer.scale());
        sprite.set_nonscalable(layer.nonscale());

        for (int i = -layer.count(); i <= layer.count(); ++i) {
            sprite.set_position(layer.position() + glm::vec2(layer.texture_width() * i, 0.0f));

            Renderer::DrawSprite(sprite, depth);
        }

        if (prev_texture_id != layer.texture_id()) {
            depth = Renderer::GetGlobalDepthIndex();
        }
        
        prev_texture_id = layer.texture_id();
    }
}

void Background::ResizeSprites(const Camera& camera) {
    for (Layer& layer : state.layers) {
        layer.calculate_count(camera);
    }
}

void spawn_cavern_background(const Camera& camera, const World& world) {
    const float underground_level = static_cast<float>(world.layers().underground) * TILE_SIZE;
    const float world_height = static_cast<float>(world.playable_area().height()) * TILE_SIZE;

    const float texture_height = static_cast<float>(Assets::GetTexture(TextureKey::Background78).size.y);

    state.layers.push_back(
        Layer(TextureKey::Background77, 1.0f, camera)
            .set_speed(0.9f, 1.0f)
            .set_anchor(Anchor::BottomCenter)
            .set_y(underground_level)
            .set_nonscale(false)
    );

    float y = underground_level;
    while (y < world_height) {
        state.layers.push_back(
            Layer(TextureKey::Background78, 1.0f, camera)
                .set_speed(0.9f, 1.0f) 
                .set_anchor(Anchor::TopCenter)
                .set_y(y)
                .set_nonscale(false)
        );

        y += texture_height;
    }
}