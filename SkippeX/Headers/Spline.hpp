#pragma once

#include <vector>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Curve
{
public:
    Curve() : _steps(100) {};

protected:
    std::vector<glm::vec3> _way_points;
public:
    void add_way_point(const glm::vec3& point)
    {
        _way_points.push_back(point);
        _on_way_point_added();
    }
    void clear()
    {
        _nodes.clear();
        _way_points.clear();
        _distances.clear();
    }

protected:
    void add_node(const glm::vec3& node) {
        _nodes.push_back(node);

        if(_nodes.size()==1)
        {
            _distances.push_back(0);
        }
        else
        {
            int new_node_index=_nodes.size() - 1;

            double segment_distance=(_nodes[new_node_index] - _nodes[new_node_index-1]).length();
            _distances.push_back(segment_distance + _distances[new_node_index-1]);
        }
    }
    virtual void _on_way_point_added()=0;

protected:
    std::vector<glm::vec3> _nodes;
    std::vector<double> _distances;
public:
    glm::vec3 node(int i) const { return _nodes[i]; }
    double length_from_starting_point(int i) const { return _distances[i]; }
    bool has_next_node(int i) const { return static_cast<int>(_nodes.size()) > i; }
    int node_count() const {  return static_cast<int>(_nodes.size()); }
    bool is_empty() { return _nodes.empty(); }
    double total_length() const
    {
        assert(!_distances.empty());
        return _distances[_distances.size() - 1];
    }

protected:
    int _steps;
public:
    void increment_steps(int steps) { _steps+=steps; }
    void set_steps(int steps) { _steps = steps; }
};

class CatmullRom : public Curve
{
public:
    CatmullRom()
            : Curve()
    {};

protected:
    void _on_way_point_added() override {
        if(_way_points.size() < 4)
            return;

        int new_control_point_index=_way_points.size() - 1;
        int pt=new_control_point_index - 2;

        for(int i=0; i<=_steps; i++)
        {
            double u=(double)i / (double)_steps;
            add_node(interpolate(u, _way_points[pt-1], _way_points[pt], _way_points[pt+1], _way_points[pt+2]));
        }
    }

protected:
    glm::vec3 interpolate(float u, const glm::vec3& P0, const glm::vec3& P1, const glm::vec3& P2, const glm::vec3& P3)
    {
        glm::vec3 point;
        point = u * u * u * ((-1.f) * P0 + 3.f * P1 - 3.f * P2 + P3) / 2.f;
        point += u * u * (2.f * P0 - 5.f * P1+ 4.f * P2 - P3) / 2.f;
        point += u * ((-1.f) * P0 + P2) / 2.f;
        point += P1;
        return point;
    }
};

class BSpline : public Curve
{
public:
    BSpline()
            : Curve()
    {};

protected:
    void _on_way_point_added() override {
        if(_way_points.size() < 4)
            return;
        int new_control_point_index=static_cast<int>(_way_points.size()) - 1;
        int pt=new_control_point_index - 3;
        for(int i=0; i<=_steps; i++)
        {
            double u=(double)i / (double)_steps;
            add_node(interpolate(u, _way_points[pt], _way_points[pt+1], _way_points[pt+2], _way_points[pt+3]));
        }
    }

protected:
    glm::vec3 interpolate(float u, const glm::vec3& P0, const glm::vec3& P1, const glm::vec3& P2, const glm::vec3& P3)
    {
        glm::vec3 point;
        point = u * u * u * ((-1.f) * P0 + 3.f * P1 - 3.f * P2 + P3) / 6.f;
        point += u * u * (3.f *P0 - 6.f * P1+ 3.f * P2) / 6.f;
        point += u * ((-3.f) * P0 + 3.f * P2) / 6.f;
        point += (P0 + 4.f * P1 + P2) / 6.f;
        return point;
    }
};