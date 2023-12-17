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

using namespace std::chrono;

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

//-------------------------------------
struct Class {
public:
    explicit Class(const std::string &name) : name(name) {}

    void method()           { printf(" - '%s' normal method\n", name.c_str());    }
    void constMethod()      { printf(" - '%s' const method\n", name.c_str());     }
    void method1()          { printf(" - '%s' method 1\n", name.c_str());         }
    void method1() const    { printf(" - '%s' method 1 const\n", name.c_str());   }
    void operator()()       { printf(" - '%s' operator()\n", name.c_str());       }
    void operator()() const { printf(" - '%s' operator() const\n", name.c_str()); }

    void inc(int &value) const  { ++value; }

protected:
    std::string name;
};

//-------------------------------------
int
main(int argc, char *argv[]) {
    Delegate<void()> delegate;
    Class cls1("cls1"), cls2("cls2");
    const Class constCls1("constCls1"), constCls2("constCls2");
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

    // Adding all possible types of functions
    printf("Adding functions:\n");
    delegate.Add(nullptr);
    delegate.Add(func);
    delegate.Add(&cls1);                                            // Functor: calls operator()
    delegate.Add(&constCls1);                                       // const Functor: calls operator()
    delegate.Add(&cls1, &Class::method);
    delegate.Add(&cls1, &Class::constMethod);
    delegate.Add(&cls1, getNonConstMethod(&Class::method1));
    delegate.Add(&cls1, getConstMethod(&Class::method1));
    delegate.Add(getNonConstMethod(&Class::method1), &cls2);
    delegate.Add(getConstMethod(&Class::method1), &cls2);
    delegate.Add(&constCls1, &Class::method);                       // Probably you should not do this
    delegate.Add(&constCls1, &Class::constMethod);
    delegate.Add(&constCls1, getNonConstMethod(&Class::method1));   // Probably you should not do this
    delegate.Add(&constCls1, getConstMethod(&Class::method1));
    delegate.Add(getNonConstMethod(&Class::method1), &constCls2);   // Probably you should not do this
    delegate.Add(getConstMethod(&Class::method1), &constCls2);
    auto lambdaSimpleId = delegate.Add(lambdaSimple1);
    delegate.Add(+lambdaSimple2);                                   // This converts a simple lambda to a function pointer
    auto lambdaComplexId = delegate.Add(lambdaComplex);

    // Calling the delegate
    printf("\nCalling delegate:\n");
    delegate();

    // Finding function indexes (mostly for internal use)
    //kCheckBadIndex(delegate.Find(nullptr));
    //kCheckIndex(delegate.Find(func));
    //kCheckIndex(delegate.Find(&cls1));                              // Functor: calls operator()
    //kCheckIndex(delegate.Find(&constCls1));                         // const Functor: calls operator() const
    //kCheckIndex(delegate.Find(&cls1, &Class::method));
    //kCheckIndex(delegate.Find(&cls1, &Class::constMethod));
    //kCheckIndex(delegate.Find(&cls1, getNonConstMethod(&Class::method1)));
    //kCheckIndex(delegate.Find(&cls1, getConstMethod(&Class::method1)));
    //kCheckIndex(delegate.Find(getNonConstMethod(&Class::method1), &cls2));
    //kCheckIndex(delegate.Find(getConstMethod(&Class::method1), &cls2));
    //kCheckIndex(delegate.Find(&constCls1, &Class::method));
    //kCheckIndex(delegate.Find(&constCls1, &Class::constMethod));
    //kCheckIndex(delegate.Find(&constCls1, getNonConstMethod(&Class::method1)));
    //kCheckIndex(delegate.Find(&constCls1, getConstMethod(&Class::method1)));
    //kCheckIndex(delegate.Find(getNonConstMethod(&Class::method1), &constCls2));
    //kCheckIndex(delegate.Find(getConstMethod(&Class::method1), &constCls2));
    //kCheckBadIndex(delegate.Find(lambdaSimple1));                   // We cannot find lambdas directly
    //kCheckIndex(delegate.Find(+lambdaSimple2));                     // Ok
    ////kCheckBadIndex(delegate.Find(lambdaComplex));                 // We cannot find complex lambdas in any way (compilation error)

    // Removing functions
    printf("\nRemoving functions:\n");
    kCheckFalse(delegate.Remove(nullptr, true));
    kCheckTrue(delegate.Remove(func, true));
    kCheckTrue(delegate.Remove(&cls1, true));                             // Functor: calls operator()
    kCheckTrue(delegate.Remove(&constCls1, true));                        // const Functor: calls operator() const
    kCheckTrue(delegate.Remove(&cls1, &Class::method, true));
    kCheckTrue(delegate.Remove(&cls1, &Class::constMethod, true));
    kCheckTrue(delegate.Remove(&cls1, getNonConstMethod(&Class::method1), true));
    kCheckTrue(delegate.Remove(&cls1, getConstMethod(&Class::method1), true));
    kCheckTrue(delegate.Remove(getNonConstMethod(&Class::method1), &cls2, true));
    kCheckTrue(delegate.Remove(getConstMethod(&Class::method1), &cls2, true));
    kCheckTrue(delegate.Remove(&constCls1, &Class::method, true));
    kCheckTrue(delegate.Remove(&constCls1, &Class::constMethod, true));
    kCheckTrue(delegate.Remove(&constCls1, getNonConstMethod(&Class::method1), true));
    kCheckTrue(delegate.Remove(&constCls1, getConstMethod(&Class::method1), true));
    kCheckTrue(delegate.Remove(getNonConstMethod(&Class::method1), &constCls2, true));
    kCheckTrue(delegate.Remove(getConstMethod(&Class::method1), &constCls2, true));
    kCheckFalse(delegate.Remove(lambdaSimple1, true));                    // We cannot remove lambdas directly
    kCheckTrue(delegate.RemoveById(lambdaSimpleId, true));                // Ok
    kCheckTrue(delegate.Remove(+lambdaSimple2, true));                    // Ok
    //delegate.Remove(lambdaComplex);                               // We cannot remove complex lambdas in any way (compilation error)
    kCheckTrue(delegate.RemoveById(lambdaComplexId, true));               // Ok

    printf("\nCalling delegate:\n");
    delegate();

    delegate.RemoveLazyDeleted();

    // rValues
    Delegate<void(std::string)> delegate2;
    delegate2.Add([](std::string str) { printf(" - lambda with parameters [str = %s]\n", str.c_str()); });
    delegate2.Add([](std::string str) { printf(" - lambda with parameters [str = %s]\n", str.c_str()); });
    delegate2("Hello world!");

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