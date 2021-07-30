
# 日志优化

新版日志是以原来的 __spdlog-1.6.1__ 为基础修改而来, 主要变动有:

1. 新增审计日志级别 __rtt.LogLevel.Audit__
2. 新增关闭日志级别 __rtt.LogTrace.Off__
3. 新增日志输出格式  __`set_backend_logpattern`__
4. 修改 __`set_backend_logfile`__ 函数, 支持通过 __`task_id`__ 映射日志句柄
5. 优化流式打印
6. 新增fmt打印

## 1 日志级别

日志级别共有七类

```python
[07-19 18:36:56 leaker@ubuntu ~/Desktop/tmp/Rosetta0712]$ more cc/python_export/_rosetta.cc
py::enum_<LogLevel>(m, "LogLevel")
.value("Trace", LogLevel::Trace)
.value("Debug", LogLevel::Debug)
.value("Audit", LogLevel::Audit)
.value("Info",  LogLevel::Info)
.value("Warn",  LogLevel::Warn)
.value("Error", LogLevel::Error)
.value("Fatal", LogLevel::Fatal)
.value("Off", LogLevel::Off)
```



其中, __LogLevel::Audit__ 为新增日志级别.  当日志级别设为 __LogLevel::Off__ 时, 则关闭所有日志(包括控制台日志). 调用格式为:

```python
#!/usr/bin/env python3

import latticex.rosetta as rtt
import tensorflow as tf

...

rtt.set_backend_loglevel(rtt.LogLevel.Info) # info level
```

## 2 日志输出样式

日志输出样式由 __`set_backend_logpattern`__ (python调用层)和 __`set_pattern`__ (日志底层)函数控制. 其格式定义为: __`SPDLOG_ROSETTA_LOGGER_PATTERN`__ . 需要主要的是, __Release__ 版本和 __Debug__ 版本的缺省输出样式有所不同:

1. Debug:  __`%Y-%m-%d %H:%M:%S.%e|%^%l%$|%s:%#|%!|%v`__
2. Release:   __`%Y-%m-%d %H:%M:%S.%e|%^%l%$|%v`__

即Debug版本的日志会带有 __file:line__ ( __%s:%#__ ) 和函数名 ( __%!__ )


## 3 日志文件

日志文件名设置函数为: __`set_backend_logfile`__ (python调用层)和 __`set_filename`__ (日志底层)函数控制. 相对旧版, 新版新增了一个参数 __`task_id`__. 用于查找日志文件句柄.

函数定义为:

```cpp
void Logger::set_filename(const std::string& filename, const std::string& task_id="");
```

函数实现逻辑为:

1. 查找日志文件 __filename__ 是否存在, 存在则函数返回. 反之, 转下一步
2. 判断 __`task_id`__ 是否为空字符串. 如果非空, 则创建一个异步非阻塞的常规日志文件( __`auto logger = spdlog::create_async_nb<spdlog::sinks::basic_file_sink_mt>(task_id.c_str(), filename.c_str())`__ ), 并把 __`filename`__ 和 __`task_id`__ 做关联后, 成功返回. 反之, 转下一步.
3. 判断缺省的日志句柄 __`SPDLOG_ROSETTA_LOGGER_NAME `__ 是否已经存在。如果存在, 则创建一个异步非阻塞的常规日志文件, 然后把文件名作为日志文件的句柄. 反之, 转下一步.
4. 创建一个异步非阻塞的常规日志文件, 然后把缺省的日志文件句柄和该日志文件做关联



## 4 流式日志

流式日志是通过宏 __`SPDLOG_LOGGER_STREAM`__ 实现:

```c
#define SPDLOG_LOGGER_STREAM(log, lvl)    LogStream(log, lvl, spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}) == LogLine()
```

其底层实现为 __LogLine__ 类. 主要是把以前旧版的 __继承std::ostringstream__ 更改为 __组合__ . 具体实现为:

```cpp
class LogLine 
{
public:
LogLine() {
}

template<typename T> LogLine& operator<<(const T& t) { 
ss_ << t; 

return *this; 
}

std::string str() const { 
return ss_.str(); 
}

private:
std::ostringstream ss_;
};
```



## 5 fmt日志

fmt日志是通过宏 __`SPDLOG_LOGGER_CALL_FUNCTION`__ 实现:

```cpp
#define SPDLOG_LOGGER_CALL_FUNCTION(logger, level, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, level, ##__VA_ARGS__)
```

每种日志级别的宏定义(以AUDIT为例):

```c++
#define TAUDIT_(task_id, ...) \
   do {\
   if (Logger::Get().get_log_to_stdout()) { \
   SPDLOG_LOGGER_CALL_FUNCTION(spdlog::default_logger(), spdlog::level::audit, ##__VA_ARGS__); \
   } \
   std::shared_ptr<spdlog::logger> logger = Logger::Get().get_logger(task_id); \
   if (logger != nullptr && logger != spdlog::default_logger())  {\
   SPDLOG_LOGGER_CALL_FUNCTION(Logger::Get().get_logger(task_id), spdlog::level::audit, ##__VA_ARGS__); \
   } \
   } while(0)

#define AUDIT(...) TAUDIT_(context_->TASK_ID, ##__VA_ARGS__)
```

## 6 注意事项
1. 流式日志( __`log_/tlog_`__ 系列)不支持std::endl强刷, 因为日志带有换行功能
2. 如果有隐式的 __`TASK_ID`__ , 请使用 __`log_`__ 系列, 不要使用 __`tlog_`__ 系列(其实, 绝大部分日志打印都是使用 __`log_`__ 系列, 除非是单模板函数没有 __`TASK_ID`__ 的日志打印的话, 才需要使用 __`tlog_`__ 系列)
3. 基于点 __2__ , 流式日志请直接使用 __`log_`__ 系列. 因为如果是错误使用, 编译会直接报错
4. 如果是 __Debug__ 模式的日志输出, 请不要再次使用 __`__FUNCTION__`__ , __`__func__`__, __`__PRETTY_FUNCTION__`__, __`__LINE__`__, __`__FILENAME__`__ 等和宏定义, 因为Debug模式下, 日志已经带了文件名:行号和函数名的相关信息
