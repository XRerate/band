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

#include "band/c/c_api_internal.h"
#include "band/interface/tensor.h"
#include "band/interface/tensor_view.h"
#include "c_api.h"

namespace {

std::vector<band::interface::ITensor*> BandTensorArrayToVec(
    BandTensor** tensors, int num_tensors) {
  std::vector<band::interface::ITensor*> vec(num_tensors);
  for (int i = 0; i < vec.size(); ++i) {
    vec[i] = tensors[i]->impl.get();
  }
  return vec;
}

BandStatus ToBandStatus(absl::Status& status) {
  if (status.code() == absl::StatusCode::kInternal) {
    return kBandErr;
  } else {
    return kBandOk;
  }
}

BandStatus ToBandStatus(absl::Status&& status) {
  if (status.code() == absl::StatusCode::kInternal) {
    return kBandErr;
  } else {
    return kBandOk;
  }
}

band::RequestOption ToRequestOption(BandRequestOption& option) {
  band::RequestOption request_option;
  request_option.target_worker = option.target_worker;
  request_option.require_callback = option.require_callback;
  request_option.slo_scale = option.slo_scale;
  request_option.slo_us = option.slo_us;
  return request_option;
}

}  // anonymous namespace

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

BandConfigBuilder* BandConfigBuilderCreate() { return new BandConfigBuilder; }

void BandSetLogSeverity(BandLogSeverity severity) {
  band::Logger::Get().SetVerbosity(static_cast<band::LogSeverity>(severity));
}

BandCallbackHandle BandSetLogReporter(void (*reporter)(BandLogSeverity severity,
                                                       const char* msg)) {
  std::function<void(band::LogSeverity, const char*)> reporter_invoke =
      [reporter](band::LogSeverity severity, const char* msg) {
        reporter(static_cast<BandLogSeverity>(severity), msg);
      };
  return band::Logger::Get().SetReporter(reporter_invoke);
}

void BandUnsetLogReporter(BandCallbackHandle handle) {
  if (!band::Logger::Get().RemoveReporter(handle).ok()) {
    BAND_LOG(band::LogSeverity::kWarning,
             "Failed to remove reporter with handle %d", handle);
  }
}

