#include <hyprutils/eventLoop/EventLoop.hpp>

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace Hyprutils::EventLoop;
using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;
using namespace std::chrono_literals;

namespace {
    struct SPipe {
        CFileDescriptor read;
        CFileDescriptor write;
    };

    void configureFD(int fd) {
        const int descriptorFlags = fcntl(fd, F_GETFD);
        ASSERT_GE(descriptorFlags, 0);
        ASSERT_EQ(fcntl(fd, F_SETFD, descriptorFlags | FD_CLOEXEC), 0);

        const int statusFlags = fcntl(fd, F_GETFL);
        ASSERT_GE(statusFlags, 0);
        ASSERT_EQ(fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK), 0);
    }

    SPipe makePipe() {
        std::array<int, 2> fds = {-1, -1};
        EXPECT_EQ(pipe(fds.data()), 0);
        configureFD(fds[0]);
        configureFD(fds[1]);
        return {CFileDescriptor{fds[0]}, CFileDescriptor{fds[1]}};
    }

    std::array<CFileDescriptor, 2> makeSocketPair() {
        std::array<int, 2> sockets = {-1, -1};
        EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets.data()), 0);
        configureFD(sockets[0]);
        configureFD(sockets[1]);
        return {CFileDescriptor{sockets[0]}, CFileDescriptor{sockets[1]}};
    }

    bool readable(int fd, int timeout = 1000) {
        pollfd pollFD = {
            .fd      = fd,
            .events  = POLLIN,
            .revents = 0,
        };
        return poll(&pollFD, 1, timeout) == 1 && (pollFD.revents & POLLIN);
    }
}

TEST(EventLoop, FDDispatchAndOwnership) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto loop = std::move(*loopResult);

    auto pipe   = makePipe();
    int  readFD = pipe.read.get();
    int  calls  = 0;

    auto sourceResult = loop->addFD(std::move(pipe.read), eEventMask::READABLE, [&](IFDSource&, FdEventMask events) {
        EXPECT_TRUE(events & eEventMask::READABLE);
        char byte = 0;
        EXPECT_EQ(read(readFD, &byte, 1), 1);
        ++calls;
    });
    ASSERT_TRUE(sourceResult);
    auto       source = *sourceResult;

    const char bytes[] = {'a', 'b'};
    ASSERT_EQ(write(pipe.write.get(), bytes, sizeof(bytes)), 2);
    ASSERT_TRUE(readable(loop->fd()));
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(calls, 2);

    source->remove();
    EXPECT_FALSE(source->fd().isValid());
    EXPECT_EQ(fcntl(readFD, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
    EXPECT_FALSE(source->setMask(eEventMask::READABLE));
}

TEST(EventLoop, DynamicSourcesAndMasks) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto                      loop = std::move(*loopResult);

    auto                      firstPipe  = makePipe();
    auto                      secondPipe = makePipe();
    int                       firstFD    = firstPipe.read.get();
    int                       secondFD   = secondPipe.read.get();
    int                       calls      = 0;
    CSharedPointer<IFDSource> secondSource;

    const char                byte = 'x';
    ASSERT_EQ(write(secondPipe.write.get(), &byte, 1), 1);

    auto firstSource = loop->addFD(std::move(firstPipe.read), eEventMask::READABLE, [&](IFDSource& self, FdEventMask) {
        char value = 0;
        EXPECT_EQ(read(firstFD, &value, 1), 1);
        ++calls;
        self.remove();

        auto added = loop->addFD(std::move(secondPipe.read), eEventMask::READABLE, [&](IFDSource& second, FdEventMask) {
            char secondValue = 0;
            EXPECT_EQ(read(secondFD, &secondValue, 1), 1);
            ++calls;
            second.remove();
        });
        ASSERT_TRUE(added);
        secondSource = *added;
    });
    ASSERT_TRUE(firstSource);

    ASSERT_EQ(write(firstPipe.write.get(), &byte, 1), 1);
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(calls, 2);

    auto sockets       = makeSocketPair();
    int  writableCalls = 0;

    auto writable = loop->addFD(std::move(sockets[0]), eEventMask::WRITABLE, [&](IFDSource& source, FdEventMask events) {
        EXPECT_TRUE(events & eEventMask::WRITABLE);
        ++writableCalls;
        EXPECT_TRUE(source.setMask(eEventMask::EMPTY));
    });
    ASSERT_TRUE(writable);
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(writableCalls, 1);
    ASSERT_TRUE((*writable)->setMask(eEventMask::WRITABLE));
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(writableCalls, 2);
    EXPECT_FALSE((*writable)->setMask(eEventMask::ERROR));
}

