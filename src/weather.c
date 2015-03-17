/*
 * Copyright (C) 2014 Ted Meyer
 *
 * see LICENSING for details
 *
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "weather.h"
#include "format.h"
#include "json.h"
#include "network.h"
#include "utils.h"

#define weather_parse_size 4096

static container * jsoncontext = 0;
static fmt_map *formatmap = 0;
static char * weather = 0;
static time_t lastime = 0;

char *get_weather(baritem* item) {
    return get_weather_string(item -> format, item -> source);
}

struct comb {
    char formatID;
    int (*formatter)(int, char *);
};

int _temperature() {
    container * w_main = $(jsoncontext, "main");
    container * temperature = $(w_main, "temp");
    return *(temperature -> number);
}

int _pressure() {
    container * w_main = $(jsoncontext, "main");
    container * pressure = $(w_main, "pressure");
    return *(pressure -> number);
}

int _humidity() {
    container * w_main = $(jsoncontext, "main");
    container * humidity = $(w_main, "humidity");
    return *(humidity -> number);
}

int temperatureK(int place, char * string) {
    return place + sprintf(string+place, "%i", _temperature());
}

int temperatureC(int place, char * string) {
    int tempC = _temperature()-273;
    return place + sprintf(string+place, "%i", tempC);
}

int temperatureF(int place, char * string) {
    int tempC = _temperature()-273;
    int tempF = ((tempC * 9) / 5) + 32;
    return place + sprintf(string+place, "%i", tempF);
}

int pressureHg(int place, char * string) {
    int pressureHg = _pressure();
    return place + sprintf(string+place, "%i", pressureHg);
}

int pressureATM(int place, char * string) {
    int pressureATM = _pressure() / 760;
    return place + sprintf(string+place, "%i", pressureATM);
}

int humidityPC(int place, char * string) {
    int humidity = _humidity();
    return place + sprintf(string+place, "%i", humidity);
}

int dew_point(int place, char * string) {
    int dewpoint = (_humidity() * _temperature()) / 100;
    return place + sprintf(string+place, "%i", dewpoint);
}

int weather_conditions(int place, char * string) {
    container * w_weather = $(jsoncontext, "weather");
    container * first = _(w_weather, 0);
    container * sky = $(first, "main");
    char * sky_condition = sky -> string;
    return place + sprintf(string+place, "%s", sky_condition);
}

fmt_map * getformatmap() {
    if (!formatmap) {
        formatmap = initmap(10);
        fmt_map_put(formatmap, 'K', &temperatureK);
        fmt_map_put(formatmap, 'F', &temperatureF);
        fmt_map_put(formatmap, 'C', &temperatureC);
        fmt_map_put(formatmap, 'W', &weather_conditions);
        fmt_map_put(formatmap, 'A', &pressureATM);
        fmt_map_put(formatmap, 'P', &pressureHg);
        fmt_map_put(formatmap, 'H', &humidityPC);
        fmt_map_put(formatmap, 'D', &dew_point);
    }
    
    return formatmap;
}

// TODO make the location configurable
void update_json_context(char * location) {
    char url[100] = {0};
    snprintf(url, 100, "/data/2.5/weather?q=%s", location);

    char * host = "api.openweathermap.org";
    char * weather_s = malloc(weather_parse_size);

    if (url_to_memory(weather_s, weather_parse_size, url, host, "162.243.44.32")) {
        if (strstr(weather_s, "HTTP/1.1 200")) {
            char * JSON = strstr(weather_s, "{");
            if (jsoncontext) {
                free_container(jsoncontext);
            }
            jsoncontext = from_string(&JSON);
        }
    }
    free(weather_s);
}

void update_weather_string(char * weather_format) {
    if (!weather) {
        weather = malloc(128);
    }
    if (jsoncontext) {
        memset(weather, 0, 128);
        format_string(weather, weather_format, getformatmap());
    }
}

void attempt_update_weather(char * weather_format, char * weather_location) {
    if (lastime == 0) {
        lastime = 1;
    } else if (time(NULL)-lastime > 1800) {
        time(&lastime);
        update_json_context(weather_location);
        update_weather_string(weather_format);
    }
}

char * get_weather_string(char * weather_format, char * weather_location) {
    attempt_update_weather(weather_format, weather_location);
    char * result;

    if (weather) {
        result = strdup(weather);
    } else {
        result = strndup("<<err>>", 8);
    }

    return result;
}

