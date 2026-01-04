#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "technique.hpp"

struct FighterState {
    std::string my_name;
    std::string position;
    std::string opponent;
    bool can_think = false;
    bool is_attacker = false;
    int stamina = 100;
    int my_points = 0;
    int opponent_points = 0;
    double time_remaining = 300.0;
    bool is_winning = false;
    int exchanges = 0;
    double total_time = 0.0;
    
    // 🔴 NUEVO: Historial de técnicas usadas recientemente
    std::vector<std::string> recent_techniques;
    const int MAX_RECENT_TECHNIQUES = 3;  // No repetir en los últimos 3 intentos
    
    // 🔴 NUEVO: Historial de posiciones
    std::vector<std::string> recent_positions;
    const int MAX_RECENT_POSITIONS = 5;
    
    std::vector<Technique> techniques;
    
    FighterState(const std::string& name) : my_name(name) {}
    
    // 🔴 NUEVO: Método para verificar si una técnica fue usada recientemente
    bool wasRecentlyUsed(const std::string& tech_id) const {
        return std::find(recent_techniques.begin(), recent_techniques.end(), tech_id) 
               != recent_techniques.end();
    }
    
    // 🔴 NUEVO: Método para registrar técnica usada
    void recordTechnique(const std::string& tech_id) {
        recent_techniques.push_back(tech_id);
        if (recent_techniques.size() > MAX_RECENT_TECHNIQUES) {
            recent_techniques.erase(recent_techniques.begin());
        }
    }
    
    // 🔴 NUEVO: Registrar posición
    void recordPosition(const std::string& pos) {
        recent_positions.push_back(pos);
        if (recent_positions.size() > MAX_RECENT_POSITIONS) {
            recent_positions.erase(recent_positions.begin());
        }
    }
    
    // 🔴 NUEVO: Detectar bloqueo posicional
    bool isPositionalDeadlock(int consecutive_count = 3) const {
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
};

inline std::ostream &operator<<(std::ostream &os, const FighterState &s)
{
    os << "{name:" << s.my_name
       << ", position:" << s.position
       << ", opponent:" << s.opponent
       << ", can_think:" << s.can_think
       << ", is_attacker:" << s.is_attacker
       << ", stamina:" << s.stamina
       << ", my_points:" << s.my_points
       << ", opponent_points:" << s.opponent_points
       << ", time_remaining:" << s.time_remaining
       << ", is_winning:" << s.is_winning
       << ", exchanges:" << s.exchanges
       << ", total_time:" << s.total_time
       << ", recent_techs:" << s.recent_techniques.size()  // 🔴 AGREGADO
       << ", techniques_count:" << s.techniques.size()
       << "}";
    return os;
}