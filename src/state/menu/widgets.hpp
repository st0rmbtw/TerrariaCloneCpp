#pragma once

#ifndef STATE_MENU_WIDGETS_HPP_
#define STATE_MENU_WIDGETS_HPP_

#include <SGE/types/font.hpp>
#include <SGE/time/timer.hpp>
#include <SGE/utils/text.hpp>

#include "../../assets.hpp"
#include "../../ui/ui.hpp"

#include "../common.hpp"

namespace widgets {

inline constexpr float MENU_BUTTON_FONT_SIZE = 48.0f;
inline constexpr sge::LinearRgba MENU_BUTTON_COLOR = sge::LinearRgba::white() * 0.9f;
inline constexpr sge::LinearRgba MENU_BUTTON_COLOR_HOVERED = sge::LinearRgba(1.0f, 1.0f, 0.0f) * 0.9f;

namespace UiTypeID {
    enum : uint8_t {
        Text = 0,
        Panel,
        CategoryPanel,
        Logo,
        Icon,
        Separator,
        WorldPreview,
        SliderHandle,
        TextInput
    };
}

struct UiPanelData {
    sge::LinearRgba background_color;
    sge::LinearRgba border_color;
};

struct UiCategoryPanelData {
    sge::LinearRgba background_color;
    bool hovered;
};

struct UiSeparatorData {
    sge::LinearRgba color;
};

struct UiIconData {
    TextureAsset icon;
};

struct UiTextInputData {
    sge::LinearRgba color;
    std::string_view data;
    const sge::Font& font;
    float size;
    bool bar_visible;
};

template <typename F>
inline void MenuButton(const sge::Font& font, std::string_view text, F&& on_click) {
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
inline void CategoryPanel(const ElementDesc& desc, sge::LinearRgba color, F&& children) {
    UI::Element<UiTypeID::CategoryPanel>(desc, [&] {
        UI::SetCustomData(UiCategoryPanelData {
            .background_color = color,
            .hovered = UI::IsHovered()
        });

        std::forward<F>(children)();
    });
}

template <typename F>
inline void CategoryPanel(const ElementDesc& desc, F&& children) {
    CategoryPanel(desc, sge::LinearRgba(63, 65, 151), std::forward<F>(children));
}

template <typename F>
inline void Button(const sge::Font& font, UiSize size, std::string_view text, F&& on_click) {
    UI::Element<UiTypeID::Panel>({
        .size = size,
        .padding = UiRect::Horizontal(20.0f),
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center
    }, [&] {
        UI::SetCustomData(UiPanelData {
            .background_color = UI::IsHovered() ? sge::LinearRgba(73, 94, 171) : sge::LinearRgba(63, 82, 151) * 0.8f,
            .border_color = UI::IsHovered() ? sge::LinearRgba(255, 231, 69) : sge::LinearRgba::black()
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

template <typename F>
inline void IconButton(TextureAsset icon, F&& on_click) {
    CategoryPanel({
        .size = UiSize::Fixed(44.0f, 44.0f),
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center
    }, [&] {
        UI::AddElement<UiTypeID::Icon>(
            {
                .size = UiSize::Fixed(Assets::GetTexture(icon).size())
            },
            UiIconData {
                .icon = icon
            }
        );

        UI::OnClick([on_click = std::forward<F>(on_click)](sge::MouseButton button) {
            if (button == sge::MouseButton::Left) {
                on_click();
            }
        });
    });
}

inline void HorizontalSeparator(Sizing width, sge::LinearRgba color = sge::LinearRgba::white()) {
    const glm::uvec2 texture_size = Assets::GetTexture(TextureAsset::UiSeparator1).size();

    UI::AddElement<UiTypeID::Separator>({
        .size = UiSize(width, Sizing::Fixed(texture_size.y)),
    }, UiSeparatorData {
        .color = color
    });
}

inline void WorldListItem(const sge::Font& font) {
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

inline void TextInput(TextInputData& data, bool bar_visible, const sge::Font& font, UiSize size) {
    UI::Element<UiTypeID::CategoryPanel>({
        .size = size,
        .padding = UiRect::Horizontal(8.0f),
        .orientation = LayoutOrientation::Horizontal,
        .vertical_alignment = Alignment::Center
    }, [&] {
        UI::SetCustomData(UiCategoryPanelData {
            .background_color = sge::LinearRgba(63, 82, 151),
            .hovered = data.active() || UI::IsHovered()
        });

        const float cursor_height = Assets::GetTexture(TextureAsset::UiSliderHandle).size().y;
        const float text_height = sge::calculate_text_height(font, 24.0f, data.text());

        UI::Element<UiTypeID::TextInput>({
            .size = UiSize::Fill(),
            .min_height = glm::max(text_height, cursor_height)
        }, [&] {
            data.set_active(UI::IsFocused());

            UI::SetCustomData(UiTextInputData {
                .color = sge::LinearRgba::white(),
                .data = data.text(),
                .font = font,
                .size = 24.0f,
                .bar_visible = data.active() && bar_visible
            });
        });
    });
}

inline void DrawPanel(const UiElement& element, sge::Batch& batch, sge::NinePatch& ninepatch) {
    const UiPanelData* data = static_cast<const UiPanelData*>(element.custom_data);
    const sge::LinearRgba background_color = data != nullptr ? data->background_color : sge::LinearRgba(63, 82, 151) * 0.7f;
    const sge::LinearRgba border_color = data != nullptr ? data->border_color : sge::LinearRgba::black();

    ninepatch.set_margin(glm::uvec4(12));

    ninepatch.set_texture(Assets::GetTexture(TextureAsset::UiPanelBackground));
    ninepatch.set_anchor(sge::Anchor::TopLeft);
    ninepatch.set_position(element.position - glm::vec2(0.5f));
    ninepatch.set_size(element.size + glm::vec2(1.0f));
    ninepatch.set_color(background_color);
    batch.DrawNinePatch(ninepatch, sge::Order(element.z_index));

    ninepatch.set_texture(Assets::GetTexture(TextureAsset::UiPanelBorder));
    ninepatch.set_anchor(sge::Anchor::TopLeft);
    ninepatch.set_position(element.position);
    ninepatch.set_size(element.size);
    ninepatch.set_color(border_color);
    batch.DrawNinePatch(ninepatch, sge::Order(element.z_index));
}

inline void DrawCategoryPanel(const UiElement& element, sge::Batch& batch, sge::NinePatch& ninepatch) {
    const UiCategoryPanelData* data = static_cast<const UiCategoryPanelData*>(element.custom_data);

    const sge::LinearRgba background_color = data->background_color;
    const bool hovered = data->hovered;
    
    ninepatch.set_margin(glm::uvec4(10));

    ninepatch.set_texture(Assets::GetTexture(TextureAsset::UiCategoryPanelBackground));
    ninepatch.set_anchor(sge::Anchor::TopLeft);
    ninepatch.set_position(element.position + glm::vec2(2.0f));
    ninepatch.set_size(element.size - glm::vec2(4.0f));
    ninepatch.set_color(background_color);
    batch.DrawNinePatch(ninepatch, sge::Order(element.z_index));

    if (hovered) {
        ninepatch.set_texture(Assets::GetTexture(TextureAsset::UiCategoryPanelBorder));
        ninepatch.set_anchor(sge::Anchor::TopLeft);
        ninepatch.set_position(element.position);
        ninepatch.set_size(element.size);
        ninepatch.set_color(sge::LinearRgba::white());
        batch.DrawNinePatch(ninepatch, sge::Order(element.z_index + 1));
    }
}

inline void DrawTextInput(const UiElement& element, sge::Batch& batch, sge::Sprite& sprite) {
    const UiTextInputData* data = static_cast<const UiTextInputData*>(element.custom_data);
                
    const sge::Texture& cursor_texture = Assets::GetTexture(TextureAsset::UiSliderHandle);
    const float line_width = element.size.x - cursor_texture.size().x;

    const uint32_t max_text_len = sge::chars_fit_in_line_from_end(data->font, data->size, data->data, line_width);
    uint32_t bytes_to_skip = 0;
    for (uint32_t i = 0; i < max_text_len; i++) {
        bytes_to_skip += sge::count_utf8_char_bytes_from_end(data->data.data(), data->data.size() - bytes_to_skip);
    }

    const auto from = data->data.begin() + std::max<int>(data->data.size() - bytes_to_skip, 0);
    const auto to = data->data.end();
    const std::string_view string = std::string_view{ from, to };

    const glm::vec2 bounds = sge::calculate_text_bounds(data->font, data->size, string);

    const sge::RichText text = sge::rich_text(string, data->size, data->color);
    batch.DrawText(text.sections, text.size(), element.position + glm::vec2(0.0f, (element.size.y - bounds.y) * 0.5f), data->font, sge::Order(element.z_index));

    if (data->bar_visible) {
        const float offset = std::min(bounds.x, element.size.x - cursor_texture.size().x);
        
        sprite.set_anchor(sge::Anchor::TopLeft);
        sprite.set_texture(cursor_texture);
        sprite.set_custom_size(std::nullopt);
        sprite.set_position(element.position + glm::vec2(offset, (element.size.y - cursor_texture.size().y) * 0.5f));
        sprite.set_color(sge::LinearRgba::white());
        sprite.set_rotation(glm::identity<glm::quat>());
        batch.DrawSprite(sprite, sge::Order(element.z_index));
    }
}



}

#endif