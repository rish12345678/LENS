# Design Decisions

## What to return from RingBuffer's try_pop() member function

Deciding what to return from the try_pop() function in the lock-free queue is a significant design decision especially when the goal is minimal latency, in which a bad design decision here could negate the latency advantage that the lock-free queue itself is aiming to provide.

There were several ideas I had on what the return type should be. The first one I thought of, and certainly most naive was returning T, the type of elements in the queue. This is extremely innefficient as it requires a copying of the element of type T, which would causes a horrible tax on performance for large elements.

A second idea would be to return T\* a pointer directly to the element in the queue. Although this sounds good for performance, only returning eight bytes regardless of the size of T, it creates a terrible correctness issue, where within the cycles from getting the pointer to processing it, or using it for something, the queue could loop back around with the push*ptr*, updating the value, mid or pre read. Correctness must be a guarantee so this is not a viable return type.

Lastly, and most efficiently, we can simply move the value into a returnable std::optional of type T so that we can return T or null correctly in efficient time.

## Costless Simplicity of the RingBuffer's Advance Pointer Helper Method

Advance pointer is a simple private helper method in the RingBuffer class that takes in a pointer by reference and increments it accounting for the buffer's wraparound.
