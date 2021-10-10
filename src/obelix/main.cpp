#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <lexer/Lexer.h>
#include <lexer/StringBuffer.h>
#include <lexer/Token.h>
#include <obelix/Syntax.h>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Obelix {

class FileBuffer {
public:
    explicit FileBuffer(char const* file_name)
        : m_buffer(std::make_unique<StringBuffer>())
        , m_file_name(file_name)
    {
        int fh = open(file_name, O_RDONLY);
        if (fh < 0) {
            perror("Open");
            return;
        }
        struct stat sb { };
        if (auto rc = fstat(fh, &sb); rc < 0) {
            perror("Stat");
            close(fh);
            return;
        }

        auto size = sb.st_size;
        auto buf = new char[size + 1];
        if (auto rc = ::read(fh, (void*) buf, size); rc < size) {
            perror("read");
            size = 0;
        } else {
            buf[size] = '\0';
            m_buffer->assign(buf);
        }
        close(fh);
        delete[] buf;
    }

    explicit FileBuffer(std::string const& file_name)
        : FileBuffer(file_name.c_str())
    {
    }

    StringBuffer& buffer()
    {
        return *m_buffer;
    }

private:
    std::string m_file_name;
    std::unique_ptr<StringBuffer> m_buffer;
};

class Parser {
public:
    constexpr static TokenCode KeywordVar = ((TokenCode)100);
    constexpr static TokenCode KeywordFunc = ((TokenCode)101);
    constexpr static TokenCode KeywordIf = ((TokenCode)102);
    constexpr static TokenCode KeywordElse = ((TokenCode)103);
    constexpr static TokenCode KeywordWhile = ((TokenCode)104);
    constexpr static TokenCode KeywordTrue = ((TokenCode)105);
    constexpr static TokenCode KeywordFalse = ((TokenCode)106);

    explicit Parser(std::string const& file_name)
        : m_file_name(file_name)
        , m_file_buffer(file_name)
        , m_lexer(m_file_buffer.buffer())
    {
        m_lexer.add_scanner<QStringScanner>();
        m_lexer.add_scanner<IdentifierScanner>();
        m_lexer.add_scanner<NumberScanner>();
        m_lexer.add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
        m_lexer.add_scanner<KeywordScanner>(7,
            "var",
            "func",
            "if",
            "else",
            "while",
            "true",
            "false");
    }

    std::shared_ptr<Module> parse()
    {
        std::shared_ptr<Module> module = std::make_shared<Module>(m_file_name);
        parse_statements(std::dynamic_pointer_cast<Block>(module), true);
        return module;
    }

private:
    void parse_statements(std::shared_ptr<Block> const& block, bool to_eof = false)
    {
        auto done = [to_eof](Token const& token) {
            if (to_eof)
                return token.code() == TokenCode::EndOfFile;
            else
                return token.code() == TokenCode::CloseBrace;
        };

        for (auto token = m_lexer.lex(); !done(token); token = m_lexer.lex()) {
            switch (token.code()) {
            case TokenCode::Identifier:
                block->append(std::make_shared<ProcedureCall>(parse_function_call(token.value())));
                break;
            case KeywordIf:
                block->append(parse_if_statement());
                break;
            case KeywordVar:
                block->append(parse_variable_declaration());
                break;
                //            case KeywordFunc:
                //                ret->append(parse_function_declaration(token));
                //                break;
            default:
                break;
            }
//            if (!m_lexer.expect(TokenCode::SemiColon)) {
//                fprintf(stderr, "Syntax Error: Expected ';' after statement\n");
//                exit(1);
//            }
            fprintf(stdout, "\n");
        }
        printf("\n");
    }

    std::shared_ptr<FunctionCall> parse_function_call(std::string const& func_name)
    {
        if (!m_lexer.expect(TokenCode::OpenParen)) {
            fprintf(stderr, "Syntax Error: Expected '(' after function name\n");
            exit(1);
        }
        std::vector<std::shared_ptr<Expression>> args {};
        auto done = false;
        do {
            args.push_back(parse_expression());
            auto next = m_lexer.lex();
            switch (next.code()) {
            case TokenCode::Comma:
                break;
            case TokenCode::CloseParen:
                done = true;
                break;
            default:
                fprintf(stderr, "Syntax Errpr: Expected ',' or ')' in function argument list");
                exit(1);
            }
        } while (!done);
        return std::make_shared<FunctionCall>(func_name, args);
    }

