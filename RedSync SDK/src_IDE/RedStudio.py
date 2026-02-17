import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import os
import subprocess
import re
import sys
import requests
import re 

# =====================================================
# CONFIGURACIÓN DE RUTAS
# =====================================================
def obtener_ruta_recurso(ruta_relativa):
    """ Obtiene la ruta absoluta para recursos internos (como el icono) """
    try:
        # PyInstaller crea una carpeta temporal y guarda la ruta en _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, ruta_relativa)

# =====================================================
# CONFIGURACIÓN VISUAL (PALETA REDSYNC)
# =====================================================
COLORES = {
    "fondo": "#1E1E1E",          
    "editor_bg": "#121212",      
    "lineas_bg": "#1E1E1E",      
    "lineas_fg": "#606060",      
    "cursor": "#F1C40F",         
    "texto_base": "#F1FAEE",     
    "seleccion": "#264F78",      
    
    # SINTAXIS
    "instruccion": "#F1C40F",    
    "funcion_mod": "#FF8C42",    
    "operador": "#00F5FF",       
    "string": "#48CAE4",         
    "comentario": "#6C757D",     
    
    # UI
    "barra_sup": "#111111",
    "boton_fg": "#F1FAEE",
    "status_bg": "#007ACC",      
    "status_warning": "#F1C40F", 
    "doc_bg": "#1E1E1E",
    
    # AUTOCOMPLETADO
    "auto_bg": "#252526",
    "auto_fg": "#C5C5C5",
    "auto_sel": "#094771",

    # NUEVO: RESALTADO DE LÍNEA
    "linea_act_bg": "#1A1A1A" 
}

KEYWORDS_BASE = ["vn", "vt", "ln", "lt", "si", "sino", "si!", "mientras", "mientras!", 
            "contar", "contar!", "cada", "cada!", "funcion", "retornar", "funcion!", 
            "importar", "entrada", "mostrar", "Y", "O", "NO"]

MODULES_BASE = ["web.leer", "archivos.crear", "archivos.escribir", "archivos.leer", 
           "archivos.inspeccionar", "random.numero", "random.elegir", 
           "sistema.esperar", "sistema.limpiar", "tiempo.hora", "tiempo.fecha", 
           "tiempo.año", "agregar", "eliminar", "ordenar", "invertir"]

