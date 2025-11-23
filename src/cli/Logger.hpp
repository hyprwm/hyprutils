#include <hyprutils/cli/Logger.hpp>
#include <fstream>
#include <filesystem>

namespace Hyprutils::CLI {
    class CLoggerImpl {
      public:
        CLoggerImpl(CLogger*);
        ~CLoggerImpl() = default;

        CLoggerImpl(const CLoggerImpl&) = delete;
        CLoggerImpl(CLoggerImpl&)       = delete;
        CLoggerImpl(CLoggerImpl&&)      = delete;

        void                  updateParentShouldLog();
        void                  appendToRolling(const std::string& s);

        std::string           m_rollingLog;
        std::ofstream         m_logOfs;
        std::filesystem::path m_logFilePath;

        bool                  m_timeEnabled   = false;
        bool                  m_stdoutEnabled = true;
        bool                  m_fileEnabled   = false;
        bool                  m_colorEnabled  = true;

        // this is fine because CLogger is NOMOVE and NOCOPY
        CLogger* m_parent = nullptr;
    };
}