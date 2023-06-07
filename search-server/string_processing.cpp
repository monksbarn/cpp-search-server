#include "string_processing.h"

bool IsInvalidCharacter(const char character)
{
    return (character >= 0 && character < 32);
}

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    size_t pos = text.find_first_not_of(' ');
    while (pos < text.size()) {
        text.remove_prefix(pos);
        for (pos = 0; pos < text.size() && text.at(pos) != ' '; ++pos) {
            if (IsInvalidCharacter(text.at(pos))) {
                throw std::invalid_argument("unreadable characters in the text");
            }
        }
        words.push_back(text.substr(0, pos));
        pos = text.find_first_not_of(' ', pos);
    }
    return words;
}