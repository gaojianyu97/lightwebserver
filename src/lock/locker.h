#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

/**
 * @brief sem类，用于封装信号量的操作,实现信号量的初始化、自动销毁、等待和发送
*/
class sem{
public:
    /**
     * @brief 初始化信号量，初始值为0
     * @throws std::exception()
    */
    sem(){
        if(sem_init(&m_sem,0,0)!=0){
            throw std::exception();
        }
    }

    /**
     * @brief 初始化信号量，初始值为num，失败则抛出异常
     * @param num:初始化信号量的值
     * @throws std::exception()
    */
    sem(int num){
        if(sem_init(&m_sem,0,num)!=0){
            throw std::exception();
        }
    }

    /**
     * @brief 析构函数，自动销毁信号量
    */
    ~sem(){
        sem_destroy(&m_sem);
    }

    /**
     * @brief 等待信号量
     * @return 成功：true，失败：false
    */
    bool wait(){
        return sem_wait(&m_sem) == 0;
    }

    /**
     * @brief 发送信号量
     * @return 成功：true，失败：false
    */
    bool post(){
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;//信号量
};

/**
 * @brief locker类，实现互斥锁的初始化、销毁、上锁和解锁
*/
class locker{
private:
    pthread_mutex_t m_mutex;//互斥锁

public:

    /**
     * @brief 初始化互斥锁
     * @throws std::exception()
    */
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw std::exception();
        }
    }

    /**
     * @brief 析构函数，自动销毁锁
    */
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    /**
     * @brief 获取互斥锁
     * @return 成功：true，失败：false
    */
    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    /**
     * @brief 释放互斥锁
     * @return 成功：true，失败：false
    */
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    /**
     * @brief 获取互斥锁的指针
    */
    pthread_mutex_t *get(){
        return &m_mutex;
    }
};

/**
 * @brief cond类，实现条件变量的初始化、销毁、等待及线程的唤醒
*/
class cond{
private:
    pthread_cond_t m_cond;

public:

    /**
     * @brief 条件变量初始化
     * @throw std::exception()
    */
    cond(){
        if(pthread_cond_init(&m_cond,NULL)!=0){
            throw std::exception();
        }
    }

    /**
     * @brief 条件变量销毁
    */
    ~cond(){
        pthread_cond_destroy(&m_cond);
    }

    /**
     * @brief 等待响应线程下的条件变量完成
     * @return 成功：true，失败：false
    */
    bool wait(pthread_mutex_t *m_mutex){
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }

    /**
     * @brief 在指定的等待时间t内等待线程m_mutex的条件变量完成
     * @return 成功：true，失败：false
    */
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t){
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }

    /**
     * @brief 发送一个信号给等待条件变量上的线程，唤醒线程
    */
    bool signal(){
        return pthread_cond_signal(&m_cond);
    }

    /**
     * @brief 广播等待在条件变量上的所有线程，唤醒所有线程
    */
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond);
    }
};
#endif