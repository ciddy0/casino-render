//
// Created by Diego Cid on 5/6/25.
//

#ifndef BEZIERANIMATION_H
#define BEZIERANIMATION_H
class BezierTranslationAnimation : public Animation {
    glm::vec3 m_p0, m_p1, m_p2, m_p3;
    glm::vec3& m_objectPosition;
    void applyAnimation(float dt) override {
        float t = currentTime() / duration();
        m_objectPosition = calculateCubicBezierPoint(t);
    };
    glm::vec3 calculateCubicBezierPoint(float t)  {
        float t2 = 1-t;
        float t2_squared = t2 * t2;
        float t2_cubed = t2_squared * t2;
        float t_squared = t * t;
        float t_cubed = t_squared* t;
        return t2_cubed * m_p0 + 3 * t2_squared * t * m_p1 + 3 * t2 * t_squared * m_p2
        + t_cubed * m_p3;
    }

public:
    BezierTranslationAnimation(glm::vec3& objectPosition, float duration, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3):
        Animation(duration),
        m_objectPosition(objectPosition),
        m_p0(p0), m_p1(p1), m_p2(p2), m_p3(p3) {}
};
#endif //BEZIERANIMATION_H
