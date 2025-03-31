#include "Actor.h"
#include "Mailbox.h"
#include "helper/TestScheduler.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Actor Mailbox with synthetic Scheduler") {
    auto scheduler = std::make_shared<TestScheduler>();

    class Foo : public ActorObject {
      public:
        Foo()
            : value(0) {}

        void increment() { value++; }
        void add(int64_t x) { value += x; }
        void set(int64_t x) { value = x; }
        void times2() { value *= 2; }

        int64_t value;
    };

    Actor<Foo> fooActor(std::make_shared<Mailbox>(scheduler));

    SECTION("single message") {
        fooActor.message(MFN(&Foo::increment));

        scheduler->drain();

        REQUIRE(fooActor.unsafe()->value == 1);
    }
    SECTION("multiple messages") {
        fooActor.message(MFN(&Foo::increment));
        fooActor.message(MFN(&Foo::add), 5);
        fooActor.message(MFN(&Foo::times2));

        scheduler->drain();

        REQUIRE(fooActor.unsafe()->value == (0 + 1 + 5) * 2);
    }
    SECTION("two messages replace") {
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 5);
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 3);

        scheduler->drain();

        REQUIRE(fooActor.unsafe()->value == 3);
    }
    SECTION("two message NOT replace") {
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 5);
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::increment));

        scheduler->drain();

        REQUIRE(fooActor.unsafe()->value == 6);
    }
    SECTION("unrelated messages don't replace") {
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::set), 2);
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 5);
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::set), 10);
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 3);

        scheduler->drain();

        // Expected sequence of operations:
        Foo expected;
        expected.set(10);
        expected.add(3);

        REQUIRE(fooActor.unsafe()->value == expected.value);
    }
    SECTION("message replace, pushed from different stack") {
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 5);
        std::thread temporary_thread(
            [&fooActor]() { fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 3); });
        temporary_thread.join();

        scheduler->drain();

        REQUIRE(fooActor.unsafe()->value == 3);
    }
    SECTION("two messages replace with interleaved others") {
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 5);
        fooActor.message(MFN(&Foo::increment));
        fooActor.message(MFN(&Foo::times2));
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 100);
        fooActor.message(MFN(&Foo::times2));
        fooActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Foo::add), 3);

        scheduler->drain();

        // Expected sequence of operations:
        Foo expected;
        expected.add(3); // add(3) replaces the add(5) that was first. add(100) was here but replaced in turn.
        expected.increment();
        expected.times2();
        expected.times2();

        REQUIRE(fooActor.unsafe()->value == expected.value);
    }

    SECTION("weakling") {
        WeakActor<Foo> weakFooActor = fooActor.weakActor<Foo>();

        weakFooActor.message(MFN(&Foo::add), 5);

        scheduler->drain();
        REQUIRE(fooActor.unsafe()->value == 5);
    }
}
