#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;
layout (std430, binding = 1) buffer outData
{
	float data[];
};

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray01;
uniform vec3 ray10;
uniform vec3 ray11;
uniform float time;
uniform float rndNum;
uniform float blendFactor;

float numRays = 0.0;
float numSSAO = 0.0;

#define PI 3.1415926535897932384626433832795
#define MAX_SCENE_BOUNDS 10000.0
#define NUM_BOXES 7
#define NUM_SPHERES 8
#define NUM_LIGHTS 1
#define EPSILON 0.01

struct intersection
{
	vec3 position;
	vec3 normal;
	float distance;
	bool inside;
	bool valid;
};

struct hitinfo
{
	intersection lambda;
	int bi;
	vec4 colour;
	float reflectivity;
	float refractivity;
	float specularIntensity;
	float specularPower;
};

struct box
{
	float reflectivity;
	float refractivity;
	vec3 min;
	vec3 max;
	vec4 colour;
};

struct sphere
{
	float reflectivity;
	float refractivity;
	float specularIntensity;
	float specularPower;
	float radius;
	float luminosity;
	vec3 origin;
	vec4 colour;
};

struct ray
{
	vec3 origin;
	vec3 direction;
	vec3 colour;
	float strength;
	int curDepth;
};

const int lights[] = {
	7, -1
};

float roomSize = 5.0;
float roomHeight = 5.0;
float wallThickness = 1.0;

const box boxes[] = {
	{0.15, 0.0, vec3(-roomSize, -wallThickness, -roomSize), vec3(+roomSize, roomHeight, -roomSize - wallThickness), vec4(1.0F, 1.0F, 1.0F, 1.0F)},
	{0.15, 0.0, vec3(-roomSize, -wallThickness, +roomSize), vec3(+roomSize, roomHeight, +roomSize + wallThickness), vec4(1.0F, 1.0F, 1.0F, 1.0F)},
	{1.00, 0.0, vec3(+roomSize, -wallThickness, -roomSize), vec3(+roomSize + wallThickness, roomHeight, +roomSize), vec4(0.0F, 0.0F, 0.0F, 1.0F)},
	{0.00, 0.0, vec3(-roomSize, -wallThickness, -roomSize), vec3(-roomSize - wallThickness, roomHeight, +roomSize), vec4(0.2F, 0.1F, 1.0F, 1.0F)},

	{0.1, 0.0, vec3(-0.5, 0.0, -0.5), vec3(0.5, 1.0, 0.5), vec4(0.5F, 1.0F, 0.5F, 1.0F)},
	{0.0, 0.0, vec3(-roomSize, -wallThickness, -roomSize), vec3(roomSize, 0.0, roomSize), vec4(0.8F, 0.8F, 0.8F, 1.0F)},
	{0.0, 0.0, vec3(-roomSize, roomHeight + wallThickness, -roomSize), vec3(roomSize, roomHeight, roomSize), vec4(0.8F, 0.8F, 0.8F, 1.0F)},
};

const sphere spheres[] = {
	{0.7, 0.0, 1.0, 32.0, 0.8, 0.0, vec3(3.0, 2.0, -3.0), vec4(0.65, 0.0, 1.0, 1.0)},
	{0.3, 0.0, 0.0, 0.0, 0.4, 0.0, vec3(-2.0, 1.3, -2.0), vec4(0.1, 0.7, 0.2, 0.25)},
	{1.0, 1.9, 0.0, 0.0, 0.9, 0.0, vec3(2.0, 0.9, 3.0), vec4(0.0, 0.0, 0.0, 1.0)},
	{0.0, 0.0, 0.0, 0.0, 0.2, 0.0, vec3(0.0, 0.2, 4.0), vec4(1.0, 0.8, 0.1, 1.0)},
	{0.0, 0.0, 0.0, 0.0, 0.5, 0.0, vec3(-1.0, 0.5, 2.0), vec4(0.1, 0.4, 0.9, 1.0)},
	{0.0, 0.0, 0.0, 0.0, 0.5, 0.0, vec3(-2.0, 0.5, 2.5), vec4(0.7, 0.4, 0.9, 1.0)},
	{0.0, 0.0, 0.0, 0.0, 0.5, 0.0, vec3(1.0, 0.5, 0.14), vec4(1.0, 1.0, 1.0, 1.0)},
	{0.0, 0.0, 0.0, 0.0, 0.15, 10.0, vec3(+1.5, +3.1, -1.2), vec4(1.0, 1.0, 1.0, 1.0)},
};

