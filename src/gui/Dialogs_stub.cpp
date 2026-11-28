#ifndef _WIN32
#include "Dialogs.h"
namespace dis {
std::string openImageDialog(void*) { return std::string(); }
void openInDefaultApp(void*, const std::string&) {}
}
#endif
