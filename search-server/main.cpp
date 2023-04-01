ostream& operator<<(ostream& out, DocumentStatus status) {
  switch (status) {
      case DocumentStatus::ACTUAL: return out << "ACTUAL"s;
      case DocumentStatus::IRRELEVANT: return out << "IRRELEVANT"s;
      case DocumentStatus::BANNED: return out << "BANNED"s;
      case DocumentStatus::REMOVED: return out << "REMOVED"s;
  default: return out << (int) status;
    }
}

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
 
    {
        SearchServer server;        
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
 
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
 
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}
 
// Тест проверяет, что поисковая система исключает минус-слова из поискового запроса
void TestExcludeMinusWordsFromQuery() {
    const int doc_id = 42;    
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    
        // Убедимся, что поиск не существующего документа вернет пустоту 
    {
        SearchServer server;        
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
    
    // Убедимся, что слова, не являющиеся минус-словами, находят нужный документ
    {        
        SearchServer server;        
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    
    // Убедимся, что поиск этого-же слова, входящего в список минус_слов возвращает пустоту
    {        
        SearchServer server;        
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);       
        ASSERT(server.FindTopDocuments("-in the"s).empty());
    }
}
 
// Тест на проверку сопоставления содержимого документа и поискового запроса
 
void TestMatchDocument() {
 
    const int doc_id = 42;    
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
 
    SearchServer server;        
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
 
    // убедимся, что слова из поискового запроса вернулись
    {
        const auto [matched_words, document_status] = server.MatchDocument("in the cat"s, doc_id);
        ASSERT_EQUAL(matched_words.size(), 3);
        vector<string> words_assert = {"cat"s, "in"s, "the"s};
        ASSERT_EQUAL(words_assert, matched_words);
        ASSERT_EQUAL(document_status, DocumentStatus::ACTUAL);
    }
 
    // убедимся, что присутствие минус-слова возвращает пустой вектор
    {
        const auto [matched_words, document_status] = server.MatchDocument("in -the cat"s, doc_id);      
        ASSERT(matched_words.empty());
    }
    
    // убедимся, что стоп-слова исключаются из возвращаемого вектора
    {
        server.SetStopWords("in the"s);
        const auto [matched_words, document_status] = server.MatchDocument("in the cat"s, doc_id);
        ASSERT_EQUAL(matched_words.size(), 1);
        vector<string> words_assert = {"cat"s};
        ASSERT_EQUAL(words_assert, matched_words);
        ASSERT_EQUAL(document_status, DocumentStatus::ACTUAL);
    }
}
 
// Тест на вычисление релеватности найденных документов
void TestRelevanceCalculation() {
    const double allowed_error = 1e-6; //+-0.000001
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "белый кот и модный ошейник"s;
    const vector<int> ratings_1 = {1, 1, 1}; //
    const string content_2 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_2 = {1, 1, 1}; //
    const string content_3 = "ухоженный кот"s;
    const vector<int> ratings_3 = {1, 1, 1}; //
 
    SearchServer server;        
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);
 
    // убедимся, что документы найдены
    const string query = "кот"s;
    const auto found_docs = server.FindTopDocuments(query);
    ASSERT_EQUAL(found_docs.size(), 3);
 
// проверим правильность вычисленных релевантностей
    {
//...(log(кол-во документов / кол-во документов, с которыми поиск пересекается) * (кол-во совпадений слов документа и слов поиска / кол-во слов документа))...
//                                       ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
    ASSERT(abs(found_docs[0].relevance - log(server.GetDocumentCount() * 1.0 / 3) * (1.0 / 2)) < allowed_error); 
    ASSERT(abs(found_docs[1].relevance - log(server.GetDocumentCount() * 1.0 / 3) * (1.0 / 4)) < allowed_error); 
    ASSERT(abs(found_docs[2].relevance - log(server.GetDocumentCount() * 1.0 / 3) * (1.0 / 5)) < allowed_error);  
    }
}

// тест на сортировку по релевантности 
void TestSortByRelevance() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "белый кот и модный ошейник"s;
    const vector<int> ratings_1 = {1, 1, 1}; //
    const string content_2 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_2 = {1, 1, 1}; //
    const string content_3 = "ухоженный кот"s;
    const vector<int> ratings_3 = {1, 1, 1}; //
 
    SearchServer server;        
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);
 
    // убедимся, что документы найдены
    {
    const string query = "кот"s;
    const auto found_docs = server.FindTopDocuments(query);
    ASSERT_EQUAL(found_docs.size(), 3);
    
    // проверим сортировку по релевантности
    ASSERT(found_docs[0].relevance >= found_docs[1].relevance);
    ASSERT(found_docs[1].relevance >= found_docs[2].relevance);
    }
}

