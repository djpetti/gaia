#ifndef GAIA_LIB_INTERNAL_QUEUE_H_
#define GAIA_LIB_INTERNAL_QUEUE_H_

#include <assert.h>
#include <stdint.h>

#include <limits>

#include "atomics.h"
#include "constants.h"
#include "pool.h"

namespace gaia {
namespace internal {

// MPSC: "Multi-producer, single consumer."
// Should be pretty self-explanatory...
//
// At the low level, an MpscQueue is basically just a vector with the nodes
// allocated in shared memory. Obviously, it's completely thread/process-safe.
// Template specialization is used to make Queues that can handle arbitrary
// types.
//
// NOTE: This should never be used directly!!! It's only there as a helper class
// for standard Queues. This is because it's designed for only a single
// consumer.
//
// Non-blocking operations on this queue are, of course, lock free and suitable
// for realtime applications.
template <class T>
class MpscQueue {
 public:
  MpscQueue();
  // Constructor that makes a new queue but uses a pool that we pass in.
  // Args:
  //  pool: The pool to use.
  explicit MpscQueue(Pool *pool);
  // A similar contructor that fetches a queue stored at a particular location
  // in shared memory. Used internally by FetchQueue.
  // Args:
  //  queue_offset: The byte offset in the shared memory block of the underlying
  //  RawQueue object.
  explicit MpscQueue(int queue_offset);
  ~MpscQueue();

  // Adds a new element to the queue, without blocking. It is lock-free, and
  // stays in userspace.
  // Args:
  //  item: The item to add to the queue.
  // Returns:
  //  True if it succeeded in adding the item, false if the queue was
  //  full already.
  bool Enqueue(const T &item);
  // Removes an element from the queue, without blocking. It is lock-free, and
  // stays in userspace.
  // Args:
  //  item: A place to copy the item.
  // Returns:
  //  True if it succeeded in getting an item, false if the queue was empty
  //  already.
  bool DequeueNext(T *item);

  // Gets the offset of the shared part of the queue in the shared memory pool.
  // Returns:
  //  The offset.
  int GetOffset();

 private:
  // Represents an item in the queue.
  struct Node {
    // The actual item we want to store.
    volatile T value;
    // A flag denoting whether this node contains valid data.
    volatile uint32_t valid;
  };

  // This is the underlying structure that will be located in shared memory, and
  // contain everything that the queue needs to store in SHM. Multiple Queue
  // classes can share one of these, and they will be different "handles" into
  // the same queue.
  struct RawQueue {
    // The underlying array.
    volatile Node array[kQueueCapacity];
    // Total length of the queue visible to writers.
    volatile int32_t write_length;
    // Current index of the head.
    volatile int32_t head_index;
  };

  // Whether we own our pool or not.
  bool own_pool_ = false;

  // For consumers, we can get away with storing the tail index locally since we
  // only have one.
  uint32_t tail_index_ = 0;

  RawQueue *queue_;
  // This is the shared memory pool that we will use to construct queue objects.
  Pool *pool_;
};

#include "mpsc_queue_impl.h"

}  // namespace internal
}  // namespace gaia

#endif  // GAIA_LIB_INTERNAL_QUEUE_H_