#include "document.h"
#include <iostream>
#include <string>

void PrintDocument(const Document& document) {
    std::cout << "{ "
         << "document_id = " << document.id << ", "
         << "relevance = " << document.relevance << ", "
         << "rating = " << document.rating << " }" << std::endl;
}

std::ostream& operator<<(std::ostream& os, const Document& document) {
        os << "{ " << "document_id" << " = " << document.id <<
        ", relevance = " << document.relevance <<
        ", rating = " << document.rating << " }";
        return os;
}