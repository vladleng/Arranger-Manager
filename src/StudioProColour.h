#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace arranger
{
// Studio Pro stores colors as AABBGGRR; JUCE expects AARRGGBB.
inline std::optional<std::uint32_t> studioProColour(std::string_view saved)
{
    if (saved.size() != 8) return std::nullopt;
    std::uint32_t abgr = 0;
    for (const char c : saved)
    {
        unsigned int digit;
        if (c >= '0' && c <= '9') digit = static_cast<unsigned int>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned int>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned int>(c - 'A' + 10);
        else return std::nullopt;
        abgr = (abgr << 4) | digit;
    }
    return (abgr & 0xff000000u) | ((abgr & 0x000000ffu) << 16)
        | (abgr & 0x0000ff00u) | ((abgr & 0x00ff0000u) >> 16);
}
}
