#include "watcher.h"

#include <Windows.h>
#include <stdint.h>
#include <wchar.h>
#include <stdlib.h>

struct Watcher
{
    HANDLE handle;
    OVERLAPPED overlapped;

    uint8_t change_buffer[WATCHER_BUFFER_SIZE];
};

static BOOL watcherReadDirChanges(Watcher* watcher)
{
    return ReadDirectoryChangesW(watcher->handle, watcher->change_buffer, WATCHER_BUFFER_SIZE, TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, &watcher->overlapped, NULL);
}


Watcher* watcherCreate(const char* path)
{
    Watcher* watcher = malloc(sizeof(Watcher));
    if (!watcher) return NULL;

    watcher->handle = CreateFileA(path, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    if (watcher->handle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            MINIMAL_ERROR("[Watcher] File %s not found", path);
        else
            MINIMAL_ERROR("[Watcher] Failed to open file %s", path);
    }

    watcher->overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

    watcherReadDirChanges(watcher);

    return watcher;
}

void watcherDestroy(Watcher* watcher)
{
    free(watcher);
}

static uint32_t watcher_actions_map[] = {
    [FILE_ACTION_ADDED]             = WATCHER_ACTION_ADDED,
    [FILE_ACTION_REMOVED]           = WATCHER_ACTION_REMOVED,
    [FILE_ACTION_MODIFIED]          = WATCHER_ACTION_MODIFIED,
    [FILE_ACTION_RENAMED_OLD_NAME]  = WATCHER_ACTION_UNKOWN,
    [FILE_ACTION_RENAMED_NEW_NAME]  = WATCHER_ACTION_UNKOWN,
};

void watcherPollEvents(MinimalApp* app, Watcher* watcher)
{
    DWORD result = WaitForSingleObject(watcher->overlapped.hEvent, 0);

    if (result == WAIT_OBJECT_0) {
        DWORD bytes_transferred;
        GetOverlappedResult(watcher->handle, &watcher->overlapped, &bytes_transferred, FALSE);

        FILE_NOTIFY_INFORMATION* notify = (FILE_NOTIFY_INFORMATION*)watcher->change_buffer;

        for (;;) {

            WatcherEvent e = {
                .action = watcher_actions_map[notify->Action],
                .name_len = notify->FileNameLength / sizeof(wchar_t)
            };
            wcstombs(e.filename, notify->FileName, WATCHER_BUFFER_SIZE);

            minimalDispatchExternalEvent(app, WATCHER_EVENT_ID, &e);

            // Are there more events to handle?
            if (!notify->NextEntryOffset) break;

            *((uint8_t**)&notify) += notify->NextEntryOffset;
        }

        // Queue the next event
        watcherReadDirChanges(watcher);
    }
}