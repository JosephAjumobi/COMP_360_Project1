#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

/*
 * COMP 360-01 - Programming Languages
 * Project 1: LearnCompiler
 *
 * Implements:
 *  1. Lexical analysis
 *  2. Lexeme/token display
 *  3. Recursive-descent parsing
 *  4. First syntax-error reporting
 */

enum class TokenType {
    KEYWORD,
    IDENT,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    SEMICOLON,
    ASSIGN,
    MULT,
    DIV,
    END,
    INVALID
};

struct Token {
    TokenType type;
    string lexeme;
    int line;
};

// Converts token types into readable names.
string tokenName(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD:
            return "KEYWORD";
        case TokenType::IDENT:
            return "IDENT";
        case TokenType::LPAREN:
            return "LPAREN";
        case TokenType::RPAREN:
            return "RPAREN";
        case TokenType::LBRACE:
            return "LBRACE";
        case TokenType::RBRACE:
            return "RBRACE";
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::ASSIGN:
            return "ASSIGN";
        case TokenType::MULT:
            return "MULT";
        case TokenType::DIV:
            return "DIV";
        case TokenType::END:
            return "EOF";
        default:
            return "INVALID";
    }
}


/*
 * Lexical Analyzer
 *
 * Reads the source program and separates it into
 * lexemes and token types.
 */
class Lexer {
private:
    string source;
    size_t position;
    int line;

public:
    Lexer(string input)
        : source(input), position(0), line(1) {}

    vector<Token> scan() {
        vector<Token> tokens;

        while (position < source.length()) {

            char current = source[position];

            // Ignore whitespace.
            if (isspace(static_cast<unsigned char>(current))) {

                if (current == '\n') {
                    line++;
                }

                position++;
                continue;
            }

            /*
             * Identifiers and keyword.
             *
             * According to the supplied grammar, identifiers
             * consist of letters a-z.
             */
            if (isalpha(static_cast<unsigned char>(current))) {

                size_t start = position;

                position++;

                while (position < source.length() &&
                       isalpha(static_cast<unsigned char>(
                           source[position]))) {

                    position++;
                }

                string lexeme =
                    source.substr(start, position - start);

                if (lexeme == "float") {
                    tokens.push_back({
                        TokenType::KEYWORD,
                        lexeme,
                        line
                    });
                }
                else {
                    tokens.push_back({
                        TokenType::IDENT,
                        lexeme,
                        line
                    });
                }

                continue;
            }

            // Single-character tokens.
            TokenType type = TokenType::INVALID;

            switch (current) {

                case '(':
                    type = TokenType::LPAREN;
                    break;

                case ')':
                    type = TokenType::RPAREN;
                    break;

                case '{':
                    type = TokenType::LBRACE;
                    break;

                case '}':
                    type = TokenType::RBRACE;
                    break;

                case ';':
                    type = TokenType::SEMICOLON;
                    break;

                case '=':
                    type = TokenType::ASSIGN;
                    break;

                case '*':
                    type = TokenType::MULT;
                    break;

                case '/':
                    type = TokenType::DIV;
                    break;

                default:
                    type = TokenType::INVALID;
            }

            tokens.push_back({
                type,
                string(1, current),
                line
            });

            position++;
        }

        tokens.push_back({
            TokenType::END,
            "EOF",
            line
        });

        return tokens;
    }
};


/*
 * Recursive-Descent Parser
 *
 * Grammar implemented:
 *
 * <program> ->
 *      <keyword> <ident>
 *      ( <keyword> <ident> )
 *      {
 *          <declares>
 *          <assign>
 *      }
 *
 * <declares> ->
 *      <keyword> <ident> ;
 *      | <keyword> <ident> ; <declares>
 *
 * <assign> ->
 *      <ident> = <expr> ;
 *
 * <expr> ->
 *      <ident> {*|/} <expr>
 *      | <ident>
 *
 * <keyword> -> float
 *
 * <ident> -> sequence of letters
 */
class Parser {
private:
    const vector<Token>& tokens;
    size_t position;
    string errorMessage;

    const Token& current() const {
        return tokens[position];
    }

    bool accept(TokenType type) {

        if (current().type == type) {
            position++;
            return true;
        }

        return false;
    }

    bool expect(TokenType type, const string& expected) {

        if (accept(type)) {
            return true;
        }

        ostringstream message;

        message
            << "Syntax error on line "
            << current().line
            << ": expected "
            << expected
            << " but found '"
            << current().lexeme
            << "'.";

        errorMessage = message.str();

        return false;
    }

    // <keyword> -> float
    bool keyword() {
        return expect(
            TokenType::KEYWORD,
            "keyword 'float'"
        );
    }

