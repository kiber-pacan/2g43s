//
// Created by down1 on 07.01.2026.
//

#ifndef INC_2G43S_PLAYERENTITY_H
#define INC_2G43S_PLAYERENTITY_H
#include "Entity.hpp"


class PlayerEntity final : Entity  {
    int Health = 100;
    int Stamina = 100;
    int Food = 100;
    int Water = 100;
};


#endif //INC_2G43S_PLAYERENTITY_H