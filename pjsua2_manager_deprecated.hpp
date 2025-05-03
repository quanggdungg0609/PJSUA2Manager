#ifndef PJSUA2_MANAGER_H
#define PJSUA2_MANAGER_H

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief Represents the information of a SIP call.
 */
typedef struct {
    char call_id[128];       ///< Unique identifier of the call
    char remote_uri[128];    ///< SIP URI of the remote party
    char local_uri[128];     ///< SIP URI of the local party
    char actual_state[64];   ///< Current call state (e.g., CALLING, CONNECTED, DISCONNECTED)
} CallData;

/**
 * @brief Opaque pointer to an instance of the PJSUA2Manager.
 */
typedef void* PJSUA2ManagerPtr;

/**
 * @brief Callback triggered when there is an incoming call.
 * 
 * @param call_id The identifier of the incoming call.
 */
typedef void (*DartIncomingCallStateCb)(const char* call_id);

/**
 * @brief Callback triggered when registration state changes.
 * 
 * @param code Registration status code.
 * @param status Status string (e.g., "Registered").
 * @param reason Reason for the change (if any).
 */
typedef void (*DartOnRegStateCb) (int code, const char* status, const char* reason);

/**
 * @brief Callback triggered when the call state changes.
 * 
 * @param remote_uri Remote SIP URI of the call.
 * @param state_text New state text (e.g., "CALLING", "DISCONNECTED").
 */
typedef void (*DartCallStateCb) (const char* remote_uri, const char*  state_text);


/**
 * @brief Callback triggered when an error occurs in the SIP stack.
 * 
 * @param title Short title of the error.
 * @param reason Detailed reason of the error.
 * @param info Additional information (e.g., SIP error code).
 * @param src_file Source file where the error occurred.
 * @param scr_line Line number in the source file.
 */
typedef void (*DartOnErrorCb) (
    const char* title, 
    const char* reason, 
    const char* info,
    const char* src_file,
    const int scr_line
);


/**
 * @brief Initializes and creates a new SIP manager.
 * 
 * @param sip_user SIP username for authentication.
 * @param sip_password SIP password.
 * @param sip_domain SIP server domain or IP address.
 * @param incomingCallCb Callback for incoming calls (optional).
 * @param onRegStateCb Callback for registration state changes (optional).
 * @param callStateCb Callback for call state changes (optional).
 * @param onErrorCb Callback for SIP or internal errors (optional).
 * @return Pointer to the created SIP manager instance.
 */
PJSUA2ManagerPtr pjsua2_manager_create(
    const char* sip_user,
    const char* sip_password,
    const char* sip_domain,
    DartIncomingCallStateCb incomingCallCb = nullptr,
    DartOnRegStateCb onRegStateCb = nullptr,
    DartCallStateCb callStateCb = nullptr,
    DartOnErrorCb onErrorCb = nullptr
);

/**
 * @brief Destroys the SIP manager instance and cleans up all resources.
 * 
 * @param manager Pointer to the manager to be destroyed.
 */
void pjsua2_manager_destroy(PJSUA2ManagerPtr manager);



/**
 * @brief Initiates a new call to the given remote SIP URI.
 * 
 * @param manager SIP manager instance.
 * @param remote_uri The target SIP address to call.
 */
void pjsua2_make_call(PJSUA2ManagerPtr manager, const char* remote_uri);

/**
 * @brief Answers an incoming call with the given call ID.
 * 
 * @param manager SIP manager instance.
 * @param call_id Identifier of the incoming call to answer.
 */
void pjsua2_answer_call(PJSUA2ManagerPtr manager, const char* call_id);

/**
 * @brief Hangs up an active or incoming call.
 * 
 * @param manager SIP manager instance.
 * @param call_id Identifier of the call to hang up.
 */
void pjsua2_hangup_call(PJSUA2ManagerPtr manager, const char* call_id);

/**
 * @brief Retrieves current information about a specific call.
 * 
 * @param manager SIP manager instance.
 * @param call_id Identifier of the call.
 * @param out_data Pointer to CallData structure to be filled.
 */
void pjsua2_get_call_info(
    PJSUA2ManagerPtr manager, 
    const char* call_id, 
    CallData* out_data // output parameter
);

/**
 * @brief Starts the internal event loop to handle SIP events (calls, registration, etc).
 * 
 * @param manager SIP manager instance.
 * @param timeout_ms Timeout in milliseconds for the loop to wait per iteration.
 */
void pjsua2_manager_start_event_loop(PJSUA2ManagerPtr manager, unsigned timeout_ms);

/**
 * @brief Stops the internal event loop.
 * 
 * @param manager SIP manager instance.
 */
void pjsua2_manager_stop_event_loop(PJSUA2ManagerPtr mgr);

#ifdef __cplusplus
}
#endif

#endif // PJSUA2_MANAGER_H