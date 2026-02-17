#ifndef MODULO_SISTEMA_HPP
#define MODULO_SISTEMA_HPP

#include "RedCodeCore.hpp"
#include <iostream>
#include <thread> // Para el sleep
#include <chrono> // Para el tiempo

class ModuloSistema {
public:
    static void cargar() {
        
        // --- 1. LIMPIAR PANTALLA ---
        modulos_registrados["sistema.limpiar"] = [](string args) {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
        };

        // --- 2. ESPERAR (SLEEP) ---
        modulos_registrados["sistema.esperar"] = [](string args) {
            // Limpiamos el argumento (por si viene con comillas o espacios)
            string limpia = "";
            for(char c : args) if(isdigit(c)) limpia += c;
            
            if(!limpia.empty()){
                int ms = stoi(limpia);
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        };

    }
};

#endif