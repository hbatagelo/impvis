#version 300 es

/**
 * @file raycast.frag
 *
 * This file is part of ImpVis (https://github.com/hbatagelo/impvis).
 *
 * @copyright (c) 2022--2026 Harlen Batagelo. All rights reserved.
 * ImpVis is released under the MIT license.
 */

precision highp float;

in vec2 fragPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outData1;
layout(location = 2) out vec4 outData2;

////////////////////////////////////////////////////////////////////////////////
// Constants and struct definitions
////////////////////////////////////////////////////////////////////////////////

@DEFINITIONS@

const float kBoundRadius = float(@BOUND_RADIUS@);
const float kBoundRadiusSquared = kBoundRadius * kBoundRadius;
const float kInvBoundRadius = 1.0 / kBoundRadius;
const float kInvBoundRadius2 = 1.0 / (kBoundRadius * 2.0);
const vec3 kBoundsMin = vec3(-kBoundRadius);
const vec3 kBoundsMax = vec3( kBoundRadius);

// MSAA sample patterns
const vec2 kMSAAPattern2x[2] = vec2[2](
  vec2(-0.25, -0.25), vec2( 0.25, 0.25)
);

const vec2 kMSAAPattern4x[4] = vec2[4](
  vec2(-0.375, -0.125), vec2(0.125, -0.375),
  vec2(-0.125,  0.375), vec2(0.375,  0.125)
);

const vec2 kMSAAPattern8x[8] = vec2[8](
  vec2(-0.4375, -0.1875), vec2(0.1875, -0.4375),
  vec2(-0.1875,  0.4375), vec2(0.4375,  0.1875),
  vec2(-0.3125,  0.0625), vec2(0.0625, -0.3125),
  vec2(-0.0625,  0.3125), vec2(0.3125, -0.0625)
);

struct Ray
{
  vec3 origin;
  vec3 direction;
};

////////////////////////////////////////////////////////////////////////////////
// Uniform data
////////////////////////////////////////////////////////////////////////////////

// Camera settings
struct Camera
{
                       // align  offset
  vec3 eye;            //    4N      0N
  vec2 pixelSize;      //    2N      4N
  mat4 viewMatrix;     //    4N      8N
                       //    4N     12N
                       //    4N     16N
                       //    4N     20N
  mat4 invViewMatrix;  //    4N     24N
                       //    4N     28N
                       //    4N     32N
                       //    4N     36N
  mat4 projMatrix;     //    4N     40N
                       //    4N     44N
                       //    4N     48N
                       //    4N     52N
  mat4 invProjMatrix;  //    4N     56N
                       //    4N     60N
                       //    4N     64N
                       //    4N     68N
  mat4 modelMatrix;    //    4N     72N
                       //    4N     76N
                       //    4N     80N
                       //    4N     84N
  mat4 invModelMatrix; //    4N     88N
                       //    4N     92N
                       //    4N     96N
                       //    4N    100N
  mat3 normalMatrix;   //    4N    104N
                       //    4N    108N
                       //    4N    112N
  float maxModelScale; //    1N    116N
};

// Shading properties
struct Shading
{
                      // align  offset
  vec3 insideKdId;    //    4N      4N
  vec3 outsideKdId;   //    4N      8N
  vec3 lightDirWorld; //    4N     12N
  float shininess;    //    1N     16N
};

// Function parameters (allow up to 16 scalar parameters)
struct Params {
                // align  offset
  vec4 data[4]; //    4N      0N
                //    4N      4N
                //    4N      8N
                //    4N     12N
};
layout (std140) uniform CameraBlock  { Camera uCamera;   };
layout (std140) uniform ShadingBlock { Shading uShading; };
layout (std140) uniform ParamsBlock  { Params uParams;   };

uniform float uIsoValue;
uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;
uniform float uGaussianCurvatureFalloff;
uniform float uMeanCurvatureFalloff;
uniform float uMaxAbsCurvatureFalloff;
uniform float uNormalLengthFalloff;
uniform float uDVRFalloff;
uniform float uDVRAbsorptionCoeff;

////////////////////////////////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////////////////////////////////

float mpow2 (in float b)
{
  return b * b;
}
float mpow3 (in float b)
{
  return b * b * b;
}
float mpow4 (in float b)
{
  float b2 = b * b;
  return b2 * b2;
}
float mpow5 (in float b)
{
  float b2 = b * b;
  return b2 * b2 * b;
}
float mpow6 (in float b)
{
  float b3 = b * b * b;
  return b3 * b3;
}
float mpow7 (in float b)
{
  float b2 = b * b;
  return b2 * b2 * b2 * b;
}
float mpow8 (in float b)
{
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4;
}
float mpow9 (in float b)
{
  float b3 = b * b * b;
  return b3 * b3 * b3;
}
float mpow10(in float b)
{
  float b2 = b * b;
  float b5 = b2 * b2 * b;
  return b5 * b5;
}
float mpow11(in float b)
{
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4 * b2 * b;
}
float mpow12(in float b)
{
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4 * b4;
}
float mpow13(in float b)
{
  float b2 = b * b;
  float b4 = b2 * b2;
  return b4 * b4 * b4 * b;
}
float mpow14(in float b)
{
  float b2 = b * b;
  float b7 = b2 * b2 * b2 * b;
  return b7 * b7;
}
float mpow15(in float b)
{
  float b2 = b * b;
  float b5 = b2 * b2 * b;
  return b5 * b5 * b5;
}
float mpow16(in float b)
{
  float b2 = b * b;
  float b4 = b2 * b2;
  float b8 = b4 * b4;
  return b8 * b8;
}
float mpow(in float b, in float n)
{
  return pow(b, n);
}

// Injected global code
@CODE_GLOBAL@

float evalFunction(in vec3 P)
{
  float x = P.x, y = P.y, z = P.z;

  // Injected local code
  @CODE_LOCAL@

  // Injected left-hand side of the expression
  return (@EXPRESSION_LHS@) - uIsoValue;
}

