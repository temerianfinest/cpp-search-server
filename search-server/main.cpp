#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
 
using namespace std;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;
 
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
 
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
 
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
 
    return words;
}
 
struct Document {
    Document() = default;
 
    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }
 
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};
 
template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
 
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
 
class SearchServer {
public:
template <typename StringContainer>
explicit SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    for (const string& stop_word : stop_words_) {
        if (!IsValidWord(stop_word)) {
            throw invalid_argument("Invalid stop word: contains invalid characters");
        }
    }
}

explicit SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
}
 
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
 
 void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    if (document_id < 0) {
        throw invalid_argument("Invalid document_id: negative value");
    }

    if (documents_.count(document_id) > 0) {
        throw invalid_argument("Invalid document_id: document with the same ID already exists");
    }

    if (!IsValidDocumentText(document)) {
        throw invalid_argument("Invalid document text: contains invalid characters");
    }

    const auto words = SplitIntoWordsNoStop(document);
    if (!IsValidWords(words)) {
        throw invalid_argument("Invalid document text: contains words with invalid characters");
    }

    const double inv_word_count = 1.0 / words.value().size();
    for (const string& word : *words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
     documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
     document_ids_.push_back(document_id);
}

 
template <typename DocumentPredicate>
vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
    // Проверка на наличие недопустимых знаков в поисковом запросе
    if (!IsValidWords(SplitIntoWords(raw_query))) {
        throw invalid_argument("Invalid query");
    }
    
    const auto query = ParseQuery(raw_query);
    if (!query) {
        throw invalid_argument("Invalid query");
    }
    
    auto result = FindAllDocuments(*query, document_predicate);
    sort(result.begin(), result.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    
    return result;
}

vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
 
    int GetDocumentCount() const {
        return documents_.size();
    }
 
tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    if (!query) {
        throw invalid_argument("Invalid query");
    }

    // Проверка на наличие недопустимых знаков в словах запроса
    for (const auto& word : query->plus_words) {
        if (word.find_first_of("\n\t\v\f\r ") != string::npos) {
            throw invalid_argument("Invalid character in query");
        }
    }
    for (const auto& word : query->minus_words) {
        if (word.find_first_of("\n\t\v\f\r ") != string::npos) {
            throw invalid_argument("Invalid character in query");
        }
    }

    // Проверка на несколько минусов перед словом
    unordered_map<string, bool> minus_words;
    for (const auto& word : query->minus_words) {
        if (word.empty()) {
            throw invalid_argument("Empty minus word in query");
        }
        if (word[0] == '-') {
            if (minus_words[word.substr(1)]) {
                throw invalid_argument("Multiple minus signs before word in query");
            }
            minus_words[word.substr(1)] = true;
        }
    }

    vector<string> matched_words;
    bool has_minus = false;

    for (const string& word : query->plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    for (const string& word : query->minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            if (minus_words[word.substr(1)]) {
                // Если найдено совпадение по минус-слову, то возвращаем пустой вектор слов и статус документа
                return {vector<string>(), documents_.at(document_id).status};
            }
            matched_words.clear();
            has_minus = true;
            break;
        }
    }

    if (matched_words.empty() && has_minus) {
        // Если нет совпадений по плюс-словам и есть минус-слова, то возвращаем пустой вектор слов и статус документа
        return {vector<string>(), documents_.at(document_id).status};
    }

    tuple<vector<string>, DocumentStatus> result = {matched_words, documents_.at(document_id).status};
    return result;
}

 
int GetDocumentId(int index) const {
    return document_ids_.at(index);
}
 
private:
    vector<int> document_ids_;
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
 
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
 
    optional<vector<string>> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
 
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }
 
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
 
    optional<QueryWord> ParseQueryWord(string text) const {
        QueryWord word;
        bool is_minus = false;
        
         if (text[0] == '-') {
 
            is_minus = true;
            text = text.substr(1);
        }
        
        if (text.size() == 0 || text[0] == '-' || !IsValidWord(text)) {
                return nullopt;
            }
 

        word = {text, is_minus, IsStopWord(text)};
 
        return word;
    }
 
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
 
    optional<Query> ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word) {
                return nullopt;
            }
            if (!query_word->is_stop) {
                if (query_word->is_minus) {
                    query.minus_words.insert(query_word->data);
                } else {
                    query.plus_words.insert(query_word->data);
                }
            }
        }
        return query;
    }
 
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
 
template <typename DocumentPredicate>
vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    map<int, double> document_to_relevance;
    for (const string& word : query.plus_words) {
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
    
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    
    if (query.minus_words.size() > 1) {
        throw invalid_argument("Invalid query: More than one minus sign in a row");
    }
    
    if (document_to_relevance.empty()) {
        return {};  // No documents match the query
    }
    
    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    
    return matched_documents;
}
 
    static bool IsValidWord(const string& word) {
        // Слова не должны содержать лишних символов
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
 
    static bool IsValidWords(optional<vector<string>> words) {
        for (const string& word : *words) {
            if (!IsValidWord(word)) {
                return false;
            }
        }
        return true;
    }
    
    bool IsValidDocumentText(const string& text) {
    for (char c : text) {
        if (c >= 0 && c <= 31) {
            return false;
        }
    }
    return true;
}

    
};
 
// ==================== для примера =========================
 
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
auto top_documents = search_server.FindTopDocuments("пушистый ухоженный кот"s);
if (!top_documents.empty()) {
    for (const Document& document : top_documents) {
        PrintDocument(document);
    }
}
    cout << "BANNED:"s << endl;
    top_documents = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    if (!top_documents.empty()) {
        for (const Document& document : top_documents) {
            PrintDocument(document);
        }
    }
    cout << "Even ids:"s << endl;
    top_documents = search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    if (!top_documents.empty()) {
        for (const Document& document : top_documents) {
            PrintDocument(document);
        }
    }

    return 0;
}
