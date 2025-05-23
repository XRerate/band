// Copyright 2023 Seoul National University
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "band/planner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "band/scheduler/scheduler.h"
#include "band/test/test_util.h"
#include "band/time.h"
#include "band/worker.h"

namespace band {
namespace test {

struct MockEngine : public MockEngineBase {
  void PrepareReenqueue(Job&) override{};
  void UpdateLatency(const SubgraphKey&, int64_t) override{};
  void EnqueueFinishedJob(Job& job) override { finished.insert(job.job_id); }
  void Trigger() override {}

  absl::Status Invoke(const SubgraphKey& key) override {
    time::SleepForMicros(50);
    return absl::OkStatus();
  }

  std::set<int> finished;
};

class MockScheduler : public IScheduler {
  using IScheduler::IScheduler;

  MOCK_METHOD1(Schedule, bool(JobQueue&));
  MOCK_METHOD0(NeedFallbackSubgraphs, bool());
  // MOCK_METHOD0(GetWorkerType, WorkerType());
  WorkerType GetWorkerType() { return WorkerType::kDeviceQueue; }
};

/*

Job cycle

planner -> scheduler -> worker -> planner

*/

TEST(PlannerSuite, SingleQueue) {
  MockEngine engine;
  Planner planner(engine);
  auto status = planner.AddScheduler(std::make_unique<MockScheduler>(engine));
  EXPECT_EQ(status, absl::OkStatus());
  // TODO: Add tests!
  EXPECT_TRUE(true);
}

}  // namespace test
}  // namespace band

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}