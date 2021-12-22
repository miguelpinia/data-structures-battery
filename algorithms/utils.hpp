#ifndef UTILS_HPP
#define UTILS_HPP
#include <stdexcept>

enum STATE_BASKET {OPEN, CLOSED};
enum STATE_PUT {OK, FULL};

const int BASKET_CLOSED = -1;
const int BOTTOM = -2;
const int TOP = -3;
const int EMPTY = -4;

class NotImplementedException : public std::logic_error
{
public:
    NotImplementedException () : std::logic_error{"Function not yet implemented."} {}
};
#endif
