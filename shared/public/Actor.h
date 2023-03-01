//
//  Actor.h
//  
//
//  Created by Stefan Mitterrutzner on 08.02.23.
//

#pragma once

#include "SchedulerInterface.h"
#include "LambdaTask.h"
#include "Mailbox.h"
#include <mutex>
#include <queue>
#include <future>
#include "Logger.h"

class ActorObject {
protected:
    constexpr ActorObject() { }
    ActorObject(ActorObject const&) { }
    ActorObject& operator=(ActorObject const&) {
        return *this;
    }

public:
    std::shared_ptr<Mailbox> mailbox;
};

template <class Object, class MemberFn, class... Args>
inline std::unique_ptr<MailboxMessage> makeMessage(const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, Object& object, MemberFn memberFn, Args&&... args) {
    auto tuple = std::make_tuple(std::forward<Args>(args)...);
    return std::make_unique<MailboxMessageImpl<Object, MemberFn, decltype(tuple)>>(object, memberFn, strategy, environment, std::move(tuple));
}

template <class ResultType, class Object, class MemberFn, class... Args>
std::unique_ptr<MailboxMessage> makeAskMessage(const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, std::promise<ResultType>&& promise, Object& object, MemberFn memberFn, Args&&... args) {
    auto tuple = std::make_tuple(std::forward<Args>(args)...);
    return std::make_unique<AskMessageImpl<ResultType, Object, MemberFn, decltype(tuple)>>(std::move(promise), object, memberFn, strategy, environment, std::move(tuple));
}

template <class Object>
class WeakActor {
public:
    WeakActor(std::weak_ptr<Mailbox> receivingMailbox, std::weak_ptr<Object> object): receivingMailbox(receivingMailbox), object(object) {
        auto strong = object.lock();
        if (strong) {
            auto casted = std::dynamic_pointer_cast<ActorObject>(strong);

            assert(casted);

            if (!casted->mailbox) {
                casted->mailbox = receivingMailbox.lock();
            }

            assert(casted->mailbox == receivingMailbox.lock());
        }
    }
    
    WeakActor(): object(), receivingMailbox() {}
    
    constexpr explicit operator bool() noexcept {
        return receivingMailbox.lock() && object.lock();
    }

    void emplaceObject(std::weak_ptr<Mailbox> mailbox, std::weak_ptr<Object> object) {
        this->receivingMailbox = mailbox;
        this->object = object;

        auto strong = object.lock();
        if (strong) {
            auto casted = std::dynamic_pointer_cast<ActorObject>(strong);

            assert(casted);

            assert(casted->mailbox == receivingMailbox);
        }
    }

