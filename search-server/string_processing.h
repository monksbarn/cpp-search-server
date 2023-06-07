#pragma once
#include <stdexcept>
#include <utility>
#include <string>
#include <string_view>
#include <vector>
#include <set>

static std::set<std::string> all_words_;

bool IsInvalidCharacter(const char character);
// разбиение строки на вектор слов
std::vector<std::string_view> SplitIntoWords(std::string_view text);

// структура, хранящая id, релевантность и рейтинг документа
template <typename StringContainer>
std::set<std::string_view> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    std::set<std::string_view> non_empty_strings;
    for (std::string_view str : strings)
    {
        if (!str.empty())
        {
            for (const char c : str) {
                if (IsInvalidCharacter(c)) {
                    throw std::invalid_argument("unreadable characters in the text");
                }
            }
            const auto [new_word, inserted] = all_words_.insert(static_cast<std::string>(str));
            non_empty_strings.insert(*new_word);
        }
    }
    return non_empty_strings;
}


