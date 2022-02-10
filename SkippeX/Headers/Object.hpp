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

class Triangle : public Object
{
public:
    Triangle(glm::vec3 _a, glm::vec3  _b, glm::vec3 _c)
    : Object(a, "Triangle"), a(_a), b(_b), c(_c)
    {};
    bool get_intersection(Ray& ray, glm::highp_f32vec3 &point, glm::highp_f32vec3 &normal, glm::vec2& t_val) const override {
        glm::vec3 orig = ray.point;
        glm::vec3 dir = ray.dir;
        glm::vec3 v0 = a;
        glm::vec3 v1 = b;
        glm::vec3 v2 = c;
        float t;

        glm::vec3 v0v1 = v1 - v0;
        glm::vec3 v0v2 = v2 - v0;
        // no need to normalize
        glm::vec3 N = glm::cross(v0v1,v0v2);


        float NdotRayDirection = glm::dot(N,dir);
        if (fabs(NdotRayDirection) < 0.000001f)
            return false;

        float d = -glm::dot(N,v0);

        t = -(glm::dot(N, orig) + d) / NdotRayDirection;


        if (t < 0) return false;

        glm::vec3 P = orig + t * dir;
        glm::vec3 C;

        glm::vec3 edge0 = v1 - v0;
        glm::vec3 vp0 = P - v0;
        C = glm::cross(edge0, vp0);
        if (glm::dot(N, C) < 0) return false;

        glm::vec3 edge1 = v2 - v1;
        glm::vec3 vp1 = P - v1;
        C = glm::cross(edge1,vp1);
        if (glm::dot(N,C) < 0)  return false;

        glm::vec3 edge2 = v0 - v2;
        glm::vec3 vp2 = P - v2;
        C = glm::cross(edge2,vp2);
        if (glm::dot(N, C) < 0) return false;

        normal = glm::vec3(N);
        float area2 = glm::length(N);
        point = glm::vec3(P);
        t_val[0] = t;
        t_val[1] = t;

        return true;
    }
    glm::vec3  a;
    glm::vec3  b;
    glm::vec3  c;
};


using sObject = std::shared_ptr<Object>;
