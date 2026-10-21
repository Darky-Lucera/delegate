#include "TestRunner.h"

#include <cstdarg>
#include <cstdio>
#include <string>

// Keeps what the delegate logs, so the tests can check it. It must be defined before including Delegate.h
static std::string gLogged;

static void
LogError(const char *format, ...) {
    char    buffer[256];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    gLogged += buffer;
}

#define DELEGATE_LOG_ERROR(...)     LogError(__VA_ARGS__)

#include "Delegate.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#if __cplusplus >= 201402L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
    #define kCpp14
#endif

using MindShake::Delegate;
using MindShake::GetConstMethod;
using MindShake::GetNonConstMethod;

//-------------------------------------
// Functions and classes used by the tests. Each one appends to gLog, so a test can check
// which functions were called, and in which order.
//-------------------------------------
static std::string gLog;

void FreeA()                        { gLog += "a"; }
void FreeB()                        { gLog += "b"; }
void FreeNoexcept(int v) noexcept   { gLog += "n" + std::to_string(v); }
void Overloaded(int v)              { gLog += "i" + std::to_string(v); }
void Overloaded(float)              { gLog += "f"; }

//-------------------------------------
struct Widget {
    int             id = 0;

    explicit        Widget(int i) : id(i) {}

    void            Method()                        { gLog += "m" + std::to_string(id);  }
    void            ConstMethod() const             { gLog += "c" + std::to_string(id);  }
    void            Overload()                      { gLog += "o" + std::to_string(id);  }
    void            Overload() const                { gLog += "O" + std::to_string(id);  }
    void            Update(int v)                   { gLog += "u" + std::to_string(v);   }
    void            Update(int v) const             { gLog += "U" + std::to_string(v);   }
    void            Update(float)                   { gLog += "uf";                      }
    void            NoexceptMethod(int v) noexcept  { gLog += "n" + std::to_string(v);   }
    void            Lvalue(int v) &                 { gLog += "l" + std::to_string(v);   }
    void            ConstLvalue(int v) const &      { gLog += "L" + std::to_string(v);   }
    void            RefPair(int v) &                { gLog += "r" + std::to_string(v);   }
    void            RefPair(int v) const &          { gLog += "R" + std::to_string(v);   }

    static void     Static()                        { gLog += "s"; }
};

//-------------------------------------
struct Functor {
    void            operator()()                    { gLog += "F"; }
    void            operator()() const              { gLog += "C"; }
};

struct ConstFunctor {
    void            operator()() const              { gLog += "K"; }
};

struct AddFunctor {
    void            operator()(int &value) const    { value += 10; }
};

struct ByValueFunctor {
    int             *calls;
    void            operator()() const              { ++*calls; }
};

//-------------------------------------
struct Entity {
    int             hp = 10;
    void            OnHit(int v)                    { gLog += "h" + std::to_string(hp - v); }
    void            OnClick(int v) &                { gLog += "k" + std::to_string(hp + v); }
};

struct Other {
    virtual         ~Other() = default;
    int             pad[3] = {};
};

// Entity is not the first base class, so a Player * must be adjusted to get the Entity *
struct Player : Other, Entity {};

struct SinglePlayer : Entity {};

//-------------------------------------
struct Shape {
    virtual         ~Shape() = default;
    virtual void    Draw()                          { gLog += "shape"; }
};

struct Circle : Shape {
    void            Draw() override                 { gLog += "circle"; }
};

//-------------------------------------
struct VirtualBase {
    virtual         ~VirtualBase() = default;
    int             vb = 40;
};

struct Left  : virtual VirtualBase { int l = 1; };
struct Right : virtual VirtualBase { int r = 2; };

struct Diamond : Left, Right {
    int             d = 50;
    void            Method(int v)                   { gLog += "d" + std::to_string(d + vb + l + r + v); }
    virtual void    Virtual(int v)                  { gLog += "v" + std::to_string(d + v); }
};

struct DiamondChild : Diamond {
    void            Virtual(int v) override         { gLog += "V" + std::to_string(d + v); }
};

//-------------------------------------
// Removes a function from the delegate when destroyed, like an RAII subscription captured by a lambda
struct Subscription {
    Delegate<void()>    *delegate   = nullptr;
    size_t              idToRemove  = 0;

                        ~Subscription() { delegate->RemoveById(idToRemove); }
};

