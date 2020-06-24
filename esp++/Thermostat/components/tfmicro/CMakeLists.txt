
# Copyright 2019 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# This component was generated for the 'hello_world' TF Micro example.
#

# Make sure that the IDF Path environment variable is defined
if(NOT DEFINED ENV{IDF_PATH})
  message(FATAL_ERROR "The IDF_PATH environment variable must point to the location of the ESP-IDF.")
endif()

idf_component_register(
  SRCS tensorflow/lite/experimental/micro/simple_memory_allocator.cc tensorflow/lite/experimental/micro/memory_helpers.cc tensorflow/lite/experimental/micro/test_helpers.cc tensorflow/lite/experimental/micro/micro_error_reporter.cc tensorflow/lite/experimental/micro/micro_mutable_op_resolver.cc tensorflow/lite/experimental/micro/debug_log.cc tensorflow/lite/experimental/micro/micro_allocator.cc tensorflow/lite/experimental/micro/micro_utils.cc tensorflow/lite/experimental/micro/micro_optional_debug_tools.cc tensorflow/lite/experimental/micro/debug_log_numbers.cc tensorflow/lite/experimental/micro/micro_interpreter.cc tensorflow/lite/experimental/micro/kernels/pack.cc tensorflow/lite/experimental/micro/kernels/logical.cc tensorflow/lite/experimental/micro/kernels/comparisons.cc tensorflow/lite/experimental/micro/kernels/reshape.cc tensorflow/lite/experimental/micro/kernels/depthwise_conv.cc tensorflow/lite/experimental/micro/kernels/conv.cc tensorflow/lite/experimental/micro/kernels/softmax.cc tensorflow/lite/experimental/micro/kernels/prelu.cc tensorflow/lite/experimental/micro/kernels/dequantize.cc tensorflow/lite/experimental/micro/kernels/pooling.cc tensorflow/lite/experimental/micro/kernels/activations.cc tensorflow/lite/experimental/micro/kernels/ceil.cc tensorflow/lite/experimental/micro/kernels/arg_min_max.cc tensorflow/lite/experimental/micro/kernels/split.cc tensorflow/lite/experimental/micro/kernels/add.cc tensorflow/lite/experimental/micro/kernels/floor.cc tensorflow/lite/experimental/micro/kernels/logistic.cc tensorflow/lite/experimental/micro/kernels/svdf.cc tensorflow/lite/experimental/micro/kernels/unpack.cc tensorflow/lite/experimental/micro/kernels/neg.cc tensorflow/lite/experimental/micro/kernels/quantize.cc tensorflow/lite/experimental/micro/kernels/all_ops_resolver.cc tensorflow/lite/experimental/micro/kernels/fully_connected.cc tensorflow/lite/experimental/micro/kernels/maximum_minimum.cc tensorflow/lite/experimental/micro/kernels/elementwise.cc tensorflow/lite/experimental/micro/kernels/strided_slice.cc tensorflow/lite/experimental/micro/kernels/round.cc tensorflow/lite/experimental/micro/memory_planner/linear_memory_planner.cc tensorflow/lite/experimental/micro/memory_planner/greedy_memory_planner.cc tensorflow/lite/c/c_api_internal.c tensorflow/lite/core/api/error_reporter.cc tensorflow/lite/core/api/flatbuffer_conversions.cc tensorflow/lite/core/api/op_resolver.cc tensorflow/lite/core/api/tensor_utils.cc tensorflow/lite/kernels/internal/quantization_util.cc tensorflow/lite/kernels/kernel_util.cc 
  INCLUDE_DIRS . third_party/gemmlowp third_party/flatbuffers/include third_party/kissfft)

# Reduce the level of paranoia to be able to compile TF sources
target_compile_options(${COMPONENT_LIB} PRIVATE
  -Wno-maybe-uninitialized
  -Wno-missing-field-initializers
  -Wno-pointer-sign
  -Wno-type-limits)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG -g -DTF_LITE_STATIC_MEMORY")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -std=c++11 -g -DTF_LITE_STATIC_MEMORY")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
