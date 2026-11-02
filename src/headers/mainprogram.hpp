#pragma once
#include "common.hpp"
#include <sodium/utils.h>

class MainProgram : Common {
    public:

        int run (sqlite3* data, const std::string& passphrase) {
            printMainMenu(data, passphrase);
            return 0;
        }
    
    private:

        void insertData (sqlite3* data, const std::string& passphrase) {
            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");

            std::string site = inputRequired(3, "Site");
            std::string login = inputRequired(4, "Login");
            std::string password = inputRequired(5, "Password", false);

            password = encryptPassword(password, passphrase);

            bool ok = insertSQL(data, site, login, password);

            if (ok) { 
                mvprintw(7, 4, "Login data added!");
            }

            mvprintw(9, 4, "Press any key to continue.");

            getch();
        }

        struct LoginRow {
            int id;
            std::string site;
            std::string login;
            std::string encPassword;
        };

        std::vector<LoginRow> fetchRows(sqlite3* db, const std::string& filter) {
            std::vector<LoginRow> rows;

            const char* selectQuery =
                "SELECT id, site, login, password FROM login_data "
                "WHERE (site LIKE ? OR login LIKE ?) "
                "AND id != ? "
                "ORDER BY site;";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                return rows;
            }

            std::string pattern = "%" + filter + "%";
            sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, kVerifierId);   // hide the verifier row

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                LoginRow r;
                r.id    = sqlite3_column_int(stmt, 0);
                r.site  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                r.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

                const void* blob = sqlite3_column_blob(stmt, 3);
                int len = sqlite3_column_bytes(stmt, 3);
                r.encPassword.assign(static_cast<const char*>(blob), len);

                rows.push_back(std::move(r));
            }
            sqlite3_finalize(stmt);
            return rows;
        }

        void selectData(sqlite3* data, const std::string& passphrase) {
            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");
            std::string filter = input(3, "Search (empty = all)");

            std::vector<LoginRow> rows = fetchRows(data, filter);

            std::vector<std::string> selectedData;
            for (const auto& r : rows) {
                selectedData.push_back(r.site + "  |  " + r.login);
            }
            selectedData.push_back("> Back");

            int sel = 0;
            printMenu(selectedData, sel);

            if (sel == (int)rows.size()) { 
                return;
            }

            std::string plain;
            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");
            mvprintw(3, 4, "Site:     %s", rows[sel].site.c_str());
            mvprintw(4, 4, "Login:    %s", rows[sel].login.c_str());

            if (decryptPassword(rows[sel].encPassword, passphrase, plain)) {
                mvprintw(5, 4, "Password: %s", plain.c_str());
            } else {
                mvprintw(5, 4, "Wrong passphrase / corrupted data");
            }

            mvprintw(7, 4, "Press any key to continue.");

            getch();

            // Wipe the plaintext only after it has been displayed
            sodium_memzero(plain.data(), plain.size());
        }

        void updateData (sqlite3* data, const std::string& passphrase) {
            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");
            std::string filter = input(3, "Search (empty = all)");

            std::vector<LoginRow> rows = fetchRows(data, filter);

            std::vector<std::string> selectedData;
            for (const auto& r : rows) {
                selectedData.push_back(r.site + "  |  " + r.login);
            }
            selectedData.push_back("> Back");

            int sel = 0;
            printMenu(selectedData, sel);

            if (sel == (int)rows.size()) { 
                return;
            }

            std::string plain;
            clear();

            std::string site = input(3, "Site (Leave empty to not change)");
            std::string login = input(4, "Login (Leave empty to not change)");
            std::string password = input(5, "Password (Leave empty to not change)", false);

            if (site == "") {
                site = rows[sel].site;
            }
            if (login == "") {
                login = rows[sel].login;
            }
            if (password == "") {
                password = decryptPassword(rows[sel].encPassword, passphrase, plain);
            }

            password = encryptPassword(password, passphrase);

            bool ok = updateSQL(data, rows[sel].id, site, login, password);

            if (ok) { 
                mvprintw(7, 4, "Login data updated!");
            }

            sodium_memzero(plain.data(), plain.size());

            mvprintw(9, 4, "Press any key to continue.");

            getch();
        }

        void deleteData (sqlite3* data, const std::string& passphrase) {
            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");
            std::string filter = input(3, "Search (empty = all)");

            std::vector<LoginRow> rows = fetchRows(data, filter);

            std::vector<std::string> selectedData;
            for (const auto& r : rows) {
                selectedData.push_back(r.site + "  |  " + r.login);
            }
            selectedData.push_back("> Back");

            int sel = 0;
            printMenu(selectedData, sel);

            if (sel == (int)rows.size()) { 
                return;
            }

            std::string plain;

            // The passphrase is verified by decrypting the entry before allowing deletion
            if (decryptPassword(rows[sel].encPassword, passphrase, plain)) {

                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");
                std::string in = input(3, "Are you sure? Login data will be permamently deleted (y/N)");

                if(in != "y" && in != "Y") {
                    sodium_memzero(plain.data(), plain.size());
                    return;
                }

                bool ok = deleteSQL(data, rows[sel].id);


                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");

                if (ok) {
                    mvprintw(3, 4, "Deleted %s login data", rows[sel].site.c_str());
                }

            } else {
                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");
                mvprintw(5, 4, "Wrong passphrase / corrupted data");
            }

            sodium_memzero(plain.data(), plain.size());

            mvprintw(7, 4, "Press any key to continue.");

            getch();
        }

        void generatePassword () {
            // Visually ambiguous characters (l, I, 1, O, 0) are left out on purpose
            const std::string lower   = "abcdefghijkmnopqrstuvwxyz";
            const std::string upper   = "ABCDEFGHJKLMNPQRSTUVWXYZ";
            const std::string digits  = "23456789";
            const std::string symbols = "!@#$%^&*-_=+?";
            const std::string all     = lower + upper + digits + symbols;

            const int length = 16;
            std::string password;

            // Guarantee at least one character from every group
            password += lower[randombytes_uniform(lower.size())];
            password += upper[randombytes_uniform(upper.size())];
            password += digits[randombytes_uniform(digits.size())];
            password += symbols[randombytes_uniform(symbols.size())];

            while ((int)password.size() < length) {
                password += all[randombytes_uniform(all.size())];
            }

            // Fisher-Yates shuffle, so the guaranteed characters are not always at the start
            for (int i = length - 1; i > 0; i--) {
                int j = randombytes_uniform(i + 1);
                std::swap(password[i], password[j]);
            }

            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");

            mvprintw(3, 4, "Password generated (copy it):");
            mvprintw(4, 4, "%s", password.c_str());
            mvprintw(6, 4, "Press any key to continue.");
            
            getch();

            sodium_memzero(password.data(), password.size());
        }

        void printBrowseMenu (sqlite3* data, const std::string& passphrase) {
            std::vector<std::string> browseMenu = {
                "> Browse login data",
                "> Update login data",
                "> Delete login data",
                "> Back"
            };
            int selected = 0;

            clear();
            printMenu(browseMenu, selected);

            switch (selected) {
                case 0:
                    selectData(data, passphrase);
                    break;
                case 1:
                    updateData(data, passphrase);
                    break;
                case 2:
                    deleteData(data, passphrase);
                    break;
                case 3:
                    return;
            }
        }

        void printMainMenu (sqlite3* data, const std::string& passphrase) {
            std::vector<std::string> mainMenu = {
                "> Add login data", 
                "> Browse login data",
                "> Generate secure password",
                "> Close"
            };

            while (true) {
                int selected = 0;

                clear();
                printMenu(mainMenu, selected);

                switch (selected) {
                    case 0:
                        insertData(data, passphrase);
                        break;
                    case 1:
                        printBrowseMenu(data, passphrase);
                        break;
                    case 2:
                        generatePassword();
                        break;
                    case 3:
                        return;
                }
            }
        }
};