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
            template <typename T>
            using RefArg = std::conditional_t<std::is_reference_v<T> || std::is_arithmetic_v<T>, T, const T&>;

          public:
            void emit(RefArg<Args>... args) {
                if constexpr (sizeof...(Args) == 0)
                    emitInternal(nullptr);
                else {
                    auto argsTuple = std::tuple<RefArg<Args>...>(args...);

                    if constexpr (sizeof...(Args) == 1)
                        // NOLINTNEXTLINE: const is reapplied by handler invocation if required
                        emitInternal(const_cast<void*>(static_cast<const void*>(&std::get<0>(argsTuple))));
                    else
                        emitInternal(&argsTuple);
                }
            }

            [[nodiscard("Listener is unregistered when the ptr is lost")]] CHyprSignalListener listen(std::function<void(RefArg<Args>...)> handler) {
                return registerListenerInternal(mkHandler(handler));
            }

            [[nodiscard("Listener is unregistered when the ptr is lost")]] CHyprSignalListener listen(std::function<void()> handler)
                requires(sizeof...(Args) != 0)
            {
                return listen([handler](RefArg<Args>... args) { handler(); });
            }

            template <typename... OtherArgs>
            [[nodiscard("Listener is unregistered when the ptr is lost")]] CHyprSignalListener forward(CSignalT<OtherArgs...>& signal) {
                if constexpr (sizeof...(OtherArgs) == 0)
                    return listen([&signal](RefArg<Args>... args) { signal.emit(); });
                else
                    return listen([&signal](RefArg<Args>... args) { signal.emit(args...); });
            }

            // deprecated, use listen()
            CHyprSignalListener registerListener(std::function<void(std::any d)> handler) {
                return listen([handler](const Args&... args) {
                    constexpr auto mkAny = [](std::any d = {}) { return d; };
                    handler(mkAny(args...));
                });
            }

            // this is for static listeners. They die with this signal.
            void listenStatic(std::function<void(RefArg<Args>...)> handler) {
                registerStaticListenerInternal(mkHandler(handler));
            }

            void listenStatic(std::function<void()> handler)
                requires(sizeof...(Args) != 0)
            {
                return listenStatic([handler](RefArg<Args>... args) { handler(); });
            }

            // Deprecated: use listenStatic()
            void registerStaticListener(std::function<void(void*, std::any)> handler, void* owner) {
                return listenStatic([handler, owner](const RefArg<Args>&... args) {
                    constexpr auto mkAny = [](std::any d = {}) { return d; };
                    handler(owner, mkAny(args...));
                });
            }

          private:
            std::function<void(void*)> mkHandler(std::function<void(RefArg<Args>...)> handler) {
                return [handler](void* args) {
                    if constexpr (sizeof...(Args) == 0)
                        handler();
                    else if constexpr (sizeof...(Args) == 1)
                        handler(*static_cast<std::remove_reference_t<std::tuple_element_t<0, std::tuple<RefArg<Args>...>>>*>(args));
                    else
                        std::apply(handler, *static_cast<std::tuple<RefArg<Args>...>*>(args));
                };
            }
        };

        // compat
        class [[deprecated("Use CSignalT")]] CSignal : public CSignalT<std::any> {
          public:
            void emit(std::any data = {});
        };
    }
}
