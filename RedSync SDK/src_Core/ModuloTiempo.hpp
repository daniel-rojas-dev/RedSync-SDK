#ifndef MODULO_TIEMPO_HPP
#define MODULO_TIEMPO_HPP

#include "RedCodeCore.hpp" // Incluye el Core para ver Contexto e InfoFuncion
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

class ModuloTiempo {
public:
    static void cargar() {
        // --- HORA ---
        modulos_registrados["tiempo.hora"] = [](string args) {
            string var, fmt;
            parsear_argumentos(args, var, fmt);
            if (var.empty()) return;

            auto p = obtener_tiempo();
            stringstream ss;
            if (fmt == "H") ss << setfill('0') << setw(2) << p.tm_hour;
            else if (fmt == "HM") ss << setfill('0') << setw(2) << p.tm_hour << ":" << setw(2) << p.tm_min;
            else ss << setfill('0') << setw(2) << p.tm_hour << ":" << setw(2) << p.tm_min << ":" << setw(2) << p.tm_sec;
            
            set_vt(var, ss.str(), false);
        };

        // --- FECHA ---
        modulos_registrados["tiempo.fecha"] = [](string args) {
            string var, fmt;
            parsear_argumentos(args, var, fmt);
            if (var.empty()) return;

            auto p = obtener_tiempo();
            stringstream ss;
            if (fmt == "D") ss << p.tm_mday;
            else if (fmt == "DM") ss << setfill('0') << setw(2) << p.tm_mday << "/" << setw(2) << (p.tm_mon + 1);
            else ss << setfill('0') << setw(2) << p.tm_mday << "/" << setw(2) << (p.tm_mon + 1) << "/" << (p.tm_year + 1900);

            set_vt(var, ss.str(), false);
        };

        // --- AÑO ---
        auto logica_anio = [](string args) {
            string var, trash;
            parsear_argumentos(args, var, trash);
            if (var.empty()) return;
            set_vn(var, (double)(obtener_tiempo().tm_year + 1900), false);
        };

        modulos_registrados["tiempo.anio"] = logica_anio;
        modulos_registrados["tiempo.anho"] = logica_anio;
        modulos_registrados["tiempo.year"] = logica_anio;
        string n_utf8 = "tiempo.a"; n_utf8 += (char)0xC3; n_utf8 += (char)0xB1; n_utf8 += "o";
        modulos_registrados[n_utf8] = logica_anio;

    }

private:
    static void parsear_argumentos(string raw, string& out_var, string& out_fmt) {
        size_t coma = raw.find(',');
        string s_var = (coma == string::npos) ? raw : raw.substr(0, coma);
        string s_fmt = (coma == string::npos) ? "" : raw.substr(coma + 1);
        
        out_var = "";
        for(char c : s_var) if(isalnum((unsigned char)c) || c == '_') out_var += c;
        
        out_fmt = "";
        for(char c : s_fmt) if(isalnum((unsigned char)c)) out_fmt += toupper((unsigned char)c);
    }

    static std::tm obtener_tiempo() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm parts{};
        #ifdef _WIN32
            localtime_s(&parts, &now_c);
        #else
            localtime_r(&now_c, &parts);
        #endif
        return parts;
    }
};
#endif