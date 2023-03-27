#pragma once
#include <iostream>
struct Document {

    Document();
    Document(int inid, double inrelevance, int inrating);
    
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

std::ostream& operator<<(std::ostream& out, Document doc);