//-------------------------------------
// Kinds of functions
//-------------------------------------
TEST(FreeFunctions) {
    gLog.clear();
    Delegate<void()> d;

    CHECK(d.Add(&FreeA) != size_t(-1));
    CHECK(d.Add(FreeB) != size_t(-1));
    d();
    CHECK(gLog == "ab");

    CHECK(d.Remove(&FreeA));
    CHECK(!d.Remove(&FreeA));
    gLog.clear();
    d();
    CHECK(gLog == "b");
    CHECK(d.GetNumDelegates() == 1);
}

//-------------------------------------
TEST(NullFunctionsAreRejected) {
    Delegate<void()> d;
    Widget           widget(1);
    Widget          *noObject = nullptr;
    void            (*noFunction)()     = nullptr;
    void            (Widget::*noMethod)() = nullptr;

    CHECK(d.Add(nullptr) == size_t(-1));
    CHECK(d.Add(noFunction) == size_t(-1));
    CHECK(d.Add(std::function<void()>()) == size_t(-1));
    CHECK(d.Add(noObject, &Widget::Method) == size_t(-1));
    CHECK(d.Add(&widget, noMethod) == size_t(-1));
    CHECK(d.GetNumDelegates() == 0);
    CHECK(!d.Remove(nullptr));
}

//-------------------------------------
TEST(RemovingANullFunctionDoesNotRemoveOthers) {
    Delegate<void()> d;
    int              n = 0;
    void            (*noFunction)() = nullptr;

    d.Add([&n] { ++n; });
    d.Add(std::function<void()>([&n] { ++n; }));
    CHECK(!d.Remove(noFunction));
    CHECK(d.GetNumDelegates() == 2);
}

//-------------------------------------
TEST(StaticOverloadedAndNoexceptFunctions) {
    gLog.clear();
    Delegate<void()>      d0;
    Delegate<void(int)>   d1;
    Delegate<void(float)> d2;

    d0.Add(&Widget::Static);
    d1.Add(&FreeNoexcept);
    d1.Add(&Overloaded);                                        // Picks Overloaded(int)
    d2.Add(&Overloaded);                                        // Picks Overloaded(float)
    d0();
    d1(1);
    d2(1.0f);
    CHECK(gLog == "sn1i1f");

    CHECK(d0.Remove(&Widget::Static));
    CHECK(d1.Remove(&FreeNoexcept));
    CHECK(d1.Remove(&Overloaded));
    CHECK(d2.Remove(&Overloaded));
}

//-------------------------------------
TEST(Methods) {
    gLog.clear();
    Delegate<void()> d;
    Widget           w1(1);
    const Widget     w2(2);

    d.Add(&w1, &Widget::Method);
    d.Add(&w1, &Widget::ConstMethod);
    d.Add(&w2, &Widget::ConstMethod);
    d.Add(&Widget::Method, &w1);                                // std::bind order
    d();
    CHECK(gLog == "m1c1c2m1");

    CHECK(d.Remove(&w1, &Widget::Method));
    CHECK(d.Remove(&w1, &Widget::ConstMethod));
    CHECK(d.Remove(&Widget::ConstMethod, &w2));                 // std::bind order
    CHECK(d.Remove(&Widget::Method, &w1));
    CHECK(d.GetNumDelegates() == 0);
}

//-------------------------------------
TEST(ConstAndNonConstOverloads) {
    gLog.clear();
    Delegate<void()>    d0;
    Delegate<void(int)> d1;
    Widget              w(3);

    d0.Add(&w, GetNonConstMethod(&Widget::Overload));
    d0.Add(&w, GetConstMethod(&Widget::Overload));
    d1.Add(&w, GetConstMethod(&Widget::Update));                // Only one const Update()
    d1.Add(&w, GetNonConstMethod<Widget, int>(&Widget::Update)); // Two non-const Update(): give the parameters
    d0();
    d1(4);
    CHECK(gLog == "o3O3U4u4");

    CHECK(d0.Remove(&w, GetNonConstMethod(&Widget::Overload)));
    CHECK(d0.Remove(&w, GetConstMethod(&Widget::Overload)));
    CHECK(d1.Remove(&w, GetConstMethod(&Widget::Update)));
    CHECK(d1.Remove(&w, GetNonConstMethod<Widget, int>(&Widget::Update)));
}

