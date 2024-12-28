#include "utils.hpp"

#include <stdio.h>
#include <tracy/Tracy.hpp>

#include "assets.hpp"
#include "defines.hpp"

bool FileExists(const char *path) {
#ifdef PLATFORM_WINDOWS
    FILE *file = NULL;
    fopen_s(&file, path, "r");
#else
    FILE *file = fopen(path, "r");
#endif

    bool exists = file != NULL;

    if (file != NULL) {
        fclose(file);
    }

    return exists;
}

glm::vec2 calculate_text_bounds(FontAsset key, const std::string &text, float size) {
    ZoneScopedN("Utils::calculate_text_bounds");

    const Font& font = Assets::GetFont(key);

    auto bounds = glm::vec2(0.0f);
    float prev_x = 0.0f;

    const float scale = size / font.font_size;

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') {
            bounds.y += size;
            prev_x = 0.0f;
            continue;
        }

        const Glyph& glyph = font.glyphs.find(text[i])->second;
        prev_x += glyph.size.x * scale;
        bounds.x = std::max(prev_x, bounds.x);
    }

    return bounds;
}