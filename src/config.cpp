#include "headers/config.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <pwd.h>

using json = nlohmann::json;

fs::path Config::homeDir() {
  if (const char *home = std::getenv("HOME"); home && *home) {
    return home;
  }
  if (const passwd *pw = getpwuid(getuid())) {
    return pw->pw_dir;
  }
  return {};
}

fs::path Config::configDir() {
  if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
    return fs::path(xdg) / "reim-pass";
  }
  return homeDir() / ".config" / "reim-pass";
}

bool Config::createDefaultConfig(const fs::path &configPath, const fs::path &defaultDir) {
  std::error_code ec;
  fs::create_directories(configPath.parent_path(), ec);
  if (ec) {
    return false;
  }

  json config = {{"data_directory", defaultDir.string()},
                 {"synchronisation", false},
                 {"sync_e2e_encryption", "INSERT.HERE.A.STRONG.PASSPHRASE"},
                 {"copy_password", true},
                 {"clear_clipboard", 20},
                 {"show_password", true},
                 {"gen_password_length", 15}
                };

  std::ofstream file(configPath);
  if (!file.is_open()) {
    return false;
  }
  file << config.dump(4) << '\n';
  return file.good();
}

bool Config::loadConfig(fs::path *directory, bool *sync, std::string *e2ePass, bool *copyPassword, int *copySeconds, bool *showPassword, int *genPassLenght) {
  fs::path configPath = configDir() / "config.json";

  std::error_code ec;
  if (!fs::exists(configPath, ec)) {
    createDefaultConfig(configPath, *directory);
  }

  std::ifstream file(configPath);
  if (!file.is_open()) {
    return false;
  }

  try {
    json config = json::parse(file);
    std::string dir = config.value("data_directory", "");
    *sync = config.value("synchronisation", false);
    *e2ePass =
        config.value("sync_e2e_encryption", "INSERT.HERE.A.STRONG.PASSPHRASE");
    *copyPassword = config.value("copy_password", true);
    *copySeconds = config.value("clear_clipboard", 20);
    *showPassword = config.value("show_password", true);
    *genPassLenght = config.value("gen_password_length", 16);
    if (dir.empty()) {
      return false;
    }
    *directory = homeDir() / dir;
    return true;
  } catch (const json::exception &) {
    return false;
  }
}