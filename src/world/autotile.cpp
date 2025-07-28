// Based on the implementation from https://github.com/TEdit/Terraria-Map-Editor/blob/main/src/TEdit/View/WorldRenderXna.xaml.cs

#include "autotile.hpp"

#include <string>
#include <array>

#include "../types/texture_atlas_pos.hpp"
#include "../types/block.hpp"
#include "../utils.hpp"

static constexpr uint8_t MERGE_VALIDATION[22][16] = {
    {11, 13, 13, 13, 14, 10, 8, 8, 8, 1, 15, 15, 4, 13, 13, 13},
    {11, 15, 15, 15, 14, 10, 15, 15, 15, 1, 15, 15, 4, 7, 7, 7},
    {11, 7, 7, 7, 14, 10, 15, 15, 15, 1, 15, 15, 4, 11, 11, 11},
    {9, 12, 9, 12, 9, 12, 2, 2, 2, 0, 0, 0, 0, 14, 14, 14},
    {3, 6, 3, 6, 3, 6, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 8, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 8, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 8, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 2, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 2, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 2, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {13, 13, 13, 13, 13, 13, 15, 15, 15, 5, 5, 5, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 10, 9, 13, 12, 9, 13, 12, 9, 13, 12},
    {4, 4, 4, 1, 1, 1, 10, 11, 15, 14, 11, 15, 14, 11, 15, 14},
    {5, 5, 5, 5, 5, 5, 10, 3, 7, 6, 3, 7, 6, 3, 7, 6},
    {11, 14, 13, 13, 13, 9, 9, 9, 12, 12, 12, 15, 15, 15, 0, 0},
    {11, 14, 7, 7, 7, 3, 3, 3, 6, 6, 6, 15, 15, 15, 0, 0},
    {11, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0, 0},
    {13, 13, 13, 13, 13, 13, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0},
    {11, 11, 11, 11, 11, 11, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0},
    {14, 14, 14, 14, 14, 14, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0}
};

struct TileRule {
    TileRule() = default;

    TileRule(const std::string& start, const std::string& end, uint32_t corner_exclusion_mask = 0, uint32_t blend_exclusion_mask = 0, uint32_t blend_inclusion_mask = 0, uint32_t corner_inclusion_mask = 0);

    [[nodiscard]] bool matches(uint32_t neighbors_mask, uint32_t blend_mask) const;
    [[nodiscard]] bool matches_relaxed(uint32_t neighbors_mask, uint32_t blend_mask) const;

public:
    TextureAtlasPos indexes[3];
private:
    uint32_t corner_exclusion_mask;
    uint32_t blend_exclusion_mask;
    uint32_t blend_inclusion_mask;
    uint32_t corner_inclusion_mask;
};

static struct {
    std::array<std::list<TileRule>, 16> base_rules;
    std::array<std::list<TileRule>, 16> blend_rules;
    std::array<std::list<TileRule>, 16> grass_rules;
} state;

TileRule::TileRule(const std::string& start, const std::string& end, uint32_t corner_exclusion_mask, uint32_t blend_exclusion_mask, uint32_t blend_inclusion_mask, uint32_t corner_inclusion_mask) :
    corner_exclusion_mask(corner_exclusion_mask),
    blend_exclusion_mask(blend_exclusion_mask << 16),
    blend_inclusion_mask(blend_inclusion_mask),
    corner_inclusion_mask(corner_inclusion_mask)
{
    const int y1 = start[0] - 'A';
    const int x1 = std::stoi(start.substr(1)) - 1;
    const int y2 = end[0] - 'A';
    const int x2 = std::stoi(end.substr(1)) - 1;
    const int y3 = y2 - (y2 - y1) / 2;
    const int x3 = x2 - (x2 - x1) / 2;
    this->indexes[0] = TextureAtlasPos(x1, y1);
    this->indexes[1] = TextureAtlasPos(x3, y3);
    this->indexes[2] = TextureAtlasPos(x2, y2);
}

bool TileRule::matches(uint32_t neighbors_mask, uint32_t blend_mask) const {
    const uint32_t upper_corner_inclusion_mask = (corner_inclusion_mask << 16) & 0x11110000;
    if ((upper_corner_inclusion_mask & neighbors_mask) != upper_corner_inclusion_mask) {
        return false;
    }
    const uint32_t upper_corner_exclusion_mask = (corner_exclusion_mask << 16) & 0x11110000;
    if (upper_corner_exclusion_mask != 0 && (upper_corner_exclusion_mask & neighbors_mask) != 0x00000000) {
        return false;
    }
    const uint32_t lower_blend_inclusion_mask = blend_inclusion_mask & 0x00001111;
    if (lower_blend_inclusion_mask != 0 && (lower_blend_inclusion_mask ^ (blend_mask & 0x00001111)) != 0x00000000) {
        return false;
    }
    const uint32_t upper_blend_corner_inclusion_mask = blend_inclusion_mask & 0x11110000;
    if ((upper_blend_corner_inclusion_mask & blend_mask) != upper_blend_corner_inclusion_mask) {
        return false;
    }
    const uint32_t lower_blend_exclusion_mask = blend_exclusion_mask & 0x00001111;
    if ((lower_blend_exclusion_mask & blend_mask) != 0x00000000) {
        return false;
    }
    const uint32_t upper_blend_corner_exclusion_mask = blend_exclusion_mask & 0x11110000;
    if (upper_blend_corner_exclusion_mask != 0 && (upper_blend_corner_exclusion_mask & blend_mask) != 0x00000000)
    {
        return false;
    }

    return true;
}

bool TileRule::matches_relaxed(uint32_t neighbors_mask, uint32_t blend_mask) const {
    uint32_t column = 0x00010000;
    for (int i = 0; i < 4; i++) {
        uint32_t upper_corner_inclusion_mask = (corner_inclusion_mask << 16) & column;
        uint32_t upper_blend_corner_inclusion_mask = blend_inclusion_mask & column;
        if ((upper_corner_inclusion_mask & upper_blend_corner_inclusion_mask) == 0x00000000) {
            if (upper_corner_inclusion_mask != 0 && (upper_corner_inclusion_mask & neighbors_mask) == 0) {
                return false;
            }
            if (upper_blend_corner_inclusion_mask != 0 && (upper_blend_corner_inclusion_mask & blend_mask) == 0) {
                return false;
            }
        } else {
            if ((upper_corner_inclusion_mask & neighbors_mask) == 0 && (upper_blend_corner_inclusion_mask & blend_mask) == 0) {
                return false;
            }
        }
        if (i < 3) {
            column <<= 4;
        }
    }
    uint32_t upper_corner_exclusion_mask = (corner_exclusion_mask << 16) & 0x11110000;
    if (upper_corner_exclusion_mask != 0 && (upper_corner_exclusion_mask & neighbors_mask) != 0x00000000) {
        return false;
    }
    uint32_t lower_blend_inclusion_mask = blend_inclusion_mask & 0x00001111;
    if (lower_blend_inclusion_mask != 0 && (lower_blend_inclusion_mask ^ (blend_mask & 0x00001111)) != 0x00000000) {
        return false;
    }
    uint32_t lower_blend_exclusion_mask = blend_exclusion_mask & 0x00001111;
    if ((lower_blend_exclusion_mask & blend_mask) != 0x00000000) {
        return false;
    }
    uint32_t upper_blend_corner_exclusion_mask = blend_exclusion_mask & 0x11110000;
    if (upper_blend_corner_exclusion_mask != 0 && (upper_blend_corner_exclusion_mask & blend_mask) != 0x00000000) {
        return false;
    }
    return true;
}

void init_tile_rules() {
    state.base_rules[0].emplace_front("D10", "D12");// None
    state.base_rules[1].emplace_front("A10", "C10");// Right
    state.base_rules[2].emplace_front("D7", "D9");  // Top
    state.base_rules[3].emplace_front("E1", "E5");  // Top, Right
    state.base_rules[4].emplace_front("A13", "C13");// Left
    state.base_rules[5].emplace_front("E7", "E9");  // Left, Right
    state.base_rules[6].emplace_front("E2", "E6");  // Left, Top
    state.base_rules[7].emplace_front("C2", "C4");  // Left, Top, Right
    state.base_rules[8].emplace_front("A7", "A9");  // Bottom
    state.base_rules[9].emplace_front("D1", "D5");  // Bottom, Right
    state.base_rules[10].emplace_front("A6", "C6"); // Bottom, Top
    state.base_rules[11].emplace_front("A1", "C1"); // Bottom, Top, Right
    state.base_rules[12].emplace_front("D2", "D6"); // Bottom, Left
    state.base_rules[13].emplace_front("A2", "A4"); // Bottom, Left, Right
    state.base_rules[14].emplace_front("A5", "C5"); // Bottom, Left, Top
    state.base_rules[15].emplace_front("B2", "B4"); // Bottom, Left, Top, Right
    state.base_rules[15].emplace_front("A11", "C11", 0x0110); // Bottom, Left, Top, Right, !TL, !BL
    state.base_rules[15].emplace_front("A12", "C12", 0x1001); // Bottom, Left, Top, Right, !TR, !BR
    state.base_rules[15].emplace_front("B7", "B9",   0x0011); // Bottom, Left, Top, Right, !TL, !TR
    state.base_rules[15].emplace_front("C7", "C9",   0x1100); // Bottom, Left, Top, Right, !BL, !BR

    state.blend_rules[0].emplace_front("N4", "N6",    0x0000, 0x0000, 0x00000001);
    state.blend_rules[0].emplace_front("I7", "K7",    0x0000, 0x0000, 0x00000010);
    state.blend_rules[0].emplace_front("N1", "N3",    0x0000, 0x0000, 0x00000100);
    state.blend_rules[0].emplace_front("F7", "H7",    0x0000, 0x0000, 0x00001000);
    state.blend_rules[0].emplace_front("L10", "L12",  0x0000, 0x0000, 0x00000101);
    state.blend_rules[0].emplace_front("M7", "O7",    0x0000, 0x0000, 0x00001010);
    state.blend_rules[0].emplace_front("L7", "L9",    0x0000, 0x0000, 0x00001111);
    state.blend_rules[1].emplace_front("O1", "O3",    0x0000, 0x0000, 0x00000100);
    state.blend_rules[2].emplace_front("F8", "H8",    0x0000, 0x0000, 0x00001000);
    state.blend_rules[3].emplace_front("M1", "M3",    0x0000, 0x0000, 0x00000100);
    state.blend_rules[3].emplace_front("F5", "H5",    0x0000, 0x0000, 0x00001000);
    state.blend_rules[3].emplace_front("G3", "K3",    0x0000, 0x0000, 0x00001100);
    state.blend_rules[4].emplace_front("O4", "O6",    0x0000, 0x0000, 0x00000001);
    state.blend_rules[5].emplace_front("K9", "K11",   0x0000, 0x0000, 0x00001010);
    state.blend_rules[6].emplace_front("M4", "M6",    0x0000, 0x0000, 0x00000001);
    state.blend_rules[6].emplace_front("F6", "H6",    0x0000, 0x0000, 0x00001000);
    state.blend_rules[6].emplace_front("G4", "K4",    0x0000, 0x0000, 0x00001001);
    state.blend_rules[7].emplace_front("F9", "F11",   0x0000, 0x0000, 0x00001000);
    state.blend_rules[8].emplace_front("I8", "K8",    0x0000, 0x0000, 0x00000010);
    state.blend_rules[9].emplace_front("I5", "K5",    0x0000, 0x0000, 0x00000010);
    state.blend_rules[9].emplace_front("L1", "L3",    0x0000, 0x0000, 0x00000100);
    state.blend_rules[9].emplace_front("F3", "J3",    0x0000, 0x0000, 0x00000110);
    state.blend_rules[10].emplace_front("H11", "J11", 0x0000, 0x0000, 0x00000101);
    state.blend_rules[11].emplace_front("H10", "J10", 0x0000, 0x0000, 0x00000100);
    state.blend_rules[12].emplace_front("L4", "L6",   0x0000, 0x0000, 0x00000001);
    state.blend_rules[12].emplace_front("I6", "K6",   0x0000, 0x0000, 0x00000010);
    state.blend_rules[12].emplace_front("F4", "J4",   0x0000, 0x0000, 0x00000011);
    state.blend_rules[13].emplace_front("G9", "G11",  0x0000, 0x0000, 0x00000010);
    state.blend_rules[14].emplace_front("H9", "J9",   0x0000, 0x0000, 0x00000001);

    for (int i = 0; i < 16; i++) {
        state.grass_rules[i] = std::list<TileRule>(state.blend_rules[i]);
        for (size_t j = 0; j < state.base_rules[i].size(); j++) {
            state.blend_rules[i].push_back(list_at(state.base_rules[i], j));
        }
    }

    state.blend_rules[1].emplace_front("F13", "H13",  0x0000, 0x0000, 0x00001110);
    state.blend_rules[2].emplace_front("I12", "K12",  0x0000, 0x0000, 0x00001101);
    state.blend_rules[4].emplace_front("I13", "K13",  0x0000, 0x0000, 0x00001011);
    state.blend_rules[5].emplace_front("B14", "B16",  0x0000, 0x0000, 0x00000010);
    state.blend_rules[5].emplace_front("A14", "A16",  0x0000, 0x0000, 0x00001000);
    state.blend_rules[8].emplace_front("F12", "H12",  0x0000, 0x0000, 0x00000111);
    state.blend_rules[10].emplace_front("C14", "C16", 0x0000, 0x0000, 0x00000001);
    state.blend_rules[10].emplace_front("D14", "D16", 0x0000, 0x0000, 0x00000100);
    state.blend_rules[15].emplace_front("G1", "K1",   0x0000, 0x0000, 0x00010000);
    state.blend_rules[15].emplace_front("G2", "K2",   0x0000, 0x0000, 0x00100000);
    state.blend_rules[15].emplace_front("F2", "J2",   0x0000, 0x0000, 0x01000000);
    state.blend_rules[15].emplace_front("F1", "J1",   0x0000, 0x0000, 0x10000000);

    state.grass_rules[7].pop_front();
    state.grass_rules[11].pop_front();
    state.grass_rules[13].pop_front();
    state.grass_rules[14].pop_front();
    state.grass_rules[3].pop_front();
    state.grass_rules[6].pop_front();
    state.grass_rules[9].pop_front();
    state.grass_rules[12].pop_front();

    state.grass_rules[1].emplace_back("P1", "R1",    0x0000, 0x00000000, 0x00001010, 0x0000);
    state.grass_rules[1].emplace_back("R9", "R11",   0x0000, 0x00000000, 0x00001110, 0x0000);
    state.grass_rules[2].emplace_back("Q3", "Q5",    0x0000, 0x00000000, 0x00000101, 0x0000);
    state.grass_rules[2].emplace_back("Q12", "Q14",  0x0000, 0x00000000, 0x00001101, 0x0000);
    state.grass_rules[3].emplace_back("Q6", "Q8",    0x0001, 0x00010000, 0x00000000, 0x0000);
    state.grass_rules[4].emplace_back("P2", "R2",    0x0000, 0x00000000, 0x00001010, 0x0000);
    state.grass_rules[4].emplace_back("R12", "R14",  0x0000, 0x00000000, 0x00001011, 0x0000);
    state.grass_rules[6].emplace_back("Q9", "Q11",   0x0010, 0x00100000, 0x00000000, 0x0000);
    state.grass_rules[7].emplace_back("O9", "O15",   0x0011, 0x00111000, 0x00000000, 0x0000);
    state.grass_rules[7].emplace_back("T1", "T3",    0x0001, 0x00011000, 0x00100000, 0x0010);
    state.grass_rules[7].emplace_back("T4", "T6",    0x0010, 0x00101000, 0x00010000, 0x0001);
    state.grass_rules[8].emplace_back("P3", "P5",    0x0000, 0x00000000, 0x00000101, 0x0000);
    state.grass_rules[8].emplace_back("P12", "P14",  0x0000, 0x00000000, 0x00000111, 0x0000);
    state.grass_rules[9].emplace_back("P6", "P8",    0x1000, 0x10000000, 0x00000000, 0x0000);
    state.grass_rules[11].emplace_back("N8", "N14",  0x1001, 0x10010100, 0x00000000, 0x0000);
    state.grass_rules[11].emplace_back("U1", "U3",   0x0001, 0x00010100, 0x10000000, 0x1000);
    state.grass_rules[11].emplace_back("U4", "U6",   0x1000, 0x10000100, 0x00010000, 0x0001);
    state.grass_rules[12].emplace_back("P9", "P11",  0x0100, 0x01000000, 0x00000000, 0x0000);
    state.grass_rules[13].emplace_back("M9", "M15",  0x1100, 0x11000010, 0x00000000, 0x0000);
    state.grass_rules[13].emplace_back("S1", "S3",   0x1000, 0x10000010, 0x01000000, 0x0100);
    state.grass_rules[13].emplace_back("S4", "S6",   0x0100, 0x01000010, 0x10000000, 0x1000);
    state.grass_rules[14].emplace_back("N10", "N16", 0x0110, 0x01100001, 0x00000000, 0x0000);
    state.grass_rules[14].emplace_back("V1", "V3",   0x0100, 0x01000001, 0x00100000, 0x0010);
    state.grass_rules[14].emplace_back("V4", "V6",   0x0010, 0x00100001, 0x01000000, 0x0100);
    state.grass_rules[15].emplace_back("N9", "N15",  0x1111, 0x11110000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("S7", "S9",   0x0111, 0x01110000, 0x10000000, 0x0000);
    state.grass_rules[15].emplace_back("T7", "T9",   0x1110, 0x11100000, 0x00010000, 0x0000);
    state.grass_rules[15].emplace_back("U7", "U9",   0x1011, 0x10110000, 0x01000000, 0x0000);
    state.grass_rules[15].emplace_back("V7", "V9",   0x1101, 0x11010000, 0x00100000, 0x0000);
    state.grass_rules[15].emplace_back("R3", "R5",   0x1010, 0x10100000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("R6", "R8",   0x0101, 0x01010000, 0x00000000, 0x0000);

    state.grass_rules[0].emplace_front("P2", "R2",   0x0000, 0x00000001, 0x00001110, 0x0000);
    state.grass_rules[0].emplace_front("P3", "P5",   0x0000, 0x00000010, 0x00001101, 0x0000);
    state.grass_rules[0].emplace_front("P1", "R1",   0x0000, 0x00000100, 0x00001011, 0x0000);
    state.grass_rules[0].emplace_front("Q3", "Q5",   0x0000, 0x00001000, 0x00000111, 0x0000);
    state.grass_rules[1].emplace_front("M1", "M3",   0x0000, 0x00001001, 0x00000110, 0x0000);
    state.grass_rules[1].emplace_front("L1", "L3",   0x0000, 0x00000011, 0x00001100, 0x0000);
    state.grass_rules[2].emplace_front("F5", "H5",   0x0000, 0x00000110, 0x00001001, 0x0000);
    state.grass_rules[2].emplace_front("F6", "H6",   0x0000, 0x00000011, 0x00001100, 0x0000);
    state.grass_rules[3].emplace_front("G3", "K3",   0x0001, 0x00010000, 0x00001100, 0x0000);
    state.grass_rules[4].emplace_front("M4", "M6",   0x0000, 0x00001100, 0x00000011, 0x0000);
    state.grass_rules[4].emplace_front("L4", "L6",   0x0000, 0x00000110, 0x00001001, 0x0000);
    state.grass_rules[6].emplace_front("G4", "K4",   0x0010, 0x00100000, 0x00001001, 0x0000);
    state.grass_rules[7].emplace_back("B7", "B9",    0x0011, 0x00110000, 0x00001000, 0x0000);
    state.grass_rules[7].emplace_back("G3", "K3",    0x0001, 0x00010000, 0x00001000, 0x0000);
    state.grass_rules[7].emplace_back("G4", "K4",    0x0010, 0x00100000, 0x00001000, 0x0000);
    state.grass_rules[8].emplace_front("I5", "K5",   0x0000, 0x00001100, 0x00000011, 0x0000);
    state.grass_rules[8].emplace_front("I6", "K6",   0x0000, 0x00001001, 0x00000110, 0x0000);
    state.grass_rules[9].emplace_front("F3", "J3",   0x1000, 0x10000000, 0x00000110, 0x0000);
    state.grass_rules[11].emplace_back("A12", "C12", 0x1001, 0x10010000, 0x00000100, 0x0000);
    state.grass_rules[11].emplace_back("G3", "K3",   0x0001, 0x00010000, 0x00000100, 0x0000);
    state.grass_rules[11].emplace_back("F3", "J3",   0x1000, 0x10000000, 0x00000100, 0x0000);
    state.grass_rules[12].emplace_front("F4", "J4",  0x0100, 0x01000000, 0x00000011, 0x0000);
    state.grass_rules[13].emplace_back("C7", "C9",   0x1100, 0x11000000, 0x00000010, 0x0000);
    state.grass_rules[13].emplace_back("F4", "J4",   0x0100, 0x01000000, 0x00000010, 0x0000);
    state.grass_rules[13].emplace_back("F3", "J3",   0x1000, 0x10000000, 0x00000010, 0x0000);
    state.grass_rules[14].emplace_back("A11", "C11", 0x0110, 0x01100000, 0x00000001, 0x0000);
    state.grass_rules[14].emplace_back("G4", "K4",   0x0010, 0x00100000, 0x00000001, 0x0000);
    state.grass_rules[14].emplace_back("F4", "J4",   0x0100, 0x01000000, 0x00000001, 0x0000);
    state.grass_rules[15].emplace_back("B7", "B9",   0x0011, 0x00110000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("C7", "C9",   0x1100, 0x11000000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("A11", "C11", 0x0110, 0x01100000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("A12", "C12", 0x1001, 0x10010000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("G3", "K3",   0x0001, 0x00010000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("G4", "K4",   0x0010, 0x00100000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("F4", "J4",   0x0100, 0x01000000, 0x00000000, 0x0000);
    state.grass_rules[15].emplace_back("F3", "J3",   0x1000, 0x10000000, 0x00000000, 0x0000);
}

void update_block_sprite_index(Block& tile, const Neighbors<Block>& neighbors) {
    if (tile.type == BlockType::Torch) {
        AnchorData anchor = block_anchor(tile.type);

        const auto to_type = [](Block tile) {
            return tile.type;
        };

        uint16_t index;
        if (check_anchor_vertical(map(neighbors.bottom, to_type), anchor.bottom)) index = 0;
        else if (check_anchor_horizontal(map(neighbors.left, to_type), anchor.left)) index = 1;
        else if (check_anchor_horizontal(map(neighbors.right, to_type), anchor.right)) index = 2;
        else index = 0;

        tile.atlas_pos = TextureAtlasPos(index, 0);

        return;
    } else if (tile.type == BlockType::Tree) {
        switch (tile.data.tree.frame) {
            case TreeFrameType::Trunk:
                tile.atlas_pos = TextureAtlasPos(0, tile.variant);
            break;
            case TreeFrameType::TrunkBranchLeft:
                tile.atlas_pos = TextureAtlasPos(4, tile.variant);
            break;
            case TreeFrameType::TrunkBranchRight:
                tile.atlas_pos = TextureAtlasPos(3, 3 + tile.variant);
            break;
            case TreeFrameType::TrunkBranchBoth:
                tile.atlas_pos = TextureAtlasPos(5, 3 + tile.variant);
            break;
            case TreeFrameType::TrunkHollowLeft:
                tile.atlas_pos = TextureAtlasPos(0, 3 + tile.variant);
            break;
            case TreeFrameType::TrunkHollowRight:
                tile.atlas_pos = TextureAtlasPos(1, tile.variant);
            break;
            case TreeFrameType::TrunkBranchCollarLeft:
                tile.atlas_pos = TextureAtlasPos(1, 3 + tile.variant);
            break;
            case TreeFrameType::TrunkBranchCollarRight:
                tile.atlas_pos = TextureAtlasPos(2, 3 + tile.variant);
            break;
            case TreeFrameType::RootLeft:
                tile.atlas_pos = TextureAtlasPos(2, 6 + tile.variant);
            break;
            case TreeFrameType::RootRight:
                tile.atlas_pos = TextureAtlasPos(1, 6 + tile.variant);
            break;
            case TreeFrameType::BaseBoth:
                tile.atlas_pos = TextureAtlasPos(4, 6 + tile.variant);
            break;
            case TreeFrameType::BaseLeft:
                tile.atlas_pos = TextureAtlasPos(3, 6 + tile.variant);
            break;
            case TreeFrameType::BaseRight:
                tile.atlas_pos = TextureAtlasPos(0, 6 + tile.variant);
            break;
            case TreeFrameType::BranchLeftBare:
                tile.atlas_pos = TextureAtlasPos(3, tile.variant);
            break;
            case TreeFrameType::BranchRightBare:
                tile.atlas_pos = TextureAtlasPos(4, 3 + tile.variant);
            break;
            case TreeFrameType::BranchLeftLeaves:
                tile.atlas_pos = TextureAtlasPos(0, tile.variant);
            break;
            case TreeFrameType::BranchRightLeaves:
                tile.atlas_pos = TextureAtlasPos(1, tile.variant);
            break;
            case TreeFrameType::TopBare:
                tile.atlas_pos = TextureAtlasPos(5, tile.variant);
            break;
            case TreeFrameType::TopLeaves:
                tile.atlas_pos = TextureAtlasPos(tile.variant, 0);
            break;
            case TreeFrameType::TopBareJagged:
                tile.atlas_pos = TextureAtlasPos(0, 9 + tile.variant);
            break;
            case TreeFrameType::StumpRootLeft:
                tile.atlas_pos = TextureAtlasPos(7, tile.variant);
            break;
            case TreeFrameType::StumpRootRight:
                tile.atlas_pos = TextureAtlasPos(7, 3 + tile.variant);
            break;
            case TreeFrameType::StumpRootBoth:
                tile.atlas_pos = TextureAtlasPos(7, 6 + tile.variant);
            break;
        }

        return;
    }

    if (tile.is_merged) return;

    uint32_t neighbors_mask = 0;
    uint32_t blend_mask = 0;

    if (block_is_stone(tile.type)) {
        neighbors_mask |= (neighbors.right.has_value()        && block_is_stone(neighbors.right->type))        ? 0x00000001 : 0x0;
        neighbors_mask |= (neighbors.top.has_value()          && block_is_stone(neighbors.top->type))          ? 0x00000010 : 0x0;
        neighbors_mask |= (neighbors.left.has_value()         && block_is_stone(neighbors.left->type))         ? 0x00000100 : 0x0;
        neighbors_mask |= (neighbors.bottom.has_value()       && block_is_stone(neighbors.bottom->type))       ? 0x00001000 : 0x0;
        neighbors_mask |= (neighbors.top_right.has_value()    && block_is_stone(neighbors.top_right->type))    ? 0x00010000 : 0x0;
        neighbors_mask |= (neighbors.top_left.has_value()     && block_is_stone(neighbors.top_left->type))     ? 0x00100000 : 0x0;
        neighbors_mask |= (neighbors.bottom_left.has_value()  && block_is_stone(neighbors.bottom_left->type))  ? 0x01000000 : 0x0;
        neighbors_mask |= (neighbors.bottom_right.has_value() && block_is_stone(neighbors.bottom_right->type)) ? 0x10000000 : 0x0;
    } else {
        neighbors_mask |= (neighbors.right.has_value()        && block_merges_with(tile.type, neighbors.right->type))        ? 0x00000001 : 0x0;
        neighbors_mask |= (neighbors.top.has_value()          && block_merges_with(tile.type, neighbors.top->type))          ? 0x00000010 : 0x0;
        neighbors_mask |= (neighbors.left.has_value()         && block_merges_with(tile.type, neighbors.left->type))         ? 0x00000100 : 0x0;
        neighbors_mask |= (neighbors.bottom.has_value()       && block_merges_with(tile.type, neighbors.bottom->type))       ? 0x00001000 : 0x0;
        neighbors_mask |= (neighbors.top_right.has_value()    && block_merges_with(tile.type, neighbors.top_right->type))    ? 0x00010000 : 0x0;
        neighbors_mask |= (neighbors.top_left.has_value()     && block_merges_with(tile.type, neighbors.top_left->type))     ? 0x00100000 : 0x0;
        neighbors_mask |= (neighbors.bottom_left.has_value()  && block_merges_with(tile.type, neighbors.bottom_left->type))  ? 0x01000000 : 0x0;
        neighbors_mask |= (neighbors.bottom_right.has_value() && block_merges_with(tile.type, neighbors.bottom_right->type)) ? 0x10000000 : 0x0;

        neighbors_mask |= (neighbors.right.has_value()        && neighbors.right->type        == tile.type) ? 0x00000001 : 0x0;
        neighbors_mask |= (neighbors.top.has_value()          && neighbors.top->type          == tile.type) ? 0x00000010 : 0x0;
        neighbors_mask |= (neighbors.left.has_value()         && neighbors.left->type         == tile.type) ? 0x00000100 : 0x0;
        neighbors_mask |= (neighbors.bottom.has_value()       && neighbors.bottom->type       == tile.type) ? 0x00001000 : 0x0;
        neighbors_mask |= (neighbors.top_right.has_value()    && neighbors.top_right->type    == tile.type) ? 0x00010000 : 0x0;
        neighbors_mask |= (neighbors.top_left.has_value()     && neighbors.top_left->type     == tile.type) ? 0x00100000 : 0x0;
        neighbors_mask |= (neighbors.bottom_left.has_value()  && neighbors.bottom_left->type  == tile.type) ? 0x01000000 : 0x0;
        neighbors_mask |= (neighbors.bottom_right.has_value() && neighbors.bottom_right->type == tile.type) ? 0x10000000 : 0x0;
    }

    if (!tile.is_merged) {
        bool check_ready = true;
        check_ready &= (!neighbors.right.has_value()  || !block_merges_with(tile.type, neighbors.right->type))  ? true : (neighbors.right->merge_id  != 0xFF);
        check_ready &= (!neighbors.top.has_value()    || !block_merges_with(tile.type, neighbors.top->type))    ? true : (neighbors.top->merge_id    != 0xFF);
        check_ready &= (!neighbors.left.has_value()   || !block_merges_with(tile.type, neighbors.left->type))   ? true : (neighbors.left->merge_id   != 0xFF);
        check_ready &= (!neighbors.bottom.has_value() || !block_merges_with(tile.type, neighbors.bottom->type)) ? true : (neighbors.bottom->merge_id != 0xFF);

        if (check_ready) {
            neighbors_mask &= 0x11111110 | ((!neighbors.right.has_value()  || !block_merges_with(tile.type, neighbors.right->type))  ? 0x00000001 : ((neighbors.right->merge_id  & 0x04) >> 2));
            neighbors_mask &= 0x11111101 | ((!neighbors.top.has_value()    || !block_merges_with(tile.type, neighbors.top->type))    ? 0x00000010 : ((neighbors.top->merge_id    & 0x08) << 1));
            neighbors_mask &= 0x11111011 | ((!neighbors.left.has_value()   || !block_merges_with(tile.type, neighbors.left->type))   ? 0x00000100 : ((neighbors.left->merge_id   & 0x01) << 8));
            neighbors_mask &= 0x11110111 | ((!neighbors.bottom.has_value() || !block_merges_with(tile.type, neighbors.bottom->type)) ? 0x00001000 : ((neighbors.bottom->merge_id & 0x02) << 11));
            tile.is_merged = true;
        }
    }

    uint32_t bucket_id = ((neighbors_mask & 0x00001000) >> 9) + ((neighbors_mask & 0x00000100) >> 6) + ((neighbors_mask & 0x00000010) >> 3) + (neighbors_mask & 0x00000001);

    std::optional<BlockType> merge_with = block_merge_with(tile.type);

    TextureAtlasPos index;

    if (tile.type == BlockType::Grass) {
        bool found = false;
        for (const TileRule& rule : state.grass_rules[bucket_id]) {
            if (rule.matches_relaxed(neighbors_mask, blend_mask)) {
                index = rule.indexes[tile.variant];
                found = true;
                break;
            }
        }

        if (!found) {
            neighbors_mask |= blend_mask;
            bucket_id = ((neighbors_mask & 0x00001000) >> 9) + ((neighbors_mask & 0x00000100) >> 6) + ((neighbors_mask & 0x00000010) >> 3) + (neighbors_mask & 0x00000001);
            for (const TileRule& rule : state.base_rules[bucket_id]) {
                if (rule.matches(neighbors_mask, blend_mask)) {
                    index = rule.indexes[tile.variant];
                    break;
                }
            }
        }
    } else if (merge_with.has_value()) {
        blend_mask |= (neighbors.right.has_value()        && neighbors.right->type        == merge_with.value()) ? 0x00000001 : 0x0;
        blend_mask |= (neighbors.top.has_value()          && neighbors.top->type          == merge_with.value()) ? 0x00000010 : 0x0;
        blend_mask |= (neighbors.left.has_value()         && neighbors.left->type         == merge_with.value()) ? 0x00000100 : 0x0;
        blend_mask |= (neighbors.bottom.has_value()       && neighbors.bottom->type       == merge_with.value()) ? 0x00001000 : 0x0;
        blend_mask |= (neighbors.top_right.has_value()    && neighbors.top_right->type    == merge_with.value()) ? 0x00010000 : 0x0;
        blend_mask |= (neighbors.top_left.has_value()     && neighbors.top_left->type     == merge_with.value()) ? 0x00100000 : 0x0;
        blend_mask |= (neighbors.bottom_left.has_value()  && neighbors.bottom_left->type  == merge_with.value()) ? 0x01000000 : 0x0;
        blend_mask |= (neighbors.bottom_right.has_value() && neighbors.bottom_right->type == merge_with.value()) ? 0x10000000 : 0x0;

        for (const TileRule& rule : state.blend_rules[bucket_id]) {
            if (rule.matches(neighbors_mask, blend_mask)) {
                index = rule.indexes[tile.variant];
                break;
            }
        }
    } else {
        for (const TileRule& rule : state.base_rules[bucket_id]) {
            if (rule.matches(neighbors_mask, blend_mask)) {
                index = rule.indexes[tile.variant];
                break;
            }
        }
    }

    tile.merge_id = MERGE_VALIDATION[index.y][index.x];
    tile.atlas_pos = index;
}

void update_wall_sprite_index(Wall& wall, const Neighbors<Wall>& neighbors) {
    uint32_t neighbors_mask = 0;
    uint32_t blend_mask = 0;

    neighbors_mask |= (neighbors.right.has_value()        && neighbors.right->type        == wall.type) ? 0x00000001 : 0x0;
    neighbors_mask |= (neighbors.top.has_value()          && neighbors.top->type          == wall.type) ? 0x00000010 : 0x0;
    neighbors_mask |= (neighbors.left.has_value()         && neighbors.left->type         == wall.type) ? 0x00000100 : 0x0;
    neighbors_mask |= (neighbors.bottom.has_value()       && neighbors.bottom->type       == wall.type) ? 0x00001000 : 0x0;
    neighbors_mask |= (neighbors.top_right.has_value()    && neighbors.top_right->type    == wall.type) ? 0x00010000 : 0x0;
    neighbors_mask |= (neighbors.top_left.has_value()     && neighbors.top_left->type     == wall.type) ? 0x00100000 : 0x0;
    neighbors_mask |= (neighbors.bottom_left.has_value()  && neighbors.bottom_left->type  == wall.type) ? 0x01000000 : 0x0;
    neighbors_mask |= (neighbors.bottom_right.has_value() && neighbors.bottom_right->type == wall.type) ? 0x10000000 : 0x0;

    uint32_t bucket_id = ((neighbors_mask & 0x00001000) >> 9) + ((neighbors_mask & 0x00000100) >> 6) + ((neighbors_mask & 0x00000010) >> 3) + (neighbors_mask & 0x00000001);

    TextureAtlasPos index;
    for (const TileRule& rule : state.base_rules[bucket_id]) {
        if (rule.matches(neighbors_mask, blend_mask)) {
            index = rule.indexes[wall.variant];
            break;
        }
    }

    wall.atlas_pos = index;
}

void reset_tiles(const TilePos& initial_pos, World& world) {
    for (int y = initial_pos.y - 3; y < initial_pos.y + 3; ++y) {
        for (int x = initial_pos.x - 3; x < initial_pos.x + 3; ++x) {
            auto pos = TilePos(x, y);
            Block* tile = world.get_block_mut(pos);

            if (tile != nullptr) {
                tile->is_merged = false;
                tile->merge_id = 0xFF;
            }
        }
    }
}