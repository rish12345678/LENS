# Design Decisions

## What to return from RingBuffer's try_pop() member function

Deciding what to return from the try_pop() function in the lock-free queue is a significant design decision especially when the goal is minimal latency, in which a bad design decision here could negate the latency advantage that the lock-free queue itself is aiming to provide.

There were several ideas I had on what the return type should be. The first one I thought of, and certainly most naive was returning T, the type of elements in the queue. This is extremely innefficient as it requires a copying of the element of type T, which would causes a horrible tax on performance for large elements.

A second idea would be to return T\* a pointer directly to the element in the queue. Although this sounds good for performance, only returning eight bytes regardless of the size of T, it creates a terrible correctness issue, where within the cycles from getting the pointer to processing it, or using it for something, the queue could loop back around with the push*ptr*, updating the value, mid or pre read. Correctness must be a guarantee so this is not a viable return type.

Lastly, and most efficiently, we can simply move the value into a returnable std::optional of type T so that we can return T or null correctly in efficient time.

## Costless Simplicity of the RingBuffer's Advance Pointer Helper Method

Advance pointer is a simple private helper method in the RingBuffer class that takes in a T pointer by reference and increments it accounting for the buffer's wraparound. This helper method is extremely efficient for three reasons.

1. The function is extremely useful in that we can use it to do a simple pointer incrementation that makes the ring buffer seem like a basic array with no wraparound, keeping the code in other functions small and easy to read.
2. Due to the simplicity and length of the function, compiler optimizations can actually inline the function, killing function call overhead, simply executing the corresponding set of instructions in place of the function call.
3. The creation of the extraneous T\* is created on the stack and potentially with compiler optimizations will just be loaded into a register causing essentially zero perfromance hit.

## How To Get The RingBuffer To Single-Threaded to SPSC Thread-Safe

We face two problems that cause our base RingBuffer impl to fall short when operated on multiple threads, one for the producer and one for the consumer:

1. When the producer thread advances its position, that location update is only set in its local cache first, and only later updated in the consumer core's cache, so the producer might be way ahead of the consumer, and the consumer doesn't even know so it doesn't get waken up yet.

2. The compiler's instruction reordering to optimize performance, can cause citical sections to execute out of order, such as pushing the pointer forward and then updating the value at its previous location, even though we designed it specifically not to behave that way, as it could cause the consumer to wake up and read the nonsense value in that location that hasn't been updated yet.

One is a performance issue of the consumer not seeing updates quickly enough and falling behind, the other is a correctness issue of the consumer interleaving its reads with the producer's out of order execution of its writes.

## Change From Concurrnecy Computation Heavy advancePointer() method to purely computational getNextPtr() method

Instead of advancing the pointer in the method, and taking care of memory ordering / thread safety there, where we also need to pass in a flag for the type fo the pointer, we make make a simpler inline function that is not too heavy and allows users to do a simple next address retrieval, and then handle the write and read pointer location updating inside the method dedicates for each.
