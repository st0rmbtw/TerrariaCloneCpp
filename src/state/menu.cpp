#include <SGE/types/order.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/engine.hpp>
#include <SGE/types/color.hpp>
#include <SGE/input.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/anchor.hpp>

#include "../ui/ui.hpp"
#include "../assets.hpp"
#include "../app.hpp"

#include "menu.hpp"
#include "ingame.hpp"

static constexpr float MENU_BUTTON_FONT_SIZE = 48.0f;
static constexpr sge::LinearRgba MENU_BUTTON_COLOR = sge::LinearRgba::white() * 0.9f;
static constexpr sge::LinearRgba MENU_BUTTON_COLOR_HOVERED = sge::LinearRgba(1.0f, 1.0f, 0.0f) * 0.9f;

namespace UiTypeID {
    enum : uint8_t {
        Text = 0,
        Panel
    };
}

struct UiPanelData {
    sge::LinearRgba background_color;
    sge::LinearRgba border_color;
};

MainMenuState::MainMenuState() :
    m_camera(sge::CameraOrigin::Center, sge::CoordinateSystem {
        .up = sge::CoordinateDirectionY::Negative,
        .forward = sge::CoordinateDirectionZ::Negative,
    }),
    m_batch(sge::Engine::Renderer(), {
        .font_shader = Assets::GetShader(ShaderAsset::FontShader).ps,
        .enable_scissor = true
    })
{
    m_camera.set_viewport(App::GetWindowResolution());
    m_camera.set_zoom(1.0f);

    m_cursor.SetForegroundColor(sge::LinearRgba(1.0, 0.08, 0.58));
    m_cursor.SetBackgroundColor(sge::LinearRgba(0.9, 0.9, 0.9));

    m_background_renderer.init();

    m_batch.SetIsUi(true);

    setup_background();

    m_position_stack.push_back(MenuPosition::MainMenu);
}

MainMenuState::~MainMenuState() {
    m_batch.Destroy(sge::Engine::Renderer().Context());
    m_background_renderer.terminate();
}

