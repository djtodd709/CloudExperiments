#ifndef SHADER_HPP
#define SHADER_HPP

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path, const char* tessC_file_path = NULL, const char* tessE_file_path = NULL );
GLuint LoadComputeShader(const char* file_path);

#endif
