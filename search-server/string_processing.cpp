#include "string_processing.h"


std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    size_t bpos=0;
    const size_t endpos=text.npos;
    while(true){
        size_t space_pos=text.find(' ', bpos);
        words.push_back(space_pos==endpos ? text.substr(bpos) : text.substr(bpos,space_pos-bpos));
        if(space_pos!=endpos){
        bpos=space_pos+1;
        }else{break;}
    }
    return words;
}





/*
std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    size_t bpos=0;
    const size_t endpos=text.npos;
    while(true){
        size_t space_pos=text.find(' ', bpos);
        words.push_back(space_pos==endpos ? text.substr(bpos) : text.substr(bpos,space_pos-bpos));
        if(bpos!=endpos){
        bpos=space_pos+1;
        }else{break;}
    }
    return words;
}


std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
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
*/
