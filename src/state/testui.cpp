#include "testui.hpp"

#include <SGE/types/order.hpp>
#include <SGE/types/nine_patch.hpp>
#include <SGE/types/anchor.hpp>
#include <SGE/types/color.hpp>
#include <SGE/engine.hpp>
#include <SGE/renderer/renderer.hpp>

#include "../ui/ui.hpp"
#include "../app.hpp"
#include "../utils.hpp"

namespace UiTypeID {
    enum : uint8_t {
        Rectangle = 0
    };
}

struct UiRectangleData {
    sge::LinearRgba color;
};

TestUI::TestUI() :
    m_camera{ sge::CameraOrigin::Center, sge::CoordinateSystem {
        .up = sge::CoordinateDirectionY::Negative,
        .forward = sge::CoordinateDirectionZ::Negative,
    }}
{
    m_camera.set_viewport(App::GetWindowResolution());
    m_camera.set_zoom(1.0f);
    
    m_batch.SetIsUi(true);
}

void TestUI::Update() {
    UI::Update();
}

void TestUI::Render() {
    sge::Renderer& renderer = sge::Engine::Renderer();

    UI::Start(RootDesc(m_camera.viewport()));

    UI::BeginElement(
        UiTypeID::Rectangle,
        ElementDesc(UiSize::Fill())
            .with_padding(UiRect::all(100.0f))
    );
    {
        UI::SetCustomData(UiRectangleData {
            .color = sge::LinearRgba(0.8f, 0.2f, 0.2f)
        });

        UI::BeginElement(
            UiTypeID::Rectangle,
            ElementDesc(UiSize::Fill())
                .with_orientation(LayoutOrientation::Horizontal)
                .with_gap(10.0f)
        );
        {
            UI::SetCustomData(UiRectangleData {
                .color = sge::LinearRgba(0.2f, 0.8f, 0.2f)
            });

            for (int i = 0; i < 10; ++i) {
                UI::BeginElement(
                    UiTypeID::Rectangle,
                    ElementDesc(UiSize::Fixed(50.0f, 50.0f))
                        .with_id(ID::Local(temp_format("ListItem{}", i)))
                        .with_self_alignment(Alignment::Center)
                        .with_padding(UiRect::all(10.0f))
                );
                {
                    UI::SetCustomData(UiRectangleData {
                        .color = sge::LinearRgba(0.2f, 0.2f, 0.8f)
                    });
                    
                    UI::AddElement(
                        UiTypeID::Rectangle,
                        ElementDesc(UiSize::Fill()),
                        UiRectangleData {
                            .color = sge::LinearRgba(0.2f, 0.8f, 0.8f)
                        }
                    );
                }
                UI::EndElement();
            }
        }
        UI::EndElement();
    }
    UI::EndElement();

    const std::vector<UiElement>& elements = UI::Finish();

    // TODO: NinePatch doesn't render
    // sge::NinePatch nine_patch(Assets::GetTexture(TextureAsset::UiInventoryBackground), glm::uvec4(6));

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