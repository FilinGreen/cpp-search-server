#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {

    
    std::set<std::set<std::string>> doki;
    std::vector <int> dublikati;
    for (const int document_id : search_server) {

        std::set<std::string> slova;
        for (auto& [slovo, chastota] : (search_server.GetWordFrequencies(document_id))) {
            slova.insert(slovo);
        }

        if (doki.count(slova) == 0) {
            doki.insert(slova);
        }
        else {
            dublikati.push_back(document_id);
        }

    }
    for (int z : dublikati) {
        search_server.RemoveDocument(z);
        std::cout << "Found duplicate document id " << z << "\n";
    }
}

    