    std::shared_ptr<IfStatement> parse_if_statement()
    {
        auto condition = parse_expression();
        if (!m_lexer.expect(TokenCode::OpenBrace)) {
            fprintf(stderr, "Syntax Error: Expected '{'");
            exit(1);
        }
        std::shared_ptr<Block> if_block = std::make_shared<Block>();
        std::shared_ptr<Block> else_block = nullptr;
        parse_statements(if_block);
        auto else_maybe = m_lexer.peek();
        if (else_maybe.code() == KeywordElse) {
            m_lexer.lex();
            if (!m_lexer.expect(TokenCode::OpenBrace)) {
                fprintf(stderr, "Syntax Error: Expected '{'");
                exit(1);
            }
            else_block = std::make_shared<Block>();
            parse_statements(else_block);
        }
        return std::make_shared<IfStatement>(condition, if_block, else_block);
    }

    std::shared_ptr<Assignment> parse_variable_declaration()
    {
        auto identifier_maybe = m_lexer.match(TokenCode::Identifier);
        if (!identifier_maybe.has_value()) {
            fprintf(stderr, "Syntax Error: expecting variable name after the 'var' keyword\n");
            exit(1);
        }
        auto identifier = identifier_maybe.value();
        if (!m_lexer.expect(TokenCode::Equals)) {
            fprintf(stderr, "Syntax Error: expecting '=' after variable name in assignment\n");
            exit(1);
        }
        auto expr = parse_expression();
        return std::make_shared<Assignment>(identifier.value(), expr);
    }

    /*
     *
     * Precedence climbing method (https://en.wikipedia.org/wiki/Operator-precedence_parser):
     *
     * parse_expression()
     *    return parse_expression_1(parse_primary(), 0)
     *
     * parse_expression_1(lhs, min_precedence)
     *    lookahead := peek next token
     *    while lookahead is a binary operator whose precedence is >= min_precedence
     *      *op := lookahead
     *      advance to next token
     *      rhs := parse_primary ()
     *      lookahead := peek next token
     *      while lookahead is a binary operator whose precedence is greater
     *              than op's, or a right-associative operator
     *              whose precedence is equal to op's
     *        rhs := parse_expression_1 (rhs, precedence of op + 1)
     *        lookahead := peek next token
     *      lhs := the result of applying op with operands lhs and rhs
     *    return lhs
     */
    std::shared_ptr<Expression> parse_expression()
    {
        return parse_expression_1(parse_primary_expression(), 0);
    }

    static int binary_precedence(Token token) {
        switch (token.code()) {
        case TokenCode::Plus:
        case TokenCode::Minus:
            return 1;
        case TokenCode::Asterisk:
        case TokenCode::Slash:
            return 2;
        default:
            return -1;
        }
    }

    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence)
    {
        auto next = m_lexer.peek();
        while (binary_precedence(next) >= min_precedence) {
            auto op = m_lexer.lex();
            auto rhs = parse_primary_expression();
            next = m_lexer.peek();
            while (binary_precedence(next) > binary_precedence(op)) {
                rhs = parse_expression_1(rhs, binary_precedence(op) + 1);
                next = m_lexer.peek();
            }
            lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
        }
        return lhs;
    }

    std::shared_ptr<Expression> parse_primary_expression()
    {
        auto t = m_lexer.lex();
        switch (t.code()) {
        case TokenCode::Plus:
        case TokenCode::Minus: {
            auto operand = parse_atom(m_lexer.lex());
            return std::make_shared<UnaryExpression>(t, operand);
        }
        default:
            return parse_atom(t);
        }
    }

    static std::shared_ptr<Expression> parse_atom(Token const& atom)
    {
        switch (atom.code()) {
        case TokenCode::Integer:
        case TokenCode::Float:
        case TokenCode::DoubleQuotedString:
        case TokenCode::SingleQuotedString:
        case KeywordTrue:
        case KeywordFalse:
            return std::make_shared<Literal>(atom);
            break;
        case TokenCode::Identifier:
            return std::make_shared<VariableReference>(atom.value());
            break;
        default:
            fprintf(stderr, "ERROR: Expected literal or variable, got '%s'\n", atom.to_string().c_str());
            exit(1);
        }
    }

    FileBuffer m_file_buffer;
    std::string m_file_name;
    Lexer m_lexer;
};

}

void usage()
{
    printf(
        "Obelix v2 - A programming language\n"
        "USAGE:\n"
        "    obelix [--debug] path/to/script.obl\n");
    exit(1);
}


int main(int argc, char** argv)
{
    std::string file_name;
    for (int ix = 1; ix < argc; ++ix) {
        if (!strcmp(argv[ix], "--help")) {
            usage();
        } else if (!strcmp(argv[ix], "--debug")) {
            Obelix::Logger::get_logger().enable("lexer");
        } else {
            file_name = argv[ix];
        }
    }

    if (file_name.empty()) {
        usage();
    }

    Obelix::Parser parser(file_name);
    auto tree = parser.parse();
    tree->dump();
    tree->execute();
    return 0;
}
