//
//  ThreadPoolSchedulerImpl.cpp
//  
//
//  Created by Stefan Mitterrutzner on 07.02.23.
//

#include "ThreadPoolSchedulerImpl.h"

std::shared_ptr<SchedulerInterface> ThreadPoolScheduler::create(const std::shared_ptr<ThreadPoolCallbacks> & callbacks) {
    return std::make_shared<ThreadPoolSchedulerImpl>(callbacks);
}

ThreadPoolSchedulerImpl::ThreadPoolSchedulerImpl(const std::shared_ptr<ThreadPoolCallbacks> & callbacks): callbacks(callbacks) {
    for (std::size_t i = 0u; i < threads.max_size(); ++i) {
        threads[i] = makeSchedulerThread(i, TaskPriority::NORMAL);
    }
}

ThreadPoolSchedulerImpl::~ThreadPoolSchedulerImpl() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        terminated = true;
    }
    cv.notify_all();
    
    for (auto& thread : threads) {
        if (std::this_thread::get_id() != thread.get_id()) {
            thread.join();
        }
    }
}

void ThreadPoolSchedulerImpl::addTask(const std::shared_ptr<TaskInterface> & task) {
    assert(task);
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(std::move(task));
    }
    
    cv.notify_one();
}

void ThreadPoolSchedulerImpl::addTasks(const std::vector<std::shared_ptr<TaskInterface>> & tasks) {
    for (auto const &task : tasks) {
        addTask(task);
    }
}

void ThreadPoolSchedulerImpl::removeTask(const std::string & id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = std::find_if(queue.begin(), queue.end(), [&] (const std::shared_ptr<TaskInterface> &task) { return task->getConfig().id == id; });
    if( it != queue.end() ) {
        queue.erase(it);
    }
}

void ThreadPoolSchedulerImpl::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    queue.clear();
}

void ThreadPoolSchedulerImpl::pause() {
    
}

void ThreadPoolSchedulerImpl::resume() {
    
}

std::thread ThreadPoolSchedulerImpl::makeSchedulerThread(size_t index, TaskPriority priority) {
    return std::thread([this, index, priority] {
        callbacks->setCurrentThreadName(std::string{"Worker_"} + std::to_string(index) + "_" + std::to_string((int)priority));
        callbacks->attachThread();
        
        while (true) {
            std::unique_lock<std::mutex> lock(mutex);
            
            cv.wait(lock, [this] { return !queue.empty() || terminated; });
            
            if (terminated) {
                callbacks->detachThread();
                return;
            }
            
            auto task = std::move(queue.front());
            queue.pop_front();
            lock.unlock();
            if (task) task->run();
        }
    });
}