# =====================================================
# CLASE PRINCIPAL: REDSYNC STUDIO
# =====================================================
class RedStudio:
    def __init__(self, root):
        self.root = root
        self.archivo_actual = None
        self.texto_modificado = False
        
        # Configuración Ventana - MAXIMIZADA
        self._actualizar_titulo()
        try:
            self.root.state('zoomed') 
        except:
            self.root.geometry("1200x800")
            
        self.root.configure(bg=COLORES["fondo"])
        try: 
            # Ahora usamos la función nueva para que busque el icono interno
            self.root.iconbitmap(obtener_ruta_recurso("Studio.ico")) 
        except: 
            pass

        self.FUENTE = ("Consolas", 12)

        self._crear_interfaz()
        self._configurar_tags()
        self._bindings()
        
        self.sugerencias = []

    def _crear_interfaz(self):
        # 1. BARRA SUPERIOR
        self.barra = tk.Frame(self.root, bg=COLORES["barra_sup"], height=40)
        self.barra.pack(fill=tk.X)
        
        btn_style = {"bg": COLORES["barra_sup"], "fg": COLORES["boton_fg"], "bd": 0, 
                     "activebackground": "#333", "activeforeground": "#FFF", "font": ("Segoe UI", 9)}
        
        tk.Button(self.barra, text=" 📂 Abrir (F1) ", command=self.abrir_archivo, **btn_style).pack(side=tk.LEFT, padx=2)
        tk.Button(self.barra, text=" 📄 Nuevo (F2) ", command=self.nuevo_archivo, **btn_style).pack(side=tk.LEFT, padx=2)
        tk.Button(self.barra, text=" 💾 Guardar (F3) ", command=self.guardar_archivo, **btn_style).pack(side=tk.LEFT, padx=2)
        tk.Button(self.barra, text=" ▶ EJECUTAR (F4) ", command=self.ejecutar_codigo, bg="#2D5A27", fg="#FFF", bd=0, font=("Segoe UI", 9, "bold")).pack(side=tk.LEFT, padx=10)
        tk.Button(self.barra, text=" ❓ Ayuda (F12) ", command=self.abrir_documentacion, **btn_style).pack(side=tk.RIGHT, padx=5)

        # 2. ÁREA CENTRAL (Editor + Líneas)
        self.contenedor = tk.Frame(self.root, bg=COLORES["fondo"])
        self.contenedor.pack(fill=tk.BOTH, expand=True)

        self.lineas = tk.Text(self.contenedor, width=5, bg=COLORES["lineas_bg"], fg=COLORES["lineas_fg"], 
                              font=self.FUENTE, state="disabled", bd=0, padx=5, pady=5)
        self.lineas.pack(side=tk.LEFT, fill=tk.Y)

        self.editor = tk.Text(self.contenedor, bg=COLORES["editor_bg"], fg=COLORES["texto_base"], 
                              font=self.FUENTE, insertbackground=COLORES["cursor"], 
                              selectbackground=COLORES["seleccion"], undo=True, bd=0, padx=5, pady=5)
        self.editor.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        scroll = tk.Scrollbar(self.contenedor, command=self._scroll_sincronizado)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.editor.config(yscrollcommand=scroll.set)
        self.lineas.config(yscrollcommand=scroll.set)

        # 3. LISTBOX DE AUTOCOMPLETADO
        self.lista_box = tk.Listbox(self.editor, bg=COLORES["auto_bg"], fg=COLORES["auto_fg"], 
                                    font=self.FUENTE, selectbackground=COLORES["auto_sel"], 
                                    selectforeground="white", bd=0, highlightthickness=1, 
                                    highlightbackground="#444", activestyle="none")

        # 4. BARRA DE ESTADO
        self.status_bar = tk.Label(self.root, text=" RedSync v3.6 | Listo ", bg=COLORES["status_bg"], 
                                   fg="white", anchor="w", font=("Segoe UI", 9))
        self.status_bar.pack(fill=tk.X, side=tk.BOTTOM)
        self.pos_info = tk.Label(self.status_bar, text="Ln: 1 | Col: 0 ", bg=COLORES["status_bg"], fg="white")
        self.pos_info.pack(side=tk.RIGHT)

    def _configurar_tags(self):
        self.editor.tag_config("instruccion", foreground=COLORES["instruccion"])
        self.editor.tag_config("funcion_mod", foreground=COLORES["funcion_mod"])
        self.editor.tag_config("operador", foreground=COLORES["operador"])
        self.editor.tag_config("string", foreground=COLORES["string"])
        self.editor.tag_config("comentario", foreground=COLORES["comentario"])
        # Tag para resaltar línea actual
        self.editor.tag_config("linea_actual", background=COLORES["linea_act_bg"])

    def _bindings(self):
        self.root.bind("<F1>", lambda e: self.abrir_archivo())
        self.root.bind("<F2>", lambda e: self.nuevo_archivo())
        self.root.bind("<F3>", lambda e: self.guardar_archivo())
        self.root.bind("<F4>", lambda e: self.ejecutar_codigo())
        self.root.bind("<F12>", lambda e: self.abrir_documentacion())
        
        self.editor.bind("<KeyRelease>", self._al_escribir)
        self.editor.bind("<Button-1>", self._al_click)
        
        self.editor.bind("<Up>", self._navegar_lista)
        self.editor.bind("<Down>", self._navegar_lista)
        self.editor.bind("<Tab>", self._insertar_sugerencia)
        self.editor.bind("<Return>", self._handle_enter)
        self.editor.bind("<Escape>", lambda e: self.lista_box.place_forget())

        for par in [('(', ')'), ('[', ']'), ('{', '}'), ('"', '"')]:
            self.editor.bind(par[0], lambda e, p=par: self._cerrar_par(p[0], p[1]))

    def _scroll_sincronizado(self, *args):
        self.editor.yview(*args)
        self.lineas.yview(*args)

    def _al_click(self, event):
        self.lista_box.place_forget()
        self._actualizar_cursor()
        self._resaltar_linea_actual()

    def _al_escribir(self, event=None):
        self._actualizar_numeros_linea()
        self._resaltar_sintaxis()
        self._actualizar_cursor()
        self._resaltar_linea_actual()
        
        # Si el texto cambia, ponemos el asterisco
        if not self.texto_modificado:
            self.texto_modificado = True
            self._actualizar_titulo()
        
        if event and (event.keysym in ["Up", "Down", "Return", "Tab", "Escape", "Shift_L", "Control_L"]):
            return
            
        self._mostrar_autocompletado()

    def _actualizar_titulo(self):
        nombre = os.path.basename(self.archivo_actual) if self.archivo_actual else "Sin Título"
        asterisco = " *" if self.texto_modificado else ""
        self.root.title(f"RedSync Studio v3.6 - {nombre}{asterisco}")

    def _resaltar_linea_actual(self):
        self.editor.tag_remove("linea_actual", "1.0", tk.END)
        self.editor.tag_add("linea_actual", "insert linestart", "insert lineend + 1c")

    def _actualizar_numeros_linea(self):
        lineas = self.editor.get("1.0", "end-1c").count("\n") + 1
        contenido = "\n".join(str(i) for i in range(1, lineas + 1))
        self.lineas.config(state="normal")
        self.lineas.delete("1.0", tk.END)
        self.lineas.insert("1.0", contenido)
        self.lineas.config(state="disabled")

    def _actualizar_cursor(self, event=None):
        try:
            pos = self.editor.index(tk.INSERT)
            linea, col = pos.split('.')
            self.pos_info.config(text=f"Ln: {linea} | Col: {col}   ")
        except: pass

    def _resaltar_sintaxis(self):
        texto = self.editor.get("1.0", tk.END)
        for tag in ["instruccion", "funcion_mod", "operador", "string", "comentario"]:
            self.editor.tag_remove(tag, "1.0", tk.END)

        for m in re.finditer(r'//.*', texto):
            self.editor.tag_add("comentario", f"1.0 + {m.start()} chars", f"1.0 + {m.end()} chars")

        for m in re.finditer(r'".*?"', texto):
            self.editor.tag_add("string", f"1.0 + {m.start()} chars", f"1.0 + {m.end()} chars")

        pattern_kw = r'\b(' + '|'.join(KEYWORDS_BASE) + r')\b'
        for m in re.finditer(pattern_kw, texto):
            self.editor.tag_add("instruccion", f"1.0 + {m.start()} chars", f"1.0 + {m.end()} chars")

        pattern_mod = r'\b(' + '|'.join([m.split('.')[0] for m in MODULES_BASE if "." in m]) + r')\.\w+'
        for m in re.finditer(pattern_mod, texto):
            self.editor.tag_add("funcion_mod", f"1.0 + {m.start()} chars", f"1.0 + {m.end()} chars")

        for m in re.finditer(r'[\=\+\-\*\/\<\>\!]', texto):
            self.editor.tag_add("operador", f"1.0 + {m.start()} chars", f"1.0 + {m.end()} chars")

    def _cerrar_par(self, char, cierre):
        self.editor.insert(tk.INSERT, char + cierre)
        self.editor.mark_set(tk.INSERT, "insert -1 chars")
        return "break"

    def _handle_enter(self, event):
        if self.lista_box.winfo_ismapped():
            return self._insertar_sugerencia(event)
        
        linea_actual = self.editor.get("insert linestart", "insert")
        match = re.match(r'^(\s*)', linea_actual)
        indent = match.group(1) if match else ""
        if linea_actual.strip().endswith("{") or any(linea_actual.strip().startswith(k) for k in ["si", "mientras", "funcion", "cada"]):
            indent += "    " 
        self.editor.insert(tk.INSERT, "\n" + indent)
        self._resaltar_linea_actual()
        return "break"

    # =====================================================
    # 🧠 AUTOCOMPLETADO DINÁMICO
    # =====================================================
    def _obtener_palabra_actual(self):
        try:
            texto = self.editor.get("insert linestart", "insert")
            match = re.search(r'([\w\.]+)$', texto)
            return match.group(1) if match else ""
        except: return ""

    def _escanear_variables(self):
        codigo = self.editor.get("1.0", tk.END)
        return list(set(re.findall(r'\b(?:vn|vt|ln|lt|funcion)\s+([a-zA-Z_]\w*)', codigo)))

    def _mostrar_autocompletado(self):
        palabra = self._obtener_palabra_actual()
        if not palabra or len(palabra) < 1:
            self.lista_box.place_forget()
            return

        vars_usuario = self._escanear_variables()
        pool = list(set(KEYWORDS_BASE + MODULES_BASE + vars_usuario))
        
        if "." in palabra:
            prefijo = palabra.split(".")[0]
            pool = [m for m in MODULES_BASE if m.startswith(prefijo)]
        
        self.sugerencias = sorted([w for w in pool if w.startswith(palabra)])[:10]

        if not self.sugerencias:
            self.lista_box.place_forget()
            return

        bbox = self.editor.bbox(tk.INSERT)
        if bbox:
            x, y, w, h = bbox
            self.lista_box.delete(0, tk.END)
            for s in self.sugerencias:
                self.lista_box.insert(tk.END, s)
            
            self.lista_box.place(x=x, y=y + h + 5, width=200)
            self.lista_box.selection_clear(0, tk.END)
            self.lista_box.selection_set(0)
            self.lista_box.lift()

    def _navegar_lista(self, event):
        if not self.lista_box.winfo_ismapped(): return
        actual = self.lista_box.curselection()
        if not actual: return
        idx = actual[0]
        if event.keysym == "Up": idx = max(0, idx - 1)
        else: idx = min(self.lista_box.size() - 1, idx + 1)
        self.lista_box.selection_clear(0, tk.END)
        self.lista_box.selection_set(idx)
        self.lista_box.see(idx)
        return "break"

    def _insertar_sugerencia(self, event=None):
        if not self.lista_box.winfo_ismapped(): return "break"
        seleccion = self.lista_box.curselection()
        if not seleccion: return "break"
        palabra_completa = self.lista_box.get(seleccion[0])
        palabra_actual = self._obtener_palabra_actual()
        if palabra_actual:
            self.editor.delete(f"insert -{len(palabra_actual)} chars", "insert")
        self.editor.insert("insert", palabra_completa)
        self.lista_box.place_forget()
        self._resaltar_sintaxis()
        return "break"

    # =====================================================
    # OPERACIONES DE ARCHIVO Y EJECUCIÓN
    # =====================================================
    def nuevo_archivo(self):
        if self.texto_modificado:
            if not messagebox.askyesno("RedSync Studio", "¿Descartar cambios no guardados?"): return
        self.editor.delete("1.0", tk.END)
        self.archivo_actual = None
        self.texto_modificado = False
        self._actualizar_titulo()
        self._actualizar_numeros_linea()

    def abrir_archivo(self):
        ruta = filedialog.askopenfilename(filetypes=[("RedSync Script", "*.red"), ("Todos", "*.*")])
        if ruta:
            self.archivo_actual = ruta
            with open(ruta, "r", encoding="utf-8") as f:
                self.editor.delete("1.0", tk.END)
                self.editor.insert("1.0", f.read())
            self.texto_modificado = False
            self._actualizar_titulo()
            self._resaltar_sintaxis()
            self._actualizar_numeros_linea()
            self._resaltar_linea_actual()

    def guardar_archivo(self):
        if not self.archivo_actual:
            self.archivo_actual = filedialog.asksaveasfilename(defaultextension=".red", filetypes=[("RedSync Script", "*.red")])
        if self.archivo_actual:
            with open(self.archivo_actual, "w", encoding="utf-8") as f:
                f.write(self.editor.get("1.0", tk.END))
            self.texto_modificado = False
            self._actualizar_titulo()
            self.status_bar.config(text=" ✔ Guardado con éxito")
            self.root.after(2000, lambda: self.status_bar.config(text=" RedSync v3.6 | Listo "))

    def ejecutar_codigo(self):
        self.guardar_archivo()
        if not self.archivo_actual: return
        
        if getattr(sys, 'frozen', False):
            base_dir = os.path.dirname(sys.executable)
        else:
            base_dir = os.path.dirname(os.path.abspath(__file__))

        exe_core = os.path.join(base_dir, "RedCore.exe")
        ruta_puente = os.path.join(base_dir, "web_bridge.tmp")

        contenido = self.editor.get("1.0", tk.END)
        patron_web = r'web\.leer\s*\(\s*["\']([^"\']+)["\']\s*,\s*["\']([^"\']+)["\']\s*,\s*["\']([^"\']+)["\']\s*\)'
        peticiones = re.findall(patron_web, contenido)

        if peticiones:
            self.status_bar.config(text=" 🌐 Conectando con API...", bg="#CC8800")
            self.root.update()
            
            try:
                with open(ruta_puente, "w", encoding="utf-8") as puente:
                    for url, clave, var_dest in peticiones:
                        try:
                            r = requests.get(url, headers={'User-Agent': 'RedSync-IDE/3.6'}, timeout=7)
                            
                            def buscar_recursivo(obj, k):
                                if isinstance(obj, dict):
                                    if k in obj: return obj[k]
                                    for v in obj.values():
                                        res = buscar_recursivo(v, k)
                                        if res is not None: return res
                                elif isinstance(obj, list):
                                    for item in obj:
                                        res = buscar_recursivo(item, k)
                                        if res is not None: return res
                                return None

                            if r.status_code == 200:
                                # --- PARCHE DE COMPATIBILIDAD ---
                                if clave == "0":
                                    valor = r.text.strip()
                                else:
                                    try:
                                        datos_json = r.json()
                                        valor = buscar_recursivo(datos_json, clave)
                                    except:
                                        valor = r.text.strip()
                                
                                dato_final = str(valor).strip() if valor is not None else "NULL"
                                puente.write(f"{var_dest}|{dato_final}\n")
                                # --------------------------------
                            else:
                                puente.write(f"{var_dest}|NULL\n")
                        except Exception:
                            puente.write(f"{var_dest}|NULL\n")
            except Exception as e:
                messagebox.showerror("Error de Red", f"No se pudo crear el archivo de comunicación:\n{e}")
                return

        self.status_bar.config(text=" ▶ Ejecutando...", bg=COLORES["status_bg"])
        comando = ["cmd", "/K", exe_core, self.archivo_actual]
        
        try:
            subprocess.Popen(
                comando, 
                cwd=base_dir,
                creationflags=subprocess.CREATE_NEW_CONSOLE
            )
        except Exception as e:
            messagebox.showerror("Error", f"No se encontró 'RedCore.exe' en:\n{base_dir}\n\nDetalle: {e}")         

    def abrir_documentacion(self):
        doc_win = tk.Toplevel(self.root)
        doc_win.title("RedSync v3.6 - DOCUMENTACIÓN")
        doc_win.geometry("800x600")
        txt = tk.Text(doc_win, bg="#1A1A1A", fg="#F1FAEE", font=("Consolas", 11), padx=25, pady=25, bd=0)
        txt.pack(fill=tk.BOTH, expand=True)
        manual = """===============================================================================
                RED CODE v3.6 - DOCUMENTACIÓN
===============================================================================

1. VARIABLES Y ASIGNACIÓN
----------------------------------------------------------------------

vn [nombre] = [expresión] -> Crea/asigna variable numérica (double).
vt [nombre] = "[texto]"    -> Crea/asigna variable de texto (string).

* Soporte Unario: vn temp = -5 + (-10)
* Soporte Matemático: +, -, *, /, %, ** (potencia), ()
* Reasignación: Una vez creada, usa 'nombre = valor' sin prefijo.

2. ENTRADA Y SALIDA (I/O)
----------------------------------------------------------------------

mostrar([contenido])      -> Imprime en consola. 
entrada("[mensaje]")      -> Pausa y espera datos.

3. LISTAS (ARRAYS) Y MÉTODOS
----------------------------------------------------------------------

ln [nombre] = [v1, v2, ...] -> Lista numérica.
lt [nombre] = ["t1", "t2"]  -> Lista de texto.

MÉTODOS:

   [lista].agregar([valor])    
   [lista].eliminar([índice])  
   [lista].ordenar()            
   [lista].invertir()           

4. FUNCIONES
----------------------------------------------------------------------

   funcion [nombre](param1, param2)
      // Código que hace algo
      retornar [valor]
   funcion!

COMPONENTES:
   * nombre: El identificador para llamar a la función luego.
   * parámetros: Variables que la función recibe para trabajar.
   * retornar: Envía un resultado de vuelta a quien llamó la función.
   * funcion!: Cierre obligatorio del bloque.

Ejemplo Práctico:
   funcion calcular_iva(precio)
      vn impuesto = precio * 0.16
      retornar impuesto
   funcion!

   vn mi_iva = calcular_iva(100)
   mostrar("El IVA es: " + mi_iva)

5. CONTROL DE FLUJO
----------------------------------------------------------------------

REGLA DE ORO: Todo bloque debe cerrarse obligatoriamente con 
el nombre del comando seguido de un signo de exclamación "!".

A) CONDICIONALES (Toma de decisiones)
----------------------------------------------------------------------
Sintaxis: si (condición) ... sino ... si!
   * Permite ejecutar código solo si se cumple la condición.
   * El 'sino' es opcional para manejar el caso contrario.

   * Ejemplo: si (x > 10) mostrar("Grande") sino mostrar("Pequeño") si!

B) BUCLES (Repetición por condición)
----------------------------------------------------------------------
Sintaxis: mientras (condición) ... mientras!
   * Repite el bloque de código mientras la condición sea verdadera.
   * Útil para esperas, validaciones o ciclos infinitos.

   * Ejemplo: mientras (vida > 0) ... código ... mientras!

C) RECORRIDO (Iteración de listas)
----------------------------------------------------------------------
Sintaxis: cada (elemento en lista) ... cada!
   * Recorre automáticamente cada valor dentro de una lista (ln o lt).
   * 'elemento' es el nombre de la variable que toma el valor actual.

   * Ejemplo: cada (p en personajes) mostrar(p) cada!

D) CONTAR (Iteración numérica fija)
----------------------------------------------------------------------
Sintaxis: contar (i de inicio a fin) ... contar!
   * Repite el código un número exacto de veces.
   * 'i' es el contador que aumenta en cada vuelta.

   * Ejemplo: contar (i de 1 a 10) mostrar(i) contar!


OPERADORES LÓGICOS PARA CONDICIONES:
----------------------------------------------------------------------
Y (&&) | O (||) | NO (!) | == (Igual) | != (Distinto)


6. SISTEMA MODULAR
----------------------------------------------------------------------
importar("nombre_modulo")

A) MÓDULO 'WEB' (Conector de APIs)
----------------------------------
Sintaxis: web.leer("URL", "CLAVE_JSON", "VAR_DESTINO")
   * URL: La dirección de la API (Ej: "https://api.binance.com...").
   * CLAVE_JSON: El dato que buscas (Búsqueda recursiva profunda).
   * VAR_DESTINO: Nombre de la variable donde se guardará el resultado.
   * NOTA: Si no encuentra el dato o falla la red, devuelve "NULL".

B) MÓDULO 'ARCHIVOS' (Gestión JSON v6.0)
----------------------------------------
Sintaxis: 
   * archivos.crear("nombre.json") 
     -> Crea un archivo JSON vacío en el directorio.
   * archivos.escribir("nombre.json", "clave", valor) 
     -> Guarda un dato. Detecta automáticamente si es texto o número.
   * archivos.leer("nombre.json", "clave", "variable") 
     -> Busca la clave en el archivo y carga su valor en la variable.
   * archivos.inspeccionar("nombre.json") 
     -> Muestra todo el contenido del JSON en la consola.

C) MÓDULO 'TIEMPO' (Reloj del Sistema)
--------------------------------------
Sintaxis: tiempo.hora("var", "FMT") | tiempo.fecha("var", "FMT")
   * FORMATOS HORA (FMT): 
     "H" (Hora), "HM" (Hora:Min), "" (Hora:Min:Seg).
   * FORMATOS FECHA (FMT): 
     "D" (Día), "DM" (Día/Mes), "" (Día/Mes/Año).
   * AÑO: tiempo.anio("var") -> Guarda el año actual (vn).

D) MÓDULO 'RANDOM' (Azar)
-------------------------
Sintaxis:
   * random.numero("var", min, max) 
     -> Genera un número aleatorio entre el rango definido.
   * random.elegir("var", "lista") 
     -> Selecciona un elemento al azar de una lista (ln/lt).

E) MÓDULO 'SISTEMA' (Control)
-----------------------------
Sintaxis:
   * sistema.esperar(ms) -> Pausa la ejecución (1000 ms = 1 segundo).
   * sistema.limpiar()   -> Limpia todo el texto de la consola.
===========================================================

======================================================================"""
        txt.insert("1.0", manual)
        txt.config(state="disabled")

if __name__ == "__main__":
    root = tk.Tk()
    app = RedStudio(root)
    root.mainloop()