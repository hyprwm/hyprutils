#include <hyprutils/utils/ScopeGuard.hpp>

using namespace Hyprutils::Utils;

Hyprutils::Utils::CScopeGuard::CScopeGuard(const std::function<void()>& fn_) : fn(fn_) {
    ;
}

Hyprutils::Utils::CScopeGuard::~CScopeGuard() {
    if (fn)
        fn();
}
