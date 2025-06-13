#ifndef VENTA_H
#define VENTA_H

#include <string>
#include <iostream> 

using namespace std; 

class Venta {
public:
    string idVenta;
    string fecha;
    string pais;
    string ciudad;
    string cliente;
    string producto;
    string categoria;
    int cantidad;
    float precioUnitario;
    float montoTotal;
    string medioEnvio;
    string estadoEnvio;

    // Constructor
    Venta(string idV, string f, string p, string c, string cli,
          string prod, string cat, int cant, float precioU, float montoT,
          string medioE, string estadoE) :
        idVenta(idV), fecha(f), pais(p), ciudad(c), cliente(cli), producto(prod),
        categoria(cat), cantidad(cant), precioUnitario(precioU), montoTotal(montoT),
        medioEnvio(medioE), estadoEnvio(estadoE) {}

    // Constructor por defecto
    Venta() : cantidad(0), precioUnitario(0.0), montoTotal(0.0) {}

    // Nueva función mostrar()
    void mostrar() const {
        cout << "==================" << endl;
        cout << "Id de la venta : " << idVenta << endl;
        cout << "Fecha : " << fecha << endl;
        cout << "Pais : " << pais << endl;
        cout << "Ciudad a la que llega : " << ciudad << endl;
        cout << "Cliente : " << cliente << endl;
        cout << "Producto a despachar : " << producto << endl;
        cout << "Categoria del producto : " << categoria << endl;
        cout << "Cantidad: " << cantidad << endl;
        cout << "Precio unitario: " << precioUnitario << endl;
        cout << "Monto total: " << montoTotal << endl;
        cout << "Medio de envio: " << medioEnvio << endl;
        cout << "Estado del envio: " << estadoEnvio << endl;
    }
};

#endif // VENTA_H