#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <cmath>
#include "set"

#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
#define COMPARISON_ERROR (1e-6)


class SearchServer {
public:

    // конструктор, задает стоп слова, переданные в контейнере
    SearchServer() = default;
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
    }

    // конструктор, задает стоп слова, переданные в строке
    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text)) // Invoke delegating constructor from std::string container
    {
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    static void FindTopDocuments(const SearchServer& search_server, std::string raw_query);

    std::set<int>::iterator begin() const;

    std::set<int>::iterator end() const;

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    // добавление документов
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    static void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
    //remove docs
    void RemoveDocument(int document_id);

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    // получить количество документов
    int GetDocumentCount() const;

private:

    // структура, содержит средний райтинг докумета и его статус
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    // сет из стоп слов
    const std::set<std::string> stop_words_;

    // мэп: ключ - слово из документа, значение - мэп: ключ - id документа, значение TF для слова
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    std::map<int, std::map<std::string, double>> word_frequencies_;

    std::map<std::string, double> empty_map_;

    // мэп: ключ - id документа, значение - структура из рейтинга и статуса документа
    std::map<int, DocumentData> documents_;
    std::set<int> documents_id_;
    // определить принадлежность слова к списку стоп-слов
    bool IsStopWord(const std::string& word) const;

    // разбить текст на слова, исключив из него стоп-слова
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    // вычисление среднего рейтинга на основе переданного вектора рейтингов
    static int ComputeAverageRating(const std::vector<int>& ratings);

    // структура, хранящая слово и его принадлежность к списку стоп слов,
    // а также является ли минус-словом
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    // оредеделение принадлежности к стоп слову и минус-слову
    QueryWord ParseQueryWord(std::string text) const;

    // структура двух сетов из плюс слов и минус слов
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const
{
    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < COMPARISON_ERROR)
            {
                return lhs.rating > rhs.rating;
            }
            else
            {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}


