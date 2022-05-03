#pragma once

#include <array>
#include <memory>
#include <optional>

constexpr bool Within(float xPos, float yPos, float xMin, float xMax, float yMin, float yMax)
{
    return xPos >= xMin && xPos <= xMax && yPos >= yMin && yPos <= yMax;
}

// this is a quadtree node that contains a bounding box only
struct TreeNode
{
    float xPos, yPos;
    float size;
    bool  hasObject = false;

    // this is not quite optimal, but for the sake of simplicity
    std::vector<TreeNode> children;

    void AddPoint(const float x, const float y, const float minSize)
    {
        const float newSize = size / 2.0f;
        if (newSize < minSize)
        {
            // put it here and do not split
            hasObject = true;
            return;
        }

        if (children.empty())
        {
            children.resize(4);
            children[0] = TreeNode{ xPos, yPos, newSize };
            children[1] = TreeNode{ xPos + newSize, yPos, newSize };
            children[2] = TreeNode{ xPos, yPos + newSize, newSize };
            children[3] = TreeNode{ xPos + newSize, yPos + newSize, newSize };
        }

        for (auto& child : children)
        {
            if (Within(x, y, child.xPos, child.xPos + child.size, child.yPos, child.yPos + child.size))
            {
                child.AddPoint(x, y, minSize);
                return;
            }
        }
    }
};

// This class builds a quadtree based on a set of points in 2D space
class GeometryTree
{
public:
    GeometryTree(float width, float minNodeSize);
    void      AddPoint(float x, float y);
    const TreeNode& GetRoot() const;

private:
    TreeNode    _root;
    const float _minNodeSize;
};
