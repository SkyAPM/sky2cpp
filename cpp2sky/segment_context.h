// Copyright 2020 SkyAPM

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <list>
#include <memory>
#include <string_view>

#include "cpp2sky/config.h"
#include "cpp2sky/propagation.h"
#include "language-agent/Tracing.pb.h"

namespace cpp2sky {

class CurrentSegmentSpan {
 public:
  virtual ~CurrentSegmentSpan() = default;

  /**
   * Generate Apache SkyWalking native span object from current segment span.
   */
  virtual SpanObject createSpanObject() = 0;

  /**
   * Get span ID.
   */
  virtual int32_t spanId() const = 0;

  /**
   * Get operation name.
   */
  virtual std::string operationName() const = 0;

  /**
   * Set parent span ID of this span.
   */
  virtual void setParentSpanId(int32_t span_id) = 0;

  /**
   * Set start time to calculate execution time.
   */
  virtual void setStartTime(int64_t start_time) = 0;

  /**
   * Set end time to calculate execution time.
   */
  virtual void setEndTime(int64_t end_time) = 0;

  /**
   * Set operation name for this span (lvalue)
   */
  virtual void setOperationName(std::string& operation_name) = 0;

  /**
   * Set operation name for this span (rvalue)
   */
  virtual void setOperationName(std::string&& operation_name) = 0;

  /**
   * Set peer address for this span (lvalue)
   */
  virtual void setPeer(std::string& remote_address) = 0;

  /**
   * Set peer address for this span (rvalue)
   */
  virtual void setPeer(std::string&& remote_address) = 0;

  /**
   * Set span type. Entry or Exit. Entry span means origin span which doesn't
   * have parent span, , like root node of span tree. Exit span has opposite
   * meaning, like leaf node of span tree.
   */
  virtual void setSpanType(SpanType type) = 0;

  /**
   * Set span layer. It supports only HTTP request tracing currently.
   */
  virtual void setSpanLayer(SpanLayer layer) = 0;

  /**
   * If error had caused on this span, This should be called.
   */
  virtual void errorOccured() = 0;

  /**
   * Determine whether to skip the analysis of this span. If we'd like to skip
   * analysis, this should be called.
   */
  virtual void skipAnalysis() = 0;

  /**
   * Set tag to current span. (lvalue)
   */
  virtual void addTag(std::string& key, std::string& value) = 0;

  /**
   * Set tag to current span. (rvalue)
   */
  virtual void addTag(std::string&& key, std::string&& value) = 0;

  /**
   * Add log related with current span.
   */
  virtual void addLog(int64_t time, std::string& key, std::string& value) = 0;
};

using CurrentSegmentSpanPtr = std::shared_ptr<CurrentSegmentSpan>;

class SegmentContext {
 public:
  virtual ~SegmentContext() = default;

  /**
   * Get trace ID. This value must be unique globally.
   */
  virtual const std::string& traceId() const = 0;

  /**
   * Get trace segment ID. This value must be unique globally.
   */
  virtual const std::string& traceSegmentId() const = 0;

  /**
   * Get service name.
   */
  virtual const std::string& service() const = 0;

  /**
   * Get service instance name.
   */
  virtual const std::string& serviceInstance() const = 0;

  /**
   * Get spans generated by this segment context.
   */
  virtual const std::list<CurrentSegmentSpanPtr>& spans() const = 0;

  /**
   * Get span context which generated this segment context as parent.
   */
  virtual SpanContextPtr parentSpanContext() const = 0;

  /**
   * Get span context extension which generated this segment context.
   */
  virtual SpanContextExtensionPtr parentSpanContextExtension() const = 0;

  /**
   * Generate a segment span related with this segment context.
   * @param parent_span Parent span which is extracted from caller.
   */
  virtual CurrentSegmentSpanPtr createCurrentSegmentSpan(
      CurrentSegmentSpanPtr parent_span) = 0;

  /**
   * Generate root segment span, called once per workload.
   */
  virtual CurrentSegmentSpanPtr createCurrentSegmentRootSpan() = 0;

  /**
   * Generate sw8 value to send SegmentRef.
   * @param parent Parent span that belongs to current segment.
   * @param target_address Target address to send request. For more detail:
   * https://github.com/apache/skywalking-data-collect-protocol/blob/master/language-agent/Tracing.proto#L97-L101
   */
  virtual std::string createSW8HeaderValue(CurrentSegmentSpanPtr parent,
                                           std::string& target_address) = 0;

  /**
   * Generate Apache SkyWalking native segment object.
   */
  virtual SegmentObject createSegmentObject() = 0;
};

using SegmentContextPtr = std::unique_ptr<SegmentContext>;

SegmentContextPtr createSegmentContext(Config& config, SpanContextPtr span_ctx,
                                       SpanContextExtensionPtr span_ctx_ext);

SegmentContextPtr createSegmentContext(Config& config, SpanContextPtr span_ctx);

SegmentContextPtr createSegmentContext(Config& config);

}  // namespace cpp2sky