void BandAddConfig(BandConfigBuilder* b, int field, int count, ...) {
  if (!b) {
    BAND_LOG(band::LogSeverity::kError, "BandConfigBuilder is null");
    return;
  }

  va_list vl;
  va_start(vl, count);
  BandConfigField new_field = static_cast<BandConfigField>(field);
  switch (field) {
    case BAND_PROFILE_ONLINE: {
      bool arg = va_arg(vl, int);
      b->impl.AddOnline(arg);
    } break;
    case BAND_PROFILE_NUM_WARMUPS: {
      int arg = va_arg(vl, int);
      b->impl.AddNumWarmups(arg);
    } break;
    case BAND_PROFILE_NUM_RUNS: {
      int arg = va_arg(vl, int);
      b->impl.AddNumRuns(arg);
    } break;
    case BAND_PROFILE_SMOOTHING_FACTOR: {
      float arg = va_arg(vl, double);
      b->impl.AddSmoothingFactor(arg);
    } break;
    case BAND_PROFILE_DATA_PATH: {
      char* arg = va_arg(vl, char*);
      b->impl.AddProfileDataPath(arg);
    } break;
    case BAND_PLANNER_SCHEDULE_WINDOW_SIZE: {
      int arg = va_arg(vl, int);
      b->impl.AddScheduleWindowSize(arg);
    } break;
    case BAND_PLANNER_SCHEDULERS: {
      std::vector<band::SchedulerType> schedulers(count);
      for (int i = 0; i < count; i++) {
        schedulers[i] = static_cast<band::SchedulerType>(va_arg(vl, int));
      }
      b->impl.AddSchedulers(schedulers);
    } break;
    case BAND_PLANNER_CPU_MASK: {
      int arg = va_arg(vl, int);
      b->impl.AddPlannerCPUMask(static_cast<band::CPUMaskFlag>(arg));
    } break;
    case BAND_PLANNER_LOG_PATH: {
      char* arg = va_arg(vl, char*);
      b->impl.AddPlannerLogPath(arg);
    } break;
    case BAND_WORKER_WORKERS: {
      std::vector<band::DeviceFlag> workers(count);
      for (int i = 0; i < count; i++) {
        int temp = va_arg(vl, int);
        workers[i] = static_cast<band::DeviceFlag>(temp);
      }
      b->impl.AddWorkers(workers);
    } break;
    case BAND_WORKER_CPU_MASKS: {
      std::vector<band::CPUMaskFlag> cpu_masks(count);
      for (int i = 0; i < count; i++) {
        cpu_masks[i] = static_cast<band::CPUMaskFlag>(va_arg(vl, int));
      }
      b->impl.AddWorkerCPUMasks(cpu_masks);
    } break;
    case BAND_WORKER_NUM_THREADS: {
      std::vector<int> num_threads(count);
      for (int i = 0; i < count; i++) {
        num_threads[i] = va_arg(vl, int);
      }
      b->impl.AddWorkerNumThreads(num_threads);
    } break;
    case BAND_WORKER_ALLOW_WORKSTEAL: {
      bool arg = va_arg(vl, int);
      b->impl.AddAllowWorkSteal(arg);
    } break;
    case BAND_WORKER_AVAILABILITY_CHECK_INTERVAL_MS: {
      int arg = va_arg(vl, int);
      b->impl.AddAvailabilityCheckIntervalMs(arg);
    } break;
    case BAND_MINIMUM_SUBGRAPH_SIZE: {
      int arg = va_arg(vl, int);
      b->impl.AddMinimumSubgraphSize(arg);
    } break;
    case BAND_SUBGRAPH_PREPARATION_TYPE: {
      int arg = va_arg(vl, int);
      b->impl.AddSubgraphPreparationType(
          static_cast<band::SubgraphPreparationType>(arg));
    } break;
    case BAND_CPU_MASK: {
      int arg = va_arg(vl, int);
      b->impl.AddCPUMask(static_cast<band::CPUMaskFlag>(arg));
    } break;
  }
  va_end(vl);
}

void BandConfigBuilderDelete(BandConfigBuilder* b) {
  if (!b) {
    BAND_LOG(band::LogSeverity::kError, "BandConfigBuilder is null");
    return;
  }

  delete b;
}

BandConfig* BandConfigCreate(BandConfigBuilder* b) {
  if (!b) {
    BAND_LOG(band::LogSeverity::kError, "BandConfigBuilder is null");
    return nullptr;
  }

  auto config = b->impl.Build();
  if (!config.status().ok()) {
    return nullptr;
  } else {
    return new BandConfig(config.value());
  }
}

void BandConfigDelete(BandConfig* config) {
  if (!config) {
    BAND_LOG(band::LogSeverity::kError, "BandConfig is null");
    return;
  }

  delete config;
}

BandModel* BandModelCreate() { return new BandModel; }

void BandModelDelete(BandModel* model) {
  if (!model) {
    BAND_LOG(band::LogSeverity::kError, "BandModel is null");
    return;
  }

  delete model;
}

BandStatus BandModelAddFromBuffer(BandModel* model,
                                  BandBackendType backend_type,
                                  const void* model_data, size_t model_size) {
  if (!model) {
    BAND_LOG(band::LogSeverity::kError, "BandModel is null");
    return kBandErr;
  }
  return ToBandStatus(
      model->impl->FromBuffer(static_cast<band::BackendType>(backend_type),
                              (const char*)model_data, model_size));
}

BandStatus BandModelAddFromFile(BandModel* model, BandBackendType backend_type,
                                const char* model_path) {
  if (!model) {
    BAND_LOG(band::LogSeverity::kError, "BandModel is null");
    return kBandErr;
  }

  return ToBandStatus(model->impl->FromPath(
      static_cast<band::BackendType>(backend_type), model_path));
}

