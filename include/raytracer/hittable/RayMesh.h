#ifndef RAYMESH_H
#define RAYMESH_H

#include "Hittable.h"
#include "../../editor/entity/util/Transform.h"
#include <numeric>
#include <glm/gtx/component_wise.hpp>
#include <algorithm>

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 n0, n1, n2;
};

struct BVHNode {
    glm::vec3 boundsMin, boundsMax;
    int left = -1, right = -1;
    int triStart = -1, triCount = 0;
};

class RayMesh : public Hittable {
public:
    RayMesh(const std::vector<float>& verts, const std::vector<unsigned int>& indices, const Transform& transform, std::shared_ptr<RayMaterial> material) {
        _material = material;

        for (size_t i = 0; i < indices.size(); i += 3) {
            auto v = [&](int idx) { return glm::vec3(verts[idx*6], verts[idx*6+1], verts[idx*6+2]); };
            auto n = [&](int idx) { return glm::vec3(verts[idx*6+3], verts[idx*6+4], verts[idx*6+5]); };

            Triangle tri;
            tri.v0 = v(indices[i]);   tri.n0 = n(indices[i]);
            tri.v1 = v(indices[i+1]); tri.n1 = n(indices[i+1]);
            tri.v2 = v(indices[i+2]); tri.n2 = n(indices[i+2]);
            _triangles.push_back(tri);
        }

        buildBVH();
        setTransform(transform);
    }

    float sdf(const glm::vec3&) const override { return 0.0f; }

    bool raymarch(const Ray& ray, HitRecord& rec) const override {
        glm::vec3 o = glm::vec3(_modelMatrixI * glm::vec4(ray.origin(), 1.0f));
        glm::vec3 d = glm::normalize(glm::vec3(_modelMatrixI * glm::vec4(ray.direction(), 0.0f)));

        float tMin = 1e30f;
        HitRecord tmpRec;
        bool hit = false;

        traverseBVH(0, o, d, tMin, tmpRec, hit);

        if (hit) {
            tmpRec.point = glm::vec3(_modelMatrix * glm::vec4(tmpRec.point, 1.0f));
            tmpRec.normal = glm::normalize(glm::vec3(_modelMatrixIT * glm::vec4(tmpRec.normal, 0.0f)));
            tmpRec.setFaceNormal(ray, tmpRec.normal);
            tmpRec.material = _material;
            rec = tmpRec;
        }

        return hit;
    }

    bool shadowMarch(const Ray& ray, float lightDist) const override {
        glm::vec3 o = glm::vec3(_modelMatrixI * glm::vec4(ray.origin(), 1.0f));
        glm::vec3 d = glm::normalize(glm::vec3(_modelMatrixI * glm::vec4(ray.direction(), 0.0f)));

        float tMin = lightDist;
        HitRecord tmp;
        bool hit = false;
        traverseBVH(0, o, d, tMin, tmp, hit);
        return hit;
    }
private:
    std::vector<Triangle> _triangles;
    std::vector<BVHNode> _bvh;
    std::vector<int> _leafTris;

    static constexpr int LEAF_SIZE = 4;

    void buildBVH() {
        std::vector<int> triIndices(_triangles.size());
        std::iota(triIndices.begin(), triIndices.end(), 0);
        _bvh.emplace_back();
        buildNode(0, triIndices, 0, triIndices.size());
    }

    glm::vec3 triCentroid(int i) const {
        return (_triangles[i].v0 + _triangles[i].v1 + _triangles[i].v2) / 3.0f;
    }

    std::pair<glm::vec3, glm::vec3> triBounds(int i) const {
        return {
            glm::min(_triangles[i].v0, glm::min(_triangles[i].v1, _triangles[i].v2)),
            glm::max(_triangles[i].v0, glm::max(_triangles[i].v1, _triangles[i].v2))
        };
    }

