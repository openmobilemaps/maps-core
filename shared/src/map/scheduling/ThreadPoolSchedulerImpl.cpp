//
//  ThreadPoolSchedulerImpl.cpp
//  
//
//  Created by Stefan Mitterrutzner on 07.02.23.
//

#include "ThreadPoolSchedulerImpl.h"
#include "Logger.h"
#include <chrono>
#include <cassert>
#include <cmath>

std::shared_ptr<SchedulerInterface> ThreadPoolScheduler::create(const std::shared_ptr<ThreadPoolCallbacks> &callbacks) {
    return std::make_shared<ThreadPoolSchedulerImpl>(callbacks, false);
}

ThreadPoolSchedulerImpl::ThreadPoolSchedulerImpl(const std::shared_ptr<ThreadPoolCallbacks> &callbacks,
                                                 bool separateGraphicsQueue)
        : callbacks(callbacks), separateGraphicsQueue(separateGraphicsQueue), delayedTaskThread(&ThreadPoolSchedulerImpl::delayedTasksThread, this), nextWakeup(std::chrono::system_clock::now() + std::chrono::seconds(1)) {
    unsigned int maxNumThreads = std::floorf(std::thread::hardware_concurrency() * 0.75f);
    if (maxNumThreads < 1) maxNumThreads = DEFAULT_MAX_NUM_THREADS;
    for (std::size_t i = 0u; i < maxNumThreads; ++i) {
        threads.emplace_back(makeSchedulerThread(i, TaskPriority::NORMAL));
    }
}

ThreadPoolSchedulerImpl::~ThreadPoolSchedulerImpl() {
    {
        std::lock_guard<std::mutex> lock(defaultMutex);
        terminated = true;
    }
    defaultCv.notify_all();
    delayedTasksCv.notify_all();
    delayedTaskThread.join();
    
    for (auto& thread : threads) {
        if (std::this_thread::get_id() != thread.get_id()) {
            thread.join();
        }
    }
}

void ThreadPoolSchedulerImpl::addTask(const std::shared_ptr<TaskInterface> & task) {
    assert(task);
    auto const &config = task->getConfig();

    if (config.delay != 0) {
        std::lock_guard<std::mutex> lock(delayedTasksMutex);
        delayedTasks.push_back({task,  std::chrono::system_clock::now() + std::chrono::milliseconds(config.delay)});
        delayedTasksCv.notify_one();
    } else {
        addTaskIgnoringDelay(task);
    }
}

void ThreadPoolSchedulerImpl::addTaskIgnoringDelay(const std::shared_ptr<TaskInterface> & task) {
    auto const &config = task->getConfig();

    if (separateGraphicsQueue && config.executionEnvironment == ExecutionEnvironment::GRAPHICS) {
        std::lock_guard<std::mutex> lock(graphicsMutex);
        graphicsQueue.push_back(task);
    } else {
        std::lock_guard<std::mutex> lock(defaultMutex);
        defaultQueue.push_back(task);
        defaultCv.notify_one();
    }
}

void ThreadPoolSchedulerImpl::addTasks(const std::vector<std::shared_ptr<TaskInterface>> & tasks) {
    for (auto const &task : tasks) {
        addTask(task);
    }
}

void ThreadPoolSchedulerImpl::removeTask(const std::string & id) {
    {
        std::lock_guard<std::mutex> lock(defaultMutex);
        auto it = std::find_if(defaultQueue.begin(), defaultQueue.end(),
                               [&](const std::shared_ptr<TaskInterface> &task) { return task->getConfig().id == id; });
        if (it != defaultQueue.end()) {
            defaultQueue.erase(it);
            return;
        }
    }
    if (separateGraphicsQueue) {
        std::lock_guard<std::mutex> lock(graphicsMutex);
        auto it = std::find_if(graphicsQueue.begin(), graphicsQueue.end(),
                               [&](const std::shared_ptr<TaskInterface> &task) { return task->getConfig().id == id; });
        if (it != graphicsQueue.end()) {
            graphicsQueue.erase(it);
            return;
        }
    }
}

void ThreadPoolSchedulerImpl::clear() {
    {
        std::lock_guard<std::mutex> lock(defaultMutex);
        defaultQueue.clear();
    }
    {
        std::lock_guard<std::mutex> lock(graphicsMutex);
        graphicsQueue.clear();
    }
}

void ThreadPoolSchedulerImpl::pause() {
    
}

void ThreadPoolSchedulerImpl::resume() {
    
}

std::thread ThreadPoolSchedulerImpl::makeSchedulerThread(size_t index, TaskPriority priority) {
    return std::thread([this, index, priority] {
        callbacks->setCurrentThreadName(std::string{"MapSDK_"} + std::to_string(index) + "_" + std::string(toString(priority)));
        callbacks->attachThread();
        
        while (true) {
            std::unique_lock<std::mutex> lock(defaultMutex);
            
            defaultCv.wait(lock, [this] { return !defaultQueue.empty() || terminated; });
            
            if (terminated) {
                callbacks->detachThread();
                return;
            }
            
            auto task = std::move(defaultQueue.front());
            defaultQueue.pop_front();
            lock.unlock();
            if (task) task->run();
        }
    });
}

bool ThreadPoolSchedulerImpl::hasSeparateGraphicsInvocation() {
    return separateGraphicsQueue;
}

std::chrono::time_point<std::chrono::steady_clock> lastEnd;
void ThreadPoolSchedulerImpl::runGraphicsTasks() {
    auto start = std::chrono::steady_clock::now();
    int i;
    for (i = 0; i < MAX_NUM_GRAPHICS_TASKS; i++) {
        {
            std::unique_lock<std::mutex> lock(graphicsMutex);
            if (graphicsQueue.empty()) {
                break;
            } else {
                auto task = std::move(graphicsQueue.front());
                graphicsQueue.pop_front();
                lock.unlock();
                if (task) task->run();
            }
        }
        auto cwtMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
        if (cwtMs.count() >= MAX_TIME_GRAPHICS_TASKS_MS) {
            break;
        }
    }
}

void ThreadPoolSchedulerImpl::delayedTasksThread() {
    callbacks->setCurrentThreadName(std::string{"MapSDK_delayed_tasks"});

    while (true) {
        std::unique_lock<std::mutex> lock(delayedTasksMutex);
        delayedTasksCv.wait_until(lock, nextWakeup);

        if (terminated) {
            return;
        }

        auto now = std::chrono::system_clock::now();

        nextWakeup = TimeStamp::max();

        for (auto it = delayedTasks.begin(); it != delayedTasks.end();) {
            // lets schedule this task
            if(it->second <= now) {
                addTaskIgnoringDelay(it->first);
                it = delayedTasks.erase(it);
            } else {
                nextWakeup = std::min(nextWakeup, it->second);
                it++;
            }
        }
    }
}
