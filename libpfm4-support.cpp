/*
 * libpfm4-support.cpp
 */
#include <iostream>
#include <exception>
#include <cstring>
#include <vector>

#include "libpfm4/include/perfmon/pfmlib.h"

#include "libpfm4-support.hpp"
#include "context.hpp"
#include "util.hpp"

bool pfmIsInit = false;

void init() {
    if (!pfmIsInit) {
        int ret = pfm_initialize();
        if (ret != PFM_SUCCESS) {
            throw std::runtime_error("libpfm4 init failed (" + std::to_string(ret) + "): " + pfm_strerror(ret));
        }
        printf("libpfm4 initialized successfully\n");
        pfmIsInit = true;
    }
}

/*
 * Returns true iff the PMU is supported.
 *
 * In reality, we don't know the list of supported PMUs and there are many PMUs for archs that
 * this tool probably doesn't even compile on, but the main goal is to
 */
bool isSupportedPMU(pfm_pmu_t pmu) {
    return pmu != PFM_PMU_PERF_EVENT && pmu != PFM_PMU_PERF_EVENT_RAW;
}

void listPfm4Events(Context& c) {
    init();

    std::ostream &out = c.out();

    int iter;

    // output the supported PMU types
    std::vector<pfm_pmu_info_t> allinfos;

    out << "Supported libpfm4 PMUs:\n";
    pfm_for_all_pmus(iter) {
        pfm_pmu_info_t pinfo;
        memset(&pinfo, 0, sizeof(pinfo));
        pinfo.size = sizeof(pinfo);
        int ret = pfm_get_pmu_info(static_cast<pfm_pmu_t>(iter), &pinfo);
        if (ret == PFM_SUCCESS && pinfo.is_present) {
            out << string_format("\t[%d, %s, \"%s\"]\n", iter, pinfo.name, pinfo.desc);
            allinfos.push_back(pinfo);
        }
    }
    out << std::endl;

    int count = 0;
    for (pfm_pmu_info_t &pinfo : allinfos) {
        for (int i = pinfo.first_event; i != -1; i = pfm_get_event_next(i)) {
            pfm_event_info_t info;
            memset(&info, 0, sizeof(info));
            info.size = sizeof(info);

            int ret = pfm_get_event_info(i, PFM_OS_NONE, &info);
            if (ret != PFM_SUCCESS) {
                c.fatal("cannot get event info: %s", pfm_strerror(ret));
            } else if (isSupportedPMU(info.pmu)) {

                // for an event with N umask attributes, you effectively have N events, like EVENT.UMASK_1, EVENT.UMASK_2, ...
                bool any_attrs = false;
                int attr_idx;
                pfm_for_each_event_attr(attr_idx, &info) {
                    pfm_event_attr_info_t ainfo = {};
                    ainfo.size = sizeof(ainfo);

                    ret = pfm_get_event_attr_info(info.idx, attr_idx, PFM_OS_NONE, &ainfo);
                    if (ret != PFM_SUCCESS) {
                        c.fatal("cannot get attribute info: %s", pfm_strerror(ret));
                    }

                    if (ainfo.type != PFM_ATTR_UMASK) {
                        continue;
                    }

                    any_attrs = true;

                    out << pinfo.name << "::" << info.name << "." << ainfo.name << std::endl;

                }

                if (!any_attrs) {
                    // otherwise it's just a plain event with umask
                    out << pinfo.name << "::" << info.name << std::endl;
                    count++;
                }

            }
        }
    }

    out << count << " total events." << std::endl;
}


std::vector<PmuEvent> parseExtraEvents(Context& c, const std::vector<std::string>& event_list) {
    init();

    std::vector<PmuEvent> all_codes;

    for (auto &event_str : event_list) {
        // prepare the event info object
        pfm_pmu_encode_arg_t encode_info;
        char *name;
        memset(&encode_info, 0, sizeof(encode_info));
        encode_info.size = sizeof(encode_info);
        encode_info.fstr = &name;


        int ret = pfm_get_os_event_encoding(event_str.c_str(), PFM_PLM0|PFM_PLM3, PFM_OS_NONE, &encode_info);

        if (ret != PFM_SUCCESS) {
            c.err() << "WARNING: Event '" << event_str << "' could not be resolved and will be ignored. Reason: " <<
                    pfm_strerror(ret) << "\n\tUse --list-events to list available events.\n";
        } else {
            if (encode_info.count < 1) {
                c.err() << "WARNING: Event '" << event_str << "' didn't have any codes and will be ignored.\n";
            } else {
                pfm_event_info_t einfo;
                memset(&einfo, 0, sizeof(einfo));
                einfo.size = sizeof(einfo);
                ret = pfm_get_event_info(encode_info.idx, PFM_OS_NONE, &einfo);
                if (ret != PFM_SUCCESS) {
                    c.err() << "WARNING: Couldn't get event_info for '" << event_str << "' - this event will be ignored.\n";
                } else if (!isSupportedPMU(einfo.pmu)) {
                    c.err() << "WARNING: Event '" << event_str << "' is a perf_events-based event which aren't supported and will " <<
                            "be ignored.\n";
                } else {
                    for (int code_idx = 0; code_idx < encode_info.count; code_idx++) {
                        uint64_t code = encode_info.codes[code_idx];
                        PmuEvent e(name, code);
                        c.out() << "Event '" << event_str << "' resolved to '" << name <<
                                ", short name: '" << e.short_name << "' with code 0x" << std::hex << e.code << std::dec << "\n";
                        all_codes.push_back(e);
                        if (code_idx == 1) {
                            c.err() << "WARNING: MULTIPLE CODES FOR '" << event_str << "'\n";
                        }
                    }
                }
            }
        }
    }

    return all_codes;
}

std::string PmuEvent::make_short_name(const std::string& full)
{
    std::vector<std::string> components = split(full, "::");
    const std::string& trimmed = components.size() > 1 ? components.at(1) : full;
    return trimmed.substr(0, std::min(size_t{6}, full.length()));
}

PmuEvent::PmuEvent(const std::string& full_name, uint64_t code, unsigned slot) : full_name{full_name},
        short_name{make_short_name(full_name)},
        code{code},
        slot{slot}
        {

        }





