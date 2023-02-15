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
inline std::unique_ptr<MailboxMessage> makeMessage(const MailboxDuplicationStrategy &strategy, Object& object, MemberFn memberFn, Args&&... args) {
    auto tuple = std::make_tuple(std::forward<Args>(args)...);
    return std::make_unique<MailboxMessageImpl<Object, MemberFn, decltype(tuple)>>(object, memberFn, strategy, std::move(tuple));
}

template <class ResultType, class Object, class MemberFn, class... Args>
std::unique_ptr<MailboxMessage> makeMessage(const MailboxDuplicationStrategy &strategy, std::promise<ResultType>&& promise, Object& object, MemberFn memberFn, Args&&... args) {
    auto tuple = std::make_tuple(std::forward<Args>(args)...);
    return std::make_unique<AskMessageImpl<ResultType, Object, MemberFn, decltype(tuple)>>(std::move(promise), object, memberFn, strategy, std::move(tuple));
}

template <class Object>
class WeakActor {
public:
    WeakActor(std::weak_ptr<Mailbox> receivingMailbox, std::weak_ptr<Object> object): receivingMailbox(receivingMailbox), object(object) {}
    
    WeakActor(): object(), receivingMailbox() {}
    
    constexpr explicit operator bool() noexcept {
        return receivingMailbox && object.lock();
    }
    
    template <typename Fn, class... Args>
    inline void message(const MailboxDuplicationStrategy &strategy, Fn fn, Args&&... args) const {
        auto strongObject = object.lock();
        if (strongObject) {
            receivingMailbox->push(makeMessage(strategy, strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
    }
    
    template <typename Fn, class... Args>
    inline void message(Fn fn, Args&&... args) const {
        auto strongObject = object.lock();
        if (strongObject) {
            receivingMailbox->push(makeMessage(MailboxDuplicationStrategy::none, strongObject, fn, std::forward<Args>(args)...));
        } else {
            LogError <<= "WeakActor holds nullptr";
        }
    }
    
    
    template <typename Fn, class... Args>
    auto converse(Fn fn, Args&&... args) const {
        using ResultType = std::invoke_result_t<decltype(fn), Object, Args...>;
        
        std::promise<ResultType> promise;
        auto future = promise.get_future();
        receivingMailbox->push(makeMessage(MailboxDuplicationStrategy::none, std::move(promise), *object, fn, std::forward<Args>(args)...));
        return future;
    }
    
    template <class CastObject>
    WeakActor<CastObject> weakActor() const {
        auto casted = std::static_pointer_cast<CastObject>(object.lock());
        return WeakActor<CastObject>(receivingMailbox, casted);
    }
    
    inline std::shared_ptr<Object>& unsafe() {
        return object.lock();
    }
    
private:
    std::weak_ptr<Object> object;
    std::shared_ptr<Mailbox> receivingMailbox;
};

template <class Object>
class Actor {
public:
    template <class... Args>
    Actor(std::shared_ptr<Mailbox> mailbox, Args&&... args): receivingMailbox(mailbox), object(std::make_shared<Object>(std::forward<Args>(args)...)) {
        if constexpr (std::is_same_v<Object, ActorObject>) {
            object.mailbox = mailbox;
        }
    }

    Actor(std::shared_ptr<Mailbox> mailbox, std::shared_ptr<Object> object): receivingMailbox(mailbox), object(object) {
        if constexpr (std::is_same_v<Object, ActorObject>) {
            assert(object.mailbox == mailbox);
        }
    }
    
    Actor(): object(nullptr), receivingMailbox(nullptr) {}
    
    constexpr explicit operator bool() noexcept {
        return object && receivingMailbox;
    }
    
    template <class... Args>
    void emplaceObject(std::shared_ptr<Mailbox> mailbox, Args&&... args) {
        this->receivingMailbox = mailbox;
        object = std::make_shared<Object>(std::forward<Args>(args)...);
        
        
        if (auto actorObject = std::dynamic_pointer_cast<ActorObject>(object)) {
            actorObject->mailbox = mailbox;
        }
    }
    
    void emplaceObject(std::shared_ptr<Mailbox> mailbox, std::shared_ptr<Object> object) {
        this->receivingMailbox = mailbox;
        this->object = object;
        
        if constexpr (std::is_same_v<Object, ActorObject>) {
            assert(object.mailbox == mailbox);
        }
    }
    
    template <typename Fn, class... Args>
    inline void message(Fn fn, Args&&... args) const {
        if(!receivingMailbox || !object) {
            return;
        }
        receivingMailbox->push(makeMessage(MailboxDuplicationStrategy::none, object, fn, std::forward<Args>(args)...));
    }
    
    
    template <typename Fn, class... Args>
    inline void message(Fn fn, const MailboxDuplicationStrategy &strategy, Args&&... args) const {
        if(!receivingMailbox || !object) {
            return;
        }
        receivingMailbox->push(makeMessage(strategy, object, fn, std::forward<Args>(args)...));
    }
    
    template <typename Fn, class... Args>
    auto converse(Fn fn, Args&&... args) const {
        using ResultType = std::invoke_result_t<decltype(fn), Object, Args...>;
        
        std::promise<ResultType> promise;
        auto future = promise.get_future();
        receivingMailbox->push(makeMessage(MailboxDuplicationStrategy::none, std::move(promise), object, fn, std::forward<Args>(args)...));
        return future;
    }
    
    inline std::shared_ptr<Object>& unsafe() {
        return object;
    }
    
    template <class CastObject>
    WeakActor<CastObject> weakActor() {
        auto casted = std::static_pointer_cast<CastObject>(object);
        return WeakActor<CastObject>(receivingMailbox, casted);
    }
    
private:
    std::shared_ptr<Object> object;
    std::shared_ptr<Mailbox> receivingMailbox;
};
