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

#ifndef BAND_SCHEDULER_SHORTEST_EXPECTED_LATENCY_SCHEDULER_H_
#define BAND_SCHEDULER_SHORTEST_EXPECTED_LATENCY_SCHEDULER_H_

#include "band/scheduler/scheduler.h"

namespace band {

class ShortestExpectedLatencyScheduler : public IScheduler {
 public:
  explicit ShortestExpectedLatencyScheduler(IEngine& engine, int window_size);

  bool Schedule(JobQueue& requests) override;
  bool NeedFallbackSubgraphs() override { return true; }
  WorkerType GetWorkerType() override { return WorkerType::kGlobalQueue; }

 private:
  const int window_size_;
};

}  // namespace band

#endif  // BAND_SCHEDULER_SHORTEST_EXPECTED_LATENCY_SCHEDULER_H_
