#pragma once
#include <string>
#include <vector>
#include <deque>
#include "search_server.h"
#include "document.h"



class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
   
    template <typename DocumentPredicate>
   std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        bool zero;
    };
    std::deque<QueryResult> requests_;
    int zero_requests_=0;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_; 
    
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto docs = search_server_.FindTopDocuments(raw_query,document_predicate);
    
        if(requests_.size()>=min_in_day_){
            if(requests_.front().zero){
            requests_.pop_front();
            --zero_requests_; 
            }else{
            requests_.pop_front();
            }
           
        }
        
        if(docs.empty()){
            ++zero_requests_;
            requests_.push_back({true}); 
        }else{
            requests_.push_back({false});
        }
        return docs;
    }