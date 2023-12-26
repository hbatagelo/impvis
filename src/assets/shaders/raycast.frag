#version 300 es

precision highp float;

in vec2 fragPosition;

out vec4 outColor;

////////////////////////////////////////////////////////////////////////////////
// Constants and definitions
////////////////////////////////////////////////////////////////////////////////

@DEFINITIONS@

const float kBoundRadius = float(@BOUND_RADIUS@);
const float kBoundRadiusSquared = kBoundRadius * kBoundRadius;

////////////////////////////////////////////////////////////////////////////////
// Uniform data
////////////////////////////////////////////////////////////////////////////////

// Camera settings
struct Camera
{
                        // base  offset
  vec3 eye;             //   4N      0N
  float focalLength;    //   1N      4N
  vec2 scale;           //   2N      8N
  float lookAtDistance; //   1N     12N
};
layout (std140) uniform CameraBlock {
  Camera uCamera;
};

// Shading properties
struct Shading
{
                     // base  offset
  vec4 KaIa;         //   4N      0N
  vec4 KdId;         //   4N      4N
  vec4 KsIs;         //   4N      8N
  vec3 lightVector;  //   4N     12N
  float shininess;   //   1N     16N
  float gaussianEps; //   1N     20N
};
layout (std140) uniform ShadingBlock {
  Shading uShading;
};

// Transformation matrices
struct Transform {
                      // base  offset
  mat4 MV_I;          //   4N      0N
                      //   4N      4N
                      //   4N      8N
                      //   4N     12N
};
layout (std140) uniform TransformBlock {
  Transform uTransform;
};

// Equation parameters
struct Params {
                      // base  offset
  vec4 data[4];       //   4N      0N
                      //   4N      4N
                      //   4N      8N
                      //   4N     12N
};
layout (std140) uniform ParamsBlock {
  Params uParams;
};

uniform float uIsoValue;

////////////////////////////////////////////////////////////////////////////////
// Data structure definitions
////////////////////////////////////////////////////////////////////////////////

struct Ray
{
  vec3 origin;
  vec3 direction;
};

////////////////////////////////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////////////////////////////////

float mpow2 (in float b) { return b * b; }
float mpow3 (in float b) { return b * b * b; }
float mpow4 (in float b) {
  float b2 = b * b;
  return b2 * b2;
}
float mpow5 (in float b) {
  float b2 = b * b;
  return b2 * b2 * b;
}
float mpow6 (in float b) {
  float b3 = b * b * b;
  return b3 * b3;
}
float mpow7 (in float b) {
  float b2 = b * b;
  return b2 * b2 * b2 * b;
}
float mpow8 (in float b) {
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4;
}
float mpow9 (in float b) {
  float b3 = b * b * b;
  return b3 * b3 * b3;
}
float mpow10(in float b) {
  float b2 = b * b;
  float b5 = b2 * b2 * b;
  return b5 * b5;
}
float mpow11(in float b) {
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4 * b2 * b;
}
float mpow12(in float b) {
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4 * b4;
}
float mpow13(in float b) {
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4 * b4 * b;
}
float mpow14(in float b) {
  float b2 = b * b;
  float b7 = b2 * b2 * b2 * b;
  return b7 * b7;
}
float mpow15(in float b) {
  float b2 = b * b;
  float b5 = b2 * b2 * b;
  return b5 * b5 * b5;
}
float mpow16(in float b) {
  float b2 = b * b;
  float b4 = b2 * b2;
  float b8 = b4 * b4;
  return b8 * b8;
}

float mpow(in float b, in float n)
{
  if (n == 0.0) return 1.0;
  if (b == 0.0) return 0.0;

  if (n < 0.0) {
    b = 1.0 / b;
    n = -n;
  }

  if (fract(n) == 0.0) {
    // Iterative squaring
    int ni = int(n);
    float y = 1.0;
    while (ni > 1) {
      if (ni - (ni / 2) * 2 == 0) {
        b = b * b;
        ni /= 2;
      } else {
        y = b * y;
        b *= b;
        ni = (ni - 1) / 2;
      }
    }
    return b * y;
  }

  if (b < 0.0) return 0.0;

  return pow(b, n);
}

