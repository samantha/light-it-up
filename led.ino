///
/// @file
/// @brief ESP32 LED web server
///

#include "Arduino.h"
#include <WiFi.h>

constexpr auto ssid           {"SSID"};                         // network credentials
constexpr auto password       {"PASSWORD"};
constexpr auto led1Pin        {26};                             // LED for on / off
constexpr auto led2Pin        {27};                             // LED for pwm
constexpr auto led2PwmChannel {0};

static WiFiServer server(80);                                   // web server
static String     header      {};                               // store HTTP request
static auto       led1State   {false};

constexpr auto webHeader
{
    "<!DOCTYPE html><html>\n"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    "<LINK rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'/>\n"
    "<link rel=\"icon\" href=\"data:,\">\n"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n"
    ".button { background-color: #000; border: none; color: white; padding: 16px 40px;\n"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\n"
    ".button2 {background-color: #ade6e6; color: black;}\n"
    ".slider {width: 300px; height: 50px;}</style></head>\n"
    "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>\n"
    "<body><h1>All of the Lights <i class='far fa-lightbulb'></i></h1>\n"
};

constexpr auto webFooter
{
    "<script>var slider = document.getElementById(\"ledSlider\");\n"
    "var ledP = document.getElementById(\"ledPos\"); ledP.innerHTML = slider.value;\n"
    "slider.oninput = function() { slider.value = this.value; ledP.innerHTML = this.value; }\n"
    "setInterval(function() { led(slider.value); }, 100); $.ajaxSetup({timeout:2000}); function led(pos) { \n"
    "$.get(\"/?value=\" + pos + \"&\");{Connection: close};\n"
    "}</script>\n"
};

void setup()
{
    Serial.begin(115200);
    pinMode(led1Pin, OUTPUT);                                   // configure LED 1 (GPIO)
    digitalWrite(led1Pin, LOW);

    ledcSetup(led2PwmChannel, 5000, 8);                         // configure LED 2 (PWM)
    ledcAttachPin(led2Pin, led2PwmChannel);

    Serial.print("Connecting to " + String{ssid});              // configure WIFI
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected!\nIP address: " + WiFi.localIP());
    server.begin();
}

void loop()
{
    WiFiClient client {server.available()};                     // listen for incoming clients

    if(client)                                                  // when a new client connects
    {
        String currentLine {""};                                // hold incoming data from the client
        while(client.connected())                               // while the client is connected
        {
            if(client.available())                              // if bytes are available
            {
                const char c {client.read()};                   // read a byte
                header += c;

                if(c == '\n')                                   // if the byte is a newline character
                {                                               // if you get two newline characters in a row,
                    if(!currentLine.length())                   // that's the end of the client HTTP request, so send a response:
                    {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        if(header.indexOf("GET /26/on") >= 0)
                        {
                            led1State = true;
                            digitalWrite(led1Pin, HIGH);
                        }
                        else if(header.indexOf("GET /26/off") >= 0)
                        {
                            led1State = false;
                            digitalWrite(led1Pin, LOW);
                        }

                        const auto led2Pwm {String(ledcRead(led2PwmChannel))};
                        const auto led1Cmd {String{led1State ? "off" : "on"}};

                        client.println(webHeader);
                        client.print("<p>GPIO 26 - State " + led1Cmd + "</p>");    // info for LED1
                        client.println("<p><a href=\"/26/" + led1Cmd + "\"><button class=\"button" + String{led1State ? "" : " button2"} + "\">" + led1Cmd + "</i></button></a></p>");
                        client.println("<p>GPIO 27 - Brightness: <p id=ledPos>" + led2Pwm + "</p></p>");
                        client.println("<input type=\"range\" min=\"0\" max=\"255\" class=\"slider\" id=\"ledSlider\" onchange=\"led(this.value)\" value=\"" + led2Pwm + "\"/>");
                        client.println(webFooter);

                        if(header.indexOf("GET /?value=")>=0)
                        {
                            const auto pos1 {header.indexOf('=')};
                            const auto pos2 {header.indexOf('&')};
                            const auto val  {header.substring(pos1 + 1, pos2)};
                            ledcWrite(led2PwmChannel, val.toInt());
                        }

                        client.println("</body></html>\n");
                        break;
                    }
                    else                                        // if you get a newline, then clear currentLine
                    {
                        currentLine = "";
                    }
                }
                else if(c != '\r')
                {                                               // if you got anything else but a carriage return character,
                    currentLine += c;                           // add it to the end of the currentLine
                }
            }
        }

        header = "";
        client.stop();
    }
}
