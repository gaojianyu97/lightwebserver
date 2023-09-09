# 线程池的说明

## 整体框架

![线程池整体框架](https://pic4.zhimg.com/80/v2-ab874df7219895195def55a02fb390f7_1440w.webp)

## 线程池接口及参数

```cpp
public:
    /**
     * @param actor_model 模型切换类型
     * @param conn_pool 数据库连接池
     * @param thread_number 线程池中线程的数量
     * @param max_request 请求队列中最多允许的等待处理的请求的数量
     */
    threadpool(int actor_model, connection_pool *conn_pool, int thread_number = 8, int max_request = 10000);
    ~threadpool();

    /**
     * @brief 向请求队列添加请求
     * @param request 请求指针
     * @param state 请求状态
     * @return 添加是否成功
     */
    bool append(T *request, int state);

    /**
     * @brief 向请求队列添加请求（无状态）
     * @param request 请求指针
     * @return 添加是否成功
     */
    bool append_p(T *request);

```


```cpp
private:
    /**
     * @brief 工作线程运行的函数，不断从工作队列中取出任务并执行
     * @param arg 线程池指针
     * @return 线程池指针
     */
    static void *worker(void *arg);
    /**
     * @brief 线程工作函数，执行任务处理
    */
    void run();

```

```cpp
private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    connection_pool *m_connPool;//数据库
    int m_actor_model;          //模型切换
```

函数实现

### 构造函数

构造函数（Constructor）是C++类中的特殊成员函数，它在对象被创建时自动调用，用于初始化对象的各个成员变量和执行其他必要的初始化工作。

```cpp
template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_connPool(connPool)
{
    // 构造函数的实现内容
}
```

关键步骤：

1. **参数初始化**：构造函数的参数会传递给它，然后用于初始化类的成员变量。在这里，构造函数接受了四个参数：`actor_model`、`connPool`、`thread_number`、`max_requests`。这些参数用于初始化类的成员变量 `m_actor_model`、`m_thread_number`、`m_max_requests`、`m_threads` 和 `m_connPool`。

2. **动态内存分配**：在构造函数中，你会看到 `m_threads` 是一个指向 `pthread_t` 数组的指针。构造函数中使用 `new` 运算符为这个数组动态分配内存，并将其地址赋值给 `m_threads`。这是为了创建线程池中的线程。

3. **线程的创建**：接下来，在一个循环中，构造函数使用 `pthread_create` 函数创建多个线程。这些线程的数量由 `thread_number` 决定。每个线程将执行 `worker` 函数，同时传递 `this` 指针，以便在工作线程中能够访问线程池的成员函数和成员变量。

4. **线程的分离**：每个线程创建后，通过 `pthread_detach` 函数将其设置为分离状态，线程结束时将自动释放其资源，无需主线程等待它们退出。

### 析构函数

析构函数（Destructor）是C++类中的特殊成员函数，用于在对象被销毁时执行清理工作和释放资源。


```cpp
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}
```

一个主要的步骤：

1. **释放内存**：析构函数的主要任务是释放在构造函数中动态分配的内存。在这里，你可以看到 `delete[] m_threads;`，它用于释放先前由构造函数动态分配的 `pthread_t` 数组的内存。这个数组存储了线程池中的线程。

当销毁（或删除）线程池对象时，C++会自动调用析构函数。在这个析构函数中，通过 `delete[]` 运算符释放了线程数组的内存，确保不会发生内存泄漏。

总之，析构函数的主要目的是在对象被销毁时释放分配的资源，以确保不会出现资源泄漏问题，释放了线程数组的内存，以便在对象生命周期结束时执行必要的清理操作。


### append()和append_p()

`append` 和 `append_p` 方法是线程池类中用于添加请求到请求队列的两个方法。下面用中文解释这两个方法的实现流程：

1. **`append` 方法**：
   ```cpp
   template <typename T>
   bool threadpool<T>::append(T *request, int state)
   {
       m_queuelocker.lock();
       if (m_workqueue.size() >= m_max_requests)
       {
           m_queuelocker.unlock();
           return false;
       }
       request->m_state = state;
       m_workqueue.push_back(request);
       m_queuelocker.unlock();
       m_queuestat.post();
       return true;
   }
   ```

   - `append` 方法用于向线程池的请求队列添加带有状态的请求。
   - 首先，它通过 `m_queuelocker.lock()` 获取对请求队列的互斥锁，确保线程安全。
   - 接着，它检查请求队列是否已满，如果队列已满，则解锁互斥锁并返回 `false`，表示添加失败。
   - 如果队列未满，它将请求的状态 `state` 赋值给请求对象，并将请求对象添加到请求队列的末尾。
   - 最后，它通过 `m_queuestat.post()` 通知等待中的工作线程有新任务可处理，并返回 `true` 表示添加成功。

2. **`append_p` 方法**：
   ```cpp
   template <typename T>
   bool threadpool<T>::append_p(T *request)
   {
       m_queuelocker.lock();
       if (m_workqueue.size() >= m_max_requests)
       {
           m_queuelocker.unlock();
           return false;
       }
       m_workqueue.push_back(request);
       m_queuelocker.unlock();
       m_queuestat.post();
       return true;
   }
   ```

   - `append_p` 方法用于向线程池的请求队列添加不带状态的请求。
   - 类似于 `append` 方法，它首先获取请求队列的互斥锁，确保线程安全。
   - 然后，它检查请求队列是否已满，如果队列已满，则解锁互斥锁并返回 `false`，表示添加失败。
   - 如果队列未满，它将请求对象添加到请求队列的末尾。
   - 最后，它通过 `m_queuestat.post()` 通知等待中的工作线程有新任务可处理，并返回 `true` 表示添加成功。

这两个方法的主要作用是将请求添加到线程池的请求队列中，以便工作线程可以异步地处理这些请求。`append` 方法用于带有状态的请求，而 `append_p` 方法用于不带状态的请求。在添加请求之前，它们都会检查队列是否已满以避免队列溢出。

### worker()

Worker （工作函数）是线程池中工作线程的执行函数，用于不断从请求队列中取出任务并执行。

```cpp
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
```

这个 `worker` 函数的主要任务是执行线程的工作。

1. `void *threadpool<T>::worker(void *arg)`：这是 `worker` 函数的定义，它是一个静态成员函数，因此可以通过类名调用，而不需要创建类的实例。

2. `threadpool *pool = (threadpool *)arg;`：在函数内部，首先将传递给 `worker` 函数的参数 `arg` 转换为 `threadpool` 类型的指针，并将其赋值给 `pool`。这是因为 `worker` 函数需要访问线程池的成员函数和成员变量。

3. `pool->run();`：接下来，调用 `pool` 指针所指向的线程池对象的 `run` 方法。这个方法是线程池中每个工作线程实际执行的主要逻辑。

4. `return pool;`：最后，函数返回 `pool` 指针，这是为了在线程结束时能够让调用者知道哪个线程已经完成了工作。但需要注意的是，这个返回值通常不会被使用，因为工作线程被设置为分离状态，它们的返回值不会被主线程获取。

总结来说，`worker` 函数是线程池中工作线程的执行函数，它通过调用线程池对象的 `run` 方法来执行实际的任务处理逻辑。在多线程环境下，多个工作线程可以并行地执行 `run` 方法，从请求队列中取出任务并处理。这样可以有效地利用多核处理器和提高任务处理的并发性。

### run()

`run` 方法是线程池中工作线程实际执行的主要逻辑。

1. `while (true)` 循环：`run` 方法是一个无限循环，工作线程会一直运行，等待任务并处理它们。

2. `m_queuestat.wait()`：使用信号量 `m_queuestat` 来等待是否有任务需要处理。如果没有任务，线程将阻塞在这里，等待信号量的通知。

3. `m_queuelocker.lock()`：获取请求队列的互斥锁 `m_queuelocker`，以确保对请求队列的访问是线程安全的。

4. `if (m_workqueue.empty())`：检查请求队列是否为空。如果队列为空，表示没有任务需要处理，线程将释放互斥锁并继续等待。

5. 从请求队列中取出任务：如果队列不为空，线程从队列的前端（使用 `front()` 函数）取出第一个请求，并从队列中移除它（使用 `pop_front()` 函数）。

6. 互斥锁释放：在获取到任务后，线程释放请求队列的互斥锁，允许其他线程访问队列。

7. 执行任务：根据线程池的模型类型 `m_actor_model`，线程执行不同的任务处理逻辑。如果是模型1，根据请求的状态 `m_state` 执行读取或写入操作，处理数据库连接等。如果不是模型1，直接执行 `request->process()`。

8. 重复：线程完成一个任务后，回到循环的开头，等待下一个任务的到来。这个过程一直循环执行，确保线程池中的工作线程能够处理队列中的所有任务。

总结来说，`run` 方法是线程池中工作线程的核心逻辑，它等待任务的到来，从请求队列中取出任务，并根据任务类型执行不同的处理逻辑。这使得线程池能够高效地处理大量的并发任务。

