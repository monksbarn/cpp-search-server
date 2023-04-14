#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>

#include "remove_duplicates.h"

//уладение дубрирующихся документов
void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> unique_docs;
    std::set<int> id_docs_for_delete;
    for (int doc_id : search_server) {
        std::set<std::string> tmp;
        for (const auto& [word, data] : search_server.GetWordFrequencies(doc_id)) {
            tmp.insert(word);
        }
        const auto [It, insertion_result] = unique_docs.insert(tmp);
        if (!insertion_result) {
            id_docs_for_delete.insert(doc_id);
        }
    }
    for (int id : id_docs_for_delete) {
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}