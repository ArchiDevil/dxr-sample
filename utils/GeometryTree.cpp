#include "GeometryTree.h"

GeometryTree::GeometryTree(float width, float minNodeSize)
    : _root{0.0f, 0.0f, width, false}
    , _minNodeSize(minNodeSize)
{
}

void GeometryTree::AddPoint(float x, float y)
{
    _root.AddPoint(x, y, _minNodeSize);
}

const TreeNode& GeometryTree::GetRoot() const
{
    return _root;
}
