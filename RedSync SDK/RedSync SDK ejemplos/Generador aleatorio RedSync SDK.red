importar("random")

lt caracteres = ["A", "B", "C", "D", "1", "2", "3", "#", "$", "&"]
vt pass = ""
vn i = 0

mostrar("Generando clave de seguridad...")
contar (i de 1 a 8)
    vt car = ""
    random.elegir("car", "caracteres")
    pass = pass + car
contar!

mostrar("Tu nueva contraseña es: " + pass)
