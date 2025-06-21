#pragma once

#include "./ImplBase.hpp"
#include <mutex>

/*
  Impl for Hyprutils shared pointers that does thread-safe reference counting.
  Instead of using atomic counters (e.g. std::atomic), this implementation just uses a mutex.
  That helps to keep the implementation simple and avoids compare_exchange loops.

  The implementation in ImplBase.hpp is perferred for single threaded contexts as it will be a bit faster than this one.

  Keep in mind that this implementation only ensures thread-safe reference counting of impl_. It does not protect the data itself.
  Doing a lock() in multithreaded context will guaranty that the data still exists.
  However, checking valid() on a weakpointer in a multithreaded context and accessing the data in the next line without any synchronization is not safe.
*/

namespace Hyprutils::Memory::Impl_ {
    template <typename T>
    class CAtomicImpl : public impl_base {
      public:
        CAtomicImpl(T* data, bool lock = true) noexcept : _lockable(lock), _data(data) {
            ;
        }

        CAtomicImpl(const CAtomicImpl&) = delete;
        CAtomicImpl(CAtomicImpl&&)      = delete;

        /* strong refcount */
        unsigned int _ref = 1;
        /* weak refcount */
        unsigned int _weak = 0;
        /* if this is lockable (shared) */
        bool        _lockable = true;

        std::mutex  _mtx;

        T*          _data = nullptr;

        friend void swap(CAtomicImpl*& a, CAtomicImpl*& b) noexcept {
            CAtomicImpl* tmp = a;
            a                = b;
            b                = tmp;
        }

        /* if the destructor was called, 
                   creating shared_ptrs is no longer valid */
        bool _destroying = false;

        void _destroy() {
            if (!_data || _destroying)
                return;

            // first, we destroy the data, but keep the pointer.
            // this way, weak pointers will still be able to
            // reference and use, but no longer create shared ones.
            _destroying = true;
            __deleter(_data);
            // now, we can reset the data and call it a day.
            _data       = nullptr;
            _destroying = false;
        }

        std::default_delete<T> __deleter{};

        //
        virtual bool inc() {
            std::lock_guard<std::mutex> lg(_mtx);
            if (_ref == 0)
                return false;

            _ref++;
            return true;
        }

        virtual bool dec() {
            std::lock_guard<std::mutex> lg(_mtx);
            _ref--;

            if (_ref == 0) {
                // if ref == 0, we can destroy impl
                _mtx.unlock();
                destroy();
                _mtx.lock();
                // if weak == 0, we tell the actual impl to delete this
                return _weak == 0;
            }

            return false;
        }

        virtual bool incWeak() {
            std::lock_guard<std::mutex> lg(_mtx);
            if (_ref == 0 && _weak == 0)
                return false;

            _weak++;
            return true;
        }

        virtual bool decWeak() {
            std::lock_guard<std::mutex> lg(_mtx);
            _weak--;

            // we need to check for _destroying,
            // because otherwise we could destroy here
            // and have a shared_ptr destroy the same thing
            // later (in situations where we have a weak_ptr to self)
            return _ref == 0 && _weak == 0 && !_destroying;
        }

        virtual unsigned int ref() noexcept {
            return _ref;
        }

        virtual unsigned int wref() noexcept {
            return _weak;
        }

        virtual void destroy() noexcept {
            _destroy();
        }

        virtual bool destroying() noexcept {
            return _destroying;
        }

        virtual bool lockable() noexcept {
            return _lockable;
        }

        virtual bool dataNonNull() noexcept {
            return _data != nullptr;
        }

        virtual void* getData() noexcept {
            return _data;
        }

        virtual ~CAtomicImpl() {
            _destroy();
        }
    };
}
