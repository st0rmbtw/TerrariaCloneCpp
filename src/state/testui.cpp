#include "testui.hpp"

#include <SGE/types/order.hpp>
#include <SGE/types/nine_patch.hpp>
#include <SGE/types/anchor.hpp>
#include <SGE/types/color.hpp>
#include <SGE/engine.hpp>
#include <SGE/renderer/renderer.hpp>

#include "../ui/ui.hpp"
#include "../assets.hpp"
#include "../app.hpp"
#include "../utils.hpp"

namespace UiTypeID {
    enum : uint8_t {
        Rectangle = 0,
        Text,
    };
}

struct UiRectangleData {
    sge::LinearRgba color;
};

TestUI::TestUI() :
    m_camera{ sge::CameraOrigin::TopLeft },
    m_batch { sge::Engine::Renderer(), Assets::GetShader(ShaderAsset::FontShader).ps }
{
    m_camera.set_viewport(App::GetWindowResolution());
    m_camera.set_zoom(1.0f);
    
    m_batch.SetIsUi(true);
}

void TestUI::Update() {
    UI::Update();
}

void TestUI::Render() {
    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);
    sge::Renderer& renderer = sge::Engine::Renderer();

    UI::Start(RootDesc(m_camera.viewport()));

    UI::Element<UiTypeID::Rectangle>({
        .size = UiSize::Fill(),
        .padding = UiRect::All(100.0f),
    }, [&] {
        UI::SetCustomData(UiRectangleData {
            .color = sge::LinearRgba(0.8f, 0.2f, 0.2f)
        });

        UI::Element<UiTypeID::Rectangle>({
            .size = UiSize::Fill(),
            .gap = 10.0f,
            .horizontal_alignment = Alignment::Center,
            .vertical_alignment = Alignment::Center
        }, [&] {
            UI::SetCustomData(UiRectangleData {
                .color = sge::LinearRgba(0.2f, 0.8f, 0.2f)
            });

            // for (int i = 0; i < 10; ++i) {
            //     UI::Element<UiTypeID::Rectangle>({
            //         .id = ID::Local(temp_format("ListItem{}", i)),
            //         .size = UiSize::Fixed(50.0f, 50.0f),
            //         .padding = UiRect::All(10.0f),
            //         .self_alignment = Alignment::Center,
            //     }, [] {
            //         UI::SetCustomData(UiRectangleData {
            //             .color = sge::LinearRgba(0.2f, 0.2f, 0.8f)
            //         });
                    
            //         UI::AddElement<UiTypeID::Rectangle>(
            //             {
            //                 .size = UiSize::Fill()
            //             },
            //             UiRectangleData {
            //                 .color = sge::LinearRgba(0.2f, 0.8f, 0.8f)
            //             }
            //         );
            //     });
            // }

            UI::Element<UiTypeID::Rectangle>({
                // .padding = UiRect::Axes(12.0f, 8.0f),
                .orientation = LayoutOrientation::Horizontal,
                .horizontal_alignment = Alignment::Center,
                .vertical_alignment = Alignment::Center,
            }, [&] {
                UI::SetCustomData(UiRectangleData {
                    .color = sge::LinearRgba(0.2f, 0.2f, 0.8f)
                });

                sge::RichText text = {{
                    { "Somebody\n", sge::LinearRgba::white(), 42.0f },
                    { "Once\n", sge::LinearRgba::white(), 42.0f },
                    { "Told me\n", sge::LinearRgba::white(), 42.0f },
                    { "The world\n", sge::LinearRgba::white(), 42.0f },
                    { "Is gonna roll me", sge::LinearRgba::white(), 42.0f }
                }};

                UI::Text<UiTypeID::Text>(font, text);
            });
        });
    });

    const std::vector<UiElement>& elements = UI::Finish();

    for (const UiElement& element : elements) {
        const sge::Order order = sge::Order(element.z_index);

        switch (element.type_id) {
            case UiTypeID::Rectangle: {
                const UiRectangleData* data = static_cast<const UiRectangleData*>(element.custom_data);

                m_batch.DrawRect(element.position, order, {
                    .size = element.size,
                    .color = data->color,
                    .border_thickness = 0.0f,
                    .border_color = sge::LinearRgba::transparent(),
                    .anchor = sge::Anchor::TopLeft
                });
            } break;

            case UiTypeID::Text: {
                const TextData* data = element.text_data;
                m_batch.DrawText(data->sections, data->sections_count, element.position, data->font, order);
            } break;
        }
    }

    renderer.Begin(m_camera);

    renderer.PrepareBatch(m_batch);
    renderer.UploadBatchData();

    renderer.BeginMainPass();
        renderer.Clear(LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f));
        renderer.RenderBatch(m_batch);
    renderer.EndPass();

    renderer.End();

    m_batch.Reset();
}

TestUI::~TestUI() {

}