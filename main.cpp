#include <fstream>      // Para operaciones con archivos (ifstream)
#include <iostream>     // Para entrada/salida de consola (cout, cin, endl)
#include <sstream>      // Para manipulación de strings como streams (stringstream)
#include <string>       // Para usar el tipo de dato string
#include <vector>       // Para usar el contenedor dinámico vector
#include <iomanip>      // Para fixed y setprecision (formato de salida flotante)
#include <stdexcept>    // Para manejar excepciones como runtime_error, invalid_argument
#include <limits>       // Necesario para numeric_limits
#include <ctime>        // Necesario para clock_t y clock()
#include <cctype>       // Necesario para tolower
#include <algorithm>    // Necesario para transform

using namespace std; // Usar el espacio de nombres estándar para simplificar el código

#include "Venta.h"      // Clase que representa una venta
#include "Lista.h"      // Implementación de Lista Enlazada
#include "HashEntry.h"  // Entrada para la tabla hash
#include "HashMapList.h" // Implementación de Tabla Hash con manejo de colisiones por listas
#include "quickSort.h"  // Algoritmo de ordenamiento QuickSort genérico

#define NOMBRE_ARCHIVO "ventas_sudamerica.csv" // Nombre del archivo CSV a procesar
#define TAMANIO_HASH_PAISES 50                 // Tamaño inicial para el hash de países
#define TAMANIO_HASH_CIUDADES 100              // Reutilizamos el tamaño para los inner HashMaps


//Contadores de condicionales
struct ConditionalCounters {
    int analizarTop5CiudadesPorPais_ifs = 0;
    int analizarMontoTotalPorProductoPorPais_ifs = 0;
    int analizarPromedioVentasPorCategoriaPorPais_ifs = 0;
    int analizarMedioEnvioMasUtilizadoPorPais_ifs = 0;
    int analizarMedioEnvioMasUtilizadoPorCategoria_ifs = 0;
    int analizarDiaMayorVentas_ifs = 0;
    int analizarProductoMasYMenosVendido_ifs = 0;
    int eliminarVenta_ifs = 0; 
    int modificarVenta_ifs = 0; 
    int listarVentasPorCiudad_ifs = 0;
    int listarVentasPorRangoFechasPorPais_ifs = 0;
    int compararDosPaises_ifs = 0;
    int compararDosProductosPorPais_ifs = 0;
    int buscarProductosPorDebajoUmbralPorPais_ifs = 0;
    int buscarProductosPorEncimaUmbral_ifs = 0;

    double time_total_analisis = 0.0;
    double time_total_gestion = 0.0;
    double time_total_consultas = 0.0;
} g_condCounters;

// --- Funciones Auxiliares ---
// Función hash simple para strings (necesaria para el HashMap)
unsigned int stringHash(string s) {
    unsigned int hash = 0;
    for (char c : s) {
        hash = hash * 31 + c;
    }
    return hash;
}

// Función para normalizar cadenas
string normalizeString(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Estructura para el promedio de ventas por categoría
struct CategoriaEstadisticas {
    float totalMonto;
    int totalCantidad;

    CategoriaEstadisticas(float tm = 0.0f, int tc = 0) : totalMonto(tm), totalCantidad(tc) {}

    float getPromedio() const {
        if (totalCantidad == 0) return 0.0f;
        return totalMonto / totalCantidad;
    }
};

struct ProductoEstadisticas {
    int totalCantidad;
    float totalMonto;

    ProductoEstadisticas(int tc = 0, float tm = 0.0f) : totalCantidad(tc), totalMonto(tm) {}
};

bool parseDate(const string& dateStr, int& day, int& month, int& year) {
    if (dateStr.length() != 10 || dateStr[2] != '/' || dateStr[5] != '/') { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
        return false; // Formato inválido
    }
    try {
        day = stoi(dateStr.substr(0, 2));
        month = stoi(dateStr.substr(3, 2));
        year = stoi(dateStr.substr(6, 4));
        // Validacion básica de rangos (ej. 1-31 para dia, 1-12 para mes)
        if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1900 || year > 2100) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
            return false;
        }
    } catch (const invalid_argument& e) {
        return false; // Error de conversion (no es un numero)
    } catch (const out_of_range& e) { 
        return false; // Numero fuera de rango
    }
    return true;
}

int compareDates(int d1, int m1, int y1, int d2, int m2, int y2) {
    if (y1 != y2) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; return y1 - y2; } 
    if (m1 != m2) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; return m1 - m2; } 
    g_condCounters.listarVentasPorRangoFechasPorPais_ifs++;
    return d1 - d2;
}

// Obtiene el monto total de ventas para un país específico
float obtenerMontoTotalPais(const Lista<Venta>& listaVentas, const string& paisAComparar) {
    float total = 0.0f;
    string paisNormalizado = normalizeString(paisAComparar);
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        if (normalizeString(ventaActual.pais) == paisNormalizado) { 
            total += ventaActual.montoTotal;
        }
    }
    return total;
}

// Obtiene los productos más vendidos (por monto) para un país específico
vector<pair<string, float>> obtenerProductosMasVendidosPais(const Lista<Venta>& listaVentas, const string& paisAComparar, int topN) {
    HashMapList<string, float> productosEnPais(TAMANIO_HASH_CIUDADES, stringHash); // Reutilizamos el tamaño

    string paisNormalizado = normalizeString(paisAComparar);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        if (normalizeString(ventaActual.pais) == paisNormalizado) { 
            string producto = ventaActual.producto;
            float monto = ventaActual.montoTotal;
            try {
                float montoActualProducto = productosEnPais.get(producto);
                productosEnPais.remove(producto);
                productosEnPais.put(producto, montoActualProducto + monto);
            } catch (const runtime_error& e) {
                productosEnPais.put(producto, monto);
            }
        }
    }

    vector<pair<string, float>> allProducts = productosEnPais.getAllEntries();
    // No usamos CiudadMonto porque es para ciudades, creamos una lambda para ordenar aquí
    sort(allProducts.begin(), allProducts.end(), [](const pair<string, float>& a, const pair<string, float>& b) {
        return a.second > b.second; // Ordenar por monto descendente
    });

    vector<pair<string, float>> topProducts;
    for (int i = 0; i < min((int)allProducts.size(), topN); ++i) {
        topProducts.push_back(allProducts[i]);
    }
    return topProducts;
}

// Obtiene el medio de envío más usado para un país específico
pair<string, int> obtenerMedioEnvioMasUsadoPais(const Lista<Venta>& listaVentas, const string& paisAComparar) {
    HashMapList<string, int> metodosEnPais(TAMANIO_HASH_CIUDADES, stringHash); // Reutilizamos el tamaño

    string paisNormalizado = normalizeString(paisAComparar);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        if (normalizeString(ventaActual.pais) == paisNormalizado) { 
            string medioEnvio = ventaActual.medioEnvio;
            try {
                int countMetodo = metodosEnPais.get(medioEnvio);
                metodosEnPais.remove(medioEnvio);
                metodosEnPais.put(medioEnvio, countMetodo + 1);
            } catch (const runtime_error& e) {
                metodosEnPais.put(medioEnvio, 1);
            }
        }
    }

    string medioMasUtilizado = "N/A";
    int maxCount = 0;

    vector<pair<string, int>> metodosCountPairs = metodosEnPais.getAllEntries();
    if (!metodosCountPairs.empty()) {
        maxCount = -1; // Reset para encontrar el maximo
        for (const auto& metodoCount : metodosCountPairs) {
            if (metodoCount.second > maxCount) { 
                maxCount = metodoCount.second;
                medioMasUtilizado = metodoCount.first;
            }
        }
    }
    return {medioMasUtilizado, maxCount};
}

// --- Funciones de Análisis --

