// pjsua2_manager_interface.hpp
#ifndef PJSUA2_MANAGER_INTERFACE_HPP
#define PJSUA2_MANAGER_INTERFACE_HPP

#include <string>
#include "pjsua2_manager.hpp"

class IPJSUA2Manager {
public:
    virtual ~IPJSUA2Manager() = default;
    virtual std::string make_call(const std::string& uri) = 0;
    virtual void answer_call(const std::string& call_id) = 0;
    virtual void hang_up_call(const std::string& call_id) = 0;
    virtual CallData get_call_info(const std::string& call_id) = 0;
    virtual void start_event_loop(unsigned timeout_ms) = 0;
    virtual void stop_event_loop() = 0;
};

#endif