#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

namespace mafia {

namespace detail {

// Один блок на объект. Счётчик атомарный: копии SharedPtr живут в разных потоках.
struct ControlBlock {
    std::atomic<int> strongCount{1};
};

}  // namespace detail

// Несколько SharedPtr владеют одним объектом в куче.
// Объект удаляется, когда strongCount последнего владельца падает до нуля.
template <typename T>
class SharedPtr {
public:
    SharedPtr() noexcept = default;

    SharedPtr(std::nullptr_t) noexcept {}

    explicit SharedPtr(T* ptr) : ptr_(ptr) {
        if (ptr_ != nullptr) {
            block_ = new detail::ControlBlock();
        }
    }

    SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        retain();
    }

    SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        SharedPtr copy(other);
        swap(copy);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        SharedPtr moved(std::move(other));
        swap(moved);
        return *this;
    }

    ~SharedPtr() { release(); }

    // Отпустить текущий объект. Если ptr не пуст, взять его со счётчиком 1.
    void reset(T* ptr = nullptr) {
        SharedPtr next(ptr);
        swap(next);
    }

    void swap(SharedPtr& other) noexcept {
        T* tmpPtr = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmpPtr;

        detail::ControlBlock* tmpBlock = block_;
        block_ = other.block_;
        other.block_ = tmpBlock;
    }

    T* get() const noexcept { return ptr_; }

    T& operator*() const { return *ptr_; }

    T* operator->() const noexcept { return ptr_; }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    friend bool operator==(const SharedPtr& lhs, const SharedPtr& rhs) noexcept {
        return lhs.get() == rhs.get();
    }

    friend bool operator!=(const SharedPtr& lhs, const SharedPtr& rhs) noexcept {
        return !(lhs == rhs);
    }

    friend bool operator==(const SharedPtr& lhs, std::nullptr_t) noexcept {
        return lhs.get() == nullptr;
    }

    friend bool operator==(std::nullptr_t, const SharedPtr& rhs) noexcept {
        return rhs.get() == nullptr;
    }

    friend bool operator!=(const SharedPtr& lhs, std::nullptr_t) noexcept {
        return lhs.get() != nullptr;
    }

    friend bool operator!=(std::nullptr_t, const SharedPtr& rhs) noexcept {
        return rhs.get() != nullptr;
    }

private:
    void retain() noexcept {
        if (block_ != nullptr) {
            block_->strongCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void release() noexcept {
        if (block_ == nullptr) {
            return;
        }
        // fetch_sub возвращает прошлое значение. 1 значит, что мы последние.
        if (block_->strongCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr_;
            delete block_;
        }
        ptr_ = nullptr;
        block_ = nullptr;
    }

    T* ptr_ = nullptr;
    detail::ControlBlock* block_ = nullptr;
};

}  // namespace mafia
