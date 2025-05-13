#include "pjsua2_manager.hpp"

void PJSUA2Manager::PJSUA2Account::onRegState(OnRegStateParam &prm) {
    AccountInfo info = getInfo();
    if (m_manager._onRegStateCb) {
        m_manager._onRegStateCb(prm.code, info.regIsActive ? "Active" : "Inactive", prm.reason.c_str());
    }
}

void PJSUA2Manager::PJSUA2Account::onIncomingCall(OnIncomingCallParam &prm) {
    if (!m_manager._account) {
        throw Error(
            PJ_EBUG, 
            "Account Error", 
            "Account not initialized", 
            __FILE__, 
            __LINE__
        );
    }
    auto call = make_unique<PJSUA2Manager::PJSUA2Call>(m_manager, *this, prm.callId);
    string callId = call->getInfo().callIdString;
        
    {
        lock_guard<recursive_mutex> lock(m_manager._inboundCallsMutex);
        m_manager._inboundCalls[callId] = move(call);
    }
                    
    if (m_manager._onIncomingCallStateCb) {
        m_manager._onIncomingCallStateCb(callId.c_str());
    }
}

PJSUA2Manager::PJSUA2Call::PJSUA2Call(PJSUA2Manager& manager, Account &acc, int call_id) : Call(acc, call_id), m_manager(manager) {}

void PJSUA2Manager::PJSUA2Call::onCallState(OnCallStateParam &prm) {
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
            {
                lock_guard<recursive_mutex> lock(m_manager._inboundCallsMutex);
                string callId = callInfo.callIdString;

                if (auto it = m_manager._inboundCalls.find(callId); it != m_manager._inboundCalls.end()) {
                    lock_guard<recursive_mutex> lock(m_manager._activeCallsMutex);
                    m_manager._activeCalls[callId] = std::move(it->second);
                    m_manager._inboundCalls.erase(it);
                }
                break;
            }
            {   
                lock_guard<recursive_mutex> lock(m_manager._inboundCallsMutex);
                string callId = callInfo.callIdString;

                if (auto it = m_manager._outboundCalls.find(callId); it != m_manager._outboundCalls.end()) {
                    lock_guard<recursive_mutex> lock(m_manager._activeCallsMutex);
                    m_manager._activeCalls[callId] = std::move(it->second);
                    m_manager._outboundCalls.erase(it);
                    break;
                }
            }
            break;
               

        case PJSIP_INV_STATE_CONFIRMED:
                    break;
        case PJSIP_INV_STATE_DISCONNECTED:
                    string callId = callInfo.callIdString;
                    {
                        lock_guard<recursive_mutex> lock(m_manager._activeCallsMutex);
                        m_manager._activeCalls.erase(callId);
                    }
                    // delete call from all maps
                    {
                        lock_guard<recursive_mutex> lock(m_manager._outboundCallsMutex);
                        m_manager._inboundCalls.erase(callId);
                    }
                    {
                        lock_guard<recursive_mutex> lock(m_manager._inboundCallsMutex);
                        m_manager._outboundCalls.erase(callId);
                    }

                    break;
            }
}

void PJSUA2Manager::PJSUA2Call::onCallMediaState(OnCallMediaStateParam &prm) {
    CallInfo callInfo = getInfo();
    // bind media
    for(unsigned i = 0; i < callInfo.media.size(); i++){
        if(callInfo.media[i].type == PJMEDIA_TYPE_AUDIO){
            try{
                AudioMedia aud_media = getAudioMedia(i);
                // Connect the call audio media to the sound device
                AudDevManager& audioDevManager = Endpoint::instance().audDevManager();
                audioDevManager.refreshDevs();
                aud_media.startTransmit(audioDevManager.getPlaybackDevMedia());
                audioDevManager.getCaptureDevMedia().startTransmit(aud_media);

            }catch(const Error &e){
                // if _onError is binded call them
                m_manager._handle_error(e);
                throw;
            }
            break;
        }
    }
}

PJSUA2Manager::PJSUA2Manager(
    const string& sip_user,
        const string& sip_password,
        const string& sip_domain,
        DartIncomingCallStateCb onIncomingCallCb,
        DartOnRegStateCb onRegStateCb,
        DartCallStateCb onCallStateCb,
        DartOnErrorCb onErrorCb
):  _isRunning(false),_endpoint(nullptr), _account(nullptr){
     // set callbacks
    _onIncomingCallStateCb = onIncomingCallCb;
    _onRegStateCb = onRegStateCb;
    _onCallStateCb = onCallStateCb;
    _onErrorCb = onErrorCb;

    try{
        _endpoint = unique_ptr<Endpoint>(new Endpoint());
       
        
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

         _account = make_unique<PJSUA2Account>(*this);
         _account->create(accCfg);
    }catch (const Error &e){
        _handle_error(e);
         throw;
    }           
}


