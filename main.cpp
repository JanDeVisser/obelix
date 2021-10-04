#include "ast/Syntax.h"
#include "lexer/Token.h"
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

class FileBuffer {
public:
    explicit FileBuffer(char const* file_name) {
        int fh = open(file_name, O_RDONLY);
        if (fh < 0) {
            perror("Open");
            return;
        }
        struct stat sb {};
        if (auto rc = fstat(fh, &sb); rc < 0) {
            perror("Stat");
            close(fh);
            return;
        }

        m_size = sb.st_size;
        m_buf = new char[m_size];
        if (auto rc = read(fh, (void *) m_buf, m_size); rc < m_size) {
            perror("read");
            m_size = 0;
            delete[] m_buf;
            m_buf = nullptr;
        }
        close(fh);
    }

    [[nodiscard]] char current() const { return lookahead(0); }
    [[nodiscard]] char lookahead(size_t how_many = 1) const {
        assert(how_many >= 0);
        if (m_current < (m_size - how_many)) {
            return m_buf[m_current + how_many];
        } else {
            return -1;
        }
    }

    bool advance(size_t how_many = 1) {
        if (m_current <= (m_size - how_many)) {
            m_current += how_many;
            return true;
        } else {
            return false;
        }
    }

    Token lex() {
        auto start = m_current;
        if (current() < 0) {
            return Token { TokenType::EndOfFile };
        }

        if (std::isspace(current()) != 0) {
            do {
                advance();
            } while ((current() > 0) && (std::isspace(current()) != 0));
            return Token { TokenType::Whitespace, std::string(m_buf + start, m_current - start) };
        }

        if (std::isdigit(current()) != 0) {
            do {
                advance();
            } while (std::isdigit(current()) != 0);
            return Token { TokenType::Number, std::string(m_buf + start, m_current - start) };
        }

        auto maybe_token = match();
        if (maybe_token.has_value()) {
            return maybe_token.value();
        }

        if ((std::isalpha(current()) != 0) || (current() == '_')) {
            do {
                advance();
            } while ((std::isalnum(current()) != 0) || (current() == '_'));
            return Token { TokenType::Identifier, std::string(m_buf + start, m_current - start) };
        }

        return Token { TokenType::Unknown, std::string(m_buf + m_current, 1) };
    }

    std::shared_ptr<SyntaxNode> parse() {
        std::shared_ptr<Block> ret = std::make_shared<Block>();
        for (auto token = lex(); token.type() != TokenType::EndOfFile; token = lex()) {
            switch (token.type()) {
            case TokenType::Whitespace:
                break;
            case TokenType::Identifier:
                ret->append(parseIdentifier(token));
                break;
            default:
                break;
            }
            fprintf(stdout, "%s ", token.to_string().c_str());
        }
        return std::dynamic_pointer_cast<SyntaxNode>(ret);
    }

private:
    std::optional<Token> match()
    {
        std::string val;
        auto cur = current();
        size_t len = 0;
#undef __ENUMERATE_TOKEN_TYPE
#define __ENUMERATE_TOKEN_TYPE(type, c, str)                                       \
    if constexpr (c > 0) {                                                         \
        if (cur == c) {                                                            \
            val[0] = c;                                                            \
            advance();                                                             \
            return Token { TokenType::type, val };                                 \
        }                                                                          \
    }                                                                              \
    if constexpr (str != nullptr) {                                                \
        len = strlen(str);                                                         \
        if ((m_current < m_size - len) && !strncmp(m_buf + m_current, str, len)) { \
            advance(len);                                                          \
            return Token { TokenType::type, str };                                 \
        }                                                                          \
    }
        ENUMERATE_TOKEN_TYPES(__ENUMERATE_TOKEN_TYPE)
#undef __ENUMERATE_TOKEN_TYPE
        return {};
    }

    std::shared_ptr<SyntaxNode> parseIdentifier(Token const& identifier)
    {
        auto next = lex();
        switch (next.type()) {
        case TokenType::Equals:
            auto expr = parseExpression();
            return std::make_shared<Assignment>(identifier, expr);
        default:
            return std::make_shared<ErrorNode>(ErrorCode::SyntaxError);
        }
        return { };
    }

    std::shared_ptr<SyntaxNode> parseExpression()
    {
        auto next = lex();
        switch (next.type()) {
        case TokenType::Number:
            return parseAssignment();
        default:
            return std::make_shared<ErrorNode>(ErrorCode::SyntaxError);
        }
        return { };
    }

    const char* m_buf { nullptr };
    size_t m_size { 0 };
    size_t m_current { 0 };
};

int main(int argc, char** argv) {
    FileBuffer buffer(argv[1]);
    auto tree = buffer.parse();
    return 0;
}
