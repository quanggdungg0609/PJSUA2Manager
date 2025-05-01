#define PJ_AUTOCONF 1

#include "pjsua2_manager.hpp"
#include <unordered_map>
#include <iostream>
#include <pjsua2.hpp>
#include <mutex>
#include <thread>
#include <atomic>

using namespace pj;
using namespace std;




class PJSUA2Manager{
    private:
        atomic<bool> _isRunning = false;
        Endpoint* _endpoint;
        Account * _account = nullptr;
        
        unordered_map<string, unique_ptr<Call>> _activeCalls;
        unordered_map<string, unique_ptr<Call>> _inboundCalls;
        unordered_map<string, unique_ptr<Call>> _outboundCalls;



        mutex _mutex;
        thread _eventThread;
        
        DartIncomingCallStateCb _onIncomingCallStateCb = nullptr;
        DartOnRegStateCb _onRegStateCb = nullptr;
        DartCallStateCb _onCallStateCb = nullptr;
        DartOnErrorCb _onErrorCb = nullptr;


        void _handleError(Error &e){
            if(_onErrorCb){
                _onErrorCb(
                    e.title().c_str(),
                    e.reason().c_str(),
                    e.info(true).c_str(),
                    e.srcFile().c_str(),
                    e.srcLine()
                );
            }else{
                cout << e.title() << endl;
                cout <<  e.reason() << endl;
                cout << e.info(true) << endl;
                cout << e.srcFile() << endl;
                cout << e.srcLine() << endl;
            }
        }
    
    public:
        friend class PJSUA2Account;
        friend class PJSUA2Call;

        class PJSUA2Account : public Account{
            private:
                PJSUA2Manager& m_manager; // reference of PJSUA2 Manager
            public:
                PJSUA2Account(PJSUA2Manager& manager) : m_manager(manager) {}
                virtual void onRegState(OnRegStateParam &prm) override {
                    AccountInfo accountInfo = getInfo();

                    if(m_manager._onRegStateCb) {
                        m_manager._onRegStateCb(
                            prm.code, 
                            accountInfo.regIsActive ? "Active" : "Inactive",
                            prm.reason.c_str()
                        );
                    }
                }

                virtual void onIncomingCall(OnIncomingCallParam &prm) override {
                    auto call = make_unique<PJSUA2Call>(this->m_manager, *this, prm.callId);

                    CallOpParam callParam;
                    callParam.statusCode = PJSIP_SC_OK;
                    if (m_manager._onIncomingCallStateCb) {
                        m_manager._onIncomingCallStateCb(call->getInfo().callIdString.c_str());
                    }
                    else{
                        cout << "Call from: " << prm.callId << endl;
                    }
                    lock_guard<mutex> lock(m_manager._mutex);
                    m_manager._inboundCalls[call->getInfo().callIdString] = std::move(call);
                }
        };

        class PJSUA2Call : public Call{
            private:
                PJSUA2Manager& m_manager;

            public:
                PJSUA2Call(PJSUA2Manager& manager, Account &acc, int call_id) : Call(acc, call_id), m_manager(manager) {}

                virtual void onCallState(OnCallStateParam &prm) override{
                    PJ_UNUSED_ARG(prm);

                    CallInfo callInfo = getInfo();
                    
                    if(m_manager._onCallStateCb){
                        m_manager._onCallStateCb(
                            callInfo.remoteUri.c_str(),
                            callInfo.stateText.c_str()
                        );
                    }

                    switch(callInfo.state){
                        case PJSIP_INV_STATE_NULL:
                            break;
                        case PJSIP_INV_STATE_CALLING:
                            break;
                        case PJSIP_INV_STATE_INCOMING:
                            break;
                        case PJSIP_INV_STATE_EARLY:
                            break;
                        case PJSIP_INV_STATE_CONNECTING:
                            lock_guard<mutex> lock(m_manager._mutex);
                            string callId = callInfo.callIdString;
                            if (auto it = m_manager._inboundCalls.find(callId); it != m_manager._inboundCalls.end()) {
                                m_manager._activeCalls[callId] = std::move(it->second);
                                m_manager._inboundCalls.erase(it);
                            }
                            if (auto it = m_manager._outboundCalls.find(callId); it != m_manager._outboundCalls.end()) {
                                m_manager._activeCalls[callId] = std::move(it->second);
                                m_manager._outboundCalls.erase(it);
                            }
                            break;
                        case PJSIP_INV_STATE_CONFIRMED:
                            break;
                        case PJSIP_INV_STATE_DISCONNECTED:
                            lock_guard<mutex> lock(m_manager._mutex);
                            string callId = callInfo.callIdString;
                            
                            // delete call from all maps
                            m_manager._activeCalls.erase(callId);
                            m_manager._inboundCalls.erase(callId);
                            m_manager._outboundCalls.erase(callId);
                            break;
                    }
                }

