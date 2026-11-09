#pragma once
#include "common.hpp"

class Config : Common {
    public: 
        static fs::path homeDir();
        static fs::path configDir();
        static bool createDefaultConfig(const fs::path &configPath, const fs::path &defaultDir);
        bool loadConfig(fs::path *directory, bool *sync, std::string *e2ePass, bool *copyPassword, int *copySeconds, bool *showPassword, int *genPassLenght);
};