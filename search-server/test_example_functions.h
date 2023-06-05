#pragma once
#include <string>

#include "search_server.h"


void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, std::vector<int> ratings);