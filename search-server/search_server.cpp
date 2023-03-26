#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text): SearchServer(SplitIntoWords(stop_words_text)){}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {

        if (document_id < 0 || documents_.count(document_id) != 0) {
           throw std::invalid_argument("ne verniy id");
        }

        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();

        for (const std::string& word : words) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("est' nevalidniy simvol");
            }
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

        index_id_.push_back(document_id);
    }

 std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status/* = DocumentStatus::ACTUAL*/) const {

        auto lambda = [status]([[maybe_unused]] int document_id, DocumentStatus stat, [[maybe_unused]] int rating) {return stat == status; };
        return FindTopDocuments(raw_query, lambda);
    }

int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

 std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
       std::vector<std::string> matched_words;
        for (const std::string& word : query.plus_words) {

            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string& word : query.minus_words) {

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

 int SearchServer::GetDocumentId(int num) {
        if (num<0 || num>documents_.size()) {
            throw std::out_of_range("index vne diapazona");
        }
        return index_id_.at(num);
    }

 bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

 bool SearchServer::IsValidWord(const std::string& word) {
        return none_of(word.begin(), word.end(), [](char c) {return c >= '\0' && c < ' ';});
    }

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
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
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

        return rating_sum / static_cast<int>(ratings.size());
    }

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
        if (!IsValidWord(text)) {
            throw std::invalid_argument("est' nevalidniy simvol");
        }

        bool is_minus = false;

        if (text[0] == '-') {
            
            is_minus = true;
            text = text.substr(1);
            if (text.empty() || text[0] == ' ' || text[0] == '-') {
                throw std::invalid_argument("ne verni minus slova");
            }

        }
        return { text, is_minus, IsStopWord(text) };
    }

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
        SearchServer::Query query;
        for (const std::string& word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
/*
#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <stdexcept>

#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"

using namespace std;

constexpr double PRECISION = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;


class SearchServer {

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {

        if (any_of(stop_words_.begin(), stop_words_.end(), [](string word) {return !IsValidWord(word);})) {
           throw invalid_argument("est' nevalidniy simvol"s);
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))
    {

    }


    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

        if (document_id < 0 || documents_.count(document_id) != 0) {
           throw invalid_argument("ne verniy id"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();

        for (const string& word : words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("est' nevalidniy simvol"s);
            }
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

        index_id_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentPredicate& document_predicate) const {
        const Query query = ParseQuery(raw_query);

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

            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {

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

    vector<int> index_id_;
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
        if (!IsValidWord(text)) {
            throw invalid_argument("est' nevalidniy simvol"s);
        }

        bool is_minus = false;

        if (text[0] == '-') {
            
            is_minus = true;
            text = text.substr(1);
            if (text.empty() || text[0] == ' ' || text[0] == '-') {
                throw invalid_argument("ne verni minus slova"s);
            }

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

*/