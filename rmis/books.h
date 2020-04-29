#include <cstddef>
#include <cstdint>
namespace books {
bool load(char const* dataPath);
void cleanup();
const size_t RMI_SIZE = 50331664;
const uint64_t BUILD_TIME_NS = 16529437560;
const char NAME[] = "books";
uint64_t lookup(uint64_t key, size_t* err);
}
