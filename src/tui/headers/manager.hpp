#pragma once
#include "common.hpp"

class MainProgram : TuiCommon {
public:
    int run(sqlite3* data, const std::string& passphrase, bool copy, int copySec, bool show, int genPassLenght);

private:
    struct LoginRow {
        int id;
        std::string site;
        std::string login;
        std::string encPassword;
        std::string last;
    };

    void insertData(sqlite3* data, const std::string& passphrase);
    std::vector<LoginRow> fetchRows(sqlite3* db, const std::string& filter);
    std::optional<LoginRow> pickRow(sqlite3* data);
    void selectData(sqlite3* data, const std::string& passphrase, bool copy, int copySec, bool show);
    void updateData(sqlite3* data, const std::string& passphrase);
    void deleteData(sqlite3* data, const std::string& passphrase);
    void generatePassword(bool copy, int copySec, bool show, int genPassLenght);
    void printBrowseMenu(sqlite3* data, const std::string& passphrase, bool copy, int copySec, bool show);
    void printMainMenu(sqlite3* data, const std::string& passphrase, bool copy, int copySec, bool show, int genPassLenght);
    static bool inPath(const std::string& programName);
    static std::optional<std::vector<std::string>> detectClipboard();
    static bool runWithInput(const std::vector<std::string>& args, const std::string& input);
    bool copyToClipboard(const std::string& password, int seconds = 20);
};