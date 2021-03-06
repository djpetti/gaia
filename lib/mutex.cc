#include "mutex.h"

#include <assert.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "atomics.h"
#include "macros.h"

namespace tachyon {
namespace {

// Annoyingly, there is no Glibc wrapper for futex calls, so we have to make the
// syscalls manually.
// Args:
//  futex: The futex we are performing the operation on.
//  futex_op: The futex op we are performing.
//  val: Can mean different things, depending on the op. See the futex
//  documentation for details.
int FutexCall(Futex *futex, int futex_op, int val) {
  return syscall(SYS_futex, futex, futex_op, val, nullptr);
}

}  // namespace

bool FutexWait(Futex *futex, int expected) {
  const int futex_ret = FutexCall(futex, FUTEX_WAIT, expected);
  assert((!futex_ret || errno == EAGAIN) &&
         "futex(FUTEX_WAIT) failed unexpectedly.");
  _UNUSED(futex_ret);

  return !futex_ret;
}

int FutexWake(Futex *futex, int num_waiters) {
  const int futex_ret = FutexCall(futex, FUTEX_WAKE, num_waiters);
  assert(futex_ret >= 0 && "futex(FUTEX_WAKE) failed unexpectedly.");
  return futex_ret;
}

void MutexInit(Mutex *mutex) {
  mutex->state = 0;
}

void MutexGrab(Mutex *mutex) {
  Futex *state = &(mutex->state);

  if (!CompareExchange(state, 0, 1)) {
    // It wasn't zero, which means there's contention and we have to call into
    // the kernel.
    do {
      // We'll assume that the lock is still taken here, and try to set the
      // futex to 2 to indicate contention.
      if (*state == 2 || CompareExchange(state, 1, 2)) {
        // There's still contention. Wait in the kernel.
        FutexWait(state, 2);
      }
    } while (!CompareExchange(state, 0, 2));
    // Someone unlocking it sets it to zero, so we should only get here if we
    // successfully waited until someone unlocked the mutex and then grabbed
    // it.
  }
}

void MutexRelease(Mutex *mutex) {
  Futex *state = &(mutex->state);

  // If the lock is uncontended, this single atomic op is all we need to do to
  // release it.
  if (!CompareExchange(state, 1, 0)) {
    // It can only go up while this function is running, so if the above failed,
    // it must be 2, and we have to wake up someone.
    const int cas_ret = CompareExchange(state, 2, 0);
    assert(cas_ret && "Double-releasing lock?");
    _UNUSED(cas_ret);

    // Wake someone up.
    FutexWake(state, 1);
  }
}

}  // namespace tachyon
