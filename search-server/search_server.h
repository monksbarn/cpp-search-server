#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <utility>
#include <cmath>
#include <set>
#include <execution>
#include <thread>
#include <cstddef>
#include <future>
#include <type_traits>

#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
#define COMPARISON_ERROR (1e-6)

using namespace std::literals::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> lock;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : m_(bucket_count), backets_(bucket_count) {
    };

    Access operator[](const Key& key) {
        return Access{ std::lock_guard(m_[key % backets_.size()]), backets_.at(key % backets_.size())[key] };
    };

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> tmp;
        for (size_t i = 0; i < backets_.size();++i) {
            std::lock_guard lock(m_[i]);
            tmp.insert(backets_.at(i).begin(), backets_.at(i).end());
        }
        return tmp;
    };

private:
    std::vector<std::mutex> m_;
    std::vector<std::map<Key, Value>> backets_;
};

class SearchServer {
public:


    SearchServer() = default;

    // конструктор, задает стоп слова, переданные в контейнере
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) :stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
    }

    // конструктор, задает стоп слова, переданные в строке
    explicit SearchServer(std::string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    explicit SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    // добавление документов
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    static void AddDocument(SearchServer& search_server, int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    //================FIND_TOP============================
    static void FindTopDocuments(const SearchServer& search_server, const std::string_view raw_query);
    static void FindTopDocuments(const std::execution::sequenced_policy&, const SearchServer& search_server, const std::string_view raw_query);
    static void FindTopDocuments(const std::execution::parallel_policy&, const SearchServer& search_server, const std::string_view raw_query);

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentStatus status) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;


    std::set<int>::iterator begin() const;

    std::set<int>::iterator end() const;

    //const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    //remove docs
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& seq, int document_id);
    void RemoveDocument(const std::execution::parallel_policy& par, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& seq, const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& par, const std::string_view raw_query, int document_id) const;

    // получить количество документов
    int GetDocumentCount() const;

private://=========================================SEARCH_SERVER_PRIVATE==============================================================

    // структура, содержит средний райтинг докумета и его статус
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string_view> stop_words_;

    // мэп: ключ - слово из документа, значение - мэп: ключ - id документа, значение TF для слова
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;

    //ключ - id документа, значение: ключ - слово, значение TF для слова
    std::map<int, std::map<std::string_view, double>> word_frequencies_;

    // мэп: ключ - id документа, значение - структура из рейтинга и статуса документа
    std::map<int, DocumentData> documents_;

    //id документов
    std::set<int> documents_id_;

    // определить принадлежность слова к списку стоп-слов
    bool IsStopWord(const std::string_view word) const;

    // разбить текст на слова, исключив из него стоп-слова
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    // вычисление среднего рейтинга на основе переданного вектора рейтингов
    static int ComputeAverageRating(const std::vector<int>& ratings);

    // структура, хранящая слово и его принадлежность к списку стоп слов,
    // а также является ли минус-словом
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    // оредеделение принадлежности к стоп слову и минус-слову
    QueryWord ParseQueryWord(const std::string_view text) const;

    // структура двух сетов из плюс слов и минус слов
    // std::set<std::string_view> plus_words;
    // std::set<std::string_view> minus_words;
    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    struct QueryParallel {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text) const;

    QueryParallel ParseQuery(const std::execution::parallel_policy& par, const std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate, typename QueryType>
    std::vector<Document> FindAllDocuments(const QueryType& query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy, typename DocumentPredicate, typename QueryType>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, QueryType& query, DocumentPredicate document_predicate) const;

    void MatchedDocumentProcessing(std::vector<Document>& matched_documents) const;
};

//============================================TEMPLATE_DEFINITION=======================================================

template <typename DocumentPredicate, typename QueryType>
std::vector<Document> SearchServer::FindAllDocuments(const QueryType& query, DocumentPredicate document_predicate) const
{
    std::map<int, double> doc_to_relevance_backet;
    for (const std::string_view& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word))
        {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    doc_to_relevance_backet[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
    }

    for (const std::string_view& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word))
        {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                doc_to_relevance_backet.erase(document_id);
            }
        }

    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : doc_to_relevance_backet)
    {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate, typename QueryType>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, QueryType& query, DocumentPredicate document_predicate) const {

    if (std::is_same_v <ExecutionPolicy, std::execution::sequenced_policy>) {
        return FindAllDocuments(query, document_predicate);
    }
    const size_t BACKETS_COUNT = 100;
    ConcurrentMap<int, double> doc_to_relevance_backet(BACKETS_COUNT);
    for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&](const auto& word) {
        if (word_to_document_freqs_.count(word))
        {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    doc_to_relevance_backet[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
        });
    std::map<int, double> document_to_relevance = std::move(doc_to_relevance_backet.BuildOrdinaryMap());
    for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) {
        if (word_to_document_freqs_.count(word))
        {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }
        });

    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());
    std::transform(document_to_relevance.begin(), document_to_relevance.end(), std::back_inserter(matched_documents), [&](const auto& data) {
        return Document{ data.first, data.second, documents_.at(data.first).rating };
        });
    return matched_documents;
}

//=============================================FIND_TOP_DOCUMENTS==============================================================

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);
    MatchedDocumentProcessing(matched_documents);
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    if (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    }

    QueryParallel query = ParseQuery(std::execution::par, raw_query);
    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);
    MatchedDocumentProcessing(matched_documents);
    return matched_documents;
}

