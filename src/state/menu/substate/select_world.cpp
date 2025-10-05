#include <filesystem>
#include <iostream>

#include <SGE/input.hpp>

#include "../../../ui/ui.hpp"
#include "../../../world/io/load.hpp"
#include "../widgets.hpp"

#include "select_world.hpp"

namespace fs = std::filesystem;

MenuSubstateSelectWorld::MenuSubstateSelectWorld() {
    get_worlds();
}

void MenuSubstateSelectWorld::get_worlds() {
    const fs::path worlds_dir = fs::current_path() / "worlds";
    for (const auto& entry : fs::directory_iterator(worlds_dir, fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;
        const fs::path& path = entry.path();
        if (path.extension() != ".wld") continue;

        read_world(path);
    }
}

void MenuSubstateSelectWorld::read_world(const fs::path& path) {
    std::ifstream a(path, std::ios::binary);

    BufferedReader reader(std::move(a));
    SGE_ASSERT(reader.good());
    SGE_ASSERT(!reader.eof());

    WorldHeader header;
    read_world_header(header, reader);

    TextureAsset icon;

    switch (header.evil) {
    case WorldEvil::Corruption:
        icon = TextureAsset::UiWorldIconCorruption;
        break;
    case WorldEvil::Crimson:
        icon = TextureAsset::UiWorldIconCrimson;
        break;
    case WorldEvil::Both:
        icon = TextureAsset::UiWorldIconCorruptionCrimson;
        break;
    }

    m_worlds.push_back(WorldInfo {
        .path = path,
        .name = std::move(header.name),
        .icon = icon
    });
}

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
                for (const WorldInfo& info : m_worlds) {
                    WorldListItem(font, info.name, info.icon, [&](sge::MouseButton button) {
                        if (button == sge::MouseButton::Left) {
                            nav_manager.push(WorldSelected {
                                .path = info.path
                            });
                        }
                    });
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

            UI::Text<UiTypeID::Text>(font, sge::rich_text("Select World", 38.0f, sge::LinearRgba::white()));
        });
    });
}