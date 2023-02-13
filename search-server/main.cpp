#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <sstream>

using namespace std;



const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
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

struct Document
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status,
                     const vector<int> &ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus external_status = DocumentStatus::ACTUAL) const
    {
        return FindTopDocuments(raw_query, [external_status](int document_id, DocumentStatus status, int rating)
                                { return status == external_status; });
    }
    template <typename Filter>
    vector<Document> FindTopDocuments(const string &raw_query, Filter filter) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6)
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

    int GetDocumentCount() const
    {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
                                                        int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string &word : query.plus_words)
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
        for (const string &word : query.minus_words)
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
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings)
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

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    template <typename Filter>
    vector<Document> FindAllDocuments(const Query &query, Filter filter) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                if (filter(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words)
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

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

//=================================FOR TESTING================================================

template <typename T>
ostream &operator<<(ostream &out, vector<T> container)
{
    out << "[";
    bool is_start = true;
    for (const auto &item : container)
    {
        if (!is_start)
            out << ", ";
        out << item;
        is_start = false;
    }
    out << "]";
    return out;
}
template <typename T>
ostream &operator<<(ostream &out, set<T> container)
{
    out << "{";
    bool is_start = true;
    for (const auto &item : container)
    {
        if (!is_start)
            out << ", ";
        out << item;
        is_start = false;
    }
    out << "}";
    return out;
}
template <typename key, typename value>
ostream &operator<<(ostream &out, map<key,value> container)
{
    out << "{";
    bool is_start = true;
    for (const auto &[name,item] : container)
    {
        if (!is_start)
            out << ", ";
        out << name << ": " << item;
        is_start = false;
    }
    out << "}";
    return out;
}


template <typename T>
void RunTestImpl(const T function_execution, const string &function_name)
{
    function_execution();
    cerr << function_name << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str, const string &u_str, const string &file,
                     const string &func, unsigned line, const string &hint)
{
    if (t != u)
    {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

void AssertImpl(bool value, const string &expr_str, const string &file, const string &func, unsigned line,
                const string &hint)
{
    if (!value)
    {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty())
        {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

//==========================================================================================

//============================TESTING SEARH SERVER========================================
void TestExcludeStopWordsFromAddedDocumentContent()
{
    int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document &doc0 = found_docs[0];
       // ASSERT(doc0.id == doc_id);
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }

    {
        SearchServer server;
        server.AddDocument(1, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("-cat in the city").empty());
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<string> query = {"in", "the", "city"};
        string str_query;
        bool is_start = true;
        for (const string &q : query)
        {
            if (!is_start)
            {
                str_query += " ";
            }
            str_query += q;
            is_start = false;
        }
        vector<string> match_words;
        DocumentStatus status;
        string minus = "";
        tie(match_words, status) = server.MatchDocument(str_query + minus, doc_id);
        if (minus.empty())
            for (const string &word : match_words)
            {
                ASSERT(count(query.begin(), query.end(), word));
            }
        else
            ASSERT(match_words.empty());
    }

    {
        SearchServer server;
        vector<string> documents = {"cat in the city", "dog in the town", "sparow and dog at the ship"};
        string query = "dog ship";
        for (const string &document : documents)
        {
            server.AddDocument(doc_id++, document, DocumentStatus::ACTUAL, ratings);
        }

        vector<Document> top = server.FindTopDocuments(query);
        ASSERT(top[0].relevance == (1. / 6. * log(3. / 1.) + 1. / 6. * log(3. / 2.)));
        ASSERT(top[1].relevance == .25 * log(3. / 2.));
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        double rating = 0.;
        for (const int r : ratings)
        {
            rating += r;
        }
        rating /= static_cast<int>(ratings.size());
        vector<Document> top = server.FindTopDocuments(content);
        //ASSERT(top[0].rating == rating);
        ASSERT_EQUAL(top[0].rating,rating);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> top = server.FindTopDocuments(content, [](int document_id, DocumentStatus status, int rating)
                                                       { return document_id % 2 == 0; });
        if (doc_id % 2 == 0)
            ASSERT(top.size());
        else
            ASSERT(top.empty());
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        vector<Document> top = server.FindTopDocuments(content, DocumentStatus::BANNED);
        ASSERT(!top.empty());
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        double relevance = .25 * log(1);
        relevance *= 4;
        vector<Document> top = server.FindTopDocuments(content);
        //ASSERT(top[0].relevance == relevance);
        ASSERT_EQUAL(top[0].relevance,relevance);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
}


int main()
{
    TestSearchServer();
    /* SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
    {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                                   { return status == DocumentStatus::ACTUAL; }))
    {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                                   { return document_id % 2 == 0; }))
    {
        PrintDocument(document);
    } */
    return 0;
}

/*

Задание
Разработайте юнит-тесты для своей поисковой системы. Примените в них макрос ASSERT, чтобы проверить работу основных функций, таких как:

    Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
    Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
    Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
    Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
    Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
    Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
    Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
    Поиск документов, имеющих заданный статус.
    Корректное вычисление релевантности найденных документов.

Как будет происходить проверка этого задания
У разработанных вами тестов должна быть точка входа, заданная функцией TestSearchServer. Код поисковой системы должен успешно проходить тесты. Тренажёр проверит работу ваших тестов на нескольких предложенных реализациях класса SearchServer. Одна из реализаций будет корректной, в других есть ошибки в логике работы класса. Задача считается решённой при выполнении следующих условий:

    Корректная реализация класса SearchServer успешно проходит разработанные вами тесты.
    Ваши тесты выявляют не менее 50% некорректных реализаций класса SearchServer.

Тренажёр ожидает, что ваша реализация класса SearchServer будет содержать следующие публичные методы:

struct Document {
    int id;
    double relevance;
    int rating;
};
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public:
    void SetStopWords(const string& text) {
        // Ваша реализация данного метода
    }
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        // Ваша реализация данного метода
    }
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        // Ваша реализация данного метода
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        // Ваша реализация данного метода
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        // Ваша реализация данного метода
    }
    int GetDocumentCount() const {
        // Ваша реализация данного метода
    }
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        // Ваша реализация данного метода
    }
private:
    // Реализация приватных методов вашей поисковой системы
};

Прекод
Перед вами исходный код с примером теста, который проверяет, что стоп-слова исключаются поисковой системой при добавлении документа.
На проверку отправьте только сами тесты, располагающиеся между комментариями:
// -------- Начало модульных тестов поисковой системы ---------- и
 // --------- Окончание модульных тестов поисковой системы -----------.

 */