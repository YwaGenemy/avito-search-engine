#include <any>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

enum class TaskStatus { kAwaiting, kCompleted };

struct TaskInfo {
  TaskStatus status = TaskStatus::kAwaiting;
  std::any result;
};

struct Thread {
  std::thread _thread;
  std::atomic<bool> is_working;
};

class Task {
 public:
  template <typename Ret, typename... FuncTypes, typename... Args>
    requires std::is_invocable_r_v<Ret, Ret (*)(FuncTypes...), Args...>
  Task(Ret (*func)(FuncTypes...), Args&&... args)
      : is_void_(std::is_void_v<Ret>) {
    if constexpr (std::is_void_v<Ret>) {
      void_func = std::bind(func, args...);
      any_func = []() -> int { return 0; };
    } else {
      any_func = std::bind(func, args...);
      ;
      void_func = []() -> void {};
    }
  }

  template <typename Object, typename Ret, typename... FuncTypes,
            typename... Args>
    requires std::is_invocable_r_v<Ret, Ret (Object::*)(FuncTypes...), Object&,
                                   Args...>
  Task(Object& obj, Ret (Object::*method)(FuncTypes...), Args&&... args)
      : is_void_(std::is_void_v<Ret>) {
    if constexpr (std::is_void_v<Ret>) {
      void_func = [&obj, method, args...]() -> void {
        return obj.*method(args...);
      };
      any_func = []() -> int { return 0; };
    } else {
      any_func = [&obj, method, args...]() { return obj.*method(args...); };
      void_func = []() -> void {};
    }
  }

  void operator()() {
    void_func();
    any_result = any_func();
  }

  std::optional<std::any> GetResult() const {
    if (is_void_ || !any_result.has_value()) {
      return std::nullopt;
    }

    return any_result;
  }

 private:
  std::function<std::any()> any_func;
  std::function<void()> void_func;
  std::any any_result;
  bool is_void_;
};

class ThreadPool {
 public:
  ThreadPool(size_t count_threads) {
    threads_.reserve(count_threads);
    for (size_t i = 0; i < count_threads; ++i) {
      threads_.emplace_back(&ThreadPool::Run, this);
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool operator=(const ThreadPool&) = delete;

  ~ThreadPool() {
    quite_ = true;
    queue_cv_.notify_all();
    for (size_t i = 0; i < threads_.size(); ++i) {
      threads_[i].join();
    }
  }

  template <typename Ret, typename... FuncTypes, typename... Args>
  uint64_t AddTask(Ret (*func)(FuncTypes...), Args&&... args) {
    const uint64_t id = last_id_++;

    std::unique_lock<std::mutex> lock(tasks_info_mutex_);
    tasks_info_[id] = TaskInfo{};
    lock.unlock();

    std::lock_guard<std::mutex> queue_lock(queue_mtx_);
    task_queue_.emplace(Task(func, std::forward<Args>(args)...), id);
    queue_cv_.notify_one();
    return id;
  }

  template <typename Object, typename Ret, typename... FuncTypes,
            typename... Args>
  uint64_t AddTask(Object& obj, Ret (Object::*method)(FuncTypes...),
                   Args&&... args) {
    const uint64_t id = last_id_++;

    std::unique_lock<std::mutex> lock(tasks_info_mutex_);
    tasks_info_[id] = TaskInfo{};
    lock.unlock();

    std::lock_guard<std::mutex> queue_lock(queue_mtx_);
    task_queue_.emplace(Task(obj, method, std::forward<Args>(args)...), id);
    queue_cv_.notify_one();
    return id;
  }

  void Wait(uint64_t task_id) {
    std::unique_lock<std::mutex> lock(tasks_info_mutex_);
    tasks_info_cv_.wait(lock, [this, task_id]() -> bool {
      return task_id < last_id_ &&
             tasks_info_[task_id].status == TaskStatus::kCompleted;
    });
  }

  std::any WaitResult(uint64_t task_id) {
    std::unique_lock<std::mutex> lock(tasks_info_mutex_);
    tasks_info_cv_.wait(lock, [this, task_id]() -> bool {
      return task_id < last_id_ &&
             tasks_info_[task_id].status == TaskStatus::kCompleted;
    });
    return tasks_info_[task_id].result;
  }

  void WaitAll() {
    std::unique_lock<std::mutex> lock(tasks_info_mutex_);
    wait_all_cv.wait(
        lock, [this]() -> bool { return count_completed_tasks_ == last_id_; });
  }

 private:
  void Run() {
    while (!quite_) {
      std::unique_lock<std::mutex> lock(queue_mtx_);
      queue_cv_.wait(lock, [this]() { return !task_queue_.empty() || quite_; });

      if (!task_queue_.empty() && !quite_) {
        auto task = std::move(task_queue_.front());
        task_queue_.pop();
        lock.unlock();

        task.first();

        std::lock_guard<std::mutex> lock(tasks_info_mutex_);
        if (task.first.GetResult().has_value()) {
          tasks_info_[task.second].result = task.first.GetResult();
        }
        tasks_info_[task.second].status = TaskStatus::kCompleted;
        ++count_completed_tasks_;
      }
      wait_all_cv.notify_all();
      tasks_info_cv_.notify_all();
    }
  }

  std::vector<std::thread> threads_;

  std::queue<std::pair<Task, uint64_t>> task_queue_;
  std::mutex queue_mtx_;
  std::condition_variable queue_cv_;

  std::unordered_map<uint64_t, TaskInfo> tasks_info_;
  std::condition_variable tasks_info_cv_;
  std::mutex tasks_info_mutex_;

  std::condition_variable wait_all_cv;

  std::atomic<bool> quite_ = false;
  std::atomic<uint64_t> last_id_ = 0;
  std::atomic<size_t> count_completed_tasks_ = 0;
};