void analizarTop5CiudadesPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarTop5CiudadesPorPais_ifs = 0; // Reiniciar contador para esta llamada
    cout << "\n--- TOP 5 DE CIUDADES CON MAYOR MONTO DE VENTAS POR PAIS ---\n";

    HashMapList<string, HashMapList<string, float>*> ventasPorPaisCiudad(TAMANIO_HASH_PAISES, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string pais = ventaActual.pais;
        string ciudad = ventaActual.ciudad;
        float monto = ventaActual.montoTotal;

        HashMapList<string, float>* ventasPorCiudad = nullptr;

        try {
            ventasPorCiudad = ventasPorPaisCiudad.get(pais);
        } catch (const runtime_error& e) {
            ventasPorCiudad = new HashMapList<string, float>(TAMANIO_HASH_CIUDADES, stringHash);
            ventasPorPaisCiudad.put(pais, ventasPorCiudad);
        }

        try {
            float montoActualCiudad = ventasPorCiudad->get(ciudad);
            ventasPorCiudad->remove(ciudad);
            ventasPorCiudad->put(ciudad, montoActualCiudad + monto);
        } catch (const runtime_error& e) {
            ventasPorCiudad->put(ciudad, monto);
        }
    }

    vector<pair<string, HashMapList<string, float>*>> paisesCitiesEntries = ventasPorPaisCiudad.getAllEntries();

    for (const auto& paisEntry : paisesCitiesEntries) {
        string pais = paisEntry.first;
        HashMapList<string, float>* ventasCiudades = paisEntry.second;

        cout << "\nPais: " << pais << endl;
        cout << "--------------------------------\n";

        vector<pair<string, float>> ciudadesMontoPairs = ventasCiudades->getAllEntries();
        vector<CiudadMonto> ciudadesMontos;
        for (const auto& par : ciudadesMontoPairs) {
            ciudadesMontos.push_back(CiudadMonto(par.first, par.second));
        }

        if (!ciudadesMontos.empty()) { g_condCounters.analizarTop5CiudadesPorPais_ifs++; 
            quickSort(ciudadesMontos, 0, ciudadesMontos.size() - 1, compararCiudadesMonto);
        }

        int count = 0;
        for (const auto& cm : ciudadesMontos) {
            if (count < 5) { g_condCounters.analizarTop5CiudadesPorPais_ifs++; 
                cout << (count + 1) << ". Ciudad: " << cm.ciudad << ", Monto Total: $" << fixed << setprecision(2) << cm.monto << endl;
                count++;
            } else { g_condCounters.analizarTop5CiudadesPorPais_ifs++;
                break;
            }
        }
        if (ciudadesMontos.empty()) { g_condCounters.analizarTop5CiudadesPorPais_ifs++;
            cout << "No hay datos de ventas para este pais." << endl;
        }
    }

    for (const auto& paisEntry : paisesCitiesEntries) {
        delete paisEntry.second;
    }
}

void analizarMontoTotalPorProductoPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarMontoTotalPorProductoPorPais_ifs = 0; // Reiniciar contador
    cout << "\n\n--- MONTO TOTAL VENDIDO POR PRODUCTO, DISCRIMINADO POR PAIS ---\n";

    HashMapList<string, HashMapList<string, float>*> productosPorPaisMontos(TAMANIO_HASH_PAISES, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string pais = ventaActual.pais;
        string producto = ventaActual.producto;
        float monto = ventaActual.montoTotal;

        HashMapList<string, float>* productosEnPais = nullptr;

        try {
            productosEnPais = productosPorPaisMontos.get(pais);
        } catch (const runtime_error& e) {
            productosEnPais = new HashMapList<string, float>(TAMANIO_HASH_CIUDADES, stringHash);
            productosPorPaisMontos.put(pais, productosEnPais);
        }

        try {
            float montoActualProducto = productosEnPais->get(producto);
            productosEnPais->remove(producto);
            productosEnPais->put(producto, montoActualProducto + monto);
        } catch (const runtime_error& e) {
            productosEnPais->put(producto, monto);
        }
    }

    vector<pair<string, HashMapList<string, float>*>> paisesConProductos = productosPorPaisMontos.getAllEntries();

    if (paisesConProductos.empty()) { g_condCounters.analizarMontoTotalPorProductoPorPais_ifs++; 
        cout << "No se encontraron datos de ventas por producto y pais." << endl;
    } else { g_condCounters.analizarMontoTotalPorProductoPorPais_ifs++; 
        for (const auto& paisEntry : paisesConProductos) {
            string pais = paisEntry.first;
            HashMapList<string, float>* productosDelPais = paisEntry.second;

            cout << "\nPais: " << pais << endl;
            cout << "--------------------------------\n";

            vector<pair<string, float>> productosMontoPairs = productosDelPais->getAllEntries();

            if (productosMontoPairs.empty()) { g_condCounters.analizarMontoTotalPorProductoPorPais_ifs++; 
                cout << "  No hay productos vendidos para este pais." << endl;
            } else { g_condCounters.analizarMontoTotalPorProductoPorPais_ifs++; 
                for (const auto& prodMonto : productosMontoPairs) {
                    cout << "  Producto: " << prodMonto.first << ", Monto Total Vendido: $"
                         << fixed << setprecision(2) << prodMonto.second << endl;
                }
            }
        }
    }

    for (const auto& paisEntry : paisesConProductos) {
        delete paisEntry.second;
    }
}

void analizarPromedioVentasPorCategoriaPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarPromedioVentasPorCategoriaPorPais_ifs = 0; // Reiniciar contador
    cout << "\n\n--- PROMEDIO DE VENTAS POR CATEGORIA EN CADA PAIS ---\n";

    HashMapList<string, HashMapList<string, CategoriaEstadisticas*>*> categoriasPorPais(TAMANIO_HASH_PAISES, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string pais = ventaActual.pais;
        string categoria = ventaActual.categoria;
        float monto = ventaActual.montoTotal;
        int cantidad = ventaActual.cantidad;

        HashMapList<string, CategoriaEstadisticas*>* categoriasEnPais = nullptr;

        try {
            categoriasEnPais = categoriasPorPais.get(pais);
        } catch (const runtime_error& e) {
            categoriasEnPais = new HashMapList<string, CategoriaEstadisticas*>(TAMANIO_HASH_CIUDADES, stringHash);
            categoriasPorPais.put(pais, categoriasEnPais);
        }

        CategoriaEstadisticas* statsCategoria = nullptr;
        try {
            statsCategoria = categoriasEnPais->get(categoria);
            statsCategoria->totalMonto += monto;
            statsCategoria->totalCantidad += cantidad;
        } catch (const runtime_error& e) {
            statsCategoria = new CategoriaEstadisticas(monto, cantidad);
            categoriasEnPais->put(categoria, statsCategoria);
        }
    }

    vector<pair<string, HashMapList<string, CategoriaEstadisticas*>*>> paisesConCategorias = categoriasPorPais.getAllEntries();

    if (paisesConCategorias.empty()) { g_condCounters.analizarPromedioVentasPorCategoriaPorPais_ifs++; 
        cout << "No se encontraron datos de ventas por categoria y pais." << endl;
    } else { g_condCounters.analizarPromedioVentasPorCategoriaPorPais_ifs++; 
        for (const auto& paisEntry : paisesConCategorias) {
            string pais = paisEntry.first;
            HashMapList<string, CategoriaEstadisticas*>* categoriasDelPais = paisEntry.second;

            cout << "\nPais: " << pais << endl;
            cout << "--------------------------------\n";

            vector<pair<string, CategoriaEstadisticas*>> categoriasStatsPairs = categoriasDelPais->getAllEntries();

            if (categoriasStatsPairs.empty()) { g_condCounters.analizarPromedioVentasPorCategoriaPorPais_ifs++; 
                cout << "  No hay categorias vendidas para este pais." << endl;
            } else { g_condCounters.analizarPromedioVentasPorCategoriaPorPais_ifs++; 
                for (const auto& catStats : categoriasStatsPairs) {
                    cout << "  Categoria: " << catStats.first
                         << ", Promedio de Ventas: $" << fixed << setprecision(2) << catStats.second->getPromedio() << endl;
                }
            }
        }
    }

    for (const auto& paisEntry : paisesConCategorias) {
        HashMapList<string, CategoriaEstadisticas*>* categoriasDelPais = paisEntry.second;
        vector<pair<string, CategoriaEstadisticas*>> innerEntries = categoriasDelPais->getAllEntries();
        for (const auto& catStats : innerEntries) {
            delete catStats.second;
        }
        delete categoriasDelPais;
    }
}

void analizarMedioEnvioMasUtilizadoPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs = 0; // Reiniciar contador
    cout << "\n\n--- MEDIO DE ENVIO MAS UTILIZADO POR PAIS ---\n";

    HashMapList<string, HashMapList<string, int>*> enviosPorPaisMetodo(TAMANIO_HASH_PAISES, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string pais = ventaActual.pais;
        string medioEnvio = ventaActual.medioEnvio;

        HashMapList<string, int>* metodosEnPais = nullptr;

        try {
            metodosEnPais = enviosPorPaisMetodo.get(pais);
        } catch (const runtime_error& e) {
            metodosEnPais = new HashMapList<string, int>(TAMANIO_HASH_CIUDADES, stringHash);
            enviosPorPaisMetodo.put(pais, metodosEnPais);
        }

        try {
            int countMetodo = metodosEnPais->get(medioEnvio);
            metodosEnPais->remove(medioEnvio);
            metodosEnPais->put(medioEnvio, countMetodo + 1);
        } catch (const runtime_error& e) {
            metodosEnPais->put(medioEnvio, 1);
        }
    }

    vector<pair<string, HashMapList<string, int>*>> paisesConEnvios = enviosPorPaisMetodo.getAllEntries();

    if (paisesConEnvios.empty()) { g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs++; 
        cout << "No se encontraron datos de medios de envio por pais." << endl;
    } else { g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs++; 
        for (const auto& paisEntry : paisesConEnvios) {
            string pais = paisEntry.first;
            HashMapList<string, int>* metodosDelPais = paisEntry.second;

            cout << "\nPais: " << pais << endl;
            cout << "--------------------------------\n";

            vector<pair<string, int>> metodosCountPairs = metodosDelPais->getAllEntries();

            if (metodosCountPairs.empty()) { g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs++; 
                cout << "  No hay medios de envio registrados para este pais." << endl;
            } else { g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs++; 
                string medioMasUtilizado = "";
                int maxCount = -1;

                for (const auto& metodoCount : metodosCountPairs) {
                    if (metodoCount.second > maxCount) { g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs++; 
                        maxCount = metodoCount.second;
                        medioMasUtilizado = metodoCount.first;
                    }
                }
                cout << "  Medio mas utilizado: " << medioMasUtilizado
                     << " (aparece " << maxCount << " veces)" << endl;
            }
        }
    }

    for (const auto& paisEntry : paisesConEnvios) {
        delete paisEntry.second;
    }
}

