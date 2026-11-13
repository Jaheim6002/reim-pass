#include "config.hpp"

class AppContext {
public:
    static AppContext& Get() {
        static AppContext instance;
        return instance;
    }
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    // Config
    Config conf;

    fs::path directory = conf.homeDir() / ".local" / "share" / "reim-pass";
    bool sync;
    std::string e2ePass = "";
    bool copyPassword;
    int copySeconds;
    bool showPassword;
    int genPassLenght;

    // Everything else (idfk how to cathegorise)
    std::string passphrase;
    sqlite3* data = nullptr; // Set in the run() or main() to display the potential error.

    bool firstRun = false;

    void initApp ();
private:
    AppContext() = default;
};