//-------------------------------------
TEST(NoexceptAndRefQualifiedMethods) {
    gLog.clear();
    Delegate<void(int)> d;
    Widget              w(0);
    const Widget       &cw = w;

    d.Add(&w, &Widget::NoexceptMethod);
    d.Add(&w, &Widget::Lvalue);
    d.Add(&cw, &Widget::ConstLvalue);
    d.Add(&w, static_cast<void (Widget::*)(int) &>(&Widget::RefPair));
    d.Add(&w, static_cast<void (Widget::*)(int) const &>(&Widget::RefPair));
    d(5);
    CHECK(gLog == "n5l5L5r5R5");

    CHECK(d.Remove(&w, &Widget::NoexceptMethod));
    CHECK(d.Remove(&w, &Widget::Lvalue));
    CHECK(d.Remove(&w, &Widget::ConstLvalue));                  // Found through a non-const pointer too
    CHECK(d.Remove(&w, static_cast<void (Widget::*)(int) &>(&Widget::RefPair)));
    CHECK(d.Remove(&cw, static_cast<void (Widget::*)(int) const &>(&Widget::RefPair)));
    CHECK(d.GetNumDelegates() == 0);
}

//-------------------------------------
TEST(MethodsOfABaseClass) {
    gLog.clear();
    Delegate<void(int)> d;
    SinglePlayer        single;
    Player              multiple;

    d.Add(&single, &SinglePlayer::OnHit);                       // OnHit is declared in Entity
    d.Add(&multiple, &Player::OnHit);
    d.Add(&multiple, &Player::OnClick);
    d(1);
    CHECK(gLog == "h9h9k11");

    CHECK(d.Remove(static_cast<Entity *>(&single), &Entity::OnHit));
    CHECK(d.Remove(static_cast<Entity *>(&multiple), &Entity::OnHit));
    CHECK(d.Remove(&multiple, &Entity::OnClick));
    CHECK(d.GetNumDelegates() == 0);
}

//-------------------------------------
TEST(VirtualMethodsAndVirtualInheritance) {
    gLog.clear();
    Delegate<void()>    d0;
    Delegate<void(int)> d1;
    Circle              circle;
    DiamondChild        child;
    Diamond            *diamond = &child;

    d0.Add(static_cast<Shape *>(&circle), &Shape::Draw);        // Calls the override
    d1.Add(diamond, &Diamond::Method);
    d1.Add(diamond, &Diamond::Virtual);
    d0();
    d1(1);
    CHECK(gLog == "circled94V51");

    CHECK(d0.Remove(&circle, &Shape::Draw));
    CHECK(d1.Remove(&child, &Diamond::Method));
    CHECK(d1.Remove(diamond, &Diamond::Virtual));
}

//-------------------------------------
TEST(Functors) {
    gLog.clear();
    Delegate<void()> d;
    Functor          functor;
    const Functor    constFunctor = {};
    ConstFunctor     onlyConst;
    int              calls = 0;

    d.Add(&functor);                                            // Calls operator()
    d.Add(&constFunctor);                                       // Calls operator() const
    d.Add(&onlyConst);                                          // Only has operator() const
    size_t byValue = d.Add(ByValueFunctor { &calls });          // The delegate stores a copy
    d();
    CHECK(gLog == "FCK");
    CHECK(calls == 1);

    CHECK(d.Remove(&functor));
    CHECK(d.Remove(&constFunctor));
    CHECK(d.Remove(&onlyConst));
    CHECK(d.RemoveById(byValue));
    CHECK(d.GetNumDelegates() == 0);

    Delegate<void(int &)> withParameters;
    AddFunctor            add;
    int                   value = 1;
    withParameters.Add(&add);
    withParameters(value);
    CHECK(value == 11);
    CHECK(withParameters.Remove(&add));
}

//-------------------------------------
TEST(Lambdas) {
    gLog.clear();
    Delegate<void()> d;
    int              counter = 0;

    auto simple       = [] { gLog += "1"; };
    auto withCaptures = [&counter] { ++counter; };
    auto byPointer    = [&counter] { counter += 10; };
    auto noexceptOne  = []() noexcept { gLog += "2"; };
    auto mutableOne   = [counter]() mutable { gLog += std::to_string(++counter); };

    d.Add(simple);
    d.Add(+[] { gLog += "+"; });
    size_t capturesId = d.Add(withCaptures);
    d.Add(&byPointer);
    d.Add(noexceptOne);
    size_t mutableId = d.Add(mutableOne);
    d();
    d();
    CHECK(gLog == "1+211+22");                                  // The mutable lambda keeps its own copy of counter
    CHECK(counter == 22);

    CHECK(d.Remove(simple));                                    // Stored as a function
    CHECK(d.RemoveById(capturesId));
    CHECK(d.Remove(&byPointer));
    CHECK(d.Remove(noexceptOne));
    CHECK(d.RemoveById(mutableId));
    CHECK(d.GetNumDelegates() == 1);                            // The +lambda: no way to name it again

#if defined(kCpp14)
    Delegate<void(int)> generic;
    auto genericLambda = [](auto v) { gLog += "g" + std::to_string(int(v)); };
    gLog.clear();
    generic.Add(genericLambda);
    generic(7);
    CHECK(gLog == "g7");
    CHECK(generic.Remove(genericLambda));
#endif
}

