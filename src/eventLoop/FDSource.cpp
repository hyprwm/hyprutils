#include "FDSource.hpp"

#include "EventLoop.hpp"

using namespace Hyprutils::EventLoop;
using namespace Hyprutils::OS;

CFDSource::CFDSource(CEventLoop& loop, uintptr_t id, CFileDescriptor&& fd, FdEventMask mask, std::function<void(IFDSource&, FdEventMask)>&& callback) :
    m_loop(&loop), m_id(id), m_fd(std::move(fd)), m_mask(mask), m_callback(Memory::makeShared<std::function<void(IFDSource&, FdEventMask)>>(std::move(callback))) {}

std::expected<void, std::string> CFDSource::setMask(FdEventMask mask) {
    if (!m_loop)
        return std::unexpected("file descriptor source has been removed");

    auto result = m_loop->updateSourceMask(*this, mask);
    if (result)
        m_mask = mask;
    return result;
}

void CFDSource::remove() {
    if (m_loop)
        m_loop->removeSource(*this);
}

const CFileDescriptor& CFDSource::fd() const {
    return m_fd;
}

FdEventMask CFDSource::mask() const {
    return m_mask;
}

uintptr_t CFDSource::id() const {
    return m_id;
}

bool CFDSource::removed() const {
    return !m_loop;
}

void CFDSource::call(FdEventMask events) {
    auto callback = m_callback;
    if (callback && *callback)
        (*callback)(*this, events);
}

void CFDSource::detach() {
    m_loop     = nullptr;
    m_callback = {};
    m_mask     = eEventMask::EMPTY;
    m_fd.reset();
}