// Injected global code
@CODE_GLOBAL@

float evalFunction(in vec3 p)
{
  vec3 p2  = p   * p;
  vec3 p3  = p2  * p;
  vec3 p4  = p2  * p2;
  vec3 p5  = p2  * p3;
  vec3 p6  = p3  * p3;
  vec3 p7  = p3  * p4;
  vec3 p8  = p4  * p4;
  vec3 p9  = p4  * p5;
  vec3 p10 = p5  * p5;
  vec3 p11 = p5  * p6;
  vec3 p12 = p6  * p6;
  vec3 p13 = p12 * p;
  vec3 p14 = p7  * p7;
  vec3 p15 = p10 * p5;
  vec3 p16 = p8  * p8;

  // Injected local code
  @CODE_LOCAL@

  // Injected left-hand side of the expression
  float lhs = @EQUATION@;

  return lhs - uIsoValue;
}

/*
 * Evaluates gradient at p using central difference approximation.
 */
vec3 evalGradient(in vec3 p)
{
  const float eps = 1e-4;
  const float inv2eps = 0.5/eps;

  const vec3 dx = vec3(1.0, 0.0, 0.0) * eps;
  const vec3 dy = vec3(0.0, 1.0, 0.0) * eps;
  const vec3 dz = vec3(0.0, 0.0, 1.0) * eps;

  float nx = evalFunction(p + dx) - evalFunction(p - dx);
  float ny = evalFunction(p + dy) - evalFunction(p - dy);
  float nz = evalFunction(p + dz) - evalFunction(p - dz);

  return vec3(nx, ny, nz) * inv2eps;
}

/*
 * Intersects ray with an axis-aligned bounding box using the slab test.
 */
bool intersectAABB(in  Ray   ray     /* ray origin and direction          */,
                   out float tStart  /* ray parameter at 1st intersection */,
                   out float tEnd    /* ray parameter at 2nd intersection */)
{
  const vec3 minimum = vec3(-kBoundRadius);
  const vec3 maximum = vec3( kBoundRadius);
  vec3 t1 = (minimum - ray.origin) / ray.direction;
  vec3 t2 = (maximum - ray.origin) / ray.direction;
  vec3 tMax = max(t1, t2);
  vec3 tMin = min(t1, t2);

  tEnd = min(tMax.x, min(tMax.y, tMax.z));
  tStart = max(max(tMin.x, 0.0), max(tMin.y, tMin.z));

  return tEnd > tStart;
}

/*
 * Intersects ray with sphere centered at origin.
 * For simplicity, assume the sphere is always in front of the ray.
 */
bool intersectSphere(in  Ray   ray    /* ray origin and direction          */,
                     out float tStart /* ray parameter at 1st intersection */,
                     out float tEnd   /* ray parameter at 2nd intersection */)
{
  vec3 proj = ray.origin + ray.direction * dot(-ray.origin, ray.direction);
  float len2 = dot(proj, proj);
  if (len2 <= kBoundRadiusSquared)
  {
    float lenHalfChord = sqrt(kBoundRadiusSquared - len2);
    float distOriginProj = distance(ray.origin, proj);
    tStart = max(0.0, distOriginProj - lenHalfChord);
    tEnd = distOriginProj + lenHalfChord;
    return true;
  }
  return false;
}

/*
 * Intersects axis-aligned bounding box centered at origin with ray starting
 * from inside the box.
 *
 * Returns ray parameter at point of exit.
 */
float intersectAABBFromInside(in Ray ray)
{
  const vec3 minimum = vec3(-kBoundRadius);
  const vec3 maximum = vec3( kBoundRadius);
  vec3 t1 = (minimum - ray.origin) / ray.direction;
  vec3 t2 = (maximum - ray.origin) / ray.direction;
  vec3 tMax = max(t1, t2);
  return min(tMax.x, min(tMax.y, tMax.z));
}

