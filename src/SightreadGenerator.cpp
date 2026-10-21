#include "SightreadGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

// All tuning lives in this file. Distances are in blocks; "rel" scales a distance
// by how much faster the current speed is than 1x, so reaction time stays similar.

namespace sightread {
namespace {

constexpr int OBJ_BLOCK = 1;
constexpr int OBJ_SPIKE = 8;
constexpr int OBJ_YELLOW_ORB = 36;

constexpr int MODE_PORTAL[ModeCount] = { 12, 13, 47, 111, 660, 745, 1331, 1933 };
constexpr int SPEED_PORTAL[SpeedCount] = { 200, 201, 202, 203, 1334 };
// Horizontal speed relative to 1x (roughly 8.4 / 10.4 / 13 / 15.6 / 19.3 blocks per second).
constexpr double SPEED_REL[SpeedCount] = { 0.807, 1.0, 1.243, 1.502, 1.849 };
// Height of the corridor GD creates for each mode, in blocks (0 = ground only).
constexpr int CORRIDOR[ModeCount] = { 0, 10, 8, 10, 10, 0, 8, 10 };
constexpr int SECTIONS[LengthCount] = { 3, 5, 8, 12 };

class Generator {
public:
    explicit Generator(Settings const& s)
        : m_settings(s),
          m_rng(s.seed),
          m_t((std::clamp(s.difficulty, 1, 10) - 1) / 9.0) {}

    std::string run() {
        // Start as a 1x cube; every section begins with its own portals.
        m_out = "kA2,0,kA3,0,kA4,0,kA8,0,kA11,0;";
        m_col = 10;

        int sections = SECTIONS[std::clamp(m_settings.length, 0, LengthCount - 1)];
        int lastMode = -1;

        for (int i = 0; i < sections; i++) {
            int mode = pick(m_settings.modeMask, ModeCount, lastMode, Cube);
            int speed = pick(m_settings.speedMask, SpeedCount, -1, Normal);
            m_rel = SPEED_REL[speed];
            bool exit = i + 1 < sections;

            int portal = m_col;
            add(SPEED_PORTAL[speed], portal, 1);
            add(MODE_PORTAL[mode], portal, 1);

            int start = portal + std::max(4, (int)std::lround(5 * m_rel));
            int end = start + (int)std::lround(lerp(32, 46) * m_rel);

            switch (mode) {
                case Cube:   groundSection(start, end, false); break;
                case Robot:  groundSection(start, end, true); break;
                case Ball:   flipSection(start, end, false, exit); break;
                case Spider: flipSection(start, end, true, exit); break;
                default:     flySection(start, end, mode, exit); break;
            }
            lastMode = mode;
        }
        return m_out;
    }

private:
    Settings m_settings;
    std::mt19937_64 m_rng;
    double m_t;          // difficulty mapped to 0..1
    double m_rel = 1.0;  // current speed factor
    int m_col = 0;       // column of the next portal
    std::string m_out;

    int randInt(int lo, int hi) {
        if (hi <= lo) return lo;
        return std::uniform_int_distribution<int>(lo, hi)(m_rng);
    }
    double randReal(double lo, double hi) {
        return std::uniform_real_distribution<double>(lo, hi)(m_rng);
    }
    bool chance(double p) { return randReal(0, 1) < p; }
    double lerp(double easy, double hard) const { return easy + (hard - easy) * m_t; }

    int pick(uint32_t mask, int count, int avoid, int fallback) {
        std::vector<int> options;
        for (int i = 0; i < count; i++) {
            if (mask & (1u << i)) options.push_back(i);
        }
        if (options.empty()) return fallback;
        if (options.size() > 1) {
            options.erase(std::remove(options.begin(), options.end(), avoid), options.end());
        }
        return options[randInt(0, (int)options.size() - 1)];
    }

    void add(int id, double col, double row, int rotation = 0) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "1,%d,2,%g,3,%g", id, col * 30 + 15, row * 30 + 15);
        m_out += buf;
        if (rotation) {
            std::snprintf(buf, sizeof(buf), ",6,%d", rotation);
            m_out += buf;
        }
        m_out += ';';
    }

    void spikes(int col, int count, int row = 0, bool flipped = false) {
        for (int i = 0; i < count; i++) add(OBJ_SPIKE, col + i, row, flipped ? 180 : 0);
    }

