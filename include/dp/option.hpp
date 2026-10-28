/**
 * @file option.hpp
 * @brief Basic option vocabulary shared by all modules.
 */
#pragma once

#include <string_view>

namespace dp {

/// Option right. Deribit options are European, so no exercise style is needed.
enum class OptionType { Call, Put };

/// +1 for calls, -1 for puts: lets call and put formulas share one expression.
constexpr double sign(OptionType type) noexcept
{
    return type == OptionType::Call ? 1.0 : -1.0;
}

constexpr std::string_view toString(OptionType type) noexcept
{
    return type == OptionType::Call ? "call" : "put";
}

}  // namespace dp
