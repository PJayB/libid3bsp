#include <id3bsp/Entities.h>
#include <lexy/action/parse.hpp>
#include <lexy/input/range_input.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy_ext/report_error.hpp>
#include <cassert>

namespace id3bsp
{
namespace impl
{
namespace storage
{
    using string = std::string;
    using key_value_pair = std::pair<string, string>;
    using dictionary = Entity::Dictionary;
    using entity_list = std::vector<Entity>;
} // storage

namespace grammar
{
namespace dsl = lexy::dsl;

struct string : lexy::token_production
{
    struct invalid_char
    {
        static LEXY_CONSTEVAL auto name()
        {
            return "invalid character in string literal";
        }
    };

    static constexpr auto rule = [] {
        auto code_point = (-dsl::unicode::control).error<invalid_char>;
        return dsl::quoted.limit(dsl::ascii::newline)(code_point);
    }();

    static constexpr auto value = lexy::as_string<storage::string,
        lexy::utf8_encoding>;
};

struct key_value_pair : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::blank;
    static constexpr auto rule = dsl::p<string> + dsl::p<string>;
    static constexpr auto value = lexy::construct<storage::key_value_pair>;
};

struct entity
{
    static constexpr auto rule = [] {
        return dsl::curly_bracketed.opt_list(dsl::p<key_value_pair>,
            dsl::trailing_sep(dsl::newline));
    }();

    static constexpr auto value = lexy::as_collection<storage::dictionary>;
};

struct entity_list
{
    static constexpr auto whitespace
        = dsl::ascii::space | LEXY_LIT("//") >> dsl::until(dsl::newline);
    static constexpr auto rule = [] {
        return dsl::list(dsl::p<entity>);
    }();
    static constexpr auto value = lexy::as_list<storage::entity_list>;
};

} // grammar
} // private

bool Entity::Parse(const std::string& entityStr, const char* filename,
    std::vector<Entity>& entities)
{
    auto input = lexy::range_input<lexy::utf8_encoding,
        std::string::const_iterator>(entityStr.begin(), entityStr.end());

    auto result = lexy::parse<impl::grammar::entity_list>(input,
        lexy_ext::report_error.path(filename));

    entities = std::move(result.value());
    return true;
}

} // id3bsp
