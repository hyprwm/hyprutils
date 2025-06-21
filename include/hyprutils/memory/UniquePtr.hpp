#pragma once

#include "ImplBase.hpp"

/*
    This is a custom impl of std::unique_ptr.
    In contrast to the STL one, it allows for
    creation of a weak_ptr, that will then be unable
    to be locked.
*/

namespace Hyprutils {
    namespace Memory {
        template <typename T>
        class CUniquePointer {
          public:
            template <typename X>
            using validHierarchy = typename std::enable_if<std::is_assignable<CUniquePointer<T>&, X>::value, CUniquePointer&>::type;
            template <typename X>
            using isConstructible = typename std::enable_if<std::is_constructible<T&, X&>::value>::type;

            /* creates a new unique pointer managing a resource
               avoid calling. Could duplicate ownership. Prefer makeUnique */
            explicit CUniquePointer(T* object) : impl_(new Impl_::impl<T>(object, false)) {}

            /* creates a shared pointer from a reference */
            template <typename U, typename = isConstructible<U>>
            CUniquePointer(const CUniquePointer<U>& ref) = delete;
            CUniquePointer(const CUniquePointer& ref)    = delete;

            template <typename U, typename = isConstructible<U>>
            CUniquePointer(CUniquePointer<U>&& ref) noexcept {
                std::swap(impl_, ref.impl_);
            }

            CUniquePointer(CUniquePointer&& ref) noexcept {
                std::swap(impl_, ref.impl_);
            }

            /* creates an empty unique pointer with no implementation */
            CUniquePointer() noexcept {
                ; // empty
            }

            /* creates an empty unique pointer with no implementation */
            CUniquePointer(std::nullptr_t) noexcept {
                ; // empty
            }

            ~CUniquePointer() {
                decrement();
            }

            template <typename U>
            validHierarchy<const CUniquePointer<U>&> operator=(const CUniquePointer<U>& rhs) = delete;
            CUniquePointer&                          operator=(const CUniquePointer& rhs)    = delete;

            template <typename U>
            validHierarchy<const CUniquePointer<U>&> operator=(CUniquePointer<U>&& rhs) {
                std::swap(impl_, rhs.impl_);
                return *this;
            }

            CUniquePointer& operator=(CUniquePointer&& rhs) {
                std::swap(impl_, rhs.impl_);
                return *this;
            }

            operator bool() const {
                return impl_;
            }

            bool operator()(const CUniquePointer& lhs, const CUniquePointer& rhs) const {
                return reinterpret_cast<uintptr_t>(lhs.impl_) < reinterpret_cast<uintptr_t>(rhs.impl_);
            }

            T* operator->() const {
                return get();
            }

            T& operator*() const {
                return *get();
            }

            void reset() {
                decrement();
                impl_ = nullptr;
            }

            T* get() const {
                return impl_ ? static_cast<T*>(impl_->getData()) : nullptr;
            }

            Impl_::impl_base* impl_ = nullptr;

          private:
            /* 
                no-op if there is no impl_
                may delete the stored object if ref == 0
                may delete and reset impl_ if ref == 0 and weak == 0
            */
            void decrement() {
                if (!impl_)
                    return;

                if (impl_->dec()) {
                    delete impl_;
                    impl_ = nullptr;
                }
            }
            /* no-op if there is no impl_ */
            void increment() {
                if (!impl_)
                    return;

                if (!impl_->inc())
                    impl_ = nullptr;
            }
        };

        template <typename U, typename... Args>
        static CUniquePointer<U> makeUnique(Args&&... args) {
            return CUniquePointer<U>(new U(std::forward<Args>(args)...));
        }
    }
}

template <typename T>
struct std::hash<Hyprutils::Memory::CUniquePointer<T>> {
    std::size_t operator()(const Hyprutils::Memory::CUniquePointer<T>& p) const noexcept {
        return std::hash<void*>{}(p.impl_);
    }
};
