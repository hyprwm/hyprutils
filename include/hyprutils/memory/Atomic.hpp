#pragma once

#include "./ImplBase.hpp"
#include "./SharedPtr.hpp"
#include "./WeakPtr.hpp"
#include <mutex>

namespace Hyprutils::Memory {
    namespace Atomic_ {
        template <typename T>
        class impl : public Impl_::impl<T> {
            std::recursive_mutex m_mutex;

          public:
            impl(T* data, bool lock = true) noexcept : Impl_::impl<T>(data, lock) {
                ;
            }

            std::lock_guard<std::recursive_mutex> lockGuard() {
                return std::lock_guard<std::recursive_mutex>(m_mutex);
            }
        };
    }

    // Forward declaration for friend
    template <typename T>
    class CAtomicWeakPointer;

    template <typename T>
    class CAtomicSharedPointer {
        template <typename X>
        using isConstructible = typename std::enable_if<std::is_constructible<T&, X&>::value>::type;
        template <typename X>
        using validHierarchy = typename std::enable_if<std::is_assignable<CAtomicSharedPointer<T>&, X>::value, CAtomicSharedPointer&>::type;

      public:
        explicit CAtomicSharedPointer(Impl_::impl_base* impl) noexcept : m_ptr(impl) {}

        CAtomicSharedPointer(const CAtomicSharedPointer<T>& ref) {
            if (!ref.m_ptr.impl_)
                return;

            auto lg = ref.implLockGuard();
            m_ptr   = ref.m_ptr;
        }

        template <typename U, typename = isConstructible<U>>
        CAtomicSharedPointer(const CAtomicSharedPointer<U>& ref) {
            if (!ref.m_ptr.impl_)
                return;

            auto lg = ref.implLockGuard();
            m_ptr   = ref.m_ptr;
        }

        template <typename U, typename = isConstructible<U>>
        CAtomicSharedPointer(CAtomicSharedPointer<U>&& ref) noexcept {
            std::swap(m_ptr, ref.m_ptr);
        }

        CAtomicSharedPointer(CAtomicSharedPointer&& ref) noexcept {
            std::swap(m_ptr, ref.m_ptr);
        }

        CAtomicSharedPointer() noexcept {
            ; // empty
        }

        /* creates an empty shared pointer with no implementation */
        CAtomicSharedPointer(std::nullptr_t) noexcept {
            ; // empty
        }

        ~CAtomicSharedPointer() {
            if (!m_ptr.impl_)
                return;

            auto lg = implLockGuard();
            m_ptr.reset();
        }

        template <typename U>
        validHierarchy<const CAtomicSharedPointer<U>&> operator=(const CAtomicSharedPointer<U>& rhs) {
            if (m_ptr.impl_) {
                auto lg = implLockGuard();
                m_ptr.reset();
            }

            if (!rhs.m_ptr.impl_)
                return *this;

            auto lg = rhs.implLockGuard();
            m_ptr   = rhs.m_ptr;
            return *this;
        }

        CAtomicSharedPointer& operator=(const CAtomicSharedPointer& rhs) {
            if (this == &rhs)
                return *this;

            if (m_ptr.impl_) {
                auto lg = implLockGuard();
                m_ptr.reset();
            }

            if (!rhs.m_ptr.impl_)
                return *this;

            auto lg = rhs.implLockGuard();
            m_ptr   = rhs.m_ptr;
            return *this;
        }

        template <typename U>
        validHierarchy<const CAtomicSharedPointer<U>&> operator=(CAtomicSharedPointer<U>&& rhs) noexcept {
            std::swap(m_ptr, rhs.m_ptr);
            return *this;
        }

        CAtomicSharedPointer& operator=(CAtomicSharedPointer&& rhs) noexcept {
            if (this == &rhs)
                return *this;

            std::swap(m_ptr, rhs.m_ptr);
            return *this;
        }

        void reset() {
            if (!m_ptr.impl_)
                return;

            auto lg = implLockGuard();
            m_ptr.reset();
        }

        T& operator*() const {
            return *m_ptr;
        }

        T* operator->() const {
            return m_ptr.get();
        }

        T* get() const {
            return m_ptr.get();
        }

        operator bool() const {
            return m_ptr;
        }

        bool operator==(const CAtomicSharedPointer& rhs) const {
            return m_ptr == rhs.m_ptr;
        }

        bool operator()(const CAtomicSharedPointer& lhs, const CAtomicSharedPointer& rhs) const {
            return lhs.m_ptr == rhs.m_ptr;
        }

        unsigned int strongRef() const {
            return m_ptr.impl_ ? m_ptr.impl_->ref() : 0;
        }

      private:
        std::lock_guard<std::recursive_mutex> implLockGuard() const {
            return ((Atomic_::impl<T>*)m_ptr.impl_)->lockGuard();
        }

        CSharedPointer<T> m_ptr;

        template <typename U>
        friend class CAtomicWeakPointer;
        template <typename U>
        friend class CAtomicSharedPointer;
    };