/*
 * Intersects sphere centered at origin with ray starting from inside the
 * sphere.
 *
 * Returns ray parameter at point of exit.
 */
float intersectSphereFromInside(in Ray ray)
{
    float A = dot(ray.direction, ray.direction);
    float B = dot(ray.direction, ray.origin);
    float C = dot(ray.origin, ray.origin) - kBoundRadiusSquared;
    float D = B * B - A * C;
    return (-B + sqrt(D)) / A;
}

/*
 * Uses simple sign test to check whether there is a root in a ray parameter
 * interval.
 */
bool signTest(in float tAValue /* value function at start of interval */,
              in float tBValue /* value function at end of interval   */)
{
  return tAValue * tBValue <= 0.0;
}

/*
 * Uses Taylor test to check whether there is a root in the given interval, as
 * described in Jag Mohan Singh's Masters dissertation "Real Time Rendering of
 * Implicit Surfaces on the GPU", 2008.
 */
bool taylorTest(in Ray   ray     /* ray origin and direction            */,
                in float tA      /* ray parameter at start of interval  */,
                in float tB      /* ray parameter at end of interval    */,
                in float tAValue /* value function at start of interval */,
                in float tBValue /* value function at end of interval   */)
{
  float halfInterval = (tB - tA) * 0.5;
  vec3 PA = ray.origin + ray.direction * tA;
  vec3 PB = ray.origin + ray.direction * tB;
  float fdA = dot(evalGradient(PA), ray.direction);
  float fdB = dot(evalGradient(PB), ray.direction);

  float p = tAValue;
  float q = tAValue + fdA * halfInterval;
  float r = tBValue - fdB * halfInterval;
  float s = tBValue;

  return min(min(p, q), min(r, s)) * max(max(p, q), max(r, s)) <= 0.0;
}

/*
 * Intersects ray with implicit surface using ray marching.
 *
 * This is a simple ray marching algorithm that divides the ray interval into
 * fixed-size segments and tests each segment using a root test.
 */
bool fixedMarch(in  Ray   ray    /* ray origin and direction             */,
                in  float tStart /* ray parameter at ray tStart          */,
                in  float tEnd   /* ray parameter at ray tEnd            */,
                out float tHit   /* ray parameter of surface tHit        */,
                out bool  inside /* true if surface was tHit from inside */)
{
  float stepSize = (tEnd - tStart) / float(RAY_MARCH_STEPS);

  float t = tStart;
  float leftValue = evalFunction(ray.origin + ray.direction * t);

  for (int i = 0; i < RAY_MARCH_STEPS; ++i)
  {
    t += stepSize;
    float rightValue = evalFunction(ray.origin + ray.direction * t);

#if defined(USE_SIGN_TEST)
    if (signTest(leftValue, rightValue))
#else
    if (taylorTest(ray, t - stepSize, t, leftValue, rightValue))
#endif
    {
      tHit = t + (rightValue * stepSize) / (leftValue - rightValue);
      inside = leftValue < 0.0 ? true : false;
      return true;
    }

    leftValue = rightValue;
  }

  return false;
}

/*
 * Constants used by adaptiveMarch and adaptiveMarchShadow.
 */
const float maxStepScale = 1.5;
const float minStepScale = 0.25;
const float tau = 0.05;
const float piOver180 = 3.141592653 / 180.0;
const float epsAngle = cos(85.0 * piOver180);
const int maxSteps = RAY_MARCH_STEPS * int(1.0 / (minStepScale * minStepScale));

