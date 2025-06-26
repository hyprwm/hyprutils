#pragma once

#include <any>
#include <functional>
#include <hyprutils/memory/SharedPtr.hpp>

namespace Hyprutils {
    namespace Signal {
        class CSignalBase;

        class CSignalListener {
          public:
            CSignalListener(CSignalListener&&)       = delete;
            CSignalListener(CSignalListener&)        = delete;
            CSignalListener(const CSignalListener&)  = delete;
            CSignalListener(const CSignalListener&&) = delete;

            [[deprecated("Relic of the legacy untyped signal API. Using this with CSignalT is undefined behavior.")]] void emit(std::any data);

          private:
            CSignalListener(std::function<void(void*)> handler);

            void                       emitInternal(void* args);

            std::function<void(void*)> m_fHandler;

            friend class CSignalBase;
        };

        typedef Hyprutils::Memory::CSharedPointer<CSignalListener> CHyprSignalListener;
    }
}
