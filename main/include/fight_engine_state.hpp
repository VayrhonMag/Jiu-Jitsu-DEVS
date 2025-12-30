#ifndef FIGHT_ENGINE_STATE_HPP
#define FIGHT_ENGINE_STATE_HPP

#include <string>
#include <ostream>
#include <cstdlib>  // Para rand()
#include <ctime>    // Para time()

struct FightEngineState
{
    bool finished = false;
    int exchanges = 0;
    std::string global_state = "Pos(A:De_Pie, O:De_Pie)";

    // 🔴 MAPEO DE ROLES: quién es A (Atacante) y O (Oponente) en este momento
    std::string actor_A = "fighterA"; // Quién representa el rol A
    std::string actor_O = "fighterB"; // Quién representa el rol O

    // 🔴 TIEMPO DE PENSAMIENTO DINÁMICO (basado en energía y experiencia)
    double think_time_A = 1.5; // Tiempo base para pensar (segundos)
    double think_time_B = 1.5;
    
    // Tiempo total transcurrido
    double total_time = 0.0;
    
    // Tiempo máximo del combate (5 minutos = 300 segundos)
    const double MAX_FIGHT_TIME = 300.0;

    bool has_output = true;
    bool last_success = false;
    std::string current_actor = ""; // Quién ataca AHORA
    std::string last_actor = "";    // Quién atacó la última vez
    bool thinking = false;
    
    // Energía de los luchadores
    int energy_fighterA = 100;
    int energy_fighterB = 100;
    
    // 🔴 NUEVO: Sistema de puntos (SOLO técnicas ofensivas)
    int points_fighterA = 0;
    int points_fighterB = 0;
    
    // Tipos de victoria
    enum VictoryType {
        NONE,
        SUBMISSION,
        KO,
        POINTS,
        DOUBLE_KO
    } victory_type = NONE;
    
    std::string winner = "";

    FightEngineState()
    {
        // Inicializar random seed
        srand(static_cast<unsigned int>(time(nullptr)));
        
        // 🔴 Inicializar tiempos de pensamiento aleatorios
        think_time_A = 0.8 + (rand() % 700) / 1000.0; // 0.8-1.5 segundos
        think_time_B = 0.8 + (rand() % 700) / 1000.0;
        
        // Inicial: quien tenga menor tiempo de pensamiento empieza
        if (think_time_A <= think_time_B)
        {
            actor_A = "fighterA";
            actor_O = "fighterB";
            current_actor = "fighterA";
        }
        else
        {
            actor_A = "fighterB";
            actor_O = "fighterA";
            current_actor = "fighterB";
        }
        
        std::cout << "[DEBUG][ENGINE] Tiempos de pensamiento - A: " << think_time_A 
                  << "s, B: " << think_time_B << "s" << std::endl;
        std::cout << "[DEBUG][ENGINE] Primer atacante (más rápido): " << current_actor << std::endl;
    }
    
    // 🔴 Método para actualizar tiempos de pensamiento basado en energía
    void updateThinkTimes() {
        // Más energía = piensa más rápido
        think_time_A = 1.5 * (100.0 - energy_fighterA) / 100.0 + 0.5;
        think_time_B = 1.5 * (100.0 - energy_fighterB) / 100.0 + 0.5;
        
        // Límites: 0.5s mínimo, 2.0s máximo
        if (think_time_A < 0.5) think_time_A = 0.5;
        if (think_time_A > 2.0) think_time_A = 2.0;
        if (think_time_B < 0.5) think_time_B = 0.5;
        if (think_time_B > 2.0) think_time_B = 2.0;
    }
};

inline std::ostream &operator<<(std::ostream &os, const FightEngineState &s)
{
    os << "{finished:" << s.finished
       << ", exchanges:" << s.exchanges
       << ", global_state:" << s.global_state
       << ", total_time:" << s.total_time
       << ", energy_A:" << s.energy_fighterA
       << ", energy_B:" << s.energy_fighterB
       << ", points_A:" << s.points_fighterA
       << ", points_B:" << s.points_fighterB
       << ", think_time_A:" << s.think_time_A
       << ", think_time_B:" << s.think_time_B
       << ", victory_type:" << s.victory_type
       << ", winner:" << s.winner
       << "}";
    return os;
}

#endif