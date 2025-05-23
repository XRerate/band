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

#include "band/c/c_api.h"

#include <gtest/gtest.h>
#include <stdarg.h>
#include <stdint.h>

#include <array>
#include <fstream>
#include <vector>

#include "band/buffer/buffer.h"
#include "band/c/c_api_buffer.h"
#include "band/test/image_util.h"

namespace band {
namespace test {
TEST(CApi, ConfigLoad) {
  BandConfigBuilder* b = BandConfigBuilderCreate();
  BandAddConfig(b, BAND_PLANNER_LOG_PATH, /*count=*/1,
                "band/test/data/log.json");
  BandAddConfig(b, BAND_PLANNER_SCHEDULERS, /*count=*/1, kBandRoundRobin);
  BandAddConfig(b, BAND_MINIMUM_SUBGRAPH_SIZE, /*count=*/1, 7);
  BandAddConfig(b, BAND_SUBGRAPH_PREPARATION_TYPE, /*count=*/1,
                kBandMergeUnitSubgraph);
  BandAddConfig(b, BAND_CPU_MASK, /*count=*/1, kBandAll);
  BandAddConfig(b, BAND_PLANNER_CPU_MASK, /*count=*/1, kBandPrimary);
  BandAddConfig(b, BAND_WORKER_WORKERS, /*count=*/2, kBandCPU, kBandCPU);
  BandAddConfig(b, BAND_WORKER_NUM_THREADS, /*count=*/2, 3, 4);
  BandAddConfig(b, BAND_WORKER_CPU_MASKS, /*count=*/2, kBandBig, kBandLittle);
  BandAddConfig(b, BAND_PROFILE_SMOOTHING_FACTOR, /*count=*/1, 0.1f);
  BandAddConfig(b, BAND_PROFILE_DATA_PATH, /*count=*/1,
                "band/test/data/profile.json");
  BandAddConfig(b, BAND_PROFILE_ONLINE, /*count=*/1, true);
  BandAddConfig(b, BAND_PROFILE_NUM_WARMUPS, /*count=*/1, 1);
  BandAddConfig(b, BAND_PROFILE_NUM_RUNS, /*count=*/1, 1);
  BandAddConfig(b, BAND_WORKER_ALLOW_WORKSTEAL, /*count=*/1, true);
  BandAddConfig(b, BAND_WORKER_AVAILABILITY_CHECK_INTERVAL_MS, /*count=*/1,
                30000);
  BandAddConfig(b, BAND_PLANNER_SCHEDULE_WINDOW_SIZE, /*count=*/1, 10);
  BandConfig* config = BandConfigCreate(b);
  EXPECT_NE(config, nullptr);
  BandConfigDelete(config);
}

TEST(CApi, ModelLoad) {
  BandModel* model = BandModelCreate();
  EXPECT_NE(model, nullptr);
#ifdef BAND_TFLITE
  EXPECT_EQ(
      BandModelAddFromFile(model, kBandTfLite, "band/test/data/add.tflite"),
      kBandOk);
#endif  // BAND_TFLITE
  BandModelDelete(model);
}

TEST(CApi, EngineSimpleSyncInvoke) {
  BandConfigBuilder* b = BandConfigBuilderCreate();
  BandAddConfig(b, BAND_PLANNER_LOG_PATH, /*count=*/1,
                "band/test/data/log.json");
  BandAddConfig(b, BAND_PLANNER_SCHEDULERS, /*count=*/1, kBandRoundRobin);
  BandAddConfig(b, BAND_MINIMUM_SUBGRAPH_SIZE, /*count=*/1, 7);
  BandAddConfig(b, BAND_SUBGRAPH_PREPARATION_TYPE, /*count=*/1,
                kBandMergeUnitSubgraph);
  BandAddConfig(b, BAND_CPU_MASK, /*count=*/1, kBandAll);
  BandAddConfig(b, BAND_PLANNER_CPU_MASK, /*count=*/1, kBandPrimary);
  BandAddConfig(b, BAND_WORKER_WORKERS, /*count=*/2, kBandCPU, kBandCPU);
  BandAddConfig(b, BAND_WORKER_NUM_THREADS, /*count=*/2, 3, 4);
  BandAddConfig(b, BAND_WORKER_CPU_MASKS, /*count=*/2, kBandBig, kBandLittle);
  BandAddConfig(b, BAND_PROFILE_SMOOTHING_FACTOR, /*count=*/1, 0.1f);
  BandAddConfig(b, BAND_PROFILE_DATA_PATH, /*count=*/1,
                "band/test/data/profile.json");
  BandAddConfig(b, BAND_PROFILE_ONLINE, /*count=*/1, true);
  BandAddConfig(b, BAND_PROFILE_NUM_WARMUPS, /*count=*/1, 1);
  BandAddConfig(b, BAND_PROFILE_NUM_RUNS, /*count=*/1, 1);
  BandAddConfig(b, BAND_WORKER_ALLOW_WORKSTEAL, /*count=*/1, true);
  BandAddConfig(b, BAND_WORKER_AVAILABILITY_CHECK_INTERVAL_MS, /*count=*/1,
                30000);
  BandAddConfig(b, BAND_PLANNER_SCHEDULE_WINDOW_SIZE, /*count=*/1, 10);
  BandConfig* config = BandConfigCreate(b);
  EXPECT_NE(config, nullptr);

  BandEngine* engine = BandEngineCreate(config);
  EXPECT_NE(engine, nullptr);

  BandModel* model = BandModelCreate();
  EXPECT_NE(model, nullptr);
#ifdef BAND_TFLITE
  EXPECT_EQ(
      BandModelAddFromFile(model, kBandTfLite, "band/test/data/add.tflite"),
      kBandOk);
  EXPECT_EQ(BandEngineRegisterModel(engine, model), kBandOk);
  EXPECT_EQ(BandEngineGetNumInputTensors(engine, model), 1);
  EXPECT_EQ(BandEngineGetNumOutputTensors(engine, model), 1);

  BandTensor* input_tensor = BandEngineCreateInputTensor(engine, model, 0);
  BandTensor* output_tensor = BandEngineCreateOutputTensor(engine, model, 0);

  std::array<float, 2> input = {1.f, 3.f};
  memcpy(BandTensorGetData(input_tensor), input.data(),
         input.size() * sizeof(float));
  EXPECT_EQ(BandEngineRequestSync(engine, model, &input_tensor, &output_tensor),
            kBandOk);

  EXPECT_EQ(reinterpret_cast<float*>(BandTensorGetData(output_tensor))[0], 3.f);
  EXPECT_EQ(reinterpret_cast<float*>(BandTensorGetData(output_tensor))[1], 9.f);

  EXPECT_EQ(BandTensorGetNumDims(output_tensor), 4);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[0], 1);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[1], 8);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[2], 8);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[3], 3);

  BandTensorDelete(input_tensor);
  BandTensorDelete(output_tensor);
