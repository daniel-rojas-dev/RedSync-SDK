# RedSync SDK
Mi lenguaje de programacion en español

# 🚀 RedSync SDK v3.6

**RedSync SDK** es un entorno de desarrollo y lenguaje de programación híbrido diseñado para ser **sencillo, potente y altamente eficiente**. Es ideal tanto para la enseñanza de lógica de programación como para la automatización de tareas en oficinas.

Gracias a su arquitectura que combina un **IDE ágil en Python** con un **Núcleo de ejecución de alto rendimiento en C++**, RedSync permite interactuar con el mundo exterior de forma nativa.

---

## ✨ Características Principales
* **Sintaxis en Español:** Pensado para un aprendizaje natural y rápido.
* **Módulo Web Inteligente:** Conecta con APIs y extrae datos JSON automáticamente.
* **Gestión de Archivos:** Manejo nativo de archivos JSON para bases de datos locales.
* **Portabilidad:** Todo lo que necesitas en un entorno integrado (Studio + Core).

---

## 🛠️ Documentación del Lenguaje

### 1. Variables y Asignación
* `vn [nombre] = [valor]` -> Variable numérica (double).
* `vt [nombre] = "[texto]"` -> Variable de texto (string).
* *Soporta operaciones matemáticas completas y reasignación sin prefijos.*

### 2. Entrada y Salida (I/O)
* `mostrar("Hola Mundo")` -> Imprime en consola.
* `entrada("Dime tu nombre")` -> Captura datos del usuario.

### 3. Control de Flujo (Regla de Oro: ¡Bloques con Cierre!)
Todos los bloques deben cerrarse con el nombre del comando seguido de un signo de exclamación `!`.
* **Condicionales:** `si (condicion) ... sino ... si!`
* **Bucles:** `mientras (condicion) ... mientras!`
* **Contar:** `contar (i de 1 a 10) ... contar!`
* **Recorrido de Listas:** `cada (elemento en lista) ... cada!`

### 4. Listas (Arrays)
* `ln lista_num = [1, 2, 3]`
* `lt lista_txt = ["A", "B"]`
* Métodos incluidos: `.agregar()`, `.eliminar()`, `.ordenar()`, `.invertir()`.

### 5. Funciones y Retornos
Las funciones permiten reutilizar código. Se definen con parámetros y pueden devolver valores.

```redcode
funcion sumar(a, b)
    vn resultado = a + b
    retornar resultado
funcion!

vn total = sumar(10, 5)
---

## 🔌 Sistema Modular (Módulos Útiles)
Para activar estas funciones, usa: `importar("nombre_modulo")`.

| Módulo | Función Principal | Ejemplo |
| :--- | :--- | :--- |
| **WEB** | Conexión con APIs y búsqueda recursiva en JSON. | `web.leer("url", "clave", "var")` |
| **ARCHIVOS** | Crear y gestionar bases de datos JSON. | `archivos.escribir("base.json", "id", 1)` |
| **RANDOM** | Generación de azar y elección en listas. | `random.numero("n", 1, 100)` |
| **TIEMPO** | Obtener hora, fecha y año con formatos. | `tiempo.hora("h", "HM")` |
| **SISTEMA** | Control de consola y esperas. | `sistema.limpiar()` |

---

## 🚀 Instalación y Uso
1. Descarga la última versión desde [URL pendiente]
2. Ejecuta `RedStudio.exe`.
3. Escribe tu código y presiona **F4** para ejecutar.

> **Nota para desarrolladores:** Este proyecto es **Open Source**. El IDE está desarrollado en Python (Tkinter) y el motor de ejecución en C++.

---
Creado para simplificar la programación.
