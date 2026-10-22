# Delegate Class

`Delegate` is a small, type-safe, header-only C++ class that keeps a list of functions and calls all of them with the same arguments. Use it for callbacks, listeners, events, or signals and slots.

It can hold:

- Free functions and static member functions, including overloaded and `noexcept` ones
- Member functions, const or non-const, including virtual, inherited and `noexcept` ones, and ones with a `&` or `const &` qualifier
- Functors (objects with an `operator()`), by pointer or by value. If `operator()` returns a value, it is ignored, so a delegate can hold another delegate
- Lambdas, with or without captures, including `mutable` and generic ones (generic lambdas need C++14)
- `std::function` and the result of `std::bind`

You can also remove a function after you add it. This is the main reason the class exists: `std::function` has no `operator==`, so once you store one, there is no way to find it again and remove it.

This class comes from an older version used in the **MindShake** game engine of **Lucera Project**. I do not remember exactly when I wrote the first version, but it was before Lucera started in 2009, and it used C++98. Some years ago I updated it to C++11.

**Note:** Each call to a `Delegate` has a small fixed cost, a few nanoseconds. It pays for letting the functions add or remove functions while the delegate runs, ignoring calls made while it is running, and catching exceptions. So with very small functions, a `Delegate` is slower than a `std::vector` of `std::function` that holds the same functions: about 2 times slower with 3 functions, and between 5% and 30% slower with 30, depending on the compiler. With functions that do real work, the difference is small. `main.cpp` measures it.

## Usage

Copy `src/Delegate.h` into your project and include it. It needs C++11 or later.

If a function throws an exception, the delegate catches it, prints it with `fprintf` to `stderr`, and continues with the next function. To use your own logger, define `DELEGATE_LOG_ERROR` before including the file. It receives a format string and its arguments, like `printf`:

```cpp
#define DELEGATE_LOG_ERROR(...)     MyLogger::Error(__VA_ARGS__)
#include "Delegate.h"
```

## Interface

- `size_t Add(function)`: Adds a function and returns its id. Returns `size_t(-1)` if the function, the object or the method is null, or if the `std::function` is empty.
- `bool Remove(function)`: Removes a function. Returns `false` if it is not found. If the delegate is running, the function is not called again, and it is deleted when the call finishes.
- `bool RemoveById(id)`: Removes the function with the id that `Add` returned. Ids are unique within each delegate.
- `void Clear()`: Removes all functions. It is safe to call it from one of the functions. When it deletes a lambda, the destructors of what the lambda captured can remove functions from the same delegate.
- `bool operator()(args...)`: Calls all the functions, in the order they were added. Functions added during a call run from the next call. Returns `false` if the call was ignored because the delegate was already running.
- `bool operator()(args...) const`: The same, for a delegate used through a const reference. Functions removed during this call are not called again, but they are freed by the next call through a non-const reference.
- `size_t GetNumDelegates() const`: Returns the number of functions.
- `bool IsRunning() const`: Returns `true` while the delegate is calling its functions. A call made then is ignored.

## Examples

`main.cpp` adds, calls and removes every kind of supported function. At the end, it compares the speed of a `Delegate` with a `std::vector` of `std::function`.

## Tests

`tests/` has the unit tests. They need nothing but the standard library. Build the project with CMake and run them with `ctest`:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Each test prints one line, `[ OK ]` or `[FAIL]`, followed by the checks that failed, if any.

## Snippets

Here are some basic examples of how to use the `Delegate` class:

```cpp
#include "Delegate.h"

void mousePosition(int x, int y) {
    // ...
}

class MyClass {
public:
    void mousePosition(int x, int y) {
        // ...
    }
};

int main() {
    MindShake::Delegate<void(int x, int y)> delegate;

    delegate.Add(mousePosition);

    MyClass myObject;
    delegate.Add(&myObject, &MyClass::mousePosition);

    delegate.Add([](int x, int y) {
        // ...
    });

    delegate(2, 3);

    // ...
}
```

