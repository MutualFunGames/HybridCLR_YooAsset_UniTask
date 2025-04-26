#if defined(__ANDROID__)

#include <signal.h>

extern int __real_sigaction(int signum, const struct sigaction *action, struct sigaction *old_action);

// Workaround a bug on Krait CPUs where a signal handler would not properly transition from thumb to non-thumb.
// Note: The workaround has a flaw if multiple signal handlers are used, then the last sigaction call overwrites wrapped_signal_handlers value.
//       Since it's only needed for ARM 32 bit, don't perform this workaround on other architectures
// Note: Previously this workaround (when used for all CPU architectures) caused a test failure in unity repo:
//        - Tests\Unity.IntegrationTests\Android\AndroidCrashHandler.cs
//       Since the test was using multiple signal handlers to validate that handlers are correcty invoked when registered before and after Unity runtime initialization
#if defined(__arm__)
    #define SIGMAX 64
    static void (*wrapped_signal_handlers[SIGMAX]) (int, siginfo_t *, void *);

    static void signal_handler(int signum, siginfo_t* siginfo, void* sigcontext)
    {
      if (wrapped_signal_handlers[signum])
        wrapped_signal_handlers[signum](signum, siginfo, sigcontext);
    }

    __attribute__((used)) int __wrap_sigaction(int signum, const struct sigaction *action, struct sigaction *old_action)
    {
      struct sigaction  wrapper_action_data;
      struct sigaction* wrapper_action = NULL;

      if (signum >= SIGMAX)
        return __real_sigaction(signum, action, old_action);

      // patch sig action with our thumb compiled dispatcher
      if (action)
      {
        wrapper_action = &wrapper_action_data;
        memcpy(wrapper_action, action, sizeof(*action));
        wrapper_action->sa_sigaction = signal_handler;

      }

      // install handler (abort on error)
      if (__real_sigaction(signum, wrapper_action, old_action) == -1)
        return -1;

      // hide any previously installed wrapper
      if (old_action && old_action->sa_sigaction == signal_handler)
        old_action->sa_sigaction = wrapped_signal_handlers[signum];

      // add action to dispatch table
      if (action)
        wrapped_signal_handlers[signum] = action->sa_sigaction;

      return 0;
    }

    #undef SIGMAX
#else
    __attribute__((used)) int __wrap_sigaction(int signum, const struct sigaction *action, struct sigaction *old_action)
    {
        return __real_sigaction(signum, action, old_action);
    }
#endif

#endif /* __ANDROID__ */