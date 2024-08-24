// #define EXEC_PATH
#include "utils/path_mgr/includes/path_mgr.h"
#include "utils/logger/includes/logger.h"



// const std::string exec_path;

std::string Path::exec_path = "";

Path::Path(const char *path_): path(path_){

  // initialize the path of the executable file
  if (exec_path.size() == 0) {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
      exec_path = std::string(dirname(result));
      LOGD("exec path: %s", exec_path.c_str());
    } else {
      LOGE("readlink failed");
    }
  }

  if (this->path[0] == '.'){
    this->path = exec_path + this->path.substr(1);
  }
}

Path::operator const char *() const {
  return this->path.c_str();
}

bool Path::operator ==(const Path& other) const {
  return this->path == other.path;
}