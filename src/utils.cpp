#include <stdio.h>
#include "utils.hpp"
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
    const Font& font = Assets::GetFont(key);

    auto bounds = glm::vec2(0.0f);
    float prev_x = 0.0f;

    float scale = size / font.font_size;

    const char* c = text.c_str();
    for (; *c != '\0'; c++) {
        if (*c == '\n') {
            bounds.y += size;
            prev_x = 0.0f;
            continue;
        }

        const Glyph& glyph = font.glyphs.find(*c)->second;
        prev_x += glyph.size.x * scale;
        bounds.x = std::max(prev_x, bounds.x);
    }

    return bounds;
}