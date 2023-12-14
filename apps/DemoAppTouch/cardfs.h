#if !defined(__cardfs_h)
#define __cardfs_h

#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <Arduino_JSON.h>

typedef void (*callbackFunction)(const char* parameter);

void listDirWithCallback(fs::FS &fs, const char * dirname, uint8_t levels, callbackFunction func);
void mountSdcard();
void showCardInfo();
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
String readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);
JSONVar readJsonFile(fs::FS &fs, const char * path);

#endif
