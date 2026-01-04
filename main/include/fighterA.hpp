#ifndef FIGHTER_A_HPP
#define FIGHTER_A_HPP

#include "simple_profiles_tactical.hpp"
#include <iostream>
#include "cadmium/modeling/devs/atomic.hpp"
#include "fighter_state.hpp"
#include "messages.hpp"
#include "technique_loader.hpp"
#include "role_helper.hpp"

using namespace cadmium;

class fighterA : public Atomic<FighterState>
{
public:
    Port<FightUpdate> in;
    Port<FightAction> out;

    fighterA(const std::string &id)
        : Atomic<FighterState>(id, FighterState("fighterA"))
    {
        in = addInPort<FightUpdate>("in");
        out = addOutPort<FightAction>("out");

        state.techniques = loadTechniqueCSV("Grafo.csv");
    }

    void externalTransition(FighterState &s, double) const override
    {
        for (const auto &u : in->getBag())
        {
            std::cout << "[DEBUG][fighterA] Recibí update. Actor: "
                      << u.actor << ", Estado: " << u.my_pos
                      << ", Mis puntos: " << u.my_points
                      << ", Tiempo restante: " << u.time_remaining << "s" << std::endl;

            // Si el combate terminó
            if (u.finished)
            {
                s.can_think = false;
                s.is_attacker = false;
                std::cout << "[DEBUG][fighterA] Combate terminado. Fin." << std::endl;
                return;
            }

            // Actualizar información
            s.my_points = u.my_points;
            s.opponent_points = u.opp_points;
            s.time_remaining = u.time_remaining;
            s.total_time = 300.0 - u.time_remaining; // 🔴 Calcular tiempo transcurrido
            s.is_winning = (u.my_points > u.opp_points);

            // Extraer mi posición
            std::string old_position = s.position; // 🔴 Guardar posición anterior
            s.position = extractMyPosition(u.my_pos, "fighterA");
            s.opponent = extractMyPosition(u.my_pos, "fighterB");
            
            // 🔴 NUEVO: Registrar posición si cambió
            if (old_position != s.position) {
                s.recordPosition(s.position);
                std::cout << "[DEBUG][fighterA] Nueva posición registrada: " << s.position << std::endl;
            }

            // Determinar si soy el atacante
            s.is_attacker = (u.actor == "fighterA");
            s.can_think = (u.actor == "fighterA");

            s.exchanges++; // 🔴 Incrementar intercambios

            std::cout << "[DEBUG][fighterA] Pos: " << s.position
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

        // Seleccionar técnica CON HISTORIAL
        if (!valid_techniques.empty())
        {
            const Technique *selected = selectTactical(
                valid_techniques,
                s.position,   // posición actual
                "aggressive", // estilo
                s.is_winning, // va ganando?
                s.stamina,    // energía
                s.recent_techniques,  // 🔴 PASAR HISTORIAL de técnicas
                s.recent_positions    // 🔴 PASAR HISTORIAL de posiciones
            );

            if (selected) {
                std::cout << "[DEBUG][" << s.my_name << "] Seleccionada: "
                          << selected->name
                          << " (Tipo: " << selected->type << "/" << selected->category
                          << ", E: " << selected->energy_cost << "/"
                          << selected->energy_gain << " T: " << selected->time_cost << "s)" << std::endl;
            
                // 🔴 REGISTRAR TÉCNICA USADA
                const_cast<FighterState&>(s).recordTechnique(selected->id);
                std::cout << "[DEBUG][" << s.my_name << "] Técnica registrada en historial: " 
                          << selected->id << std::endl;
            
                out->addMessage(FightAction(s.my_name, *selected));
            } else {
                std::cout << "[ERROR][" << s.my_name << "] No se pudo seleccionar técnica" << std::endl;
            }
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
            // Más cansado = piensa más lento
            double base_time = 0.5 + (rand() % 500) / 1000.0;          // 0.5-1.0s aleatorio
            double stamina_factor = 1.0 + (100.0 - s.stamina) / 200.0; // +0% a +50%

            return base_time * stamina_factor;
        }
        return std::numeric_limits<double>::infinity();
    }
};

#endif