#endif  // BAND_TFLITE
  BandEngineDelete(engine);
  BandConfigDelete(config);
  BandModelDelete(model);
}

TEST(CApi, EngineSimpleAsyncInvoke) {
  BandConfigBuilder* b = BandConfigBuilderCreate();
  BandAddConfig(b, BAND_PLANNER_LOG_PATH, /*count=*/1,
                "band/test/data/log.json");
  BandAddConfig(b, BAND_PLANNER_SCHEDULERS, /*count=*/1, kBandRoundRobin);
  BandAddConfig(b, BAND_MINIMUM_SUBGRAPH_SIZE, /*count=*/1, 7);
  BandAddConfig(b, BAND_SUBGRAPH_PREPARATION_TYPE, /*count=*/1,
                kBandMergeUnitSubgraph);
  BandAddConfig(b, BAND_CPU_MASK, /*count=*/1, kBandAll);
  BandAddConfig(b, BAND_PLANNER_CPU_MASK, /*count=*/1, kBandPrimary);
  BandAddConfig(b, BAND_WORKER_WORKERS, /*count=*/2, kBandCPU, kBandCPU);
  BandAddConfig(b, BAND_WORKER_NUM_THREADS, /*count=*/2, 3, 4);
  BandAddConfig(b, BAND_WORKER_CPU_MASKS, /*count=*/2, kBandBig, kBandLittle);
  BandAddConfig(b, BAND_PROFILE_SMOOTHING_FACTOR, /*count=*/1, 0.1f);
  BandAddConfig(b, BAND_PROFILE_DATA_PATH, /*count=*/1,
                "band/test/data/profile.json");
  BandAddConfig(b, BAND_PROFILE_ONLINE, /*count=*/1, true);
  BandAddConfig(b, BAND_PROFILE_NUM_WARMUPS, /*count=*/1, 1);
  BandAddConfig(b, BAND_PROFILE_NUM_RUNS, /*count=*/1, 1);
  BandAddConfig(b, BAND_WORKER_ALLOW_WORKSTEAL, /*count=*/1, true);
  BandAddConfig(b, BAND_WORKER_AVAILABILITY_CHECK_INTERVAL_MS, /*count=*/1,
                30000);
  BandAddConfig(b, BAND_PLANNER_SCHEDULE_WINDOW_SIZE, /*count=*/1, 10);
  BandConfig* config = BandConfigCreate(b);
  EXPECT_NE(config, nullptr);

  BandEngine* engine = BandEngineCreate(config);
  EXPECT_NE(engine, nullptr);

  BandModel* model = BandModelCreate();
  EXPECT_NE(model, nullptr);

  bool is_finished = false;
  auto callback = [](void* user_data, BandRequestHandle handle, BandStatus s) {
    bool* is_finished = reinterpret_cast<bool*>(user_data);
    *is_finished = true;
  };

  BandCallbackHandle callback_handle =
      BandEngineSetOnEndRequest(engine, callback, &is_finished);

#ifdef BAND_TFLITE
  EXPECT_EQ(
      BandModelAddFromFile(model, kBandTfLite, "band/test/data/add.tflite"),
      kBandOk);
  EXPECT_EQ(BandEngineRegisterModel(engine, model), kBandOk);
  EXPECT_EQ(BandEngineGetNumInputTensors(engine, model), 1);
  EXPECT_EQ(BandEngineGetNumOutputTensors(engine, model), 1);

  BandTensor* input_tensor = BandEngineCreateInputTensor(engine, model, 0);
  BandTensor* output_tensor = BandEngineCreateOutputTensor(engine, model, 0);

  std::array<float, 2> input = {1.f, 3.f};
  memcpy(BandTensorGetData(input_tensor), input.data(),
         input.size() * sizeof(float));
  BandRequestHandle handle =
      BandEngineRequestAsync(engine, model, &input_tensor);

  EXPECT_EQ(BandEngineWait(engine, handle, &output_tensor, 1), kBandOk);
  EXPECT_EQ(is_finished, true);

  EXPECT_EQ(reinterpret_cast<float*>(BandTensorGetData(output_tensor))[0], 3.f);
  EXPECT_EQ(reinterpret_cast<float*>(BandTensorGetData(output_tensor))[1], 9.f);

  EXPECT_EQ(BandTensorGetNumDims(output_tensor), 4);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[0], 1);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[1], 8);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[2], 8);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[3], 3);

  EXPECT_EQ(BandEngineUnsetOnEndRequest(engine, callback_handle), kBandOk);

  BandTensorDelete(input_tensor);
  BandTensorDelete(output_tensor);
