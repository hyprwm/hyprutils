#pragma once

#include <functional>
#include <any>
#include <type_traits>
#include <utility>
#include <vector>
#include <memory>
#include <tuple>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include "./Listener.hpp"

namespace Hyprutils {
    namespace Signal {
        class CSignalBase {
          protected:
            CHyprSignalListener                                             registerListenerInternal(std::function<void(void*)> handler);
            void                                                            registerStaticListenerInternal(std::function<void(void*)> handler);
            void                                                            emitInternal(void* args);

            std::vector<Hyprutils::Memory::CWeakPointer<CSignalListener>>   m_vListeners;
            std::vector<Hyprutils::Memory::CSharedPointer<CSignalListener>> m_vStaticListeners;
        };

        template <typename... Args>
        class CSignalT : public CSignalBase {
          public:
            void emit(Args... args) {
                if constexpr (sizeof...(Args) == 0)
                    emitInternal(nullptr);
                else {
                    auto argsTuple = std::tuple<Args...>(args...);
                    if constexpr (sizeof...(Args) == 1)
                        // NOLINTNEXTLINE: const is reapplied by handler invocation if required
                        emitInternal(const_cast<void*>(static_cast<const void*>(&std::get<0>(argsTuple))));
                    else
                        emitInternal(&argsTuple);
                }
            }

            [[nodiscard("Listener is unregistered when the ptr is lost")]] CHyprSignalListener listen(std::function<void(Args...)> handler) {
                return registerListenerInternal(mkHandler(handler));
            }

            [[deprecated("Use the typed signal API")]] CHyprSignalListener registerListener(std::function<void(std::any d)> handler) {
                return listen([handler](const Args&... args) {
                    constexpr auto mkAny = [](std::any d = {}) { return d; };
                    handler(mkAny(args...));
                });
            }

            // this is for static listeners. They die with this signal.
            void listenStatic(std::function<void(Args...)> handler) {
                registerStaticListenerInternal(mkHandler(handler));
            }

            [[deprecated("Use the typed signal API")]] void registerStaticListener(std::function<void(void*, std::any)> handler, void* owner) {
                return listenStatic([handler, owner](const Args&... args) {
                    constexpr auto mkAny = [](std::any d = {}) { return d; };
                    handler(owner, mkAny(args...));
                });
            }

          private:
            std::function<void(void*)> mkHandler(std::function<void(Args...)> handler) {
                return [handler](void* args) {
                    if constexpr (sizeof...(Args) == 0)
                        handler();
                    else if constexpr (sizeof...(Args) == 1)
                        handler(*static_cast<std::remove_reference_t<std::tuple_element_t<0, std::tuple<Args...>>>*>(args));
                    else
                        std::apply(handler, *static_cast<std::tuple<Args...>*>(args));
                };
            }
        };

        // compat
        class [[deprecated("Use the typed signal API")]] CSignal : public CSignalT<std::any> {
          public:
            void emit(std::any data = {});
        };
    }
}
