/**
 * REDSYNC v3.5.0 - Modulos
 * Main Interpreter File...
 */ 

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <stack>
#include <iomanip>
#include <variant>
#include <optional>
#include <functional>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

// --- IMPORTANTE: CONEXIONES ---
//Nombre de modulos
#include "RedCodeCore.hpp"   // Define Contexto e InfoFuncion
#include "ModuloTiempo.hpp"  
#include "ModuloSistema.hpp"
#include "ModuloRandom.hpp"
#include "ModuloArchivos.hpp"
#include "ModuloWeb.hpp"

using namespace std;

// Estructura local (solo usada en main, no necesita ir al Core)
struct ValorRetorno {
    double n = 0.0;
    string t = "";
    bool es_texto = false;
    bool activo = false; 
};

// --- GLOBALES (Definición Real) ---
// Aquí reservamos la memoria real. RedCodeCore.hpp solo dice que existen (extern).
vector<string> script;
map<int, int> saltos;
map<string, InfoFuncion> funciones; // Ahora InfoFuncion ya es reconocida gracias al include
vector<Contexto> pila_memoria; 
map<string, function<void(string)>> modulos_registrados;

// --- PROTOTIPOS ---
double evaluar_matematica(string expr);
string obtener_texto(string t);
string obtener_texto_simple(string t); 
ValorRetorno ejecutar_bloque(int pc_start, int pc_end = -1);
ValorRetorno invocar_funcion_generica(string nombre, vector<string> args_raw);

// --- HERRAMIENTAS (UTILS) ---

string trim(const string& s) {
    auto first = s.find_first_not_of(" \t\n\r");
    if (string::npos == first) return "";
    auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, (last - first + 1));
}

string a_string_universal(double valor) {
    stringstream ss;
    ss << fixed << setprecision(6) << valor; 
    string s = ss.str();
    size_t decimal = s.find('.');
    if (decimal != string::npos) {
        s.erase(s.find_last_not_of('0') + 1, string::npos);
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

vector<string> split_smart(string s, char delimiter) {
    vector<string> tokens;
    string token;
    bool in_quotes = false;
    int paren_level = 0;
    for (char c : s) {
        if (c == '"') in_quotes = !in_quotes;
        else if (c == '(' && !in_quotes) paren_level++;
        else if (c == ')' && !in_quotes) paren_level--;
        
        if (c == delimiter && !in_quotes && paren_level == 0) {
            tokens.push_back(token);
            token = "";
        } else {
            token += c;
        }
    }
    tokens.push_back(token);
    return tokens;
}

// --- GESTIÓN DE MEMORIA (SCOPE DINÁMICO) ---
// NOTA: set_vt y set_vn se definen aquí, y RedCodeCore.hpp permite que los plugins las vean.

double get_vn(const string& nombre) {
    if (pila_memoria.empty()) return 0.0;
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) {
        if (it->vn.count(nombre)) return it->vn.at(nombre);
    }
    return 0.0;
}

string get_vt(const string& nombre) {
    if (pila_memoria.empty()) return "";
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) {
        if (it->vt.count(nombre)) return it->vt.at(nombre);
    }
    return "";
}

void set_vn(const string& nombre, double val, bool forzar_local) {
    if (pila_memoria.empty()) return;
    if (forzar_local) { pila_memoria.back().vn[nombre] = val; return; }
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) {
        if (it->vn.count(nombre)) { it->vn[nombre] = val; return; }
    }
    pila_memoria.back().vn[nombre] = val;
}

void set_vt(const string& nombre, const string& val, bool forzar_local) {
    if (pila_memoria.empty()) return;
    if (forzar_local) { pila_memoria.back().vt[nombre] = val; return; }
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) {
        if (it->vt.count(nombre)) { it->vt[nombre] = val; return; }
    }
    pila_memoria.back().vt[nombre] = val;
}

vector<double>* get_ln_ptr(const string& nombre) {
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) 
        if (it->ln.count(nombre)) return &it->ln.at(nombre);
    return nullptr;
}
vector<string>* get_lt_ptr(const string& nombre) {
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) 
        if (it->lt.count(nombre)) return &it->lt.at(nombre);
    return nullptr;
}