float seed = 0.0;

int nextPowerOfTwo(int i)
{
	i--;
	i |= i >> 1; 
	i |= i >> 2; 
	i |= i >> 4; 
	i |= i >> 8; 
	i |= i >> 16;

	i++;
	return i;
}

uint hash(uint x)
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
	
    return x;
}

float rand1(float f)
{
	//f *= rndNum;
    const uint mantissaMask = 0x007FFFFFu;
    const uint one = 0x3F800000u;
    
    uint h = hash(floatBitsToUint(f));
    h &= mantissaMask;
    h |= one;
    
    float  r2 = uintBitsToFloat(h);
    return r2 - 1.0;
}

float rand2(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float discriminat(float a, float b, float c)
{
	return b * b - 4.0 * a * c;
}

vec3 randHemisphereVector(vec3 normal, int i)
{
	vec3 v = normalize(vec3(rand1(time * i *  seed++) - 0.5, rand1(time * i *  seed++) - 0.5, rand1(time * i *  seed++) - 0.5));
		
	if (dot(normal, v) < 0.0)
	{
		v = -v;
	}
	
	return v;
}

intersection intersect(ray r, box b)
{
	vec3 tMin = (b.min - r.origin) / r.direction;
	vec3 tMax = (b.max - r.origin) / r.direction;
	vec3 t1 = min(tMin, tMax);
	vec3 t2 = max(tMin, tMax);
	
	intersection i;
	
	float near = max(max(t1.x, t1.y), t1.z);
	float far = min(min(t2.x, t2.y), t2.z);
	
	bool valid = near > 0.0 && near < far;
	
	if (valid)
	{
		vec3 pos = r.origin + near * normalize(r.direction);
		vec3 c = (b.min + b.max) / 2.0;
		vec3 size = b.max - b.min;
		
		vec3 normal;
		vec3 localPoint = pos - c;
		float min = 2147483647.0;
		
		float distance = abs(size.x - abs(localPoint.x));
		if (distance < min)
		{
			min = distance;
			normal = vec3 (localPoint.x > 0.0 ? 1.0 : -1.0, 0.0, 0.0);
		}
		
		distance = abs(size.y - abs(localPoint.y));
		if (distance < min)
		{
			min = distance;
			normal = vec3(0.0, localPoint.y > 0.0 ? 1.0 : -1.0, 0.0);
		}
		
		distance = abs(size.z - abs(localPoint.z));
		if (distance < min)
		{ 
			min = distance; 
			normal = vec3(0.0, 0.0, localPoint.z > 0.0 ? 1.0 : -1.0);
		}
		
		i.distance = near;
		i.position = pos;
		i.normal = normal;
	} else
	{
		i.distance = -1.0;
	}
	
	i.valid = valid;
	
	return i;
}

intersection intersect(ray r, sphere s, bool cullface)
{
	bool valid = true;
	vec3 l = s.origin - r.origin; 
    float tca = dot(l, r.direction); 
	
    if (tca < 0.0)
	{
		valid = false; 
	}
	
    float d2 = dot(l, l) - tca * tca; 
    if (d2 > s.radius * s.radius)
	{
		valid = false;
	}
	
    float thc = sqrt(s.radius * s.radius - d2); 
    float t0 = tca - thc; 
    float t1 = tca + thc; 
	
	bool inside = distance(r.origin, s.origin) < s.radius;
	float t = (inside ? t1 : t0);
	vec3 position = r.origin + r.direction * t;
	vec3 normal = inside ? normalize(s.origin - position) : normalize(position - s.origin);
	
	intersection i;
	
	i.valid = valid;
	i.inside = inside;
	i.position = position;
	i.normal = normal;
	i.distance = t;
	
	return i;
}

bool castRay(ray r, out hitinfo info)
{
	float smallest = MAX_SCENE_BOUNDS;
	bool found = false;
	
	intersection lambda;
	lambda.position = vec3(0.0);
	lambda.normal = vec3(0.0);
	lambda.distance = 0.0;
	lambda.inside = false;
	lambda.valid = false;
	
	info.lambda = lambda;
	info.bi = 0;
	info.colour = vec4(0.0);
	
	for (int i = 0; i < NUM_BOXES; i++)
	{
		box b = boxes[i];
		lambda = intersect(r, b);
		
		if (lambda.valid && lambda.distance < smallest)
		{
			info.lambda = lambda;
			info.bi = i;
			info.colour = b.colour;
			info.reflectivity = b.reflectivity;
			info.refractivity = b.refractivity;
			smallest = lambda.distance;
		}
	}
	
	for (int i = 0; i < NUM_SPHERES; i++)
	{
		sphere s = spheres[i];
		lambda = intersect(r, s, false);
		
		if (lambda.valid && lambda.distance < smallest)
		{
			info.lambda = lambda;
			info.bi = i;
			info.colour = s.colour;
			info.reflectivity = s.reflectivity;
			info.refractivity = s.refractivity;
			info.specularIntensity = s.specularIntensity;
			info.specularPower = s.specularPower;
			smallest = lambda.distance;
		}
	}
	
	return info.lambda.valid;
}

vec4 getGlobalIllumination(vec3 position, vec3 normal, int samples, float strength)
{
	vec4 colour = vec4(0.0);
	
	for (int i = 1; i < samples + 1; i++)
	{
		vec3 v = randHemisphereVector(normal, i);
		
		hitinfo info;
		
		if (castRay(ray(position, v, vec3(1.0), 1.0, 0), info))
		{
			//float d = info.lambda.distance;
			
			colour += (info.colour * strength) / i;
		}
	}
	
	return colour;
}

float getAmbientOcclusion(vec3 position, vec3 normal, int samples)
{
	float numOccluded = 0.0;
	float attenuation = 3;
	
	for (int i = 0; i < samples; i++)
	{
		vec3 v = randHemisphereVector(normal, i);
		
		hitinfo info;
		
		if (castRay(ray(position, v, vec3(1.0), 1.0, 0), info))
		{
			float d = info.lambda.distance;
			numOccluded += 1.0 / pow(attenuation, d);
			numSSAO++;
		}
	}
	
	return (1.0 - (numOccluded / samples));
}

vec4 getDiffuseLight(vec3 eye, vec3 position, vec3 normal)
{
	vec4 diffuse = vec4(0.0);
	
	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		sphere l = spheres[lights[i]];
		
		vec3 surfaceToLight = (l.origin + normalize(position - l.origin) * (l.radius + EPSILON)) - position;
		float dist = length(surfaceToLight);
		surfaceToLight = normalize(surfaceToLight);
	
		hitinfo info1;
		
		bool flag = castRay(ray(position, surfaceToLight, vec3(1.0), 1.0, 0), info1);
		
		if (!flag || info1.lambda.distance > dist)
		{
			float a = 0.0;
			float b = 1.0 / (l.luminosity * l.luminosity);
			float attenuation = 1.0 / (1.0 + a * dist + b * dist * dist);
			float diffuseCoefficient = dot(normal, surfaceToLight);
			
			if (diffuseCoefficient > 0.0)
			{
				float f = diffuseCoefficient * attenuation;
				diffuse += vec4(l.colour * f);
			}
		}
	}
	
	return clamp(diffuse, 0.0, 1.0);
}

