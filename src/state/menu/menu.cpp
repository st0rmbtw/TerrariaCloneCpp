#include <SGE/types/order.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/engine.hpp>
#include <SGE/types/color.hpp>
#include <SGE/input.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/anchor.hpp>
#include <SGE/math/quat.hpp>
#include <SGE/utils/text.hpp>
#include <SGE/utils/utf8.hpp>
#include <SGE/assert.hpp>

#include <glm/trigonometric.hpp>

#include "../../ui/ui.hpp"
#include "../../assets.hpp"

#include "../../world/world_gen.h"
#include "../../world/io/load.hpp"

#include "../common.hpp"
#include "../ingame.hpp"

#include "menu.hpp"
#include "nav_manager.hpp"
#include "substate/create_world.hpp"
#include "substate/select_world.hpp"
#include "widgets.hpp"

static constexpr float LOGO_ANIM_MIN_SCALE = 0.9f;
static constexpr float LOGO_ANIM_MAX_SCALE = 1.1f;
static constexpr float LOGO_ANIM_MIN_ROTATION = -5.0f;
static constexpr float LOGO_ANIM_MAX_ROTATION = 5.0f;

MainMenuState::MainMenuState(const std::shared_ptr<sge::Renderer>& renderer, uint8_t samples) :
    m_batch(*renderer, {
        .font_shader = Assets::GetShader(ShaderAsset::FontShader).ps,
        .enable_scissor = true
    }),
    m_background_renderer(renderer),
    m_renderer(renderer)
{
    m_camera = sge::Camera(sge::CameraConfig {
        .origin = sge::CameraOrigin::Center,
        .coordinateSystem = sge::CoordinateSystem {
            .up = sge::CoordinateDirectionY::Negative,
            .forward = sge::CoordinateDirectionZ::Negative,
        },
        .samples = samples
    });

    m_cursor.SetForegroundColor(sge::LinearRgba(1.0, 0.08, 0.58));
    m_cursor.SetBackgroundColor(sge::LinearRgba(0.9, 0.9, 0.9));

    m_batch.SetIsUi(true);

    setup_background();

    m_nav_manager.push<MainMenu>();

    srand(std::time(nullptr));
}

MainMenuState::~MainMenuState() {
    m_renderer->DestroyBatch(m_batch);
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

    const float pos = m_camera.viewport().height;

    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background7, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_y(pos - 150.0f)
            .set_speed(0.2f, 0.9f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background90, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(0.4f, 0.8f)
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

    m_cursor.Update(sge::Input::CursorPosition());

    m_camera.set_position(m_camera.position() + glm::vec2(50.0f, 0.0f) * float(sge::Time::DeltaSeconds()));

    for (BackgroundLayer& layer : m_background_layers) {
        if (layer.fill_screen_height()) {
            layer.set_height(m_camera.viewport().height);
        }
        if (layer.fill_screen_width()) {
            layer.set_width(m_camera.viewport().width);
        }
    }

    m_camera.update();

    if (sge::Input::JustPressed(sge::Key::Escape)) {
        m_nav_manager.pop();
    }

    update_logo();

    switch (m_nav_manager.top().index()) {
        case variant_index<NavItem, SelectWorld>:
            if (m_prev_position != m_nav_manager.top().index()) {
                m_substate = MenuSubstateSelectWorld();
            }
            break;
        case variant_index<NavItem, CreateWorld>:
            if (m_prev_position != m_nav_manager.top().index()) {
                m_substate = MenuSubstateCreateWorld();
            }
            break;
        default:
            break;
    }

    m_prev_position = m_nav_manager.top().index();
}

void MainMenuState::update_logo() {
    m_logo_animation.tick(sge::Time::DeltaSeconds());

    m_logo_scale = LOGO_ANIM_MIN_SCALE + (LOGO_ANIM_MAX_SCALE - LOGO_ANIM_MIN_SCALE) * m_logo_animation.progress();
    m_logo_rotation = Quat::from_rotation_z(glm::radians(LOGO_ANIM_MIN_ROTATION + (LOGO_ANIM_MAX_ROTATION - LOGO_ANIM_MIN_ROTATION) * m_logo_animation.progress()));
}

void MainMenuState::Render(const std::shared_ptr<sge::GlfwWindow>& window) {
    m_batch.Reset();
    m_background_renderer.reset();

    draw_ui();

    for (const BackgroundLayer& layer : m_background_layers) {
        draw_background(layer);
    }

    m_renderer->Begin();
        m_renderer->PrepareBatch(m_batch);
        m_renderer->UploadBatchData();

        m_renderer->BeginPass(window, m_camera);
            m_renderer->Clear(LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), LLGL::ClearFlags::ColorDepth);

            m_background_renderer.render();

            m_renderer->RenderBatch(m_batch);
        m_renderer->EndPass();
    m_renderer->End();

    m_renderer->Present(window);
}

void MainMenuState::draw_main_menu() {
    using namespace widgets;

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    UI::Container({
        .size = UiSize::Fill(),
        .self_alignment = Alignment::Center,
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center,
    }, [&] {
        UI::Container({
            .gap = 24.0f,
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center
        }, [&] {
            MenuButton(font, "Play", [this] {
                m_nav_manager.push<SelectWorld>();
            });

            MenuButton(font, "Settings", [this] {
                m_nav_manager.push<Settings>();
            });

            MenuButton(font, "Exit", [this] {
                m_exit = true;
            });
        });
    });
}