                virtual void onCallMediaState(OnCallMediaStateParam &prm){
                    CallInfo callInfo = getInfo();
                    // bind media
                    for(unsigned i = 0; i < callInfo.media.size(); i++){
                        if(callInfo.media[i].type == PJMEDIA_TYPE_AUDIO){
                            try{
                                AudioMedia aud_media = getAudioMedia();
                                // Connect the call audio media to the sound device
                                AudDevManager& audioDevManager = Endpoint::instance().audDevManager();
                                aud_media.startTransmit(audioDevManager.getPlaybackDevMedia());
                                audioDevManager.getCaptureDevMedia().startTransmit(aud_media);

                            }catch(const Error &e){
                                // if _onError is binded call them
                                _handleError(e);
                            }
                            break;
                        }
                    }
                };
};

        ~PJSUA2Manager(){
            stop_event_loop();
            _activeCalls.clear();
            delete _account;
            _endpoint->libDestroy();
            delete _endpoint;
        }

        // Constructor
        PJSUA2Manager(
            const string& sip_user,
            const string& sip_password,
            const string& sip_domain,
            DartIncomingCallStateCb onIncomingCallCb = nullptr,
            DartOnRegStateCb onRegStateCb = nullptr,
            DartCallStateCb onCallStateCb = nullptr,
            DartOnErrorCb onErrorCb = nullptr
        ){
            // set callbacks
            _onIncomingCallStateCb = onIncomingCallCb;
            _onRegStateCb = onRegStateCb;
            _onCallStateCb = onCallStateCb;
            _onErrorCb = onErrorCb;

            try{
                if(!_endpoint){
                    _endpoint = new Endpoint();
                }
                _account = nullptr;
    
                
    
                // Initialize Endpoint
                _endpoint->libCreate();
    
                EpConfig epConfig;
                epConfig.uaConfig.threadCnt = 1;
                epConfig.uaConfig.maxCalls = 2;
    
                epConfig.medConfig.ecOptions = PJMEDIA_ECHO_DEFAULT;
                epConfig.medConfig.ecTailLen = 200;
                epConfig.medConfig.quality = 10;
                epConfig.medConfig.ptime = 20;
    
                epConfig.logConfig.level = 3;
    
                _endpoint->libInit(epConfig);
    
                _endpoint->codecSetPriority("opus/48000/2", PJMEDIA_CODEC_PRIO_HIGHEST);
                _endpoint->codecSetPriority("PCMU/8000/1", PJMEDIA_CODEC_PRIO_HIGHEST);
                _endpoint->codecSetPriority("PCMA/8000/1", PJMEDIA_CODEC_PRIO_HIGHEST);
    
                TransportConfig tcfg;
                tcfg.port = 5060;
                _endpoint->transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
    
                _endpoint->libStart();
    
                AccountConfig accCfg;
                std::string sipUri = "sip:" + sip_user + "@" + sip_domain;
                std::string registrarUri = "sip:" + sip_domain;
                accCfg.idUri = sipUri;
                accCfg.regConfig.registrarUri = registrarUri;
    
                AuthCredInfo cred("digest", "*", sip_user, 0, sip_password);
                accCfg.sipConfig.authCreds.push_back(cred);
    
                PJSUA2Account* acc = new PJSUA2Account(*this);
                acc->create(accCfg);
                _account = acc;
            }catch (Error &e){
                _handleError(e);
                throw;
            }           
            
        }

        void start_event_loop(unsigned timeout_ms){
            _isRunning.store(true, std::memory_order_release);
            _eventThread = thread([this]() {
                while (_isRunning.load(std::memory_order_relaxed)) {
                    this->handle_events(timeout_ms); 
                }
            });
        }

        void handle_events(unsigned timeout_ms) {
            lock_guard<mutex> lock(_mutex);
            _endpoint->libHandleEvents(timeout_ms);
        }

        void stop_event_loop() {
            _isRunning = false;
            if (_eventThread.joinable()) {
                _eventThread.join();
            }
        }
        
        void make_call(const string& dest_uri){
            try{
                auto *newCall =make_unique<PJSUA2Call>(*this, this->_account);
                string callId = newCall->getInfo().callIdString;
                CallOpParam prm(true); // Use default call settings
                newCall->makeCall(dest_uri, prm);
                lock_guard<mutex> lock(_mutex);
                _outboundCalls[callId] = unique_ptr<Call>(newCall);
            }catch(const Error &e){
                _handleError(e);
            }
        }

