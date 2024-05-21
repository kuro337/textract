
// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string_view>
#include <unordered_set>

/// @brief Image Processing Mode or Document Processing Modes
enum class ImgMode { document, image };

/// @brief Core Capacity Enum
enum class CORES { single, half, max };

/// @brief  Valid Image File Extensions
const std::unordered_set<std::string_view> validExtensions = {
    "jpg", "jpeg", "png", "bmp", "gif", "tif"};

/// @brief Supported Languages
enum class ISOLang { en, es, fr, hi, zh, de };

namespace ISOLanguage {
    static constexpr auto eng = "eng";
    static constexpr auto esp = "spa";
    static constexpr auto fra = "fra";
    static constexpr auto ger = "deu";
    static constexpr auto chi = "chi_sim";
    static constexpr auto hin = "hin";
} // namespace ISOLanguage

namespace Ansi {
    static constexpr auto BOLD          = "\x1b[1m";
    static constexpr auto ITALIC        = "\x1b[3m";
    static constexpr auto UNDERLINE     = "\x1b[4mT";
    static constexpr auto BRIGHT_WHITE  = "\x1b[97m";
    static constexpr auto LIGHT_GREY    = "\x1b[37m";
    static constexpr auto GREEN         = "\x1b[92m";
    static constexpr auto BOLD_WHITE    = "\x1b[1m";
    static constexpr auto CYAN          = "\x1b[96m";
    static constexpr auto BLUE          = "\x1b[94m";
    static constexpr auto GREEN_BOLD    = "\x1b[1;32m";
    static constexpr auto ERROR         = "\x1b[31m";
    static constexpr auto SUCCESS_TICK  = "\x1b[32m✔\x1b[0m";
    static constexpr auto FAILURE_CROSS = "\x1b[31m✖\x1b[0m";
    static constexpr auto WARNING       = "\x1b[93m";
    static constexpr auto WARNING_BOLD  = "\x1b[1;33m";
    static constexpr auto END           = "\x1b[0m";
    static constexpr auto DELIMITER_STAR =
        "\x1b[90m******************************************************"
        "\x1b[";
    static constexpr auto DELIMITER_DIM =
        "\x1b[90m******************************************************"
        "\x1b[";
    static constexpr auto DELIMITER_ITEM =
        "--------------------------------------------------------------";
} // namespace Ansi

#endif // CONSTANTS_H