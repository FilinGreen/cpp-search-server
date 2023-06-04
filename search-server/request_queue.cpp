#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server):search_server_(search_server){}

std::vector<Document> RequestQueue::AddFindRequest(const std::string_view raw_query, DocumentStatus status) {
     auto docs =  search_server_.FindTopDocuments(raw_query,status);
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



std::vector<Document> RequestQueue::AddFindRequest(const std::string_view raw_query) {
     auto docs = search_server_.FindTopDocuments(raw_query);
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
    int RequestQueue::GetNoResultRequests() const {
        return zero_requests_;
    }

