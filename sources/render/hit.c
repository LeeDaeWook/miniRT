/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hit.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: siyang <siyang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/06/01 18:43:21 by siyang            #+#    #+#             */
/*   Updated: 2023/06/20 19:09:29 by siyang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minirt.h"

void	init_hit(bool (*fp[4])(t_lst *obj, t_ray *ray, double t_max, t_hit_record *rec))
{
	fp[SP] = hit_sp;
	fp[PL] = hit_pl;
	fp[CY] = hit_cy;
	fp[CO] = hit_co;
}

bool	hit_obj(t_lst *obj, t_ray *ray, double t_max, t_hit_record *rec)
{
	bool			((*hit[4])(t_lst *, t_ray *, double, t_hit_record *));
	bool			is_hit;
	t_hit_record	temp_rec;
	double			closest_so_far;

	init_hit(hit);
	is_hit = false;
	closest_so_far = t_max;
	while (obj)
	{
		if (hit[obj->id](obj, ray, closest_so_far, &temp_rec))
		{
			is_hit = true;
			closest_so_far = temp_rec.t;
			rec->p = temp_rec.p;
			rec->normal = temp_rec.normal;
			rec->t = temp_rec.t;
			rec->front_face = temp_rec.front_face;
			rec->color = temp_rec.color;
			rec->obj = temp_rec.obj;
			rec->texture = temp_rec.texture;
		}
		obj = obj->next;
	}
	return (is_hit);
}

bool	hit_sp(t_lst *obj, t_ray *ray, double t_max, t_hit_record *rec)
{
	t_sphere	*sphere;
	double		a;
	double		b;
	double		c;

	sphere = (t_sphere *)obj;
	a = dot(ray->direction, ray->direction);
	b = 2.0 * dot(vector_sub(ray->origin, sphere->coord), ray->direction);
	c = dot(vector_sub(ray->origin, sphere->coord), vector_sub(ray->origin, sphere->coord)) - sphere->radius * sphere->radius;

	// Find the nearest root that lies in the acceptable range.
	rec->t = get_root(a, b, c, t_max);
	if (rec->t == -1)
		 return (false);
	rec->p = ray_at(ray, rec->t);
	rec->normal = scala_div(vector_sub(rec->p, sphere->coord), sphere->radius);
	if (dot(ray->direction, rec->normal) < 0.0)
		rec->front_face = true;
	else
	{
		rec->front_face = false;
		rec->normal = scala_mul(rec->normal, -1); // camera가 오브젝트 안에 있을 때 법선벡터의 방향을 바꾸는 것의 의미는?
	}
	rec->color = sphere->color;
	rec->obj = obj;
//	rec->texture = NONE;
//	rec->texture = CHECKER;
	rec->texture = BUMP;
	return (true);
}

bool	hit_pl(t_lst *obj, t_ray *ray, double t_max, t_hit_record *rec)
{
	t_plane		*plane;
	double 		denom;
	double		nom;

	plane = (t_plane*)obj;
	rec->normal = unit_vector(plane->vec);
	if (dot(rec->normal, ray->direction) > 0.0)
		rec->normal = scala_mul(rec->normal, -1);
	denom = dot(rec->normal, ray->direction);
	if (fabs(denom) > EPSILON)
	{
		nom = dot(vector_sub(plane->coord, ray->origin), rec->normal);
		rec->t = nom / denom;
		if (rec->t < T_MIN || rec->t > t_max)
			return (false);
		rec->p = ray_at(ray, rec->t);
		rec->front_face = true;
		rec->color = plane->color;
		rec->obj = obj;
//		rec->texture = NONE;
		rec->texture = CHECKER;
	}
	return (true);
}

double	hit_cy_base(t_cylinder *cy, t_ray *ray, double t_max, bool is_top)
{
	t_point3	c;
	double		r;
	double		denom;
	double		t;

	if (is_top == true)
		c = vector_add(cy->coord, scala_mul(cy->vec, cy->height / 2.0));
	else
		c = vector_sub(cy->coord, scala_mul(cy->vec, cy->height / 2.0));
	denom = dot(ray->direction, cy->vec);
	if (fabs(denom) < EPSILON)
		return (-1);
	t = dot(vector_sub(c, ray->origin), cy->vec) / denom;
	if (t < T_MIN || t > t_max)
		return (-1);
	r = length(vector_sub(ray_at(ray, t), c));
	if (r > cy->diameter / 2.0)
		return (-1);
	return (t);
}

double	hit_cy_surface(t_cylinder *cy, t_ray *ray, double t_max)
{
	double		a;
	double		b;
	double		c;
	double		discriminant;
	double		sqrtd;
	double		root;
	t_point3	p;
	t_vec3		ce;
	t_vec3		cp;

	ce = vector_sub(ray->origin, vector_sub(cy->coord, scala_mul(cy->vec, cy->height / 2.0)));
	a = dot(ray->direction, ray->direction) - pow(dot(ray->direction, cy->vec), 2.0);
	b = 2.0 * (dot(ce, ray->direction) - dot(ray->direction, cy->vec) * dot(ce, cy->vec));
	c = dot(ce, ce) - pow(dot(ce, cy->vec), 2.0) - pow((cy->diameter / 2.0), 2.0);
	
	discriminant = b * b - 4.0 * a * c;
	if (discriminant < EPSILON)
		return (-1);
	sqrtd = sqrt(discriminant);
	root = (-b - sqrtd) / (2.0 * a);
	if (root < T_MIN || root > t_max)
	{
		root = (-b + sqrtd) / (2.0 * a);
		if (root < T_MIN || root > t_max)
			return (-1);
	}
	p = ray_at(ray, root);
	cp = vector_sub(p, vector_sub(cy->coord, scala_mul(cy->vec, cy->height / 2.0)));
	if (dot(cp, cy->vec) < 0.0 || dot(cp, cy->vec) > cy->height)
		return (-1);
	return (root);
}

bool	hit_cy(t_lst *obj, t_ray *ray, double t_max, t_hit_record *rec)
{
	t_cylinder	*cylinder;
	double		surface;
	double		top;
	double		bottom;
	t_vec3		cp;

	cylinder = (t_cylinder *)obj;
	surface = hit_cy_surface(cylinder, ray, t_max);
	top = hit_cy_base(cylinder, ray, t_max, true);
	bottom = hit_cy_base(cylinder, ray, t_max, false);

	if (surface == -1 && top == -1 && bottom == -1)
		return (false);
	else if (surface == -1)
	{
		rec->t = top;
		if (top == -1)
			rec->t = bottom;
		else if (bottom != -1 && top > bottom)
			rec->t = bottom;
	}
	else
	{
		rec->t =surface;
		if (top != -1 && top < surface)
			rec->t = top;
		else if (bottom != -1 && bottom < surface)
			rec->t = bottom;
	}
	rec->p = ray_at(ray, rec->t);
	if (rec->t == surface)
	{
		cp = vector_sub(rec->p, vector_sub(cylinder->coord, scala_mul(cylinder->vec, cylinder->height / 2.0)));
		rec->normal = unit_vector(vector_sub(cp, scala_mul(cylinder->vec, dot(cp, cylinder->vec))));
	}
	else if (rec->t == top)
		rec->normal = unit_vector(cylinder->vec);
	else if (rec->t == bottom)
		rec->normal = scala_mul(unit_vector(cylinder->vec), -1);

	if (dot(ray->direction, rec->normal) < 0.0)
		rec->front_face = true;
	else
	{
		rec->front_face = false;
		rec->normal = scala_mul(rec->normal, -1);
	}
	rec->color = cylinder->color;
	rec->obj = obj;
//	rec->texture = NONE;
//	rec->texture = CHECKER;
	rec->texture = BUMP;
	return (true);
}

bool	hit_co(t_lst *obj, t_ray *ray, double t_max, t_hit_record *rec)
{
	t_cone		*cone;
	double		a;
	double		b;
	double		c;
	t_point3	vertex;
	double		cosine;
	t_vec3		cp;
	double		base;

	cone = (t_cone *)obj;
	base = hit_co_base(cone, ray, t_max, rec);
	cosine = cos(atan2(cone->diameter / 2.0, cone->height));
	vertex = vector_sub(cone->base_center, scala_mul(cone->vec, cone->height));

	a = dot(ray->direction, ray->direction) * pow(cosine, 2.0) - pow(dot(ray->direction, cone->vec), 2.0);
	
	b = 2 * dot(vector_sub(ray->origin, vertex), ray->direction) * pow(cosine, 2.0) \
	- 2 * dot(ray->direction, cone->vec) * dot(vector_sub(ray->origin, vertex), cone->vec);
	
	c = dot(vector_sub(ray->origin, vertex), vector_sub(ray->origin, vertex)) \
	* pow(cosine, 2.0) - pow(dot(vector_sub(ray->origin, vertex), cone->vec), 2.0);

	rec->t = get_root(a, b, c, t_max);
	cp = vector_add(vector_sub(ray->origin, vertex), scala_mul(ray->direction, rec->t));
	if (dot(cp, cone->vec) > cone->height || dot(unit_vector(cp), cone->vec) < 0.0)
		rec->t = -1;
	if (rec->t == -1 && base == -1)
		return (false);
	else if (base != -1 && base < rec->t)
	{
		rec->t = base;
		return (true);
	}
	rec->p = ray_at(ray, rec->t);
	rec->normal = unit_vector(vector_sub(cp, scala_mul(cone->vec, length(cp) / cosine)));
	if (dot(ray->direction, rec->normal) < 0.0)
		rec->front_face = true;
	else
	{
		rec->front_face = false;
		rec->normal = scala_mul(rec->normal, -1);
	}
	rec->color = cone->color;
	return (true);
}

double	hit_co_base(t_cone *cone, t_ray *ray, double t_max, t_hit_record *rec)
{
	double 		denom;
	double		nom;
	double		r;

	rec->normal = cone->vec;
	if (dot(rec->normal, ray->direction) > 0.0)
		rec->normal = scala_mul(rec->normal, -1);
	denom = dot(rec->normal, ray->direction);
	if (fabs(denom) > EPSILON)
	{
		nom = dot(vector_sub(cone->base_center, ray->origin), rec->normal);
		rec->t = nom / denom;
		rec->p = ray_at(ray, rec->t);
		rec->front_face = true;
		rec->color = cone->color;
		r = length(vector_sub(ray_at(ray, rec->t), cone->base_center));
		if (r > cone->diameter / 2.0)
			return (-1);
		if (rec->t > T_MIN && rec->t < t_max)
			return (rec->t);
	}
	return (-1);
}
