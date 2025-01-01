#include <dataaverage.h>

#include <dht.h>

// Enable debug prints to serial monitor
#define MY_DEBUG
#define MY_NODE_ID 4

#include <MyConfigFlea.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define MY_REPEATER_FEATURE

static const uint8_t ds18b20Pin = A2;
OneWire oneWire(ds18b20Pin); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature dsTemp(&oneWire); // Pass the oneWire reference to Dallas Temperature.
// to hold device address
DeviceAddress serreTemp;
DeviceAddress pompeTemp;
DeviceAddress bassinTemp;
uint8_t numberOfTemp = 0;

static const uint8_t pinPompe = 9;

String chainePresentation = "Shaton Terrasse";

void printAdresse(DeviceAddress& adresse)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        Serial.print("0x");
        if (adresse[i] < 0x10) Serial.print("0");
        Serial.print(adresse[i], HEX);
        if (i < 7) Serial.print(", ");
    }
}

void before()
{
    // Startup up the OneWire library
    dsTemp.begin();
    dsTemp.setResolution(12);
    numberOfTemp = dsTemp.getDeviceCount();
    if (numberOfTemp > 0)
    {
        for (int i = 0; i < numberOfTemp; i++)
        {
            DeviceAddress adresse;
            if (!dsTemp.getAddress(adresse, i))
            {
                Serial.print("Unable to find address for DS18B20 #");
                Serial.println(i);
            }
            else
            {
                Serial.print("Adresse du DS18B20 #");
                Serial.print(i);
                Serial.print(" = ");
                printAdresse(adresse);
                Serial.println("");
            }
            // cellier = 0x28, 0xFF, 0x28, 0x85, 0xB3, 0x16, 0x03, 0x2A
            /*
            switch(i)
            {
            case serreTempIndex:
                if (!dsTemp.getAddress(serreTemp, i))
                {
                    Serial.println("Unable to find address for DS18B20 cellier");
                }
                break;
            case pompeTempIndex:
                if (!dsTemp.getAddress(pompeTemp, i))
                {
                    Serial.println("Unable to find address for DS18B20 grange");
                }
                break;
            case bassinTempIndex:
                if (!dsTemp.getAddress(bassinTemp, i))
                {
                    Serial.println("Unable to find address for DS18B20 cuve");
                }
                break;
            default:
                Serial.println("Trop de DS18B20");
                break;
            }
*/
        }
    }
    else
    {
        Serial.println("Pas de DS18B20");
    }
    // Température serre
    serreTemp[0] = 0x28;
    serreTemp[1] = 0xFF;
    serreTemp[2] = 0x28;
    serreTemp[3] = 0x85;
    serreTemp[4] = 0xB3;
    serreTemp[5] = 0x16;
    serreTemp[6] = 0x03;
    serreTemp[7] = 0x2A;
    // Température pompe
    pompeTemp[0] = 0x28;
    pompeTemp[1] = 0xFF;
    pompeTemp[2] = 0xE7;
    pompeTemp[3] = 0x52;
    pompeTemp[4] = 0xC0;
    pompeTemp[5] = 0x17;
    pompeTemp[6] = 0x05;
    pompeTemp[7] = 0xF2;
    // Température bassin
    bassinTemp[0] = 0x28;
    bassinTemp[1] = 0xFF;
    bassinTemp[2] = 0x7C;
    bassinTemp[3] = 0x05;
    bassinTemp[4] = 0xB3;
    bassinTemp[5] = 0x17;
    bassinTemp[6] = 0x01;
    bassinTemp[7] = 0x01;
    // On force les 3 températures
    numberOfTemp = 3;
    // On éteint le pompe
    pinMode(pinPompe, OUTPUT);
    digitalWrite(pinPompe, LOW);
}

bool metric = true;

enum {
    CHILD_ID_TEMP_SERRE,
    CHILD_ID_TEMP_POMPE,
    CHILD_ID_TEMP_BASSIN,
    CHILD_ID_VOLUME_HAUT,
    CHILD_ID_VOLUME_BAS,
    CHILD_ID_POMPE
};

enum
{
    PIN_VOLUME_HAUT = A3,
    PIN_VOLUME_BAS = A0
};

void presentation()
{
    Serial.println(chainePresentation);
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo(chainePresentation.c_str(), "1.0");

    // Register all sensors to gateway (they will be created as child devices)
    present(CHILD_ID_TEMP_SERRE, S_TEMP, "Serre");
    present(CHILD_ID_TEMP_POMPE, S_TEMP, "Pompe");
    present(CHILD_ID_TEMP_BASSIN, S_TEMP, "Bassin");
    present(CHILD_ID_POMPE, S_BINARY, "Pompe");
    present(CHILD_ID_VOLUME_HAUT, S_BINARY, "Plein");
    present(CHILD_ID_VOLUME_BAS, S_BINARY, "Vide");
    metric = getControllerConfig().isMetric;
}

//static const uint8_t dhtPin = A3;
//dht1wire dhtGrange(dhtPin, dht::DHT11);
//dht12 dht;

void setup()
{
    if (numberOfTemp > 0)
    {
        dsTemp.begin();
        // requestTemperatures() will not block current thread
        dsTemp.setWaitForConversion(false);
    }
//    // Capteurs de hauteur d'eau
// Il faut des pulldown esternes
//    pinMode(PIN_VOLUME_HAUT, INPUT_PULLUP);
//    pinMode(PIN_VOLUME_BAS, INPUT_PULLUP);

}

