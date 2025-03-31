#include "SchedulerInterface.h"
#include <deque>

class TestScheduler : public SchedulerInterface {
  public:
    virtual void addTask(const /*not-null*/ std::shared_ptr<TaskInterface> &task) override {
        std::lock_guard<std::mutex> guard(tasksMutex);
        tasks.push_back(task);
    }
    virtual void addTasks(const std::vector</*not-null*/ std::shared_ptr<TaskInterface>> &tasks) override {
        std::lock_guard<std::mutex> guard(tasksMutex);
        for (auto task : tasks) {
            this->tasks.push_back(task);
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
    bool process_one() {
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

    void drain() {
        while (process_one()) {
        }
    }

  private:
    std::mutex tasksMutex;
    std::deque<std::shared_ptr<TaskInterface>> tasks;
};