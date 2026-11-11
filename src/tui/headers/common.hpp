#include "../../core/headers/common.hpp"

class TuiCommon : protected Common {
    protected:

        int printMenu (std::vector<std::string> menu, int& selected) {
            int n = menu.size();

            while (true) {
                clear();
                mvprintw(1, 2, "--=/ REIM-PASS \\=--");

                for (int i = 0; i < n; i++) {
                    if (i == selected) {
                        attron(A_REVERSE);
                    }

                    mvprintw(3 + i, 4, "%s", menu[i].c_str());

                    if (i == selected) {
                        attroff(A_REVERSE);
                    }
                }

                refresh();

                int key = getch();

                if (key == KEY_UP) {
                    selected = (selected - 1 + n) % n;
                } else if (key == KEY_DOWN) {
                    selected = (selected + 1) % n;
                } else if (key == '\n' || key == KEY_ENTER) {
                    break;
                }
            }
            return selected;
        }
        std::string input (int row, const char* label, bool doEcho = true) {
            char buffer[256];

            mvprintw(row, 4, "%s: ", label);
            refresh();
            curs_set(1);

            if (doEcho) {
                echo();
            }

            getnstr(buffer, sizeof(buffer) - 1);

            noecho();
            curs_set(0);

            return std::string(buffer);
        }

        std::string inputRequired (int row, const char* label, bool doEcho = true) {
            while (true) {
                move(row, 0);
                clrtoeol();
                std::string value = input(row, label, doEcho);

                if (!value.empty()) {
                    move(7, 0);
                    clrtoeol();
                    return value;
                }

                mvprintw(7, 4, "This field cannot be empty.");
            }
        }
};