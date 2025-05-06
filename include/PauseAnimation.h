//
// Created by Diego Cid on 5/1/25.
//

#ifndef PAUSEANIMATION_H
#define PAUSEANIMATION_H
#include "Object3D.h"
#include "Animation.h"
class PauseAnimation : public Animation {
private:
    void applyAnimation(float dt) override {}
public:
    /**
     * @brief Constructs a animation of a constant rotation by the given total rotation
     * angle, linearly interpolated across the given duration.
     */
    PauseAnimation(Object3D& object, float duration) :
        Animation(object, duration) {}
};
#endif //PAUSEANIMATION_H
