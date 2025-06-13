#ifndef U02_LISTAS_LISTA_LISTA_H_
#define U02_LISTAS_LISTA_LISTA_H_

#include <iostream>
#include "Nodo.h"

/**
 * Clase que implementa una Lista Enlazada genérica, ya que puede
 * almacenar cualquier tipo de dato T
 * @tparam T cualquier tipo de dato
 */
template <class T>
class Lista {
    private:
        Nodo<T> *inicio;


    public:
        Lista();

        Lista(const Lista<T> &li);

        ~Lista();

        bool esVacia();

        int getTamanio() const; // <--- ¡Añade 'const' aquí!

        void insertar(int pos, T dato);

        void insertarPrimero(T dato);

        void insertarUltimo(T dato);

        void remover(int pos);

        T getDato(int pos) const; // <--- ¡Añade 'const' aquí!

        void reemplazar(int pos, T dato);

        void vaciar();

        void imprimir();

        void insertAfter2(int oldValue, int n, int newValue);

        Nodo<T> *getInicio();
};

/**
 * Constructor de la clase Lista
 * @tparam T
 */
template <class T>
Lista<T>::Lista() {
    inicio = nullptr;
}

/**
 * Constructor por copia de la clase Lista
 * @tparam T
 * @param li
 */
template <class T>
Lista<T>::Lista(const Lista<T> &li) {
    inicio = nullptr;
    Nodo<T> *aux = li.inicio;
    while(aux != nullptr) {
        insertarUltimo(aux->getDato());
        aux = aux->getSiguiente();
    }
}

/**
 * Destructor de la clase Lista, se encarga de liberar la memoria de todos los
 * nodos utilizados en la lista
 * @tparam T
 */
template <class T>
Lista<T>::~Lista() {
    vaciar();
}

/**
 * Función que informa si la lista enlazada esta vacia
 * @tparam T
 * @return true si esta vacia, false si no
 */
template <class T>
bool Lista<T>::esVacia() {
    return inicio == nullptr;
}

/**
 * Función que devuelve el tamaño de la lista enlazada
 * @tparam T
 * @return tamaño de la lista
 */
template <class T>
int Lista<T>::getTamanio() const { // <--- ¡Añade 'const' aquí también!
    int tamanio = 0;
    Nodo<T> *aux = inicio;

    while(aux != nullptr) {
        tamanio++;
        aux = aux->getSiguiente();
    }

    return tamanio;
}

/**
 * Función que inserta un nodo en la posicion indicada
 * @tparam T
 * @param pos posición a insertar
 * @param dato dato a insertar
 */
template <class T>
void Lista<T>::insertar(int pos, T dato) {
    if (pos < 0 || pos > getTamanio()) {
        throw 400; // Puedes cambiar por std::out_of_range
    }

    if (pos == 0) {
        insertarPrimero(dato);
        return;
    }

    if (pos == getTamanio()) {
        insertarUltimo(dato);
        return;
    }

    Nodo<T> *nuevo = new Nodo<T>(dato, nullptr);
    Nodo<T> *aux = inicio;

    for (int i = 0; i < pos - 1; i++) {
        aux = aux->getSiguiente();
    }

    nuevo->setSiguiente(aux->getSiguiente());
    aux->setSiguiente(nuevo);
}

/**
 * Función que inserta un nodo al principio de la lista
 * @tparam T
 * @param dato dato a insertar
 */
template <class T>
void Lista<T>::insertarPrimero(T dato) {
    Nodo<T> *nuevo = new Nodo<T>(dato, inicio);
    inicio = nuevo;
}

/**
 * Función que inserta un nodo al final de la lista
 * @tparam T
 * @param dato dato a insertar
 */
template <class T>
void Lista<T>::insertarUltimo(T dato) {
    Nodo<T> *nuevo = new Nodo<T>(dato, nullptr);

    if (esVacia()) {
        inicio = nuevo;
        return;
    }

    Nodo<T> *aux = inicio;
    while(aux->getSiguiente() != nullptr) {
        aux = aux->getSiguiente();
    }

    aux->setSiguiente(nuevo);
}

