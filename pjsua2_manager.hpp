#ifndef PJSUA2_MANAGER_H
#define PJSUA2_MANAGER_H

#define PJ_AUTOCONF 1

#include <pjsua2.hpp>
#include <pjsua-lib/pjsua.h>
#include <unordered_map>
#include <iostream>
#include <mutex>
#include <thread>
#include <pthread.h>
#include <atomic>
#include <memory>

using namespace pj;
using namespace std;


/**
 * @brief Structure containing call information data.
 * 
 * This structure holds details about a SIP call including call identifiers,
 * remote/local URIs, and current call state.
 */
typedef struct {
    char call_id[128];       /**< Unique call identifier (null-terminated string) */
    char remote_uri[128];    /**< Remote party URI (null-terminated string) */
    char local_uri[128];     /**< Local party URI (null-terminated string) */
    char actual_state[64];   /**< Current call state description (null-terminated string) */
} CallData;

typedef void (*DartIncomingCallStateCb)(const char* call_id); /**< Incoming call state callback */
typedef void (*DartOnRegStateCb)(int code, const char* status, const char* reason); /**< Registration state callback */
typedef void (*DartCallStateCb)(const char* remote_uri, const char* state_text); /**< General call state callback */
typedef void (*DartOnErrorCb)(const char* title, const char* reason, const char* info, const char* src_file, int src_line);  /**< Error reporting callback */


/**
 * @brief Main manager class for PJSUA2 operations.
 * 
 * Provides SIP account management, call control, and event handling capabilities.
 * Manages threading, synchronization, and interaction with PJSIP stack.
 */
class PJSUA2Manager {
public:

    /**
     * @brief Construct a new PJSUA2Manager object
     * 
     * @param sip_user SIP account username
     * @param sip_password SIP account password
     * @param sip_domain SIP domain/server address
     * @param onIncomingCallCb Incoming call notification callback (optional)
     * @param onRegStateCb Registration state change callback (optional)
     * @param onCallStateCb Call state change callback (optional)
     * @param onErrorCb Error reporting callback (optional)
     */
    PJSUA2Manager(
        const string& sip_user,
        const string& sip_password,
        const string& sip_domain,
        DartIncomingCallStateCb onIncomingCallCb = nullptr,
        DartOnRegStateCb onRegStateCb = nullptr,
        DartCallStateCb onCallStateCb = nullptr,
        DartOnErrorCb onErrorCb = nullptr
    );

    /**
     * @brief Destroy the PJSUA2Manager object
     * 
     * Stops event loop and cleans up SIP resources
     */
    ~PJSUA2Manager();

    /**
     * @brief Start the event processing loop
     * 
     * @param timeout_ms Timeout for event polling in milliseconds
     */
    void start_event_loop(unsigned timeout_ms);

    /**
     * @brief Stop the event processing loop
     */
    void stop_event_loop();

      /**
     * @brief Initiate an outgoing call
     * 
     * @param dest_uri Destination SIP URI
     * @return string The created call's ID
     * @throw Error on failure
     */
    string make_call(const string& dest_uri);

    /**
    * @brief Answer an incoming call
    * 
    * @param call_id ID of the call to answer
    * @throw Error on failure
    */
    void answer_call(const string& call_id);

    /**
    * @brief Terminate a call
    * 
    * @param call_id ID of the call to terminate
    * @throw Error on failure
    */
    void hang_up_call(const string& call_id);

    /**
    * @brief Retrieve call information
    * 
    * @param call_id ID of the target call
    * @return CallData Structure containing call details
    * @throw Error if call not found
    */
    CallData get_call_info(const string& call_id);
private:
    friend class PJSUA2Account;  /**< Friend class for account management */
    friend class PJSUA2Call;     /**< Friend class for call operations */

    pj_thread_desc _mainThreadDesc; 
    atomic<bool> _isRunning;                     /**< Event loop control flag */
    unique_ptr<Endpoint> _endpoint;               /**< PJSUA2 endpoint instance */
    unique_ptr<Account> _account;                 /**< SIP account instance */
    unordered_map<string, unique_ptr<Call>> _activeCalls;   /**< Active calls map */
    unordered_map<string, unique_ptr<Call>> _inboundCalls;  /**< Incoming calls map */
    unordered_map<string, unique_ptr<Call>> _outboundCalls; /**< Outgoing calls map */
    thread _eventThread;                          /**< Event processing thread */

    // Mutex
    recursive_mutex _activeCallsMutex;                                 /**< Synchronization active calls mutex */
    recursive_mutex _inboundCallsMutex;                                 /**< Synchronization inbound calls mutex */
    recursive_mutex _outboundCallsMutex;                                 /**< Synchronization outbound calls mutex */


    // Callback handlers
    DartIncomingCallStateCb _onIncomingCallStateCb; /**< Incoming call callback */
    DartOnRegStateCb _onRegStateCb;               /**< Registration state callback */
    DartCallStateCb _onCallStateCb;               /**< Call state callback */
    DartOnErrorCb _onErrorCb;                     /**< Error callback */

    /**
     * @brief Internal event processing method
     * 
     * @param timeout_ms Event polling timeout
     */
    void _handle_events(unsigned timeout_ms);


    /**
     * @brief Error handling method
     * 
     * @param e Reference to error object
     */
    void _handle_error(const Error &e);

    /**
     * @brief Nested class for SIP account management
     */
    class PJSUA2Account : public Account {
    private:
        PJSUA2Manager& m_manager; /**< Reference to parent manager */
    public:
        PJSUA2Account(PJSUA2Manager& manager) : m_manager(manager) {}

         /**
         * @brief Handle registration state changes
         */
        virtual void onRegState(OnRegStateParam &prm) override;

        /**
         * @brief Handle incoming calls
         */
        virtual void onIncomingCall(OnIncomingCallParam &prm) override;
    };

    /**
     * @brief Nested class for call operations
     */
    class PJSUA2Call : public Call {
    private:
        PJSUA2Manager& m_manager; /**< Reference to parent manager */
    public:
        PJSUA2Call(PJSUA2Manager& manager, Account &acc, int call_id);

         /**
         * @brief Handle call state changes
         */
        virtual void onCallState(OnCallStateParam &prm) override;

        /**
         * @brief Handle media state changes
         */
        virtual void onCallMediaState(OnCallMediaStateParam &prm) override;
    };
};

#endif // PJSUA2_MANAGER_H