/*
 * Evaluates a one-sided sigmoid for input x in (-inf, +inf) and falloff k>0.
 * The returned value is in the range [0, 1].
 */
float halfSigmoid(in float x, in float k)
{
  return clamp(tanh(max(0.0, k) * x), 0.0, 1.0);
}

/*
 * Evaluates a two-sided sigmoid for input x in (-inf, +inf) and falloff k.
 * The returned value is in the range [0, 1].
 */
float sigmoid(in float x, in float k)
{
  return (tanh(k * x) + 1.0) * 0.5;
}

/*
 * Samples the color of a sequential colormap for a given x in [0,1].
 */
vec4 sampleSequentialColormap(in float x)
{
  // SEQ_COLORMAP_SIZE and SEQ_COLORMAP are defined in @DEFINITIONS@
  float t = x * float(SEQ_COLORMAP_SIZE - 1);
  int i = int(floor(t));
  int j = min(i + 1, SEQ_COLORMAP_SIZE - 1);
  return mix(SEQ_COLORMAP[i], SEQ_COLORMAP[j], fract(t));
}

/*
 * Samples the color of a diverging colormap for a given x in [0,1].
 */
vec4 sampleDivergingColormap(in float x)
{
  // DIV_COLORMAP_SIZE and DIV_COLORMAP are defined in @DEFINITIONS@
  float t = x * float(DIV_COLORMAP_SIZE - 1);
  int i = int(floor(t));
  int j = min(i + 1, DIV_COLORMAP_SIZE - 1);
  return mix(DIV_COLORMAP[i], DIV_COLORMAP[j], fract(t));
}

/*
 * Gradient evaluation using forward difference.
 * f'(x) ≈ (f(x+h) - f(x)) / h
 * O(h) error, 4 evaluations.
 */
vec3 evalGradientForward(in vec3 P)
{
  const float h = 1e-4;
  const float invH = 1.0 / h;

  float f0 = evalFunction(P);
  return vec3(
    evalFunction(P + vec3(h, 0.0, 0.0)) - f0,
    evalFunction(P + vec3(0.0, h, 0.0)) - f0,
    evalFunction(P + vec3(0.0, 0.0, h)) - f0
  ) * invH;
}

/*
 * Gradient evaluation using central difference.
 * f'(x) ≈ (f(x+h) - f(x-h)) / (2h)
 * O(h^2) error, 6 evaluations.
 */
vec3 evalGradientCentral(in vec3 P)
{
  const float h = 1e-4;
  const float inv2H = 0.5 / h;

  const vec3 dx = vec3(h, 0.0, 0.0);
  const vec3 dy = vec3(0.0, h, 0.0);
  const vec3 dz = vec3(0.0, 0.0, h);

  return vec3(
    evalFunction(P + dx) - evalFunction(P - dx),
    evalFunction(P + dy) - evalFunction(P - dy),
    evalFunction(P + dz) - evalFunction(P - dz)
  ) * inv2H;
}

/*
 * Gradient evaluation using 5-point stencil:
 * f'(x) ≈ (-f(x+2h) + 8f(x+h) - 8f(x-h) + f(x-2h)) / (12h)
 * O(h^4) error, 12 evaluations.
 */
vec3 evalGradientFivePoint(in vec3 P)
{
  const float h = 1e-3;
  const float inv12H = 1.0 / (12.0 * h);

  const vec3 dx = vec3(h, 0.0, 0.0);
  const vec3 dy = vec3(0.0, h, 0.0);
  const vec3 dz = vec3(0.0, 0.0, h);

  float nx = -evalFunction(P + 2.0*dx) + 8.0*evalFunction(P + dx)
             -8.0*evalFunction(P - dx) + evalFunction(P - 2.0*dx);
  float ny = -evalFunction(P + 2.0*dy) + 8.0*evalFunction(P + dy)
             -8.0*evalFunction(P - dy) + evalFunction(P - 2.0*dy);
  float nz = -evalFunction(P + 2.0*dz) + 8.0*evalFunction(P + dz)
             -8.0*evalFunction(P - dz) + evalFunction(P - 2.0*dz);

  return vec3(nx, ny, nz) * inv12H;
}

/*
 * Evaluates gradient at P.
 */
vec3 evalGradient(in vec3 P)
{
#if defined(GRADIENT_FIVE_POINT_STENCIL)
  return evalGradientFivePoint(P);
#elif defined(GRADIENT_CENTRAL_DIFFERENCE)
  return evalGradientCentral(P);
#else
  return evalGradientForward(P);
#endif
}

#if defined(USE_CURVATURE)

/*
 * Computes the Hessian matrix at P.
 * O(h^2) error.
 */
mat3 evalHessian(in vec3 P)
{
  const float h = 1e-2;
  const float invH2 = 1.0 / (h * h);
  const float quarterInvH2 = 0.25 * invH2;

  const vec3 dx = vec3(h, 0.0, 0.0);
  const vec3 dy = vec3(0.0, h, 0.0);
  const vec3 dz = vec3(0.0, 0.0, h);

  float f0x2 = evalFunction(P) * 2.0;

  // Second derivatives (diagonal elements)
  float Hxx = (evalFunction(P + dx) - f0x2 + evalFunction(P - dx)) * invH2;
  float Hyy = (evalFunction(P + dy) - f0x2 + evalFunction(P - dy)) * invH2;
  float Hzz = (evalFunction(P + dz) - f0x2 + evalFunction(P - dz)) * invH2;

  // Mixed partial derivatives (off-diagonal elements)
  float Hxy = (evalFunction(P + dx + dy) - evalFunction(P + dx - dy) -
               evalFunction(P - dx + dy) + evalFunction(P - dx - dy)) * quarterInvH2;
  float Hxz = (evalFunction(P + dx + dz) - evalFunction(P + dx - dz) -
               evalFunction(P - dx + dz) + evalFunction(P - dx - dz)) * quarterInvH2;
  float Hyz = (evalFunction(P + dy + dz) - evalFunction(P + dy - dz) -
               evalFunction(P - dy + dz) + evalFunction(P - dy - dz)) * quarterInvH2;

  return mat3(
    Hxx, Hxy, Hxz,
    Hxy, Hyy, Hyz,
    Hxz, Hyz, Hzz
  );
}

