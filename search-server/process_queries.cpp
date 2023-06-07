#include <numeric>
#include <execution>

#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&](const std::string& query) {
        return search_server.FindTopDocuments(query);
        });
    return result;
}

QueriesJoined<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    QueriesJoined<Document> result;
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        result.push_back(documents);
    }
    return result;
}