#pragma once
#include <vector>
#include <deque>
#include <string>
#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const {
        return empty_requests_;
    }
private:
    const SearchServer& search_server_;
    struct QueryResult {
        bool request_success;
        int time_request = 0;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int time_count_ = 1;
    int empty_requests_ = 0;
    // возможно, здесь вам понадобится что-то ещё
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    if (time_count_++ > min_in_day_) {
        requests_.pop_front();
        --empty_requests_;
    }
    std::vector<Document> find_top_document = search_server_.FindTopDocuments(raw_query, document_predicate);
    QueryResult tmp;
    tmp.request_success = !find_top_document.empty();
    tmp.time_request = time_count_;
    requests_.push_back(tmp);
    if (!tmp.request_success) {
        empty_requests_++;
    }
    return find_top_document;
}