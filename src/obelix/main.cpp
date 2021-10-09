#include <ast/Syntax.h>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <lexer/Lexer.h>
#include <lexer/Token.h>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Obelix {

class FileBuffer {
public:
    explicit FileBuffer(char const* file_name)
        : m_file_name(file_name)
    {
        int fh = open(file_name, O_RDONLY);
        if (fh < 0) {
            perror("Open");
            return;
        }
        struct stat sb {
        };
        if (auto rc = fstat(fh, &sb); rc < 0) {
            perror("Stat");
            close(fh);
            return;
        }

        m_size = sb.st_size;
        m_buf = new char[m_size];
        if (auto rc = read(fh, (void*)m_buf, m_size); rc < m_size) {
            perror("read");
            m_size = 0;
            delete[] m_buf;
            m_buf = nullptr;
        }
        close(fh);
        m_lexer = Lexer(m_buf);
        m_lexer.add_scanner<QStringScanner>();
        m_lexer.add_scanner<IdentifierScanner>();
        m_lexer.add_scanner<NumberScanner>();
        m_lexer.add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
        m_lexer.add_scanner<KeywordScanner>(5,
            "var",
            "func",
            "if",
            "else",
            "while");

        m_tokens = m_lexer.tokenize();
    }

    constexpr static TokenCode KeywordVar = ((TokenCode)100);
    constexpr static TokenCode KeywordFunc = ((TokenCode)101);
    constexpr static TokenCode KeywordIf = ((TokenCode)102);
    constexpr static TokenCode KeywordElse = ((TokenCode)103);
    constexpr static TokenCode KeywordWhile = ((TokenCode)104);

    Token const& peek(size_t how_many = 0)
    {
        oassert(m_current + how_many < m_tokens.size(), "Token buffer underflow");
        return m_tokens[m_current + how_many];
    }

    Token const& lex()
    {
        auto const& ret = peek(0);
        if (m_current < (m_tokens.size() - 1))
            m_current++;
        fprintf(stdout, "%s ", ret.to_string().c_str());
        return ret;
    }

    std::optional<Token const> match(TokenCode code)
    {
        if (m_tokens[m_current].code() != code)
            return {};
        return lex();
    }

    bool expect(TokenCode code)
    {
        auto token_maybe = match(code);
        return token_maybe.has_value();
    }

    std::shared_ptr<Module> parse()
    {
        std::shared_ptr<Module> module = std::make_shared<Module>(m_file_name);
        parse_statements(std::dynamic_pointer_cast<Block>(module));
        return module;
    }

    void parse_statements(std::shared_ptr<Block> block)
    {
        for (auto token = lex(); token.code() != TokenCode::EndOfFile; token = lex()) {
            switch (token.code()) {
            case TokenCode::Identifier:
                block->append(std::make_shared<ProcedureCall>(parse_function_call(token.value())));
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
            if (!expect(TokenCode::SemiColon)) {
                fprintf(stderr, "Syntax Error: Expected ';' after statement\n");
                exit(1);
            }
            fprintf(stdout, "\n");
        }
        printf("\n");
    }

private:
    std::shared_ptr<FunctionCall> parse_function_call(std::string const& func_name)
    {
        if (!expect(TokenCode::OpenParen)) {
            fprintf(stderr, "Syntax Error: Expected '(' after function name\n");
            exit(1);
        }
        std::vector<std::shared_ptr<Expression>> args {};
        auto done = false;
        do {
            args.push_back(parse_expression());
            auto next = lex();
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

    std::shared_ptr<Assignment> parse_variable_declaration()
    {
        auto identifier_maybe = match(TokenCode::Identifier);
        if (!identifier_maybe.has_value()) {
            fprintf(stderr, "Syntax Error: expecting variable name after the 'var' keyword\n");
            exit(1);
        }
        auto identifier = identifier_maybe.value();
        if (!expect(TokenCode::Equals)) {
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
        auto next = peek();
        while (binary_precedence(next) >= min_precedence) {
            auto op = lex();
            auto rhs = parse_primary_expression();
            next = peek();
            while (binary_precedence(next) > binary_precedence(op)) {
                rhs = parse_expression_1(rhs, binary_precedence(op) + 1);
                next = peek();
            }
            lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
        }
        return lhs;
    }

    std::shared_ptr<Expression> parse_primary_expression()
    {
        auto t = lex();
        switch (t.code()) {
        case TokenCode::Plus:
        case TokenCode::Minus: {
            auto operand = parse_atom(lex());
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

    std::string m_file_name;
    const char* m_buf { nullptr };
    Obelix::Lexer m_lexer {};
    std::vector<Token> m_tokens {};
    size_t m_size { 0 };
    size_t m_current { 0 };
};

}

int main(int argc, char** argv)
{
    Obelix::FileBuffer buffer(argv[1]);
    auto tree = buffer.parse();
    tree->dump();
    tree->execute();
    return 0;
}
