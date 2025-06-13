#ifndef QUICKSORT_H_
#define QUICKSORT_H_

#include <vector>     // Necesario para std::vector
#include <algorithm>  // Necesario para std::swap
#include <string>     // Necesario para std::string en CiudadMonto
#include <functional> // Necesario para std::function (aunque no estrictamente si pasas función normal)

// Es importante que CiudadMonto y compararCiudadesMonto estén definidos
// antes de que quickSort.h sea incluido, o que estén dentro de este .h
// Los incluyo aquí para que el quickSort.h sea autocontenido para el ejemplo.

struct CiudadMonto {
    std::string ciudad;
    float monto;

    CiudadMonto(std::string c = "", float m = 0.0f) : ciudad(c), monto(m) {}
};

// Función de comparación para ordenar CiudadMonto por monto (descendente)
bool compararCiudadesMonto(const CiudadMonto& a, const CiudadMonto& b) {
    return a.monto > b.monto; // Mayor monto primero
}


// Versión genérica de quickSort que acepta un comparador
template <class T, class Compare> // Removí el = std::function<...> para simplificar la plantilla por ahora
void quickSort(std::vector<T>& arr, int inicio, int fin, Compare comp) {
    if (inicio >= fin) return;

    int i = inicio;
    int j = fin;
    T pivot = arr[(inicio + fin) / 2];

    while (i <= j) {
        // Mientras arr[i] debe ir ANTES que pivot según el comparador
        // (es decir, arr[i] es "mayor" o igual que pivot en orden descendente)
        while (i <= j && comp(arr[i], pivot)) {
            i++;
        }
        // Mientras pivot debe ir ANTES que arr[j] según el comparador
        // (es decir, pivot es "mayor" o igual que arr[j] en orden descendente)
        while (i <= j && comp(pivot, arr[j])) {
            j--;
        }

        if (i <= j) {
            std::swap(arr[i], arr[j]);
            i++;
            j--;
        }
    }

    if (j > inicio)
        quickSort(arr, inicio, j, comp);
    if (i < fin)
        quickSort(arr, i, fin, comp);
}

#endif // QUICKSORT_H_