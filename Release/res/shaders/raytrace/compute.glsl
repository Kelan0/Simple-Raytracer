#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray01;
uniform vec3 ray10;
uniform vec3 ray11;

struct intersection
{
	vec3 position;
	vec3 normal;
	float distance;
	bool valid;
};

struct hitinfo
{
	intersection lambda;
	int bi;
	vec4 colour;
};

struct box
{
	vec3 min;
	vec3 max;
	vec4 colour;
};

struct sphere
{
	float radius;
	vec3 origin;
	vec4 colour;
};

struct ray
{
	vec3 origin;
	vec3 direction;
};

#define MAX_SCENE_BOUNDS 100.0
#define NUM_BOXES 2
#define NUM_SPHERES 2

const box boxes[] = {
	/* The ground */
	{vec3(-5.0, -0.1, -5.0), vec3(5.0, 0.0, 5.0), vec4(0.8F, 0.8F, 0.8F, 1.0F)},
	/* Box in the middle */
	{vec3(-0.5, 0.0, -0.5), vec3(0.5, 1.0, 0.5), vec4(0.5F, 1.0F, 0.5F, 1.0F)}
};

const sphere spheres[] = {
	{0.8, vec3(4.0, 2.0, -3.0), vec4(0.65, 0.0, 1.0, 1.0)},
	
	{0.4, vec3(3.0, 1.3, -2.0), vec4(0.1, 0.7, 0.2, 1.0)}
};

float discriminat(float a, float b, float c)
{
	return b * b - 4.0 * a * c;
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
			normal = vec3 (localPoint.x <= 0.0 ? 1.0 : -1.0, 0.0, 0.0);
		}
		
		distance = abs(size.y - abs(localPoint.y));
		if (distance < min)
		{
			min = distance;
			normal = vec3(0.0, localPoint.y <= 0.0 ? 1.0 : -1.0, 0.0);
		}
		
		distance = abs(size.z - abs(localPoint.z));
		if (distance < min)
		{ 
			min = distance; 
			normal = vec3(0.0, 0.0, localPoint.z <= 0.0 ? 1.0 : -1.0);
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

intersection intersect(ray r, sphere s)
{
	vec3 dir = r.direction; 	//direction
	vec3 eye = r.origin;		//eye
	vec3 ctr = s.origin;		//centre
	float rad = s.radius;		//radius
	
	float a = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
	float b = 2.0 * dir.x * (eye.x - ctr.x) + 2.0 * dir.y * (eye.y - ctr.y) + 2.0 * dir.z * (eye.z - ctr.z);
	float c = ctr.x * ctr.x + ctr.y * ctr.y + ctr.z * ctr.z + eye.x * eye.x + eye.y * eye.y + eye.z * eye.z - 2.0 * (ctr.x * eye.x + ctr.y * eye.y + ctr.z * eye.z) - rad * rad;
	
	float dis = discriminat(a, b, c);
	
	intersection i;
	
	float t = (-b - sqrt(dis)) / (2.0 * a);
	
	i.distance = t;
	i.position = eye + t * dir;
	i.normal = normalize(i.position - ctr);
	i.valid = t > 0.0;
	
	return i;
}

bool findIntersection(int reflection, ray r, out hitinfo info)
{
	float smallest = MAX_SCENE_BOUNDS;
	bool found = false;
	
	info.colour = vec4(0.0);
	for (int i = 0; i < NUM_BOXES; i++)
	{
		box b = boxes[i];
		intersection lambda = intersect(r, b);
		
		if (lambda.valid && lambda.distance < smallest)
		{
			info.lambda = lambda;
			info.bi = i;
			info.colour = b.colour;
			smallest = lambda.distance;
			found = true;
		}
	}
	
	for (int i = 0; i < NUM_SPHERES; i++)
	{
		sphere s = spheres[i];
		intersection lambda = intersect(r, s);
		
		if (lambda.valid && lambda.distance < smallest)
		{
			info.lambda = lambda;
			info.bi = i;
			info.colour = s.colour;
			smallest = lambda.distance;
			found = true;
		}
	}
	
	return found;
}

vec4 trace(vec3 eye, vec3 dir)
{
	ray r;
	
	r.origin = eye;
	r.direction = dir;
	
	vec4 finalColour = vec4(0.0);
	
	for (int i = 0; i < 10; i++)
	{
		hitinfo info;
		if (findIntersection(i, r, info))
		{
			r.origin = info.lambda.position;
			r.direction = normalize(reflect(r.direction, info.lambda.normal));
			finalColour += info.colour / (i + 1) * 1.8;
			finalColour *= info.colour;
		}
	}
	
	return finalColour;
}

layout (local_size_x = 16, local_size_y = 16) in;
void main(void)
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pix.x >= size.x || pix.y >= size.y)
	{
		return;
	}
	
	vec2 pos = vec2(pix) / vec2(size.x - 1, size.y - 1);
	vec3 dir = normalize(mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x));
	vec4 colour = trace(eye, dir);
	
	imageStore(framebuffer, pix, colour);
}