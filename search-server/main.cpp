#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double PRECISION = 1e-6;

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
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {

    Document() :id(0), relevance(0), rating(0) {};

    Document(int inid, double inrelevance, int inrating) :id(inid), relevance(inrelevance), rating(inrating) {};

    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
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

class SearchServer {

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {

        for (const string& word : stop_words_) {
            if (!IsValidWord(word)) {
                throw invalid_argument("est' nevalidniy simvol"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))
    {

    }


    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

        if (document_id >= 0 && documents_.count(document_id) == 0) {
            const vector<string> words = SplitIntoWordsNoStop(document);
            const double inv_word_count = 1.0 / words.size();

            for (const string& word : words) {
                if (IsValidWord(word)) {
                    word_to_document_freqs_[word][document_id] += inv_word_count;

                }
                else {
                    throw invalid_argument("est' nevalidniy simvol"s);
                }
            }

            documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

            int n = index_id_.size();
            index_id_[n] = document_id;

        }else { throw invalid_argument("ne verniy id"s);}

    }
    
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentPredicate& document_predicate) const {
        const Query query = ParseQuery(raw_query);
        for (string word : query.plus_words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("est' nevalidniy simvol"s);
            }
        }
        for (const string& word : query.minus_words) {
            if (!IsValidWord(word) || word[0] == ' ' || word[0] == '-' || word.size() == 0) {
                throw invalid_argument("ne verni minus slova"s);
            }
        }
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < PRECISION) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }


    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {

        auto lambda = [status]([[maybe_unused]] int document_id, DocumentStatus stat, [[maybe_unused]] int rating) {return stat == status; };
        return FindTopDocuments(raw_query, lambda);
    }


    int GetDocumentCount() const {
        return documents_.size();
    }



    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("est' nevalidniy simvol"s);
            }
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (!IsValidWord(word) || word[0] == ' ' || word[0] == '-' || word.empty()) {
                throw invalid_argument("ne verni minus slova"s);
            }
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }


    int GetDocumentId(int num) {
        if (num<0 || num>documents_.size()) {
            throw out_of_range("index vne diapazona"s);
        }
        return index_id_.at(num);
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    map<int, int> index_id_;
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {

        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
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

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }


    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Lambda>
    vector<Document> FindAllDocuments(const Query& query, const Lambda& lambda) const {
        map<int, double> document_to_relevance;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {

                if (lambda(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
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
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};


template <typename T>
ostream& operator<< (ostream& is, const vector<T>& vec) {
    bool flag = false;
    is << "[";
    for (const auto& elem : vec) {
        if (flag) {
            is << ", ";
        }
        is << elem;
        flag = true;
    }
    is << "]";
    return is;
}

template <typename T>
ostream& operator<< (ostream& is, const set<T>& vec) {
    bool flag = false;
    is << "{";
    for (const auto& elem : vec) {
        if (flag) {
            is << ", ";
        }
        is << elem;
        flag = true;
    }
    is << "}";
    return is;
}

template <typename T, typename Y>
ostream& operator<< (ostream& is, const map<T, Y>& mapa) {
    bool flag = false;
    size_t num = mapa.size();
    size_t i = 1;
    is << "{"s;
    for (const auto& [key, value] : mapa) {
        if (flag) {
            is << " "s;
        }
        flag = true;
        is << key << ": "s << value;
        if (i < num) {
            is << ","s;
        }
        ++i;
    }

    is << "}";
    return is;
}
/*
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename F>
void RunTestImpl(F& function, const string& fname) {
    function();
    cerr << fname << " OK" << endl;

}

#define RUN_TEST(func)  RunTestImpl(func,#func)


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("dog"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}


void TestMinusWords() {


    const vector<int> ratings = { 1, 2, 3 };
    const string raw_query = "-cat city dog"s;

    {
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, "dog in the village"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "cat in the house"s, DocumentStatus::ACTUAL, ratings);
        vector<Document> result = server.FindTopDocuments(raw_query);
        ASSERT_EQUAL(result.size(), 2);
    }

    {
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        auto [result, status] = server.MatchDocument(raw_query, 1);
        vector<string> tresult = { "city"s,"dog"s };
        ASSERT_EQUAL(result, tresult);
    }
    {
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        auto [result, status] = server.MatchDocument(raw_query, 1);
        ASSERT(result.empty());
    }


}

void TestRelevance() {

    const vector<int> ratings = { 1, 2, 3 };
    const string raw_query = "-cat city dog"s;
    {
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, "dog in the village"s, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "cat in the house"s, DocumentStatus::ACTUAL, ratings);
        vector<Document>result = server.FindTopDocuments(raw_query);
        ASSERT(result[0].relevance > result[1].relevance);
    }
}


void TestRating() {

    const string raw_query = "-cat city dog"s;
    {
        const vector<int> ratings = { 1, 2, 3 };
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
        vector<Document>result = server.FindTopDocuments(raw_query);
        ASSERT_EQUAL(result[0].rating, 2);
    }
    {
        const vector<int> ratings = { -1, -2, -3 };
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(2, "dog in the forest"s, DocumentStatus::ACTUAL, ratings);
        vector<Document>result = server.FindTopDocuments(raw_query);
        ASSERT_EQUAL(result[0].rating, (-2));
    }
    {
        const vector<int> ratings = { 7, -2, 3 };
        SearchServer server("in the"s);
        //server.SetStopWords("in the"s);
        server.AddDocument(3, "mouse in the city"s, DocumentStatus::ACTUAL, ratings);
        vector<Document>result = server.FindTopDocuments(raw_query);
        ASSERT_EQUAL(result[0].rating, 2);
    }
}

void TestFilter() {

    const vector<int> ratings = { 1, 2, 3 };
    const string raw_query = "-cat city dog"s;
    SearchServer server("in the"s);
    //server.SetStopWords("in the"s);
    server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "dog in the village"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(3, "cat in the house"s, DocumentStatus::ACTUAL, ratings);
    {

        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(result.size(), 2);
    }
    {

        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::BANNED);
        ASSERT_EQUAL(result.size(), 0);
    }

    vector<Document>result = server.FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) {if (status == DocumentStatus::ACTUAL && rating != 0) { return document_id % 2 == 0; }return false; });
    ASSERT_EQUAL(result.size(), 1);
}

void TestStatus() {

    const vector<int> ratings = { 1, 2, 3 };
    const string raw_query = "-cat city dog"s;
    SearchServer server("in the"s);
    //server.SetStopWords("in the"s);
    server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "dog in the village"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(3, "cat in the house"s, DocumentStatus::ACTUAL, ratings);
    {

        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(result.size(), 2);
    }
    {

        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::BANNED);
        ASSERT_EQUAL(result.size(), 0);
    }
    {
        server.AddDocument(4, "dog in the forest"s, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(5, "dog and cat in the house"s, DocumentStatus::IRRELEVANT, ratings);
        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(result.size(), 1);
    }
    {
        server.AddDocument(7, "dog in the forest"s, DocumentStatus::REMOVED, ratings);
        server.AddDocument(10, "dog and cat in the house"s, DocumentStatus::REMOVED, ratings);
        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::REMOVED);
        ASSERT_EQUAL(result.size(), 1);
    }

}

void TestRelevanceСalculation() {
    const vector<int> ratings = { 1, 2, 3 };
    const string raw_query = "-cat city dog"s;
    SearchServer server("in the"s);
    //server.SetStopWords("in the"s);
    server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "dog in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "dog in the village"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(3, "cat in the house"s, DocumentStatus::ACTUAL, ratings);
    {
        vector<Document>result = server.FindTopDocuments(raw_query, DocumentStatus::ACTUAL);

        double result1 = 0.693147;
        double result2 = 0.346574;
        ASSERT(abs(result[0].relevance - result1) < PRECISION);
        ASSERT(abs(result[1].relevance - result2) < PRECISION);

    }
}



void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestRating);
    RUN_TEST(TestFilter);
    RUN_TEST(TestStatus);
    RUN_TEST(TestRelevanceСalculation);
}



int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}*/