#include "search_server.h"
#include "log_duration.h"

#include <functional>

std::set<int>::iterator SearchServer::begin() const
{
    return documents_id_.begin();
}
std::set<int>::iterator SearchServer::end() const
{
    return documents_id_.end();
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
    // если id меньше нуля или id уже есть среди добавленных документов или переданная
    // строка содрежит спецсимволы, вернуть false
    if (document_id < 0)
        throw std::invalid_argument("ID cannot be negative");
    if (documents_.count(document_id))
        throw std::invalid_argument("this ID already exists");
    SearchServer::documents_id_.insert(document_id);
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words)
    {
        const auto [new_word, inserted] = all_words_.insert(static_cast<std::string>(word));
        word_to_document_freqs_[*new_word][document_id] += inv_word_count;
        word_frequencies_[document_id][*new_word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
}

void SearchServer::AddDocument(SearchServer& search_server, int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
    search_server.AddDocument(document_id, document, status, ratings);
}

// сложность RemoveDocument должна быть O(w(log⁡N+log⁡W))O(w(logN+logW)), где ww — количество слов в удаляемом документе;
void SearchServer::RemoveDocument(int document_id)
{
    if (documents_id_.count(document_id))
    {
        documents_id_.erase(document_id);
        documents_.erase(document_id);
        word_frequencies_.erase(document_id);
        for (auto& [word, data] : word_to_document_freqs_)
        {
            data.erase(document_id);
        }
    }
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& seq, int document_id)
{
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& par, int document_id)
{
    if (documents_id_.find(document_id) != documents_id_.end())
    {
        documents_.erase(document_id);
        documents_id_.erase(document_id);
        std::vector<std::string_view> tmp(word_frequencies_.at(document_id).size());
        std::transform(par, word_frequencies_.at(document_id).begin(), word_frequencies_.at(document_id).end(), tmp.begin(), [](const auto& data)
            { return data.first; });
        std::for_each(par, tmp.begin(), tmp.end(), [&](const auto& word)
            { SearchServer::word_to_document_freqs_.at(word).erase(document_id); });
        word_frequencies_.erase(document_id);
    }
}

// сложность GetWordFrequencies должна быть O(log⁡N)O(logN);
// const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
// {
//     static std::map<std::string, double> empty_map_;
//     return word_frequencies_.count(document_id) ? word_frequencies_.at(document_id) : empty_map_;
// }

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string_view, double> empty_map_;
    return word_frequencies_.count(document_id) ? word_frequencies_.at(document_id) : empty_map_;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(
        raw_query, [&status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(
        raw_query, [&status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(
        std::execution::par, raw_query, [&status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query) const
{
    return FindTopDocuments(raw_query);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query) const
{
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

void SearchServer::FindTopDocuments(const SearchServer& search_server, const std::string_view raw_query)
{
    search_server.FindTopDocuments(raw_query);
}

void SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, const SearchServer& search_server, const std::string_view raw_query)
{
    search_server.FindTopDocuments(raw_query);
}

void SearchServer::FindTopDocuments(const std::execution::parallel_policy&, const SearchServer& search_server, const std::string_view raw_query)
{
    search_server.FindTopDocuments(std::execution::par, raw_query);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
    Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    if (word_frequencies_.count(document_id)) {
        bool have_minus_word = std::any_of(query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view& word)
            { return (word_frequencies_.at(document_id).count(word)); });
        if (!have_minus_word)
        {
            std::copy_if(query.plus_words.begin(), query.plus_words.end(), std::back_inserter(matched_words), [&](const std::string_view& word) {
                return word_frequencies_.at(document_id).count(word);
                });
        }
    }
    return std::tuple{matched_words, documents_.at(document_id).status};
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& seq, const std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& par, const std::string_view raw_query, int document_id) const
{
    if (!documents_id_.count(document_id)) {
        throw std::out_of_range("id not exists");
    }
    QueryParallel query = ParseQuery(par, raw_query);
    std::vector<std::string_view> matched_words;
    if (word_frequencies_.count(document_id)) {
        bool have_minus_word = std::any_of(query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view& word)
            { return (word_frequencies_.at(document_id).count(word)); });
        if (!have_minus_word)
        {
            std::copy_if(query.plus_words.begin(), query.plus_words.end(), std::back_inserter(matched_words), [&](const std::string_view& word) {
                return word_frequencies_.at(document_id).count(word);
                });
        }
    }
    return std::tuple{matched_words, documents_.at(document_id).status};
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    return ratings.empty() ? 0 : std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const
{
    return (!text.empty() && text.front() == '-') ? QueryWord{text.substr(1), true, IsStopWord(text)} : QueryWord{ text, false, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
    Query query;
    for (const std::string_view& word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (query_word.data.empty() || query_word.data.front() == '-')
            throw std::invalid_argument("it is not allowed to use \"--word\" or only \"-\" in the request.\nCorrectly: \"-word\"");
        if (!query_word.is_stop)
        {
            (query_word.is_minus) ? query.minus_words.insert(query_word.data) : query.plus_words.insert(query_word.data);
        }
    }
    return query;
}

SearchServer::QueryParallel SearchServer::ParseQuery(const std::execution::parallel_policy&, const std::string_view text) const
{
    QueryParallel query;
    std::vector<std::string_view> separate_text = std::move(SplitIntoWords(text));
    std::sort(separate_text.begin(), separate_text.end());
    separate_text.erase(std::unique(separate_text.begin(), separate_text.end()), separate_text.end());
    auto bound = std::find_if_not(separate_text.begin(), separate_text.end(), [](const auto& word) {
        return word.front() == '-';
        });
    std::transform(separate_text.begin(), bound, separate_text.begin(), [](auto& word) {
        return word.substr(1);
        });
    query.minus_words.reserve(std::distance(separate_text.begin(), bound));
    std::copy_if(separate_text.begin(), bound, std::back_inserter(query.minus_words), [&](const auto& word) {
        return(!IsStopWord(word));
        });
    query.plus_words.reserve(std::distance(bound, separate_text.end()));
    std::copy_if(bound, separate_text.end(), std::back_inserter(query.plus_words), [&](const auto& word) {
        return (!IsStopWord(word));
        });
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsStopWord(const std::string_view word) const
{
    return stop_words_.count(word);
}