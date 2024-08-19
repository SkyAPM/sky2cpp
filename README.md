# cpp2sky

![cpp2sky test](https://github.com/SkyAPM/cpp2sky/workflows/cpp2sky%20test/badge.svg)

Distributed tracing and monitor SDK in CPP for Apache SkyWalking APM. This SDK is compatible with C++ 17, C++ 14, and C++ 11.

## Build

#### Bazel

Download cpp2sky tarball with specified version.

```
http_archive(
  name = "com_github_skyapm_cpp2sky",
  sha256 = <SHA256>,
  urls = ["https://github.com/skyAPM/cpp2sky/archive/<VERSION>.tar.gz"],
)
```

Add interface definition and library to your project.

```
cc_binary(
  name = "example",
  srcs = ["example.cc"],
  deps = [
    "@com_github_skyapm_cpp2sky//cpp2sky:cpp2sky_interface",
    "@com_github_skyapm_cpp2sky//source:cpp2sky_lib"
  ],
)
```

#### Cmake

You can compile this project, according to the following steps:
```
step 01: git clone git@github.com:SkyAPM/cpp2sky.git
step 02: git clone -b v9.1.0 https://github.com/apache/skywalking-data-collect-protocol.git ./3rdparty/skywalking-data-collect-protocol
step 03: git clone -b v1.46.6 https://github.com/grpc/grpc.git --recursive
step 04: cmake -S ./grpc -B ./grpc/build && cmake --build ./grpc/build --parallel 8 --target install
step 05: cmake -S . -B ./build && cmake --build ./build
```

You can also use find_package to get target libary in your project. Like this:
```
find_package(cpp2sky CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} cpp2sky::cpp2sky proto_lib)
```
Of course, if OS is similar to Unix, you can also use pkgconfig to build the project. Like this:
```
find_package(PkgConfig REQUIRED)
pkg_check_modules(CPP2SKY_PKG REQUIRED cpp2sky)
```

Note:
- If you want to build this project over c11, you must update grpc version(current version:v1.46.6).
- Only test cmake using Centos and Ubuntu.

#### Develop

Generate `compile_commands.json` for this repo by `bazel run :refresh_compile_commands`. Thank https://github.com/hedronvision/bazel-compile-commands-extractor for it provide the great script/tool to make this so easy!

#### Docs

cpp2sky configration is based on protobuf, and docs are generated by [protodoc](https://github.com/etcd-io/protodoc). If you have any API change, you should run below.

```
protodoc --directory=./cpp2sky --parse="message" --languages="C++" --title=cpp2sky config --output=docs/README.md
```

## Basic usage

#### Config

cpp2sky provides simple configuration for tracer. API docs are available at `docs/README.md`.
The detail information is described in [official protobuf definition](https://github.com/apache/skywalking-data-collect-protocol/blob/master/language-agent/Tracing.proto#L57-L67).

```cpp
#include <cpp2sky/config.pb.h>

int main() {
  using namespace cpp2sky;

  static const std::string service_name = "service_name";
  static const std::string instance_name = "instance_name";
  static const std::string oap_addr = "oap:12800";
  static const std::string token = "token";

  TracerConfig tracer_config;

  config.set_instance_name(instance_name);
  config.set_service_name(service_name);
  config.set_address(oap_addr);
  config.set_token(token);
}
```

#### Create tracer

After you constructed config, then setup tracer. Tracer supports gRPC reporter only, also TLS adopted gRPC reporter isn't available now.
TLS adoption and REST tracer will be supported in the future.

```cpp
TracerConfig tracer_config;

// Setup

TracerPtr tracer = createInsecureGrpcTracer(tracer_config);
```

#### Fetch propagated span

cpp2sky supports only HTTP tracer now.
Tracing span will be delivered from `sw8` and `sw8-x` HTTP headers. For more detail, please visit [here](https://github.com/apache/skywalking/blob/08781b41a8255bcceebb3287364c81745a04bec6/docs/en/protocols/Skywalking-Cross-Process-Propagation-Headers-Protocol-v3.md)
Then, you can create propagated span object by decoding these items.

```cpp
SpanContextSharedPtr parent_span = createSpanContext(parent);
```

#### Create span

First, you must create tracing context that holds all spans, then crete initial entry span.

```cpp
TracingContextSharedPtr tracing_context = tracer->newContext();
TracingSpanSharedPtr tracing_span = tracing_context->createEntrySpan();
```

After that, you can create another span to trace another workload, such as RPC to other services.
Note that you must have parent span to create secondary span. It will construct parent-child relation when analysis.

```cpp
TracingSpanSharedPtr current_span = tracing_context->createExitSpan(current_span);
```

Alternative approach is RAII based one. It is used like below,

```cpp
{
  StartEntrySpan entry_span(tracing_context, "sample_op1");
  {
    StartExitSpan exit_span(tracing_context, entry_span.get(), "sample_op2");

    // something...
  }
}
```

#### Send segment to OAP

Note that TracingContext is unique pointer. So when you'd like to send data, you must move it and don't refer after sending,
to avoid undefined behavior.

```cpp
TracingContextSharedPtr tracing_context = tracer->newContext();
TracingSpanSharedPtr tracing_span = tracing_context->createEntrySpan();

tracing_span->startSpan("sample_workload");
tracing_span->endSpan();

tracer->report(std::move(tracing_context));
```

#### Skywalking CDS

C++ agent implements Skywalking CDS feature it allows to change bootstrap config dynamically from the response of sync request, invoked from this periodically.
Dynamically configurable values are described in description of properties on `docs/README.md'.

```cpp
TracerConfig config;
// If you set this value as zero, CDS request won't occur.
config.set_cds_request_interval(5); // CDS request interval should be 5sec
```

If you are using Consul KVS as backend, we could put configuration value through HTTP request.

```yaml
configurations:
  service_name:
    ignore_suffix: '/ignore, /hoge'
```

After setup configurations, try to put values with

```
curl --request PUT --data-binary "@./config.yaml" http://localhost:8500/v1/kv/configuration-discovery.default.agentConfigurations
```

## Trace and Log integration

cpp2sky implements to output logs which is the key to integrate with actual tracing context.

#### Supported Logger

- [spdlog](https://github.com/gabime/spdlog)

```cpp
#include <spdlog/spdlog.h>
#include <cpp2sky/trace_log.h>

int main() {
  auto logger = spdlog::default_logger();
  // set_pattern must be called.
  logger->set_pattern(logFormat<decltype(logger)::element_type>());

  // It will generate log message as follows.
  //
  // {"level": "warning", "msg": "sample", "SW_CTX": ["service","instance","trace_id","segment_id","span_id"]}
  //
  logger->warn(tracing_context->logMessage("sample"));
}
```

## Security

If you've found any security issues, please read [Security Reporting Process](https://github.com/SkyAPM/cpp2sky/blob/main/SECURITY.md) and take described steps.

## LICENSE

Apache 2.0 License. See [LICENSE](https://github.com/SkyAPM/cpp2sky/blob/main/LICENSE) for more detail.
