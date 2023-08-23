#include "remove_duplicates.h"
#include "search_server.h"
#include <set>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> unique_document_texts;
    std::vector<int> documents_to_remove;

    for (int document_id : search_server) {
        const std::set<std::string> document_words = search_server.GetDocumentWordsById(document_id);
        
        // Если такой набор слов уже встречался, добавляем документ в список на удаление
        if (unique_document_texts.count(document_words) > 0) {
            documents_to_remove.push_back(document_id);
        } else {
            // В противном случае, добавляем этот набор слов в набор уже встреченных
            unique_document_texts.insert(document_words);
        }
    }

    // Удаляем документы, которые находятся в списке на удаление
    for (int document_id : documents_to_remove) {
        search_server.RemoveDocument(document_id);
    }
}