void BandTensorDelete(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return;
  }

  delete tensor;
}

BandDataType BandTensorGetType(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return BandDataType::kBandNumDataType;
  }

  return static_cast<BandDataType>(tensor->impl->GetType());
}

void* BandTensorGetData(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return nullptr;
  }

  return tensor->impl->GetData();
}

size_t BandTensorGetNumDims(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return 0;
  }

  return tensor->impl->GetNumDims();
}

const int* BandTensorGetDims(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return nullptr;
  }

  return tensor->impl->GetDims();
}

size_t BandTensorGetBytes(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return 0;
  }

  return tensor->impl->GetBytes();
}

const char* BandTensorGetName(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return nullptr;
  }

  return tensor->impl->GetName();
}

BandQuantizationType BandTensorGetQuantizationType(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return BandQuantizationType::kBandNumQuantizationType;
  }

  return static_cast<BandQuantizationType>(
      tensor->impl->GetQuantization().GetType());
}

void* BandTensorGetQuantizationParams(BandTensor* tensor) {
  if (!tensor) {
    BAND_LOG(band::LogSeverity::kError, "BandTensor is null");
    return nullptr;
  }

  return tensor->impl->GetQuantization().GetParams();
}

BandRequestOption BandRequestOptionGetDefault() { return {-1, true, -1, -1.f}; }

BandEngine* BandEngineCreateWithDefaultConfig() {
  BandConfig config{band::RuntimeConfigBuilder::GetDefaultConfig()};
  return BandEngineCreate(&config);
}

BandEngine* BandEngineCreate(BandConfig* config) {
  if (!config) {
    BAND_LOG(band::LogSeverity::kError, "BandConfig is null");
    return nullptr;
  }

  std::unique_ptr<band::Engine> engine(band::Engine::Create(config->impl));
  return engine ? new BandEngine(std::move(engine)) : nullptr;
}

void BandEngineDelete(BandEngine* engine) {
  if (!engine) {
    BAND_LOG(band::LogSeverity::kError, "BandEngine is null");
    return;
  }

  delete engine;
}

BandStatus BandEngineRegisterModel(BandEngine* engine, BandModel* model) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return kBandErr;
  }

  auto status = engine->impl->RegisterModel(model->impl.get());
  if (status == absl::OkStatus()) {
    engine->models.push_back(model->impl);
  }
  return ToBandStatus(status);
}

int BandEngineGetNumInputTensors(BandEngine* engine, BandModel* model) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return -1;
  }

  return engine->impl->GetInputTensorIndices(model->impl->GetId()).size();
}

int BandEngineGetNumOutputTensors(BandEngine* engine, BandModel* model) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return -1;
  }

  return engine->impl->GetOutputTensorIndices(model->impl->GetId()).size();
}

int BandEngineGetNumWorkers(BandEngine* engine) {
  if (!engine) {
    BAND_LOG(band::LogSeverity::kError, "BandEngine is null");
    return -1;
  }

  return engine->impl->GetNumWorkers();
}

BandDeviceFlag BandEngineGetWorkerDevice(BandEngine* engine, int worker_id) {
  if (!engine) {
    BAND_LOG(band::LogSeverity::kError, "BandEngine is null");
    return BandDeviceFlag::kBandNumDeviceFlag;
  }

  return static_cast<BandDeviceFlag>(engine->impl->GetWorkerDevice(worker_id));
}

BandTensor* BandEngineCreateInputTensor(BandEngine* engine, BandModel* model,
                                        size_t index) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return nullptr;
  }

  auto input_indices =
      engine->impl->GetInputTensorIndices(model->impl->GetId());
  return new BandTensor(
      engine->impl->CreateTensor(model->impl->GetId(), input_indices[index]));
}

BandTensor* BandEngineCreateOutputTensor(BandEngine* engine, BandModel* model,
                                         size_t index) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return nullptr;
  }

  auto output_indices =
      engine->impl->GetOutputTensorIndices(model->impl->GetId());
  return new BandTensor(
      engine->impl->CreateTensor(model->impl->GetId(), output_indices[index]));
}

