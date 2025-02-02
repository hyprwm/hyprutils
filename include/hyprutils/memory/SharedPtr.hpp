#pragma once

#include "ImplBase.hpp"

/*
    This is a custom impl of std::shared_ptr.
    It is not thread-safe like the STL one,
    but Hyprland is single-threaded anyways.

    It differs a bit from how the STL one works,
    namely in the fact that it keeps the T* inside the
    control block, and that you can still make a CWeakPtr
    or deref an existing one inside the destructor.
*/

namespace Hyprutils::Memory {
    template <typename T>
    class CSharedPointer {
      public:
        template <typename X>
        using validHierarchy = std::enable_if_t<std::is_assignable_v<CSharedPointer<T>&, X>, CSharedPointer&>;
        template <typename X>
        using isConstructible = std::enable_if_t<std::is_constructible_v<T&, X&>>;

        /* creates a new shared pointer managing a resource
               avoid calling. Could duplicate ownership. Prefer makeShared */
        explicit CSharedPointer(T* object) noexcept : impl_(new Impl_::impl<T>(object)) {
            increment();
        }

        /* creates a shared pointer from a reference */
        template <typename U, typename = isConstructible<U>>
        CSharedPointer(const CSharedPointer<U>& ref) noexcept : impl_(ref.impl_) {
            increment();
        }

        CSharedPointer(const CSharedPointer& ref) noexcept : impl_(ref.impl_) {
            increment();
        }

        template <typename U, typename = isConstructible<U>>
        // reason - ref param not moved. But is that correct?
        CSharedPointer(CSharedPointer<U>& ref) noexcept {
            std::swap(impl_, ref.impl_);
        }

        CSharedPointer(CSharedPointer&& ref) noexcept {
            std::swap(impl_, ref.impl_);
        }

        /* allows weakPointer to create from an impl */
        CSharedPointer(Impl_::impl_base* implementation) noexcept : impl_(implementation) {
            increment();
        }

        /* creates an empty shared pointer with no implementation */
        CSharedPointer() noexcept {
            ; // empty
        }

        /* creates an empty shared pointer with no implementation */
        CSharedPointer(std::nullptr_t) noexcept {
            ; // empty
        }

        ~CSharedPointer() {
            decrement();
        }

        template <typename U>
        // Same. And also what should i do with the warning of unconventional assign operators?
        validHierarchy<const CSharedPointer<U>&> operator=(const CSharedPointer<U>& rhs) {
            if (impl_ == rhs.impl_)
                return *this;

            decrement();
            impl_ = rhs.impl_;
            increment();
            return *this;
        }
        //Self assignment warning. What to do?
        CSharedPointer& operator=(const CSharedPointer& rhs) {
            if (impl_ == rhs.impl_)
                return *this;

            decrement();
            impl_ = rhs.impl_;
            increment();
            return *this;
        }

        template <typename U>
        // Same
        validHierarchy<const CSharedPointer<U>&> operator=(CSharedPointer<U>& rhs) {
            std::swap(impl_, rhs.impl_);
            return *this;
        }

        CSharedPointer& operator=(CSharedPointer&& rhs) noexcept {
            std::swap(impl_, rhs.impl_);
            return *this;
        }

        operator bool() const {
            return impl_ && impl_->dataNonNull();
        }

        bool operator==(const CSharedPointer& rhs) const {
            return impl_ == rhs.impl_;
        }

        bool operator()(const CSharedPointer& lhs, const CSharedPointer& rhs) const {
            return reinterpret_cast<uintptr_t>(lhs.impl_) < reinterpret_cast<uintptr_t>(rhs.impl_);
        }

        bool operator<(const CSharedPointer& rhs) const {
            return reinterpret_cast<uintptr_t>(impl_) < reinterpret_cast<uintptr_t>(rhs.impl_);
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

        unsigned int strongRef() const {
            return impl_ ? impl_->ref() : 0;
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
    static CSharedPointer<U> makeShared(Args&&... args) {
        return CSharedPointer<U>(new U(std::forward<Args>(args)...));
    }
}

template <typename T>
struct std::hash<Hyprutils::Memory::CSharedPointer<T>> {
    std::size_t operator()(const Hyprutils::Memory::CSharedPointer<T>& p) const noexcept {
        return std::hash<void*>{}(p.impl_);
    }
};
