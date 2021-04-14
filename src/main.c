#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fastmath.h>
#include "gd32vf103.h"
#include "spi0.h"
#include "lcd.h"
#include "led.h"
#include "sd.h"
#include "fat.h"
#include "usart.h"
#include "extram.h"
#include "types.h"
#include "systick.h"

#define DEFAULT

#ifdef DEFAULT
int main(void)
{
	u32 br;
	u8 ret, buf[512], b2[512];

	led_init();
	spi0_init();
	lcd_init();
	extram_init();
	usart_init(115200);

	lcd_clear(LCD_BLACK);
	if((ret = sd_init()))
	{
		lcd_string(2, 2, LCD_WHITE, LCD_BLACK, "Error initializing SD/MMC card");
		goto fail;
	}

	if((ret = fat_mount()))
	{
		lcd_string(2, 2, LCD_WHITE, LCD_BLACK, "Error mounting filesystem");
		goto fail;
	}

	if((ret = fat_fopen("TEST.TXT")))
	{
		lcd_string(2, 2, LCD_WHITE, LCD_BLACK, "Error opening TEST.TXT");
		goto fail;
	}

	br = 0;
	if((ret = fat_fread(buf, 512, &br)))
	{
		lcd_string(2, 2, LCD_WHITE, LCD_BLACK, "Error reading from TEST.TXT");
		goto fail;
	}

	*strchr((char *)buf, '\n') = '\0';
	lcd_string(2, 2, LCD_RED, LCD_BLACK, (char *)buf);
	for(br = 0; buf[br]; ++br)
	{
		usart_tx(buf[br]);
	}

	extram_write(0, buf, br);
	delay_ms(100);
	extram_read(0, b2, br);

	lcd_string(2, 14, LCD_YELLOW, LCD_BLACK, (char *)b2);

fail:
	for(;;)
	{
	}

	return 0;
}
#endif

#ifdef FIRE

u16 palette[] = 
{
	0x0020,
	0x1820,
	0x2860,
	0x4060,
	0x50a0,
	0x60e0,
	0x70e0,
	0x8920,
	0x9960,
	0xa9e0,
	0xba20,
	0xc220,
	0xda20,
	0xdaa0,
	0xdaa0,
	0xd2e0,
	0xd2e0,
	0xd321,
	0xcb61,
	0xcba1,
	0xcbe1,
	0xcc22,
	0xc422,
	0xc462,
	0xc4a3,
	0xbce3,
	0xbce3,
	0xbd24,
	0xbd24,
	0xbd65,
	0xb565,
	0xb5a5,
	0xb5a6,
	0xce6d,
	0xdef3,
	0xef78,
	0xffff
};

#define arrlen(a) (sizeof(a) / sizeof(*a))

int main(void)
{
	lcd_init();
	lcd_clear(LCD_BLACK);

	u32 i, rnd;
	u8 fire[LCD_W * LCD_H];
	for(i = 0; i < LCD_W * (LCD_H - 1); ++i)
	{
		fire[i] = 0;
	}

	for(; i < LCD_W * LCD_H; ++i)
	{
		fire[i] = arrlen(palette) - 1;
	}

	for(;;)
	{
		for(i = LCD_W; i < LCD_W * LCD_H; ++i)
		{
			if(fire[i] == 0)
			{
				fire[i - LCD_W] = fire[i];
			}
			else
			{
				rnd = rand() % 3;
				fire[(i - rnd + 1) - LCD_W] = fire[i] - (rnd & 1);
			}
		}   

		lcd_address_set(0, 0, LCD_W - 1, LCD_H - 1);    
		for(i = 0; i < LCD_W * LCD_H; ++i)
		{
			lcd_write_data_16(palette[fire[i]]);
		}
	}

	return 0;
}
#endif

/* raytracing test */
#ifdef RAYTRACING
#include <math.h>
#include <float.h>

typedef struct
{
	union
	{
		float v[3];
		struct
		{
			float x, y, z;
		};
	};
}
vec3_t;

typedef struct
{
	vec3_t org, dir;
}
ray_t;

typedef struct
{
	vec3_t org, dir, right, up;
}
cam_t;

typedef struct
{
	u8 r : 5;
	u8 g : 6;
	u8 b : 5;
}
rgb_t;

typedef struct
{
	float t;
	vec3_t n;
}
hit_t;

static inline float sq(float t)
{
	return t * t;
}

