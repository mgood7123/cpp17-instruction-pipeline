// aim: provide an interface to output text in a similar way to gradle:
//
// line ORIGIN is terminal line 50
//
// (50) line ORIGIN + 0 INFO A
// (51) line ORIGIN + 1 STATUS A
// 
// some time later
// 
// (50) line ORIGIN + 0 INFO A
// (51) line ORIGIN + 1 INFO B
// (52) line ORIGIN + 2 STATUS A
// (53) line ORIGIN + 3 STATUS B
// (54) line ORIGIN + 4 STATUS C
//
// IMPORTANT: a terminal's lines go from top to bottom
// IMPORTANT: such that
// IMPORTANT: line 0: this is line A <new line feed>
// IMPORTANT: line 1: this is a new line, the terminal may have scrolled down such
// IMPORTANT:         that this appears as if this is still line 0
// IMPORTANT:         depending on the terminal height
// IMPORTANT:         this gives the illusion that the previous line 0 is now line -1
// IMPORTANT:         when this is not the case
// 
// POSSIBLE SIMULATION:
//
// <character stream is fed, column is incremented>
// (50) line ORIGIN + 0 INFO A
// <new line feed, line is incremented, and column is reset>
//
// <character stream is fed, column is incremented>
// (51) line ORIGIN + 1 STATUS A
// <new line feed, line is incremented, and column is reset>
// 
// some time later
//
// the order in which this would actually happen is user defined
// 
// (50) line ORIGIN + 0 INFO A
//
// OLD (51) line ORIGIN + 1 STATUS A
//
// <current line is line 52, nothing has yet been appended>
// <save current line, save current column>
//
// <current line is set to line 51, current column is reset>
// <lines from line current to saved line are moved down by a result of saved line minus current line>
// <a new line feed is appended>
// <line 52 is moved to line 53>
// <line 51 is moved to line 52>
// <appending new line feeds as neccesary>
//
// OLD (51) line ORIGIN + 1 STATUS A
// NEW (52) line ORIGIN + 2 STATUS A

// <character stream is fed, column is incremented>
// (51) line ORIGIN + 1 INFO B
// (52) line ORIGIN + 2 STATUS A
//
// <saved line is line 52, saved column is 1>
// <line is reset from saved state, column is reset from saved state>
// <adjust to account for insertion above, new line is 53, column is unchanged>

// <character stream is fed, column is incremented>
// (53) line ORIGIN + 3 STATUS B
// <new line feed, line is incremented, and column is reset>
// <character stream is fed, column is incremented>
// (54) line ORIGIN + 4 STATUS C
// <new line feed, line is incremented, and column is reset>

#include <cstdio>
#include <termios.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>

#include <deque>
#include <vector>
#include <algorithm>

namespace std {

    template<class InputIt, class T>
    constexpr InputIt findConstExpr(InputIt first, InputIt last, const T& value)
    {
        for (; first != last; ++first) {
            if (*first == value) {
                return first;
            }
        }
        return last;
    }
    
    template< class T, class Alloc, class U >
    constexpr typename std::vector<T,Alloc>::size_type
    erase(std::vector<T,Alloc>& c, const U& value) {
        auto it = std::remove(c.begin(), c.end(), value);
        auto r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }

    template< class T, class Alloc, class Pred >
    constexpr typename std::deque<T,Alloc>::size_type
    erase_if(std::deque<T,Alloc>& c, Pred pred) {
        auto it = std::remove_if(c.begin(), c.end(), pred);
        auto r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }
    template< class T, class Alloc, class U >
    constexpr typename std::deque<T,Alloc>::size_type
    erase(std::deque<T,Alloc>& c, const U& value) {
        auto it = std::remove(c.begin(), c.end(), value);
        auto r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }
    
    template< class T, class Alloc, class Pred >
    constexpr typename std::vector<T,Alloc>::size_type
    erase_if(std::vector<T,Alloc>& c, Pred pred) {
        auto it = std::remove_if(c.begin(), c.end(), pred);
        auto r = std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }
}

typedef unsigned long long EditorPositionType;

struct Editor {
    EditorPositionType line = 1;
    EditorPositionType column = 1;
    
    EditorPositionType terminal_line;
    EditorPositionType terminal_column;

    bool new_line_invokes_carrage_return = true;
    
    void store_current_position(EditorPositionType & line, EditorPositionType & column) {
        char buffer[4096];
        memset(buffer, 0, 4096);
        
        struct termios term;
        tcgetattr(1, &term);
        tcflag_t old = term.c_lflag;
        term.c_lflag &= ~ICANON & ~ECHO;
        tcsetattr(1, TCSANOW, &term);
        printf("\033[6n");
        fflush(stdout);
        
        getchar(); // dont need "\033"
        getchar(); // dont need "["
        
        scanf("%[^;]s", buffer);
        line = atoll(buffer);
        memset(buffer, 0, 4096);
        
        getchar(); // dont need ";"
        
        scanf("%[^R]s",buffer);
        column = atoll(buffer);
        memset(buffer, 0, 4096);
        
        getchar(); // dont need R
        
        term.c_lflag = old;
        tcsetattr(1, TCSANOW, &term);
    }
    
    static std::deque<Editor*> editors;
    
    std::vector<std::vector<char>> textMap;
    
    EditorPositionType end_line;
    EditorPositionType end_column;
    
    Editor() {
        editors.push_back(this);
        store_current_position(terminal_line, terminal_column);
        end_line = terminal_line;
        end_column = terminal_column;
        textMap.resize(line);
        textMap[line-1].resize(terminal_column);
        memset(textMap[line-1].data(), '\0', terminal_column);
        printf("\\033[%llu;%lluH\n", terminal_line, terminal_column);
    }
    
    void update() {
        for (int i = 0; i != textMap.size(); i++) {
            move((end_line+1)-(end_line-terminal_line), 1);
            end_line++;
            const char * data = textMap[i].data();
            if (data[0] == '(') return;
            printf("%s", data);
        }
        printf("\033[r\n");
        printf("\033[M\n");
        printf("\033[M\n");
        printf("\033[M\n");
        printf("\033[M\n");
        printf("SCROLL \\033[%llu;%lluH\n", terminal_line, terminal_column);
        printf("SCROLL \\033[%llu;%lluH\n", (end_line+1)-(end_line-terminal_line), terminal_column);
    }
    
    ~Editor() {
        move(end_line, end_column);
        std::erase(editors, this);
    }
    
    void reset_line(EditorPositionType & line) {
        line = 1;
    };
    
    void reset_column(EditorPositionType & column) {
        column = 1;
    };
    
    void move(const EditorPositionType & line, const EditorPositionType & column) {
        printf("\033[%llu;%lluH", line, column);
    }
    
    
    void write(const char & text) {
        switch(text) {
            case '\r':
                reset_column(column);
                break;
            case '\n':
                // new lines are generally included
                textMap[line-1][column-1] = text;
                line++;
                textMap.resize(line);
                textMap[line-1].resize(terminal_column);
                memset(textMap[line-1].data(), '\0', terminal_column);
                if (new_line_invokes_carrage_return) reset_column(column);
                break;
            default:
                textMap[line-1][column-1] = text;
                column++;
                break;
        }
    }
    
    void write(const char * text) {
        for (int i = 0; text[i] != '\0'; i++) write(text[i]);
    }
};

std::deque<Editor*> Editor::editors = std::deque<Editor*>();

int main() {
    Editor editor2 = Editor();
    editor2.write("editor 2 A\n");
    editor2.update();
}
