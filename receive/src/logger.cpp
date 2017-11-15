// Because fuck C++

#include "logger.h"
#undef LOG_TYPE

#define LOG_TYPE(_,NAME,...)\
constexpr char CONCAT(NAME,_log)::message[];\
constexpr uint8_t CONCAT(NAME,_log)::id;\
constexpr std::array<char const*, NUMARGS(__VA_ARGS__)> CONCAT(NAME,_log)::arg_names;

#include "loglist.h"
