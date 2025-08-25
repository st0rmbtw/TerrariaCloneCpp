#include <stdio.h>
#include <stdint.h>

#include <ft2build.h>
#include <freetype/freetype.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

struct GlyphInfo {
    uint8_t* buffer;
    uint32_t bitmap_width;
    uint32_t bitmap_rows;
    FT_Int bitmap_left;
    FT_Int bitmap_top;
    FT_Vector advance;
    uint32_t col;
    uint32_t row;
};

template <typename T>
static void write_to_file(std::ofstream& file, T data) {
    file.write(reinterpret_cast<char*>(&data), sizeof(T));
}

static bool generate_font_assets(FT_Library ft, const fs::path& input_path, const fs::path& output_path) {
    static constexpr uint32_t FONT_SIZE = 68;
    static constexpr uint32_t PADDING = 4;

    const std::string input_path_str = input_path.string();

    FT_Face face;
    if (FT_New_Face(ft, input_path_str.c_str(), 0, &face)) {
        printf("[ERROR] Failed to load font: %s", input_path_str.c_str());
        return false;
    }
    
    FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);

    std::vector<std::pair<FT_ULong, GlyphInfo>> glyphs;

    FT_UInt index;
    FT_ULong character = FT_Get_First_Char(face, &index);

    const uint32_t texture_width = 1512;

    uint32_t col = PADDING;
    uint32_t row = PADDING;

    uint32_t max_height = 0;

    while (true) {
        if (FT_Load_Char(face, character, FT_LOAD_DEFAULT)) {
            printf("[ERROR] Failed to load glyph %lu", character);
        } else {
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);

            if (col + face->glyph->bitmap.width + PADDING >= texture_width) {
                col = PADDING;
                row += max_height + PADDING;
                max_height = 0;
            }

            max_height = std::max(max_height, face->glyph->bitmap.rows);

            GlyphInfo info;
            info.bitmap_width = face->glyph->bitmap.width;
            info.bitmap_rows = face->glyph->bitmap.rows;
            info.bitmap_left = face->glyph->bitmap_left;
            info.bitmap_top = face->glyph->bitmap_top;
            info.advance = face->glyph->advance;
            info.col = col;
            info.row = row;
            info.buffer = new uint8_t[face->glyph->bitmap.pitch * info.bitmap_rows];
            memcpy(info.buffer, face->glyph->bitmap.buffer, face->glyph->bitmap.pitch * info.bitmap_rows);

            glyphs.emplace_back(character, info);

            col += info.bitmap_width + PADDING;
        }

        character = FT_Get_Next_Char(face, character, &index);
        if (!index) break;
    }

    const uint32_t texture_height = row + max_height;

    uint8_t* texture_data = new uint8_t[texture_width * texture_height];
    memset(texture_data, 0, texture_width * texture_height * sizeof(uint8_t));

    fs::create_directories(output_path.parent_path());

    const fs::path meta_path = output_path.parent_path() / output_path.stem().concat(".meta");
    std::ofstream font_metadata(meta_path, std::ios::binary | std::ios::out);

    if (!font_metadata.good()) {
        printf("[ERROR] Failed to create file %s\n", meta_path.string().c_str());
        return false;
    }

    write_to_file<FT_Short>(font_metadata, face->ascender);
    write_to_file<uint32_t>(font_metadata, FONT_SIZE);
    write_to_file<uint32_t>(font_metadata, texture_width);
    write_to_file<uint32_t>(font_metadata, texture_height);
    write_to_file<uint32_t>(font_metadata, glyphs.size());

    for (auto [c, info] : glyphs) {
        for (uint32_t y = 0; y < info.bitmap_rows; ++y) {
            for (uint32_t x = 0; x < info.bitmap_width; ++x) {
                texture_data[(info.row + y) * texture_width + info.col + x] = info.buffer[y * info.bitmap_width + x];
            }
        }

        write_to_file<FT_ULong>(font_metadata, c);
        write_to_file<uint32_t>(font_metadata, info.bitmap_width);
        write_to_file<uint32_t>(font_metadata, info.bitmap_rows);
        write_to_file<FT_Int>(font_metadata, info.bitmap_left);
        write_to_file<FT_Int>(font_metadata, info.bitmap_top);
        write_to_file<FT_Pos>(font_metadata, info.advance.x);
        write_to_file<uint32_t>(font_metadata, info.col);
        write_to_file<uint32_t>(font_metadata, info.row);

        delete[] info.buffer;
    }

    font_metadata.close();

    stbi_write_png(output_path.string().c_str(), texture_width, texture_height, 1, texture_data, texture_width);

    delete[] texture_data;

    FT_Done_Face(face);

    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>", argv[0]);
        return 0;
    }

    const fs::path input_path{ argv[1] };
    const fs::path output_path{ argv[2] };
    
    if (!input_path.has_filename()) {
        printf("[ERROR] %s is not a path to a file\n", argv[1]);
        return 1;
    }

    if (!output_path.has_filename()) {
        printf("[ERROR] %s is not a path to a file\n", argv[2]);
        return 1;
    }

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        printf("[ERROR] Couldn't init FreeType library.\n");
        return 1;
    }

    if (!generate_font_assets(ft, input_path, output_path)) return 1;

    FT_Done_FreeType(ft);

    return 0;
}