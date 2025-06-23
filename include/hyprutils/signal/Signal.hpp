#pragma once

#include <functional>
#include <any>
#include <vector>
#include <memory>
#include <tuple>
#include <hyprutils/memory/WeakPtr.hpp>
#include "./Listener.hpp"

namespace Hyprutils {
    namespace Signal {
        class CUntypedSignal {
          protected:
            CHyprSignalListener                                           registerListenerInternal(std::function<void(void*)> handler);
            void                                                          registerStaticListenerInternal(std::function<void(void*)> handler);
            void                                                          emitInternal(void* args);

            std::vector<Hyprutils::Memory::CWeakPointer<CSignalListener>> m_vListeners;
            std::vector<std::unique_ptr<CSignalListener>>                 m_vStaticListeners;
        };

        template <typename... Args>
        class CSignalT : public CUntypedSignal {
          public:
            void emit(Args... args) {
                auto argsTuple = std::make_tuple(args...);
                emitInternal(&argsTuple);
            }

            [[nodiscard("Listener is unregistered when the ptr is lost")]] CHyprSignalListener registerListener(std::function<void(Args...)> handler) {
                return registerListenerInternal([handler](void* argsPtr) { std::apply(handler, *static_cast<std::tuple<Args...>*>(argsPtr)); });
            }

            // this is for static listeners. They die with this signal.
            void registerStaticListener(std::function<void(Args...)> handler) {
                registerStaticListenerInternal([handler](void* argsPtr) { std::apply(handler, *static_cast<std::tuple<Args...>*>(argsPtr)); });
            }

            template <typename Owner>
            void registerStaticListener(std::function<void(Owner*, Args...)> handler, Owner* owner) {
                registerStaticListener([owner, handler](Args... args) { handler(owner, args...); });
            }
        };

        // compat
        class CSignal : public CSignalT<std::any> {
          public:
            void                                                                               emit(std::any data = {});
            [[nodiscard("Listener is unregistered when the ptr is lost")]] CHyprSignalListener registerListener(std::function<void(std::any)> handler);
            void                                                                               registerStaticListener(std::function<void(void*, std::any)> handler, void* owner);
        };
    }
}
