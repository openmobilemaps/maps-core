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
#include "SchedulerGraphicsTaskCallbacks.h"
#include <condition_variable>
#include <mutex>
#include <deque>
#include <chrono>
#include <thread>
#include <array>

class ThreadPoolSchedulerImpl: public SchedulerInterface {
public:
    ThreadPoolSchedulerImpl(const std::shared_ptr<ThreadPoolCallbacks> &callbacks);

    virtual void addTask(const std::shared_ptr<TaskInterface> & task) override;

    virtual void addTasks(const std::vector<std::shared_ptr<TaskInterface>> & tasks) override;

    virtual void removeTask(const std::string & id) override;

    virtual void setSchedulerGraphicsTaskCallbacks(const /*not-null*/ std::shared_ptr<SchedulerGraphicsTaskCallbacks> & callbacks) override;

    virtual void clear() override;

    virtual void pause() override;

    virtual void resume() override;

    void destroy() override;

    bool hasSeparateGraphicsInvocation() override;

    bool runGraphicsTasks() override;

    void addTaskIgnoringDelay(const std::shared_ptr<TaskInterface> & task);

private:
    std::thread makeSchedulerThread(size_t index, TaskPriority priority);
    std::thread makeDelayedTasksThread();
    
    std::shared_ptr<ThreadPoolCallbacks> callbacks;

    std::mutex defaultMutex;
    std::deque<std::shared_ptr<TaskInterface>> defaultQueue;
    std::condition_variable defaultCv;

    bool separateGraphicsQueue;
    static const uint8_t MAX_NUM_GRAPHICS_TASKS = 8;
    static const uint64_t MAX_TIME_GRAPHICS_TASKS_MS = 4;
    std::mutex graphicsMutex;

    std::deque<std::shared_ptr<TaskInterface>> graphicsQueue;
    static const uint8_t DEFAULT_MIN_NUM_THREADS = 8;
    std::vector<std::thread> threads;

    using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;
    std::mutex delayedTasksMutex;
    TimeStamp nextWakeup;
    std::condition_variable delayedTasksCv;
    //contains the tasks and the timestamp at which they were added to the list
    std::vector<std::pair<std::shared_ptr<TaskInterface>, TimeStamp>> delayedTasks;

    std::atomic_bool terminated{false};

    std::weak_ptr<SchedulerGraphicsTaskCallbacks> graphicsCallbacks;
};
