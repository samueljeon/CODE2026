//
// Created by Adam G. on 4/26/25.
//

#ifndef VISUAL_H
#define VISUAL_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include "Controller.h"

namespace Visual {
    void DrawImGuiController(const Controller& ctrl);
}

#endif // VISUAL_H
