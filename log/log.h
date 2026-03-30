#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdarg.h>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

class Log
{
public:
    static Log* get_instance()
    {
        static Log instance;
        return &instance;
    }

    bool init(const char* file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char* format, ...);

    void flush(void);

private:
    Log();
    ~Log();
    void async_write_log();

private:
    char dir_name[128];
    char log_name[128];
    int m_split_lines;
    int m_log_buf_size;
    long long m_count;
    int m_today;
    FILE* m_fp;
    char* m_buf;
    bool m_is_async;
    int m_close_log;

    vector<string> m_buf1;
    vector<string> m_buf2;
    vector<string>* m_produce_buf;
    vector<string>* m_consume_buf;

    mutex m_mutex;
    condition_variable m_cond;
    thread m_write_thread;
    bool m_exit;
};

#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__);
#define LOG_INFO(format, ...)  Log::get_instance()->write_log(1, format, ##__VA_ARGS__);
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__);

#endif
