#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <sstream>
#include <iterator>
#include <iomanip>
#include <iostream>

#define DUMP_DEC_FORMAT( width ) \
    std::setw( width ) << std::setfill( ' ' ) << std::dec << std::right
#define DUMP_HEX0x_FORMAT( width ) \
    "0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << std::right
#define DUMP_HEX_FORMAT( width ) \
    std::setw( width ) << std::setfill( '0' ) << std::hex << std::right
#define DUMP_STR_FORMAT( width ) \
    std::setw( width ) << std::setfill( ' ' ) << std::hex << std::left

#define PUSH_FLAG(ARR, TYPE, NS, FLAG) \
    if (_ ## ARR & (TYPE)NS::FLAG) \
            ARR.push_back(#FLAG)

namespace vita {
namespace coredump {
namespace utils {

template <typename Range, typename Value = typename Range::value_type>
std::string join(Range const& elements, const char *const delimiter) {
    std::ostringstream os;
    auto b = begin(elements), e = end(elements);

    if (b != e) {
        std::copy(b, prev(e), std::ostream_iterator<Value>(os, delimiter));
        b = prev(e);
    }
    if (b != e) {
        os << *b;
    }

    return os.str();
}

}; // utils
}; // coredump
}; // vita

#endif