BandStatus BandEngineRequestSync(BandEngine* engine, BandModel* model,
                                 BandTensor** input_tensors,
                                 BandTensor** output_tensors) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return kBandErr;
  }

  return ToBandStatus(engine->impl->RequestSync(
      model->impl->GetId(), band::RequestOption::GetDefaultOption(),
      BandTensorArrayToVec(input_tensors,
                           BandEngineGetNumInputTensors(engine, model)),
      BandTensorArrayToVec(output_tensors,
                           BandEngineGetNumOutputTensors(engine, model))));
}

BandRequestHandle BandEngineRequestAsync(BandEngine* engine, BandModel* model,
                                         BandTensor** input_tensors) {
  if (!engine || !model) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) is "
             "null",
             engine, model);
    return 0;
  }

  return engine->impl
      ->RequestAsync(
          model->impl->GetId(), band::RequestOption::GetDefaultOption(),
          BandTensorArrayToVec(input_tensors,
                               BandEngineGetNumInputTensors(engine, model)))
      .value();
}

BandStatus BandEngineRequestSyncOptions(BandEngine* engine, BandModel* model,
                                        BandRequestOption options,
                                        BandTensor** input_tensors,
                                        BandTensor** output_tensors) {
  if (!engine || !model || !input_tensors || !output_tensors) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) or input_tensors (%d) or "
             "output_tensors (%d) is "
             "null",
             engine, model, input_tensors, output_tensors);
    return kBandErr;
  }

  return ToBandStatus(engine->impl->RequestSync(
      model->impl->GetId(), ToRequestOption(options),
      BandTensorArrayToVec(input_tensors,
                           BandEngineGetNumInputTensors(engine, model)),
      BandTensorArrayToVec(output_tensors,
                           BandEngineGetNumOutputTensors(engine, model))));
}

BandRequestHandle BandEngineRequestAsyncOptions(BandEngine* engine,
                                                BandModel* model,
                                                BandRequestOption options,
                                                BandTensor** input_tensors) {
  if (!engine || !model || !input_tensors) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or BandModel (%d) or input_tensors (%d) is "
             "null",
             engine, model, input_tensors);
    return 0;
  }

  return engine->impl
      ->RequestAsync(
          model->impl->GetId(), ToRequestOption(options),
          BandTensorArrayToVec(input_tensors,
                               BandEngineGetNumInputTensors(engine, model)))
      .value();
}

BandStatus BandEngineWait(BandEngine* engine, BandRequestHandle handle,
                          BandTensor** output_tensors, size_t num_outputs) {
  if (!engine || !output_tensors) {
    BAND_LOG(band::LogSeverity::kError,
             "BandEngine (%d) or output_tensors (%d) is null", engine,
             output_tensors);
    return kBandErr;
  }

  return ToBandStatus(engine->impl->Wait(
      handle, BandTensorArrayToVec(output_tensors, num_outputs)));
}

BandCallbackHandle BandEngineSetOnEndRequest(
    BandEngine* engine,
    void (*on_end_invoke)(void* user_data, int job_id, BandStatus status),
    void* user_data) {
  if (!engine) {
    BAND_LOG(band::LogSeverity::kError, "BandEngine is null");
    return 0;
  }

  auto user_data_invoke = std::bind(
      on_end_invoke, user_data, std::placeholders::_1, std::placeholders::_2);
  std::function<void(int, absl::Status)> new_on_end_invoke =
      [user_data_invoke](int job_id, absl::Status status) {
        user_data_invoke(job_id, ToBandStatus(status));
      };
  return engine->impl->SetOnEndRequest(new_on_end_invoke);
}

BandStatus BandEngineUnsetOnEndRequest(BandEngine* engine,
                                       BandCallbackHandle handle) {
  if (!engine) {
    BAND_LOG(band::LogSeverity::kError, "BandEngine is null");
    return kBandErr;
  }

  return ToBandStatus(engine->impl->UnsetOnEndRequest(handle));
}

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