// --- CORE MATEMÁTICO ---

double aplicar_mat(double a, double b, const string& op) {
    if (op == "+") return a + b;
    if (op == "-") return a - b;
    if (op == "*") return a * b;
    if (op == "/") return (b != 0.0) ? a / b : 0.0;
    if (op == "%") return (b != 0.0) ? fmod(a, b) : 0.0;
    if (op == "^" || op == "**") return pow(a, b);
    return 0.0;
}

int prec_mat(const string& op) {
    if (op == "**" || op == "^") return 3;
    if (op == "*" || op == "/" || op == "%") return 2;
    if (op == "+" || op == "-") return 1;
    return 0;
}

// --- FUNCIONES Y EVALUACIÓN ---

ValorRetorno invocar_funcion_generica(string nombre, vector<string> args_raw) {
    ValorRetorno ret_default;
    if (!funciones.count(nombre)) return ret_default;

    InfoFuncion info = funciones[nombre];
    
    vector<pair<string, string>> args_texto;
    vector<pair<string, double>> args_num;

    for (size_t i = 0; i < info.parametros.size(); i++) {
        if (i < args_raw.size()) {
            string arg = trim(args_raw[i]);
            if (arg.front() == '"' || !get_vt(arg).empty()) {
                args_texto.push_back({info.parametros[i], obtener_texto(arg)});
            } else {
                args_num.push_back({info.parametros[i], evaluar_matematica(arg)});
            }
        }
    }

    Contexto nuevo_frame;
    pila_memoria.push_back(nuevo_frame);
    
    for(const auto& p : args_texto) set_vt(p.first, p.second, true);
    for(const auto& p : args_num) set_vn(p.first, p.second, true);

    ValorRetorno resultado = ejecutar_bloque(info.linea_inicio);
    
    pila_memoria.pop_back();
    resultado.activo = false; 
    return resultado;
}

double obtener_valor_numerico(string token) {
    token = trim(token);
    if (token.empty()) return 0.0;
    
    size_t par_open = token.find('(');
    size_t par_close = token.find_last_of(')');
    if (par_open != string::npos && par_close != string::npos && par_close > par_open) {
        string nombre = trim(token.substr(0, par_open));
        if (funciones.count(nombre)) {
            string args_interior = token.substr(par_open + 1, par_close - par_open - 1);
            vector<string> args = split_smart(args_interior, ',');
            if (args.size() == 1 && trim(args[0]).empty()) args.clear();
            ValorRetorno ret = invocar_funcion_generica(nombre, args);
            return ret.n; 
        }
    }

    size_t bracket_open = token.find('[');
    if (bracket_open != string::npos) {
        string n = trim(token.substr(0, bracket_open));
        size_t bracket_close = token.find(']');
        if (bracket_close != string::npos) {
            string index_expr = token.substr(bracket_open + 1, bracket_close - bracket_open - 1);
            int idx = static_cast<int>(evaluar_matematica(index_expr));
            vector<double>* vec_n = get_ln_ptr(n);
            if (vec_n && idx >= 0 && idx < (int)vec_n->size()) return (*vec_n)[idx];
        }
        return 0.0;
    }

    double val = get_vn(token);
    if (val != 0.0) return val;
    for (auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) 
        if (it->vn.count(token)) return it->vn.at(token);

    try { return stod(token); } catch (...) { return 0.0; }
}

