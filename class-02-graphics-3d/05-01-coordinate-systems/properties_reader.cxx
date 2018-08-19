#include "properties_reader.hxx"

//#include <charconv> // not found on Visual Studio 2017.7
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

enum class token_type : uint8_t
{
    none = 0,
    type_key_word,
    prop_name,
    assign,
    int_value,
    float_value,
    string_value
};

struct properties_lexer
{

    struct token
    {
        token_type       t = token_type::none;
        std::string_view value;
        std::regex*      regex_ptr = nullptr;
    };

    std::vector<token> token_list;
    std::string        content;

    struct token_regex
    {
        token_regex(token_type t_, const char* r_)
            : type{ t_ }
            , regex{ r_ }
        {
        }
        token_type type;
        std::regex regex;
    };

    std::vector<token_regex> token_bind;

    properties_lexer(std::string content_)
        : content{ std::move(content_) }
    {
        token_bind.reserve(10);
        token_bind.emplace_back(token_type::type_key_word,
                                R"(^(float)|(string)|(int))");
        token_bind.emplace_back(token_type::assign, R"(^=)");
        token_bind.emplace_back(token_type::prop_name,
                                R"(^[a-zA-Z_][a-zA-Z0-9_]*)");
        token_bind.emplace_back(token_type::int_value, R"(^[1-9]\d*)");
        token_bind.emplace_back(token_type::float_value,
                                R"(^[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)f)");
        // original from internet: /"([^"\\]|\\.)*"/
        token_bind.emplace_back(token_type::string_value,
                                R"(^"([^"\\]|\\.)*")");
        token_bind.emplace_back(token_type::none, R"(^ |;|\n|\t)");

        std::string_view rest_content{ content };
        bool             cant_find_match = false;

        while (!rest_content.empty() && !cant_find_match)
        {
            std::cmatch  token_best_match;
            token_regex* best_token_regex = nullptr;
            for (token_regex& tok_regex : token_bind)
            {
                std::cmatch token_match;
                if (std::regex_search(rest_content.data(),
                                      rest_content.data() +
                                          rest_content.length(),
                                      token_match, tok_regex.regex))
                {
                    auto& first = *token_match.cbegin();
                    if (token_best_match.empty() ||
                        first.length() > token_best_match.cbegin()->length())
                    {
                        token_best_match = token_match;
                        best_token_regex = &tok_regex;
                    }
                }
            }

            if (!token_best_match.empty())
            {
                token tok;
                tok.t                      = best_token_regex->type;
                tok.regex_ptr              = &best_token_regex->regex;
                auto&       first_match    = token_best_match[0];
                const char* first_char_ptr = first_match.first;
                size_t      length = static_cast<size_t>(first_match.length());
                tok.value          = std::string_view(first_char_ptr, length);

                rest_content = rest_content.substr(tok.value.size());

                if (best_token_regex->type == token_type::none)
                {
                    continue;
                }
                else
                {
                    token_list.push_back(tok);
                }
            }
            else
            {
                cant_find_match = true;
            }
        } // while

        if (!rest_content.empty())
        {
            throw std::runtime_error("can't find regex startfing from: " +
                                     std::string(rest_content));
        }
    }
};

std::ostream& operator<<(std::ostream& stream, const token_type t)
{
    switch (t)
    {
        case (token_type::assign):
            stream << "assign";
            break;
        case (token_type::float_value):
            stream << "float";
            break;
        case (token_type::int_value):
            stream << "int";
            break;
        case (token_type::prop_name):
            stream << "prop_name";
            break;
        case (token_type::string_value):
            stream << "string";
            break;
        case (token_type::type_key_word):
            stream << "type_name";
            break;
        case (token_type::none):
            stream << "ws";
            break;
    }
    return stream;
}

using value_type = std::variant<std::string, std::int32_t, float>;

struct properties_parser
{
    struct program_structure
    {
        struct assing_command
        {
            properties_lexer::token* type_name     = nullptr;
            properties_lexer::token* variable_name = nullptr;
            properties_lexer::token* value         = nullptr;
            value_type               real_value;
        };

        std::vector<assing_command> commands;
    } program;

    properties_lexer& lexer;

    properties_parser(properties_lexer& lexer_)
        : lexer{ lexer_ }
    {
        for (auto it = begin(lexer.token_list), end_it = end(lexer.token_list);
             it != end_it;)
        {
            program_structure::assing_command cmd = parse_assing_command(it);
            program.commands.push_back(cmd);
        }
    }

    std::string print_position_of_token(const properties_lexer::token& token)
    {
        std::string_view from_start_to_token(
            lexer.content.data(),
            static_cast<size_t>(token.value.data() - lexer.content.data()) +
                token.value.length());
        std::stringstream ss;
        ss << '\n' << from_start_to_token << '\n';
        size_t last_line_length = 0;
        for (auto back_char = &from_start_to_token.back();
             *back_char != '\0' && *back_char != '\n'; --back_char)
        {
            ++last_line_length;
        }
        last_line_length -= token.value.length();
        ss << std::string(last_line_length, ' ') << '^';
        return ss.str();
    }

    properties_lexer::token* expected(
        const std::vector<properties_lexer::token>::iterator& it,
        const token_type                                      type)
    {
        if (it == end(lexer.token_list))
        {
            std::stringstream ss;
            ss << "error: expected " << type << " but got: EOF";
            throw std::runtime_error(ss.str());
        }
        if (it->t != type)
        {
            std::stringstream ss;
            ss << "error: expected " << type << " but got: " << it->t;
            ss << print_position_of_token(*it);
            throw std::runtime_error(ss.str());
        }
        return &(*it);
    }

    properties_lexer::token* expected_one_of(
        const std::vector<properties_lexer::token>::iterator& it,
        const std::initializer_list<token_type>&              types)
    {
        if (it == end(lexer.token_list))
        {
            std::stringstream ss;
            ss << "error: expected one of: ";
            for (auto type : types)
            {
                ss << type << ", ";
            }
            ss << " but got: EOF";
            throw std::runtime_error(ss.str());
        }
        auto type_it = std::find(begin(types), end(types), it->t);
        if (type_it == end(types))
        {
            std::stringstream ss;
            ss << "error: expected ";
            for (auto type : types)
            {
                ss << type << ", ";
            }
            ss << "but got: " << it->t;
            ss << print_position_of_token(*it);
            throw std::runtime_error(ss.str());
        }
        return &(*it);
    }

    program_structure::assing_command parse_assing_command(
        std::vector<properties_lexer::token>::iterator& it)
    {
        program_structure::assing_command cmd;

        cmd.type_name = expected(it, token_type::type_key_word);

        cmd.variable_name = expected(++it, token_type::prop_name);

        expected(++it, token_type::assign);

        cmd.value = expected_one_of(++it, { token_type::float_value,
                                            token_type::int_value,
                                            token_type::string_value });

        auto        str     = cmd.value->value;
        const char* ptr     = &str.front();
        const char* end_ptr = ptr + str.length();

        switch (cmd.value->t)
        {
            case (token_type::float_value):
            {
                float result;

                // std::from_chars(ptr, end_ptr, result); // not compile ???
                result = std::stof(std::string(cmd.value->value).c_str());

                cmd.real_value = result;
            }
            break;
            case (token_type::int_value):
            {
                std::int32_t result;
                // std::from_chars(ptr, end_ptr, result); // not working with
                // msvc 2017.7
                result         = stoi(std::string(ptr, end_ptr));
                cmd.real_value = result;
            }
            break;
            case (token_type::string_value):
                cmd.real_value = std::string(cmd.value->value);
                break;
            default:
                break;
        };
        ++it; // go to next token

        return cmd;
    }
};

struct properties_interpretator
{
    properties_parser& parser;
    properties_interpretator(properties_parser& parser_)
        : parser{ parser_ }
    {
    }
    void generate(std::unordered_map<std::string, value_type>& key_values)
    {
        for (const auto& command : parser.program.commands)
        {
            key_values.insert(std::make_pair(command.variable_name->value,
                                             command.real_value));
        }

        // debug
        for (auto it : key_values)
        {
            std::clog << "key: " << it.first << " value: ";
            if (std::holds_alternative<std::string>(it.second))
            {
                std::clog << std::get<std::string>(it.second);
            }
            else if (std::holds_alternative<std::int32_t>(it.second))
            {
                std::clog << std::get<std::int32_t>(it.second);
            }
            else if (std::holds_alternative<float>(it.second))
            {
                std::clog << std::get<float>(it.second);
            }
            std::clog << std::endl;
        }
    }
};

class properties_reader::impl
{
public:
    impl(const fs::path& path_)
        : path{ path_ }
    {
        // TODO parse file
        std::ifstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);

        file.open(path);

        std::stringstream ss;
        ss << file.rdbuf();

        const std::string content{ ss.str() };

        properties_lexer         lexer(content);
        properties_parser        parser(lexer);
        properties_interpretator generator(parser);

        generator.generate(key_values);
    }

private:
    fs::path                                    path;
    std::unordered_map<std::string, value_type> key_values;
};

properties_reader::properties_reader(const fs::path& path)
    : ptr(new impl(path))
{
}

void properties_reader::update_changes() {}

std::string_view properties_reader::get_string(std::string_view) const
{
    return {};
}

int32_t properties_reader::get_int(std::string_view) const
{
    return {};
}

float properties_reader::get_float(std::string_view) const
{
    return {};
}

properties_reader::~properties_reader() {}