/**
 * Función que remueve un nodo de la posición indicada
 * @tparam T
 * @param pos posición a remover
 */
template <class T>
void Lista<T>::remover(int pos) {
    if (pos < 0 || pos >= getTamanio()) {
        throw 400; // Puedes cambiar por std::out_of_range
    }

    Nodo<T> *aBorrar = inicio;
    if (pos == 0) {
        inicio = inicio->getSiguiente();
        delete aBorrar;
        return;
    }

    Nodo<T> *aux = inicio;
    for (int i = 0; i < pos - 1; i++) {
        aux = aux->getSiguiente();
    }
    aBorrar = aux->getSiguiente();
    aux->setSiguiente(aBorrar->getSiguiente());
    delete aBorrar;
}

/**
 * Función que devuelve el dato de un nodo de la posición indicada
 * @tparam T
 * @param pos posición
 * @return dato del nodo
 */
template <class T>
T Lista<T>::getDato(int pos) const { // <--- ¡Añade 'const' aquí también!
    if (pos < 0 || pos >= getTamanio()) {
        throw 400; // Puedes cambiar por std::out_of_range
    }

    Nodo<T> *aux = inicio;
    for (int i = 0; i < pos; i++) {
        aux = aux->getSiguiente();
    }

    return aux->getDato();
}

/**
 * Función que reemplaza el dato de un nodo de la posición indicada
 * @tparam T
 * @param pos posición
 * @param dato nuevo dato
 */
template <class T>
void Lista<T>::reemplazar(int pos, T dato) {
    if (pos < 0 || pos >= getTamanio()) {
        throw 400; // Puedes cambiar por std::out_of_range
    }

    Nodo<T> *aux = inicio;
    for (int i = 0; i < pos; i++) {
        aux = aux->getSiguiente();
    }

    aux->setDato(dato);
}

/**
 * Función que vacia la lista enlazada
 * @tparam T
 */
template <class T>
void Lista<T>::vaciar() {
    Nodo<T> *aux = inicio, *aBorrar;

    while(aux != nullptr) {
        aBorrar = aux;
        aux = aux->getSiguiente();
        delete aBorrar;
    }

    inicio = nullptr;
}

/**
 * Función que imprime la lista enlazada
 * @tparam T
 */
template <class T>
void Lista<T>::imprimir() {
    Nodo<T> *aux = inicio;

    while(aux != nullptr) {
        std::cout << aux->getDato() << "->";
        aux = aux->getSiguiente();
    }

    std::cout << "NULL" << std::endl;
}

/**
 * Método que inserta un nodo con el valor newValue después de la enésima
 * ocurrencia de oldValue
 * @tparam T
 * @param oldValue valor dentro de la lista
 * @param n número de repeticiones de oldValue
 * @param newValue nuevo valor a insertar
 */
template <class T>
void Lista<T>::insertAfter2(int oldValue, int n, int newValue) {
    Nodo<T> *aux = inicio;
    int contador = 0;

    while (aux != nullptr) {
        // Asumiendo que T es un tipo que se puede comparar con int o tiene un operador de conversión
        // Si T no es int, necesitarías una comparación más específica, por ejemplo, aux->getDato().someField == oldValue
        // Para este ejemplo, mantendremos la comparación directa.
        if (aux->getDato() == oldValue) {
            contador++;
            if (contador == n) {
                Nodo<T> *nuevo = new Nodo<T>(newValue, aux->getSiguiente());
                aux->setSiguiente(nuevo);
                return; // Se insertó el elemento, salir
            }
        }
        aux = aux->getSiguiente();
    }
    // Si llegamos aquí, no se encontró la n-ésima ocurrencia
    // Podrías lanzar una excepción o imprimir un mensaje
    std::cout << "No se encontró la " << n << "-ésima ocurrencia de " << oldValue << std::endl;
}

template <class T>
Nodo<T> *Lista<T>::getInicio() {
    return inicio;
}


#endif // U02_LISTAS_LISTA_LISTA_H_