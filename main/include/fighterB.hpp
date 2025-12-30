#ifndef FIGHTER_B_HPP
#define FIGHTER_B_HPP

#include <iostream>
#include "cadmium/modeling/devs/atomic.hpp"
#include "fighter_state.hpp"
#include "messages.hpp"
#include "technique_loader.hpp"
#include "role_helper.hpp"

using namespace cadmium;

class fighterB : public Atomic<FighterState>
{
public:
    Port<FightUpdate> in;
    Port<FightAction> out;

    fighterB(const std::string &id)
        : Atomic<FighterState>(id, FighterState("fighterB"))
    {
        in = addInPort<FightUpdate>("in");
        out = addOutPort<FightAction>("out");

        state.techniques = loadTechniqueCSV("Grafo.csv");
    }

    void externalTransition(FighterState &s, double) const override
    {
        for (const auto &u : in->getBag())
        {
            std::cout << "[DEBUG][fighterB] Recibí update. Actor: "
                      << u.actor << ", Estado: " << u.my_pos 
                      << ", Mis puntos: " << u.my_points 
                      << ", Tiempo restante: " << u.time_remaining << "s" << std::endl;

            // Si el combate terminó
            if (u.finished)
            {
                s.can_think = false;
                s.is_attacker = false;
                std::cout << "[DEBUG][fighterB] Combate terminado. Fin." << std::endl;
                return;
            }

            // Actualizar información
            s.my_points = u.my_points;
            s.opponent_points = u.opp_points;
            s.time_remaining = u.time_remaining;
            s.total_time = 300.0 - u.time_remaining; // 🔴 Calcular tiempo transcurrido
            s.is_winning = (u.my_points > u.opp_points);

            // Extraer mi posición
            s.position = extractMyPosition(u.my_pos, "fighterB");
            s.opponent = extractMyPosition(u.my_pos, "fighterA");

            // Determinar si soy el atacante
            s.is_attacker = (u.actor == "fighterB");
            s.can_think = (u.actor == "fighterB");

            s.exchanges++; // 🔴 Incrementar intercambios

            std::cout << "[DEBUG][fighterB] Pos: " << s.position
                      << ", Opp: " << s.opponent
                      << ", Actor actual: " << u.actor
                      << ", Soy atacante: " << s.is_attacker
                      << ", CanThink: " << s.can_think 
                      << ", Puntos: " << s.my_points << "-" << s.opponent_points 
                      << ", Tiempo: " << s.total_time << "/300s" 
                      << ", Intercambios: " << s.exchanges << std::endl;
        }
    }

    void internalTransition(FighterState &s) const override
    {
        s.can_think = false;
        s.is_attacker = false;
    }

