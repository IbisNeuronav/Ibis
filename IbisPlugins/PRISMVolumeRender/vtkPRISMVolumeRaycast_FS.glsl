#extension GL_ARB_texture_rectangle : require

uniform sampler2DRect back_tex_id;
uniform sampler2DRect depthBuffer;
uniform float time;
uniform sampler3D volumes[@NumberOfVolumes@];
uniform sampler1D transferFunctions[@NumberOfVolumes@];
uniform int volOn[@NumberOfVolumes@];
uniform ivec2 windowSize;
uniform float stepSize;
uniform float stepSizeAdjustment;
uniform float multFactor;
uniform mat4 eyeToTexture;
uniform vec3 interactionPoint1;
uniform vec3 interactionPoint2;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform vec2 volumeDistanceRange;

// Determine if ptTest is inside the cylinder defined by pt1, pt2 and radius.
// Returns distance to the central axis of the cylinder if inside and -1.0 otherwise
float IsInsideCylinder( vec3 ptTest, vec3 pt1, vec3 pt2, float radius )
{
    vec3 d = pt2 - pt1;
    float cylLength = length( d );
    float cylLength2 = cylLength * cylLength;
    vec3 pd = ptTest - pt1;
    float dot_d_pd = dot( d, pd );
    
    if( dot_d_pd < 0.0 || dot_d_pd > cylLength2 )
        return -1.0;
    
    float dsq = dot(pd,pd) - dot_d_pd * dot_d_pd / cylLength2;
    if( dsq > radius * radius )
        return -1.0;
    
    return sqrt( dsq );
}

// Determine if ptTest is inside the sphere defined by center and radius.
// Returns distance to the center of the sphere if inside and -1.0 otherwise
float IsInsideSphere( vec3 ptTest, vec3 center, float radius )
{
    vec3 d = ptTest - center;
    float dist2 = dot(d,d);
    float radius2 = radius * radius;
    if( dist2 > radius2 )
        return -1.0;
        
    return sqrt( dist2 );
}

float SampleVolume( int volIndex, vec3 pos )
{
    vec4 volSample = texture3D( volumes[volIndex], pos );
    return volSample.x;
}

vec4 SampleVolumeWithTF( int volIndex, vec3 pos )
{
    vec4 volSample = texture3D( volumes[volIndex], pos );
    vec4 tFuncSample = texture1D( transferFunctions[volIndex], volSample.x );
    return tFuncSample;
}

vec4 SampleTF( int volIndex, float value )
{
    vec4 tFuncSample = texture1D( transferFunctions[volIndex], value );
    return tFuncSample;
}

float SampleVolumeSmooth( int volIndex, vec3 pos, float sampleSize )
{
    float volSample = 0.33333 * texture3D( volumes[volIndex], pos ).x;
    volSample += 0.11111 * texture3D( volumes[volIndex], pos +  vec3(sampleSize,0.0,0.0)  ).x;
    volSample += 0.11111 * texture3D( volumes[volIndex], pos +  vec3(0.0, sampleSize, 0.0) ).x;
    volSample += 0.11111 * texture3D( volumes[volIndex], pos +  vec3(0.0,0.0, sampleSize) ).x;
    volSample += 0.11111 * texture3D( volumes[volIndex], pos -  vec3(sampleSize,0.0,0.0) ).x;
    volSample += 0.11111 * texture3D( volumes[volIndex], pos -  vec3(0.0, sampleSize, 0.0) ).x;
    volSample += 0.11111 * texture3D( volumes[volIndex], pos -  vec3(0.0,0.0,sampleSize) ).x;
    return volSample;
}

vec4 SampleVolumeSmoothWithTF( int volIndex, vec3 pos, float sampleSize )
{
    float volSample = SampleVolumeSmooth( volIndex, pos, sampleSize );
    vec4 tFuncSample = texture1D( transferFunctions[volIndex], volSample );
    return tFuncSample;
}

vec4 ComputeGradient( int volIndex, vec3 pos, float gradStep )
{
    vec3 g1;
    g1.x = texture3D( volumes[volIndex], pos + vec3(gradStep,0.0,0.0)  ).x;
    g1.y = texture3D( volumes[volIndex], pos +  vec3(0.0, gradStep, 0.0) ).x;
    g1.z = texture3D( volumes[volIndex], pos +  vec3(0.0,0.0, gradStep) ).x;
    vec3 g2;
    g2.x = texture3D( volumes[volIndex], pos -  vec3(gradStep,0.0,0.0) ).x;
    g2.y = texture3D( volumes[volIndex], pos -  vec3(0.0, gradStep, 0.0) ).x;
    g2.z = texture3D( volumes[volIndex], pos -  vec3(0.0,0.0,gradStep) ).x;
    vec3 n = g2 - g1;
    float nLength = length( n );
    if( nLength > 0.0 )
        n = normalize( n );
    else
        n = vec3( 0.0, 0.0, 0.0 );
    vec4 ret;
    ret.rgb = n;
    ret.a = nLength;
    return ret;
}

