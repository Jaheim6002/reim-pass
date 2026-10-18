#include <iostream>
#include <ncurses.h>
#include <vector>
#include <sqlite3.h>
#include <sodium.h>
#include <string>
#include <cstring>
#include <stdexcept>

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

std::string getPassphrase () {
    std::string passphrase;
    std::string passphrase2;

    clear();
    mvprintw(1, 2, "--=/ REIM-PASS \\=--");

    while (true) {
        clear();
        passphrase = inputRequired(3, "Enter Passphrase", false);
        passphrase2 = inputRequired(4, "Enter once again", false);

        if (passphrase == passphrase2) {
            break;
        } else {
            mvprintw(2, 4, "Passphrases are not the same, try once again.");
            getch();
        }
    }

    return passphrase;
}

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

void insertData (sqlite3* data, const std::string& passphrase) {
    clear();
    mvprintw(1, 2, "--=/ REIM-PASS \\=--");

    std::string site = inputRequired(3, "Site");
    std::string login = inputRequired(4, "Login");
    std::string password = inputRequired(5, "Password", false);

    password = encryptPassword(password, passphrase);

    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(
        data, 
        "INSERT INTO login_data (site, login, password) VALUES (?, ?, ?)", 
        -1, 
        &stmt, 
        nullptr
    );

    if (result != SQLITE_OK) {
        mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
        getch();
        return;
    }

    sqlite3_bind_text(stmt, 1, site.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, login.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, password.data(), (int)password.size(), SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (ok) { 
        mvprintw(7, 4, "Login data added!");
    } else { 
        mvprintw(7, 4, "Insert Error: %s", sqlite3_errmsg(data));
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
    sqlite3_stmt* stmt;

    const char* sql =
        "SELECT id, site, login, password FROM login_data "
        "WHERE site LIKE ? OR login LIKE ? ORDER BY site";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return rows;

    std::string pattern = "%" + filter + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);

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

        sqlite3_stmt* stmt;
        int result = sqlite3_prepare_v2(
            data, 
            "DELETE FROM login_data WHERE id = ?", 
            -1, 
            &stmt, 
            nullptr
        );

        if (result != SQLITE_OK) {
            mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
            sodium_memzero(plain.data(), plain.size());
            getch();
            return;
        }

        sqlite3_bind_int(stmt, 1, rows[sel].id);

        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);

        clear();
        mvprintw(1, 2, "--=/ REIM-PASS \\=--");

        if (ok) {
            mvprintw(3, 4, "Deleted %s login data", rows[sel].site.c_str());
        } else {
            mvprintw(3, 4, "Delete Error: %s", sqlite3_errmsg(data));
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
            deleteData(data, passphrase);
            break;
        case 2:
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

int main () {
    // Must be called before any libsodium function is used
    if (sodium_init() < 0) {
        std::cerr << "error: libsodium initialization failed\n";
        return 1;
    }

    sqlite3* data;

    if (sqlite3_open("ReimPass/data.db", &data) != SQLITE_OK) {
        std::cerr << "error: " << sqlite3_errmsg(data) << "\n";
        return 1;
    }

    const char* loginDataTable =
        "CREATE TABLE IF NOT EXISTS login_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "site TEXT NOT NULL,"
        "login TEXT NOT NULL,"
        "password BLOB NOT NULL);";

    char* error = nullptr;
    if (sqlite3_exec(data, loginDataTable, nullptr, nullptr, &error) != SQLITE_OK) {
        std::cerr << "Blad SQL: " << error << "\n";
        sqlite3_free(error);
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    std::string passphrase = getPassphrase();

    printMainMenu(data, passphrase);

    endwin();

    sqlite3_close(data);

    return 0;
}