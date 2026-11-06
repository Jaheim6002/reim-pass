***

<p align="center">
    <img src="reim-pass.png" alt="Logo" width="500">
</p>

***


<p align="center">
    Simple terminal password manager for unix-like operating systems.
</p>


## Table of contents

- [Extended description](#extended-description)
- [Setting Up](#setting-up)
  - [Dependencies](#dependencies)
  - [Compilation](#compilation)
  - [Configuration](#configuration)
  - [Synchronisation](#synchronisation)
- [Trouble-shooting](#trouble-shooting)

## Extended description

Made using `ncurses` library, **so it won't work on Windows**.

The manager works really simple, you write your passphrase on start and then you add some login data to the database. Passwords are encrypted with your the passphrase you logged in to the manager.

Login data is stored in SQLite 3 database as well as the passphrase (so you don't have to always type your passphrase twice and if you do any mistakes the program will say it).

In this password manager you can:

- Add login data
- Browse login data
- Update login data
- Delete login data
- Generate safe passwords
- Keep all your passwords safe

**Nobody can access your passwords without the passphrase.**

Everything is written in C++.

## Setting Up

### Dependencies

1. ncurses
2. SQLite 3
3. libsodium
4. nlohmann/json

### Compilation

To compile just enter the repository and use this commands:

```bash

cmake -S . -B build
cmake --build build

```

And optionally (recommended for a comfort use) you can install the program globally with:

```bash

sudo cmake --install build

```

### Configuration

you can configure the program with a `config.json` file in the `~/.config/reim-pass`.

- Set `data_directory` property to directory in which you want the database to be stored. Default directory is in `~/.local/share/reim-pass/`.
- Change `synchronisation` property to true if you want synchronisation between devices. Default is `false`.
- Set `sync_e2e_encryption` property to your E2E encryption passphrase (Used only for synchronisation purposes, ignored if previous is set to `false`). Default is `INSERT.HERE.A.STRONG.PASSsPHRASE`. It's recommended to generate the passphrase using Reim Pass
- Change `copy_password` property to `false` if you don't want the program to automatically copy the password to your clipboard. Default is `true`.
- Set `clear_clipboard` property to time after you want the program to automatically clear your clipboard (in seconds). Default is `20`.
- Change `show_password` property to `false` if you want to just copy the password to clipboard without showing it in the terminal. Default is `true`.

### Synchronisation

To setup synchronisation first what you need is an external synchronisation software like [Syncthing](https://syncthing.net/) or [Dropbox](https://www.dropbox.com/). Then you set up folder synchronisation in your `[data_directory]/sync/` **DO NOT SET FOLDER SYNCHRONISATION IN ENTIRE `[data_directory]`** because some files have to stay local. Make sure that you have the `synchronisation` property in config set to true **it won't work else**. Remember also to set on every device the same `sync_e2e_encryption`. If you change the property on one device, but don't do the same on every other one, the entire synchronisation won't work.

## Trouble-shooting

- If you get the error message "Couldn't find clipboard", check if the correct clipboard is installed on your system:
  - wlcopy (for Wayland)
  - xlip & xsel (for X11)
  - pbcopy (for Mac)

That's all the known problems for now. If you found any please inform me in the "issues" tab on github.
