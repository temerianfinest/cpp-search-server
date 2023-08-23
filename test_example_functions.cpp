#include "test_example_functions.h"
#include <iostream>
#include "search_server.h"
#include "log_duration.h"
#include "document.h"

void AddDocument(SearchServer search_server,int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    LOG_DURATION();
    AddDocument(search_server, document_id, document, status, ratings);
}