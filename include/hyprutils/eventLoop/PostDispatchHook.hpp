#pragma once

namespace Hyprutils::EventLoop {
    class IPostDispatchHook {
      public:
        virtual ~IPostDispatchHook() = default;

        IPostDispatchHook(const IPostDispatchHook&)            = delete;
        IPostDispatchHook(IPostDispatchHook&&)                 = delete;
        IPostDispatchHook& operator=(const IPostDispatchHook&) = delete;
        IPostDispatchHook& operator=(IPostDispatchHook&&)      = delete;

      private:
        IPostDispatchHook() = default;
        friend class CPostDispatchHook;
    };
}