/*
 * Intersects ray with implicit surface using adaptive ray marching.
 *
 * The step size is changed according to the function's value
 * $S: \mathbb{R}^3 \to \mathbb{R}$ and gradient $\nabla S$ evaluated at the
 * current step.
 *
 * The ray marching starts with a base step size that is multiplied by $|S|$
 * clamped to $[s_{min}, s_{max}]$, where $s_{min}$ and $s_{max}$ are minimum
 * and maximum step scaling factors (we use $s_{min}=0.25$ and $s_{max}=1.5$).
 *
 * If $|S| < \tau$, where $\tau$ is a small distance from the level set (we use
 * $\tau=0.05$), and furthermore the ray is close to the silhouette of the
 * isosurface, the step size is multiplied by $\max \{ s_{min}, |S|/\tau \}$,
 * i.e. the step size decreases as the ray approaches the surface near a
 * silhouette. This silhouette condition is tested by checking if the ray is at
 * a small grazing angle with respect to the local gradient:
 * $|\nabla S\cdot \hat{r}_{\rm dir}| < \cos{\left(\pi/2-\theta\right)}$, where
 * $\hat{r}_{\rm dir}$ is the ray direction and $\theta$ is the threshold of the
 * grazing angle. We use $\theta=\pi/36=5^{\circ}$.
 *
 * Since the rays exit the bounding geometry at different step sizes, artifacts
 * may be visible at these exit points. To prevent that, we decrease the step
 * size as the ray approaches the exit. We measure the distance to the exit as
 * $l=t_{end}-t_{cur}$, where $t_{end}$ is the ray parameter at the exit point
 * and $t_{cur}\leq t_{end}$ is the current ray parameter. If $l$ is smaller
 * than an epsilon, the step size is multiplied by $\max\{s_{min},l/\tau\}$.
 * For the epsilon, we use the base step size multiplied by $s_{max}$.
 */
bool adaptiveMarch(in  Ray   ray    /* ray origin and direction             */,
                   in  float tStart /* ray parameter at ray tStart          */,
                   in  float tEnd   /* ray parameter at ray tEnd            */,
                   out float tHit   /* time of ray surface tHit             */,
                   out bool  inside /* true if surface was tHit from inside */)
{
  float baseStepSize = (tEnd - tStart) / float(RAY_MARCH_STEPS);
  float epsExit = baseStepSize * maxStepScale;

  float tCurrent = tStart;
  vec3 P = ray.origin + ray.direction * tCurrent;
  float leftValue = evalFunction(P);

  for (int i = 0; i < maxSteps; ++i)
  {
    float leftValueAbs = abs(leftValue);

    // Step size is proportional to the function value
    float stepSize = baseStepSize * clamp(leftValueAbs, minStepScale, maxStepScale);

    // Decrease the step size when the ray is near the surface (<= tau) at a
    // small grazing angle
    if ((leftValueAbs < tau &&
        abs(dot(evalGradient(P), ray.direction)) < epsAngle))
    {
      stepSize *= max(leftValueAbs / tau, minStepScale);
    }
    else
    {
      // Decrease the step size if the ray is exiting the bounding geometry
      float distFromExit = tEnd - tCurrent;
      if (distFromExit < epsExit)
      {
        stepSize *= max(distFromExit / epsExit, minStepScale);
      }
    }

    tCurrent += stepSize;
    P += ray.direction * stepSize;
    float rightValue = evalFunction(P);

#if defined(USE_SIGN_TEST)
    if (signTest(leftValue, rightValue))
#else
    if (taylorTest(ray, tCurrent - stepSize, tCurrent, leftValue, rightValue))
#endif
    {
      float diffValue = (leftValue - rightValue);
#if !defined(USE_SIGN_TEST)
      // Assume the surface was hit from the left
      if (abs(diffValue) < 1e-5)
         tHit = tCurrent - stepSize;
      else
#endif
        tHit = tCurrent + (rightValue * stepSize) / diffValue;
      inside = leftValue < 0.0 ? true : false;
      return true;
    }

    // Early exit
    if (tCurrent >= tEnd)
    {
      break;
    }

    leftValue = rightValue;
  }

  inside = false;
  return false;
}

