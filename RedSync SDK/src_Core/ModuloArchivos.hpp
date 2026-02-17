#ifndef MODULO_ARCHIVOS_HPP
#define MODULO_ARCHIVOS_HPP

#include "RedCodeCore.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

// Importamos la memoria global para que el módulo pueda consultar variables
extern vector<Contexto> pila_memoria;

class ModuloArchivos {
public:
    static void cargar() {
        
        // --- CREAR ARCHIVO ---
        modulos_registrados["archivos.crear"] = [](string args_raw) {
            auto args = parsear(args_raw);
            if (args.empty()) return;

            // Resolvemos si el nombre es una variable o texto directo
            string nombre_final = resolver_string(args[0]);
            
            ifstream check(nombre_final);
            if (check.good()) return; 
            
            ofstream archivo(nombre_final);
            archivo << "{\n}";
            archivo.close();
        };

        // --- ESCRIBIR (CON RESOLUCIÓN TOTAL) ---
        modulos_registrados["archivos.escribir"] = [](string args_raw) {
            auto args = parsear(args_raw);
            if (args.size() < 3) return;

            // 1. Resolvemos todos los parámetros
            string nombre_final = resolver_string(args[0]); // ¿partida.json o archivo_log?
            string clave = args[1];                        // La clave siempre es literal
            string valor_final = resolver_string(args[2]); // ¿68000 o btc_ahora?

            // 2. Leer datos existentes para no sobrescribir todo el archivo
            vector<pair<string, string>> datos;
            bool actualizado = false;

            ifstream lectura(nombre_final);
            if (lectura.is_open()) {
                string linea;
                while (getline(lectura, linea)) {
                    size_t pos = linea.find(":");
                    if (pos != string::npos) {
                        string c = limpiar_total(linea.substr(0, pos));
                        string v = limpiar_total(linea.substr(pos + 1));
                        
                        if (!c.empty()) {
                            if (c == clave) { 
                                v = valor_final; 
                                actualizado = true; 
                            }
                            datos.push_back({c, v});
                        }
                    }
                }
                lectura.close();
            }

            if (!actualizado) datos.push_back({clave, valor_final});

            // 3. Reescritura física
            ofstream escritura(nombre_final, ios::trunc);
            if (!escritura.is_open()) return;

            escritura << "{\n";
            for (size_t i = 0; i < datos.size(); i++) {
                escritura << "    \"" << datos[i].first << "\": ";
                if (es_numerico(datos[i].second)) {
                    escritura << datos[i].second;
                } else {
                    escritura << "\"" << datos[i].second << "\"";
                }
                if (i < datos.size() - 1) escritura << ",";
                escritura << "\n";
            }
            escritura << "}";
            escritura.flush(); 
            escritura.close();
        };

        // --- LEER ---
        modulos_registrados["archivos.leer"] = [](string args_raw) {
            auto args = parsear(args_raw);
            if (args.size() < 3) return;
            
            string nombre_final = resolver_string(args[0]);
            string clave = args[1];
            string var_dest = args[2];

            ifstream archivo(nombre_final);
            string linea;
            while (getline(archivo, linea)) {
                size_t pos = linea.find(":");
                if (pos != string::npos) {
                    if (limpiar_total(linea.substr(0, pos)) == clave) {
                        string v = limpiar_total(linea.substr(pos + 1));
                        if (es_numerico(v)) set_vn(var_dest, stod(v));
                        else set_vt(var_dest, v);
                        return;
                    }
                }
            }
        };

        // --- INSPECCIONAR ---
        modulos_registrados["archivos.inspeccionar"] = [](string args_raw) {
            auto args = parsear(args_raw);
            if (args.empty()) return;
            string nombre_final = resolver_string(args[0]);
            
            ifstream f(nombre_final); 
            string l;
            cout << "--- CONTENIDO JSON (" << nombre_final << ") ---" << endl;
            if(!f.is_open()) cout << "[ERROR] No se pudo abrir el archivo." << endl;
            while(getline(f, l)) cout << l << endl;
            cout << "------------------------------------" << endl;
        };
    }

private:
    // Función auxiliar para saber si un string es una variable o un literal
    static string resolver_string(string input) {
        if (pila_memoria.empty()) return input;
        auto& mem = pila_memoria.back();

        // Si es variable numérica (vn)
        if (mem.vn.count(input)) {
            string val = to_string(mem.vn[input]);
            val.erase(val.find_last_not_of('0') + 1, string::npos);
            if (val.back() == '.') val.pop_back();
            return val;
        }
        // Si es variable de texto (vt)
        if (mem.vt.count(input)) {
            return mem.vt[input];
        }
        // Si no es ninguna, es un texto literal
        return input;
    }

    static string limpiar_total(string s) {
        size_t f = s.find_first_not_of(" \t\n\r\",{}");
        size_t l = s.find_last_not_of(" \t\n\r\",{}");
        if (f == string::npos) return "";
        return s.substr(f, l - f + 1);
    }

    static vector<string> parsear(string raw) {
        vector<string> res; 
        stringstream ss(raw); 
        string item;
        while(getline(ss, item, ',')) {
            size_t f = item.find_first_not_of(" "), l = item.find_last_not_of(" ");
            if (f != string::npos) {
                string t = item.substr(f, l - f + 1);
                string out = "";
                for(char c : t) if(c != '\"' && c != '\'') out += c;
                res.push_back(out);
            }
        }
        return res;
    }

    static bool es_numerico(const string& s) {
        if (s.empty()) return false;
        bool punto = false;
        for(size_t i = 0; i < s.length(); i++) {
            if (i == 0 && s[i] == '-') continue;
            if (s[i] == '.') { if(punto) return false; punto = true; }
            else if (!isdigit(s[i])) return false;
        }
        return true;
    }
};

#endif