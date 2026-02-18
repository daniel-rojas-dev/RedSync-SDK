importar("archivos")
importar("sistema")

vt continuar = "si"
// Creamos el archivo solo si no existe
archivos.crear("database.json")

mientras (continuar == "si")
    sistema.limpiar()
    mostrar("--- GESTOR DE USUARIOS REDSYNC ---")
    mostrar("1. Registrar nuevo usuario")
    mostrar("2. Listar todos los usuarios (JSON)")
    mostrar("3. Salir")
    
    vt opcion = entrada("Selecciona una opción: ")

    si (opcion == "1")
        vt nombre = entrada("Nombre único del usuario: ")
        vt cargo = entrada("Cargo o puesto: ")

        // Guarda el nombre como llave y el cargo como valor
        archivos.escribir("database.json", nombre, cargo)
        
        mostrar("... Usuario [" + nombre + "] guardado con éxito ...")
        sistema.esperar(1500)
    sino
        si (opcion == "2")
            sistema.limpiar()
            mostrar("--- CONSULTANDO BASE DE DATOS LOCAL ---")
            // Esto imprime el contenido del JSON en pantalla
            archivos.inspeccionar("database.json")
            mostrar("---------------------------------------")
            // Pausa necesaria para que el bucle no limpie la pantalla de inmediato
            vt salida = entrada("Presiona ENTER para volver al menú: ")
        sino
            continuar = "no"
        si!
    si!
mientras!

mostrar("Cerrando sistema... ¡Buen trabajo!")

