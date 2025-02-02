#pragma once

#include <functional>

//NOLINTNEXTLINE
namespace Hyprutils::Utils {
    // calls a function when it goes out of scope
    class CScopeGuard {
      public:
        CScopeGuard(const std::function<void()>& fn_);
        ~CScopeGuard();

      private:
        std::function<void()> fn;
    };
};