void analizarMedioEnvioMasUtilizadoPorCategoria(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs = 0; // Reiniciar contador
    cout << "\n\n--- MEDIO DE ENVIO MAS UTILIZADO POR CATEGORIA ---\n";

    HashMapList<string, HashMapList<string, int>*> enviosPorCategoriaMetodo(TAMANIO_HASH_CIUDADES, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string categoria = ventaActual.categoria;
        string medioEnvio = ventaActual.medioEnvio;

        HashMapList<string, int>* metodosEnCategoria = nullptr;

        try {
            metodosEnCategoria = enviosPorCategoriaMetodo.get(categoria);
        } catch (const runtime_error& e) {

            metodosEnCategoria = new HashMapList<string, int>(TAMANIO_HASH_CIUDADES, stringHash);
            enviosPorCategoriaMetodo.put(categoria, metodosEnCategoria);
        }

        try {
            int countMetodo = metodosEnCategoria->get(medioEnvio);
            metodosEnCategoria->remove(medioEnvio);
            metodosEnCategoria->put(medioEnvio, countMetodo + 1);
        } catch (const runtime_error& e) {
            metodosEnCategoria->put(medioEnvio, 1);
        }
    }

    vector<pair<string, HashMapList<string, int>*>> categoriasConEnvios = enviosPorCategoriaMetodo.getAllEntries();

    if (categoriasConEnvios.empty()) { g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs++; 
        cout << "No se encontraron datos de medios de envio por categoria." << endl;
    } else { g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs++; 
        for (const auto& categoriaEntry : categoriasConEnvios) {
            string categoria = categoriaEntry.first;
            HashMapList<string, int>* metodosDeLaCategoria = categoriaEntry.second;

            cout << "\nCategoria: " << categoria << endl;
            cout << "--------------------------------\n";

            vector<pair<string, int>> metodosCountPairs = metodosDeLaCategoria->getAllEntries();

            if (metodosCountPairs.empty()) { g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs++; 
                cout << "  No hay medios de envio registrados para esta categoria." << endl;
            } else { g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs++; 
                string medioMasUtilizado = "";
                int maxCount = -1;

                for (const auto& metodoCount : metodosCountPairs) {
                    if (metodoCount.second > maxCount) { g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs++; 
                        maxCount = metodoCount.second;
                        medioMasUtilizado = metodoCount.first;
                    }
                }
                cout << "  Medio mas utilizado: " << medioMasUtilizado
                     << " (aparece " << maxCount << " veces)" << endl;
            }
        }
    }

    for (const auto& categoriaEntry : categoriasConEnvios) {
        delete categoriaEntry.second;
    }
}

void analizarDiaMayorVentas(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarDiaMayorVentas_ifs = 0; // Reiniciar contador
    cout << "\n\n--- DIA CON MAYOR CANTIDAD DE VENTAS (POR MONTO DE DINERO) ---\n";

    HashMapList<string, float> ventasPorFecha(TAMANIO_HASH_CIUDADES * 2, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string fecha = ventaActual.fecha;
        float monto = ventaActual.montoTotal;

        try {
            float montoActualFecha = ventasPorFecha.get(fecha);
            ventasPorFecha.remove(fecha);
            ventasPorFecha.put(fecha, montoActualFecha + monto);
        } catch (const runtime_error& e) {
            ventasPorFecha.put(fecha, monto);
        }
    }

    string diaMayorVenta = "";
    float mayorMontoDia = -1.0f;

    vector<pair<string, float>> fechasMontos = ventasPorFecha.getAllEntries();

    if (fechasMontos.empty()) { g_condCounters.analizarDiaMayorVentas_ifs++; 
        cout << "No se encontraron datos de ventas por dia." << endl;
    } else { g_condCounters.analizarDiaMayorVentas_ifs++;
        for (const auto& entry : fechasMontos) {
            if (entry.second > mayorMontoDia) { g_condCounters.analizarDiaMayorVentas_ifs++; 
                mayorMontoDia = entry.second;
                diaMayorVenta = entry.first;
            }
        }
        cout << "El dia con mayor cantidad de ventas fue: " << diaMayorVenta
             << " con un monto total de: $" << fixed << setprecision(2) << mayorMontoDia << endl;
    }
}

void analizarProductoMasYMenosVendido(const Lista<Venta>& listaVentas) {
    g_condCounters.analizarProductoMasYMenosVendido_ifs = 0; // Reiniciar contador
    cout << "\n\n--- PRODUCTO MAS VENDIDO Y MENOS VENDIDO EN CANTIDAD TOTAL (UNIDADES) ---\n";

    HashMapList<string, int> cantidadVendidaPorProducto(TAMANIO_HASH_CIUDADES * 2, stringHash);

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string producto = ventaActual.producto;
        int cantidad = ventaActual.cantidad;

        try {
            int cantidadActualProducto = cantidadVendidaPorProducto.get(producto);
            cantidadVendidaPorProducto.remove(producto);
            cantidadVendidaPorProducto.put(producto, cantidadActualProducto + cantidad);
        } catch (const runtime_error& e) {
            cantidadVendidaPorProducto.put(producto, cantidad);
        }
    }

    string productoMasVendido = "";
    int maxCantidadVendida = -1;
    string productoMenosVendido = "";
    int minCantidadVendida = numeric_limits<int>::max();

    vector<pair<string, int>> productosCantidades = cantidadVendidaPorProducto.getAllEntries();

    if (productosCantidades.empty()) { g_condCounters.analizarProductoMasYMenosVendido_ifs++; 
        cout << "No se encontraron datos de productos vendidos." << endl;
    } else { g_condCounters.analizarProductoMasYMenosVendido_ifs++;
        for (const auto& entry : productosCantidades) {
            if (entry.second > maxCantidadVendida) { g_condCounters.analizarProductoMasYMenosVendido_ifs++; 
                maxCantidadVendida = entry.second;
                productoMasVendido = entry.first;
            }
            if (entry.second < minCantidadVendida) { g_condCounters.analizarProductoMasYMenosVendido_ifs++; 
                minCantidadVendida = entry.second;
                productoMenosVendido = entry.first;
            }
        }
        cout << "El producto mas vendido en cantidad total fue: " << productoMasVendido
             << " con " << maxCantidadVendida << " unidades vendidas." << endl;
        cout << "El producto menos vendido en cantidad total fue: " << productoMenosVendido
             << " con " << minCantidadVendida << " unidades vendidas." << endl;
    }
}

// Función que realiza todos los análisis
void realizarTodosLosAnalisis(const Lista<Venta>& listaVentas) {
    clock_t begin_func = clock(); // Iniciar medición para el bloque completo

    analizarTop5CiudadesPorPais(listaVentas);
    analizarMontoTotalPorProductoPorPais(listaVentas);
    analizarPromedioVentasPorCategoriaPorPais(listaVentas);
    analizarMedioEnvioMasUtilizadoPorPais(listaVentas);
    analizarMedioEnvioMasUtilizadoPorCategoria(listaVentas);
    analizarDiaMayorVentas(listaVentas);
    analizarProductoMasYMenosVendido(listaVentas);

    clock_t end_func = clock(); // Finalizar medición
    g_condCounters.time_total_analisis += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
}
// --- Funciones de Gestión de Datos ---