/*
 * Returns the value of a Gaussian radial basis function for a given distance
 * r. The is used by colorFromScalar to map scalar values from r \in [0,inf) to
 * a range that starts with 1 at r=0 and approaches 0 as |r| -> inf.
 */
float gaussianRBF(in float r)
{
  float t = r * uShading.gaussianEps;
  return exp(-(t*t));
}

/*
 * Computes the color for a given scalar field value using a hard-coded
 * transfer function.
 */
vec4 colorFromScalar(in float x) {
  const vec4 c0 = vec4(0.5, 0.5, 1.0, 0.5);  // Light blue, 50% alpha
  const vec4 c1 = vec4(1.0, 0.0, 1.0, 1.0);  // Magenta
  const vec4 c2 = vec4(1.0, 1.0, 1.0, 1.0);  // White
  const vec4 c3 = vec4(1.0, 0.0, 0.0, 1.0);  // Red
  const vec4 c4 = vec4(1.0, 0.25, 0.0, 0.5); // Orange, 50% alpha

  const float stepMax = 4.0;
  const float s1 = 0.0 / stepMax;
  const float s2 = 1.0 / stepMax;
  const float s3 = 2.0 / stepMax;
  const float s4 = 3.0 / stepMax;
  const float s5 = 4.0 / stepMax;

  float t = x >= 0.0 ? 0.5 + (1.0 - gaussianRBF(x)) / 2.0 :
                       gaussianRBF(-x) / 2.0;

  vec4 color;
  color = mix(   c0, c1, smoothstep(s1, s2, t));
  color = mix(color, c2, smoothstep(s2, s3, t));
  color = mix(color, c3, smoothstep(s3, s4, t));
  color = mix(color, c4, smoothstep(s4, s5, t));

  return color;
}

/*
 * Computes the color-valued density of the scalar field.
 */
vec4 marchScalarField(in Ray   ray    /* ray origin and direction    */,
                      in float tStart /* ray parameter at ray tStart */,
                      in float tEnd   /* ray parameter at ray tEnd   */)
{
  const int steps = 250;
  float stepSize = (tEnd - tStart) / float(steps);
  float alphaStep = stepSize / kBoundRadius;

  float t = tStart;
  vec3 P = ray.origin + ray.direction * tStart;
  vec3 stepP = ray.direction * stepSize;

  vec4 accumColor = vec4(0);
  for (int n = 0; n < steps; ++n)
  {
    vec4 s2c = colorFromScalar(evalFunction(P));
    accumColor += vec4(s2c.rgb, s2c.a * alphaStep);
    if (accumColor.a >= 1.0)
    {
      return vec4(accumColor.rgb / float(n), accumColor.a);
    }
    P += stepP;
  }
  return vec4(accumColor.rgb / float(steps), accumColor.a);
}

/*
 * Same as adaptiveMarch, but simplified for shadow rays.
 */
bool adaptiveMarchShadowRay(in  Ray   ray  /* ray origin and direction */,
                            in  float tEnd /* ray parameter at exit    */)
{
  float baseStepSize = tEnd / float(RAY_MARCH_STEPS);
  float epsExit = baseStepSize * maxStepScale;

  float tCurrent = 0.0;
  vec3 P = ray.origin;
  float leftValue = evalFunction(P);

  for (int i = 0; i < maxSteps; ++i)
  {
    float leftValueAbs = abs(leftValue);

    // Step size is proportional to the function value
    float stepSize = baseStepSize * clamp(leftValueAbs, minStepScale, maxStepScale);

    // Halve the step size when the ray is near the surface (<= tau) at a
    // small grazing angle
    if (leftValueAbs <= tau &&
        abs(dot(evalGradient(P), ray.direction)) < epsAngle)
    {
      stepSize *= max(leftValueAbs / tau, minStepScale);
    }
    else
    {
      float distFromExit = tEnd - tCurrent;
      if (distFromExit < epsExit)
      {
        stepSize *= max(distFromExit / epsExit, minStepScale);
      }
    }

    tCurrent += stepSize;
    P += ray.direction * stepSize;
    float rightValue = evalFunction(P);

    if (signTest(leftValue, rightValue))
    {
      return true;
    }

    // Early exit
    if (tCurrent >= tEnd)
    {
      break;
    }

    leftValue = rightValue;
  }

  return false;
}

