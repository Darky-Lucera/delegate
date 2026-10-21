#include <Delegate.h>
#include <functional>
#include <cstdio>
#include <string>
#include <chrono>
#include <functional>

#define kExpected(func, value)      { if ((func) != (value)) { fprintf(stderr, "Error: %s\n", #func); } }
#define kNotExpected(func, value)   { if ((func) == (value)) { fprintf(stderr, "Error: %s\n", #func); } }
#define kCheckTrue(func)            kExpected(func, true)
#define kCheckFalse(func)           kExpected(func, false)
#define kCheckIndex(func)           kNotExpected(func, -1)
#define kCheckBadIndex(func)        kExpected(func, -1)

#if __cplusplus >= 201402L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
    #define kCpp14
#endif

using namespace std::chrono;
using namespace std::placeholders;

//--------------------------------------
template <typename T>
uint32_t
GetTime(T time) {
    return uint32_t(duration_cast<nanoseconds>(time).count());
}

//--------------------------------------
static inline high_resolution_clock::time_point
Now() {
    return high_resolution_clock::now();
}

//-------------------------------------
using namespace MindShake;

//-------------------------------------
void
func() {
    printf(" - func\n");
}

void
inc(int &value) {
    ++value;
}

void
funcNoexcept(int value) noexcept {
    printf(" - noexcept function [value = %d]\n", value);
}

void
overloaded(int value) {
    printf(" - overloaded function, int version [value = %d]\n", value);
}

void
overloaded(float value) {
    printf(" - overloaded function, float version [value = %.1f]\n", value);
}

//-------------------------------------
struct Class {
public:
    explicit Class(const std::string &name) : name(name) {}

    void method()            { printf(" - '%s' normal method\n", name.c_str());    }
    void constMethod() const { printf(" - '%s' const method\n", name.c_str());    }
    void method1()           { printf(" - '%s' method 1\n", name.c_str());         }
    void method1() const     { printf(" - '%s' method 1 const\n", name.c_str());   }
    void operator()()        { printf(" - '%s' operator()\n", name.c_str());       }
    void operator()() const  { printf(" - '%s' operator() const\n", name.c_str()); }

    void inc(int &value) const  { ++value; }

    static void staticMethod()  { printf(" - static method\n"); }

protected:
    std::string name;
};

//-------------------------------------
struct ConstFunctor {
    void operator()() const { printf(" - functor with only operator() const\n"); }
};

//-------------------------------------
struct Incrementer {
    void operator()(int &value) const { ++value; }
};

//-------------------------------------
struct Base {
    virtual         ~Base() = default;

    void            baseMethod()    { printf(" - method inherited from a base class\n"); }
    virtual void    virtualMethod() { printf(" - virtual method (base version)\n");      }
};

struct Other {
    virtual         ~Other() = default;

    int             other = 0;
};

// Base is not the first base class, so a Derived * must be adjusted to get the Base *
struct Derived : Other, Base {
    void            virtualMethod() override { printf(" - virtual method (derived version)\n"); }
};

//-------------------------------------
struct VirtualBase {
    virtual         ~VirtualBase() = default;

    int             value = 42;
};

struct Left  : virtual VirtualBase {};
struct Right : virtual VirtualBase {};

struct Diamond : Left, Right {
    void            method() { printf(" - method of a class with virtual inheritance [value = %d]\n", value); }
};

//-------------------------------------
struct Overloads {
    void            update(int value)       { printf(" - update(int) [value = %d]\n", value);         }
    void            update(int value) const { printf(" - update(int) const [value = %d]\n", value);   }
    void            update(float value)     { printf(" - update(float) [value = %.1f]\n", value);     }
};

//-------------------------------------
struct Special {
    void            noexceptMethod(int value) noexcept   { printf(" - noexcept method [value = %d]\n", value);       }
    void            lvalueMethod(int value) &            { printf(" - method with & [value = %d]\n", value);        }
    void            constLvalueMethod(int value) const & { printf(" - method with const & [value = %d]\n", value);  }
    void            rvalueMethod(int) &&                 {}
};

//-------------------------------------
struct Printer {
    const char      *text;

    void            operator()(int value) const { printf(" - %s [value = %d]\n", text, value); }
};