void agregarVenta(Lista<Venta>& listaVentas) {
    cout << "\n--- AGREGAR NUEVA VENTA ---\n";

    string idVenta, fecha, pais, ciudad, cliente, producto, categoria, medioEnvio, estadoEnvio;
    int cantidad;
    float precioUnitario, montoTotal;

    cout << "ID de Venta: ";
    cin >> idVenta;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Fecha (DD/MM/AAAA): ";
    getline(cin, fecha);

    cout << "Pais: ";
    getline(cin, pais);

    cout << "Ciudad: ";
    getline(cin, ciudad);

    cout << "Cliente: ";
    getline(cin, cliente);

    cout << "Producto: ";
    getline(cin, producto);

    cout << "Categoria: ";
    getline(cin, categoria);

    cout << "Cantidad: ";
    cin >> cantidad;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Precio Unitario: ";
    cin >> precioUnitario;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    montoTotal = cantidad * precioUnitario;
    cout << "Monto Total (calculado): " << fixed << setprecision(2) << montoTotal << endl;

    cout << "Medio de Envio: ";
    getline(cin, medioEnvio);

    cout << "Estado de Envio: ";
    getline(cin, estadoEnvio);

    Venta nuevaVenta(idVenta, fecha, pais, ciudad, cliente, producto, categoria,
                     cantidad, precioUnitario, montoTotal, medioEnvio, estadoEnvio);

    listaVentas.insertarUltimo(nuevaVenta);
    cout << "\nVenta agregada exitosamente:\n";
    nuevaVenta.mostrar();
}

void eliminarVenta(Lista<Venta>& listaVentas) {
    g_condCounters.eliminarVenta_ifs = 0; // Reiniciar contador
    cout << "\n--- ELIMINAR VENTA ---\n";
    cout << "Ingrese el pais o la ciudad para filtrar las ventas (o 'cancelar' para volver): ";
    string filtro;
    getline(cin, filtro);

    string filtroNormalizado = normalizeString(filtro);

    if (filtro == "cancelar") { g_condCounters.eliminarVenta_ifs++; 
        cout << "Operacion de eliminacion cancelada." << endl;
        return;
    }

    vector<pair<int, Venta>> ventasFiltradas;

    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);

        string paisNormalizado = normalizeString(ventaActual.pais);
        string ciudadNormalizada = normalizeString(ventaActual.ciudad);

        if (paisNormalizado == filtroNormalizado || ciudadNormalizada == filtroNormalizado) { g_condCounters.eliminarVenta_ifs++; 
            ventasFiltradas.push_back({i, ventaActual});
        }
    }

    if (ventasFiltradas.empty()) { g_condCounters.eliminarVenta_ifs++;
        cout << "No se encontraron ventas que coincidan con el filtro "<< filtro << "'." << endl;
        return;
    }

    cout << "\nVentas encontradas para el filtro " << filtro << " :\n";
    cout << "--------------------------------------------------\n";
    for (size_t i = 0; i < ventasFiltradas.size(); ++i) {
        cout << "[" << i + 1 << "] ID: " << ventasFiltradas[i].second.idVenta
             << ", Fecha: " << ventasFiltradas[i].second.fecha
             << ", Pais: " << ventasFiltradas[i].second.pais
             << ", Ciudad: " << ventasFiltradas[i].second.ciudad
             << ", Monto: $" << fixed << setprecision(2) << ventasFiltradas[i].second.montoTotal << endl;
    }
    cout << "--------------------------------------------------\n";

    cout << "Ingrese el ID de la venta que desea eliminar (o escriba 'cancelar' para abortar): ";
    string idAEliminar;
    getline(cin, idAEliminar);

    if (idAEliminar == "cancelar") { g_condCounters.eliminarVenta_ifs++; 
        cout << "Operacion de eliminacion cancelada." << endl;
        return;
    }

    int originalIndexToRemove = -1;
    for (const auto& par : ventasFiltradas) {
        if (par.second.idVenta == idAEliminar) { g_condCounters.eliminarVenta_ifs++; 
            originalIndexToRemove = par.first;
            break;
        }
    }

    if (originalIndexToRemove != -1) { g_condCounters.eliminarVenta_ifs++; 
        cout << "¿Esta seguro que desea eliminar la venta con ID " << idAEliminar << "? (s/n): ";
        char confirm;
        cin >> confirm;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (tolower(confirm) == 's') { g_condCounters.eliminarVenta_ifs++; 
            try {
                listaVentas.remover(originalIndexToRemove);
                cout << "Venta con ID "<<idAEliminar << " eliminada exitosamente." << endl;
            } catch (int e) {
                cout << "Error al intentar remover la venta. Codigo: " << e << endl;
            } catch (const runtime_error& e) {
                cout << "Error al intentar remover la venta: " << e.what() << endl;
            }
        } else { g_condCounters.eliminarVenta_ifs++; 
            cout << "Eliminacion cancelada por el usuario." << endl;
        }
    } else { g_condCounters.eliminarVenta_ifs++;
        cout << "El ID '" << idAEliminar << "' no se encontro en la lista de ventas filtradas. Asegurese de ingresar un ID valido de la lista mostrada." << endl;
    }
}

void modificarVenta(Lista<Venta>& listaVentas) {
    g_condCounters.modificarVenta_ifs = 0; // Reiniciar contador
    cout << "\n--- MODIFICAR VENTA ---\n";
    cout << "Ingrese el ID de la venta a modificar (o 'cancelar' para volver): ";
    string idAModificar;
    getline(cin, idAModificar);

    if (idAModificar == "cancelar") { g_condCounters.modificarVenta_ifs++; 
        cout << "Operacion de modificacion cancelada." << endl;
        return;
    }

    int indexToModify = -1;
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        if (listaVentas.getDato(i).idVenta == idAModificar) { g_condCounters.modificarVenta_ifs++; 
            indexToModify = i;
            break;
        }
    }

    if (indexToModify == -1) { g_condCounters.modificarVenta_ifs++; 
        cout << "Venta con ID '" << idAModificar << "' no encontrada." << endl;
        return;
    }

    Venta ventaOriginal = listaVentas.getDato(indexToModify);
    cout << "\nVenta encontrada (ID: " << ventaOriginal.idVenta << "):" << endl;
    ventaOriginal.mostrar();
    cout << "\nIngrese nuevos valores (deje vacio y presione Enter para mantener el valor actual):\n";

    string input;
    string newFecha = ventaOriginal.fecha;
    string newPais = ventaOriginal.pais;
    string newCiudad = ventaOriginal.ciudad;
    string newCliente = ventaOriginal.cliente;
    string newProducto = ventaOriginal.producto;
    string newCategoria = ventaOriginal.categoria;
    int newCantidad = ventaOriginal.cantidad;
    float newPrecioUnitario = ventaOriginal.precioUnitario;
    string newMedioEnvio = ventaOriginal.medioEnvio;
    string newEstadoEnvio = ventaOriginal.estadoEnvio;
 
    cout << "Fecha (" << ventaOriginal.fecha << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newFecha = input; } 

    cout << "Pais (" << ventaOriginal.pais << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newPais = input; } 

    cout << "Ciudad (" << ventaOriginal.ciudad << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newCiudad = input; } 

    cout << "Cliente (" << ventaOriginal.cliente << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newCliente = input; } 

    cout << "Producto (" << ventaOriginal.producto << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newProducto = input; } 

    cout << "Categoria (" << ventaOriginal.categoria << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newCategoria = input; } 

    cout << "Cantidad (" << ventaOriginal.cantidad << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; 
        try {
            newCantidad = stoi(input);
        } catch (const invalid_argument& e) { 
            cout << "Cantidad invalida. Se mantiene el valor original." << endl;
        } catch (const out_of_range& e) { 
            cout << "Cantidad fuera de rango. Se mantiene el valor original." << endl;
        }
    }

    cout << "Precio Unitario (" << ventaOriginal.precioUnitario << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; 
        try {
            newPrecioUnitario = stof(input);
        } catch (const invalid_argument& e) { g_condCounters.modificarVenta_ifs++; 
            cout << "Precio Unitario invalido. Se mantiene el valor original." << endl;
        } catch (const out_of_range& e) { g_condCounters.modificarVenta_ifs++; 
            cout << "Precio Unitario fuera de rango. Se mantiene el valor original." << endl;
        }
    }

    float newMontoTotal = newCantidad * newPrecioUnitario; // Recalcular monto total

    cout << "Medio de Envio (" << ventaOriginal.medioEnvio << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newMedioEnvio = input; } 

    cout << "Estado de Envio (" << ventaOriginal.estadoEnvio << "): ";
    getline(cin, input);
    if (!input.empty()) { g_condCounters.modificarVenta_ifs++; newEstadoEnvio = input; } 

    Venta ventaModificada(ventaOriginal.idVenta, newFecha, newPais, newCiudad, newCliente,
                          newProducto, newCategoria, newCantidad, newPrecioUnitario,
                          newMontoTotal, newMedioEnvio, newEstadoEnvio);

    try {
        listaVentas.reemplazar(indexToModify, ventaModificada);
        cout << "\nVenta con ID '" << idAModificar << "' modificada exitosamente." << endl;
        ventaModificada.mostrar();
    } catch (int e) {
        cout << "Error al intentar reemplazar la venta en la lista. Codigo: " << e << endl;
    } catch (const runtime_error& e) {
        cout << "Error al intentar reemplazar la venta en la lista: " << e.what() << endl;
    }
}

