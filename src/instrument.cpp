#include "dp/instrument.hpp"

#include "text.hpp"

#include <algorithm>
#include <array>

namespace dp {

namespace {

std::optional<std::chrono::month> parseMonth(std::string_view abbreviation)
{
    constexpr std::array<std::string_view, 12> names = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    const auto found = std::ranges::find(names, abbreviation);
    if (found == names.end()) {
        return std::nullopt;
    }
    return std::chrono::month{static_cast<unsigned>(found - names.begin()) + 1};
}

/// "27DEC24" or "3JAN25" -> 08:00 UTC of that day.
std::optional<std::chrono::sys_seconds> parseExpiry(std::string_view field)
{
    constexpr std::size_t monthAndYearLength = 5;  // "DEC24"
    if (field.size() != monthAndYearLength + 1 && field.size() != monthAndYearLength + 2) {
        return std::nullopt;
    }
    const std::size_t dayLength = field.size() - monthAndYearLength;
    const auto day = text::parseNumber<unsigned>(field.substr(0, dayLength));
    const auto month = parseMonth(field.substr(dayLength, 3));
    const auto year = text::parseNumber<int>(field.substr(dayLength + 3));
    if (!day || !month || !year) {
        return std::nullopt;
    }

    const std::chrono::year_month_day date{std::chrono::year{2000 + *year}, *month, std::chrono::day{*day}};
    if (!date.ok()) {
        return std::nullopt;
    }
    return std::chrono::sys_days{date} + std::chrono::hours{8};
}

std::optional<double> parseStrike(std::string_view field)
{
    std::string normalized{field};
    std::ranges::replace(normalized, 'd', '.');
    const auto strike = text::parseNumber<double>(normalized);
    if (!strike || !(*strike > 0.0)) {
        return std::nullopt;
    }
    return strike;
}

std::optional<OptionType> parseType(std::string_view field)
{
    if (field == "C") {
        return OptionType::Call;
    }
    if (field == "P") {
        return OptionType::Put;
    }
    return std::nullopt;
}

}  // namespace

std::optional<Instrument> parseInstrumentName(std::string_view name)
{
    const auto parts = text::split(name, '-');
    if (parts.size() != 4 || parts[0].empty()) {
        return std::nullopt;
    }
    const auto expiry = parseExpiry(parts[1]);
    const auto strike = parseStrike(parts[2]);
    const auto type = parseType(parts[3]);
    if (!expiry || !strike || !type) {
        return std::nullopt;
    }
    return Instrument{
        .currency = std::string{parts[0]},
        .expiry = *expiry,
        .strike = *strike,
        .type = *type,
    };
}

double yearFraction(std::chrono::sys_seconds from, std::chrono::sys_seconds to)
{
    constexpr double secondsPerYear = 365.0 * 24 * 60 * 60;
    return std::chrono::duration<double>(to - from).count() / secondsPerYear;
}

}  // namespace dp