double evaluar_matematica(string expr) {
    string limpia = "";
    for (char c : expr) if (!isspace(c)) limpia += c;
    if (limpia.empty()) return 0.0;

    stack<double> vals;
    stack<string> ops;

    auto procesar_pila = [&]() {
        if (vals.size() < 2) return;
        double v2 = vals.top(); vals.pop();
        double v1 = vals.top(); vals.pop();
        string op = ops.top(); ops.pop();
        vals.push(aplicar_mat(v1, v2, op));
    };

    bool esperando_operando = true;

    for (int i = 0; i < (int)limpia.length(); i++) {
        char c = limpia[i];

        if (c == '(') {
            ops.push("(");
            esperando_operando = true;
        } 
        else if (c == ')') {
            while (!ops.empty() && ops.top() != "(") procesar_pila();
            if (!ops.empty()) ops.pop();
            esperando_operando = false;
        } 
        else if (string("+*/%^").find(c) != string::npos) {
            string op(1, c);
            if (c == '*' && i + 1 < (int)limpia.length() && limpia[i+1] == '*') {
                op = "**";
                i++;
            }
            while (!ops.empty() && ops.top() != "(" && prec_mat(ops.top()) >= prec_mat(op)) procesar_pila();
            ops.push(op);
            esperando_operando = true;
        }
        else if (c == '-') {
            if (esperando_operando) {
                string token = "-";
                i++; 
                int par_bal = 0;
                while (i < (int)limpia.length()) {
                    char next_c = limpia[i];
                    if (next_c == '(') par_bal++;
                    else if (next_c == ')') {
                        if (par_bal == 0) break;
                        par_bal--;
                    }
                    bool es_op = (string("+-*/%^").find(next_c) != string::npos);
                    if (es_op && par_bal == 0 && next_c != 'e') { 
                         if (next_c == '-' || next_c == '+') {
                             char prev = limpia[i-1];
                             if (tolower(prev) != 'e') break;
                         } else { break; }
                    }
                    token += next_c;
                    i++;
                }
                i--; 
                vals.push(obtener_valor_numerico(token));
                esperando_operando = false;
            } else {
                string op = "-";
                while (!ops.empty() && ops.top() != "(" && prec_mat(ops.top()) >= prec_mat(op)) procesar_pila();
                ops.push(op);
                esperando_operando = true;
            }
        }
        else {
            string token = "";
            int par_bal = 0;
            int brack_bal = 0;
            while (i < (int)limpia.length()) {
                char next_c = limpia[i];
                if (next_c == '(') par_bal++;
                else if (next_c == ')') {
                    if (par_bal == 0) break;
                    par_bal--;
                }
                else if (next_c == '[') brack_bal++;
                else if (next_c == ']') brack_bal--;

                bool es_op = (string("+-*/%^").find(next_c) != string::npos);
                if (es_op && par_bal == 0 && brack_bal == 0) {
                     if ((next_c == '+' || next_c == '-') && i > 0) {
                         char prev = limpia[i-1];
                         if (tolower(prev) != 'e') break;
                     } else { break; }
                }
                token += next_c;
                i++;
            }
            i--; 
            vals.push(obtener_valor_numerico(token));
            esperando_operando = false;
        }
    }
    while (!ops.empty()) procesar_pila();
    return vals.empty() ? 0.0 : vals.top();
}

string obtener_texto(string t) {
    t = trim(t);
    if (t.empty()) return "";

    string resultado = "";
    string buffer = "";
    bool en_comillas = false;
    int nivel_par = 0;

    for (size_t i = 0; i < t.length(); i++) {
        char c = t[i];
        if (c == '"') en_comillas = !en_comillas;
        if (c == '(' && !en_comillas) nivel_par++;
        if (c == ')' && !en_comillas) nivel_par--;

        if (c == '+' && !en_comillas && nivel_par == 0) {
            resultado += obtener_texto_simple(buffer);
            buffer = "";
        } else {
            buffer += c;
        }
    }
    resultado += obtener_texto_simple(buffer);
    return resultado;
}

