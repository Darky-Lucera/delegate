# Delegate Class

The Delegate class in C++ is a versatile, easy-to-use, type-safe and header-only implementation for delegates. It empowers you to craft delegate / callback / listener / event-handler objects capable of holding multiple references to a variety of entities:

- Free functions
- Static member functions
- Functors
- Non-static member functions (const or non-const)
- Lambda functions (with and without captures)

This utility is derived from an earlier version employed in the **MindShake** video game engine from **Lucera Project**. While I can't recall the precise inception date of the initial class, I developed it prior to the establishment of Lucera in 2009, utilizing C++98. Subsequently, I enhanced it to leverage the features introduced in C++11, a transformation that took place several years ago.

**Note:** In all my tests, the Delegate has demonstrated comparable speed to std::function and, in some cases, even faster performance (depending on the specific scenario and compiler flags). Additionally, it is capable of holding multiple functions.

## Usage

Just drop the class into your code folder and include it.

## Interface

 - `id       Add(function)`: Adds a function.
 - `bool     Remove(function, bool lazy)`: Lazy delays the deletetion until RemoveLazyDeleted is called (useful in multi-thread environments).
 - `bool     RemoveById(id)`: Removes a funtion given its id.
 - `void     RemoveLazyDeleted()`: Deletes functions previously marked for lazy deletion.
 - `void     Clear()`: Removes all functions.
 - `void     operator(...) const`: Runs the functions added.
 - `size_t   GetNumDelegates() const`: Get the number of delegates added.

## Examples

The main.cpp contains an exhaustive example of how to use this class.

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

If you find yourself needing to distinguish between calling a const or a non-const function, you can employ the helper functions: getNonConstMethod and getConstMethod:

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

int kk() {
    MindShake::Delegate<void()> delegate;

    MyClass myObject;
    delegate.Add(&myObject, MindShake::getNonConstMethod(&MyClass::memberFunction));

    delegate.Add(&myObject, MindShake::getConstMethod(&MyClass::memberFunction));

    // ...
}

```

If you no longer wish to receive additional events, or if you are in the process of destroying the class that holds the delegate, it is advisable to remove it:

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

If you wish to cease receiving further events for a lambda function, take note that the Add function returns an ID, facilitating the straightforward identification of the delegate to be removed:

```cpp
Delegate<void()> delegate;

auto lambdaId = delegate.Add([]() {
    // ...
});

// ...
delegate.RemoveById(lambdaId);
```
