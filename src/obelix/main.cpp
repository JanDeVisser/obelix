#include <core/StringBuffer.h>
#include <cstdio>
#include <fcntl.h>
#include <lexer/Lexer.h>
#include <lexer/Token.h>
#include <obelix/NativeFunction.h>
#include <obelix/Scope.h>
#include <obelix/Syntax.h>
#include <optional>
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
        struct stat sb {
        };
        if (auto rc = fstat(fh, &sb); rc < 0) {
            perror("Stat");
            close(fh);
            return;
        }

        auto size = sb.st_size;
        auto buf = new char[size + 1];
        if (auto rc = ::read(fh, (void*)buf, size); rc < size) {
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
    constexpr static TokenCode KeywordVar = ((TokenCode)200);
    constexpr static TokenCode KeywordFunc = ((TokenCode)201);
    constexpr static TokenCode KeywordIf = ((TokenCode)202);
    constexpr static TokenCode KeywordElse = ((TokenCode)203);
    constexpr static TokenCode KeywordWhile = ((TokenCode)204);
    constexpr static TokenCode KeywordTrue = ((TokenCode)205);
    constexpr static TokenCode KeywordFalse = ((TokenCode)206);
    constexpr static TokenCode KeywordReturn = ((TokenCode)207);
    constexpr static TokenCode KeywordBreak = ((TokenCode)208);
    constexpr static TokenCode KeywordContinue = ((TokenCode)209);

    explicit Parser(std::string const& file_name)
        : m_file_name(file_name)
        , m_file_buffer(file_name)
        , m_lexer(m_file_buffer.buffer())
    {
        m_lexer.add_scanner<QStringScanner>();
        m_lexer.add_scanner<IdentifierScanner>();
        m_lexer.add_scanner<NumberScanner>(Obelix::NumberScanner::Config { true, false, true, true });
        m_lexer.add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
        m_lexer.add_scanner<KeywordScanner>(
            Token(KeywordVar, "var"),
            Token(KeywordFunc, "func"),
            Token(KeywordIf, "if"),
            Token(KeywordElse, "else"),
            Token(KeywordWhile, "while"),
            Token(KeywordTrue, "true"),
            Token(KeywordFalse, "false"),
            Token(KeywordReturn, "return"),
            Token(KeywordBreak, "break"),
            Token(KeywordContinue, "continue"),
            TokenCode::GreaterEqualThan,
            TokenCode::LessEqualThan,
            TokenCode::EqualsTo,
            TokenCode::NotEqualTo,
            TokenCode::LogicalAnd,
            TokenCode::LogicalOr,
            TokenCode::ShiftLeft,
            TokenCode::ShiftRight);
    }

    std::shared_ptr<Module> parse()
    {
        std::shared_ptr<Module> module = std::make_shared<Module>(m_file_name);
        parse_statements(std::dynamic_pointer_cast<Block>(module));
        return module;
    }

private:
    std::shared_ptr<Statement> parse_statement()
    {
        auto token = m_lexer.peek();
        switch (token.code()) {
        case TokenCode::SemiColon:
            m_lexer.lex();
            return std::make_shared<Pass>();
        case TokenCode::OpenBrace:
            m_lexer.lex();
            return parse_block(std::make_shared<Block>());
        case TokenCode::Identifier: {
            m_lexer.lex();
            switch (m_lexer.current_code()) {
            case TokenCode::OpenParen:
                return std::make_shared<ProcedureCall>(parse_function_call(token.value()));
            case TokenCode::Equals:
                return parse_assignment(token.value());
            default:
                fprintf(stderr, "Syntax Error: Expected '(' or ' ='\n");
                exit(1);
            }
        }
        case KeywordIf:
            m_lexer.lex();
            return parse_if_statement();
        case KeywordWhile:
            m_lexer.lex();
            return parse_while_statement();
        case KeywordVar:
            m_lexer.lex();
            return parse_variable_declaration();
        case KeywordFunc:
            m_lexer.lex();
            return parse_function_definition();
        case KeywordReturn:
            m_lexer.lex();
            return std::make_shared<Return>(parse_expression());
        case KeywordBreak:
            m_lexer.lex();
            return std::make_shared<Break>();
        case KeywordContinue:
            m_lexer.lex();
            return std::make_shared<Continue>();
        default:
            break;
        }
        return nullptr;
    }

    void parse_statements(std::shared_ptr<Block> const& block)
    {
        while (true) {
            auto statement = parse_statement();
            if (!statement)
                break;
            block->append(statement);
        };
    }

    std::shared_ptr<Block> parse_block(std::shared_ptr<Block> block)
    {
        parse_statements(block);
        if (!m_lexer.expect(TokenCode::CloseBrace)) {
            fprintf(stderr, "Syntax Error: Expected '}'\n");
            exit(1);
        }
        return block;
    }

    std::shared_ptr<FunctionCall> parse_function_call(std::string const& func_name)
    {
        if (!m_lexer.expect(TokenCode::OpenParen)) {
            fprintf(stderr, "Syntax Error: Expected '(' after function name\n");
            exit(1);
        }
        std::vector<std::shared_ptr<Expression>> args {};
        auto done = m_lexer.current_code() == TokenCode::CloseParen;
        while (!done) {
            args.push_back(parse_expression());
            auto next = m_lexer.lex();
            switch (next.code()) {
            case TokenCode::Comma:
                break;
            case TokenCode::CloseParen:
                done = true;
                break;
            default:
                fprintf(stderr, "Syntax Error: Expected ',' or ')' in function argument list");
                exit(1);
            }
        };
        return std::make_shared<FunctionCall>(func_name, args);
    }

    std::shared_ptr<Assignment> parse_assignment(std::string const& identifier)
    {
        m_lexer.lex(); // Consume the equals
        auto expr = parse_expression();
        return std::make_shared<Assignment>(identifier, expr);
    }

    std::shared_ptr<FunctionDef> parse_function_definition()
    {
        auto name_maybe = m_lexer.match(TokenCode::Identifier);
        if (!name_maybe.has_value()) {
            fprintf(stderr, "Syntax Error: expecting variable name after the 'func' keyword\n");
            exit(1);
        }
        if (!m_lexer.expect(TokenCode::OpenParen)) {
            fprintf(stderr, "Syntax Error: expecting '(' after function name in definition\n");
            exit(1);
        }
        std::vector<std::string> params {};
        auto done = m_lexer.current_code() == TokenCode::CloseParen;
        while (!done) {
            auto param_name_maybe = m_lexer.match(TokenCode::Identifier);
            if (!param_name_maybe.has_value()) {
                fprintf(stderr, "Syntax Error: Expected parameter name\n");
                exit(1);
            }
            params.push_back(param_name_maybe.value().value());
            auto next = m_lexer.lex();
            switch (next.code()) {
            case TokenCode::Comma:
                break;
            case TokenCode::CloseParen:
                done = true;
                break;
            default:
                fprintf(stderr, "Syntax Error: Expected ',' or ')' in function parameter list");
                exit(1);
            }
        }
        if (!m_lexer.expect(TokenCode::OpenBrace)) {
            fprintf(stderr, "Syntax Error: expecting '{' after function parameter list\n");
            exit(1);
        }
        auto function_def = std::make_shared<FunctionDef>(name_maybe.value().value(), params);
        parse_block(function_def);
        return function_def;
    }

    std::shared_ptr<IfStatement> parse_if_statement()
    {
        auto condition = parse_expression();
        std::shared_ptr<Statement> else_stmt = nullptr;
        auto if_stmt = parse_statement();
        auto else_maybe = m_lexer.peek();
        if (else_maybe.code() == KeywordElse) {
            m_lexer.lex();
            else_stmt = parse_statement();
        }
        return std::make_shared<IfStatement>(condition, if_stmt, else_stmt);
    }

    std::shared_ptr<WhileStatement> parse_while_statement()
    {
        auto condition = parse_expression();
        auto stmt = parse_statement();
        return std::make_shared<WhileStatement>(condition, stmt);
    }

    std::shared_ptr<Assignment> parse_variable_declaration()
    {
        auto identifier_maybe = m_lexer.match(TokenCode::Identifier);
        if (!identifier_maybe.has_value()) {
            fprintf(stderr, "Syntax Error: expecting variable name after the 'var' keyword\n");
            exit(1);
        }
        if (!m_lexer.expect(TokenCode::Equals)) {
            fprintf(stderr, "Syntax Error: expecting '=' after variable name in assignment\n");
            exit(1);
        }
        auto expr = parse_expression();
        return std::make_shared<Assignment>(identifier_maybe.value().value(), expr, true);
    }

    /*
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

    static int binary_precedence(Token const& token)
    {
        switch (token.code()) {
        case TokenCode::LogicalOr:
            return 3;
        case TokenCode::LogicalAnd:
            return 4;
        case TokenCode::Pipe:
            return 5;
        case TokenCode::Hat:
            return 6;
        case TokenCode::Ampersand:
            return 7;
        case TokenCode::EqualsTo:
        case TokenCode::NotEqualTo:
            return 8;
        case TokenCode::GreaterThan:
        case TokenCode::LessThan:
        case TokenCode::GreaterEqualThan:
        case TokenCode::LessEqualThan:
            return 9;
        case TokenCode::ShiftLeft:
        case TokenCode::ShiftRight:
            return 10;
        case TokenCode::Plus:
        case TokenCode::Minus:
            return 11;
        case TokenCode::Asterisk:
        case TokenCode::Slash:
        case TokenCode::Percent:
            return 12;
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
        case TokenCode::OpenParen: {
            auto ret = parse_expression();
            if (!m_lexer.expect(TokenCode::CloseParen)) {
                fprintf(stderr, "Syntax Error: Expected ')'\n");
                exit(1);
            }
            return ret;
        }
        case TokenCode::Tilde:
        case TokenCode::ExclamationPoint:
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
            fprintf(stderr, "ERROR: Expected literal or variable, got '%s'\n", atom.value().c_str());
            exit(1);
        }
    }

    FileBuffer m_file_buffer;
    std::string m_file_name;
    Lexer m_lexer;
};

std::string format_arguments(Ptr<Arguments> args)
{
    std::string fmt;
    std::vector<Obelix::Obj> format_args;
    for (auto& a : args->arguments()) {
        if (fmt.empty()) {
            fmt = a->to_string();
        } else {
            format_args.push_back(a);
        }
    }
    return format(fmt, format_args);
}

extern "C" void oblfunc_print(const char* name, Obelix::Ptr<Obelix::Arguments>* args, Obj* ret)
{
    Ptr <Arguments> arguments = *args;
    printf("%s\n", format_arguments(arguments).c_str());
    *ret = Object::null();
}

extern "C" void oblfunc_format(const char* name, Obelix::Ptr<Obelix::Arguments>* args, Obj* ret)
{
    Ptr <Arguments> arguments = *args;
    *ret = make_obj<String>(format_arguments(arguments).c_str());
}

void seed_global_scope(Obelix::Scope& global_scope)
{
    Resolver::get_resolver().open("");
    global_scope.declare("print", Obelix::make_obj<Obelix::NativeFunction>("oblfunc_print"));
}

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
            Obelix::Logger::get_logger().enable("resolve");
        } else {
            file_name = argv[ix];
        }
    }

    if (file_name.empty()) {
        usage();
    }

    Obelix::Parser parser(file_name);
    auto tree = parser.parse();
//    tree->dump(0);
    Obelix::Scope global_scope;
    seed_global_scope(global_scope);
    tree->execute(global_scope);
    return 0;
}
