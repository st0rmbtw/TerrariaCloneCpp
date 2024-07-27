#include "ui.hpp"
#include "renderer/renderer.hpp"
#include "input.hpp"
#include "assets.hpp"

static struct UiState {

} ui_state;

void render_inventory() {
    
}

void UI::Update() {

}

void UI::Render(const Camera& camera) {
    Sprite sprite;
    sprite.set_custom_size(glm::vec2(25.0f));
    sprite.set_color(glm::vec3(1.0f, 0.0f, 0.0f));
    sprite.set_position(MouseInput::ScreenPosition());
    sprite.set_texture(Assets::GetTexture(AssetKey::TextureUiCursorForeground));

    Renderer::DrawSprite(sprite, RenderLayer::UI);
}