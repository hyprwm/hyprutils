#pragma once

#include <any>
#include <functional>
#include <hyprutils/memory/SharedPtr.hpp>

namespace Hyprutils {
    namespace Signal {
        class CUntypedSignal;

        class CSignalListener;
        class CSignalListenerListHead;

        class CSignalListenerListEntry {
          public:
            ~CSignalListenerListEntry();

          protected:
            CSignalListenerListEntry* m_previous = nullptr;
            CSignalListener*          m_next     = nullptr;

            friend class CSignalListenerListHead; // ?
        };

        class CSignalListenerListHead : public CSignalListenerListEntry {
          public:
            CSignalListenerListHead(bool ownsListeners);
            ~CSignalListenerListHead(); // intentionally non-virtual
            void append(CSignalListener* listener);
            void removeLast();
            // returns true if the list head is still alive
            bool             emitAll(void* args);

            CSignalListener* m_last            = nullptr;
            CSignalListener* m_lastEmit        = nullptr;
            CSignalListener* m_currentListener = nullptr;

          private:
            bool m_ownsListeners;
        };

        class CSignalListener : public CSignalListenerListEntry {
          public:
            CSignalListener(CSignalListener&&)       = delete;
            CSignalListener(CSignalListener&)        = delete;
            CSignalListener(const CSignalListener&)  = delete;
            CSignalListener(const CSignalListener&&) = delete;

            [[deprecated("Relic of the legacy untyped signal API. Using this with CSignalT is undefined behavior.")]] void emit(std::any data);

          private:
            CSignalListener(std::function<void(void*)> handler);

            // returns true if the list head is still alive
            bool                       emitUntil(void* args, CSignalListenerListHead* head, bool headOwnsListeners);
            void                       emitInternal(void* args);

            std::function<void(void*)> m_fHandler;

            friend class CSignalListenerListHead;
            friend class CUntypedSignal;
        };

        typedef Hyprutils::Memory::CSharedPointer<CSignalListener> CHyprSignalListener;
    }
}
