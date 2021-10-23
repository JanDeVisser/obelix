/*
 * oblasm.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lexer/FileBuffer.h>
#include <lexer/Lexer.h>
#include <vm/VirtualMachine.h>

namespace Obelix {

class Assembler {
public:
#define __ENUM_Opcode(S) \
    S(Nop)               \
    S(Mov)               \
    S(Push)              \
    S(Pop)               \
    S(Inc)               \
    S(Dec)               \
    S(Add)               \
    S(Sub)               \
    S(Mul)               \
    S(Div)               \
    S(Jump)              \
    S(JumpIf)            \
    S(JumpZero)          \
    S(Call)              \
    S(Ret)               \
    S(Native)            \
    S(Halt)

    enum class Opcode {
        Db = 200,
#undef __ENUM_Opcode_V
#define __ENUM_Opcode_V(value) value,
        __ENUM_Opcode(__ENUM_Opcode_V)
#undef __ENUM_Opcode_V
    };

    constexpr static TokenCode KeywordDb = (TokenCode)Opcode::Db;
#undef __ENUM_Opcode_V
#define __ENUM_Opcode_V(value) \
    constexpr static TokenCode Keyword##value = (TokenCode)Opcode::value;
    __ENUM_Opcode(__ENUM_Opcode_V)
#undef __ENUM_Opcode_V

        explicit Assembler(std::string const& file_name)
        : m_file_buffer(file_name)
        , m_file_name(file_name)
        , m_lexer(m_file_buffer.buffer())
    {
        m_lexer.add_scanner<QStringScanner>();
        m_lexer.add_scanner<IdentifierScanner>();
        m_lexer.add_scanner<NumberScanner>(Obelix::NumberScanner::Config { true, false, true, true });
        m_lexer.add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
        m_lexer.add_scanner<CommentScanner>(
            CommentScanner::CommentMarker { false, false, "/*", "*/" },
            CommentScanner::CommentMarker { false, true, "//", "" },
            CommentScanner::CommentMarker { false, true, "#", "" });
        m_lexer.filter_codes(TokenCode::Whitespace, TokenCode::Comment);
        m_lexer.add_scanner<KeywordScanner>(
#undef __ENUM_Opcode_V
#define __ENUM_Opcode_V(value) \
    Token(Keyword##value, #value),
            __ENUM_Opcode(__ENUM_Opcode_V)
#undef __ENUM_Opcode_V
                Token(KeywordDb, "db"));
    }

    void parse()
    {
        do {
            auto token = m_lexer.peek();
            switch (token.code()) {
            case TokenCode::EndOfFile:
                m_lexer.lex();
                return;
            case KeywordNop:
                m_lexer.lex();
                break;
            case KeywordMov:
                m_lexer.lex();
                parse_mov();
                break;
            }
        } while (true);
    }

private:
    void parse_mov()
    {
        bool indirect_to = false;
        uint64_t indirect_to_addr = 0;
        if (m_lexer.current_code() == TokenCode::OpenBracket) {
            indirect_to = true;
            m_lexer.lex();
        }
        if (indirect_to && m_lexer.current_code() == TokenCode::HexNumber) {
            auto num_token = m_lexer.lex();
            num_token.
        }
    }

    FileBuffer m_file_buffer;
    std::string m_file_name;
    Lexer m_lexer;
    std::vector<Instruction> m_instructions {};
};

}