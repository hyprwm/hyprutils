#pragma once
#include <cstdint>

#include "ImplBase.hpp"
#include "Casts.hpp"

/*
    This is a custom impl of std::shared_ptr.
    It is not thread-safe like the STL one,
    but Hyprland is single-threaded anyways.

    It differs a bit from how the STL one works,
    namely in the fact that it keeps the T* inside the
    control block, and that you can still make a CWeakPtr
    or deref an existing one inside the destructor.
*/

namespace Hyprutils {
    namespace Memory {

        template <typename T>
        class CSharedPointer {
          public:
            template <typename X>
            using validHierarchy = std::enable_if_t<std::is_assignable_v<CSharedPointer<T>&, X>, CSharedPointer&>;
            template <typename X>
            using isConstructible = std::enable_if_t<std::is_constructible_v<T&, X&>>;

            /* creates a new shared pointer managing a resource
               avoid calling. Could duplicate ownership. Prefer makeShared */
            explicit CSharedPointer(T* object) noexcept : impl_(new Impl_::impl_base(Impl_::dataPointer(object), _delete)), m_data(Impl_::dataPointer(object)) {
                increment();
            }

            /* creates a shared pointer from a reference */
            template <typename U, typename = isConstructible<U>>
            CSharedPointer(const CSharedPointer<U>& ref) noexcept : impl_(ref.impl_), m_data(Impl_::dataPointer(sc<T*>(ref.get()))) {
                increment();
            }

            CSharedPointer(const CSharedPointer& ref) noexcept : impl_(ref.impl_), m_data(ref.m_data) {
                increment();
            }

            template <typename U, typename = isConstructible<U>>
            CSharedPointer(CSharedPointer<U>&& ref) noexcept {
                impl_       = ref.impl_;
                m_data      = Impl_::dataPointer(sc<T*>(ref.get()));
                ref.impl_   = nullptr;
                ref.m_data  = nullptr;
            }

            CSharedPointer(CSharedPointer&& ref) noexcept {
                std::swap(impl_, ref.impl_);
                std::swap(m_data, ref.m_data);
            }

            /* allows weakPointer to create from an impl */
            CSharedPointer(Impl_::impl_base* implementation, void* data) noexcept : impl_(implementation), m_data(data) {
                increment();
            }

            /* creates an empty shared pointer with no implementation */
            CSharedPointer() noexcept = default;

            /* creates an empty shared pointer with no implementation */
            CSharedPointer(std::nullptr_t) noexcept {
                ; // empty
            }

            ~CSharedPointer() {
                decrement(impl_);
            }

            template <typename U>
            validHierarchy<const CSharedPointer<U>&> operator=(const CSharedPointer<U>& rhs) {
                if (impl_ == rhs.impl_) {
                    m_data = Impl_::dataPointer(sc<T*>(rhs.get()));
                    return *this;
                }

                decrement(impl_);
                impl_  = rhs.impl_;
                m_data = Impl_::dataPointer(sc<T*>(rhs.get()));
                increment();
                return *this;
            }

            CSharedPointer& operator=(const CSharedPointer& rhs) {
                if (impl_ == rhs.impl_)
                    return *this;

                decrement(impl_);
                impl_  = rhs.impl_;
                m_data = rhs.m_data;
                increment();
                return *this;
            }

            template <typename U>
            validHierarchy<const CSharedPointer<U>&> operator=(CSharedPointer<U>&& rhs) {
                auto* rhsData = rhs.get();

                std::swap(impl_, rhs.impl_);
                std::swap(m_data, rhs.m_data);
                m_data = Impl_::dataPointer(sc<T*>(rhsData));
                return *this;
            }

            CSharedPointer& operator=(CSharedPointer&& rhs) noexcept {
                std::swap(impl_, rhs.impl_);
                std::swap(m_data, rhs.m_data);
                return *this;
            }

            explicit operator bool() const {
                return impl_ && impl_->dataNonNull();
            }

            // this compares that the pointed-to object is the same, but in multiple inheritance,
            // different typed pointers can be equal if the object is the same
            bool operator==(const CSharedPointer& rhs) const {
                return impl_ == rhs.impl_;
            }

            bool operator()(const CSharedPointer& lhs, const CSharedPointer& rhs) const {
                return rc<uintptr_t>(lhs.impl_) < rc<uintptr_t>(rhs.impl_);
            }

            bool operator<(const CSharedPointer& rhs) const {
                return rc<uintptr_t>(impl_) < rc<uintptr_t>(rhs.impl_);
            }

            T* operator->() const {
                return get();
            }

            T& operator*() const {
                return *get();
            }

            void reset() {
                auto ptr = impl_;
                impl_    = nullptr;
                m_data   = nullptr;
                decrement(ptr);
            }

            T* get() const {
                return impl_ && impl_->dataNonNull() ? sc<T*>(m_data) : nullptr;
            }

            unsigned int strongRef() const {
                return impl_ ? impl_->ref() : 0;
            }

            Impl_::impl_base* impl_ = nullptr;

            // Never use directly: raw data ptr, could be UAF
            void* m_data = nullptr;

          private:
            static void _delete(void* p) {
                std::default_delete<T>{}(sc<T*>(p));
            }

            /*
                no-op if there is no impl_
                may delete the stored object if ref == 0
                may delete and reset impl_ if ref == 0 and weak == 0
            */
            void decrement(Impl_::impl_base* base) {
                if (!base)
                    return;

                base->dec();

                // if ref == 0, we can destroy impl
                if (base->ref() == 0)
                    destroyImpl(base);
            }
            /* no-op if there is no impl_ */
            void increment() {
                if (!impl_)
                    return;

                impl_->inc();
            }

            /* destroy the pointed-to object
               if able, will also destroy impl */
            void destroyImpl(Impl_::impl_base* base) {
                // this call can destroy this, so we need to not use thisptr anymore
                base->destroy();

                // check for weak refs, if zero, we can also delete base
                if (base->wref() == 0)
                    delete base;
            }
        };

        template <typename U, typename... Args>
        [[nodiscard]] inline CSharedPointer<U> makeShared(Args&&... args) {
            return CSharedPointer<U>(new U(std::forward<Args>(args)...));
        }

        template <typename T, typename U>
        CSharedPointer<T> reinterpretPointerCast(const CSharedPointer<U>& ref) {
            return CSharedPointer<T>(ref.impl_, ref.m_data);
        }

        template <typename T, typename U>
        CSharedPointer<T> dynamicPointerCast(const CSharedPointer<U>& ref) {
            if (!ref)
                return nullptr;
            T* newPtr = dynamic_cast<T*>(ref.get());
            if (!newPtr)
                return nullptr;
            return CSharedPointer<T>(ref.impl_, Impl_::dataPointer(newPtr));
        }
    }
}

template <typename T>
struct std::hash<Hyprutils::Memory::CSharedPointer<T>> {
    std::size_t operator()(const Hyprutils::Memory::CSharedPointer<T>& p) const noexcept {
        return std::hash<void*>{}(p.impl_);
    }
};
