#include <dataaverage.h>

#include <dht.h>

// Enable debug prints to serial monitor
#define MY_DEBUG
#define MY_NODE_ID 4
#define MY_REPEATER_FEATURE

#include <MyConfigFlea.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>


static const uint8_t ds18b20Pin = A2;
OneWire oneWire(ds18b20Pin); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature dsTemp(&oneWire); // Pass the oneWire reference to Dallas Temperature.
// to hold device address
DeviceAddress exterieurTemperature;
DeviceAddress pompeTemperature;
DeviceAddress bassinTemperature;
uint8_t numberOfTemp = 0;

enum
{
    pinPompeBleue = 6,
    pinPompeVerte = A1,
    PIN_CUVE_BLEUE_HAUT = 2,
    PIN_CUVE_BLEUE_BAS = 3,
    PIN_CUVE_VERTE_HAUT = 4,
    PIN_CUVE_VERTE_BAS = 5
};

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

bool cuvePrincipalePleine = false;

void before()
{
    cuvePrincipalePleine = false;
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
        }
    }
    else
    {
        Serial.println("Pas de DS18B20");
    }
    // Température Exterieur
    exterieurTemperature[0] = 0x28;
    exterieurTemperature[1] = 0xFF;
    exterieurTemperature[2] = 0xB0;
    exterieurTemperature[3] = 0xf2;
    exterieurTemperature[4] = 0x92;
    exterieurTemperature[5] = 0x16;
    exterieurTemperature[6] = 0x05;
    exterieurTemperature[7] = 0x6f;
    // Température pompe
    pompeTemperature[0] = 0x28;
    pompeTemperature[1] = 0xFF;
    pompeTemperature[2] = 0xE7;
    pompeTemperature[3] = 0x52;
    pompeTemperature[4] = 0xC0;
    pompeTemperature[5] = 0x17;
    pompeTemperature[6] = 0x05;
    pompeTemperature[7] = 0xF2;
    // Température bassin
    bassinTemperature[0] = 0x28;
    bassinTemperature[1] = 0xFF;
    bassinTemperature[2] = 0x7C;
    bassinTemperature[3] = 0x05;
    bassinTemperature[4] = 0xB3;
    bassinTemperature[5] = 0x17;
    bassinTemperature[6] = 0x01;
    bassinTemperature[7] = 0x01;
    // On force les 3 températures
    numberOfTemp = 3;
    // On éteint les pompes
    pinMode(pinPompeBleue, OUTPUT);
    digitalWrite(pinPompeBleue, LOW);
    pinMode(pinPompeVerte, OUTPUT);
    digitalWrite(pinPompeVerte, LOW);
}

enum {
    CHILD_ID_TEMP_EXTERIEUR,
    CHILD_ID_TEMP_POMPE,
    CHILD_ID_TEMP_BASSIN,
    CHILD_ID_CUVE_BLEUE_HAUT,
    CHILD_ID_CUVE_BLEUE_BAS,
    CHILD_ID_POMPE_BLEUE,
    CHILD_ID_CUVE_VERTE_HAUT,
    CHILD_ID_CUVE_VERTE_BAS,
    CHILD_ID_POMPE_VERTE,
    CHILD_ID_CUVE_PRINCIPALE_PLEINE
};

void presentation()
{
    Serial.println(chainePresentation);
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo(chainePresentation.c_str(), "1.0");

    // Register all sensors to gateway (they will be created as child devices)
    present(CHILD_ID_TEMP_EXTERIEUR, S_TEMP, "Extérieur");
    present(CHILD_ID_TEMP_POMPE, S_TEMP, "Pompe");
    present(CHILD_ID_TEMP_BASSIN, S_TEMP, "Bassin");
    present(CHILD_ID_POMPE_BLEUE, S_BINARY, "Pompe bleue");
    present(CHILD_ID_CUVE_BLEUE_HAUT, S_BINARY, "Plein cuve Bleue");
    present(CHILD_ID_CUVE_BLEUE_BAS, S_BINARY, "Vide cuve Bleue");
    present(CHILD_ID_POMPE_VERTE, S_BINARY, "Pompe Verte");
    present(CHILD_ID_CUVE_VERTE_HAUT, S_BINARY, "Plein cuve Verte");
    present(CHILD_ID_CUVE_VERTE_BAS, S_BINARY, "Vide cuve Verte");
    present(CHILD_ID_CUVE_PRINCIPALE_PLEINE, S_BINARY, "Plein cuve Principale");
}

