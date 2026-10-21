#pragma once

#include <cstdint>
#include <string>

// Procedural "sightreading" layout generator.
//
// Coordinates follow GD's level-string convention: one grid block is 30 units,
// x = column * 30 + 15, y = row * 30 + 15, and row 0 sits on the ground.

namespace sightread {
    enum Mode { Cube, Ship, Ball, Ufo, Wave, Robot, Spider, Swing, ModeCount };
    enum Speed { Slow, Normal, Fast, Faster, Fastest, SpeedCount }; // 0.5x 1x 2x 3x 4x
    enum Length { Short, Medium, Long, VeryLong, LengthCount };

    inline constexpr char const* MODE_NAMES[ModeCount] = {
        "Cube", "Ship", "Ball", "UFO", "Wave", "Robot", "Spider", "Swing"
    };
    inline constexpr char const* SPEED_NAMES[SpeedCount] = { "0.5x", "1x", "2x", "3x", "4x" };
    inline constexpr char const* LENGTH_NAMES[LengthCount] = { "Short", "Medium", "Long", "Very Long" };

    struct Settings {
        int difficulty = 5;          // 1 (gentle) .. 10 (brutal)
        int length = Medium;
        uint32_t modeMask = 1u << Cube;
        uint32_t speedMask = 1u << Normal;
        uint64_t seed = 0;
    };

    // Returns an uncompressed level string (header + objects).
    std::string generate(Settings const& settings);
}
