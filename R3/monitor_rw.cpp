#include "monitor_rw.h"
#include <stdexcept>
thread_local int id_hebra = -1;   
MonitorRW::MonitorRW(Politica p):politica(p) {}
void MonitorRW::anotar(Tipo t, Accion a, long turno) {
    traza.push_back({id_hebra, t, a, turno});
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lk(m);
    anotar(Tipo::Lectora, Accion::Pide);          
    switch (politica) {
        case Politica::Escritores: entrar_lectura_escritores(lk); break;
        default: throw std::logic_error("politica no implementada aun");
    }
    anotar(Tipo::Lectora, Accion::Entra);        
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lk(m);
    anotar(Tipo::Lectora, Accion::Sale);
    switch (politica) {
        case Politica::Escritores: salir_lectura_escritores(lk); break;
        default: throw std::logic_error("politica no implementada aun");
    }
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lk(m);
    anotar(Tipo::Escritora, Accion::Pide);
    switch (politica) {
        case Politica::Escritores: entrar_escritura_escritores(lk); break;
        default: throw std::logic_error("politica no implementada aun");
    }
    anotar(Tipo::Escritora, Accion::Entra);
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lk(m);
    anotar(Tipo::Escritora, Accion::Sale);
    switch (politica) {
        case Politica::Escritores: salir_escritura_escritores(lk); break;
        default: throw std::logic_error("politica no implementada aun");
    }
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::lock_guard<std::mutex> lk(m);   // copia en exclusión mutua (R6)
    return traza;
}

void MonitorRW::entrar_lectura_escritores(std::unique_lock<std::mutex>& lk) {
    while (escritora_activa || escritoras_esperando > 0)
        cv_lectoras.wait(lk);   // libera m mientras duerme, lo retoma al volver
    lectoras_activas++;
}

void MonitorRW::salir_lectura_escritores(std::unique_lock<std::mutex>&) {
    lectoras_activas--;
    if (lectoras_activas == 0 && escritoras_esperando > 0)
        cv_escritoras.notify_one();
}

void MonitorRW::entrar_escritura_escritores(std::unique_lock<std::mutex>& lk) {
    escritoras_esperando++;
    while (escritora_activa || lectoras_activas > 0)
        cv_escritoras.wait(lk);
    escritoras_esperando--;     
    escritora_activa = true;
}

void MonitorRW::salir_escritura_escritores(std::unique_lock<std::mutex>&) {
    escritora_activa = false;
    if (escritoras_esperando > 0) {
        cv_escritoras.notify_one();
    } else {
        cv_lectoras.notify_all();
    }
}