/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   render.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: siyang <siyang@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/30 14:51:51 by siyang            #+#    #+#             */
/*   Updated: 2023/05/30 15:00:26 by siyang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minirt.h"

void	render(t_scene *scene, t_screen *screen)
{
	int			*pixel;
	int			x;
	int			y;
	int			i;
	double		u;
	double		v;
	t_ray		ray;
	t_color3	color;

	cam_init(scene);
	pixel = (int *)screen->img.addr;
	y = HEIGHT - 1;
	while (y >= 0)
	{
		x = 0;
		while (x < WIDTH)
		{
			i = 0;
			color = color3(0.0, 0.0, 0.0);
			while (i < SAMPLES)
			{
				u = (double)(x + random_double(i)) / (WIDTH - 1);
				v = (double)(HEIGHT - y - 1 + random_double(i)) / (HEIGHT - 1);
				ray = get_ray(scene->c, u, v);
				color = vector_add(color, ray_color(scene->obj_lst, &ray));
				i++;
			}
			*pixel = write_color(color);
			pixel = (int *)(screen->img.addr + (y * screen->img.line_size + \
				(x * (screen->img.bits_per_pixel / 8))));
			x++;
		}
		y--;
	}
}

int	write_color(t_color3 color)
{
	int res;
	int tmp[3];

	res = 0;
	tmp[0] = clamp(((color.x / SAMPLES) + 1.0) * 0.5, 0.0, 1.0) * 255.0;
	tmp[1] = clamp(((color.y / SAMPLES) + 1.0) * 0.5, 0.0, 1.0) * 255.0;
	tmp[2] = clamp(((color.z / SAMPLES) + 1.0) * 0.5, 0.0, 1.0) * 255.0;
	res += tmp[0] << 16;
	res += tmp[1] << 8;
	res += tmp[2];
	return (res);
}

void	cam_init(t_scene *scene)
{
	double theta;
	double h;
	double aspect_ratio;

	scene->c.focal_length = 1.0;
	aspect_ratio = WIDTH / HEIGHT;
	theta = degrees_to_radians(scene->c.fov);
	h = tan(theta / 2) * scene->c.focal_length;
	scene->c.viewport_w = 2.0 * h;
	scene->c.viewport_h = scene->c.viewport_w / aspect_ratio;
	scene->c.horizontal = vec3(scene->c.viewport_w, 0, 0);
	scene->c.vertical = vec3(0, scene->c.viewport_h, 0);
	scene->c.lower_left_corner = vector_sub(vector_sub(vector_sub(scene->c.coord, \
		scala_div(scene->c.horizontal, 2)), scala_div(scene->c.vertical, 2)), \
		vec3(0, 0, scene->c.focal_length));
}
