#pragma once

#include "./ImplBase.hpp"
#include "./SharedPtr.hpp"
#include "./WeakPtr.hpp"
#include <mutex>

/*
    This header provides a thread-safe wrapper for Hyprutils shared pointer implementations.
    Like with STL shared pointers, that does not mean that individual SP/WP objects can be shared across threads without synchronization.
    It only means that the refcounting of the data is thread-safe.

    Should an Atomic SP/WP be shared across threads, calling a non-const member leads to a data race.
    To avoid that, each thread should have thread-local SP/WP objects.

    Example:
      We have a CAtomicSharedPointer member in a class. Suppose this member is accessed by multiple threads and is not constant.
      In such a case we need external synchronization to ensure valid data access.
      However, if we create a copy of this CAtomicWeakPointer member for each thread that accesses it,
      then the references to the object will be counted in a thread-safe manner and it will be safe to lock a WP and to access the data in case of an SP.
      In such an example, the inner data would need its own synchronization mechanism if it isn't constant itself.
*/

namespace Hyprutils::Memory {
    namespace Atomic_ {
        // Lock guard with the option to unlock early
        class CCLockGuard {
            std::recursive_mutex* m_mutex;

          public:
            CCLockGuard(std::recursive_mutex* mutex) : m_mutex(mutex) {
                m_mutex->lock();
            }

            ~CCLockGuard() {
                unlockDrop();
            };

            void unlockDrop() {
                if (m_mutex)
                    m_mutex->unlock();

                m_mutex = nullptr;
            }
        };

        template <typename T>
        class impl : public Impl_::impl<T> {
            std::recursive_mutex m_mutex;

          public:
            impl(T* data, bool lock = true) noexcept : Impl_::impl<T>(data, lock) {
                ;
            }

            CCLockGuard lockGuard() {
                return CCLockGuard(&m_mutex);
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

        CAtomicSharedPointer(std::nullptr_t) noexcept {
            ; // empty
        }

        ~CAtomicSharedPointer() {
            reset();
        }

        template <typename U>
        validHierarchy<const CAtomicSharedPointer<U>&> operator=(const CAtomicSharedPointer<U>& rhs) {
            reset();

            if (!rhs.m_ptr.impl_)
                return *this;

            auto lg = rhs.implLockGuard();
            m_ptr   = rhs.m_ptr;
            return *this;
        }

        CAtomicSharedPointer& operator=(const CAtomicSharedPointer& rhs) {
            if (this == &rhs)
                return *this;

            reset();

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
            if (m_ptr.impl_->ref() > 1) {
                m_ptr.reset();
                return;
            }

            if (m_ptr.impl_->wref() == 0) {
                lg.unlockDrop(); // Don't hold the mutex when destroying it

                m_ptr.reset();
                return;
            }

            // When the control block gets destroyed, the mutex is destroyed with it.
            // Thus we must avoid attempting an unlock after impl_ has been destroyed.
            // There is no safe way of checking whether it has been destroyed or not.
            //
            // To avoid this altogether, keep a weak pointer here.
            // This guarantees that impl_ is still valid after the reset.
            CWeakPointer<T> guard = m_ptr;
            m_ptr.reset();

            // Now we can safely check if guard is the last wref.
            if (guard.impl_->wref() == 1)
                lg.unlockDrop();

            // guard.reset()
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
        Atomic_::CCLockGuard implLockGuard() const {
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

        CAtomicWeakPointer(std::nullptr_t) noexcept {
            ; // empty
        }

        ~CAtomicWeakPointer() {
            reset();
        }

        template <typename U>
        validHierarchy<const CAtomicWeakPointer<U>&> operator=(const CAtomicWeakPointer<U>& rhs) {
            reset();

            auto lg = rhs.implLockGuard();
            m_ptr   = rhs.m_ptr;
            return *this;
        }

        CAtomicWeakPointer& operator=(const CAtomicWeakPointer& rhs) {
            if (this == &rhs)
                return *this;

            reset();

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
            if (m_ptr.impl_->ref() == 0 && m_ptr.impl_->wref() == 1)
                lg.unlockDrop();

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
        Atomic_::CCLockGuard implLockGuard() const {
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
