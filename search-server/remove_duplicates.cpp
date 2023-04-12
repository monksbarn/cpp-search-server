#include <string>
#include <map>
#include <vector>
#include <set>

#include "remove_duplicates.h"

//сложность RemoveDuplicates должна быть O(wN(log⁡N+log⁡W))O(wN(logN+logW)), где ww — максимальное количество слов в документе.
void RemoveDuplicates(SearchServer& search_server) {
    std::map<int, std::map<std::string, double>> all_docs;
    std::set<int> id_docs_for_delete;
    bool match = false;
    for (const int doc_id : search_server) {
        all_docs[doc_id] = search_server.GetWordFrequencies(doc_id);
    }
    std::map<int, std::map<std::string, double>>::iterator Iterator_1 = all_docs.begin();
    for (;Iterator_1 != all_docs.end();++Iterator_1) {
        std::map<int, std::map<std::string, double>>::iterator Iterator_2 = Iterator_1;
        advance(Iterator_2, 1);
        for (;Iterator_2 != all_docs.end();++Iterator_2) {
            if ((*Iterator_1).second.size() != (*Iterator_2).second.size()) {
                continue;
            }
            for (auto j = (*Iterator_1).second.begin(); j != (*Iterator_1).second.end(); ++j) {
                match = (*Iterator_2).second.count((*j).first);
                if (!match) {
                    break;
                }
            }
            if (match) {
                id_docs_for_delete.insert((*Iterator_2).first);
            }
        }
    }
    for (const auto& id : id_docs_for_delete) {
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}