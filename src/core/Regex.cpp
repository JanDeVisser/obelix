//
// Created by Jan de Visser on 2021-10-04.
//

#include <core/List.h>
#include <core/Regex.h>

namespace Obelix {

logging_category(regex);

Ptr<Object> Regex::compile()
{
    if (!m_is_compiled) {
        try {
            m_compiled = std::regex(m_pattern, m_re_flags);
            m_is_compiled = true;
        }
        catch (const std::regex_error& e) {
            return make_obj<Exception>(ErrorCode::RegexpSyntaxError, e.what());
        }
    }
    return Object::null();
}

Ptr<Object> Regex::match(std::string const& text)
{
    debug(regex, "%s .match(%s)", to_string().c_str(), text.c_str());
    if (auto ret = compile(); ret->is_exception())
        return ret;
    auto matches_begin =
        std::sregex_iterator(text.begin(), text.end(), m_compiled);
    auto matches_end = std::sregex_iterator();
    auto count = std::distance(matches_begin, matches_end);

    if (!count) {
        debug(regex, "%s .match(%s): No matches", to_string().c_str(), text.c_str());
        return to_obj(Boolean::False());
    } else if (count == 1) {
        debug(regex, "%s .match(%s): One match", to_string().c_str(), text.c_str());
        auto match = *matches_begin;
        return make_obj<String>(match.str());
    } else {
        debug(regex, "%s .match(%s): %d matches", to_string().c_str(), text.c_str(), count);
        auto list = make_typed<List>();
        for (auto it = matches_begin; it != matches_end; ++it) {
            auto match = *it;
            list->emplace_back<String>(match.str());
        }
        return to_obj(list);
    }
}

Ptr<Object> Regex::replace(std::string const& text, std::vector<std::string>)
{
    debug(regex, "%s .replace(%s)", to_string().c_str(), text.c_str());
    debug(regex, "Not yet implemented!");
    return Object::null();
}

std::string Regex::to_string() const
{
    std::string ret = "/" + m_pattern + "/";
    if (!m_flags.empty())
        ret += m_flags;
    return ret;
}

int Regex::compare(Object const& other) const
{
    auto other_re = dynamic_cast<Regex const&>(other);
    return m_pattern.compare(other_re.m_pattern);
}

}
