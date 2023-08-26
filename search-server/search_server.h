#pragma once
 
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include "string_processing.h"
#include "document.h"
 
using namespace std::literals;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
 
class SearchServer {
public:
    template <typename StringContainer>
explicit SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& stop_word : stop_words) {
            if (!IsValidWord(stop_word)) {
                throw std::invalid_argument("Некорректное содержание в списке 'стоп-слов'"s);
            }
        }
    }

 
    explicit SearchServer(const std::string& stop_words_text);
 
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
 
    std::set<std::string> GetDocumentWordsById(int document_id) const;
 
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
 
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
 
    int GetDocumentId(int index);
 
    std::set<int>::const_iterator begin();
 
    std::set<int>::const_iterator end();
 
    int GetDocumentCount() const;
 
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
 
     void RemoveDocument(int document_id);
 
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::set<std::string>> words_to_documents_;
    std::set<int> documents_id_;
 
    bool IsStopWord(const std::string& word) const;
 
    static bool IsValidWord(const std::string& word) {  
        // A valid word must not contain special characters  
        return none_of(word.begin(), word.end(), [](char c) {  
            return c >= '\0' && c < ' ';  
        });  
    }
    
   template <typename Container>
static bool IsValidText(const Container& text) {
    for (const auto& word : text) {
        if (!IsValidWord(word) || word.empty()) {
            return false;
        }
    }
    return true;
}
 
 
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
 
    static int ComputeAverageRating(const std::vector<int>& ratings);
 
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
 
    QueryWord ParseQueryWord(std::string text) const;
 
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
 
    Query ParseQuery(const std::string& text) const;
 
    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;
 
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};
 
// Вне класса SearchServer:
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
 
    std::vector<Document> documents;
    const Query query = ParseQuery(raw_query);
    if (!IsValidText(query.plus_words) || !IsValidText(query.minus_words)) {
        throw std::invalid_argument("Некорректное содержание в списке слов запроса"s);
    }
    documents = FindAllDocuments(query, document_predicate);
 
    sort(documents.begin(), documents.end(),
         [](const Document& lhs, const Document& rhs) {
            double ALLOWABLE_ERROR = 1e-6;
             if (std::abs(lhs.relevance - rhs.relevance) < ALLOWABLE_ERROR) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return documents;
}
 
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
 
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
 
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}