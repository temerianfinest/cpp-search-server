#include "remove_duplicates.h"
#include "search_server.h"
#include <set>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicates;
    for (auto iter = search_server.begin(); iter != search_server.end(); ++iter) {
        duplicates.merge(search_server.FindDuplicates(*iter));
    }
    

    for (int document_id : duplicates) {
        search_server.RemoveDocument(document_id);
    }
}