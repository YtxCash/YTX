#ifndef RESOURCEPOOL_H
#define RESOURCEPOOL_H

#include <QMutex>
#include <deque>

template <typename T>
concept Resettable = requires(T t) {
    { t.Reset() } -> std::same_as<void>;
};

template <typename Container>
concept Iterable = requires(Container c) {
    { c.begin() } -> std::same_as<typename Container::iterator>;
    { c.end() } -> std::same_as<typename Container::iterator>;
};

template <Resettable T> class ResourcePool {
public:
    static ResourcePool& Instance();
    T* Allocate();
    void Recycle(T* trans);
    template <Iterable Container> void Recycle(Container& trans_list);

private:
    ResourcePool();
    ~ResourcePool();

    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) = delete;
    ResourcePool& operator=(ResourcePool&&) = delete;

    void ExpandCapacity(int size);

private:
    std::deque<T*> pool_;
    QMutex mutex_;

    static constexpr qsizetype kSize { 100 };
    static constexpr qsizetype kExpandThreshold { 20 };
    static constexpr qsizetype kShrinkThreshold { 1001 };
};

template <Resettable T> ResourcePool<T>& ResourcePool<T>::Instance()
{
    static ResourcePool<T> instance;
    return instance;
}

template <Resettable T> T* ResourcePool<T>::Allocate()
{
    QMutexLocker locker(&mutex_);

    if (pool_.size() <= kExpandThreshold)
        ExpandCapacity(kSize);

    if (pool_.empty()) {
        return new T();
    }

    T* trans { pool_.front() };
    pool_.pop_front();
    return trans;
}

template <Resettable T> void ResourcePool<T>::Recycle(T* trans)
{
    if (!trans)
        return;

    if (pool_.size() >= kShrinkThreshold) {
        delete trans;
        return;
    }

    QMutexLocker locker(&mutex_);
    trans->Reset();
    pool_.push_back(trans);
}

template <Resettable T> template <Iterable Container> void ResourcePool<T>::Recycle(Container& container)
{
    if (container.isEmpty())
        return;

    if (pool_.size() + container.size() >= kShrinkThreshold) {
        qDeleteAll(container);
    } else {
        QMutexLocker locker(&mutex_);

        for (T* trans : container) {
            if (trans) {
                trans->Reset();
                pool_.push_back(trans);
            }
        }
    }

    container.clear();
}

template <Resettable T> ResourcePool<T>::ResourcePool() { ExpandCapacity(kSize); }

template <Resettable T> void ResourcePool<T>::ExpandCapacity(int size)
{
    for (int i = 0; i < size; ++i) {
        pool_.push_back(new T());
    }
}

template <Resettable T> ResourcePool<T>::~ResourcePool()
{
    QMutexLocker locker(&mutex_);
    qDeleteAll(pool_);
    pool_.clear();
}

#endif // RESOURCEPOOL_H