vec4 getSpecularLight(vec3 eye, vec3 position, vec3 normal, vec4 materialColour, float specularIntensity, float specularPower)
{
	vec4 specular = vec4(0.0);
	eye = normalize(eye);
	
	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		sphere l = spheres[lights[i]];
		vec3 refl = normalize(reflect(position - l.origin, normal));
	
		float RdotE = dot(refl, eye);
	
		if (RdotE < 0.0)
		{
			return vec4(0.0);
		}
		
		specular += l.colour * l.luminosity * materialColour * specularIntensity * pow(RdotE, specularPower);
	}
	
	return clamp(specular, 0.0, 1.0);
}

float fresnel(vec3 I, vec3 N, float n1, float n2)
{
	I = normalize(I);
	N = normalize(-N);
	
	float R0 = pow((n1 - n2) / (n1 + n2), 2.0);
	
	float NdotI = clamp(1.0, 0.0, dot(N, I));
	
	return R0 + (1.0 - R0) * pow(1.0 - NdotI, 5.0);
}

vec4 trace(ivec2 pix, vec3 eye, vec3 dir)
{
	vec4 finalColour = vec4(0.0);
	int ssaoDepth = 8;
	int ssgiDepth = 8;
	int rayDepth = 12;
	
	float globalIlluminationStrength = 0.1;
	
	ray r = ray(eye, dir, vec3(1.0), 1.0, 0);
	hitinfo info;
	
	for (int i = 1; i < rayDepth + 1; i++) //num ray bounces
	{
		if (!castRay(r, info))
		{
			break;
		}
		
		numRays++;
		
		vec3 position = info.lambda.position;
		vec3 normal = info.lambda.normal;
		
		vec4 diffuse = getDiffuseLight(eye, position, normal);
		vec4 specular = vec4(0.0);//getSpecularLight(eye, position, normal, info.colour, info.specularIntensity, info.specularPower);
		vec4 globalIllumination = getGlobalIllumination(position, normal, ssgiDepth, globalIlluminationStrength);
		float ambientocclusion = getAmbientOcclusion(position, normal, ssaoDepth);
		float reflectivity = info.reflectivity;
		float refractivity = info.refractivity;
		
		bool doRefract = refractivity > 0.0;
		bool doReflect = reflectivity > 0.0;
		
		vec4 directLight = info.colour * diffuse * ambientocclusion + specular;
		vec4 indirectLight = globalIllumination;
		
		if (doReflect)
		{
			if (doRefract)
			{
				//reflection and refraction
				
				bool inside = info.lambda.inside;
				
				float n1 = inside ? reflectivity : 1.0;
				float n2 = inside ? 1.0 : reflectivity;
				
				float f = fresnel(r.direction, normal, n1, n2);
				vec3 bias = normal * EPSILON;
				
				if (rand1(time * i * seed++) > f)
				{	
					r = ray(inside ? position + bias : position - bias, refract(r.direction, normal, 1.0 / refractivity), vec3(1.0), r.strength, 0);
				} else
				{
					r = ray(inside ? position - bias : position + bias, reflect(r.direction, normal), vec3(1.0), r.strength, 0);
				}
				
				//this was a test to see if the fresnel equation works
				//vec4 a = vec4(1.0);
				//vec4 b = vec4(0.0);
				//finalColour = mix(a, b, f);
			} else
			{
				//only reflection
				finalColour += mix(directLight, indirectLight, globalIlluminationStrength) * r.strength;
				r = ray(position, reflect(r.direction, normal), vec3(info.colour), r.strength * reflectivity, 0);
			}
		} else
		{
			//lambert/diffuse (no reflection)
			finalColour += mix(directLight, indirectLight, globalIlluminationStrength) * r.strength;
			break;
		}
	}
	
	return finalColour;
}

const uint workgroup_size_x = 16;
const uint workgroup_size_y = 16;

layout (local_size_x = 16, local_size_y = 16) in;
void main(void)
{
	ivec2 size = imageSize(framebuffer);
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	
	ivec2 wgid = ivec2(gl_WorkGroupID.xy);
	ivec2 nwg =  ivec2(nextPowerOfTwo(size.x) / workgroup_size_x, nextPowerOfTwo(size.y) / workgroup_size_y);
	
	if (pix.x >= size.x || pix.y >= size.y)
	{
		return;
	}
	
	seed = pix.x * size.x + pix.y;
	
	vec2 pos = vec2(pix) / vec2(size.x - 1, size.y - 1);
	
	vec3 dir = normalize(mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x));
	vec4 pixel = vec4(imageLoad(framebuffer, pix).rgba);
	
	vec4 colour = mix(trace(pix, eye, dir), pixel, blendFactor);
	
	float aspect = 4.0 / 3.0;
	data[wgid.x * nwg.x + wgid.y] = (workgroup_size_x * workgroup_size_y / aspect) * numRays; //number of pixels in a workgroup * number of rays per pixel;
	
	imageStore(framebuffer, pix, colour);
}