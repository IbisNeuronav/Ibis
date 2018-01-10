uniform sampler2DRect mainTexture;
uniform bool UseTransparency;
uniform bool UseGradient;
uniform bool ShowMask;
uniform vec2 TransparencyPosition;
uniform vec2 TransparencyRadius;
uniform vec2 ImageOffset;
uniform vec2 ImageCenter;
uniform float LensDistortion;
uniform float GlobalOpacity;
uniform float Saturation;
uniform float Brightness;

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

void main()
{
    vec2 offsetTexCoord = gl_TexCoord[0].st + ImageOffset;
    vec2 dist = offsetTexCoord - ImageCenter;
    float r2 = dist.x * dist.x + dist.y * dist.y;
    vec2 distortionOffset = dist * LensDistortion * r2;
    vec2 newTexCoord = offsetTexCoord + distortionOffset;
    float factor = 0.0;
    if( UseTransparency )
    {
        float dist = length( gl_TexCoord[0].st - TransparencyPosition );
        if( dist < TransparencyRadius.y )
        {
            if( dist > TransparencyRadius.x )
            {
                float ratio = ( dist - TransparencyRadius.x ) / ( TransparencyRadius.y - TransparencyRadius.x );
                factor = exp(-(ratio*ratio)/0.25);
            }
            else
                factor = 1.0;
        }
    }
    vec4 origColor = texture2DRect( mainTexture, newTexCoord );
    vec3 hsv = rgb2hsv( origColor.rgb );
    hsv.y *= Saturation;
    hsv.z *= Brightness;
    vec3 modColor = hsv2rgb( hsv );
    gl_FragColor.rgb = modColor;
    gl_FragColor.a = 1.0;
    float alpha = 1.0;
    if( UseGradient )
    {
        if( factor > 0.01 )
        {
            vec4 gx = texture2DRect( mainTexture, newTexCoord + vec2(-1.0,-1.0) );
            gx -= texture2DRect( mainTexture, newTexCoord + vec2(1.0,1.0) );
            gx += 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(-1.0,0.0) );
            gx -= 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(1.0,0.0) );
            gx += texture2DRect( mainTexture, newTexCoord + vec2(-1.0,1.0) );
            gx -= texture2DRect( mainTexture, newTexCoord + vec2(1.0,-1.0) );
            vec4 gy = texture2DRect( mainTexture, newTexCoord + vec2(-1.0,1.0) );
            gy -= texture2DRect( mainTexture, newTexCoord + vec2(-1.0,-1.0) );
            gy += 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(0.0,1.0) );
            gy -= 2.0 * texture2DRect( mainTexture, newTexCoord + vec2(0.0,-1.0) );
            gy += texture2DRect( mainTexture, newTexCoord + vec2(1.0,1.0) );
            gy -= texture2DRect( mainTexture, newTexCoord + vec2(1.0,-1.0) );
            vec2 gradAll = vec2( length( gx ), length( gy ) );
            alpha = ( 1.0 - factor ) + factor * length( gradAll );
        }
    }
    else
    {
        alpha = 1.0 - factor;
    }
    if( ShowMask )
    {
        gl_FragColor.rgb = vec3( alpha, alpha, alpha );
        gl_FragColor.a = 1.0;
    }
    else
    {
        gl_FragColor.a = min( alpha, GlobalOpacity );
    }
}
