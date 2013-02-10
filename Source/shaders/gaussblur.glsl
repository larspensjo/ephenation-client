// This algorithm is based on an algorithm by Callum Hay at 9/21/2010 11:25:00 AM
// See http://callumhay.blogspot.se/2010/09/gaussian-blur-shader-glsl.html

-- Vertex

layout (location=0) in vec2 vertex;
out vec2 screen;             // Screen coordinate
void main(void)
{
	vec4 pos = vec4(vertex*2-1, 0, 1); // Transform from interval 0 to 1, to interval -1 to 1.
	gl_Position = pos;
	screen = pos.xy/2+0.5;
}

-- Fragment

// The sigma value for the Gaussian function: higher value means more blur
// A good value for 9x9 is around 3 to 5
// A good value for 7x7 is around 2.5 to 4
// A good value for 5x5 is around 2 to 3.5
// ... play around with this based on what you need :)
uniform float Usigma = 3.0;

// Texture that will be blurred by this shader
uniform sampler2D UblurSampler;

const float pi = 3.14159265f;

uniform float UnumBlurPixelsPerSide = 4.0f;

// This uniform should either have the value 1,0 or 0,1.
uniform vec2 UblurMultiplyVec = vec2(0.0f, 1.0f);

in vec2 screen;               // The screen position

layout(location = 0) out vec3 fragColor;

void main() {
	ivec2 bitmapSize = textureSize(UblurSampler, 0);
	vec2 pixelSize = 1.0 / vec2(bitmapSize);
	// Incremental Gaussian Coefficient Calculation (See GPU Gems 3 pp. 877 - 889)
	vec3 incrementalGaussian;
	incrementalGaussian.x = 1.0f / (sqrt(2.0f * pi) * Usigma);
	incrementalGaussian.y = exp(-0.5f / (Usigma * Usigma));
	incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

	vec4 avgValue = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	float coefficientSum = 0.0f;

	// Take the central sample first.
	avgValue += texture2D(UblurSampler, screen) * incrementalGaussian.x;
	coefficientSum += incrementalGaussian.x;
	incrementalGaussian.xy *= incrementalGaussian.yz;

	vec2 offset = pixelSize * UblurMultiplyVec; // Component-wise
	for (float i = 1.0f; i <= UnumBlurPixelsPerSide; i++) {
		avgValue += texture2D(UblurSampler, screen - i*offset) * incrementalGaussian.x;
		avgValue += texture2D(UblurSampler, screen + i*offset) * incrementalGaussian.x;
		coefficientSum += 2 * incrementalGaussian.x;
		incrementalGaussian.xy *= incrementalGaussian.yz;
	}

	fragColor = avgValue.xyz / coefficientSum;
}
