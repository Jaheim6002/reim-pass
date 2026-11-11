#include "headers/merge.hpp"

void Merging::Sync(const fs::path& directory, std::string e2ePass) {
    fs::path basePath  = directory / "base";
    fs::path localPath = directory / "logindata.db";
    fs::path syncPath  = directory / "sync" / "logindata.db";

    bool localExists  = fs::exists(localPath);
    bool remoteExists = fs::exists(syncPath);
    if (!localExists && !remoteExists) return;

    std::string remote, remoteHash;
    if (remoteExists) {
        auto dec = decryptFile(syncPath, e2ePass);
        if (!dec) throw std::runtime_error("Can't decrypt sync file (wrong sync_e2e_encryption?)");
        remote = std::move(*dec);
        remoteHash = hashHex(remote);
    }

    if (!localExists) {                         // new device: pull
        atomicWrite(localPath, remote);
        writeBase(basePath, remoteHash);
    } else {
        std::string base  = readBase(basePath);
        std::string local = fileHashHex(localPath);

        if (!remoteExists) {                    // nothing remote yet: push
            atomicCopy(localPath, syncPath, e2ePass);
            writeBase(basePath, local);
        } else if (local == remoteHash) {
            writeBase(basePath, local);
        } else if (local == base) {             // only remote changed: pull
            atomicWrite(localPath, remote);
            writeBase(basePath, remoteHash);
        } else if (remoteHash == base) {        // only local changed: push
            atomicCopy(localPath, syncPath, e2ePass);
            writeBase(basePath, local);
        } else {                                // both changed: merge, then push
            Merge(localPath, remote);
            atomicCopy(localPath, syncPath, e2ePass);
            writeBase(basePath, fileHashHex(localPath));
        }
    }
    sodium_memzero(remote.data(), remote.size());
}

void Merging::Merge(const fs::path& localPath, const std::string& syncData) {
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