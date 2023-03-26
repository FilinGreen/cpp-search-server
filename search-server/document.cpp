#include "document.h"

Document::Document() :id(0), relevance(0), rating(0) {}
Document::Document(int inid, double inrelevance, int inrating) :id(inid), relevance(inrelevance), rating(inrating) {}