    // Cube / robot: jumps over spike rows, onto block steps, and orb-assisted long gaps.
    void groundSection(int start, int end, bool robot) {
        double jump = (robot ? 3.8 : 3.6) * m_rel;   // horizontal length of a full jump
        int maxSpikes = std::clamp((int)std::floor(jump - 1.2), 1, 3);
        maxSpikes = std::min(maxSpikes, 1 + (int)(m_t * 2.99));
        int minGap = (int)std::ceil(jump) + 1;

        int c = start;
        while (c < end - 6) {
            double roll = randReal(0, 1);
            int width;
            if (m_t >= 0.45 && roll < 0.18) {
                // Long spike run with a yellow orb near the top of the first jump.
                width = std::min(maxSpikes + 2, (int)std::floor(jump * 1.3));
                spikes(c, width);
                add(OBJ_YELLOW_ORB, c + std::max(0L, std::lround(1.8 * m_rel - 1)), 2);
            } else if (roll < 0.45) {
                // Jump onto a 1-high platform, then off it over a spike.
                int w = randInt(2, 4);
                for (int i = 0; i < w; i++) add(OBJ_BLOCK, c + i, 0);
                width = w;
                if (m_t >= 0.25) {
                    spikes(c + w, 1);
                    width++;
                }
            } else {
                width = randInt(1, maxSpikes);
                spikes(c, width);
            }
            int gap = (int)std::lround(lerp(9, 4) * m_rel * randReal(0.8, 1.25));
            c += width + std::max(minGap, gap);
        }
        m_col = std::max(c, end);
    }

    // Ship / UFO / wave / swing: a corridor of gates, each a column of blocks with one opening.
    void flySection(int start, int end, int mode, bool exit) {
        int height = CORRIDOR[mode];
        double gapEasy = 6, gapHard = 3, shiftK = 0.5;
        int minSpacing = 3;
        switch (mode) {
            case Ufo:   gapHard = 4; shiftK = 0.35; minSpacing = 4; break;
            case Wave:  shiftK = 0.7; break;
            case Swing: gapHard = 4; shiftK = 0.5; break;
            default: break;
        }
        int gap = (int)std::lround(lerp(gapEasy, gapHard));
        int spacing = std::max(minSpacing, (int)std::lround(lerp(9, 4.5) * m_rel));
        auto maxShift = [&](int sp) { return std::max(1, (int)std::floor(sp * shiftK)); };

        int bottom = 0; // lowest open row of the previous gate
        int c = start + 2;
        bool first = true;
        while (c < end) {
            int sp = spacing + randInt(0, 1);
            int shift = maxShift(sp);
            int next = first ? randInt(0, shift) : bottom + randInt(-shift, shift);
            bottom = std::clamp(next, 0, height - gap);
            gate(c, bottom, gap, height);
            c += sp;
            first = false;
        }

        if (!exit) {
            m_col = c;
            return;
        }
        // Step the opening down to the floor, then funnel the player into a low portal.
        while (bottom > 0) {
            bottom = std::max(0, bottom - maxShift(spacing));
            gate(c, bottom, gap, height);
            c += spacing;
        }
        int funnelGap = std::max(spacing, (int)std::ceil(std::max(0, gap - 3) / shiftK) + 1);
        c += funnelGap - spacing;
        for (int r = 3; r < height; r++) add(OBJ_BLOCK, c, r);
        m_col = c;
    }

    void gate(int col, int bottom, int gap, int height) {
        for (int r = 0; r < bottom; r++) add(OBJ_BLOCK, col, r);
        for (int r = bottom + gap; r < height; r++) add(OBJ_BLOCK, col, r);
    }

    // Ball / spider: spike clusters on the floor and ceiling, never both in one column.
    void flipSection(int start, int end, bool spider, bool exit) {
        int height = CORRIDOR[spider ? Spider : Ball];
        int ceilingRow = height - 1;
        int flip = spider ? 1 : (int)std::ceil(5 * m_rel); // blocks travelled while switching sides
        int maxWidth = 1 + (int)(m_t * 2.2);

        bool onCeiling = chance(0.5);
        bool lastOnCeiling = true;
        int lastEnd = start;
        int c = start + 2;
        while (c < end) {
            int w = randInt(1, maxWidth);
            spikes(c, w, onCeiling ? ceilingRow : 0, onCeiling);
            lastOnCeiling = onCeiling;
            lastEnd = c + w;

            bool next = chance(0.7) ? !onCeiling : onCeiling;
            int gap = (next != onCeiling)
                ? std::max(flip + 1, (int)std::lround(lerp(9, 4) * m_rel))
                : std::max(2, (int)std::lround(lerp(6, 3) * m_rel));
            c = lastEnd + gap;
            onCeiling = next;
        }

        if (!exit) {
            m_col = c;
            return;
        }
        // A ceiling strip right before the portal forces the player onto the floor.
        if (!lastOnCeiling) c = std::max(c, lastEnd + flip + 1);
        spikes(c, 3, ceilingRow, true);
        m_col = c + 3;
    }
};

} // namespace

std::string generate(Settings const& settings) {
    return Generator(settings).run();
}

} // namespace sightread