TEST(EventLoop, MutableCallbacksAndHangup) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto             loop = std::move(*loopResult);

    auto             pipe   = makePipe();
    int              readFD = pipe.read.get();
    std::vector<int> states;
    auto             source = loop->addFD(std::move(pipe.read), eEventMask::READABLE, [readFD, state = 0, &states](IFDSource&, FdEventMask) mutable {
        char byte = 0;
        EXPECT_EQ(read(readFD, &byte, 1), 1);
        states.emplace_back(++state);
    });
    ASSERT_TRUE(source);

    const char bytes[] = {'a', 'b'};
    ASSERT_EQ(write(pipe.write.get(), bytes, sizeof(bytes)), 2);
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(states, (std::vector<int>{1, 2}));

    auto sockets      = makeSocketPair();
    bool hungUp       = false;
    auto hangupSource = loop->addFD(std::move(sockets[0]), eEventMask::READABLE, [&](IFDSource& self, FdEventMask events) {
        hungUp = static_cast<bool>(events & eEventMask::HUP);
        self.remove();
    });
    ASSERT_TRUE(hangupSource);
    ASSERT_EQ(shutdown(sockets[1].get(), SHUT_WR), 0);
    ASSERT_TRUE(loop->dispatch());
    EXPECT_TRUE(hungUp);
}

TEST(EventLoop, TimersAndIdleSnapshots) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto loop = std::move(*loopResult);

    int  timerCalls = 0;
    auto timer      = loop->addTimer(0ns, [&](ITimer& self) {
        ++timerCalls;
        if (timerCalls == 1)
            self.updateTimeout(1h);
    });

    ASSERT_TRUE(readable(loop->fd()));
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(timerCalls, 1);
    timer->updateTimeout(0ns);
    ASSERT_TRUE(readable(loop->fd()));
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(timerCalls, 2);

    int  droppedCalls = 0;
    auto dropped      = loop->addTimer(0ns, [&](ITimer&) { ++droppedCalls; });
    dropped.reset();
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(droppedCalls, 0);

    int idleCalls = 0;
    loop->addIdle([&] {
        ++idleCalls;
        loop->addIdle([&] { ++idleCalls; });
    });

    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(idleCalls, 1);
    ASSERT_TRUE(readable(loop->fd()));
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(idleCalls, 2);
}

TEST(EventLoop, OrderingAndPostDispatchHooks) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto        loop = std::move(*loopResult);

    std::string order;
    auto        pipe   = makePipe();
    int         readFD = pipe.read.get();
    auto        source = loop->addFD(std::move(pipe.read), eEventMask::READABLE, [&](IFDSource& self, FdEventMask) {
        char byte = 0;
        EXPECT_EQ(read(readFD, &byte, 1), 1);
        order += 'f';
        loop->addIdle([&] { order += 'i'; });
        self.remove();
    });
    ASSERT_TRUE(source);

    auto hook = loop->addPostDispatch([&] { order += 'p'; });
    ASSERT_TRUE(hook);

    const char byte = 'x';
    ASSERT_EQ(write(pipe.write.get(), &byte, 1), 1);
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(order, "fip");

    hook.reset();
    loop->addIdle([&] { order += 'x'; });
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(order, "fipx");
}

TEST(EventLoop, ExecutorAndDetachedHandles) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto             loop     = std::move(*loopResult);
    auto             executor = loop->executor();

    std::atomic<int> calls = 0;
    std::thread      worker([executor, &calls] { executor->post([&calls] { ++calls; }); });
    worker.join();

    ASSERT_TRUE(readable(loop->fd()));
    ASSERT_TRUE(loop->dispatch());
    EXPECT_EQ(calls.load(), 1);

    auto pipe         = makePipe();
    auto sourceResult = loop->addFD(std::move(pipe.read), eEventMask::READABLE, [](IFDSource&, FdEventMask) {});
    ASSERT_TRUE(sourceResult);
    auto source = *sourceResult;
    auto timer  = loop->addTimer(1s, [](ITimer&) {});

    loop.reset();
    EXPECT_FALSE(source->fd().isValid());
    EXPECT_FALSE(source->setMask(eEventMask::READABLE));
    source->remove();
    timer->updateTimeout(1ms);
    timer.reset();

    std::thread lateWorker([executor, &calls] { executor->post([&calls] { ++calls; }); });
    lateWorker.join();
    EXPECT_EQ(calls.load(), 1);
}

TEST(EventLoop, CallbackCanReleaseLastLoopReference) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto loop    = std::move(*loopResult);
    auto rawLoop = loop.get();
    bool called  = false;

    loop->addIdle([&] {
        called = true;
        loop.reset();
    });

    EXPECT_TRUE(rawLoop->dispatch());
    EXPECT_TRUE(called);
    EXPECT_FALSE(loop);
}

TEST(EventLoop, CallbackExceptionsRestoreDispatchState) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto loop = std::move(*loopResult);

    bool deferredCalled = false;
    loop->addIdle([&] {
        loop->addIdle([&] { deferredCalled = true; });
        throw std::runtime_error("callback failure");
    });

    EXPECT_THROW(
        {
            auto result = loop->dispatch();
            (void)result;
        },
        std::runtime_error);
    ASSERT_TRUE(readable(loop->fd()));
    EXPECT_TRUE(loop->dispatch());
    EXPECT_TRUE(deferredCalled);
}

TEST(EventLoop, EnterLoopCanStopThroughExecutor) {
    auto loopResult = IEventLoop::create();
    ASSERT_TRUE(loopResult);
    auto        loop     = std::move(*loopResult);
    auto        executor = loop->executor();
    auto        rawLoop  = loop.get();

    std::thread worker([executor, rawLoop] { executor->post([rawLoop] { rawLoop->stop(); }); });
    ASSERT_TRUE(loop->enterLoop());
    worker.join();
}
