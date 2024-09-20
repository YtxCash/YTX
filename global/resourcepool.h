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
    void Recycle(T* resource);
    template <Iterable Container> void Recycle(Container& resource_list);

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

    T* resource { pool_.front() };
    pool_.pop_front();
    return resource;
}

template <Resettable T> void ResourcePool<T>::Recycle(T* resource)
{
    if (!resource)
        return;

    if (pool_.size() >= kShrinkThreshold) {
        delete resource;
        return;
    }

    QMutexLocker locker(&mutex_);
    resource->Reset();
    pool_.push_back(resource);
}

template <Resettable T> template <Iterable Container> void ResourcePool<T>::Recycle(Container& container)
{
    if (container.isEmpty())
        return;

    if (pool_.size() + container.size() >= kShrinkThreshold) {
        qDeleteAll(container);
    } else {
        QMutexLocker locker(&mutex_);

        for (T* resource : container) {
            if (resource) {
                resource->Reset();
                pool_.push_back(resource);
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