/*
 * Computes the Gaussian curvature K, mean curvature H and
 * principal curvatures k1 and k2 at point P on the surface.
 * Returns vec4(K, H, k1, k2) with k1 >= k2.
 * When inside is true, the inward normal is used.
 */
vec4 computeCurvatures(in vec3 P, bool inside)
{
  const float eps2 = 1e-12;

  vec3 grad = evalGradient(P);
  float gradLen2 = dot(grad, grad);

  // Degenerate case: undefined normal
  if (gradLen2 < eps2)
  {
    return vec4(0.0);
  }

  float invGradLen  = inversesqrt(gradLen2);
  float invGradLen2 = invGradLen * invGradLen;
  float invGradLen4 = invGradLen2 * invGradLen2;

  mat3 Hess = evalHessian(P);

  // Extract symmetric entries
  float a = Hess[0][0], b = Hess[0][1], c = Hess[0][2],
            /*b*/       d = Hess[1][1], e = Hess[1][2],
            /*c*/       /*e*/           f = Hess[2][2];

  // Adjugate of Hessian
  mat3 adjHess = mat3(
    d*f - e*e, e*c - b*f, b*e - d*c,
    e*c - b*f, a*f - c*c, b*c - a*e,
    b*e - d*c, b*c - a*e, a*d - b*b
  );

  // Gaussian curvature
  // K = ( ∇F^t * adj(HessF) * ∇F ) / |∇F|^4
  float K = dot(grad, adjHess * grad) * invGradLen4;

  // Mean curvature (outward normal)
  // H = ( |∇F|² * tr(HessF) - ∇F^t * HessF * ∇F ) / (2 * |∇F|^3)
  float gradHgrad = dot(grad, Hess * grad);
  float traceHess = a + d + f;
  float H = (traceHess - gradHgrad * invGradLen2) * 0.5 * invGradLen;

  // Flip orientation if inward normal is requested
  H *= inside ? -1.0 : 1.0;

  // Minimal-surface stabilization
  if (abs(H) < 1e-6 && K < 0.0)
  {
    float k = sqrt(-K);
    return vec4(K, H, k, -k);
  }

  // κ₁,₂ = H ± sqrt(H² - K)
  float disc = H * H - K;
  float s = sqrt(max(disc, 0.0));

  float kappa1 = H + s;
  float kappa2 = H - s;

  return vec4(K, H, kappa1, kappa2);
}

#endif // USE_CURVATURE

/*
 * Intersects ray with an axis-aligned bounding box using the slab test.
 */
bool intersectAABB(in  Ray   ray   /* ray origin and direction          */,
                   out float tNear /* ray parameter at 1st intersection */,
                   out float tFar  /* ray parameter at 2nd intersection */)
{
  vec3 invDirection = 1.0 / ray.direction;
  vec3 aEntry = (kBoundsMin - ray.origin) * invDirection;
  vec3 aExit = (kBoundsMax - ray.origin) * invDirection;

  vec3 aNear = min(aEntry, aExit);
  vec3 aFar = max(aEntry, aExit);

  tNear = max(max(aNear.x, aNear.y), aNear.z);
  tNear = max(tNear, 0.0);
  tFar = min(min(aFar.x, aFar.y), aFar.z);

  return tFar > tNear;
}

/*
 * Intersects ray with sphere centered at origin.
 * For simplicity, assume the sphere is always in front of the ray.
 */
bool intersectSphere(in  Ray   ray   /* ray origin and direction          */,
                     out float tNear /* ray parameter at 1st intersection */,
                     out float tFar  /* ray parameter at 2nd intersection */)
{
  vec3 originToCenter = -ray.origin; // sphere center is at (0,0,0)
  float projCenterDist = dot(originToCenter, ray.direction);
  float centerOffsetSq = dot(originToCenter, originToCenter) - kBoundRadiusSquared;
  float discriminant = projCenterDist * projCenterDist - centerOffsetSq;

  if (discriminant < 0.0)
  {
    return false;
  }

  float sqrtDiscriminant = sqrt(discriminant);
  tNear = max(0.0, projCenterDist - sqrtDiscriminant);
  tFar = projCenterDist + sqrtDiscriminant;

  return true;
}

/*
 * Intersects axis-aligned bounding box centered at origin with ray starting
 * from inside the box.
 *
 * Returns ray parameter at point of exit.
 */
float intersectAABBFromInside(in Ray ray)
{
  vec3 aEntry = (kBoundsMin - ray.origin) / ray.direction;
  vec3 aExit = (kBoundsMax - ray.origin) / ray.direction;
  vec3 aFar = max(aEntry, aExit);

  return min(min(aFar.x, aFar.y), aFar.z);
}

/*
 * Intersects sphere centered at origin with ray starting from inside the
 * sphere. Assumes normalized ray direction.
 *
 * Returns ray parameter at point of exit.
 */
float intersectSphereFromInside(in Ray ray)
{
  float projOriginDist = dot(ray.direction, ray.origin);
  float centerOffsetSq = dot(ray.origin, ray.origin) - kBoundRadiusSquared;
  float discriminant = projOriginDist * projOriginDist - centerOffsetSq;

  return -projOriginDist + sqrt(discriminant);
}

/*
 * Uses simple sign test to check whether there is a root in a ray parameter
 * interval.
 */
bool signTest(in float fa /* function value at start of interval */,
              in float fb /* function value at end of interval   */)
{
  return fa * fb <= 0.0;
}

/*
 * Uses 1st-order Taylor expansion to check whether there is a root in the given interval.
 */
