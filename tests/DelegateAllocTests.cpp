#include "TestRunner.h"
#include "Delegate.h"

#include <cstdlib>
#include <functional>
#include <memory>
#include <new>

// This file replaces the global operator new, so it can make an allocation fail on purpose.
// That is why these tests are in their own executable.

using MindShake::Delegate;

static int gAllocations  = 0;
static int gLive         = 0;
static int gFailAt       = -1;

void *
operator new(size_t size) {
    if (++gAllocations == gFailAt) {
        throw std::bad_alloc();
    }

    if (void *p = std::malloc(size == 0 ? 1 : size)) {
        ++gLive;
        return p;
    }

    throw std::bad_alloc();
}

void operator delete(void *p) noexcept              { if (p != nullptr) --gLive; std::free(p); }
void operator delete(void *p, size_t) noexcept      { if (p != nullptr) --gLive; std::free(p); }

static void Free() {}

// Runs action when destroyed, like a captured object whose destructor uses the delegate
struct OnDestroy {
    std::function<void()>   action;

                            ~OnDestroy() { action(); }
};

//-------------------------------------
TEST(FailedAddDoesNotLeak) {
    int liveBefore = gLive;
    {
        Delegate<void()> d;
        int              captured = 1;

        // Each Add allocates the wrapper first, and then the buffer of the vector. Make the second one fail.
        gFailAt = gAllocations + 2;
        bool threw = false;
        try {
            d.Add([captured] { (void) captured; });
        }
        catch (const std::bad_alloc &) {
            threw = true;
        }
        CHECK(threw);
        CHECK(d.GetNumDelegates() == 0);

        gFailAt = gAllocations + 2;
        threw = false;
        try {
            d.Add(&Free);
        }
        catch (const std::bad_alloc &) {
            threw = true;
        }
        CHECK(threw);
        CHECK(d.GetNumDelegates() == 0);

        gFailAt = -1;
        CHECK(d.Add(&Free) != size_t(-1));                      // Still usable
        CHECK(d.GetNumDelegates() == 1);
    }
    CHECK(gLive == liveBefore);
}

//-------------------------------------
TEST(FailedWrapperAllocationDoesNotLeak) {
    int liveBefore = gLive;
    {
        Delegate<void()> d;
        gFailAt = gAllocations + 1;                             // The wrapper itself
        bool threw = false;
        try {
            d.Add(&Free);
        }
        catch (const std::bad_alloc &) {
            threw = true;
        }
        gFailAt = -1;
        CHECK(threw);
        CHECK(d.GetNumDelegates() == 0);
    }
    CHECK(gLive == liveBefore);
}

//-------------------------------------
// When the destructor of the delegate deletes the lambda, it has already taken out its functions.
// A function that goes into the delegate after that is never freed.
TEST(AddDuringTheDestructorDoesNotLeak) {
    int    liveBefore = gLive;
    size_t addedId    = 0;
    {
        Delegate<void()> d;
        auto             onDestroy = std::make_shared<OnDestroy>();
        onDestroy->action = [&] { addedId = d.Add(&Free); };
        d.Add([onDestroy] {});
    }
    CHECK(addedId == size_t(-1));
    CHECK(gLive == liveBefore);
}

//-------------------------------------
TEST(MoveDuringTheDestructorDoesNotLeak) {
    int liveBefore = gLive;
    {
        Delegate<void()> source;
        source.Add(&Free);
        {
            Delegate<void()> d;
            auto             onDestroy = std::make_shared<OnDestroy>();
            onDestroy->action = [&] { d = std::move(source); };
            d.Add([onDestroy] {});
        }
        CHECK(source.GetNumDelegates() == 1);                   // The move was ignored
    }
    CHECK(gLive == liveBefore);
}

//-------------------------------------
int
main() {
    return TestRunner::RunAll();
}
