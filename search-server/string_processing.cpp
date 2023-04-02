#include <stdexcept>
#include "string_processing.h"

bool IsInvalidCharacter(const char character)
{
    if (character >= 0 && character < 32)
        return true;
    return false;
}

std::vector<std::string> SplitIntoWords(const std::string& text)
{
    std::vector<std::string> words;
    std::string word;
    for (const char c : text)
    {
        if (IsInvalidCharacter(c))
            throw std::invalid_argument("unreadable characters in the text");
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }
    return words;
}