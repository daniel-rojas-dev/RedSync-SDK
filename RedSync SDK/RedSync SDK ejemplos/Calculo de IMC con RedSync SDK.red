// Cálculo de Índice de Masa Corporal

mostrar("--- CALCULADORA DE SALUD ---")
vn peso = entrada("Introduce tu peso en kg: ")
vn altura = entrada("Introduce tu altura en metros (ej: 1.75): ")

vn imc = peso / (altura ** 2)
mostrar("Tu IMC es: " + imc)

si (imc < 18.5)
    mostrar("Estado: Bajo peso")
sino
    si (imc >= 18.5 Y imc < 25)
        mostrar("Estado: Peso normal")
    sino
        mostrar("Estado: Sobrepeso")
    si!
si!

