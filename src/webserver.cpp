#include "main.h"

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static AsyncEventSource events("/events");

static void handleNotFound(AsyncWebServerRequest *request) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";
    for (uint8_t i = 0; i < request->args(); i++) {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }
    request->send(404, "text/plain", message);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        // client connected
        dbg("ws[%s][%u] connect\n", server->url(), client->id());
        String str = get_settings();
        client->printf("%s", str.c_str());
        client->ping();
    } else if (type == WS_EVT_DISCONNECT) {
        // client disconnected
        dbg("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    } else if (type == WS_EVT_ERROR) {
        // error was received from the other end
        dbg("ws[%s][%u] error(%u): %s\n", server->url(), client->id(),
               *((uint16_t *)arg), (char *)data);
    } else if (type == WS_EVT_PONG) {
        // pong message was received (in response to a ping request maybe)
        dbg("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len,
               (len) ? (char *)data : "");
    } else if (type == WS_EVT_DATA) {
        // data packet
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len) {
            // the whole message is in a single frame and we got all of it's
            // data printf("ws[%s][%u] %s-message[%llu]: ", server->url(),
            // client->id(), (info->opcode == WS_TEXT) ? "text" : "binary",
            // info->len);
            if (info->opcode == WS_TEXT) {
                data[len] = 0;
                dbg("data: %s\n", (char *)data);
                //        parse_cmd((char *)data, client);
            }
        }
    }
}

void webserver_setup() {
        // attach AsyncWebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    
    // attach AsyncEventSource
    server.addHandler(&events);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(
            200, "text/html", data_index_html_start,
            data_index_html_end - data_index_html_start - 1);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(
            200, "application/javascript", data_script_js_start,
            data_script_js_end - data_script_js_start - 1);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
    server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request) {
        String output = get_settings();
        AsyncWebServerResponse *response =
            request->beginResponse(200, "application/json", output);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
    server.on(
        "/settings.json", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response =
                request->beginResponse(204, "text/html");
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Methods",
                                "POST,GET,OPTIONS");
            response->addHeader("Access-control-Allow-Credentials", "false");
            response->addHeader("Access-control-Allow-Headers",
                                "x-requested-with");
            response->addHeader("Access-Control-Allow-Headers",
                                "X-PINGOTHER, Content-Type");

            request->send(response);
        });
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/settings.json", [](AsyncWebServerRequest *request, JsonVariant &json) {
        if (!parse_settings(json)) {
            err("deserializeJson failed\n");
            request->send(501, "text/plain", "deserializeJson failed");
        } else {
            String output = get_settings();
            AsyncWebServerResponse *response =
                request->beginResponse(200, "application/json", output);
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
            dbg("/settings.json: post settings done\n");
        }
    });
    server.addHandler(handler);
    server.on("/reboot", [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Connection", "close");
        request->send(response);
        EEPROM.commit();
        sleep(1);
        ESP.restart();
    });
    server.on("/factoryreset", [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        cfg.version = 0xff;
        write_config();
        sleep(1);
        ESP.restart();
    });

    server.onNotFound(handleNotFound);
    server.begin();
}

void ws2All(const char *str) {
    ws.textAll(str);
}
