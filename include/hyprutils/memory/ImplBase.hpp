#pragma once

#include <memory>

//NOLINTNEXTLINE
namespace Hyprutils::Memory::Impl_ {
    //NOLINTNEXTLINE
    class impl_base {
      public:
        virtual ~impl_base() = default;

        virtual void         inc() noexcept         = 0;
        virtual void         dec() noexcept         = 0;
        virtual void         incWeak() noexcept     = 0;
        virtual void         decWeak() noexcept     = 0;
        virtual unsigned int ref() noexcept         = 0;
        virtual unsigned int wref() noexcept        = 0;
        virtual void         destroy() noexcept     = 0;
        virtual bool         destroying() noexcept  = 0;
        virtual bool         dataNonNull() noexcept = 0;
        virtual bool         lockable() noexcept    = 0;
        virtual void*        getData() noexcept     = 0;
    };

    template <typename T>
    //NOLINTNEXTLINE
    class impl : public impl_base {
      public:
        impl(T* data, bool lock = true) noexcept : _data(data), _lockable(lock) {
            ;
        }

        /* strong refcount */
        unsigned int _ref = 0;
        /* weak refcount */
        unsigned int _weak = 0;
        /* if this is lockable (shared) */
        bool        _lockable = true;

        T*          _data = nullptr;

        friend void swap(impl*& a, impl*& b) noexcept {
            impl* tmp = a;
            a         = b;
            b         = tmp;
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
            _deleter(_data);
            // now, we can reset the data and call it a day.
            _data       = nullptr;
            _destroying = false;
        }

        std::default_delete<T> _deleter{};

        //
        virtual void inc() noexcept {
            _ref++;
        }

        virtual void dec() noexcept {
            _ref--;
        }

        virtual void incWeak() noexcept {
            _weak++;
        }

        virtual void decWeak() noexcept {
            _weak--;
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

        virtual ~impl() {
            destroy();
        }
    };
}
