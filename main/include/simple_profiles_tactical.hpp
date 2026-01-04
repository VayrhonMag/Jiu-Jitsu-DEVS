#ifndef SIMPLE_PROFILES_TACTICAL_HPP
#define SIMPLE_PROFILES_TACTICAL_HPP

#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>
#include "technique.hpp"

inline void initRandomOnce()
{
    static bool initialized = false;
    if (!initialized)
    {
        srand(static_cast<unsigned int>(time(nullptr)));
        initialized = true;
    }
}

// Determinar si una posición es ventajosa/ofensiva
inline bool isAdvantagePosition(const std::string &my_position)
{
    // Solo posiciones de DOMINIO REAL (no guardia)
    return (my_position == "Montada_Superior" ||
            my_position == "Control_Lateral_Superior" ||
            my_position == "S-Mount_Superior" ||
            my_position == "Back_Mount_Superior" ||
            my_position == "North_South_Superior" ||
            my_position.find("Mount") != std::string::npos); // Cualquier montada
}

// Determinar si una posición es de desventaja/defensiva
inline bool isDisadvantagePosition(const std::string &my_position)
{
    // Posiciones donde ESTÁS en desventaja REAL
    return (my_position == "Montada_Inferior" ||
            my_position == "Control_Lateral_Inferior" ||
            my_position == "S-Mount_Inferior" ||
            my_position == "Back_Mount_Inferior" ||
            my_position == "North_South_Inferior" ||
            my_position.find("Sumision") != std::string::npos);
}

// Determinar si es posición neutral (guardia)
inline bool isNeutralPosition(const std::string &my_position)
{
    // Posiciones de guardia son neutrales
    return (my_position.find("Guardia") != std::string::npos ||
            my_position == "De_Pie" ||
            my_position.find("Media_Guardia") != std::string::npos ||
            my_position.find("Half_Guard") != std::string::npos);
}

// 🔴 NUEVA FUNCIÓN: Detectar bloqueo posicional
inline bool isPositionalDeadlock(const std::string& position, 
                                 const std::vector<std::string>& recent_positions,
                                 int consecutive_count = 3) {
    if (recent_positions.size() < consecutive_count) return false;
    
    // Verificar si la posición se repite
    int count = 0;
    for (int i = recent_positions.size() - 1; 
         i >= 0 && i >= (int)recent_positions.size() - consecutive_count; 
         i--) {
        if (recent_positions[i] == position) count++;
    }
    
    return count >= consecutive_count;
}

