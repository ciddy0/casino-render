//
// Created by Diego Cid on 5/6/25.
//

#ifndef TRANSLATIONANIMATION_H
#define TRANSLATIONANIMATION_H
#include "Animation.h"

class TranslationAnimation : public Animation {
    glm::vec3 m_perSecond;
    glm::vec3& m_objectPosition;
    void applyAnimation(float dt) override {
        m_objectPosition += m_perSecond * dt;
    };
public:
    TranslationAnimation(glm::vec3& objectPosition, float duration, const glm::vec3& totalPosition) :
       Animation(duration), m_objectPosition(objectPosition), m_perSecond(totalPosition / duration) {}
};

#endif //TRANSLATIONANIMATION_H