bool taylorTest1stOrder(in Ray   ray /* ray origin and direction            */,
                        in float ta  /* ray parameter at start of interval  */,
                        in float tb  /* ray parameter at end of interval    */,
                        in float fa  /* function value at start of interval */,
                        in float fb  /* function value at end of interval   */)
{
  vec3 Pa = ray.origin + ray.direction * ta;
  vec3 Pb = ray.origin + ray.direction * tb;

  // First derivatives
  float fda = dot(evalGradient(Pa), ray.direction);
  float fdb = dot(evalGradient(Pb), ray.direction);

  // 1st-order Taylor expansion
  float halfH = (tb - ta) * 0.5;
  float p = fa;
  float q = fa + fda * halfH;
  float r = fb - fdb * halfH;
  float s = fb;

  return min(min(p, q), min(r, s)) * max(max(p, q), max(r, s)) <= 0.0;
}

/*
 * Uses 2nd-order Taylor expansion to check whether there is a root in the given interval.
 */
bool taylorTest2ndOrder(in Ray   ray /* ray origin and direction            */,
                        in float ta  /* ray parameter at start of interval  */,
                        in float tb  /* ray parameter at end of interval    */,
                        in float fa  /* function value at start of interval */,
                        in float fb  /* function value at end of interval   */)
{
  float h = tb - ta;
  float halfH = h * 0.5;

  vec3 Pa = ray.origin + ray.direction * ta;
  vec3 Pb = ray.origin + ray.direction * tb;
  vec3 Pm = ray.origin + ray.direction * (ta + halfH);

  // First derivatives
  float fda = dot(evalGradient(Pa), ray.direction);
  float fdb = dot(evalGradient(Pb), ray.direction);
    // Evaluate function at midpoint
  float fm = evalFunction(Pm);

  // Estimate second derivative using three points
  // f'' ≈ [f(ta) - 2f(tm) + f(tb)] / (h/2)^2
  float sd = (fa - 2.0 * fm + fb) / (h * h * 0.25);

  // 2nd-order Taylor expansions
  float p = fa;
  float q = fa + fda * halfH + 0.5 * sd * halfH * halfH;
  float r = fb - fdb * halfH + 0.5 * sd * halfH * halfH;
  float s = fb;

  return min(min(p, q), min(r, s)) * max(max(p, q), max(r, s)) <= 0.0;
}

/*
 * Uses 3rd-order Taylor expansion to check whether there is a root in the given interval.
 */
bool taylorTest3rdOrder(in Ray   ray /* ray origin and direction            */,
                        in float ta  /* ray parameter at start of interval  */,
                        in float tb  /* ray parameter at end of interval    */,
                        in float fa  /* function value at start of interval */,
                        in float fb  /* function value at end of interval   */)
{
  float h = tb - ta;
  float quarterH = h * 0.25;

  vec3 Pa = ray.origin + ray.direction * ta;
  vec3 P1 = ray.origin + ray.direction * (ta + quarterH);
  vec3 P2 = ray.origin + ray.direction * (ta + 2.0 * quarterH);
  vec3 P3 = ray.origin + ray.direction * (ta + 3.0 * quarterH);
  vec3 Pb = ray.origin + ray.direction * tb;

  // Function values
  float f1 = evalFunction(P1);
  float f2 = evalFunction(P2);
  float f3 = evalFunction(P3);

  // First derivatives at endpoints
  float fda = dot(evalGradient(Pa), ray.direction);
  float fdb = dot(evalGradient(Pb), ray.direction);

  // Estimate third derivative using five points
  // f''' ≈ [f(tb) - 3f(ta+3h/4) + 3f(ta+h/2) - f(ta+h/4)] / (h/4)^3
  float td = (fb - 3.0 * f3 + 3.0 * f2 - f1) / (quarterH * quarterH * quarterH);

  // Estimate second derivative
  float sd = (fa - 2.0 * f2 + fb) / (h * h * 0.25);

  // 3rd-order Taylor expansions
  float halfH = h * 0.5;
  float q = fa + fda * halfH + 0.5 * sd * halfH * halfH + (1.0/6.0) * td * halfH * halfH * halfH;
  float r = fb - fdb * halfH + 0.5 * sd * halfH * halfH - (1.0/6.0) * td * halfH * halfH * halfH;

  return min(min(fa, q), min(r, fb)) * max(max(fa, q), max(r, fb)) <= 0.0;
}

/*
 * Intersects ray with implicit surface using ray marching.
 *
 * This is a simple ray marching algorithm that divides the ray interval into
 * fixed-size segments and tests each segment using a root test.
 */
bool fixedMarch(in  Ray   ray    /* ray origin and direction            */,
                in  float tStart /* ray parameter at start of interval  */,
                in  float tEnd   /* ray parameter at end of interval    */,
                out float tHit   /* ray parameter at surface hit        */,
                out bool  inside /* true if surface was hit from inside */)
{
  float dt = (tEnd - tStart) / float(ISOSURFACE_RAYMARCH_STEPS);

  float t = tStart;
  float curValue = evalFunction(ray.origin + ray.direction * t);

  for (int i = 0; i < ISOSURFACE_RAYMARCH_STEPS; ++i)
  {
    t += dt;
    float nextValue = evalFunction(ray.origin + ray.direction * t);

#if defined(USE_SIGN_TEST)
    if (signTest(curValue, nextValue))
#else // USE_SIGN_TEST
#if defined(USE_TAYLOR_1ST)
    if (taylorTest1stOrder(ray, t - dt, t, curValue, nextValue))
#else // USE_TAYLOR_1ST
#if defined(USE_TAYLOR_2ND)
    if (taylorTest2ndOrder(ray, t - dt, t, curValue, nextValue))
#else // USE_TAYLOR_2ND
    if (taylorTest3rdOrder(ray, t - dt, t, curValue, nextValue))
#endif // USE_TAYLOR_2ND
#endif // USE_TAYLOR_1ST
#endif // USE_SIGN_TEST
    {
      tHit = t + (nextValue * dt) / (curValue - nextValue);
      inside = curValue < 0.0 ? true : false;
      return true;
    }

    curValue = nextValue;
  }

  return false;
}

