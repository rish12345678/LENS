#pragma once
#include <iostream>
#include <array>
#include <new>
#include <optional>
#include <atomic>

template <typename T>
class RingBuffer
{
private:
    // We will keep capacity const 63 for
    // now, sacrificing one slot for
    // easy full capacity check
    const int capacity = 64;
    // buffer array and pointers for producer and consumer
    std::array<T, 64> main_arr{{}};
    T *base_ptr_ = main_arr.data();
    alignas(std::hardware_destructive_interference_size) std::atomic<T *> pop_ptr_{main_arr.data()};
    alignas(std::hardware_destructive_interference_size) std::atomic<T *> push_ptr_{main_arr.data()};

    // Helper function to return the address of the next
    // location the pointer should move to, given its current address
    [[nodiscard]] inline T *getNextPointer(T *current_ptr) const
    {
        if (current_ptr + 1 == base_ptr_ + capacity)
            return base_ptr_;
        else
            return current_ptr + 1;
    }

public:
    [[nodiscard]] bool is_full()
    {
        // Use acquire to see where consumer's pop_ptr_ is
        T *next_push = getNextPointer(push_ptr_.load(std::memory_order_relaxed));
        return next_push == pop_ptr_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool is_empty()
    {
        return push_ptr_ == pop_ptr_;
    }

    // Tries to push to the queue, will return true if complete, false if
    // call fails due to capacity reached
    bool try_push(const T &push_val) // We are passing a const ref of T, so this packet is being passed around
    {
        if (this->is_full())
            return false;

        // Critical section
        (*push_ptr_) = push_val; // Place input T in queue

        advancePointer(push_ptr_, 1);
        // Section Ends

        // Return Status
        return true;
    }

    // Tries to pop from the queue, will return T instance if complete,
    // std::nullopt if call fails due to queue being empty

    std::optional<T> try_pop()
    {
        if (this->is_empty())
            return std::nullopt;

        // Critical section
        T ret_T = std::move(*pop_ptr_);

        advancePointer(pop_ptr_);
        // Section Ends

        // NRVO: one move, no copies
        return ret_T;
    }
};
