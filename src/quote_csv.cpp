#include "dp/quote_csv.hpp"

#include "text.hpp"

namespace dp {

std::optional<OptionQuote> parseQuoteCsvRow(std::string_view line)
{
    if (line.ends_with('\r')) {  // files saved on Windows
        line.remove_suffix(1);
    }
    const auto fields = text::split(line, ',');
    if (fields.size() != 5 || fields[0].empty()) {
        return std::nullopt;
    }
    const auto underlying = text::parseNumber<double>(fields[1]);
    const auto markPrice = text::parseNumber<double>(fields[2]);
    const auto markIv = text::parseNumber<double>(fields[3]);
    const auto timestamp = text::parseNumber<std::int64_t>(fields[4]);
    if (!underlying || !markPrice || !markIv || !timestamp) {
        return std::nullopt;
    }
    return OptionQuote{
        .instrumentName = std::string{fields[0]},
        .underlyingPrice = *underlying,
        .markPriceCoin = *markPrice,
        .markIvPercent = *markIv,
        .timestampMs = *timestamp,
    };
}

}  // namespace dp
