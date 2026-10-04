#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

//  g++ -std=c++17 -o learncompiler learncompiler.cpp
// ./learncompiler sample1.txt
// ./learncompiler sample2.txt



// Grammar as written has no ';' after <assign>, but Sample 1 has one.
// true = accept an optional ';' after the assignment.
const bool ALLOW_ASSIGN_SEMICOLON = true;

enum class Tok { KEYWORD, IDENT, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, ASSIGN, MULT, DIV, END, UNKNOWN };

// Turn a token type into text so we can print it

string tokName(Tok t) {
    switch (t) {
        case Tok::KEYWORD: return "KEYWORD";
        case Tok::IDENT:   return "IDENT";
        case Tok::LPAREN:  return "LPAREN";
        case Tok::RPAREN:  return "RPAREN";
        case Tok::LBRACE:  return "LBRACE";
        case Tok::RBRACE:  return "RBRACE";
        case Tok::SEMI:    return "SEMI";
        case Tok::ASSIGN:  return "ASSIGN";
        case Tok::MULT:    return "MULT";
        case Tok::DIV:     return "DIV";
        case Tok::END:     return "EOF";
        default:           return "UNKNOWN";
    }
}

// One token: its type, its text, and its line number

struct Token {
    Tok type;
    string lexeme;
    int line;
};

// Thrown by the parser when it finds a syntax error
struct SyntaxError : runtime_error {
    explicit SyntaxError(const string& m) : runtime_error(m) {}
};

// ---------- Lexical analyzer ----------

// True if the word is only lowercase letters (a valid <ident>)
bool allLower(const string& s) {
    for (char c : s) if (c < 'a' || c > 'z') return false;
    return !s.empty();
}

// Split the source text into tokens
vector<Token> lex(const string& src) {
    vector<Token> out;
    size_t i = 0, n = src.size();
    int line = 1;
    while (i < n) {
        char c = src[i];
        if (c == '\n') { line++; i++; continue; }
        if (isspace((unsigned char)c)) { i++; continue; }
        if (isalnum((unsigned char)c)) {
            size_t j = i;
            while (j < n && isalnum((unsigned char)src[j])) j++;
            string w = src.substr(i, j - i);
            Tok t;
            if (w == "float") t = Tok::KEYWORD;
            else if (allLower(w)) t = Tok::IDENT;
            else t = Tok::UNKNOWN;  // digits or uppercase are not in <ident>
            out.push_back({t, w, line});
            i = j;
            continue;
        }

        
        // A single symbol
        Tok t;
        switch (c) {
            case '(': t = Tok::LPAREN; break;
            case ')': t = Tok::RPAREN; break;
            case '{': t = Tok::LBRACE; break;
            case '}': t = Tok::RBRACE; break;
            case ';': t = Tok::SEMI;   break;
            case '=': t = Tok::ASSIGN; break;
            case '*': t = Tok::MULT;   break;
            case '/': t = Tok::DIV;    break;
            default:  t = Tok::UNKNOWN;
        }
        out.push_back({t, string(1, c), line});
        i++;
    }
    out.push_back({Tok::END, "EOF", line});
    return out;
}


// ---------- Recursive-descent parser ----------

// One function per grammar rule.
class Parser {
    const vector<Token>& toks;
    size_t pos = 0;
    
    // The token we are looking at right now
    const Token& cur() const { return toks[pos]; }

    // Check the current token is the type we need.
    // If yes, move to the next token. If no, report the error.

    void expect(Tok t, const string& what) {
        const Token& c = cur();
        if (c.type != t)
            throw SyntaxError("line " + to_string(c.line) + ": expected " + what +
                              " but found '" + c.lexeme + "' (" + tokName(c.type) + ")");
        pos++;
    }

    // <declares> -> <keyword> <ident> ; | <keyword> <ident> ; <declares>
    void declares() {
        expect(Tok::KEYWORD, "declaration starting with 'float'");
        expect(Tok::IDENT, "identifier");
        expect(Tok::SEMI, "';'");
        if (cur().type == Tok::KEYWORD) declares();
    }

    // <expr> -> <ident> {*|/} <expr> | <ident>
    void expr() {
        expect(Tok::IDENT, "identifier");
        if (cur().type == Tok::MULT || cur().type == Tok::DIV) {
            pos++;
            expr();
        }
    }

    // <assign> -> <ident> = <expr>
    void assign() {
        expect(Tok::IDENT, "identifier");
        expect(Tok::ASSIGN, "'='");
        expr();
        if (ALLOW_ASSIGN_SEMICOLON && cur().type == Tok::SEMI) pos++;
    }

public:
    explicit Parser(const vector<Token>& t) : toks(t) {}

    // <program> -> <keyword> <ident> ( <keyword> <ident> ) { <declares> <assign> }
    void program() {
        expect(Tok::KEYWORD, "keyword 'float'");
        expect(Tok::IDENT, "identifier");
        expect(Tok::LPAREN, "'('");
        expect(Tok::KEYWORD, "keyword 'float'");
        expect(Tok::IDENT, "identifier");
        expect(Tok::RPAREN, "')'");
        expect(Tok::LBRACE, "'{'");
        declares();
        assign();
        expect(Tok::RBRACE, "'}'");
        expect(Tok::END, "end of input");
    }
};

int main(int argc, char* argv[]) {
    
    // Step 1: read one sample program from the file given
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <sample-program-file>\n";
        return 1;
    }
    ifstream in(argv[1]);
    if (!in) {
        cout << "Cannot open file: " << argv[1] << "\n";
        return 1;
    }
    stringstream buf;
    buf << in.rdbuf();
    string src = buf.str();

    cout << "=== Source ===\n" << src << "\n";

    // Step 2: run the lexer and display lexemes with token types
    vector<Token> toks = lex(src);
    cout << "=== Lexemes and tokens ===\n";
    for (const Token& t : toks)
        if (t.type != Tok::END)
            cout << left << setw(9) << t.lexeme << tokName(t.type) << "\n";

    // Steps 3 and 4: run the parser and print the result
    cout << "\n=== Parse result ===\n";
    try {
        Parser(toks).program();
        cout << "The Sample Program is generated by the BNF grammar\n";
    } catch (const SyntaxError& e) {
        cout << "The Sample Program cannot be generated by the LearnCompiler BNF Grammar\n";
        cout << "First syntax error: " << e.what() << "\n";
    }
    return 0;
}
