#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <stdexcept>
#include <tuple>
#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"

constexpr double PRECISION = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;


class SearchServer {

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, const DocumentPredicate& document_predicate) const;


    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    int GetDocumentCount() const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,int document_id) const;
    int GetDocumentId(int num) ;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::vector<int> index_id_;
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;

    bool IsStopWord(const std::string& word) const ;
    static bool IsValidWord(const std::string& word) ;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const ;
    static int ComputeAverageRating(const std::vector<int>& ratings) ;

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const ;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const ;
    double ComputeWordInverseDocumentFreq(const std::string& word) const ;

    template <typename Lambda>
    std::vector<Document> FindAllDocuments(const Query& query, const Lambda& lambda) const ;
};

  template <typename StringContainer>
    SearchServer::SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {

        if (any_of(stop_words_.begin(), stop_words_.end(), [](std::string word) {return !IsValidWord(word);})) {
           throw std::invalid_argument("est' nevalidniy simvol");
        }
    }
    
 template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, const DocumentPredicate& document_predicate) const {
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < PRECISION) {
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

  template <typename Lambda>
    std::vector<Document> SearchServer::FindAllDocuments(const Query& query, const Lambda& lambda) const {
        std::map<int, double> document_to_relevance;

        for (const std::string& word : query.plus_words) {
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
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }