#include "headers/mainprogram.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

class StartProgram : Common {
    public:
        int run() {
            if (sodium_init() < 0) {
                clear();
                mvprintw(1, 2, "Libsodium initialization failed");
                getch();
                return 1;
            }

            fs::path directory = ".reim-pass";
            loadConfig(&directory);
            directory = fs::absolute(directory);
            fs::path dbPath = directory / "logindata.db";

            bool firstRun = !fs::exists(dbPath);

            std::error_code ec;
            fs::create_directories(directory, ec);
            if (ec) {
                clear();
                mvprintw(1, 2, "Cannot create directory: %s", ec.message().c_str());
                getch();
                return 1;
            }

            sqlite3* data = nullptr;
            if (sqlite3_open(dbPath.string().c_str(), &data) != SQLITE_OK) {
                clear();
                mvprintw(1, 2, "SQL Error: %s", sqlite3_errmsg(data));
                getch();

                sqlite3_close(data);
                return 1;
            }

            initscr();
            cbreak();
            noecho();
            keypad(stdscr, TRUE);
            curs_set(0);

            std::string passphrase;

            if (firstRun) {
                const char* loginDataTable =
                    "CREATE TABLE IF NOT EXISTS login_data ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "site TEXT NOT NULL,"
                    "login TEXT NOT NULL,"
                    "password BLOB NOT NULL);";

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

            MainProgram mp;
            mp.run(data, passphrase);

            sodium_memzero(passphrase.data(), passphrase.size());
            endwin();
            sqlite3_close(data);
            return 0;
        }

    private:
        static constexpr const char* kVerifierText = "reimpass-verifier";

        bool loadConfig (fs::path* directory) {
            std::ifstream file("config.json");
            if (!file.is_open()) {
                return false;   // no config -> default directory
            }

            try {
                json config = json::parse(file);
                std::string dir = config.value("data_directory", "");
                if (dir.empty()) {
                    return false;
                }
                *directory = dir;
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