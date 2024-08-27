#ifndef connectionManager_h
#define connectionManager_h
#include <WString.h>
#include <ArduinoJson.h>

class connectionManager
{
  public:
    void setup(const char* _ssid, const char* _password, const String _deviceId, const String _deviceKey, void _onMessage(DynamicJsonDocument message));
    void loop();
    void send(String _string);
    bool isConnected();
    bool isAuthenticated();
  private:
    String version = "1.0";
    String serverIP = "192.168.178.92"; //"192.168.0.190"; //192.168.0.158
    int serverPort = 8081;
    int heartbeatFrequency = 10000; // ms
};

#endif
