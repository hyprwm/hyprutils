=======
Signals
=======

.. code-block:: cpp

   #include <hyprutils/signal/Signal.hpp>

.. cpp:namespace:: Hyprutils::Signal

.. cpp:class:: template<typename ...Args> CSignalT

   An event source carrying ``Args...``. Instantiate one per event, as a member
   of the object that produces it. ``CSignalT<>`` carries no data.

   .. code-block:: cpp

      CSignalT<>                   destroy;
      CSignalT<SButtonEvent>       button;
      CSignalT<int, const char*>   twoArgs;

   A signal is usable immediately after construction and holds no listeners.
   It has no user-declared copy or move operations, and copying one duplicates
   its listener lists, which is rarely what anyone means. Treat signals as
   fixed-in-place members and pass them around by reference.

.. cpp:class:: CSignalBase

   The non-template base of every :cpp:class:`CSignalT`. It holds the listener
   lists and dispatches against a type-erased payload. Its interface is
   entirely protected; you never instantiate or call it directly.

.. cpp:namespace-push:: template<typename ...Args> CSignalT

How arguments are passed
========================

Handlers do not necessarily receive ``Args...`` verbatim. Each declared type is
mapped through an internal alias, ``RefArg<T>``, before it reaches ``emit()``
and the handler:

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Declared as
     - Handler receives
     - Why
   * - ``T``, trivially copyable
     - ``T`` by value
     - Copying an ``int`` or an enum is cheaper than indirecting through a
       reference.
   * - ``T``, not trivially copyable
     - ``const T&``
     - Payload structs, strings and smart pointers are never copied on the way
       to a handler.
   * - ``T&``
     - ``T&``
     - An explicit mutable reference survives the mapping, so handlers can
       write back to the emitter's object.
   * - ``const T&``
     - ``const T&``
     - Unchanged.

The practical consequence is that ``CSignalT<SButtonEvent>`` already does the
right thing: declare the plain type and handlers get ``const SButtonEvent&``
with no copies. Write ``const&`` in the signal's parameter list only when you
want it to be visible at the declaration site, and write a plain ``T&`` only
when mutation is the point.

Emitting
========

.. cpp:function:: void emit(RefArg<Args>... args)

   Invokes every live listener, in registration order, and returns once the
   last one has run. If the signal has no listeners at all, ``emit()`` returns
   immediately without touching the arguments.

   .. code-block:: cpp

      CSignalT<>             frame;
      CSignalT<SButtonEvent> button;

      frame.emit();
      button.emit(SButtonEvent{.timeMs = now, .button = BTN_LEFT, .pressed = true});

   Arguments live on the emitter's stack for the duration of the call and every
   handler observes the same object. A handler must not stash a reference to an
   argument for later; copy what it needs instead.

Mutable arguments and out-parameters
------------------------------------

Because ``T&`` survives the mapping described above, a signal can collect a
result from its listeners. The common shape is a cancellable event: the emitter
puts a small info struct on the stack, passes it by mutable reference, and
inspects it once the handlers have run.

.. code-block:: cpp

   struct SCallbackInfo {
       bool cancelled = false;
   };

   CSignalT<SButtonEvent, SCallbackInfo&> button;

   // consumer
   auto listener = button.listen([](const SButtonEvent& event, SCallbackInfo& info) {
       if (shouldSwallow(event))
           info.cancelled = true;
   });

   // producer
   SCallbackInfo info;
   button.emit(event, info);
   if (info.cancelled)
       return;

Every handler sees the same ``info``, so a later listener can observe and
override what an earlier one wrote.

Subscribing
===========

.. cpp:function:: CHyprSignalListener listen(std::function<void(RefArg<Args>...)> handler)

   Registers ``handler`` and returns the handle that owns the subscription. The
   handler runs until the returned :cpp:type:`CHyprSignalListener` is released.

   Marked ``[[nodiscard]]``: dropping the return value unsubscribes on the
   spot.

   .. code-block:: cpp

      m_listeners.button = device.m_events.button.listen([this](const SButtonEvent& event) {
          onButton(event);
      });

   The handler only has to be *callable* with the mapped arguments, not to
   match them exactly. A signal declared ``CSignalT<SButtonEvent>`` accepts a
   handler taking ``SButtonEvent`` by value, and one declared with a mutable
   ``T&`` accepts a handler that takes ``const T&`` if it does not intend to
   write back.

.. cpp:function:: CHyprSignalListener listen(std::function<void()> handler)

   Available only when the signal declares at least one argument. Subscribes a
   handler that ignores the payload, which saves naming parameters that are
   never read.

   .. code-block:: cpp

      // valueChanged is a CSignalT<int>
      m_listeners.dirty = valueChanged.listen([this] { markDirty(); });

   Overload resolution picks between the two ``listen`` overloads by what the
   callable accepts, so an ordinary lambda is never ambiguous. A variadic
   generic lambda such as ``[](auto&&...) {}`` accepts both and will not
   compile; give it a concrete signature.

