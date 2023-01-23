#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(const int& document_id, const string& document) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words) {
            word_idfreq_[word][document_id] += 1.0 / words.size();
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query plusminus_ = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(plusminus_);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct QueryWord {
        string data;
        bool minus;
        bool stop;
    };
    struct Query {
        set<string> plus;
        set<string> minus;
    };
    map<string, set<int>> documents_;
    set<string> stop_words_;
    map<string, map<int, double>> word_idfreq_;
    int document_count_ = 0;


    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    //-------------------------------------------------------------------------------------------------------


    QueryWord ParseQueryWord(string text) const {
        bool minus = false;
        if (text[0] == '-') {
            minus = true;
            text = text.substr(1);
        }
        return { text, minus, IsStopWord(text) };
    }



    Query ParseQuery(const string& query) const {
        Query result;

        for (const string& word : SplitIntoWords(query)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.stop) {
                if (query_word.minus) {
                    result.minus.insert(query_word.data);
                }
                else {
                    result.plus.insert(query_word.data);
                }
            }
        }
        return result;
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

    vector<Document>  FindAllDocuments(const Query plusminus_) const {
        vector<Document> matched_documents;
        map<int, double> id_relev;
        for (const auto& qword : plusminus_.plus) {
            if (word_idfreq_.count(qword) != 0) {
                double idf = log(document_count_ * 1.0 / word_idfreq_.at(qword).size());
                for (const auto& [id, tf] : word_idfreq_.at(qword)) {
                    id_relev[id] += tf * idf;
                }
            }
        }
        for (const auto& qword : plusminus_.minus) {
            if (word_idfreq_.count(qword) != 0) {
                for (const auto [id, value] : word_idfreq_.at(qword)) {
                    id_relev.erase(id);
                }
            }
        }
        for (const auto& result : id_relev) {
            matched_documents.push_back({ result.first,result.second });
        }
        return matched_documents;
    }
};
//-----------------------------------------------------------------------------------------------------------------

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}