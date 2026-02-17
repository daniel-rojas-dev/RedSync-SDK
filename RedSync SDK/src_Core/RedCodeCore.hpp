#ifndef REDCODECORE_HPP
#define REDCODECORE_HPP

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>

using namespace std;

// --- ESTRUCTURAS COMPARTIDAS ---

// 1. Estructura para guardar información de funciones (ESTO FALTABA)
struct InfoFuncion {
    int linea_inicio;
    vector<string> parametros;
};

// 2. Estructura de la memoria (Contexto)
struct Contexto {
    map<string, double> vn;          // Variables numéricas
    map<string, string> vt;          // Variables de texto
    map<string, vector<double>> ln;  // Listas numéricas
    map<string, vector<string>> lt;  // Listas texto
    map<int, int> contadores_bucle;
};

// --- DECLARACIONES EXTERNAS (PROMESAS) ---
// Estas variables existen realmente en main.cpp, aquí solo las anunciamos.

extern vector<string> script;
extern map<int, int> saltos;
extern map<string, InfoFuncion> funciones;
extern vector<Contexto> pila_memoria; 
extern map<string, function<void(string)>> modulos_registrados;

// Funciones clave para que los plugins escriban en memoria
extern void set_vt(const string& nombre, const string& val, bool forzar_local = false);
extern void set_vn(const string& nombre, double val, bool forzar_local = false);
extern void cargar_puente_web();

#endif