vec3 ComputePhong( int volIndex, vec3 pos, float gradStep, vec3 lightPos, vec3 rayDir, float diffuseK, vec3 diffuseColor, float specularK, vec3 specularColor, float specularExp )
{
    vec3 n = ComputeGradient( volIndex, pos, gradStep ).rgb;
    vec3 l = normalize( lightPos - pos );
    vec3 v = -1.0 * rayDir;
    float dotNL = dot( n, l );
    vec3 phong = clamp(dotNL, 0.0, 1.0 ) * diffuseColor * diffuseK;

    if( dotNL > 0.0 )
    {
        vec3 h = normalize( l + v );
        float spectFactor = clamp( dot(n,h), 0.0, 1.0 );
        phong += specularK * pow( spectFactor, specularExp ) * specularColor;
    }
    return phong;
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float rand2D(in vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    // Basic depth test : if something is in front of the front cube, just give up now
    vec4 depthSample = texture2DRect( depthBuffer, gl_FragCoord.xy );
    if( depthSample.x < gl_FragCoord.z )
        discard;

    // Compute volume entry and exit point in GL normalized texture coordinates
    vec4 entryBack = texture2DRect( back_tex_id, gl_FragCoord.xy );
    vec4 entryFront = gl_Color;
    vec3 diff =  entryBack.rgb - entryFront.rgb;

    // Find real eye-space depth of front side of the cube
    vec4 ndcPos;  // get normalized device coordinate
    ndcPos.xy = ((gl_FragCoord.xy / vec2(windowSize) ) * 2.0) - 1.0;
    ndcPos.z = (2.0 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff;
    ndcPos.w = 1.0;
    vec4 clipPos = ndcPos / gl_FragCoord.w; // back to cartesian (from homogeneous)
    vec4 eyeSpaceCoord = gl_ProjectionMatrixInverse * clipPos;
    float depthFront = -eyeSpaceCoord.z;

    // Find the real eye-space depth of the back of the cube
    float depthBack = entryBack.a;

    // Find eye-space depth of what was in the depth buffer before volume rendering
    float percent = float(1.0);
    vec4 ndcPosDepth;
    ndcPosDepth.xy = ((gl_FragCoord.xy / vec2(windowSize) ) * 2.0) - 1.0;
    ndcPosDepth.z = (2.0 * depthSample.x - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff;
    ndcPosDepth.w = 1.0;
    vec4 eyeSpaceDepth = gl_ProjectionMatrixInverse * ndcPosDepth;
    eyeSpaceDepth /= eyeSpaceDepth.w;
    float realDepth = -eyeSpaceDepth.z;
    if( realDepth < depthBack )
    {
        percent = ( realDepth - depthFront ) / ( depthBack - depthFront );
    }

    // Compute ray properties
    vec3 rayStart = entryFront.rgb;
    float backDist = length( diff );      // total length of ray (texture coord)
    float stopDist = backDist * percent;  // where ray stops, taking into account depth buffer
    vec3 rayDir = normalize( diff );      // direction of the ray (unit vector)

    // Initialize accumulator
    float currentDistance = 0.0;  // distance from the start of the ray in texture coord units
    vec4 finalColor = vec4( 0.0, 0.0, 0.0, 0.0 );

    // Put custom initialization code here
    @ShaderInit@

    // Do the actual ray tracing
    while( currentDistance < stopDist )
    {
        // Compute current sample position
        vec3 pos = rayStart + rayDir * currentDistance;

        // Compute new sample
        vec4 sampleRGBA = vec4( 0.0, 0.0, 0.0, 0.0 );

        @VolumeContributions@

        // Adjust alpha to compensate for smaller or larger step sizes
        sampleRGBA.a = 1.0 - pow( 1.0 - sampleRGBA.a, stepSizeAdjustment );

        // merge with existing
        float oneMinusDstAlpha = 1.0 - finalColor.a;
        finalColor.rgb += oneMinusDstAlpha * sampleRGBA.a * sampleRGBA.rgb;
        finalColor.a += oneMinusDstAlpha * sampleRGBA.a;

        @StopCondition@

        currentDistance += stepSize;
    }

    gl_FragColor.rgb = finalColor.rgb * multFactor;
    gl_FragColor.a = finalColor.a;
}
