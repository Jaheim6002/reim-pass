# Reim Pass

Simple terminal password manager for unix-like operating systems.

## Extended description

Made using `ncurses` library, **so it won't work on Windows**.

The manager works really simple, you write your passphrase on start and then you add some login data to the database. Passwords are encrypted with your the passphrase you logged in to the manager.

Login data is stored in SQLite 3 database as well as the passphrase (so you don't have to always type your passphrase twice and if you do any mistakes the program will say it).

In this password manager you can:

- Add login data
- Browse login data
- Update login data
- Delete login data

**Nobody can access your passwords without the passphrase.**

Everything is written in C++.

### Dependencies

1. ncurses
2. SQLite 3
3. libsodium
4. nlohmann/json

### Compilation

To compile just enter the repository and use this command:

```bash

g++ -std=c++17 src/main.cpp -o reim-pass -lncurses -lsqlite3 -lsodium 

```

### Configuration

you can configure the program with a `config.json` file in the `~/.config/reim-pass`.

- Set `data_directory` property to directory in which you want the database to be stored. Default directory is in `~/.local/share/reim-pass/`.
- Change `synchronisation` property to true if you want synchronisation between devices. Default is `false`.
- Change `copy_password` property to false if you don't want the programme to automatically copy the password to your clipboard. Default is `true`.
- Set `clear_clipboard` property to time after you want the programme to automatically clear your clipboard (in seconds). Default is `90`.
- Change `copy_without_showing` property to true if you want to just copy the password to clipboard without showing it in the terminal. Default is `false`.
