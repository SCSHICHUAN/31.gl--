/* 
 model.fs
  opengl环境

  Created by Stan on 2024/9/22.
  
*/
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
}