/*
 * Constants used by adaptiveMarch and adaptiveMarchShadow.
 */
const float minDtScale = 0.25;
const float maxDtScale = 1.5;
const float tau = 0.05;
const float invTau = 1.0 / tau;
const float piOver180 = 3.14159265359 / 180.0;
const float epsAngle = cos(85.0 * piOver180);
const int maxSteps = ISOSURFACE_RAYMARCH_STEPS * int(1.0 / (minDtScale * minDtScale));

/*
 * Intersects a ray with an implicit surface using adaptive ray marching.
 *
 * The step size is adjusted according to the function value
 * S: R^3 -> R and its gradient ∇S, evaluated at the current step.
 *
 * Ray marching starts with a base step size multiplied by |S|,
 * clamped to [s_min, s_max], where s_min and s_max are the minimum
 * and maximum step scaling factors (we use s_min = 0.25 and s_max = 1.5).
 *
 * If |S| < tau, where tau is a small distance from the level set (we use
 * tau = 0.05), and the ray is close to the silhouette of the isosurface,
 * the step size is further multiplied by max{s_min, |S| / tau}. Thus,
 * the step size decreases as the ray approaches the surface near a
 * silhouette. The silhouette condition is tested by checking whether
 * the ray has a small grazing angle relative to the local gradient:
 *
 *   |∇S · r_dir| < cos(pi/2 − theta),
 *
 * where r_dir is the ray direction and theta is the grazing-angle threshold.
 * We use theta = pi/36 = 5°.
 *
 * Since rays exit the bounding geometry with different step sizes,
 * artifacts may appear at exit points. To avoid this, the step size
 * is reduced as the ray approaches the exit. The distance to the exit
 * is measured as l = t_end − t_cur, where t_end is the ray parameter
 * at the exit point and t_cur <= t_end is the current ray parameter.
 * If l is smaller than an epsilon, the step size is multiplied by
 * max{s_min, l / τ}. The epsilon is chosen as the base step size
 * multiplied by s_max.
 */
bool adaptiveMarch(in  Ray   ray    /* ray origin and direction            */,
                   in  float tStart /* ray parameter at start of interval  */,
                   in  float tEnd   /* ray parameter at end of interval    */,
                   out float tHit   /* ray parameter at surface hit        */,
                   out bool  inside /* true if surface was hit from inside */)
{
  float baseDt = (tEnd - tStart) / float(ISOSURFACE_RAYMARCH_STEPS);
  float epsExit = baseDt * maxDtScale;
  float invEpsExit = 1.0 / epsExit;

  float t = tStart;
  vec3 P = ray.origin + ray.direction * t;
  float curValue = evalFunction(P);

  for (int i = 0; i < maxSteps; ++i)
  {
    float curValueAbs = abs(curValue);

    // Step size is proportional to the function value
    float dt = baseDt * clamp(curValueAbs, minDtScale, maxDtScale);

    // Decrease the step size when the ray is near the surface (<= tau) at a
    // small grazing angle
    if ((curValueAbs < tau &&
        abs(dot(evalGradient(P), ray.direction)) < epsAngle))
    {
      dt *= max(curValueAbs * invTau, minDtScale);
    }
    else
    {
      // Decrease the step size if the ray is exiting the bounding geometry
      float distFromExit = tEnd - t;
      if (distFromExit < epsExit)
      {
        dt *= max(distFromExit * invEpsExit, minDtScale);
      }
    }

    t += dt;
    P += ray.direction * dt;
    float nextValue = evalFunction(P);

#if defined(USE_SIGN_TEST)
    if (signTest(curValue, nextValue))
#else // USE_SIGN_TEST
#if defined(USE_TAYLOR_1ST)
    if (taylorTest1stOrder(ray, t - dt, t, curValue, nextValue))
#else // USE_TAYLOR_1ST
#if defined(USE_TAYLOR_2ND)
    if (taylorTest2ndOrder(ray, t - dt, t, curValue, nextValue))
#else // USE_TAYLOR_2ND
    if (taylorTest3rdOrder(ray, t - dt, t, curValue, nextValue))
#endif // USE_TAYLOR_2ND
#endif // USE_TAYLOR_1ST
#endif // USE_SIGN_TEST
    {
      float diffValue = curValue - nextValue;
#if !defined(USE_SIGN_TEST)
      if (abs(diffValue) < 1e-5)
      {
        tHit = t - dt;
      }
      else
#endif // USE_SIGN_TEST
      {
        tHit = t + (nextValue * dt) / diffValue;
      }
      inside = curValue < 0.0 ? true : false;
      return true;
    }

    // Early exit
    if (t >= tEnd)
    {
      break;
    }

    curValue = nextValue;
  }

  inside = false;
  return false;
}

/*
 * Direct volume rendering.
 * Front-to-back emission-absorption compositing.
 */
