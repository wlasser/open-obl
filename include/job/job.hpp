#ifndef OPENOBL_JOB_JOB_HPP
#define OPENOBL_JOB_JOB_HPP

#include <boost/fiber/all.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

namespace oo {

class JobCounter {
 private:
  int mV{};
  boost::fibers::condition_variable mCv{};
  boost::fibers::mutex mMutex{};

 public:
  explicit JobCounter(int v) noexcept : mV(v) {}

  void wait() noexcept {
    std::unique_lock lock{mMutex};
    if (mV <= 0) return;
    mCv.wait(lock, [this]() { return mV <= 0; });
  }

  void decrement() noexcept {
    {
      std::unique_lock lock{mMutex};
      --mV;
    }
    mCv.notify_all();
  }

  int get() const noexcept {
    return mV;
  }
};

struct Job {
  std::function<void()> mFunc{};
  JobCounter *mCounter{};

  template<class F> explicit Job(F &&f, JobCounter *counter)
      : mFunc(std::forward<F>(f)), mCounter(counter) {}
  Job() = default;

  void operator()() {
    mFunc();
    if (mCounter) mCounter->decrement();
  }
};

/// Fiber-based concurrent job manager for launching asynchronous tasks.
/// The `oo::JobManager` does not follow the singleton pattern and instead
/// behaves more like a namespace; all its methods are `static` methods.
///
/// Example usage is as follows.
/// ```cpp
/// // Call once in the entire program to spawn the worker threads.
/// oo::JobManager::start();
///
/// // Create an atomic job counter for notification that the job has finished.
/// // Such counters are automatically decremented when a job finishes.
/// oo::JobCounter jc{1};
/// oo::JobManager::runJob([]() {
///   std::cerr << "Job 1 says goodnight\n";
///   boost::this_fiber::sleep_for(std::chrono::seconds(3));
///   std::cerr << "Job 1 woke up!\n";
/// }, &jc);
///
/// // We don't care when this job completes, so don't use a counter.
/// // This will run while Job 1 is sleeping.
/// oo::JobManager::runJob([]() { std::cerr << "Job 2 says hi\n"; });
///
/// // Wait on job 0
/// oo::JobManager::waitOn(&jc0);
///
/// // Call once at the end of the program to join the worker threads
/// oo::JobManager::stop();
/// ```
///
/// Note that because jobs execute on worker threads, jobs will never run on the
/// render thread and thus cannot use GPU resources.
///
/// This class is not a replacement for `Ogre::WorkQueue` as it does not operate
/// using the same request-response pattern.
class JobManager {
 private:
  static constexpr unsigned long NUM_WORKER_THREADS{2u};
  static constexpr std::size_t BUFFER_CAPACITY{1024u};
  using JobQueue = boost::fibers::buffered_channel<Job>;
  static inline std::vector<std::thread> mWorkers{};

  /// Get a reference to the job queue.
  /// \remark This is to get the linter to stop complaining about `JobQueue`'s
  ///         potentially throwing constructor, even though it can be statically
  ///         checked to not throw because `BUFFER_CAPACITY` is a constant
  ///         expression.
  static JobQueue &getQueue() {
    static JobQueue queue{BUFFER_CAPACITY};
    return queue;
  }

  static void runWorker() {
    // MSVC demands static or explicit capture below.
    static constexpr auto CLOSED{boost::fibers::channel_op_status::closed};

    // Create pool of fibers that can be pulling jobs.
    for (int i = 0; i < 10; ++i) {
      boost::fibers::fiber{[]() {
        Job job;
        while (getQueue().pop(job) != CLOSED) job();
      }}.detach();
    }

    // Process jobs on the main fiber too.
    Job job;
    while (getQueue().pop(job) != CLOSED) job();
  }

 public:
  JobManager() = delete;
  JobManager(const JobManager &) = delete;
  JobManager &operator=(const JobManager &) = delete;
  JobManager(JobManager &&) = delete;
  JobManager &operator=(JobManager &&) = delete;

  /// Start the job system by spawning worker threads.
  static void start() noexcept {
    for (std::size_t i = 0; i < NUM_WORKER_THREADS; ++i) {
      mWorkers.emplace_back(runWorker);
    }
  }

  /// Stop the job system by closing the job queue and joining the worker
  /// threads.
  static void stop() noexcept {
    getQueue().close();
    for (auto &worker : mWorkers) worker.join();
  }

  /// Add a new job to the queue.
  template<class F> static void runJob(F &&f, JobCounter *counter = nullptr) {
    getQueue().push(Job(std::forward<F>(f), counter));
  }

  /// Wait on a job counter.
  static void waitOn(JobCounter *counter) noexcept { counter->wait(); }
};

/// Fiber-based job manager that takes control of the calling thread.
/// `oo::JobManager` is unsuitable for tasks that must be run on the rendering
/// thread, since all the tasks execute on worker threads created by the
/// `oo::JobManager`. This manager acts similarly to `oo::JobManager` but does
/// not spawn any workers, and instead blocks---executing the jobs in its
/// queue---until the queue is empty. Because of this, the `start()` function
/// must be given an initial `Job` which takes on the role of the main thread,
/// presumably launching `Job`s on both the main thread---via
/// `oo::RenderJobManager`---and on worker threads---via `oo::JobManager`.
/// Worker jobs are themselves able to launch jobs using the
/// `oo::RenderJobManager`, which allows asynchronous tasks to use the rendering
/// thread when needed.
class RenderJobManager {
 private:
  static constexpr std::size_t BUFFER_CAPACITY{4096u};
  static constexpr std::size_t POOL_SIZE{1024u};
  using JobQueue = boost::fibers::buffered_channel<Job>;

  /// Get a reference to the job queue.
  /// \see JobManager::getJobQueue()
  static JobQueue &getQueue() {
    static JobQueue queue{BUFFER_CAPACITY};
    return queue;
  }

 public:
  RenderJobManager() = delete;
  RenderJobManager(const RenderJobManager &) = delete;
  RenderJobManager &operator=(const RenderJobManager &) = delete;
  RenderJobManager(RenderJobManager &&) = delete;
  RenderJobManager &operator=(RenderJobManager &&) = delete;

  /// Add a new job to the queue.
  /// This can be called from any thread.
  template<class F> static void runJob(F &&f, JobCounter *counter = nullptr) {
    getQueue().push(Job(std::forward<F>(f), counter));
  }

  /// Start the job system on the calling thread.
  template<class F> static void start(F &&f) noexcept {
    static constexpr auto CLOSED{boost::fibers::channel_op_status::closed};

    // Create pool of fibers that can be pulling jobs.
    for (std::size_t i = 0; i < POOL_SIZE; ++i) {
      boost::fibers::fiber{[]() {
        Job job;
        while (getQueue().pop(job) != CLOSED) job();
      }}.detach();
    }

    // Run the initial job on the main fiber, closing the buffer when it's done
    JobCounter jc{1};
    Job job(std::forward<F>(f), &jc);
    job();
    waitOn(&jc);

    getQueue().close();
  }

  /// Wait on a job counter.
  static void waitOn(JobCounter *counter) noexcept { counter->wait(); }
};

} // namespace oo

#endif // OPENOBL_JOB_JOB_HPP
