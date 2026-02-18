// Gestión de una lista de compras
lt inventario = ["Pan", "Leche", "Huevos"]

mostrar("Inventario actual:")
cada (item en inventario)
    mostrar("- " + item)
cada!

vt nuevo = entrada("¿Qué producto deseas añadir? ")
inventario.agregar(nuevo)
inventario.ordenar()

mostrar("Inventario actualizado y ordenado:")
cada (item en inventario)
    mostrar("> " + item)
cada!
