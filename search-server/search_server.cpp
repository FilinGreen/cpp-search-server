#include "search_server.h"
#include "log_duration.h"



SearchServer::SearchServer(const std::string& stop_words_text): SearchServer
(std::string_view(stop_words_text)) {}

SearchServer::SearchServer(std::string_view stop_words_text): SearchServer(SplitIntoWords(stop_words_text)){}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {

        if (document_id < 0 || documents_.count(document_id) != 0) {
           throw std::invalid_argument("ne verniy id");
        }
    
       storage_.emplace_front(document);

        std::vector<std::string_view> words=SplitIntoWordsNoStop(storage_.front());
        const double inv_word_count = 1.0 / words.size();

        for (const auto& word : words) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("est' nevalidniy simvol");
            }
            word_to_document_freqs_[word][document_id] += inv_word_count;
            id_to_document_freqs_[document_id][word]+= inv_word_count;
        }

        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
      
        index_id_.insert(document_id);
    }

 std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {

        auto lambda = [status]([[maybe_unused]] int document_id, DocumentStatus stat, [[maybe_unused]] int rating) {return stat == status; };
        return FindTopDocuments(raw_query, lambda);
    }

int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

 matching_result SearchServer::MatchDocument(const std::string_view raw_query,int document_id) const {
     
        
      std::vector<std::string_view> matched_words;
      if(!documents_.count(document_id)){ return { matched_words, documents_.at(document_id).status }; }
      const Query query = ParseQuery(raw_query);  
     
        if(std::any_of(query.minus_words.begin(),query.minus_words.end(),[this,document_id](const auto word){
            return (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id));})){ 
            return { matched_words, documents_.at(document_id).status };
                            }
     
        for (const auto word : query.plus_words) {

            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }

        matched_words.erase(std::unique(matched_words.begin(),matched_words.end()),matched_words.end());
        return { matched_words, documents_.at(document_id).status };
    }



matching_result SearchServer::MatchDocument(std::execution::sequenced_policy,const std::string_view raw_query,int document_id) const{
    return MatchDocument(raw_query,document_id);
}


   matching_result SearchServer::MatchDocument(std::execution::parallel_policy,const std::string_view raw_query,int document_id) const{
        
        
        
 std::vector<std::string_view> matched_words;     
 if(!documents_.count(document_id)){ return { matched_words, documents_.at(document_id).status }; }
 const Query query = ParseQuery(std::execution::par,raw_query);   
        
 if(std::any_of(std::execution::par,query.minus_words.begin(),query.minus_words.end(),[this,document_id](const auto word){
            return (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id));})){
           
            return { matched_words, documents_.at(document_id).status };
                            }
   
     matched_words.resize(query.plus_words.size());
        
     std::copy_if(std::execution::par,query.plus_words.begin(),query.plus_words.end(),matched_words.begin(),[this,document_id](const auto& word){
return(word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id));});  
     
 std::sort(std::execution::par,matched_words.begin(),matched_words.end());    
 matched_words.erase(std::unique(std::execution::par,matched_words.begin(),matched_words.end())-1,matched_words.end()); 
      
        return { matched_words, documents_.at(document_id).status };  
    }


 std::set<int>::const_iterator SearchServer::begin() {
     return index_id_.begin();

 }

 std::set<int>::const_iterator SearchServer::end() {
     return index_id_.end();
 }
 

 const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
     if(id_to_document_freqs_.count(document_id)){
     return id_to_document_freqs_.at(document_id);
     }else{
         static std::map<std::string_view, double> null_to_return_;
         return null_to_return_;
     }
 }

 void SearchServer::RemoveDocument(int document_id) {
     assert(documents_.count(document_id)!=0);
     
     documents_.erase(document_id);
     id_to_document_freqs_.erase(document_id);
     index_id_.erase(document_id);
     
     for(auto& [word,id_freq]:word_to_document_freqs_ ){
         id_freq.erase(document_id);
         if (word.empty()) {
             word_to_document_freqs_.erase(word);
         }
     }
     
 }

void SearchServer::RemoveDocument(std::execution::sequenced_policy,int document_id) {
   RemoveDocument(document_id);
 }

void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {
    assert(documents_.count(document_id) != 0);

    const auto& word_freq = id_to_document_freqs_.at(document_id);
    std::vector< std::string_view> to_erase(word_freq.size()); 
    std::transform(std::execution::par, word_freq.begin(), word_freq.end(), to_erase.begin(), [](auto& item) {
        return item.first;
        });

    std::for_each(std::execution::par, to_erase.begin(), to_erase.end(), [document_id, this](auto word) {
     word_to_document_freqs_.at(word).erase(document_id);
     });
    
    
    documents_.erase(document_id);
    id_to_document_freqs_.erase(document_id);
    index_id_.erase(document_id);

}

 bool SearchServer::IsStopWord(const std::string_view word) const {
        return std::count(stop_words_.begin(),stop_words_.end(),word);
       
    }

 bool SearchServer::IsValidWord(const std::string_view word) {
        return std::none_of(word.begin(), word.end(), [](char c) {return c >= '\0' && c < ' ';});
    }

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
        std::vector<std::string_view> words;
        for (const std::string_view word : SplitIntoWords(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
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


SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
        SearchServer::Query query;
        for (const std::string_view word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.push_back(query_word.data);
                }
                else {
                    query.plus_words.push_back(query_word.data);
                }
            }
        }
    std::sort(query.plus_words.begin(),query.plus_words.end());
    std::sort(query.minus_words.begin(),query.minus_words.end());
    query.plus_words.erase(std::unique(query.plus_words.begin(),query.plus_words.end()),query.plus_words.end());
    query.minus_words.erase(std::unique(query.minus_words.begin(),query.minus_words.end()),query.minus_words.end());
    
        return query;
    }

SearchServer::Query SearchServer::ParseQuery(std::execution::sequenced_policy,const std::string_view text) const{
    return ParseQuery(text);
}

SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy,const std::string_view text) const {

    SearchServer::Query query;
        for (const auto word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.push_back(query_word.data);
                }
                else {
                    query.plus_words.push_back(query_word.data);
                }
            }
        }
       
        return query;
    }

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }







