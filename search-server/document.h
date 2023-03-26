#pragma once
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