void TestRatingCalculation() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "белый кот модный ошейник"s;
    const vector<int> ratings_1 = {-1, 2, 2}; 
    const string content_2 = "белый кот модный ошейник"s;
    const vector<int> ratings_2 = {1, 2, 3}; 
    const string content_3 = "белый кот модный ошейник"s;
    const vector<int> ratings_3 = {2, 3, 4}; 
 
    SearchServer server;        
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);
 
    // убедимся, что документы найдены
    const string query = "кот"s;
    const auto found_docs = server.FindTopDocuments(query);
    ASSERT_EQUAL(found_docs.size(), 3);
    
    // проверим правильность вычисленных рейтингов
    // (сумма рейтингов / их количество) 
    //                         ↓↓↓↓↓↓↓↓↓↓↓↓↓
    ASSERT_EQUAL(found_docs[0].rating, (2 + 3 + 4) / 3);
    ASSERT_EQUAL(found_docs[1].rating, (1 + 2 + 3) / 3);
    ASSERT_EQUAL(found_docs[2].rating, (-1 + 2 + 2) / 3);
}
    
    void TestSortByRating() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "белый кот модный ошейник"s;
    const vector<int> ratings_1 = {-1, 2, 2}; 
    const string content_2 = "белый кот модный ошейник"s;
    const vector<int> ratings_2 = {1, 2, 3}; 
    const string content_3 = "белый кот модный ошейник"s;
    const vector<int> ratings_3 = {2, 3, 4}; 
 
    SearchServer server;        
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);
 
    // убедимся, что документы найдены
    const string query = "кот"s;
    const auto found_docs = server.FindTopDocuments(query);
    ASSERT_EQUAL(found_docs.size(), 3);
        
    // проверим сортировку по рейтингу
        {
        ASSERT(found_docs[0].relevance >= found_docs[1].relevance);
        ASSERT(found_docs[1].relevance >= found_docs[2].relevance);
        }
    }
 
// Тест на фильтрацию результата с использованием предиката
void TestFilterByPredicate() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = {-1, 2, 2}; // rating 1 relev = 0.304098831081123
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = {1, 2, 3}; // rating 2 relev = 0.0810930216216328
    const string content_3 = "silent assasin village cat in the village of darkest realms"s;
    const vector<int> ratings_3 = {2, 3, 4}; // rating 3 relev = 0.202732554054082
 
    SearchServer server;        
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
 
    // Найдём документы с рейтингом
    {
        const auto found_docs = server.FindTopDocuments("cat in the loan village"s, [](int document_id, DocumentStatus, int rating) { 
            return rating == 3; 
        });
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id3);
    }
}
 
void TestSearchByStatusDocuments() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const int doc_id4 = 4;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = {-1, 2, 2};
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = {1, 2, 3};
    const string content_3 = "silent assasin village cat in the village of darkest realms"s;
    const vector<int> ratings_3 = {2, 3, 4};
    const string content_4 = "cat of the loan village"s;
    const vector<int> ratings_4 = {6, 4, 2};
 
    SearchServer server;        
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::IRRELEVANT, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
    server.AddDocument(doc_id4, content_4, DocumentStatus::REMOVED, ratings_4);
 
    // вернем документы со статусом ACTUAL
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::ACTUAL); 
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id1);
    }
 
    // вернем документы со статусом IRRELEVANT
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::IRRELEVANT);  
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id2);
    }
 
    // вернем документы со статусом BANNED
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id3);
    }
 
    // вернем документы со статусом REMOVED
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id4);
    }
 
    // проверим, что поиск документа по несоответсвующему статусу вернёт пустое значение
    {
        auto found_docs = server.FindTopDocuments("dog"s, DocumentStatus::IRRELEVANT); 
        ASSERT_EQUAL(found_docs.size(), 1);
        found_docs = server.FindTopDocuments("dog"s, DocumentStatus::ACTUAL);
        ASSERT(found_docs.empty());
    }    
}
 
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);    
    RUN_TEST(TestExcludeMinusWordsFromQuery);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevanceCalculation);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestRatingCalculation);
    RUN_TEST(TestSortByRating);
    RUN_TEST(TestFilterByPredicate);
    RUN_TEST(TestSearchByStatusDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
