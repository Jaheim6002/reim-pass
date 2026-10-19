# Reim Pass

Simple terminal password manager for unix-like operating systems.

### Extended description

Made using `ncurses` library, so it won't work on Windows.

The manager works really simple, you write your passphrase on start and then you add some login data to the database. Passwords are encrypted with your the passphrase you logged in to the manager.

Login data is stored in SQLite 3 database ass well as the passphrase (so you don't have to always type your passphrase twice and if you do any mistakes the program will say it).

**Nobody can access your passwords without the passphrase.**

Everything is written in C++.

##### Dependencies

1. ncurses
2. SQLite 3
3. libsodium
4. nlohmann/json

##### Compilation

To compile just enter the repository and use this command:
```bash
g++ -std=c++17 main.cpp -o reim-pass -lncurses -lsqlite3 -lsodium 
```

##### Configuration

you can configure the program with a `config.json` file in the repository.

for now, there's only option to change location of the database with `data_directory` property. Default directory is in your `home/.reim-pass/`.