#include "control_atomico_rw.h"
#include <thread>
thread_local FallosAdmision ControlAtomicoRW::fallos;
void ControlAtomicoRW::entrar_lectura() {
    std::uint64_t esperado = estado.load();
    while (true) {
        if (escritora_activa(esperado)) {
            fallos.recurso_ocupado++;              
        } else if (esperando(esperado) > 0) {
            fallos.escritoras_esperando++;         
        } else {
            if (estado.compare_exchange_weak(esperado, esperado + UNA_LECTORA))
                return;                            
            fallos.cas_fallido++;
            continue;
        }
        std::this_thread::yield();                 
        esperado = estado.load();
    }
}

void ControlAtomicoRW::salir_lectura() {
    estado.fetch_sub(UNA_LECTORA);
}

void ControlAtomicoRW::entrar_escritura() {
    std::uint64_t esperado = estado.fetch_add(UNA_ESPERANDO) + UNA_ESPERANDO;
    while (true) {
        if (escritora_activa(esperado) || lectoras(esperado) > 0) {
            fallos.recurso_ocupado++;
        } else {
            std::uint64_t nuevo = esperado - UNA_ESPERANDO + ESCRITORA_ACT;
            if (estado.compare_exchange_weak(esperado, nuevo))
                return;
            fallos.cas_fallido++;
            continue;
        }
        std::this_thread::yield();
        esperado = estado.load();
    }
}

void ControlAtomicoRW::salir_escritura() {
    estado.fetch_sub(ESCRITORA_ACT);
}