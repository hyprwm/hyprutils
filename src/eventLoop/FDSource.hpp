#pragma once

#include "hyprutils/eventLoop/EventMask.hpp"
#include <hyprutils/eventLoop/FDSource.hpp>
#include <hyprutils/memory/SharedPtr.hpp>

#include <cstdint>
#include <functional>

namespace Hyprutils::EventLoop {
    class CEventLoop;

    class CFDSource : public IFDSource {
      public:
        CFDSource(CEventLoop& loop, uintptr_t id, OS::CFileDescriptor&& fd, FdEventMask mask, std::function<void(IFDSource&, FdEventMask)>&& callback);

        virtual ~CFDSource() = default;

        virtual std::expected<void, std::string> setMask(FdEventMask mask) override;
        virtual void                             remove() override;
        virtual const OS::CFileDescriptor&       fd() const override;

        FdEventMask                              mask() const;
        uintptr_t                                id() const;
        bool                                     removed() const;
        void                                     call(FdEventMask events);
        void                                     detach();

      private:
        CEventLoop*                                                          m_loop = nullptr;
        uintptr_t                                                            m_id   = 0;
        OS::CFileDescriptor                                                  m_fd;
        FdEventMask                                                          m_mask = eEventMask::EMPTY;
        Memory::CSharedPointer<std::function<void(IFDSource&, FdEventMask)>> m_callback;
    };
}
