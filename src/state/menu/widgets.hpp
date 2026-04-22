#pragma once

#ifndef STATE_MENU_WIDGETS_HPP_
#define STATE_MENU_WIDGETS_HPP_

#include <optional>
#include <SGE/types/font.hpp>
#include <SGE/types/color.hpp>
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
                .size = UiSize::Fixed(glm::vec2(Assets::GetTexture(icon).size()))
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

template <typename F>
inline void WorldListItem(const sge::Font& font, const std::string_view name, TextureAsset icon, F&& on_click) {
    UI::Element<UiTypeID::Panel>({
        .size = UiSize(Sizing::Fill(), Sizing::Fixed(100.0f)),
        .padding = UiRect::Axes(8.0f, 0.0f),
        .orientation = LayoutOrientation::Vertical,
    }, [&] {
        UI::SetCustomData(UiPanelData {
            .background_color = sge::LinearRgba(73, 94, 171),
            .border_color = sge::LinearRgba(89, 116, 213)
        });
        UI::OnClick(std::forward<F>(on_click));

        UI::Container({
            .size = UiSize::Fill(),
            .orientation = LayoutOrientation::Vertical,
        }, [&] {
            UI::Container({
                .size = UiSize::Width(Sizing::Fill()),
                .gap = 6.0f,
                .orientation = LayoutOrientation::Horizontal,
            }, [&] {
                UI::AddElement<UiTypeID::Icon>({
                    .size = UiSize::Fixed(glm::vec2(Assets::GetTexture(icon).size())),
                    .self_alignment = Alignment::Center
                }, UiIconData {
                    .icon = icon
                });

                UI::Container({
                    .orientation = LayoutOrientation::Vertical,
                }, [&] {
                    UI::Text<UiTypeID::Text>(font, sge::rich_text(name, 20.0f, sge::LinearRgba::white()));     
                });
            });

            UI::Container({
                .orientation = LayoutOrientation::Horizontal
            }, [&] {
                // TODO: Buttons
            });
        });
    });
}

template <size_t PrefixSize = 1>
struct TextInputDesc { 
    std::optional<sge::RichText<PrefixSize>> prefix = std::nullopt;
    UiSize size;
};

template <size_t PrefixSize = 1>
inline void TextInput(TextInputData& data, const sge::Font& font, const TextInputDesc<PrefixSize>& desc) {
    UI::Element<UiTypeID::CategoryPanel>({
        .size = desc.size,
        .padding = UiRect::Horizontal(8.0f),
        .orientation = LayoutOrientation::Horizontal,
        .vertical_alignment = Alignment::Center
    }, [&] {
        data.set_active(data.active() || UI::IsFocused());

        const float cursor_height = Assets::GetTexture(TextureAsset::UiSliderHandle).size().height;
        const float text_height = sge::calculate_text_height(font, 24.0f, data.text());

        if (desc.prefix) {
            UI::Text<UiTypeID::Text>(font, desc.prefix.value());
        }

        UI::TextInput<UiTypeID::TextInput>(data, font, {
            .size = UiSize::Fill(),
            .text_size = 24.0f,
            // .min_height = glm::max(text_height, cursor_height)
        });//, [&] {
            // UI::SetCustomData(UiTextInputData {
            //     .color = sge::LinearRgba::white(),
            //     .font = font,
            //     .data = data,
            //     .size = 24.0f,
            //     .bar_visible = bar_visible
            // });
        //});

        UI::SetCustomData(UiCategoryPanelData {
            .background_color = sge::LinearRgba(63, 82, 151),
            .hovered = data.active() || UI::IsHovered()
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

inline void DrawTextInput(const UiElement& element, sge::Batch& batch) {
    ZoneScoped;

    const TextInputNodeData* text_input_data = element.text_input_data;

    const TextInputData& data = text_input_data->data;
    const sge::Font& font = text_input_data->font;
    const sge::LinearRgba color = text_input_data->color;
    const float text_size = text_input_data->text_size;
    const float bar_height = text_size;

    const bool bar_visible = data.bar_visible();
    
    const float line_width = element.size.x;
    float x = 0.0f;

    if (!data.empty()) {
        auto begin = data.text().begin() + data.display_begin();
        auto end = data.text().end();
        sge::FitResult fit_result = sge::chars_fit_in_line_from_start(font, text_size, std::string_view{ begin, end }, line_width);

        // Draw text before cursor
        const auto to = begin + fit_result.bytes;
        const std::string_view string = std::string_view{ begin, to };

        const glm::vec2 pre_cursor_bounds = sge::calculate_text_bounds(font, text_size, std::string_view{ begin, begin + (data.cursor_position() - data.display_begin()) });
        const float text_height = sge::calculate_text_height(font, text_size, string);

        const sge::RichText text = sge::rich_text(string, text_size, color);
        const glm::vec2 position = glm::vec2(element.position.x + x, element.position.y + (element.size.y - text_height) * 0.5f);
        batch.DrawText(text.sections, text.size(), position, font, sge::Order(element.z_index));

        
        x += pre_cursor_bounds.x;
    }

    if (data.active() && bar_visible) {
        batch.DrawRect(element.position + glm::vec2(x, (element.size.y - bar_height) * 0.5f), sge::Order(element.z_index + 1), {
            .size = glm::vec2(2.0f, bar_height),
            .color = sge::LinearRgba::white(),
            .anchor = sge::Anchor::TopLeft
        });
    }
}



}

#endif
