#ifndef MODULO_WEB_HPP
#define MODULO_WEB_HPP

#include "RedCodeCore.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio> // Para remove()

using namespace std;

// Necesitamos acceso a la memoria para inyectar las variables que trajo Python
extern vector<Contexto> pila_memoria;

class ModuloWeb {
public:
    static void cargar() {
        
        // --- PASO 1: CARGAR EL PUENTE (Python -> C++) ---
        // Esto se ejecuta AUTOMÁTICAMENTE al arrancar el programa.
        procesar_puente();

        // --- PASO 2: REGISTRAR EL COMANDO (Para que el script no falle) ---
        // Aunque Python ya hizo la petición, el script todavía tiene la línea:
        // web.leer("url", "clave", "var")
        // Aquí definimos qué hace C++ cuando ve esa línea (básicamente, nada, solo validar).
        
        modulos_registrados["web.leer"] = [](string args_raw) {
            // No hacemos la petición HTTP aquí (ya la hizo el IDE).
            // Solo verificamos si la variable llegó bien a la memoria.
            
            auto args = parsear(args_raw);
            if (args.size() < 3) return;
            
            string var_destino = args[2]; // El tercer argumento es la variable
            
            // Verificación visual para el usuario
            if (existe_variable(var_destino)) {
                // Opcional: Descomentar para depurar
                // cout << "[WEB] Dato sincronizado en '" << var_destino << "'" << endl;
            } else {
                cout << "[WEB-ERROR] La variable '" << var_destino << "' no recibio datos." << endl;
                cout << "            Verifica tu conexion o la clave del JSON." << endl;
            }
        };

    }

private:
    // Esta función lee el archivo temporal que dejó Python
    static void procesar_puente() {
        string ruta_puente = "web_bridge.tmp";
        ifstream archivo(ruta_puente);

        if (!archivo.is_open()) {
            // Si no hay archivo, es que el usuario no usó web.leer o falló Python.
            // No pasa nada, seguimos.
            return;
        }

        string linea;
        while (getline(archivo, linea)) {
            // Formato esperado: variable|valor
            size_t separador = linea.find('|');
            
            if (separador != string::npos) {
                string nombre_var = linea.substr(0, separador);
                string valor_raw = linea.substr(separador + 1);

                // Limpieza básica de saltos de línea que a veces quedan
                if (!valor_raw.empty() && valor_raw.back() == '\n') {
                    valor_raw.pop_back();
                }

                // Detectar si es número o texto para guardarlo correctamente en RAM
                if (es_numero(valor_raw)) {
                    set_vn(nombre_var, stod(valor_raw));
                } else {
                    set_vt(nombre_var, valor_raw);
                }
            }
        }

        archivo.close();
        
        // IMPORTANTE: Borrar el puente para no reusar datos viejos en el futuro
        remove(ruta_puente.c_str());
    }

    // --- UTILIDADES ---

    static bool existe_variable(string nombre) {
        if (pila_memoria.empty()) return false;
        auto& mem = pila_memoria.back();
        return (mem.vn.count(nombre) || mem.vt.count(nombre));
    }

    static vector<string> parsear(string raw) {
        vector<string> res;
        stringstream ss(raw);
        string item;
        while(getline(ss, item, ',')) {
            res.push_back(limpiar(item));
        }
        return res;
    }

    static string limpiar(string s) {
        size_t f = s.find_first_not_of(" ");
        size_t l = s.find_last_not_of(" ");
        if (f == string::npos) return "";
        string t = s.substr(f, (l - f + 1));
        string out = "";
        for(char c : t) if(c != '\"' && c != '\'') out += c;
        return out;
    }

    static bool es_numero(const string& s) {
        if (s.empty()) return false;
        try {
            size_t pos;
            stod(s, &pos);
            return pos == s.length(); // Asegura que todo el string sea número
        } catch(...) {
            return false;
        }
    }
};

#endif