static inline vec3_t vec3(float x, float y, float z)
{
	vec3_t v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

static inline vec3_t vec3_add(vec3_t a, vec3_t b)
{
	vec3_t v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	v.z = a.z + b.z;
	return v;
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b)
{
	vec3_t v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

static inline vec3_t vec3_mul(vec3_t a, vec3_t b)
{
	vec3_t v;
	v.x = a.x * b.x;
	v.y = a.y * b.y;
	v.z = a.z * b.z;
	return v;
}

static inline vec3_t vec3_mulf(vec3_t a, float s)
{
	vec3_t v;
	v.x = a.x * s;
	v.y = a.y * s;
	v.z = a.z * s;
	return v;
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b)
{
	vec3_t v;
	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	return v;
}

static inline float vec3_dot(vec3_t a, vec3_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3_t vec3_norm(vec3_t a)
{
	float inv = 1.0f / sqrtf(vec3_dot(a, a));
	return vec3_mulf(a, inv);
}

static inline vec3_t vec3_neg(vec3_t a)
{
	vec3_t v;
	v.x = -a.x;
	v.y = -a.y;
	v.z = -a.z;
	return v;
}

static inline rgb_t rgb(u8 r, u8 g, u8 b)
{
	rgb_t c;
	c.r = r;
	c.g = g;
	c.b = b;
	return c;
}

static inline hit_t hit(float t, vec3_t n)
{
	hit_t h = { t, n };
	return h;
}

static void intersect_sphere
		(const ray_t *ray, hit_t *h, float tmin, vec3_t center, float sq_radius)
{
	vec3_t oc  = vec3_sub(ray->org, center);
	float a = vec3_dot(ray->dir, ray->dir);
	float b = 2.0f * vec3_dot(ray->dir, oc);
	float c = vec3_dot(oc, oc) - sq_radius;
	float delta = b * b - 4.0f * a * c;
	if(delta >= 0)
	{
		float inv = 0.5f / a;
		float t0 = -(b + sqrtf(delta)) * inv;
		float t1 = -(b - sqrtf(delta)) * inv;
		float t = fminf(t0 > tmin ? t0 : t1, t1 > tmin ? t1 : t0);
		if(t > tmin && t < h->t)
		{
			vec3_t p = vec3_add(vec3_mulf(ray->dir, t), ray->org);
			h->t = t;
			h->n = vec3_norm(vec3_sub(p, center));
		}
	}
}

static void intersect_cylinder
		(const ray_t *ray, hit_t *h, float tmin, vec3_t center, vec3_t dir, float sq_radius, float height)
{
	vec3_t oc = vec3_sub(ray->org, center);
	float d_oc = vec3_dot(dir, oc);
	float d_rd = vec3_dot(dir, ray->dir);
	float a = vec3_dot(ray->dir, ray->dir) - d_rd * d_rd;
	float b = 2.0f * (vec3_dot(ray->dir, oc) - d_oc * d_rd);
	float c = vec3_dot(oc, oc) - d_oc * d_oc - sq_radius;
	float delta = b * b - 4.0f * a * c;
	if(delta >= 0)
	{
		float inv = 0.5f / a;
		float t0 = -(b + sqrtf(delta)) * inv;
		float t1 = -(b - sqrtf(delta)) * inv;
		float t = fminf(t0 > tmin ? t0 : t1, t1 > tmin ? t1 : t0);
		if(t > tmin && t < h->t)
		{
			vec3_t p = vec3_add(vec3_mulf(ray->dir, t), ray->org);
			vec3_t q = vec3_add(center, vec3_mulf(dir, vec3_dot(dir, vec3_sub(p, center))));
			if(fabsf(vec3_dot(vec3_sub(p, center), dir)) < height)
			{
				h->t = t;
				h->n = vec3_sub(p, q);
			}
			else
			{
				float inv = (float)1.0 / d_rd;
				float t0 = (height - d_oc) * inv;
#ifdef LOWER_CAP
				float t1 = (- height - d_oc) * inv;
				float t = fminf(t0 > tmin ? t0 : t1, t1 > tmin ? t1 : t0);
#endif
				float t = t0;
				vec3_t r = vec3_sub(vec3_add(vec3_mulf(ray->dir, t), ray->org), center);
				if(vec3_dot(r, r) - sq(vec3_dot(r, dir)) <= sq_radius)
				{
					h->t = t;
					h->n = t == t0 ? dir : vec3_neg(dir);
				}
			}
		}
	}
}

static void intersect_box
		(const ray_t *ray, hit_t *h, float tmin, vec3_t min, vec3_t max)
{
	float t0 = tmin, t1 = h->t;
	u8 axis = 0;
	for(u8 i = 0; i < 3; ++i)
	{
		float tmin = (min.v[i] - ray->org.v[i]) / ray->dir.v[i];
		float tmax = (max.v[i] - ray->org.v[i]) / ray->dir.v[i];
		float tx = fminf(tmin, tmax);
		if(t0 < tx)
		{
			t0 = tx;
			axis = i;
		}

		t1 = fminf(t1, fmaxf(tmin, tmax));
	}

	if(t0 < t1)
	{
		h->t = t0;
		h->n = vec3(0, 0, 0);
		h->n.v[axis] = ray->dir.v[axis] > 0 ? -1 : 1;
	}
}

static rgb_t trace(float t, const ray_t *ray)
{
	hit_t h = hit(FLT_MAX, vec3(0, 1, 0));
	intersect_box(ray, &h, 0, vec3(-2, -0.5, -4), vec3(2, 1, 4));
	for(u8 x = 0; x < 2; ++x)
	{
		for(u8 y = 0; y < 4; ++y)
		{
			intersect_cylinder(ray, &h, 0, vec3(-1 + x * 2, 1, -3 + y * 2), vec3(0, 1, 0), 0.5, 0.5);
		}
	}

	if(h.t < FLT_MAX)
	{
		float kd = fminf(1, -vec3_dot(h.n, vec3_norm(ray->dir)));
		t = t - (i16)t;
		t = t < 0 ? -t : t;
		float k1 = fabsf((1.0f - t) * (0.5f - t));
		float k2 = fabsf((0.0f - t) * (1.0f - t));
		float k3 = fabsf((0.5f - t) * (0.0f - t));
		float inv = (float)3.0f / (k1 + k2 + k3);
		float r = inv * ((k1 + k3) * 0.9 + k2 * 0.5);
		float g = inv * ((k1 + k3) * 0.1 + k2 * 0.8);
		float b = inv * ((k1 + k3) * 0.4 + k2 * 0.7);
		return rgb(
				(r * kd) * ((1 << 3) - 1),
				(g * kd) * ((1 << 4) - 1),
				(b * kd) * ((1 << 3) - 1));
	}

	return rgb(255, 255, 255);
}

static inline cam_t gen_cam(float t)
{
	const float r = 6;
	vec3_t eye = { r * cosf(t), 3, r * sinf(t) };
	vec3_t dir = { -eye.x, -3, -eye.z };
	vec3_t up  = { 0, 1, 0 };
	const float ratio = 0.5; // height / width
	const float w = 1.0, h = w * ratio; // 1.6 ~= fov of 90 deg
	vec3_t right = vec3_norm(vec3_cross(dir, up));
	up = vec3_norm(vec3_cross(right, dir));
	dir = vec3_norm(dir);
	right = vec3_mulf(right, w);
	up = vec3_mulf(up, h);
	cam_t cam = { eye, dir, right, up };
	return cam;
}

static inline ray_t gen_ray(cam_t cam, float x, float y)
{
	ray_t ray;
	ray.org = cam.org;
	ray.dir = vec3_add(cam.dir, vec3_add(vec3_mulf(cam.right, 2 * x - 1), vec3_mulf(cam.up, 1 - 2 * y)));
	return ray;
}

int main(void)
{
	u16 frame[LCD_W * LCD_H];
	lcd_init();
	lcd_clear(LCD_WHITE);
	float t = 0;
	for(;;)
	{
		cam_t cam = gen_cam(t);
		for(u8 y = 0; y < LCD_H; ++y)
		{
			for(u8 x = 0; x < LCD_W; ++x)
			{
				ray_t ray = gen_ray(cam, (float)x / (float)LCD_W, (float)y / (float)LCD_H);
				rgb_t color = trace(t, &ray);
				u16 data = color.b | (color.g << 5) | (color.r << 11);
				frame[y * LCD_W + x] = data;
			}
		}

		lcd_address_set(0, 0, LCD_W - 1, LCD_H - 1);
		for(u32 i = 0; i < LCD_W * LCD_H; ++i)
		{
			lcd_write_data_16(frame[i]);
		}

		t += (float)0.1;
	}
}
#endif
