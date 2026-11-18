# Signal

```cpp
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/signal/Listener.hpp>

using namespace Hyprutils::Signal;
```

```{cpp:namespace} Hyprutils::Signal
```

The signal module implements a synchronous, single-threaded observer pattern. An
object exposes one {cpp:class}`CSignalT` per event it can produce, and interested
parties subscribe to it. Emitting calls every registered handler inline, in
registration order, and returns once they have all run. There is no queue, no
dispatch thread, and no allocation per emission.

The module has two halves:

{doc}`signals`
: {cpp:class}`CSignalT`, the emitter. It is a variadic template over the types
  the event carries, so handler signatures are checked at compile time instead of
  unpacking a `std::any` at runtime.

{doc}`listeners`
: {cpp:class}`CSignalListener`, always held through the
  {cpp:type}`CHyprSignalListener` shared pointer. Ownership of that pointer *is*
  the subscription: when the last strong reference goes away, the handler stops
  being called.

## A minimal example

```cpp
CSignalT<>    ready;
CSignalT<int> valueChanged;

// subscribe; keep the returned handle alive for as long as you want events
CHyprSignalListener onChange = valueChanged.listen([](int value) {
    printf("value is now %d\n", value);
});

valueChanged.emit(42);   // prints
onChange.reset();        // unsubscribe
valueChanged.emit(7);    // nothing happens
```

## The ownership model

The signal holds a *weak* reference to each listener; the subscriber holds the
*strong* one. That inversion is the central design decision of the module, and it
has two consequences worth internalising up front.

**Discarding the result of {cpp:func}`CSignalT::listen` unsubscribes
immediately.** The temporary shared pointer dies at the end of the full
expression, taking the listener with it, and the handler never runs. `listen()`
is marked `[[nodiscard]]` for exactly this reason.

```cpp
signal.listen([] { /* never called */ });        // wrong, and warns
auto listener = signal.listen([] { /* ok */ });  // right
```

**Subscriptions are torn down by the subscriber's destructor, for free.** Store
the handle in the object whose lifetime should bound the subscription and there
is nothing to unregister by hand, no callback that can fire into a destroyed
object, and no disconnect API to forget.

## The idiomatic shape

Producers group their signals in an anonymous struct member, so the event set
reads as a unit and callers write `obj.m_events.something`:

```cpp
class CDevice {
  public:
    struct SButtonEvent {
        uint32_t timeMs = 0;
        uint32_t button = 0;
        bool     pressed = false;
    };

    struct {
        CSignalT<SButtonEvent> button;
        CSignalT<>             frame;
        CSignalT<>             destroy;
    } m_events;
};
```

Note that events carrying data almost always carry a *single payload struct*
rather than a list of loose parameters. Adding a field to the struct is source
compatible with every existing handler; adding a template parameter to the signal
is not.

Consumers mirror that with a struct of handles, named after the signals they are
bound to:

```cpp
class CDeviceWatcher {
  public:
    explicit CDeviceWatcher(CDevice& device) {
        m_listeners.button = device.m_events.button.listen([this](const CDevice::SButtonEvent& event) {
            onButton(event);
        });
        m_listeners.destroy = device.m_events.destroy.listen([this] { onDestroy(); });
    }

    // no destructor needed: m_listeners dies with *this, so the lambdas
    // capturing `this` can never be invoked afterwards

  private:
    void onButton(const CDevice::SButtonEvent& event);
    void onDestroy();

    struct {
        CHyprSignalListener button;
        CHyprSignalListener destroy;
    } m_listeners;
};
```

When the set of subscriptions is not known at compile time, a
`std::vector<CHyprSignalListener>` or a map keyed by name works the same way:
erasing the element unsubscribes. See {ref}`storing-listeners`.

The one place the ownership model does not apply is
{cpp:func}`CSignalT::listenStatic`, which hands the signal ownership of the
handler. See {ref}`static-listeners` for when that is the right call.

## Thread safety

There is none. Neither {cpp:class}`CSignalT` nor {cpp:class}`CSignalListener`
synchronises anything: the listener lists are plain vectors and the reference
counts behind {cpp:type}`CHyprSignalListener` are non-atomic. Register, emit, and
destroy on one thread. To cross a thread boundary, hand the event to your event
loop and emit from the loop's thread.

```{toctree}
:maxdepth: 2

signals
listeners
```
