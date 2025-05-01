#ifndef PJSUA2_MANAGER_H
#define PJSUA2_MANAGER_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    char call_id[128];
    char remote_uri[128];
    char local_uri[128];
    char actual_state[64];
} CallData;

typedef void* PJSUA2ManagerPtr;

typedef void (*DartIncomingCallStateCb)(const char* call_id);
typedef void (*DartOnRegStateCb) (int code, const char* status, const char* reason);
typedef void (*DartCallStateCb) (const char* remote_uri, const char*  state_text);
typedef void (*DartOnErrorCb) (
    const char* title, 
    const char* reason, 
    const char* info,
    const char* src_file,
    const int scr_line
);

PJSUA2ManagerPtr pjsua2_manager_create(
    const char* sip_user,
    const char* sip_password,
    const char* sip_domain,
    DartIncomingCallStateCb incomingCallCb = nullptr,
    DartOnRegStateCb onRegStateCb = nullptr,
    DartCallStateCb callStateCb = nullptr,
    DartOnErrorCb onErrorCb = nullptr
);

void pjsua2_manager_destroy(PJSUA2ManagerPtr manager);

void pjsua2_make_call(PJSUA2ManagerPtr manager, const char* remote_uri);
void pjsua2_answer_call(PJSUA2ManagerPtr manager, const char* call_id);
void pjsua2_hangup_call(PJSUA2ManagerPtr manager, const char* call_id);
void pjsua2_get_call_info(
    PJSUA2ManagerPtr manager, 
    const char* call_id, 
    CallData* out_data // output parameter
);

void pjsua2_manager_start_event_loop(PJSUA2ManagerPtr manager, unsigned timeout_ms);
void pjsua2_manager_stop_event_loop(PJSUA2ManagerPtr mgr);

#ifdef __cplusplus
}
#endif

#endif // PJSUA2_MANAGER_H