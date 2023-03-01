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
#include "assert.h"

enum class MailboxDuplicationStrategy {
    none = 0,
    replaceNewest = 1
};

enum class MailboxExecutionEnvironment {
    computation = 0,
    graphics = 1
};

class MailboxMessage {
public:
    MailboxMessage(const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, void* identifier)
    : strategy(strategy), environment(environment), identifier(identifier) {}
    virtual ~MailboxMessage() = default;
    virtual void operator()() = 0;
    
    const MailboxDuplicationStrategy strategy;
    const MailboxExecutionEnvironment environment;
    const void* identifier;
};

template <class Object, class MemberFn, class ArgsTuple>
class MailboxMessageImpl: public MailboxMessage {
public:
    MailboxMessageImpl(Object object_, MemberFn memberFn_, const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, ArgsTuple argsTuple_)
      : MailboxMessage(strategy, environment, (void*)(void*&) memberFn_),
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
    AskMessageImpl(std::promise<ResultType> promise_, Object object_, MemberFn memberFn_, const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, ArgsTuple argsTuple_)
        : MailboxMessage(strategy, environment, (void*)(void*&) memberFn_),
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
        bool wasEmpty = false;

        auto pushLambda = [&message, &wasEmpty](std::mutex& mutex, std::deque<std::unique_ptr<MailboxMessage>> &queue) {
            bool didReplace = false;
            std::lock_guard<std::mutex> queueLock(mutex);
            wasEmpty = queue.empty();
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
        };

        auto environment = message->environment;
        switch (environment) {
            case MailboxExecutionEnvironment::computation:
                pushLambda(computationQueueMutex, computationQueue);
                break;
            case MailboxExecutionEnvironment::graphics:
                pushLambda(graphicsQueueMutex, graphicsQueue);
                break;
        }

        if (wasEmpty) {
            scheduler->addTask(makeTask(shared_from_this(), environment));
        }
    };
    
    void receive(const MailboxExecutionEnvironment &environment) {
        std::lock_guard<std::recursive_mutex> receivingLock(receivingMutex);
        std::unique_ptr<MailboxMessage> message;
        bool wasEmpty;


        auto receiveLambda = [&message, &wasEmpty](std::mutex& mutex, std::deque<std::unique_ptr<MailboxMessage>> &queue) {
            std::lock_guard<std::mutex> queueLock(mutex);
            assert(!queue.empty());
            message = std::move(queue.front());
            queue.pop_front();
            wasEmpty = queue.empty();
        };

        switch (environment) {
            case MailboxExecutionEnvironment::computation:
                receiveLambda(computationQueueMutex, computationQueue);
                break;
            case MailboxExecutionEnvironment::graphics:
                receiveLambda(graphicsQueueMutex, graphicsQueue);
                break;
        }
        (*message)();
        
        if (!wasEmpty) {
            scheduler->addTask(makeTask(shared_from_this(), environment));
        }
    }
    
    static inline std::shared_ptr<LambdaTask> makeTask(std::weak_ptr<Mailbox> mailbox, MailboxExecutionEnvironment environment){
        ExecutionEnvironment executionEnvironment;
        switch (environment) {
            case MailboxExecutionEnvironment::computation:
                executionEnvironment = ExecutionEnvironment::COMPUTATION;
                break;
            case MailboxExecutionEnvironment::graphics:
                executionEnvironment = ExecutionEnvironment::GRAPHICS;
                break;
        }
        return std::make_shared<LambdaTask>(TaskConfig("", 0, TaskPriority::NORMAL, executionEnvironment),
                                            [environment = std::move(environment), mailbox = std::move(mailbox)] {
                                                auto selfPtr = mailbox.lock();
                                                if (selfPtr) {
                                                    selfPtr->receive(environment);
                                                }
                                            });
    }
    
private:
    std::shared_ptr<SchedulerInterface> scheduler;
    
    std::recursive_mutex receivingMutex;
    std::mutex pushingMutex;

    std::mutex computationQueueMutex;
    std::deque<std::unique_ptr<MailboxMessage>> computationQueue;
    std::mutex graphicsQueueMutex;
    std::deque<std::unique_ptr<MailboxMessage>> graphicsQueue;
};
