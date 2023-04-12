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
    word_frequencies_.erase(document_id);
}
//сложность GetWordFrequencies должна быть O(log⁡N)O(logN);
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
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


/*
Расширьте поисковый сервер, добавив в него дополнительные методы.

    Откажитесь от метода GetDocumentId(int index) и вместо него определите методы begin и end. Они вернут итераторы. Итератор даст доступ к id всех документов, хранящихся в поисковом сервере. Вы можете не разрабатывать собственный итератор, а применить готовый константный итератор удобного контейнера.
     Если begin и end определены корректно, появится возможность использовать упрощённую форму for с поисковым сервером:

 SearchServer search_server;

 for (const int document_id : search_server) {
     // ...
 }


Разработайте метод получения частот слов по id документа:

 const map<string, double>& GetWordFrequencies(int document_id) const;


 Если документа не существует, возвратите ссылку на пустой map.
Разработайте метод удаления документов из поискового сервера

 void RemoveDocument(int document_id);


Вне класса сервера разработайте функцию поиска и удаления дубликатов:

 void RemoveDuplicates(SearchServer& search_server);


     Дубликатами считаются документы, у которых наборы встречающихся слов совпадают. Совпадение частот необязательно. Порядок слов неважен, а стоп-слова игнорируются. Функция должна использовать только доступные к этому моменту методы поискового сервера.
     При обнаружении дублирующихся документов функция должна удалить документ с большим id из поискового сервера, и при этом сообщить id удалённого документа в соответствии с форматом выходных данных, приведённым ниже.

Будьте аккуратны, если функция RemoveDuplicates проходит циклом по поисковому серверу так:

void RemoveDuplicates(SearchServer& search_server) {
    ...
    for (const int document_id : search_server) {
        ...
    }
    ...
}

В подобном случае удалять документы внутри цикла нельзя — это может привести к невалидности внутреннего итератора.
Все реализации должны быть эффективными. Если NN — общее количество документов, а WW — количество слов во всех документах, то:

    сложность GetWordFrequencies должна быть O(log⁡N)O(logN);
    сложность RemoveDocument должна быть O(w(log⁡N+log⁡W))O(w(logN+logW)), где ww — количество слов в удаляемом документе;
    сложность begin и end — O(1)O(1);
    сложность RemoveDuplicates должна быть O(wN(log⁡N+log⁡W))O(wN(logN+logW)), где ww — максимальное количество слов в документе.

В этом задании может потребоваться рефакторинг вашего кода. Например, замена одного вида контейнера на другой или введение нового индекса.
Вам предстоит оценить сложность разрабатываемых алгоритмов, чтобы знать, что они достаточно быстрые.
Формат выходных данных
Функция RemoveDuplicates должна для каждого удаляемого документа вывести в cout сообщение в формате Found duplicate document id N, где вместо N следует подставить id удаляемого документа.
Ограничения
Сохраните корректную и быструю работу всех методов кроме GetDocumentId.
Что отправлять на проверку
Загрузите полный код поискового сервера и вспомогательных функций. Код нужно разбить на файлы. Функция main при проверке будет игнорироваться.
Как будет тестироваться ваш код
Будет проверено, что:

    вы не изменили работу методов и функций, которые не описаны в условии,
    вы реализовали требуемые методы и функцию RemoveDuplicates,
    реализованные методы работают достаточно быстро и их сложность соответствует условию.

Пример использования

int main() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
}

Ожидаемый вывод этой программы:

Before duplicates removed: 9
Found duplicate document id 3
Found duplicate document id 4
Found duplicate document id 5
Found duplicate document id 7
After duplicates removed: 5
 */