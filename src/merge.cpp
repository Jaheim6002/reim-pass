#include "headers/merge.hpp"

void Merging::Sync (const fs::path& directory, std::string e2ePass) {
    fs::path basePath = directory / "base";
    fs::path localPath = directory / "logindata.db";
    fs::path syncPath = directory / "sync" / "logindata.db";
    std::string base = readBase(basePath);
    std::string local = fileHashHex(localPath);
    std::string sync;
    if (fs::exists(syncPath)) {
        auto dec = decryptFile(syncPath, e2ePass);
        if (!dec) {
            throw std::runtime_error("Can't decrypt " + syncPath.string());
        }
        sync = std::move(*dec);
    }
    std::string syncHash = hashHex(sync);

    if (local == syncHash) {
        writeBase(basePath, local);
    } else if (local == base) {
        mvprintw(1, 2, "--=/ REIM-PASS \\=--");
        mvprintw(3, 4, "Sync: Pulling...");
        atomicCopy(syncPath, localPath); // pull
        writeBase(basePath, syncHash);
    } else if (syncHash == base || !fs::exists(syncPath)) {
        mvprintw(1, 2, "--=/ REIM-PASS \\=--");
        mvprintw(3, 4, "Sync: Pushing...");
        atomicCopy(localPath, syncPath, e2ePass); // push
        writeBase(basePath, syncHash);
    } else {
        mvprintw(1, 2, "--=/ REIM-PASS \\=--");
        mvprintw(3, 4, "Sync: Conflict! — Merging...");
        //Merge();
        atomicCopy(localPath, syncPath, e2ePass); // push for now
        writeBase(basePath, local);
    }
    sodium_memzero(sync.data(), sync.size());
}

void Merging::Merge(const fs::path& localPath, const std::string& syncData, const std::string& passphrase) {
    sqlite3* local = nullptr;
    if (sqlite3_open(localPath.c_str(), &local) != SQLITE_OK) {
        throw std::runtime_error("Can't open local database");
    }

    sqlite3* remote = openFromMemory(syncData);
    if (!remote) { 
        sqlite3_close(local); throw std::runtime_error("Can't open sync database"); 
    }

    try {
        auto ver = selectPassword(remote, kVerifierId);
        std::string tmp;
        if (!ver || !decryptPassword(*ver, passphrase, tmp)) {
            throw std::runtime_error("Sync database uses a different passphrase");
        }

        sqlite3_exec(local, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);

        sqlite3_stmt* sel = nullptr;   // all record except the main passphrase
        sqlite3_prepare_v2(remote,
            "SELECT uid, site, login, password, last, deleted "
            "FROM login_data WHERE id != ?;", 
            -1, &sel, nullptr
        );
        sqlite3_bind_int(sel, 1, kVerifierId);

        sqlite3_stmt* find = nullptr;
        sqlite3_prepare_v2(local,
            "SELECT last FROM login_data WHERE uid = ?;", 
            -1, &find, nullptr
        );

        sqlite3_stmt* ins = nullptr;
        sqlite3_prepare_v2(local,
            "INSERT INTO login_data (uid, site, login, password, last, deleted) "
            "VALUES (?, ?, ?, ?, ?, ?);", 
            -1, &ins, nullptr
        );

        sqlite3_stmt* upd = nullptr;
        sqlite3_prepare_v2(local,
            "UPDATE login_data SET site=?, login=?, password=?, last=?, deleted=? "
            "WHERE uid = ?;", 
            -1, &upd, nullptr
        );

        while (sqlite3_step(sel) == SQLITE_ROW) {
            const char* uid = (const char*)sqlite3_column_text(sel, 0);
            const char* last = (const char*)sqlite3_column_text(sel, 4);

            sqlite3_reset(find);
            sqlite3_bind_text(find, 1, uid, -1, SQLITE_TRANSIENT);

            sqlite3_stmt* st;
            if (sqlite3_step(find) == SQLITE_ROW) {
                const char* localLast = (const char*)sqlite3_column_text(find, 0);
                if (strcmp(last, localLast) <= 0) {
                    continue;
                }
                st = upd;
                sqlite3_reset(st);
                for (int i = 0; i < 5; i++) {
                    sqlite3_bind_value(st, i + 1, sqlite3_column_value(sel, i + 1));
                }
                sqlite3_bind_text(st, 6, uid, -1, SQLITE_TRANSIENT);
            } else {
                st = ins;
                sqlite3_reset(st);
                for (int i = 0; i < 6; i++) {
                    sqlite3_bind_value(st, i + 1, sqlite3_column_value(sel, i));
                }
            }
            if (sqlite3_step(st) != SQLITE_DONE) {
                throw std::runtime_error(sqlite3_errmsg(local));
            }
        }

        for (auto* s : {sel, find, ins, upd}) sqlite3_finalize(s);
        sqlite3_exec(local, "COMMIT", nullptr, nullptr, nullptr);
    } catch (...) {
        sqlite3_exec(local, "ROLLBACK", nullptr, nullptr, nullptr);
        sqlite3_close(remote);
        sqlite3_close(local);
        throw;
    }
    sqlite3_close(remote);
    sqlite3_close(local);
}