#endif  // BAND_TFLITE

  BandEngineDelete(engine);
  BandConfigDelete(config);
  BandModelDelete(model);
}

TEST(CApi, EngineFixedDeviceFixedWorkerInvoke) {
  BandConfigBuilder* b = BandConfigBuilderCreate();
  BandAddConfig(b, BAND_PLANNER_LOG_PATH, /*count=*/1,
                "band/test/data/log.json");
  BandAddConfig(b, BAND_PLANNER_SCHEDULERS, /*count=*/1, kBandFixedWorker);
  BandConfig* config = BandConfigCreate(b);
  EXPECT_NE(config, nullptr);

  BandEngine* engine = BandEngineCreate(config);
  EXPECT_NE(engine, nullptr);

#ifdef BAND_TFLITE
  BandModel* model = BandModelCreate();
  EXPECT_NE(model, nullptr);
  EXPECT_EQ(
      BandModelAddFromFile(model, kBandTfLite, "band/test/data/add.tflite"),
      kBandOk);
  EXPECT_EQ(BandEngineRegisterModel(engine, model), kBandOk);
  EXPECT_EQ(BandEngineGetNumInputTensors(engine, model), 1);
  EXPECT_EQ(BandEngineGetNumOutputTensors(engine, model), 1);

  BandTensor* input_tensor = BandEngineCreateInputTensor(engine, model, 0);
  BandTensor* output_tensor = BandEngineCreateOutputTensor(engine, model, 0);

  std::array<float, 2> input = {1.f, 3.f};
  memcpy(BandTensorGetData(input_tensor), input.data(),
         input.size() * sizeof(float));
  BandRequestOption request_option = BandRequestOptionGetDefault();
  request_option.target_worker = 0;
  EXPECT_EQ(BandEngineRequestSyncOptions(engine, model, request_option,
                                         &input_tensor, &output_tensor),
            kBandOk);

  EXPECT_EQ(reinterpret_cast<float*>(BandTensorGetData(output_tensor))[0], 3.f);
  EXPECT_EQ(reinterpret_cast<float*>(BandTensorGetData(output_tensor))[1], 9.f);

  EXPECT_EQ(BandTensorGetNumDims(output_tensor), 4);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[0], 1);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[1], 8);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[2], 8);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[3], 3);

  BandTensorDelete(input_tensor);
  BandTensorDelete(output_tensor);