string obtener_texto_simple(string t) {
    t = trim(t);
    if (t.empty()) return "";
    
    if (t.size() >= 2 && t.front() == '\"' && t.back() == '\"') 
        return t.substr(1, t.length() - 2);

    if (t.front() == '(' && t.back() == ')') {
        return a_string_universal(evaluar_matematica(t));
    }

    size_t par_open = t.find('(');
    if (par_open != string::npos && t.back() == ')') {
        string nombre = t.substr(0, par_open);
        if (funciones.count(nombre)) {
             string args_int = t.substr(par_open+1, t.size() - par_open - 2);
             vector<string> args = split_smart(args_int, ',');
             if (args.size()==1 && trim(args[0]).empty()) args.clear();
             ValorRetorno ret = invocar_funcion_generica(nombre, args);
             return ret.es_texto ? ret.t : a_string_universal(ret.n);
        }
    }

    vector<double>* ln = get_ln_ptr(t);
    if (ln) {
        stringstream ss; ss << "[";
        for(size_t i = 0; i < ln->size(); ++i) ss << a_string_universal((*ln)[i]) << (i < ln->size()-1 ? ", " : "");
        ss << "]";
        return ss.str();
    }
    vector<string>* lt = get_lt_ptr(t);
    if (lt) {
        stringstream ss; ss << "[";
        for(size_t i = 0; i < lt->size(); ++i) ss << "\"" << (*lt)[i] << "\"" << (i < lt->size()-1 ? ", " : "");
        ss << "]";
        return ss.str();
    }

    size_t b_open = t.find('[');
    if (b_open != string::npos) {
        string n = trim(t.substr(0, b_open));
        string idx_s = t.substr(b_open+1, t.find(']') - b_open - 1);
        int idx = (int)evaluar_matematica(idx_s);
        vector<double>* vn_ptr = get_ln_ptr(n);
        if (vn_ptr && idx >= 0 && idx < (int)vn_ptr->size()) return a_string_universal((*vn_ptr)[idx]);
        vector<string>* vt_ptr = get_lt_ptr(n);
        if (vt_ptr && idx >= 0 && idx < (int)vt_ptr->size()) return (*vt_ptr)[idx];
    }

    string val_t = get_vt(t);
    if (!val_t.empty()) return val_t;
    for(auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) 
        if(it->vn.count(t)) return a_string_universal(it->vn.at(t));
    
    if (t.find_first_of("0123456789") != string::npos) {
         return a_string_universal(evaluar_matematica(t));
    }
    
    return t; 
}

bool evaluar_comparacion(string cond) {
    cond = trim(cond);
    string ops_c[] = {"==", "!=", ">=", "<=", ">", "<"};
    string op = "";
    size_t pos = string::npos;
    
    for (const string& o : ops_c) {
        pos = cond.find(o);
        if (pos != string::npos) { op = o; break; }
    }
    
    if (pos == string::npos) return evaluar_matematica(cond) != 0.0;

    string lhs = trim(cond.substr(0, pos));
    string rhs = trim(cond.substr(pos + op.length()));

    bool lhs_is_text = (lhs.front() == '"' || !get_vt(lhs).empty());
    if (!lhs_is_text) {
        for(auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) if(it->vt.count(lhs)) lhs_is_text = true;
    }

    if (lhs_is_text) {
        string t1 = obtener_texto(lhs);
        string t2 = obtener_texto(rhs);
        if (op == "==") return t1 == t2;
        if (op == "!=") return t1 != t2;
        return false;
    }

    double v1 = evaluar_matematica(lhs);
    double v2 = evaluar_matematica(rhs);
    
    if (op == "==") return abs(v1 - v2) < 1e-5;
    if (op == "!=") return abs(v1 - v2) > 1e-5;
    if (op == ">")  return v1 > v2;
    if (op == "<")  return v1 < v2;
    if (op == ">=") return v1 >= v2;
    if (op == "<=") return v1 <= v2;
    return false;
}

