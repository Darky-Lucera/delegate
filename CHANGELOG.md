# Changelog

## Unreleased

### Delegate.h

- Fix a use-after-free in `Clear()` and the destructor: when deleting a lambda destroyed an object that removed a function from the same delegate, `Remove()` read wrappers that were already freed.
- `operator()` returns `bool`: `false` if the call was ignored because the delegate was already running. Code that ignores the result does not change.
- `Add()` and `Remove()` with a pointer to a functor accept an `operator()` that returns a value, and ignore that value. So a `Delegate<void()>` can still be added to another one, now that its `operator()` returns `bool`.
- Add `IsRunning()`, to know before a call if it would be ignored. With several threads, only the value that `operator()` returns is always right.
- `kDelegateVersion` is now `20261022`.

### Tests and build

- Add the `DestructorUsesTheDelegateDuringClear` test.
- Add the `AddThatGrowsTheVectorDuringACall` test: a function adds 1000 functions during a call, and then throws.
- Add the `IsRunning` test.
- Add the `FunctorsThatReturnAValue` test, which also adds a delegate to another one.

### Other

- `main.cpp` compared a delegate with 3 functions against 3 separate `std::function` calls, which made the delegate look more than 2 times slower. It now compares it with a `std::vector` of `std::function` that holds the same functions, with 3 and 30 functions, and shows the best of 5 runs in ns per call.
- README: the performance note gives the measured numbers, and `Clear()` says that the destructors of what a lambda captured can remove functions from the same delegate.

## 1.1.0 (2026-10-20)

### Breaking changes

- `getConstMethod` and `getNonConstMethod` are renamed to `GetConstMethod` and `GetNonConstMethod`.

### Delegate.h

- Each method is stored in a wrapper typed on its own class, instead of being cast to a method of an unknown class, which was undefined behavior. `Remove()` matches methods by a per-type tag.
- Support methods of base classes (including multiple inheritance), `noexcept` functions and methods, and methods with `&` or `const &` qualifiers. Methods with `&&` are rejected at compile time.
- Support functors and lambdas by pointer, and `mutable` and generic lambdas.
- `Delegate` is now movable (but not copyable), and has a const `operator()`.
- Ids are now unique per delegate instead of global.
- `Add()` returns `size_t(-1)` for null functions, objects, methods and empty `std::function`.
- Add `DELEGATE_LOG_ERROR` so users can plug in their own logger.
- Remove the C++14 digit separator in the version constant, so the header builds as C++11.
- `GetConstMethod` and `GetNonConstMethod` now work on methods with parameters.

### Tests and build

- Add `tests/` with a small test runner, functional tests and allocation tests, run through CTest.
- Build the tests with high warning levels (`/W4`, `-Wall -Wextra -Wpedantic`).
- Bump the CMake minimum to 3.16 and the project version to 1.1.0.

### Other

- Rewrite the README: interface, tests, and a "Things to know" section (reentrancy, thread safety, lifetime, move-only types, DLLs).
- Update `main.cpp` to cover every supported kind of function.
- Add `.editorconfig` and `.gitattributes` to force LF line endings.
- Fix a typo in `.gitignore` (`/mimgw`) and add more build folders.

## 2024-12-22

### Breaking changes

- `Remove()` and `RemoveById()` no longer take a `lazy` flag, and `RemoveLazyDeleted()` is no longer public.

### Delegate.h

- Support `std::function` and the result of `std::bind`.
- A function removed while the delegate is running is disabled at once, and deleted when the call finishes.
- A call made while the delegate is already running is ignored.
- Catch the exceptions thrown by each function, print them with `fprintf`, and continue with the next function.
- Add the `kDelegateVersion` constant.
- Replace the `kUnConst` macro with `unConst()` helper functions, and use more macros to shorten the `Add`, `Remove` and `Find` overloads.

## 1.0.0 (2023-12-17)

First public version: a C++11 rewrite of the delegate class of the **MindShake** engine.

- Header-only `Delegate<void(Args...)>` that holds free functions, static member functions, functors, const and non-const member functions, and lambdas with or without captures.
- `Add()` returns an id that `RemoveById()` accepts.
- `getConstMethod` and `getNonConstMethod` helpers to pick one overload of a method.
- Example in `main.cpp`, CMake project and README.

## Pre-C++11 version (2006)

Written in C++98 for the **MindShake** engine of **Lucera Project**, before Lucera started in 2009. It was never published on its own.

The game moved between scenes such as the menu and the gameplay, and each scene had to subscribe to events when it started and unsubscribe when it ended. So the engine needed a delegate that could hold several pointers to functions and member functions, and remove any of them later.

- The API was very similar to 1.0.0: one delegate holds several functions, which you add, remove and call together.
- Inspired by:
  - Don Clugston, "Member Function Pointers and the Fastest Possible C++ Delegates" (FastDelegate).
  - Sergey Ryazanov, "The Impossibly Fast C++ Delegates".
  - Delegates and events in C#.
