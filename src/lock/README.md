# 锁的说明

以下是对每个函数的中文说明以及其实现过程的解释：

## sem 类（信号量）

- `sem()`：构造函数，初始化信号量，初始值为0。通过调用 `sem_init()` 函数来实现初始化，如果初始化失败，则抛出异常。

- `sem(int num)`：构造函数，初始化信号量，初始值为 `num`。同样使用 `sem_init()` 来进行初始化，失败时抛出异常。

- `~sem()`：析构函数，自动销毁信号量。使用 `sem_destroy()` 函数来销毁信号量。

- `bool wait()`：等待信号量，即尝试获取信号量。使用 `sem_wait()` 函数，如果成功获取信号量返回 `true`，否则返回 `false`。

- `bool post()`：发送信号量，即释放信号量。使用 `sem_post()` 函数，如果成功释放信号量返回 `true`，否则返回 `false`。

## locker 类（互斥锁）

- `locker()`：构造函数，初始化互斥锁。使用 `pthread_mutex_init()` 函数来初始化互斥锁，如果失败则抛出异常。

- `~locker()`：析构函数，自动销毁互斥锁。使用 `pthread_mutex_destroy()` 函数来销毁互斥锁。

- `bool lock()`：获取互斥锁。使用 `pthread_mutex_lock()` 函数，如果成功获取锁返回 `true`，否则返回 `false`。

- `bool unlock()`：释放互斥锁。使用 `pthread_mutex_unlock()` 函数，如果成功释放锁返回 `true`，否则返回 `false`。

- `pthread_mutex_t *get()`：获取互斥锁的指针，允许直接访问底层的互斥锁对象。

## cond 类（条件变量）

- `cond()`：构造函数，初始化条件变量。使用 `pthread_cond_init()` 函数来初始化条件变量，如果失败则抛出异常。

- `~cond()`：析构函数，自动销毁条件变量。使用 `pthread_cond_destroy()` 函数来销毁条件变量。

- `bool wait(pthread_mutex_t *m_mutex)`：等待条件变量完成，需要传入一个互斥锁的指针 `m_mutex`。使用 `pthread_cond_wait()` 函数，在等待期间会释放互斥锁，待条件满足后重新获取锁。

- `bool timewait(pthread_mutex_t *m_mutex, struct timespec t)`：在指定的等待时间内等待条件变量完成，同样需要传入互斥锁的指针和等待时间。使用 `pthread_cond_timedwait()` 函数，允许在一段时间内等待条件满足。

- `bool signal()`：发送一个信号给等待条件变量上的线程，唤醒一个等待的线程。使用 `pthread_cond_signal()` 函数。

- `bool broadcast()`：广播等待在条件变量上的所有线程，唤醒所有等待的线程。使用 `pthread_cond_broadcast()` 函数。

这些类用于多线程程序中管理信号量、互斥锁和条件变量，确保线程的协同工作和同步。通过使用这些类，可以更容易地编写多线程程序，确保线程安全。