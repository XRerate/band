/*
 * Copyright 2023 Seoul National University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BAND_PLANNER_H_
#define BAND_PLANNER_H_

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "band/config.h"
#include "band/safe_bool.h"
#include "band/scheduler/scheduler.h"
#include "band/worker.h"

namespace band {

// The maximum number of available job outputs at one time.
#define NUM_FINISHED_RECORDS 1000

// The job queue which can be shared by multiple threads.
struct ConcurrentJobQueue {
  JobQueue queue;
  std::mutex mtx;
};

class Planner {
 public:
  explicit Planner(IEngine& engine);
  ~Planner();

  absl::Status Init(const PlannerConfig& config);
  absl::Status AddScheduler(std::unique_ptr<IScheduler> scheduler);

  // Enqueues a job to a worker request queue.
  JobId EnqueueRequest(Job job, bool push_front = false);
  // Enqueues a batch of jobs to a worker request queue.
  // Assigns new job id for non-continuous job.
  std::vector<JobId> EnqueueBatch(std::vector<Job> jobs,
                                  bool push_front = false);
  // Waits until the jobs are done.
  // The interpreter calls the method.
  void Wait(std::vector<int> job_ids);
  void WaitAll();
  // Enqueues a finised job to the queue.
  // A worker calls the method.
  void EnqueueFinishedJob(Job& job);
  void PrepareReenqueue(Job& job);
  // Enqueue the request to the worker.
  // Returns true if the request is successfully enqueued.
  bool EnqueueToWorker(const std::vector<ScheduleAction>& action);
  void Trigger() { planner_safe_bool_.notify(); }

  // Checks if the schedulers can handle fallback subgraphs.
  // Returns true if any of the scheduler can handle fallback subgraphs.
  // But, note that having both types of scheduler (w/ fallback, w/o fallback),
  // may lead to unexpected results.
  bool NeedFallbackSubgraphs() const;

  std::mutex& GetRequestsMtx() { return requests_.mtx; }
  JobQueue& GetRequests() { return requests_.queue; }
  int GetWindowSize() const { return schedule_window_size_; }
  void SetWindowSize(int schedule_window_size);
  const std::map<int, int>& GetModelExecutionCounts() const {
    return model_execution_count_;
  }
  // Sets the callback function pointer to report the end of invoke.
  CallbackId SetOnEndRequest(
      std::function<void(int, absl::Status)> on_end_request);
  absl::Status UnsetOnEndRequest(CallbackId callback_id);

  // Get the Job instance with the `job_id`.
  Job GetFinishedJob(int job_id);
  // Get which worker types the schedulers require.
  int GetWorkerType() const;
  std::map<ModelId, WorkerId>& GetModelWorkerMap() { return model_worker_map_; }

 private:
  // Main loop for planner_thread_
  absl::Status Plan();
  // Write job logs and delete the job from the finished queue.
  void FlushFinishedJobs();
  // Copy the Job instances from the `requests_` to the local queue.
  // Note that this function is to minimize the hold time for the queue lock.
  void CopyToLocalQueues();
  // Check if the job violated the specified SLO.
  // This func assumes that workers_waiting_, job.profiled_time,
  // job.device_id, and job.enqueue_time are all up to date.
  bool IsSLOViolated(Job& job);
  // Update the job information based on next target key
  void UpdateJobScheduleStatus(Job& job, const SubgraphKey& target_key);
  // Update `model_worker_map_`.
  void TryUpdateModelWorkerMapping();
  bool IsJobIdValid(int job_id);
  int GetJobRecordIndex(int job_id) const;

  CpuSet cpu_set_;
  bool need_cpu_update_ = false;

  SafeBool planner_safe_bool_;

  // Jobs Finished
  std::map<int, int> model_execution_count_;

  mutable std::mutex on_end_request_mtx_;
  std::map<CallbackId, std::function<void(int, absl::Status)>>
      on_end_request_callbacks_;
  CallbackId next_callback_id_ = 0;

  // Request Queue
  ConcurrentJobQueue requests_;

  // Multi-level Local Queue.
  // The closer the index is to 0, the higher the priority.
  std::vector<JobQueue> local_queues_;
  std::vector<std::unique_ptr<IScheduler>> schedulers_;

  std::mutex job_finished_mtx_;
  std::array<Job, NUM_FINISHED_RECORDS> jobs_finished_record_;
  std::atomic<int> num_submitted_jobs_;
  int num_finished_jobs_ = 0;

  std::condition_variable end_invoke_;
  std::string log_path_;

  int schedule_window_size_ = std::numeric_limits<int>::max();

  std::thread planner_thread_;
  // Map structure to find assigned worker of model idx (model_id, worker_id)
  std::map<ModelId, WorkerId> model_worker_map_;
  IEngine& engine_;
};

}  // namespace band

#endif  // BAND_PLANNER_H_
