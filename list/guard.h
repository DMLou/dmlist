/*****************************************************************************

    guard.h

    Function guarding to generate a stack trace at runtime, especially in
    Release builds.

*****************************************************************************/

#ifndef _GUARD_H
#define _GUARD_H

#include <stdexcept>
#include <string>
#include <ostream>

class stack_trace : public std::exception
{
public:
    stack_trace (const char *message = "stack trace:\n")
        : std::exception (message)
    {
        return;
    }
};

//#define ENABLE_GUARDING

#ifndef ENABLE_GUARDING

// In debug builds, have throw statements immediately cause a breakpoint so
// that we can jump into the debugger!
#ifdef NDEBUG
    #define guard_throw(x) throw x
#else
    #define guard_throw(x) __asm int 3
#endif

    #define guard_named(NAME) {
    #define guard {
    #define guard_rethrow(type)
    #define unguard }

#else

    #define guard_throw(x) throw x

    #define guard_named(NAME)                                                 \
    {                                                                         \
        static const char *__GUARD_BLOCK_NAME = NAME;                         \
                                                                              \
        try

    #define guard guard_named(__FUNCTION__)

    #define guard_rethrow(type)                                               \
        catch (type ex)                                                       \
        {                                                                     \
            throw ex;                                                         \
        }                                                                    

    #define unguard                                                           \
        catch (stack_trace &st)                                               \
        {                                                                     \
            throw stack_trace ((std::string(st.what())                        \
                              + std::string (" <- ")                          \
                              + std::string (__GUARD_BLOCK_NAME)).c_str());   \
        }                                                                     \
                                                                              \
        catch (exception &ex)                                                 \
        {                                                                     \
            throw stack_trace ((std::string (ex.what())                       \
                              + std::string ("\nstack trace:\n")              \
                              + std::string (__GUARD_BLOCK_NAME)).c_str());   \
        }                                                                     \
                                                                              \
        catch (...)                                                           \
        {                                                                     \
            throw stack_trace ((std::string ("unhandled C++ exception\n")     \
                              + std::string ("stack trace:\n")                \
                              + std::string (__GUARD_BLOCK_NAME)).c_str());   \
        }                                                                     \
    }                                                                    

#endif // ENABLE_GUARDING

#endif // _GUARD_H