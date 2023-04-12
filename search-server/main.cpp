#include <iostream>

#include "paginator.h"
#include "document.h"
#include "read_input_functions.h"
#include "search_server.h"
#include "request_queue.h"
#include "remove_duplicates.h"

using namespace std;



int main() {
    /* SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
    //Total empty requests: 1437

 */
 /* SearchServer search_server("and with"s);
 search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
 search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
 search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
 search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
 search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
 {

 }
 SearchServer::FindTopDocuments(search_server, "curly dog"s);
 const auto search_results = search_server.FindTopDocuments("curly dog"s);
 int page_size = 2;
 const auto pages = Paginate(search_results, page_size);
 // Выводим найденные документы по страницам

 for (auto page = pages.begin(); page != pages.end(); ++page) {
     cout << *page << endl;
     cout << "Page break"s << endl;
 }
 for (const int document_id : search_server) {
     cout << document_id << endl;
 } */

    SearchServer search_server("and with"s);

    SearchServer::AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    SearchServer::AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    SearchServer::AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    SearchServer::AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    SearchServer::AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    SearchServer::AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    SearchServer::AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    SearchServer::AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    SearchServer::AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
    //______________________________________________________________________________ 
    /*  try
     {
         SearchServer search_server("и в на"s);
         search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
         search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
         search_server.AddDocument(0, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
         search_server.AddDocument(3, "большой пёс скворец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
         for (const Document& document : search_server.FindTopDocuments("пушистый пёс -скворец"s))
         {
             PrintDocument(document);
         }
         cout << search_server.GetDocumentId(90) << endl;
     }
     catch (const invalid_argument& error)
     {
         cout << "Error: " << error.what() << endl;
     }
     catch (const out_of_range& error)
     {
         cout << "Error: " << error.what() << endl;
     }
     return 0; */
}
