#include <hyprutils/cli/ArgumentParser.hpp>

#include <map>
#include <variant>
#include <vector>

namespace Hyprutils::CLI {
    enum eArgumentType : uint8_t {
        ARG_TYPE_BOOL = 0,
        ARG_TYPE_INT,
        ARG_TYPE_FLOAT,
        ARG_TYPE_STR,
        ARG_TYPE_END,
    };

    struct SArgumentKey {
        using Value = std::variant<std::monostate, bool, int, float, std::string>;

        std::string   full, abbrev, desc;
        eArgumentType argType = ARG_TYPE_BOOL;

        Value         val;
    };

    class CArgumentParserImpl {
      public:
        CArgumentParserImpl(const std::span<const char*>& args);
        ~CArgumentParserImpl() = default;

        std::string                         getDescription(const std::string_view& header, std::optional<size_t> maxWidth = {});
        std::expected<void, std::string>    parse();
        std::vector<SArgumentKey>::iterator getValue(const std::string_view& sv);
        std::expected<void, std::string>    registerOption(const std::string_view& name, const std::string_view& abbrev, const std::string_view& description, eArgumentType type);

        std::vector<SArgumentKey>           m_values;

        std::vector<std::string_view>       m_argv;
    };
}