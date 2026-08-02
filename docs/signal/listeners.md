# Listeners

```cpp
#include <hyprutils/signal/Listener.hpp>
```

```{cpp:namespace} Hyprutils::Signal
```

A listener is one handler registered on one signal. You never construct one
directly; {cpp:func}`CSignalT::listen` and {cpp:func}`CSignalT::forward` return
one, and holding on to the returned handle is what keeps the subscription alive.

::::{cpp:type} CHyprSignalListener = Hyprutils::Memory::CSharedPointer<CSignalListener>

The handle. This is the type you declare, store and reset; the class it points at
has no public API worth calling.

It is a shared pointer, so it behaves like one:

```cpp
CHyprSignalListener listener;          // empty, subscribes to nothing
if (!listener)                         // testable
    listener = signal.listen(handler); // assignable later

listener.reset();                      // unsubscribe
```

Assigning over a live handle drops the old subscription as part of the
assignment, so rebinding to a different signal needs no explicit reset.
::::

::::{cpp:class} CSignalListener

The listener object itself. Its constructor is private and only
{cpp:class}`CSignalBase` can create one, so instances only ever exist behind a
{cpp:type}`CHyprSignalListener`.

Copy and move are deleted in all four forms. A subscription has an identity and
cannot be duplicated; move the handle instead, which is cheap.

:::{cpp:function} void emit(std::any data)

Deprecated. Invokes this one listener directly, bypassing its signal, and is
undefined behaviour for anything registered through {cpp:class}`CSignalT`. It
exists for the legacy untyped API only. Emit on the signal.
:::
::::

## Lifetime is the subscription

The signal keeps a weak reference to each listener and the handle keeps the only
strong one. Nothing else needs to happen for a subscription to end correctly:

* Releasing the handle unregisters the handler, immediately.
* Destroying the object that stores the handle unregisters the handler as part of
  its destruction.
* If the signal is destroyed first, the handle simply stops being called. It
  remains safe to hold and to reset afterwards.

Because both directions are safe, neither party has to know about the other's
lifetime, and there is no disconnect call to forget.

(storing-listeners)=

## Where to put the handle

The storage choice is really a statement about how long the subscription should
last.

**As a struct of members**, for a fixed set of subscriptions bound to the owner's
lifetime. This is the default, and the reason handlers can capture `this` without
further thought.

```cpp
struct {
    CHyprSignalListener button;
    CHyprSignalListener destroy;
} m_listeners;
```

**In a vector**, when the subscriptions are established in a loop or their count
is not known up front.

```cpp
std::vector<CHyprSignalListener> m_listeners;

for (const auto& source : sources) {
    m_listeners.emplace_back(source->m_events.changed.listen([this] { invalidate(); }));
}
```

**In a map**, when individual subscriptions have to be dropped by name later.
Erasing the entry is the unsubscribe.

```cpp
std::unordered_map<std::string, CHyprSignalListener> m_listeners;

m_listeners[name] = source->m_events.changed.listen(handler);
m_listeners.erase(name);   // unsubscribed
```

**In a function-local `static`**, for a subscription that should last as long as
the process. The handle is initialised on first pass and never released.

```cpp
void CRenderer::init() {
    static auto listener = bus()->m_events.tick.listen([this] { onTick(); });
}
```

Note that a plain local variable is the same pattern with a much shorter
lifetime: the subscription ends when the scope does, which is occasionally what
you want and much more often a bug.

## Handlers

A handler is any callable convertible to the signal's handler type. Lambdas
capturing `this` are the norm, since the handle usually lives in the same object:

```cpp
m_listeners.button = device.m_events.button.listen([this](const SButtonEvent& event) {
    if (event.pressed)
        press(event.button);
});
```

Two properties of handler invocation are worth knowing.

**Arguments are borrowed, not owned.** They refer to storage on the emitter's
stack that is valid only for the duration of the call. Copy anything that has to
outlive the handler.

**Handlers may modify the signal they are running on.** Subscribing,
unsubscribing, destroying another listener, or destroying the signal are all safe
from inside a handler, including destroying the running listener's own handle:

```cpp
CHyprSignalListener listener;
listener = signal.listen([&] {
    doWorkOnce();
    listener.reset();   // safe: unsubscribes after this handler returns
});
```

The exact ordering rules for these cases, including which handlers still run
after such a change, are given under {ref}`emission-semantics`.

## Common mistakes

**Discarding the handle.** The single most common error, which is why `listen()`
is `[[nodiscard]]`:

```cpp
signal.listen([] { neverRuns(); });
```

The temporary dies at the end of the statement and the handler is unregistered
before the next line runs.

**Storing the handle somewhere longer-lived than what the handler touches.** A
handle in a global or a `static` whose lambda captures a member of a short-lived
object will call into freed memory. Keep the handle in the object the handler
talks to.

**Using {cpp:func}`CSignalListener::emit`.** It is deprecated, undefined
behaviour with typed signals, and there is no situation that calls for it.

**Reaching for {cpp:func}`CSignalT::listenStatic` to avoid storing a handle.** It
is the correct choice only when the signal cannot outlive the handler's captures.
See {ref}`static-listeners`.
