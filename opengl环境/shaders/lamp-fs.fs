/* 
  light.fragment.strings
  opengl环境

  Created by Stan on 2024/8/12.
  
*/
#version 330 core
out vec4 FragColor;
uniform vec3 lampColor;

void main()
{
    FragColor = vec4(lampColor, 1.0); // set all 4 vector values to 1.0
}