#endif  // BAND_TFLITE

  BandEngineDelete(engine);
  BandConfigDelete(config);
}

TEST(CApi, EngineClassification) {
  BandConfigBuilder* b = BandConfigBuilderCreate();
  BandAddConfig(b, BAND_PLANNER_LOG_PATH, /*count=*/1,
                "band/test/data/log.json");
  BandAddConfig(b, BAND_PLANNER_SCHEDULERS, /*count=*/1,
                kBandHeterogeneousEarliestFinishTime);
  BandConfig* config = BandConfigCreate(b);
  EXPECT_NE(config, nullptr);

  BandEngine* engine = BandEngineCreate(config);
  EXPECT_NE(engine, nullptr);
#ifdef BAND_TFLITE

  BandModel* model = BandModelCreate();
  EXPECT_NE(model, nullptr);
  EXPECT_EQ(
      BandModelAddFromFile(model, kBandTfLite,
                           "band/test/data/mobilenet_v2_1.0_224_quant.tflite"),
      kBandOk);
  EXPECT_EQ(BandEngineRegisterModel(engine, model), kBandOk);
  // create tensors
  BandTensor* input_tensor = BandEngineCreateInputTensor(engine, model, 0);
  BandTensor* output_tensor = BandEngineCreateOutputTensor(engine, model, 0);
  // create buffer
  std::tuple<unsigned char*, int, int> image_buffer =
      LoadRGBImageRaw("band/test/data/cat.jpg");
  BandBuffer* buffer = BandBufferCreate();
  BandBufferSetFromRawData(buffer, std::get<0>(image_buffer),
                           std::get<1>(image_buffer), std::get<2>(image_buffer),
                           kBandRGB);
  BandImageProcessorBuilder* builder = BandImageProcessorBuilderCreate();
  BandImageProcessor* processor = BandImageProcessorBuilderBuild(builder);
  BandImageProcessorProcess(processor, buffer, input_tensor);

  EXPECT_EQ(BandEngineRequestSync(engine, model, &input_tensor, &output_tensor),
            kBandOk);

  EXPECT_EQ(BandTensorGetNumDims(output_tensor), 2);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[0], 1);
  EXPECT_EQ(BandTensorGetDims(output_tensor)[1], 1001);
  // TODO: post processing library
  unsigned char* output =
      static_cast<unsigned char*>(BandTensorGetData(output_tensor));

  size_t max_index = 0;
  unsigned char max_value = 0;
  for (size_t i = 0; i < 1001; ++i) {
    if (output[i] > max_value) {
      max_value = output[i];
      max_index = i;
    }
  }

  // tiger cat
  EXPECT_EQ(max_index, 282);

  BandTensorDelete(input_tensor);
  BandTensorDelete(output_tensor);

  BandImageProcessorBuilderDelete(builder);
  BandImageProcessorDelete(processor);
  delete[] std::get<0>(image_buffer);

#endif  // BAND_TFLITE

  BandEngineDelete(engine);
  BandConfigDelete(config);
}

}  // namespace test
}  // namespace band

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
