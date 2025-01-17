// Generated file

// clang-format off
const char *g_detail_normalmap_shader = 
"#version 450\n"
"\n"
"// Takes signed distance values and computes a world-space normal from them.\n"
"// The normal's direction is then clamped based on triangles associated with each value.\n"
"// Then results are encoded in an output image.\n"
"\n"
"layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;\n"
"\n"
"layout (set = 0, binding = 0, std430) restrict readonly buffer SDBuffer {\n"
"	// 4 values per index\n"
"	float values[];\n"
"} u_in_sd;\n"
"\n"
"layout (set = 0, binding = 1, std430) restrict readonly buffer MeshVertices {\n"
"	vec3 data[];\n"
"} u_vertices;\n"
"\n"
"layout (set = 0, binding = 2, std430) restrict readonly buffer MeshIndices {\n"
"	int data[];\n"
"} u_indices;\n"
"\n"
"layout (set = 0, binding = 3, std430) restrict readonly buffer HitBuffer {\n"
"	// X, Y, Z is hit position (UNUSED)\n"
"	// W is integer triangle index\n"
"	vec4 positions[];\n"
"} u_hits;\n"
"\n"
"layout (set = 0, binding = 4, std430) restrict readonly buffer Params {\n"
"	int tile_size_pixels;\n"
"	int tiles_x;\n"
"	// cos(max_deviation_angle)\n"
"	float max_deviation_cosine;\n"
"	// sin(max_deviation_angle)\n"
"	float max_deviation_sine;\n"
"} u_params;\n"
"\n"
"layout (set = 0, binding = 5, rgba8ui) writeonly uniform uimage2D u_target_image;\n"
"\n"
"vec3 get_triangle_normal(vec3 v0, vec3 v1, vec3 v2) {\n"
"	vec3 e1 = v1 - v0;\n"
"	vec3 e2 = v2 - v0;\n"
"	return normalize(cross(e2, e1));\n"
"}\n"
"\n"
"mat3 basis_from_axis_angle_cs(vec3 p_axis, float cosine, float sine) {\n"
"	// Rotation matrix from axis and angle, see\n"
"	// https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_angle\n"
"\n"
"	mat3 cols;\n"
"\n"
"	const vec3 axis_sq = vec3(p_axis.x * p_axis.x, p_axis.y * p_axis.y, p_axis.z * p_axis.z);\n"
"	cols[0][0] = axis_sq.x + cosine * (1.0f - axis_sq.x);\n"
"	cols[1][1] = axis_sq.y + cosine * (1.0f - axis_sq.y);\n"
"	cols[2][2] = axis_sq.z + cosine * (1.0f - axis_sq.z);\n"
"\n"
"	const float t = 1 - cosine;\n"
"\n"
"	float xyzt = p_axis.x * p_axis.y * t;\n"
"	float zyxs = p_axis.z * sine;\n"
"	cols[1][0] = xyzt - zyxs;\n"
"	cols[0][1] = xyzt + zyxs;\n"
"\n"
"	xyzt = p_axis.x * p_axis.z * t;\n"
"	zyxs = p_axis.y * sine;\n"
"	cols[2][0] = xyzt + zyxs;\n"
"	cols[0][2] = xyzt - zyxs;\n"
"\n"
"	xyzt = p_axis.y * p_axis.z * t;\n"
"	zyxs = p_axis.x * sine;\n"
"	cols[2][1] = xyzt - zyxs;\n"
"	cols[1][2] = xyzt + zyxs;\n"
"\n"
"	return cols;\n"
"}\n"
"\n"
"vec3 rotate_vec3_cs(const vec3 v, const vec3 axis, float cosine, float sine) {\n"
"	return basis_from_axis_angle_cs(axis, cosine, sine) * v;\n"
"}\n"
"\n"
"void main() {\n"
"	const ivec2 pixel_pos_in_tile = ivec2(gl_GlobalInvocationID.xy);\n"
"	const int tile_index = int(gl_GlobalInvocationID.z);\n"
"\n"
"	const int index = pixel_pos_in_tile.x + pixel_pos_in_tile.y * u_params.tile_size_pixels \n"
"		+ tile_index * u_params.tile_size_pixels * u_params.tile_size_pixels;\n"
"\n"
"	const ivec2 pixel_pos = \n"
"		ivec2(tile_index % u_params.tiles_x, tile_index / u_params.tiles_x)\n"
"		* u_params.tile_size_pixels + pixel_pos_in_tile;\n"
"\n"
"	const int tri_index = int(u_hits.positions[index].w);\n"
"	if (tri_index == -1) {\n"
"		imageStore(u_target_image, pixel_pos, ivec4(127, 127, 127, 255));\n"
"		return;\n"
"	}\n"
"\n"
"	const int sdi = index * 4;\n"
"\n"
"	const float sd000 = u_in_sd.values[sdi];\n"
"	const float sd100 = u_in_sd.values[sdi + 1];\n"
"	const float sd010 = u_in_sd.values[sdi + 2];\n"
"	const float sd001 = u_in_sd.values[sdi + 3];\n"
"\n"
"	vec3 normal = normalize(vec3(sd100 - sd000, sd010 - sd000, sd001 - sd000));\n"
"\n"
"	const int ii0 = tri_index * 3;\n"
"\n"
"	const int i0 = u_indices.data[ii0];\n"
"	const int i1 = u_indices.data[ii0 + 1];\n"
"	const int i2 = u_indices.data[ii0 + 2];\n"
"\n"
"	const vec3 v0 = u_vertices.data[i0];\n"
"	const vec3 v1 = u_vertices.data[i1];\n"
"	const vec3 v2 = u_vertices.data[i2];\n"
"\n"
"	// In theory we could compute triangle normals once per triangle,\n"
"	// seems more efficient. But would it be significantly faster?\n"
"	const vec3 tri_normal = get_triangle_normal(v0, v1, v2);\n"
"\n"
"	// Clamp normal if it deviates too much\n"
"	const float tdot = dot(normal, tri_normal);\n"
"	if (tdot < u_params.max_deviation_cosine) {\n"
"		if (tdot < -0.999) {\n"
"			normal = tri_normal;\n"
"		} else {\n"
"			const vec3 axis = normalize(cross(tri_normal, normal));\n"
"			normal = rotate_vec3_cs(tri_normal, axis, \n"
"				u_params.max_deviation_cosine, \n"
"				u_params.max_deviation_sine);\n"
"		}\n"
"	}\n"
"\n"
"	const ivec4 col = ivec4(255.0 * (vec3(0.5) + 0.5 * normal), 255);\n"
"\n"
"	imageStore(u_target_image, pixel_pos, col);\n"
"}\n"
"\n";
// clang-format on
