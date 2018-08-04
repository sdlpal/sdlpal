/* ############################################################################################

   Cartoon shader - Copyright (C) 2013 guest(r) - guest.r@gmail.com

   ############################################################################################

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
   ############################################################################################
   converted to GLSL by palxex 2018
   */
#if defined(VERTEX)
#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 TexCoord;

COMPAT_VARYING vec2 v_texCoord;

COMPAT_VARYING vec4 t1;
COMPAT_VARYING vec4 t2;
COMPAT_VARYING vec4 t3;
COMPAT_VARYING vec4 t4;
  
void main()
{
//VERTEX_STUFF0 PASS1_VERTEX (float3 p : POSITION, float2 tc : TEXCOORD0)
  
	float dx = 1.0/InputSize.x;
	float dy = 1.0/InputSize.y;

    gl_Position = MVPMatrix * VertexCoord;
    v_texCoord = TexCoord.xy;

	t1.xy = v_texCoord + vec2(-dx,  0);
	t2.xy = v_texCoord + vec2( dx,  0);
	t3.xy = v_texCoord + vec2(  0,-dy);
	t4.xy = v_texCoord + vec2(  0, dy);
	t1.zw = v_texCoord + vec2(-dx,-dy);
	t2.zw = v_texCoord + vec2(-dx, dy);
	t3.zw = v_texCoord + vec2( dx,-dy);
	t4.zw = v_texCoord + vec2( dx, dy);
}

#elif defined(FRAGMENT)
#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_VARYING vec2 v_texCoord;
COMPAT_VARYING vec4 t1;
COMPAT_VARYING vec4 t2;
COMPAT_VARYING vec4 t3;
COMPAT_VARYING vec4 t4;

uniform sampler2D tex0;

vec3  dt = vec3(1,1,1);

void main()
{
	vec3 c00 = COMPAT_TEXTURE(tex0, t1.zw).xyz; 
	vec3 c10 = COMPAT_TEXTURE(tex0, t3.xy).xyz; 
	vec3 c20 = COMPAT_TEXTURE(tex0, t3.zw).xyz; 
	vec3 c01 = COMPAT_TEXTURE(tex0, t1.xy).xyz; 
	vec3 c11 = COMPAT_TEXTURE(tex0, v_texCoord).xyz; 
	vec3 c21 = COMPAT_TEXTURE(tex0, t2.xy).xyz; 
	vec3 c02 = COMPAT_TEXTURE(tex0, t2.zw).xyz; 
	vec3 c12 = COMPAT_TEXTURE(tex0, t4.xy).xyz; 
	vec3 c22 = COMPAT_TEXTURE(tex0, t4.zw).xyz;

	float d1=dot(abs(c00-c22),dt)+0.001;
	float d2=dot(abs(c20-c02),dt)+0.001;
	float hl=dot(abs(c01-c21),dt)+0.001;
	float vl=dot(abs(c10-c12),dt)+0.001;

	float md = d1+d2;  float mc = hl+vl;
	hl*=  md;vl*= md;  d1*=  mc;d2*= mc;
	
	float ww = d1+d2+hl+vl;

	FragColor = vec4((hl*(c10+c12)+vl*(c01+c21)+d1*(c20+c02)+d2*(c00+c22)+ww*c11)/(3.0*ww),1);
}
#endif