void setup()
{
    if (numberOfTemp > 0)
    {
        dsTemp.begin();
        // requestTemperatures() will not block current thread
        dsTemp.setWaitForConversion(false);
    }
    //    // Capteurs de hauteur d'eau
    // Il faut des pulldown externes
    //    pinMode(PIN_CUVE_BLEUE_HAUT, INPUT_PULLUP);
    //    pinMode(PIN_CUVE_BLEUE_BAS, INPUT_PULLUP);
    //    pinMode(PIN_CUVE_VERTE_HAUT, INPUT_PULLUP);
    //    pinMode(PIN_CUVE_VERTE_BAS, INPUT_PULLUP);

}

MyMessage temperatureExterieurMsg(CHILD_ID_TEMP_EXTERIEUR, V_TEMP);
MyMessage temperaturePompeBleueMsg(CHILD_ID_TEMP_POMPE, V_TEMP);
MyMessage temperatureBassinMsg(CHILD_ID_TEMP_BASSIN, V_TEMP);
MyMessage cuveBleueHautMsg(CHILD_ID_CUVE_BLEUE_HAUT, V_STATUS);
MyMessage cuveBleueBasMsg(CHILD_ID_CUVE_BLEUE_BAS, V_STATUS);
MyMessage pompeBleueMsg(CHILD_ID_POMPE_BLEUE, V_STATUS);
MyMessage cuveVerteHautMsg(CHILD_ID_CUVE_VERTE_HAUT, V_STATUS);
MyMessage cuveVerteBasMsg(CHILD_ID_CUVE_VERTE_BAS, V_STATUS);
MyMessage pompeVerteMsg(CHILD_ID_POMPE_VERTE, V_STATUS);
MyMessage cuvePrincipalePleineMsg(CHILD_ID_CUVE_PRINCIPALE_PLEINE, V_STATUS);

static const long cycleMs = 2500;
static const uint8_t cycleCountSize = 6 * (60000 / cycleMs);
DataAverage temperatureExterieurAverage(cycleCountSize);
DataAverage temperaturePompeAverage(cycleCountSize);
DataAverage temperatureBassinAverage(cycleCountSize);

bool processTemperature(float& temperature, const DeviceAddress& device, DataAverage& dataAverage)
{
    bool retour = true;
    temperature = dsTemp.getTempC(device);
    if (!isnan(temperature) && (temperature > -127) && (temperature < 80))
    {
        dataAverage.addSample(temperature);

#ifdef MY_DEBUG
        Serial.print("T : ");
        Serial.println(temperature);
#endif
    }
    else
    {
#ifdef MY_DEBUG
        Serial.print("Erreur T : ");
        Serial.println(temperature);
        retour = false;
#endif
    }
    return retour;
}