// --- Funciones de Consultas Dinámicas ---

// Función para listar las ventas realizadas en una ciudad específica
void listarVentasPorCiudad(const Lista<Venta>& listaVentas) {
    g_condCounters.listarVentasPorCiudad_ifs = 0; // Reiniciar contador
    cout << "\n--- LISTADO DE VENTAS POR CIUDAD ---\n";
    cout << "Ingrese el nombre de la ciudad a buscar (o 'cancelar' para volver): ";
    string ciudadBuscar;
    getline(cin, ciudadBuscar);

    string ciudadBuscarNormalizada = normalizeString(ciudadBuscar);

    if (ciudadBuscar == "cancelar") { g_condCounters.listarVentasPorCiudad_ifs++; 
        cout << "Operacion de listado cancelada." << endl;
        return;
    }

    bool encontradas = false;
    cout << "\nVentas en '" << ciudadBuscar << "':\n";
    cout << "--------------------------------------------------\n";
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string ciudadVentaNormalizada = normalizeString(ventaActual.ciudad);

        if (ciudadVentaNormalizada == ciudadBuscarNormalizada) { 
            ventaActual.mostrar();
            cout << "--------------------------------------------------\n";
            encontradas = true;
        }
    }

    if (!encontradas) { g_condCounters.listarVentasPorCiudad_ifs++; 
        cout << "No se encontraron ventas en la ciudad '" << ciudadBuscar << "'." << endl;
    }
}

// Función para listar ventas realizadas en un rango de fechas por país
void listarVentasPorRangoFechasPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.listarVentasPorRangoFechasPorPais_ifs = 0; // Reiniciar contador

    cout << "\n--- LISTADO DE VENTAS POR RANGO DE FECHAS Y PAIS ---\n";
    
    string fechaInicioStr, fechaFinStr, paisBuscar;
    int d_inicio, m_inicio, y_inicio;
    int d_fin, m_fin, y_fin;

    // Pedir fecha de inicio
    cout << "Ingrese la fecha de inicio (DD/MM/AAAA) o 'cancelar' para volver: ";
    getline(cin, fechaInicioStr);
    if (fechaInicioStr == "cancelar") { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 
    while (!parseDate(fechaInicioStr, d_inicio, m_inicio, y_inicio)) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
        cout << "Fecha de inicio invalida. Ingrese en formato DD/MM/AAAA: ";
        getline(cin, fechaInicioStr);
        if (fechaInicioStr == "cancelar") { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 
    }

    // Pedir fecha de fin
    cout << "Ingrese la fecha de fin (DD/MM/AAAA) o 'cancelar' para volver: ";
    getline(cin, fechaFinStr);
    if (fechaFinStr == "cancelar") { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 
    while (!parseDate(fechaFinStr, d_fin, m_fin, y_fin)) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
        cout << "Fecha de fin invalida. Ingrese en formato DD/MM/AAAA: ";
        getline(cin, fechaFinStr);
        if (fechaFinStr == "cancelar") { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 
    }

    // Validar que la fecha de inicio no sea posterior a la fecha de fin
    if (compareDates(d_inicio, m_inicio, y_inicio, d_fin, m_fin, y_fin) > 0) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
        cout << "La fecha de inicio no puede ser posterior a la fecha de fin. Operacion cancelada." << endl;
        return;
    }

    // Pedir pais
    cout << "Ingrese el pais a buscar (o 'cancelar' para volver): ";
    getline(cin, paisBuscar);
    string paisBuscarNormalizado = normalizeString(paisBuscar);
    if (paisBuscar == "cancelar") { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 

    bool encontradas = false;
    cout << "\nVentas en " << paisBuscar << " entre " << fechaInicioStr << " y " << fechaFinStr << ":\n";
    cout << "--------------------------------------------------\n";
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        
        int d_venta, m_venta, y_venta;
        if (!parseDate(ventaActual.fecha, d_venta, m_venta, y_venta)) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
            // Si la fecha de la venta es inválida, la saltamos o manejamos el error
            continue; 
        }
        
        string paisVentaNormalizado = normalizeString(ventaActual.pais);

        // Comprobar si la fecha de la venta está dentro del rango Y si el país coincide
        if (compareDates(d_venta, m_venta, y_venta, d_inicio, m_inicio, y_inicio) >= 0 && // Venta es posterior o igual a fecha de inicio
            compareDates(d_venta, m_venta, y_venta, d_fin, m_fin, y_fin) <= 0 &&         // Venta es anterior o igual a fecha de fin
            paisVentaNormalizado == paisBuscarNormalizado) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
            
            ventaActual.mostrar();
            cout << "--------------------------------------------------\n";
            encontradas = true;
        }
    }

    if (!encontradas) { g_condCounters.listarVentasPorRangoFechasPorPais_ifs++; 
        cout << "No se encontraron ventas para " << paisBuscar << " en el rango de fechas especificado." << endl;
    }
}

// Función para comparar dos países
void compararDosPaises(const Lista<Venta>& listaVentas) {
    g_condCounters.compararDosPaises_ifs = 0; // Reiniciar contador
    cout << "\n--- COMPARACION ENTRE DOS PAISES ---\n";
    string pais1_str, pais2_str;

    cout << "Ingrese el nombre del primer pais (o 'cancelar' para volver): ";
    getline(cin, pais1_str);
    string pais1Normalizado = normalizeString(pais1_str);
    if (pais1_str == "cancelar") { g_condCounters.compararDosPaises_ifs++; cout << "Operacion cancelada." << endl; return; } 

    cout << "Ingrese el nombre del segundo pais (o 'cancelar' para volver): ";
    getline(cin, pais2_str);
    string pais2Normalizado = normalizeString(pais2_str);
    if (pais2_str == "cancelar") { g_condCounters.compararDosPaises_ifs++; cout << "Operacion cancelada." << endl; return; } 

    if (pais1Normalizado == pais2Normalizado) { g_condCounters.compararDosPaises_ifs++; 
        cout << "Los paises ingresados son el mismo. Por favor, ingrese dos paises diferentes." << endl;
        return;
    }

    cout << "\n--- Resultados de la comparacion entre " << pais1_str << " y " << pais2_str << " ---\n";

    // a. Monto total de ventas
    cout << "\n1. Monto total de ventas:\n";
    float monto1 = obtenerMontoTotalPais(listaVentas, pais1_str);
    float monto2 = obtenerMontoTotalPais(listaVentas, pais2_str);
    cout << "   " << pais1_str << ": $" << fixed << setprecision(2) << monto1 << endl;
    cout << "   " << pais2_str << ": $" << fixed << setprecision(2) << monto2 << endl;
    if (monto1 > monto2) { g_condCounters.compararDosPaises_ifs++;
        cout << "   " << pais1_str << " tiene un mayor monto total de ventas." << endl;
    } else if (monto2 > monto1) { g_condCounters.compararDosPaises_ifs++; 
        cout << "   " << pais2_str << " tiene un mayor monto total de ventas." << endl;
    } else { g_condCounters.compararDosPaises_ifs++; 
        cout << "   Ambos paises tienen el mismo monto total de ventas." << endl;
    }

    // b. Producto mas vendido (solo el mas vendido)
    cout << "\n2. Producto mas vendido (por monto):\n";
    vector<pair<string, float>> topProductos1 = obtenerProductosMasVendidosPais(listaVentas, pais1_str, 1); 
    vector<pair<string, float>> topProductos2 = obtenerProductosMasVendidosPais(listaVentas, pais2_str, 1);

    cout << "   -Producto mas vendido en " << pais1_str << ":\n";
    if (topProductos1.empty()) { g_condCounters.compararDosPaises_ifs++; 
        cout << "     No se encontraron productos para este pais." << endl;
    } else { g_condCounters.compararDosPaises_ifs++; 
        cout << "     " << topProductos1[0].first << " ($" << fixed << setprecision(2) << topProductos1[0].second << ")" << endl;
    }

    cout << "   -Producto mas vendido en " << pais2_str << ":\n";
    if (topProductos2.empty()) { g_condCounters.compararDosPaises_ifs++; 
        cout << "    No se encontraron productos para este pais." << endl;
    } else { g_condCounters.compararDosPaises_ifs++; 
        cout << "     " << topProductos2[0].first << " ($" << fixed << setprecision(2) << topProductos2[0].second << ")" << endl;
    }

    // c. Medio de envio mas usado
    cout << "\n3. Medio de envio mas usado:\n";
    pair<string, int> medioEnvio1 = obtenerMedioEnvioMasUsadoPais(listaVentas, pais1_str);
    pair<string, int> medioEnvio2 = obtenerMedioEnvioMasUsadoPais(listaVentas, pais2_str);

    cout << "   -Medio mas usado en " << pais1_str << ": " << medioEnvio1.first << " (" << medioEnvio1.second << " veces)" << endl;
    cout << "   -Medio mas usado en " << pais2_str << ": " << medioEnvio2.first << " (" << medioEnvio2.second << " veces)" << endl;
    if (medioEnvio1.second > medioEnvio2.second) { g_condCounters.compararDosPaises_ifs++; 
        cout << "   " << pais1_str << " usa mas frecuentemente " << medioEnvio1.first << " que " << pais2_str << " usa " << medioEnvio2.first << "." << endl;
    } else if (medioEnvio2.second > medioEnvio1.second) { g_condCounters.compararDosPaises_ifs++; 
        cout << "   " << pais2_str << " usa mas frecuentemente " << medioEnvio2.first << " que " << pais1_str << " usa " << medioEnvio1.first << "." << endl;
    } else if (medioEnvio1.second > 0) { g_condCounters.compararDosPaises_ifs++; 
        cout << "   Ambos paises usan sus medios de envio mas frecuentes la misma cantidad de veces." << endl;
    } else { g_condCounters.compararDosPaises_ifs++; 
        cout << "   No hay datos suficientes para comparar los medios de envio." << endl;
    }
}

