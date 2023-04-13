#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>

#include "remove_duplicates.h"

//сложность RemoveDuplicates должна быть O(wN(log⁡N+log⁡W))O(wN(logN+logW)), где ww — максимальное количество слов в документе.
void RemoveDuplicates(SearchServer& search_server) {
    std::map<int, std::map<std::string, double>> all_docs;
    std::set<int> id_docs_for_delete;
    for (const int doc_id : search_server) {
        const auto tmp = search_server.GetWordFrequencies(doc_id);
        all_docs[doc_id] = tmp;
        if (all_docs.size() > 1) {
            std::map<int, std::map<std::string, double>>::iterator It = all_docs.begin();
            for (;It != all_docs.end();++It) {
                if (tmp.size() != (*It).second.size() || (*It).first == doc_id) {
                    continue;
                }
                bool match = false;
                for (const auto& [word, tf] : (*It).second) {
                    match = tmp.count(word);
                    if (!match) {
                        break;
                    }
                }
                if (match) {
                    id_docs_for_delete.insert(doc_id);
                }
            }
        }
    }
    for (const auto& id : id_docs_for_delete) {
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}