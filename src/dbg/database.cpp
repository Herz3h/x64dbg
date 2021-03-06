/**
@file database.cpp

@brief Implements runtime database saving and loading.
*/

#include "lz4\lz4file.h"
#include "console.h"
#include "breakpoint.h"
#include "patches.h"
#include "comment.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"
#include "loop.h"
#include "commandline.h"
#include "database.h"
#include "threading.h"
#include "filehelper.h"

/**
\brief Directory where program databases are stored (usually in \db). UTF-8 encoding.
*/
char dbbasepath[deflen];

/**
\brief Path of the current program database. UTF-8 encoding.
*/
char dbpath[deflen];

void DbSave(DbLoadSaveType saveType)
{
    EXCLUSIVE_ACQUIRE(LockDatabase);

    dprintf("Saving database...");
    DWORD ticks = GetTickCount();
    JSON root = json_object();

    // Save only command line
    if(saveType == DbLoadSaveType::CommandLine || saveType == DbLoadSaveType::All)
    {
        CmdLineCacheSave(root);
    }

    if(saveType == DbLoadSaveType::DebugData || saveType == DbLoadSaveType::All)
    {
        CommentCacheSave(root);
        LabelCacheSave(root);
        BookmarkCacheSave(root);
        FunctionCacheSave(root);
        LoopCacheSave(root);
        BpCacheSave(root);

        //save notes
        char* text = nullptr;
        GuiGetDebuggeeNotes(&text);
        if(text)
        {
            json_object_set_new(root, "notes", json_string(text));
            BridgeFree(text);
        }
        GuiSetDebuggeeNotes("");
    }

    WString wdbpath = StringUtils::Utf8ToUtf16(dbpath);
    if(json_object_size(root))
    {
        char* jsonText = json_dumps(root, JSON_INDENT(4));

        if(jsonText)
        {
            // Dump JSON to disk (overwrite any old files)
            if(!FileHelper::WriteAllText(dbpath, jsonText))
            {
                dputs("\nFailed to write database file!");
                json_free(jsonText);
                json_decref(root);
                return;
            }

            json_free(jsonText);
        }

        if(!settingboolget("Engine", "DisableDatabaseCompression"))
            LZ4_compress_fileW(wdbpath.c_str(), wdbpath.c_str());
    }
    else //remove database when nothing is in there
        DeleteFileW(wdbpath.c_str());
    dprintf("%ums\n", GetTickCount() - ticks);
    json_decref(root); //free root
}

void DbLoad(DbLoadSaveType loadType)
{
    EXCLUSIVE_ACQUIRE(LockDatabase);

    // If the file doesn't exist, there is no DB to load
    if(!FileExists(dbpath))
        return;

    if(loadType == DbLoadSaveType::CommandLine)
        dputs("Loading commandline...");
    else
        dprintf("Loading database...");
    DWORD ticks = GetTickCount();

    // Multi-byte (UTF8) file path converted to UTF16
    WString databasePathW = StringUtils::Utf8ToUtf16(dbpath);

    // Decompress the file if compression was enabled
    bool useCompression = !settingboolget("Engine", "DisableDatabaseCompression");
    LZ4_STATUS lzmaStatus = LZ4_INVALID_ARCHIVE;
    {
        lzmaStatus = LZ4_decompress_fileW(databasePathW.c_str(), databasePathW.c_str());

        // Check return code
        if(useCompression && lzmaStatus != LZ4_SUCCESS && lzmaStatus != LZ4_INVALID_ARCHIVE)
        {
            dputs("\nInvalid database file!");
            return;
        }
    }

    // Read the database file
    String databaseText;

    if(!FileHelper::ReadAllText(dbpath, databaseText))
    {
        dputs("\nFailed to read database file!");
        return;
    }

    // Restore the old, compressed file
    if(lzmaStatus != LZ4_INVALID_ARCHIVE && useCompression)
        LZ4_compress_fileW(databasePathW.c_str(), databasePathW.c_str());

    // Deserialize JSON and validate
    JSON root = json_loads(databaseText.c_str(), 0, 0);

    if(!root)
    {
        dputs("\nInvalid database file (JSON)!");
        return;
    }

    // Load only command line
    if(loadType == DbLoadSaveType::CommandLine || loadType == DbLoadSaveType::All)
    {
        CmdLineCacheLoad(root);
    }

    if(loadType == DbLoadSaveType::DebugData || loadType == DbLoadSaveType::All)
    {
        // Finally load all structures
        CommentCacheLoad(root);
        LabelCacheLoad(root);
        BookmarkCacheLoad(root);
        FunctionCacheLoad(root);
        LoopCacheLoad(root);
        BpCacheLoad(root);

        // Load notes
        const char* text = json_string_value(json_object_get(root, "notes"));
        GuiSetDebuggeeNotes(text);
    }

    // Free root
    json_decref(root);

    if(loadType != DbLoadSaveType::CommandLine)
        dprintf("%ums\n", GetTickCount() - ticks);
}

void DbClose()
{
    DbSave(DbLoadSaveType::All);
    CommentClear();
    LabelClear();
    BookmarkClear();
    FunctionClear();
    LoopClear();
    BpClear();
    PatchClear();
}

void DbSetPath(const char* Directory, const char* ModulePath)
{
    EXCLUSIVE_ACQUIRE(LockDatabase);

    // Initialize directory only if it was supplied
    if(Directory)
    {
        ASSERT_TRUE(strlen(Directory) > 0);

        // Copy to global
        strcpy_s(dbbasepath, Directory);

        // Create directory
        if(!CreateDirectoryW(StringUtils::Utf8ToUtf16(Directory).c_str(), nullptr))
        {
            if(GetLastError() != ERROR_ALREADY_EXISTS)
                dprintf("Warning: Failed to create database folder '%s'. Path may be read only.\n", Directory);
        }
    }

    // The database file path may be relative (dbbasepath) or a full path
    if(ModulePath)
    {
        ASSERT_TRUE(strlen(ModulePath) > 0);

#ifdef _WIN64
        const char* dbType = "dd64";
#else
        const char* dbType = "dd32";
#endif // _WIN64

        // Get the module name and directory
        char dbName[deflen];
        char fileDir[deflen];
        {
            // Dir <- file path
            strcpy_s(fileDir, ModulePath);

            // Find the last instance of a path delimiter (slash)
            char* fileStart = strrchr(fileDir, '\\');

            if(fileStart)
            {
                strcpy_s(dbName, fileStart + 1);
                fileStart[0] = '\0';
            }
            else
            {
                // Directory or file with no extension
                strcpy_s(dbName, fileDir);
            }
        }

        if(settingboolget("Engine", "SaveDatabaseInProgramDirectory"))
        {
            // Absolute path in the program directory
            sprintf_s(dbpath, "%s\\%s.%s", fileDir, dbName, dbType);
        }
        else
        {
            // Relative path in debugger directory
            sprintf_s(dbpath, "%s\\%s.%s", dbbasepath, dbName, dbType);
        }

        dprintf("Database file: %s\n", dbpath);
    }
}