//-------------------------------------
TEST(StdFunctionAndStdBind) {
    gLog.clear();
    Delegate<void()> d;
    Widget           w(8);

    size_t functionId = d.Add(std::function<void()>(&FreeA));
    size_t bindId     = d.Add(std::bind(&Widget::Method, &w));
    d();
    CHECK(gLog == "am8");
    CHECK(d.RemoveById(functionId));
    CHECK(d.RemoveById(bindId));
    CHECK(!d.RemoveById(bindId));
}

//-------------------------------------
TEST(Arguments) {
    Delegate<void(int &)> byReference;
    int                   value = 0;
    byReference.Add([](int &v) { ++v; });
    byReference.Add([](int &v) { v *= 10; });
    byReference(value);
    CHECK(value == 10);

    Delegate<void(std::string)> byValue;
    std::string                 received;
    byValue.Add([&received](std::string s) { received += s; });
    byValue.Add([&received](std::string s) { received += s; });
    byValue(std::string("x"));                                  // Every function gets the same value
    CHECK(received == "xx");
}

//-------------------------------------
// Behavior
//-------------------------------------
TEST(FunctionsAreCalledInOrder) {
    gLog.clear();
    Delegate<void()> d;
    d.Add([] { gLog += "1"; });
    d.Add([] { gLog += "2"; });
    d.Add([] { gLog += "3"; });
    size_t id = d.Add([] { gLog += "4"; });
    d.Add([] { gLog += "5"; });
    d.RemoveById(id);
    d();
    CHECK(gLog == "1235");
}

//-------------------------------------
TEST(RemoveDuringACall) {
    gLog.clear();
    Delegate<void()> d;
    size_t           ids[6] = {};
    size_t           addedId = 0;
    size_t           countDuringCall = 0;
    bool             first = true;

    ids[0] = d.Add([] { gLog += "0"; });
    ids[1] = d.Add([&] {
        gLog += "1";
        if (first) {
            first = false;
            d.RemoveById(ids[0]);                               // Already called
            d.RemoveById(ids[3]);                               // Not called yet
            d.RemoveById(ids[5]);                               // The last one
            d.RemoveById(ids[5]);                               // Twice: must be ignored
            addedId = d.Add([] { gLog += "A"; });               // Added and removed during the same call
            d.Add([] { gLog += "B"; });
            d.RemoveById(addedId);
            countDuringCall = d.GetNumDelegates();
        }
    });
    ids[2] = d.Add([] { gLog += "2"; });
    ids[3] = d.Add([] { gLog += "3"; });
    ids[4] = d.Add([] { gLog += "4"; });
    ids[5] = d.Add([] { gLog += "5"; });

    d();
    CHECK(gLog == "0124");                                      // Removed: not called. Added: from the next call
    CHECK(countDuringCall == 4);
    CHECK(d.GetNumDelegates() == 4);

    gLog.clear();
    d();
    CHECK(gLog == "124B");
    CHECK(!d.RemoveById(ids[0]) && !d.RemoveById(ids[3]) && !d.RemoveById(ids[5]) && !d.RemoveById(addedId));
}

//-------------------------------------
TEST(RemoveByIdTwiceDuringACall) {
    gLog.clear();
    Delegate<void()> d;
    size_t           id = 0;

    d.Add([] { gLog += "1"; });
    id = d.Add([&] { d.RemoveById(id); d.RemoveById(id); });
    d.Add([] { gLog += "3"; });
    d();
    CHECK(gLog == "13");
    CHECK(d.GetNumDelegates() == 2);                            // Only the one asked for is removed
}

//-------------------------------------
TEST(ClearOutsideAndDuringACall) {
    gLog.clear();
    Delegate<void()> d;
    d.Add(&FreeA);
    d.Add(&FreeB);
    d.Clear();
    d();
    CHECK(gLog.empty());
    CHECK(d.GetNumDelegates() == 0);

    d.Add([&d] { gLog += "x"; d.Clear(); d.Add([] { gLog += "y"; }); });
    d.Add([] { gLog += "z"; });
    d();
    CHECK(gLog == "x");                                         // z was cleared before its turn
    CHECK(d.GetNumDelegates() == 1);

    gLog.clear();
    d();
    CHECK(gLog == "y");
}

