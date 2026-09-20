#version 460 core

uniform vec2 resolution;

uniform vec3 cameraPosition;
uniform vec3 cameraFront;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

uniform float focalLength;

out vec4 FragColor;

float sphereSDF(vec3 p, vec3 center, float radius)
{
    return length(p - center) - radius;
}

float sceneSDF(vec3 p)
{
    return sphereSDF(
        p,
        vec3(0.0, 0.0, 0.0),
        1.0
    );
}

vec3 getNormal(vec3 p)
{
    const float e = 0.001;

    vec2 h = vec2(e, 0.0);

    return normalize(vec3(
        sceneSDF(p + h.xyy) - sceneSDF(p - h.xyy),
        sceneSDF(p + h.yxy) - sceneSDF(p - h.yxy),
        sceneSDF(p + h.yyx) - sceneSDF(p - h.yyx)
    ));
}

float raymarch(vec3 ro, vec3 rd)
{
    float t = 0.0;

    const int maxSteps = 128;
    const float maxDistance = 100.0;
    const float hitDistance = 0.001;

    for (int i = 0; i < maxSteps; i++)
    {
        vec3 p = ro + rd * t;

        float d = sceneSDF(p);

        if (d < hitDistance)
            return t;

        t += d;

        if (t > maxDistance)
            break;
    }

    return -1.0;
}

void main()
{
    vec2 uv =
        (2.0 * gl_FragCoord.xy - resolution)
        / resolution.y;

    vec3 ro = cameraPosition;

    vec3 rd = normalize(
        cameraFront * focalLength +
        cameraRight * uv.x +
        cameraUp * uv.y
    );

    float t = raymarch(ro, rd);

    if (t < 0.0)
    {
        FragColor = vec4(
            0.015,
            0.02,
            0.03,
            1.0
        );

        return;
    }

    vec3 p = ro + rd * t;

    vec3 normal = getNormal(p);

    vec3 lightDirection = normalize(
        vec3(
            1.0,
            1.5,
            2.0
        )
    );

    float diffuse = max(
        dot(
            normal,
            lightDirection
        ),
        0.0
    );

    float ambient = 0.15;

    vec3 sphereColor =
        vec3(
            0.3,
            0.55,
            1.0
        );

    vec3 color =
        sphereColor *
        (
            ambient +
            diffuse
        );

    FragColor = vec4(
        color,
        1.0
    );
}