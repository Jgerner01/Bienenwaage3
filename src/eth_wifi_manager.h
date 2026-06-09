// Bienenwaage3 – Ethernet (primär) + WiFi-AP (Fallback/Konfiguration)
// Zustandsautomat: ETH_CONNECTING → ETH_CONNECTED / ETH_RECONNECTING → AP_CONFIG_MODE
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include <ETH.h>
#include "config.h"
#include "storage.h"

class EthWifiManager {
public:
    enum class State {
        ETH_CONNECTING,
        ETH_CONNECTED,
        ETH_RECONNECTING,
        AP_CONFIG_MODE
    };

    void  begin();
    void  loop();

    State       getState()  const { return _state; }
    String      getLocalIp() const;
    bool        isConnected() const { return _state == State::ETH_CONNECTED; }
    const NetworkConfig& getConfig() const { return _cfg; }
    void        applyNetworkConfig(const NetworkConfig& cfg);

private:
    State         _state          = State::ETH_CONNECTING;
    NetworkConfig _cfg;
    unsigned long _stateEnteredMs = 0;
    unsigned long _lastReconnectMs = 0;
    int           _reconnectCount  = 0;

    void _enterState(State s);
    void _startEthConnect();
    void _startApMode();

    // ETH-Ereignisse werden statisch weitergeleitet
    static void _onEthEvent(arduino_event_id_t event, arduino_event_info_t info);
    static EthWifiManager* _instance;
    void _handleEthConnected();
    void _handleEthDisconnected();
};