//-------------------------------------
int
main(int argc, char *argv[]) {
    Delegate<void()> delegate;
    Class cls1("cls1"), cls2("cls2");
    const Class constCls1("const Cls1"), constCls2("const Cls2");
    int var = 1234;

    auto lambdaSimple1 = []() {
        printf(" - lambda without captures\n");
    };

    auto lambdaSimple2 = []() {
        printf(" - lambda without captures (as func)\n");
    };

    auto lambdaComplex = [&var]() {
        printf(" - lambda with captures [var = %d]\n", var);
    };

    auto lambdaByPointer = [&var]() {
        printf(" - lambda with captures added by pointer [var = %d]\n", var);
    };

    int calls = 0;
    auto lambdaMutable = [calls]() mutable {
        printf(" - mutable lambda [calls = %d]\n", ++calls);
    };

    ConstFunctor    constFunctor;
    Derived         derived;
    Diamond         diamond;

    // Adding all possible types of functions
    printf("Adding functions:\n");
    delegate.Add(nullptr);
    delegate.Add(func);
    delegate.Add(&cls1);                                            // Functor: calls operator()
    delegate.Add(&constCls1);                                       // const Functor: calls operator()
    delegate.Add(&cls1, &Class::method);
    delegate.Add(&cls1, &Class::constMethod);
    delegate.Add(&cls1, GetNonConstMethod(&Class::method1));
    delegate.Add(&cls1, GetConstMethod(&Class::method1));
    delegate.Add(GetNonConstMethod(&Class::method1), &cls2);
    delegate.Add(GetConstMethod(&Class::method1), &cls2);
    delegate.Add(&constCls1, &Class::method);                       // Probably you should not do this
    delegate.Add(&constCls1, &Class::constMethod);
    delegate.Add(&constCls1, GetNonConstMethod(&Class::method1));   // Probably you should not do this
    delegate.Add(&constCls1, GetConstMethod(&Class::method1));
    delegate.Add(GetNonConstMethod(&Class::method1), &constCls2);   // Probably you should not do this
    delegate.Add(GetConstMethod(&Class::method1), &constCls2);
    auto lambdaSimpleId = delegate.Add(lambdaSimple1);
    delegate.Add(+lambdaSimple2);                                   // This converts a simple lambda to a function pointer
    auto lambdaComplexId = delegate.Add(lambdaComplex);
    std::function<void()> f = func;
    auto stdFunc = delegate.Add(f);
    auto stdBind = delegate.Add(std::bind(&Class::method, &cls2));
    delegate.Add(&Class::staticMethod);                             // Static method: same as a free function
    delegate.Add(&constFunctor);                                    // Functor with only operator() const
    delegate.Add(&derived, &Derived::baseMethod);                   // Method declared in a base class
    delegate.Add(&derived, &Base::virtualMethod);                   // Virtual method: calls the override
    delegate.Add(&diamond, &Diamond::method);                       // Class with virtual inheritance
    delegate.Add(&lambdaByPointer);                                 // Lambda by pointer: Remove(&lambda) works
    auto lambdaMutableId = delegate.Add(lambdaMutable);
    kExpected(delegate.Add(std::function<void()>()), size_t(-1));   // An empty std::function is rejected

    // Calling the delegate
    printf("\nCalling delegate:\n");
    delegate();

    // Removing functions
    printf("\nRemoving functions:\n");
    kCheckFalse(delegate.Remove(nullptr));
    kCheckTrue(delegate.Remove(func));
    kCheckTrue(delegate.Remove(&cls1));                             // Functor: calls operator()
    kCheckTrue(delegate.Remove(&constCls1));                        // const Functor: calls operator() const
    kCheckTrue(delegate.Remove(&cls1, &Class::method));
    kCheckTrue(delegate.Remove(&cls1, &Class::constMethod));
    kCheckTrue(delegate.Remove(&cls1, GetNonConstMethod(&Class::method1)));
    kCheckTrue(delegate.Remove(&cls1, GetConstMethod(&Class::method1)));
    kCheckTrue(delegate.Remove(GetNonConstMethod(&Class::method1), &cls2));
    kCheckTrue(delegate.Remove(GetConstMethod(&Class::method1), &cls2));
    kCheckTrue(delegate.Remove(&constCls1, &Class::method));
    kCheckTrue(delegate.Remove(&constCls1, &Class::constMethod));
    kCheckTrue(delegate.Remove(&constCls1, GetNonConstMethod(&Class::method1)));
    kCheckTrue(delegate.Remove(&constCls1, GetConstMethod(&Class::method1)));
    kCheckTrue(delegate.Remove(GetNonConstMethod(&Class::method1), &constCls2));
    kCheckTrue(delegate.Remove(GetConstMethod(&Class::method1), &constCls2));
    kCheckTrue(delegate.Remove(lambdaSimple1));                     // Lambdas without captures are stored as functions
    kCheckFalse(delegate.RemoveById(lambdaSimpleId));               // Already removed by the line above
    kCheckTrue(delegate.Remove(+lambdaSimple2));                    // Ok
    //delegate.Remove(lambdaComplex);                                 // Lambdas with captures cannot be removed by value (compilation error)
    kCheckTrue(delegate.RemoveById(lambdaComplexId));               // Ok
    kCheckTrue(delegate.RemoveById(stdBind));                       // Ok
    kCheckTrue(delegate.Remove(&Class::staticMethod));
    kCheckTrue(delegate.Remove(&constFunctor));
    kCheckTrue(delegate.Remove(static_cast<Base *>(&derived), &Base::baseMethod));   // A base pointer also works
    kCheckTrue(delegate.Remove(&derived, &Base::virtualMethod));
    kCheckTrue(delegate.Remove(&diamond, &Diamond::method));
    kCheckTrue(delegate.Remove(&lambdaByPointer));
    kCheckTrue(delegate.RemoveById(lambdaMutableId));
    kExpected(delegate.GetNumDelegates(), size_t(1));               // Only the std::function is left

    printf("\nCalling delegate:\n");
    delegate();
    kCheckTrue(delegate.RemoveById(stdFunc));                       // A std::function can only be removed by id
    kExpected(delegate.GetNumDelegates(), size_t(0));

    // rValues
    Delegate<void(std::string)> delegate2;
    delegate2.Add([](std::string str) { printf(" - lambda with parameters [str = %s]\n", str.c_str()); });
    delegate2.Add([](std::string str) { printf(" - lambda with parameters [str = %s]\n", str.c_str()); });
    delegate2("Hello world!");

    // Functors with parameters
    Delegate<void(int &)> delegate4;
    Incrementer incrementer;
    int counter = 0;
    delegate4.Add(&incrementer);
    delegate4(counter);
    kExpected(counter, 1);
    kCheckTrue(delegate4.Remove(&incrementer));

    // Const and non-const overloads of a method with parameters
    printf("\nOverloaded methods with parameters:\n");
    Delegate<void(int)> delegate5;
    Overloads overloads;
    delegate5.Add(&overloads, GetNonConstMethod<Overloads, int>(&Overloads::update));     // Two non-const update(): give the parameters
    delegate5.Add(&overloads, GetConstMethod(&Overloads::update));                         // Only one const update()
    delegate5(1);
    kCheckTrue(delegate5.Remove(&overloads, GetNonConstMethod<Overloads, int>(&Overloads::update)));
    kCheckTrue(delegate5.Remove(&overloads, GetConstMethod(&Overloads::update)));

    // More kinds of functions
    printf("\nMore kinds of functions:\n");
    Delegate<void(int)> delegate6;
    Special special;
    delegate6.Add(&funcNoexcept);                                   // Since C++17 noexcept is part of the function type
    delegate6.Add(&overloaded);                                     // Picks the overload that matches void(int)
    delegate6.Add(&special, &Special::noexceptMethod);
    auto printerId = delegate6.Add(Printer{"functor by value"});    // The delegate stores a copy: remove it by id
    auto lambdaNoexceptId = delegate6.Add([](int value) noexcept { printf(" - noexcept lambda [value = %d]\n", value); });
#if defined(kCpp14)
    auto lambdaGeneric = [](auto value) { printf(" - generic lambda [value = %d]\n", int(value)); };
    delegate6.Add(lambdaGeneric);                                   // Generic lambdas need C++14
#endif
    delegate6.Add(&special, &Special::lvalueMethod);
    delegate6.Add(&special, &Special::constLvalueMethod);
    //delegate6.Add(&special, &Special::rvalueMethod);              // Methods with && are not supported (compilation error)
    delegate6(2);

    kCheckTrue(delegate6.Remove(&funcNoexcept));
    kCheckTrue(delegate6.Remove(&overloaded));
    kCheckTrue(delegate6.Remove(&special, &Special::noexceptMethod));
    kCheckTrue(delegate6.Remove(&special, &Special::lvalueMethod));
    kCheckTrue(delegate6.Remove(&special, &Special::constLvalueMethod));
    kCheckTrue(delegate6.RemoveById(printerId));
    kCheckTrue(delegate6.RemoveById(lambdaNoexceptId));
#if defined(kCpp14)
    kCheckTrue(delegate6.Remove(lambdaGeneric));                    // Without captures, so it is stored as a function
#endif
    kExpected(delegate6.GetNumDelegates(), size_t(0));

    // References && performance
    constexpr const size_t times = 1000000;
    int value = 0;

    Delegate<void(int &)> delegate3;
    delegate3.Add(inc);
    delegate3.Add(&cls1, &Class::inc);
    delegate3.Add([](int &var) { ++var; });

    auto timeStamp1 = Now();
    for(size_t i=0; i<times; ++i)
        delegate3(value);
    auto time1 = Now() - timeStamp1;
    kExpected(value, 3*times);

    value = 0;
    std::function<void(int &)> funct  = inc;
    std::function<void(int &)> member = std::bind(&Class::inc, &cls1, std::placeholders::_1);
    std::function<void(int &)> lambda = [](int &var) { ++var; };

    auto timeStamp2 = Now();
    for(size_t i=0; i<times; ++i) {
        funct(value);
        member(value);
        lambda(value);
    }
    auto time2 = Now() - timeStamp2;
    kExpected(value, 3*times);

    printf("Time Delegate:      %d\n", GetTime(time1));
    printf("Time std::function: %d\n", GetTime(time2));

    return 0;
}