    void buildNode(int nodeIdx, std::vector<int>& triIndices, int start, int end) {
        glm::vec3 bMin(1e30f), bMax(-1e30f);
        for (int i = start; i < end; i++) {
            auto [mn, mx] = triBounds(triIndices[i]);
            bMin = glm::min(bMin, mn);
            bMax = glm::max(bMax, mx);
        }
        _bvh[nodeIdx].boundsMin = bMin;
        _bvh[nodeIdx].boundsMax = bMax;

        int count = end - start;
        if (count <= LEAF_SIZE) {
            int leafStart = (int)_leafTris.size();
            for (int i = start; i < end; i++)
                _leafTris.push_back(triIndices[i]);
            _bvh[nodeIdx].triStart = leafStart;
            _bvh[nodeIdx].triCount = count;
            return;
        }

        glm::vec3 extent = bMax - bMin;
        int axis = (extent.x > extent.y && extent.x > extent.z) ? 0 : (extent.y > extent.z ? 1 : 2);

        std::sort(triIndices.begin() + start, triIndices.begin() + end, [&](int a, int b) {
            return triCentroid(a)[axis] < triCentroid(b)[axis];
        });

        int mid = (start + end) / 2;
        int leftIdx = (int)_bvh.size(); _bvh.emplace_back();
        int rightIdx = (int)_bvh.size(); _bvh.emplace_back();
        _bvh[nodeIdx].left = leftIdx;
        _bvh[nodeIdx].right = rightIdx;

        buildNode(leftIdx, triIndices, start, mid);
        buildNode(rightIdx, triIndices, mid, end);
    }

    bool aabbHit(const BVHNode& node, const glm::vec3& o, const glm::vec3& d, float tMax) const {
        glm::vec3 invD = 1.0f / d;
        glm::vec3 t0 = (node.boundsMin - o) * invD;
        glm::vec3 t1 = (node.boundsMax - o) * invD;
        float tmin = glm::compMax(glm::min(t0, t1));
        float tmax = glm::compMin(glm::max(t0, t1));
        return tmax >= glm::max(tmin, 0.0f) && tmin < tMax;
    }

    void traverseBVH(int nodeIdx, const glm::vec3& o, const glm::vec3& d,
                     float& tMin, HitRecord& rec, bool& hit) const {
        const BVHNode& node = _bvh[nodeIdx];
        if (!aabbHit(node, o, d, tMin)) return;

        if (node.triCount > 0) {
            for (int i = node.triStart; i < node.triStart + node.triCount; i++)
                intersectTri(_triangles[_leafTris[i]], o, d, tMin, rec, hit);
            return;
        }

        traverseBVH(node.left, o, d, tMin, rec, hit);
        traverseBVH(node.right, o, d, tMin, rec, hit);
    }

    void intersectTri(const Triangle& tri, const glm::vec3& o, const glm::vec3& d,
                      float& tMin, HitRecord& rec, bool& hit) const {
        const float EPSILON = 1e-7f;
        glm::vec3 e1 = tri.v1 - tri.v0;
        glm::vec3 e2 = tri.v2 - tri.v0;
        glm::vec3 h = glm::cross(d, e2);
        float det = glm::dot(e1, h);

        if (fabs(det) < EPSILON) return;

        float invDet = 1.0f / det;
        glm::vec3 s = o - tri.v0;
        float u = glm::dot(s, h) * invDet;
        if (u < 0.0f || u > 1.0f) return;

        glm::vec3 q = glm::cross(s, e1);
        float v = glm::dot(d, q) * invDet;
        if (v < 0.0f || u + v > 1.0f) return;

        float t = glm::dot(e2, q) * invDet;
        if (t < EPSILON || t >= tMin) return;

        tMin = t;
        rec.t = t;
        rec.point = o + d * t;
        rec.normal = glm::normalize(tri.n0 * (1.0f - u - v) + tri.n1 * u + tri.n2 * v);
        hit = true;
    }
};

#endif