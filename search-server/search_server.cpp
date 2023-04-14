#include "search_server.h"
#include "log_duration.h"


std::set<int>::iterator SearchServer::begin() const {
    return documents_id_.begin();
}
std::set<int>::iterator SearchServer::end() const {
    return documents_id_.end();
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings)
{
    // если id меньше нуля или id уже есть среди добавленных документов или переданная
    // строка содрежит спецсимволы, вернуть false
    if (document_id < 0)
        throw std::invalid_argument("ID cannot be negative");
    if (documents_.count(document_id))
        throw std::invalid_argument("this ID already exists");
    SearchServer::documents_id_.insert(document_id);
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_frequencies_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
}

void SearchServer::AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}

//сложность RemoveDocument должна быть O(w(log⁡N+log⁡W))O(w(logN+logW)), где ww — количество слов в удаляемом документе;
void SearchServer::RemoveDocument(int document_id) {
    documents_id_.erase(document_id);
    documents_.erase(document_id);
    for (auto& [word, data] : word_frequencies_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    word_frequencies_.erase(document_id);

}
//сложность GetWordFrequencies должна быть O(log⁡N)O(logN);
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static  std::map<std::string, double> empty_map_;
    return word_frequencies_.count(document_id) ? word_frequencies_.at(document_id) : empty_map_;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

void SearchServer::FindTopDocuments(const SearchServer& search_server, std::string raw_query) {
    search_server.FindTopDocuments(raw_query);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
    Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
        }
    }
    return std::tuple{ matched_words, documents_.at(document_id).status };
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings)
    {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
    bool is_minus = false;
    // Word shouldn't be empty
    if (text.front() == '-')
    {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
    Query query;
    for (const std::string& word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (query_word.data.empty() || query_word.data.front() == '-')
            throw std::invalid_argument("it is not allowed to use \"--word\" or only \"-\" in the request.\nCorrectly: \"-word\"");
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.insert(query_word.data);
            }
            else
            {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
bool SearchServer::IsStopWord(const std::string& word) const
{
    return stop_words_.count(word);
}