vec4 dvrMarch(in Ray   ray    /* ray origin and direction           */,
              in float tStart /* ray parameter at start of interval */,
              in float tEnd   /* ray parameter at end of interval   */)
{
  float ds = (tEnd - tStart) / float(DVR_RAYMARCH_STEPS);
  float opticalDepthScale = ds * kInvBoundRadius * uDVRAbsorptionCoeff;

  vec3 samplePos = ray.origin + ray.direction * tStart;
  vec3 rayStep   = ray.direction * ds;

  vec3 radiance  = vec3(0.0);
  float opacity  = 0.0;

  // How much "matter" the ray passed through
  float opticalDepth = 0.0;
  // Maximum contribution sample: where the volume "is"
  float maxContribution = 0.0;
  float tMax = tStart;
  // Average scalar along the ray
  float scalarSum = 0.0;
  float weightSum = 0.0;

  for (int i = 0; i < DVR_RAYMARCH_STEPS; ++i)
  {
    // Sample scalar field -> emission color + base opacity
    float scalar = evalFunction(samplePos);
    vec4 emission = sampleDivergingColormap(sigmoid(scalar, uDVRFalloff));

    // Beer-Lambert absorption
    float alpha = 1.0 - exp(-emission.a * opticalDepthScale);

    // Front-to-back emission-absorption compositing
    float transmittance = 1.0 - opacity;
    radiance += transmittance * alpha * emission.rgb;
    opacity  += transmittance * alpha;

    opticalDepth += emission.a * opticalDepthScale;
    float contribution = transmittance * alpha;
    if (contribution > maxContribution)
    {
      maxContribution = contribution;
      tMax = tStart + float(i) * ds;
    }

    scalarSum += scalar * alpha;
    weightSum += alpha;

    // Early ray termination
    if (opacity >= 1.0)
    {
      break;
    }

    samplePos += rayStep;
  }

  outData1 = vec4(ray.origin + ray.direction * tMax, 1.0);

  float avgScalar = (weightSum > 0.0) ? scalarSum / weightSum : 0.0;
  outData2 = vec4(opticalDepth, avgScalar, opacity, 0.0);

  return vec4(radiance, opacity);
}

/*
 * Same as adaptiveMarch, but simplified for shadow rays.
 */
