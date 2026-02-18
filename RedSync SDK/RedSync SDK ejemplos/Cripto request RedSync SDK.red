importar("web")

mostrar("--- CONECTOR BINANCE OFICIAL ---")
mostrar("Consultando precio actual de BTC/USDT...")

// URL de Binance para el precio actual (Ticker)
// Buscamos la clave "price" que devuelve Binance
web.leer("https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT", "price", "valor")

si (valor != "NULL")
    mostrar("================================")
    mostrar("BITCOIN: $" + valor)
    mostrar("================================")
sino
    mostrar("Error: No se recibió respuesta de Binance.")
    mostrar("Verifica tu conexión a internet.")
si!

sistema.esperar(4000)
