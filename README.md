# Reim Pass

Simple terminal password manager for unix-like operating systems.

### Extended description

Made using `ncurses` library, so it won't work on Windows.

The manager works really simple, you write your passphrase on start and then you add some login data to the database. Passwords are encrypted with your the passphrase you logged in to the manager.

Login data is stored in SQLite database. **The passphrase isn't stored anywhere.** If you enter Reim Pass with wrong passphrase you'll see all saved passwords corrupted.

Everything is written in C++ in one short script.