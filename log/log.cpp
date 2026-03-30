#include "log.h"
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <algorithm>
#include <sys/time.h>
#ifdef _WIN32
#include <windows.h>
void gettimeofday(struct timeval* tv, void* tz)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    long long t = ((long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    t -= 116444736000000000LL;
    tv->tv_sec = (long)(t / 10000000);
    tv->tv_usec = (long)((t % 10000000) / 10);
}
#endif

Log::Log()
    : m_count(0), m_is_async(false), m_close_log(0),
      m_fp(nullptr), m_buf(nullptr), m_exit(false)
{
    m_produce_buf = &m_buf1;
    m_consume_buf = &m_buf2;
}

Log::~Log()
{
    if (m_is_async) {
        m_exit = true;
        m_cond.notify_one();
        if (m_write_thread.joinable())
            m_write_thread.join();
    }
    if (m_fp) {
        fflush(m_fp);
        fclose(m_fp);
    }
    delete[] m_buf;
}

bool Log::init(const char* file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size]();
    m_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char* p = strrchr(file_name, '/');
    char log_full_name[256] = { 0 };

    if (p == nullptr) {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s",
            my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s",
            dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;
    m_fp = fopen(log_full_name, "a");
    if (!m_fp) return false;

    if (max_queue_size > 0) {
        m_is_async = true;
        m_buf1.reserve(max_queue_size);
        m_buf2.reserve(max_queue_size);
        m_write_thread = thread(&Log::async_write_log, this);
    }
    return true;
}

void Log::write_log(int level, const char* format, ...)
{
    if (m_close_log) return;

    struct timeval now;
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    char s[16] = "[info]:";
    switch (level) {
        case 0: strcpy(s, "[debug]:"); break;
        case 1: strcpy(s, "[info]:");  break;
        case 2: strcpy(s, "[warn]:");  break;
        case 3: strcpy(s, "[erro]:");  break;
        default: break;
    }

    {
        unique_lock<mutex> lock(m_mutex);
        m_count++;
        if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
            fflush(m_fp);
            fclose(m_fp);
            char tail[32] = {0};
            snprintf(tail, 32, "%d_%02d_%02d_",
                my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

            char new_log[256] = {0};
            if (m_today != my_tm.tm_mday) {
                snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
                m_today = my_tm.tm_mday;
                m_count = 0;
            } else {
                snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
            }
            m_fp = fopen(new_log, "a");
        }
    }

    va_list valst;
    va_start(valst, format);
    string log_str;

    {
        unique_lock<mutex> lock(m_mutex);
        int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
            my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
            my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

        int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);
        m_buf[n + m] = '\n';
        m_buf[n + m + 1] = '\0';
        log_str = m_buf;
    }
    va_end(valst);

    if (m_is_async) {
        unique_lock<mutex> lock(m_mutex);
        m_produce_buf->push_back(log_str);

        if (m_produce_buf->size() >= m_produce_buf->capacity()) {
            swap(m_produce_buf, m_consume_buf);
            m_cond.notify_one();
        }
    } else {
        unique_lock<mutex> lock(m_mutex);
        fputs(log_str.c_str(), m_fp);
    }
}

void Log::flush()
{
    unique_lock<mutex> lock(m_mutex);
    fflush(m_fp);
}

void Log::async_write_log()
{
    while (!m_exit) {
        unique_lock<mutex> lock(m_mutex);
        m_cond.wait(lock, [this] {
            return !m_consume_buf->empty() || m_exit;
        });

        if (m_exit && m_consume_buf->empty()) break;
        swap(m_produce_buf, m_consume_buf);
        lock.unlock();

        for (const string& str : *m_consume_buf) {
            unique_lock<mutex> lock(m_mutex);
            fputs(str.c_str(), m_fp);
        }
        m_consume_buf->clear();
    }
}
