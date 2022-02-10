#pragma once

#include <vector>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Curve
{
public:
    Curve() {};
    int samples = 50;
    std::vector<glm::vec3> control_points;
    std::vector<glm::vec3> points;

    glm::vec3 at(int i) const {
        return points[i];
    }
    void add_point(const glm::vec3& point);
};

void Curve::add_point(const glm::vec3& point)
{
    control_points.push_back(point);
    if(control_points.size() < 4)
        return;

    int ctrl_pt = control_points.size() - 4;
    for(int i = 0; i <= samples; i++)
    {
        // Implementation of BSpline algorithm for 3D Points
        float u=(float)i / (float)samples;
        glm::vec3 point;
        point = u * u * u * ((-1.f) * control_points[ctrl_pt] + 3.f * control_points[ctrl_pt + 1] - 3.f * control_points[ctrl_pt + 2] + control_points[ctrl_pt + 3]) / 6.f;
        point += u * u * (3.f * control_points[ctrl_pt] - 6.f * control_points[ctrl_pt + 1]+ 3.f * control_points[ctrl_pt + 2]) / 6.f;
        point += u * ((-3.f) * control_points[ctrl_pt] + 3.f * control_points[ctrl_pt + 2]) / 6.f;
        point += (control_points[ctrl_pt] + 4.f * control_points[ctrl_pt + 1] + control_points[ctrl_pt + 2]) / 6.f;
        points.push_back(point);
    }
}