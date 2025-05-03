#include "ffi_bindings.hpp"
#include "pjsua2_manager.hpp"

extern "C" {
    PJSUA2ManagerPtr pjsua2_manager_create(const char* sip_user, const char* sip_password, const char* sip_domain,
                                           DartIncomingCallStateCb incomingCallCb, DartOnRegStateCb onRegStateCb,
                                           DartCallStateCb callStateCb, DartOnErrorCb onErrorCb) {
        try {
            PJSUA2Manager manager = new PJSUA2Manager(   
                string(sip_user), string(sip_password), string(sip_domain),
                incomingCallCb, onRegStateCb, callStateCb, onErrorCb
            );
            return static_cast<PJSUA2ManagerPtr>(manager);
        } catch (const Error &e) {
            return nullptr;
        }
    }

    int pjsua2_manager_destroy(PJSUA2ManagerPtr mgr) {
        delete static_cast<PJSUA2Manager*>(mgr);
    }

    char* pjsua2_make_call(PJSUA2ManagerPtr mgr, const char* remote_uri, char* out_call_id, int buffer_size) {
        try {
            PJSUA2Manager* manager = static_cast<PJSUA2Manager*>(mgr);
            string call_id = manager->make_call(remote_uri);

            if (out_call_id && buffer_size > 0) {
                strncpy(out_call_id, call_id.c_str(), buffer_size - 1);
                out_call_id[buffer_size - 1] = '\0'; 
            }
            return 0; 
        } 
        catch (const Error &e) {
            if (out_call_id && buffer_size > 0) {
                strncpy(out_call_id, e.what(), buffer_size - 1);
                out_call_id[buffer_size - 1] = '\0';
            }
            return -1;
        }
    }

    int pjsua2_hangup_call(PJSUA2ManagerPtr mgr, const char* call_id){
        try{
            static_cast<PJSUA2Manager*>(mgr)->hang_up_call(call_id);
            return 0;
        }catch(const Error &e){
            return -1;
        }
    }

    int pjsua2_answer_call(PJSUA2ManagerPtr mgr, const char* call_id){
        try{
            static_cast<PJSUA2Manager*>(mgr)->answer_call(call_id);
            return 0;
        }catch(const Error &e){
            return -1;
        }catch(const exception &e){
            return -1;
        }
    }

    int pjsua2_get_call_info(
        PJSUA2ManagerPtr mgr,
        const char* call_id,
        CallData* output_data
    ){
        try{
            PJSUA2Manager* manager = static_cast<PJSUA2Manager*>(mgr);

            CallData data = manager->get_call_info(call_id);


            strncpy(output_data->call_id, data.call_id, sizeof(output_data->call_id) - 1);
            output_data->call_id[sizeof(output_data->call_id) - 1] = '\0';

            strncpy(output_data->remote_uri, data.remote_uri, sizeof(output_data->remote_uri) - 1);
            output_data->remote_uri[sizeof(output_data->remote_uri) - 1] = '\0';

            strncpy(output_data->local_uri, data.local_uri, sizeof(output_data->local_uri) - 1);
            output_data->local_uri[sizeof(output_data->local_uri) - 1] = '\0';

            strncpy(output_data->actual_state, data.actual_state, sizeof(output_data->actual_state) - 1);
            output_data->actual_state[sizeof(output_data->actual_state) - 1] = '\0';

            return 0;
        }catch(const Error &e){
            return -1;
        }catch(const exception &e){
            return -1;
        }
    }

}