#pragma once

#include <cstdint>
#include <memory>

namespace Hyprutils {
    namespace Memory {
        namespace Impl_ {
            class impl_base {
              public:
                using DeleteFn = void (*)(void*);

                impl_base(void* data, DeleteFn deleter, bool lock = true) noexcept : _lockable(lock), _data(data), _deleter(deleter) {
                    ;
                }

                void inc() noexcept {
                    _ref++;
                }

                void dec() noexcept {
                    _ref--;
                }

                void incWeak() noexcept {
                    _weak++;
                }

                void decWeak() noexcept {
                    _weak--;
                }

                unsigned int ref() noexcept {
                    return _ref;
                }

                unsigned int wref() noexcept {
                    return _weak;
                }

                void destroy() noexcept {
                    _destroy();
                }

                bool destroying() noexcept {
                    return _destroying;
                }

                bool lockable() noexcept {
                    return _lockable;
                }

                bool dataNonNull() noexcept {
                    return _data != nullptr;
                }

                void* getData() noexcept {
                    return _data;
                }

                ~impl_base() {
                    destroy();
                }

              private:
                /* strong refcount */
                unsigned int _ref = 0;
                /* weak refcount */
                unsigned int _weak = 0;
                /* if this is lockable (shared) */
                bool  _lockable = true;

                void* _data = nullptr;

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

                DeleteFn _deleter = nullptr;
            };
        }
    }

}