bool evaluar_condicion_maestra(string expr) {
    string l = ""; 
    for (char c : expr) { 
        if (c == '(') l += " ( "; 
        else if (c == ')') l += " ) "; 
        else l += c; 
    }
    
    stringstream ss(l);
    string t;
    stack<bool> vals;
    stack<string> ops;
    string buffer = "";

    auto resolver = [&]() {
        if (ops.empty()) return;
        string op = ops.top(); ops.pop();
        if (op == "NO") {
            if (vals.empty()) return;
            bool v = vals.top(); vals.pop();
            vals.push(!v);
        } else {
            if (vals.size() < 2) return;
            bool v2 = vals.top(); vals.pop();
            bool v1 = vals.top(); vals.pop();
            if (op == "Y") vals.push(v1 && v2);
            else vals.push(v1 || v2);
        }
    };

    while (ss >> t) {
        string up = t;
        transform(up.begin(), up.end(), up.begin(), ::toupper);
        if (up == "Y" || up == "O" || up == "NO" || t == "(" || t == ")") {
            if (!buffer.empty()) { vals.push(evaluar_comparacion(buffer)); buffer = ""; }
            if (t == "(") ops.push("(");
            else if (t == ")") { while (!ops.empty() && ops.top() != "(") resolver(); if (!ops.empty()) ops.pop(); }
            else {
                int prec_curr = (up == "NO") ? 3 : (up == "Y" ? 2 : 1);
                while (!ops.empty() && ops.top() != "(") {
                    int prec_top = (ops.top() == "NO") ? 3 : (ops.top() == "Y" ? 2 : 1);
                    if (prec_curr <= prec_top) resolver(); else break;
                }
                ops.push(up);
            }
        } else { if (!buffer.empty()) buffer += " "; buffer += t; }
    }
    if (!buffer.empty()) vals.push(evaluar_comparacion(buffer));
    while (!ops.empty()) resolver();
    return vals.empty() ? false : vals.top();
}

//MODULOS

void cargar_modulo_externo(string nombre) {
    nombre = trim(nombre);
    if (nombre.size() >= 2 && nombre.front() == '"' && nombre.back() == '"') 
        nombre = nombre.substr(1, nombre.size() - 2);

    if (nombre == "tiempo") {
        ModuloTiempo::cargar();
    }
    else if (nombre == "sistema") {
        ModuloSistema::cargar();
    }
    else if (nombre == "random") {
        ModuloRandom::cargar();
    }
    else if (nombre == "archivos") {
        ModuloArchivos::cargar();
    }
    else if (nombre == "web") {
        ModuloWeb::cargar();
    }
    else {
        cout << "[ERROR] Modulo '" << nombre << "' no encontrado." << endl;
    }
}