    void output(const FighterState &s) const override
    {
        if (!s.can_think || !s.is_attacker)
            return;

        std::cout << "[DEBUG][" << s.my_name << "] Buscando técnica. Mi posición: " << s.position 
                  << " (Tiempo: " << s.total_time << "s, Intercambios: " << s.exchanges << ")" << std::endl;

        std::vector<const Technique *> valid_techniques;
        
        // Buscar técnicas válidas como A
        std::string patron_A = "A:" + s.position;
        for (const auto &t : s.techniques)
        {
            size_t pos = t.from_state.find(patron_A);
            bool match = false;

            if (pos != std::string::npos)
            {
                if (pos + patron_A.length() < t.from_state.length())
                {
                    char siguiente = t.from_state[pos + patron_A.length()];
                    match = (siguiente == ',' || siguiente == ')');
                }
                else
                {
                    match = true;
                }
            }

            bool has_energy = s.stamina >= t.energy_cost;

            if (match && has_energy)
            {
                valid_techniques.push_back(&t);
            }
        }

        // Si no encuentro como A, buscar como O
        if (valid_techniques.empty())
        {
            std::string patron_O = "O:" + s.position;
            for (const auto &t : s.techniques)
            {
                size_t pos = t.from_state.find(patron_O);
                bool match = false;

                if (pos != std::string::npos)
                {
                    if (pos + patron_O.length() < t.from_state.length())
                    {
                        char siguiente = t.from_state[pos + patron_O.length()];
                        match = (siguiente == ',' || siguiente == ')');
                    }
                    else
                    {
                        match = true;
                    }
                }

                bool has_energy = s.stamina >= t.energy_cost;

                if (match && has_energy)
                {
                    valid_techniques.push_back(&t);
                }
            }
        }

        // Seleccionar técnica
        if (!valid_techniques.empty())
        {
            const Technique *selected = valid_techniques[0];
            double best_score = -1000.0;

            for (const auto *t : valid_techniques)
            {
                double score = 0.0;
                
                // 🔴 ESTRATEGIA MEJORADA BASADA EN TIEMPO
                
                // Base: eficiencia energética
                double efficiency = (t->energy_cost > 0) ? 
                    (double)t->energy_gain / t->energy_cost : 1.0;
                
                // Velocidad
                double speed = 1.0 / (t->time_cost + 0.1);
                
                // 🔴 ESTRATEGIA TEMPORAL:
                // - Primer minuto: juego seguro, defensas y posiciones
                // - 1-2 minutos: técnicas de control y puntos
                // - 2+ minutos: buscar finalización si hay ventaja
                
                if (s.total_time < 60) { // Primer minuto
                    if (t->type == "Defensiva") {
                        score += 3.0; // Prioridad alta a defensas
                    } else if (t->type == "Neutra") {
                        score += 2.0; // Técnicas de posición
                    } else if (t->type == "Ofensiva") {
                        score += 1.0; // Ofensivas bajas
                        if (t->category == "Sumision") {
                            score -= 5.0; // Evitar sumisiones tempranas
                        }
                    }
                } 
                else if (s.total_time < 120) { // Minutos 1-2
                    if (t->type == "Ofensiva") {
                        score += 2.5; // Buscar puntos
                        if (t->category == "Sumision") {
                            // Solo sumisiones si tengo ventaja clara
                            if (s.is_winning && s.my_points > s.opponent_points + 5) {
                                score += 1.0;
                            } else {
                                score -= 2.0;
                            }
                        }
                    } else if (t->type == "Defensiva") {
                        score += 1.5;
                    } else {
                        score += 1.0;
                    }
                }
                else { // Después de 2 minutos
                    if (t->type == "Ofensiva") {
                        score += 3.0; // Prioridad alta a ofensivas
                        if (t->category == "Sumision") {
                            // Sumisiones más probables si:
                            // 1. Tengo ventaja de puntos
                            // 2. El oponente está cansado (baja stamina)
                            // 3. Llevo varios intercambios
                            if (s.is_winning && s.stamina > 40 && s.exchanges > 8) {
                                score += 3.0;
                                std::cout << "[DEBUG][" << s.my_name 
                                          << "] Considerando sumisión (ventaja tardía)" << std::endl;
                            }
                        }
                    } else if (t->type == "Defensiva") {
                        score += 1.0;
                    } else {
                        score += 0.5;
                    }
                }
                
                // Bonus por situación específica
                if (!s.is_winning && s.time_remaining < 60) {
                    // Perdiendo y poco tiempo -> ofensiva agresiva
                    if (t->type == "Ofensiva") score += 2.0;
                }
                
                if (s.is_winning && s.time_remaining < 60) {
                    // Ganando y poco tiempo -> juego seguro
                    if (t->type == "Defensiva") score += 1.5;
                }
                
                // Penalización por bajo stamina
                double stamina_penalty = (100.0 - s.stamina) / 200.0; // Penalización reducida
                
                score = score + efficiency * 1.0 + speed * 0.5 - stamina_penalty;
                
                if (score > best_score)
                {
                    best_score = score;
                    selected = t;
                }
            }

            std::cout << "[DEBUG][" << s.my_name << "] Seleccionada: "
                      << selected->name
                      << " (Tipo: " << selected->type << "/" << selected->category
                      << ", E: " << selected->energy_cost << "/"
                      << selected->energy_gain << " T: " << selected->time_cost << "s)" << std::endl;
            out->addMessage(FightAction(s.my_name, *selected));
        }
        else
        {
            std::cout << "[ERROR][" << s.my_name << "] No encontré técnica válida"
                      << " para posición: " << s.position << std::endl;
        }
    }

    double timeAdvance(const FighterState &s) const override
    {
        if (s.can_think && s.is_attacker)
        {
            // 🔴 TIEMPO DE PENSAMIENTO DINÁMICO
            // Base: 0.5-1.0 segundos
            // Más cansado = piensa más lento
            double base_time = 0.5 + (rand() % 500) / 1000.0; // 0.5-1.0s aleatorio
            double stamina_factor = 1.0 + (100.0 - s.stamina) / 200.0; // +0% a +50%
            
            return base_time * stamina_factor;
        }
        return std::numeric_limits<double>::infinity();
    }
};

#endif