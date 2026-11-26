#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "control_rw.h"
#include "control_atomico_rw.h"

struct Registro {
    long version= 0;
    long doble_version= 0;
};

struct DiagnosticoR2 {
    std::atomic<int> lectoras_activas{0};
    std::atomic<int> escritoras_activas{0};
    std::atomic<int> max_lectoras{0};
    std::atomic<long> violaciones_exclusion{0};
    std::atomic<long> inconsistencias{0};
};

static void actualizar_max(std::atomic<int>& maximo, int valor) {
    int actual= maximo.load();
    while (valor > actual && !maximo.compare_exchange_weak(actual, valor)) {}
}

struct ConfigE1 {
    int L, E;
    long K;
    int dur_lec_us, dur_esc_us;
};

static bool ejecutar_verificacion(const ConfigE1& cfg) {
    std::unique_ptr<ControlRW> control= std::make_unique<ControlAtomicoRW>();
    Registro reg{};
    DiagnosticoR2 diag{};

    std::vector<FallosAdmision> fallos_lec(cfg.L);
    std::vector<FallosAdmision> fallos_esc(cfg.E);
    std::vector<std::thread> hebras;
    hebras.reserve(cfg.L + cfg.E);

    auto t0= std::chrono::steady_clock::now();

    for (int i= 0; i < cfg.L; ++i) {
        hebras.emplace_back([&, i]() {
            for (long k= 0; k < cfg.K; ++k) {
                control->entrar_lectura();

                int act= diag.lectoras_activas.fetch_add(1) + 1;
                actualizar_max(diag.max_lectoras, act);
                if (diag.escritoras_activas.load() > 0) {
                    diag.violaciones_exclusion.fetch_add(1);
                }

                long v= reg.version;
                long dv= reg.doble_version;
                if (dv != 2 * v) {
                    diag.inconsistencias.fetch_add(1);
                }

                if (cfg.dur_lec_us > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(cfg.dur_lec_us));
                }

                if (diag.escritoras_activas.load() > 0) {
                    diag.violaciones_exclusion.fetch_add(1);
                }
                diag.lectoras_activas.fetch_sub(1);

                control->salir_lectura();
            }
            fallos_lec[i]= ControlAtomicoRW::fallos_hebra_actual();
        });
    }

    for (int j= 0; j < cfg.E; ++j) {
        hebras.emplace_back([&, j]() {
            for (long k= 0; k < cfg.K; ++k) {
                control->entrar_escritura();

                int esc= diag.escritoras_activas.fetch_add(1) + 1;
                if (esc > 1 || diag.lectoras_activas.load() > 0) {
                    diag.violaciones_exclusion.fetch_add(1);
                }

                reg.version++;
                reg.doble_version= 2 * reg.version;

                if (cfg.dur_esc_us > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(cfg.dur_esc_us));
                }

                if (diag.escritoras_activas.load() > 1 || diag.lectoras_activas.load() > 0) {
                    diag.violaciones_exclusion.fetch_add(1);
                }
                diag.escritoras_activas.fetch_sub(1);

                control->salir_escritura();
            }
            fallos_esc[j]= ControlAtomicoRW::fallos_hebra_actual();
        });
    }

    for (auto& h : hebras) {
        h.join();
    }

    auto t1= std::chrono::steady_clock::now();
    double ms= std::chrono::duration<double, std::milli>(t1 - t0).count();

    FallosAdmision tot_lec{}, tot_esc{};
    for (const auto& f : fallos_lec) {
        tot_lec.recurso_ocupado+= f.recurso_ocupado;
        tot_lec.escritoras_esperando+= f.escritoras_esperando;
        tot_lec.cas_fallido+= f.cas_fallido;
    }
    for (const auto& f : fallos_esc) {
        tot_esc.recurso_ocupado+= f.recurso_ocupado;
        tot_esc.cas_fallido+= f.cas_fallido;
    }

    long version_esperada= static_cast<long>(cfg.E) * cfg.K;
    bool correcto= (reg.version == version_esperada) &&
                    (reg.doble_version == 2 * version_esperada) &&
                    (diag.inconsistencias.load() == 0) &&
                    (diag.violaciones_exclusion.load() == 0);

    std::cout << "modo=atomic politica=escritores"
              << " L=" << cfg.L << " E=" << cfg.E << " K=" << cfg.K
              << " dur=(" << cfg.dur_lec_us << "," << cfg.dur_esc_us << ")us\n"
              << "  version=" << reg.version << " doble_version=" << reg.doble_version
              << " inconsistencias=" << diag.inconsistencias.load()
              << " viol_exclusion=" << diag.violaciones_exclusion.load()
              << " max_lectoras=" << diag.max_lectoras.load()
              << " tiempo=" << ms << "ms"
              << " correcto=" << (correcto ? "SI" : "NO") << "\n"
              << "  fallos_lectoras:   recurso_ocupado=" << tot_lec.recurso_ocupado
              << " escritoras_esperando=" << tot_lec.escritoras_esperando
              << " cas_fallido=" << tot_lec.cas_fallido << "\n"
              << "  fallos_escritoras: recurso_ocupado=" << tot_esc.recurso_ocupado
              << " cas_fallido=" << tot_esc.cas_fallido << "\n\n";

    return correcto;
}

int main() {
    std::vector<ConfigE1> casos= {
        {1, 1,  20,  500, 500},
        {6, 2, 100,  200, 500},
        {2, 6, 100,  500, 200},
        {8, 8, 100,    0,   0},
        {8, 1, 200, 1000, 100},
    };

    bool ok= true;
    for (const auto& c : casos) {
        ok= ejecutar_verificacion(c) && ok;
    }
    return ok ? 0 : 1;
}