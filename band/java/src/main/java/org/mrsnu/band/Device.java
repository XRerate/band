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

package org.mrsnu.band;

public enum Device {
  CPU(0),
  GPU(1),
  DSP(2),
  NPU(3);

  private final int value;
  private static final Device[] enumValues = Device.values();

  Device(int value) {
    this.value = value;
  }

  int getValue() {
    return value;
  }

  public static Device fromValue(int value) {
    return enumValues[value];
  }
}
