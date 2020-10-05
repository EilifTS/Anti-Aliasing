struct ShadowPayload
{
    bool hit;
};

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}

[shader("closesthit")]
void ShadowCHS(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}