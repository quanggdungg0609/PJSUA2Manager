#ifndef FFI_BINDINGS_H
#define FFI_BINDINGS_H

#include "pjsua2_manager.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* PJSUA2ManagerPtr;

PJSUA2ManagerPtr pjsua2_manager_create(
    const char* sip_user,
    const char* sip_password, 
    const char* sip_domain,
    DartIncomingCallStateCb incomingCallCb, 
    DartOnRegStateCb onRegStateCb,
    DartCallStateCb callStateCb, 
    DartOnErrorCb onErrorCb
);
int pjsua2_manager_destroy(PJSUA2ManagerPtr mgr);

int pjsua2_make_call(PJSUA2ManagerPtr mgr, const char* remote_uri, char* out_call_id, int buffer_size);
int pjsua2_hangup_call(PJSUA2ManagerPtr mgr, const char* call_id);
int pjsua2_answer_call(PJSUA2ManagerPtr mgr, const char* call_id);
int pjsua2_get_call_info(PJSUA2ManagerPtr mgr, const char* call_id, CallData* output_data);


void pjsua2_start_events_loop(PJSUA2ManagerPtr mgr, unsigned timeout_ms);
void pjsua2_stop_events_loop(PJSUA2ManagerPtr mgr);

#ifdef __cplusplus
}
#endif

#endif // FFI_BINDINGS_H