bool adaptiveMarchShadow(in Ray   ray  /* ray origin and direction */,
                         in float tEnd /* ray parameter at exit    */)
{
  float baseDt = tEnd / float(ISOSURFACE_RAYMARCH_STEPS);
  float epsExit = baseDt * maxDtScale;
  float invEpsExit = 1.0 / epsExit;

  float t = 0.0;
  vec3 P = ray.origin;
  float curValue = evalFunction(P);

  for (int i = 0; i < maxSteps; ++i)
  {
    float leftValueAbs = abs(curValue);

    // Step size is proportional to the function value
    float dt = baseDt * clamp(leftValueAbs, minDtScale, maxDtScale);

    // Halve the step size when the ray is near the surface (<= tau) at a
    // small grazing angle
    if (leftValueAbs <= tau &&
        abs(dot(evalGradient(P), ray.direction)) < epsAngle)
    {
      dt *= max(leftValueAbs * invTau, minDtScale);
    }
    else
    {
      float distFromExit = tEnd - t;
      if (distFromExit < epsExit)
      {
        dt *= max(distFromExit * invEpsExit, minDtScale);
      }
    }

    P += ray.direction * dt;
    float nextValue = evalFunction(P);

    if (signTest(curValue, nextValue))
    {
      return true;
    }

    t += dt;
    if (t >= tEnd)
    {
      break;
    }

    curValue = nextValue;
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
#else // USE_BOUNDING_BOX
  float tEnd = intersectSphereFromInside(ray);
#endif // USE_BOUNDING_BOX
  return adaptiveMarchShadow(ray, tEnd);
}

/*
 * Determines the color of the shaded surface at P with normal N.
 * Diffuse color may be colorcoded normals (SHOW_NORMAL_VECTOR),
 * normal magnitude (SHOW_NORMAL_MAGNITUDE),
 * curvature (SHOW_GAUSSIAN_CURVATURE,SHOW_MEAN_CURVATURE,
 * SHOW_MAX_CURVATURE), or a constant color.
 */
vec4 shade(in vec3 PView  /* intersection point in view space */,
           in vec3 PModel /* intersection point in model space */,
           in vec3 NModel /* Unnormalized normal in model space */,
           in bool inside /* whether the ray hit from inside */)
{
  vec3 KsIs = vec3(1.0);
#if !defined(SHOW_NORMAL_VECTOR) && !defined(SHOW_NORMAL_MAGNITUDE) && !defined(USE_CURVATURE)
  vec3 KdId = inside ? uShading.insideKdId : uShading.outsideKdId;
#else // !SHOW_NORMAL_VECTOR && !SHOW_NORMAL_MAGNITUDE && !USE_CURVATURE
  vec3 KdId = vec3(0.7);
#endif // !SHOW_NORMAL_VECTOR && !SHOW_NORMAL_MAGNITUDE && !USE_CURVATURE

  vec3 ambientColor = vec3(0.0);
  vec3 diffuseColor = KdId;
  vec3 specularColor = vec3(0.0);

#if defined(USE_SHADOWS)
  float lightMask = 1.0;
  vec3 LModel = normalize(mat3(uCamera.invModelMatrix) * (-uShading.lightDirWorld) );
  if (inShadow(PModel, LModel))
  {
    lightMask = 0.05;
  }
#endif // USE_SHADOWS

#if defined(USE_BLINN_PHONG)
  ambientColor = KdId * 0.25;
  vec3 NView = normalize(uCamera.normalMatrix * NModel);
  vec3 N = inside ? -NView : NView;
  vec3 L = normalize(mat3(uCamera.viewMatrix) * (-uShading.lightDirWorld) );
  float lambertian = max(dot(N, L), 0.0);
  if (lambertian > 0.0)
  {
#if defined(USE_SHADOWS)
    if (lightMask == 1.0)
    {
      vec3 V = normalize(-PView);
      vec3 H = normalize(L + V);
      float angle = max(dot(H, N), 0.0);
      float specular = pow(angle, uShading.shininess);
      specularColor = KsIs * specular;
    }
#else // USE_SHADOWS
    vec3 V = normalize(-PView);
    vec3 H = normalize(L + V);
    float angle = max(dot(H, N), 0.0);
    float specular = pow(angle, uShading.shininess);
    specularColor = KsIs * specular;
#endif // USE_SHADOWS
  }
#else // USE_BLINN_PHONG
  // Unlit
  float lambertian = 1.0;
#endif // USE_BLINN_PHONG

#if defined(SHOW_NORMAL_VECTOR)
  vec3 diffuseColorNormal = normalize(NModel);
#if defined(INWARD_NORMALS)
  diffuseColorNormal *= inside ? -1.0 : 1.0;
#endif // INWARD_NORMALS
  diffuseColor = (diffuseColorNormal + 1.0) * 0.5;
#endif // SHOW_NORMAL_VECTOR

#if defined(SHOW_NORMAL_MAGNITUDE)
  float t = halfSigmoid(length(NModel), uNormalLengthFalloff);
  diffuseColor = sampleSequentialColormap(t).rgb;
#endif // SHOW_NORMAL_MAGNITUDE

#if defined(USE_CURVATURE)
  vec4 KHKappas = computeCurvatures(PModel, inside);
  outData2 = KHKappas;
#endif // USE_CURVATURE

#if defined(SHOW_GAUSSIAN_CURVATURE)
  // Negative Gaussian curvature ->  hyperbolic
  // Zero Gaussian curvature -> parabolic
  // Positive Gaussian curvature-> elliptic
  float t = sigmoid(KHKappas.x, uGaussianCurvatureFalloff);
  diffuseColor = sampleDivergingColormap(t).rgb;
#endif // SHOW_GAUSSIAN_CURVATURE

#if defined(SHOW_MEAN_CURVATURE)
  // Negative mean curvature -> concave
  // Zero mean curvature -> minimal surface
  // Positive mean curvature -> convex
  float t = sigmoid(KHKappas.y, uMeanCurvatureFalloff);
  diffuseColor = sampleDivergingColormap(t).rgb;
#endif // SHOW_MEAN_CURVATURE

#if defined(SHOW_MAX_CURVATURE)
  float maxAbsKappas = max(abs(KHKappas.z), abs(KHKappas.w));
  float t = halfSigmoid(maxAbsKappas, uMaxAbsCurvatureFalloff);
  // t=0 -> Flat
  // t=1 -> Highly curved
  diffuseColor = sampleSequentialColormap(t).rgb;
#endif // SHOW_MAX_CURVATURE

  diffuseColor *= lambertian;

#if defined(USE_SHADOWS)
  diffuseColor *= lightMask;
#endif // USE_SHADOWS

  vec3 totalColor = ambientColor + diffuseColor + specularColor;

  return vec4(totalColor, 1.0);
}

/*
 * Generates the primary ray in world-space coordinates with a
 * given screen-space pixel offset, then transforms it to model space.
 */
Ray generatePrimaryRay(vec2 pixelOffset)
{
  vec2 posNDC = fragPosition;
#if defined(MSAA_ENABLED)
  posNDC += pixelOffset * uCamera.pixelSize;
#endif

  bool isPerspective = (uCamera.projMatrix[3][3] == 0.0);

  vec3 originWorld, dirWorld;

  if (isPerspective)
  {
    // Undo projection scale
    float px = posNDC.x / uCamera.projMatrix[0][0];
    float py = posNDC.y / uCamera.projMatrix[1][1];

    vec3 dirView = normalize(vec3(px, py, -1.0));
    dirWorld = normalize(mat3(uCamera.invViewMatrix) * dirView);
    originWorld = uCamera.eye;
  }
  else
  {
    vec3 dirView = vec3(0.0, 0.0, -1.0);
    dirWorld = normalize(mat3(uCamera.invViewMatrix) * dirView);

    // For each pixel, origin is the unprojected position on the near plane
    vec3 originView = vec3(
        posNDC.x / uCamera.projMatrix[0][0],
        posNDC.y / uCamera.projMatrix[1][1],
        -1.0
    );

    originWorld = uCamera.eye + (mat3(uCamera.invViewMatrix) * originView);
  }

  // Transform ray from world space to model space
  vec3 originModel = (uCamera.invModelMatrix * vec4(originWorld, 1.0)).xyz;
  vec3 dirModel = normalize(mat3(uCamera.invModelMatrix) * dirWorld);

  return Ray(originModel, dirModel);
}

/*
 * Calculates the maximum ray parameter based on depth buffer.
 */
float getMaxRayParamFromDepth(float depth, vec2 screenCoord, Ray rayModel)
{
  // If depth is 1.0, there's no geometry rendered
  if (depth < 0.9999)
  {
    vec4 posNDC = vec4(screenCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 posView = uCamera.invProjMatrix * posNDC;
    posView /= posView.w;

    vec3 rayOriginWorld = (uCamera.modelMatrix * vec4(rayModel.origin, 1.0)).xyz;
    vec3 rayDirWorld = mat3(uCamera.modelMatrix) * rayModel.direction;

    // Calculate the scaling factor (ratio of world space to model space distance)
    float worldDirLength = length(rayDirWorld);
    vec3 rayDirWorldNormalized = rayDirWorld / worldDirLength;

    vec3 rayOriginView = (uCamera.viewMatrix * vec4(rayOriginWorld, 1.0)).xyz;
    vec3 rayDirView = normalize((uCamera.viewMatrix * vec4(rayDirWorldNormalized, 0.0)).xyz);

    // Calculate distance along ray to reach the depth plane
    // rayOriginView.z + t * rayDirView.z = -viewDepth
    if (abs(rayDirView.z) > 1e-6)
    {
      // View depth is negative Z
      float viewDepth = -posView.z;
      float tWorld = (-viewDepth - rayOriginView.z) / rayDirView.z;
      if (tWorld > 0.0)
      {
        // Convert from world space parameter to model space parameter
        return tWorld / worldDirLength;
      }
    }
  }

  return 1e10;
}

/*
 * Checks if the ray intersects the bounding box/sphere and triggers
 * the ray marching method of choice.
 */
vec4 rayMarch(in Ray rayModel)
{
#if defined(SHOW_AXES)
  vec2 screenCoord = (fragPosition + 1.0) * 0.5;
  vec4 dstColor = texture(uColorTexture, screenCoord);
#else // SHOW_AXES
  vec4 dstColor = vec4(0.0);
#endif // SHOW_AXES

  float tStart, tEnd;
#if defined(USE_BOUNDING_BOX)
  if (!intersectAABB(rayModel, tStart, tEnd))
#else // USE_BOUNDING_BOX
  if (!intersectSphere(rayModel, tStart, tEnd))
#endif // USE_BOUNDING_BOX
  {
    gl_FragDepth = 1.0;
    return dstColor;
  }

#if defined(SHOW_AXES)
  // Limit ray based on depth buffer
  float dstDepth = texture(uDepthTexture, screenCoord).r;
  float maxT = getMaxRayParamFromDepth(dstDepth, screenCoord, rayModel);
  tEnd = min(tEnd, maxT);

  // If the mesh is in front of the bounding volume start, skip rendering
  if (tEnd < tStart)
  {
    gl_FragDepth = 1.0;
    return dstColor;
  }
#endif // SHOW_AXES

  float tHit;
  bool inside;
  vec4 srcColor;

#if defined(SHOW_ISOSURFACE)
#if defined(USE_ADAPTIVE_RAY_MARCH)
  if (adaptiveMarch(rayModel, tStart, tEnd, tHit, inside))
  {
#else // USE_ADAPTIVE_RAY_MARCH
  if (fixedMarch(rayModel, tStart, tEnd, tHit, inside))
  {
#endif // USE_ADAPTIVE_RAY_MARCH
    vec3 PModel = rayModel.origin + rayModel.direction * tHit;
    vec3 PWorld = (uCamera.modelMatrix * vec4(PModel, 1.0)).xyz;
    vec3 PView = (uCamera.viewMatrix * vec4(PWorld, 1.0)).xyz;
    vec3 NModel = evalGradient(PModel);

    outData1 = vec4(PModel, 1.0);
#if defined(SHOW_NORMAL_VECTOR) || defined(SHOW_NORMAL_MAGNITUDE)
    vec3 outData2Normal = normalize(NModel);
#if defined(INWARD_NORMALS)
    outData2Normal *= inside ? -1.0 : 1.0;
#endif // INWARD_NORMALS
    outData2 = vec4(outData2Normal, length(NModel));
#endif // SHOW_NORMAL_VECTOR || SHOW_NORMAL_MAGNITUDE

    srcColor = shade(PView, PModel, NModel, inside);

#if defined(USE_FOG)
    float worldRadius = kBoundRadius * uCamera.maxModelScale;
    float distToCenter = length(uCamera.eye);
    float fogMinDist = distToCenter - worldRadius;
    float fogMaxDist = distToCenter + worldRadius * 3.0;
    float d = length(PWorld - uCamera.eye);
    float t = clamp((d - fogMinDist) / (fogMaxDist - fogMinDist), 0.0, 1.0);

    srcColor.a = 1.0 - t * t; // Quadratic falloff
#endif // USE_FOG
    dstColor.a = 0.0;

    vec4 PClip = uCamera.projMatrix * vec4(PView, 1.0);
    float depthNDC = PClip.z / PClip.w;
#if defined(SHOW_AXES)
    gl_FragDepth = min(dstDepth, (depthNDC + 1.0) * 0.5);
#else // SHOW_AXES
    gl_FragDepth = (depthNDC + 1.0) * 0.5;
#endif // SHOW_AXES
  }
  else
  {
#if defined(SHOW_AXES)
    gl_FragDepth = dstDepth;
#else // SHOW_AXES
    gl_FragDepth = 1.0;
#endif // SHOW_AXES
    return dstColor;
  }
#else
  // Direct volume rendering
  srcColor = dvrMarch(rayModel, tStart, tEnd);
#if defined(SHOW_AXES)
    gl_FragDepth = dstDepth;
#else // SHOW_AXES
    gl_FragDepth = 1.0;
#endif // SHOW_AXES
#endif // SHOW_ISOSURFACE

  // Blend using premultiplied alpha
  float oneMinusSrcA = 1.0 - srcColor.a;
  vec3 outRGB = (srcColor.rgb * srcColor.a) + oneMinusSrcA * (dstColor.rgb * dstColor.a);
  float outA = srcColor.a + oneMinusSrcA * dstColor.a;
  dstColor = vec4(outRGB, outA);

  return dstColor;
}

/*
 * Accumulates ray march samples for MSAA and returns the averaged premultiplied RGBA.
 */
vec4 rayMarchMSAA()
{
  vec4 accumColor = vec4(0);

#if defined(MSAA_2X)
  const int sampleCount = 2;
  for (int i = 0; i < sampleCount; i++)
  {
    accumColor += rayMarch(generatePrimaryRay(kMSAAPattern2x[i]));
  }
#else // MSAA_2X
#if defined(MSAA_4X)
  const int sampleCount = 4;
  for (int i = 0; i < sampleCount; i++)
  {
    accumColor += rayMarch(generatePrimaryRay(kMSAAPattern4x[i]));
  }
#else // MSAA_4X
#if defined(MSAA_8X)
  const int sampleCount = 8;
  for (int i = 0; i < sampleCount; i++)
  {
    accumColor += rayMarch(generatePrimaryRay(kMSAAPattern8x[i]));
  }
#else // MSAA_8X
  // Use 8x pattern twice with slight rotation
  const int sampleCount = 16;
  for (int i = 0; i < 8; i++)
  {
    accumColor += rayMarch(generatePrimaryRay(kMSAAPattern8x[i]));
    accumColor += rayMarch(generatePrimaryRay(kMSAAPattern8x[i] * 0.7));
  }
#endif // MSAA_8X
#endif // MSAA_4X
#endif // MSAA_2X

  return accumColor / float(sampleCount);
}

void main()
{
  outData1 = vec4(0.0);
  outData2 = vec4(0.0);
#if defined(MSAA_ENABLED)
  outColor = rayMarchMSAA();
#else // MSAA_ENABLED
  outColor = rayMarch(generatePrimaryRay(vec2(0)));
#endif // MSAA_ENABLED
}