### Const and non-const versions of a method

If a class has a const and a non-const version of the same method, `&MyClass::memberFunction` is ambiguous, and you must say which one you want. Use the helpers `GetNonConstMethod` and `GetConstMethod`:

```cpp
class MyClass {
public:
    void memberFunction() {
        // ...
    }

    void memberFunction() const {
        // ...
    }
};

int main() {
    MindShake::Delegate<void()> delegate;

    MyClass myObject;
    delegate.Add(&myObject, MindShake::GetNonConstMethod(&MyClass::memberFunction));
    delegate.Add(&myObject, MindShake::GetConstMethod(&MyClass::memberFunction));

    // ...
}
```

They also work with methods that have parameters. If there are several overloads with the same constness, the helper cannot know which one you want, so give the class and the parameter types:

```cpp
class MyClass {
public:
    void update(int value)       { /* ... */ }
    void update(int value) const { /* ... */ }
    void update(float value)     { /* ... */ }
};

MindShake::Delegate<void(int)> delegate;
MyClass myObject;
delegate.Add(&myObject, MindShake::GetConstMethod(&MyClass::update));                  // Only one const update()
delegate.Add(&myObject, MindShake::GetNonConstMethod<MyClass, int>(&MyClass::update)); // Two non-const update()
```

The helpers do not handle `&` and `const &` qualifiers. If a method has a `&` and a `const &` version, use `static_cast`, for example `static_cast<void (MyClass::*)(int) const &>(&MyClass::update)`.

### Removing a method

Remove a method before its object is destroyed. Otherwise the delegate keeps a pointer to an object that no longer exists. The destructor is a good place to do it:

```cpp
struct Holder {
    static MindShake::Delegate<void(int)> delegate;
};

class MyClass {
public:
    MyClass() {
        Holder::delegate.Add(this, &MyClass::memberFunction);
    }

    virtual ~MyClass() {
        Holder::delegate.Remove(this, &MyClass::memberFunction);
    }

    void memberFunction(int value) {
        // ...
    }
};

int main() {
    MyClass myObject;
    // ...
    Holder::delegate(123);
    // ...
}
```

### Removing a lambda

There are three ways to remove a lambda:

```cpp
MindShake::Delegate<void()> delegate;
int counter = 0;

// 1. A lambda without captures is stored as a plain function, so you can remove it like one.
auto simple = []() { /* ... */ };
delegate.Add(simple);
delegate.Remove(simple);

// 2. For a lambda with captures, keep the id that Add returns.
auto id = delegate.Add([&counter]() { ++counter; });
delegate.RemoveById(id);

// 3. Or add the lambda by pointer, and remove it with the same pointer.
//    The lambda must stay alive while it is in the delegate.
auto withCaptures = [&counter]() { ++counter; };
delegate.Add(&withCaptures);
delegate.Remove(&withCaptures);
```

## Things to know

### Calls made while the delegate is running are ignored

If one of the functions calls the same delegate again, directly or through other code, that inner call does nothing:

```cpp
MindShake::Delegate<void(int)> onValue;
onValue.Add([&onValue](int value) {
    printf("value %d\n", value);
    if (value == 0)
        onValue(1);     // Ignored: onValue is already running
});

onValue(0);             // Prints only "value 0"
```

The same happens if another thread calls the delegate while it is running. A lambda that calls itself directly, without going through the delegate, works as usual.

`operator()` returns `false` when it ignores a call. For example, to keep the event and send it later:

```cpp
if (onValue(value) == false) {
    pendingValues.push_back(value);
}
```

`IsRunning()` tells you the same thing before the call. But if another thread calls the delegate, it can start running between `IsRunning()` and your call. Only the value that `operator()` returns is always right.

### It is not thread-safe

Do not call a delegate, or use `Add`, `Remove` or `Clear` on it, from two threads at the same time. If you need to, protect it with a mutex.

