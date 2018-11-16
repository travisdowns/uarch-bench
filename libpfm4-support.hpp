/*
 * Support for named PMU events as implemented by libpfm4.
 *
 * libpfm4-support.hpp
 */
#include <vector>

#include "context.hpp"

void listPfm4Events(Context &c);

/* an extra PMU evente requested by the end user */
struct PmuEvent {
    /* full libpfm name, including attributes and PMU prefix */
    std::string full_name;
    /* short name that can be used for the table header */
    std::string short_name;
    /* the code for PMU programming */
    uint64_t code;
    /* the PMU slot assigned to the event (not filled in by parseExtraEvents, fill it in yourself */
    unsigned slot;

    PmuEvent(const std::string& full_name, uint64_t code, unsigned slot = -1);

    static std::string make_short_name(const std::string& full);
};

std::vector<PmuEvent> parseExtraEvents(Context &c, const std::string& event_list);
