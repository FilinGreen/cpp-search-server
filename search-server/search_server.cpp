#include "search_server.h"
#include "log_duration.h"



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
            id_to_document_freqs_[document_id][word]+= inv_word_count;
        }

        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

        index_id_.push_back(document_id);
    }

 std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        //LOG_DURATION_STREAM("Operation time", std::cout);
        auto lambda = [status]([[maybe_unused]] int document_id, DocumentStatus stat, [[maybe_unused]] int rating) {return stat == status; };
        return FindTopDocuments(raw_query, lambda);
    }

int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

 std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
        int document_id) const {
       // LOG_DURATION_STREAM("Operation time", std::cout);
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

 std::vector<int>::const_iterator SearchServer::begin() {
     return index_id_.begin();

 }

 std::vector<int>::const_iterator SearchServer::end() {
     return index_id_.end();
 }
 

 const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
     return id_to_document_freqs_.at(document_id);
 }

 void SearchServer::RemoveDocument(int document_id) {
     documents_.erase(document_id);
     id_to_document_freqs_.erase(document_id);
     index_id_.erase(find(index_id_.begin(), index_id_.end(), document_id));  
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



void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, std::vector<int> ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}



