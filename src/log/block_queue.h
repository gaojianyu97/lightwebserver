#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "./lock/locker.h"

using namespace std;
/**
 * @class block_queue
 * @brief block_queue模板类，能够容纳类型为T的元素的阻塞队列
*/
template <class T>
class block_queue{
private:
    locker m_mutex;//互斥锁
    cond m_cond;//条件变量

    T *m_array;//队列元素数组指针
    int m_size;//队列实际大小
    int m_max_size;//队列最大容量
    int m_front;//队列头元素位置
    int m_back;//队列尾元素位置

public:
    /**
     * @brief 构造函数
    */
    block_queue(int max_size = 1000){
        if(max_size<=0){
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }


    /**
     * @brief 清空队列
    */
    void clear(){
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    /**
     * @brief 析构函数，释放资源
    */
    ~block_queue(){
        m_mutex.lock();
        if(m_array!=NULL)
            delete[] m_array;

        m_mutex.unlock();
    }

    /**
     * @brief 判断队列是否为满
     * @return 返回是（true）否（false）满
    */
   bool full(){
        m_mutex.lock();
        if(m_size>=m_max_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
   }

    /**
     * @brief 判断队列是否为空
     * @return 返回是（true）否（false）空
    */
   bool empty(){
        m_mutex.lock();
        if(0==m_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
   }
   
    /**
     * @brief 获取队列队首元素
     * @return true:成功，false:失败
    */
   bool front(T &value){
        m_mutex.lock();
        if(0==m_size){
            m_mutex.unlock();
            return false;//队列为空，无队首元素，返回false
        }

        value = m_array[m_front];
        m_mutex.unlock();
        return true;//成功返回队首元素
   }

    /**
     * @brief 获取队列队尾元素
     * @return 
     * true:成功
     * false:失败
    */
   bool back(T &value){
        m_mutex.lock();
        if(0==m_size){
            m_mutex.unlock();
            return false;//队列为空，无队尾元素，返回false
        }

        value = m_array[m_back];
        m_mutex.unlock();
        return true;//成功返回队尾元素
   }

    /**
     * @brief 获取队列实际大小
     * @return 返回队列大小整型值
    */
   int size(){
        int tmp = 0;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
   }

    /**
     * @brief 获取队列的最大容量
     * @return 返回队列最大容量整型值
    */
   int max_size(){
        int tmp = 0;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
   }

    /**
     * @brief 往队列中添加元素，唤醒所有等待条件的线程
     * @return true:成功，false:失败
    */
   bool push(const T &item){
        m_mutex.lock();
        if(m_size>=m_max_size){
            m_cond.broadcast();
            m_mutex.unlock();
            return false;//线程队列已满，无法继续添加
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;//循环添加线程队尾元素
        m_size++;

        m_cond.broadcast();
        m_mutex.unlock();
        return true;
   }

    /**
     * @brief 从队列弹出元素
     * @return true:成功，false:失败
    */
   bool pop(T &item){
        m_mutex.lock();
        while(m_size<=0){
            if(!m_cond.wait(m_mutex.get())){
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;

        m_mutex.unlock();
        return true;
   }

    /**
     * @brief 从队列弹出元素，如果超时则失败
     * @return true:成功，false:失败
    */
   bool pop(T &item,int ms_timeout){
        struct timespec t = {0, 0};
        struct timespec now = {0, 0};
        gettimeofday(&now, NULL);
        m_mutex.lock();

        //在这个判断中，首先检查 m_size 是否小于等于 0。
        //如果 m_size 小于等于 0，说明队列当前为空，需要等待元素到来。
        //因此，使用 m_cond.timewait 函数等待条件变量，同时将超时时间 t 传递给它。
        //如果等待超时，则函数返回 false，否则继续执行。
        if(m_size<=0){
            t.tv_sec = now.tv_sec + ms_timeout / 1000;//计算秒
            t.tv_nsec = (ms_timeout % 1000) * 1000;//计算纳秒
            if(!m_cond.timewait(m_mutex.get(),t)){
                m_mutex.unlock();
                return false;
            }
        }
        //再次检查 m_size 是否小于等于 0。
        //这是因为，在等待条件变量之后，线程被唤醒后应该再次检查队列是否为空。
        //可能在等待期间其他线程已经修改了队列，使得它不再为空。如果队列仍为空，则直接返回 false。
        if(m_size<=0){
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
   }
};

#endif BLOCK_QUEUE_H