//-------------------------------------
TEST(CallsWhileRunningAreIgnored) {
    Delegate<void(int)> d;
    int                 calls = 0;

    d.Add([&d, &calls](int depth) {
        ++calls;
        if (depth == 0)
            d(1);
    });
    d(0);
    CHECK(calls == 1);
}

//-------------------------------------
TEST(ConstCall) {
    gLog.clear();
    Delegate<void()>        d;
    const Delegate<void()> &cd = d;
    size_t                  a = d.Add([] { gLog += "a"; });

    d.Add([&d, &a] { gLog += "b"; d.RemoveById(a); });
    cd();
    CHECK(gLog == "ab");
    CHECK(d.GetNumDelegates() == 1);

    gLog.clear();
    cd();
    CHECK(gLog == "b");                                         // Not called again, even if not freed yet
    d();                                                        // Frees it
    CHECK(gLog == "bb");
    CHECK(d.GetNumDelegates() == 1);
}

//-------------------------------------
TEST(DestructorUsesTheDelegateWhileItDeletes) {
    // A wrapper was already freed in the same pass when the destructor walks the vector
    {
        gLog.clear();
        Delegate<void()> d;
        size_t           x = d.Add([] { gLog += "X"; });
        size_t           h = 0;
        {
            std::shared_ptr<Subscription> sub = std::make_shared<Subscription>();
            sub->delegate = &d;
            h = d.Add([sub] { gLog += "H"; });                  // The only owner of sub after this block
            sub->idToRemove = d.Add([] { gLog += "K"; });
        }
        d.Add([&] { d.RemoveById(x); d.RemoveById(h); });
        d();
        d();
        CHECK(gLog == "XHK");
        CHECK(d.GetNumDelegates() == 1);
    }

    // The compaction moved a kept wrapper before the one being deleted
    {
        gLog.clear();
        Delegate<void()> d;
        d.Add([] { gLog += "1"; });
        size_t x  = d.Add([] { gLog += "X"; });
        size_t k2 = d.Add([] { gLog += "2"; });
        size_t h  = 0;
        {
            std::shared_ptr<Subscription> sub = std::make_shared<Subscription>();
            sub->delegate   = &d;
            sub->idToRemove = k2;
            h = d.Add([sub] { gLog += "H"; });
        }
        d.Add([&] { d.RemoveById(x); d.RemoveById(h); });
        d();
        CHECK(gLog == "1X2H");
        CHECK(d.GetNumDelegates() == 2);

        gLog.clear();
        d();
        CHECK(gLog == "1");
    }
}

//-------------------------------------
TEST(ExceptionsAreCaughtAndLogged) {
    gLog.clear();
    gLogged.clear();
    Delegate<void()> d;

    d.Add([] { throw std::runtime_error("boom"); });
    d.Add(+[] { throw 42; });
    d.Add([] { gLog += "after"; });
    d();
    CHECK(gLog == "after");                                     // The next functions still run
    CHECK(gLogged.find("boom") != std::string::npos);
    CHECK(gLogged.find("Unknown exception") != std::string::npos);
}

//-------------------------------------
TEST(Move) {
    gLog.clear();
    Delegate<void()> source;
    size_t           a = source.Add([] { gLog += "a"; });

    Delegate<void()> target(std::move(source));
    CHECK(source.GetNumDelegates() == 0);
    CHECK(target.GetNumDelegates() == 1);
    target();
    CHECK(gLog == "a");

    Delegate<void()> assigned;
    assigned.Add([] { gLog += "lost"; });
    assigned = std::move(target);
    gLog.clear();
    assigned();
    CHECK(gLog == "a");                                         // The old functions of assigned are gone

    size_t b = assigned.Add([] {});
    CHECK(b != a);                                              // Ids stay unique after a move
    CHECK(assigned.RemoveById(a) && assigned.RemoveById(b));

    struct Button {
        Delegate<void()> onPressed;
    };
    std::vector<Button> buttons;
    buttons.emplace_back();
    buttons[0].onPressed.Add([] { gLog += "p"; });
    buttons.emplace_back();                                     // Moves the first Button
    buttons.emplace_back();
    gLog.clear();
    buttons[0].onPressed();
    CHECK(gLog == "p");
}

//-------------------------------------
TEST(IdsBelongToEachDelegate) {
    Delegate<void()> d1;
    Delegate<void()> d2;

    size_t a1 = d1.Add([] {});
    size_t b1 = d1.Add([] {});
    size_t a2 = d2.Add([] {});
    CHECK(a1 != b1);
    CHECK(a1 == a2);                                            // Each delegate counts on its own
    CHECK(!d1.RemoveById(1000));
}

//-------------------------------------
int
main() {
    return TestRunner::RunAll();
}