        void answer_call(const string& call_id){
            try{
                lock_guard<mutex> lock(_mutex);
                auto it = _inboundCalls.find(call_id);
                if(it != _inboundCalls.end()){
                    CallOpParam prm;
                    prm.statusCode = PJSIP_SC_OK;
                    it->second.get()->answer(prm);

                }
            }catch(Error &e){
                _handleError(e);
                throw;
            }
        }

        void hang_up_call(const string& call_id){
            try {
                lock_guard<mutex> lock(_mutex);
        
                // Helper function to handle hangup with appropriate status code
                auto hangup_call = [](Call* call, pjsip_status_code default_code) {
                    CallInfo call_info = call->getInfo(); // Get current call state
                    CallOpParam prm;
        
                    // Select status code based on call state
                    switch (call_info.state) {
                        case PJSIP_INV_STATE_CONFIRMED:
                            prm.statusCode = PJSIP_SC_OK;         // 200 OK for established call
                            break;
                        case PJSIP_INV_STATE_EARLY:
                        case PJSIP_INV_STATE_CALLING:
                            prm.statusCode = PJSIP_SC_REQUEST_TERMINATED; // 487 for unconnected outgoing call
                            break;
                        case PJSIP_INV_STATE_INCOMING:
                            prm.statusCode = PJSIP_SC_DECLINE;    // 603 for unaccepted incoming call
                            break;
                        default:
                            prm.statusCode = default_code;        // Fallback status code
                    }
        
                    call->hangup(prm); // Send hangup request
                };
        
                // Check and process call in inbound/outbound/active maps
                if (auto it = _inboundCalls.find(call_id); it != _inboundCalls.end()) {
                    hangup_call(it->second.get(), PJSIP_SC_DECLINE);
                } 
                else if (auto it = _outboundCalls.find(call_id); it != _outboundCalls.end()) {
                    hangup_call(it->second.get(), PJSIP_SC_REQUEST_TERMINATED);
                } 
                else if (auto it = _activeCalls.find(call_id); it != _activeCalls.end()) {
                    hangup_call(it->second.get(), PJSIP_SC_BUSY_HERE);
                }
        
            } catch (const Error& e) {
                _handleError(e);
                throw;
            }
        }

        void get_call_info(const string& call_id, CallData* out_data){
            lock_guard<mutex> lock(_mutex);

            const initializer_list<const decltype(_activeCalls)*> maps = {
                &_activeCalls,
                &_inboundCalls, 
                &_outboundCalls
            };

            for (const auto& map : maps) {
                if (auto it = map->find(call_id); it != map->end()) {
                    const CallInfo& ci = it->second->getInfo();
                    
                    // Copy data với buffer size checking
                    strncpy(out_data->call_id, ci.callIdString.c_str(), sizeof(out_data->call_id));
                    strncpy(out_data->remote_uri, ci.remoteUri.c_str(), sizeof(out_data->remote_uri));
                    strncpy(out_data->local_uri, ci.localUri.c_str(), sizeof(out_data->local_uri));
                    strncpy(out_data->actual_state, ci.stateText.c_str(), sizeof(out_data->actual_state));
                    
                    // Đảm bảo null-terminated
                    out_data->call_id[sizeof(out_data->call_id)-1] = '\0';
                    out_data->remote_uri[sizeof(out_data->remote_uri)-1] = '\0';
                    out_data->local_uri[sizeof(out_data->local_uri)-1] = '\0';
                    out_data->actual_state[sizeof(out_data->actual_state)-1] = '\0';
                    return;
                }
            }

            memset(out_data, 0, sizeof(CallData));
        }
}


extern "C"{
    PJSUA2ManagerPtr pjsua2_manager_create(
        const char* sip_user,
        const char* sip_password,
        const char* sip_domain,
        DartIncomingCallStateCb incomingCallCb = nullptr,
        DartOnRegStateCb onRegStateCb = nullptr,
        DartCallStateCb callStateCb = nullptr,
        DartOnErrorCb onErrorCb = nullptr
    ){
        PJSUA2Manager* manager = new PJSUA2Manager(
            sip_user,
            sip_password,
            sip_domain,
            incomingCallCb,
            onRegStateCb,
            callStateCb,
            onErrorCb
        );
        return static_cast<PJSUA2ManagerPtr>(manager);
    };

    void pjsua2_manager_destroy(PJSUA2ManagerPtr mgr){
        PJSUA2Manager* manager = static_cast<PJSUA2Manager*>(mgr);
        delete manager;
    }


}