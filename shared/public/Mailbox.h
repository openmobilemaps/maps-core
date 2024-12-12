/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "SchedulerInterface.h"
#include "LambdaTask.h"
#include <mutex>
#include <deque>
#include <future>
#include <typeinfo>
#include "Logger.h"
#include "Hash.h"
#include <functional>
#include <cassert>

enum class MailboxDuplicationStrategy {
    none = 0,
    replaceNewest = 1
};

enum class MailboxExecutionEnvironment {
    computation = 0,
    graphics = 1
};

struct MessageDiagnostics {
    const char * senderFile;
    const char * senderFunction;
    const char * senderLine;
    const char * targetFunction;

    MessageDiagnostics(const char *senderFile, const char *senderFunction, const char *senderLine, const char *targetFunction) : senderFile(senderFile),
                                                                                                           senderFunction(senderFunction),
                                                                                                           senderLine(senderLine),
                                                                                                           targetFunction(targetFunction) {}

    std::string toString() const {
        return std::string(senderFile) + "::" + std::string(senderFunction) + ":" + std::string(senderLine) + " -> " + std::string(targetFunction);
    }
};

template<class MemberFn>
struct MemberFunctionWrapper {
    MemberFn memberFn;
    uint64_t identifier;
    MessageDiagnostics diagnostics;

    MemberFunctionWrapper(MemberFn memberFn, size_t identifier, MessageDiagnostics diagnostics) : memberFn(memberFn), identifier(identifier), diagnostics(diagnostics) {}
};

#define TO_STRING_IMPL(x) #x
#define TO_STRING(x) TO_STRING_IMPL(x)
#define MFN(memberFn) MemberFunctionWrapper(memberFn, const_hash(#memberFn), MessageDiagnostics(__FILE_NAME__, __func__, TO_STRING(__LINE__), #memberFn))

class MailboxMessage {
public:
    MailboxMessage(const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, uint64_t identifier, const MessageDiagnostics diagnostics)
    : strategy(strategy), environment(environment), identifier(identifier), diagnostics(diagnostics) {}
    virtual ~MailboxMessage() = default;
    virtual void operator()() = 0;
    
    const MailboxDuplicationStrategy strategy;
    const MailboxExecutionEnvironment environment;
    const uint64_t identifier;
    const MessageDiagnostics diagnostics;
};

template <class Object, class MemberFn, class ArgsTuple>
class MailboxMessageImpl: public MailboxMessage {
public:
    MailboxMessageImpl(Object object_, MemberFunctionWrapper<MemberFn> memberFn_, const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, ArgsTuple argsTuple_)
      : MailboxMessage(strategy,
                       environment,
                       memberFn_.identifier,
                       memberFn_.diagnostics),
        object(object_),
        memberFn(memberFn_),
        argsTuple(std::move(argsTuple_)) { }

    void operator()() override {
        invoke(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>());
    }

    template <std::size_t... I>
    void invoke(std::index_sequence<I...>) {
        if (auto strongObject = object.lock()) {
            ((*strongObject).*memberFn.memberFn)(std::move(std::get<I>(argsTuple))...);
        } else {
            LogError <<= "Mailbox Object is expired";
        }
    }

    Object object;
    MemberFunctionWrapper<MemberFn> memberFn;
    ArgsTuple argsTuple;
};

template <class ResultType, class Object, class MemberFn, class ArgsTuple>
class AskMessageImpl : public MailboxMessage {
public:
    AskMessageImpl(std::promise<ResultType> promise_, Object object_, MemberFunctionWrapper<MemberFn> memberFn_, const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, ArgsTuple argsTuple_)
        : MailboxMessage(strategy, environment, memberFn_.identifier, memberFn_.diagnostics),
          object(object_),
          memberFn(memberFn_),
          argsTuple(std::move(argsTuple_)),
          promise(std::move(promise_)){}

    void operator()() override {
        promise.set_value(ask(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>()));
    }

    template <std::size_t... I>
    ResultType ask(std::index_sequence<I...>) {
        if (auto strongObject = object.lock()) {
            return ((*strongObject).*memberFn.memberFn)(std::move(std::get<I>(argsTuple))...);
        } else {
            LogError <<= "Mailbox Object is expired";
            throw std::invalid_argument("Mailbox Object is expired");
            return ResultType();
        }
    }

    Object object;
    MemberFunctionWrapper<MemberFn> memberFn;
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
        auto strongScheduler = scheduler.lock();
        if (wasEmpty && strongScheduler) {
            strongScheduler->addTask(makeTask(shared_from_this(), environment));
        }
    };
    
    void receive(const MailboxExecutionEnvironment &environment) {
        // Make sure to never block the graphics queue
        if (environment == MailboxExecutionEnvironment::graphics) {
            if (!receivingMutex.try_lock()) {
                auto strongScheduler = scheduler.lock();
                if (strongScheduler) {
                    strongScheduler->addTask(makeTask(shared_from_this(), environment));
                }
                return;
            }
        } else {
            receivingMutex.lock();
        }
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

        auto strongScheduler = scheduler.lock();
        if (!wasEmpty && strongScheduler) {
            strongScheduler->addTask(makeTask(shared_from_this(), environment));
        }

        receivingMutex.unlock();
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

    std::recursive_mutex receivingMutex;
private:
    std::weak_ptr<SchedulerInterface> scheduler;

    std::mutex pushingMutex;

    std::mutex computationQueueMutex;
    std::deque<std::unique_ptr<MailboxMessage>> computationQueue;
    std::mutex graphicsQueueMutex;
    std::deque<std::unique_ptr<MailboxMessage>> graphicsQueue;
};
