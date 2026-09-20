#version 460 core

uniform vec2 resolution;

uniform vec3 cameraPosition;
uniform vec3 cameraFront;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

uniform float focalLength;

out vec4 FragColor;

const float NORMAL_EPSILON = 0.0005;
const float MARCH_EPSILON = 0.001;
const float MAX_MARCH_DIST = 100.0;
const int MAX_NUM_MARCHES = 256;

struct SceneSample
{
    float dist;
    float matID;
};

vec2 rotate(vec2 p, float t)
{
    float c = cos(t);
    float s = sin(t);

    return vec2(
        p.x * c - p.y * s,
        p.x * s + p.y * c
    );
}

float chash11(float p)
{
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;

    return fract(p);
}

float scubic(float t)
{
    return 3.0 * t * t -
           2.0 * t * t * t;
}

float tmod(float a, float b)
{
    return mod(
        a + b / 2.0,
        b
    ) - b / 2.0;
}

vec2 tmod(vec2 a, vec2 b)
{
    return mod(
        a + b / 2.0,
        b
    ) - b / 2.0;
}

vec3 tmod(vec3 a, vec3 b)
{
    return mod(
        a + b / 2.0,
        b
    ) - b / 2.0;
}

float noise_value_plane(
    int seed,
    vec2 p
)
{
    float xt =
        fract(p.x);

    float i =
        p.x - xt;

    float yt =
        fract(p.y);

    float j =
        p.y - yt;

    float a =
        chash11(
            849.0 * float(seed) +
            384.0 * i +
            1911.0 * j
        );

    i += 1.0;

    float b =
        chash11(
            849.0 * float(seed) +
            384.0 * i +
            1911.0 * j
        );

    j += 1.0;

    float d =
        chash11(
            849.0 * float(seed) +
            384.0 * i +
            1911.0 * j
        );

    i -= 1.0;

    float c =
        chash11(
            849.0 * float(seed) +
            384.0 * i +
            1911.0 * j
        );

    float sx =
        scubic(xt);

    float sy =
        scubic(yt);

    float value = a;

    value +=
        (b - a) *
        sx;

    value +=
        (c - a) *
        sy;

    value +=
        (d - b - c + a) *
        sx *
        sy;

    return value;
}

float graphTerrain(
    vec2 xz,
    int quality
)
{
    float ty = 0.0;

    float amplitude =
        1.0;

    for (
        int i = 0;
        i < quality;
        ++i
    )
    {
        ty +=
            amplitude *
            noise_value_plane(
                0,
                xz
            );

        float theta =
            chash11(
                float(i)
            ) *
            acos(0.0);

        xz =
            rotate(
                xz,
                theta
            );

        amplitude /=
            2.3;

        xz *=
            2.0;
    }

    return ty;
}

float terrainSDFGraph(
    vec3 p,
    float frequency,
    float height,
    int quality
)
{
    float safeFrequency =
        max(
            abs(frequency),
            0.0001
        );

    float h =
        graphTerrain(
            p.xz *
                safeFrequency,
            quality
        ) *
        height;

    // Conservative correction because this is a height field rather than
    // a mathematically exact Euclidean SDF.
    float maxSlope =
        10.0 *
        abs(
            safeFrequency *
            height
        );

    float bound =
        1.0 /
        sqrt(
            1.0 +
            maxSlope *
            maxSlope
        );

    return
        (p.y - h) *
        bound;
}

float sdSphere(
    vec3 p,
    float radius
)
{
    return
        length(p) -
        radius;
}

float sdBox(
    vec3 p,
    vec3 halfSize
)
{
    vec3 q =
        abs(p) -
        halfSize;

    return
        length(
            max(
                q,
                vec3(0.0)
            )
        ) +
        min(
            max(
                q.x,
                max(
                    q.y,
                    q.z
                )
            ),
            0.0
        );
}

