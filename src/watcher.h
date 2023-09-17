#ifndef WATCHER_H
#define WATCHER_H

#include "minimal/minimal.h"

#define WATCHER_EVENT          32

#define WATCHER_BUFFER_SIZE  1024

#define WATCHER_ACTION_UNKOWN   0
#define WATCHER_ACTION_ADDED    1
#define WATCHER_ACTION_REMOVED  2
#define WATCHER_ACTION_MODIFIED 3

typedef struct Watcher Watcher;

typedef struct
{
    uint32_t action;
    char filename[WATCHER_BUFFER_SIZE];
    uint32_t name_len;
} WatcherEvent;

Watcher* watcherCreate(const char* path);
void watcherDestroy(Watcher* watcher);

void watcherPollEvents(MinimalApp* app, Watcher* watcher);

#endif // !WATCHER_H
