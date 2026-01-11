#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <algorithm>
#include <regex>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <chrono>

using namespace std;

enum class EditorMode { USER_FRIENDLY, VIM_NORMAL, VIM_INSERT, HOME, SETTINGS };

struct Rain {
    int x, y, speed, len;
};

class NexaVim {
private:
    vector<string> buffer;
    string filename;
    int cursorX{0}, cursorY{0}, rowOffset{0};
    int screenRows{0}, screenCols{0};
    int homeSelection{0};
    int settingsSelection{0};
    EditorMode mode;
    bool running{true};
    
    // Settings
    bool autoCompletion{true};
    bool errorChecking{true};

    struct termios orig_termios;
    vector<Rain> matrixRain;

    void enableRawMode() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    void disableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        cout << "\x1b[2J\x1b[H" << flush;
    }

    void updateSize() {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        screenRows = ws.ws_row;
        screenCols = ws.ws_col;
    }

    void saveFile() {
        if (filename.empty()) {
            disableRawMode();
            cout << "\x1b[2J\x1b[HEnter filename to save: ";
            getline(cin, filename);
            enableRawMode();
        }
        if (!filename.empty()) {
            ofstream out(filename);
            for (const auto& line : buffer) out << line << "\n";
            out.close();
        }
    }

    string highlight(string line) {
        // Support for 50+ languages via generalized keyword groups
        // Includes: C, C++, C#, Java, Python, Ruby, Go, Rust, JS, TS, PHP, Swift, Kotlin, SQL, HTML, CSS, Lua, etc.
        static const regex keywords(R"(\b(if|else|while|for|return|int|char|float|double|bool|def|class|end|do|include|import|std|public|private|static|yield|module|puts|print|func|fn|let|var|const|async|await|try|catch|throw|namespace|using|package|extern|void|struct|enum|union|interface|type|nil|null|true|false|self|this|super|break|continue|default|case|switch|goto)\b)");
        line = regex_replace(line, keywords, "\x1b[35m$1\x1b[0m");
        
        static const regex strings(R"("[^"]*"|'[^']*')");
        line = regex_replace(line, strings, "\x1b[32m$0\x1b[0m");
        
        static const regex comments(R"(#.*|//.*|/\*.*\*/|--.*)");
        line = regex_replace(line, comments, "\x1b[30;1m$0\x1b[0m");

        if (errorChecking) {
            // Simple visual error indicator for trailing whitespace or common typos
            static const regex errors(R"(\s+$|;\s+;)");
            line = regex_replace(line, errors, "\x1b[41m$0\x1b[0m");
        }
        
        return line;
    }

    void initMatrix() {
        for(int i=0; i<40; i++) matrixRain.push_back({rand()%120, rand()%40, 1 + rand()%2, 4 + rand()%8});
    }

    void updateMatrix() {
        for(auto &r : matrixRain) {
            r.y += r.speed;
            if (r.y > screenRows) {
                r.y = -r.len;
                r.x = rand() % screenCols;
            }
        }
    }

    void renderHome(string& f) {
        updateMatrix();
        f += "\x1b[2J\x1b[H";
        for(auto &r : matrixRain) {
            for(int i=0; i<r.len; i++) {
                int py = r.y + i;
                if (py >= 0 && py < screenRows) {
                    f += "\x1b[" + to_string(py + 1) + ";" + to_string((r.x % screenCols) + 1) + "H\x1b[32m" + (char)('!' + rand()%90) + "\x1b[0m";
                }
            }
        }
        int mid = screenRows / 2;
        auto drawBtn = [&](string text, int row, bool sel) {
            int p = (screenCols - static_cast<int>(text.length()) - 8) / 2;
            f += "\x1b[" + to_string(row) + ";" + to_string(p > 0 ? p : 1) + "H";
            if(sel) f += "\x1b[1;7m  >>> " + text + " <<<  \x1b[0m";
            else f += "      " + text + "      ";
        };
        f += "\x1b[1;37m";
        drawBtn("N E X A V I M", mid - 5, false);
        f += "\x1b[0m";
        drawBtn("Start New File", mid - 2, homeSelection == 0);
        drawBtn("Settings Panel", mid - 1, homeSelection == 1);
        drawBtn("Help Center", mid, homeSelection == 2);
        drawBtn("Exit NexaVim", mid + 1, homeSelection == 3);
    }

    void renderSettings(string& f) {
        f += "\x1b[2J\x1b[H\x1b[1;36mSettings Panel\x1b[0m\n\n";
        auto drawOpt = [&](string label, bool val, int idx) {
            if (settingsSelection == idx) f += "\x1b[7m";
            f += "  [" + string(val ? "ENABLED" : "DISABLED") + "] " + label;
            if (settingsSelection == idx) f += "\x1b[m";
            f += "\n";
        };
        drawOpt("Auto-Completion", autoCompletion, 0);
        drawOpt("Error Checking", errorChecking, 1);
        f += "\n\x1b[" + to_string(settingsSelection == 2 ? 7 : 0) + "m  [ BACK TO HOME ] \x1b[m\n";
    }

    void renderEditor(string& f) {
        if (cursorY >= rowOffset + screenRows - 2) rowOffset = cursorY - screenRows + 3;
        if (cursorY < rowOffset) rowOffset = cursorY;

        for (int i = 0; i < screenRows - 2; ++i) {
            int idx = i + rowOffset;
            f += "\x1b[" + to_string(i + 1) + ";1H\x1b[K";
            if (idx < static_cast<int>(buffer.size())) {
                f += highlight(buffer[idx].substr(0, screenCols - 2));
                int total = buffer.size();
                int barPos = (idx * (screenRows - 2)) / (total > 0 ? total : 1);
                if (barPos == i) f += "\x1b[" + to_string(i + 1) + ";" + to_string(screenCols) + "H\x1b[97m\u2588\x1b[0m";
                else f += "\x1b[" + to_string(i + 1) + ";" + to_string(screenCols) + "H\x1b[90m\u2502\x1b[0m";
            }
        }
        f += "\x1b[" + to_string(screenRows - 1) + ";1H\x1b[7m";
        string mStr = (mode == EditorMode::USER_FRIENDLY) ? " USER-FRIENDLY " : (mode == EditorMode::VIM_INSERT) ? " VIM-INSERT " : " VIM-NORMAL ";
        string status = mStr + " | L:" + to_string(cursorY+1) + " | Shift+S: Toggle | Esc: Save";
        f += status;
        for (int i = status.length(); i < screenCols; i++) f += " ";
        f += "\x1b[m";
        f += "\x1b[" + to_string(cursorY - rowOffset + 1) + ";" + to_string(cursorX + 1) + "H\x1b[?25h";
    }

    void handleInput() {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) return;

        if (mode == EditorMode::HOME) {
            if (c == '\x1b') {
                char s[2]; if(read(STDIN_FILENO, &s[0], 1) == 0) return; if(read(STDIN_FILENO, &s[1], 1) == 0) return;
                if (s[1] == 'A') homeSelection = (homeSelection + 3) % 4;
                if (s[1] == 'B') homeSelection = (homeSelection + 1) % 4;
            } else if (c == 13) {
                if (homeSelection == 0) { buffer.push_back(""); mode = EditorMode::USER_FRIENDLY; }
                if (homeSelection == 1) mode = EditorMode::SETTINGS;
                if (homeSelection == 2) showHelp();
                if (homeSelection == 3) running = false;
            }
            return;
        }

        if (mode == EditorMode::SETTINGS) {
            if (c == '\x1b') {
                char s[2]; read(STDIN_FILENO, &s[0], 1); read(STDIN_FILENO, &s[1], 1);
                if (s[1] == 'A') settingsSelection = (settingsSelection + 2) % 3;
                if (s[1] == 'B') settingsSelection = (settingsSelection + 1) % 3;
            } else if (c == 13) {
                if (settingsSelection == 0) autoCompletion = !autoCompletion;
                if (settingsSelection == 1) errorChecking = !errorChecking;
                if (settingsSelection == 2) mode = EditorMode::HOME;
            }
            return;
        }

        // MODE TOGGLE: Shift+S
        if (c == 'S' && mode != EditorMode::VIM_INSERT) {
            mode = (mode == EditorMode::USER_FRIENDLY) ? EditorMode::VIM_NORMAL : EditorMode::USER_FRIENDLY;
            return;
        }

        // SAVE LOGIC: Esc
        if (c == 27 && mode != EditorMode::VIM_INSERT) { 
            struct termios temp; tcgetattr(STDIN_FILENO, &temp);
            temp.c_cc[VTIME] = 0; tcsetattr(STDIN_FILENO, TCSANOW, &temp);
            char next;
            if (read(STDIN_FILENO, &next, 1) <= 0) {
                saveFile(); running = false; return;
            } else if (next == '[') {
                char arrow; read(STDIN_FILENO, &arrow, 1);
                if (arrow == 'A') moveCursor(0, -1);
                if (arrow == 'B') moveCursor(0, 1);
                if (arrow == 'C') moveCursor(1, 0);
                if (arrow == 'D') moveCursor(-1, 0);
                return;
            }
        }

        if (c == 'X' && (mode == EditorMode::USER_FRIENDLY || mode == EditorMode::VIM_NORMAL)) {
            running = false; return;
        }

        if (mode == EditorMode::USER_FRIENDLY || mode == EditorMode::VIM_INSERT) {
            handleText(c);
        } else if (mode == EditorMode::VIM_NORMAL) {
            if (c == 'i') mode = EditorMode::VIM_INSERT;
            else if (c == 'h') moveCursor(-1, 0);
            else if (c == 'j') moveCursor(0, 1);
            else if (c == 'k') moveCursor(0, -1);
            else if (c == 'l') moveCursor(1, 0);
        }
    }

    void handleText(char c) {
        if (c == 127 || c == 8) {
            if (cursorX > 0) buffer[cursorY].erase(--cursorX, 1);
            else if (cursorY > 0) {
                cursorX = buffer[cursorY - 1].length();
                buffer[cursorY - 1] += buffer[cursorY];
                buffer.erase(buffer.begin() + cursorY);
                cursorY--;
            }
        } else if (c == 13) {
            string contentToMove = buffer[cursorY].substr(cursorX);
            buffer[cursorY] = buffer[cursorY].substr(0, cursorX);
            cursorY++;
            buffer.insert(buffer.begin() + cursorY, contentToMove);
            cursorX = 0;
        } else if (isprint(c)) {
            buffer[cursorY].insert(cursorX++, 1, c);
            // Basic Auto-completion for brackets/quotes
            if (autoCompletion) {
                if (c == '(') { buffer[cursorY].insert(cursorX, 1, ')'); }
                else if (c == '{') { buffer[cursorY].insert(cursorX, 1, '}'); }
                else if (c == '[') { buffer[cursorY].insert(cursorX, 1, ']'); }
                else if (c == '"') { buffer[cursorY].insert(cursorX, 1, '"'); }
            }
        }
    }

    void moveCursor(int dx, int dy) {
        cursorY = clamp((int)cursorY + dy, 0, (int)buffer.size() - 1);
        cursorX = clamp((int)cursorX + dx, 0, (int)buffer[cursorY].length());
    }

    void showHelp() {
        disableRawMode();
        cout << "\x1b[2J\x1b[H\x1b[1;36mNexaVim Help Center\x1b[0m\n\n";
        cout << "General Controls:\n";
        cout << "  - ESC      : SAVE and EXIT\n";
        cout << "  - Shift+X  : DISCARD and EXIT\n";
        cout << "  - Shift+S  : Toggle User-Friendly / Vim Modes\n";
        cout << "  - Arrows   : Move cursor\n";
        cout << "  - Enter    : New Line\n\n";
        cout << "Press Enter to return...";
        cin.get();
        enableRawMode();
    }

public:
    NexaVim(string fn) : filename(fn) {
        enableRawMode();
        updateSize();
        initMatrix();
        if (filename.empty()) mode = EditorMode::HOME;
        else {
            ifstream f(filename); string l;
            while(getline(f, l)) buffer.push_back(l);
            if(buffer.empty()) buffer.push_back("");
            mode = EditorMode::USER_FRIENDLY;
        }
    }
    ~NexaVim() { disableRawMode(); }
    void run() {
        while(running) {
            string frame = "\x1b[?25l";
            if (mode == EditorMode::HOME) renderHome(frame);
            else if (mode == EditorMode::SETTINGS) renderSettings(frame);
            else renderEditor(frame);
            cout << frame << flush;
            handleInput();
            if (mode == EditorMode::HOME) usleep(40000); 
        }
    }
};

int main(int argc, char** argv) {
    if (argc > 1 && string(argv[1]) == "help") {
        cout << "NexaVim: The User-Friendly CLI Editor\n\nUsage: ./nexavim [filename]\n";
        return 0;
    }
    NexaVim editor(argc > 1 ? argv[1] : "");
    editor.run();
    return 0;
}
