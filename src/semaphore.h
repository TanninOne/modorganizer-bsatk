#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore {
public:
  Semaphore(int count)
    : m_Count(count)
  {
  }

  void post() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    ++m_Count;
    m_Condition.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    while (m_Count == 0)
      m_Condition.wait(lock);
    --m_Count;
  }

  bool try_wait() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (m_Count > 0) {
      --m_Count;
      return true;
    }
    return false;
  }

private:
  std::mutex m_Mutex;
  std::condition_variable m_Condition;
  unsigned long m_Count;
};
