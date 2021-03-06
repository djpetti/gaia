#ifndef TACHYON_LIB_IPC_CONSTANTS_H_
#define TACHYON_LIB_IPC_CONSTANTS_H_

#include <stdint.h>

// Contains constants that are used by various files in one place for easy
// modification.

namespace tachyon {

// Name of the shared memory block.
extern const char *kShmName;
// We allocate portions of SHM in blocks. This block size should be chosen to
// balance overhead with wasted space, and ideally the page size should be
// an integer multiple of this number.
constexpr uint32_t kBlockSize = 128;

// How many items we want our queues to be able to hold.
static constexpr int kQueueCapacity = 64;
// Size to use when initializing the underlying pool.
// TODO (danielp): This will have to be increased eventually for actual tachyon
// stuff.
static constexpr int kPoolSize = 64000;
// The maximum number of consumers a queue can have.
static constexpr int kMaxConsumers = 64;

// Number of buckets in the hashmap that stores queue names.
static constexpr int kNameMapSize = 128;
// Byte offset in shared memory where the hashmap that stores queue names is
// located.
static constexpr int kNameMapOffset = 0;

}  // namespace tachyon

#endif // TACHYON_LIB_IPC_CONSTANTS_H_
