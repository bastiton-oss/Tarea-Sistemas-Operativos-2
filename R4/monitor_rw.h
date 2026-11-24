#pragma once
#include <condition_variable>
#include <mutex>
#include <vector>
#include "control_rw.h"
class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica p);
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;
    std::vector<AnotacionTraza> obtener_traza();   
private:
    Politica politica;
    std::mutex m;                         
    std::vector<AnotacionTraza> traza;     
    int  lectoras_activas = 0;     
    bool escritora_activa = false;  
    int  escritoras_esperando = 0;  
    std::condition_variable cv_lectoras;
    std::condition_variable cv_escritoras;
    void anotar(Tipo t, Accion a, long turno = -1);
    // Todas reciben el lock ya tomado.
    void entrar_lectura_escritores(std::unique_lock<std::mutex>& lk);
    void salir_lectura_escritores(std::unique_lock<std::mutex>& lk);
    void entrar_escritura_escritores(std::unique_lock<std::mutex>& lk);
    void salir_escritura_escritores(std::unique_lock<std::mutex>& lk);
};