// LÓGICA TÁCTICA INTELIGENTE MEJORADA CON HISTORIAL
inline const Technique *selectTactical(
    const std::vector<const Technique *> &techniques,
    const std::string &my_position,
    const std::string &style, // "aggressive" o "defensive"
    bool is_winning,
    int stamina,
    const std::vector<std::string>& recent_techs = {},  // 🔴 PARÁMETRO NUEVO: historial
    const std::vector<std::string>& recent_positions = {}  // 🔴 PARÁMETRO NUEVO: posiciones
) {

    if (techniques.empty())
        return nullptr;
    initRandomOnce();

    // 🔴 NUEVO: Detectar bloqueo posicional
    bool positional_deadlock = isPositionalDeadlock(my_position, recent_positions, 3);
    if (positional_deadlock) {
        std::cout << "[TACTICAL] ⚠️ ¡BLOQUEO POSICIONAL DETECTADO en " 
                  << my_position << "! Forzando cambio..." << std::endl;
    }

    // 🔴 NUEVO: Crear lista de técnicas FRESCAS (no usadas recientemente)
    std::vector<const Technique *> fresh_techniques;
    std::vector<const Technique *> repeated_techniques;
    
    for (const auto *t : techniques) {
        bool was_recently_used = false;
        
        // Verificar si fue usada recientemente
        for (const auto& recent_id : recent_techs) {
            if (t->id == recent_id || t->name.find(recent_id) != std::string::npos) {
                was_recently_used = true;
                break;
            }
        }
        
        if (was_recently_used) {
            repeated_techniques.push_back(t);
        } else {
            fresh_techniques.push_back(t);
        }
    }
    
    // 🔴 PRINCIPIO: PREFERIR TÉCNICAS FRESCAS
    std::cout << "[TACTICAL] Técnicas disponibles: " << techniques.size() 
              << " | Frescas: " << fresh_techniques.size() 
              << " | Repetidas: " << repeated_techniques.size() << std::endl;
    
    // Usar la lista principal
    const std::vector<const Technique *>* tech_list = &techniques;
    
    if (!fresh_techniques.empty()) {
        tech_list = &fresh_techniques;
        std::cout << "[TACTICAL] Usando técnicas FRESCAS para variar" << std::endl;
    } else if (!repeated_techniques.empty()) {
        std::cout << "[TACTICAL] Todas las técnicas fueron usadas recientemente" << std::endl;
    }
    
    // 🔴 PRIMERO: Determinar REALMENTE la situación
    std::vector<const Technique *> tactical_choices;

    if (isAdvantagePosition(my_position))
    {
        // CASO 1: DOMINIO REAL (Montada, Control, etc.)
        std::cout << "[TACTICAL] DOMINIO: busco atacar/finalizar" << std::endl;
        for (const auto *t : *tech_list)
        {
            if (t->type == "Ofensiva")
            {
                tactical_choices.push_back(t);
            }
        }
        if (tactical_choices.empty())
            tactical_choices = *tech_list;
    }
    else if (isDisadvantagePosition(my_position))
    {
        // CASO 2: EN DESVENTAJA REAL
        std::cout << "[TACTICAL] DESVENTAJA: busco defender/escapar" << std::endl;
        for (const auto *t : *tech_list)
        {
            if (t->type == "Defensiva" ||
                t->category.find("Escape") != std::string::npos ||
                t->category.find("Raspado") != std::string::npos)
            {
                tactical_choices.push_back(t);
            }
        }
        if (tactical_choices.empty())
            tactical_choices = *tech_list;
    }
    else
    {
        // CASO 3: POSICIÓN NEUTRAL (Guardia, De Pie, etc.)
        std::cout << "[TACTICAL] NEUTRAL (" << my_position << "): estilo " << style << std::endl;

        if (style == "aggressive")
        {
            // Agresivo en neutral: prefiere pasar guardia/avanzar
            for (const auto *t : *tech_list)
            {
                if (t->category.find("Pase") != std::string::npos ||
                    t->type == "Ofensiva")
                {
                    tactical_choices.push_back(t);
                }
            }
            if (tactical_choices.empty())
                tactical_choices = *tech_list;
        }
        else
        { // defensive
            // Defensivo en neutral: prefiere guardia/control
            for (const auto *t : *tech_list)
            {
                if (t->category.find("Guardia") != std::string::npos ||
                    t->type == "Defensiva" ||
                    t->type == "Neutra")
                {
                    tactical_choices.push_back(t);
                }
            }
            if (tactical_choices.empty())
                tactical_choices = *tech_list;
        }
    }

    // 🔴 SEGUNDO: Dentro de opciones tácticas, aplicar ESTILO + HISTORIAL

    // Calcular pesos
    std::vector<int> weights(tactical_choices.size(), 10);

    for (size_t i = 0; i < tactical_choices.size(); i++)
    {
        const auto *t = tactical_choices[i];

        // 🔴 NUEVO: Verificar si es una técnica reciente
        bool is_recent = false;
        for (const auto& recent_id : recent_techs) {
            if (t->id == recent_id || t->name.find(recent_id) != std::string::npos) {
                is_recent = true;
                break;
            }
        }
        
        // 🔴 GRAN PENALIZACIÓN POR REPETIR TÉCNICA
        if (is_recent) {
            weights[i] -= 30;  // Penalización MUY fuerte
            std::cout << "[TACTICAL] Penalizando " << t->name << " (-30) por uso reciente" << std::endl;
        }
        
        // 🔴 BONUS POR ROMPER BLOQUEO POSICIONAL
        if (positional_deadlock) {
            // Bonus para técnicas que CAMBIAN la posición
            if (t->to_state_success.find(my_position) == std::string::npos &&
                t->to_state_success != t->from_state) {
                weights[i] += 40;  // Bonus enorme por cambiar posición
                std::cout << "[TACTICAL] Bonus +40 a " << t->name << " para romper bloqueo" << std::endl;
            }
        }

        // BONUS por estilo
        if (style == "aggressive")
        {
            if (t->type == "Ofensiva")
                weights[i] += 20;
            if (t->category.find("Sumision") != std::string::npos)
            {
                // Sumisiones solo si tengo ventaja o buen stamina
                if (isAdvantagePosition(my_position) && stamina > 50)
                {
                    weights[i] += 15;
                }
                else
                {
                    weights[i] -= 15;  // Penalización por sumisión en mala posición
                }
            }
        }
        else
        { // defensive
            if (t->type == "Defensiva")
                weights[i] += 20;
            if (t->category.find("Raspado") != std::string::npos)
                weights[i] += 15;
            if (t->category.find("Escape") != std::string::npos)
                weights[i] += 10;

            // Defensivo también ataca si tiene buena oportunidad
            if (t->type == "Ofensiva" && isAdvantagePosition(my_position))
            {
                weights[i] += 10; // Bonus para contraataques
            }
        }

        // BONUS por situación de combate
        if (!is_winning && stamina > 60)
        {
            // Perdiendo pero con energía → más riesgo
            if (t->type == "Ofensiva")
                weights[i] += 10;
        }

        if (is_winning && stamina < 40)
        {
            // Ganando pero cansado → juego seguro
            if (t->type == "Defensiva" || t->type == "Neutra")
                weights[i] += 10;
        }
        
        // Bonus por eficiencia energética
        if (t->energy_cost > 0) {
            double efficiency = (double)t->energy_gain / t->energy_cost;
            if (efficiency > 1.5) weights[i] += 10;
        }
        
        // Bonus por velocidad (menos tiempo es mejor)
        if (t->time_cost < 3) weights[i] += 5;
        
        // Mínimo peso de 1
        if (weights[i] < 1) weights[i] = 1;
    }

    // Selección ponderada
    int total_weight = 0;
    for (int w : weights)
        total_weight += w;

    if (total_weight <= 0) {
        std::cout << "[TACTICAL] Error en pesos, seleccionando primera opción" << std::endl;
        return tactical_choices[0];
    }

    int random_val = rand() % total_weight;
    int cumulative = 0;

    for (size_t i = 0; i < tactical_choices.size(); i++)
    {
        cumulative += weights[i];
        if (random_val < cumulative)
        {
            std::cout << "[TACTICAL] Seleccionada " << tactical_choices[i]->name 
                      << " con peso " << weights[i] << "/" << total_weight << std::endl;
            return tactical_choices[i];
        }
    }

    return tactical_choices[0];
}

#endif