// This marker is replaced by Application.cpp every time the graph changes.
SceneSample sceneSDF(vec3 p)
{
    float n0_distance = terrainSDFGraph(p, 0.430000, 0.640000, int(clamp(10.170000, 1.0, 16.0)));
    float n0_material = 4.000000;
    return SceneSample(n0_distance, n0_material);
}


vec3 sceneNormal(
    vec3 p
)
{
    vec2 e =
        vec2(
            NORMAL_EPSILON,
            0.0
        );

    float dx =
        sceneSDF(
            p + e.xyy
        ).dist -
        sceneSDF(
            p - e.xyy
        ).dist;

    float dy =
        sceneSDF(
            p + e.yxy
        ).dist -
        sceneSDF(
            p - e.yxy
        ).dist;

    float dz =
        sceneSDF(
            p + e.yyx
        ).dist -
        sceneSDF(
            p - e.yyx
        ).dist;

    return normalize(
        vec3(
            dx,
            dy,
            dz
        )
    );
}

bool raymarch(
    vec3 ro,
    vec3 rd,
    out float travel,
    out float material
)
{
    travel =
        0.0;

    material =
        0.0;

    for (
        int i = 0;
        i < MAX_NUM_MARCHES;
        ++i
    )
    {
        vec3 p =
            ro +
            rd *
            travel;

        SceneSample sceneSample =
            sceneSDF(p);

        if (
            sceneSample.dist <
            MARCH_EPSILON
        )
        {
            material =
                sceneSample.matID;

            return true;
        }

        travel +=
            max(
                sceneSample.dist,
                MARCH_EPSILON *
                0.25
            );

        if (
            travel >
            MAX_MARCH_DIST
        )
        {
            break;
        }
    }

    return false;
}

vec3 materialColor(
    float material
)
{
    int id =
        int(
            floor(
                material +
                0.5
            )
        );

    if (id == 0)
    {
        return vec3(
            0.38,
            0.22,
            0.10
        );
    }

    if (id == 1)
    {
        return vec3(
            0.08,
            0.55,
            0.16
        );
    }

    if (id == 2)
    {
        return vec3(
            0.25,
            0.48,
            1.0
        );
    }

    float h =
        chash11(
            float(id) *
            17.123
        );

    return
        0.35 +
        0.65 *
        vec3(
            chash11(h + 1.0),
            chash11(h + 2.0),
            chash11(h + 3.0)
        );
}

vec3 skyColor(
    vec3 rd
)
{
    float t =
        clamp(
            rd.y * 0.5 +
            0.5,
            0.0,
            1.0
        );

    return mix(
        vec3(
            0.78,
            0.86,
            1.0
        ),
        vec3(
            0.08,
            0.24,
            0.65
        ),
        t
    );
}

void main()
{
    vec2 uv =
        (
            2.0 *
            gl_FragCoord.xy -
            resolution
        ) /
        resolution.y;

    vec3 ro =
        cameraPosition;

    vec3 rd =
        normalize(
            cameraFront *
                focalLength +
            cameraRight *
                uv.x +
            cameraUp *
                uv.y
        );

    float travel = 0.0;
    float material = 0.0;

    bool hit =
        raymarch(
            ro,
            rd,
            travel,
            material
        );

    if (!hit)
    {
        FragColor =
            vec4(
                skyColor(rd),
                1.0
            );

        return;
    }

    vec3 p =
        ro +
        rd *
        travel;

    vec3 n =
        sceneNormal(p);

    vec3 lightDir =
        normalize(
            vec3(
                -0.7,
                1.0,
                0.6
            )
        );

    float diffuse =
        max(
            dot(
                n,
                lightDir
            ),
            0.0
        );

    float ambient =
        0.18;

    vec3 color =
        materialColor(
            material
        ) *
        (
            ambient +
            0.82 *
            diffuse
        );

    float fog =
        1.0 -
        exp(
            -0.035 *
            travel
        );

    color =
        mix(
            color,
            skyColor(rd),
            fog
        );

    FragColor =
        vec4(
            color,
            1.0
        );
}
