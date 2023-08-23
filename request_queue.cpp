#include "request_queue.h"
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include "document.h"
#include "search_server.h"

    RequestQueue::RequestQueue(const SearchServer& server) : search_server(server) {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return ManageRequest(search_server.FindTopDocuments(raw_query, status));
    }
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        return ManageRequest(search_server.FindTopDocuments(raw_query));
    }
    int RequestQueue::GetNoResultRequests() const {
        return std::count_if(requests_.begin(), requests_.end(), [](QueryResult res) {return !res.request_type;});
    }

    std::vector<Document> RequestQueue::ManageRequest(const std::vector<Document>& documents) {
        if (requests_.size() >= min_in_day_) {
            requests_.pop_front();
        }
        if (!documents.empty()) {
            requests_.push_back({true});
        } else {
            
            requests_.push_back({false});
        }
        return documents;
    }