void MainMenuState::setup_background() {
    m_background_layers.clear();

    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background0, 2.0f)
            .set_anchor(sge::Anchor::TopLeft)
            .set_speed(0.0f, 0.08f)
            .set_fill_screen_height(true)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );

    const float pos = m_camera.viewport().y;

    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background7, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_y(pos - 150.0f)
            .set_speed(0.3f, 0.9f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background90, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(0.5f, 0.8f)
            .set_y(pos + 50.0f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background91, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(0.6f, 0.7f)
            .set_y(pos + 150.0f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background92, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(1.0f, 0.6f)
            .set_y(pos + 550.0f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    // m_background_layers.push_back(
    //     BackgroundLayer(BackgroundAsset::Background112, 1.2f)
    //     .set_anchor(sge::Anchor::TopLeft)
    //         .set_speed(0.3f, 0.7f)
    //         .set_y(0.0f)
    //         .set_fill_screen_width(true)
    //         .set_is_ui(true)
    // );
}

void MainMenuState::Update() {
    UI::Update();

    m_cursor.Update(sge::Input::MouseScreenPosition());

    m_camera.set_position(m_camera.position() + glm::vec2(50.0f, 0.0f) * sge::Time::DeltaSeconds());

    for (BackgroundLayer& layer : m_background_layers) {
        if (layer.fill_screen_height()) {
            layer.set_height(m_camera.viewport().y);
        }
        if (layer.fill_screen_width()) {
            layer.set_width(m_camera.viewport().x);
        }
    }

    m_camera.update();

    if (sge::Input::JustPressed(sge::Key::Escape)) {
        set_previous_position();
    }
}

void MainMenuState::Render() {
    m_batch.Reset();
    m_background_renderer.reset();

    draw_ui();

    for (const BackgroundLayer& layer : m_background_layers) {
        draw_background(layer);
    }

    sge::Renderer& renderer = sge::Engine::Renderer();

    renderer.Begin(m_camera);

    renderer.PrepareBatch(m_batch);
    renderer.UploadBatchData();

    renderer.BeginMainPass();
        renderer.Clear(LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), LLGL::ClearFlags::ColorDepth);

        m_background_renderer.render();

        renderer.RenderBatch(m_batch);
    renderer.EndPass();

    renderer.End();
}

template <typename F>
static void MenuButton(const sge::Font& font, std::string_view text, F&& on_click) {
    UI::Container({}, [&] {
        const sge::LinearRgba color = UI::IsHovered() ? MENU_BUTTON_COLOR_HOVERED : MENU_BUTTON_COLOR;
        UI::Text<UiTypeID::Text>(font, sge::rich_text(text, MENU_BUTTON_FONT_SIZE, color));
        UI::OnClick([on_click = std::forward<F>(on_click)](sge::MouseButton button) {
            if (button == sge::MouseButton::Left) {
                on_click();
            }
        });
    });
}

template <typename F>
static void Button(const sge::Font& font, UiSize size, std::string_view text, F&& on_click) {
    UI::Element<UiTypeID::Panel>({
        .size = size,
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center
    }, [&] {
        UI::SetCustomData(UiPanelData {
            .background_color = sge::LinearRgba(63, 82, 151, 0.9f),
            .border_color = sge::LinearRgba::black()
        });

        const sge::LinearRgba color = UI::IsHovered() ? MENU_BUTTON_COLOR_HOVERED : MENU_BUTTON_COLOR;
        UI::Text<UiTypeID::Text>(font, sge::rich_text(text, 36.0f, color));
        UI::OnClick([on_click = std::forward<F>(on_click)](sge::MouseButton button) {
            if (button == sge::MouseButton::Left) {
                on_click();
            }
        });
    });
}

void MainMenuState::draw_main_menu() {
    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    UI::Container({
        .size = UiSize::Fill(),
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center,
    }, [&] {
        UI::Container({
            .gap = 24.0f,
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center
        }, [&] {
            MenuButton(font, "Play", [this]() {
                set_next_position(MenuPosition::SelectWorld);
            });

            MenuButton(font, "Settings", [this]() {
                set_next_position(MenuPosition::Settings);
            });

            MenuButton(font, "Exit", [this]() {
                m_exit = true;
            });
        });
    });
}

static void WorldListItem(const sge::Font& font) {
    UI::Element<UiTypeID::Panel>({
        .size = UiSize(Sizing::Fill(), Sizing::Fixed(100.0f)),
        .orientation = LayoutOrientation::Vertical,
        .horizontal_alignment = Alignment::Center,
    }, [&] {
        UI::SetCustomData(UiPanelData {
            .background_color = sge::LinearRgba(73, 94, 171),
            .border_color = sge::LinearRgba(89, 116, 213)
        });
    });
}

void MainMenuState::draw_select_world() {
    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    
    UI::Container({
        .orientation = LayoutOrientation::Vertical,
        .horizontal_alignment = Alignment::Center
    }, [&] {
        UI::Text<UiTypeID::Text>(font, sge::rich_text("Select World", 42.0f, sge::LinearRgba::white()));
        UI::Spacer(UiSize::Height(Sizing::Fixed(8.0f)));
        
        UI::Element<UiTypeID::Panel>({
            .id = ID::Local("WorldList"),
            .size = UiSize::Fixed(600.0f, 400.0f),
            .padding = UiRect::Axes(12.0f, 8.0f),
            .gap = 8.0f,
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center,
            .scrollable = true,
        }, [&] {
            for (size_t i = 0; i < 15; ++i) {
                WorldListItem(font);
            }
        });

        UI::Spacer(UiSize::Height(Sizing::Fixed(6.0f)));

        UI::Container({
            .id = ID::Local("ButtonsRow"),
            .size = UiSize::Width(Sizing::Fill()),
            .gap = 24.0f,
            .orientation = LayoutOrientation::Horizontal,
        }, [&] {
            Button(font, UiSize::Width(Sizing::Fill()), "Back", [this]() {
                set_previous_position();
            });
            Button(font, UiSize::Width(Sizing::Fill()), "New", [this]() {
                // TODO
                m_world_selected = true;
            });
        });
    });
}

void MainMenuState::draw_ui() {
    UI::Start(RootDesc(m_camera.viewport()));

    MenuPosition current_state = m_position_stack.back();

    UI::Container({
        .size = UiSize::Fill(),
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center,
    }, [&] {
        switch (current_state) {
        case MenuPosition::MainMenu:
            draw_main_menu();
        break;
        case MenuPosition::SelectWorld:
            draw_select_world();
        break;
        case MenuPosition::CreateWorld:
        break;
        case MenuPosition::Settings:
        break;
        }
    });

    const std::vector<UiElement>& elements = UI::Finish();

    sge::Sprite sprite(Assets::GetTexture(TextureAsset::Stub));
    sge::NinePatch panel(Assets::GetTexture(TextureAsset::UiPanelBackground), glm::uvec4(12));
    sge::NinePatch border(Assets::GetTexture(TextureAsset::UiPanelBorder), glm::uvec4(12));

    m_batch.BeginOrderMode();

    for (const UiElement& element : elements) {
        const sge::Order order = sge::Order(element.z_index);

        if (element.scissor_start) {
            m_batch.BeginScissorMode(element.scissor_data->area);
            continue;
        }

        if (element.scissor_end) {
            m_batch.EndScissorMode();
            continue;
        }

        switch (element.type_id) {
            case UiTypeID::Text: {
                const TextData* data = element.text_data;
                m_batch.DrawText(data->sections, data->sections_count, element.position, data->font, order);
            } break;

            case UiTypeID::Panel: {
                const UiPanelData* data = static_cast<const UiPanelData*>(element.custom_data);

                const sge::LinearRgba background_color = data != nullptr ? data->background_color : sge::LinearRgba(63, 82, 151) * 0.7f;
                const sge::LinearRgba border_color = data != nullptr ? data->border_color : sge::LinearRgba::black();

                panel.set_anchor(sge::Anchor::TopLeft);
                panel.set_position(element.position - glm::vec2(0.5f));
                panel.set_size(element.size + glm::vec2(1.0f));
                panel.set_color(background_color);
                m_batch.DrawNinePatch(panel, order);

                border.set_anchor(sge::Anchor::TopLeft);
                border.set_position(element.position);
                border.set_size(element.size);
                border.set_color(border_color);
                m_batch.DrawNinePatch(border, sge::Order(order.value + 1));
            } break;
        }
    }

    m_batch.EndOrderMode();

    m_batch.DrawSprite(m_cursor.Background());
    m_batch.DrawSprite(m_cursor.Foreground());
}

BaseState* MainMenuState::GetNextState() {
    if (m_exit)
        return nullptr;

    if (m_world_selected)
        return new InGameState();

    return this;
}

void MainMenuState::set_next_position(MenuPosition new_state) {
    m_position_stack.push_back(new_state);
}

void MainMenuState::set_previous_position() {
    if (m_position_stack.size() > 1) {
        m_position_stack.pop_back();
    }
}

void MainMenuState::draw_background(const BackgroundLayer& layer) {
    ZoneScoped;

    // TODO
    // const sge::Rect aabb = sge::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());
    // if (!state.camera_frustums[layer.nonscale()].intersects(aabb)) return;

    if (layer.is_world()) {
        m_background_renderer.draw_world_layer(layer);
    } else {
        m_background_renderer.draw_layer(layer);
    }
}