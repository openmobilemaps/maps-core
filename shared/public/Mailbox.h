//
//  Mailbox.h
//  
//
//  Created by Stefan Mitterrutzner on 08.02.23.
//

#pragma once

#include "SchedulerInterface.h"
#include "LambdaTask.h"
#include <mutex>
#include <deque>
#include <future>
#include "Logger.h"
#include "assert.h"

enum class MailboxDuplicationStrategy {
    none = 0,
    replaceNewest = 1
};

class MailboxMessage {
public:
    MailboxMessage(const MailboxDuplicationStrategy &strategy, void* identifier) : strategy(strategy), identifier(identifier) {}
    virtual ~MailboxMessage() = default;
    virtual void operator()() = 0;
    
    const MailboxDuplicationStrategy strategy;
    const void* identifier;
};

template <class Object, class MemberFn, class ArgsTuple>
class MailboxMessageImpl: public MailboxMessage {
public:
    MailboxMessageImpl(Object object_, MemberFn memberFn_, ArgsTuple argsTuple_)
      : MailboxMessage(MailboxDuplicationStrategy::none, (void*)(void*&) memberFn_),
        object(object_),
        memberFn(memberFn_),
        argsTuple(std::move(argsTuple_)) {
    }
    
    MailboxMessageImpl(Object object_, MemberFn memberFn_, const MailboxDuplicationStrategy &strategy, ArgsTuple argsTuple_)
      : MailboxMessage(strategy, (void*)(void*&) memberFn_),
        object(object_),
        memberFn(memberFn_),
        argsTuple(std::move(argsTuple_)) {
    }
    
    void operator()() override {
        invoke(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>());
    }

    template <std::size_t... I>
    void invoke(std::index_sequence<I...>) {
        ((*object).*memberFn)(std::move(std::get<I>(argsTuple))...);
    }

    Object object;
    MemberFn memberFn;
    ArgsTuple argsTuple;
};

template <class ResultType, class Object, class MemberFn, class ArgsTuple>
class AskMessageImpl : public MailboxMessage {
public:
    AskMessageImpl(std::promise<ResultType> promise_, Object object_, MemberFn memberFn_, ArgsTuple argsTuple_, const MailboxDuplicationStrategy &strategy)
        : MailboxMessage(MailboxDuplicationStrategy::none, (void*)(void*&) memberFn_),
          object(object_),
          memberFn(memberFn_),
          argsTuple(std::move(argsTuple_)),
          promise(std::move(promise_)) {
    }

    AskMessageImpl(std::promise<ResultType> promise_, Object object_, MemberFn memberFn_, const MailboxDuplicationStrategy &strategy, ArgsTuple argsTuple_)
        : MailboxMessage(strategy, (void*)(void*&) memberFn_),
          object(object_),
          memberFn(memberFn_),
          argsTuple(std::move(argsTuple_)),
          promise(std::move(promise_)) {
    }

    void operator()() override {
        promise.set_value(ask(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>()));
    }

    template <std::size_t... I>
    ResultType ask(std::index_sequence<I...>) {
        return ((*object).*memberFn)(std::move(std::get<I>(argsTuple))...);
    }

    Object object;
    MemberFn memberFn;
    ArgsTuple argsTuple;
    std::promise<ResultType> promise;
};

class Mailbox: public std::enable_shared_from_this<Mailbox>  {
public:
    Mailbox(std::shared_ptr<SchedulerInterface> scheduler): scheduler(scheduler) {};
    
    void push(std::unique_ptr<MailboxMessage> message) {
        std::lock_guard<std::mutex> pushingLock(pushingMutex);
        std::lock_guard<std::mutex> queueLock(queueMutex);
        bool wasEmpty = queue.empty();
        
        bool didReplace = false;
        if (message->strategy == MailboxDuplicationStrategy::replaceNewest) {
            for (auto it = queue.begin(); it != queue.end(); it++) {
                if ((*it)->identifier == message->identifier) {
                    assert((*it)->strategy == MailboxDuplicationStrategy::replaceNewest);
                    auto const newIt = queue.erase (it);
                    queue.insert(newIt, std::move(message));
                    didReplace = true;
                    break;
                }
            }
        }
        if (!didReplace) {
            queue.push_back(std::move(message));
        }
        if (wasEmpty) {
            scheduler->addTask(makeTask(shared_from_this()));
        }
    };
    
    void receive() {
        std::lock_guard<std::recursive_mutex> receivingLock(receivingMutex);
        
        std::unique_ptr<MailboxMessage> message;
        bool wasEmpty;
        
        {
            std::lock_guard<std::mutex> queueLock(queueMutex);
            assert(!queue.empty());
            message = std::move(queue.front());
            queue.pop_front();
            wasEmpty = queue.empty();
        }
        
        
        (*message)();
        
        if (!wasEmpty) {
            scheduler->addTask(makeTask(shared_from_this()));
        }
    }
    
    static inline std::shared_ptr<LambdaTask> makeTask(std::weak_ptr<Mailbox> mailbox){
        return std::make_shared<LambdaTask>(TaskConfig("", 0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                            [mailbox = std::move(mailbox)] {
                                                auto selfPtr = mailbox.lock();
                                                if (selfPtr) {
                                                    selfPtr->receive();
                                                }
                                            });
    }
    
private:
    std::shared_ptr<SchedulerInterface> scheduler;
    
    std::recursive_mutex receivingMutex;
    std::mutex pushingMutex;

    std::mutex queueMutex;
    std::deque<std::unique_ptr<MailboxMessage>> queue;
};
