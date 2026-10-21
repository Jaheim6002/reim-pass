#pragma once
#include <string>
#include <ncurses.h>
#include <vector>
#include <sqlite3.h>
#include <sodium.h>
#include <cstring>
#include <stdexcept>
#include <optional>

class Common {
    protected:

        // Terminal UI

        int printMenu (std::vector<std::string> menu, int& selected) {
            int n = menu.size();

            while (true) {
                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");

                for (int i = 0; i < n; i++) {
                    if (i == selected) {
                        attron(A_REVERSE);
                    }

                    mvprintw(3 + i, 4, "%s", menu[i].c_str());

                    if (i == selected) {
                        attroff(A_REVERSE);
                    }
                }

                refresh();

                int key = getch();

                if (key == KEY_UP) {
                    selected = (selected - 1 + n) % n;
                } else if (key == KEY_DOWN) {
                    selected = (selected + 1) % n;
                } else if (key == '\n' || key == KEY_ENTER) {
                    break;
                }
            }
            return selected;
        }
        std::string input (int row, const char* label, bool doEcho = true) {
            char buffer[256];

            mvprintw(row, 4, "%s: ", label);
            refresh();
            curs_set(1);

            if (doEcho) {
                echo();
            }

            getnstr(buffer, sizeof(buffer) - 1);

            noecho();
            curs_set(0);

            return std::string(buffer);
        }

        std::string inputRequired (int row, const char* label, bool doEcho = true) {
            while (true) {
                move(row, 0);
                clrtoeol();
                std::string value = input(row, label, doEcho);

                if (!value.empty()) {
                    move(7, 0);
                    clrtoeol();
                    return value;
                }

                mvprintw(7, 4, "This field cannot be empty.");
            }
        }

        //Encryption

        std::string encryptPassword(const std::string& plain, const std::string& pass) {
            unsigned char salt[crypto_pwhash_SALTBYTES];
            unsigned char nonce[crypto_secretbox_NONCEBYTES];
            unsigned char key[crypto_secretbox_KEYBYTES];

            randombytes_buf(salt, sizeof salt);
            randombytes_buf(nonce, sizeof nonce);

            if (crypto_pwhash(key, sizeof key, pass.c_str(), pass.size(), salt,
                            crypto_pwhash_OPSLIMIT_INTERACTIVE,
                            crypto_pwhash_MEMLIMIT_INTERACTIVE,
                            crypto_pwhash_ALG_DEFAULT
                            ) != 0) {
                throw std::runtime_error("Not enough memory");
            }

            // Stored layout: salt | nonce | ciphertext + MAC
            std::string out(sizeof salt + sizeof nonce + plain.size() + crypto_secretbox_MACBYTES, '\0');
            unsigned char* p = reinterpret_cast<unsigned char*>(out.data());

            memcpy(p, salt, sizeof salt);
            memcpy(p + sizeof salt, nonce, sizeof nonce);
            crypto_secretbox_easy(p + sizeof salt + sizeof nonce,
                                reinterpret_cast<const unsigned char*>(plain.data()),
                                plain.size(), nonce, key
                                );

            sodium_memzero(key, sizeof key);
            return out;
        }

        bool decryptPassword(const std::string& enc, const std::string& pass, std::string& plain) {
            const size_t header = crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES;
            if (enc.size() < header + crypto_secretbox_MACBYTES) { 
                return false; 
            }

            const unsigned char* p = reinterpret_cast<const unsigned char*>(enc.data());
            unsigned char key[crypto_secretbox_KEYBYTES];

            if (crypto_pwhash(key, sizeof key, pass.c_str(), pass.size(), p,
                            crypto_pwhash_OPSLIMIT_INTERACTIVE,
                            crypto_pwhash_MEMLIMIT_INTERACTIVE,
                            crypto_pwhash_ALG_DEFAULT) != 0) {
                return false;
            }

            size_t length = enc.size() - header - crypto_secretbox_MACBYTES;
            plain.resize(length);

            int r = crypto_secretbox_open_easy(
                reinterpret_cast<unsigned char*>(plain.data()),
                p + header, enc.size() - header,
                p + crypto_pwhash_SALTBYTES, key);

            sodium_memzero(key, sizeof key);
            return r == 0;
        }

        // Row that holds the encrypted passphrase verifier (first row of a fresh database)
        static constexpr int kVerifierId = 1;

        // SQL Queries

        bool insertSQL (sqlite3* data, const std::string& site, const std::string& login, const std::string& password) {
            const char* insertQuery =
                "INSERT INTO login_data (site, login, password) "
                "VALUES (?, ?, ?);";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(data, insertQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
                getch();
                return false;
            }

            sqlite3_bind_text(stmt, 1, site.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, login.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_blob(stmt, 3, password.data(), (int)password.size(), SQLITE_TRANSIENT);

            bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
            if (!ok) {
                mvprintw(7, 4, "Insert Error: %s", sqlite3_errmsg(data));
                getch();
            }

            sqlite3_finalize(stmt);
            return ok;
        }

        bool deleteSQL (sqlite3* data, int id) {
            const char* deleteQuery =
                "DELETE FROM login_data "
                "WHERE id = ?;";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(data, deleteQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
                getch();
                return false;
            }

            sqlite3_bind_int(stmt, 1, id);

            bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
            if (!ok) {
                mvprintw(7, 4, "Delete Error: %s", sqlite3_errmsg(data));
                getch();
            }

            sqlite3_finalize(stmt);
            return ok;
        }

        // Returns the still-encrypted blob, or nullopt if the row does not exist
        std::optional<std::string> selectPassword (sqlite3* data, int id) {
            const char* selectPasswordQuery =
                "SELECT password FROM login_data "
                "WHERE id = ?;";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(data, selectPasswordQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
                getch();
                return std::nullopt;
            }

            sqlite3_bind_int(stmt, 1, id);

            std::optional<std::string> result;

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const void* blob = sqlite3_column_blob(stmt, 0);
                int len = sqlite3_column_bytes(stmt, 0);

                if (blob && len > 0) {
                    result = std::string(static_cast<const char*>(blob), len);
                }
            }

            sqlite3_finalize(stmt);
            return result;
        }
};