#include "headers/manager.hpp"
#include <sodium/utils.h>

int MainProgram::run(sqlite3* data, const std::string& passphrase) {
    printMainMenu(data, passphrase);
    return 0;
}

void MainProgram::insertData(sqlite3* data, const std::string& passphrase) {
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

std::vector<MainProgram::LoginRow> MainProgram::fetchRows(sqlite3* db, const std::string& filter) {
    std::vector<LoginRow> rows;

    const char* selectQuery =
        "SELECT id, site, login, password, last FROM login_data "
        "WHERE (site LIKE ? OR login LIKE ?) "
        "AND id != ? AND deleted != 1 "
        "ORDER BY site DESC, last ASC;";

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
        r.last = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}

std::optional<MainProgram::LoginRow> MainProgram::pickRow(sqlite3* data) {
    clear();
    mvprintw(1, 2, "--=/ REIM-PASS \\=--");
    std::string filter = input(3, "Search (empty = all)");

    std::vector<LoginRow> rows = fetchRows(data, filter);

    std::vector<std::string> menu;
    for (const auto& r : rows) {
        menu.push_back(r.site + "  |  " + r.login + "  |  " + r.last);
    }
    menu.push_back("> Back");

    int sel = 0;
    printMenu(menu, sel);

    if (sel == (int)rows.size()) {
        return std::nullopt;
    }

    return std::move(rows[sel]);
}

void MainProgram::selectData(sqlite3* data, const std::string& passphrase) {
    auto picked = pickRow(data);
    if (!picked) {
        return;
    }
    const LoginRow& row = *picked;

    std::string plain;
    clear();
    mvprintw(1, 2, "--=/ REIM-PASS \\=--");
    mvprintw(3, 4, "Site:     %s", row.site.c_str());
    mvprintw(4, 4, "Login:    %s", row.login.c_str());

    if (decryptPassword(row.encPassword, passphrase, plain)) {
        mvprintw(5, 4, "Password: %s", plain.c_str());
    } else {
        mvprintw(5, 4, "Wrong passphrase / corrupted data");
    }

    mvprintw(7, 4, "Press any key to continue.");

    getch();

    // Wipe the plaintext only after it has been displayed
    sodium_memzero(plain.data(), plain.size());
}

void MainProgram::updateData(sqlite3* data, const std::string& passphrase) {
    auto picked = pickRow(data);
    if (!picked) {
        return;
    }
    const LoginRow& row = *picked;

    std::string plain;
    clear();

    std::string site = input(3, "Site (Leave empty to not change)");
    std::string login = input(4, "Login (Leave empty to not change)");
    std::string password = input(5, "Password (Leave empty to not change)", false);

    if (site == "") {
        site = row.site;
    }
    if (login == "") {
        login = row.login;
    }
    if (password == "") {
        // decryptPassword returns bool; the decrypted text lands in `plain`
        if (!decryptPassword(row.encPassword, passphrase, plain)) {
            clear();
            mvprintw(1, 2, "--=/ REIM-PASS \\=--");
            mvprintw(3, 4, "Wrong passphrase / corrupted data");
            mvprintw(5, 4, "Press any key to continue.");
            getch();
            return;
        }
        password = plain;
    }

    std::string encrypted = encryptPassword(password, passphrase);

    bool ok = updateSQL(data, row.id, site, login, encrypted);

    if (ok) {
        mvprintw(7, 4, "Login data updated!");
    }

    sodium_memzero(plain.data(), plain.size());
    sodium_memzero(password.data(), password.size());

    mvprintw(9, 4, "Press any key to continue.");

    getch();
}

void MainProgram::deleteData(sqlite3* data, const std::string& passphrase) {
    auto picked = pickRow(data);
    if (!picked) {
        return;
    }
    const LoginRow& row = *picked;

    std::string plain;

    // The passphrase is verified by decrypting the entry before allowing deletion
    if (decryptPassword(row.encPassword, passphrase, plain)) {

        clear();
        mvprintw(1, 2, "--=/ REIM-PASS \\=--");
        std::string in = input(3, "Are you sure? Login data will be permamently deleted (y/N)");

        if (in != "y" && in != "Y") {
            sodium_memzero(plain.data(), plain.size());
            return;
        }

        bool ok = deleteSQL(data, row.id);

        clear();
        mvprintw(1, 2, "--=/ REIM-PASS \\=--");

        if (ok) {
            mvprintw(3, 4, "Deleted %s login data", row.site.c_str());
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

void MainProgram::generatePassword() {
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

void MainProgram::printBrowseMenu(sqlite3* data, const std::string& passphrase) {
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

void MainProgram::printMainMenu(sqlite3* data, const std::string& passphrase) {
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