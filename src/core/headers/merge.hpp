#pragma once
#include "common.hpp"

class Merging : Common {
public:
    void Sync(const fs::path& directory, std::string e2ePass);
    void Merge(const fs::path& localPath, const std::string& syncData);
};