//
//  ThreadPoolSchedulerImpl.hpp
//  
//
//  Created by Stefan Mitterrutzner on 07.02.23.
//

#pragma once

#include "ThreadPoolScheduler.h"
#include "SchedulerInterface.h"
#include "ThreadPoolCallbacks.h"
#include "TaskInterface.h"
#include "TaskConfig.h"
#include <condition_variable>
#include <mutex>
#include <deque>
#include <thread>
#include <array>

class ThreadPoolSchedulerImpl: public SchedulerInterface {
public:
    ThreadPoolSchedulerImpl(const std::shared_ptr<ThreadPoolCallbacks> & callbacks);
    
    ~ThreadPoolSchedulerImpl();
    
    virtual void addTask(const std::shared_ptr<TaskInterface> & task) override;

    virtual void addTasks(const std::vector<std::shared_ptr<TaskInterface>> & tasks) override;

    virtual void removeTask(const std::string & id) override;

    virtual void clear() override;

    virtual void pause() override;

    virtual void resume() override;
    
private:
    std::thread makeSchedulerThread(size_t index, TaskPriority priority);
    
    const std::shared_ptr<ThreadPoolCallbacks> callbacks;

    std::deque<std::shared_ptr<TaskInterface>> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool terminated{false};
    
    std::array<std::thread, 5> threads;
};