// Función para comparar dos productos discriminado por todos los países
void compararDosProductosPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.compararDosProductosPorPais_ifs = 0; // Reiniciar contador
    cout << "\n--- COMPARACION ENTRE DOS PRODUCTOS DISCRIMINADO POR PAIS ---\n";
    string producto1_str, producto2_str;

    cout << "Ingrese el nombre del primer producto (o 'cancelar' para volver): ";
    getline(cin, producto1_str);
    string prod1Normalizado = normalizeString(producto1_str);
    if (producto1_str == "cancelar") { g_condCounters.compararDosProductosPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 

    cout << "Ingrese el nombre del segundo producto (o 'cancelar' para volver): ";
    getline(cin, producto2_str);
    string prod2Normalizado = normalizeString(producto2_str);
    if (producto2_str == "cancelar") { g_condCounters.compararDosProductosPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 

    if (prod1Normalizado == prod2Normalizado) { g_condCounters.compararDosProductosPorPais_ifs++; 
        cout << "Los productos ingresados son el mismo. Por favor, ingrese dos productos diferentes." << endl;
        return;
    }

    // HashMap principal: Clave=País (string), Valor=Puntero a HashMapList<Producto (string), ProductoEstadisticas*>
    // La clave del inner HashMapList ahora será el nombre NORMALIZADO del producto.
    HashMapList<string, HashMapList<string, ProductoEstadisticas*>*> datosPorPaisProducto(TAMANIO_HASH_PAISES, stringHash);

    // Recolectar datos para ambos productos en todos los países
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string pais = ventaActual.pais;
        string producto = ventaActual.producto; // Nombre original del producto del CSV
        float monto = ventaActual.montoTotal;
        int cantidad = ventaActual.cantidad;

        string paisNormalizado = normalizeString(pais);
        string prodNormalizado = normalizeString(producto); // Nombre normalizado del producto del CSV
        

        bool isProd1 = (prodNormalizado == prod1Normalizado);
        bool isProd2 = (prodNormalizado == prod2Normalizado);

        if (isProd1 || isProd2) { g_condCounters.compararDosProductosPorPais_ifs++; 
            HashMapList<string, ProductoEstadisticas*>* productosEnPais = nullptr;

            try {
                // La clave del HashMap de paises es el nombre original del pais.
                productosEnPais = datosPorPaisProducto.get(pais);
            } catch (const runtime_error& e) {
                productosEnPais = new HashMapList<string, ProductoEstadisticas*>(TAMANIO_HASH_CIUDADES, stringHash);
                datosPorPaisProducto.put(pais, productosEnPais);
            }

            ProductoEstadisticas* statsProducto = nullptr;
            try {
                statsProducto = productosEnPais->get(prodNormalizado);
                statsProducto->totalCantidad += cantidad;
                statsProducto->totalMonto += monto;
            } catch (const runtime_error& e) {
                statsProducto = new ProductoEstadisticas(cantidad, monto);
                productosEnPais->put(prodNormalizado, statsProducto);
            }
        }
    }

    // --- Mostrar la comparación ---
    vector<pair<string, HashMapList<string, ProductoEstadisticas*>*>> paisesConDatos = datosPorPaisProducto.getAllEntries();

    if (paisesConDatos.empty()) { g_condCounters.compararDosProductosPorPais_ifs++; 
        cout << "No se encontraron ventas para los productos '" << producto1_str << "' o '" << producto2_str << "' en ningun pais." << endl;
    } else { g_condCounters.compararDosProductosPorPais_ifs++; 
        cout << "\n--- Comparacion detallada por Pais ---\n";
        bool alMenosUnProductoEncontradoGlobal = false;
        for (const auto& paisEntry : paisesConDatos) {
            string pais = paisEntry.first; // Nombre original del país
            HashMapList<string, ProductoEstadisticas*>* productosDelPais = paisEntry.second;

            ProductoEstadisticas* statsProd1 = nullptr;
            ProductoEstadisticas* statsProd2 = nullptr;

            // *** CAMBIO CLAVE AQUI ***
            // Al recuperar: usar las claves NORMALIZADAS de los productos que el usuario ingresó.
            // Esto es crucial para que coincida con lo que se almacenó (que también es normalizado).
            try { statsProd1 = productosDelPais->get(prod1Normalizado);}
            catch (const runtime_error& e){}
            try { statsProd2 = productosDelPais->get(prod2Normalizado);}
            catch (const runtime_error& e){}

            // Solo imprimir el pais si al menos uno de los dos productos tiene datos
            if ((statsProd1 && (statsProd1->totalCantidad > 0 || statsProd1->totalMonto > 0)) ||
                (statsProd2 && (statsProd2->totalCantidad > 0 || statsProd2->totalMonto > 0))) { g_condCounters.compararDosProductosPorPais_ifs++; // If for product stats existence
                
                alMenosUnProductoEncontradoGlobal = true;

                cout << "\nPais: " << pais << endl;
                cout << "----------------------------------------\n";

                // a. Cantidad total vendida
                cout << "  1. Cantidad total vendida:\n";
                cout << "     " << producto1_str << ": " << (statsProd1 ? statsProd1->totalCantidad : 0) << " unidades\n";
                cout << "     " << producto2_str << ": " << (statsProd2 ? statsProd2->totalCantidad : 0) << " unidades\n";
                if (statsProd1 && statsProd2) { g_condCounters.compararDosProductosPorPais_ifs++; 
                    if (statsProd1->totalCantidad > statsProd2->totalCantidad) { g_condCounters.compararDosProductosPorPais_ifs++; 
                    } else if (statsProd2->totalCantidad > statsProd1->totalCantidad) { g_condCounters.compararDosProductosPorPais_ifs++; 
                        cout << "     " << producto2_str << " se vendio mas en cantidad en este pais.\n";
                    } else { g_condCounters.compararDosProductosPorPais_ifs++; 
                        cout << "     Ambos productos se vendieron la misma cantidad en este pais.\n";
                    }
                } else if (statsProd1) { g_condCounters.compararDosProductosPorPais_ifs++;
                     cout << "     Solo " << producto1_str << " tiene ventas en este pais.\n";
                } else if (statsProd2) { g_condCounters.compararDosProductosPorPais_ifs++; 
                     cout << "     Solo " << producto2_str << " tiene ventas en este pais.\n";
                } else { g_condCounters.compararDosProductosPorPais_ifs++; 
                    cout << "     Ninguno de los productos tiene ventas en este pais.\n";
                }


                // b. Monto total
                cout << "  2. Monto total vendido:\n";
                cout << "     " << producto1_str << ": $" << fixed << setprecision(2) << (statsProd1 ? statsProd1->totalMonto : 0.0f) << "\n";
                cout << "     " << producto2_str << ": $" << fixed << setprecision(2) << (statsProd2 ? statsProd2->totalMonto : 0.0f) << "\n";
                if (statsProd1 && statsProd2) { g_condCounters.compararDosProductosPorPais_ifs++; 
                    if (statsProd1->totalMonto > statsProd2->totalMonto) { g_condCounters.compararDosProductosPorPais_ifs++; 
                        cout << "     " << producto1_str << " genero mas monto en este pais.\n";
                    } else if (statsProd2->totalMonto > statsProd1->totalMonto) { g_condCounters.compararDosProductosPorPais_ifs++; 
                        cout << "     " << producto2_str << " genero mas monto en este pais.\n";
                    } else { g_condCounters.compararDosProductosPorPais_ifs++; 
                        cout << "     Ambos productos generaron el mismo monto en este pais.\n";
                    }
                } else if (statsProd1) { g_condCounters.compararDosProductosPorPais_ifs++; 
                    cout << "     Solo " << producto1_str << " tiene ventas (monto) en este pais.\n";
                } else if (statsProd2) { g_condCounters.compararDosProductosPorPais_ifs++; 
                    cout << "     Solo " << producto2_str << " tiene ventas (monto) en este pais.\n";
                } else { g_condCounters.compararDosProductosPorPais_ifs++; 
                    cout << "     Ninguno de los productos tiene ventas (monto) en este pais.\n";
                }
            }
        } 

        if (!alMenosUnProductoEncontradoGlobal) { g_condCounters.compararDosProductosPorPais_ifs++; 
            cout << "Ninguno de los productos '" << producto1_str << "' o '" << producto2_str << "' tuvo ventas registradas en ningun pais." << endl;
        }
    }

    // --- CRÍTICO: Liberar la memoria ---
    for (const auto& paisEntry : paisesConDatos) {
        HashMapList<string, ProductoEstadisticas*>* productosDelPais = paisEntry.second;
        vector<pair<string, ProductoEstadisticas*>> innerEntries = productosDelPais->getAllEntries();
        for (const auto& prodStats : innerEntries) {
            delete prodStats.second; // Libera cada objeto ProductoEstadisticas*
        }
        delete productosDelPais; // Libera el HashMapList<string, ProductoEstadisticas*>* de productos
    }
}

