#include "select_world.hpp"

#include "../../../ui/ui.hpp"

#include "../widgets.hpp"

void MenuSubstateSelectWorld::draw(NavManager& nav_manager) {
    using namespace widgets;

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    UI::Container({
        .self_alignment = Alignment::Center,
        .horizontal_alignment = Alignment::Center
    }, [&] {
        UI::Container({
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center
        }, [&] {
            UI::Element<UiTypeID::Panel>({
                .size = UiSize::Fixed(600.0f, 400.0f),
                .padding = UiRect::Axes(12.0f, 8.0f),
                .gap = 5.0f,
                .orientation = LayoutOrientation::Vertical,
                .horizontal_alignment = Alignment::Center,
                .scrollable = true,
            }, [&] {
                for (size_t i = 0; i < 15; ++i) {
                    WorldListItem(font);
                }
            });

            UI::Spacer(UiSize::Height(Sizing::Fixed(12.0f)));

            UI::Container({
                .size = UiSize::Width(Sizing::Fill()),
                .gap = 24.0f,
                .orientation = LayoutOrientation::Horizontal,
            }, [&] {
                Button(font, UiSize::Width(Sizing::Fill()), "Back", [&] {
                    nav_manager.pop();
                });
                Button(font, UiSize::Width(Sizing::Fill()), "New", [&] {
                    nav_manager.push<CreateWorld>();
                });
            });
        });

        UI::Element<UiTypeID::Panel>({
            .id = ID::Local("Header"),
            .padding = UiRect::Horizontal(12.0f),
            .offset = glm::vec2(0.0f, -35.0f)
        }, [&] {
            UI::SetCustomData(UiPanelData {
                .background_color = sge::LinearRgba(73, 94, 171),
                .border_color = sge::LinearRgba::black()
            });

            UI::Text<UiTypeID::Text>(font, sge::rich_text("Select World", 42.0f, sge::LinearRgba::white()));
        });
    });
}