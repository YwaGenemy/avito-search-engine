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

class Task;
class ThreadPool;

class TaskInfo {
 public:
  void Wait() {
    std::unique_lock lock(mutex_);

    complete_.wait(lock, [&] { return status_ == TaskStatus::kCompleted; });
  }

  std::optional<std::any> GetResult() { return result_; }

 private:
  friend Task;

  TaskStatus status_{TaskStatus::kAwaiting};
  std::optional<std::any> result_;
  std::mutex mutex_;
  std::condition_variable complete_;
};

class Task {
 public:
  Task() = default;

  template <typename Ret, typename... FuncTypes, typename... Args>
    requires std::is_invocable_r_v<Ret, Ret (*)(FuncTypes...), Args...>
  Task(Ret (*func)(FuncTypes...), Args&&... args)
      : is_void_(std::is_void_v<Ret>), info_(std::make_shared<TaskInfo>()) {
    if constexpr (std::is_void_v<Ret>) {
      void_func = std::bind(func, args...);
      any_func = []() -> int { return 0; };
    } else {
      any_func = std::bind(func, args...);
      void_func = []() -> void {};
    }
  }

  template <typename Object, typename Ret, typename... FuncTypes,
            typename... Args>
    requires std::is_invocable_r_v<Ret, Ret (Object::*)(FuncTypes...), Object&,
                                   Args...>
  Task(Object& obj, Ret (Object::*method)(FuncTypes...), Args&&... args)
      : is_void_(std::is_void_v<Ret>), info_(std::make_shared<TaskInfo>()) {
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
    if (is_void_) {
      void_func();
      {
        std::unique_lock lock(info_->mutex_);
        info_->result_ = std::nullopt;
        info_->status_ = TaskStatus::kCompleted;
      }
    } else {
      {
        std::unique_lock lock(info_->mutex_);
        info_->result_ = any_func();
        info_->status_ = TaskStatus::kCompleted;
      }
    }
    info_->complete_.notify_all();
  }

  bool operator!() { return !any_func && !void_func; }

 private:
  friend ThreadPool;

  std::function<std::any()> any_func;
  std::function<void()> void_func;
  bool is_void_;
  std::shared_ptr<TaskInfo> info_;
};

class NotificationQueue {
  using lock_t = std::unique_lock<std::mutex>;

 public:
  void Done() {
    {
      lock_t lock(mutex_);
      done_ = true;
    }
    ready_.notify_all();
  }

  bool Pop(Task& task) {
    lock_t lock(mutex_);

    ready_.wait(lock, [this] { return !task_q_.empty() || done_; });
    if (task_q_.empty()) return false;

    task = std::move(task_q_.front());
    task_q_.pop();
    return true;
  }

  template <typename T>
  void Push(T&& task) {
    {
      lock_t lock(mutex_);
      task_q_.emplace(std::forward<T>(task));
    }
    ready_.notify_one();
  }

  bool TryPop(Task& task) {
    lock_t lock(mutex_, std::try_to_lock);

    if (!lock || task_q_.empty()) return false;

    task = std::move(task_q_.front());
    task_q_.pop();
    return true;
  }

  template <typename T>
  bool TryPush(T&& task) {
    {
      lock_t lock(mutex_, std::try_to_lock);
      if (!lock) return false;
      task_q_.emplace(std::forward<T>(task));
    }
    ready_.notify_one();
    return true;
  }

 private:
  std::queue<Task> task_q_;
  bool done_{false};
  std::mutex mutex_;
  std::condition_variable ready_;
};

class ThreadPool {
 public:
  ThreadPool() {
    threads_.reserve(count_);
    for (unsigned i = 0; i < count_; ++i) {
      threads_.emplace_back([this, i] { Run(i); });
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool operator=(const ThreadPool&) = delete;

  ~ThreadPool() {
    for (auto& q : queue_) q.Done();
    for (auto& t : threads_) t.join();
  }

  template <typename Ret, typename... FuncTypes, typename... Args>
  std::shared_ptr<TaskInfo> AddTask(Ret (*func)(FuncTypes...), Args&&... args) {
    auto i = index_++;
    auto task = Task(func, std::forward<Args>(args)...);

    for (unsigned n = 0; n != count_ * 10; ++n) {
      if (queue_[(i + n) % count_].TryPush(task)) return task.info_;
    }
    queue_[i % count_].Push(std::move(task));

    return task.info_;
  }

  template <typename Object, typename Ret, typename... FuncTypes,
            typename... Args>
  std::shared_ptr<TaskInfo> AddTask(Object& obj,
                                    Ret (Object::*method)(FuncTypes...),
                                    Args&&... args) {
    auto i = index_++;
    auto task = Task(obj, method, std::forward<Args>(args)...);

    for (unsigned n = 0; n != count_ * 10; ++n) {
      if (queue_[(i + n) % count_].TryPush(task)) return task.info_;
    }
    queue_[i % count_].Push(std::move(task));

    return task.info_;
  }

 private:
  void Run(unsigned i) {
    while (true) {
      Task task;
      for (unsigned n = 0; n != count_; ++n) {
        if (queue_[(i + n) % count_].TryPop(task)) break;
      }
      if (!task && !queue_[i].Pop(task)) break;

      task();
    }
  }

  const unsigned count_{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads_;
  std::vector<NotificationQueue> queue_{count_};
  std::atomic<unsigned> index_{0};
};
