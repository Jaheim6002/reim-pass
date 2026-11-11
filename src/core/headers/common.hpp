#pragma once
#include <string>
#include <ncurses.h>
#include <vector>
#include <sqlite3.h>
#include <sodium.h>
#include <cstring>
#include <stdexcept>
#include <optional>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <fstream>

namespace fs = std::filesystem;

class Common {
    protected:

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

        std::string generateUid() {
            unsigned char raw[16];
            randombytes_buf(raw, sizeof raw);

            char hex[sizeof raw * 2 + 1];
            sodium_bin2hex(hex, sizeof hex, raw, sizeof raw);
            return hex;
        }

        bool insertSQL (sqlite3* data, const std::string& site, const std::string& login, const std::string& password) {
            const char* insertQuery =
                "INSERT INTO login_data (uid, site, login, password) "
                "VALUES (?, ?, ?, ?);";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(data, insertQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
                getch();
                return false;
            }

            std::string uid = generateUid();

            sqlite3_bind_text(stmt, 1, uid.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, site.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, login.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_blob(stmt, 4, password.data(), (int)password.size(), SQLITE_TRANSIENT);

            bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
            if (!ok) {
                mvprintw(7, 4, "Insert Error: %s", sqlite3_errmsg(data));
                getch();
            }

            sqlite3_finalize(stmt);
            return ok;
        }

        bool updateSQL (sqlite3* data, int id, const std::string& site, const std::string& login, const std::string& password) {
            const char* updateQuery =
                "UPDATE login_data "
                "SET site = ?, "
                "login = ?, "
                "password = ?, "
                "last = strftime('%Y-%m-%d %H:%M','now') "
                "WHERE id = ?;";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(data, updateQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                mvprintw(7, 4, "SQL Error: %s", sqlite3_errmsg(data));
                getch();
                return false;
            }

            sqlite3_bind_text(stmt, 1, site.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, login.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_blob(stmt, 3, password.data(), (int)password.size(), SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 4, id);

            bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
            if (!ok) {
                mvprintw(7, 4, "Update Error: %s", sqlite3_errmsg(data));
                getch();
            }

            sqlite3_finalize(stmt);
            return ok;
        }

        bool deleteSQL (sqlite3* data, int id) {
            const char* deleteQuery =
                "UPDATE login_data "
                "SET site = '', "
                "login = '', "
                "password = x'', "
                "last = strftime('%Y-%m-%d %H:%M','now'), "
                "deleted = 1 "
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
                mvprintw(7, 4, "Update Error: %s", sqlite3_errmsg(data));
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

        // Synchronisation

        static constexpr size_t kChunk = 64 * 1024;

        static sqlite3* openFromMemory(const std::string& data) {
            sqlite3* db = nullptr;
            if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
                return nullptr;
            }

            auto* mem = static_cast<unsigned char*>(sqlite3_malloc64(data.size()));
            if (!mem) { 
                sqlite3_close(db); return nullptr; 
            }
            memcpy(mem, data.data(), data.size());

            if (sqlite3_deserialize(db, "main", mem, data.size(), data.size(), SQLITE_DESERIALIZE_FREEONCLOSE) != SQLITE_OK) {
                sqlite3_close(db);
                return nullptr;
            }
            return db;
        }

        void encryptFileTo(const fs::path& in, const fs::path& out, const std::string& pass) {
            std::ifstream fin(in, std::ios::binary);
            if (!fin) throw std::runtime_error("Can't open " + in.string());
            std::ofstream fout(out, std::ios::binary | std::ios::trunc);
            if (!fout) throw std::runtime_error("Can't open " + out.string());

            unsigned char salt[crypto_pwhash_SALTBYTES];
            unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
            unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
            randombytes_buf(salt, sizeof salt);

            if (crypto_pwhash(key, sizeof key, pass.c_str(), pass.size(), salt,
                            crypto_pwhash_OPSLIMIT_INTERACTIVE,
                            crypto_pwhash_MEMLIMIT_INTERACTIVE,
                            crypto_pwhash_ALG_DEFAULT) != 0) {
                throw std::runtime_error("Not enough memory");
            }

            crypto_secretstream_xchacha20poly1305_state st;
            crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
            sodium_memzero(key, sizeof key);

            fout.write(reinterpret_cast<char*>(salt), sizeof salt);
            fout.write(reinterpret_cast<char*>(header), sizeof header);

            std::vector<unsigned char> buf(kChunk);
            std::vector<unsigned char> enc(kChunk + crypto_secretstream_xchacha20poly1305_ABYTES);

            bool last = false;
            while (!last) {
                fin.read(reinterpret_cast<char*>(buf.data()), kChunk);
                size_t n = fin.gcount();
                last = fin.eof() || fin.peek() == EOF;

                unsigned long long encLen;
                crypto_secretstream_xchacha20poly1305_push(
                    &st, enc.data(), &encLen, buf.data(), n, nullptr, 0,
                    last ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0);
                fout.write(reinterpret_cast<char*>(enc.data()), encLen);
            }

            fout.close();
            if (!fout) throw std::runtime_error("Write failed");
        }

        std::optional<std::string> decryptFile(const fs::path& in, const std::string& pass) {
            std::ifstream fin(in, std::ios::binary);
            if (!fin) return std::nullopt;

            unsigned char salt[crypto_pwhash_SALTBYTES];
            unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
            unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

            fin.read(reinterpret_cast<char*>(salt), sizeof salt);
            fin.read(reinterpret_cast<char*>(header), sizeof header);
            if (!fin) return std::nullopt;

            if (crypto_pwhash(key, sizeof key, pass.c_str(), pass.size(), salt,
                            crypto_pwhash_OPSLIMIT_INTERACTIVE,
                            crypto_pwhash_MEMLIMIT_INTERACTIVE,
                            crypto_pwhash_ALG_DEFAULT) != 0) {
                return std::nullopt;
            }

            crypto_secretstream_xchacha20poly1305_state st;
            int rc = crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key);
            sodium_memzero(key, sizeof key);
            if (rc != 0) return std::nullopt;

            const size_t encChunk = kChunk + crypto_secretstream_xchacha20poly1305_ABYTES;
            std::vector<unsigned char> enc(encChunk), buf(kChunk);
            std::string result;

            bool gotFinal = false;
            while (!gotFinal) {
                fin.read(reinterpret_cast<char*>(enc.data()), encChunk);
                size_t n = fin.gcount();
                if (n == 0) break;

                unsigned long long len;
                unsigned char tag;
                if (crypto_secretstream_xchacha20poly1305_pull(
                        &st, buf.data(), &len, &tag, enc.data(), n, nullptr, 0) != 0) {
                    sodium_memzero(buf.data(), buf.size());
                    sodium_memzero(result.data(), result.size());
                    return std::nullopt;
                }
                result.append(reinterpret_cast<char*>(buf.data()), len);
                gotFinal = (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL);
            }

            sodium_memzero(buf.data(), buf.size());
            if (!gotFinal) {                       // plik obcięty
                sodium_memzero(result.data(), result.size());
                return std::nullopt;
            }
            return result;
        }

        void fsyncPath(const fs::path& p) {
            int fd = ::open(p.c_str(), O_RDONLY);
            if (fd < 0) {
                throw std::system_error(errno, std::generic_category(), "open");
            }
            int rc = ::fsync(fd);
            int err = errno;
            ::close(fd);
            if (rc != 0) {
                throw std::system_error(err, std::generic_category(), "fsync");
            }
        }

        void atomicCopy(const fs::path& src, const fs::path& dst, const std::string& pass = "") {
            if (fs::is_directory(src)) {
                fs::create_directories(dst);
                for (const auto& e : fs::directory_iterator(src)) {
                    atomicCopy(e.path(), dst / e.path().filename(), pass);
                }
                return;
            }

            if (dst.has_parent_path()) {
                fs::create_directories(dst.parent_path());
            }

            fs::path tmp = dst;
            tmp += ".tmp";

            try {
                if (pass != "") {
                    encryptFileTo(src, tmp, pass);
                } else {
                    fs::copy_file(src, tmp, fs::copy_options::overwrite_existing);
                }
                fsyncPath(tmp);
            } catch (...) {
                std::error_code ec;
                fs::remove(tmp, ec);
                throw;
            }

            fs::rename(tmp, dst);
        }

        void atomicWrite(const fs::path& dst, const std::string& data) {
            fs::path tmp = dst;
            tmp += ".tmp";
            try {
                {
                    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
                    if (!f) throw std::runtime_error("Can't open " + tmp.string());
                    f.write(data.data(), data.size());
                    f.flush();
                    if (!f) throw std::runtime_error("Write failed");
                }
                fsyncPath(tmp);
            } catch (...) {
                std::error_code ec;
                fs::remove(tmp, ec);
                throw;
            }
            fs::rename(tmp, dst);
        }

        std::string hashHex(const std::string& data) {
            unsigned char h[crypto_generichash_BYTES];
            crypto_generichash(h, sizeof h,
                            reinterpret_cast<const unsigned char*>(data.data()),
                            data.size(), nullptr, 0);

            char hex[crypto_generichash_BYTES * 2 + 1];
            sodium_bin2hex(hex, sizeof hex, h, sizeof h);
            return hex;
        }

        std::string fileHashHex(const fs::path& path) {
            std::ifstream f(path, std::ios::binary);
            if (!f) {
                throw std::runtime_error("Can't open " + path.string());
            }

            std::string data((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
            std::string hex = hashHex(data);
            sodium_memzero(data.data(), data.size());
            return hex;
        }

        std::string readBase (const fs::path& path) {
            std::ifstream f(path);
            std::string hash;
            std::getline(f, hash);
            return hash;
        }

        void writeBase (const fs::path& path, const std::string& hash) {
            std::ofstream f(path, std::ios::trunc);
            f << hash << '\n';
            f.flush();
            f.close();
            if (!f) {
                throw std::runtime_error("Can't write " + path.string());
            }
        }
};