### It can be moved, but not copied

A delegate owns the functions it stores, so it cannot be copied. It can be moved, so classes that contain one can be moved too:

```cpp
struct Button {
    MindShake::Delegate<void()> onPressed;
};

std::vector<Button> buttons;
buttons.emplace_back();                                     // OK: Button can be moved

MindShake::Delegate<void()> copy = buttons[0].onPressed;   // Does not compile: a delegate cannot be copied
```

### Do not destroy a delegate from one of its functions

After a function returns, the delegate goes on with the next one. If that function destroyed the delegate, the delegate is now using freed memory. This is easy to do by mistake, for example with a dialog that deletes itself when it is closed:

```cpp
struct Dialog {
    MindShake::Delegate<void()> onClose;

    Dialog() {
        onClose.Add([this]() { delete this; });     // Wrong: deletes onClose while it is running
    }
};
```

Instead, only mark the object in the function, and destroy it after the call:

```cpp
struct Dialog {
    MindShake::Delegate<void()> onClose;
    bool                        mustClose = false;

    Dialog() {
        onClose.Add([this]() { mustClose = true; });
    }
};

dialog->onClose();
if (dialog->mustClose)
    delete dialog;
```

Moving another delegate into a delegate from one of its functions has the same problem.

### Lambdas that can only be moved cannot be added

The delegate stores a copy of each lambda. A lambda that captures a value that cannot be copied, such as a `std::unique_ptr`, cannot be copied either:

```cpp
// Init captures need C++14
std::unique_ptr<Texture> texture = LoadTexture();
delegate.Add([texture = std::move(texture)]() { /* ... */ });   // Does not compile

std::shared_ptr<Texture> shared = LoadSharedTexture();
delegate.Add([shared]() { /* ... */ });                         // OK
```

### Move-only arguments cannot be passed by value

All the functions receive the same arguments, and one value cannot be moved into several functions. So a delegate that takes a `std::unique_ptr` by value cannot hold any function. Take it by reference instead, and let the function that wants the object move it:

```cpp
MindShake::Delegate<void(std::unique_ptr<Message>)> onMessage;     // Add() does not compile

MindShake::Delegate<void(std::unique_ptr<Message> &)> onMessage;   // OK
onMessage.Add([&queue](std::unique_ptr<Message> &message) {
    if (message)
        queue.push_back(std::move(message));    // The next functions receive an empty pointer
});
```

### Methods of a base class

You can add a method with a pointer to the object's own class or to any of its base classes. The delegate converts the pointer to the class that declares the method, so `Add` and `Remove` find each other even if they use different pointer types. This also works with multiple inheritance:

```cpp
struct Entity {
    void onHit(int damage) { /* ... */ }
};

struct Player : Renderable, Entity {
    // ...
};

Player player;
delegate.Add(&player, &Player::onHit);                              // onHit is declared in Entity
delegate.Remove(static_cast<Entity *>(&player), &Entity::onHit);    // Found
```

### Methods with a `&&` qualifier are not supported

Methods with a `&` or `const &` qualifier work like any other method. A method with a `&&` qualifier can only be called on an object that is about to be destroyed, and it often moves the object's data out. The delegate would call it again and again on the same object, so it does not accept these methods:

```cpp
struct Widget {
    void onClick(int x) &;
    void onFinish(int x) &&;
};

Widget widget;
delegate.Add(&widget, &Widget::onClick);                                // OK
delegate.Add(&widget, &Widget::onFinish);                               // Does not compile
delegate.Add([&widget](int x) { std::move(widget).onFinish(x); });     // If you really need it, be explicit
```

### Add and Remove a method from the same module

To find a method, `Remove` uses some internal type information. On Windows, each DLL has its own copy of it, and so do shared libraries on Linux or macOS when they are built with hidden symbols. So if your program uses them, remove a method from the same module (executable or library) that added it.