    // <ident>
    bool ident() {
        return expect(
            TokenType::IDENT,
            "identifier"
        );
    }

    /*
     * <declares> ->
     *      <keyword> <ident> ;
     *      | <keyword> <ident> ; <declares>
     */
    bool declares() {

        if (!keyword()) {
            return false;
        }

        if (!ident()) {
            return false;
        }

        if (!expect(
                TokenType::SEMICOLON,
                "';'")) {

            return false;
        }

        // Additional declarations.
        while (current().type ==
               TokenType::KEYWORD) {

            if (!keyword()) {
                return false;
            }

            if (!ident()) {
                return false;
            }

            if (!expect(
                    TokenType::SEMICOLON,
                    "';'")) {

                return false;
            }
        }

        return true;
    }

    /*
     * <expr> ->
     *      <ident> {*|/} <expr>
     *      | <ident>
     */
    bool expr() {

        if (!ident()) {
            return false;
        }

        if (current().type == TokenType::MULT ||
            current().type == TokenType::DIV) {

            position++;

            return expr();
        }

        return true;
    }

    /*
     * <assign> ->
     *      <ident> = <expr> ;
     */
    bool assign() {

        if (!ident()) {
            return false;
        }

        if (!expect(
                TokenType::ASSIGN,
                "'='")) {

            return false;
        }

        if (!expr()) {
            return false;
        }

        if (!expect(
                TokenType::SEMICOLON,
                "';'")) {

            return false;
        }

        return true;
    }

public:
    Parser(const vector<Token>& inputTokens)
        : tokens(inputTokens), position(0) {}

    /*
     * <program>
     */
    bool parse() {

        // First detect invalid lexical tokens.
        for (const Token& token : tokens) {

            if (token.type == TokenType::INVALID) {

                ostringstream message;

                message
                    << "Lexical/syntax error on line "
                    << token.line
                    << ": invalid token '"
                    << token.lexeme
                    << "'.";

                errorMessage = message.str();

                return false;
            }
        }

        // Return type.
        if (!keyword()) {
            return false;
        }

        // Function name.
        if (!ident()) {
            return false;
        }

        if (!expect(
                TokenType::LPAREN,
                "'('")) {

            return false;
        }

        // Parameter type.
        if (!keyword()) {
            return false;
        }

        // Parameter name.
        if (!ident()) {
            return false;
        }

        if (!expect(
                TokenType::RPAREN,
                "')'")) {

            return false;
        }

        if (!expect(
                TokenType::LBRACE,
                "'{'")) {

            return false;
        }

        /*
         * The supplied grammar requires
         * at least one declaration.
         */
        if (current().type !=
            TokenType::KEYWORD) {

            errorMessage =
                "Syntax error on line " +
                to_string(current().line) +
                ": expected at least one "
                "declaration beginning with 'float'.";

            return false;
        }

        if (!declares()) {
            return false;
        }

        if (!assign()) {
            return false;
        }

        if (!expect(
                TokenType::RBRACE,
                "'}'")) {

            return false;
        }

        if (!expect(
                TokenType::END,
                "end of file")) {

            return false;
        }

        return true;
    }

    string getError() const {
        return errorMessage;
    }
};


int main(int argc, char* argv[]) {

    if (argc != 2) {

        cerr
            << "Usage: "
            << argv[0]
            << " <sample-program-file>"
            << endl;

        return 1;
    }

    ifstream inputFile(argv[1]);

    if (!inputFile) {

        cerr
            << "Error: could not open "
            << argv[1]
            << endl;

        return 1;
    }

    // Read entire input file.
    stringstream buffer;
    buffer << inputFile.rdbuf();

    string source = buffer.str();

    // ----------------------------
    // Lexical Analysis
    // ----------------------------

    Lexer lexer(source);

    vector<Token> tokens =
        lexer.scan();

    cout << "Lexemes and Tokens" << endl;
    cout << "------------------" << endl;

    for (const Token& token : tokens) {

        if (token.type != TokenType::END) {

            cout
                << token.lexeme
                << "\t"
                << tokenName(token.type)
                << endl;
        }
    }

    // ----------------------------
    // Syntax Analysis
    // ----------------------------

    cout << endl;
    cout << "Syntax Analysis" << endl;
    cout << "---------------" << endl;

    Parser parser(tokens);

    if (parser.parse()) {

        cout
            << "The Sample Program is generated "
            << "by the BNF grammar"
            << endl;
    }
    else {

        cout
            << "The Sample Program cannot be "
            << "generated by the LearnCompiler "
            << "BNF Grammar"
            << endl;

        cout
            << parser.getError()
            << endl;
    }

    return 0;
}