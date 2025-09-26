#pragma once

#include "ImplBase.hpp"
#include "Casts.hpp"

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
            using validHierarchy = std::enable_if_t<std::is_assignable_v<CUniquePointer<T>&, X>, CUniquePointer&>;
            template <typename X>
            using isConstructible = std::enable_if_t<std::is_constructible_v<T&, X&>>;

            /* creates a new unique pointer managing a resource
               avoid calling. Could duplicate ownership. Prefer makeUnique */
            explicit CUniquePointer(T* object) noexcept : impl_(new Impl_::impl<T>(object, false)) {
                increment();
            }

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
            CUniquePointer() noexcept = default;

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

            CUniquePointer& operator=(CUniquePointer&& rhs) noexcept {
                std::swap(impl_, rhs.impl_);
                return *this;
            }

            operator bool() const {
                return impl_;
            }

            bool operator()(const CUniquePointer& lhs, const CUniquePointer& rhs) const {
                return rc<uintptr_t>(lhs.impl_) < rc<uintptr_t>(rhs.impl_);
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
                return impl_ ? sc<T*>(impl_->getData()) : nullptr;
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

                impl_->dec();

                // if ref == 0, we can destroy impl
                if (impl_->ref() == 0)
                    destroyImpl();
            }
            /* no-op if there is no impl_ */
            void increment() {
                if (!impl_)
                    return;

                impl_->inc();
            }

            /* destroy the pointed-to object 
               if able, will also destroy impl */
            void destroyImpl() {
                // destroy the impl contents
                impl_->destroy();

                // check for weak refs, if zero, we can also delete impl_
                if (impl_->wref() == 0) {
                    delete impl_;
                    impl_ = nullptr;
                }
            }
        };

        template <typename U, typename... Args>
        [[nodiscard]] inline CUniquePointer<U> makeUnique(Args&&... args) {
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
