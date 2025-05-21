#include "SchedulerInterface.h"
#include "TaskConfig.h"
#include "TaskInterface.h"
#include <deque>
#include <queue>

class TestScheduler : public SchedulerInterface {
  public:
    virtual void addTask(const /*not-null*/ std::shared_ptr<TaskInterface> &task) override {
        std::lock_guard<std::mutex> guard(tasksMutex);
        auto taskConfig = task->getConfig();
        if (taskConfig.delay <= 0) {
            tasks.push_back(task);
        } else {
            delayedTasks.emplace(now + taskConfig.delay, task);
        }
    }
    virtual void addTasks(const std::vector</*not-null*/ std::shared_ptr<TaskInterface>> &tasks) override {
        for (auto task : tasks) {
            addTask(task);
        }
    }
    virtual void removeTask(const std::string &id) override {}
    virtual void clear() override {}
    virtual void pause() override {}
    virtual void resume() override {}
    virtual void destroy() override {}
    virtual bool hasSeparateGraphicsInvocation() override { return false; }
    virtual bool runGraphicsTasks() override { return false; }
    virtual void setSchedulerGraphicsTaskCallbacks(const std::shared_ptr<SchedulerGraphicsTaskCallbacks> &callbacks) override {}

  public:
    // Process tasks, including delayed tasks, until there is nothing left to do
    void drain() {
        while (processOneFromQueue(tasks) || processOneDelayedTaskBefore(INT64_MAX)) {
        }
    }

    // Process tasks including delayed tasks scheduled to execute before `until`, until there is nothing left to do.
    void drainUntil(int64_t until) {
        {
            std::lock_guard<std::mutex> guard(tasksMutex);
            now = until;
        }
        while (processOneFromQueue(tasks) || processOneDelayedTaskBefore(until)) {
        }
    }

    // Process tasks until the scheduled time of the next delayed task
    void stepUntilNextDelayed() {
        int64_t until;
        {
            std::lock_guard<std::mutex> guard(tasksMutex);
            if (delayedTasks.empty()) {
                return;
            }
            until = delayedTasks.top().when;
        }
        drainUntil(until);
    }

  private:
    bool processOneDelayedTaskBefore(int64_t until) {
        std::shared_ptr<TaskInterface> task;
        {
            std::lock_guard<std::mutex> guard(tasksMutex);
            if (delayedTasks.empty()) {
                return false;
            }
            auto &top = delayedTasks.top();
            if (top.when > until) {
                return false;
            }
            task = top.task;
            delayedTasks.pop();
        }
        task->run();
        return true;
    }

    bool processOneFromQueue(std::deque<std::shared_ptr<TaskInterface>> &taskQueue) {
        std::shared_ptr<TaskInterface> task;
        {
            std::lock_guard<std::mutex> guard(tasksMutex);
            if (tasks.empty()) {
                return false;
            }
            task = tasks.front();
            tasks.pop_front();
        }
        task->run();
        return true;
    }

  private:
    struct DelayedTask {
        int64_t when;
        std::shared_ptr<TaskInterface> task;

        bool operator<(const DelayedTask &o) const { return when < o.when; }
    };

  private:
    std::mutex tasksMutex;
    std::deque<std::shared_ptr<TaskInterface>> tasks;
    std::priority_queue<DelayedTask> delayedTasks;

    int64_t now = 0; // simulated time in seconds for delayed tasks, starting from 0.
};