void buscarProductosPorDebajoUmbralPorPais(const Lista<Venta>& listaVentas) {
    g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs = 0; // Reiniciar contador
    cout << "\n--- BUSCAR PRODUCTOS POR DEBAJO DE UMBRAL POR PAIS ---\n";
    string paisBuscar;
    float umbralMonto;

    cout << "Ingrese el pais a buscar (o 'cancelar' para volver): ";
    getline(cin, paisBuscar);
    string paisBuscarNormalizado = normalizeString(paisBuscar);
    if (paisBuscar == "cancelar") { g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs++; cout << "Operacion cancelada." << endl; return; }

    cout << "Ingrese el monto umbral (ej. 500.00) o -1 para cancelar: ";
    string umbralStr;
    getline(cin, umbralStr);
    if (umbralStr == "-1" || umbralStr == "cancelar") { g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs++; cout << "Operacion cancelada." << endl; return; } 
    try {
        umbralMonto = stof(umbralStr);
    } catch (const invalid_argument& e) {
        cout << "Umbral invalido. Operacion cancelada." << endl;
        return;
    } catch (const out_of_range& e) {
        cout << "Umbral fuera de rango. Operacion cancelada." << endl;
        return;
    }

    // HashMap para acumular cantidad y monto por producto en el país especificado
    HashMapList<string, ProductoEstadisticas*> productosPorPais(TAMANIO_HASH_CIUDADES, stringHash);

    // Recolectar datos para los productos del pais especificado
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string paisVentaNormalizado = normalizeString(ventaActual.pais);

        if (paisVentaNormalizado == paisBuscarNormalizado) { g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs++; 
            string producto = ventaActual.producto; // Usar el nombre original del producto
            float monto = ventaActual.montoTotal;
            int cantidad = ventaActual.cantidad;

            ProductoEstadisticas* statsProducto = nullptr;
            try {
                // La clave en el HashMap de productos es el nombre ORIGINAL del producto.
                // Es importante si queremos mostrar el nombre original al final.
                statsProducto = productosPorPais.get(producto);
                statsProducto->totalCantidad += cantidad;
                statsProducto->totalMonto += monto;
            } catch (const runtime_error& e) {
                statsProducto = new ProductoEstadisticas(cantidad, monto);
                productosPorPais.put(producto, statsProducto);
            }
        }
    }

    // --- Mostrar productos que cumplen la condición ---
    vector<pair<string, ProductoEstadisticas*>> productosEncontrados = productosPorPais.getAllEntries();
    
    bool productosMostrados = false;
    cout << "\nProductos en " << paisBuscar << " con promedio de venta por debajo de $" << fixed << setprecision(2) << umbralMonto << ":\n";
    cout << "--------------------------------------------------\n";
    
    for (const auto& entry : productosEncontrados) {
        string producto = entry.first;
        ProductoEstadisticas* stats = entry.second;

        if (stats->totalCantidad > 0) { g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs++;
            float promedioVenta = stats->totalMonto / stats->totalCantidad;
            if (promedioVenta < umbralMonto) { g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs++;
                cout << "  - Producto: " << producto
                     << ", Promedio: $" << fixed << setprecision(2) << promedioVenta
                     << " (Cantidad total: " << stats->totalCantidad
                     << ", Monto total: $" << stats->totalMonto << ")" << endl;
                productosMostrados = true;
            }
        }
    }

    if (!productosMostrados) { g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs++; 
        cout << "No se encontraron productos que cumplan la condicion en " << paisBuscar << "." << endl;
    }
    cout << "--------------------------------------------------\n";

    // --- CRÍTICO: Liberar la memoria ---
    for (const auto& entry : productosEncontrados) {
        delete entry.second; 
    }
}

void buscarProductosPorEncimaUmbral(const Lista<Venta>& listaVentas) {
    g_condCounters.buscarProductosPorEncimaUmbral_ifs = 0; // Reiniciar contador
    cout << "\n--- BUSCAR PRODUCTOS POR ENCIMA DE UMBRAL (GLOBAL) ---\n";
    float umbralMonto;

    cout << "Ingrese el monto umbral (ej. 500.00) o -1 para cancelar: ";
    string umbralStr;
    getline(cin, umbralStr);
    if (umbralStr == "-1" || umbralStr == "cancelar") { g_condCounters.buscarProductosPorEncimaUmbral_ifs++; cout << "Operacion cancelada." << endl; return; } 
    try {
        umbralMonto = stof(umbralStr);
    } catch (const invalid_argument& e) {
        cout << "Umbral invalido. Operacion cancelada." << endl;
        return;
    } catch (const out_of_range& e) {
        cout << "Umbral fuera de rango. Operacion cancelada." << endl;
        return;
    }

    // HashMap para acumular cantidad y monto por producto (globalmente)
    HashMapList<string, ProductoEstadisticas*> productosTotales(TAMANIO_HASH_CIUDADES * 2, stringHash);

    // Recolectar datos para todos los productos
    for (int i = 0; i < listaVentas.getTamanio(); ++i) {
        Venta ventaActual = listaVentas.getDato(i);
        string producto = ventaActual.producto;
        float monto = ventaActual.montoTotal;
        int cantidad = ventaActual.cantidad;

        ProductoEstadisticas* statsProducto = nullptr;
        try {
            // La clave en el HashMap de productos es el nombre ORIGINAL del producto.
            statsProducto = productosTotales.get(producto);
            statsProducto->totalCantidad += cantidad;
            statsProducto->totalMonto += monto;
        } catch (const runtime_error& e) {
            statsProducto = new ProductoEstadisticas(cantidad, monto);
            productosTotales.put(producto, statsProducto);
        }
    }

    // --- Mostrar productos que cumplen la condición ---
    vector<pair<string, ProductoEstadisticas*>> productosEncontrados = productosTotales.getAllEntries();
    
    bool productosMostrados = false;
    cout << "\nProductos (global) con promedio de venta por encima de $" << fixed << setprecision(2) << umbralMonto << ":\n";
    cout << "--------------------------------------------------\n";
    
    for (const auto& entry : productosEncontrados) {
        string producto = entry.first;
        ProductoEstadisticas* stats = entry.second;

        if (stats->totalCantidad > 0) { g_condCounters.buscarProductosPorEncimaUmbral_ifs++; 
            float promedioVenta = stats->totalMonto / stats->totalCantidad;
            if (promedioVenta > umbralMonto) { g_condCounters.buscarProductosPorEncimaUmbral_ifs++; 
                cout << "  - Producto: " << producto
                     << ", Promedio: $" << fixed << setprecision(2) << promedioVenta
                     << " (Cantidad total: " << stats->totalCantidad
                     << ", Monto total: $" << stats->totalMonto << ")" << endl;
                productosMostrados = true;
            }
        }
    }

    if (!productosMostrados) { g_condCounters.buscarProductosPorEncimaUmbral_ifs++; 
        cout << "No se encontraron productos que cumplan la condicion (global)." << endl;
    }
    cout << "--------------------------------------------------\n";

    // --- CRÍTICO: Liberar la memoria ---
    for (const auto& entry : productosEncontrados) {
        delete entry.second; // Libera cada objeto ProductoEstadisticas*
    }
}

