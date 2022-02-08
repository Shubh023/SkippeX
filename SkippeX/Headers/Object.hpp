#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

class Ray
{
public:
    glm::highp_f32vec3 point;
    glm::highp_f32vec3 dir;

    Ray() {};
    Ray(glm::highp_f32vec3 p, glm::highp_f32vec3 d) :
            point(p), dir(d)
    {};
    glm::highp_f32vec3 get_sample(float t)
    {
        glm::highp_f32vec3 result;
        glm::highp_f32vec3 direction = normalize(dir);
        result = glm::highp_f32vec3(point.x + t * direction.x, point.y + t * direction.y, point.z + t * direction.z);
        return result;
    }
};


class Object
{
public:
    explicit Object() = default;
    explicit Object(const glm::vec3& v, std::string type) : origin(v), type(type)
    {};

    virtual bool get_intersection(Ray& ray, glm::highp_f32vec3 &point, glm::highp_f32vec3 &normal, glm::vec2& t_val) const = 0;

    glm::vec3 origin;
    std::string type;
};


class Sphere : public Object
{
public:
    float radius;

    Sphere() {};
    Sphere(glm::vec3 p, float r) :
            Object(p, "Sphere"), radius(r)
    {};
    bool get_intersection(Ray& ray, glm::highp_f32vec3 &point, glm::highp_f32vec3 &normal, glm::vec2& t_val) const override
    {
        float t0 = 0.f, t1 = 0.f;
        // geometric solution
        glm::highp_f32vec3 L = origin - ray.point;
        float tca = glm::dot(L, ray.dir);
        // if (tca < 0) return false;
        float d2 = glm::dot(L, L) - tca * tca;
        float radius2 = 2 * radius;
        if (d2 > radius2) return false;
        float thc = sqrt(radius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        if (t0 > t1)
            std::swap(t0, t1);

        if (t0 < 0) {
            t0 = t1;
            if (t0 < 0)
                return false;
        }

        float t = t0;
        t_val[0] = t0;
        t_val[1] = t1;
        point = ray.get_sample(t);

        normal = glm::highp_f32vec3(point.x - origin.x, point.y - origin.y, point.z - origin.z);
        normal = glm::normalize(normal);


        return true;
    }
};

class Plane : public Object
{
public:
    glm::vec3 normal;
    float maxDist;

    Plane() {};
    Plane(glm::vec3 p, glm::vec3 n, float distmax = 10.f) :
            Object(p, "Plane"), normal(n), maxDist(distmax)
    {};

    bool get_intersection(Ray& ray, glm::highp_f32vec3 &point, glm::highp_f32vec3 &normal, glm::vec2& t_val) const override {
        float denom = glm::dot(normal, ray.dir);
        float t;
        if (denom > float(1e-6)) {
            glm::vec3 oc = origin - ray.point;
            t = glm::dot(oc, normal) / denom;
            if (t < 0)
                return false;
        }

        point = ray.get_sample(t);
        glm::vec3 p = point - origin;
        float d2 = glm::dot(p, p);
        if (glm::sqrt(d2) > maxDist)
            return false;

        normal = glm::highp_f32vec3(point.x - origin.x, point.y - origin.y, point.z - origin.z);
        normal = glm::normalize(normal);

        t_val[0] = t;
        t_val[1] = t;
        return true;
    };
};

using sObject = std::shared_ptr<Object>;
