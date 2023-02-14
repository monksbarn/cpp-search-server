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
#define COMPARISON_ERROR (1e-6)

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
                 if (abs(lhs.relevance - rhs.relevance) < COMPARISON_ERROR)
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
ostream &operator<<(ostream &out, map<key, value> container)
{
    out << "{";
    bool is_start = true;
    for (const auto &[name, item] : container)
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
/*

 Лучше для простоты отладки и полноты разделить тесты на отдельные тесты-функции, дополнив их: тесты на добавление документа, что они все находятся в нём, тесты на то, что релевантность убывает в выдаче, тесты на расчёт рейтинга (3 вида оценок - положительные. отрицательные и смешанные), тесты на  поиск по всем статусам, тесты на поиск по предикате и тесты на расчёт ненулевой релевантности (сравнить релевантность и результаты по через сравнения модуля разности с погрешности).

 */
void TestAddDocument()
{
    int id_doc_first = 42;
    int id_doc_second = 1;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "big dog behind the sun"s;
    const vector<int> ratings_doc_first = {1, 2, 3};
    const vector<int> ratings_doc_second = {10, -1, 9};
    SearchServer server;
    server.AddDocument(id_doc_first, content_doc_first, DocumentStatus::ACTUAL, ratings_doc_first);
    server.AddDocument(id_doc_second, content_doc_second, DocumentStatus::BANNED, ratings_doc_second);
    vector<Document> found_docs = server.FindTopDocuments("in"s);
    ASSERT(found_docs.size() == 1);
    ASSERT_EQUAL(found_docs.front().id, id_doc_first);
    ASSERT_EQUAL(found_docs.front().rating, 2);
    found_docs = server.FindTopDocuments("behind"s, DocumentStatus::BANNED);
    ASSERT(found_docs.size() == 1);
    ASSERT_EQUAL(found_docs.front().id, id_doc_second);
    ASSERT_EQUAL(found_docs.front().rating, 6);
}
void TestFindStopWord()
{
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in the city", DocumentStatus::ACTUAL, {5, 5, 5});
    ASSERT(server.FindTopDocuments("in"s).empty());
}
void TestFindMinusWord()
{
    SearchServer server;
    server.AddDocument(1, "cat in the city", DocumentStatus::ACTUAL, {5, 5, 5});
    ASSERT(server.FindTopDocuments("-cat in the city").empty());
}
void TestMatchDocument()
{
    SearchServer server;
    int doc_id = 1;
    server.AddDocument(doc_id, "cat in the city", DocumentStatus::ACTUAL, {5, 5, 5});
    vector<string> match_words;
    DocumentStatus status;
    tie(match_words, status) = server.MatchDocument("cat city", doc_id); // tuple<vector<string>, DocumentStatus>
    ASSERT(count(match_words.begin(), match_words.end(), "cat"));
    ASSERT(count(match_words.begin(), match_words.end(), "city"));
    ASSERT(!count(match_words.begin(), match_words.end(), "the"));
    ASSERT_EQUAL(static_cast<int>(status), static_cast<int>(DocumentStatus::ACTUAL));
}
void TestRelevanseFoundedDocuments(){
    {
        int doc_id = 0;
        SearchServer server;
        vector<string> documents = {"cat in the city", "dog in the town", "sparow and dog at the ship"};
        string query = "dog ship";
        for (const string &document : documents)
            server.AddDocument(doc_id++, document, DocumentStatus::ACTUAL, {5,5,5});
        vector<Document> top = server.FindTopDocuments(query);
        double doc_2_relevanse = .25 * log(3. / 2.);
        double doc_3_relevanse = 1. / 6. * log(3. / 1.) + 1. / 6. * log(3. / 2.);
        ASSERT_EQUAL(top.front().relevance, doc_3_relevanse);
        ASSERT_EQUAL(top.back().relevance, doc_2_relevanse);
        ASSERT(doc_3_relevanse > doc_2_relevanse);
    }
}
void TestRatingAndEqualRelevanceDocuments()
{
    int doc_id = 0;
    vector<int> ratings_positive = {1, 5, 6, 8, 5};//5
    vector<int> ratings_negative = {-4, -9, -4, -1, -7, -3};//-4
    vector<int> ratings_mixed = {-1, 5, 7, -10, 1, 6};//1
    vector<vector<int>> ratings= {ratings_positive, ratings_negative, ratings_mixed};
    vector<int> average_ratings = {5, 1, -4};
    SearchServer server;
    for (doc_id; doc_id < 3;++doc_id)
        server.AddDocument(doc_id, "cat in the forest", DocumentStatus::ACTUAL, ratings[doc_id]);
    vector<Document> top = server.FindTopDocuments("cat");
    int i = 0;
    for (const auto &doc : top)
        ASSERT_EQUAL(doc.rating, average_ratings[i++]);
}
void TestPredicatFunction(){
    {
        SearchServer server;
        const int odd_id = 1;
        const int even_id = 2;
        server.AddDocument(odd_id, "the horse in the coat", DocumentStatus::ACTUAL, {7, 7, 7});
        server.AddDocument(even_id, "the horse in the coat", DocumentStatus::BANNED, {7, 7, 7});
        vector<Document> top = server.FindTopDocuments("horse", [](int document_id, DocumentStatus status, int rating)
                                                       { return document_id % 2 == 0; });
        ASSERT_EQUAL(top.front().id, even_id);
        ASSERT_EQUAL(top.size(), 1);
        top = server.FindTopDocuments("horse", DocumentStatus::BANNED);
        ASSERT_EQUAL(top.front().id, even_id);
        ASSERT_EQUAL(top.size(), 1);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestFindStopWord);
    RUN_TEST(TestFindMinusWord);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevanseFoundedDocuments);
    RUN_TEST(TestRatingAndEqualRelevanceDocuments);
    RUN_TEST(TestPredicatFunction);
}

int main()
{
    RUN_TEST(TestSearchServer);
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