    template <typename T>
    class CAtomicWeakPointer {

        template <typename X>
        using isConstructible = typename std::enable_if<std::is_constructible<T&, X&>::value>::type;
        template <typename X>
        using validHierarchy = typename std::enable_if<std::is_assignable<CAtomicWeakPointer<T>&, X>::value, CAtomicWeakPointer&>::type;

      public:
        CAtomicWeakPointer(const CAtomicWeakPointer<T>& ref) {
            if (!ref.m_ptr.impl_)
                return;

            auto lg = ref.implLockGuard();
            m_ptr   = ref.m_ptr;
        }

        template <typename U, typename = isConstructible<U>>
        CAtomicWeakPointer(const CAtomicWeakPointer<U>& ref) {
            if (!ref.m_ptr.impl_)
                return;

            auto lg = ref.implLockGuard();
            m_ptr   = ref.m_ptr;
        }

        template <typename U, typename = isConstructible<U>>
        CAtomicWeakPointer(CAtomicWeakPointer<U>&& ref) noexcept {
            std::swap(m_ptr, ref.m_ptr);
        }

        CAtomicWeakPointer(CAtomicWeakPointer&& ref) noexcept {
            std::swap(m_ptr, ref.m_ptr);
        }

        CAtomicWeakPointer(const CAtomicSharedPointer<T>& ref) {
            if (!ref.m_ptr.impl_)
                return;

            auto lg = ref.implLockGuard();
            m_ptr   = ref.m_ptr;
        }

        CAtomicWeakPointer() noexcept {
            ; // empty
        }

        /* creates an empty shared pointer with no implementation */
        CAtomicWeakPointer(std::nullptr_t) noexcept {
            ; // empty
        }

        ~CAtomicWeakPointer() {
            if (!m_ptr.impl_)
                return;

            auto lg = implLockGuard();
            m_ptr.reset();
        }

        template <typename U>
        validHierarchy<const CAtomicWeakPointer<U>&> operator=(const CAtomicWeakPointer<U>& rhs) {
            if (m_ptr.impl_) {
                auto lg = implLockGuard();
                m_ptr.reset();
            }

            auto lg = rhs.implLockGuard();
            m_ptr   = rhs.m_ptr;
            return *this;
        }

        CAtomicWeakPointer& operator=(const CAtomicWeakPointer& rhs) {
            if (this == &rhs)
                return *this;

            if (m_ptr.impl_) {
                auto lg = implLockGuard();
                m_ptr.reset();
            }

            auto lg = rhs.implLockGuard();
            m_ptr   = rhs.m_ptr;
            return *this;
        }

        template <typename U>
        validHierarchy<const CAtomicWeakPointer<U>&> operator=(CAtomicWeakPointer<U>&& rhs) noexcept {
            std::swap(m_ptr, rhs.m_ptr);
            return *this;
        }

        CAtomicWeakPointer& operator=(CAtomicWeakPointer&& rhs) noexcept {
            if (this == &rhs)
                return *this;

            std::swap(m_ptr, rhs.m_ptr);
            return *this;
        }

        void reset() {
            if (!m_ptr.impl_)
                return;

            auto lg = implLockGuard();
            m_ptr.reset();
        }

        T& operator*() const {
            return *m_ptr;
        }

        T* operator->() const {
            return m_ptr.get();
        }

        T* get() const {
            return m_ptr.get();
        }

        operator bool() const {
            return m_ptr;
        }

        bool operator==(const CAtomicWeakPointer& rhs) const {
            return m_ptr == rhs.m_ptr;
        }

        bool operator==(const CAtomicSharedPointer<T>& rhs) const {
            return m_ptr == rhs.m_ptr;
        }

        bool operator()(const CAtomicWeakPointer& lhs, const CAtomicWeakPointer& rhs) const {
            return lhs.m_ptr == rhs.m_ptr;
        }

        bool expired() {
            return m_ptr.expired();
        }

        bool valid() {
            return m_ptr.valid();
        }

        CAtomicSharedPointer<T> lock() const {
            if (!m_ptr.impl_)
                return {};

            auto lg = implLockGuard();

            if (!m_ptr.impl_->dataNonNull() || m_ptr.impl_->destroying() || !m_ptr.impl_->lockable())
                return {};

            return CAtomicSharedPointer<T>(m_ptr.impl_);
        }

      private:
        std::lock_guard<std::recursive_mutex> implLockGuard() const {
            return ((Atomic_::impl<T>*)m_ptr.impl_)->lockGuard();
        }

        CWeakPointer<T> m_ptr;

        template <typename U>
        friend class CAtomicWeakPointer;
        template <typename U>
        friend class CAtomicSharedPointer;
    };

    template <typename U, typename... Args>
    static CAtomicSharedPointer<U> makeAtomicShared(Args&&... args) {
        return CAtomicSharedPointer<U>(new Atomic_::impl<U>(new U(std::forward<Args>(args)...)));
    }
}
