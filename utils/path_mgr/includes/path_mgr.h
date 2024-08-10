#include <libgen.h>
#include <linux/limits.h>
#include <unistd.h>
#include <iostream>

#ifndef PATH_MGR_H
#define PATH_MGR_H

// #ifndef EXEC_PATH
// extern const std::string exec_path;
// #endif // EXEC_PATH

struct Path{
private:
    static std::string exec_path;
    std::string path;

public:

    Path(const char *path);
    operator const char *() const;
};

#endif // PATH_MGR_H