#include "headers/all.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <pwd.h>
#include <unistd.h>

using json = nlohmann::json;

class StartProgram : Common {
    public:

        int run() {
            initscr();
            cbreak();
            noecho();
            keypad(stdscr, TRUE);
            curs_set(0);

            if (sodium_init() < 0) {
                clear();
                mvprintw(1, 2, "Libsodium initialization failed");
                getch();
                return 1;
            }

            fs::path directory = homeDir() / ".local" / "share" / "reim-pass";
            bool sync = false;
            std::string e2ePass = "";
            bool copyPassword = true;
            int copySeconds = 20;
            bool showPassword = true;
            loadConfig(&directory, &sync, &e2ePass, &copyPassword, &copySeconds, &showPassword);
            directory = fs::absolute(directory);
            fs::path dbPath = directory / "logindata.db";

            bool firstRun = !fs::exists(dbPath);

            std::error_code ec;
            fs::create_directories(directory, ec);
            if (sync) {
                fs::create_directories(directory / "sync", ec);
            }
            if (ec) {
                clear();
                mvprintw(1, 2, "Cannot create directory: %s", ec.message().c_str());
                getch();
                return 1;
            }

            Merging merg;
            MainProgram mp;

            if (!firstRun) {
                if (sync) {
                merg.Sync(directory, e2ePass);
                }
            }

            sqlite3* data = nullptr;
            if (sqlite3_open(dbPath.string().c_str(), &data) != SQLITE_OK) {
                clear();
                mvprintw(1, 2, "SQL Error: %s", sqlite3_errmsg(data));
                getch();

                sqlite3_close(data);
                return 1;
            }

            std::string passphrase;
            
            if (firstRun) {
                const char* loginDataTable =
                    "CREATE TABLE IF NOT EXISTS login_data ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "uid TEXT NOT NULL UNIQUE,"
                        "site TEXT NOT NULL,"
                        "login TEXT NOT NULL,"
                        "password BLOB NOT NULL,"
                        "last TEXT NOT NULL DEFAULT (strftime('%d-%m-%Y %H:%M','now')),"
                        "deleted INTEGER NOT NULL DEFAULT 0"
                    ");";

                char* error = nullptr;
                if (sqlite3_exec(data, loginDataTable, nullptr, nullptr, &error) != SQLITE_OK) {
                    endwin();
                    clear();
                    mvprintw(1, 2, "SQL Error: %s", error);
                    getch();
                    sqlite3_free(error);
                    sqlite3_close(data);
                    return 1;
                }
                passphrase = SignUp(data);
            } else {
                passphrase = LogIn(data);
            }

            if (passphrase.empty()) {
                endwin();
                sqlite3_close(data);
                return 1;
            }

            mp.run(data, passphrase, copyPassword, copySeconds, showPassword);

            sqlite3_close(data);
            if (sync) {
                merg.Sync(directory, e2ePass);
            }
            sodium_memzero(passphrase.data(), passphrase.size());
            endwin();
            return 0;
        }

    private:
    
        static constexpr const char* kVerifierText = "reimpass-verifier";
        
        static fs::path homeDir() {
            if (const char* home = std::getenv("HOME"); home && *home) {
                return home;
            }
            if (const passwd* pw = getpwuid(getuid())) {
                return pw->pw_dir;
            }
            return {};
        }

        static fs::path configDir() {
            if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
                return fs::path(xdg) / "reim-pass";
            }
            return homeDir() / ".config" / "reim-pass";
        }

        static bool createDefaultConfig(const fs::path& configPath, const fs::path& defaultDir) {
            std::error_code ec;
            fs::create_directories(configPath.parent_path(), ec);
            if (ec) {
                return false;
            }

            json config = {
                {"data_directory", defaultDir.string()},
                {"synchronisation", false},
                {"sync_e2e_encryption", "INSERT.HERE.A.STRONG.PASSPHRASE"},
                {"copy_password", true},
                {"clear_clipboard", 20},
                {"show_password", true}
            };

            std::ofstream file(configPath);
            if (!file.is_open()) {
                return false;
            }
            file << config.dump(4) << '\n';
            return file.good();
        }

        bool loadConfig (fs::path* directory, bool* sync, std::string* e2ePass, bool* copyPassword, int* copySeconds, bool* showPassword) {
            fs::path configPath = configDir() / "config.json";

            std::error_code ec;
            if (!fs::exists(configPath, ec)) {
                createDefaultConfig(configPath, *directory);
                return false; // no config -> default directory
            }

            std::ifstream file(configPath);
            if (!file.is_open()) {
                return false;
            }

            try {
                json config = json::parse(file);
                std::string dir = config.value("data_directory", "");
                *sync = config.value("synchronisation", false);
                *e2ePass = config.value("sync_e2e_encryption", "INSERT.HERE.A.STRONG.PASSPHRASE");
                *copyPassword = config.value("copy_password", true);
                *copySeconds = config.value("clear_clipboard", 20);
                *showPassword = config.value("show_password", true);
                if (dir.empty()) {
                    return false;
                }
                *directory = homeDir() / dir;
                return true;
            } catch (const json::exception&) {
                return false;
            }
        }

        std::string SignUp (sqlite3* data) {
            std::string passphrase;
            std::string passphrase2;

            while (true) {
                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");
                passphrase = inputRequired(3, "Enter Passphrase", false);
                passphrase2 = inputRequired(4, "Enter once again", false);

                if (passphrase == passphrase2) {
                    break;
                }
                mvprintw(2, 4, "Passphrases are not the same (press Enter to try once again)");
                getch();
            }

            // Only an encrypted verifier is stored, never the passphrase itself
            std::string verifier = encryptPassword(kVerifierText, passphrase);

            if (!insertSQL(data, "reimpass", "reimpass.user", verifier)) {
                return "";
            }
            return passphrase;
        }

        std::string LogIn (sqlite3* data) {
            std::optional<std::string> enc = selectPassword(data, kVerifierId);
            if (!enc) {
                return "";
            }

            for (int attempt = 0; attempt < 3; attempt++) {
                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");
                std::string passphrase = inputRequired(3, "Enter Passphrase", false);

                std::string check;
                if (decryptPassword(*enc, passphrase, check)) {
                    return passphrase;
                }

                mvprintw(2, 4, "Wrong Passphrase (press Enter to try again)");
                getch();
            }
            return "";
        }
};

int main () {
    StartProgram app;
    return app.run();
}