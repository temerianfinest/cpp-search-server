#include "search_server.h"  
#include <string>  
#include <vector>  
#include <set>  
#include <map>  
#include <stdexcept>  
#include <algorithm>  
#include <cmath>  
#include <iterator>  
#include "document.h"  
#include "string_processing.h"  
#include "log_duration.h"  
 
using namespace std::literals;  
 
SearchServer::SearchServer(const std::string& stop_words_text)  
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container  
{  
}  
 
void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {  
    LogDuration("AddDocument");  
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);  
    if (document_id < 0 || documents_.count(document_id) != 0) {  
        throw std::invalid_argument("Некорректный id документа"s);  
    }  
    const double inv_word_count = 1.0 / words.size();  
    for (const std::string& word : words) {  
        word_to_document_freqs_[word][document_id] += inv_word_count;  
        words_to_documents_[document_id].insert(word);  
    }  
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});  
    documents_id_.insert(document_id);  
}  
 
std::set<std::string> SearchServer::GetDocumentWordsById(int document_id) const {   
    static std::set<std::string> empty_set;  // Статическая переменная для пустого множества
    if (words_to_documents_.count(document_id)) {   
        return words_to_documents_.at(document_id);   
    } else {   
        return empty_set;  // Возвращаем статическое пустое множество
    }   
} 
 
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {  
    return FindTopDocuments(  
        raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {  
            return document_status == status;  
        });  
}  
 
int SearchServer::GetDocumentId(int index) {
    if (index >= 0 && index < documents_id_.size()) {
        auto it = documents_id_.begin();
        std::advance(it, index);
        return *it;
    } else {
        throw std::out_of_range("Некорректный индекс документа");
    }
} 
 
std::set<int>::const_iterator  SearchServer::begin() {  
    return documents_id_.begin();  
}  
 
std::set<int>::const_iterator  SearchServer::end() {  
    return documents_id_.end();  
}  
 
void SearchServer::RemoveDocument(int document_id) {  
    // Удаляем соответствующие слова из индекса  
    for (auto& word : words_to_documents_[document_id]) {  
        word_to_document_freqs_[word].erase(document_id);  
    }  
 
    // Удаляем информацию о документе из words_to_documents_ и documents_id_  
    words_to_documents_.erase(document_id);  
    documents_id_.erase(document_id);  
 
    // Удаляем информацию о документе из documents_  
    documents_.erase(document_id);  
}  
 
int SearchServer::GetDocumentCount() const {  
    return documents_.size();  
}  
 
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
 
 
    if (!IsValidText(query.plus_words) || !IsValidText(query.minus_words)) {
        throw std::invalid_argument("Некорректное содержание в списке слов запроса"s);
    }
 
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
 
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
 
    return {matched_words, documents_.at(document_id).status};
}
 
 
bool SearchServer::IsStopWord(const std::string& word) const {  
    return stop_words_.count(word) > 0;  
}  
 
bool SearchServer::IsValidWord(const std::string& word) {  
    if (word[0] == '-') {  
        return false;  
    }  
 
    return std::none_of(word.begin(), word.end(), [](char c) {  
        return c >= '\0' && c < ' ';  
    });  
} 
 
 
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {  
    std::vector<std::string> words;  
    for (const std::string& word : SplitIntoWords(text)) {  
        if (!IsValidWord(word)) {  
            throw std::invalid_argument("Некорректное содержание в списке слов документа"s);  
        }  
        if (!IsStopWord(word)) {  
            words.push_back(word);  
        }  
    }  
    return words;  
}  
 
int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {  
    if (ratings.empty()) {  
        return 0;  
    }  
    int rating_sum = 0;  
    for (const int rating : ratings) {  
        rating_sum += rating;  
    }  
    return rating_sum / static_cast<int>(ratings.size());  
}  
 
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {  
    bool is_minus = false;  
    // Word shouldn't be empty  
    if (text[0] == '-') {  
        is_minus = true;  
        text = text.substr(1);  
    }  
    return {text, is_minus, IsStopWord(text)};  
}  
 
SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {  
    SearchServer::Query query;  
    for (const std::string& word : SplitIntoWords(text)) {  
        const QueryWord query_word = ParseQueryWord(word);  
        if (!query_word.is_stop) {  
            if (query_word.is_minus) {  
                query.minus_words.insert(query_word.data);  
            } else {  
                query.plus_words.insert(query_word.data);  
            }  
        }  
    }  
    return query;  
}  
 
 
 
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {  
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());  
}  