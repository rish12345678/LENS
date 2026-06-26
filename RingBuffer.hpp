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
    T *push_ptr_ = main_arr.data();
    alignas(std::hardware_destructive_interference_size) std::atomic<T *> pop_ptr_{main_arr.data()};
    alignas(std::hardware_destructive_interference_size) std::atomic<T *> push_ptr_{main_arr.data()};

    // Helper function to move the input pointer forward
    void advancePointer(const T *&ptr, const int ptr_type)
    {
        if (ptr_type == 1)
        { // push_ptr_
            if (ptr + 1 == base_ptr_ + capacity)
                ptr.store(base_ptr_);
            else
                ptr.store(ptr + 1);
        }
        else
        { // pop_ptr_
            
        }
    }

public:
    [[nodiscard]] bool is_full()
    {
        // Simply check if the pointer after push_ptr is pop_ptr
        T *next_push = push_ptr_;
        advancePointer(next_push);
        return next_push == pop_ptr_;
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