/*
 * Check whether the point P is in shadow with respect to light direction L.
 */
bool inShadow(in vec3 P /* intersection point with surface */,
              in vec3 L /* normalized direction to light source */)
{
  const float bias = 2.0 * kBoundRadius / 1e3;
  Ray ray = Ray(P + L * bias, L);

#if defined(USE_BOUNDING_BOX)
  float tEnd = intersectAABBFromInside(ray);
#else
  float tEnd = intersectSphereFromInside(ray);
#endif
  return adaptiveMarchShadowRay(ray, tEnd);
}

/*
 * Determines the color of the shaded surface at P with normal N.
 */
vec4 shade(in vec3 P      /* intersection point with surface */,
           in vec3 N      /* normal to the surface at P */,
           in bool inside /* whether the ray hit from inside */) {
  vec4 ambientColor = uShading.KaIa;

#if defined(USE_SHADOWS)
  if (inShadow(P, uShading.lightVector)) {
    return ambientColor;
  }
#endif

  vec3 origN = N;
  if (inside) {
    N = -N;
  }

#if defined(USE_BLINN_PHONG)
  // Blinn-Phong
  vec3 L = uShading.lightVector;
  float lambertian = max(dot(N, L), 0.0);
  float specular = 0.0;
  if (lambertian > 0.0) {
    vec3 V = normalize(uCamera.eye - P);
    vec3 H = normalize(L + V);
    float angle = max(dot(H, N), 0.0);
    specular = pow(angle, uShading.shininess);
  }
#else
  // Unlit
  float lambertian = 1.0;
  float specular = 0.0;
#endif

#if defined(USE_NORMALS_AS_COLORS)
  vec4 diffuseColor = vec4((origN + 1.0) * 0.5 * lambertian, 1.0);
#else
  const vec4 insideColor = vec4(0.9, 0.1, 0.9, 1.0);
  vec4 diffuseColor = (inside ? insideColor : uShading.KdId) * lambertian;
#endif
  vec4 specularColor = uShading.KsIs * specular;

  return ambientColor + diffuseColor + specularColor;
}

/*
 * Generates the primary ray in world-space coordinates.
 */
Ray generatePrimaryRay()
{
  vec2 coords = fragPosition * uCamera.scale;
  vec3 direction = mat3(uTransform.MV_I) * vec3(coords, -uCamera.focalLength);
  return Ray(uCamera.eye, normalize(direction));
}

/*
 * Checks if the ray intersects the bounding box/sphere and triggers
 * the ray marching method of choice.
 */
vec4 rayMarch(in Ray ray)
{
  float tStart, tEnd;
#if defined(USE_BOUNDING_BOX)
  if (intersectAABB(ray, tStart, tEnd))
#else
  if (intersectSphere(ray, tStart, tEnd))
#endif
  {
    float tHit;
    bool inside;
#if defined(SHOW_ISOSURFACE)

#if defined(USE_ADAPTIVE_RAY_MARCH)
    if (adaptiveMarch(ray, tStart, tEnd, tHit, inside)) {
#else
    if (fixedMarch(ray, tStart, tEnd, tHit, inside)) {
#endif
      vec3 P = ray.origin + ray.direction * tHit;
      vec3 N = normalize(evalGradient(P));
      vec4 color = shade(P, N, inside);
#if defined(USE_FOG)
      color.a = clamp(1.0 -
        (tHit - uCamera.lookAtDistance) / (kBoundRadius * 2.0), 0.0, 1.0);
#endif
      return color;
    }

#else
    return marchScalarField(ray, tStart, tEnd);
#endif
  }
  return vec4(0);
}

void main()
{
  outColor = rayMarch(generatePrimaryRay());
}
