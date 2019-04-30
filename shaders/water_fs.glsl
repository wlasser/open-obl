#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;
in vec4 ScreenPos;

uniform vec4 ambientLightColor;
uniform vec4 sunlightColor;
uniform vec4 sunlightDirection;
uniform mat4 view;
uniform mat4 proj;
uniform float nearClipDist;
uniform float farClipDist;
uniform vec4 viewportSize;
uniform float time;

uniform sampler2D diffuse;
uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

out vec4 FragColor;

void main() {
    vec4 homScreenPos = ScreenPos / ScreenPos.w;
    vec2 uv = homScreenPos.xy * 0.5f + 0.5f;

    vec3 lightCol = pow(sunlightColor.rgb, vec3(2.2f));
    vec3 lightDir = normalize(sunlightDirection.xyz);

    vec3 shallowCol = vec3(2.0f, 21.0f, 30.0f) / 255.0f * ambientLightColor.rgb;
    vec3 deepCol = vec3(32.0f, 46.0f, 53.0f) / 255.0f * ambientLightColor.rgb;
    vec3 reflectCol = vec3(31.0f, 68.0f, 75.0f) / 255.0f * ambientLightColor.rgb;

    float fresnelAmount = 0.15f;
    float reflectivityAmount = 0.9f;

    vec2 texCoord = TexCoord + vec2(time);

    // Compute normal map using the water diffuse as a height map.
    const ivec3 offset = ivec3(-1, 0, 1);
    float n01 = textureOffset(diffuse, texCoord, offset.xy).r;
    float n10 = textureOffset(diffuse, texCoord, offset.yx).r;
    float n21 = textureOffset(diffuse, texCoord, offset.zy).r;
    float n12 = textureOffset(diffuse, texCoord, offset.yz).r;
    vec3 nU = normalize(vec3(2.0f, n21 - n01, 0.0f));
    vec3 nV = normalize(vec3(0.0f, n12 - n10, 2.0f));
    vec3 normal = cross(nV, nU).xyz;

    vec2 refractedUv = uv + normal.xz * 0.01f;

    vec4 bedPos = texture(Tex0, refractedUv);
    vec3 bedNormal = texture(Tex1, refractedUv).xyz;
    vec3 bedAlbedo = texture(Tex2, refractedUv).rgb;

    if (bedPos.w < homScreenPos.z) {
        bedPos = texture(Tex0, uv);
        bedNormal = texture(Tex1, uv).xyz;
        bedAlbedo = texture(Tex2, uv).rgb;
    }

    // Apply sunlight diffuse to underwater objects.
    float diff = max(dot(bedNormal, lightDir), 0.0f);
    vec3 bedCol = bedAlbedo.rgb * (ambientLightColor.rgb + diff * lightCol.rgb);

    float depth = length(FragPos - bedPos.xyz);

    float ior = 1.3333f;
    float R0 = pow((1.0f - ior) / (1.0f + ior), 2);

    vec3 viewDir = normalize(ViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float nDotH = max(dot(normal, halfwayDir), 0.0f);
    float R = mix(pow(1.0f - nDotH, 5.0f), 1.0f, R0);

    float spec = pow(nDotH, 80.0f);
    vec3 specCol = fresnelAmount * R * reflectCol
    + 0.5f * reflectivityAmount * spec * lightCol;

    float distScale = clamp(2.0f / depth, 0.0f, 1.0f);
    vec3 deepWaterCol = mix(deepCol, bedCol, distScale);
    //    vec3 waterCol = mix(deepWaterCol, shallowCol, nDotH);
    //    vec3 depthAdjustedCol = mix(waterCol, bedCol, distScale);

    //===------------------------------------------------------------------===//
    // Raymarching implementation modified from work by
    // Morgan McGuire and Michael Mara, used under the following license.
    //===------------------------------------------------------------------===//
    // Copyright (c) 2014, Morgan McGuire and Michael Mara
    // All rights reserved.
    //
    // From McGuire and Mara, Efficient GPU Screen-Space Ray Tracing,
    // Journal of Computer Graphics Techniques, 2014
    //
    // This software is open source under the "BSD 2-clause license":
    //
    //    Redistribution and use in source and binary forms, with or
    //    without modification, are permitted provided that the following
    //    conditions are met:
    //
    //    1. Redistributions of source code must retain the above
    //    copyright notice, this list of conditions and the following
    //    disclaimer.
    //
    //    2. Redistributions in binary form must reproduce the above
    //    copyright notice, this list of conditions and the following
    //    disclaimer in the documentation and/or other materials provided
    //    with the distribution.
    //
    //    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    //    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    //    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    //    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    //    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    //    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    //    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    //    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
    //    USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
    //    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    //    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    //    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
    //    THE POSSIBILITY OF SUCH DAMAGE.

    const float maxDistance = 100.0f;
    const float maxSteps = 60.0f;
    const float jitter = 0.5f;
    const float stride = 6.0f;
    const float zThickness = 10.0f;

    // @formatter:off
    vec4 startPosVS = view * vec4(FragPos, 1.0f);
    vec3 reflectDir = -viewDir - 2.0f * dot(normal, -viewDir) * normal;
    vec4 reflectDirVS = view * vec4(reflectDir, 0.0f);
    // Clip view space reflect ray to the near clip plane.
    // Note: Negative clip distances are needed, but nearClipDist and
    //       farClipDist are postive.
    float maxDistZ = startPosVS.z + maxDistance * reflectDirVS.z;
    float rayLength = maxDistZ > -nearClipDist
        ? -(nearClipDist + startPosVS.z) / reflectDirVS.z
        : maxDistance;
    vec4 endPosVS = startPosVS + rayLength * reflectDirVS;

    // Project reflected ray into clip space.
    // Note h0.w is the (positive) view space depth of startPosVS.
    vec4 h0 = proj * startPosVS;
    float k0 = 1.0f / h0.w;
    h0 *= k0;

    vec4 h1 = proj * endPosVS;
    float k1 = 1.0f / h1.w;
    h1 *= k1;

    // Transform clip space ray (NDC) into screen space.
    h0.xy = 0.5f * h0.xy + vec2(0.5f);
    h0.xy *= viewportSize.xy;

    h1.xy = 0.5f * h1.xy + vec2(0.5f);
    h1.xy *= viewportSize.xy;

    vec2 p0 = h0.xy;
    vec2 p1 = h1.xy;

    // View space endpoints that transform linearly.
    vec3 q0 = k0 * startPosVS.xyz;
    vec3 q1 = k1 * endPosVS.xyz;

    // Screen bounds in pixels. Pixel centers are at half coordinates.
    vec2 pxMin = vec2(0.5f);
    vec2 pxMax = viewportSize.xy - pxMin;
    float pxAlpha = 0.0f;

    // Clip p1, k1, q1 to the viewport. The ternary operators select which of
    // the arguments to the || is true, cutting a branch each.
    // e.g. if p1.y > pxMax.y then pxAlpha = (p1.y - pxMax.y) / (p1.y - p0.y)
    // so p1 = (1 - pxAlpha) * p1 + pxAlpha * p0 = pxMax.y.
    if (p1.y > pxMax.y || p1.y < pxMin.y) {
        pxAlpha = (p1.y - (p1.y > pxMax.y ? pxMax.y : pxMin.y)) / (p1.y - p0.y);
    }
    if (p1.x > pxMax.x || p1.x < pxMin.x) {
        pxAlpha = max(pxAlpha, (p1.x - (p1.x > pxMax.x ? pxMax.x : pxMin.x)) / (p1.x - p0.x));
    }

    p1 = mix(p1, p0, pxAlpha);
    k1 = mix(k1, k0, pxAlpha);
    q1 = mix(q1, q0, pxAlpha);

    // Ensure line is nondegenerate so we don't get division by zero.
    p1 += vec2((dot(p1 - p0, p1 - p0) < 0.0001f) ? 0.01f : 0.0f);
    vec2 delta = p1 - p0;

    // Permute coordinates to remove quadrant checking in later DDA.
    // If the line is more vertical than horizontal (between top and bottom
    // wedge of 'X') then swap x and y so that only the horizontal case has to
    // be dealt with.
    bool permute = false;
    if (abs(delta.x) < abs(delta.y)) {
        permute = true;
        delta = delta.yx;
        p0 = p0.yx;
        p1 = p1.yx;
    }
    float stepDir = sign(delta.x);
    //    invDx = abs(1.0f / delta.x);
    float invDx = stepDir / delta.x;

    // Differentiate wrt p. These are constant because q and k interpolate
    // linearly, and thus we don't have to worry about delta being tiny.
    vec3 dq = (q1 - q0) * invDx;
    float dk = (k1 - k0) * invDx;
    //   dp = (p1 - p0) * invDx;
    //      = stepDir * (p1 - p0) / (p1 - p0).x
    //      = vec2(stepDir, stepDir * delta.y / delta.x);
    vec2 dp = vec2(stepDir, delta.y * invDx);

    // Scale derivatives by given pixel stride and add jitter.
    dq *= stride; dk *= stride; dp *= stride;
    q0 += jitter * dq; k0 += jitter * dk; p0 += jitter * dp;

    vec2 p = p0;
    vec3 q = q0;
    float k = k0;
    float stepCount = 0.0f;
    float end = p1.x * stepDir;

    float prevZMaxEstimate = startPosVS.z;
    float rayZMin = prevZMaxEstimate;
    float rayZMax = prevZMaxEstimate;
    float sceneZMax = rayZMax + 100.0f;

    // Pixel coordinates of ray.
    vec2 hitPixel = vec2(-1.0f, -1.0f);

    // TODO: Compute this on the cpu and pass it as a uniform.
    float clipDistRatio = farClipDist / nearClipDist;
    float clipDistRatio_1 = clipDistRatio - 1.0f;

    // Perform the actual ray marching, checking the depth value on pixel
    // midpoints and exiting when a collision is found.
    for (; (p.x * stepDir) <= end && stepCount < maxSteps && sceneZMax != 0
        && (rayZMax < sceneZMax - zThickness || rayZMin > sceneZMax);
        p += dp, q.z += dq.z, k += dk, ++stepCount) {

        rayZMin = prevZMaxEstimate;
        rayZMax = (q.z + dq.z * 0.5f) / (k + dk * 0.5f);
        prevZMaxEstimate = rayZMax;
        if (rayZMin > rayZMax) {
            // swap(rayZMin, rayZMax)
            float t = rayZMin;
            rayZMin = rayZMax;
            rayZMax = t;
        }

        hitPixel = permute ? p.yx : p;
        // Get view space depth at current ray point.
        sceneZMax = texelFetch(Tex0, ivec2(hitPixel), 0).w;

        // Convert [0, 1] depth buffer into camera space depth.
        //   ZMax = farClipDist * nearClipDist / ((farClipDist - nearClipDist) * sceneZMax - farClipDist);
        sceneZMax = farClipDist / (clipDistRatio_1 * sceneZMax - clipDistRatio);
    }

    // Find homogeneous coordinates of hit, convert to view space, project
    // into clip space, then finally convert to texture coordinates.
    q.xy += stepCount * dq.xy;
    vec4 hitPoint = proj * vec4(q / k, 1.0f);
    hitPoint /= hitPoint.w;
    vec2 reflectedUv = hitPoint.xy * 0.5f + 0.5f;

    vec3 reflectedCol = vec3(0.0f);
    float reflectedAlpha = 0.0f;
    if (rayZMax >= sceneZMax - zThickness && rayZMin <= sceneZMax) {
        reflectedAlpha = 1.0f - stepCount / maxSteps;
        reflectedCol = texture(Tex2, reflectedUv).rgb;
    }

    // @formatter:on

    //===------------------------------------------------------------------===//
    // End of raymarching implementation
    //===------------------------------------------------------------------===//

    // Apply sunlight diffuse to reflected objects.
    vec3 reflectedNormal = texture(Tex1, reflectedUv).xyz;
    diff = max(dot(reflectedNormal, lightDir), 0.0f);
    reflectedCol = reflectedCol.rgb * (ambientLightColor.rgb + diff * lightCol.rgb);

    // Fade out reflection before it disappears completely.
    reflectedCol = mix(vec3(0.0f), reflectedCol, reflectedAlpha);

    vec3 finalCol = deepWaterCol + specCol + reflectivityAmount * R * reflectedCol;
    FragColor = vec4(finalCol, 1.0f);
}