ValorRetorno ejecutar_bloque(int pc_start, int pc_end) {
    ValorRetorno retorno;
    int pc = pc_start;
    int limit = (pc_end == -1) ? (int)script.size() : pc_end;

    while (pc < limit) {
        string instr = script[pc];
        stringstream ss(instr);
        string cmd; ss >> cmd;
        string raw_cmd = cmd; 
        if (cmd.find('(') != string::npos) cmd = cmd.substr(0, cmd.find('('));

        if (cmd == "si" || cmd == "mientras") {
            size_t p1 = instr.find('(');
            size_t p2 = instr.find_last_of(')');
            string cond = (p1 != string::npos && p2 != string::npos) ? instr.substr(p1+1, p2-p1-1) : "";
            if (!evaluar_condicion_maestra(cond)) { pc = saltos[pc]; continue; }
        }
        else if (cmd == "sino") { pc = saltos[pc]; continue; }
        else if (cmd == "si!" || cmd == "funcion!") { /* pass */ }
        else if (cmd == "mientras!") { pc = saltos[pc] - 1; }
        
        else if (cmd == "cada") {
            size_t p1 = instr.find('(');
            size_t p2 = instr.find_last_of(')');
            if (p1 != string::npos && p2 != string::npos) {
                string contenido = instr.substr(p1+1, p2-p1-1);
                stringstream ss_c(contenido);
                string var_iter, en_kw, nombre_lista;
                ss_c >> var_iter >> en_kw >> nombre_lista;
                
                var_iter = trim(var_iter);
                nombre_lista = trim(nombre_lista);

                vector<double>* ln_ptr = get_ln_ptr(nombre_lista);
                vector<string>* lt_ptr = get_lt_ptr(nombre_lista);
                int sz = 0;
                if (ln_ptr) sz = (int)ln_ptr->size();
                else if (lt_ptr) sz = (int)lt_ptr->size();

                if (!pila_memoria.back().contadores_bucle.count(pc)) {
                    pila_memoria.back().contadores_bucle[pc] = 0;
                }

                int idx = pila_memoria.back().contadores_bucle[pc];

                if (idx < sz) {
                    if (ln_ptr) set_vn(var_iter, (*ln_ptr)[idx], true);
                    else if (lt_ptr) set_vt(var_iter, (*lt_ptr)[idx], true);
                } else {
                    pila_memoria.back().contadores_bucle.erase(pc);
                    pc = saltos[pc];
                    continue;
                }
            }
        }
        else if (cmd == "cada!") {
            int ini_pc = saltos[pc];
            if (pila_memoria.back().contadores_bucle.count(ini_pc)) {
                pila_memoria.back().contadores_bucle[ini_pc]++;
            }
            pc = ini_pc - 1;
        }

        else if (cmd == "contar") {
            size_t p1 = instr.find('(');
            size_t p2 = instr.find_last_of(')');
            if (p1 != string::npos && p2 != string::npos) {
                string cont = instr.substr(p1+1, p2-p1-1);
                stringstream ss_c(cont);
                string v, de, ini, a, fin;
                ss_c >> v >> de >> ini >> a >> fin; 
                
                if (!pila_memoria.back().contadores_bucle.count(pc)) {
                    set_vn(v, evaluar_matematica(ini), true); 
                    pila_memoria.back().contadores_bucle[pc] = 1;
                }
                
                if (get_vn(v) > evaluar_matematica(fin)) {
                    pila_memoria.back().contadores_bucle.erase(pc);
                    pc = saltos[pc];
                    continue;
                }
            }
        }
        else if (cmd == "contar!") {
            int ini_pc = saltos[pc];
            string lin_ini = script[ini_pc];
            size_t p1 = lin_ini.find('(');
            stringstream ss_inc(lin_ini.substr(p1+1));
            string v; ss_inc >> v;
            set_vn(v, get_vn(v) + 1);
            pc = ini_pc - 1;
        }

        else if (cmd == "funcion") {
            pc = saltos[pc]; 
            continue; 
        }
        else if (cmd == "retornar") {
            string expr_ret = trim(instr.substr(8)); 
            if (expr_ret.empty()) expr_ret = "0";
            
            bool parece_texto = false;
            if (expr_ret.front() == '"') parece_texto = true;
            else if (!get_vt(expr_ret).empty()) parece_texto = true;
            else {
                for(auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) 
                    if(it->vt.count(expr_ret)) parece_texto = true;
            }

            if (!parece_texto && expr_ret.find('"') != string::npos && expr_ret.find('+') != string::npos) {
                parece_texto = true;
            }

            if (parece_texto) {
                retorno.t = obtener_texto(expr_ret); 
                retorno.es_texto = true;
            } else {
                retorno.n = evaluar_matematica(expr_ret);
                retorno.es_texto = false;
            }
            retorno.activo = true;
            return retorno; 
        }

        else if (cmd == "importar") {
            size_t p1 = instr.find('(');
            size_t p2 = instr.find_last_of(')');
            if (p1 != string::npos && p2 != string::npos) {
                string mod_nombre = instr.substr(p1+1, p2-p1-1);
                cargar_modulo_externo(mod_nombre);
            }
        }
        else if (modulos_registrados.count(cmd)) {
            string args_mod = "";
            size_t p1 = instr.find('(');
            size_t p2 = instr.find_last_of(')');
            if (p1 != string::npos && p2 != string::npos) {
                args_mod = instr.substr(p1+1, p2-p1-1);
            }
            modulos_registrados[cmd](args_mod);
        }

        else if (cmd == "mostrar") {
            size_t p1 = instr.find('(');
            size_t p2 = instr.find_last_of(')');
            string contenido = instr.substr(p1+1, p2-p1-1);
            cout << obtener_texto(contenido) << endl; 
        }

        else if (cmd == "ln" || cmd == "lt") {
            string nombre, eq, val;
            ss >> nombre >> eq;
            getline(ss, val);
            val = trim(val);
            size_t b1 = val.find('['), b2 = val.find(']');
            if (b1 != string::npos && b2 != string::npos) {
                string interno = val.substr(b1+1, b2-b1-1);
                vector<string> partes = split_smart(interno, ',');
                if (cmd == "ln") {
                    vector<double> v;
                    for (const auto& p : partes) if(!trim(p).empty()) v.push_back(evaluar_matematica(p));
                    pila_memoria.back().ln[nombre] = v;
                } else {
                    vector<string> v;
                    for (const auto& p : partes) if(!trim(p).empty()) v.push_back(obtener_texto(p));
                    pila_memoria.back().lt[nombre] = v;
                }
            }
        }
        else if (raw_cmd.find('.') != string::npos) {
            size_t dot = raw_cmd.find('.');
            string var = raw_cmd.substr(0, dot);
            string metodo = raw_cmd.substr(dot+1);
            size_t p1 = instr.find('(');
            if (p1 != string::npos) metodo = raw_cmd.substr(dot+1, p1 - dot - 1);
            string arg_str = "";
            size_t p2 = instr.find_last_of(')');
            if (p1 != string::npos && p2 != string::npos) arg_str = instr.substr(p1+1, p2-p1-1);

            vector<double>* ln_ptr = get_ln_ptr(var);
            vector<string>* lt_ptr = get_lt_ptr(var);

            if (ln_ptr) {
                if (metodo == "agregar") ln_ptr->push_back(evaluar_matematica(arg_str));
                else if (metodo == "eliminar") {
                    int idx = (int)evaluar_matematica(arg_str);
                    if (idx >= 0 && idx < (int)ln_ptr->size()) ln_ptr->erase(ln_ptr->begin() + idx);
                }
                else if (metodo == "ordenar") sort(ln_ptr->begin(), ln_ptr->end());
                else if (metodo == "invertir") reverse(ln_ptr->begin(), ln_ptr->end());
            } 
            else if (lt_ptr) {
                if (metodo == "agregar") lt_ptr->push_back(obtener_texto(arg_str));
                else if (metodo == "eliminar") {
                    int idx = (int)evaluar_matematica(arg_str);
                    if (idx >= 0 && idx < (int)lt_ptr->size()) lt_ptr->erase(lt_ptr->begin() + idx);
                }
                else if (metodo == "ordenar") sort(lt_ptr->begin(), lt_ptr->end());
                else if (metodo == "invertir") reverse(lt_ptr->begin(), lt_ptr->end());
            }
        }

        else if (instr.find('=') != string::npos && instr.find("==") == string::npos) {
            string lhs_full = trim(instr.substr(0, instr.find('=')));
            string rhs_full = trim(instr.substr(instr.find('=')+1));
            string nombre_var = lhs_full;
            bool es_nueva_vn = false, es_nueva_vt = false;
            
            stringstream ss_l(lhs_full);
            string temp; ss_l >> temp;
            if (temp == "vn") { es_nueva_vn = true; ss_l >> nombre_var; }
            else if (temp == "vt") { es_nueva_vt = true; ss_l >> nombre_var; }

            if (rhs_full.find("entrada") == 0) {
                size_t p1 = rhs_full.find('(');
                size_t p2 = rhs_full.find_last_of(')');
                if (p1 != string::npos && p2 != string::npos) {
                    string msg = rhs_full.substr(p1+1, p2-p1-1);
                    cout << obtener_texto(msg); 
                    string input_usr;
                    getline(cin, input_usr);
                    if (es_nueva_vn || pila_memoria.back().vn.count(nombre_var)) {
                        try { set_vn(nombre_var, stod(input_usr), es_nueva_vn); } catch(...) { set_vn(nombre_var, 0, es_nueva_vn); }
                    } else {
                        set_vt(nombre_var, input_usr, es_nueva_vt);
                    }
                }
            } 
            else {
                bool es_texto = false;
                if (es_nueva_vt || rhs_full.find('"') != string::npos) es_texto = true;
                else {
                    for(auto it = pila_memoria.rbegin(); it != pila_memoria.rend(); ++it) if(it->vt.count(nombre_var)) es_texto = true;
                }

                if (es_texto) {
                    set_vt(nombre_var, obtener_texto(rhs_full), es_nueva_vt);
                } else {
                    double res = evaluar_matematica(rhs_full);
                    set_vn(nombre_var, res, es_nueva_vn);
                }
            }
        }
        
        pc++;
    }
    return retorno;
}


