// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#version 450 core

uniform nap
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 modelMatrix;
	//mat4 normalMatrix;
	//vec3 cameraPosition;
} mvp;

uniform UBO
{
	vec3 color;
	float alpha;
} ubo;

in vec4	in_Position;
in vec4 in_Normals;
in vec4	in_UV0;
in vec4 in_Color0;

// out vec3 pass_Uvs;
out vec4 pass_Color;


void main(void)
{
	// Calculate position
    gl_Position = mvp.projectionMatrix * mvp.viewMatrix * mvp.modelMatrix * in_Position;

	// Pass color
	pass_Color = in_Color0;
}