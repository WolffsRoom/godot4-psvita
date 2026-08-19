#ifndef VITA_COREDUMP_SYNCTHREAD_H
#define VITA_COREDUMP_SYNCTHREAD_H

#include <cstdint>

namespace vita
{
namespace coredump
{

typedef uint32_t ultSyncThread;

typedef struct  {
   uint32_t pid;
   uint32_t thread_id;
} syncObjectThread;

}; // coredump
}; // vita

#endif