void MainMenuState::draw_ui() {
    UI::Start(RootDesc(glm::vec2(m_camera.viewport())));

    UI::Container({
        .size = UiSize::Fill(),
        .padding = UiRect::Vertical(50.0f),
        .orientation = LayoutOrientation::Vertical,
        .horizontal_alignment = Alignment::Center
    }, [&] {
        UI::AddElement<widgets::UiTypeID::Logo>({
            .size = UiSize::Fixed(glm::vec2(Assets::GetTexture(TextureAsset::UiLogo).size())),
        });

        UI::Container({
            .size = UiSize::Fill(),
            .horizontal_alignment = Alignment::Center,
            .vertical_alignment = Alignment::Center
        }, [&] {
            switch (m_nav_manager.top().index()) {
                case variant_index<NavItem, MainMenu>:
                    draw_main_menu();
                    break;
                case variant_index<NavItem, SelectWorld>:
                    unsafe_get<MenuSubstateSelectWorld>(m_substate).draw(m_nav_manager);
                    break;
                case variant_index<NavItem, CreateWorld>:
                    unsafe_get<MenuSubstateCreateWorld>(m_substate).draw(m_nav_manager);
                    break;
                default:
                    break;
            }
        });
    });

    const std::vector<UiElement>& elements = UI::Finish();

    sge::Sprite sprite(Assets::GetTexture(TextureAsset::Stub));
    sge::NinePatch ninepatch(Assets::GetTexture(TextureAsset::UiPanelBackground), glm::uvec4(12));

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
            case widgets::UiTypeID::Text: {
                const TextNodeData* data = element.text_data;
                m_batch.DrawText(data->sections, data->sections_count, element.position, data->font, order);
            } break;

            case widgets::UiTypeID::Panel: {
                widgets::DrawPanel(element, m_batch, ninepatch);
            } break;

            case widgets::UiTypeID::CategoryPanel: {
                widgets::DrawCategoryPanel(element, m_batch, ninepatch);
            } break;

            case widgets::UiTypeID::Logo: {
                sprite.set_anchor(sge::Anchor::Center);
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiLogo));
                sprite.set_position(element.position + element.size * 0.5f);
                sprite.set_custom_size(element.size);
                sprite.set_scale(m_logo_scale);
                sprite.set_rotation(m_logo_rotation);
                m_batch.DrawSprite(sprite, order);
            } break;

            case widgets::UiTypeID::Icon: {
                const widgets::UiIconData* data = static_cast<const widgets::UiIconData*>(element.custom_data);
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_texture(Assets::GetTexture(data->icon));
                sprite.set_position(element.position);
                sprite.set_custom_size(element.size);
                sprite.set_rotation(glm::identity<glm::quat>());
                sprite.set_scale(1.0f);
                m_batch.DrawSprite(sprite, order);
            } break;

            case widgets::UiTypeID::Separator: {
                const widgets::UiSeparatorData* data = static_cast<const widgets::UiSeparatorData*>(element.custom_data);
                ninepatch.set_margin(glm::uvec4(2));
                ninepatch.set_texture(Assets::GetTexture(TextureAsset::UiSeparator1));
                ninepatch.set_anchor(sge::Anchor::TopLeft);
                ninepatch.set_position(element.position);
                ninepatch.set_size(element.size);
                ninepatch.set_color(data->color);
                m_batch.DrawNinePatch(ninepatch, order);
            } break;

            case widgets::UiTypeID::WorldPreview: {
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_position(element.position + glm::vec2(4.0f));
                sprite.set_custom_size(element.size - glm::vec2(4.0f));
                sprite.set_color(sge::LinearRgba::white());
                sprite.set_rotation(glm::identity<glm::quat>());
                
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewDifficultyNormal1));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 1));

                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewEvilRandom));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 2));

                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewSizeLarge));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 3));

                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewDifficultyNormal2));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 4));

                sprite.set_position(element.position);
                sprite.set_custom_size(element.size);
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewBorder));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 5));
            } break;

            case widgets::UiTypeID::SliderHandle: {
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_position(element.position);
                sprite.set_custom_size(element.size);
                sprite.set_color(sge::LinearRgba::white());
                sprite.set_rotation(glm::identity<glm::quat>());
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiSliderHandle));
                m_batch.DrawSprite(sprite, order);
            } break;

            case widgets::UiTypeID::TextInput: {
                widgets::DrawTextInput(element, m_batch);
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

    if (const WorldSelected* opts = m_nav_manager.get<WorldSelected>()) {
        WorldData world_data;
        load_world(world_data, opts->path);
        return new InGameState(m_renderer, m_camera.samples(), std::move(world_data));
    }

    if (const WorldCreating* opts = m_nav_manager.get<WorldCreating>()) {
        WorldData world_data;
        world_generate(world_data, opts->world_width, opts->world_height, 0);
        return new InGameState(m_renderer, m_camera.samples(), std::move(world_data));
    }

    return this;
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
