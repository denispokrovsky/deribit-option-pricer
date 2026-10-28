// Small string helpers shared by the parsers. Internal: not part of the public API.
#pragma once

#include <charconv>
#include <optional>
#include <string_view>
#include <vector>

namespace dp::text {

inline std::vector<std::string_view> split(std::string_view line, char separator)
{
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t end = line.find(separator, start);
        fields.push_back(line.substr(start, end - start));
        if (end == std::string_view::npos) {
            return fields;
        }
        start = end + 1;
    }
}

/// Parses the whole field as a number; partial matches ("12abc") are rejected.
template <typename Number>
std::optional<Number> parseNumber(std::string_view field)
{
    Number value{};
    const char* end = field.data() + field.size();
    const auto [stop, error] = std::from_chars(field.data(), end, value);
    if (error != std::errc{} || stop != end) {
        return std::nullopt;
    }
    return value;
}

}  // namespace dp::text
