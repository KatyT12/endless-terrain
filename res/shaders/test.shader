#shader vertex
#version 330 core
	    
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;


layout (location = 2) in vec3 aColor;

out vec3 ourColor;

//Defining uniform for our projection matrix
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 Normal;


void main()
{
    Normal = normal;
	gl_Position = proj * view * model * position; //Multiply the projection matrix by the position
	ourColor = aColor;
}
   



#shader fragment
#version 330 core

layout (location = 0) out vec4 color; 

in vec3 ourColor;
in vec3 Normal;

uniform vec4 u_Color;


void main()
{
	//Getting the color from our texture with the right coord and assigning the pixels on the screen to it
	color = u_Color; 
	color = vec4(ourColor,1.0f);
    color = vec4(Normal,1.0);
	
};
