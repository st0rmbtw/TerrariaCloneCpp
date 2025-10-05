#include "load.hpp"

#include <vector>

struct TileData {
    uint16_t type;
    uint16_t u;
    uint16_t v;
    bool is_active;
    bool inactive;
    bool actuator;
    uint8_t tile_color;
    uint8_t wall;
    uint8_t wall_color;
    uint8_t liquid_type;
    uint8_t liquid_amount;
    bool wire_red;
    bool wire_blue;
    bool wire_yellow;
    bool wire_green;
};

static TileData deserialize_tile_data_v2(BufferedReader& reader, const std::vector<bool>& frame_important, int32_t version, int& rle) {
    TileData tile;

    int tileType = -1;
    uint8_t header4 = 0;
    uint8_t header3 = 0;
    uint8_t header2 = 0;
    uint8_t header1 = reader.read<uint8_t>();

    bool hasHeader2 = false;
    bool hasHeader3 = false;
    bool hasHeader4 = false;

    // check bit[0] to see if header2 has data
    if ((header1 & 0b0000'0001) == 0b0000'0001)
    {
        hasHeader2 = true;
        header2 = reader.read<uint8_t>();
    }

    // check bit[0] to see if header3 has data
    if (hasHeader2 && (header2 & 0b0000'0001) == 0b0000'0001)
    {
        hasHeader3 = true;
        header3 = reader.read<uint8_t>();
    }

    if (version >= 269) // 1.4.4+ 
    {
        // check bit[0] to see if header4 has data
        if (hasHeader3 && (header3 & 0b0000'0001) == 0b0000'0001)
        {
            hasHeader4 = true;
            header4 = reader.read<uint8_t>();
        }
    }

    // check bit[1] for active tile
    bool is_active = (header1 & 0b0000'0010) == 0b0000'0010;

    if (is_active)
    {
        tile.is_active = is_active;
        // read tile type

        if ((header1 & 0b0010'0000) != 0b0010'0000) // check bit[5] to see if tile is byte or little endian int16
        {
            // tile is byte
            tileType = reader.read<uint8_t>();
        }
        else
        {
            // tile is little endian int16
            uint8_t lowerByte = reader.read<uint8_t>();
            tileType = reader.read<uint8_t>();
            tileType = tileType << 8 | lowerByte;
        }
        tile.type = (ushort)tileType; // convert type to ushort after bit operations

        // read frame UV coords
        if (!frame_important[tileType])
        {
            tile.u = 0;//-1;
            tile.v = 0;//-1;
        }
        else
        {
            // read UV coords
            tile.u = reader.read<uint16_t>();
            tile.v = reader.read<uint16_t>();

            // reset timers
            // if (tile.type == TileType.Timer)
            // {
            //     tile.V = 0;
            // }

        }

        // check header3 bit[3] for tile color
        if ((header3 & 0b0000'1000) == 0b0000'1000)
        {
            tile.tile_color = reader.read<uint8_t>();
        }
    }

    // Read Walls
    if ((header1 & 0b0000'0100) == 0b0000'0100) // check bit[3] bit for active wall
    {
        tile.wall = reader.read<uint8_t>();


        // check bit[4] of header3 to see if there is a wall color
        if ((header3 & 0b0001'0000) == 0b0001'0000)
        {
            tile.wall_color = reader.read<uint8_t>();
        }
    }

    // check for liquids, grab the bit[3] and bit[4], shift them to the 0 and 1 bits
    uint8_t liquidType = (header1 & 0b0001'1000) >> 3;
    if (liquidType != 0)
    {
        tile.liquid_amount = reader.read<uint8_t>();
        tile.liquid_type = liquidType; // water, lava, honey

        // shimmer (v 1.4.4 +)
        if (version >= 269 && (header3 & 0b1000'0000) == 0b1000'0000)
        {
            // tile.liquid_type = LiquidType::Shimmer;
        }

    }

    // check if we have data in header2 other than just telling us we have header3
    if (header2 > 1)
    {
        // check bit[1] for red wire
        if ((header2 & 0b0000'0010) == 0b0000'0010)
        {
            tile.wire_red = true;
        }
        // check bit[2] for blue wire
        if ((header2 & 0b0000'0100) == 0b0000'0100)
        {
            tile.wire_blue = true;
        }
        // check bit[3] for green wire
        if ((header2 & 0b0000'1000) == 0b0000'1000)
        {
            tile.wire_green = true;
        }

        // grab bits[4, 5, 6] and shift 4 places to 0,1,2. This byte is our brick style
        // byte brickStyle = (byte)((header2 & 0b0111'0000) >> 4);
        // if (brickStyle != 0 && WorldConfiguration.TileProperties.Count > tile.Type && WorldConfiguration.TileProperties[tile.Type].HasSlopes)
        // {
        //     tile.BrickStyle = (BrickStyle)brickStyle;
        // }
    }

    // check if we have data in header3 to process
    if (header3 > 1)
    {
        // check bit[1] for actuator
        if ((header3 & 0b0000'0010) == 0b0000'0010)
        {
            tile.actuator = true;
        }

        // check bit[2] for inactive due to actuator
        if ((header3 & 0b0000'0100) == 0b0000'0100)
        {
            tile.inactive = true;
        }

        if ((header3 & 0b0010'0000) == 0b0010'0000)
        {
            tile.wire_yellow = true;
        }

        if (version >= 222)
        {
            if ((header3 & 0b0100'0000) == 0b0100'0000)
            {
                tile.wall = (ushort)(reader.read<uint8_t>() << 8 | tile.wall);
            }
        }
    }

    // if (version >= 269 && header4 > 1)
    // {
    //     if ((header4 & 0b0000'0010) == 0b0000'0010)
    //     {
    //         tile.InvisibleBlock = true;
    //     }
    //     if ((header4 & 0b0000'0100) == 0b0000'0100)
    //     {
    //         tile.InvisibleWall = true;
    //     }
    //     if ((header4 & 0b0000'1000) == 0b0000'1000)
    //     {
    //         tile.FullBrightBlock = true;
    //     }
    //     if ((header4 & 0b0001'0000) == 0b0001'0000)
    //     {
    //         tile.FullBrightWall = true;
    //     }
    // }

    // get bit[6,7] shift to 0,1 for RLE encoding type
    // 0 = no RLE compression
    // 1 = byte RLE counter
    // 2 = int16 RLE counter
    // 3 = not implemented, assume int16
    uint8_t rleStorageType = (header1 & 192) >> 6;

    if (rleStorageType == 0) {
        rle = 0;
    } else if (rleStorageType == 1) {
        rle = reader.read<uint8_t>();
    } else {
        rle = reader.read<uint16_t>();
    }
    return tile;
}

static std::vector<TileData> read_tile_data_v2(BufferedReader& reader, const WorldHeader& header) {
    std::vector<TileData> tiles(header.width * header.height, TileData{});

    int rle = 0;
    for (int x = 0; x < header.width; ++x) {
        for (int y = 0; y < header.height; y++) {
            const int index = y * header.width + x;

            TileData tile = deserialize_tile_data_v2(reader, header.tile_frame_important, header.version, rle);
            tiles[index] = tile;

            while (rle > 0) {
                y++;
                if (y >= header.height) {
                    break;
                }

                const int index = y * header.width + x;
                tiles[index] = tile;
                rle--;
            }
        }
    }

    return tiles;
}

static std::vector<bool> read_bit_array(BufferedReader& reader) {
    // get the number of bits
    int length = reader.read<int16_t>();

    // read the bit data
    std::vector<bool> booleans(length, false);

    uint8_t data = 0;
    uint8_t bitmask = 128;
    for (int i = 0; i < length; i++) {
        // If we read the last bit mask (B1000000 = 0x80 = 128), read the next byte from the stream and start the mask over.
        // Otherwise, keep incrementing the mask to get the next bit.
        if (bitmask != 128) {
            bitmask = bitmask << 1;
        } else {
            data = reader.read<uint8_t>();
            bitmask = 1;
        }

        // Check the mask, if it is set then set the current boolean to true
        if ((data & bitmask) == bitmask) {
            booleans[i] = true;
        }
    }

    return booleans;
}

void load_world(WorldData& world, const std::filesystem::path& path) {
    BufferedReader reader(std::ifstream(path, std::ios::binary));
    WorldHeader header;
    read_world_header(header, reader);
    std::vector<TileData> tile_data;
    
    if (header.version >= 88) {
        tile_data = read_tile_data_v2(reader, header);
    } else {
        // TODO
        SGE_UNREACHABLE();
    }

    const sge::IRect area = sge::IRect::from_corners(glm::vec2(0), glm::ivec2(header.width, header.height));
    const sge::IRect playable_area = area;

    const int surface_level = playable_area.min.y;
    const int underground_level = header.surface_layer;
    const int cavern_level = header.rock_layer;
    const int dirt_height = 50;

    const Layers layers = {
        .surface = surface_level,
        .underground = underground_level,
        .cavern = cavern_level,
        .dirt_height = dirt_height
    };

    world.blocks = new std::optional<Block>[static_cast<size_t>(area.width() * area.height())];
    world.walls = new std::optional<Wall>[static_cast<size_t>(area.width() * area.height())];
    world.lightmap = LightMap(area.width(), area.height());
    world.playable_area = playable_area;
    world.area = area;
    world.layers = layers;

    world.spawn_point = glm::ivec2(header.spawn_point);

    for (int y = 0; y < header.height; ++y) {
        for (int x = 0; x < header.width; ++x) {
            const int index = y * header.width + x;

            const TileData& tile = tile_data[index];

            if (tile.is_active) {
                switch (tile.type) {
                    case 0:
                    case 59: // Mud
                    case 123: // Silt
                        world.blocks[index] = Block(BlockType::Dirt);
                        break;
                    case 1:
                    case 25: // Ebonstone
                        world.blocks[index] = Block(BlockType::Stone);
                        break;
                    case 2:
                    case 60: // Jungle grass
                    case 23: // Corrupt grass
                    case 199: // Crimson grass
                        world.blocks[index] = Block(BlockType::Grass);
                        break;
                    case 4:
                        world.blocks[index] = Block(BlockType::Torch);
                        break;
                    case 5:
                        world.blocks[index] = Block(BlockType::Tree);
                        break;
                    case 30:
                        world.blocks[index] = Block(BlockType::Wood);
                        break;
                }
            }

            if (tile.wall > 0) {
                switch (tile.wall) {
                    case 1:
                        world.walls[index] = Wall(WallType::StoneWall);
                        break;
                    case 2:
                        world.walls[index] = Wall(WallType::DirtWall);
                        break;
                    case 4:
                        world.walls[index] = Wall(WallType::WoodWall);
                        break;
                }
            }
        }
    }

    world.update_tiles_sprites();
    memset(world.lightmap.colors, 0xFF, world.lightmap.width * world.lightmap.height * sizeof(Color));
    // world.lightmap_init_area(area);
    // world.lightmap_blur_area(area);
}

static inline std::string read_string(BufferedReader& stream) {
    const uint8_t length = stream.read<uint8_t>();
    std::string str(length, '\0');
    stream.read(str.data(), length);
    return str;
}

void read_world_header(WorldHeader& header, BufferedReader& stream) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"

    const auto version = stream.read<int32_t>();
    header.version = version;

    if (version >= 135) {
        // Read "relogic" string
        char string[7];
        stream.read(string);
        SGE_ASSERT(memcmp(string, "relogic", sizeof(string)) == 0);

        auto type = stream.read<uint8_t>();
        SGE_ASSERT(type == 2); // 2 is world type

        stream.skip(12);
    }
    
    if (version >= 88) {
        auto sections = stream.read<int16_t>();
        for (int16_t i = 0; i < sections; ++i) {
            stream.read<int32_t>();
        }

        header.tile_frame_important = read_bit_array(stream);
    }

    header.name = read_string(stream);

    if (version >= 179) {
        std::string seed = read_string(stream);
        int64_t generator_version = stream.read<int64_t>();
    }

    if (version >= 181) {
        char guid[16];
        stream.read(guid);
    }

    header.id = stream.read<int32_t>();

    header.bounds.min.x = stream.read<int32_t>();
    header.bounds.max.x = stream.read<int32_t>();
    header.bounds.min.y = stream.read<int32_t>();
    header.bounds.max.y = stream.read<int32_t>();

    header.height = stream.read<int32_t>();
    header.width = stream.read<int32_t>();

    if (version >= 209) {
        auto game_mode = stream.read<int32_t>();
    }

    if (version >= 222) {
        auto drunk_world = stream.read<bool>();
    }

    if (version >= 227) {
        auto get_good_world = stream.read<bool>();
    }

    if (version >= 112 && version < 209) {
        auto expert_world = stream.read<bool>();
    }

    if (version >= 208 && version < 209) {
        auto master_world = stream.read<bool>();
    }

    if (version >= 141) {
        auto creation_time = stream.read<int64_t>();
    }

    if (version >= 63) {
        auto moon = stream.read<uint8_t>();
    }

    if (version >= 44) {
        int32_t tree_type_x[3];
        stream.read(tree_type_x);

        int32_t tree_style[4];
        stream.read(tree_style);
    }

    if (version >= 60) {
        int32_t cave_back_x[3];
        stream.read(cave_back_x);

        int32_t cabe_back_style[4];
        stream.read(cabe_back_style);

        auto ice_back_style = stream.read<int32_t>();
    }

    if (version >= 61) {
        auto jungle_back_style = stream.read<int32_t>();
        auto hell_back_style = stream.read<int32_t>();
    }

    header.spawn_point.x = stream.read<int32_t>();
    header.spawn_point.y = stream.read<int32_t>();

    header.surface_layer = stream.read<double>();
    header.rock_layer = stream.read<double>();
    auto game_time = stream.read<double>();

    auto day_night = stream.read<uint8_t>();
    
    auto moon_phase = stream.read<int32_t>();

    auto blood_moon = stream.read<bool>();

    if (version >= 70) {
        auto eclipse = stream.read<bool>();
    }

    auto dungeon_x = stream.read<int32_t>();
    auto dungeon_y = stream.read<int32_t>();

    if (version >= 56) {
        auto crimson = stream.read<bool>();
        header.evil = crimson ? WorldEvil::Crimson : WorldEvil::Corruption;
    }

    auto eye_of_cthulhu_downed = stream.read<bool>();
    auto eater_or_brain_downed = stream.read<bool>();
    auto skeletron_downed = stream.read<bool>();

    if (version >= 66) {
        auto queen_bee_downed = stream.read<bool>();
    }

    if (version >= 44) {
        auto destroyer_downed = stream.read<bool>();
        auto twins_downed = stream.read<bool>();
        auto skeletron_prime_downed = stream.read<bool>();
        auto any_hardmode_boss_downed = stream.read<bool>();
    }

    if (version >= 64) {
        auto plantera_downed = stream.read<bool>();
        auto golem_downed = stream.read<bool>();
    }

    if (version >= 118) {
        auto king_slime_downed = stream.read<bool>();
    }

    if (version >= 29) {
        auto tinkerer_saved = stream.read<bool>();
        auto wizard_saved = stream.read<bool>();
    }

    if (version >= 34) {
        auto mechanic_saved = stream.read<bool>();
    }

    if (version >= 29) {
        auto goblin_invasion_defeated = stream.read<bool>();
    }

    if (version >= 32) {
        auto clown_downed = stream.read<bool>();
    }

    if (version >= 37) {
        auto frost_legion_defeated = stream.read<bool>();
    }

    if (version >= 56) {
        auto pirates_defeated = stream.read<bool>();
    }    

    auto broke_a_shadow_orb = stream.read<bool>();
    auto meteor_spawned = stream.read<bool>();
    auto three_shadow_orbs_broken = stream.read<bool>();

    if (version >= 23) {
        auto altars_smashed = stream.read<int32_t>();
        auto hardmode = stream.read<bool>();
    }

    auto invasion_delay = stream.read<int32_t>();
    auto invasion_size = stream.read<int32_t>();
    auto invasion_type = stream.read<int32_t>();

    auto invasion_x = stream.read<double>();

    if (version >= 118) {
        auto slime_rain_time = stream.read<double>();
    }

    if (version >= 113) {
        auto sundial_cooldown = stream.read<uint8_t>();
    }

    if (version >= 53) {
        auto is_raining = stream.read<bool>();
        auto rain_time = stream.read<int32_t>();
        auto max_rain = stream.read<float>();
    }

    if (version >= 54) {
        auto tier1_ore_id = stream.read<int32_t>();
        auto tier2_ore_id = stream.read<int32_t>();
        auto tier3_ore_id = stream.read<int32_t>();
    }

    if (version >= 55) {
        auto tree_style = stream.read<uint8_t>();
        auto corruption_style = stream.read<uint8_t>();
        auto jungle_style = stream.read<uint8_t>();
    }

    if (version >= 60) {
        auto snow_style = stream.read<uint8_t>();
        auto hallow_style = stream.read<uint8_t>();
        auto crimson_style = stream.read<uint8_t>();
        auto desert_style = stream.read<uint8_t>();
        auto ocean_style = stream.read<uint8_t>();
        auto cloud_background = stream.read<uint32_t>();
    }

    if (version >= 62) {
        auto clouds_number = stream.read<int16_t>();
        auto wind_speed = stream.read<float>();
    }

    if (version >= 95) {
        auto count = stream.read<int32_t>();
        std::vector<std::string> players_finished_fishing_quest_today;
        players_finished_fishing_quest_today.reserve(count);

        for (auto i = 0; i < count; ++i) {
            players_finished_fishing_quest_today.push_back(read_string(stream));
        }
    }

    if (version >= 99) {
        auto angler_saved = stream.read<bool>();
    }

    if (version >= 101) {
        auto current_angler_quest = stream.read<int32_t>();
    }

    if (version >= 104) {
        auto stylist_saved = stream.read<bool>();
    }

    if (version >= 129) {
        auto tax_collector_saved = stream.read<bool>();
    }

    if (version >= 201) {
        auto golfer_saved = stream.read<bool>();
    }

    if (version >= 107) {
        auto invasion_size_start = stream.read<int32_t>();
    }

    if (version >= 108) {
        auto cultist_delay = stream.read<int32_t>();
    }

    if (version >= 109) {
        auto kill_count_size = stream.read<int16_t>();

        std::vector<int32_t> kill_counts;
        kill_counts.reserve(kill_count_size);

        for (auto i = 0; i < kill_count_size; ++i) {
            kill_counts.push_back(stream.read<int32_t>());
        }
    }

    if (version >= 128) {
        auto time_fast_forwarding = stream.read<bool>();
    }

    if (version >= 131) {
        auto fishron_downed = stream.read<bool>();
        auto ancient_cultist_downed = stream.read<bool>();
        auto moonlord_downed = stream.read<bool>();
        auto pumpking_downed = stream.read<bool>();
        auto spooky_wood_downed = stream.read<bool>();
        auto ice_queen_downed = stream.read<bool>();
        auto san_tank_downed = stream.read<bool>();
        auto christmas_tree_downed = stream.read<bool>();
    }

    if (version >= 140) {
        auto solar_pillar_downed = stream.read<bool>();
        auto vortex_pillar_downed = stream.read<bool>();
        auto nebula_pillar_downed = stream.read<bool>();
        auto stardust_pillar_downed = stream.read<bool>();

        auto solar_pillar_barrier_active = stream.read<bool>();
        auto vortex_pillar_barrier_active = stream.read<bool>();
        auto nebula_pillar_barrier_active = stream.read<bool>();
        auto stardust_pillar_barrier_active = stream.read<bool>();

        auto lunar_apocalypse = stream.read<bool>();
    }

    if (version >= 170) {
        auto manual_party_active = stream.read<bool>();
        auto genuine_party_active = stream.read<bool>();

        auto party_cooldown = stream.read<int32_t>();
        auto partiers_number = stream.read<int32_t>();

        std::vector<int32_t> partying_npcs;
        partying_npcs.reserve(partiers_number);

        for (auto i = 0; i < partiers_number; ++i) {
            partying_npcs.push_back(stream.read<int32_t>());
        }
    }

    if (version >= 174) {
        auto sandstorm_active = stream.read<bool>();
        auto sandstorm_time_left = stream.read<int32_t>();
        auto sandstorm_severity = stream.read<float>();
        auto sandstorm_max_severity = stream.read<float>();
    }

    if (version >= 178) {
        auto bartender_saved = stream.read<bool>();
        auto dd2_invasion_finished_1 = stream.read<bool>();
        auto dd2_invasion_finished_2 = stream.read<bool>();
        auto dd2_invasion_finished_3 = stream.read<bool>();
    }

    if (version >= 195) {
        auto style8 = stream.read<uint8_t>();
    }

    if (version >= 215) {
        auto style9 = stream.read<uint8_t>();
    }

    if (version >= 196) {
        auto style10 = stream.read<uint8_t>();
        auto style11 = stream.read<uint8_t>();
        auto style12 = stream.read<uint8_t>();
    }

    if (version >= 204) {
        auto combat_book_used = stream.read<bool>();
    }

    if (version >= 207) {
        auto lantern_night_cooldown = stream.read<int32_t>();
        auto genuine_lantern_night = stream.read<bool>();
        auto manual_lantern_night = stream.read<bool>();
        auto next_lantern_night_is_genuine = stream.read<bool>();
    }

    if (version >= 211) {
        auto tree_tops_number = stream.read<int32_t>();

        std::vector<int32_t> tree_tops;
        tree_tops.reserve(tree_tops_number);

        for (auto i = 0; i < tree_tops_number; ++i) {
            tree_tops.push_back(stream.read<int32_t>());
        }
    }

    if (version >= 212) {
        auto forced_hallowen = stream.read<bool>();
        auto forced_christmas = stream.read<bool>();
    }

    if (version >= 216) {
        auto copper_type_id = stream.read<int32_t>();
        auto iron_type_id = stream.read<int32_t>();
        auto silver_type_id = stream.read<int32_t>();
        auto gold_type_id = stream.read<int32_t>();
    }

    if (version >= 217) {
        auto cat_bought = stream.read<bool>();
        auto dog_bought = stream.read<bool>();
        auto bunny_bought = stream.read<bool>();
    }

    if (version >= 223) {
        auto empress_of_light_downed = stream.read<bool>();
        auto queen_slime_downed = stream.read<bool>();
    }

    #pragma clang diagnostic pop
}