void loop()
{
    static const long preReadMs = 1000;
    static bool startup = true;
    static uint8_t cycleCount = 0; // compteur de cycle pour la moyenne 6 minutes

    dsTemp.requestTemperatures();
    sleep(preReadMs);

    float temperatureExterieur;
    if (processTemperature(temperatureExterieur, exterieurTemperature, temperatureExterieurAverage) && startup)
    {
        send(temperatureExterieurMsg.set(temperatureExterieur, 1));
    }

    float temperaturePompe;
    if (processTemperature(temperaturePompe, pompeTemperature, temperaturePompeAverage) && startup)
    {
        send(temperaturePompeBleueMsg.set(temperaturePompe, 1));
    }

    float temperatureBassin;
    if (processTemperature(temperatureBassin, bassinTemperature, temperatureBassinAverage) && startup)
    {
      send(temperatureBassinMsg.set(temperatureBassin, 1));
    }

    bool cuveBleueHaut = digitalRead(PIN_CUVE_BLEUE_HAUT) == HIGH;
    bool cuveBleueBas = digitalRead(PIN_CUVE_BLEUE_BAS) != HIGH;
    bool cuveVerteHaut = digitalRead(PIN_CUVE_VERTE_HAUT) == HIGH;
    bool cuveVerteBas = digitalRead(PIN_CUVE_VERTE_BAS) != HIGH;
    static bool pompeBleueOnMemo = false;
    bool pompeBleueOn = pompeBleueOnMemo;
    static bool pompeVerteOnMemo = false;
    bool pompeVerteOn = pompeVerteOnMemo;
    if (startup)
    {
        send(cuveBleueBasMsg.set(cuveBleueBas, 0));
        send(cuveBleueHautMsg.set(cuveBleueHaut, 0));
        send(pompeBleueMsg.set(pompeBleueOn, 0));
        send(cuveVerteBasMsg.set(cuveVerteBas, 0));
        send(cuveVerteHautMsg.set(cuveVerteHaut, 0));
        send(pompeVerteMsg.set(pompeVerteOn, 0));
        send(cuvePrincipalePleineMsg.set(cuvePrincipalePleine, 0));
    }
    if (cuveBleueHaut && !cuvePrincipalePleine)
    {
        // On alume le pompe
        pompeBleueOn = true;
        digitalWrite(pinPompeBleue, HIGH);
    }
    if (cuveBleueBas || cuvePrincipalePleine)
    {
        // On éteint le pompe
        pompeBleueOn = false;
        digitalWrite(pinPompeBleue, LOW);
    }
    if (pompeBleueOn != pompeBleueOnMemo)
    {
        pompeBleueOnMemo = pompeBleueOn;
        send(pompeBleueMsg.set(pompeBleueOn, 0));
    }
    if (cuveVerteHaut && !cuvePrincipalePleine)
    {
        // On alume le pompe
        pompeVerteOn = true;
        digitalWrite(pinPompeVerte, HIGH);
    }
    if (cuveVerteBas || cuvePrincipalePleine)
    {
        // On éteint le pompe
        pompeVerteOn = false;
        digitalWrite(pinPompeVerte, LOW);
    }
    if (pompeVerteOn != pompeVerteOnMemo)
    {
        pompeVerteOnMemo = pompeVerteOn;
        send(pompeVerteMsg.set(pompeVerteOn, 0));
    }
#ifdef MY_DEBUG
    Serial.print("Haut: lu = ");
    Serial.print(digitalRead(PIN_CUVE_BLEUE_HAUT));
    Serial.print(", plein = ");
    Serial.println(cuveBleueHaut);
    Serial.print("Bas: lu = ");
    Serial.print(digitalRead(PIN_CUVE_BLEUE_BAS));
    Serial.print(", vide = ");
    Serial.println(cuveBleueBas);
    Serial.print("Pompe: ");
    Serial.println(pompeBleueOn);
    Serial.print("Haut: lu = ");
    Serial.print(digitalRead(PIN_CUVE_VERTE_HAUT));
    Serial.print(", plein = ");
    Serial.println(cuveVerteHaut);
    Serial.print("Bas: lu = ");
    Serial.print(digitalRead(PIN_CUVE_VERTE_BAS));
    Serial.print(", vide = ");
    Serial.println(cuveVerteBas);
    Serial.print("Pompe: ");
    Serial.println(pompeVerteOn);
#endif

    if (startup)
    {
        startup = false;
    }

    cycleCount++;
    if (cycleCount == cycleCountSize)
    {
        if (temperatureExterieurAverage.sampleCount() > cycleCountSize / 2)
        {
            send(temperatureExterieurMsg.set(temperatureExterieurAverage.average(), 1));
        }
        if (temperaturePompeAverage.sampleCount() > cycleCountSize / 2)
        {
            send(temperaturePompeBleueMsg.set(temperaturePompeAverage.average(), 1));
        }
        if (temperatureBassinAverage.sampleCount() > cycleCountSize / 2)
        {
            send(temperatureBassinMsg.set(temperatureBassinAverage.average(), 1));
        }
        send(cuveBleueBasMsg.set(cuveBleueBas, 0));
        send(cuveBleueHautMsg.set(cuveVerteHaut, 0));
        send(pompeBleueMsg.set(pompeBleueOn, 0));
        send(cuveVerteBasMsg.set(cuveVerteBas, 0));
        send(cuveVerteHautMsg.set(cuveVerteHaut, 0));
        send(pompeVerteMsg.set(pompeVerteOn, 0));
        send(cuvePrincipalePleineMsg.set(cuvePrincipalePleine, 0));
        cycleCount = 0;
    }
    sleep(cycleMs - preReadMs);
}

void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    if (message.getType() == V_STATUS)
    {
        Serial.println(message.getSensor());
        switch(message.getSensor())
        {
        default:
        case CHILD_ID_CUVE_BLEUE_HAUT:
        case CHILD_ID_CUVE_BLEUE_BAS:
        case CHILD_ID_POMPE_BLEUE:
        case CHILD_ID_CUVE_VERTE_HAUT:
        case CHILD_ID_CUVE_VERTE_BAS:
        case CHILD_ID_POMPE_VERTE:
            break;
        case CHILD_ID_CUVE_PRINCIPALE_PLEINE:
            cuvePrincipalePleine = message.getBool();
#ifdef MY_DEBUG
            Serial.print("cuvePrincipalePleine = ");
            Serial.print(cuvePrincipalePleine);
#endif
            break;
        }
    }
}

