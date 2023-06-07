#pragma once

#include <string>
#include <vector>
#include <list>

#include "search_server.h"
#include "document.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries);

template<typename Type>
class QueriesJoined {
public:

    using Iterator = typename std::list<Type>::iterator;
    using ConstIterator = typename std::list<Type>::const_iterator;
    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;

    QueriesJoined() = default;

    void push_back(std::vector<Document> documents) {
        std::transform(documents.begin(), documents.end(), std::back_inserter(data_), [](const auto& document) {
            return document;
            });
    }
    size_t size() {
        return data_.size();
    }
    Iterator begin() {
        return data_.begin();
    }

    Iterator end() {
        return data_.end();
    }

    ConstIterator cbegin() {
        return data_.cbegin();
    }

    ConstIterator cend() {
        return data_.cend();
    }

private:
    std::list<Type> data_;
};

QueriesJoined<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);