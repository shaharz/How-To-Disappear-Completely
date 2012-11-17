// define our rectangular texture samplers 
uniform sampler2D tex0;
uniform sampler2D tex1;	// bars and distortion texture
uniform vec2 texdim0;

uniform float barsamount;	// changes bars amount
uniform float distortion;	// amount of distortion
uniform int resolution;		// for scanlines
uniform float vsync;		// sync
uniform float hsync;

// float barsamount = 0.0;	// changes bars amount
// float distortion = 0.0;	// amount of distortion
// int resolution = 10;		// for scanlines
// float vsync = 0.0;		// sync
// float hsync = 0.0;

// define our varying texture coordinates 
varying vec2 texcoord0;

void main (void) 
{ 		
	vec2 point = texcoord0;
	vec2 original = point;

	vec4 bars = texture2D(tex1, texcoord0);

	// scanlines
	vec4 stripes; // our base 'stripe color'
	stripes = vec4(floor(mod(texcoord0.y, float(resolution)+0.0001)));
	stripes = clamp(stripes, 0.0, 1.0);
	stripes = 2.0 - (stripes - vec4(0.)); // subtract

	// get rough luma 
	vec4 key = texture2D(tex0, (vec2(point.y, point.y)) );
	key += texture2D(tex0, (1.0 - vec2(point.y, point.y)) );
	key -= bars.r;
	//key *= 0.33;
	float d = key.r + key.g + key.b;
	d /= 3.0;
	point.x -= d * distortion * 0.1;	

	//sync			
	vec2 texcoord = point + ( mod(vec2(hsync, vsync), 1.0)); 
	
	// wrap
	texcoord = mod(texcoord, 1.0);
	
	// outout
	vec4 result = stripes * texture2D(tex0, texcoord);
	gl_FragColor = mix(result, bars*result, barsamount);
	//gl_FragColor.r = 1.0;
} 


