#pragma once
#include <atomic>
#include <cstdint>
#include "control_rw.h"
struct FallosAdmision {
    long recurso_ocupado = 0;      
    long escritoras_esperando = 0; 
    long cas_fallido = 0;         
};

class ControlAtomicoRW : public ControlRW {
public:
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;
    static FallosAdmision fallos_hebra_actual() { return fallos; }

private:
    static constexpr std::uint64_t UNA_LECTORA   = 1ULL;
    static constexpr std::uint64_t MASK_LECTORAS = 0xFFFFULL;           
    static constexpr std::uint64_t ESCRITORA_ACT = 1ULL << 16;
    static constexpr std::uint64_t UNA_ESPERANDO = 1ULL << 32;  
    static constexpr std::uint64_t MASK_ESPERANDO = 0xFFFFULL << 32;
    static std::uint64_t lectoras(std::uint64_t s)   { return s & MASK_LECTORAS; }
    static bool escritora_activa(std::uint64_t s)    { return (s & ESCRITORA_ACT) != 0; }
    static std::uint64_t esperando(std::uint64_t s)  { return (s & MASK_ESPERANDO) >> 32; }
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free, "atomic<uint64_t> debe ser lock-free");
    std::atomic<std::uint64_t> estado{0};
    static thread_local FallosAdmision fallos;
};