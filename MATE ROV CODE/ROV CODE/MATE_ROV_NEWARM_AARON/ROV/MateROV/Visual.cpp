//
// Created by Adam G. on 4/26/25.
//

#include "Visual.h"

namespace Visual {
    void DrawImGuiController(const Controller& ctrl) {
        ImGui::Begin("Controller Visual");

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();

        // Estimate drawing size
        ImVec2 visualSize = ImVec2(600, 400); // width, height of entire controller layout

        // Centered origin
        ImVec2 offset = (contentRegion - visualSize) * 0.5f;
        ImVec2 origin = cursorPos + offset;

        auto drawButton = [&](ImVec2 p, ImVec2 size, bool pressed, ImU32 baseColor, ImU32 pressColor) {
            draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), pressed ? pressColor : baseColor, 4.0f);
        };

        // draw->AddRect(origin,origin + visualSize,IM_COL32(200, 200, 200, 255));

        // Joysticks
        float radius = 60.0f;

        float joystickPadding = 150.0f;

        auto ClampToCircle = [](ImVec2 v) {
            float lenSq = v.x * v.x + v.y * v.y;
            if (lenSq > 1.0f) {
                float len = sqrtf(lenSq);
                return ImVec2(v.x / len, v.y / len);
            }
            return v;
        };

        ImVec2 centerL = origin + ImVec2(joystickPadding, visualSize.y - radius - 10);
        ImVec2 centerR = origin + ImVec2(visualSize.x - joystickPadding, visualSize.y - radius - 10);


        draw->AddCircle(centerL, radius, IM_COL32(200, 200, 200, 255));
        draw->AddCircle(centerR, radius, IM_COL32(200, 200, 200, 255));

        ImU32 joyColor = IM_COL32(120, 180, 220, 255);
        ImU32 joyPressedColor = IM_COL32(40, 200, 40, 255);

        ImVec2 leftJoy = ClampToCircle(ImVec2(ctrl.LeftX(), ctrl.LeftY()));
        ImVec2 rightJoy = ClampToCircle(ImVec2(ctrl.RightX(), ctrl.RightY()));

        draw->AddCircleFilled(centerL + leftJoy * radius, 6.0f, ctrl.L3() ? joyPressedColor : joyColor);
        draw->AddCircleFilled(centerR + rightJoy * radius, 6.0f, ctrl.R3() ? joyPressedColor : joyColor);

        // D-Pad
        ImVec2 btnSize = ImVec2(20, 20);
        ImVec2 dpadOrigin = origin + ImVec2(50, visualSize.y / 2);

        std::array<bool, 4> dpad = {
            ctrl.DpadDown(),
            ctrl.DpadRight(),
            ctrl.DpadUp(),
            ctrl.DpadLeft()
        };

        // Offsets for each direction, centered around origin
        float halfW = btnSize.x * 0.5f;
        float halfH = btnSize.y * 0.5f;

        float dpadPadding = btnSize.x * 1.5f;

        std::array<ImVec2, 4> dpadOffsets = {
            ImVec2(0,  dpadPadding),  // down
            ImVec2( dpadPadding, 0),  // right
            ImVec2(0, -dpadPadding),  // up
            ImVec2(-dpadPadding, 0)   // left
        };

        for (int i = 0; i < 4; ++i) {
            ImVec2 pCenter = dpadOrigin + dpadOffsets[i];
            ImVec2 pTopLeft = pCenter - ImVec2(halfW, halfH);

            drawButton(pTopLeft, btnSize, dpad[i], IM_COL32(100, 100, 100, 255), IM_COL32(200, 200, 40, 255));
        }

        // Face buttons
        ImVec2 faceOrigin = origin + ImVec2(visualSize.x - 50, visualSize.y / 2);

        std::array<bool, 4> face = {
            ctrl.Cross(),     // down
            ctrl.Circle(),    // right
            ctrl.Triangle(),  // up
            ctrl.Square()     // left
        };

        float facePadding = btnSize.x * 1.5f;

        std::array<ImVec2, 4> faceOffsets = {
            ImVec2(0,  facePadding),   // Cross
            ImVec2( facePadding, 0),   // Circle
            ImVec2(0, -facePadding),   // Triangle
            ImVec2(-facePadding, 0)    // Square
        };

        for (int i = 0; i < 4; ++i) {
            ImVec2 pCenter = faceOrigin + faceOffsets[i];
            ImVec2 pTopLeft = pCenter - ImVec2(halfW, halfH);

            drawButton(pTopLeft, btnSize, face[i], IM_COL32(100, 100, 100, 255), IM_COL32(40, 200, 40, 255));
        }


        // Touchpad
        ImVec2 padSize = visualSize / 2;
        ImVec2 padPos = origin + ImVec2(padSize.x / 2, 10);// + ImVec2(0, -25);


        SDL_GameController* gc = ctrl.Raw();
        int pressed = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_TOUCHPAD);
        ImU32 padColor = pressed ? IM_COL32(255, 0, 0, 255) : IM_COL32(180, 180, 180, 255);
        draw->AddRect(padPos, padPos + padSize, padColor);

        Uint8 down;
        float tx, ty, pressure;
        int fingers = SDL_GameControllerGetNumTouchpadFingers(gc, 0);

        for (int i = 0; i < fingers; ++i) {
            if (SDL_GameControllerGetTouchpadFinger(gc, 0, i, &down, &tx, &ty, &pressure) == 0 && down) {
                ImVec2 p = padPos + ImVec2(tx * (padSize.x - 5), ty * (padSize.y - 5));
                draw->AddCircleFilled(p, 5.0f, IM_COL32(250, 50, 250, 255));
            }
        }

        // Button Colors
        ImU32 baseColor = IM_COL32(150, 150, 150, 255);
        ImU32 pressedColor = IM_COL32(255, 0, 0, 255);

        // PS Button
        ImVec2 psBtnPos = padPos + ImVec2(padSize.x / 2, padSize.y) + ImVec2(-btnSize.x / 2, 5);
        draw->AddRect(psBtnPos, psBtnPos + ImVec2(btnSize.x, btnSize.y / 2), ctrl.PSButton() ? pressedColor : baseColor);

        // Microphone Button
        draw->AddRect(psBtnPos + ImVec2(0, 15), psBtnPos + ImVec2(0, 15) + ImVec2(btnSize.x, btnSize.y / 2), ctrl.MicrophoneButton() ? pressedColor : baseColor);

        float tPadding = 5.0f;

        // Share Button
        ImVec2 shareBtnSize = ImVec2(20, 10);
        ImVec2 shareBtnPos = padPos + ImVec2(-shareBtnSize.x - tPadding, 0);
        draw->AddRect(shareBtnPos, shareBtnPos + shareBtnSize, ctrl.Share() ? pressedColor : baseColor);

        // Left Bumper (L1)

        ImVec2 leftBumpSize = ImVec2(30, 10);
        ImVec2 leftBumpPos = shareBtnPos + ImVec2(-10, shareBtnSize.y + tPadding);
        draw->AddRect(leftBumpPos, leftBumpPos + leftBumpSize, ctrl.L1() ? pressedColor : baseColor);

        // Option Button
        ImVec2 optionBtnPos = padPos + ImVec2(padSize.x + tPadding, 0);
        draw->AddRect(optionBtnPos, optionBtnPos + shareBtnSize, ctrl.Options() ? pressedColor : baseColor);

        // Right Bumper (R1)

        ImVec2 rightBumpPos = optionBtnPos + ImVec2(0, shareBtnSize.y + tPadding);
        draw->AddRect(rightBumpPos, rightBumpPos + leftBumpSize, ctrl.R1() ? pressedColor : baseColor);

        // Triggers
        ImVec2 triggerSize = ImVec2(30, 75);
        float tY = 10.0f;
        ImVec2 baseL = leftBumpPos + ImVec2(0, leftBumpSize.y + tPadding);
        ImVec2 baseR = rightBumpPos + ImVec2(0, leftBumpSize.y + tPadding);

        draw->AddRect(baseL, baseL + triggerSize, IM_COL32(150, 150, 150, 255));
        draw->AddRect(baseR, baseR + triggerSize, IM_COL32(150, 150, 150, 255));

        draw->AddRectFilled(
            ImVec2(baseL.x, baseL.y + triggerSize.y - triggerSize.y * ctrl.L2()),
            baseL + triggerSize,
            IM_COL32(255, 0, 0, 200)
        );
        draw->AddRectFilled(
            ImVec2(baseR.x, baseR.y + triggerSize.y - triggerSize.y * ctrl.R2()),
            baseR + triggerSize,
            IM_COL32(255, 0, 0, 200)
        );

        ImGui::Dummy(visualSize); // reserve layout space
        ImGui::End();
    }
}