    template <typename Fn, class... Args>
    inline void message(Fn fn, Args&&... args) const {
        auto strongObject = object.lock();
        auto strongMailbox = receivingMailbox.lock();
        if (strongObject && strongMailbox) {
            strongMailbox->push(makeMessage(MailboxDuplicationStrategy::none, MailboxExecutionEnvironment::computation, strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
    }

    template <typename Fn, class... Args>
    inline void message(const MailboxDuplicationStrategy &strategy, Fn fn, Args&&... args) const {
        auto strongObject = object.lock();
        auto strongMailbox = receivingMailbox.lock();
        if (strongObject && strongMailbox) {
            strongMailbox->push(makeMessage(strategy, MailboxExecutionEnvironment::computation, strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
    }

    template <typename Fn, class... Args>
    inline void message(const MailboxExecutionEnvironment &environment, Fn fn, Args&&... args) const {
        auto strongObject = object.lock();
        auto strongMailbox = receivingMailbox.lock();
        if (strongObject && strongMailbox) {
            strongMailbox->push(makeMessage(MailboxDuplicationStrategy::none, environment, strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
    }

    template <typename Fn, class... Args>
    inline void message(const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, Fn fn, Args&&... args) const {
        auto strongObject = object.lock();
        auto strongMailbox = receivingMailbox.lock();
        if (strongObject && strongMailbox) {
            strongMailbox->push(makeMessage(strategy, environment, strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
    }

    
    template <typename Fn, class... Args>
    auto converse(Fn fn, Args&&... args) const {
        using ResultType = std::invoke_result_t<decltype(fn), Object, Args...>;

        auto strongObject = object.lock();
        auto strongMailbox = receivingMailbox.lock();

        std::promise<ResultType> promise;
        auto future = promise.get_future();

        if (strongObject && strongMailbox) {
            strongMailbox->push(makeAskMessage(MailboxDuplicationStrategy::none, MailboxExecutionEnvironment::computation, std::move(promise), strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
        return future;
    }

    template <typename Fn, class... Args>
    auto converse(const MailboxExecutionEnvironment &environment, Fn fn, Args&&... args) const {
        using ResultType = std::invoke_result_t<decltype(fn), Object, Args...>;

        auto strongObject = object.lock();
        auto strongMailbox = receivingMailbox.lock();

        std::promise<ResultType> promise;
        auto future = promise.get_future();

        if (strongObject && strongMailbox) {
            strongMailbox->push(makeAskMessage(MailboxDuplicationStrategy::none, environment, std::move(promise), strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
        return future;
    }
    
    template <class CastObject>
    WeakActor<CastObject> weakActor() const {
        auto casted = std::static_pointer_cast<CastObject>(object.lock());
        return WeakActor<CastObject>(receivingMailbox, casted);
    }

    template <typename Fn>
    auto syncAccess(Fn lambda) const {
        auto strongMailbox = receivingMailbox.lock();
        if (!strongMailbox) {
            return lambda(object);
        }

        std::lock_guard<std::recursive_mutex> lock(strongMailbox->receivingMutex);
        return lambda(object);
    }
    
    inline const std::shared_ptr<Object>& unsafe() const {
        return object.lock();
    }
    
private:
    std::weak_ptr<Object> object;
    std::weak_ptr<Mailbox> receivingMailbox;
};

template <class Object>
class Actor {
public:
    template <class... Args>
    Actor(std::shared_ptr<Mailbox> mailbox, Args&&... args): receivingMailbox(mailbox), object(std::make_shared<Object>(std::forward<Args>(args)...)) {
        assert(std::dynamic_pointer_cast<ActorObject>(object));
        object->mailbox = mailbox;
    }

    Actor(std::shared_ptr<Mailbox> mailbox, std::shared_ptr<Object> object): receivingMailbox(mailbox), object(object) {
        assert(std::dynamic_pointer_cast<ActorObject>(object));

        if (!object->mailbox) {
            object->mailbox = mailbox;
        }
        assert(object->mailbox == mailbox);
    }

    Actor(): object(nullptr), receivingMailbox(nullptr) {}
    
    constexpr explicit operator bool() noexcept {
        return object && receivingMailbox;
    }
    
    template <class... Args>
    void emplaceObject(std::shared_ptr<Mailbox> mailbox, Args&&... args) {
        this->receivingMailbox = mailbox;
        object = std::make_shared<Object>(std::forward<Args>(args)...);

        assert(std::dynamic_pointer_cast<ActorObject>(object));
        
        object->mailbox = mailbox;
    }
    
    void emplaceObject(std::shared_ptr<Mailbox> mailbox, std::shared_ptr<Object> object) {
        this->receivingMailbox = mailbox;
        this->object = object;

        assert(std::dynamic_pointer_cast<ActorObject>(object));

        if (!object->mailbox) {
            object->mailbox = mailbox;
        }

        assert(object->mailbox == mailbox);
    }


    template <typename Fn, class... Args>
    inline void message(Fn fn, Args&&... args) const {
        if(!receivingMailbox || !object) {
            return;
        }
        receivingMailbox->push(makeMessage(MailboxDuplicationStrategy::none, MailboxExecutionEnvironment::computation, object, fn, std::forward<Args>(args)...));
    }

    
    template <typename Fn, class... Args>
    inline void message(const MailboxDuplicationStrategy &strategy, Fn fn, Args&&... args) const {
        if(!receivingMailbox || !object) {
            return;
        }
        receivingMailbox->push(makeMessage(strategy, MailboxExecutionEnvironment::computation, object, fn, std::forward<Args>(args)...));
    }


    template <typename Fn, class... Args>
    inline void message(const MailboxExecutionEnvironment &environment, Fn fn, Args&&... args) const {
        if(!receivingMailbox || !object) {
            return;
        }
        receivingMailbox->push(makeMessage(MailboxDuplicationStrategy::none, environment, object, fn, std::forward<Args>(args)...));
    }


    template <typename Fn, class... Args>
    inline void message(const MailboxDuplicationStrategy &strategy, const MailboxExecutionEnvironment &environment, Fn fn, Args&&... args) const {
        if(!receivingMailbox || !object) {
            return;
        }
        receivingMailbox->push(makeMessage(strategy, environment, object, fn, std::forward<Args>(args)...));
    }

    template <typename Fn, class... Args>
    auto converse(Fn fn, Args&&... args) const {
        using ResultType = std::invoke_result_t<decltype(fn), Object, Args...>;
        
        std::promise<ResultType> promise;
        auto future = promise.get_future();
        receivingMailbox->push(makeAskMessage(MailboxDuplicationStrategy::none, MailboxExecutionEnvironment::computation, std::move(promise), object, fn, std::forward<Args>(args)...));
        return future;
    }

    template <typename Fn, class... Args>
    auto converse(const MailboxExecutionEnvironment &environment, Fn fn, Args&&... args) const {
        using ResultType = std::invoke_result_t<decltype(fn), Object, Args...>;

        std::promise<ResultType> promise;
        auto future = promise.get_future();
        receivingMailbox->push(makeAskMessage(MailboxDuplicationStrategy::none, environment, std::move(promise), object, fn, std::forward<Args>(args)...));
        return future;
    }
    
    inline const std::shared_ptr<Object>& unsafe() const {
        return object;
    }

    template <typename Fn>
    auto syncAccess(Fn lambda) const {
        std::lock_guard<std::recursive_mutex> lock(receivingMailbox->receivingMutex);
        return lambda(object);
    }

    template <class CastObject>
    WeakActor<CastObject> weakActor() {
        auto casted = std::static_pointer_cast<CastObject>(object);
        return WeakActor<CastObject>(receivingMailbox, casted);
    }

    template <class CastObject>
    Actor<CastObject> strongActor() {
        auto casted = std::static_pointer_cast<CastObject>(object);
        return Actor<CastObject>(receivingMailbox, casted);
    }
    
private:
    std::shared_ptr<Object> object;
    std::shared_ptr<Mailbox> receivingMailbox;
};