.. cpp:function:: template<typename ...OtherArgs> CHyprSignalListener forward(CSignalT<OtherArgs...>& signal)

   Subscribes a handler that re-emits on ``signal``. Use it to republish an
   event from a wrapped object under your own signal without writing a
   pass-through lambda.

   .. code-block:: cpp

      m_listeners.button = m_wrapped->m_events.button.forward(m_events.button);
      m_listeners.frame  = m_wrapped->m_events.frame.forward(m_events.frame);

   The target may declare no arguments, in which case it is emitted empty;
   otherwise it must accept the source's arguments. Two caveats:

   * The return value owns the link and is ``[[nodiscard]]`` for the same
     reason ``listen()`` is. Store it.
   * The target is captured by reference, so it must outlive the returned
     handle.

   Forwarding is only worth it for a verbatim relay. When the payload needs
   adjusting on the way through, write the handler out:

   .. code-block:: cpp

      m_listeners.motion = m_wrapped->m_events.motion.listen([this](SMotionEvent event) {
          event.device = m_self.lock();
          m_events.motion.emit(event);
      });

.. _static-listeners:

Static listeners
================

.. cpp:function:: void listenStatic(std::function<void(RefArg<Args>...)> handler)
.. cpp:function:: void listenStatic(std::function<void()> handler)

   Registers a handler owned by the signal itself. There is no handle and no
   way to unsubscribe: the handler lives until the signal is destroyed. As with
   ``listen()``, the second overload exists only for signals that declare
   arguments.

   .. code-block:: cpp

      // the manager owns the device, so the subscription cannot outlive it
      device->m_events.destroy.listenStatic([this, raw = device.get()] {
          removeDevice(raw);
      });

   This is the right tool when the *signal* is the shorter-lived party, which
   typically means an owner subscribing to something it owns. It removes the
   bookkeeping of a handle that would be destroyed at the same moment anyway,
   and it makes capturing a raw pointer to the signal's own object safe, since
   the handler cannot outlive it.

   It is the wrong tool anywhere else. A static handler that captures something
   which dies before the signal is a dangling call with no way to detach.

   Static listeners run after all handle-owned listeners, in registration
   order.

.. _emission-semantics:

Emission semantics
==================

The behaviour below is deliberate, covered by the test suite, and safe to rely
on. All of it exists because handlers routinely mutate the very structures the
signal is iterating.

**Registration order.** Handle-owned listeners run first, in the order they
were registered, followed by static listeners in their own registration order.

**The listener set is snapshotted at the start of an emission.** A listener
registered from inside a handler does not run in that emission. It runs in the
next one.

.. code-block:: cpp

   CSignalT<>          signal;
   CHyprSignalListener second;
   int                 count = 0;

   auto first = signal.listen([&] {
       count++;
       if (!second)
           second = signal.listen([&] { count++; });
   });

   signal.emit();   // count == 1, `second` was registered too late to run
   signal.emit();   // count == 3

**A listener released mid-emission does not run.** If a handler resets someone
else's handle, or its own, the dropped listener is skipped even though it was
in the snapshot.

**The signal may be destroyed from inside a handler.** The remaining listeners
of the in-flight emission still run to completion, and the destruction takes
effect afterwards. This is what makes ``destroy`` signals that free their own
emitter work.

**Emissions may nest.** Emitting a signal from within one of its own handlers
is memory-safe; each nested emission takes its own snapshot. Guarding against
unbounded recursion is the caller's problem.

**Expired listeners are reclaimed lazily.** Dead entries are pruned the next
time something calls ``listen()``, never during an emission, since a handler
may have destroyed the signal itself. A signal that is emitted often but
subscribed to rarely holds on to a few empty slots; they cost a branch each.

.. cpp:namespace-pop::

Deprecated API
==============

An older untyped API is still present for source compatibility. It passes
payloads as ``std::any``, so mistakes surface as a runtime
``std::bad_any_cast`` rather than a compile error. None of it should appear in
new code.

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Deprecated
     - Replacement
   * - ``CSignal`` (an alias for ``CSignalT<std::any>``)
     - ``CSignalT<T>`` with the concrete payload type
   * - ``registerListener(std::function<void(std::any)>)``
     - :cpp:func:`CSignalT::listen`
   * - ``registerStaticListener(std::function<void(void*, std::any)>, void* owner)``
     - :cpp:func:`CSignalT::listenStatic`, capturing the owner in the lambda
   * - ``CSignalListener::emit(std::any)``
     - :cpp:func:`CSignalT::emit` on the signal itself

Migrating is usually mechanical:

.. code-block:: cpp

   // before
   CSignal signal;
   auto    listener = signal.registerListener([](std::any data) {
       const auto VALUE = std::any_cast<int>(data);
   });
   signal.emit(42);

   // after
   CSignalT<int> signal;
   auto          listener = signal.listen([](int value) {
   });
   signal.emit(42);

.. warning::

   ``CSignalListener::emit()`` invokes a single listener directly, bypassing
   its signal. It is undefined behaviour on any listener registered through
   :cpp:class:`CSignalT`, because the data it synthesises does not match the
   layout the typed handler expects. There is no reason to call it.