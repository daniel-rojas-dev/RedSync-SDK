#ifndef MODULO_RANDOM_HPP
#define MODULO_RANDOM_HPP

#include "RedCodeCore.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <random> // El motor moderno

using namespace std;

extern vector<Contexto> pila_memoria; 

class ModuloRandom {
private:
    // Generador de alta calidad sembrado con hardware (random_device)
    static mt19937& get_engine() {
        static random_device rd; 
        static mt19937 engine(rd());
        return engine;
    }

public:
    static void cargar() {
        // --- 1. RANDOM NUMERO (Saltos erráticos y grandes) ---
        modulos_registrados["random.numero"] = [](string args_raw) {
            auto args = parsear(args_raw);
            if (args.size() < 3) return;

            string var = args[0];
            // Usamos long long para evitar cualquier residuo decimal
            long long v_min = stoll(args[1]);
            long long v_max = stoll(args[2]);

            uniform_int_distribution<long long> dist(v_min, v_max);
            
            // Guardamos como double porque tu Core lo requiere, 
            // pero el valor es un entero puro (ej: 500.00000)
            set_vn(var, (double)dist(get_engine()));
        };

        // --- 2. RANDOM ELEGIR ---
        modulos_registrados["random.elegir"] = [](string args_raw) {
            auto args = parsear(args_raw);
            if (args.size() < 2) return;

            string var_dest = args[0];
            string nombre_lista = args[1];
            auto& memoria = pila_memoria.back();

            if (memoria.ln.count(nombre_lista)) {
                auto& lista = memoria.ln[nombre_lista];
                if (lista.empty()) return;
                uniform_int_distribution<int> dist(0, (int)lista.size() - 1);
                set_vn(var_dest, lista[dist(get_engine())]);
            }
            else if (memoria.lt.count(nombre_lista)) {
                auto& lista = memoria.lt[nombre_lista];
                if (lista.empty()) return;
                uniform_int_distribution<int> dist(0, (int)lista.size() - 1);
                set_vt(var_dest, lista[dist(get_engine())]);
            }
        };

    }

private:
    static vector<string> parsear(string raw) {
        vector<string> res;
        stringstream ss(raw);
        string item;
        while(getline(ss, item, ',')) res.push_back(limpiar(item));
        return res;
    }

    static string limpiar(string s) {
        string out = "";
        for(char c : s) if(c != '\"' && c != '\'' && c > 32) out += c;
        return out;
    }
};

#endif