void mostrarMenuGestionVentas(Lista<Venta>& listaVentas) {
    int opcionGestion;
    do {
        cout << "\n\n--- MENU DE GESTION DE VENTAS ---\n";
        cout << "  1. Agregar nueva venta\n";
        cout << "  2. Eliminar venta (filtrar y por ID)\n";
        cout << "  3. Modificar venta (por ID)\n";
        cout << "  0. Volver al Menu Principal\n";
        cout << "Ingrese su opcion de gestion: ";
        cin >> opcionGestion;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar buffer después de leer opción

        clock_t begin_func, end_func; 

        switch(opcionGestion) {
            case 1:
                begin_func = clock();
                agregarVenta(listaVentas);
                end_func = clock();
                g_condCounters.time_total_gestion += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 2:
                begin_func = clock();
                eliminarVenta(listaVentas);
                end_func = clock();
                g_condCounters.time_total_gestion += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 3:
                begin_func = clock();
                modificarVenta(listaVentas);
                end_func = clock();
                g_condCounters.time_total_gestion += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 0:
                cout << "Volviendo al Menu Principal...\n";
                break;
            default:
                cout << "Opcion de gestion no valida. Intente de nuevo.\n";
                break;
        }
    } while (opcionGestion != 0);
}

void mostrarMenuConsultas(const Lista<Venta>& listaVentas) {
    int opcionConsulta;
    do {
        cout << "\n\n--- MENU DE CONSULTAS DINAMICAS ---\n";
        cout << "1. Listar ventas realizadas en una ciudad especifica\n";
        cout << "2. Listar ventas realizadas en un rango de fechas por pais\n";
        cout << "3. Comparacion entre dos paises\n"; 
        cout << "4. Comparacion entre dos productos discriminado por pais\n";
        cout << "5. Buscar productos vendidos en promedio por debajo de un umbral por pais\n";
        cout << "6. Buscar productos vendidos en promedio por encima de un umbral (global)\n";
        cout << "0. Volver al Menu Principal\n";
        cout << "Ingrese su opcion: ";
        cin >> opcionConsulta;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        clock_t begin_func, end_func; 

        switch (opcionConsulta) {
            case 1:
                begin_func = clock();
                listarVentasPorCiudad(listaVentas);
                end_func = clock();
                g_condCounters.time_total_consultas += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 2:
                begin_func = clock();
                listarVentasPorRangoFechasPorPais(listaVentas);
                end_func = clock();
                g_condCounters.time_total_consultas += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 3:
                begin_func = clock();
                compararDosPaises(listaVentas);
                end_func = clock();
                g_condCounters.time_total_consultas += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 4:
                begin_func = clock();
                compararDosProductosPorPais(listaVentas);
                end_func = clock();
                g_condCounters.time_total_consultas += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 5:
                begin_func = clock();
                buscarProductosPorDebajoUmbralPorPais(listaVentas);
                end_func = clock();
                g_condCounters.time_total_consultas += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;
            case 6:
                begin_func = clock();
                buscarProductosPorEncimaUmbral(listaVentas);
                end_func = clock();
                g_condCounters.time_total_consultas += static_cast<double>(end_func - begin_func) / CLOCKS_PER_SEC;
                break;

                cout << "Volviendo al Menu Principal...\n";
                break;
            default:
                cout << "Opcion no valida. Intente de nuevo.\n";
                break;
        }
    } while (opcionConsulta != 0);
}

// Función principal del programa
int main() {
    clock_t begin;

    cout << "Comenzando a medir Tiempo\n" << endl;

    begin = clock();
    Lista<Venta> listaVentas;

    // Cargar ventas desde el archivo CSV al inicio
    ifstream archivo(NOMBRE_ARCHIVO);
    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo." << endl;
        return 1;
    }

    string linea;
    char delimitador = ',';
    getline(archivo, linea);

    while (getline(archivo, linea)) {
        stringstream stream(linea);
        string ID_Venta_str, Fecha, Pais, Ciudad, Cliente, Producto, Categoria,
               Cantidad_str, Precio_Unitario_str, Monto_Total_str, Medio_Envio, Estado_Envio;

        getline(stream, ID_Venta_str, delimitador);
        getline(stream, Fecha, delimitador);
        getline(stream, Pais, delimitador);
        getline(stream, Ciudad, delimitador);
        getline(stream, Cliente, delimitador);
        getline(stream, Producto, delimitador);
        getline(stream, Categoria, delimitador);
        getline(stream, Cantidad_str, delimitador);
        getline(stream, Precio_Unitario_str, delimitador);
        getline(stream, Monto_Total_str, delimitador);
        getline(stream, Medio_Envio, delimitador);
        getline(stream, Estado_Envio, delimitador);

        int cantidad = stoi(Cantidad_str);
        float precioUnitario = stof(Precio_Unitario_str);
        float montoTotal = stof(Monto_Total_str);

        Venta nuevaVenta(ID_Venta_str, Fecha, Pais, Ciudad, Cliente, Producto, Categoria,
                         cantidad, precioUnitario, montoTotal, Medio_Envio, Estado_Envio);
        listaVentas.insertarUltimo(nuevaVenta);
    }
    archivo.close();
    cout << "Se han cargado " << listaVentas.getTamanio() << " ventas." << endl;

    // --- Menú Principal ---
    int opcion;
    do {
        cout << "\n--- MENU PRINCIPAL ---\n";
        cout << "1. Gestionar Ventas (Agregar, Eliminar, Modificar)\n";
        cout << "2. Consultas Dinamicas\n";
        cout << "3. Realizar todos los analisis\n";
        cout << "0. Salir\n";
        cout << "Ingrese su opcion: ";
        cin >> opcion;

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (opcion) {
            case 1: 
                mostrarMenuGestionVentas(listaVentas);
                break;
            case 2: 
                mostrarMenuConsultas(listaVentas);
                break;
            case 3: 
                cout << "\nRealizando analisis...\n";
                realizarTodosLosAnalisis(listaVentas);
                break;
            case 0:
                cout << "Saliendo del programa. Hasta luego!\n";
                break;
            default:
                cout << "Opcion no valida. Intente de nuevo.\n";
                break;
        }
    } while (opcion != 0);

    clock_t total_program_end = clock(); // Finalizar medición del tiempo total del programa

    cout << "\nTiempo de ejecucion total: " <<g_condCounters.time_total_analisis + g_condCounters.time_total_gestion + g_condCounters.time_total_consultas << " segundos" << endl;

    cout << "\n--- Conteo Total de Condicionales Ejecutados por Proceso Principal ---\n"<<endl;
    cout << "analizarTop5CiudadesPorPais: " << g_condCounters.analizarTop5CiudadesPorPais_ifs << " condicionales\n";
    cout << "analizarMontoTotalPorProductoPorPais: " << g_condCounters.analizarMontoTotalPorProductoPorPais_ifs << " condicionales\n";
    cout << "analizarPromedioVentasPorCategoriaPorPais: " << g_condCounters.analizarPromedioVentasPorCategoriaPorPais_ifs << " condicionales\n";
    cout << "analizarMedioEnvioMasUtilizadoPorPais: " << g_condCounters.analizarMedioEnvioMasUtilizadoPorPais_ifs << " condicionales\n";
    cout << "analizarMedioEnvioMasUtilizadoPorCategoria: " << g_condCounters.analizarMedioEnvioMasUtilizadoPorCategoria_ifs << " condicionales\n";
    cout << "analizarDiaMayorVentas: " << g_condCounters.analizarDiaMayorVentas_ifs << " condicionales\n";
    cout << "analizarProductoMasYMenosVendido: " << g_condCounters.analizarProductoMasYMenosVendido_ifs << " condicionales\n";
    cout << "GESTION (eliminarVenta): " << g_condCounters.eliminarVenta_ifs << " condicionales\n";
    cout << "GESTION (modificarVenta): " << g_condCounters.modificarVenta_ifs << " condicionales\n";
    cout << "CONSULTA (listarVentasPorCiudad): " << g_condCounters.listarVentasPorCiudad_ifs << " condicionales\n";
    cout << "CONSULTA (listarVentasPorRangoFechasPorPais): " << g_condCounters.listarVentasPorRangoFechasPorPais_ifs << " condicionales\n";
    cout << "CONSULTA (compararDosPaises): " << g_condCounters.compararDosPaises_ifs << " condicionales\n";
    cout << "CONSULTA (compararDosProductosPorPais): " << g_condCounters.compararDosProductosPorPais_ifs << " condicionales\n";
    cout << "CONSULTA (buscarProductosPorDebajoUmbralPorPais): " << g_condCounters.buscarProductosPorDebajoUmbralPorPais_ifs << " condicionales\n";
    cout << "CONSULTA (buscarProductosPorEncimaUmbral): " << g_condCounters.buscarProductosPorEncimaUmbral_ifs << " condicionales\n";

    return 0;
}