PJSUA2Manager::~PJSUA2Manager(){
    stop_event_loop();
    _activeCalls.clear();
    if (_endpoint) {
        _endpoint->libDestroy();
    }
}

void PJSUA2Manager::start_event_loop(unsigned timeout_ms){
    _isRunning.store(true, std::memory_order_release);
    _eventThread = thread([this, timeout_ms]() { 
        if(!_endpoint->libIsThreadRegistered()){
            char threadName[16];
            pthread_getname_np(pthread_self(), threadName, sizeof(threadName));
            std::string name = threadName;
            _endpoint->libRegisterThread(threadName);
        }
        while (_isRunning.load(std::memory_order_relaxed)) {
                _handle_events(timeout_ms); 
        }
    });
}

void PJSUA2Manager::_handle_events(unsigned timeout_ms) {
    _endpoint->libHandleEvents(timeout_ms);
}

void PJSUA2Manager::stop_event_loop(){
    _isRunning = false;
    if (_eventThread.joinable()) {
        _eventThread.join();
        _endpoint->libStopWorkerThreads();
    }
}

void PJSUA2Manager::_handle_error(const Error &e){
    if(_onErrorCb){
        _onErrorCb(
            e.title.c_str(),
            e.reason.c_str(),
            e.info(true).c_str(),
            e.srcFile.c_str(),
            e.srcLine
        );
    }else{
        cout << e.title << endl;
        cout <<  e.reason << endl;
        cout << e.info(true) << endl;
        cout << e.srcFile<< endl;
        cout << e.srcLine << endl;
    }
}

string PJSUA2Manager::make_call(const string& dest_uri){
    try{
        auto newCall =make_unique<PJSUA2Call>(*this, *_account, 0);
        CallOpParam prm(true); // Use default call settings
        newCall->makeCall(dest_uri, prm);
        string callId = newCall->getInfo().callIdString;
        lock_guard<recursive_mutex> lock(_outboundCallsMutex);
        _outboundCalls[callId] = move(newCall);
        return callId;
    }catch(const Error &e){
        _handle_error(e);
        throw;
    }
}

void PJSUA2Manager::answer_call(const string& call_id){
    try{
        lock_guard<recursive_mutex> lock(_inboundCallsMutex);
        auto it = _inboundCalls.find(call_id);
        if(it != _inboundCalls.end()){
            CallOpParam prm;
            prm.statusCode = PJSIP_SC_OK;
            it->second.get()->answer(prm);
        }
    }catch(Error &e){
        _handle_error(e);
        throw;
    }
}

void PJSUA2Manager::hang_up_call(const string& call_id){
    try {

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
                    break;
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
        {
            lock_guard<recursive_mutex> lock(_inboundCallsMutex);
            if (auto it = _inboundCalls.find(call_id); it != _inboundCalls.end()) {
                hangup_call(it->second.get(), PJSIP_SC_DECLINE);
            } 
        }
        {
            lock_guard<recursive_mutex> lock(_outboundCallsMutex);
            if (auto it = _outboundCalls.find(call_id); it != _outboundCalls.end()) {
                hangup_call(it->second.get(), PJSIP_SC_REQUEST_TERMINATED);
            } 
        }
        {
            lock_guard<recursive_mutex> lock(_activeCallsMutex);
            if (auto it = _activeCalls.find(call_id); it != _activeCalls.end()) {
                hangup_call(it->second.get(), PJSIP_SC_BUSY_HERE);
            }
        }
       

    } catch (const Error& e) {
        _handle_error(e);
        throw;
    }
}


CallData PJSUA2Manager::get_call_info(const string& call_id){
    CallData result;
    memset(&result, 0, sizeof(result));
    const initializer_list<const decltype(_activeCalls)*> maps = {
        &_activeCalls,
        &_inboundCalls, 
        &_outboundCalls
    };
    for (const auto& map : maps) {
        if (auto it = map->find(call_id); it != map->end()) {
            const CallInfo& ci = it->second->getInfo();
            
            // Copy data with buffer size checking
            strncpy(result.call_id, ci.callIdString.c_str(), sizeof(result.call_id) - 1);
            strncpy(result.remote_uri, ci.remoteUri.c_str(), sizeof(result.remote_uri) - 1);
            strncpy(result.local_uri, ci.localUri.c_str(), sizeof(result.local_uri) - 1);
            strncpy(result.actual_state, ci.stateText.c_str(), sizeof(result.actual_state) - 1);
            
            // Assure null-terminated
            result.call_id[sizeof(result.call_id)-1] = '\0';
            result.remote_uri[sizeof(result.remote_uri)-1] = '\0';
            result.local_uri[sizeof(result.local_uri)-1] = '\0';
            result.actual_state[sizeof(result.actual_state)-1] = '\0';
            
            return result;
        }
    }
    return result;
}
