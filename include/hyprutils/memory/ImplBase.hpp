#pragma once

#include <memory>

namespace Hyprutils {
    namespace Memory {
        namespace Impl_ {
            // Control block implementation interface for hyprutils smart pointers.
            class impl_base {
              public:
                virtual ~impl_base() = default;

                // If inc returns false, remove the pointer to this impl (but don't delete).
                virtual bool inc() = 0;
                // If dec returns true, the callee owned the last reference (strong and weak).
                // Thus the impl must be deleted and any reference to it must be invalidated.
                virtual bool dec() = 0;

                // If incWeak returns false, remove the pointer to this impl (but don't delete).
                virtual bool incWeak() = 0;
                // If dec returns true, there is no strong ref and the callee owned the last weak reference.
                // Thus the impl must be deleted and any reference to it must be invalidated.
                virtual bool         decWeak() = 0;

                virtual unsigned int ref() noexcept         = 0;
                virtual unsigned int wref() noexcept        = 0;
                virtual void         destroy() noexcept     = 0;
                virtual bool         destroying() noexcept  = 0;
                virtual bool         dataNonNull() noexcept = 0;
                virtual bool         lockable() noexcept    = 0;
                virtual void*        getData() noexcept     = 0;
            };

            template <typename T>
            class impl : public impl_base {
              public:
                impl(T* data, bool lock = true) noexcept : _lockable(lock), _data(data) {
                    ;
                }

                impl(const impl&) = delete;
                impl(impl&&)      = delete;

                /* strong refcount */
                unsigned int _ref = 1;
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
                    __deleter(_data);
                    // now, we can reset the data and call it a day.
                    _data       = nullptr;
                    _destroying = false;
                }

                std::default_delete<T> __deleter{};

                //
                virtual bool inc() {
                    if (_ref == 0)
                        return false;

                    _ref++;
                    return true;
                }

                virtual bool dec() {
                    _ref--;

                    if (_ref == 0) {
                        // if ref == 0, we can destroy impl
                        destroy();
                        // if weak == 0, we tell the actual impl to delete this
                        return _weak == 0;
                    }

                    return false;
                }

                virtual bool incWeak() {
                    if (_ref == 0)
                        return false;

                    _weak++;
                    return true;
                }

                virtual bool decWeak() {
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

                virtual ~impl() {
                    _destroy();
                }
            };
        }
    }

}
