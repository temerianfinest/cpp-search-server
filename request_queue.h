#pragma once

#include <vector>
#include <string>
#include <deque>
#include <algorithm>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        return ManageRequest(search_server.FindTopDocuments(raw_query, document_predicate));
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool request_type;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server;
    
    std::vector<Document> ManageRequest(const std::vector<Document>& documents);
};