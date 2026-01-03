// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file filament_database.h
 * @brief Static database of filament materials with temperature recommendations
 *
 * Provides a comprehensive list of common 3D printing materials with their
 * recommended temperature ranges. Used by the Edit Filament modal to auto-derive
 * temperatures when a material is selected.
 *
 * Temperature sources:
 * - Manufacturer recommendations from major brands (Bambu, Polymaker, eSUN, etc.)
 * - Community consensus from r/3Dprinting and Voron Discord
 * - Tested ranges from the author's Voron 2.4
 */

namespace filament {

/**
 * @brief Material information with temperature recommendations
 */
struct MaterialInfo {
    const char* name;     ///< Material name (e.g., "PLA", "PETG")
    int nozzle_min;       ///< Minimum nozzle temperature (°C)
    int nozzle_max;       ///< Maximum nozzle temperature (°C)
    int bed_temp;         ///< Recommended bed temperature (°C)
    const char* category; ///< Category for grouping (e.g., "Standard", "Engineering")

    /**
     * @brief Get recommended nozzle temperature (midpoint of range)
     */
    [[nodiscard]] constexpr int nozzle_recommended() const {
        return (nozzle_min + nozzle_max) / 2;
    }
};

/**
 * @brief Static database of common filament materials
 *
 * Materials are grouped by category:
 * - Standard: PLA, PETG - most common, beginner-friendly
 * - Engineering: ABS, ASA, PC, PA - require enclosure/higher temps
 * - Flexible: TPU, TPE - rubber-like materials
 * - Support: PVA, HIPS - dissolvable/breakaway supports
 * - Specialty: Wood-fill, Marble, Metal-fill - decorative
 * - High-Temp: PEEK, PEI - industrial applications
 */
// clang-format off
inline constexpr MaterialInfo MATERIALS[] = {
    // === Standard Materials (No enclosure required) ===
    {"PLA",         190, 220, 60,  "Standard"},
    {"PLA+",        200, 230, 60,  "Standard"},
    {"PLA-CF",      200, 230, 60,  "Standard"},   // Carbon fiber PLA
    {"PLA-GF",      200, 230, 60,  "Standard"},   // Glass fiber PLA
    {"Silk PLA",    200, 230, 60,  "Standard"},   // Shiny finish PLA
    {"Matte PLA",   200, 230, 60,  "Standard"},
    {"PETG",        230, 260, 80,  "Standard"},
    {"PETG-CF",     240, 270, 80,  "Standard"},   // Carbon fiber PETG
    {"PETG-GF",     240, 270, 80,  "Standard"},   // Glass fiber PETG

    // === Engineering Materials (Enclosure recommended) ===
    {"ABS",         240, 270, 100, "Engineering"},
    {"ABS+",        240, 270, 100, "Engineering"},
    {"ASA",         240, 270, 100, "Engineering"}, // UV-resistant ABS alternative
    {"PC",          260, 300, 110, "Engineering"}, // Polycarbonate
    {"PC-CF",       270, 300, 110, "Engineering"}, // Carbon fiber PC
    {"PC-ABS",      250, 280, 100, "Engineering"}, // PC/ABS blend

    // === Nylon/Polyamide (Enclosure required, dry storage) ===
    {"PA",          250, 280, 80,  "Engineering"}, // Generic nylon
    {"PA6",         250, 280, 80,  "Engineering"},
    {"PA12",        250, 280, 80,  "Engineering"},
    {"PA-CF",       260, 290, 80,  "Engineering"}, // Carbon fiber nylon
    {"PA-GF",       260, 290, 80,  "Engineering"}, // Glass fiber nylon

    // === Flexible Materials ===
    {"TPU",         210, 240, 50,  "Flexible"},    // Shore 95A typical
    {"TPU-Soft",    200, 230, 50,  "Flexible"},    // Shore 85A or softer
    {"TPE",         200, 230, 50,  "Flexible"},

    // === Support Materials ===
    {"PVA",         180, 210, 60,  "Support"},     // Water-soluble
    {"HIPS",        230, 250, 100, "Support"},     // Limonene-soluble
    {"BVOH",        190, 220, 60,  "Support"},     // Water-soluble (better than PVA)

    // === Specialty/Decorative ===
    {"Wood PLA",    190, 220, 60,  "Specialty"},   // Wood fiber fill
    {"Marble PLA",  200, 220, 60,  "Specialty"},   // Marble effect
    {"Metal PLA",   200, 230, 60,  "Specialty"},   // Metal powder fill
    {"Glow PLA",    200, 230, 60,  "Specialty"},   // Glow-in-the-dark
    {"Color-Change",200, 230, 60,  "Specialty"},   // Temperature reactive

    // === High-Temperature Industrial ===
    {"PEEK",        370, 420, 120, "High-Temp"},   // Requires all-metal hotend
    {"PEI",         340, 380, 120, "High-Temp"},   // ULTEM
    {"PSU",         340, 380, 120, "High-Temp"},   // Polysulfone
    {"PPSU",        350, 390, 140, "High-Temp"},   // Medical grade
};
// clang-format on

/// Number of materials in the database
inline constexpr size_t MATERIAL_COUNT = sizeof(MATERIALS) / sizeof(MATERIALS[0]);

/**
 * @brief Find material info by name (case-insensitive)
 * @param name Material name to look up
 * @return MaterialInfo if found, std::nullopt otherwise
 */
inline std::optional<MaterialInfo> find_material(std::string_view name) {
    for (const auto& mat : MATERIALS) {
        // Case-insensitive comparison
        std::string mat_lower(mat.name);
        std::string name_lower(name);
        std::transform(mat_lower.begin(), mat_lower.end(), mat_lower.begin(), ::tolower);
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

        if (mat_lower == name_lower) {
            return mat;
        }
    }
    return std::nullopt;
}

/**
 * @brief Get all materials in a category
 * @param category Category name (e.g., "Standard", "Engineering")
 * @return Vector of matching materials
 */
inline std::vector<MaterialInfo> get_materials_by_category(std::string_view category) {
    std::vector<MaterialInfo> result;
    for (const auto& mat : MATERIALS) {
        if (category == mat.category) {
            result.push_back(mat);
        }
    }
    return result;
}

/**
 * @brief Get list of all unique category names
 * @return Vector of category names in order of appearance
 */
inline std::vector<const char*> get_categories() {
    std::vector<const char*> categories;
    for (const auto& mat : MATERIALS) {
        bool found = false;
        for (const auto* cat : categories) {
            if (std::string_view(cat) == mat.category) {
                found = true;
                break;
            }
        }
        if (!found) {
            categories.push_back(mat.category);
        }
    }
    return categories;
}

/**
 * @brief Get list of all material names (for dropdown population)
 * @return Vector of material name strings
 */
inline std::vector<const char*> get_all_material_names() {
    std::vector<const char*> names;
    names.reserve(MATERIAL_COUNT);
    for (const auto& mat : MATERIALS) {
        names.push_back(mat.name);
    }
    return names;
}

} // namespace filament
