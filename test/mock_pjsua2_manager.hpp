// mock_pjsua2_manager.hpp
#ifndef MOCK_PJSUA2_MANAGER_HPP
#define MOCK_PJSUA2_MANAGER_HPP

#include <gmock/gmock.h>
#include "../pjsua2_manager_interface.hpp"

class MockPJSUA2Manager : public IPJSUA2Manager {
public:
    MOCK_METHOD(std::string, make_call, (const std::string& uri), (override));
    MOCK_METHOD(void, answer_call, (const std::string& call_id), (override));
    MOCK_METHOD(void, hang_up_call, (const std::string& call_id), (override));
    MOCK_METHOD(CallData, get_call_info, (const std::string& call_id), (override));
    MOCK_METHOD(void, start_event_loop, (unsigned timeout_ms), (override));
    MOCK_METHOD(void, stop_event_loop, (), (override));
};

#endif