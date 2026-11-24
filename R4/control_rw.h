#pragma once
#include <vector>
class ControlRW {
public:
    virtual ~ControlRW() = default;
    virtual void entrar_lectura() = 0;
    virtual void salir_lectura() = 0;
    virtual void entrar_escritura() = 0;
    virtual void salir_escritura() = 0;
};
enum class Politica { Lectores, Escritores, Fifo };
enum class Tipo { Lectora, Escritora };
enum class Accion { Pide, Entra, Sale };
struct AnotacionTraza {
    int hebra;     
    Tipo tipo;
    Accion accion;
    long turno;    
};
extern thread_local int id_hebra;