// SISTEMA DE SINCRONIZACIÓN WEB (Polling)

void cargar_puente_web() {
    
    string nombre_puente = "web_bridge.tmp";
    int intentos = 0;
    
    // Espera hasta 3 segundos a que Python escriba el archivo
    while (intentos < 30) {
        ifstream test(nombre_puente);
        if (test.good()) {
            test.seekg(0, ios::end);
            if (test.tellg() > 0) break; 
        }
        test.close();
        this_thread::sleep_for(chrono::milliseconds(100));
        intentos++;
    }

    ifstream archivo(nombre_puente);
    if (archivo.is_open()) {
        string linea;
        while (getline(archivo, linea)) {
            size_t sep = linea.find("|");
            if (sep != string::npos) {
                string var = linea.substr(0, sep);
                string val = linea.substr(sep + 1);
                if (!val.empty() && val.back() == '\r') val.pop_back();

                if (!pila_memoria.empty()) {
                    set_vt(var, val); 
                }
            }
        }
        archivo.close();
        remove(nombre_puente.c_str());
    }
}

//MAIN

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(65001); 
    #endif

    string arch = ""; 
    bool modo_web = false;

    // Analizar argumentos
    for (int i = 1; i < argc; i++) {
        string argumento = argv[i];
        if (argumento == "--web") {
            modo_web = true; 
        } 
        else if (arch == "") { 
            arch = argumento;
        }
    }

    if (arch == "") arch = "script.red";

    ifstream f(arch);
    if (!f.is_open()) {
        cout << "Error: No se pudo abrir '" << arch << "'" << endl;
        return 1;
    }

    string lin;
    while (getline(f, lin)) {
        size_t p = lin.find("//");
        if (p != string::npos) lin = lin.substr(0, p);
        lin = trim(lin);
        if (!lin.empty()) script.push_back(lin);
    }
    f.close();

    stack<int> p_si, p_b, p_func;
    
    for (int i = 0; i < (int)script.size(); i++) {
        string raw = script[i];
        stringstream ss_p(raw); 
        string cmd; ss_p >> cmd;
        if (cmd.find('(') != string::npos) cmd = cmd.substr(0, cmd.find('('));

        if (cmd == "si") p_si.push(i);
        else if (cmd == "sino") {
            if (!p_si.empty()) {
                int origen = p_si.top(); p_si.pop();
                saltos[origen] = i + 1; 
                p_si.push(i);
            }
        } 
        else if (cmd == "si!") {
            if (!p_si.empty()) {
                int origen = p_si.top(); p_si.pop();
                saltos[origen] = i; 
            }
        }
        else if (cmd == "mientras" || cmd == "contar" || cmd == "cada") p_b.push(i);
        else if (cmd == "mientras!" || cmd == "contar!" || cmd == "cada!") {
            if (!p_b.empty()) {
                int ini = p_b.top(); p_b.pop();
                saltos[i] = ini;
                saltos[ini] = i + 1;
            }
        }
        else if (cmd == "funcion") {
            p_func.push(i);
            size_t p1 = raw.find('(');
            size_t p2 = raw.find(')');
            if (p1 != string::npos && p2 != string::npos) {
                string parte_nombre = raw.substr(0, p1);
                stringstream ss_n(parte_nombre);
                string f_tag, f_name;
                ss_n >> f_tag >> f_name; 

                string params_raw = raw.substr(p1+1, p2-p1-1);
                vector<string> p_list;
                if (!trim(params_raw).empty()) {
                    vector<string> parts = split_smart(params_raw, ',');
                    for(auto& pt : parts) {
                        stringstream ss_pt(pt);
                        string tipo, p_nom;
                        ss_pt >> tipo; 
                        if (ss_pt >> p_nom) p_list.push_back(p_nom); 
                        else p_list.push_back(tipo); 
                    }
                }
                funciones[f_name] = {i + 1, p_list};
            }
        }
        else if (cmd == "funcion!") {
            if (!p_func.empty()) {
                int ini = p_func.top(); p_func.pop();
                saltos[ini] = i; 
            }
        }
    }

    pila_memoria.push_back(Contexto());
    
    if (modo_web) {
        cargar_puente_web(); 
    }
    
    ejecutar_bloque(0);

    return 0;
}