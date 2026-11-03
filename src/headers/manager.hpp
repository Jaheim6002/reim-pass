#pragma once
#include "common.hpp"
#include <string>
#include <vector>

class MainProgram : Common {
public:
    int run(sqlite3* data, const std::string& passphrase);

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
    void selectData(sqlite3* data, const std::string& passphrase);
    void updateData(sqlite3* data, const std::string& passphrase);
    void deleteData(sqlite3* data, const std::string& passphrase);
    void generatePassword();
    void printBrowseMenu(sqlite3* data, const std::string& passphrase);
    void printMainMenu(sqlite3* data, const std::string& passphrase);
};