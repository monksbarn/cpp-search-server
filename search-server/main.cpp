#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>

using namespace std;
// максимальное число документов, показанных в результате работы программы
const int MAX_RESULT_DOCUMENT_COUNT = 5;
// считывание документа
string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}
// считывание количества документов
int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
// разделение строки на слова и записьв вектор
vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    string minusWord;
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
// структура для сохранения результатов поиска
// в виде id документа и его релевантноси
struct Document
{
    int id;
    double relevance;
};
// класс для поиска и хранения промежуточных данных поиска
class SearchServer
{
public:
    // заполнение контейнера со стоп словами
    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }
    // заполнение контейнера word_to_document_freqs_
    void AddDocument(int document_id, const string &document)
    {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string &w : words)
        {
            double freqs = 1.0 / words.size();
            // word_to_document_freqs_[w].[document_id] += freqs; конструкция с точкой между квадратными скобкани  не компилируется
            // могли бы уточнить, почему?
            word_to_document_freqs_[w][document_id] += freqs;
        }
    }
    // сортировка релевантных запросу результатов и
    // и записьв количестве, не превышающем максимальное
    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    // структура для хранения слов запроса (прюс слов)
    // и слов, присутствие которых в документе, должно
    // удалять документ их соответствующих запросу (минус слов)
    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    int document_count_ = 0;
    // контейнер map для хранения слов из документов и id документов,
    // где это слово встречается (из ранних реализаций)
    map<string, set<int>> documents_;
    // контейнер set для хранения стоп-слов
    set<string> stop_words_;
    // определение соответствия переданного в метод слова стоп-слову
    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }
    // контейнер map для хранения слова, id документов, в которых оно есть,
    // а также параметра TF (отноешние количества слов из запроса, присутствующих
    // в документе, к общему количеству слов в документе)
    map<string, map<int, double>> word_to_document_freqs_;
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
    // разделение поискового запроса на вектор строк,
    // исключая стоп-слова, которые не должны принмать участия в поиске

    Query ParseQuery(const string &text) const
    {
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // стоит организовать отдельный метод по обработке отдельных слов запроса
        // и возврата характеристик в структурированном виде - сам слова и его признаки (стоп и минус)
        //
        // здесь не понял, что именно нужно изменить?
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Query query;
        for (const string &word : SplitIntoWordsNoStop(text))
        {
            if (word[0] == '-')
                query.minus_words.insert(word.substr(1));
            else
                query.plus_words.insert(word);
        }
        return query;
    }
    // поиск всех соответствующих поисковому запросу документов,
    // вычисление релевантности документа запросу,
    // исключение документов, содержащих минус-слова
    double getIDF(const string &word) const
    {
        return log((double)document_count_ / word_to_document_freqs_.at(word).size());
    }
    vector<Document> FindAllDocuments(const Query &query_words) const
    {
        vector<Document> matched_documents;
        map<int, double> relevanceDoc;
        // IDF количество всех документов делят на количество тех, где встречается слово + log получивегося значения
        // TF отношение числа повторений слова в документе к общему количеству слов в данном документе
        // вычисляется IDF каждого слова в запросе,
        // вычисляется TF каждого слова запроса в документе  в методе AddDocument(),
        // IDF каждого слова запроса умножается на TF этого слова в этом документе,
        // все произведения IDF и TF в документе суммируются.
        for (const string &q : query_words.plus_words)
        {
            if (word_to_document_freqs_.find(q) == word_to_document_freqs_.end())
                continue;
            double IDF = getIDF(q);
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            // стоит использовать декомпозиция переменной,чтобы не работать с id.first и id.second. Декомпозицию делают через const auto& [var1, var2]
            //
            // for (const auto & [var1,var2]: word_to_document_freqs_.at(q)) не компилируется (identifier var1 is undefined)
            // возможно, я что-то неправильно пишу... версия моего компилятора может не поддержитвать такую конструкцию?
            //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            for (const auto &id : word_to_document_freqs_.at(q))
            {
                relevanceDoc[id.first] += id.second * IDF;
            }
        }
        // удаление минус-слов
        for (const string &sWord : query_words.minus_words)
        {
            if (word_to_document_freqs_.find(sWord) == word_to_document_freqs_.end())
                continue;
            for (const auto &id : word_to_document_freqs_.at(sWord))
            {
                relevanceDoc.erase(id.first);
            }
        }
        // копирование данных из контейнера map в вектор
        for (const auto &id : relevanceDoc)
        {
            matched_documents.push_back({id.first, id.second});
        }
        return matched_documents;
    }
    int ComputeAverageRating(const vector<int> &ratings) const
    {
        return accumulate(ratings.begin(), ratings.end(), 0) / ratings.size();
    }
};
// функция создания класса
SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    for (const auto &ss : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << ss.id << ", "
             << "relevance = "s << ss.relevance << " }"s << endl;
    }
    return 0;
}