MyMessage temperatureSerreMsg(CHILD_ID_TEMP_SERRE, V_TEMP);
MyMessage temperaturePompeMsg(CHILD_ID_TEMP_POMPE, V_TEMP);
MyMessage temperatureBassinMsg(CHILD_ID_TEMP_BASSIN, V_TEMP);
MyMessage volumeHautMsg(CHILD_ID_VOLUME_HAUT, V_STATUS);
MyMessage volumeBasMsg(CHILD_ID_VOLUME_BAS, V_STATUS);
MyMessage pompeMsg(CHILD_ID_POMPE, V_STATUS);

static const uint8_t cycleCountSize = 6 * 20;
DataAverage temperatureSerreAverage(cycleCountSize);
DataAverage temperaturePompeAverage(cycleCountSize);
DataAverage temperatureBassinAverage(cycleCountSize);

void loop()
{
    static const long cycleMs = 2500;
    static const long preReadMs = 1000;
    static bool startup = true;
    static uint8_t cycleCount = 0; // compteur de cycle pour la moyenne 6 minutes

    dsTemp.requestTemperatures();
    sleep(preReadMs);

    float temperatureSerre = dsTemp.getTempC(serreTemp);
    if (!isnan(temperatureSerre) && (temperatureSerre > -127))
    {
        if (!metric)
        {
            temperatureSerre = dsTemp.toFahrenheit(temperatureSerre);
        }
        if (startup)
        {
            send(temperatureSerreMsg.set(temperatureSerre, 1));
        }
        temperatureSerreAverage.addSample(temperatureSerre);

#ifdef MY_DEBUG
        Serial.print("T Serre : ");
        Serial.println(temperatureSerre);
#endif
    }
    else
    {
#ifdef MY_DEBUG
        Serial.println("Erreur T Serre");
#endif
    }

    float temperaturePompe = dsTemp.getTempC(pompeTemp);
    if (!isnan(temperaturePompe) && (temperaturePompe > -127))
    {
        if (!metric)
        {
            temperaturePompe = dsTemp.toFahrenheit(temperaturePompe);
        }
        if (startup)
        {
            send(temperaturePompeMsg.set(temperaturePompe, 1));
        }
        temperaturePompeAverage.addSample(temperaturePompe);

#ifdef MY_DEBUG
        Serial.print("T Pompe: ");
        Serial.println(temperaturePompe);
#endif
    }
    else
    {
#ifdef MY_DEBUG
        Serial.println("Erreur T Pompe");
#endif
    }

    float temperatureBassin = dsTemp.getTempC(bassinTemp);
    if (!isnan(temperatureBassin) && (temperatureBassin > -127))
    {
        if (!metric)
        {
            temperatureBassin = dsTemp.toFahrenheit(temperatureBassin);
        }
        if (startup)
        {
            send(temperatureBassinMsg.set(temperatureBassin, 1));
        }
        temperatureBassinAverage.addSample(temperatureBassin);

#ifdef MY_DEBUG
        Serial.print("T Bassin: ");
        Serial.println(temperatureBassin);
#endif
    }
    else
    {
#ifdef MY_DEBUG
        Serial.println("Erreur T Bassin");
#endif
    }
    bool volumeHaut = digitalRead(PIN_VOLUME_HAUT) == HIGH;
    bool volumeBas = digitalRead(PIN_VOLUME_BAS) != HIGH;
    static bool pompeOnMemo = false;
    bool pompeOn = pompeOnMemo;
    if (startup)
    {
        send(volumeBasMsg.set(volumeBas, 0));
        send(volumeHautMsg.set(volumeHaut, 0));
        send(pompeMsg.set(pompeOn, 0));
    }
    if (volumeHaut)
    {
        // On alume le pompe
        pompeOn = true;
        digitalWrite(pinPompe, HIGH);
    }
    if (volumeBas)
    {
        // On éteint le pompe
        pompeOn = false;
        digitalWrite(pinPompe, LOW);
    }
    if (pompeOn != pompeOnMemo)
    {
        pompeOnMemo = pompeOn;
        send(pompeMsg.set(pompeOn, 0));
    }
#ifdef MY_DEBUG
    Serial.print("Haut: lu = ");
    Serial.print(digitalRead(PIN_VOLUME_HAUT));
    Serial.print(", plein = ");
    Serial.println(volumeHaut);
    Serial.print("Bas: lu = ");
    Serial.print(digitalRead(PIN_VOLUME_BAS));
    Serial.print(", vide = ");
    Serial.println(volumeBas);
    Serial.print("Pompe: ");
    Serial.println(pompeOn);
#endif

    cycleCount++;
    if (cycleCount == cycleCountSize)
    {
        if (startup)
        {
            startup = false;
        }
        if (temperatureSerreAverage.sampleCount() > cycleCountSize / 2)
        {
            send(temperatureSerreMsg.set(temperatureSerreAverage.average(), 1));
        }
        if (temperaturePompeAverage.sampleCount() > cycleCountSize / 2)
        {
            send(temperaturePompeMsg.set(temperaturePompeAverage.average(), 1));
        }
        if (temperatureBassinAverage.sampleCount() > cycleCountSize / 2)
        {
            send(temperatureBassinMsg.set(temperatureBassinAverage.average(), 1));
        }
        send(volumeBasMsg.set(volumeBas, 0));
        send(volumeHautMsg.set(volumeHaut, 0));
        send(pompeMsg.set(pompeOn, 0));
        cycleCount = 0;
    }
    sleep(cycleMs - preReadMs);
}
