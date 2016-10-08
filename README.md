# zcpointer: Zero-Cost Poisoning Unique Pointers

The zcpointer library is a specialization of
[std::unique\_ptr](http://en.cppreference.com/w/cpp/memory/unique_ptr) that tracks and poisons weak
references. The goal is to allow C++ developers to write programs without _ever_ having a pointer
that is not automatically managed by a smart scoper.

The library provides two types to do this:

- `zc::owned<T>` provides an identical interface to `std::unique_ptr<T>`, but which can track outstanding
  weak references.
- `zc::ref<T>` is the return value of `zc::owned<T>::get()`. It stands for a `T*` but is used to
  ensure use-after-free does not occur.

To achieve zero-cost, the zcpointer library can be compiled with or without the
`ZCPOINTER_TRACK_REFS` option. When disabled, `zc::owned` is just a type alias for `std::unique_ptr`
and `zc::ref` is a type alias for a raw pointer. With the option turned on, however, it becomes
impossible to unwrap a `zc::owned` into a raw pointer. All object access has to be indirected via a
`ref` object. When an `owned` object is deleted, it notifies all the outstanding `ref`s so that they
can be poisoned against future use.

When the tracking option is disabled and compiler optimization is turned up, the cost of using
zcpointer is none, compared to using a normal `unique_ptr`.

## Suggested Usage

When using zcpointer, it is recommended to compile with `ZCPOINTER_TRACK_REFS` for debug builds, or
if your project provides a hardened compilation mode (such as `_FORTIFY_SOURCE`). For production
builds, turn off reference tracking to eliminate the overhead of zcpointer. Tests should be run
using `ZCPOINTER_TRACK_REFS=1` to catch errors during development.
