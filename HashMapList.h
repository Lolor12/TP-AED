#ifndef U05_HASH_HASHMAP_HASHMAPLIST_H_
#define U05_HASH_HASHMAP_HASHMAPLIST_H_

#include "HashEntry.h"
#include "Lista.h"
#include <vector>   // Necesario para std::vector
#include <utility>  // Necesario para std::pair

//Tabla hash con manejo de colisiones usando listas enlazadas
template <class K, class T>
class HashMapList {
private:
    Lista<HashEntry<K, T>> **tabla;

    unsigned int tamanio;

    static unsigned int hashFunc(K clave);

    unsigned int (*hashFuncP)(K clave);

public:
    explicit HashMapList(unsigned int k);

    HashMapList(unsigned int k, unsigned int (*hashFuncP)(K clave));

    void getList(K clave);

    void put(K clave, T valor);

    void remove(K clave);

    T get(K clave);

    ~HashMapList();

    bool esVacio();

    void print();

    // NUEVO MÉTODO: Obtener todas las entradas del HashMap
    std::vector<std::pair<K, T>> getAllEntries();
};

template <class K, class T>
HashMapList<K, T>::HashMapList(unsigned int k) {
    tamanio = k;
    tabla = new Lista<HashEntry<K, T>> *[tamanio];
    for(int i = 0; i < tamanio; i++) {
        tabla[i] = NULL;
    }
    hashFuncP = hashFunc;
}

template <class K, class T>
HashMapList<K, T>::HashMapList(unsigned int k, unsigned int (*fp)(K)) {
    tamanio = k;
    tabla = new Lista<HashEntry<K, T>> *[tamanio];
    for(int i = 0; i < tamanio; i++) {
        tabla[i] = NULL;
    }
    hashFuncP = fp;
}

template <class K, class T>
HashMapList<K, T>::~HashMapList() {
    for (unsigned int i = 0; i < tamanio; ++i) {
        if (tabla[i] != nullptr) {
            delete tabla[i]; // Llama al destructor de Lista, que a su vez libera los Nodos y HashEntry
        }
    }
    delete[] tabla;
}

template <class K, class T>
void HashMapList<K, T>::put(K clave, T valor) {
    unsigned int pos = hashFuncP(clave) % tamanio;

    if (tabla[pos] == nullptr) {
        tabla[pos] = new Lista<HashEntry<K, T>>();
    }

    // Busca si la clave ya existe para actualizar el valor
    Nodo<HashEntry<K, T>> *aux = tabla[pos]->getInicio();
    while (aux != nullptr) {
        if (aux->getDato().getClave() == clave) {
            aux->setDato(HashEntry<K, T>(clave, valor)); // Actualiza el valor
            return;
        }
        aux = aux->getSiguiente();
    }

    // Si la clave no existe, inserta una nueva entrada
    tabla[pos]->insertarUltimo(HashEntry<K, T>(clave, valor));
}

template <class K, class T>
void HashMapList<K, T>::remove(K clave) {
    unsigned int pos = hashFuncP(clave) % tamanio;

    if (tabla[pos] == nullptr) {
        throw std::runtime_error("Clave no encontrada para eliminar"); // O puedes lanzar la excepción 404
    }

    // Busca y elimina la entrada de la lista
    // Necesitamos una forma de remover por valor o clave en la Lista
    // Tu Lista.h tiene remover(int pos), por lo que necesitamos encontrar la posición.
    Nodo<HashEntry<K, T>> *actual = tabla[pos]->getInicio();
    int idx = 0;
    while (actual != nullptr) {
        if (actual->getDato().getClave() == clave) {
            tabla[pos]->remover(idx); // Eliminar por índice
            if (tabla[pos]->esVacia()) { // Si la lista queda vacía, eliminar el puntero
                delete tabla[pos];
                tabla[pos] = nullptr;
            }
            return;
        }
        actual = actual->getSiguiente();
        idx++;
    }

    throw std::runtime_error("Clave no encontrada para eliminar"); // O puedes lanzar la excepción 404
}


template <class K, class T>
T HashMapList<K, T>::get(K clave) {
    unsigned int pos = hashFuncP(clave) % tamanio;

    if (tabla[pos] == nullptr) {
        throw std::runtime_error("Clave no encontrada"); // Lanza una excepción si el bucket está vacío
    }

    Nodo<HashEntry<K, T>> *nodo = tabla[pos]->getInicio();
    while (nodo != nullptr) {
        if (nodo->getDato().getClave() == clave) {
            return nodo->getDato().getValor(); // Devuelve el valor asociado a la clave
        }
        nodo = nodo->getSiguiente();
    }

    throw std::runtime_error("Clave no encontrada"); // Si llegamos aquí, la clave no se encontró en la lista
}


template <class K, class T>
bool HashMapList<K, T>::esVacio() {
    for(unsigned int i = 0; i < tamanio; i++) { // Cambié int por unsigned int
        if(tabla[i] != NULL) {
            return false;
        }
    }
    return true;
}

template <class K, class T>
unsigned int HashMapList<K, T>::hashFunc(K clave) {
    // Implementación de hash por defecto para tipos que pueden ser casteados a unsigned int
    return (unsigned int) clave;
}

// Nueva implementación del método getAllEntries()
template <class K, class T>
std::vector<std::pair<K, T>> HashMapList<K, T>::getAllEntries() {
    std::vector<std::pair<K, T>> allEntries;
    for (unsigned int i = 0; i < tamanio; ++i) {
        if (tabla[i] != nullptr) {
            Nodo<HashEntry<K, T>>* nodo = tabla[i]->getInicio();
            while (nodo != nullptr) {
                // Asumiendo que HashEntry tiene getClave() y getValor() públicos y constantes
                allEntries.push_back(std::make_pair(nodo->getDato().getClave(), nodo->getDato().getValor()));
                nodo = nodo->getSiguiente();
            }
        }
    }
    return allEntries;
}


template <class K, class T>
void HashMapList<K, T>::getList(K clave) { //Método que devuelve la lista según la clave que recibe
    unsigned int pos = hashFuncP(clave) % tamanio;

    if(tabla[pos] == NULL) {
        throw 404; // Considera usar std::runtime_error("Clave no encontrada") para consistencia
    }

    Nodo<HashEntry<K, T>> *aux = tabla[pos]->getInicio();

    while (aux != NULL) {
        std::cout << aux->getDato().getValor() << std::endl;
        aux = aux->getSiguiente();
    }
}

template <class K, class T>
void HashMapList<K, T>::print() {
    for(unsigned int i = 0; i < tamanio; i++) { // Cambié int por unsigned int
        std::cout << "Bucket " << i << ": ";
        if(tabla[i] != NULL) {
            // Imprime los elementos de la lista enlazada en este bucket
            // Asumiendo que HashEntry::getClave() y getValor() son accesibles
            Nodo<HashEntry<K, T>> *aux = tabla[i]->getInicio();
            while (aux != NULL) {
                std::cout << "(" << aux->getDato().getClave() << ", " << aux->getDato().getValor() << ") ";
                aux = aux->getSiguiente();
            }
            std::cout << std::endl;
        } else {
            std::cout << "VACIO" << std::endl;
        }
    }
}


#endif // U05_HASH_HASHMAP_HASHMAPLIST_H_