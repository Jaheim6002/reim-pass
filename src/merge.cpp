#include "headers/merge.hpp"

void Merging::Sync (const fs::path& directory) {
    fs::path basePath = directory / "base";
    fs::path localPath = directory / "logindata.db";
    fs::path syncPath = directory / "sync" / "logindata.db";
    std::string base = readBase(basePath);
    std::string local = fileHashHex(localPath);
    std::string sync = fs::exists(syncPath) ? fileHashHex(syncPath) : "";

    if (local == sync) {
        writeBase(basePath, local);
    } else if (local == base) {
        atomicCopy(syncPath, localPath); // pull
        writeBase(basePath, local);
    } else if (sync == base) {
        atomicCopy(localPath, syncPath); // push
        writeBase(basePath, sync);
    } else {
        //Merge();
        atomicCopy(localPath, syncPath); // push for now
        writeBase(basePath, local);
    }
    }
/*
void Merge () {
    // Mergeing is to be added
}
*/