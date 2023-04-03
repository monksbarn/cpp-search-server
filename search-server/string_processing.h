#pragma once
#include <utility>
#include <string>
#include <vector>
#include <set>

bool IsInvalidCharacter(const char character);
// разбиение строки на вектор слов
std::vector<std::string> SplitIntoWords(const std::string& text);

// структура, хранящая id, релевантность и рейтинг документа
template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings)
    {
        if (!str.empty())
        {
            for (const char c : str)
            {
                if (IsInvalidCharacter(c))
                    throw std::invalid_argument("unreadable characters in the stop-words");
            }
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}


